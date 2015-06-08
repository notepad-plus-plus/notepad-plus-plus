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


#include <iostream>
#include "ToolTip.h"

INT_PTR CALLBACK dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

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

		::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
		_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticWinProc));
	}
}


void ToolTip::Show(RECT rectTitle, const TCHAR * pszTitle, int iXOff, int iWidthOff)
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
	::SendMessage(_hSelf, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &_ti);
	::SendMessage(_hSelf, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(_ti.rect.left + iXOff, _ti.rect.top + iWidthOff));
	::SendMessage(_hSelf, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO) &_ti);
}


LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}

