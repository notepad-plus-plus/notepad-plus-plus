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


#include <algorithm>
#include <shlwapi.h>
#include <Shlobj.h>
#include <uxtheme.h>
#include "StaticDialog.h"



#include "Common.h"
#include "../Utf8.h"

WcharMbcsConvertor * WcharMbcsConvertor::_pSelf = new WcharMbcsConvertor;

void printInt(int int2print) 
{
	TCHAR str[32];
	wsprintf(str, TEXT("%d"), int2print);
	::MessageBox(NULL, str, TEXT(""), MB_OK);
};

void printStr(const TCHAR *str2print) 
{
	::MessageBox(NULL, str2print, TEXT(""), MB_OK);
};

std::string getFileContent(const TCHAR *file2read)
{
	const size_t blockSize = 1024;
	char data[blockSize];
	std::string wholeFileContent = "";
	FILE *fp = generic_fopen(file2read, TEXT("rb"));

	size_t lenFile = 0;
	do {
		lenFile = fread(data, 1, blockSize - 1, fp);
		if (lenFile <= 0) break;

		if (lenFile >= blockSize - 1)
			data[blockSize - 1] = '\0';
		else
			data[lenFile] = '\0';

		wholeFileContent += data;

	} while (lenFile > 0);

	fclose(fp);
	return wholeFileContent;
}


char getDriveLetter()
{
	char drive = '\0';
	TCHAR current[MAX_PATH];

	::GetCurrentDirectory(MAX_PATH, current);
	int driveNbr = ::PathGetDriveNumber(current);
	if (driveNbr != -1)
		drive = 'A' + char(driveNbr);

	return drive;
}

generic_string relativeFilePathToFullFilePath(const TCHAR *relativeFilePath)
{
	generic_string fullFilePathName = TEXT("");
	TCHAR fullFileName[MAX_PATH];
	BOOL isRelative = ::PathIsRelative(relativeFilePath);

	if (isRelative)
	{
		::GetFullPathName(relativeFilePath, MAX_PATH, fullFileName, NULL);
		fullFilePathName += fullFileName;
	}
	else
	{
		if ((relativeFilePath[0] == '\\' && relativeFilePath[1] != '\\') || relativeFilePath[0] == '/')
		{
			fullFilePathName += getDriveLetter();
			fullFilePathName += ':';
		}
		fullFilePathName += relativeFilePath;
	}

	return fullFilePathName;
}

void writeFileContent(const TCHAR *file2write, const char *content2write)
{	
	FILE *f = generic_fopen(file2write, TEXT("w+"));
	fwrite(content2write, sizeof(content2write[0]), strlen(content2write), f);
	fflush(f);
	fclose(f);
}

void writeLog(const TCHAR *logFileName, const char *log2write)
{	
	FILE *f = generic_fopen(logFileName, TEXT("a+"));
	fwrite(log2write, sizeof(log2write[0]), strlen(log2write), f);
	fputc('\n', f);
	fflush(f);
	fclose(f);
}

// Set a call back with the handle after init to set the path.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/callbackfunctions/browsecallbackproc.asp
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED && pData != 0)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
};

