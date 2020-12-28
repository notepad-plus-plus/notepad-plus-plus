// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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

#pragma once

#include "columnEditor_rc.h"
#include "StaticDialog.h"

class ScintillaEditView;

const bool activeText = true;
const bool activeNumeric = false;

class ColumnEditorDlg : public StaticDialog
{
public :
	ColumnEditorDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView);

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) {
		StaticDialog::create(dialogID, isRTL, msgDestParent);
	};

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_COLUMNEDIT, isRTL);
		bool isTextMode = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_COL_TEXT_RADIO, BM_GETCHECK, 0, 0));
		display();
		::SetFocus(::GetDlgItem(_hSelf, isTextMode?IDC_COL_TEXT_EDIT:IDC_COL_INITNUM_EDIT));
	};

    virtual void display(bool toShow = true) const;
	void switchTo(bool toText);
	UCHAR getFormat();

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
    ScintillaEditView **_ppEditView = nullptr;
};

