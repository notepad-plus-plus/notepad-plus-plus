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
#include "Window.h"
#include "Common.h"


#define SV_HORIZONTAL		0x00000001
#define SV_VERTICAL			0x00000002
#define SV_FIXED			0x00000004
#define SV_ENABLERDBLCLK	0x00000008
#define SV_ENABLELDBLCLK	0x00000010
#define SV_RESIZEWTHPERCNT	0x00000020


#define WM_GETSPLITTER_X		(SPLITTER_USER + 1)
#define WM_GETSPLITTER_Y		(SPLITTER_USER + 2)
#define WM_DOPOPUPMENU			(SPLITTER_USER + 3)
#define WM_RESIZE_CONTAINER		(SPLITTER_USER + 4)

const int HIEGHT_MINIMAL = 15;


enum class Arrow { left, up, right, down };

enum class WH { height, width };

enum class ZONE_TYPE { bottomRight, topLeft };

enum class SplitterMode: std::uint8_t
{
    DYNAMIC, LEFT_FIX, RIGHT_FIX
};





class Splitter : public Window
{
public:
	Splitter() = default;
	virtual ~Splitter() = default;

	virtual void destroy() override;

	void resizeSpliter(RECT *pRect = NULL);
	void init(HINSTANCE hInst, HWND hPere, int splitterSize, double iSplitRatio, DWORD dwFlags);
	void rotate();

	int getPhisicalSize() const
	{
		return _splitterSize;
	}


private:
	RECT _rect = {};
	double _splitPercent = 0.;
	int _splitterSize = 0;
	bool _isDraged = false;
	bool _isLeftButtonDown = false;
	DWORD _dwFlags = 0;
	bool _isFixed = false;
	static bool _isHorizontalRegistered;
	static bool _isVerticalRegistered;
	static bool _isHorizontalFixedRegistered;
	static bool _isVerticalFixedRegistered;

	RECT _clickZone2TL = {};
	RECT _clickZone2BR = {};

	static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK spliterWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	int getClickZone(WH which);
	void adjustZoneToDraw(RECT & rc2def, ZONE_TYPE whichZone);
	void drawSplitter();
	bool isVertical() const {return (_dwFlags & SV_VERTICAL) != 0;};
	void paintArrow(HDC hdc, const RECT &rect, Arrow arrowDir);
	void gotoTopLeft();
	void gotoRightBottom();

	bool isInLeftTopZone(const POINT& p) const
	{
		return ((p.x >= _clickZone2TL.left)
			and (p.x <= _clickZone2TL.left + _clickZone2TL.right)
			and (p.y >= _clickZone2TL.top)
			and (p.y <= _clickZone2TL.top + _clickZone2TL.bottom));
	}

	bool isInRightBottomZone(const POINT& p) const
	{
		return ((p.x >= _clickZone2BR.left)
			and (p.x <= _clickZone2BR.left + _clickZone2BR.right)
			and (p.y >= _clickZone2BR.top)
			and (p.y <= _clickZone2BR.top + _clickZone2BR.bottom));
	}

	int getSplitterFixPosX() const
	{
		long result = long(::SendMessage(_hParent, WM_GETSPLITTER_X, 0, 0));
		return (LOWORD(result) - ((HIWORD(result) == static_cast<std::uint8_t>(SplitterMode::RIGHT_FIX)) ? _splitterSize : 0));
	}

	int getSplitterFixPosY() const
	{
		long result = long(::SendMessage(_hParent, WM_GETSPLITTER_Y, 0, 0));
		return (LOWORD(result) - ((HIWORD(result) == static_cast<std::uint8_t>(SplitterMode::RIGHT_FIX)) ? _splitterSize : 0));
	}
};
