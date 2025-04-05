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
#include "menuCmdID.h"
#include "Parameters.h"
#include "resource.h"
#include "localization.h"

using namespace std;

#define GET_X_LPARAM(lp) static_cast<short>(LOWORD(lp))
#define GET_Y_LPARAM(lp) static_cast<short>(HIWORD(lp))

#define CLMNEXT_ID     1
#define CLMNPATH_ID    2
#define SEP_POS        3
#define LVGROUPS_ID    4

COLORREF VerticalFileSwitcher::_bgColor = 0xFFFFFF;

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	sortCompareData* sortData = (sortCompareData*)lParamSort;
	wchar_t str1[MAX_PATH] = { '\0' };
	wchar_t str2[MAX_PATH] = { '\0' };

	ListView_GetItemText(sortData->hListView, lParam1, sortData->columnIndex, str1, sizeof(str1));
	ListView_GetItemText(sortData->hListView, lParam2, sortData->columnIndex, str2, sizeof(str2));

	int result = lstrcmp(str1, str2);

	if (sortData->sortDirection == SORT_DIRECTION_UP)
		return result;

	return (0 - result);
}

LRESULT run_listViewProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

		case WM_MBUTTONUP:
		{
			// Redirect the message to parent
			::SendMessage(::GetParent(hwnd), WM_PARENTNOTIFY, WM_MBUTTONUP, lParam);
			return TRUE;
		}

		default:
			break;
	}
	return ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
}

