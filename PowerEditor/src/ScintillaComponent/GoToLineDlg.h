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

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_GOLINE, isRTL);
		display();
	};

	void display(bool toShow = true) const override {
		Window::display(toShow);
		if (toShow)
		{
			updateLinesNumbers();
			::SetFocus(::GetDlgItem(_hSelf, ID_GOLINE_EDIT));
			::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
		}
		else
		{
			// clean Line Edit
			::SetDlgItemText(_hSelf, ID_GOLINE_EDIT, TEXT(""));
		}
	};

	void updateLinesNumbers() const;

protected :
	enum mode {go2line, go2offsset};
	mode _mode = go2line;
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	ScintillaEditView **_ppEditView = nullptr;

	long long getLine() const {
		constexpr int maxLen = 256;
		char goLineEditStr[maxLen] = {'\0'};
		UINT count = ::GetDlgItemTextA(_hSelf, ID_GOLINE_EDIT, goLineEditStr, maxLen);
		if (!count)
			return -1;
		char* p_end = nullptr;
		return strtoll(goLineEditStr, &p_end, 10);
	};
};
