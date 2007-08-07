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
#include "SysMsg.h"



void ColourPicker::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);

	_hSelf = ::CreateWindowEx(
					0,
					"Button",
					"F",
					WS_CHILD |  WS_VISIBLE,
					0, 0, 25, 25,
					_hParent,
					NULL,
					_hInst,
					(LPVOID)0);
	if (!_hSelf)
	{
		systemMessage("System Err");
		throw int(6969);
	}

    
    ::SetWindowLong(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_buttonDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(staticWinProc)));	 

}

void ColourPicker::drawSelf(HDC hDC)
{
    PAINTSTRUCT ps;
    RECT rc;

	HDC hdc = hDC?hDC:(::BeginPaint(_hSelf, &ps));
    getClientRect(rc);
    HBRUSH hbrush = ::CreateSolidBrush(_currentColour);
	FillRect(hdc, &rc, hbrush);
    ::DeleteObject(hbrush);
    ::EndPaint(_hSelf, &ps);
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
				_pColourPopup = new ColourPopup;
				_pColourPopup->init(_hInst, _hSelf);
				_pColourPopup->doDialog(p);
			}
            return TRUE;
        }

        case WM_PAINT :
        {
            drawSelf((HDC)wParam);
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
