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

#include <shlwapi.h>
#include <stdexcept>
#include "VerticalFileSwitcherListView.h"
#include "Buffer.h"
#include "localization.h"

using namespace std;

void VerticalFileSwitcherListView::init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst)
{
	Window::init(hInst, parent);
	_hImaLst = hImaLst;
	INITCOMMONCONTROLSEX icex{};

	// Ensure that the common control DLL is loaded. 
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT /*| LVS_SINGLESEL*/ | LVS_AUTOARRANGE\
						| LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS | LVS_ALIGNTOP;

	_hSelf = ::CreateWindow(WC_LISTVIEW,
                                L"",
                                WS_CHILD | WS_BORDER | listViewStyles,
                                0,
                                0,
                                0,
                                0,
                                _hParent,
                                nullptr,
                                hInst,
                                nullptr);
	if (!_hSelf)
	{
		throw std::runtime_error("VerticalFileSwitcherListView::init : CreateWindowEx() function return null");
	}

	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);
	ListView_SetItemCountEx(_hSelf, 50, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, _hImaLst, LVSIL_SMALL);

	LVGROUP group{};
	constexpr size_t headerLen = 1;
	wchar_t header[headerLen] = L"";
	group.cbSize = sizeof(LVGROUP);
	group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	group.pszHeader = header;
	group.cchHeader = headerLen;
	group.iGroupId = _groupID;
	group.state = LVGS_COLLAPSIBLE;

	LVGROUP group2 = group;
	group2.iGroupId = _group2ID;

	ListView_InsertGroup(_hSelf, -1, &group);
	ListView_InsertGroup(_hSelf, -1, &group2);
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
		TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
		delete tlfs;
	}
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
} 

