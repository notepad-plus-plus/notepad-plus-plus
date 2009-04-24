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

#include "process.h"

BOOL Process::run()
{
	BOOL result = TRUE;

	// stdout & stderr pipes for process to write
	HANDLE hPipeOutW = NULL;
	HANDLE hPipeErrW = NULL;

	HANDLE hListenerStdOutThread = NULL;
	HANDLE hListenerStdErrThread = NULL;

	HANDLE hWaitForProcessEndThread = NULL;

	HANDLE hListenerEvent[2];
	hListenerEvent[0] = NULL;
	hListenerEvent[1] = NULL;

	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE }; // inheritable handle

	try {
		// Create stdout pipe
		if (!::CreatePipe(&_hPipeOutR, &hPipeOutW, &sa, 0))
			error(TEXT("CreatePipe"), result, 1000);
		
		// Create stderr pipe
		if (!::CreatePipe(&_hPipeErrR, &hPipeErrW, &sa, 0))
			error(TEXT("CreatePipe"), result, 1001);

		STARTUPINFO startup;
		PROCESS_INFORMATION procinfo;
		::ZeroMemory(&startup, sizeof(startup));
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        startup.wShowWindow = (_type == WIN32_PROG)?SW_SHOW:SW_HIDE; // hidden console window
		startup.hStdInput = NULL; // not used
		startup.hStdOutput = hPipeOutW;
		startup.hStdError = hPipeErrW;

		generic_string cmd = TEXT("\"");
		cmd += _command;
		cmd += TEXT("\"");

		if (_args[0])
		{
			cmd += TEXT(" ");
			cmd += _args;
		}
        BOOL started = ::CreateProcess(NULL,        // command is part of input generic_string
						(TCHAR *)cmd.c_str(),         // (writeable) command generic_string
						NULL,        // process security
						NULL,        // thread security
						TRUE,        // inherit handles flag
						(_type == WIN32_PROG)?NULL:CREATE_SUSPENDED,           // flags
						NULL,        // inherit environment
						_curDir,        // inherit directory
						&startup,    // STARTUPINFO
						&procinfo);  // PROCESS_INFORMATION
		
		_hProcess = procinfo.hProcess;
		_hProcessThread = procinfo.hThread;

		if(!started)
			error(TEXT("CreateProcess"), result, 1002);

        if (_type == CONSOLE_PROG)
        {
		    hListenerEvent[0] = ::CreateEvent(NULL, FALSE, FALSE, TEXT("listenerEvent"));
		    if(!hListenerEvent[0])
			    error(TEXT("CreateEvent"), result, 1003);

		    hListenerEvent[1] = ::CreateEvent(NULL, FALSE, FALSE, TEXT("listenerStdErrEvent"));
		    if(!hListenerEvent[1])
			    error(TEXT("CreateEvent"), result, 1004);

        
		    // The process is running so we set this to FALSE
		    _bProcessEnd = FALSE;

		    hWaitForProcessEndThread = ::CreateThread(NULL, 0, staticWaitForProcessEnd, this, 0, NULL);
		    if (!hWaitForProcessEndThread)
			    error(TEXT("CreateThread"), result, 1005);

		    hListenerStdOutThread = ::CreateThread(NULL, 0, staticListenerStdOut, this, 0, NULL);
		    if (!hListenerStdOutThread)
			    error(TEXT("CreateThread"), result, 1006);
    		
		    hListenerStdErrThread = ::CreateThread(NULL, 0, staticListenerStdErr, this, 0, NULL);
		    if (!hListenerStdErrThread)
			    error(TEXT("CreateThread"), result, 1007);

		    // We wait until the process is over
		    // TO DO: This should be a bit secured in case something happen and the
		    // _bProcessEnd variable never gets set to TRUE... (by checking process
		    // state as well for instance to see if it is still running...)
		    while (!_bProcessEnd)
		    {
			    MSG msg;
			    while( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
			    {
				    if( msg.message == WM_QUIT)
				    {
					    ::PostQuitMessage(0);
					    // We do not exit but simply break in order to close
					    // handles properly
					    _bProcessEnd = TRUE;
					    break;
				    }
				    else
				    {
					    ::TranslateMessage( &msg);
					    ::DispatchMessage( &msg);
				    }
			    }
		    }
        }
	} catch (int coderr){
		TCHAR str[10];
		wsprintf(str, TEXT("%d"), coderr);
		::MessageBox(NULL, str, TEXT("Exception :"), MB_OK);
	}

	// on va fermer toutes les handles
	if (hPipeOutW)
		::CloseHandle(hPipeOutW);
	if (hPipeErrW)
		::CloseHandle(hPipeErrW);
	if (_hPipeOutR)
		::CloseHandle(_hPipeOutR);
	if (_hPipeErrR)
		::CloseHandle(_hPipeErrR);
	if (hListenerStdOutThread)
		::CloseHandle(hListenerStdOutThread);
	if (hListenerStdErrThread)
		::CloseHandle(hListenerStdErrThread);
	if (hWaitForProcessEndThread)
		::CloseHandle(hWaitForProcessEndThread);
	if (hListenerEvent[0])
		::CloseHandle(hListenerEvent[0]);
	if (hListenerEvent[1])
		::CloseHandle(hListenerEvent[1]);

	return result;
}