void folderBrowser(HWND parent, int outputCtrlID, const TCHAR *defaultStr)
{
	// This code was copied and slightly modifed from:
	// http://www.bcbdev.com/faqs/faq62.htm

	// SHBrowseForFolder returns a PIDL. The memory for the PIDL is
	// allocated by the shell. Eventually, we will need to free this
	// memory, so we need to get a pointer to the shell malloc COM
	// object that will free the PIDL later on.
	LPMALLOC pShellMalloc = 0;
	if (::SHGetMalloc(&pShellMalloc) == NO_ERROR)
	{
		// If we were able to get the shell malloc object,
		// then proceed by initializing the BROWSEINFO stuct
		BROWSEINFO info;
		memset(&info, 0, sizeof(info));
		info.hwndOwner = parent;
		info.pidlRoot = NULL;
		TCHAR szDisplayName[MAX_PATH];
		info.pszDisplayName = szDisplayName;
		info.lpszTitle = TEXT("Select a folder to search from");
		info.ulFlags = 0;
		info.lpfn = BrowseCallbackProc;
		TCHAR directory[MAX_PATH];
		::GetDlgItemText(parent, outputCtrlID, directory, _countof(directory));
		directory[_countof(directory) - 1] = '\0';

		if (!directory[0] && defaultStr)
			info.lParam = reinterpret_cast<LPARAM>(defaultStr);
		else
			info.lParam = reinterpret_cast<LPARAM>(directory);

		// Execute the browsing dialog.
		LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

		// pidl will be null if they cancel the browse dialog.
		// pidl will be not null when they select a folder.
		if (pidl) 
		{
			// Try to convert the pidl to a display generic_string.
			// Return is true if success.
			TCHAR szDir[MAX_PATH];
			if (::SHGetPathFromIDList(pidl, szDir))
				// Set edit control to the directory path.
				::SetDlgItemText(parent, outputCtrlID, szDir);
			pShellMalloc->Free(pidl);
		}
		pShellMalloc->Release();
	}
}


generic_string getFolderName(HWND parent, const TCHAR *defaultDir)
{
	generic_string folderName(TEXT(""));
	LPMALLOC pShellMalloc = 0;
	if (::SHGetMalloc(&pShellMalloc) == NO_ERROR)
	{
		BROWSEINFO info;
		memset(&info, 0, sizeof(info));
		info.hwndOwner = parent;
		info.pidlRoot = NULL;
		TCHAR szDisplayName[MAX_PATH];
		info.pszDisplayName = szDisplayName;
		info.lpszTitle = TEXT("Select a folder");
		info.ulFlags = 0;
		info.lpfn = BrowseCallbackProc;
		info.lParam = reinterpret_cast<LPARAM>(defaultDir);

		// Execute the browsing dialog.
		LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

		// pidl will be null if they cancel the browse dialog.
		// pidl will be not null when they select a folder.
		if (pidl) 
		{
			// Try to convert the pidl to a display generic_string.
			// Return is true if success.
			TCHAR szDir[MAX_PATH];
			if (::SHGetPathFromIDList(pidl, szDir))
				// Set edit control to the directory path.
				folderName = szDir;
			pShellMalloc->Free(pidl);
		}
		pShellMalloc->Release();
	}
	return folderName;
}


void ClientRectToScreenRect(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};

