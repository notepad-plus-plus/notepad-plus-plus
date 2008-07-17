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

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep) 
{
   if (code == EXCEPTION_ACCESS_VIOLATION)
      return EXCEPTION_EXECUTE_HANDLER;

   return EXCEPTION_CONTINUE_SEARCH;
}

std::string purgeMenuItemString(const char * menuItemStr, bool keepAmpersand)
{
	char cleanedName[64] = "";
	size_t j = 0;
	size_t menuNameLen = strlen(menuItemStr);
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