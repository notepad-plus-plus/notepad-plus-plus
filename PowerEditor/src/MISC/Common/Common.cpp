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
#include <algorithm>
#include <stdexcept>
#include <shlwapi.h>
#include <uxtheme.h>
#include <cassert>
#include <codecvt>
#include <locale>
#include "StaticDialog.h"
#include "CustomFileDialog.h"

#include "FileInterface.h"
#include "Common.h"
#include "Utf8.h"
#include <Parameters.h>
#include "Buffer.h"

void printInt(int int2print)
{
	TCHAR str[32];
	wsprintf(str, TEXT("%d"), int2print);
	::MessageBox(NULL, str, TEXT(""), MB_OK);
}


void printStr(const TCHAR *str2print)
{
	::MessageBox(NULL, str2print, TEXT(""), MB_OK);
}

generic_string commafyInt(size_t n)
{
	generic_stringstream ss;
	ss.imbue(std::locale(""));
	ss << n;
	return ss.str();
}

std::string getFileContent(const TCHAR *file2read)
{
	if (!::PathFileExists(file2read))
		return "";

	const size_t blockSize = 1024;
	char data[blockSize];
	std::string wholeFileContent = "";
	FILE *fp = generic_fopen(file2read, TEXT("rb"));

	size_t lenFile = 0;
	do
	{
		lenFile = fread(data, 1, blockSize, fp);
		if (lenFile <= 0) break;
		wholeFileContent.append(data, lenFile);
	}
	while (lenFile > 0);

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
	generic_string fullFilePathName;
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
	Win32_IO_File file(file2write);

	if (file.isOpened())
		file.writeStr(content2write);
}


void writeLog(const TCHAR *logFileName, const char *log2write)
{
	const DWORD accessParam{ GENERIC_READ | GENERIC_WRITE };
	const DWORD shareParam{ FILE_SHARE_READ | FILE_SHARE_WRITE };
	const DWORD dispParam{ OPEN_ALWAYS }; // Open existing file for writing without destroying it or create new
	const DWORD attribParam{ FILE_ATTRIBUTE_NORMAL };
	HANDLE hFile = ::CreateFileW(logFileName, accessParam, shareParam, NULL, dispParam, attribParam, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER offset;
		offset.QuadPart = 0;
		::SetFilePointerEx(hFile, offset, NULL, FILE_END);

		SYSTEMTIME currentTime = {};
		::GetLocalTime(&currentTime);
		generic_string dateTimeStrW = getDateTimeStrFrom(TEXT("yyyy-MM-dd HH:mm:ss"), currentTime);
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string log2writeStr = converter.to_bytes(dateTimeStrW);
		log2writeStr += "  ";
		log2writeStr += log2write;
		log2writeStr += "\n";

		DWORD bytes_written = 0;
		::WriteFile(hFile, log2writeStr.c_str(), static_cast<DWORD>(log2writeStr.length()), &bytes_written, NULL);

		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}
}


generic_string folderBrowser(HWND parent, const generic_string & title, int outputCtrlID, const TCHAR *defaultStr)
{
	generic_string folderName;
	CustomFileDialog dlg(parent);
	dlg.setTitle(title.c_str());

	// Get an initial directory from the edit control or from argument provided
	TCHAR directory[MAX_PATH] = {};
	if (outputCtrlID != 0)
		::GetDlgItemText(parent, outputCtrlID, directory, _countof(directory));
	directory[_countof(directory) - 1] = '\0';
	if (!directory[0] && defaultStr)
		dlg.setFolder(defaultStr);
	else if (directory[0])
		dlg.setFolder(directory);

	folderName = dlg.pickFolder();
	if (!folderName.empty())
	{
		// Send the result back to the edit control
		if (outputCtrlID != 0)
			::SetDlgItemText(parent, outputCtrlID, folderName.c_str());
	}
	return folderName;
}


generic_string getFolderName(HWND parent, const TCHAR *defaultDir)
{
	return folderBrowser(parent, TEXT("Select a folder"), 0, defaultDir);
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
}


std::vector<generic_string> tokenizeString(const generic_string & tokenString, const char delim)
{
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
}


int filter(unsigned int code, struct _EXCEPTION_POINTERS *)
{
    if (code == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


bool isInList(const TCHAR *token, const TCHAR *list)
{
	if ((!token) || (!list))
		return false;

	const size_t wordLen = 64;
	size_t listLen = lstrlen(list);

	TCHAR word[wordLen];
	size_t i = 0;
	size_t j = 0;

	for (; i <= listLen; ++i)
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

			if (j >= wordLen)
				return false;
		}
	}
	return false;
}


generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand)
{
	const size_t cleanedNameLen = 64;
	TCHAR cleanedName[cleanedNameLen] = TEXT("");
	size_t j = 0;
	size_t menuNameLen = lstrlen(menuItemStr);
	if (menuNameLen >= cleanedNameLen)
		menuNameLen = cleanedNameLen - 1;

	for (size_t k = 0 ; k < menuNameLen ; ++k)
	{
		if (menuItemStr[k] == '\t')
		{
			cleanedName[k] = 0;
			break;
		}
		else
		{
			if (menuItemStr[k] == '&')
			{
				if (keepAmpersand)
					cleanedName[j++] = menuItemStr[k];
				//else skip
			}
			else
				cleanedName[j++] = menuItemStr[k];
		}
	}

	cleanedName[j] = 0;
	return cleanedName;
}


const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, size_t codepage, int lenMbcs, int* pLenWc, int* pBytesNotProcessed)
{
	// Do not process NULL pointer
	if (!mbcs2Convert)
		return nullptr;

	// Do not process empty strings
	if (lenMbcs == 0 || lenMbcs == -1 && mbcs2Convert[0] == 0)
	{
		_wideCharStr.empty();
		return _wideCharStr;
	}

	UINT cp = static_cast<UINT>(codepage);
	int bytesNotProcessed = 0;
	int lenWc = 0;

	// If length not specified, simply convert without checking
	if (lenMbcs == -1)
	{
		lenWc = MultiByteToWideChar(cp, 0, mbcs2Convert, lenMbcs, NULL, 0);
	}
	// Otherwise, test if we are cutting a multi-byte character at end of buffer
	else if (lenMbcs != -1 && cp == CP_UTF8) // For UTF-8, we know how to test it
	{
		int indexOfLastChar = Utf8::characterStart(mbcs2Convert, lenMbcs-1); // get index of last character
		if (indexOfLastChar != 0 && !Utf8::isValid(mbcs2Convert+indexOfLastChar, lenMbcs-indexOfLastChar)) // if it is not valid we do not process it right now (unless its the only character in string, to ensure that we always progress, e.g. that bytesNotProcessed < lenMbcs)
		{
			bytesNotProcessed = lenMbcs-indexOfLastChar;
		}
		lenWc = MultiByteToWideChar(cp, 0, mbcs2Convert, lenMbcs-bytesNotProcessed, NULL, 0);
	}
	else // For other encodings, ask system if there are any invalid characters; note that it will not correctly know if last character is cut when there are invalid characters inside the text
	{
		lenWc = MultiByteToWideChar(cp, (lenMbcs == -1) ? 0 : MB_ERR_INVALID_CHARS, mbcs2Convert, lenMbcs, NULL, 0);
		if (lenWc == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
		{
			// Test without last byte
			if (lenMbcs > 1) lenWc = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, mbcs2Convert, lenMbcs-1, NULL, 0);
			if (lenWc == 0) // don't have to check that the error is still ERROR_NO_UNICODE_TRANSLATION, since only the length parameter changed
			{
				// TODO: should warn user about incorrect loading due to invalid characters
				// We still load the file, but the system will either strip or replace invalid characters (including the last character, if cut in half)
				lenWc = MultiByteToWideChar(cp, 0, mbcs2Convert, lenMbcs, NULL, 0);
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
		MultiByteToWideChar(cp, 0, mbcs2Convert, lenMbcs-bytesNotProcessed, _wideCharStr, lenWc);
	}
	else
		_wideCharStr.empty();

	if (pLenWc)
		*pLenWc = lenWc;
	if (pBytesNotProcessed)
		*pBytesNotProcessed = bytesNotProcessed;

	return _wideCharStr;
}


// "mstart" and "mend" are pointers to indexes in mbcs2Convert,
// which are converted to the corresponding indexes in the returned wchar_t string.
const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, size_t codepage, intptr_t* mstart, intptr_t* mend)
{
	// Do not process NULL pointer
	if (!mbcs2Convert) return NULL;
	UINT cp = static_cast<UINT>(codepage);
	int len = MultiByteToWideChar(cp, 0, mbcs2Convert, -1, NULL, 0);
	if (len > 0)
	{
		_wideCharStr.sizeTo(len);
		len = MultiByteToWideChar(cp, 0, mbcs2Convert, -1, _wideCharStr, len);

		if ((size_t)*mstart < strlen(mbcs2Convert) && (size_t)*mend <= strlen(mbcs2Convert))
		{
			*mstart = MultiByteToWideChar(cp, 0, mbcs2Convert, static_cast<int>(*mstart), _wideCharStr, 0);
			*mend   = MultiByteToWideChar(cp, 0, mbcs2Convert, static_cast<int>(*mend), _wideCharStr, 0);
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


const char* WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, size_t codepage, int lenWc, int* pLenMbcs)
{
	if (nullptr == wcharStr2Convert)
		return nullptr;
	UINT cp = static_cast<UINT>(codepage);
	int lenMbcs = WideCharToMultiByte(cp, 0, wcharStr2Convert, lenWc, NULL, 0, NULL, NULL);
	if (lenMbcs > 0)
	{
		_multiByteStr.sizeTo(lenMbcs);
		WideCharToMultiByte(cp, 0, wcharStr2Convert, lenWc, _multiByteStr, lenMbcs, NULL, NULL);
	}
	else
		_multiByteStr.empty();

	if (pLenMbcs)
		*pLenMbcs = lenMbcs;
	return _multiByteStr;
}


const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, size_t codepage, intptr_t* mstart, intptr_t* mend)
{
	if (nullptr == wcharStr2Convert)
		return nullptr;
	UINT cp = static_cast<UINT>(codepage);
	int len = WideCharToMultiByte(cp, 0, wcharStr2Convert, -1, NULL, 0, NULL, NULL);
	if (len > 0)
	{
		_multiByteStr.sizeTo(len);
		len = WideCharToMultiByte(cp, 0, wcharStr2Convert, -1, _multiByteStr, len, NULL, NULL); // not needed?

        if (*mstart < lstrlenW(wcharStr2Convert) && *mend < lstrlenW(wcharStr2Convert))
        {
			*mstart = WideCharToMultiByte(cp, 0, wcharStr2Convert, (int)*mstart, NULL, 0, NULL, NULL);
			*mend = WideCharToMultiByte(cp, 0, wcharStr2Convert, (int)*mend, NULL, 0, NULL, NULL);
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
	if (len > 0)
	{
		std::vector<wchar_t> vw(len);
		MultiByteToWideChar(codepage, 0, rString.c_str(), -1, &vw[0], len);
		return &vw[0];
	}
	return std::wstring();
}


std::string wstring2string(const std::wstring & rwString, UINT codepage)
{
	int len = WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, NULL, 0, NULL, NULL);
	if (len > 0)
	{
		std::vector<char> vw(len);
		WideCharToMultiByte(codepage, 0, rwString.c_str(), -1, &vw[0], len, NULL, NULL);
		return &vw[0];
	}
	return std::string();
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

	vt.push_back('0' + static_cast<TCHAR>(std::abs(val % 10)));
	val /= 10;
	while (val != 0)
	{
		vt.push_back('0' + static_cast<TCHAR>(std::abs(val % 10)));
		val /= 10;
	}

	if (isNegative)
		vt.push_back('-');

	return generic_string(vt.rbegin(), vt.rend());
}

generic_string uintToString(unsigned int val)
{
	std::vector<TCHAR> vt;

	vt.push_back('0' + static_cast<TCHAR>(val % 10));
	val /= 10;
	while (val != 0)
	{
		vt.push_back('0' + static_cast<TCHAR>(val % 10));
		val /= 10;
	}

	return generic_string(vt.rbegin(), vt.rend());
}

// Build Recent File menu entries from given
generic_string BuildMenuFileName(int filenameLen, unsigned int pos, const generic_string &filename, bool ordinalNumber)
{
	generic_string strTemp;

	if (ordinalNumber)
	{
		if (pos < 9)
		{
			strTemp.push_back('&');
			strTemp.push_back('1' + static_cast<TCHAR>(pos));
		}
		else if (pos == 9)
		{
			strTemp.append(TEXT("1&0"));
		}
		else
		{
			div_t splitDigits = div(pos + 1, 10);
			strTemp.append(uintToString(splitDigits.quot));
			strTemp.push_back('&');
			strTemp.append(uintToString(splitDigits.rem));
		}
		strTemp.append(TEXT(": "));
	}
	else
	{
		strTemp.push_back('&');
	}

	if (filenameLen > 0)
	{
		std::vector<TCHAR> vt(filenameLen + 1);
		// W removed from PathCompactPathExW due to compiler errors for ANSI version.
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


generic_string PathRemoveFileSpec(generic_string& path)
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
        else if (lastBackslash == 0 && path.size() > 1) // "\foo.exe" becomes "\"
            path.erase(1);
        else
            path.erase(lastBackslash);
    }
	return path;
}


generic_string pathAppend(generic_string& strDest, const generic_string& str2append)
{
	if (strDest.empty() && str2append.empty()) // "" + ""
	{
		strDest = TEXT("\\");
		return strDest;
	}

	if (strDest.empty() && !str2append.empty()) // "" + titi
	{
		strDest = str2append;
		return strDest;
	}

	if (strDest[strDest.length() - 1] == '\\' && (!str2append.empty() && str2append[0] == '\\')) // toto\ + \titi
	{
		strDest.erase(strDest.length() - 1, 1);
		strDest += str2append;
		return strDest;
	}

	if ((strDest[strDest.length() - 1] == '\\' && (!str2append.empty() && str2append[0] != '\\')) // toto\ + titi
		|| (strDest[strDest.length() - 1] != '\\' && (!str2append.empty() && str2append[0] == '\\'))) // toto + \titi
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
							if (SendMessage(hWnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdcMem), 0))
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
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), 
        [](TCHAR ch){ return static_cast<TCHAR>(_totupper(ch)); }
    );
    return strToConvert;
}

