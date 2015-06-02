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


#ifndef URLCTRL_INCLUDED
#define URLCTRL_INCLUDED

#include "Window.h"
#include "Common.h"

class URLCtrl : public Window {
public:
    URLCtrl():_hfUnderlined(0),_hCursor(0), _msgDest(NULL), _cmdID(0), _oldproc(NULL), \
		_linkColor(), _visitedColor(), _clicking(false), _URL(TEXT("")){};

    void create(HWND itemHandle, TCHAR * link, COLORREF linkColor = RGB(0,0,255));
	void create(HWND itemHandle, int cmd, HWND msgDest = NULL);
    void destroy();
private:
	void action();
protected :
    generic_string _URL;
    HFONT	_hfUnderlined;
    HCURSOR	_hCursor;

	HWND _msgDest;
	unsigned long _cmdID;

    WNDPROC  _oldproc;
    COLORREF _linkColor;			
    COLORREF _visitedColor;
    bool  _clicking;

    static LRESULT CALLBACK URLCtrlProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
        return ((URLCtrl *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam);
    };
    LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
};

#endif //URLCTRL_INCLUDED
