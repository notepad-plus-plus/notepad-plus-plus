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

#include "findCharsInRange_rc.h"
#include "ScintillaEditView.h"

class FindCharsInRangeDlg : public StaticDialog
{
public :
	FindCharsInRangeDlg() = default;

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw std::runtime_error("FindCharsInRangeDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	};

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_FINDCHARACTERS, isRTL);
		display();
	};

	void display(bool toShow = true) const override {
		Window::display(toShow);
	};

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	ScintillaEditView **_ppEditView = nullptr;
	bool findCharInRange(unsigned char beginRange, unsigned char endRange, intptr_t startPos, bool direction, bool wrap);
	bool getRangeFromUI(unsigned char & startRange, unsigned char & endRange);
	void getDirectionFromUI(bool & whichDirection, bool & isWrap);
};

