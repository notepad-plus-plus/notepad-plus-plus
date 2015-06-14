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


#include <windows.h>
#include <commctrl.h>
#include "StatusBar.h"

//#define IDC_STATUSBAR 789

const int defaultPartWidth = 5;

void StatusBar::init(HINSTANCE hInst, HWND hPere, int nbParts)
{
	Window::init(hInst, hPere);
    InitCommonControls();

	_hSelf = //CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS, NULL, _hParent, IDC_STATUSBAR);
	::CreateWindowEx(
	               0,
	               STATUSCLASSNAME,
	               TEXT(""),
	               WS_CHILD | SBARS_SIZEGRIP ,
	               0, 0, 0, 0,
	               _hParent,
				   NULL,
	               _hInst,
	               0);

	if (!_hSelf)
	{
		throw std::runtime_error("StatusBar::init : CreateWindowEx() function return null");
	}

    _nbParts = nbParts;
    _partWidthArray = new int[_nbParts];

	// Set the default width
	for (int i = 0 ; i < _nbParts ; ++i)
		_partWidthArray[i] = defaultPartWidth;

    // Allocate an array for holding the right edge coordinates.
    _hloc = ::LocalAlloc(LHND, sizeof(int) * _nbParts);
    _lpParts = (LPINT)::LocalLock(_hloc);

	RECT rc;
	::GetClientRect(_hParent, &rc);
	adjustParts(rc.right);
}

void StatusBar::adjustParts(int clientWidth)
{
    // Calculate the right edge coordinate for each part, and
    // copy the coordinates to the array.
    int nWidth = clientWidth - 20;
    for (int i = _nbParts - 1 ; i >= 0 ; i--) 
    { 
       _lpParts[i] = nWidth;
       nWidth -= _partWidthArray[i];
    }

    // Tell the status bar to create the window parts.
    ::SendMessage(_hSelf, SB_SETPARTS, (WPARAM)_nbParts, (LPARAM)_lpParts);
}

bool StatusBar::setText(const TCHAR *str, int whichPart)
{
	if (whichPart > _nbParts)
		return false;
	_lastSetText = str;
	return (::SendMessage(_hSelf, SB_SETTEXT, whichPart, (LPARAM)_lastSetText.c_str()) == TRUE);
}

bool StatusBar::setOwnerDrawText(const TCHAR *str)
{
	_lastSetText = str;
	return (::SendMessage(_hSelf, SB_SETTEXT, SBT_OWNERDRAW, (LPARAM)_lastSetText.c_str()) == TRUE);
}
