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


#ifndef PRINTER_H
#define PRINTER_H

#ifndef SCINTILLA_EDIT_VIEW_H
#include "ScintillaEditView.h"
#endif //SCINTILLA_EDIT_VIEW_H


struct NPP_RangeToFormat {
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
	size_t doPrint() {
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
