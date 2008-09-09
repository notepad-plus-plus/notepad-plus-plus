/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ColourPicker.h"
#include "Common.h"


void ColourPicker::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);

	_hSelf = ::CreateWindowEx(
					0,
					TEXT("Button"),
					TEXT("F"),
					WS_CHILD |  WS_VISIBLE,
					0, 0, 25, 25,
					_hParent,
					NULL,
					_hInst,
					(LPVOID)0);
	if (!_hSelf)
	{
		systemMessage(TEXT("System Err"));
		throw int(6969);
	}

    
    ::SetWindowLongPtr(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_buttonDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(staticWinProc)));	 

}

void ColourPicker::drawBackground(HDC hDC)
{
    RECT rc;
	HBRUSH hbrush;

	if(!hDC)
		return;

    getClientRect(rc);
	hbrush = ::CreateSolidBrush(_currentColour);
	HGDIOBJ oldObj = ::SelectObject(hDC, hbrush);
	::Rectangle(hDC, 0, 0, rc.right, rc.bottom);
	::SelectObject(hDC, oldObj);
	//FillRect(hDC, &rc, hbrush);
    ::DeleteObject(hbrush);
}

void ColourPicker::drawForeground(HDC hDC)
{
    RECT rc;
	HBRUSH hbrush;

	if(!hDC || _isEnabled)
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
        case WM_LBUTTONDBLCLK :
        case WM_LBUTTONDOWN :
        {
			if (!_pColourPopup)
			{
				RECT rc;
				POINT p;
				
				Window::getClientRect(rc);
				::InflateRect(&rc, -2, -2);
				p.x = rc.left;
				p.y = rc.top + rc.bottom;

				::ClientToScreen(_hSelf, &p);
				_pColourPopup = new ColourPopup(_currentColour);
				_pColourPopup->init(_hInst, _hSelf);
				_pColourPopup->doDialog(p);
			}
            return TRUE;
        }
		case WM_RBUTTONDOWN:
		{
			_isEnabled = !_isEnabled;
			redraw();
			::SendMessage(_hParent, WM_COMMAND, MAKELONG(0, CPN_COLOURPICKED), (LPARAM)_hSelf);
			break;
		}

		case WM_ERASEBKGND:
		{
			HDC dc = (HDC)wParam;
			drawBackground(dc);
			return TRUE;
			break;
		}

        case WM_PAINT :
        {
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(_hSelf, &ps);
            drawForeground(dc);
			::EndPaint(_hSelf, &ps);
            return TRUE;
        }

        case WM_PICKUP_COLOR :
        {
            _currentColour = (COLORREF)wParam;
            redraw();

			_pColourPopup->destroy();
            delete _pColourPopup;
			_pColourPopup = NULL;
			::SendMessage(_hParent, WM_COMMAND, MAKELONG(0, CPN_COLOURPICKED), (LPARAM)_hSelf);
            return TRUE;
        }

        case WM_ENABLE :
        {
            if ((BOOL)wParam == FALSE)
            {
                _currentColour = ::GetSysColor(COLOR_3DFACE);
                redraw();
            }
            return TRUE;
        }

		case WM_PICKUP_CANCEL :
        case WM_DESTROY :
        {
            if (_pColourPopup)
            {
				_pColourPopup->destroy();
                delete _pColourPopup;
				_pColourPopup = NULL;

                return TRUE;
            }
            break;
        }

		default :
			return ::CallWindowProc(_buttonDefaultProc, _hSelf, Message, wParam, lParam);
	}
	return FALSE;
}
