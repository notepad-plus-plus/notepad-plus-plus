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
// along with this program.	 If not, see <https://www.gnu.org/licenses/>.

#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>
#include "URLCtrl.h"
#include "Tools.h"
#include "Notepad_plus_msgs.h"


static COLORREF getCtrlBgColor(HWND hWnd)
{
	COLORREF crRet = CLR_INVALID;

	if (hWnd && ::IsWindow(hWnd))
	{
		RECT rc;

		if (::GetClientRect(hWnd, &rc))
		{
			HDC hDC = ::GetDC(hWnd);

			if (hDC)
			{
				HDC hdcMem = ::CreateCompatibleDC(hDC);

				if (hdcMem)
				{
					HBITMAP hBmp = ::CreateCompatibleBitmap(hDC, rc.right, rc.bottom);

					if (hBmp)
					{
						HGDIOBJ hOld = ::SelectObject(hdcMem, hBmp);

						if (hOld)
						{
							if (::SendMessageW(hWnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdcMem), 0))
								crRet = ::GetPixel(hdcMem, 2, 2); // 0, 0 is usually on the border

							::SelectObject(hdcMem, hOld);
						}

						::DeleteObject(hBmp);
					}

					::DeleteDC(hdcMem);
				}

				::ReleaseDC(hWnd, hDC);
			}
		}
	}

	return crRet;
}


void URLCtrl::create(HWND itemHandle, const wchar_t* link, COLORREF linkColor)
{
	// turn on notify style
	::SetWindowLongPtrW(itemHandle, GWL_STYLE, ::GetWindowLongPtrW(itemHandle, GWL_STYLE) | SS_NOTIFY);

	// set the URL text (not the display text)
	if (link)
		_URL = link;

	// set the hyperlink colour
	_linkColor = linkColor;

	// subclass the static control
	_oldproc = reinterpret_cast<WNDPROC>(
			::SetWindowLongPtrW(itemHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(URLCtrlProc)));

	// associate the URL structure with the static control
	::SetWindowLongPtrW(itemHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	// save hwnd
	_hSelf = itemHandle;

	loadHandCursor();
}

void URLCtrl::create(HWND itemHandle, int cmd, HWND msgDest)
{
	// turn on notify style
	::SetWindowLongPtrW(itemHandle, GWL_STYLE, ::GetWindowLongPtrW(itemHandle, GWL_STYLE) | SS_NOTIFY);

	_cmdID = cmd;
	_msgDest = msgDest;

	// set the hyperlink colour
	_linkColor = ::GetSysColor(COLOR_HOTLIGHT);

	// subclass the static control
	_oldproc = reinterpret_cast<WNDPROC>(
			::SetWindowLongPtrW(itemHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(URLCtrlProc)));

	// associate the URL structure with the static control
	::SetWindowLongPtrW(itemHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	// save hwnd
	_hSelf = itemHandle;

	loadHandCursor();
}

void URLCtrl::destroy()
{
	if (_hfUnderlined)
	{
		::DeleteObject(_hfUnderlined);
		_hfUnderlined = nullptr;
	}
}

HCURSOR& URLCtrl::loadHandCursor()
{
	if (_hCursor == nullptr)
		_hCursor = (HCURSOR)::LoadImage(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

	return _hCursor;
}

void URLCtrl::action()
{
	if (_cmdID)
	{
		::SendMessage(_msgDest?_msgDest:_hParent, WM_COMMAND, _cmdID, 0);
	}
	else
	{
		// Open a browser
		if (!_URL.empty())
		{
			::ShellExecuteW(NULL, L"open", _URL.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			wchar_t szWinText[MAX_PATH] = { '\0' };
			::GetWindowTextW(_hSelf, szWinText, MAX_PATH);
			::ShellExecuteW(NULL, L"open", szWinText, NULL, NULL, SW_SHOWNORMAL);
		}
	}
}

LRESULT URLCtrl::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		// Free up the structure we allocated
		case WM_NCDESTROY:
			break;

		// Paint the static control using our custom
		// colours, and with an underline text style
		case WM_PAINT:
		{
			DWORD dwStyle = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
			DWORD dwDTStyle = DT_SINGLELINE;

			//Test if centered horizontally or vertically
			if (dwStyle & SS_CENTER)		dwDTStyle |= DT_CENTER;
			if (dwStyle & SS_RIGHT)			dwDTStyle |= DT_RIGHT;
			if (dwStyle & SS_CENTERIMAGE)	dwDTStyle |= DT_VCENTER;

			RECT rect{};
			::GetClientRect(hwnd, &rect);

			PAINTSTRUCT ps{};
			HDC hdc = ::BeginPaint(hwnd, &ps);

			::SetTextColor(hdc, _linkColor);
			::SetBkColor(hdc, getCtrlBgColor(GetParent(hwnd)));

			// Create an underline font
			if (_hfUnderlined == nullptr)
				_hfUnderlined = createFontFromSystemDefault(SysFont::Message, 0, true);

			HANDLE hOld = ::SelectObject(hdc, _hfUnderlined);

			// Draw the text!
			wchar_t szWinText[MAX_PATH] = { L'\0' };
			::GetWindowTextW(hwnd, szWinText, MAX_PATH);
			::DrawTextW(hdc, szWinText, -1, &rect, dwDTStyle);

			::SelectObject(hdc, hOld);

			::EndPaint(hwnd, &ps);

			return 0;
		}

		case WM_SETTEXT:
		{
			LRESULT ret = ::CallWindowProcW(_oldproc, hwnd, Message, wParam, lParam);
			::InvalidateRect(hwnd, 0, 0);
			return ret;
		}
		// Provide a hand cursor when the mouse moves over us
		case WM_MOUSEMOVE:
		{
			::SetCursor(loadHandCursor());
			return TRUE;
		}

		case WM_LBUTTONDOWN:
			_clicking = true;
			break;

		case WM_LBUTTONUP:
			if (_clicking)
			{
				_clicking = false;

				action();
			}

			break;

		//Support using space to activate this object
		case WM_KEYDOWN:
			if (wParam == VK_SPACE)
				_clicking = true;
			break;

		case WM_KEYUP:
			if (wParam == VK_SPACE && _clicking)
			{
				_clicking = false;

				action();
			}
			break;

		// A standard static control returns HTTRANSPARENT here, which
		// prevents us from receiving any mouse messages. So, return
		// HTCLIENT instead.
		case WM_NCHITTEST:
			return HTCLIENT;
	}

	return ::CallWindowProcW(_oldproc, hwnd, Message, wParam, lParam);
}
