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
#include "ColourPopupResource.h"
#include "resource.h"
#include "Window.h"

#define WM_PICKUP_COLOR  (COLOURPOPUP_USER + 1)
#define WM_PICKUP_CANCEL (COLOURPOPUP_USER + 2)


class ColourPopup : public Window
{
public :
	ColourPopup() = default;
	explicit ColourPopup(COLORREF defaultColor) : _colour(defaultColor) {}
	virtual ~ColourPopup() {}

	bool isCreated() const
	{
		return (_hSelf != NULL);
	}

	void create(int dialogID);

	void doDialog(POINT p)
	{
		if (!isCreated())
			create(IDD_COLOUR_POPUP);
		::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
	}

	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
	}

	void setColour(COLORREF c)
	{
		_colour = c;
	}

	COLORREF getSelColour(){return _colour;};

private :
	RECT _rc = {};
	COLORREF _colour = RGB(0xFF, 0xFF, 0xFF);

	static intptr_t CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};
