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

#ifndef COLOUR_PICKER_H
#define COLOUR_PICKER_H

#include "Window.h"
#include "ColourPopup.h"

//#define CP_CLASS_NAME "colourPickerButton"
#define CPN_COLOURPICKED (BN_CLICKED)

class ColourPicker : public Window
{
public :
	ColourPicker() : Window(),  _currentColour(RGB(0xFF, 0x00, 0x00)), _pColourPopup(NULL), _isEnabled(true) {};
    ~ColourPicker(){};
	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy() {
		DestroyWindow(_hSelf);
	};
    void setColour(COLORREF c) {
        _currentColour = c;
        //drawSelf();
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
