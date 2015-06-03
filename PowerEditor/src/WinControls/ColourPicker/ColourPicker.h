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


#ifndef COLOUR_PICKER_H
#define COLOUR_PICKER_H

#include "Window.h"

class ColourPopup;

#define CPN_COLOURPICKED (BN_CLICKED)

class ColourPicker : public Window
{
public :
	ColourPicker() : Window(),  _currentColour(RGB(0xFF, 0x00, 0x00)), _pColourPopup(NULL), _isEnabled(true) {};
    ~ColourPicker(){};
	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy();
    void setColour(COLORREF c) {
        _currentColour = c;
    };

	COLORREF getColour() const {return _currentColour;};
	bool isEnabled() {return _isEnabled;};
	void setEnabled(bool enabled) {_isEnabled = enabled;};

private :
	COLORREF _currentColour;
    WNDPROC _buttonDefaultProc;
	ColourPopup *_pColourPopup;
	bool _isEnabled;

    static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
        return (((ColourPicker *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(Message, wParam, lParam));
    };
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void drawForeground(HDC hDC);
	void drawBackground(HDC hDC);
};

#endif // COLOUR_PICKER_H
