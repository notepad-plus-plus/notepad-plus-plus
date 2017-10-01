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
#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <windows.h>
#include <iso646.h>
#include <cstdint>


const bool dirUp = true;
const bool dirDown = false;

#define NPP_CP_WIN_1252           1252
#define NPP_CP_DOS_437            437
#define NPP_CP_BIG5               950

#define LINKTRIGGERED WM_USER+555

#define BCKGRD_COLOR (RGB(255,102,102))
#define TXT_COLOR    (RGB(255,255,255))

#define generic_strtol wcstol
#define generic_strncpy wcsncpy
#define generic_stricmp wcsicmp
#define generic_strncmp wcsncmp
#define generic_strnicmp wcsnicmp
#define generic_strncat wcsncat
#define generic_strchr wcschr
#define generic_atoi _wtoi
#define generic_itoa _itow
#define generic_atof _wtof
#define generic_strtok wcstok
#define generic_strftime wcsftime
#define generic_fprintf fwprintf
#define generic_sprintf swprintf
#define generic_sscanf swscanf
#define generic_fopen _wfopen
#define generic_fgets fgetws
#define generic_stat _wstat
#define COPYDATA_FILENAMES COPYDATA_FILENAMESW

typedef std::basic_string<TCHAR> generic_string;
typedef std::basic_stringstream<TCHAR> generic_stringstream;

generic_string folderBrowser(HWND parent, const generic_string & title = TEXT(""), int outputCtrlID = 0, const TCHAR *defaultStr = NULL);
generic_string getFolderName(HWND parent, const TCHAR *defaultDir = NULL);

void printInt(int int2print);
void printStr(const TCHAR *str2print);
generic_string commafyInt(size_t n);

void writeLog(const TCHAR *logFileName, const char *log2write);
int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);
generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand = false);
std::vector<generic_string> tokenizeString(const generic_string & tokenString, const char delim);

void ClientRectToScreenRect(HWND hWnd, RECT* rect);
void ScreenRectToClientRect(HWND hWnd, RECT* rect);

std::wstring string2wstring(const std::string & rString, UINT codepage);
std::string wstring2string(const std::wstring & rwString, UINT codepage);
bool isInList(const TCHAR *token, const TCHAR *list);
generic_string BuildMenuFileName(int filenameLen, unsigned int pos, const generic_string &filename);

std::string getFileContent(const TCHAR *file2read);
generic_string relativeFilePathToFullFilePath(const TCHAR *relativeFilePath);
void writeFileContent(const TCHAR *file2write, const char *content2write);
bool matchInList(const TCHAR *fileName, const std::vector<generic_string> & patterns);

class WcharMbcsConvertor final
{
public:
	static WcharMbcsConvertor * getInstance() {return _pSelf;}
	static void destroyInstance() {delete _pSelf;}

	const wchar_t * char2wchar(const char *mbStr, UINT codepage, int lenIn=-1, int *pLenOut=NULL, int *pBytesNotProcessed=NULL);
	const wchar_t * char2wchar(const char *mbcs2Convert, UINT codepage, int *mstart, int *mend);
	const char * wchar2char(const wchar_t *wcStr, UINT codepage, int lenIn = -1, int *pLenOut = NULL);
	const char * wchar2char(const wchar_t *wcStr, UINT codepage, long *mstart, long *mend);

	const char * encode(UINT fromCodepage, UINT toCodepage, const char *txt2Encode, int lenIn=-1, int *pLenOut=NULL, int *pBytesNotProcessed=NULL)
	{
		int lenWc = 0;
        const wchar_t * strW = char2wchar(txt2Encode, fromCodepage, lenIn, &lenWc, pBytesNotProcessed);
        return wchar2char(strW, toCodepage, lenWc, pLenOut);
    }

protected:
	WcharMbcsConvertor() {}
	~WcharMbcsConvertor() {}

	// Since there's no public ctor, we need to void the default assignment operator and copy ctor.
	// Since these are marked as deleted does not matter under which access specifier are kept
	WcharMbcsConvertor(const WcharMbcsConvertor&) = delete;
	WcharMbcsConvertor& operator= (const WcharMbcsConvertor&) = delete;

	static WcharMbcsConvertor* _pSelf;

	template <class T>
	class StringBuffer final
	{
	public:
		~StringBuffer() { if(_allocLen) delete[] _str; }

		void sizeTo(size_t size)
		{
			if (_allocLen < size)
			{
				if (_allocLen)
					delete[] _str;
				_allocLen = max(size, initSize);
				_str = new T[_allocLen];
			}
		}

		void empty()
		{
			static T nullStr = 0; // routines may return an empty string, with null terminator, without allocating memory; a pointer to this null character will be returned in that case
			if (_allocLen == 0)
				_str = &nullStr;
			else
				_str[0] = 0;
		}

		operator T* () { return _str; }
		operator const T* () const { return _str; }

	protected:
		static const int initSize = 1024;
		size_t _allocLen = 0;
		T* _str = nullptr;
	};

	StringBuffer<char> _multiByteStr;
	StringBuffer<wchar_t> _wideCharStr;
};



#define MACRO_RECORDING_IN_PROGRESS 1
#define MACRO_RECORDING_HAS_STOPPED 2

#define REBARBAND_SIZE sizeof(REBARBANDINFO)

generic_string PathRemoveFileSpec(generic_string & path);
generic_string PathAppend(generic_string &strDest, const generic_string & str2append);
COLORREF getCtrlBgColor(HWND hWnd);
generic_string stringToUpper(generic_string strToConvert);
generic_string stringToLower(generic_string strToConvert);
generic_string stringReplace(generic_string subject, const generic_string& search, const generic_string& replace);
std::vector<generic_string> stringSplit(const generic_string& input, const generic_string& delimiter);
generic_string stringJoin(const std::vector<generic_string>& strings, const generic_string& separator);
generic_string stringTakeWhileAdmissable(const generic_string& input, const generic_string& admissable);
double stodLocale(const generic_string& str, _locale_t loc, size_t* idx = NULL);

int OrdinalIgnoreCaseCompareStrings(LPCTSTR sz1, LPCTSTR sz2);

bool str2Clipboard(const generic_string &str2cpy, HWND hwnd);

generic_string GetLastErrorAsString(DWORD errorCode = 0);

generic_string intToString(int val);
generic_string uintToString(unsigned int val);

HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText);

bool isCertificateValidated(const generic_string & fullFilePath, const generic_string & subjectName2check);
bool isAssoCommandExisting(LPCTSTR FullPathName);
