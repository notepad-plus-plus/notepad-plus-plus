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


#include "VerticalFileSwitcherListView.h"

#include <windows.h>

#include <commctrl.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Buffer.h"
#include "Notepad_plus_msgs.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "TaskListDlg.h"
#include "Window.h"
#include "dpiManagerV2.h"
#include "localization.h"

static constexpr int kViewsLimit = 2; // only 2 views max can have same buffer

void VerticalFileSwitcherListView::init(HINSTANCE hInst, HWND hParent, UINT_PTR listViewID, HIMAGELIST hImaLst)
{
	Window::init(hInst, hParent);
	_hImaLst = hImaLst;
	INITCOMMONCONTROLSEX icex{};

	// Ensure that the common control DLL is loaded. 
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Create the list-view window in report view with label editing enabled.
	static constexpr DWORD listViewStyles = LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;

	_hSelf = ::CreateWindowExW(0, WC_LISTVIEW,
                                L"",
                                WS_CHILD | WS_BORDER | listViewStyles,
                                0,
                                0,
                                0,
                                0,
                                _hParent,
                                listViewID == 0 ? nullptr : reinterpret_cast<HMENU>(listViewID),
                                hInst,
                                nullptr);
	if (!_hSelf)
	{
		throw std::runtime_error("VerticalFileSwitcherListView::init : CreateWindowEx() function return null");
	}

	NppDarkMode::subclassListViewControl(_hSelf);

	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	ListView_SetImageList(_hSelf, _hImaLst, LVSIL_SMALL);

	LVGROUP group{};
	constexpr size_t headerLen = 1;
	wchar_t header[headerLen] = L"";
	group.cbSize = sizeof(LVGROUP);
	group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	group.pszHeader = header;
	group.cchHeader = headerLen;
	group.iGroupId = MAIN_VIEW;
	group.state = LVGS_COLLAPSIBLE;
	ListView_InsertGroup(_hSelf, -1, &group);

	group.iGroupId = SUB_VIEW;
	ListView_InsertGroup(_hSelf, -1, &group);

	setDarkTheme();
}

void VerticalFileSwitcherListView::destroy()
{
	LVITEM item{};
	item.mask = LVIF_PARAM;
	int nbItem = ListView_GetItemCount(_hSelf);
	for (int i = 0 ; i < nbItem ; ++i)
	{
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		auto* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
		delete odi;
	}
	::DestroyWindow(_hSelf);
	_hSelf = nullptr;
} 

void VerticalFileSwitcherListView::reSizeToWH(int width, int height)
{
	::SetWindowPos(_hSelf, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE); // SWP_NOMOVE must not be used due to RTL
	resizeColumns(width);
}

