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
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
    ScintillaEditView **_ppEditView;
	bool findCharInRange(unsigned char beginRange, unsigned char endRange, int startPos, bool direction, bool wrap);
	bool getRangeFromUI(unsigned char & startRange, unsigned char & endRange);
	void getDirectionFromUI(bool & whichDirection, bool & isWrap);
};

#endif //FINDCHARSINRANGE_DLG_H
