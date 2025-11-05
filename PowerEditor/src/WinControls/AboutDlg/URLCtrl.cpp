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


#include "URLCtrl.h"

#include <commctrl.h>

#include "Common.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "dpiManagerV2.h"


void URLCtrl::create(HWND itemHandle, const wchar_t * link, COLORREF linkColor)
{
	// turn on notify style
    ::SetWindowLongPtr(itemHandle, GWL_STYLE, ::GetWindowLongPtr(itemHandle, GWL_STYLE) | SS_NOTIFY);

	// set the URL text (not the display text)
	if (link)
		_URL = link;

	// set the hyperlink colour
    _linkColor = linkColor;

	// set the visited colour
	_visitedColor = RGB(128,0,128);

	// subclass the static control
	::SetWindowSubclass(itemHandle, URLCtrlProc, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

	// save hwnd
	_hSelf = itemHandle;

	loadHandCursor();
}
void URLCtrl::create(HWND itemHandle, int cmd, HWND msgDest)
{
	// turn on notify style
    ::SetWindowLongPtr(itemHandle, GWL_STYLE, ::GetWindowLongPtr(itemHandle, GWL_STYLE) | SS_NOTIFY);

	_cmdID = cmd;
	_msgDest = msgDest;

	// set the hyperlink colour
    _linkColor = RGB(0,0,255);

	// subclass the static control
	::SetWindowSubclass(itemHandle, URLCtrlProc, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

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
	{
		_hCursor = static_cast<HCURSOR>(::LoadImage(nullptr, MAKEINTRESOURCE(OCR_HAND), IMAGE_CURSOR, SM_CXCURSOR, SM_CYCURSOR, LR_SHARED));
	}

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
		_linkColor = _visitedColor;

		::InvalidateRect(_hSelf, 0, 0);
		::UpdateWindow(_hSelf);

		// Open a browser
		if (!_URL.empty())
		{
			::ShellExecute(NULL, L"open", _URL.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			wchar_t szWinText[MAX_PATH] = { '\0' };
			::GetWindowText(_hSelf, szWinText, MAX_PATH);
			::ShellExecute(NULL, L"open", szWinText, NULL, NULL, SW_SHOWNORMAL);
		}
	}
}

LRESULT CALLBACK URLCtrl::URLCtrlProc(
	HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto* pRefData = reinterpret_cast<URLCtrl*>(dwRefData);

	switch (Message)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hwnd, URLCtrlProc, uIdSubclass);
			break;
		}

	    // Paint the static control using our custom
	    // colours, and with an underline text style
	    case WM_PAINT:
        {
			DWORD dwStyle = static_cast<DWORD>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		    DWORD dwDTStyle = DT_SINGLELINE;

		    //Test if centered horizontally or vertically
		    if (dwStyle & SS_CENTER)	     dwDTStyle |= DT_CENTER;
		    if (dwStyle & SS_RIGHT)		 dwDTStyle |= DT_RIGHT;
		    if (dwStyle & SS_CENTERIMAGE) dwDTStyle |= DT_VCENTER;

			RECT rect{};
            ::GetClientRect(hwnd, &rect);

			PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);

			if ((pRefData->_linkColor == pRefData->_visitedColor) || (pRefData->_linkColor == NppDarkMode::getDarkerTextColor()))
			{
				pRefData->_linkColor = NppDarkMode::isEnabled() ? NppDarkMode::getDarkerTextColor() : pRefData->_visitedColor;
				::SetTextColor(hdc, pRefData->_linkColor);
			}
			else if (NppDarkMode::isEnabled())
			{
				::SetTextColor(hdc, NppDarkMode::getLinkTextColor());
			}
			else
			{
				::SetTextColor(hdc, pRefData->_linkColor);
			}

            ::SetBkColor(hdc, getCtrlBgColor(GetParent(hwnd))); ///*::GetSysColor(COLOR_3DFACE)*/);

			// Create an underline font
			if (pRefData->_hfUnderlined == nullptr)
			{
				// Get the default GUI font
				LOGFONT lf{ DPIManagerV2::getDefaultGUIFontForDpi(::GetParent(hwnd)) };
				lf.lfUnderline = TRUE;

				// Create a new font
				pRefData->_hfUnderlined = ::CreateFontIndirect(&lf);
			}

			auto hOld = static_cast<HFONT>(::SelectObject(hdc, pRefData->_hfUnderlined));

		    // Draw the text!
			wchar_t szWinText[MAX_PATH] = { '\0' };
            ::GetWindowText(hwnd, szWinText, MAX_PATH);
            ::DrawText(hdc, szWinText, -1, &rect, dwDTStyle);

            ::SelectObject(hdc, hOld);

            ::EndPaint(hwnd, &ps);

		    return 0;
        }

		case WM_DPICHANGED_AFTERPARENT:
		{
			pRefData->destroy();
			return 0;
		}

		case WM_SETTEXT:
		{
			const LRESULT ret = ::DefSubclassProc(hwnd, Message, wParam, lParam);
			::InvalidateRect(hwnd, 0, 0);
			return ret;
		}
		// Provide a hand cursor when the mouse moves over us
		//case WM_SETCURSOR:
		case WM_MOUSEMOVE:
		{
			::SetCursor(pRefData->loadHandCursor());
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			pRefData->_clicking = true;
			break;
		}

		case WM_LBUTTONUP:
		{
			if (pRefData->_clicking)
			{
				pRefData->_clicking = false;
				pRefData->action();
			}
			break;
		}

		//Support using space to activate this object
		case WM_KEYDOWN:
		{
			if (wParam == VK_SPACE)
				pRefData->_clicking = true;

			break;
		}

		case WM_KEYUP:
		{
			if (wParam == VK_SPACE && pRefData->_clicking)
			{
				pRefData->_clicking = false;
				pRefData->action();
			}
			break;
		}

		// A standard static control returns HTTRANSPARENT here, which
		// prevents us from receiving any mouse messages. So, return
		// HTCLIENT instead.
		case WM_NCHITTEST:
		{
			return HTCLIENT;
		}

		case WM_DESTROY:
		{
			pRefData->destroy();
			break;
		}
	}
	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
}
