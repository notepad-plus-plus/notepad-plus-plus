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
#include <windows.h>
#include <algorithm>
#include <stdexcept>
#include <shlwapi.h>
#include <uxtheme.h>
#include <cassert>
#include <locale>
#include "StaticDialog.h"
#include "CustomFileDialog.h"
#include "FileInterface.h"
#include "Common.h"
#include "Utf8.h"
#include "Parameters.h"
#include "Buffer.h"

using namespace std;

void printInt(int int2print)
{
	wchar_t str[32];
	wsprintf(str, L"%d", int2print);
	::MessageBox(NULL, str, L"", MB_OK);
}


void printStr(const wchar_t *str2print)
{
	::MessageBox(NULL, str2print, L"", MB_OK);
}

wstring commafyInt(size_t n)
{
	std::basic_stringstream<wchar_t> ss;
	ss.imbue(std::locale(""));
	ss << n;
	return ss.str();
}

std::string getFileContent(const wchar_t *file2read)
{
	if (!doesFileExist(file2read))
		return "";

	const size_t blockSize = 1024;
	char data[blockSize];
	std::string wholeFileContent = "";
	FILE *fp = _wfopen(file2read, L"rb");
	if (!fp)
		return "";

	size_t lenFile = 0;
	do
	{
		lenFile = fread(data, 1, blockSize, fp);
		if (lenFile == 0) break;
		wholeFileContent.append(data, lenFile);
	}
	while (lenFile > 0);

	fclose(fp);
	return wholeFileContent;
}

char getDriveLetter()
{
	char drive = '\0';
	wchar_t current[MAX_PATH];

	::GetCurrentDirectory(MAX_PATH, current);
	int driveNbr = ::PathGetDriveNumber(current);
	if (driveNbr != -1)
		drive = 'A' + char(driveNbr);

	return drive;
}


