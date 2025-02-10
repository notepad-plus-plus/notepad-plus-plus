// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <windows.h>
#include <iso646.h>
#include <cstdint>
#include <unordered_set>
#include <algorithm>
#include <tchar.h>

#pragma deprecated(PathFileExists)  // Use doesFileExist, doesDirectoryExist or doesPathExist (for file or directory) instead.
#pragma deprecated(PathIsDirectory) // Use doesDirectoryExist instead.


const bool dirUp = true;
const bool dirDown = false;

#define NPP_CP_WIN_1252           1252
#define NPP_CP_DOS_437            437
#define NPP_CP_BIG5               950

#define LINKTRIGGERED WM_USER+555

#define BCKGRD_COLOR (RGB(255,102,102))
#define TXT_COLOR    (RGB(255,255,255))

#ifndef __MINGW32__
#define WCSTOK wcstok
#else
#define WCSTOK wcstok_s
#endif


#define NPP_INTERNAL_FUCTION_STR L"Notepad++::InternalFunction"


std::wstring folderBrowser(HWND parent, const std::wstring & title = L"", int outputCtrlID = 0, const wchar_t *defaultStr = NULL);
std::wstring getFolderName(HWND parent, const wchar_t *defaultDir = NULL);

void printInt(int int2print);
void printStr(const wchar_t *str2print);
std::wstring commafyInt(size_t n);

void writeLog(const wchar_t* logFileName, const char* log2write);
void writeLog(const wchar_t* logFileName, const wchar_t* log2write);
int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);
std::wstring purgeMenuItemString(const wchar_t* menuItemStr, bool keepAmpersand = false);
std::vector<std::wstring> tokenizeString(const std::wstring & tokenString, const char delim);

void ClientRectToScreenRect(HWND hWnd, RECT* rect);
void ScreenRectToClientRect(HWND hWnd, RECT* rect);

std::wstring string2wstring(const std::string & rString, UINT codepage);
std::string wstring2string(const std::wstring & rwString, UINT codepage);
bool isInList(const wchar_t *token, const wchar_t *list);
std::wstring BuildMenuFileName(int filenameLen, unsigned int pos, const std::wstring &filename, bool ordinalNumber = true);

std::string getFileContent(const wchar_t *file2read);
std::wstring relativeFilePathToFullFilePath(const wchar_t *relativeFilePath);
void writeFileContent(const wchar_t *file2write, const char *content2write);
bool matchInList(const wchar_t *fileName, const std::vector<std::wstring> & patterns);
bool matchInExcludeDirList(const wchar_t* dirName, const std::vector<std::wstring>& patterns, size_t level);
bool allPatternsAreExclusion(const std::vector<std::wstring>& patterns);

class WcharMbcsConvertor final
{
public:
	static WcharMbcsConvertor& getInstance() {
		static WcharMbcsConvertor instance;
		return instance;
	}

	const wchar_t * char2wchar(const char *mbStr, size_t codepage, int lenMbcs =-1, int* pLenOut=NULL, int* pBytesNotProcessed=NULL);
	const wchar_t * char2wchar(const char *mbcs2Convert, size_t codepage, intptr_t* mstart, intptr_t* mend);
	const char * wchar2char(const wchar_t *wcStr, size_t codepage, int lenIn = -1, int* pLenOut = NULL);
	const char * wchar2char(const wchar_t *wcStr, size_t codepage, intptr_t* mstart, intptr_t* mend);

	const char * encode(UINT fromCodepage, UINT toCodepage, const char *txt2Encode, int lenIn = -1, int* pLenOut=NULL, int* pBytesNotProcessed=NULL)
	{
		int lenWc = 0;
        const wchar_t * strW = char2wchar(txt2Encode, fromCodepage, lenIn, &lenWc, pBytesNotProcessed);
        return wchar2char(strW, toCodepage, lenWc, pLenOut);
    }

protected:
	WcharMbcsConvertor() = default;
	~WcharMbcsConvertor() = default;

	// Since there's no public ctor, we need to void the default assignment operator and copy ctor.
	// Since these are marked as deleted does not matter under which access specifier are kept
	WcharMbcsConvertor(const WcharMbcsConvertor&) = delete;
	WcharMbcsConvertor& operator= (const WcharMbcsConvertor&) = delete;

	// No move ctor and assignment
	WcharMbcsConvertor(WcharMbcsConvertor&&) = delete;
	WcharMbcsConvertor& operator= (WcharMbcsConvertor&&) = delete;

	template <class T>
	class StringBuffer final
	{
	public:
		~StringBuffer() { if (_allocLen) delete[] _str; }

