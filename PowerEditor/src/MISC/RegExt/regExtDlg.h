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

#ifndef REG_EXT_DLG_H
#define REG_EXT_DLG_H

#include "StaticDialog.h"
#include "regExtDlgRc.h"

const int extNameLen = 32;

class RegExtDlg : public StaticDialog
{
public :
	RegExtDlg() : _isCustomize(false){};
	~RegExtDlg(){};
	void doDialog(bool isRTL = false);


private :
	bool _isCustomize;

	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	
	void getRegisteredExts();
	void getDefSupportedExts();
	void addExt(TCHAR *ext);
	bool deleteExts(const TCHAR *ext2Delete);
	void writeNppPath();

	int getNbSubKey(HKEY hKey) const {
		int nbSubKey;
		long result = ::RegQueryInfoKey(hKey, NULL, NULL, NULL, (LPDWORD)&nbSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return (result == ERROR_SUCCESS)?nbSubKey:0;
	};

	int getNbSubValue(HKEY hKey) const {
		int nbSubValue;
		long result = ::RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, (LPDWORD)&nbSubValue, NULL, NULL, NULL, NULL);
		return (result == ERROR_SUCCESS)?nbSubValue:0;
	};
};

#endif //REG_EXT_DLG_H
