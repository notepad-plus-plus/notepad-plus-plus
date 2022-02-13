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

#include <windows.h>
#include <commctrl.h>
#include "Window.h"

class ToolTip : public Window
{
public:
	ToolTip() = default;

	void destroy() {
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hParent);
	void Show(RECT rectTitle, const TCHAR* pszTitleText, int iXOff = 0, int iWidthOff = 0);

protected:
	WNDPROC		_defaultProc = nullptr;
	BOOL		_bTrackMouse = FALSE;
	TOOLINFO	_ti = {};

    static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
        return (((ToolTip *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(Message, wParam, lParam));
    };
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
};

