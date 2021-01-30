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
#include "ToolTip.h"
#include "Common.h"

void ToolTip::init(HINSTANCE hInst, HWND hParent)
{
	if (_hSelf == NULL)
	{
		Window::init(hInst, hParent);

		_hSelf = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL );
		if (!_hSelf)
		{
			throw std::runtime_error("ToolTip::init : CreateWindowEx() function return null");
		}

		::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticWinProc)));
	}
}


void ToolTip::show(const TCHAR * pszTitle, int iXOff, int iWidthOff, const RECT& rectTitle)
{
	if (isVisible())
		destroy();

	if (lstrlen(pszTitle) == 0)
		return;

	// INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
	_ti.cbSize		= sizeof(TOOLINFO);
	_ti.uFlags		= TTF_TRACK | TTF_ABSOLUTE;
	_ti.hwnd		= ::GetParent(_hParent);
	_ti.hinst		= _hInst;
	_ti.uId			= 0;

	_ti.rect.left	= rectTitle.left;
	_ti.rect.top	= rectTitle.top;
	_ti.rect.right	= rectTitle.right;
	_ti.rect.bottom	= rectTitle.bottom;

	HFONT	_hFont = (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0);	
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), TRUE);

	// Bleuargh...  const_cast.  Will have to do for now.
	_ti.lpszText  = const_cast<TCHAR *>(pszTitle);
	::SendMessage(_hSelf, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&_ti));
	::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, MAKELONG(_ti.rect.left + iXOff, _ti.rect.top + iWidthOff));
	::SendMessage(_hSelf, TTM_TRACKACTIVATE, true, reinterpret_cast<LPARAM>(&_ti));

	reposition({ iXOff, iWidthOff });
}

void ToolTip::reposition(const POINT& pt)
{
	LRESULT size = ::SendMessage(_hSelf, TTM_GETBUBBLESIZE, 0, reinterpret_cast<LPARAM>(&_ti));
	LONG width = LOWORD(size);
	LONG height = HIWORD(size);

	// Vertical offset so that the tooltip is below the cursor.
	// This is roughly the height of the visible cursor area.
	// To get more precise value, one would need to analyze DIB of the cursor info.
	static const int kCursorHeight = 20;

	RECT rc;
	rc.left = pt.x;
	rc.top = pt.y + kCursorHeight;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	// Fit it within the display
	RECT displayRc = getDisplayRect(_hSelf);

	int dx = 0;
	if (rc.left < displayRc.left)
		dx = width;
	else if (rc.right > displayRc.right)
		dx = -width;
	int dy = 0;
	if (rc.top < displayRc.top)
		dy = height;
	else if (rc.bottom > displayRc.bottom)
		dy = -height;

	OffsetRect(&rc, dx, dy);

	::SetWindowPos(_hSelf, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void ToolTip::showAtCursor(const TCHAR* pszTitleText)
{
	POINT pt{};
	if (::GetCursorPos(&pt))
		show(pszTitleText, pt.x, pt.y);
}

LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}

void ToolTip::trackMouse(HWND hwnd, DWORD flags)
{
	TRACKMOUSEEVENT tme{};
	tme.cbSize = sizeof(tme);
	tme.hwndTrack = hwnd;
	tme.dwFlags = flags;
	tme.dwHoverTime = 1000;
	_bTrackMouse = _TrackMouseEvent(&tme);
}
