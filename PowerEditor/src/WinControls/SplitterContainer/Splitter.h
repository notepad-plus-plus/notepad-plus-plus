// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef SPLITTER_H
#define SPLITTER_H

#ifndef RESOURCE_H
#include "resource.h"
#endif //RESOURCE_H

#include "Window.h"

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

enum Arrow {ARROW_LEFT, ARROW_UP, ARROW_RIGHT, ARROW_DOWN};

typedef bool WH;
const bool WIDTH = true;
const bool HEIGHT = false;

typedef bool ZONE_TYPE;
const bool TOP_LEFT = true;
const bool BOTTOM_RIGHT = false;

enum SplitterMode {
    DYNAMIC, LEFT_FIX, RIGHT_FIX
};

class Splitter : public Window
{
public:	
	Splitter();
	~Splitter(){};
	void destroy() {
		::DestroyWindow(_hSelf);
	};
	void resizeSpliter(RECT *pRect = NULL);
	void init(HINSTANCE hInst, HWND hPere, int splitterSize,
			int iSplitRatio, DWORD dwFlags);
	void rotate();
	int getPhisicalSize() const {
		return _spiltterSize;
	};

private:
	RECT _rect;
	int _splitPercent;
	int _spiltterSize;
	bool _isDraged;
	DWORD _dwFlags;
	bool _isFixed;
	static bool _isHorizontalRegistered;
	static bool _isVerticalRegistered;
    static bool _isHorizontalFixedRegistered;
    static bool _isVerticalFixedRegistered;

	RECT _clickZone2TL, _clickZone2BR;

	static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK spliterWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    int getClickZone(WH which);
	void adjustZoneToDraw(RECT & rc2def, ZONE_TYPE whichZone);	 
	void drawSplitter();
	bool isVertical() const {return (_dwFlags & SV_VERTICAL) != 0;};
	void paintArrow(HDC hdc, const RECT &rect, Arrow arrowDir);
	void gotoTopLeft();
	void gotoRightBouuom();

	bool isInLeftTopZone(const POINT &p) const {
		return (((p.x >= _clickZone2TL.left) && (p.x <= _clickZone2TL.left + _clickZone2TL.right)) &&
			(p.y >= _clickZone2TL.top) && (p.y <= _clickZone2TL.top + _clickZone2TL.bottom));
	};

	bool isInRightBottomZone(const POINT &p) const {
		return (((p.x >= _clickZone2BR.left) && 
			(p.x <= _clickZone2BR.left + _clickZone2BR.right)) &&
			(p.y >= _clickZone2BR.top) && 
			(p.y <= _clickZone2BR.top + _clickZone2BR.bottom));
	};
	
	int getSplitterFixPosX() {
		long result = long(::SendMessage(_hParent, WM_GETSPLITTER_X, 0, 0));
		return (LOWORD(result) - ((HIWORD(result) == RIGHT_FIX) ? _spiltterSize : 0));
	};

	int getSplitterFixPosY() {
		long result = long(::SendMessage(_hParent, WM_GETSPLITTER_Y, 0, 0));
		return (LOWORD(result) - ((HIWORD(result) == RIGHT_FIX) ? _spiltterSize : 0));
	};
};
#endif //SPLITTER_H