generic_string stringToLower(generic_string strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::towlower);
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
	size_t start = 0U;
	size_t end = input.find(delimiter);
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


bool str2numberVector(generic_string str2convert, std::vector<size_t>& numVect)
{
	numVect.clear();

	for (auto i : str2convert)
	{
		switch (i)
		{
		case ' ':
		case '0': case '1':	case '2': case '3':	case '4':
		case '5': case '6':	case '7': case '8':	case '9':
		{
			// correct. do nothing
		}
		break;

		default:
			return false;
		}
	}

	std::vector<generic_string> v = stringSplit(str2convert, TEXT(" "));
	for (auto i : v)
	{
		// Don't treat empty string and the number greater than 9999
		if (!i.empty() && i.length() < 5)
		{
			numVect.push_back(std::stoi(i));
		}
	}
	return true;
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
#ifdef __MINGW32__
	double ans = ::wcstod(ptr, &eptr);
#else
	double ans = ::_wcstod_l(ptr, &eptr, loc);
#endif
	if (ptr == eptr)
		throw std::invalid_argument("invalid stod argument");
	if (errno == ERANGE)
		throw std::out_of_range("stod argument out of range");
	if (idx != NULL)
		*idx = (size_t)(eptr - ptr);
	return ans;
}

// Source: https://blogs.msdn.microsoft.com/greggm/2005/09/21/comparing-file-names-in-native-code/
// Modified to use TCHAR's instead of assuming Unicode and reformatted to conform with Notepad++ code style
static TCHAR ToUpperInvariant(TCHAR input)
{
	TCHAR result;
	LONG lres = LCMapString(LOCALE_INVARIANT, LCMAP_UPPERCASE, &input, 1, &result, 1);
	if (lres == 0)
	{
		assert(false and "LCMapString failed to convert a character to upper case");
		result = input;
	}
	return result;
}

