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


#ifndef SPLITTER_CONTAINER_H
#define SPLITTER_CONTAINER_H

#ifndef SPLITTER_H
#include "Splitter.h"
#endif //SPLITTER_H

#ifndef MENUCMDID_H
#include "menuCmdID.h"
#endif //MENUCMDID_H

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
		_pWin0->redraw(true);
		_pWin1->redraw(true);
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
