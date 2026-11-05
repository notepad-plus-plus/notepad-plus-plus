// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "VerticalFileSwitcher.h"

#include <windows.h>

#include <commctrl.h>
#include <windowsx.h>

#include <string>

#include "Buffer.h"
#include "DockingDlgInterface.h"
#include "Notepad_plus_msgs.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "TaskListDlg.h"
#include "VerticalFileSwitcherListView.h"
#include "dockingResource.h"
#include "localization.h"
#include "menuCmdID.h"
#include "resource.h"
#include <dpiManagerV2.h>

enum VFSLstMenuItemId
{
	CLMNEXT_ID = 1,
	CLMNPATH_ID,
	SEP_POS,
	LVGROUPS_ID
};

COLORREF VerticalFileSwitcher::_bgColor = 0xFFFFFF;

static int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const sortCompareData* sortData = reinterpret_cast<sortCompareData*>(lParamSort);
	wchar_t str1[MAX_PATH] = { '\0' };
	wchar_t str2[MAX_PATH] = { '\0' };

	ListView_GetItemText(sortData->hListView, lParam1, sortData->columnIndex, str1, sizeof(str1));
	ListView_GetItemText(sortData->hListView, lParam2, sortData->columnIndex, str2, sizeof(str2));

	int result = lstrcmp(str1, str2);

	if (sortData->sortDirection == SORT_DIRECTION_UP)
		return result;

	return (0 - result);
}

LRESULT CALLBACK VerticalFileSwitcher::ListViewParentNotifyProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	[[maybe_unused]] DWORD_PTR dwRefData
)
{
	switch (uMsg)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, VerticalFileSwitcher::ListViewParentNotifyProc, uIdSubclass);
			break;
		}

		case WM_MBUTTONUP:
		{
			// Redirect the message to parent
			::SendMessage(::GetParent(hWnd), WM_PARENTNOTIFY, WM_MBUTTONUP, lParam);
			return 0;
		}

		default:
			break;
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void VerticalFileSwitcher::startColumnSort()
{
	// reset sorting if exts column was just disabled
	HWND colHeader = reinterpret_cast<HWND>(SendMessage(_fileListView.getHSelf(), LVM_GETHEADER, 0, 0));
	const auto columnCount = static_cast<int>(SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));
	if (_lastSortingColumn >= columnCount)
	{
		_lastSortingColumn = 0;
		_lastSortingDirection = SORT_DIRECTION_NONE;
	}

	if (_lastSortingDirection != SORT_DIRECTION_NONE)
	{
		sortCompareData sortData = {_fileListView.getHSelf(), _lastSortingColumn, _lastSortingDirection};
		ListView_SortItemsEx(_fileListView.getHSelf(), ListViewCompareProc, reinterpret_cast<LPARAM>(&sortData));
	}
	
	updateHeaderArrow();
}