// Source: https://blogs.msdn.microsoft.com/greggm/2005/09/21/comparing-file-names-in-native-code/
// Modified to use TCHAR's instead of assuming Unicode and reformatted to conform with Notepad++ code style
int OrdinalIgnoreCaseCompareStrings(LPCTSTR sz1, LPCTSTR sz2)
{
	if (sz1 == sz2)
	{
		return 0;
	}

	if (sz1 == nullptr) sz1 = _T("");
	if (sz2 == nullptr) sz2 = _T("");

	for (;; sz1++, sz2++)
	{
		const TCHAR c1 = *sz1;
		const TCHAR c2 = *sz2;

		// check for binary equality first
		if (c1 == c2)
		{
			if (c1 == 0)
			{
				return 0; // We have reached the end of both strings. No difference found.
			}
		}
		else
		{
			if (c1 == 0 || c2 == 0)
			{
				return (c1-c2); // We have reached the end of one string
			}

			// IMPORTANT: this needs to be upper case to match the behavior of the operating system.
			// See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndotnet/html/StringsinNET20.asp
			const TCHAR u1 = ToUpperInvariant(c1);
			const TCHAR u2 = ToUpperInvariant(c2);
			if (u1 != u2)
			{
				return (u1-u2); // strings are different
			}
		}
	}
}

