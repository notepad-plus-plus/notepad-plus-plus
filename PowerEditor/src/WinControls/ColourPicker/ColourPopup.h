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


#ifndef COLOUR_POPUP_H
#define COLOUR_POPUP_H

#ifndef COLOUR_POPUP_RESOURCE_H
#include "ColourPopupResource.h"
#endif //COLOUR_POPUP_RESOURCE_H

#ifndef RESOURCE_H
#include "resource.h"
#endif //RESOURCE_H

#include "Window.h"

#define WM_PICKUP_COLOR (COLOURPOPUP_USER + 1)
#define WM_PICKUP_CANCEL (COLOURPOPUP_USER + 2)

class ColourPopup : public Window
{
public :
    ColourPopup() : Window()/*, isColourChooserLaunched(false)*/ {};
	ColourPopup(COLORREF defaultColor) : Window(), /* isColourChooserLaunched(false), */ _colour(defaultColor) {};
	~ColourPopup(){};
	
	bool isCreated() const {
		return (_hSelf != NULL);
	};
	
	void create(int dialogID);

        void doDialog(POINT p) {
            if (!isCreated())
                create(IDD_COLOUR_POPUP);
            ::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
	};

    virtual void destroy() {
	    ::DestroyWindow(_hSelf);
	};

	void setColour(COLORREF c) {
        _colour = c;
    };

    COLORREF getSelColour(){return _colour;};

private :
	RECT _rc;
    COLORREF _colour;
	//bool isColourChooserLaunched;

	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //COLOUR_POPUP_H


