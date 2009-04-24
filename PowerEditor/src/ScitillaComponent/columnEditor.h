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

#ifndef COLUMNEDITOR_H
#define COLUMNEDITOR_H

#include "columnEditor_rc.h"
#include "StaticDialog.h"
#include "ScintillaEditView.h"

const bool activeText = true;
const bool activeNumeric = false;

class ColumnEditorDlg : public StaticDialog
{
public :
	ColumnEditorDlg() : StaticDialog() {};

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
			create(IDD_COLUMNEDIT, isRTL);
		bool isTextMode = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_GETCHECK, 0, 0));
		display();
		::SetFocus(::GetDlgItem(_hSelf, isTextMode?IDC_COL_TEXT_EDIT:IDC_COL_INITNUM_EDIT));
	};

    virtual void display(bool toShow = true) const {
        Window::display(toShow);
        if (toShow)
            ::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
    };

	void switchTo(bool toText);

	UCHAR getFormat() {
		bool isLeadingZeros = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_LEADZERO_CHECK, BM_GETCHECK, 0, 0));
		UCHAR f = 0; // Dec by default
		if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_HEX_RADIO, BM_GETCHECK, 0, 0))
			f = 1;
		else if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_OCT_RADIO, BM_GETCHECK, 0, 0))
			f = 2;
		else if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_BIN_RADIO, BM_GETCHECK, 0, 0))
			f = 3;
		return (f | (isLeadingZeros?MASK_ZERO_LEADING:0));
	};

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :

    ScintillaEditView **_ppEditView;


};
#endif// COLUMNEDITOR_H