LRESULT VerticalFileSwitcher::listViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

	switch (lplvcd->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
		{
			if ((lplvcd->dwItemType == LVCDI_GROUP) && NppDarkMode::isThemeDark())
			{
				RECT rcHeader{};
				ListView_GetGroupRect(lplvcd->nmcd.hdr.hwndFrom, lplvcd->nmcd.dwItemSpec, LVGGR_HEADER, &rcHeader);

				const auto oldClr = ::SetDCBrushColor(lplvcd->nmcd.hdc, VerticalFileSwitcher::_bgColor);
				::FillRect(lplvcd->nmcd.hdc, &rcHeader, static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
				::SetDCBrushColor(lplvcd->nmcd.hdc, oldClr);
			}
			return CDRF_NOTIFYITEMDRAW;
		}

		case CDDS_ITEMPREPAINT:
		{
			const HWND hList = lplvcd->nmcd.hdr.hwndFrom;
			const RECT& rcRow = lplvcd->nmcd.rc;

			const bool isThemeDark = NppDarkMode::isThemeDark();
			const bool isSelected = ListView_GetItemState(hList, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;
			const bool isHot = (lplvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT;

			const int colorID = reinterpret_cast<TaskLstFnStatus*>(lplvcd->nmcd.lItemlParam)->_docColor;

			COLORREF bgColor = 0xFFFFFF;
			bool applyColor = false;

			if (colorID != -1)
			{
				bgColor = NppParameters::getInstance().getIndividualTabColor(colorID, isThemeDark, false);
				applyColor = true;
			}
			else if (isThemeDark && (isSelected || isHot))
			{
				bgColor = isSelected ? NppDarkMode::getCtrlBackgroundColor() : NppDarkMode::getHotBackgroundColor();
				applyColor = true;
			}

			if (applyColor)
			{
				if (isThemeDark)
				{
					lplvcd->clrText = NppDarkMode::getTextColor();
				}

				lplvcd->clrTextBk = bgColor;

				const auto oldClr = ::SetDCBrushColor(lplvcd->nmcd.hdc, bgColor);
				const auto hBrush = static_cast<HBRUSH>(::GetStockObject(DC_BRUSH));

				const HWND hHeader = ListView_GetHeader(hList);
				const int nCol = Header_GetItemCount(hHeader);

				const LONG paddingLeft = isThemeDark ? 1 : 0;
				const LONG paddingRight = isThemeDark ? 2 : 1;

				const auto lvii = LVITEMINDEX{ .iItem = static_cast<int>(lplvcd->nmcd.dwItemSpec), .iGroup = -1 };
				RECT rcSubitem{
					rcRow.left
					, rcRow.top
					, rcRow.left + ListView_GetColumnWidth(hList, 0) - paddingRight
					, rcRow.bottom
				};
				::FillRect(lplvcd->nmcd.hdc, &rcSubitem, hBrush);

				for (int i = 1; i < nCol; ++i)
				{
					ListView_GetItemIndexRect(hList, &lvii, i, LVIR_BOUNDS, &rcSubitem);
					rcSubitem.left -= paddingLeft;
					rcSubitem.right -= paddingRight;
					::FillRect(lplvcd->nmcd.hdc, &rcSubitem, hBrush);
				}

				::SetDCBrushColor(lplvcd->nmcd.hdc, oldClr);
			}

			if (isSelected)
			{
				::DrawFocusRect(lplvcd->nmcd.hdc, &rcRow);
			}
			else if (isHot)
			{
				::FrameRect(lplvcd->nmcd.hdc, &rcRow, isThemeDark ? NppDarkMode::getHotEdgeBrush() : ::GetSysColorBrush(COLOR_WINDOWTEXT));
			}

			return CDRF_DODEFAULT;
		}

		default:
			break;
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK VerticalFileSwitcher::FileSwitcherNotifySubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto vfs = reinterpret_cast<VerticalFileSwitcher*>(dwRefData);

	switch (uMsg)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, VerticalFileSwitcher::FileSwitcherNotifySubclass, uIdSubclass);
			break;
		}

		case WM_NOTIFY:
		{
			auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			switch (nmhdr->code)
			{
				case NM_CUSTOMDRAW:
				{
					if (nmhdr->hwndFrom == vfs->_fileListView.getHSelf())
					{
						return VerticalFileSwitcher::listViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam);
					}
					break;
				}
			}
			break;
		}
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

intptr_t CALLBACK VerticalFileSwitcher::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			VerticalFileSwitcher::initPopupMenus();

			_fileListView.init(_hInst, _hSelf, _hImaLst);
			_fileListView.initList();
			_fileListView.display();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			::SetWindowSubclass(_hSelf, VerticalFileSwitcher::FileSwitcherNotifySubclass, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));
			::SetWindowSubclass(_fileListView.getHSelf(), VerticalFileSwitcher::ListViewParentNotifyProc, static_cast<UINT_PTR>(SubclassID::first), 0);

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		// Different from WM_MBUTTONDOWN, WM_MBUTTONUP message is not sent to parent hwnd by WIN32 API
		// So we subclass listview to redirect WM_MBUTTONUP via WM_PARENTNOTIFY (as WM_MBUTTONDOWN)
		case WM_PARENTNOTIFY:
		{
			switch (wParam)
			{
				case WM_MBUTTONUP:
				{
					// Get item ID under cursor
					LVHITTESTINFO hitInfo{};
					hitInfo.pt.x = GET_X_LPARAM(lParam);
					hitInfo.pt.y = GET_Y_LPARAM(lParam);

					::ClientToScreen(getHSelf(), &hitInfo.pt);
					::ScreenToClient(_fileListView.getHSelf(), &hitInfo.pt);
					ListView_HitTest(_fileListView.getHSelf(), &hitInfo);
			
					if (hitInfo.iItem != -1)
					{
						// Get the actual item info from the ID
						LVITEM item{};
						item.mask = LVIF_PARAM;
						item.iItem = hitInfo.iItem;
						ListView_GetItem(_fileListView.getHSelf(), &item);
						auto* tlfs = reinterpret_cast<TaskLstFnStatus*>(item.lParam);

						// Close the document
						closeDoc(tlfs);

						return TRUE;
					}
				}
			}

			break;
		}


		case WM_NOTIFY:
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
				case DMN_CLOSE:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DOCLIST, 0);
					return TRUE;
				}

				case NM_DBLCLK:
				{
					const auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
					int i = lpnmitem->iItem;
					if (i == -1)
					{
						::SendMessage(_hParent, WM_COMMAND, IDM_FILE_NEW, 0);
					}
					return TRUE;
				}

				case NM_CLICK:
				{
					if (::GetKeyState(VK_CONTROL) < 0 || ::GetKeyState(VK_SHIFT) < 0)
						return TRUE;

					const auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
					int nbItem = ListView_GetItemCount(_fileListView.getHSelf());
					int i = lpnmitem->iItem;
					if (i == -1 || i >= nbItem)
						return TRUE;

					LVITEM item{};
					item.mask = LVIF_PARAM;
					item.iItem = i;
					ListView_GetItem(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom, &item);
					auto tlfs = reinterpret_cast<TaskLstFnStatus*>(item.lParam);

					activateDoc(tlfs);
					return TRUE;
				}

				case NM_RCLICK :
				{
					// Switch to the right document
					const auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

					if (lpnmitem->hdr.hwndFrom != _fileListView.getHSelf())
					{
						colHeaderRClick = true;
						return TRUE;
					}

					int nbItem = ListView_GetItemCount(_fileListView.getHSelf());

					if (nbSelectedFiles() == 1)
					{
						int i = lpnmitem->iItem;
						if (i == -1 || i >= nbItem)
 							return TRUE;

						LVITEM item{};
						item.mask = LVIF_PARAM;
						item.iItem = i;
						ListView_GetItem(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom, &item);
						auto tlfs = reinterpret_cast<TaskLstFnStatus*>(item.lParam);

						activateDoc(tlfs);
					}

					if (nbSelectedFiles() >= 1)
					{
						// Redirect NM_RCLICK message to Notepad_plus handle
						NMHDR nmhdr{};
						nmhdr.code = reinterpret_cast<LPNMHDR>(lParam)->code; //NM_RCLICK
						nmhdr.hwndFrom = _hSelf;
						nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
						::SendMessage(_hParent, WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
					}
					return TRUE;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP pGetInfoTip = (LPNMLVGETINFOTIP)lParam;
					int i = pGetInfoTip->iItem;
					if (i == -1)
						return TRUE;
					std::wstring fn = getFullFilePath(static_cast<size_t>(i));
					lstrcpyn(pGetInfoTip->pszText, fn.c_str(), pGetInfoTip->cchTextMax);
					return TRUE;
				}

				case LVN_COLUMNCLICK:
				{
					const auto pnmLV = reinterpret_cast<LPNMLISTVIEW>(lParam);
					_lastSortingDirection = setHeaderOrder(pnmLV->iSubItem);
					_lastSortingColumn = pnmLV->iSubItem;
					if (_lastSortingDirection != SORT_DIRECTION_NONE)
					{
						startColumnSort();
					}
					else
					{
						_fileListView.reload();
						updateHeaderArrow();
					}
					return TRUE;
				}
				case HDN_DIVIDERDBLCLICK:
				case HDN_ENDTRACK:
				{
					NppParameters& nppParams = NppParameters::getInstance();
					NativeLangSpeaker* pNativeSpeaker = nppParams.getNativeLangSpeaker();
					
					const auto test = reinterpret_cast<LPNMHEADERW>(lParam);
					HWND hwndHD = ListView_GetHeader(_fileListView.getHSelf());
					wchar_t HDtext[MAX_PATH] = { '\0' };
					HDITEM hdi = {};
					hdi.mask = HDI_TEXT | HDI_WIDTH;
					hdi.pszText = HDtext;
					hdi.cchTextMax = MAX_PATH;
					Header_GetItem(hwndHD, test->iItem, &hdi);

					// storing column width data
					if (hdi.pszText == pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT))
						nppParams.getNppGUI()._fileSwitcherExtWidth = _dpiManager.unscale(hdi.cxy);
					else if (hdi.pszText == pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH))
						nppParams.getNppGUI()._fileSwitcherPathWidth = _dpiManager.unscale(hdi.cxy);

					return TRUE;
				}
				case LVN_KEYDOWN:
				{
					switch (reinterpret_cast<LPNMLVKEYDOWN>(lParam)->wVKey)
					{
						case VK_RETURN:
						{
							int i = ListView_GetSelectionMark(_fileListView.getHSelf());
							if (i == -1)
								return TRUE;

							LVITEM item{};
							item.mask = LVIF_PARAM;
							item.iItem = i;	
							ListView_GetItem(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom, &item);
							auto tlfs = reinterpret_cast<TaskLstFnStatus*>(item.lParam);
							activateDoc(tlfs);
							return TRUE;
						}
						default:
							break;
					}
				}
				break;

				default:
					break;
			}
		}
		return TRUE;

        case WM_SIZE:
        {
			int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			::MoveWindow(_fileListView.getHSelf(), 0, 0, width, height, TRUE);
			_fileListView.resizeColumns(width);
            break;
        }
        
		case WM_CONTEXTMENU:
		{
			if (nbSelectedFiles() == 0 || colHeaderRClick)
			{
				::TrackPopupMenu(_hGlobalMenu, 
					NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
					GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, _hSelf, NULL);
				colHeaderRClick = false;
			}
			return TRUE;
		}

		case WM_COMMAND:
		{
			popupMenuCmd(LOWORD(wParam));
			break;
		}

		case WM_DESTROY:
        {
			_fileListView.destroy();
			::DestroyMenu(_hGlobalMenu);
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void VerticalFileSwitcher::initPopupMenus()
{
	NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();

	std::wstring extStr = pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT);
	std::wstring pathStr = pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH);
	std::wstring groupStr = pNativeSpeaker->getAttrNameStr(L"Group by View", FS_ROOTNODE, FS_LVGROUPS);

	_hGlobalMenu = ::CreatePopupMenu();
	::InsertMenu(_hGlobalMenu, CLMNEXT_ID, MF_BYCOMMAND | MF_STRING, CLMNEXT_ID, extStr.c_str());
	::InsertMenu(_hGlobalMenu, CLMNPATH_ID, MF_BYCOMMAND | MF_STRING, CLMNPATH_ID, pathStr.c_str());
	::InsertMenu(_hGlobalMenu, SEP_POS, MF_BYCOMMAND | MF_SEPARATOR, 0, nullptr);
	::InsertMenu(_hGlobalMenu, LVGROUPS_ID, MF_BYCOMMAND | MF_STRING, LVGROUPS_ID, groupStr.c_str());

	bool isExtColumn = nppGUI._fileSwitcherWithoutExtColumn;
	::CheckMenuItem(_hGlobalMenu, CLMNEXT_ID, MF_BYCOMMAND | (isExtColumn ? MF_UNCHECKED : MF_CHECKED));
	bool isPathColumn = nppGUI._fileSwitcherWithoutPathColumn;
	::CheckMenuItem(_hGlobalMenu, CLMNPATH_ID, MF_BYCOMMAND | (isPathColumn ? MF_UNCHECKED : MF_CHECKED));
	bool isListViewGroups = nppGUI._fileSwitcherDisableListViewGroups;
	::CheckMenuItem(_hGlobalMenu, LVGROUPS_ID, MF_BYCOMMAND | (isListViewGroups ? MF_UNCHECKED : MF_CHECKED));
}

