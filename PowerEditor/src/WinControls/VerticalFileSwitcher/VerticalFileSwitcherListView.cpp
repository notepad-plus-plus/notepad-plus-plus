// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <shlwapi.h>
#include <stdexcept>
#include "VerticalFileSwitcherListView.h"
#include "Buffer.h"
#include "localization.h"

#define FS_ROOTNODE					"DocSwitcher"
#define FS_CLMNNAME					"ColumnName"
#define FS_CLMNEXT					"ColumnExt"

void VerticalFileSwitcherListView::init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst)
{
	Window::init(hInst, parent);
	_hImaLst = hImaLst;
    INITCOMMONCONTROLSEX icex;
    
    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT /*| LVS_SINGLESEL*/ | LVS_AUTOARRANGE\
						| LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;

	_hSelf = ::CreateWindow(WC_LISTVIEW,
                                TEXT(""),
                                WS_CHILD | listViewStyles,
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

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticProc)));

	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_INFOTIP);
	ListView_SetItemCountEx(_hSelf, 50, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, _hImaLst, LVSIL_SMALL);
}

void VerticalFileSwitcherListView::destroy()
{
	LVITEM item;
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

LRESULT VerticalFileSwitcherListView::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}

void VerticalFileSwitcherListView::initList()
{
	HWND colHeader = reinterpret_cast<HWND>(SendMessage(_hSelf, LVM_GETHEADER, 0, 0));
	int columnCount = static_cast<int32_t>(SendMessage(colHeader, HDM_GETITEMCOUNT, 0, 0));
	
	NppParameters& nppParams = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
	
	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	
	// check if columns need to be added
	if (columnCount <= 1)
	{
		RECT rc;
		::GetClientRect(_hParent, &rc);
		int totalWidth = rc.right - rc.left;
		
		if (columnCount == 0)
		{
			generic_string nameStr = pNativeSpeaker->getAttrNameStr(TEXT("Name"), FS_ROOTNODE, FS_CLMNNAME);
			insertColumn(nameStr.c_str(), (isExtColumn ? totalWidth - 50 : totalWidth), 0);
		}
		
		if (isExtColumn)
		{
			// resize "Name" column when "exts" won't fit
			LVCOLUMN lvc;
			lvc.mask = LVCF_WIDTH;
			SendMessage(_hSelf, LVM_GETCOLUMN, 0, reinterpret_cast<LPARAM>(&lvc));
			
			if (lvc.cx + 50 > totalWidth)
			{
				lvc.cx = totalWidth - 50;
				SendMessage(_hSelf, LVM_SETCOLUMN, 0, reinterpret_cast<LPARAM>(&lvc));
			}
			
			generic_string extStr = pNativeSpeaker->getAttrNameStr(TEXT("Ext."), FS_ROOTNODE, FS_CLMNEXT);
			insertColumn(extStr.c_str(), 50, 1);
		}
	}
	
	// "exts" was disabled
	if (columnCount >= 2 && !isExtColumn)
	{
		ListView_DeleteColumn(_hSelf, 1);
	}
	
	TaskListInfo taskListInfo;
	static HWND nppHwnd = ::GetParent(_hParent);
	::SendMessage(nppHwnd, WM_GETTASKLISTINFO, reinterpret_cast<WPARAM>(&taskListInfo), TRUE);

	for (size_t i = 0, len = taskListInfo._tlfsLst.size(); i < len ; ++i)
	{
		TaskLstFnStatus & fileNameStatus = taskListInfo._tlfsLst[i];

		TaskLstFnStatus *tl = new TaskLstFnStatus(fileNameStatus._iView, fileNameStatus._docIndex, fileNameStatus._fn, fileNameStatus._status, (void *)fileNameStatus._bufID);

		TCHAR fn[MAX_PATH];
		wcscpy_s(fn, ::PathFindFileName(fileNameStatus._fn.c_str()));

		if (isExtColumn)
		{
			::PathRemoveExtension(fn);
		}
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		
		item.pszText = fn;
		item.iItem = static_cast<int32_t>(i);
		item.iSubItem = 0;
		item.iImage = fileNameStatus._status;
		item.lParam = reinterpret_cast<LPARAM>(tl);
		ListView_InsertItem(_hSelf, &item);
		if (isExtColumn)
		{
			ListView_SetItemText(_hSelf, i, 1, ::PathFindExtension(fileNameStatus._fn.c_str()));
		}
	}
	ListView_SetItemState(_hSelf, taskListInfo._currentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void VerticalFileSwitcherListView::reload()
{
	removeAll();
	initList();
}

BufferID VerticalFileSwitcherListView::getBufferInfoFromIndex(int index, int & view) const
{
	int nbItem = ListView_GetItemCount(_hSelf);
	if (index < 0 || index >= nbItem)
		return BUFFER_INVALID;

	LVITEM item;
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
	Buffer *buf = static_cast<Buffer *>(bufferID);
	
	TCHAR fn[MAX_PATH];
	wcscpy_s(fn, ::PathFindFileName(buf->getFileName()));
	bool isExtColumn = !(NppParameters::getInstance()).getNppGUI()._fileSwitcherWithoutExtColumn;
	if (isExtColumn)
	{
		::PathRemoveExtension(fn);
	}
	LVITEM item;
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

			if (isExtColumn)
			{
				ListView_SetItemText(_hSelf, i, 1, (LPTSTR)::PathFindExtension(buf->getFileName()));
			}
		}
	}
}

