// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#ifndef REG_EXT_DLG_H
#define REG_EXT_DLG_H

#include "regExtDlgRc.h"
#include "StaticDialog.h"

const int extNameLen = 32;

class RegExtDlg : public StaticDialog
{
public :
	RegExtDlg() : _isCustomize(false){};
	~RegExtDlg(){};
	void doDialog(bool isRTL = false);


private :
	bool _isCustomize;

	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	
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
