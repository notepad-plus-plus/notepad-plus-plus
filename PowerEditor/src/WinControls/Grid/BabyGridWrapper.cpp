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


#include "BabyGridWrapper.h"

const wchar_t* babyGridClassName = L"BABYGRID";

bool BabyGridWrapper::_isRegistered = false;

void BabyGridWrapper::init(HINSTANCE hInst, HWND parent, int16_t id)
{
	Window::init(hInst, parent);

	if (!_isRegistered)
		RegisterGridClass(_hInst);
 
	_hSelf = ::CreateWindowEx(0,
					babyGridClassName,
					L"",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
					CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
					_hParent,
					reinterpret_cast<HMENU>(id),
					_hInst,
					NULL);

	_isRegistered = true;
}
