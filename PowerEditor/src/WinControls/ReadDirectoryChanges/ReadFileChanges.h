#pragma once


#include <stdio.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif


#include <windows.h>


class CReadFileChanges
{
public:
	CReadFileChanges();
	~CReadFileChanges();
	void AddFile(LPCTSTR szDirectory, DWORD dwNotifyFilter);
	BOOL DetectChanges();
	void Terminate();

private:
	LPCTSTR _szFile;
	DWORD _dwNotifyFilter;
	WIN32_FILE_ATTRIBUTE_DATA _lastFileInfo;

};