bool str2Clipboard(const generic_string &str2cpy, HWND hwnd)
{
	size_t len2Allocate = (str2cpy.size() + 1) * sizeof(TCHAR);
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

bool buf2Clipborad(const std::vector<Buffer*>& buffers, bool isFullPath, HWND hwnd)
{
	const generic_string crlf = _T("\r\n");
	generic_string selection;
	for (auto&& buf : buffers)
	{
		if (buf)
		{
			const TCHAR* fileName = isFullPath ? buf->getFullPathName() : buf->getFileName();
			if (fileName)
				selection += fileName;
		}
		if (!selection.empty() && !endsWith(selection, crlf))
			selection += crlf;
	}
	if (!selection.empty())
		return str2Clipboard(selection, hwnd);
	return false;
}

bool matchInList(const TCHAR *fileName, const std::vector<generic_string> & patterns)
{
	bool is_matched = false;
	for (size_t i = 0, len = patterns.size(); i < len; ++i)
	{
		if (patterns[i].length() > 1 && patterns[i][0] == '!')
		{
			if (PathMatchSpec(fileName, patterns[i].c_str() + 1))
				return false;

			continue;
		} 

		if (PathMatchSpec(fileName, patterns[i].c_str()))
			is_matched = true;
	}
	return is_matched;
}

bool matchInExcludeDirList(const TCHAR* dirName, const std::vector<generic_string>& patterns, size_t level)
{
	for (size_t i = 0, len = patterns.size(); i < len; ++i)
	{
		size_t patterLen = patterns[i].length();

		if (patterLen > 3 && patterns[i][0] == '!' && patterns[i][1] == '+' && patterns[i][2] == '\\') // check for !+\folderPattern: for all levels - search this pattern recursively
		{
			if (PathMatchSpec(dirName, patterns[i].c_str() + 3))
				return true;
		}
		else if (patterLen > 2 && patterns[i][0] == '!' && patterns[i][1] == '\\') // check for !\folderPattern: exclusive pattern for only the 1st level
		{
			if (level == 1)
				if (PathMatchSpec(dirName, patterns[i].c_str() + 2))
					return true;
		}
	}
	return false;
}

bool allPatternsAreExclusion(const std::vector<generic_string> patterns)
{
	bool oneInclusionPatternFound = false;
	for (size_t i = 0, len = patterns.size(); i < len; ++i)
	{
		if (patterns[i][0] != '!')
		{
			oneInclusionPatternFound = true;
			break;
		}
	}
	return !oneInclusionPatternFound;
}

generic_string GetLastErrorAsString(DWORD errorCode)
{
	generic_string errorMsg(_T(""));
	// Get the error message, if any.
	// If both error codes (passed error n GetLastError) are 0, then return empty
	if (errorCode == 0)
		errorCode = GetLastError();
	if (errorCode == 0)
		return errorMsg; //No error message has been recorded

	LPWSTR messageBuffer = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	errorMsg += messageBuffer;

	//Free the buffer.
	LocalFree(messageBuffer);

	return errorMsg;
}

HWND CreateToolTip(int toolID, HWND hDlg, HINSTANCE hInst, const PTSTR pszText, bool isRTL)
{
	if (!toolID || !hDlg || !pszText)
	{
		return NULL;
	}

	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);
	if (!hwndTool)
	{
		return NULL;
	}

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(isRTL ? WS_EX_LAYOUTRTL : 0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
		hInst, NULL);

	if (!hwndTip)
	{
		return NULL;
	}

	NppDarkMode::setDarkTooltips(hwndTip, NppDarkMode::ToolTipsType::tooltip);

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	if (!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}

	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 200);
	// Make tip stay 15 seconds
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((15000), (0)));

	return hwndTip;
}

HWND CreateToolTipRect(int toolID, HWND hWnd, HINSTANCE hInst, const PTSTR pszText, const RECT rc)
{
	if (!toolID || !hWnd || !pszText)
	{
		return NULL;
	}

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, NULL,
		hInst, NULL);

	if (!hwndTip)
	{
		return NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hWnd;
	toolInfo.uFlags = TTF_SUBCLASS;
	toolInfo.uId = toolID;
	toolInfo.lpszText = pszText;
	toolInfo.rect = rc;
	if (!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}

	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, 200);
	// Make tip stay 15 seconds
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM((15000), (0)));

	return hwndTip;
}

