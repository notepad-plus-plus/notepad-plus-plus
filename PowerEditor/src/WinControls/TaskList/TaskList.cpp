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



#include "TaskList.h"
#include "TaskListDlg_rc.h"
#include "colors.h"
#include "ImageListSet.h"
#include "Parameters.h"

void TaskList::init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst, int nbItem, int index2set)
{
	Window::init(hInst, parent);

	_currentIndex = index2set;

    INITCOMMONCONTROLSEX icex;
    
    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

	_nbItem = nbItem;
    
    // Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT | LVS_OWNERDATA | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER\
						| /*LVS_NOSCROLL |*/ LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_OWNERDRAWFIXED\
						| LVS_SHAREIMAGELISTS/* | WS_BORDER*/;

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
		throw std::runtime_error("TaskList::init : CreateWindowEx() function return null");
	}

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticProc));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);


	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_WIDTH;

	lvColumn.cx = 500;

	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	ListView_SetItemCountEx(_hSelf, _nbItem, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, hImaLst, LVSIL_SMALL);

	ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	ListView_SetBkColor(_hSelf, lightYellow);
}

void TaskList::destroy()
{
	if (_hFont)
		DeleteObject(_hFont);
	if (_hFontSelected)
		DeleteObject(_hFontSelected);
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
}

RECT TaskList::adjustSize()
{
	RECT rc;
	ListView_GetItemRect(_hSelf, 0, &rc, LVIR_ICON);
	const int imgWidth = rc.right - rc.left;
	const int leftMarge = 30;
	const int xpBottomMarge = 5;
	const int w7BottomMarge = 15;

	// Temporary set "selected" font to get the worst case widths
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFontSelected), 0);
	int maxwidth = -1;

	_rc.left = 0;
	_rc.top = 0;
	_rc.bottom = 0;
	for (int i = 0 ; i < _nbItem ; ++i)
	{
		TCHAR buf[MAX_PATH];
		ListView_GetItemText(_hSelf, i, 0, buf, MAX_PATH);
		int width = ListView_GetStringWidth(_hSelf, buf);
		if (width > maxwidth)
			maxwidth = width;
		_rc.bottom += rc.bottom - rc.top;
	}
	_rc.right = maxwidth + imgWidth + leftMarge;
	ListView_SetColumnWidth(_hSelf, 0, _rc.right);
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);

	reSizeTo(_rc);
	winVer ver = (NppParameters::getInstance())->getWinVersion();
	_rc.bottom += (ver <= WV_XP && ver != WV_UNKNOWN)?xpBottomMarge:w7BottomMarge;
	return _rc;
}

void TaskList::setFont(TCHAR *fontName, size_t fontSize)
{
	if (_hFont)
		::DeleteObject(_hFont);
	if (_hFontSelected)
		::DeleteObject(_hFontSelected);

	_hFont = ::CreateFont(fontSize, 0, 0, 0,
		                   FW_NORMAL,
			               0, 0, 0, 0,
			               0, 0, 0, 0,
				           fontName);

	_hFontSelected = ::CreateFont(fontSize, 0, 0, 0,
		                   FW_BOLD,
			               0, 0, 0, 0,
			               0, 0, 0, 0,
				           fontName);

	if (_hFont)
		::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
}

int TaskList::updateCurrentIndex()
{
	for (int i = 0 ; i < _nbItem ; ++i)
	{
		int isSelected = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
		if (isSelected == LVIS_SELECTED)
		{
			_currentIndex = i;
			return _currentIndex;
		}
	}
	return _currentIndex;
}

LRESULT TaskList::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_KEYUP:
		{
			if (wParam == VK_CONTROL)
			{
				::SendMessage(_hParent, WM_COMMAND, ID_PICKEDUP, _currentIndex);
			}
		}
		return TRUE;

		case WM_MOUSEWHEEL :
		{
			short zDelta = (short) HIWORD(wParam);
			if (zDelta > 0)
			{
				size_t selected = (_currentIndex - 1) < 0 ? (_nbItem - 1) : (_currentIndex - 1);
				ListView_SetItemState(_hSelf, _currentIndex, 0, LVIS_SELECTED|LVIS_FOCUSED);
				// tells what item(s) to be repainted
				ListView_RedrawItems(_hSelf, _currentIndex, _currentIndex);
				// repaint item(s)
				UpdateWindow(_hSelf); 
				ListView_SetItemState(_hSelf, selected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
				// tells what item(s) to be repainted
				ListView_RedrawItems(_hSelf, selected, selected);
				// repaint item(s)
				UpdateWindow(_hSelf);              
				_currentIndex = selected;
			}
			else
			{
				size_t selected = (_currentIndex + 1) > (_nbItem - 1) ? 0 : (_currentIndex + 1);
				ListView_SetItemState(_hSelf, _currentIndex, 0, LVIS_SELECTED|LVIS_FOCUSED);
				// tells what item(s) to be repainted
				ListView_RedrawItems(_hSelf, _currentIndex, _currentIndex);
				// repaint item(s)
				UpdateWindow(_hSelf); 
				ListView_SetItemState(_hSelf, selected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
				// tells what item(s) to be repainted
				ListView_RedrawItems(_hSelf, selected, selected);
				// repaint item(s)
				UpdateWindow(_hSelf);              
				_currentIndex = selected;
			}
			return TRUE;
		}

		case WM_KEYDOWN :
		{
			return TRUE;
		}
		

		case WM_GETDLGCODE :
		{
			MSG *msg = (MSG*)lParam;

			if ( msg != NULL)
			{
				if ((msg->message == WM_KEYDOWN) && (0x80 & GetKeyState(VK_CONTROL)))
				{
					// Shift+Tab is cool but I think VK_UP and VK_LEFT are also cool :-)
					if (((msg->wParam == VK_TAB) && (0x80 & GetKeyState(VK_SHIFT))) ||
					    (msg->wParam == VK_UP))
					{ 
						size_t selected = (_currentIndex - 1) < 0 ? (_nbItem - 1) : (_currentIndex - 1);
						ListView_SetItemState(_hSelf, _currentIndex, 0, LVIS_SELECTED|LVIS_FOCUSED);
						// tells what item(s) to be repainted
						ListView_RedrawItems(_hSelf, _currentIndex, _currentIndex);
						// repaint item(s)
						UpdateWindow(_hSelf); 
						ListView_SetItemState(_hSelf, selected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						// tells what item(s) to be repainted
						ListView_RedrawItems(_hSelf, selected, selected);
						// repaint item(s)
						UpdateWindow(_hSelf);              
						_currentIndex = selected;
					}
					// VK_DOWN and VK_RIGHT do the same as VK_TAB does
					else if ((msg->wParam == VK_TAB) || (msg->wParam == VK_DOWN))
					{
						size_t selected = (_currentIndex + 1) > (_nbItem - 1) ? 0 : (_currentIndex + 1);
						ListView_SetItemState(_hSelf, _currentIndex, 0, LVIS_SELECTED|LVIS_FOCUSED);
						// tells what item(s) to be repainted
						ListView_RedrawItems(_hSelf, _currentIndex, _currentIndex);
						// repaint item(s)
						UpdateWindow(_hSelf);
						ListView_SetItemState(_hSelf, selected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						// tells what item(s) to be repainted
						ListView_RedrawItems(_hSelf, selected, selected);
						// repaint item(s)
						UpdateWindow(_hSelf);              
						_currentIndex = selected;
					}
				}
				else
				{
					return TRUE;
				}
			}
			return DLGC_WANTALLKEYS	;
		}

		default :
			return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
	}
}