std::vector<generic_string> tokenizeString(const generic_string & tokenString, const char delim) {
	//Vector is created on stack and copied on return
	std::vector<generic_string> tokens;

    // Skip delimiters at beginning.
	generic_string::size_type lastPos = tokenString.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    generic_string::size_type pos     = tokenString.find_first_of(delim, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(tokenString.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = tokenString.find_first_not_of(delim, pos);
        // Find next "non-delimiter"
        pos = tokenString.find_first_of(delim, lastPos);
    }
	return tokens;
}

void ScreenRectToClientRect(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
};

int filter(unsigned int code, struct _EXCEPTION_POINTERS *) 
{
    if (code == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;

    return EXCEPTION_CONTINUE_SEARCH;
}

bool isInList(const TCHAR *token, const TCHAR *list) {
	if ((!token) || (!list))
		return false;
	TCHAR word[64];
	size_t i = 0;
	size_t j = 0;
	for (size_t len = lstrlen(list); i <= len; ++i)
	{
		if ((list[i] == ' ')||(list[i] == '\0'))
		{
			if (j != 0)
			{
				word[j] = '\0';
				j = 0;
				
				if (!generic_stricmp(token, word))
					return true;
			}
		}
		else 
		{
			word[j] = list[i];
			++j;
		}
	}
	return false;
}


generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand)
{
	TCHAR cleanedName[64] = TEXT("");
	size_t j = 0;
	size_t menuNameLen = lstrlen(menuItemStr);
	for(size_t k = 0 ; k < menuNameLen ; ++k) 
	{
		if (menuItemStr[k] == '\t')
		{
			cleanedName[k] = 0;
			break;
		}
		else if (menuItemStr[k] == '&')
		{
			if (keepAmpersand)
				cleanedName[j++] = menuItemStr[k];
			//else skip
		}
		else
		{
			cleanedName[j++] = menuItemStr[k];
		}
	}
	cleanedName[j] = 0;
	return cleanedName;
};

const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, UINT codepage, int lenMbcs, int *pLenWc, int *pBytesNotProcessed)
{
	// Do not process NULL pointer
	if (!mbcs2Convert) return NULL;

	// Do not process empty strings
	if (lenMbcs == 0 || lenMbcs == -1 && mbcs2Convert[0] == 0) { _wideCharStr.empty(); return _wideCharStr;	}

	int bytesNotProcessed = 0;
	int lenWc = 0;

	// If length not specified, simply convert without checking
	if (lenMbcs == -1)
	{
		lenWc = MultiByteToWideChar(codepage, 0, mbcs2Convert, lenMbcs, NULL, 0);
	}
	// Otherwise, test if we are cutting a multi-byte character at end of buffer
	else if(lenMbcs != -1 && codepage == CP_UTF8) // For UTF-8, we know how to test it
	{
		int indexOfLastChar = Utf8::characterStart(mbcs2Convert, lenMbcs-1); // get index of last character
		if (indexOfLastChar != 0 && !Utf8::isValid(mbcs2Convert+indexOfLastChar, lenMbcs-indexOfLastChar)) // if it is not valid we do not process it right now (unless its the only character in string, to ensure that we always progress, e.g. that bytesNotProcessed < lenMbcs)
		{
			bytesNotProcessed = lenMbcs-indexOfLastChar;
		}
		lenWc = MultiByteToWideChar(codepage, 0, mbcs2Convert, lenMbcs-bytesNotProcessed, NULL, 0);
	}
	else // For other encodings, ask system if there are any invalid characters; note that it will not correctly know if last character is cut when there are invalid characters inside the text
	{
		lenWc = MultiByteToWideChar(codepage, (lenMbcs == -1) ? 0 : MB_ERR_INVALID_CHARS, mbcs2Convert, lenMbcs, NULL, 0);
		if (lenWc == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
		{
			// Test without last byte
			if (lenMbcs > 1) lenWc = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, mbcs2Convert, lenMbcs-1, NULL, 0);
			if (lenWc == 0) // don't have to check that the error is still ERROR_NO_UNICODE_TRANSLATION, since only the length parameter changed
			{
				// TODO: should warn user about incorrect loading due to invalid characters
				// We still load the file, but the system will either strip or replace invalid characters (including the last character, if cut in half)
				lenWc = MultiByteToWideChar(codepage, 0, mbcs2Convert, lenMbcs, NULL, 0);
			}
			else
			{
				// We found a valid text by removing one byte.
				bytesNotProcessed = 1;
			}
		}
	}

	if (lenWc > 0)
	{
		_wideCharStr.sizeTo(lenWc);
		MultiByteToWideChar(codepage, 0, mbcs2Convert, lenMbcs-bytesNotProcessed, _wideCharStr, lenWc);
	}
	else
		_wideCharStr.empty();

	if(pLenWc) *pLenWc = lenWc;
	if(pBytesNotProcessed) *pBytesNotProcessed = bytesNotProcessed;
	return _wideCharStr;
}