void VerticalFileSwitcherListView::initList()
{
	NppParameters& nppParams = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();

	const bool isListViewGroups = !nppParams.getNppGUI()._fileSwitcherDisableListViewGroups;
	ListView_EnableGroupView(_hSelf, isListViewGroups ? TRUE : FALSE);

	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !nppParams.getNppGUI()._fileSwitcherWithoutPathColumn;

	const UINT dpi = DPIManagerV2::getDpiForWindow(_hParent);
	const int extWidthDyn = DPIManagerV2::scale(nppParams.getNppGUI()._fileSwitcherExtWidth, dpi);
	const int pathWidthDyn = DPIManagerV2::scale(nppParams.getNppGUI()._fileSwitcherPathWidth, dpi);

	RECT rc{};
	::GetClientRect(_hParent, &rc);
	int nameWidth = rc.right - rc.left;
	int colIndex = 0;
	if (isExtColumn)
		nameWidth -= extWidthDyn;
	if (isPathColumn)
		nameWidth -= pathWidthDyn;

	//add columns
	std::wstring nameStr = pNativeSpeaker->getAttrNameStr(L"Name", FS_ROOTNODE, FS_CLMNNAME);
	insertColumn(nameStr.c_str(), nameWidth, colIndex);
	if (isExtColumn)
	{
		std::wstring extStr = pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT);
		insertColumn(extStr.c_str(), extWidthDyn, ++colIndex); //2nd column
	}
	if (isPathColumn)
	{
		std::wstring pathStr = pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH);
		insertColumn(pathStr.c_str(), pathWidthDyn, ++colIndex); //2nd column if .ext is off
	}

	TaskListInfo taskListInfo;
	static HWND nppHwnd = ::GetParent(_hParent);
	::SendMessage(nppHwnd, WM_GETTASKLISTINFO, reinterpret_cast<WPARAM>(&taskListInfo), TRUE);

	const size_t len = taskListInfo._openedDocInfoList.size();
	ListView_SetItemCount(_hSelf, len);

	for (size_t i = 0; i < len; ++i)
	{
		OpenedDocInfo & fileNameStatus = taskListInfo._openedDocInfoList[i];

		OpenedDocInfo *tl = new OpenedDocInfo(fileNameStatus);

		wchar_t fn[MAX_PATH] = { '\0' };
		wcscpy_s(fn, ::PathFindFileName(fileNameStatus._fullPathName.c_str()));

		if (isExtColumn)
		{
			::PathRemoveExtension(fn);
		}
		LVITEM item{};
		item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID;
		
		item.pszText = fn;
		item.iItem = static_cast<int>(i);
		item.iSubItem = 0;
		item.iImage = fileNameStatus._status;
		item.lParam = reinterpret_cast<LPARAM>(tl);
		item.iGroupId = fileNameStatus._iView;
		ListView_InsertItem(_hSelf, &item);
		int colIndex2 = 0;
		if (isExtColumn)
		{
			ListView_SetItemText(_hSelf, i, ++colIndex2, ::PathFindExtension(fileNameStatus._fullPathName.c_str()));
		}
		if (isPathColumn)
		{
			wchar_t dir[MAX_PATH] = { '\0' }, drive[MAX_PATH] = { '\0' };
			_wsplitpath_s(fileNameStatus._fullPathName.c_str(), drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0);
			wcscat_s(drive, dir);
			ListView_SetItemText(_hSelf, i, ++colIndex2, drive);
		}
	}
	_currentIndex = taskListInfo._currentIndex;
	selectCurrentItem();
	ensureVisibleCurrentItem();	// without this call the current item may become invisible after adding/removing columns
}

void VerticalFileSwitcherListView::reload()
{
	// Suppress redraws for performance. We target _hParent to prevent scroll bar flickering.
	::SendMessage(_hSelf, WM_SETREDRAW, false, 0);
	::SendMessage(_hParent, WM_SETREDRAW, false, 0);
	removeAll();
	initList();

	RECT rc{};
	::GetClientRect(_hParent, &rc);
	resizeColumns(rc.right - rc.left);
	::SendMessage(_hParent, WM_SETREDRAW, true, 0);
	::SendMessage(_hSelf, WM_SETREDRAW, true, 0);

	redrawItems();
	ensureVisibleCurrentItem();
}

void VerticalFileSwitcherListView::redrawItems()
{
	int nbItem = ListView_GetItemCount(_hSelf);
	ListView_RedrawItems(_hSelf, 0, nbItem - 1);
}

BufferID VerticalFileSwitcherListView::getBufferInfoFromIndex(int index, int & view) const
{
	int nbItem = ListView_GetItemCount(_hSelf);
	if (index < 0 || index >= nbItem)
		return BUFFER_INVALID;

	LVITEM item{};
	item.mask = LVIF_PARAM;
	item.iItem = index;
	ListView_GetItem(_hSelf, &item);
	const OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);

	view = odi->_iView;
	return static_cast<BufferID>(odi->_bufID);
}

int VerticalFileSwitcherListView::newItem(BufferID bufferID, int iView)
{
	int i = find(bufferID, iView);
	if (i == -1)
	{
		i = add(bufferID, iView);
	}
	return i;
}