void VerticalFileSwitcher::startColumnSort()
{
	// reset sorting if exts column was just disabled
	HWND colHeader = reinterpret_cast<HWND>(SendMessage(_fileListView.getHSelf(), LVM_GETHEADER, 0, 0));
	int columnCount = static_cast<int32_t>(SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));
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

				HBRUSH hBrush = ::CreateSolidBrush(VerticalFileSwitcher::_bgColor);
				::FillRect(lplvcd->nmcd.hdc, &rcHeader, hBrush);
				::DeleteObject(hBrush);
				hBrush = nullptr;
			}
			return CDRF_NOTIFYITEMDRAW;
		}

		case CDDS_ITEMPREPAINT:
		{
			const RECT& rcRow = lplvcd->nmcd.rc;

			const bool isThemeDark = NppDarkMode::isThemeDark();

			const auto hHeader = ListView_GetHeader(lplvcd->nmcd.hdr.hwndFrom);
			const auto colCount = Header_GetItemCount(hHeader);

			const LONG paddingLeft = isThemeDark ? 1 : 0;
			const LONG paddingRight = isThemeDark ? 2 : 1;

			RECT rcSubItem{ rcRow };
			RECT rcSubItem2{};
			RECT rcSubItem3{};

			rcSubItem.right -= paddingRight;

			auto setRectForSubItem = [hHeader, paddingLeft, paddingRight](RECT& first, RECT& second, int idxSecond) -> void {
				Header_GetItemRect(hHeader, idxSecond, &second);
				first.right = second.left - paddingRight;

				second.left -= paddingLeft;
				second.right -= paddingRight;
				second.top = first.top;
				second.bottom = first.bottom;
			};

			if (colCount >= 2)
			{
				setRectForSubItem(rcSubItem, rcSubItem2, 1);
			}

			if (colCount == 3)
			{
				setRectForSubItem(rcSubItem2, rcSubItem3, 2);
			}

			const auto isSelected = ListView_GetItemState(lplvcd->nmcd.hdr.hwndFrom, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;
			const bool isHot = (lplvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT;
			const int colorID = reinterpret_cast<TaskLstFnStatus*>(lplvcd->nmcd.lItemlParam)->_docColor;

			COLORREF bgColor{0xFFFFFF};
			bool applyColor = false;

			if (colorID != -1)
			{
				bgColor = NppParameters::getInstance().getIndividualTabColor(colorID, isThemeDark, false);
				applyColor = true;
			}
			else if (isThemeDark)
			{
				if (isSelected)
				{
					bgColor = NppDarkMode::getCtrlBackgroundColor();
					applyColor = true;
				}
				else if (isHot)
				{
					bgColor = NppDarkMode::getHotBackgroundColor();
					applyColor = true;
				}
			}

			if (applyColor)
			{
				if (isThemeDark)
				{
					lplvcd->clrText = NppDarkMode::getTextColor();
				}

				lplvcd->clrTextBk = bgColor;

				HBRUSH hBrush = ::CreateSolidBrush(bgColor);

				::FillRect(lplvcd->nmcd.hdc, &rcSubItem, hBrush);
				if (colCount >= 2)
				{
					::FillRect(lplvcd->nmcd.hdc, &rcSubItem2, hBrush);
				}

				if (colCount == 3)
				{
					::FillRect(lplvcd->nmcd.hdc, &rcSubItem3, hBrush);
				}

				::DeleteObject(hBrush);
				hBrush = nullptr;
			}

			if (isSelected)
			{
				::DrawFocusRect(lplvcd->nmcd.hdc, &rcRow);
			} 
			else if (isHot)
			{
				::FrameRect(lplvcd->nmcd.hdc, &rcRow, isThemeDark ? NppDarkMode::getHotEdgeBrush() : ::GetSysColorBrush(COLOR_WINDOWTEXT));
			}

			return CDRF_NEWFONT;
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
	DWORD_PTR /*dwRefData*/
)
{
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
					constexpr size_t classNameLen = 16;
					wchar_t className[classNameLen]{};
					GetClassName(nmhdr->hwndFrom, className, classNameLen);

					if (wcscmp(className, WC_LISTVIEW) == 0)
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

void VerticalFileSwitcher::autoSubclassWindowNotify(HWND hParent)
{
	::SetWindowSubclass(hParent, VerticalFileSwitcher::FileSwitcherNotifySubclass, _fileSwitcherNotifySubclassID, 0);
}

intptr_t CALLBACK VerticalFileSwitcher::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			VerticalFileSwitcher::initPopupMenus();

			_fileListView.init(_hInst, _hSelf, _hImaLst);
			_fileListView.initList();
			_fileListView.display();

			::SetWindowLongPtr(_fileListView.getHSelf(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_defaultListViewProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_fileListView.getHSelf(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(listViewStaticProc)));

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			VerticalFileSwitcher::autoSubclassWindowNotify(_hSelf);

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
			switch ( wParam )
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
						TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

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
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int i = lpnmitem->iItem;
					if (i == -1)
					{
						::SendMessage(_hParent, WM_COMMAND, IDM_FILE_NEW, 0);
					}
					return TRUE;
				}

				case NM_CLICK:
				{
					if ((0x80 & GetKeyState(VK_CONTROL)) || (0x80 & GetKeyState(VK_SHIFT)))
						return TRUE;

					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
					int nbItem = ListView_GetItemCount(_fileListView.getHSelf());
					int i = lpnmitem->iItem;
					if (i == -1 || i >= nbItem)
						return TRUE;

					LVITEM item{};
					item.mask = LVIF_PARAM;
					item.iItem = i;
					ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
					TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

					activateDoc(tlfs);
					return TRUE;
				}

				case NM_RCLICK :
				{
					// Switch to the right document
					LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;

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
						ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
						TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

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
					wstring fn = getFullFilePath((size_t)i);
					lstrcpyn(pGetInfoTip->pszText, fn.c_str(), pGetInfoTip->cchTextMax);
					return TRUE;
				}

				case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmLV = (LPNMLISTVIEW)lParam;
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
					
					LPNMHEADER test = (LPNMHEADER)lParam;
					HWND hwndHD = ListView_GetHeader(_fileListView.getHSelf());
					wchar_t HDtext[MAX_PATH] = { '\0' };
					HDITEM hdi = {};
					hdi.mask = HDI_TEXT | HDI_WIDTH;
					hdi.pszText = HDtext;
					hdi.cchTextMax = MAX_PATH;
					Header_GetItem(hwndHD, test->iItem, &hdi);

					// storing column width data
					if (hdi.pszText == pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT))
						nppParams.getNppGUI()._fileSwitcherExtWidth = hdi.cxy;
					else if (hdi.pszText == pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH))
						nppParams.getNppGUI()._fileSwitcherPathWidth = hdi.cxy;

					return TRUE;
				}
				case LVN_KEYDOWN:
				{
					switch (((LPNMLVKEYDOWN)lParam)->wVKey)
					{
						case VK_RETURN:
						{
							int i = ListView_GetSelectionMark(_fileListView.getHSelf());
							if (i == -1)
								return TRUE;

							LVITEM item{};
							item.mask = LVIF_PARAM;
							item.iItem = i;	
							ListView_GetItem(((LPNMHDR)lParam)->hwndFrom, &item);
							TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
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

	wstring extStr = pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT);
	wstring pathStr = pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH);
	wstring groupStr = pNativeSpeaker->getAttrNameStr(L"Group by View", FS_ROOTNODE, FS_LVGROUPS);

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
	
	int docPosInfo = static_cast<int32_t>(::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, reinterpret_cast<WPARAM>(bufferID), view));
	int view2set = docPosInfo >> 30;
	int index2Switch = (docPosInfo << 2) >> 2;

	::SendMessage(_hParent, NPPM_ACTIVATEDOC, view2set, index2Switch);
}

void VerticalFileSwitcher::closeDoc(TaskLstFnStatus *tlfs) const
{
	int view = tlfs->_iView;
	BufferID bufferID = static_cast<BufferID>(tlfs->_bufID);
		
	int docPosInfo = static_cast<int32_t>(::SendMessage(_hParent, NPPM_GETPOSFROMBUFFERID, reinterpret_cast<WPARAM>(bufferID), view));
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
		int columnCount = static_cast<int32_t>(SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));
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
