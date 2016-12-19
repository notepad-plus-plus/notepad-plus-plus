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


#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include "Common.h"
#include "Notepad_plus_msgs.h"

const int nbExtMax = 256;
const int extLenMax = 64;

typedef std::vector<generic_string> stringVector;

generic_string changeExt(generic_string fn, generic_string ext, bool forceReplaced = true);
void goToCenter(HWND hwnd);


class FileDialog
{
public:
	FileDialog(HWND hwnd, HINSTANCE hInst);
	~FileDialog();
	void setExtFilter(const TCHAR *, const TCHAR *, ...);
	
	int setExtsFilter(const TCHAR *extText, const TCHAR *exts);
	void setDefFileName(const TCHAR *fn){lstrcpy(_fileName, fn);}

	TCHAR * doSaveDlg();
	stringVector * doOpenMultiFilesDlg();
	TCHAR * doOpenSingleFileDlg();
	bool isReadOnly() {return _ofn.Flags & OFN_READONLY;};
    void setExtIndex(int extTypeIndex) {_extTypeIndex = extTypeIndex;};

	static int _dialogFileBoxId;
protected :
    static UINT_PTR CALLBACK OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL APIENTRY run(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	TCHAR _fileName[MAX_PATH*8];

	TCHAR * _fileExt;
	int _nbCharFileExt;

	stringVector _fileNames;

	OPENFILENAME _ofn;
	winVer _winVersion;
	
    int _nbExt;
    int _extTypeIndex;
    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
