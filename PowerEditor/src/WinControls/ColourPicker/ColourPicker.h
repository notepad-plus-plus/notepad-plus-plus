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


#pragma once

#include "Window.h"

class ColourPopup;

#define CPN_COLOURPICKED  (BN_CLICKED)

class ColourPicker : public Window
{
public :
	ColourPicker() = default;
    ~ColourPicker() = default;
	virtual void init(HINSTANCE hInst, HWND parent);
	virtual void destroy();
    void setColour(COLORREF c) {
        _currentColour = c;
    };

	COLORREF getColour() const {return _currentColour;};
	bool isEnabled() {return _isEnabled;};
	void setEnabled(bool enabled) {_isEnabled = enabled;};
	void disableRightClick() {_disableRightClick = true;};

private :
	COLORREF _currentColour = RGB(0xFF, 0x00, 0x00);
    WNDPROC _buttonDefaultProc = nullptr;
	ColourPopup *_pColourPopup = nullptr;
	bool _isEnabled = true;
	bool _disableRightClick = false;

    static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
        return (((ColourPicker *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(Message, wParam, lParam));
    };
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void drawForeground(HDC hDC);
	void drawBackground(HDC hDC);
};

