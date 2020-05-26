// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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


#include <shlwapi.h>

#include "FileDialog.h"
#include "Parameters.h"

#include <algorithm>

FileDialog *FileDialog::staticThis = NULL;

FileDialog::FileDialog(HWND hwnd, HINSTANCE hInst) 
	: _nbCharFileExt(0), _nbExt(0), _fileExt(NULL), _extTypeIndex(-1)
{
	staticThis = this;
    
	memset(_fileName, 0, sizeof(_fileName));
	_winVersion = (NppParameters::getInstance()).getWinVersion();

	_ofn.lStructSize = sizeof(_ofn);
	if (_winVersion < WV_W2K)
		_ofn.lStructSize = sizeof(OPENFILENAME);
	_ofn.hwndOwner = hwnd; 
	_ofn.hInstance = hInst;
	_ofn.lpstrCustomFilter = (LPTSTR) NULL;
	_ofn.nMaxCustFilter = 0L;
	_ofn.nFilterIndex = 1L;
	_ofn.lpstrFile = _fileName;
	_ofn.nMaxFile = sizeof(_fileName)/sizeof(TCHAR);
	_ofn.lpstrFileTitle = NULL;
	_ofn.nMaxFileTitle = 0;
	_ofn.lpstrInitialDir = NULL;
	_ofn.lpstrTitle = NULL;
	_ofn.nFileOffset  = 0;
	_ofn.nFileExtension = 0;
	_ofn.lpfnHook = NULL;
	_ofn.lpstrDefExt = NULL;  // No default extension
	_ofn.lCustData = 0;
	_ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | OFN_HIDEREADONLY;
	_ofn.pvReserved = NULL;
	_ofn.dwReserved = 0;
	_ofn.FlagsEx = 0;
}

FileDialog::~FileDialog()
{
	delete[] _fileExt;
	_fileExt = NULL;
}

// This function set and concatenate the filter into the list box of FileDialog.
// The 1st parameter is the description of the file type, the 2nd .. Nth parameter(s) is (are)
// the file extension which should be ".WHATEVER", otherwise it (they) will be considered as
// a file name to filter. Since the nb of arguments is variable, you have to add NULL at the end.
// example : 
// FileDialog.setExtFilter(TEXT("c/c++ src file"), TEXT(".c"), TEXT(".cpp"), TEXT(".cxx"), TEXT(".h"), NULL);
// FileDialog.setExtFilter(TEXT("Makefile"), TEXT("makefile"), TEXT("GNUmakefile"), NULL);
void FileDialog::setExtFilter(const TCHAR *extText, const TCHAR *ext, ...)
{
    // fill out the ext array for save as file dialog
	generic_string exts;

    va_list pArg;
    va_start(pArg, ext);

    const TCHAR *ext2Concat;
	ext2Concat = ext;
    do
	{
        if (ext2Concat[0] == TEXT('.'))
            exts += TEXT("*");
        exts += ext2Concat;
        exts += TEXT(";");
	}
	while ( (ext2Concat = va_arg(pArg, const TCHAR *)) != NULL );

	va_end(pArg);

	// remove the last ';'
    exts = exts.substr(0, exts.length()-1);

	setExtsFilter(extText, exts.c_str());
}

int FileDialog::setExtsFilter(const TCHAR *extText, const TCHAR *exts)
{
    // fill out the ext array for save as file dialog
    generic_string extFilter = extText;
	TCHAR *oldFilter = NULL;

    extFilter += TEXT(" (");
    extFilter += exts;
	extFilter += TEXT(")");	
	
	// Resize filter buffer
	int nbCharAdditional = static_cast<int32_t>(extFilter.length() + _tcsclen(exts) + 3); // 3 additional for nulls
	if (_fileExt)
	{
		oldFilter = new TCHAR[_nbCharFileExt];
		memcpy(oldFilter, _fileExt, _nbCharFileExt * sizeof(TCHAR));

		delete[] _fileExt;
		_fileExt = NULL;
	}

	int nbCharNewFileExt = _nbCharFileExt + nbCharAdditional;
	_fileExt = new TCHAR[nbCharNewFileExt];
	memset(_fileExt, 0, nbCharNewFileExt * sizeof(TCHAR));

	// Restore previous filters
	if (oldFilter)
	{		
		memcpy(_fileExt, oldFilter, _nbCharFileExt * sizeof(TCHAR));
		delete[] oldFilter;
		oldFilter = NULL;
	}

	// Append new filter    
    TCHAR *pFileExt = _fileExt + _nbCharFileExt;
	auto curLen = extFilter.length() + 1;
	wcscpy_s(pFileExt, curLen, extFilter.c_str());
	_nbCharFileExt += static_cast<int32_t>(curLen);
    
    pFileExt = _fileExt + _nbCharFileExt;
	curLen = _tcsclen(exts) + 1;
	wcscpy_s(pFileExt, curLen, exts);
	_nbCharFileExt += static_cast<int32_t>(curLen);

	// Set file dialog pointer
	_ofn.lpstrFilter = _fileExt;

	return _nbExt;
}