void VerticalFileSwitcherListView::setItemIconStatus(BufferID bufferID)
{
	Buffer *buf = bufferID;
	
	wchar_t fn[MAX_PATH] = { '\0' };
	wcscpy_s(fn, ::PathFindFileName(buf->getFileName()));
	bool isExtColumn = !(NppParameters::getInstance()).getNppGUI()._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !(NppParameters::getInstance()).getNppGUI()._fileSwitcherWithoutPathColumn;
	if (isExtColumn)
	{
		::PathRemoveExtension(fn);
	}
	LVITEM item{};
	item.pszText = fn;
	item.iSubItem = 0;
	item.iImage = buf->isMonitoringOn()?3:(buf->isReadOnly()?2:(buf->isDirty()?1:0));

	int nbItem = ListView_GetItemCount(_hSelf);

	int limit = kViewsLimit;

	for (int i = 0 ; i < nbItem ; ++i)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
		if (odi->_bufID == bufferID)
		{
			odi->_fullPathName = buf->getFullPathName();
			item.mask = LVIF_TEXT | LVIF_IMAGE;
			ListView_SetItem(_hSelf, &item);
			int colIndex = 0;
			if (isExtColumn)
			{
				ListView_SetItemText(_hSelf, i, ++colIndex, ::PathFindExtension(buf->getFileName()));
			}
			if (isPathColumn)
			{
				wchar_t dir[MAX_PATH] = { '\0' }, drive[MAX_PATH] = { '\0' };
				_wsplitpath_s(buf->getFullPathName(), drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0);
				wcscat_s(drive, dir);
				ListView_SetItemText(_hSelf, i, ++colIndex, drive);
			}

			if (--limit <= 0)
			{
				break;
			}
		}
	}
}

void VerticalFileSwitcherListView::setItemColor(BufferID bufferID)
{
	Buffer* buf = bufferID;

	LVITEM item{};
	item.mask = LVIF_PARAM;

	int nbItem = ListView_GetItemCount(_hSelf);

	int limit = kViewsLimit;

	for (int i = 0; i < nbItem; ++i)
	{
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
		if (odi->_bufID != bufferID)
		{
			continue;
		}

		odi->_docColor = buf->getDocColorId();
		ListView_RedrawItems(_hSelf, i, i);

		if (--limit <= 0)
		{
			break;
		}
	}
}

std::wstring VerticalFileSwitcherListView::getFullFilePath(size_t i) const
{
	size_t nbItem = ListView_GetItemCount(_hSelf);
	if (i >= nbItem)
		return L"";

	LVITEM item{};
	item.mask = LVIF_PARAM;
	item.iItem = static_cast<int>(i);
	ListView_GetItem(_hSelf, &item);
	const OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);;

	return odi->_fullPathName;
}

int VerticalFileSwitcherListView::closeItem(BufferID bufferID, int iView)
{
	int i = find(bufferID, iView);
	if (i != -1)
		remove(i);
	return i;
}

void VerticalFileSwitcherListView::activateItem(BufferID bufferID, int iView)
{
	// Suppress redraws while we're resetting states
	::SendMessage(_hSelf, WM_SETREDRAW, false, 0);

	// Clean all selection
	int nbItem = ListView_GetItemCount(_hSelf);
	for (int i = 0; i < nbItem; ++i)
		ListView_SetItemState(_hSelf, i, 0, LVIS_FOCUSED|LVIS_SELECTED);

	_currentIndex = newItem(bufferID, iView);
	selectCurrentItem();
	// Have to enable redraw to be able to move selection to the current item
	::SendMessage(_hSelf, WM_SETREDRAW, true, 0);

	redrawItems();
	ensureVisibleCurrentItem();
}

int VerticalFileSwitcherListView::add(BufferID bufferID, int iView)
{
	_currentIndex = ListView_GetItemCount(_hSelf);
	Buffer *buf = bufferID;
	const wchar_t *fileName = buf->getFileName();
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
	OpenedDocInfo* tl = new OpenedDocInfo(iView, 0, buf->getFullPathName(), 0, bufferID, -1);

	wchar_t fn[MAX_PATH] = { '\0' };
	wcscpy_s(fn, ::PathFindFileName(fileName));
	bool isExtColumn = !nppGUI._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !nppGUI._fileSwitcherWithoutPathColumn;
	if (isExtColumn)
	{
		::PathRemoveExtension(fn);
	}
	LVITEM item{};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID;
	
	item.pszText = fn;
	item.iItem = _currentIndex;
	item.iSubItem = 0;
	item.iImage = buf->isMonitoringOn()?3:(buf->isReadOnly()?2:(buf->isDirty()?1:0));
	item.lParam = reinterpret_cast<LPARAM>(tl);
	item.iGroupId = iView;
	ListView_InsertItem(_hSelf, &item);
	int colIndex = 0;
	if (isExtColumn)
	{
		ListView_SetItemText(_hSelf, _currentIndex, ++colIndex, ::PathFindExtension(fileName));
	}
	if (isPathColumn)
	{
		wchar_t dir[MAX_PATH] = { '\0' }, drive[MAX_PATH] = { '\0' };
		_wsplitpath_s(buf->getFullPathName(), drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0);
		wcscat_s(drive, dir);
		ListView_SetItemText(_hSelf, _currentIndex, ++colIndex, drive);
	}
	selectCurrentItem();

	return _currentIndex;
}