bool isCertificateValidated(const generic_string & fullFilePath, const generic_string & subjectName2check)
{
	bool isOK = false;
	HCERTSTORE hStore = NULL;
	HCRYPTMSG hMsg = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	BOOL result = FALSE;
	DWORD dwEncoding = 0;
	DWORD dwContentType = 0;
	DWORD dwFormatType = 0;
	PCMSG_SIGNER_INFO pSignerInfo = NULL;
	DWORD dwSignerInfo = 0;
	CERT_INFO CertInfo;
	LPTSTR szName = NULL;

	generic_string subjectName;

	try {
		// Get message handle and store handle from the signed file.
		result = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
			fullFilePath.c_str(),
			CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
			CERT_QUERY_FORMAT_FLAG_BINARY,
			0,
			&dwEncoding,
			&dwContentType,
			&dwFormatType,
			&hStore,
			&hMsg,
			NULL);

		if (!result)
		{
			generic_string errorMessage = TEXT("Check certificate of ") + fullFilePath + TEXT(" : ");
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Get signer information size.
		result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
		if (!result)
		{
			generic_string errorMessage = TEXT("CryptMsgGetParam first call: ");
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Allocate memory for signer information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (!pSignerInfo)
		{
			generic_string errorMessage = TEXT("CryptMsgGetParam memory allocation problem: ");
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Get Signer Information.
		result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo);
		if (!result)
		{
			generic_string errorMessage = TEXT("CryptMsgGetParam: ");
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Search for the signer certificate in the temporary 
		// certificate store.
		CertInfo.Issuer = pSignerInfo->Issuer;
		CertInfo.SerialNumber = pSignerInfo->SerialNumber;

		pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo, NULL);
		if (!pCertContext)
		{
			generic_string errorMessage = TEXT("Certificate context: ");
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		DWORD dwData;

		// Get Subject name size.
		dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
		if (dwData <= 1)
		{
			throw generic_string(TEXT("Certificate checking error: getting data size problem."));
		}

		// Allocate memory for subject name.
		szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
		if (!szName)
		{
			throw generic_string(TEXT("Certificate checking error: memory allocation problem."));
		}

		// Get subject name.
		if (CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szName, dwData) <= 1)
		{
			throw generic_string(TEXT("Cannot get certificate info."));
		}

		// check Subject name.
		subjectName = szName;
		if (subjectName != subjectName2check)
		{
			throw generic_string(TEXT("Certificate checking error: the certificate is not matched."));
		}

		isOK = true;
	}
	catch (const generic_string& s)
	{
		// display error message
		MessageBox(NULL, s.c_str(), TEXT("Certificate checking"), MB_OK);
	}
	catch (...)
	{
		// Unknown error
		generic_string errorMessage = TEXT("Unknown exception occured. ");
		errorMessage += GetLastErrorAsString(GetLastError());
		MessageBox(NULL, errorMessage.c_str(), TEXT("Certificate checking"), MB_OK);
	}

	// Clean up.
	if (pSignerInfo != NULL) LocalFree(pSignerInfo);
	if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
	if (hStore != NULL) CertCloseStore(hStore, 0);
	if (hMsg != NULL) CryptMsgClose(hMsg);
	if (szName != NULL) LocalFree(szName);

	return isOK;
}

bool isAssoCommandExisting(LPCTSTR FullPathName)
{
	bool isAssoCommandExisting = false;

	bool isFileExisting = PathFileExists(FullPathName) != FALSE;

	if (isFileExisting)
	{
		PTSTR ext = PathFindExtension(FullPathName);

		HRESULT hres;
		wchar_t buffer[MAX_PATH] = TEXT("");
		DWORD bufferLen = MAX_PATH;

		// check if association exist
		hres = AssocQueryString(ASSOCF_VERIFY|ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_COMMAND, ext, NULL, buffer, &bufferLen);
        
        isAssoCommandExisting = (hres == S_OK)                  // check if association exist and no error
			&& (buffer != NULL)                                 // check if buffer is not NULL
			&& (wcsstr(buffer, TEXT("notepad++.exe")) == NULL); // check association with notepad++
        
	}
	return isAssoCommandExisting;
}

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX("Error in N++ string conversion s2ws!", L"Error in N++ string conversion s2ws!");

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX("Error in N++ string conversion ws2s!", L"Error in N++ string conversion ws2s!");

	return converterX.to_bytes(wstr);
}

bool deleteFileOrFolder(const generic_string& f2delete)
{
	auto len = f2delete.length();
	TCHAR* actionFolder = new TCHAR[len + 2];
	wcscpy_s(actionFolder, len + 2, f2delete.c_str());
	actionFolder[len] = 0;
	actionFolder[len + 1] = 0;

	SHFILEOPSTRUCT fileOpStruct = {};
	fileOpStruct.hwnd = NULL;
	fileOpStruct.pFrom = actionFolder;
	fileOpStruct.pTo = NULL;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_ALLOWUNDO;
	fileOpStruct.fAnyOperationsAborted = false;
	fileOpStruct.hNameMappings = NULL;
	fileOpStruct.lpszProgressTitle = NULL;

	int res = SHFileOperation(&fileOpStruct);

	delete[] actionFolder;
	return (res == 0);
}

