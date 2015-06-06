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


#ifndef FINDCHARSINRANGE_DLG_H
#define FINDCHARSINRANGE_DLG_H

#ifndef FINDCHARSINRANGE_RC_H
#include "findCharsInRange_rc.h"
#endif //FINDCHARSINRANGE_RC_H

#ifndef SCINTILLA_EDIT_VIEW_H
#include "ScintillaEditView.h"
#endif //SCINTILLA_EDIT_VIEW_H

class FindCharsInRangeDlg : public StaticDialog
{
public :
	FindCharsInRangeDlg() : StaticDialog() {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw std::runtime_error("FindCharsInRangeDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	};

	virtual void create(int dialogID, bool isRTL = false) {
		StaticDialog::create(dialogID, isRTL);
	};

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_FINDCHARACTERS, isRTL);
		display();
	};

    virtual void display(bool toShow = true) const {
        Window::display(toShow);
    };

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
    ScintillaEditView **_ppEditView;
	bool findCharInRange(unsigned char beginRange, unsigned char endRange, int startPos, bool direction, bool wrap);
	bool getRangeFromUI(unsigned char & startRange, unsigned char & endRange);
	void getDirectionFromUI(bool & whichDirection, bool & isWrap);
};

#endif //FINDCHARSINRANGE_DLG_H
