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

#ifndef GOTILINE_DLG_H
#define GOTILINE_DLG_H

#include "StaticDialog.h"
#include "..\resource.h"

#include "ScintillaEditView.h"

class GoToLineDlg : public StaticDialog
{
public :
	GoToLineDlg() : StaticDialog(), _mode(go2line) {};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw int(9900);
		_ppEditView = ppEditView;
	};

	virtual void create(int dialogID, bool isRTL = false) {
		StaticDialog::create(dialogID, isRTL);
	};

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_GOLINE, isRTL);
		display();
	};

    virtual void display(bool toShow = true) const {
        Window::display(toShow);
        if (toShow)
            ::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
    };

protected :
	enum mode {go2line, go2offsset};
	mode _mode;
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :

    ScintillaEditView **_ppEditView;

    void updateLinesNumbers() const {
		unsigned int current = 0;
		unsigned int limit = 0;
		
		if (_mode == go2line)
		{
			current = (unsigned int)((*_ppEditView)->getCurrentLineNumber() + 1);
			limit = (unsigned int)((*_ppEditView)->execute(SCI_GETLINECOUNT));
		}
		else
		{
			current = (unsigned int)(*_ppEditView)->execute(SCI_GETCURRENTPOS);
			limit = (unsigned int)((*_ppEditView)->getCurrentDocLen() - 1);
		}
        ::SetDlgItemInt(_hSelf, ID_CURRLINE, current, FALSE);
        ::SetDlgItemInt(_hSelf, ID_LASTLINE, limit, FALSE);
    };

    void cleanLineEdit() const {
        ::SetDlgItemText(_hSelf, ID_GOLINE_EDIT, TEXT(""));
    };

    int getLine() const {
        BOOL isSuccessful;
        int line = ::GetDlgItemInt(_hSelf, ID_GOLINE_EDIT, &isSuccessful, FALSE);
        return (isSuccessful?line:-1);
    };

};

#endif //GOTILINE_DLG_H