void VerticalFileSwitcherListView::initList()
{
	NppParameters& nppParams = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
	
	const bool isListViewGroups = !nppParams.getNppGUI()._fileSwitcherDisableListViewGroups;
	ListView_EnableGroupView(_hSelf, isListViewGroups ? TRUE : FALSE);

	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !nppParams.getNppGUI()._fileSwitcherWithoutPathColumn;

	RECT rc{};
	::GetClientRect(_hParent, &rc);
	int nameWidth = rc.right - rc.left;
	int colIndex = 0;
	if (isExtColumn)
		nameWidth -= nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherExtWidth);
	if (isPathColumn)
		nameWidth -= nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherPathWidth);

	//add columns
	wstring nameStr = pNativeSpeaker->getAttrNameStr(L"Name", FS_ROOTNODE, FS_CLMNNAME);
	insertColumn(nameStr.c_str(), nameWidth, ++colIndex);
	if (isExtColumn)
	{
		wstring extStr = pNativeSpeaker->getAttrNameStr(L"Ext.", FS_ROOTNODE, FS_CLMNEXT);
		insertColumn(extStr.c_str(), nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherExtWidth), ++colIndex); //2nd column
	}
	if (isPathColumn)
	{
		wstring pathStr = pNativeSpeaker->getAttrNameStr(L"Path", FS_ROOTNODE, FS_CLMNPATH);
		insertColumn(pathStr.c_str(), nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherPathWidth), ++colIndex); //2nd column if .ext is off
	}

	TaskListInfo taskListInfo;
	static HWND nppHwnd = ::GetParent(_hParent);
	::SendMessage(nppHwnd, WM_GETTASKLISTINFO, reinterpret_cast<WPARAM>(&taskListInfo), TRUE);

	for (size_t i = 0, len = taskListInfo._tlfsLst.size(); i < len ; ++i)
	{
		TaskLstFnStatus & fileNameStatus = taskListInfo._tlfsLst[i];

		TaskLstFnStatus *tl = new TaskLstFnStatus(fileNameStatus);

		wchar_t fn[MAX_PATH] = { '\0' };
		wcscpy_s(fn, ::PathFindFileName(fileNameStatus._fn.c_str()));

		if (isExtColumn)
		{
			::PathRemoveExtension(fn);
		}
		LVITEM item{};
		item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID;
		
		item.pszText = fn;
		item.iItem = static_cast<int32_t>(i);
		item.iSubItem = 0;
		item.iImage = fileNameStatus._status;
		item.lParam = reinterpret_cast<LPARAM>(tl);
		item.iGroupId = (fileNameStatus._iView == MAIN_VIEW) ? _groupID : _group2ID;
		ListView_InsertItem(_hSelf, &item);
		int colIndex2 = 0;
		if (isExtColumn)
		{
			ListView_SetItemText(_hSelf, i, ++colIndex2, ::PathFindExtension(fileNameStatus._fn.c_str()));
		}
		if (isPathColumn)
		{
			wchar_t dir[MAX_PATH] = { '\0' }, drive[MAX_PATH] = { '\0' };
			_wsplitpath_s(fileNameStatus._fn.c_str(), drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0);
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
	::SendMessage(_hParent, WM_SETREDRAW, false, 0);
	removeAll();
	initList();

	RECT rc{};
	::GetClientRect(_hParent, &rc);
	resizeColumns(rc.right - rc.left);
	::SendMessage(_hParent, WM_SETREDRAW, true, 0);
	redrawItems();
}

void VerticalFileSwitcherListView::redrawItems()
{
	int nbItem = ListView_GetItemCount(_hSelf);
	::SendMessage(_hSelf, WM_PAINT, 0, 0);
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
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

	view = tlfs->_iView;
	return static_cast<BufferID>(tlfs->_bufID);
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

	for (int i = 0 ; i < nbItem ; ++i)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		TaskLstFnStatus *tlfs = (TaskLstFnStatus *)(item.lParam);
		if (tlfs->_bufID == bufferID)
		{
			tlfs->_fn = buf->getFullPathName();
			item.mask = LVIF_TEXT | LVIF_IMAGE;
			ListView_SetItem(_hSelf, &item);
			int colIndex = 0;
			if (isExtColumn)
			{
				ListView_SetItemText(_hSelf, i, ++colIndex, (LPTSTR)::PathFindExtension(buf->getFileName()));
			}
			if (isPathColumn)
			{
				wchar_t dir[MAX_PATH] = { '\0' }, drive[MAX_PATH] = { '\0' };
				_wsplitpath_s(buf->getFullPathName(), drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0);
				wcscat_s(drive, dir);
				ListView_SetItemText(_hSelf, i, ++colIndex, drive);
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

	for (int i = 0; i < nbItem; ++i)
	{
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		TaskLstFnStatus* tlfs = reinterpret_cast<TaskLstFnStatus*>(item.lParam);
		if (tlfs->_bufID == bufferID)
		{
			tlfs->_docColor = buf->getDocColorId();
			ListView_SetItem(_hSelf, &item);
		}
	}

	redraw();
}

wstring VerticalFileSwitcherListView::getFullFilePath(size_t i) const
{
	size_t nbItem = ListView_GetItemCount(_hSelf);
	if (i > nbItem)
		return L"";

	LVITEM item{};
	item.mask = LVIF_PARAM;
	item.iItem = static_cast<int32_t>(i);
	ListView_GetItem(_hSelf, &item);
	const TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

	return tlfs->_fn;
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
	ensureVisibleCurrentItem();
	redrawItems();
}

int VerticalFileSwitcherListView::add(BufferID bufferID, int iView)
{
	_currentIndex = ListView_GetItemCount(_hSelf);
	Buffer *buf = bufferID;
	const wchar_t *fileName = buf->getFileName();
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
	TaskLstFnStatus *tl = new TaskLstFnStatus(iView, 0, buf->getFullPathName(), 0, (void *)bufferID, -1);

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
	item.iGroupId = (iView == MAIN_VIEW) ? _groupID : _group2ID;
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
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
	delete tlfs;
	
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
	int columnCount = static_cast<int32_t>(SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));

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
		const TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
		if (tlfs->_bufID == bufferID && tlfs->_iView == iView)
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
	lvColumn.pszText = (wchar_t *)name;
	ListView_InsertColumn(_hSelf, index, &lvColumn); // index is not 0 based but 1 based
}

void VerticalFileSwitcherListView::resizeColumns(int totalWidth)
{
	NppParameters& nppParams = NppParameters::getInstance();
	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	bool isPathColumn = !nppParams.getNppGUI()._fileSwitcherWithoutPathColumn;

	const int extWidthDyn = nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherExtWidth);
	const int pathWidthDyn = nppParams._dpiManager.scaleX(nppParams.getNppGUI()._fileSwitcherPathWidth);
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
		totalColWidthDynExceptName += ::GetSystemMetrics(SM_CXVSCROLL);
	}

	ListView_SetColumnWidth(_hSelf, 0, totalWidth - totalColWidthDynExceptName);
}

std::vector<BufferViewInfo> VerticalFileSwitcherListView::getSelectedFiles(bool reverse) const
{
	std::vector<BufferViewInfo> files;
	LVITEM item{};
	int nbItem = ListView_GetItemCount(_hSelf);
	int i = 0;
	for (; i < nbItem ; ++i)
	{
		int isSelected = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
		bool isChosen = reverse?isSelected != LVIS_SELECTED:isSelected == LVIS_SELECTED;
		if (isChosen)
		{
			item.mask = LVIF_PARAM;
			item.iItem = i;
			ListView_GetItem(_hSelf, &item);

			TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
			files.push_back(BufferViewInfo(static_cast<BufferID>(tlfs->_bufID), tlfs->_iView));
		}
	}

	return files;
}