// Get a vector of full file paths in a given folder. File extension type filter should be *.*, *.xml, *.dll... according the type of file you want to get.  
void getFilesInFolder(std::vector<generic_string>& files, const generic_string& extTypeFilter, const generic_string& inFolder)
{
	generic_string filter = inFolder;
	pathAppend(filter, extTypeFilter);

	WIN32_FIND_DATA foundData;
	HANDLE hFindFile = ::FindFirstFile(filter.c_str(), &foundData);

	if (hFindFile != INVALID_HANDLE_VALUE && !(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		generic_string foundFullPath = inFolder;
		pathAppend(foundFullPath, foundData.cFileName);
		files.push_back(foundFullPath);

		while (::FindNextFile(hFindFile, &foundData))
		{
			generic_string foundFullPath2 = inFolder;
			pathAppend(foundFullPath2, foundData.cFileName);
			files.push_back(foundFullPath2);
		}
	}
	::FindClose(hFindFile);
}

void trim(generic_string& str)
{
	// remove any leading or trailing spaces from str

	generic_string::size_type pos = str.find_last_not_of(' ');

	if (pos != generic_string::npos)
	{
		str.erase(pos + 1);
		pos = str.find_first_not_of(' ');
		if (pos != generic_string::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
}

bool endsWith(const generic_string& s, const generic_string& suffix)
{
#if defined(_MSVC_LANG) && (_MSVC_LANG > 201402L)
#error Replace this function with basic_string::ends_with
#endif
	size_t pos = s.find(suffix);
	return pos != s.npos && ((s.length() - pos) == suffix.length());
}

int nbDigitsFromNbLines(size_t nbLines)
{
	int nbDigits = 0; // minimum number of digit should be 4
	if (nbLines < 10) nbDigits = 1;
	else if (nbLines < 100) nbDigits = 2;
	else if (nbLines < 1000) nbDigits = 3;
	else if (nbLines < 10000) nbDigits = 4;
	else if (nbLines < 100000) nbDigits = 5;
	else if (nbLines < 1000000) nbDigits = 6;
	else // rare case
	{
		nbDigits = 7;
		nbLines /= 1000000;

		while (nbLines)
		{
			nbLines /= 10;
			++nbDigits;
		}
	}
	return nbDigits;
}

namespace
{
	constexpr TCHAR timeFmtEscapeChar = 0x1;
	constexpr TCHAR middayFormat[] = _T("tt");

	// Returns AM/PM string defined by the system locale for the specified time.
	// This string may be empty or customized.
	generic_string getMiddayString(const TCHAR* localeName, const SYSTEMTIME& st)
	{
		generic_string midday;
		midday.resize(MAX_PATH);
		int ret = GetTimeFormatEx(localeName, 0, &st, middayFormat, &midday[0], static_cast<int>(midday.size()));
		if (ret > 0)
			midday.resize(ret - 1); // Remove the null-terminator.
		else
			midday.clear();
		return midday;
	}

	// Replaces conflicting time format specifiers by a special character.
	bool escapeTimeFormat(generic_string& format)
	{
		bool modified = false;
		for (auto& ch : format)
		{
			if (ch == middayFormat[0])
			{
				ch = timeFmtEscapeChar;
				modified = true;
			}
		}
		return modified;
	}

	// Replaces special time format characters by actual AM/PM string.
	void unescapeTimeFormat(generic_string& format, const generic_string& midday)
	{
		if (midday.empty())
		{
			auto it = std::remove(format.begin(), format.end(), timeFmtEscapeChar);
			if (it != format.end())
				format.erase(it, format.end());
		}
		else
		{
			size_t i = 0;
			while ((i = format.find(timeFmtEscapeChar, i)) != generic_string::npos)
			{
				if (i + 1 < format.size() && format[i + 1] == timeFmtEscapeChar)
				{
					// 'tt' => AM/PM
					format.erase(i, std::size(middayFormat) - 1);
					format.insert(i, midday);
				}
				else
				{
					// 't' => A/P
					format[i] = midday[0];
				}
			}
		}
	}
}

generic_string getDateTimeStrFrom(const generic_string& dateTimeFormat, const SYSTEMTIME& st)
{
	const TCHAR* localeName = LOCALE_NAME_USER_DEFAULT;
	const DWORD flags = 0;

	constexpr int bufferSize = MAX_PATH;
	TCHAR buffer[bufferSize] = {};
	int ret = 0;


	// 1. Escape 'tt' that means AM/PM or 't' that means A/P.
	// This is needed to avoid conflict with 'M' date format that stands for month.
	generic_string newFormat = dateTimeFormat;
	const bool hasMiddayFormat = escapeTimeFormat(newFormat);

	// 2. Format the time (h/m/s/t/H).
	ret = GetTimeFormatEx(localeName, flags, &st, newFormat.c_str(), buffer, bufferSize);
	if (ret != 0)
	{
		// 3. Format the date (d/y/g/M). 
		// Now use the buffer as a format string to process the format specifiers not recognized by GetTimeFormatEx().
		ret = GetDateFormatEx(localeName, flags, &st, buffer, buffer, bufferSize, nullptr);
	}

	if (ret != 0)
	{
		if (hasMiddayFormat)
		{
			// 4. Now format only the AM/PM string.
			const generic_string midday = getMiddayString(localeName, st);
			generic_string result = buffer;
			unescapeTimeFormat(result, midday);
			return result;
		}
		return buffer;
	}

	return {};
}

// Don't forget to use DeleteObject(createdFont) before leaving the program
HFONT createFont(const TCHAR* fontName, int fontSize, bool isBold, HWND hDestParent)
{
	HDC hdc = GetDC(hDestParent);

	LOGFONT logFont = {};
	logFont.lfHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	if (isBold)
		logFont.lfWeight = FW_BOLD;

	_tcscpy_s(logFont.lfFaceName, fontName);

	HFONT newFont = CreateFontIndirect(&logFont);

	ReleaseDC(hDestParent, hdc);

	return newFont;
}

Version::Version(const generic_string& versionStr)
{
	try {
		auto ss = tokenizeString(versionStr, '.');

		if (ss.size() > 4)
		{
			std::wstring msg(L"\"");
			msg += versionStr;
			msg += L"\"";
			msg += TEXT(": Version parts are more than 4. The string to parse is not a valid version format. Let's make it default value in catch block.");
			throw msg;
		}

		int i = 0;
		std::vector<unsigned long*> v = { &_major, &_minor, &_patch, &_build };
		for (const auto& s : ss)
		{
			if (!isNumber(s))
			{
				std::wstring msg(L"\"");
				msg += versionStr;
				msg += L"\"";
				msg += TEXT(": One of version character is not number. The string to parse is not a valid version format. Let's make it default value in catch block.");
				throw msg;
			}
			*(v[i]) = std::stoi(s);

			++i;
		}
	}
#ifdef DEBUG
	catch (const std::wstring& s)
	{
		_major = 0;
		_minor = 0;
		_patch = 0;
		_build = 0;

		throw s;
	}
#endif
	catch (...)
	{
		_major = 0;
		_minor = 0;
		_patch = 0;
		_build = 0;
#ifdef DEBUG
		throw std::wstring(TEXT("Unknown exception from \"Version::Version(const generic_string& versionStr)\""));
#endif
	}
}


void Version::setVersionFrom(const generic_string& filePath)
{
	if (!filePath.empty() && ::PathFileExists(filePath.c_str()))
	{
		DWORD uselessArg = 0; // this variable is for passing the ignored argument to the functions
		DWORD bufferSize = ::GetFileVersionInfoSize(filePath.c_str(), &uselessArg);

		if (bufferSize <= 0)
			return;

		unsigned char* buffer = new unsigned char[bufferSize];
		::GetFileVersionInfo(filePath.c_str(), uselessArg, bufferSize, buffer);

		VS_FIXEDFILEINFO* lpFileInfo = nullptr;
		UINT cbFileInfo = 0;
		VerQueryValue(buffer, TEXT("\\"), reinterpret_cast<LPVOID*>(&lpFileInfo), &cbFileInfo);
		if (cbFileInfo)
		{
			_major = (lpFileInfo->dwFileVersionMS & 0xFFFF0000) >> 16;
			_minor = lpFileInfo->dwFileVersionMS & 0x0000FFFF;
			_patch = (lpFileInfo->dwFileVersionLS & 0xFFFF0000) >> 16;
			_build = lpFileInfo->dwFileVersionLS & 0x0000FFFF;
		}
		delete[] buffer;
	}
}

generic_string Version::toString()
{
	if (_build == 0 && _patch == 0 && _minor == 0 && _major == 0) // ""
	{
		return TEXT("");
	}
	else if (_build == 0 && _patch == 0 && _minor == 0) // "major"
	{
		return std::to_wstring(_major);
	}
	else if (_build == 0 && _patch == 0) // "major.minor"
	{
		std::wstring v = std::to_wstring(_major);
		v += TEXT(".");
		v += std::to_wstring(_minor);
		return v;
	}
	else if (_build == 0) // "major.minor.patch"
	{
		std::wstring v = std::to_wstring(_major);
		v += TEXT(".");
		v += std::to_wstring(_minor);
		v += TEXT(".");
		v += std::to_wstring(_patch);
		return v;
	}

	// "major.minor.patch.build"
	std::wstring ver = std::to_wstring(_major);
	ver += TEXT(".");
	ver += std::to_wstring(_minor);
	ver += TEXT(".");
	ver += std::to_wstring(_patch);
	ver += TEXT(".");
	ver += std::to_wstring(_build);

	return ver;
}

int Version::compareTo(const Version& v2c) const
{
	if (_major > v2c._major)
		return 1;
	else if (_major < v2c._major)
		return -1;
	else // (_major == v2c._major)
	{
		if (_minor > v2c._minor)
			return 1;
		else if (_minor < v2c._minor)
			return -1;
		else // (_minor == v2c._minor)
		{
			if (_patch > v2c._patch)
				return 1;
			else if (_patch < v2c._patch)
				return -1;
			else // (_patch == v2c._patch)
			{
				if (_build > v2c._build)
					return 1;
				else if (_build < v2c._build)
					return -1;
				else // (_build == v2c._build)
				{
					return 0;
				}
			}
		}
	}
}

bool Version::isCompatibleTo(const Version& from, const Version& to) const
{
	// This method determinates if Version object is in between "from" version and "to" version, it's useful for testing compatibility of application.
	// test in versions <from, to> example: 
	// 1. <0.0.0.0, 0.0.0.0>: both from to versions are empty, so it's 
	// 2. <6.9, 6.9>: plugin is compatible to only v6.9
	// 3. <4.2, 6.6.6>: from v4.2 (included) to v6.6.6 (included)
	// 4. <0.0.0.0, 8.2.1>: all version until v8.2.1 (included)
	// 5. <8.3, 0.0.0.0>: from v8.3 (included) to the latest verrsion
	
	if (empty()) // if this version is empty, then no compatible to all version
		return false;

	if (from.empty() && to.empty()) // both versions "from" and "to" are empty: it's considered compatible, whatever this version is (match to 1)
	{
		return true;
	}

	if (from <= *this && to >= *this) // from_ver <= this_ver <= to_ver (match to 2, 3 and 4)
	{
		return true;
	}
		
	if (from <= *this && to.empty()) // from_ver <= this_ver (match to 5)
	{
		return true;
	}

	return false;
}
