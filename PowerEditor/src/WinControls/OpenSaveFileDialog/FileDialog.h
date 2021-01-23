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
	void setDefFileName(const TCHAR *fn){ wcscpy_s(_fileName, fn);}
	void setDefExt(const TCHAR *ext){ _defExt = ext;}

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
	generic_string _defExt;

	TCHAR * _fileExt = nullptr;
	int _nbCharFileExt = 0;

	stringVector _fileNames;

	OPENFILENAME _ofn;
	winVer _winVersion;
	
    int _nbExt = 0;
    int _extTypeIndex = -1;
    static FileDialog *staticThis;
};

