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

const int nbExtMax = 10;
const int extLenMax = 10;

using namespace std;

typedef vector<string> stringVector;
//const bool styleOpen = true;
//const bool styleSave = false;

class FileDialog
{
public:
	FileDialog(HWND hwnd, HINSTANCE hInst);
	void setExtFilter(const char *, const char *, ...);
	
	void setExtsFilter(const char *extText, const char *exts);
	void setDefFileName(const char *fn){strcpy(_fileName, fn);}

	char * doSaveDlg();
	stringVector * doOpenMultiFilesDlg();
	char * doOpenSingleFileDlg();
	bool isReadOnly() {return _ofn.Flags & OFN_READONLY;};

protected :
    static UINT APIENTRY OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL APIENTRY run(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg)
        {
            case WM_NOTIFY :
			{
				LPNMHDR pNmhdr = (LPNMHDR)lParam;
				switch(pNmhdr->code)
				{
					case CDN_FILEOK :
					{
						printStr("CDN_FILEOK");
						if ((_fileName)&&(!strrchr(_extArray[_ofn.nFilterIndex - 1], '*'))
							&& (strcmp(_extArray[_ofn.nFilterIndex - 1], _fileName + _ofn.nFileExtension - 1)))
						{
							strcat(_fileName, _extArray[_ofn.nFilterIndex - 1]);
						}
						break;
					}
					case CDN_TYPECHANGE :
					{
						HWND fnControl = ::GetDlgItem(::GetParent(hWnd), edt1);
						char fn[256];
						::GetWindowText(fnControl, fn, sizeof(fn));
						if (*fn == '\0')
							return TRUE;

						HWND typeControl = ::GetDlgItem(::GetParent(hWnd), cmb1);
						int i = ::SendMessage(typeControl, CB_GETCURSEL, 0, 0);
						char ext[256];
						::SendMessage(typeControl, CB_GETLBTEXT, i, (LPARAM)ext);
						//printInt(i);
						//
						char *pExt = get1stExt(ext);
						if (*pExt == '\0')
							return TRUE;
						//::SendMessage(::GetParent(hWnd), CDM_SETDEFEXT, 0, (LPARAM)pExt);

						string fnExt = fn;

						int index = fnExt.find_last_of(".");
						string extension = ".";
						extension += pExt;
						if (index == string::npos)
						{
							fnExt += extension;
						}
						else
						{
							int len = (extension.length() > fnExt.length() - index + 1)?extension.length():fnExt.length() - index + 1;
							fnExt.replace(index, len, extension);
						}

						::SetWindowText(fnControl, fnExt.c_str());
						break;
					}
					default :
						return FALSE;
				}
				return TRUE;
			}
			default :
				return FALSE;
        }
    };

private:
	char _fileName[MAX_PATH*8];

	char _fileExt[MAX_PATH*10];
	int _nbCharFileExt;

	stringVector _fileNames;
	OPENFILENAME _ofn;

    char _extArray[nbExtMax][extLenMax];
    int _nbExt;

	char * get1stExt(char *ext) { // precondition : ext should be under the format : Batch (*.bat;*.cmd;*.nt)
		char *begin = ext;
		for ( ; *begin != '.' ; begin++);
		char *end = ++begin;
		for ( ; *end != ';' && *end != ')' ; end++);
		*end = '\0';
		if (*begin == '*')
			*begin = '\0';
		return begin;
	};
    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