#define MAX_LINE_LENGTH 1024

void Process::listenerStdOut()
{
	//BOOL Result = 0;
	//DWORD size = 0;
	DWORD bytesAvail = 0;
	BOOL result = 0;
	HANDLE hListenerEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerEvent"));
	//FILE *fp = NULL;

	int taille = 0;
	TCHAR bufferOut[MAX_LINE_LENGTH + 1];
	//TCHAR bufferErr[MAX_LINE_LENGTH + 1];

	int nExitCode = STILL_ACTIVE;
	
	DWORD outbytesRead;

	::ResumeThread(_hProcessThread);

	while (true)
	{ // got data
		memset(bufferOut,0x00,MAX_LINE_LENGTH + 1); 
		//memset(bufferErr,0x00,MAX_LINE_LENGTH + 1);
		taille = sizeof(bufferOut) - sizeof(TCHAR);
		
		Sleep(50);

		if (!::PeekNamedPipe(_hPipeOutR, bufferOut, taille, &outbytesRead, &bytesAvail, NULL)) 
		{
			bytesAvail = 0;
			break;
		}

		if(outbytesRead)
		{
			result = :: ReadFile(_hPipeOutR, bufferOut, taille, &outbytesRead, NULL);
			if ((!result) && (outbytesRead == 0))
				break;
		}
		//outbytesRead = lstrlen(bufferOut);
		bufferOut[outbytesRead] = '\0';
		generic_string s;
		s.assign(bufferOut);
		_stdoutStr += s;

		if (::GetExitCodeProcess(_hProcess, (unsigned long*)&nExitCode))
		{
			if (nExitCode != STILL_ACTIVE)
				break; // EOF condition
		}
		//else
			//break;
	}
	_exitCode = nExitCode;

	if(!::SetEvent(hListenerEvent))
	{
		systemMessage(TEXT("Thread listenerStdOut"));
	}
}

void Process::listenerStdErr()
{
	//BOOL Result = 0;
	//DWORD size = 0;
	DWORD bytesAvail = 0;
	BOOL result = 0;
	HANDLE hListenerEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerStdErrEvent"));

	int taille = 0;
	//TCHAR bufferOut[MAX_LINE_LENGTH + 1];
	TCHAR bufferErr[MAX_LINE_LENGTH + 1];

	int nExitCode = STILL_ACTIVE;
	
	DWORD errbytesRead;

	::ResumeThread(_hProcessThread);

	while (true)
	{ // got data
		memset(bufferErr, 0x00, MAX_LINE_LENGTH + 1);
		taille = sizeof(bufferErr) - sizeof(TCHAR);

		Sleep(50);

		if (!::PeekNamedPipe(_hPipeErrR, bufferErr, taille, &errbytesRead, &bytesAvail, NULL)) 
		{
			bytesAvail = 0;
			break;
		}

		if(errbytesRead)
		{
			result = :: ReadFile(_hPipeErrR, bufferErr, taille, &errbytesRead, NULL);
			if ((!result) && (errbytesRead == 0))
				break;
		}
		//outbytesRead = lstrlen(bufferOut);
		bufferErr[errbytesRead] = '\0';
		generic_string s;
		s.assign(bufferErr);
		_stderrStr += s;

		if (::GetExitCodeProcess(_hProcess, (unsigned long*)&nExitCode))
		{
			if (nExitCode != STILL_ACTIVE)
				break; // EOF condition
		}
	}

	if(!::SetEvent(hListenerEvent))
	{
		systemMessage(TEXT("Thread stdout listener"));
	}
}

void Process::waitForProcessEnd()
{
	HANDLE hListenerEvent[2];
	hListenerEvent[0] = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerEvent"));
	hListenerEvent[1] = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerStdErrEvent"));

	::WaitForSingleObject(_hProcess, INFINITE);
	::WaitForMultipleObjects(2, hListenerEvent, TRUE, INFINITE);

	_bProcessEnd = TRUE;
}

void Process::error(const TCHAR *txt2display, BOOL & returnCode, int errCode)
{
	systemMessage(txt2display);
	returnCode = FALSE;
	throw int(errCode);
}