// "mstart" and "mend" are pointers to indexes in mbcs2Convert,
// which are converted to the corresponding indexes in the returned wchar_t string.
const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, UINT codepage, int *mstart, int *mend)
{
	// Do not process NULL pointer
	if (!mbcs2Convert) return NULL;

	int len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, NULL, 0);
	if (len > 0)
	{
		_wideCharStr.sizeTo(len);
		len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, len);

		if ((size_t)*mstart < strlen(mbcs2Convert) && (size_t)*mend <= strlen(mbcs2Convert))
		{
			*mstart = MultiByteToWideChar(codepage, 0, mbcs2Convert, *mstart, _wideCharStr, 0);
			*mend   = MultiByteToWideChar(codepage, 0, mbcs2Convert, *mend, _wideCharStr, 0);
			if (*mstart >= len || *mend >= len)
			{
				*mstart = 0;
				*mend = 0;
			}
		}
	}
	else
	{
		_wideCharStr.empty();
		*mstart = 0;
		*mend = 0;
	}
	return _wideCharStr;
} 

const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, UINT codepage, int lenWc, int *pLenMbcs) 
{
	// Do not process NULL pointer
	if (!wcharStr2Convert) return NULL;

	int lenMbcs = WideCharToMultiByte(codepage, 0, wcharStr2Convert, lenWc, NULL, 0, NULL, NULL);
	if (lenMbcs > 0)
	{
		_multiByteStr.sizeTo(lenMbcs);
		WideCharToMultiByte(codepage, 0, wcharStr2Convert, lenWc, _multiByteStr, lenMbcs, NULL, NULL);
	}
	else
		_multiByteStr.empty();

	if(pLenMbcs) *pLenMbcs = lenMbcs;
	return _multiByteStr;
}

const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, UINT codepage, long *mstart, long *mend) 
{
	// Do not process NULL pointer
	if (!wcharStr2Convert) return NULL;

	int len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, NULL, 0, NULL, NULL);
	if (len > 0)
	{
		_multiByteStr.sizeTo(len);
		len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, len, NULL, NULL); // not needed?

        if ((int)*mstart < lstrlenW(wcharStr2Convert) && (int)*mend < lstrlenW(wcharStr2Convert))
        {
			*mstart = WideCharToMultiByte(codepage, 0, wcharStr2Convert, *mstart, NULL, 0, NULL, NULL);
			*mend = WideCharToMultiByte(codepage, 0, wcharStr2Convert, *mend, NULL, 0, NULL, NULL);
			if (*mstart >= len || *mend >= len)
			{
				*mstart = 0;
				*mend = 0;
			}
		}
	}
	else
		_multiByteStr.empty();

	return _multiByteStr;
}

std::wstring string2wstring(const std::string & rString, UINT codepage)
{
	int len = MultiByteToWideChar(codepage, 0, rString.c_str(), -1, NULL, 0);
	if(len > 0)
	{		
		std::vector<wchar_t> vw(len);
		MultiByteToWideChar(codepage, 0, rString.c_str(), -1, &vw[0], len);
		return &vw[0];
	}
	else
		return L"";
}

std::string wstring2string(const std::wstring & rwString, UINT codepage)
{
	int len = WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, NULL, 0, NULL, NULL);
	if(len > 0)
	{		
		std::vector<char> vw(len);
		WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, &vw[0], len, NULL, NULL);
		return &vw[0];
	}
	else
		return "";
}

// Escapes ampersands in file name to use it in menu
template <typename T>
generic_string convertFileName(T beg, T end)
{
	generic_string strTmp;

	for (T it = beg; it != end; ++it)
	{
		if (*it == '&') strTmp.push_back('&');
		strTmp.push_back(*it);
	}

	return strTmp;
}

generic_string intToString(int val)
{
	std::vector<TCHAR> vt;
	bool isNegative = val < 0;
	// can't use abs here because std::numeric_limits<int>::min() has no positive representation
	//val = std::abs(val);

	vt.push_back('0' + (TCHAR)(std::abs(val % 10)));
	val /= 10;
	while (val != 0) {
		vt.push_back('0' + (TCHAR)(std::abs(val % 10)));
		val /= 10;
	}

	if (isNegative)
		vt.push_back('-');

	return generic_string(vt.rbegin(), vt.rend());
}

