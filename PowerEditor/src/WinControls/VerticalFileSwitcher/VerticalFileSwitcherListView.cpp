//this file is part of notepad++
//Copyright (C)2011 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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
	int listViewStyles = LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER\
						| LVS_SINGLESEL | LVS_AUTOARRANGE\
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
/*
	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);
*/
	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_INFOTIP);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_WIDTH;

	lvColumn.cx = 200;

	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	ListView_SetItemCountEx(_hSelf, 50, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, _hImaLst, LVSIL_SMALL);

	ListView_SetItemState(_hSelf, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	//ListView_SetBkColor(_hSelf, lightYellow);
}

void VerticalFileSwitcherListView::destroy()
{
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
} 

LRESULT VerticalFileSwitcherListView::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}

void VerticalFileSwitcherListView::initList()
{
	::SendMessage(::GetParent(_hParent), WM_GETTASKLISTINFO, (WPARAM)&_taskListInfo, TRUE);
	for (size_t i = 0 ; i < _taskListInfo._tlfsLst.size() ; i++)
	{
		TaskLstFnStatus & fileNameStatus = _taskListInfo._tlfsLst[i];

		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		
		item.pszText = (TCHAR *)::PathFindFileName(fileNameStatus._fn.c_str());
		item.iItem = i;
		item.iSubItem = 0;
		item.iImage = fileNameStatus._status;
		ListView_InsertItem(_hSelf, &item);
	}
}

int VerticalFileSwitcherListView::getBufferInfoFromIndex(int index, int & view) const {
	if (index < 0 || index >= int(_taskListInfo._tlfsLst.size()))
		return -1;
	view = _taskListInfo._tlfsLst[index]._iView;
	return int(_taskListInfo._tlfsLst[index]._bufID);
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
	LVITEM item;

	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.pszText = (TCHAR *)::PathFindFileName(buf->getFileName());
	item.iSubItem = 0;
	item.iImage = buf->getUserReadOnly()||buf->getFileReadOnly()?2:(buf->isDirty()?1:0);

	int i = find(bufferID, MAIN_VIEW);
	if (i != -1)
	{
		item.iItem = i;	
		ListView_SetItem(_hSelf, &item);
	}

	int j = find(bufferID, SUB_VIEW);
	if (j != -1 && j != i)
	{
		item.iItem = j;
		ListView_SetItem(_hSelf, &item);
	}
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
	int index = int(_taskListInfo._tlfsLst.size());
	Buffer *buf = (Buffer *)bufferID;
	const TCHAR *fn = buf->getFileName();

	_taskListInfo._tlfsLst.push_back(TaskLstFnStatus(iView, 0, fn, 0, (void *)bufferID));

	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	
	item.pszText = (TCHAR *)::PathFindFileName(fn);
	item.iItem = index;
	item.iSubItem = 0;
	item.iImage = buf->getUserReadOnly()||buf->getFileReadOnly()?2:(buf->isDirty()?1:0);
	ListView_InsertItem(_hSelf, &item);
	ListView_SetItemState(_hSelf, index, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
	
	return index;
}

void VerticalFileSwitcherListView::remove(int index)
{
	_taskListInfo._tlfsLst.erase(_taskListInfo._tlfsLst.begin() + index);
	ListView_DeleteItem(_hSelf, index);
}

int VerticalFileSwitcherListView::find(int bufferID, int iView) const
{
	bool found = false;
	size_t i = 0;
	for (; i < _taskListInfo._tlfsLst.size() ; i++)
	{
		if (_taskListInfo._tlfsLst[i]._bufID == (void *)bufferID &&
			_taskListInfo._tlfsLst[i]._iView == iView)
		{
			found = true;
			break;
		}
	}
	return (found?i:-1);	
}

