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
#include "resource.h"
#include "ScintillaEditView.h"

class GoToLineDlg : public StaticDialog
{
public :
	GoToLineDlg() = default;

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw std::runtime_error("GoToLineDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	};

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) {
		StaticDialog::create(dialogID, isRTL, msgDestParent);
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
	mode _mode = go2line;
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
    ScintillaEditView **_ppEditView = nullptr;

    void updateLinesNumbers() const;

    void cleanLineEdit() const {
        ::SetDlgItemText(_hSelf, ID_GOLINE_EDIT, TEXT(""));
    };

    long long getLine() const {
		const int maxLen = 256;
		char goLineEditStr[maxLen] = {'\0'};
		UINT count = ::GetDlgItemTextA(_hSelf, ID_GOLINE_EDIT, goLineEditStr, maxLen);
		if (!count)
			return -1;
		char* p_end;
		return strtoll(goLineEditStr, &p_end, 10);
    };

};