void VerticalFileSwitcher::popupMenuCmd(int cmdID)
{
	switch (cmdID)
	{
		case CLMNEXT_ID:
		{
			bool& isExtColumn = NppParameters::getInstance().getNppGUI()._fileSwitcherWithoutExtColumn;
			isExtColumn = !isExtColumn;
			::CheckMenuItem(_hGlobalMenu, CLMNEXT_ID, MF_BYCOMMAND | (isExtColumn ? MF_UNCHECKED : MF_CHECKED));
			reload();
		}
		break;
		case CLMNPATH_ID:
		{
			bool& isPathColumn = NppParameters::getInstance().getNppGUI()._fileSwitcherWithoutPathColumn;
			isPathColumn = !isPathColumn;
			::CheckMenuItem(_hGlobalMenu, CLMNPATH_ID, MF_BYCOMMAND | (isPathColumn ? MF_UNCHECKED : MF_CHECKED));
			reload();
		}
		break;
		case LVGROUPS_ID:
		{
			bool& isListViewGroups = NppParameters::getInstance().getNppGUI()._fileSwitcherDisableListViewGroups;
			isListViewGroups = !isListViewGroups;
			::CheckMenuItem(_hGlobalMenu, LVGROUPS_ID, MF_BYCOMMAND | (isListViewGroups ? MF_UNCHECKED : MF_CHECKED));
			reload();
		}
		break;
	}
}

