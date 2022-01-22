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

#include "regExtDlgRc.h"
#include "StaticDialog.h"

const int extNameLen = 32;

class RegExtDlg : public StaticDialog
{
public :
	RegExtDlg() = default;
	~RegExtDlg() = default;
	void doDialog(bool isRTL = false);


private :
	bool _isCustomize = false;

	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

	void getRegisteredExts();
	void getDefSupportedExts();
	void addExt(TCHAR *ext);
	bool deleteExts(const TCHAR *ext2Delete);
	void writeNppPath();

	int getNbSubKey(HKEY hKey) const {
		int nbSubKey;
		long result = ::RegQueryInfoKey(hKey, NULL, NULL, NULL, (LPDWORD)&nbSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return (result == ERROR_SUCCESS)?nbSubKey:0;
	}

	int getNbSubValue(HKEY hKey) const {
		int nbSubValue;
		long result = ::RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, (LPDWORD)&nbSubValue, NULL, NULL, NULL, NULL);
		return (result == ERROR_SUCCESS)?nbSubValue:0;
	}
};
