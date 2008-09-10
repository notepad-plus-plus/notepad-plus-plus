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

#include "TaskList.h"
#include "TaskListDlg_rc.h"
#include "colors.h"
#include "Common.h"

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
						| LVS_NOSCROLL | LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_OWNERDRAWFIXED\
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
		systemMessage(TEXT("System Err"));
		throw int(69);
	}

	::SetWindowLongPtr(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(staticProc)));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT ;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);


	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_WIDTH;

	lvColumn.cx = 1500;

	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	ListView_SetItemCountEx(_hSelf, _nbItem, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, hImaLst, LVSIL_SMALL);

	ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	ListView_SetBkColor(_hSelf, veryLiteGrey);
	ListView_SetTextBkColor(_hSelf, veryLiteGrey);
}

RECT TaskList::adjustSize()
{
	RECT rc;
	ListView_GetItemRect(_hSelf, 0, &rc, LVIR_ICON);
	const int imgWidth = rc.right - rc.left;
	const int marge = 30;

	for (int i = 0 ; i < _nbItem ; i++)
	{
		TCHAR buf[MAX_PATH];
		ListView_GetItemText(_hSelf, i, 0, buf, MAX_PATH);
		int width = ListView_GetStringWidth(_hSelf, buf);

		if (width > (_rc.right - _rc.left))
			_rc.right = _rc.left + width + imgWidth + marge;

		_rc.bottom += rc.bottom - rc.top;

	}

	// additional space for horizontal scroll-bar
	_rc.bottom += rc.bottom - rc.top;

	reSizeTo(_rc);
	return _rc;
}

int TaskList::updateCurrentIndex()
{
	for (int i = 0 ; i < _nbItem ; i++)
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
			//if (LOWORD(wParam) & MK_RBUTTON) 
			// It's not easy to press RBUTTON while moving a mouse wheel and holding CTRL :-)
			// Actually, I thought MOUSEWHEEL is not working until I saw this code
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
			}
			return TRUE;
		}

		case WM_KEYDOWN :
		{
			//printStr(TEXT("WM_KEYDOWN"));
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
					//printStr(TEXT("else"));
					return TRUE;
				}
					//return DLGC_WANTALLKEYS	;

			}
			return DLGC_WANTALLKEYS	;
		}

		default :
			return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
	}
	return FALSE;
}