generic_string uintToString(unsigned int val)
{
	std::vector<TCHAR> vt;

	vt.push_back('0' + (TCHAR)(val % 10));
	val /= 10;
	while (val != 0) {
		vt.push_back('0' + (TCHAR)(val % 10));
		val /= 10;
	}

	return generic_string(vt.rbegin(), vt.rend());
}

// Build Recent File menu entries from given 
generic_string BuildMenuFileName(int filenameLen, unsigned int pos, const generic_string &filename)
{
	generic_string strTemp;

	if (pos < 9)
	{
		strTemp.push_back('&');
		strTemp.push_back('1' + (TCHAR)pos);
	}
	else if (pos == 9)
	{
		strTemp.append(TEXT("1&0"));
	}
	else
	{
		strTemp.append(uintToString(pos + 1));
	}
	strTemp.append(TEXT(": "));
	
	if (filenameLen > 0)
	{
		std::vector<TCHAR> vt(filenameLen + 1);
		//--FLS: W removed from PathCompactPathExW due to compiler errors for ANSI version.
		PathCompactPathEx(&vt[0], filename.c_str(), filenameLen + 1, 0);
		strTemp.append(convertFileName(vt.begin(), vt.begin() + lstrlen(&vt[0])));
	}
	else
	{
		// (filenameLen < 0)
		generic_string::const_iterator it = filename.begin();

		if (filenameLen == 0)
			it += PathFindFileName(filename.c_str()) - filename.c_str();

		// MAX_PATH is still here to keep old trimming behaviour.
		if (filename.end() - it < MAX_PATH)
		{
			strTemp.append(convertFileName(it, filename.end()));
		}
		else
		{
			strTemp.append(convertFileName(it, it + MAX_PATH / 2 - 3));
			strTemp.append(TEXT("..."));
			strTemp.append(convertFileName(filename.end() - MAX_PATH / 2, filename.end()));
		}
	}

	return strTemp;
}

generic_string PathRemoveFileSpec(generic_string & path)
{
    generic_string::size_type lastBackslash = path.find_last_of(TEXT('\\'));
    if (lastBackslash == generic_string::npos)
    {
        if (path.size() >= 2 && path[1] == TEXT(':'))  // "C:foo.bar" becomes "C:"
            path.erase(2);
        else
            path.erase();
    }
    else
    {
        if (lastBackslash == 2 && path[1] == TEXT(':') && path.size() >= 3)  // "C:\foo.exe" becomes "C:\"
            path.erase(3);
        else if (lastBackslash == 0 && path.size() > 1)  //   "\foo.exe" becomes "\"
            path.erase(1);
        else
            path.erase(lastBackslash);
    }
	return path;
}

generic_string PathAppend(generic_string &strDest, const generic_string & str2append)
{
	if (strDest == TEXT("") && str2append == TEXT("")) // "" + ""
	{
		strDest = TEXT("\\");
		return strDest;
	}

	if (strDest == TEXT("") && str2append != TEXT("")) // "" + titi
	{
		strDest = str2append;
		return strDest;
	}

	if (strDest[strDest.length() - 1] == '\\' && (str2append != TEXT("") && str2append[0] == '\\')) // toto\ + \titi
	{
		strDest.erase(strDest.length() - 1, 1);
		strDest += str2append;
		return strDest;
	}

	if ((strDest[strDest.length() - 1] == '\\' && (str2append != TEXT("") && str2append[0] != '\\')) // toto\ + titi
		|| (strDest[strDest.length() - 1] != '\\' && (str2append != TEXT("") && str2append[0] == '\\'))) // toto + \titi
	{
		strDest += str2append;
		return strDest;
	}

	// toto + titi
	strDest += TEXT("\\");
	strDest += str2append;

	return strDest;
}

