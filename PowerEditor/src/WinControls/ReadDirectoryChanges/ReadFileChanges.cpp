#include "ReadFileChanges.h"



CReadFileChanges::CReadFileChanges()
{
}


CReadFileChanges::~CReadFileChanges()
{
}


BOOL CReadFileChanges::DetectChanges() {

	WIN32_FILE_ATTRIBUTE_DATA fInfo;
	BOOL rValue = FALSE;
	::GetFileAttributesEx(szFile, GetFileExInfoStandard, &fInfo);

	if ((dwNotifyFilter & FILE_NOTIFY_CHANGE_SIZE) && (fInfo.nFileSizeHigh != lastFileInfo.nFileSizeHigh || fInfo.nFileSizeLow != lastFileInfo.nFileSizeLow)) {
		rValue = TRUE;
	}

	if ((dwNotifyFilter & FILE_NOTIFY_CHANGE_LAST_WRITE) && (fInfo.ftLastWriteTime.dwHighDateTime != lastFileInfo.ftLastWriteTime.dwHighDateTime || fInfo.ftLastWriteTime.dwLowDateTime != lastFileInfo.ftLastWriteTime.dwLowDateTime)) {
		rValue = TRUE;
	}

	lastFileInfo = fInfo;
	return rValue;
}

void CReadFileChanges::AddFile(LPCTSTR szFile, DWORD dwNotifyFilter)
{
	this->szFile = szFile;
	this->dwNotifyFilter = dwNotifyFilter;
	::GetFileAttributesEx(szFile, GetFileExInfoStandard, &lastFileInfo);
}


void CReadFileChanges::Terminate()
{
	this->szFile = NULL;
	this->dwNotifyFilter = 0;
}
