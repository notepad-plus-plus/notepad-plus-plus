//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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
#include "ListView.h"


void ListView::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);
    INITCOMMONCONTROLSEX icex;
    
    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT | LVS_NOSORTHEADER\
						| LVS_SINGLESEL | LVS_AUTOARRANGE\
						| LVS_SHAREIMAGELISTS;

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
		throw std::runtime_error("ListView::init : CreateWindowEx() function return null");
	}

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticProc));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT|LVCF_WIDTH;
	lvColumn.cx = 45;

	lvColumn.pszText = TEXT("Value");
	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	lvColumn.pszText = TEXT("Char");
	ListView_InsertColumn(_hSelf, 1, &lvColumn);

	lvColumn.pszText = TEXT("Ctrl char");
	lvColumn.cx = 90;
	ListView_InsertColumn(_hSelf, 2, &lvColumn);
	
	//ListView_SetImageList(_hSelf, hImaLst, LVSIL_SMALL);

	//ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	//ListView_SetBkColor(_hSelf, lightYellow);
}

void ListView::resetValues(int codepage)
{
	ListView_DeleteAllItems(_hSelf);
	for (int i = 0 ; i < 256 ; i++)
	{
		LVITEM item;
		item.mask = LVIF_TEXT;
		TCHAR num[8];
		generic_sprintf(num, TEXT("%d"), i); 
		item.pszText = num;
		item.iItem = i;
		item.iSubItem = 0;
		ListView_InsertItem(_hSelf, &item);

		char ascii[8];
		ascii[0] = (unsigned char)i;
		ascii[1] = '\0';
#ifdef UNICODE
        wchar_t wCharStr[10];
        MultiByteToWideChar(codepage, 0, ascii, -1, wCharStr, sizeof(wCharStr));
		ListView_SetItemText(_hSelf, i, 1, wCharStr);
#else
		codepage = 0; // make it compile in ANSI Release mode
		ListView_SetItemText(_hSelf, i, 1, ascii);
#endif
	}
}
void ListView::destroy()
{
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
}


LRESULT ListView::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}

