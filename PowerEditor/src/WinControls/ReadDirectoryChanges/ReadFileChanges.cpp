#include "ReadFileChanges.h"



BOOL CReadFileChanges::DetectChanges()
{
	WIN32_FILE_ATTRIBUTE_DATA fInfo{};
	fInfo.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	BOOL rValue = FALSE;

	::GetFileAttributesEx(_szFile, GetFileExInfoStandard, &fInfo);

	if ((_dwNotifyFilter & FILE_NOTIFY_CHANGE_SIZE) && (fInfo.nFileSizeHigh != _lastFileInfo.nFileSizeHigh || fInfo.nFileSizeLow != _lastFileInfo.nFileSizeLow))
	{
		rValue = TRUE;
	}

	if ((_dwNotifyFilter & FILE_NOTIFY_CHANGE_LAST_WRITE) && (fInfo.ftLastWriteTime.dwHighDateTime != _lastFileInfo.ftLastWriteTime.dwHighDateTime || fInfo.ftLastWriteTime.dwLowDateTime != _lastFileInfo.ftLastWriteTime.dwLowDateTime))
	{
		rValue = TRUE;
	}

	_lastFileInfo = fInfo;
	return rValue;
}

void CReadFileChanges::AddFile(LPCTSTR szFile, DWORD dwNotifyFilter)
{
	_szFile = szFile;
	_dwNotifyFilter = dwNotifyFilter;
	::GetFileAttributesEx(szFile, GetFileExInfoStandard, &_lastFileInfo);
}


void CReadFileChanges::Terminate()
{
	_szFile = NULL;
	_dwNotifyFilter = 0;
}
