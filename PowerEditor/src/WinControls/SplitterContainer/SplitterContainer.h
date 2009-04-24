//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef SPLITTER_CONTAINER_H
#define SPLITTER_CONTAINER_H

#include "Window.h"
#include "Splitter.h"

#define SPC_CLASS_NAME TEXT("splitterContainer")

#define ROTATION_A_GAUCHE 2000
#define ROTATION_A_DROITE 2001

typedef bool DIRECTION;
const bool LEFT = true;
const bool RIGHT = false;



class SplitterContainer : public Window
{
public :
	SplitterContainer(): Window(), _x(0), _y(0), _hPopupMenu(NULL),
		_dwSplitterStyle(SV_ENABLERDBLCLK | SV_ENABLELDBLCLK | SV_RESIZEWTHPERCNT){
	};
	~SplitterContainer(){};
	void create(Window *pWin0, Window *pWin1, int splitterSize = 4,
				SplitterMode mode = DYNAMIC, int ratio = 50,  bool _isVertical = true);

	void destroy() {
		if (_hPopupMenu)
			::DestroyMenu(_hPopupMenu);
		_splitter.destroy();
		::DestroyWindow(_hSelf);
	};
	void reSizeTo(RECT & rc) {
		_x = rc.left;
		_y = rc.top;
		::MoveWindow(_hSelf, _x, _y, rc.right, rc.bottom, FALSE);
		_splitter.resizeSpliter();
	};
	virtual void display(bool toShow = true) const {
		Window::display(toShow);
		
		_pWin0->display(toShow);
		_pWin1->display(toShow);
		_splitter.display(toShow);
	};
	virtual void redraw() const {
		_pWin0->redraw();
		_pWin1->redraw();
	};

    void setWin0(Window *pWin) {
        _pWin0 = pWin;

    };

    void setWin1(Window *pWin) {
        _pWin1 = pWin;
    };

	bool isVertical() const {
		return ((_dwSplitterStyle & SV_VERTICAL) != 0);
	};
private :
	Window *_pWin0; // left or top window
	Window *_pWin1; // right or bottom window

	Splitter _splitter;
	int _splitterSize;
	int _ratio;
	int _x, _y;
	HMENU _hPopupMenu;
	DWORD _dwSplitterStyle;

	SplitterMode _splitterMode;
	static bool _isRegistered;

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void rotateTo(DIRECTION direction);

};

#endif //SPLITTER_CONTAINER_H
