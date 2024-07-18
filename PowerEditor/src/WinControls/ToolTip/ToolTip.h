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

	void destroy() override {
		if (_hSelf != nullptr)
		{
			::DestroyWindow(_hSelf);
			_hSelf = nullptr;
		}
	};

	void init(HINSTANCE hInst, HWND hParent) override;
	void Show(RECT rectTitle, const wchar_t* pszTitleText, int iXOff = 0, int iWidthOff = 0);

protected:
	BOOL		_bTrackMouse = FALSE;
	TOOLINFO	_ti = {};
};
