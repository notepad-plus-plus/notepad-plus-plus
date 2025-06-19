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

#include <stdexcept>
#include "TaskList.h"
#include "TaskListDlg_rc.h"
#include "colors.h"
#include "ImageListSet.h"
#include "Parameters.h"
#include "TaskListDlg.h"

void TaskList::init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst, int nbItem, int index2set)
{
	Window::init(hInst, parent);

	_currentIndex = index2set;

	INITCOMMONCONTROLSEX icex{};

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
		L"",
		WS_CHILD | listViewStyles,
		0,
		0,
		0,
		0,
		_hParent,
		NULL,
		hInst,
		NULL);
	if (!_hSelf)
	{
		throw std::runtime_error("TaskList::init : CreateWindowEx() function return null");
	}

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticProc)));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);


	LVCOLUMN lvColumn{};
	lvColumn.mask = LVCF_WIDTH;

	lvColumn.cx = 500;

	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	ListView_SetItemCountEx(_hSelf, _nbItem, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, hImaLst, LVSIL_SMALL);

	ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_SetBkColor(_hSelf, NppDarkMode::isEnabled() ? NppDarkMode::getBackgroundColor() : lightYellow);

	_hToolTip = CreateWindowEx(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _hSelf, nullptr, GetModuleHandle(nullptr), nullptr);

	TOOLINFO ti = { sizeof(TOOLINFO) };
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = _hSelf;
	ti.uId = (UINT_PTR)_hSelf;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	SendMessage(_hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
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
	RECT rc{};
	ListView_GetItemRect(_hSelf, 0, &rc, LVIR_ICON);
	const int imgWidth = rc.right - rc.left;
	const int aSpaceWidth = ListView_GetStringWidth(_hSelf, L" ");
	const int paddedBorder = ::GetSystemMetrics(SM_CXPADDEDBORDER);
	const int leftMarge = (::GetSystemMetrics(SM_CXFRAME) + paddedBorder) * 2 + aSpaceWidth * 4;

	// Temporary set "selected" font to get the worst case widths
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFontSelected), 0);
	int maxwidth = -1;

	_rc = { 0, 0, 0, 0 };
	wchar_t buf[MAX_PATH] = { '\0' };
	for (int i = 0 ; i < _nbItem ; ++i)
	{
		ListView_GetItemText(_hSelf, i, 0, buf, MAX_PATH);
		int width = ListView_GetStringWidth(_hSelf, buf);
		if (width > maxwidth)
			maxwidth = width;
		_rc.bottom += rc.bottom - rc.top;
	}

	_rc.right = maxwidth + imgWidth + leftMarge;
	ListView_SetColumnWidth(_hSelf, 0, _rc.right);
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);

	//if the tasklist exceeds the height of the display, leave some space at the bottom
	const LONG maxHeight = ::GetSystemMetrics(SM_CYSCREEN) - 120L;
	if (_rc.bottom > maxHeight)
	{
		_rc.bottom = maxHeight;
	}
	reSizeToWH(_rc);

	// Task List's border is 1px smaller than ::GetSystemMetrics(SM_CYFRAME) returns
	_rc.bottom += (::GetSystemMetrics(SM_CYFRAME) + paddedBorder - 1) * 2;
	return _rc;
}

void TaskList::setFont(const wchar_t *fontName, int fontSize)
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
				int32_t selected = (_currentIndex - 1) < 0 ? (_nbItem - 1) : (_currentIndex - 1);
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
				int32_t selected = (_currentIndex + 1) > (_nbItem - 1) ? 0 : (_currentIndex + 1);
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
			ListView_EnsureVisible(_hSelf, _currentIndex, true);
			return TRUE;
		}

		case WM_MOUSEMOVE:
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(_hSelf, &pt);


			LVHITTESTINFO hit = {};
			hit.pt = pt;
			int hoveredIndex = ListView_HitTest(_hSelf, &hit);


			if (hoveredIndex != _lastToolTipIndex && hoveredIndex >= 0) {
				wchar_t buf[MAX_PATH] = { 0 };
				ListView_GetItemText(_hSelf, hoveredIndex, 0, buf, MAX_PATH);


				TOOLINFO ti = { sizeof(TOOLINFO) };
				ti.uFlags = TTF_IDISHWND;
				ti.hwnd = _hSelf;
				ti.uId = (UINT_PTR)_hSelf;
				ti.lpszText = buf;
				SendMessage(_hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);


				_lastToolTipIndex = hoveredIndex;
			}
			break;
		}

		case WM_KEYDOWN :
		{
			return TRUE;
		}
		
		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;
			if (nmhdr->code == TTN_GETDISPINFO) {
				NMTTDISPINFO* dispInfo = (NMTTDISPINFO*)lParam;
				if (_lastToolTipIndex >= 0) {
					// Set the tooltip text to the full file path
					wcsncpy_s(
						dispInfo->szText,
						_countof(dispInfo->szText),
						_taskListInfo->_tlfsLst[_lastToolTipIndex]._fn.c_str(),
						_TRUNCATE
					);
					dispInfo->szText[_countof(dispInfo->szText) - 1] = L'\0';
				}
				return TRUE;
			}
			break;
		}

		case WM_GETDLGCODE :
		{
			const MSG *msg = (MSG*)lParam;

			if ( msg != NULL)
			{
				if ((msg->message == WM_KEYDOWN) && (0x80 & GetKeyState(VK_CONTROL)))
				{
					// Shift+Tab is cool but I think VK_UP and VK_LEFT are also cool :-)
					if (((msg->wParam == VK_TAB) && (0x80 & GetKeyState(VK_SHIFT))) ||
					    (msg->wParam == VK_UP))
					{ 
						int32_t selected = (_currentIndex - 1) < 0 ? (_nbItem - 1) : (_currentIndex - 1);
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
						int32_t selected = (_currentIndex + 1) > (_nbItem - 1) ? 0 : (_currentIndex + 1);
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
					ListView_EnsureVisible(_hSelf, _currentIndex, true);
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
	return 0;
}
