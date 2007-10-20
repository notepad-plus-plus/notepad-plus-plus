//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <shlwapi.h>
#include <windows.h>
//#include <shlobj.h>

#include <vector>
#include <string>
#include "SysMsg.h"
#include "Parameters.h"

const int nbExtMax = 256;
const int extLenMax = 64;

using namespace std;

typedef vector<string> stringVector;
//const bool styleOpen = true;
//const bool styleSave = false;


static string changeExt(string fn, string ext)
{
	if (ext == "")
		return fn;

	string fnExt = fn;
	
	int index = fnExt.find_last_of(".");
	string extension = ".";
	extension += ext;
	if (index == string::npos)
	{
		fnExt += extension;
	}
	else
	{
		int len = (extension.length() > fnExt.length() - index + 1)?extension.length():fnExt.length() - index + 1;
		fnExt.replace(index, len, extension);
	}
	return fnExt;
};

static void goToCenter(HWND hwnd)
{
    RECT rc;
	HWND hParent = ::GetParent(hwnd);
	::GetClientRect(hParent, &rc);
    POINT center;
    center.x = rc.left + (rc.right - rc.left)/2;
    center.y = rc.top + (rc.bottom - rc.top)/2;
    ::ClientToScreen(hParent, &center);

	RECT _rc;
	::GetWindowRect(hwnd, &_rc);
	int x = center.x - (_rc.right - _rc.left)/2;
	int y = center.y - (_rc.bottom - _rc.top)/2;

	::SetWindowPos(hwnd, HWND_TOP, x, y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
};

class FileDialog
{
public:
	FileDialog(HWND hwnd, HINSTANCE hInst);
	void setExtFilter(const char *, const char *, ...);
	
	int setExtsFilter(const char *extText, const char *exts);
	void setDefFileName(const char *fn){strcpy(_fileName, fn);}

	char * doSaveDlg();
	stringVector * doOpenMultiFilesDlg();
	char * doOpenSingleFileDlg();
	bool isReadOnly() {return _ofn.Flags & OFN_READONLY;};

protected :
    static UINT APIENTRY OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL APIENTRY run(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	char _fileName[MAX_PATH*8];

	char _fileExt[MAX_PATH*10];
	int _nbCharFileExt;

	stringVector _fileNames;
	OPENFILENAME _ofn;

    char _extArray[nbExtMax][extLenMax];
    int _nbExt;

    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
