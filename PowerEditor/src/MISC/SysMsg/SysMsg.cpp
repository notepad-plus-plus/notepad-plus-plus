//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "SysMsg.h"
#include <memory>
#include <algorithm>

WcharMbcsConvertor * WcharMbcsConvertor::_pSelf = new WcharMbcsConvertor;

void systemMessage(const TCHAR *title)
{
  LPVOID lpMsgBuf;
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
				 ::GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL );// Process any inserts in lpMsgBuf.
  MessageBox( NULL, (LPTSTR)lpMsgBuf, title, MB_OK | MB_ICONSTOP);
  ::LocalFree(lpMsgBuf);
}

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

void writeLog(const TCHAR *logFileName, const TCHAR *log2write)
{	
	FILE *f = generic_fopen(logFileName, TEXT("a+"));
	const TCHAR * ptr = log2write;
	fwrite(log2write, sizeof(log2write[0]), lstrlen(log2write), f);
	fputc('\n', f);
	fflush(f);
	fclose(f);
}

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep) 
{
   if (code == EXCEPTION_ACCESS_VIOLATION)
      return EXCEPTION_EXECUTE_HANDLER;

   return EXCEPTION_CONTINUE_SEARCH;
}

int getCpFromStringValue(const TCHAR * encodingStr)
{
	if (!encodingStr)
		return CP_ACP;

	if (generic_stricmp(TEXT("windows-1250"), encodingStr) == 0)
		return 1250;
	if (generic_stricmp(TEXT("windows-1251"), encodingStr) == 0)
		return 1251;
	if (generic_stricmp(TEXT("windows-1252"), encodingStr) == 0)
		return 1252;
	if (generic_stricmp(TEXT("windows-1253"), encodingStr) == 0)
		return 1253;
	if (generic_stricmp(TEXT("windows-1254"), encodingStr) == 0)
		return 1254;
	if (generic_stricmp(TEXT("windows-1255"), encodingStr) == 0)
		return 1255;
	if (generic_stricmp(TEXT("windows-1256"), encodingStr) == 0)
		return 1256;
	if (generic_stricmp(TEXT("windows-1257"), encodingStr) == 0)
		return 1257;
	if (generic_stricmp(TEXT("windows-1258"), encodingStr) == 0)
		return 1258;

	if (generic_stricmp(TEXT("big5"), encodingStr) == 0)
		return 950;
	if (generic_stricmp(TEXT("gb2312"), encodingStr) == 0)
		return 936;
	if (generic_stricmp(TEXT("shift_jis"), encodingStr) == 0)
		return 936;
	if (generic_stricmp(TEXT("euc-kr"), encodingStr) == 0)
		return 936;
	if (generic_stricmp(TEXT("tis-620"), encodingStr) == 0)
		return 874;

	if (generic_stricmp(TEXT("iso-8859-8"), encodingStr) == 0)
		return 28598;

	if (generic_stricmp(TEXT("utf-8"), encodingStr) == 0)
		return 65001;

	return CP_ACP;
}

std::basic_string<TCHAR> purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand)
{
	TCHAR cleanedName[64] = TEXT("");
	size_t j = 0;
	size_t menuNameLen = lstrlen(menuItemStr);
	for(size_t k = 0 ; k < menuNameLen ; k++) 
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

const wchar_t * WcharMbcsConvertor::char2wchar(const char * mbcs2Convert, UINT codepage)
{
	if (!_wideCharStr)
	{
		_wideCharStr = new wchar_t[initSize];
		_wideCharAllocLen = initSize;
	}

	int len = MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, 0);
	if (len > 0)
	{
		if (len > int(_wideCharAllocLen))
		{
			delete [] _wideCharStr;
			_wideCharAllocLen = len;
			_wideCharStr = new wchar_t[_wideCharAllocLen];
		}
		MultiByteToWideChar(codepage, 0, mbcs2Convert, -1, _wideCharStr, len);
	}
	else
		_wideCharStr[0] = 0;

	return _wideCharStr;
}

const char * WcharMbcsConvertor::wchar2char(const wchar_t * wcharStr2Convert, UINT codepage) 
{
	if (!_multiByteStr)
	{
		_multiByteStr = new char[initSize];
		_multiByteAllocLen = initSize;
	}

	int len = WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, 0, NULL, NULL);
	if (len > 0)
	{
		if (len > int(_multiByteAllocLen))
		{
			delete [] _multiByteStr;
			_multiByteAllocLen = len;
			_multiByteStr = new char[_multiByteAllocLen];
		}
		WideCharToMultiByte(codepage, 0, wcharStr2Convert, -1, _multiByteStr, len, NULL, NULL);
	}
	else
		_multiByteStr[0] = 0;

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