generic_string VerticalFileSwitcherListView::getFullFilePath(size_t i) const
{
	size_t nbItem = ListView_GetItemCount(_hSelf);
	if (i > nbItem)
		return TEXT("");

	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = static_cast<int32_t>(i);
	ListView_GetItem(_hSelf, &item);
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

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
	// Clean all selection
	int nbItem = ListView_GetItemCount(_hSelf);
	for (int i = 0; i < nbItem; ++i)
		ListView_SetItemState(_hSelf, i, 0, LVIS_FOCUSED|LVIS_SELECTED);

	int i = newItem(bufferID, iView);
	ListView_SetItemState(_hSelf, i, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}

int VerticalFileSwitcherListView::add(BufferID bufferID, int iView)
{
	int index = ListView_GetItemCount(_hSelf);
	Buffer *buf = static_cast<Buffer *>(bufferID);
	const TCHAR *fileName = buf->getFileName();

	TaskLstFnStatus *tl = new TaskLstFnStatus(iView, 0, buf->getFullPathName(), 0, (void *)bufferID);

	TCHAR fn[MAX_PATH];
	wcscpy_s(fn, ::PathFindFileName(fileName));
	bool isExtColumn = !(NppParameters::getInstance()).getNppGUI()._fileSwitcherWithoutExtColumn;
	if (isExtColumn)
	{
		::PathRemoveExtension(fn);
	}
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	
	item.pszText = fn;
	item.iItem = index;
	item.iSubItem = 0;
	item.iImage = buf->isMonitoringOn()?3:(buf->isReadOnly()?2:(buf->isDirty()?1:0));
	item.lParam = reinterpret_cast<LPARAM>(tl);
	ListView_InsertItem(_hSelf, &item);

	if (isExtColumn)
	{
		ListView_SetItemText(_hSelf, index, 1, ::PathFindExtension(fileName));
	}
	ListView_SetItemState(_hSelf, index, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
	
	return index;
}


void VerticalFileSwitcherListView::remove(int index)
{
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = index;
	ListView_GetItem(_hSelf, &item);
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
	delete tlfs;
	ListView_DeleteItem(_hSelf, index);
}

void VerticalFileSwitcherListView::removeAll()
{
	int nbItem = ListView_GetItemCount(_hSelf);
	
	for (int i = nbItem - 1; i >= 0 ; --i)
	{
		remove(i);
	}
}

int VerticalFileSwitcherListView::find(BufferID bufferID, int iView) const
{
	LVITEM item;
	bool found = false;
	int nbItem = ListView_GetItemCount(_hSelf);
	int i = 0;
	for (; i < nbItem ; ++i)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
		if (tlfs->_bufID == bufferID && tlfs->_iView == iView)
		{
			found =  true;
			break;
		}
	}
	return (found?i:-1);	
}

void VerticalFileSwitcherListView::insertColumn(const TCHAR *name, int width, int index)
{
	LVCOLUMN lvColumn;
 
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.cx = width;
	lvColumn.pszText = (TCHAR *)name;
	ListView_InsertColumn(_hSelf, index, &lvColumn);
}

void VerticalFileSwitcherListView::resizeColumns(int totalWidth)
{
	NppParameters& nppParams = NppParameters::getInstance();
	bool isExtColumn = !nppParams.getNppGUI()._fileSwitcherWithoutExtColumn;
	if (isExtColumn)
	{
		ListView_SetColumnWidth(_hSelf, 0, totalWidth - 50);
		ListView_SetColumnWidth(_hSelf, 1, 50);
	}
	else
	{
		ListView_SetColumnWidth(_hSelf, 0, totalWidth);
	}
}

std::vector<SwitcherFileInfo> VerticalFileSwitcherListView::getSelectedFiles(bool reverse) const
{
	std::vector<SwitcherFileInfo> files;
	LVITEM item;
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
			files.push_back(SwitcherFileInfo(static_cast<BufferID>(tlfs->_bufID), tlfs->_iView));
		}
	}

	return files;
}
