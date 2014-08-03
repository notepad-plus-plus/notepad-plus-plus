// Replace an icon group in an executable by one from an ICO file
// By Francois-R.Boyer@PolyMtl.ca for Notepad++
// 2010-11-20
//
// This code is based on: Maria Nadejde, "Replacing ICON resources in EXE and DLL files", The Code Project, 13 Nov 2008
//     ( http://www.codeproject.com/KB/DLL/ICON_Resources.aspx )
//     original article and code is licenced under The GNU General Public License (GPLv3)
//
//
// this file is part of ChangeIcon
// Copyright (C)2010 Francois-R Boyer <Francois-R.Boyer@PolyMtl.ca>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <stddef.h>		// for offsetof

#ifdef _DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

BOOL getMaxIconId_EnumNamesFunc(HANDLE hModule, LPCTSTR lpType, LPTSTR lpName, WORD* lpMaxID)  
{ 
	if(IS_INTRESOURCE(lpName) && (USHORT)lpName>*lpMaxID)
		*lpMaxID=(USHORT)lpName;
	return true;
}

WORD getMaxIconId(TCHAR* lpFileName)
{
	WORD nMaxID = 0;
	HINSTANCE hLib = LoadLibraryEx(lpFileName,NULL,DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	if(hLib == NULL) { _tprintf(_T("Unable to load library '%s'\n"), lpFileName); return 0xFFFF; }
	// Enumerate icon "names" (IDs) to get next available ID
	if(!EnumResourceNames(hLib, RT_ICON, (ENUMRESNAMEPROC)getMaxIconId_EnumNamesFunc,(LONG_PTR)&nMaxID)) { _tprintf(_T("Unable to enum icons\n")); return 0xFFFF; }	
	FreeLibrary(hLib);
	IFDEBUG( _tprintf(_T("MaxIcon=%d\n"), nMaxID); )
	return nMaxID;
}

class Icon {
public:
	// Icon format from http://msdn.microsoft.com/en-us/library/ms997538.aspx
	// for ICO and EXE files
	struct ICONDIR {			// File header:
		WORD	idReserved;		// Reserved (must be 0)
		WORD	idType;			// Resource Type (1 for icons)
		WORD	idCount;		// How many images?
	};
	struct ICONDIRENTRY {		// One for each image:
		BYTE	bWidth;			// Width, in piexels, of the image
		BYTE	bHeight;		// Height, in pixels, of the image (times 2)
		BYTE	bColorCount;	// Number of colors in image (0 if >=8bpp)
		BYTE	bReserved;		// Reserved (must be 0)
		WORD	wPlanes;		// Color Planes
		WORD	wBitCount;		// Bits per pixel
		DWORD	dwBytesInRes;	// How many bytes in this resource?
		union {
			DWORD	dwImageOffset;// Where in the file is this image  (in ICO file)
			WORD	nID;		// the ID (in EXE file)
		};
	};
	static const UINT sizeof_iconDirEntry_ICO = sizeof(ICONDIRENTRY);
	static const UINT sizeof_iconDirEntry_EXE = offsetof(ICONDIRENTRY,nID)+sizeof(WORD);

	ICONDIR _head;
	ICONDIRENTRY *_entries;
	LPBYTE *_imagesData;
	
	Icon() : _entries(NULL), _imagesData(NULL) { _head.idCount = 0; }
	void clear() {
		if(_imagesData) { for(int i=0; i<_head.idCount; ++i) delete _imagesData[i]; delete[] _imagesData; _imagesData = 0; }
		if(_entries) delete[] _entries; _entries = 0;
		_head.idCount = 0;
	}
	~Icon() { clear(); }
	
	bool readICO(TCHAR* filename);
	bool readEXE(TCHAR* lpFileName, LPCTSTR lpResName, UINT resLangId); // Does not currently read image data
	
	bool writeToEXE(TCHAR* lpFileName, LPCTSTR lpResName, UINT resLangId);
	
	WORD count() { return _head.idCount; }
};

bool Icon::readICO(TCHAR* filename)
{
	clear();
	HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) { _tprintf(_T("Error opening file '%s' for Reading\n"), filename); return false; }
	DWORD dwBytesRead;
	// Read header
	if(!ReadFile( hFile, &_head, sizeof(_head), &dwBytesRead, NULL )) { _tprintf(_T("Error reading file '%s'\n"), filename); return false; }
	IFDEBUG( _tprintf(_T("%d icon entries\n"), count()); )
	// Read entries
	_entries = new ICONDIRENTRY[count()];
	if(!ReadFile( hFile, _entries, sizeof(*_entries)*count(), &dwBytesRead, NULL )) { _tprintf(_T("Error reading file '%s'\n"), filename); return false; }
	// Read images
	_imagesData=new LPBYTE[count()]; memset(_imagesData, 0, sizeof(LPBYTE)*count());
	for(int i=0; i<count(); ++i)
	{
		IFDEBUG( _tprintf(_T("%d: offset=%d, size=%d\n"), i, _entries[i].dwImageOffset, _entries[i].dwBytesInRes); )
		_imagesData[i] = (LPBYTE)malloc(_entries[i].dwBytesInRes);
		if(SetFilePointer(hFile, _entries[i].dwImageOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) { _tprintf(_T("Error moving read pointer in '%s'\n"), filename); return false; }
		if(!ReadFile(hFile, _imagesData[i], _entries[i].dwBytesInRes, &dwBytesRead, NULL)) { _tprintf(_T("Error reading file '%s'\n"), filename); return false; }
	}
	return true;
}

bool Icon::readEXE(TCHAR* lpFileName, LPCTSTR lpResName, UINT resLangId)
{
	bool result = false;
	clear();
	HINSTANCE hLib = LoadLibraryEx(lpFileName, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	if(hLib == NULL) { _tprintf(_T("Unable to load library '%s'\n"), lpFileName); goto error1; }
	
	HRSRC hRsrc = FindResourceEx(hLib, RT_GROUP_ICON, lpResName, resLangId);
	if(hRsrc == NULL) { _tprintf(IS_INTRESOURCE(lpResName) ? _T("Icon group %d (lang %d) not found in '%s'\n") : _T("Icon group %s (lang %d) not found in '%s'\n"), lpResName, resLangId, lpFileName); goto error2; }

	HGLOBAL hGlobal = LoadResource(hLib, hRsrc);
	if(hGlobal == NULL) { _tprintf(IS_INTRESOURCE(lpResName) ? _T("Unable to load icon group %d from '%s'\n") : _T("ReplaceIconResource: icon group %s not found in '%s'\n"), lpResName, lpFileName); goto error2; }

	LPBYTE resData = (BYTE*)LockResource(hGlobal);
	if(resData == NULL) { _tprintf(_T("Unable to lock resource data\n")); goto error2; }

	LPBYTE readPtr = resData;
#define _myRead(ptr, size) { CopyMemory(ptr, readPtr, size); readPtr += size; }
	_myRead(&_head, sizeof(_head));
	IFDEBUG( _tprintf(_T("%d icon entries\n"), count()); )
	_entries = new ICONDIRENTRY[count()];
	for(int i=0; i<count(); ++i)
	{
		_myRead(&_entries[i], sizeof_iconDirEntry_EXE);
		IFDEBUG( _tprintf(_T("%d: ID=%d\n"), i, _entries[i].nID); )
	}
	// NOTE: This routine currently do not load image data from EXE.
	//_imagesData=new LPBYTE[count()]; memset(_imagesData, sizeof(LPBYTE)*count(), 0);

#undef _myRead
	result = true;
error3:
	UnlockResource((HGLOBAL)resData);
error2:
	FreeLibrary(hLib);
error1:
	return result;
}

bool Icon::writeToEXE(TCHAR* lpFileName, LPCTSTR lpResName, UINT resLangId)
{
	Icon oldIcon;
	// NOTE: This routine currently cannot add an icon groupe, only replace an existing one.
	if(!oldIcon.readEXE(lpFileName, lpResName, resLangId)) return false;

	//lpInitGrpIconDir   is oldIcon
	oldIcon._head.idReserved = _head.idReserved;
	
	// Set icon IDs for each icon in the group
	WORD nMaxID = 0xFFFF;
	for(int i=0; i<count(); ++i)
	{
		if(i<oldIcon.count()) { // use IDs of old icons
			_entries[i].nID = oldIcon._entries[i].nID;
			IFDEBUG( _tprintf(_T("replacing icon %d\n"), _entries[i].nID); )
		} else { // if new icon group has more icons, allocate new IDs
			if(nMaxID == 0xFFFF && (nMaxID = getMaxIconId(lpFileName)) == 0xFFFF) return false;
			nMaxID++;
			_entries[i].nID = nMaxID;
			IFDEBUG( _tprintf(_T("adding icon %d\n"), _entries[i].nID); )
		}
	}

	// _tchmod(lpFileName,_S_IWRITE); // if needed...
	HANDLE hUpdate = BeginUpdateResource(lpFileName, FALSE);
	if(hUpdate==NULL) { _tprintf(_T("Unable to update resource\n")); return false; }

	{
	// Build icon group resource
	WORD cbRes = sizeof(ICONDIR) + count()*sizeof_iconDirEntry_EXE;
	BYTE* resData = new BYTE[cbRes];
	LPBYTE writePtr = resData;
#define _myWrite(ptr, size) { CopyMemory(writePtr, ptr, size); writePtr += size; }
	_myWrite(&_head, sizeof(_head));
	for(int i=0; i<count(); ++i)
		_myWrite(&_entries[i], sizeof_iconDirEntry_EXE);
#undef _myWrite

	// Replace icon group
	if(!UpdateResource(hUpdate, RT_GROUP_ICON, lpResName, resLangId, resData, cbRes)) 
	{
		_tprintf(_T("Unable to update icon group\n")); delete[] resData; return false;
	}
	IFDEBUG( _tprintf(_T("Updated group %d (lang %d)\n"), lpResName, resLangId); )
	delete [] resData;
	}

	// Replace/add icons
	for(int i=0; i<count(); ++i)
	{
		if(!UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(_entries[i].nID), resLangId, _imagesData[i], _entries[i].dwBytesInRes)) { _tprintf(_T("Unable to update icon %d\n"), _entries[i].nID); return false; }
		IFDEBUG( _tprintf(_T("Updated icon %d (lang %d)\n"), _entries[i].nID, resLangId); )
	}

	// Delete unused icons
	for(int i=count(); i<oldIcon.count(); ++i)
	{
		if(!UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(oldIcon._entries[i].nID), resLangId, NULL, 0)) { _tprintf(_T("Unable to delete icon %d\n"), oldIcon._entries[i].nID); }
		IFDEBUG( _tprintf(_T("Removed icon %d (lang %d)\n"), oldIcon._entries[i].nID, resLangId); )
	}

	if(!EndUpdateResource(hUpdate,FALSE)) { _tprintf(_T("Error in EndUpdateResource\n")); }
	IFDEBUG( _tprintf(_T("EndUpdateResource\n")); )

	return true;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	IFDEBUG( printf("sizeof(TCHAR)=%d\n", sizeof(TCHAR)) );
	if(argc != 5) {
		_tprintf(_T("Usage: %s source.ico destination.exe icon_group# icon_lang#\n"), argv[0]);
		return 0;
	}

	TCHAR* szICOname = argv[1];
	TCHAR* szEXEname = argv[2];
	int groupId = _ttoi(argv[3]);
	int langId = _ttoi(argv[4]);
	
	IFDEBUG( _tprintf(_T("ICO='%s' EXE='%s' group#=%d lang#=%d\n"), szICOname, szEXEname, groupId, langId); )

	Icon newIcon; if(!newIcon.readICO(szICOname)) return false;
	newIcon.writeToEXE(szEXEname, MAKEINTRESOURCE(groupId), langId);

	return nRetCode;
}
