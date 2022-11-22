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

#include <iostream>
#include <stdexcept>
#include "ColourPicker.h"
#include "ColourPopup.h"
#include "NppDarkMode.h"

void ColourPicker::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);

	_hSelf = ::CreateWindowEx(
		0,
		TEXT("Button"),
		TEXT("F"),
		WS_CHILD |  WS_VISIBLE,
		0, 0, 25, 25,
		_hParent, NULL, _hInst, (LPVOID)0);

	if (!_hSelf)
		throw std::runtime_error("ColourPicker::init : CreateWindowEx() function return null");

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_buttonDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticWinProc)));
}


void ColourPicker::destroy()
{
	delete _pColourPopup;
	_pColourPopup = NULL;
	::DestroyWindow(_hSelf);
}


void ColourPicker::drawBackground(HDC hDC)
{
	RECT rc;
	HBRUSH hbrush;

	if (!hDC)
		return;

	getClientRect(rc);
	hbrush = ::CreateSolidBrush(_currentColour);
	HGDIOBJ oldObj = ::SelectObject(hDC, hbrush);
	HPEN holdPen = nullptr;
	if (NppDarkMode::isEnabled())
	{
		holdPen = static_cast<HPEN>(::SelectObject(hDC, NppDarkMode::getEdgePen()));
	}
	::Rectangle(hDC, 0, 0, rc.right, rc.bottom);
	if (NppDarkMode::isEnabled() && holdPen)
	{
		::SelectObject(hDC, holdPen);
	}
	::SelectObject(hDC, oldObj);
	//FillRect(hDC, &rc, hbrush);
	::DeleteObject(hbrush);
}


void ColourPicker::drawForeground(HDC hDC)
{
	RECT rc;
	HBRUSH hbrush = NULL;

	if (!hDC || _isEnabled)
		return;

	int oldMode = ::SetBkMode(hDC, TRANSPARENT);
	getClientRect(rc);
	COLORREF strikeOut = RGB(0,0,0);
	if ((((_currentColour      ) & 0xFF) +
		 ((_currentColour >>  8) & 0xFF) +
		 ((_currentColour >> 16) & 0xFF)) < 200)	//check if the color is too dark, if so, use white strikeout
		strikeOut = RGB(0xFF,0xFF,0xFF);
	if (!_isEnabled)
		hbrush = ::CreateHatchBrush(HS_FDIAGONAL, strikeOut);
	HGDIOBJ oldObj = ::SelectObject(hDC, hbrush);
	::Rectangle(hDC, 0, 0, rc.right, rc.bottom);
	::SelectObject(hDC, oldObj);
	//FillRect(hDC, &rc, hbrush);
	::DeleteObject(hbrush);
	::SetBkMode(hDC, oldMode);
}


LRESULT ColourPicker::runProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			RECT rc;
			POINT p;
			Window::getClientRect(rc);
			::InflateRect(&rc, -2, -2);
			p.x = rc.left;
			p.y = rc.top + rc.bottom;
			::ClientToScreen(_hSelf, &p);

			if (!_pColourPopup)
			{
				_pColourPopup = new ColourPopup(_currentColour);
				_pColourPopup->init(_hInst, _hSelf);
				_pColourPopup->doDialog(p);
			}
			else
			{
				_pColourPopup->setColour(_currentColour);
				_pColourPopup->doDialog(p);
				_pColourPopup->display(true);
			}
			return TRUE;
		}

		case WM_RBUTTONDOWN:
		{
			_isEnabled = !_isEnabled;
			redraw();
			::SendMessage(_hParent, WM_COMMAND, MAKELONG(0, CPN_COLOURPICKED), reinterpret_cast<LPARAM>(_hSelf));
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			if (_pColourPopup)
			{
				::SendMessage(_pColourPopup->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			}
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			HDC dc = (HDC)wParam;
			drawBackground(dc);
			return TRUE;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(_hSelf, &ps);
			drawForeground(dc);
			::EndPaint(_hSelf, &ps);
			return TRUE;
		}

		case WM_PICKUP_COLOR:
		{
			_currentColour = (COLORREF)wParam;
			redraw();

			_pColourPopup->display(false);
			::SendMessage(_hParent, WM_COMMAND, MAKELONG(0, CPN_COLOURPICKED), reinterpret_cast<LPARAM>(_hSelf));
			return TRUE;
		}

		case WM_ENABLE:
		{
			if ((BOOL)wParam == FALSE)
			{
				_currentColour = NppDarkMode::isEnabled() ? NppDarkMode::getDarkerBackgroundColor() : ::GetSysColor(COLOR_3DFACE);
				redraw();
			}
			return TRUE;
		}

		case WM_PICKUP_CANCEL:
		{
			_pColourPopup->display(false);
			return TRUE;
		}

		default:
			return ::CallWindowProc(_buttonDefaultProc, _hSelf, Message, wParam, lParam);
	}

	return FALSE;
}
