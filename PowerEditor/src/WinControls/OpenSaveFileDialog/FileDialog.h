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

const int nbExtMax = 10;
const int extLenMax = 10;

typedef std::vector<std::string> stringVector;
//const bool styleOpen = true;
//const bool styleSave = false;

class FileDialog
{
public:
	FileDialog(HWND hwnd, HINSTANCE hInst);
	void setExtFilter(const char *, const char *, ...);
	void setDefFileName(const char *fn){strcpy(_fileName, fn);}

	char * doSaveDlg();
	stringVector * doOpenMultiFilesDlg();
	char * doOpenSingleFileDlg();
	bool isReadOnly() {return _ofn.Flags & OFN_READONLY;};
/*
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
						if ((_fileName)&&(!strrchr(_extArray[_ofn.nFilterIndex - 1], '*'))
							&& (strcmp(_extArray[_ofn.nFilterIndex - 1], _fileName + _ofn.nFileExtension - 1)))
						{
							strcat(_fileName, _extArray[_ofn.nFilterIndex - 1]);
						}
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
*/
private:
	char _fileName[MAX_PATH*8];

	char _fileExt[MAX_PATH*2];
	int _nbCharFileExt;
	//bool _isMultiSel;

	stringVector _fileNames;
	OPENFILENAME _ofn;

    char _extArray[nbExtMax][extLenMax];
    int _nbExt;

    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
