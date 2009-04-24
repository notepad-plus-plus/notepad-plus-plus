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

#include "StatusBar.h"
#include "Common.h"

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
		systemMessage(TEXT("System Err"));
		throw int(9);
	}

    _nbParts = nbParts;
    _partWidthArray = new int[_nbParts];

	// Set the default width
	for (int i = 0 ; i < _nbParts ; i++)
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
