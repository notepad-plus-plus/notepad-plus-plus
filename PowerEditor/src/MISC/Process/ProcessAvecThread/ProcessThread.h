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


#ifndef PROCESS_THREAD_H
#define PROCESS_THREAD_H

#include "process.h"

class ProcessThread
{
public :
	ProcessThread(const TCHAR *appName, const TCHAR *cmd, const TCHAR *cDir, HWND hwnd) : _hwnd(hwnd) {
		lstrcpy(_appName, appName);
		lstrcpy(_command, cmd);
		lstrcpy(_curDir, cDir);
	};
	
	BOOL run(){
		HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, TEXT("localVarProcessEvent"));

		_hProcessThread = ::CreateThread(NULL, 0, staticLauncher, this, 0, NULL);

		::WaitForSingleObject(hEvent, INFINITE);

		::CloseHandle(hEvent);
		return TRUE;
	};

protected :
	// ENTREES
	TCHAR _appName[256];
    TCHAR _command[256];
	TCHAR _curDir[256];
	HWND _hwnd;
	HANDLE _hProcessThread;

	static DWORD WINAPI staticLauncher(void *myself) {
		((ProcessThread *)myself)->launch();
		return TRUE;
	};

	bool launch() {
		HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("localVarProcessEvent"));
		HWND hwnd = _hwnd;
		TCHAR appName[256];
		lstrcpy(appName, _appName);
		HANDLE hMyself = _hProcessThread;

		Process process(_command, _curDir);

		if(!::SetEvent(hEvent))
		{
			systemMessage(TEXT("Thread launcher"));
		}

		process.run();
		
		int code = process.getExitCode();
		TCHAR codeStr[256];
		generic_sprintf(codeStr, TEXT("%s : %0.4X"), appName, code);
		::MessageBox(hwnd, process.getStdout(), codeStr, MB_OK);
		
		if (process.hasStderr())
			::MessageBox(hwnd, process.getStderr(), codeStr, MB_OK);

		::CloseHandle(hMyself);
		return true;
	};
};

#endif PROCESS_THREAD_H
