/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef BABYGRIDWRAPPER
#define BABYGRIDWRAPPER

#include "babygrid.h"
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
/*
    static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
        return (((BabyGridWrapper *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(Message, wParam, lParam));
    };
	LRESULT runProc(UINT Message, WPARAM wParam, LPARAM lParam);
*/
};

#endif //BABYGRIDWRAPPER

