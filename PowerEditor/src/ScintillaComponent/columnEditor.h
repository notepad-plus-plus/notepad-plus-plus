// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "columnEditor_rc.h"
#include "StaticDialog.h"
#include "Parameters.h"

class ScintillaEditView;


class ColumnEditorDlg : public StaticDialog
{
public :
	ColumnEditorDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView);

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_COLUMNEDIT, isRTL);
		const bool isTextMode = isCheckedOrNot(IDC_COL_TEXT_RADIO);
		display();
		::SetFocus(::GetDlgItem(_hSelf, isTextMode?IDC_COL_TEXT_EDIT:IDC_COL_INITNUM_EDIT));
	}

	void display(bool toShow = true) const override;
	void switchTo(bool toText);
	UCHAR getFormat();
	ColumnEditorParam::leadingChoice getLeading();
	UCHAR getHexCase();

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	ScintillaEditView **_ppEditView = nullptr;
	void setNumericFields(const ColumnEditorParam& colEditParam);
	int getNumericFieldValueFromText(int formatChoice, wchar_t str[], size_t stringSize);
	int sendValidationErrorMessage(int whichFlashRed, int formatChoice, wchar_t str[]);
};