TCHAR* FileDialog::doOpenSingleFileDlg()
{
	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);
	NppParameters& params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params.getWorkingDir();
	_ofn.lpstrDefExt = _defExt.c_str();

	_ofn.Flags |= OFN_FILEMUSTEXIST;

	if (!params.useNewStyleSaveDlg())
	{
		_ofn.Flags |= OFN_ENABLEHOOK | OFN_NOVALIDATE;
		_ofn.lpfnHook = OFNHookProc;
	}

	TCHAR *fn = NULL;
	try
	{
		fn = ::GetOpenFileName(&_ofn) ? _fileName : NULL;

		if (params.getNppGUI()._openSaveDir == dir_last)
		{
			::GetCurrentDirectory(MAX_PATH, dir);
			params.setWorkingDir(dir);
		}
	}
	catch (std::exception& e)
	{
		generic_string msg = TEXT("An exception occurred while opening file: ");
		msg += _fileName;
		msg += TEXT("\r\n\r\nException reason: ");
		msg += s2ws(e.what());

		::MessageBox(NULL, msg.c_str(), TEXT("File Open Exception"), MB_OK);
	}
	catch (...)
	{
		::MessageBox(NULL, TEXT("doOpenSingleFileDlg crashes!!!"), TEXT(""), MB_OK);
	}

	::SetCurrentDirectory(dir);

	return (fn);
}

stringVector * FileDialog::doOpenMultiFilesDlg()
{
	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);

	NppParameters& params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params.getWorkingDir();

	_ofn.Flags |= OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_ENABLESIZING;

	if (!params.useNewStyleSaveDlg())
	{
		_ofn.Flags |= OFN_ENABLEHOOK | OFN_NOVALIDATE;
		_ofn.lpfnHook = OFNHookProc;
	}

	BOOL res = ::GetOpenFileName(&_ofn);
	if (params.getNppGUI()._openSaveDir == dir_last)
	{
		::GetCurrentDirectory(MAX_PATH, dir);
		params.setWorkingDir(dir);
	}
	::SetCurrentDirectory(dir);

	if (res)
	{
		TCHAR* pFn = _fileName + lstrlen(_fileName) + 1;
		TCHAR fn[MAX_PATH*8];
		memset(fn, 0x0, sizeof(fn));

		if (!(*pFn))
		{
			_fileNames.push_back(generic_string(_fileName));
		}
		else
		{
			wcscpy_s(fn, _fileName);
			if (fn[lstrlen(fn) - 1] != '\\')
				wcscat_s(fn, TEXT("\\"));
		}

		int term = lstrlen(fn);

		while (*pFn)
		{
			fn[term] = '\0';
			wcscat_s(fn, pFn);
			_fileNames.push_back(generic_string(fn));
			pFn += lstrlen(pFn) + 1;
		}

		return &_fileNames;
	}
	return nullptr;
}


