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

//#define _WIN32_WINNT  0x0600

#include <shlwapi.h>
#include <windows.h>
#include <vector>
#include <string>
#include "Parameters.h"

const int nbExtMax = 256;
const int extLenMax = 64;

using namespace std;

typedef vector<generic_string> stringVector;
//const bool styleOpen = true;
//const bool styleSave = false;

struct OPENFILENAMENPP {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCTSTR      lpstrFilter;
   LPTSTR       lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPTSTR       lpstrFile;
   DWORD        nMaxFile;
   LPTSTR       lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCTSTR      lpstrInitialDir;
   LPCTSTR      lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCTSTR      lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCTSTR      lpTemplateName;
   void *		pvReserved;
   DWORD        dwReserved;
   DWORD        FlagsEx;
};


static generic_string changeExt(generic_string fn, generic_string ext)
{
	if (ext == TEXT(""))
		return fn;

	generic_string fnExt = fn;
	
	int index = fnExt.find_last_of(TEXT("."));
	generic_string extension = TEXT(".");
	extension += ext;
	if (index == generic_string::npos)
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
	
	//If window coordinates are all zero(ie,window is minimised),then assign desktop as the parent window.
 	if(rc.left == 0 && rc.right == 0 && rc.top == 0 && rc.bottom == 0)
 	{
 		//hParent = ::GetDesktopWindow();
		::ShowWindow(hParent, SW_SHOWNORMAL);
 		::GetClientRect(hParent,&rc);
 	}
	
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
	~FileDialog();
	void setExtFilter(const TCHAR *, const TCHAR *, ...);
	
	int setExtsFilter(const TCHAR *extText, const TCHAR *exts);
	void setDefFileName(const TCHAR *fn){lstrcpy(_fileName, fn);}

	TCHAR * doSaveDlg();
	stringVector * doOpenMultiFilesDlg();
	TCHAR * doOpenSingleFileDlg();
	bool isReadOnly() {return _ofn.Flags & OFN_READONLY;};

	static int _dialogFileBoxId;
protected :
    static UINT_PTR CALLBACK OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL APIENTRY run(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	TCHAR _fileName[MAX_PATH*8];

	TCHAR * _fileExt;
	int _nbCharFileExt;

	stringVector _fileNames;

	OPENFILENAMENPP _ofn;
	winVer _winVersion;
	

    //TCHAR _extArray[nbExtMax][extLenMax];
    int _nbExt;

    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