void VerticalFileSwitcherListView::remove(int index, bool removeFromListview)
{
	LVITEM item{};
	item.mask = LVIF_PARAM;
	item.iItem = index;
	ListView_GetItem(_hSelf, &item);
	OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
	delete odi;
	
	if (removeFromListview)
		ListView_DeleteItem(_hSelf, index);
}

void VerticalFileSwitcherListView::removeAll()
{
	int nbItem = ListView_GetItemCount(_hSelf);
	
	for (int i = nbItem - 1; i >= 0 ; --i)
	{
		remove(i, false);
	}
	ListView_DeleteAllItems(_hSelf);

	HWND colHeader = reinterpret_cast<HWND>(SendMessage(_hSelf, LVM_GETHEADER, 0, 0));
	const auto columnCount = static_cast<int>(::SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));

	for (int i = 0; i < columnCount; ++i)
	{
		ListView_DeleteColumn(_hSelf, 0);
	}
}

int VerticalFileSwitcherListView::find(BufferID bufferID, int iView) const
{
	LVITEM item{};
	bool found = false;
	int nbItem = ListView_GetItemCount(_hSelf);
	int i = 0;
	for (; i < nbItem ; ++i)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		const OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
		if (odi->_bufID == bufferID && odi->_iView == iView)
		{
			found =  true;
			break;
		}
	}
	return (found?i:-1);	
}

void VerticalFileSwitcherListView::insertColumn(const wchar_t *name, int width, int index)
{
	LVCOLUMN lvColumn{};
 
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.cx = width;
	lvColumn.pszText = const_cast<wchar_t*>(name);
	ListView_InsertColumn(_hSelf, index, &lvColumn);
}

void VerticalFileSwitcherListView::resizeColumns(int totalWidth)
{
	NppParameters& nppParams = NppParameters::getInstance();
	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !nppParams.getNppGUI()._fileSwitcherWithoutPathColumn;

	const UINT dpi = DPIManagerV2::getDpiForWindow(_hParent);
	const int extWidthDyn = DPIManagerV2::scale(nppParams.getNppGUI()._fileSwitcherExtWidth, dpi);
	const int pathWidthDyn = DPIManagerV2::scale(nppParams.getNppGUI()._fileSwitcherPathWidth, dpi);

	int totalColWidthDynExceptName = 0;
	int colIndex = 0;

	if (isExtColumn)
	{
		totalColWidthDynExceptName += extWidthDyn;
		ListView_SetColumnWidth(_hSelf, ++colIndex, extWidthDyn);
	}
	if (isPathColumn)
	{
		totalColWidthDynExceptName += pathWidthDyn;
		ListView_SetColumnWidth(_hSelf, ++colIndex, pathWidthDyn);
	}

	const auto style = ::GetWindowLongPtr(_hSelf, GWL_STYLE);
	if ((style & WS_VSCROLL) == WS_VSCROLL)
	{
		totalColWidthDynExceptName += DPIManagerV2::getSystemMetricsForDpi(SM_CXVSCROLL, dpi);
	}

	ListView_SetColumnWidth(_hSelf, 0, totalWidth - totalColWidthDynExceptName);
}

std::vector<BufferViewInfo> VerticalFileSwitcherListView::getSelectedFiles(bool reverse) const
{
	std::vector<BufferViewInfo> files;
	LVITEM item{};
	int nbItem = ListView_GetItemCount(_hSelf);
	for (int i = 0; i < nbItem; ++i)
	{
		int isSelected = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
		bool isChosen = reverse?isSelected != LVIS_SELECTED:isSelected == LVIS_SELECTED;
		if (isChosen)
		{
			item.mask = LVIF_PARAM;
			item.iItem = i;
			ListView_GetItem(_hSelf, &item);

			const OpenedDocInfo* odi = reinterpret_cast<OpenedDocInfo*>(item.lParam);
			files.emplace_back(static_cast<BufferID>(odi->_bufID), odi->_iView);
		}
	}

	return files;
}
