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

// created by Daniel Volk mordorpost@volkarts.com

#pragma once

#include "RunMacroDlg_rc.h"
#include "StaticDialog.h"

class RunMacroDlg : public StaticDialog
{
public :
	RunMacroDlg() = default;
	~RunMacroDlg() = default;

	void doDialog(bool isRTL = false) {
		if (!isCreated())
			create(IDD_RUN_MACRO_DLG, isRTL);
		else
		{
			// Shortcut might have been updated for current session
			// So reload the macro list (issue #4526)
			initMacroList();
			::ShowWindow(_hSelf, SW_SHOW);
			::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
		}
	};

	void initMacroList();

	int isMulti() const { return isCheckedOrNot(IDC_M_RUN_MULTI); };
	int getTimes() const {return _times;};
	int getMacro2Exec() const;

private :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	int _times = 1;
	int _macroIndex = 0;
};
