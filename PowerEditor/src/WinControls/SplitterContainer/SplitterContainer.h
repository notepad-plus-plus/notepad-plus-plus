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
#include "Splitter.h"
#include "menuCmdID.h"


#define SPC_CLASS_NAME TEXT("splitterContainer")

#define ROTATION_LEFT 2000
#define ROTATION_RIGHT 2001


enum class DIRECTION
{
	RIGHT,
	LEFT
};




class SplitterContainer : public Window
{
public :
	virtual ~SplitterContainer() = default;

	void create(Window *pWin0, Window *pWin1, int splitterSize,
		SplitterMode mode = SplitterMode::DYNAMIC, int ratio = 50, bool _isVertical = true);

	void destroy();

	void reSizeTo(RECT & rc);

	virtual void display(bool toShow = true) const;

	virtual void redraw(bool forceUpdate = false) const;

	void setWin0(Window* pWin)
	{
		_pWin0 = pWin;
	}

	void setWin1(Window* pWin)
	{
		_pWin1 = pWin;
	}

	bool isVertical() const
	{
		return ((_dwSplitterStyle & SV_VERTICAL) != 0);
	}


private :
	Window* _pWin0 = nullptr; // left or top window
	Window* _pWin1 = nullptr; // right or bottom window

	Splitter _splitter;
	int _splitterSize = 0;
	int _ratio = 0;
	int _x = 0;
	int _y = 0;
	HMENU _hPopupMenu = NULL;
	DWORD _dwSplitterStyle = (SV_ENABLERDBLCLK | SV_ENABLELDBLCLK | SV_RESIZEWTHPERCNT);

	SplitterMode _splitterMode = SplitterMode::DYNAMIC;
	static bool _isRegistered;

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void rotateTo(DIRECTION direction);
};
