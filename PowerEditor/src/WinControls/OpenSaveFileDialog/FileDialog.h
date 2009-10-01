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

const int nbExtMax = 256;
const int extLenMax = 64;

using namespace std;

typedef vector<generic_string> stringVector;

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

	OPENFILENAMENPP _ofn;
	winVer _winVersion;
	

    //TCHAR _extArray[nbExtMax][extLenMax];
    int _nbExt;
    int _extTypeIndex;
    static FileDialog *staticThis;
};

#endif //FILE_DIALOG_H
