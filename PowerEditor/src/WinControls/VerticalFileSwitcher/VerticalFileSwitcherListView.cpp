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
						| LVS_SHAREIMAGELISTS /*| WS_BORDER*/;

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

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);

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
	::SendMessage(::GetParent(_hParent), WM_GETTASKLISTINFO, (WPARAM)&_taskListInfo, 0);
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
