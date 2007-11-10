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
#include <string>
#include <algorithm>

/*
DWORD ShortToLongPathName(LPCTSTR lpszShortPath, LPTSTR lpszLongPath, DWORD cchBuffer)
{
    // Catch null pointers.
    if (!lpszShortPath || !lpszLongPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Check whether the input path is valid.
    if (0xffffffff == GetFileAttributes(lpszShortPath))
        return 0;

    // Special characters.
    char const sep = '\\';
    char const colon = ':';

    // Make some short aliases for basic_strings of TCHAR.
    typedef std::basic_string<TCHAR> tstring;
    typedef tstring::traits_type traits;
    typedef tstring::size_type size;
    size const npos = tstring::npos;

    // Copy the short path into the work buffer and convert forward 
    // slashes to backslashes.
    tstring path = lpszShortPath;
    std::replace(path.begin(), path.end(), '/', sep);

    // We need a couple of markers for stepping through the path.
    size left = 0;
    size right = 0;

    // Parse the first bit of the path.
    if (path.length() >= 2 && isalpha(path[0]) && colon == path[1]) // Drive letter?
    {
        if (2 == path.length()) // 'bare' drive letter
        {
            right = npos; // skip main block
        }
        else if (sep == path[2]) // drive letter + backslash
        {
            // FindFirstFile doesn't like "X:\"
            if (3 == path.length())
            {
                right = npos; // skip main block
            }
            else
            {
                left = right = 3;
            }
        }
        else return 0; // parsing failure
    }
    else if (path.length() >= 1 && sep == path[0])
    {
        if (1 == path.length()) // 'bare' backslash
        {
            right = npos;  // skip main block
        }
        else 
        {
            if (sep == path[1]) // is it UNC?
            {
                // Find end of machine name
                right = path.find_first_of(sep, 2);
                if (npos == right)
                    return 0;

                // Find end of share name
                right = path.find_first_of(sep, right + 1);
                if (npos == right)
                    return 0;
            }
            ++right;
        }
    }
    // else FindFirstFile will handle relative paths

    // The data block for FindFirstFile.
    WIN32_FIND_DATA fd;

    // Main parse block - step through path.
    while (npos != right)
    {
        left = right; // catch up

        // Find next separator.
        right = path.find_first_of(sep, right);

        // Temporarily replace the separator with a null character so that
        // the path so far can be passed to FindFirstFile.
        if (npos != right)
            path[right] = 0;

        // See what FindFirstFile makes of the path so far.
        HANDLE hf = FindFirstFile(path.c_str(), &fd);
        if (INVALID_HANDLE_VALUE == hf)
            return 0;
        FindClose(hf);

        // Put back the separator.
        if (npos != right)
            path[right] = sep;

        // The file was found - replace the short name with the long.
        size old_len = (npos == right) ? path.length() - left : right - left;
        size new_len = traits::length(fd.cFileName);
        path.replace(left, old_len, fd.cFileName, new_len);

        // More to do?
        if (npos != right)
        {
            // Yes - move past separator .
            right = left + new_len + 1;

            // Did we overshoot the end? (i.e. path ends with a separator).
            if (right >= path.length())
                right = npos;
        }
    }

    // If buffer is too small then return the required size.
    if (cchBuffer <= path.length())
        return path.length() + 1;

    // Copy the buffer and return the number of characters copied.
    traits::copy(lpszLongPath, path.c_str(), path.length() + 1);
    return path.length();
}
*/

void systemMessage(const char *title)
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
	char str[32];
	sprintf(str, "%d", int2print);
	::MessageBox(NULL, str, "", MB_OK);
}

void printStr(const char *str2print)
{
	::MessageBox(NULL, str2print, "", MB_OK);
}

void writeLog(const char *logFileName, const char *log2write)
{	
	FILE *f = fopen(logFileName, "a+");
	const char * ptr = log2write;
	fwrite(log2write, sizeof(log2write[0]), strlen(log2write), f);
	fputc('\n', f);
	fflush(f);
	fclose(f);
}