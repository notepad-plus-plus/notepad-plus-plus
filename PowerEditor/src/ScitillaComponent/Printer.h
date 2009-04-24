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

#ifndef PRINTER_H
#define PRINTER_H

#include <windows.h>
#include "ScintillaEditView.h"
#include "RunDlg.h"
#include "Parameters.h"

struct RangeToFormat {
	HDC hdc;
	HDC hdcTarget;
	RECT rc;
	RECT rcPage;
	CharacterRange chrg;
};

class Printer
{
public :
	Printer(){};
	void init(HINSTANCE hInst, HWND hwnd, ScintillaEditView *pSEView, bool showDialog, int startPos, int endPos);
	size_t Printer::doPrint() {
		if (!::PrintDlg(&_pdlg))
				return 0;

		return doPrint(true);
	};
	size_t doPrint(bool justDoIt);

private :
	PRINTDLG _pdlg;
	ScintillaEditView *_pSEView;
	size_t _startPos;
	size_t _endPos;
	size_t _nbPageTotal;
};

#endif //PRINTER_H
