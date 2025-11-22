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

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

class TaskList : public Window
{
public:
	TaskList() : Window() {
		_rc.left = 0;
		_rc.top = 0;
		_rc.right = 150;
		_rc.bottom = 0;
	}

	~TaskList() override = default;
	using Window::init;
	void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst, int nbItem, int index2set);
	void destroy() override;
	void setFont(int fontSize, const wchar_t* fontName = nullptr);
	void destroyFont();
	RECT adjustSize();
	int getCurrentIndex() const {return _currentIndex;}
	int updateCurrentIndex();

	HIMAGELIST getImgLst() const {
		return ListView_GetImageList(_hSelf, LVSIL_SMALL);
	}

	HFONT GetFontSelected() const { return _hFontSelected; }

protected:
	HFONT _hFont = nullptr;
	HFONT _hFontSelected = nullptr;
	int _nbItem = 0;
	int _currentIndex = 0;
	RECT _rc = {};

private:
	void moveSelection(int direction);
	static LRESULT CALLBACK TaskListSelectProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};