void VerticalFileSwitcher::display(bool toShow) const
{
	DockingDlgInterface::display(toShow);
	_fileListView.ensureVisibleCurrentItem();	// without this call the current item may stay above visible area after the program startup
}

void VerticalFileSwitcher::activateDoc(TaskLstFnStatus *tlfs) const
{
	int view = tlfs->_iView;
	BufferID bufferID = static_cast<BufferID>(tlfs->_bufID);
	
	auto currentView = ::SendMessage(_hParent, NPPM_GETCURRENTVIEW, 0, 0);
	BufferID currentBufID = reinterpret_cast<BufferID>(::SendMessage(_hParent, NPPM_GETCURRENTBUFFERID, 0, 0));

	if (bufferID == currentBufID && view == currentView)
		return;
	
	const auto docPosInfo = static_cast<int>(::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, reinterpret_cast<WPARAM>(bufferID), view));
	int view2set = docPosInfo >> 30;
	int index2Switch = (docPosInfo << 2) >> 2;

	::SendMessage(_hParent, NPPM_ACTIVATEDOC, view2set, index2Switch);
}

void VerticalFileSwitcher::closeDoc(TaskLstFnStatus *tlfs) const
{
	int view = tlfs->_iView;
	BufferID bufferID = static_cast<BufferID>(tlfs->_bufID);
		
	const auto docPosInfo = static_cast<int>(::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, reinterpret_cast<WPARAM>(bufferID), view));
	int view2set = docPosInfo >> 30;
	int index2Switch = (docPosInfo << 2) >> 2;

	::SendMessage(_hParent, NPPM_INTERNAL_CLOSEDOC, view2set, index2Switch);
}