TCHAR * FileDialog::doSaveDlg()
{
	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);

	NppParameters& params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params.getWorkingDir();
	_ofn.lpstrDefExt = _defExt.c_str();

	_ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ENABLESIZING;

	if (!params.useNewStyleSaveDlg())
	{
		_ofn.Flags |= OFN_ENABLEHOOK | OFN_NOVALIDATE;
		_ofn.lpfnHook = OFNHookProc;
	}

	TCHAR *fn = NULL;
	try
	{
		fn = ::GetSaveFileName(&_ofn) ? _fileName : NULL;
		if (params.getNppGUI()._openSaveDir == dir_last)
		{
			::GetCurrentDirectory(MAX_PATH, dir);
			params.setWorkingDir(dir);
		}
	}
	catch (std::exception& e)
	{
		generic_string msg = TEXT("An exception occurred while saving file: ");
		msg += _fileName;
		msg += TEXT("\r\n\r\nException reason: ");
		msg += s2ws(e.what());

		::MessageBox(NULL, msg.c_str(), TEXT("File Save Exception"), MB_OK);
	}
	catch (...)
	{
		::MessageBox(NULL, TEXT("GetSaveFileName crashes!!!"), TEXT(""), MB_OK);
	}

	::SetCurrentDirectory(dir);

	return (fn);
}

static HWND hFileDlg = NULL;
static WNDPROC oldProc = NULL;
static generic_string currentExt = TEXT("");


static LRESULT CALLBACK fileDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
    {
		case WM_COMMAND :
		{
			switch (wParam)
			{	
				case IDOK :
				{
					HWND fnControl = ::GetDlgItem(hwnd, FileDialog::_dialogFileBoxId);
					TCHAR fn[MAX_PATH];
					::GetWindowText(fnControl, fn, MAX_PATH);

					// Check condition to have the compability of default behaviour 
					if (*fn == '\0')
						return oldProc(hwnd, message, wParam, lParam);
					else if (::PathIsDirectory(fn))
						return oldProc(hwnd, message, wParam, lParam);

					// Process
					if (currentExt != TEXT(""))
					{
						generic_string fnExt = changeExt(fn, currentExt, false);
						::SetWindowText(fnControl, fnExt.c_str());
					}
					return oldProc(hwnd, message, wParam, lParam);
				}

				default :
					break;
			}
		}
	}
	return oldProc(hwnd, message, wParam, lParam);
};


static TCHAR * get1stExt(TCHAR *ext)
{
	// precondition : ext should be under the format : Batch (*.bat;*.cmd;*.nt)
	TCHAR *begin = ext;
	for ( ; *begin != '.' ; begin++);
	TCHAR *end = ++begin;
	for ( ; *end != ';' && *end != ')' ; end++);
	*end = '\0';
	if (*begin == '*')
		*begin = '\0';
	return begin;
};

static generic_string addExt(HWND textCtrl, HWND typeCtrl)
{
	TCHAR fn[MAX_PATH];
	::GetWindowText(textCtrl, fn, MAX_PATH);
	
	auto i = ::SendMessage(typeCtrl, CB_GETCURSEL, 0, 0);

	auto cbTextLen = ::SendMessage(typeCtrl, CB_GETLBTEXTLEN, i, 0);
	TCHAR * ext = new TCHAR[cbTextLen + 1];
	::SendMessage(typeCtrl, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(ext));
	
	TCHAR *pExt = get1stExt(ext);
	if (*fn != '\0')
	{
		generic_string fnExt = changeExt(fn, pExt);
		::SetWindowText(textCtrl, fnExt.c_str());
	}

	generic_string returnExt = pExt;
	delete[] ext;
	return returnExt;
};

UINT_PTR CALLBACK FileDialog::OFNHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG :
        {
			NppParameters& nppParam = NppParameters::getInstance();
			int index = nppParam.getFileSaveDlgFilterIndex();

			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(staticThis));
			hFileDlg = ::GetParent(hWnd);
			goToCenter(hFileDlg);

			if (index != -1)
			{
				HWND typeControl = ::GetDlgItem(hFileDlg, cmb1);
				::SendMessage(typeControl, CB_SETCURSEL, index, 0);
			}
			// Don't touch the following 3 lines, they are cursed !!!
			oldProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hFileDlg, GWLP_WNDPROC));
			if (oldProc)
				::SetWindowLongPtr(hFileDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(fileDlgProc));

			return FALSE;
		}

		default :
		{
			FileDialog *pFileDialog = reinterpret_cast<FileDialog *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!pFileDialog)
			{
				return FALSE;
			}
			return pFileDialog->run(hWnd, uMsg, wParam, lParam);
		}
    }
}

