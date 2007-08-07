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

#ifndef PROCESS_THREAD_H
#define PROCESS_THREAD_H

#include "process.h"

class ProcessThread
{
public :
	ProcessThread(const char *appName, const char *cmd, const char *cDir, HWND hwnd) : _hwnd(hwnd) {
		strcpy(_appName, appName);
		strcpy(_command, cmd);
		strcpy(_curDir, cDir);
	};
	
	BOOL run(){
		HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, "localVarProcessEvent");

		_hProcessThread = ::CreateThread(NULL, 0, staticLauncher, this, 0, NULL);

		::WaitForSingleObject(hEvent, INFINITE);

		::CloseHandle(hEvent);
		return TRUE;
	};

protected :
	// ENTREES
	char _appName[256];
    char _command[256];
	char _curDir[256];
	HWND _hwnd;
	HANDLE _hProcessThread;

	static DWORD WINAPI staticLauncher(void *myself) {
		((ProcessThread *)myself)->launch();
		return TRUE;
	};

	bool launch() {
		HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, "localVarProcessEvent");
		HWND hwnd = _hwnd;
		char appName[256];
		strcpy(appName, _appName);
		HANDLE hMyself = _hProcessThread;

		Process process(_command, _curDir);

		if(!::SetEvent(hEvent))
		{
			systemMessage("Thread launcher");
		}

		process.run();
		
		int code = process.getExitCode();
		char codeStr[256];
		sprintf(codeStr, "%s : %0.4X", appName, code);
		::MessageBox(hwnd, (char *)process.getStdout(), codeStr, MB_OK);
		
		if (process.hasStderr())
			::MessageBox(hwnd, (char *)process.getStderr(), codeStr, MB_OK);

		::CloseHandle(hMyself);
		return true;
	};
};

#endif PROCESS_THREAD_H