		void sizeTo(size_t size)
		{
			if (_allocLen < size)
			{
				if (_allocLen)
					delete[] _str;
				_allocLen = std::max<size_t>(size, initSize);
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


#define REBARBAND_SIZE sizeof(REBARBANDINFO)

std::wstring pathRemoveFileSpec(std::wstring & path);
std::wstring pathAppend(std::wstring &strDest, const std::wstring & str2append);
COLORREF getCtrlBgColor(HWND hWnd);
std::wstring stringToUpper(std::wstring strToConvert);
std::wstring stringToLower(std::wstring strToConvert);
std::wstring stringReplace(std::wstring subject, const std::wstring& search, const std::wstring& replace);
void stringSplit(const std::wstring& input, const std::wstring& delimiter, std::vector<std::wstring>& output);
bool str2numberVector(std::wstring str2convert, std::vector<size_t>& numVect);
void stringJoin(const std::vector<std::wstring>& strings, const std::wstring& separator, std::wstring& joinedString);
std::wstring stringTakeWhileAdmissable(const std::wstring& input, const std::wstring& admissable);
double stodLocale(const std::wstring& str, _locale_t loc, size_t* idx = NULL);

bool str2Clipboard(const std::wstring &str2cpy, HWND hwnd);
class Buffer;
bool buf2Clipboard(const std::vector<Buffer*>& buffers, bool isFullPath, HWND hwnd);

std::wstring GetLastErrorAsString(DWORD errorCode = 0);

std::wstring intToString(int val);
std::wstring uintToString(unsigned int val);

HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText, bool isRTL);
HWND CreateToolTipRect(int toolID, HWND hWnd, HINSTANCE hInst, const PTSTR pszText, const RECT rc);

bool isCertificateValidated(const std::wstring & fullFilePath, const std::wstring & subjectName2check);
bool isAssoCommandExisting(LPCTSTR FullPathName);

bool deleteFileOrFolder(const std::wstring& f2delete);

void getFilesInFolder(std::vector<std::wstring>& files, const std::wstring& extTypeFilter, const std::wstring& inFolder);

template<typename T> size_t vecRemoveDuplicates(std::vector<T>& vec, bool isSorted = false, bool canSort = false)
{
	if (!isSorted && canSort)
	{
		std::sort(vec.begin(), vec.end());
		isSorted = true;
	}

	if (isSorted)
	{
		typename std::vector<T>::iterator it;
		it = std::unique(vec.begin(), vec.end());
		vec.resize(distance(vec.begin(), it));  // unique() does not shrink the vector
	}
	else
	{
		std::unordered_set<T> seen;
		auto newEnd = std::remove_if(vec.begin(), vec.end(), [&seen](const T& value)
			{
				return !seen.insert(value).second;
			});
		vec.erase(newEnd, vec.end());
	}
	return vec.size();
}

void trim(std::wstring& str);

int nbDigitsFromNbLines(size_t nbLines);

std::wstring getDateTimeStrFrom(const std::wstring& dateTimeFormat, const SYSTEMTIME& st);

HFONT createFont(const wchar_t* fontName, int fontSize, bool isBold, HWND hDestParent);
bool removeReadOnlyFlagFromFileAttributes(const wchar_t* fileFullPath);

bool isWin32NamespacePrefixedFileName(const std::wstring& fileName);
bool isWin32NamespacePrefixedFileName(const wchar_t* szFileName);
bool isUnsupportedFileName(const std::wstring& fileName);
bool isUnsupportedFileName(const wchar_t* szFileName);

class Version final
{
public:
	Version() = default;
	Version(const std::wstring& versionStr);

	void setVersionFrom(const std::wstring& filePath);
	std::wstring toString();
	bool isNumber(const std::wstring& s) const {
		return !s.empty() &&
			find_if(s.begin(), s.end(), [](wchar_t c) { return !_istdigit(c); }) == s.end();
	};

	int compareTo(const Version& v2c) const;

	bool operator < (const Version& v2c) const {
		return compareTo(v2c) == -1;
	};

	bool operator <= (const Version& v2c) const {
		int r = compareTo(v2c);
		return r == -1 || r == 0;
	};

	bool operator > (const Version& v2c) const {
		return compareTo(v2c) == 1;
	};

	bool operator >= (const Version& v2c) const {
		int r = compareTo(v2c);
		return r == 1 || r == 0;
	};

	bool operator == (const Version& v2c) const {
		return compareTo(v2c) == 0;
	};

	bool operator != (const Version& v2c) const {
		return compareTo(v2c) != 0;
	};

	bool empty() const {
		return _major == 0 && _minor == 0 && _patch == 0 && _build == 0;
	}

	bool isCompatibleTo(const Version& from, const Version& to) const;

private:
	unsigned long _major = 0;
	unsigned long _minor = 0;
	unsigned long _patch = 0;
	unsigned long _build = 0;
};


BOOL getDiskFreeSpaceWithTimeout(const wchar_t* dirPath, ULARGE_INTEGER* freeBytesForUser,
	DWORD milliSec2wait = 0, bool* isTimeoutReached = nullptr);
BOOL getFileAttributesExWithTimeout(const wchar_t* filePath, WIN32_FILE_ATTRIBUTE_DATA* fileAttr,
	DWORD milliSec2wait = 0, bool* isTimeoutReached = nullptr, DWORD* pdwWin32ApiError = nullptr);

bool doesFileExist(const wchar_t* filePath, DWORD milliSec2wait = 0, bool* isTimeoutReached = nullptr);
bool doesDirectoryExist(const wchar_t* dirPath, DWORD milliSec2wait = 0, bool* isTimeoutReached = nullptr);
bool doesPathExist(const wchar_t* path, DWORD milliSec2wait = 0, bool* isTimeoutReached = nullptr);


// check if the window rectangle intersects with any currently active monitor's working area
bool isWindowVisibleOnAnyMonitor(const RECT& rectWndIn);

bool isCoreWindows();