BOOL APIENTRY FileDialog::run(HWND hWnd, UINT uMsg, WPARAM, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NOTIFY :
		{
			LPNMHDR pNmhdr = (LPNMHDR)lParam;
			switch(pNmhdr->code)
			{
                case CDN_INITDONE :
                {
                    if (_extTypeIndex == -1)
                        return TRUE;

                    HWND fnControl = ::GetDlgItem(::GetParent(hWnd), _dialogFileBoxId);
                    HWND typeControl = ::GetDlgItem(::GetParent(hWnd), cmb1);
                    ::SendMessage(typeControl, CB_SETCURSEL, _extTypeIndex, 0);

                    currentExt = addExt(fnControl, typeControl);
                    return TRUE;
                }

				case CDN_TYPECHANGE :
				{
					HWND fnControl = ::GetDlgItem(::GetParent(hWnd), _dialogFileBoxId);
					HWND typeControl = ::GetDlgItem(::GetParent(hWnd), cmb1);
					currentExt = addExt(fnControl, typeControl);
					return TRUE;
				}

				case CDN_FILEOK :
				{
					HWND typeControl = ::GetDlgItem(::GetParent(hWnd), cmb1);
					int index = static_cast<int32_t>(::SendMessage(typeControl, CB_GETCURSEL, 0, 0));
					NppParameters& nppParam = NppParameters::getInstance();
					nppParam.setFileSaveDlgFilterIndex(index);

					// change forward-slash to back-slash directory paths so dialog can interpret
					OPENFILENAME* ofn = reinterpret_cast<LPOFNOTIFY>(lParam)->lpOFN;
					TCHAR* fileName = ofn->lpstrFile;

					// note: this check is essential, because otherwise we could return True
					//       with a OFN_NOVALIDATE dialog, which leads to opening every file
					//       in the specified directory. Multi-select terminator is \0\0.
					if ((ofn->Flags & OFN_ALLOWMULTISELECT) &&
						(*(fileName + lstrlen(fileName) + 1) != '\0'))
						return FALSE;

					if (::PathIsDirectory(fileName))
					{
						// change to backslash, and insert trailing '\' to indicate directory
						hFileDlg = ::GetParent(hWnd);
						std::wstring filePath(fileName);
						std::replace(filePath.begin(), filePath.end(), '/', '\\');

						if (filePath.back() != '\\')
							filePath.insert(filePath.end(), '\\');

						// There are two or more double backslash, then change it to single
						while (filePath.find(L"\\\\") != std::wstring::npos)
							filePath.replace(filePath.find(TEXT("\\\\")), 2, TEXT("\\"));

						// change the dialog directory selection
						::SendMessage(hFileDlg, CDM_SETCONTROLTEXT, edt1,
							reinterpret_cast<LPARAM>(filePath.c_str()));
						::PostMessage(hFileDlg, WM_COMMAND, IDOK, 0);
						::SetWindowLongPtr(hWnd, 0 /*DWL_MSGRESULT*/, 1);
					}
					return TRUE;
				}

				default :
					return FALSE;
			}
			
		}
		default :
			return FALSE;
    }
}

void goToCenter(HWND hwnd)
{
    RECT rc;
	HWND hParent = ::GetParent(hwnd);
	::GetClientRect(hParent, &rc);
	
	//If window coordinates are all zero(ie,window is minimised),then assign desktop as the parent window.
 	if (rc.left == 0 && rc.right == 0 && rc.top == 0 && rc.bottom == 0)
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
}

generic_string changeExt(generic_string fn, generic_string ext, bool forceReplaced)
{
	if (ext == TEXT(""))
		return fn;

	generic_string fnExt = fn;
	
	auto index = fnExt.find_last_of(TEXT("."));
	generic_string extension = TEXT(".");
	extension += ext;
	if (index == generic_string::npos)
	{
		fnExt += extension;
	}
	else if (forceReplaced)
	{
		auto len = (extension.length() > fnExt.length() - index + 1)?extension.length():fnExt.length() - index + 1;
		fnExt.replace(index, len, extension);
	}
	return fnExt;
}
