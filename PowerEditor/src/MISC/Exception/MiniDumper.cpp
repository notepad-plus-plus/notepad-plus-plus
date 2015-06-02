//Adapted from http://www.codeproject.com/KB/debug/postmortemdebug_standalone1.aspx#_Reading_a_Minidump_with%20Visual%20Stud
//Modified for use by Npp

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


#include <shlwapi.h>
#include "MiniDumper.h"

LPCTSTR msgTitle = TEXT("Notepad++ crash analysis");

MiniDumper::MiniDumper()
{
}

bool MiniDumper::writeDump(EXCEPTION_POINTERS * pExceptionInfo)
{
	TCHAR szDumpPath[MAX_PATH];
	TCHAR szScratch[MAX_PATH];
	LPCTSTR szResult = NULL;
	bool retval = false;

	HMODULE hDll = ::LoadLibrary( TEXT("DBGHELP.DLL") );	//that wont work on older windows version than XP, #care :)

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			::GetModuleFileName(NULL, szDumpPath, MAX_PATH);
			::PathRemoveFileSpec(szDumpPath);
			lstrcat(szDumpPath, TEXT("\\NppDump.dmp"));

			// ask the user if they want to save a dump file
			int msgret = ::MessageBox(NULL, TEXT("Do you want to save a dump file?\r\nDoing so can aid in developing Notepad++."), msgTitle, MB_YESNO);
			if (msgret == IDYES)
			{
				// create the file
				HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
					if (bOK)
					{
						wsprintf( szScratch, TEXT("Saved dump file to '%s'"), szDumpPath );
						szResult = szScratch;
						retval = true;
					}
					else
					{
						wsprintf( szScratch, TEXT("Failed to save dump file to '%s' (error %d)"), szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					::CloseHandle(hFile);
				}
				else
				{
					wsprintf( szScratch, TEXT("Failed to create dump file '%s' (error %d)"), szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = TEXT("The debugging DLL is outdated,\r\nfind a recent copy of dbghelp.dll and install it.");
		}
	}
	else
	{
		szResult = TEXT("Unable to load the debugging DLL,\r\nfind a recent copy of dbghelp.dll and install it.");
	}

	if (szResult)
		::MessageBox(NULL, szResult, msgTitle, MB_OK);

	return retval;
}
