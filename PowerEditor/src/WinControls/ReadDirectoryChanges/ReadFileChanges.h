#pragma once


#include <stdio.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif


#include <windows.h>


class CReadFileChanges
{
public:
	CReadFileChanges() {
		_lastFileInfo.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	};
	~CReadFileChanges() {};
	void AddFile(LPCTSTR szDirectory, DWORD dwNotifyFilter);
	BOOL DetectChanges();
	void Terminate();

private:
	LPCTSTR _szFile = nullptr;
	DWORD _dwNotifyFilter = 0;
	WIN32_FILE_ATTRIBUTE_DATA _lastFileInfo = {};
};

