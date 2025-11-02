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

#include "resource.h"

#include "StaticDialog.h"

#define WM_PICKUP_COLOR (COLOURPOPUP_USER + 1)

class ColourPopup : public StaticDialog
{
public:
	ColourPopup() = default;
	explicit ColourPopup(COLORREF defaultColor) : _colour(defaultColor) {}
	~ColourPopup() override {}

	void createColorPopup();
	void doDialog(POINT p);

	void destroy() override {
		::DestroyWindow(_hSelf);
	}

private:
	COLORREF _colour = RGB(0xFF, 0xFF, 0xFF);

	static intptr_t CALLBACK dlgClrPopupProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	static uintptr_t CALLBACK chooseColorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