wstring relativeFilePathToFullFilePath(const wchar_t *relativeFilePath)
{
	wstring fullFilePathName;
	BOOL isRelative = ::PathIsRelative(relativeFilePath);

	if (isRelative)
	{
		wchar_t fullFileName[MAX_PATH];
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


void writeFileContent(const wchar_t *file2write, const char *content2write)
{
	Win32_IO_File file(file2write);

	if (file.isOpened())
		file.writeStr(content2write);
}


void writeLog(const wchar_t* logFileName, const char* log2write)
{
	const DWORD accessParam{ GENERIC_READ | GENERIC_WRITE };
	const DWORD shareParam{ FILE_SHARE_READ | FILE_SHARE_WRITE };
	const DWORD dispParam{ OPEN_ALWAYS }; // Open existing file for writing without destroying it or create new
	const DWORD attribParam{ FILE_ATTRIBUTE_NORMAL };
	HANDLE hFile = ::CreateFileW(logFileName, accessParam, shareParam, NULL, dispParam, attribParam, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER offset{};
		offset.QuadPart = 0;
		::SetFilePointerEx(hFile, offset, NULL, FILE_END);

		SYSTEMTIME currentTime = {};
		::GetLocalTime(&currentTime);
		wstring dateTimeStrW = getDateTimeStrFrom(L"yyyy-MM-dd HH:mm:ss", currentTime);
		string log2writeStr = wstring2string(dateTimeStrW, CP_UTF8);
		log2writeStr += "  ";
		log2writeStr += log2write;
		log2writeStr += "\n";

		DWORD bytes_written = 0;
		::WriteFile(hFile, log2writeStr.c_str(), static_cast<DWORD>(log2writeStr.length()), &bytes_written, NULL);

		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}
}

void writeLog(const wchar_t* logFileName, const wchar_t* log2write)
{
	string log2WriteA = wstring2string(log2write, CP_ACP);
	return writeLog(logFileName, log2WriteA.c_str());
}

wstring folderBrowser(HWND parent, const wstring & title, int outputCtrlID, const wchar_t *defaultStr)
{
	wstring folderName;
	CustomFileDialog dlg(parent);
	dlg.setTitle(title.c_str());

	// Get an initial directory from the edit control or from argument provided
	wchar_t directory[MAX_PATH] = {};
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


wstring getFolderName(HWND parent, const wchar_t *defaultDir)
{
	return folderBrowser(parent, L"Select a folder", 0, defaultDir);
}


void ClientRectToScreenRect(HWND hWnd, RECT* rect)
{
	POINT		pt{};

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


std::vector<wstring> tokenizeString(const wstring & tokenString, const char delim)
{
	//Vector is created on stack and copied on return
	std::vector<wstring> tokens;

    // Skip delimiters at beginning.
	wstring::size_type lastPos = tokenString.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    wstring::size_type pos     = tokenString.find_first_of(delim, lastPos);

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
	POINT		pt{};

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


bool isInList(const wchar_t *token, const wchar_t *list)
{
	if ((!token) || (!list))
		return false;

	const size_t wordLen = 64;
	size_t listLen = lstrlen(list);

	wchar_t word[wordLen] = { '\0' };
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

				if (!wcsicmp(token, word))
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


wstring purgeMenuItemString(const wchar_t * menuItemStr, bool keepAmpersand)
{
	const size_t cleanedNameLen = 64;
	wchar_t cleanedName[cleanedNameLen] = L"";
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
	if (lenMbcs == 0 || (lenMbcs == -1 && mbcs2Convert[0] == 0))
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
	if (!wcharStr2Convert)
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
	if (!wcharStr2Convert)
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
wstring convertFileName(T beg, T end)
{
	wstring strTmp;

	for (T it = beg; it != end; ++it)
	{
		if (*it == '&') strTmp.push_back('&');
		strTmp.push_back(*it);
	}

	return strTmp;
}


wstring intToString(int val)
{
	std::vector<wchar_t> vt;
	bool isNegative = val < 0;
	// can't use abs here because std::numeric_limits<int>::min() has no positive representation
	//val = std::abs(val);

	vt.push_back('0' + static_cast<wchar_t>(std::abs(val % 10)));
	val /= 10;
	while (val != 0)
	{
		vt.push_back('0' + static_cast<wchar_t>(std::abs(val % 10)));
		val /= 10;
	}

	if (isNegative)
		vt.push_back('-');

	return wstring(vt.rbegin(), vt.rend());
}

wstring uintToString(unsigned int val)
{
	std::vector<wchar_t> vt;

	vt.push_back('0' + static_cast<wchar_t>(val % 10));
	val /= 10;
	while (val != 0)
	{
		vt.push_back('0' + static_cast<wchar_t>(val % 10));
		val /= 10;
	}

	return wstring(vt.rbegin(), vt.rend());
}

// Build Recent File menu entries from given
wstring BuildMenuFileName(int filenameLen, unsigned int pos, const wstring &filename, bool ordinalNumber)
{
	wstring strTemp;

	if (ordinalNumber)
	{
		if (pos < 9)
		{
			strTemp.push_back('&');
			strTemp.push_back('1' + static_cast<wchar_t>(pos));
		}
		else if (pos == 9)
		{
			strTemp.append(L"1&0");
		}
		else
		{
			div_t splitDigits = div(pos + 1, 10);
			strTemp.append(uintToString(splitDigits.quot));
			strTemp.push_back('&');
			strTemp.append(uintToString(splitDigits.rem));
		}
		strTemp.append(L": ");
	}
	else
	{
		strTemp.push_back('&');
	}

	if (filenameLen > 0)
	{
		std::vector<wchar_t> vt(filenameLen + 1);
		// W removed from PathCompactPathExW due to compiler errors for ANSI version.
		PathCompactPathEx(&vt[0], filename.c_str(), filenameLen + 1, 0);
		strTemp.append(convertFileName(vt.begin(), vt.begin() + lstrlen(&vt[0])));
	}
	else
	{
		// (filenameLen < 0)
		wstring::const_iterator it = filename.begin();

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
			strTemp.append(L"...");
			strTemp.append(convertFileName(filename.end() - MAX_PATH / 2, filename.end()));
		}
	}

	return strTemp;
}


wstring pathRemoveFileSpec(wstring& path)
{
    wstring::size_type lastBackslash = path.find_last_of(L'\\');
    if (lastBackslash == wstring::npos)
    {
        if (path.size() >= 2 && path[1] == L':')  // "C:foo.bar" becomes "C:"
            path.erase(2);
        else
            path.erase();
    }
    else
    {
        if (lastBackslash == 2 && path[1] == L':' && path.size() >= 3)  // "C:\foo.exe" becomes "C:\"
            path.erase(3);
        else if (lastBackslash == 0 && path.size() > 1) // "\foo.exe" becomes "\"
            path.erase(1);
        else
            path.erase(lastBackslash);
    }
	return path;
}


wstring pathAppend(wstring& strDest, const wstring& str2append)
{
	if (strDest.empty() && str2append.empty()) // "" + ""
	{
		strDest = L"\\";
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
	strDest += L"\\";
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


wstring stringToUpper(wstring strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), 
        [](wchar_t ch){ return static_cast<wchar_t>(towupper(ch)); }
    );
    return strToConvert;
}

wstring stringToLower(wstring strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::towlower);
    return strToConvert;
}


wstring stringReplace(wstring subject, const wstring& search, const wstring& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}


void stringSplit(const wstring& input, const wstring& delimiter, std::vector<wstring>& output)
{
	size_t start = 0U;
	size_t end = input.find(delimiter);
	const size_t delimiterLength = delimiter.length();
	while (end != std::string::npos)
	{
		output.push_back(input.substr(start, end - start));
		start = end + delimiterLength;
		end = input.find(delimiter, start);
	}
	output.push_back(input.substr(start, end));
}


bool str2numberVector(wstring str2convert, std::vector<size_t>& numVect)
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

	std::vector<wstring> v;
	stringSplit(str2convert, L" ", v);
	for (const auto& i : v)
	{
		// Don't treat empty string and the number greater than 9999
		if (!i.empty() && i.length() < 5)
		{
			numVect.push_back(std::stoi(i));
		}
	}
	return true;
}

void stringJoin(const std::vector<wstring>& strings, const wstring& separator, wstring& joinedString)
{
	size_t length = strings.size();
	for (size_t i = 0; i < length; ++i)
	{
		joinedString += strings.at(i);
		if (i != length - 1)
		{
			joinedString += separator;
		}
	}
}


wstring stringTakeWhileAdmissable(const wstring& input, const wstring& admissable)
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


double stodLocale(const wstring& str, [[maybe_unused]] _locale_t loc, size_t* idx)
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


bool str2Clipboard(const wstring &str2cpy, HWND hwnd)
{
	size_t len2Allocate = (str2cpy.size() + 1) * sizeof(wchar_t);
	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, len2Allocate);
	if (hglbCopy == NULL)
	{
		return false;
	}

	if (!::OpenClipboard(hwnd))
	{
		::GlobalFree(hglbCopy);
		return false;
	}

	if (!::EmptyClipboard())
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	// Lock the handle and copy the text to the buffer.
	wchar_t *pStr = (wchar_t *)::GlobalLock(hglbCopy);
	if (!pStr)
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	wcscpy_s(pStr, len2Allocate / sizeof(wchar_t), str2cpy.c_str());
	::GlobalUnlock(hglbCopy);
	// Place the handle on the clipboard.
	unsigned int clipBoardFormat = CF_UNICODETEXT;
	if (!::SetClipboardData(clipBoardFormat, hglbCopy))
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

bool buf2Clipboard(const std::vector<Buffer*>& buffers, bool isFullPath, HWND hwnd)
{
	const wstring crlf = L"\r\n";
	wstring selection;
	for (auto&& buf : buffers)
	{
		if (buf)
		{
			const wchar_t* fileName = isFullPath ? buf->getFullPathName() : buf->getFileName();
			if (fileName)
				selection += fileName;
		}

		if (!selection.empty() && !selection.ends_with(crlf))
			selection += crlf;
	}

	if (!selection.empty())
		return str2Clipboard(selection, hwnd);
	return false;
}

bool matchInList(const wchar_t *fileName, const std::vector<wstring> & patterns)
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

bool matchInExcludeDirList(const wchar_t* dirName, const std::vector<wstring>& patterns, size_t level)
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

bool allPatternsAreExclusion(const std::vector<wstring>& patterns)
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

wstring GetLastErrorAsString(DWORD errorCode)
{
	wstring errorMsg(L"");
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

bool isCertificateValidated(const wstring & fullFilePath, const wstring & subjectName2check)
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
	CERT_INFO CertInfo{};
	LPTSTR szName = NULL;

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
			wstring errorMessage = L"Check certificate of " + fullFilePath + L" : ";
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Get signer information size.
		result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
		if (!result)
		{
			wstring errorMessage = L"CryptMsgGetParam first call: ";
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Allocate memory for signer information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (!pSignerInfo)
		{
			wstring errorMessage = L"CryptMsgGetParam memory allocation problem: ";
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		// Get Signer Information.
		result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo);
		if (!result)
		{
			wstring errorMessage = L"CryptMsgGetParam: ";
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
			wstring errorMessage = L"Certificate context: ";
			errorMessage += GetLastErrorAsString(GetLastError());
			throw errorMessage;
		}

		DWORD dwData;

		// Get Subject name size.
		dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
		if (dwData <= 1)
		{
			throw wstring(L"Certificate checking error: getting data size problem.");
		}

		// Allocate memory for subject name.
		szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(wchar_t));
		if (!szName)
		{
			throw wstring(L"Certificate checking error: memory allocation problem.");
		}

		// Get subject name.
		if (CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szName, dwData) <= 1)
		{
			throw wstring(L"Cannot get certificate info.");
		}

		// check Subject name.
		wstring subjectName = szName;
		if (subjectName != subjectName2check)
		{
			throw wstring(L"Certificate checking error: the certificate is not matched.");
		}

		isOK = true;
	}
	catch (const wstring& s)
	{
		// display error message
		MessageBox(NULL, s.c_str(), L"Certificate checking", MB_OK);
	}
	catch (...)
	{
		// Unknown error
		wstring errorMessage = L"Unknown exception occured. ";
		errorMessage += GetLastErrorAsString(GetLastError());
		MessageBox(NULL, errorMessage.c_str(), L"Certificate checking", MB_OK);
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
	bool isAssoCmdExist = false;

	bool isFileExisting = doesFileExist(FullPathName);

	if (isFileExisting)
	{
		PTSTR ext = PathFindExtension(FullPathName);

		HRESULT hres;
		wchar_t buffer[MAX_PATH] = L"";
		DWORD bufferLen = MAX_PATH;

		// check if association exist
		hres = AssocQueryString(ASSOCF_VERIFY|ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_COMMAND, ext, NULL, buffer, &bufferLen);

		isAssoCmdExist = (hres == S_OK)                  // check if association exist and no error
			&& (wcsstr(buffer, L"notepad++.exe")) == NULL;   // check association with notepad++

	}
	return isAssoCmdExist;
}

bool deleteFileOrFolder(const wstring& f2delete)
{
	auto len = f2delete.length();
	wchar_t* actionFolder = new wchar_t[len + 2];
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
void getFilesInFolder(std::vector<wstring>& files, const wstring& extTypeFilter, const wstring& inFolder)
{
	wstring filter = inFolder;
	pathAppend(filter, extTypeFilter);

	WIN32_FIND_DATA foundData;
	HANDLE hFindFile = ::FindFirstFile(filter.c_str(), &foundData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				wstring foundFullPath = inFolder;
				pathAppend(foundFullPath, foundData.cFileName);
				files.push_back(foundFullPath);
			}
		} while (::FindNextFile(hFindFile, &foundData));
		::FindClose(hFindFile);
	}
}

// remove any leading or trailing spaces from str
void trim(std::wstring& str)
{
	std::wstring::size_type pos = str.find_last_not_of(' ');

	if (pos != std::wstring::npos)
	{
		str.erase(pos + 1);
		pos = str.find_first_not_of(' ');
		if (pos != std::wstring::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
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
		nbLines /= 10000000;

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
	constexpr wchar_t timeFmtEscapeChar = 0x1;
	constexpr wchar_t middayFormat[] = L"tt";

	// Returns AM/PM string defined by the system locale for the specified time.
	// This string may be empty or customized.
	wstring getMiddayString(const wchar_t* localeName, const SYSTEMTIME& st)
	{
		wstring midday;
		midday.resize(MAX_PATH);
		int ret = GetTimeFormatEx(localeName, 0, &st, middayFormat, &midday[0], static_cast<int>(midday.size()));
		if (ret > 0)
			midday.resize(ret - 1); // Remove the null-terminator.
		else
			midday.clear();
		return midday;
	}

	// Replaces conflicting time format specifiers by a special character.
	bool escapeTimeFormat(wstring& format)
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
	void unescapeTimeFormat(wstring& format, const wstring& midday)
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
			while ((i = format.find(timeFmtEscapeChar, i)) != wstring::npos)
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

wstring getDateTimeStrFrom(const wstring& dateTimeFormat, const SYSTEMTIME& st)
{
	const wchar_t* localeName = LOCALE_NAME_USER_DEFAULT;
	const DWORD flags = 0;

	constexpr int bufferSize = MAX_PATH;
	wchar_t buffer[bufferSize] = {};
	int ret = 0;


	// 1. Escape 'tt' that means AM/PM or 't' that means A/P.
	// This is needed to avoid conflict with 'M' date format that stands for month.
	wstring newFormat = dateTimeFormat;
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
			const wstring midday = getMiddayString(localeName, st);
			wstring result = buffer;
			unescapeTimeFormat(result, midday);
			return result;
		}
		return buffer;
	}

	return {};
}

// Don't forget to use DeleteObject(createdFont) before leaving the program
HFONT createFont(const wchar_t* fontName, int fontSize, bool isBold, HWND hDestParent)
{
	HDC hdc = GetDC(hDestParent);

	LOGFONT logFont{};
	logFont.lfHeight = DPIManagerV2::scaleFont(fontSize, hDestParent);
	if (isBold)
		logFont.lfWeight = FW_BOLD;

	wcscpy_s(logFont.lfFaceName, fontName);

	HFONT newFont = CreateFontIndirect(&logFont);

	ReleaseDC(hDestParent, hdc);

	return newFont;
}

bool removeReadOnlyFlagFromFileAttributes(const wchar_t* fileFullPath)
{
	DWORD dwFileAttribs = ::GetFileAttributes(fileFullPath);

	if (dwFileAttribs == INVALID_FILE_ATTRIBUTES || (dwFileAttribs & FILE_ATTRIBUTE_DIRECTORY))
		return false;

	dwFileAttribs &= ~FILE_ATTRIBUTE_READONLY;
	return (::SetFileAttributes(fileFullPath, dwFileAttribs) != FALSE);
}

// "For file I/O, the "\\?\" prefix to a path string tells the Windows APIs to disable all string parsing
// and to send the string that follows it straight to the file system..."
// Ref: https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#win32-file-namespaces
bool isWin32NamespacePrefixedFileName(const wstring& fileName)
{
	// TODO:
	// ?! how to handle similar NT Object Manager path style prefix case \??\...
	// (the \??\ prefix instructs the NT Object Manager to search in the caller's local device directory for an alias...)

	// the following covers the \\?\... raw Win32-filenames or the \\?\UNC\... UNC equivalents
	// and also its *nix like forward slash equivalents
	return (fileName.starts_with(L"\\\\?\\") || fileName.starts_with(L"//?/"));
}

bool isWin32NamespacePrefixedFileName(const wchar_t* szFileName)
{
	const wstring fileName = szFileName;
	return isWin32NamespacePrefixedFileName(fileName);
}

bool isUnsupportedFileName(const wstring& fileName)
{
	bool isUnsupported = true;

	// until the Notepad++ (and its plugins) will not be prepared for filenames longer than the MAX_PATH,
	// we have to limit also the maximum supported length below
	if ((fileName.size() > 0) && (fileName.size() < MAX_PATH))
	{
		// possible raw filenames can contain space(s) or dot(s) at its end (e.g. "\\?\C:\file."), but the Notepad++ advanced
		// Open/SaveAs IFileOpenDialog/IFileSaveDialog COM-interface based dialogs currently do not handle this well
		// (but e.g. direct Notepad++ Ctrl+S works ok even with these filenames)
		// 
		// Exception for the standard filenames ending with the dot-char:
		// - when someone tries to open e.g. the 'C:\file.', we will accept that as this is the way how to work with filenames
		//   without an extension (some of the WINAPI calls used later trim that dot-char automatically ...)
		if (!(fileName.ends_with(L'.') && isWin32NamespacePrefixedFileName(fileName)) && !fileName.ends_with(L' '))
		{
			bool invalidASCIIChar = false;

			for (size_t pos = 0; pos < fileName.size(); ++pos)
			{
				wchar_t c = fileName.at(pos);
				if (c <= 31)
				{
					invalidASCIIChar = true;
				}
				else
				{
					// as this could be also a complete filename with path and there could be also a globbing used,
					// we tolerate here some other reserved Win32-filename chars: /, \, :, ?, *
					switch (c)
					{
						case '<':
						case '>':
						case '"':
						case '|':
							invalidASCIIChar = true;
							break;
					}
				}

				if (invalidASCIIChar)
					break;
			}

			if (!invalidASCIIChar)
			{
				// strip input string to a filename without a possible path and/or ending dot-char
				wstring fileNameOnly;
				if (fileName.ends_with(L'.'))
					fileNameOnly = fileName.substr(0, fileName.rfind(L"."));
				else
					fileNameOnly = fileName;

				size_t pos = fileNameOnly.find_last_of(L"\\");
				if (pos == std::string::npos)
					pos = fileNameOnly.find_last_of(L"/");
				if (pos != std::string::npos)
					fileNameOnly = fileNameOnly.substr(pos + 1);

				// upperize because the std::find is case sensitive unlike the Windows OS filesystem
				std::transform(fileNameOnly.begin(), fileNameOnly.end(), fileNameOnly.begin(), ::towupper);

				const std::vector<wstring>  reservedWin32NamespaceDeviceList {
				L"CON", L"PRN", L"AUX", L"NUL",
				L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
				L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
				};

				// last check is for all the old reserved Windows OS filenames
				if (std::find(reservedWin32NamespaceDeviceList.begin(), reservedWin32NamespaceDeviceList.end(), fileNameOnly) == reservedWin32NamespaceDeviceList.end())
				{
					// ok, the current filename tested is not even on the blacklist
					isUnsupported = false;
				}
			}
		}
	}

	return isUnsupported;
}

bool isUnsupportedFileName(const wchar_t* szFileName)
{
	const wstring fileName = szFileName;
	return isUnsupportedFileName(fileName);
}


Version::Version(const wstring& versionStr)
{
	try {
		auto ss = tokenizeString(versionStr, '.');

		if (ss.size() > 4)
		{
			std::wstring msg(L"\"");
			msg += versionStr;
			msg += L"\"";
			msg += L": Version parts are more than 4. The string to parse is not a valid version format. Let's make it default value in catch block.";
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
				msg += L": One of version character is not number. The string to parse is not a valid version format. Let's make it default value in catch block.";
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
		throw std::wstring(L"Unknown exception from \"Version::Version(const wstring& versionStr)\"");
#endif
	}
}


void Version::setVersionFrom(const wstring& filePath)
{
	if (filePath.empty() || !doesFileExist(filePath.c_str()))
		return;

	DWORD uselessArg = 0; // this variable is for passing the ignored argument to the functions
	DWORD bufferSize = ::GetFileVersionInfoSize(filePath.c_str(), &uselessArg);

	if (bufferSize <= 0)
		return;

	unsigned char* buffer = new unsigned char[bufferSize];
	::GetFileVersionInfo(filePath.c_str(), 0, bufferSize, buffer);

	VS_FIXEDFILEINFO* lpFileInfo = nullptr;
	UINT cbFileInfo = 0;
	VerQueryValue(buffer, L"\\", reinterpret_cast<LPVOID*>(&lpFileInfo), &cbFileInfo);
	if (cbFileInfo)
	{
		_major = (lpFileInfo->dwFileVersionMS & 0xFFFF0000) >> 16;
		_minor = lpFileInfo->dwFileVersionMS & 0x0000FFFF;
		_patch = (lpFileInfo->dwFileVersionLS & 0xFFFF0000) >> 16;
		_build = lpFileInfo->dwFileVersionLS & 0x0000FFFF;
	}
	delete[] buffer;
}

wstring Version::toString()
{
	if (_build == 0 && _patch == 0 && _minor == 0 && _major == 0) // ""
	{
		return L"";
	}
	else if (_build == 0 && _patch == 0 && _minor == 0) // "major"
	{
		return std::to_wstring(_major);
	}
	else if (_build == 0 && _patch == 0) // "major.minor"
	{
		std::wstring v = std::to_wstring(_major);
		v += L".";
		v += std::to_wstring(_minor);
		return v;
	}
	else if (_build == 0) // "major.minor.patch"
	{
		std::wstring v = std::to_wstring(_major);
		v += L".";
		v += std::to_wstring(_minor);
		v += L".";
		v += std::to_wstring(_patch);
		return v;
	}

	// "major.minor.patch.build"
	std::wstring ver = std::to_wstring(_major);
	ver += L".";
	ver += std::to_wstring(_minor);
	ver += L".";
	ver += std::to_wstring(_patch);
	ver += L".";
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


#define DEFAULT_MILLISEC 3000


//----------------------------------------------------

struct GetDiskFreeSpaceParamResult
{
	std::wstring _dirPath;
	ULARGE_INTEGER _freeBytesForUser {};
	BOOL _result = FALSE;
	bool _isTimeoutReached = true;

	GetDiskFreeSpaceParamResult(wstring dirPath) : _dirPath(dirPath) {};
};

DWORD WINAPI getDiskFreeSpaceExWorker(void* data)
{
	GetDiskFreeSpaceParamResult* inAndOut = static_cast<GetDiskFreeSpaceParamResult*>(data);
	inAndOut->_result = ::GetDiskFreeSpaceExW(inAndOut->_dirPath.c_str(), &(inAndOut->_freeBytesForUser), nullptr, nullptr);
	inAndOut->_isTimeoutReached = false;
	return ERROR_SUCCESS;
};

BOOL getDiskFreeSpaceWithTimeout(const wchar_t* dirPath, ULARGE_INTEGER* freeBytesForUser, DWORD milliSec2wait, bool* isTimeoutReached)
{
	GetDiskFreeSpaceParamResult data(dirPath);

	HANDLE hThread = ::CreateThread(NULL, 0, getDiskFreeSpaceExWorker, &data, 0, NULL);
	if (!hThread)
	{
		return FALSE;
	}

	// wait for our worker thread to complete or terminate it when the required timeout has elapsed
	DWORD dwWaitStatus = ::WaitForSingleObject(hThread, milliSec2wait == 0 ? DEFAULT_MILLISEC : milliSec2wait);
	switch (dwWaitStatus)
	{
		case WAIT_OBJECT_0: // Ok, the state of our worker thread is signaled, so it finished itself in the timeout given		
			// - nothing else to do here, except the thread handle closing later
			break;

		case WAIT_TIMEOUT: // the timeout interval elapsed, but the worker's state is still non-signaled
		default: // any other dwWaitStatus is a BAD one here
			// WAIT_FAILED or WAIT_ABANDONED
			::TerminateThread(hThread, dwWaitStatus);
			break;
	}
	CloseHandle(hThread);

	*freeBytesForUser = data._freeBytesForUser;

	if (isTimeoutReached != nullptr)
		*isTimeoutReached = data._isTimeoutReached;

	return data._result;
}


//----------------------------------------------------

struct GetAttrExParamResult
{
	wstring _filePath;
	WIN32_FILE_ATTRIBUTE_DATA _attributes{};
	BOOL _result = FALSE;
	DWORD _error = NO_ERROR;
	bool _isTimeoutReached = true;

	GetAttrExParamResult(wstring filePath): _filePath(filePath) {
		_attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	}
};

DWORD WINAPI getFileAttributesExWorker(void* data)
{
	GetAttrExParamResult* inAndOut = static_cast<GetAttrExParamResult*>(data);
	::SetLastError(NO_ERROR);
	inAndOut->_result = ::GetFileAttributesExW(inAndOut->_filePath.c_str(), GetFileExInfoStandard, &(inAndOut->_attributes));
	if (!(inAndOut->_result))
		inAndOut->_error = ::GetLastError();
	inAndOut->_isTimeoutReached = false;
	return ERROR_SUCCESS;
};

BOOL getFileAttributesExWithTimeout(const wchar_t* filePath, WIN32_FILE_ATTRIBUTE_DATA* fileAttr,
	DWORD milliSec2wait, bool* isTimeoutReached, DWORD* pdwWin32ApiError)
{
	GetAttrExParamResult data(filePath);

	HANDLE hThread = ::CreateThread(NULL, 0, getFileAttributesExWorker, &data, 0, NULL);
	if (!hThread)
	{
		return FALSE;
	}

	// wait for our worker thread to complete or terminate it when the required timeout has elapsed
	DWORD dwWaitStatus = ::WaitForSingleObject(hThread, milliSec2wait == 0 ? DEFAULT_MILLISEC : milliSec2wait);
	switch (dwWaitStatus)
	{
		case WAIT_OBJECT_0: // Ok, the state of our worker thread is signaled, so it finished itself in the timeout given		
			// - nothing else to do here, except the thread handle closing later
			break;

		case WAIT_TIMEOUT: // the timeout interval elapsed, but the worker's state is still non-signaled
		default: // any other dwWaitStatus is a BAD one here
			// WAIT_FAILED or WAIT_ABANDONED
			::TerminateThread(hThread, dwWaitStatus);
			break;
	}
	::CloseHandle(hThread);

	*fileAttr = data._attributes;

	if (isTimeoutReached != nullptr)
		*isTimeoutReached = data._isTimeoutReached;

	if (pdwWin32ApiError != nullptr)
		*pdwWin32ApiError = data._error;

	return data._result;
}

bool doesFileExist(const wchar_t* filePath, DWORD milliSec2wait, bool* isTimeoutReached)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	getFileAttributesExWithTimeout(filePath, &attributes, milliSec2wait, isTimeoutReached);
	return (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool doesDirectoryExist(const wchar_t* dirPath, DWORD milliSec2wait, bool* isTimeoutReached)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	getFileAttributesExWithTimeout(dirPath, &attributes, milliSec2wait, isTimeoutReached);
	return (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES && (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool doesPathExist(const wchar_t* path, DWORD milliSec2wait, bool* isTimeoutReached)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	getFileAttributesExWithTimeout(path, &attributes, milliSec2wait, isTimeoutReached);
	return (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES);
}


#if defined(__GNUC__)
#define LAMBDA_STDCALL __attribute__((__stdcall__))
#else
#define LAMBDA_STDCALL 
#endif

// check if the window rectangle intersects with any currently active monitor's working area
// (this func handles also possible extended monitors mode aka the MS Virtual Screen)
bool isWindowVisibleOnAnyMonitor(const RECT& rectWndIn)
{
	struct Param4InOut
	{
		const RECT& rectWndIn;
		bool isWndVisibleOut = false;
	};

	// callback func to check for intersection with each existing monitor
	auto callback = []([[maybe_unused]] HMONITOR hMon, [[maybe_unused]] HDC hdc, LPRECT lprcMon, LPARAM lpInOut) -> BOOL LAMBDA_STDCALL
	{
		Param4InOut* paramInOut = reinterpret_cast<Param4InOut*>(lpInOut);
		RECT rectIntersection{};
		if (::IntersectRect(&rectIntersection, &(paramInOut->rectWndIn), lprcMon))
		{
			paramInOut->isWndVisibleOut = true; // the window is at least partially visible on this monitor
			return FALSE; // ok, stop the enumeration
		}
		return TRUE; // continue enumeration as no intersection yet
	};

	// get scaled Virtual Screen size (scaled coordinates are saved by the Notepad++ into config.xml)
	// - for unscaled, one has to 1st set the SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) & then use GetSystemMetricsForDpi with 96
	// - for getting the VS RECT, we cannot use here the SystemParametersInfo with SPI_GETWORKAREA!
	//   (while the SPI_SETWORKAREA is working with the VS coordinates the SPI_GETWORKAREA not...)
	RECT rectVirtualScreen{ ::GetSystemMetrics(SM_XVIRTUALSCREEN), ::GetSystemMetrics(SM_YVIRTUALSCREEN),
		::GetSystemMetrics(SM_CXVIRTUALSCREEN), ::GetSystemMetrics(SM_CYVIRTUALSCREEN) }; 

	// 1) Before checking for intersections with individual monitors, we verify if the window's rectangle
	//    is within the MS Virtual Screen area. If it is outside, this func exits with false early,
	//    as the window in question cannot be visible on any individual monitor present.
	RECT rectIntersection{};
	if (!::IntersectRect(&rectIntersection, &rectWndIn, &rectVirtualScreen))
	{
		// the window in question is completely outside the overall Virtual Screen bounds
		return false;
	}

	// 2) Using the EnumDisplayMonitors WINAPI to check each present monitor's visible area, we ensure that we are only looking
	//    at monitors that are part of the Virtual Screen but not at Virtual Space coordinates where is NOT a monitor present.
	Param4InOut param4InOut{ rectWndIn, false };
	::EnumDisplayMonitors(NULL, &rectVirtualScreen, callback, reinterpret_cast<LPARAM>(&param4InOut));
	return param4InOut.isWndVisibleOut;
}

#pragma warning(disable:4996) // 'GetVersionExW': was declared deprecated
bool isCoreWindows()
{
	bool isCoreWindows = false;

	// older Windows (Windows Server 2008 R2-) check 1st
	OSVERSIONINFOEXW osviex{};
	osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	if (::GetVersionEx(reinterpret_cast<LPOSVERSIONINFOW>(&osviex)))
	{
		DWORD dwReturnedProductType = 0;
		if (::GetProductInfo(osviex.dwMajorVersion, osviex.dwMinorVersion, osviex.wServicePackMajor, osviex.wServicePackMinor, &dwReturnedProductType))
		{
			switch (dwReturnedProductType)
			{
				case PRODUCT_STANDARD_SERVER_CORE:
				case PRODUCT_STANDARD_A_SERVER_CORE:
				case PRODUCT_STANDARD_SERVER_CORE_V:
				case PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE:
				case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE:
				case PRODUCT_ENTERPRISE_SERVER_CORE:
				case PRODUCT_ENTERPRISE_SERVER_CORE_V:
				case PRODUCT_DATACENTER_SERVER_CORE:
				case PRODUCT_DATACENTER_A_SERVER_CORE:
				case PRODUCT_DATACENTER_SERVER_CORE_V:
				case PRODUCT_STORAGE_STANDARD_SERVER_CORE:
				case PRODUCT_STORAGE_WORKGROUP_SERVER_CORE:
				case PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE:
				case PRODUCT_STORAGE_EXPRESS_SERVER_CORE:
				case PRODUCT_WEB_SERVER_CORE:
					isCoreWindows = true;
			}
		}
	}

	if (!isCoreWindows)
	{
		// in Core Server 2012+, the recommended way to determine is via the Registry
		HKEY hKey = nullptr;
		if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
			0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			constexpr size_t bufLen = 127;
			wchar_t wszBuf[bufLen + 1]{}; // +1 ... to be always NULL-terminated string
			DWORD dataSize = sizeof(wchar_t) * bufLen;
			if (::RegQueryValueExW(hKey, L"InstallationType", nullptr, nullptr, reinterpret_cast<LPBYTE>(&wszBuf), &dataSize) == ERROR_SUCCESS)
			{
				if (lstrcmpiW(wszBuf, L"Server Core") == 0)
					isCoreWindows = true;
			}
			::RegCloseKey(hKey);
			hKey = nullptr;
		}
	}

	return isCoreWindows;
}
#pragma warning(default:4996)
