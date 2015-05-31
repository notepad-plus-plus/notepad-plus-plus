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


#ifndef BABYGRIDWRAPPER
#define BABYGRIDWRAPPER

#ifndef BABYGRID_H
#include "babygrid.h"
#endif// BABYGRID_H

#include "Window.h"

class BabyGridWrapper : public Window
{
public :
	BabyGridWrapper() : Window(){};
    ~BabyGridWrapper(){};
	virtual void init(HINSTANCE hInst, HWND parent, int id);
	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};
	void setLineColNumber(size_t nbRow, size_t nbCol) {
		::SendMessage(_hSelf, BGM_SETGRIDDIM, nbRow, nbCol);
	};

	void setCursorColour(COLORREF coulour) {
		::SendMessage(_hSelf, BGM_SETCURSORCOLOR, (UINT)coulour, 0);
	};

	void hideCursor() {
		setCursorColour(RGB(0, 0, 0));
	};

	void setColsNumbered(bool isNumbered = true) {
		::SendMessage(_hSelf, BGM_SETCOLSNUMBERED, isNumbered?TRUE:FALSE, 0);
	}

	void setText(size_t row, size_t col, const TCHAR *text) {
		_BGCELL cell;
		cell.row = row;
		cell.col = col;
		::SendMessage(_hSelf, BGM_SETCELLDATA, (UINT)&cell, (long)text);
	};

	void makeColAutoWidth(bool autoWidth = true) {
		::SendMessage(_hSelf, BGM_SETCOLAUTOWIDTH, autoWidth?TRUE:FALSE, 0);
	};

	int getSelectedRow() {
		return ::SendMessage(_hSelf, BGM_GETROW, 0, 0);
	};

	void deleteCell(int row, int col) {
		_BGCELL cell;
		cell.row = row;
		cell.col = col;
		::SendMessage(_hSelf, BGM_DELETECELL, (UINT)&cell, 0);
	};

	void setColWidth(unsigned int col, unsigned int width) {
		::SendMessage(_hSelf, BGM_SETCOLWIDTH, col, width);
	};

	void clear() {
		::SendMessage(_hSelf, BGM_CLEARGRID, 0, 0);
	};

private :
	static bool _isRegistered;
};

#endif //BABYGRIDWRAPPER