COLORREF getCtrlBgColor(HWND hWnd)
{
	COLORREF crRet = CLR_INVALID;
	if (hWnd && IsWindow(hWnd))
	{
		RECT rc;
		if (GetClientRect(hWnd, &rc))
		{
			HDC hDC = GetDC(hWnd);
			if (hDC)
			{
				HDC hdcMem = CreateCompatibleDC(hDC);
				if (hdcMem)
				{
					HBITMAP hBmp = CreateCompatibleBitmap(hDC,
					rc.right, rc.bottom);
					if (hBmp)
					{
						HGDIOBJ hOld = SelectObject(hdcMem, hBmp);
						if (hOld)
						{
							if (SendMessage(hWnd,	WM_ERASEBKGND, (WPARAM)hdcMem, 0))
							{
								crRet = GetPixel(hdcMem, 2, 2); // 0, 0 is usually on the border
							}
							SelectObject(hdcMem, hOld);
						}
						DeleteObject(hBmp);
					}
					DeleteDC(hdcMem);
				}
				ReleaseDC(hWnd, hDC);
			}
		}
	}
	return crRet;
}

generic_string stringToUpper(generic_string strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);
    return strToConvert;
}

generic_string stringReplace(generic_string subject, const generic_string& search, const generic_string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

std::vector<generic_string> stringSplit(const generic_string& input, const generic_string& delimiter)
{
	auto start = 0U;
	auto end = input.find(delimiter);
	std::vector<generic_string> output;
	const size_t delimiterLength = delimiter.length();
	while (end != std::string::npos)
	{
		output.push_back(input.substr(start, end - start));
		start = end + delimiterLength;
		end = input.find(delimiter, start);
	}
	output.push_back(input.substr(start, end));
	return output;
}

generic_string stringJoin(const std::vector<generic_string>& strings, const generic_string& separator)
{
	generic_string joined;
	size_t length = strings.size();
	for (size_t i = 0; i < length; ++i)
	{
		joined += strings.at(i);
		if (i != length - 1)
		{
			joined += separator;
		}
	}
	return joined;
}

generic_string stringTakeWhileAdmissable(const generic_string& input, const generic_string& admissable)
{
	// Find first non-admissable character in "input", and remove everything after it.
	size_t idx = input.find_first_not_of(admissable);
	if (idx == std::string::npos)
	{
		return input;
	}
	else
	{
		return input.substr(0, idx);
	}
}

double stodLocale(const generic_string& str, _locale_t loc, size_t* idx)
{
	// Copied from the std::stod implementation but uses _wcstod_l instead of wcstod.
	const wchar_t* ptr = str.c_str();
	errno = 0;
	wchar_t* eptr;
	double ans = ::_wcstod_l(ptr, &eptr, loc);
	if (ptr == eptr)
		throw new std::invalid_argument("invalid stod argument");
	if (errno == ERANGE)
		throw new std::out_of_range("stod argument out of range");
	if (idx != NULL)
		*idx = (size_t)(eptr - ptr);
	return ans;
}

bool str2Clipboard(const generic_string &str2cpy, HWND hwnd)
{
	int len2Allocate = (str2cpy.size() + 1) * sizeof(TCHAR);
	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, len2Allocate);
	if (hglbCopy == NULL)
	{
		return false;
	}
	if (!::OpenClipboard(hwnd))
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}
	if (!::EmptyClipboard())
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}
	// Lock the handle and copy the text to the buffer.
	TCHAR *pStr = (TCHAR *)::GlobalLock(hglbCopy);
	if (pStr == NULL)
	{
		::GlobalUnlock(hglbCopy);
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}
	_tcscpy_s(pStr, len2Allocate / sizeof(TCHAR), str2cpy.c_str());
	::GlobalUnlock(hglbCopy);
	// Place the handle on the clipboard.
	unsigned int clipBoardFormat = CF_UNICODETEXT;
	if (::SetClipboardData(clipBoardFormat, hglbCopy) == NULL)
	{
		::GlobalUnlock(hglbCopy);
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}
	if (!::CloseClipboard())
	{
		return false;
	}
	return true;
}