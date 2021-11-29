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

#include <vector>
#include <windows.h>

class ScintillaEditView;

class ScintillaCtrls {
public :
	void init(HINSTANCE hInst, HWND hNpp) {
		_hInst = hInst;
		_hParent = hNpp;
	};

	HWND createSintilla(HWND hParent);
	ScintillaEditView * getScintillaEditViewFrom(HWND handle2Find);
	//bool destroyScintilla(HWND handle2Destroy);
	void destroy();
	
private:
	std::vector<ScintillaEditView *> _scintVector;
	HINSTANCE _hInst = nullptr;
	HWND _hParent = nullptr;

	int getIndexFrom(HWND handle2Find);
};

