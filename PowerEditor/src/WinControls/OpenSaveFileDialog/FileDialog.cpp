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


#include <shlwapi.h>

#include "FileDialog.h"
#include "Parameters.h"


FileDialog *FileDialog::staticThis = NULL;

FileDialog::FileDialog(HWND hwnd, HINSTANCE hInst) 
	: _nbCharFileExt(0), _nbExt(0), _fileExt(NULL), _extTypeIndex(-1)
{
	staticThis = this;

	_fileName[0] = '\0';
 
	_winVersion = (NppParameters::getInstance())->getWinVersion();

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
	_ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_LONGNAMES | DS_CENTER | OFN_HIDEREADONLY;
	_ofn.pvReserved = NULL;
	_ofn.dwReserved = 0;
	_ofn.FlagsEx = 0;
}

FileDialog::~FileDialog()
{
	if (_fileExt)
	{
		delete[] _fileExt;
		_fileExt = NULL;
	}
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
	int nbCharAdditional = extFilter.length() + lstrlen(exts) + 3; // 3 additional for nulls
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
	lstrcpy(pFileExt, extFilter.c_str());
    _nbCharFileExt += extFilter.length() + 1;
    
    pFileExt = _fileExt + _nbCharFileExt;
	lstrcpy(pFileExt, exts);
    _nbCharFileExt += lstrlen(exts) + 1;

	// Set file dialog pointer
	_ofn.lpstrFilter = _fileExt;

	return _nbExt;
}

TCHAR* FileDialog::doOpenSingleFileDlg()
{
	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);
	NppParameters * params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params->getWorkingDir();

	_ofn.Flags |= OFN_FILEMUSTEXIST;

	TCHAR *fn = NULL;
	try {
		fn = ::GetOpenFileName(&_ofn)?_fileName:NULL;
		
		if (params->getNppGUI()._openSaveDir == dir_last)
		{
			::GetCurrentDirectory(MAX_PATH, dir);
			params->setWorkingDir(dir);
		}
	} catch(std::exception e) {
		::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
	} catch(...) {
		::MessageBox(NULL, TEXT("GetSaveFileName crashes!!!"), TEXT(""), MB_OK);
	}

	::SetCurrentDirectory(dir); 

	return (fn);
}

stringVector * FileDialog::doOpenMultiFilesDlg()
{
	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);
	//_ofn.lpstrInitialDir = dir;

	NppParameters * params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params->getWorkingDir();

	_ofn.Flags |= OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	BOOL res = ::GetOpenFileName(&_ofn);
	if (params->getNppGUI()._openSaveDir == dir_last)
	{
		::GetCurrentDirectory(MAX_PATH, dir);
		params->setWorkingDir(dir);
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
			lstrcpy(fn, _fileName);
			if (fn[lstrlen(fn) - 1] != '\\')
				lstrcat(fn, TEXT("\\"));
		}

		int term = lstrlen(fn);

		while (*pFn)
		{
			fn[term] = '\0';
			lstrcat(fn, pFn);
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

	NppParameters * params = NppParameters::getInstance();
	_ofn.lpstrInitialDir = params->getWorkingDir();

	//restore persisted filter index 
	int index = params->getFileSaveDlgFilterIndex();

	if (_extTypeIndex != -1)
	{
		//+1 due to start index of all is 1, zero is reserved for customer mode
		_ofn.nFilterIndex = _extTypeIndex + 1;
	}
	else if (index != -1)
	{
		//fallback to last stored file extension as filter
		_ofn.nFilterIndex = index;
	}
	else
	{
		//fallback to all
		//starting with 1, zero is reserved for customer mode
		_ofn.nFilterIndex = 1;
	}
	
	_ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ENABLESIZING;

	TCHAR *fn = NULL;
	try {
		fn = ::GetSaveFileName(&_ofn)?_fileName:NULL;
		if (params->getNppGUI()._openSaveDir == dir_last)
		{
			::GetCurrentDirectory(MAX_PATH, dir);
			params->setWorkingDir(dir);
		}
		
		//store filter index for the next call to save file dialog 
		params->setFileSaveDlgFilterIndex(_ofn.nFilterIndex);

	} catch(std::exception e) {
		::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
	} catch(...) {
		::MessageBox(NULL, TEXT("GetSaveFileName crashes!!!"), TEXT(""), MB_OK);
	}

	::SetCurrentDirectory(dir); 

	return (fn);
}



