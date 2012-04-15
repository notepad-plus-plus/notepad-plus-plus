// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#include "precompiledHeaders.h"
#include "VerticalFileSwitcherListView.h"
#include "Buffer.h"

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
	int listViewStyles = LVS_REPORT | LVS_SINGLESEL | LVS_AUTOARRANGE\
						| LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;

	_hSelf = ::CreateWindow(WC_LISTVIEW,
                                TEXT(""),
                                WS_CHILD | listViewStyles,
                                0,
                                0,
                                0,
                                0,
                                _hParent,
                                (HMENU) NULL,
                                hInst,
                                NULL);
	if (!_hSelf)
	{
		throw std::runtime_error("VerticalFileSwitcherListView::init : CreateWindowEx() function return null");
	}

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticProc));

	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_INFOTIP);
	ListView_SetItemCountEx(_hSelf, 50, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, _hImaLst, LVSIL_SMALL);
	ListView_SetItemState(_hSelf, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
}

void VerticalFileSwitcherListView::destroy()
{
	LVITEM item;
	item.mask = LVIF_PARAM;
	int nbItem = ListView_GetItemCount(_hSelf);
	for (int i = 0 ; i < nbItem ; i++)
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
	TaskListInfo taskListInfo;
	::SendMessage(::GetParent(_hParent), WM_GETTASKLISTINFO, (WPARAM)&taskListInfo, TRUE);
	for (size_t i = 0 ; i < taskListInfo._tlfsLst.size() ; i++)
	{
		TaskLstFnStatus & fileNameStatus = taskListInfo._tlfsLst[i];

		TaskLstFnStatus *tl = new TaskLstFnStatus(fileNameStatus._iView, fileNameStatus._docIndex, fileNameStatus._fn, fileNameStatus._status, (void *)fileNameStatus._bufID);

		TCHAR fn[MAX_PATH];
		lstrcpy(fn, ::PathFindFileName(fileNameStatus._fn.c_str()));
		::PathRemoveExtension(fn);

		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		
		item.pszText = fn;
		item.iItem = i;
		item.iSubItem = 0;
		item.iImage = fileNameStatus._status;
		item.lParam = (LPARAM)tl;
		ListView_InsertItem(_hSelf, &item);
		ListView_SetItemText(_hSelf, i, 1, (LPTSTR)::PathFindExtension(fileNameStatus._fn.c_str()));
	}
}

int VerticalFileSwitcherListView::getBufferInfoFromIndex(int index, int & view) const {
	int nbItem = ListView_GetItemCount(_hSelf);
	if (index < 0 || index >= nbItem)
		return -1;

	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = index;
	ListView_GetItem(_hSelf, &item);
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

	view = tlfs->_iView;
	return int(tlfs->_bufID);
}

int VerticalFileSwitcherListView::newItem(int bufferID, int iView)
{
	int i = find(bufferID, iView);
	if (i == -1)
	{
		i = add(bufferID, iView);
	}
	return i;
}

void VerticalFileSwitcherListView::setItemIconStatus(int bufferID)
{
	Buffer *buf = (Buffer *)bufferID;
	
	TCHAR fn[MAX_PATH];
	lstrcpy(fn, ::PathFindFileName(buf->getFileName()));
	::PathRemoveExtension(fn);

	LVITEM item;
	item.pszText = fn;
	item.iSubItem = 0;
	item.iImage = buf->getUserReadOnly()||buf->getFileReadOnly()?2:(buf->isDirty()?1:0);

	int nbItem = ListView_GetItemCount(_hSelf);

	for (int i = 0 ; i < nbItem ; i++)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		TaskLstFnStatus *tlfs = (TaskLstFnStatus *)(item.lParam);
		if (int(tlfs->_bufID) == bufferID)
		{
			item.mask = LVIF_TEXT | LVIF_IMAGE;
			ListView_SetItem(_hSelf, &item);
			ListView_SetItemText(_hSelf, i, 1, (LPTSTR)::PathFindExtension(buf->getFileName()));
		}
	}
}

generic_string VerticalFileSwitcherListView::getFullFilePath(size_t i) const
{
	size_t nbItem = ListView_GetItemCount(_hSelf);
	if (i < 0 || i > nbItem)
		return TEXT("");

	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = i;
	ListView_GetItem(_hSelf, &item);
	TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;

	return tlfs->_fn;
}

int VerticalFileSwitcherListView::closeItem(int bufferID, int iView)
{
	int i = find(bufferID, iView);
	if (i != -1)
		remove(i);
	return i;
}

void VerticalFileSwitcherListView::activateItem(int bufferID, int iView)
{
	int i = find(bufferID, iView);
	if (i == -1)
	{
		newItem(bufferID, iView);
	}
	ListView_SetItemState(_hSelf, i, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}

int VerticalFileSwitcherListView::add(int bufferID, int iView)
{
	int index = ListView_GetItemCount(_hSelf);
	Buffer *buf = (Buffer *)bufferID;
	const TCHAR *fileName = buf->getFileName();

	TaskLstFnStatus *tl = new TaskLstFnStatus(iView, 0, fileName, 0, (void *)bufferID);

	TCHAR fn[MAX_PATH];
	lstrcpy(fn, ::PathFindFileName(fileName));
	::PathRemoveExtension(fn);

	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	
	item.pszText = fn;
	item.iItem = index;
	item.iSubItem = 0;
	item.iImage = buf->getUserReadOnly()||buf->getFileReadOnly()?2:(buf->isDirty()?1:0);
	item.lParam = (LPARAM)tl;
	ListView_InsertItem(_hSelf, &item);
	
	ListView_SetItemText(_hSelf, index, 1, ::PathFindExtension(fileName));
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

int VerticalFileSwitcherListView::find(int bufferID, int iView) const
{
	LVITEM item;
	bool found = false;
	int nbItem = ListView_GetItemCount(_hSelf);
	int i = 0;
	for (; i < nbItem ; i++)
	{
		item.mask = LVIF_PARAM;
		item.iItem = i;
		ListView_GetItem(_hSelf, &item);
		TaskLstFnStatus *tlfs = (TaskLstFnStatus *)item.lParam;
		if (int(tlfs->_bufID) == bufferID && tlfs->_iView == iView)
		{
			found =  true;
			break;
		}
	}
	return (found?i:-1);	
}

void VerticalFileSwitcherListView::insertColumn(TCHAR *name, int width, int index)
{
	LVCOLUMN lvColumn;
 
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.cx = width;
	lvColumn.pszText = name;
	ListView_InsertColumn(_hSelf, index, &lvColumn);
}