int VerticalFileSwitcher::setHeaderOrder(int columnIndex)
{
	HWND hListView = _fileListView.getHSelf();
	LVCOLUMN lvc{};
	lvc.mask = LVCF_FMT;
	
	//strip HDF_SORTUP and HDF_SORTDOWN from old sort column
	if (_lastSortingColumn != columnIndex && _lastSortingDirection != SORT_DIRECTION_NONE)
	{
		HWND colHeader = reinterpret_cast<HWND>(SendMessage(hListView, LVM_GETHEADER, 0, 0));
		const auto columnCount = static_cast<int>(::SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));
		if (_lastSortingColumn < columnCount)
		{
			// Get current fmt
			SendMessage(hListView, LVM_GETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
			
			// remove both sort-up and sort-down
			lvc.fmt = lvc.fmt & (~HDF_SORTUP) & (~HDF_SORTDOWN);
			SendMessage(hListView, LVM_SETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
		}
		
		_lastSortingDirection = SORT_DIRECTION_NONE;
	}
	
	if (_lastSortingDirection == SORT_DIRECTION_NONE)
	{
		return SORT_DIRECTION_UP;
	}
	
	if (_lastSortingDirection == SORT_DIRECTION_UP)
	{
		return SORT_DIRECTION_DOWN;
	}

	//if (_lastSortingDirection == SORT_DIRECTION_DOWN)
	return SORT_DIRECTION_NONE;
}

void VerticalFileSwitcher::updateHeaderArrow()
{
	HWND hListView = _fileListView.getHSelf();
	LVCOLUMN lvc{};
	lvc.mask = LVCF_FMT;
	
	SendMessage(hListView, LVM_GETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
	
	if (_lastSortingDirection == SORT_DIRECTION_UP)
	{
		lvc.fmt = (lvc.fmt | HDF_SORTUP) & ~HDF_SORTDOWN;
		SendMessage(hListView, LVM_SETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
	}
	else if (_lastSortingDirection == SORT_DIRECTION_DOWN)
	{
		lvc.fmt = (lvc.fmt & ~HDF_SORTUP) | HDF_SORTDOWN;
		SendMessage(hListView, LVM_SETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
	}
	else if (_lastSortingDirection == SORT_DIRECTION_NONE)
	{
		lvc.fmt = lvc.fmt & (~HDF_SORTUP) & (~HDF_SORTDOWN);
		SendMessage(hListView, LVM_SETCOLUMN, _lastSortingColumn, reinterpret_cast<LPARAM>(&lvc));
	}
}
