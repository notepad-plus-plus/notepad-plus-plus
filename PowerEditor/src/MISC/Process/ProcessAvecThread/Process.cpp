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


#include "precompiledHeaders.h"
#include "process.h"
//#include "SysMsg.h"

BOOL Process::run()
{
	BOOL result = TRUE;

	// stdout & stderr pipes for process to write
	HANDLE hPipeOutW = NULL;
	HANDLE hPipeErrW = NULL;

	HANDLE hListenerStdOutThread = NULL;
	HANDLE hListenerStdErrThread = NULL;

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
		startup.wShowWindow = SW_HIDE; // hidden console window
		startup.hStdInput = NULL; // not used
		startup.hStdOutput = hPipeOutW;
		startup.hStdError = hPipeErrW;

		BOOL started = ::CreateProcess(NULL,        // command is part of input string
						_command,         // (writeable) command string
						NULL,        // process security
						NULL,        // thread security
						TRUE,        // inherit handles flag
						CREATE_SUSPENDED,           // flags
						NULL,        // inherit environment
						_curDir,        // inherit directory
						&startup,    // STARTUPINFO
						&procinfo);  // PROCESS_INFORMATION
		
		_hProcess = procinfo.hProcess;
		_hProcessThread = procinfo.hThread;

		if(!started)
			error(TEXT("CreateProcess"), result, 1002);

		hListenerEvent[0] = ::CreateEvent(NULL, FALSE, FALSE, TEXT("listenerEvent"));
		if(!hListenerEvent[0])
			error(TEXT("CreateEvent"), result, 1003);

		hListenerEvent[1] = ::CreateEvent(NULL, FALSE, FALSE, TEXT("listenerStdErrEvent"));
		if(!hListenerEvent[1])
			error(TEXT("CreateEvent"), result, 1004);

		hListenerStdOutThread = ::CreateThread(NULL, 0, staticListenerStdOut, this, 0, NULL);
		if (!hListenerStdOutThread)
			error(TEXT("CreateThread"), result, 1005);
		
		hListenerStdErrThread = ::CreateThread(NULL, 0, staticListenerStdErr, this, 0, NULL);
		if (!hListenerStdErrThread)
			error(TEXT("CreateThread"), result, 1006);

		::WaitForSingleObject(_hProcess, INFINITE);
		::WaitForMultipleObjects(2, hListenerEvent, TRUE, INFINITE);
	} catch (int /*coderr*/){}

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
	if (hListenerEvent[0])
		::CloseHandle(hListenerEvent[0]);
	if (hListenerEvent[1])
		::CloseHandle(hListenerEvent[1]);

	return result;
}


#define MAX_LINE_LENGTH 1024

void Process::listenerStdOut()
{
	DWORD bytesAvail = 0;
	BOOL result = 0;
	HANDLE hListenerEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerEvent"));

	TCHAR bufferOut[MAX_LINE_LENGTH + 1];

	int nExitCode = STILL_ACTIVE;
	
	DWORD outbytesRead;

	::ResumeThread(_hProcessThread);

    bool goOn = true;
	while (goOn)
	{ // got data
		memset(bufferOut,0x00,MAX_LINE_LENGTH + 1); 
		int taille = sizeof(bufferOut) - sizeof(TCHAR);
		
		Sleep(50);

		if (!::PeekNamedPipe(_hPipeOutR, bufferOut, taille, &outbytesRead, &bytesAvail, NULL)) 
		{
			bytesAvail = 0;
            goOn = false;
			break;
		}

		if(outbytesRead)
		{
			result = :: ReadFile(_hPipeOutR, bufferOut, taille, &outbytesRead, NULL);
			if ((!result) && (outbytesRead == 0))
            {
                goOn = false;
				break;
            }
		}

		bufferOut[outbytesRead] = '\0';
		generic_string s;
		s.assign(bufferOut);
		_stdoutStr += s;

		if (::GetExitCodeProcess(_hProcess, (unsigned long*)&nExitCode))
		{
			if (nExitCode != STILL_ACTIVE)
            {
                goOn = false;
				break; // EOF condition
            }
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
	DWORD bytesAvail = 0;
	BOOL result = 0;
	HANDLE hListenerEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("listenerStdErrEvent"));

	int taille = 0;
	TCHAR bufferErr[MAX_LINE_LENGTH + 1];
	int nExitCode = STILL_ACTIVE;

	::ResumeThread(_hProcessThread);

    bool goOn = true; 
	while (goOn)
	{ // got data
		memset(bufferErr, 0x00, MAX_LINE_LENGTH + 1);
		taille = sizeof(bufferErr) - sizeof(TCHAR);

		Sleep(50);
		DWORD errbytesRead;
		if (!::PeekNamedPipe(_hPipeErrR, bufferErr, taille, &errbytesRead, &bytesAvail, NULL)) 
		{
			bytesAvail = 0;
            goOn = false;
			break;
		}

		if(errbytesRead)
		{
			result = :: ReadFile(_hPipeErrR, bufferErr, taille, &errbytesRead, NULL);
			if ((!result) && (errbytesRead == 0))
            {
                goOn = false;
				break;
            }
		}
		bufferErr[errbytesRead] = '\0';
		generic_string s;
		s.assign(bufferErr);
		_stderrStr += s;

		if (::GetExitCodeProcess(_hProcess, (unsigned long*)&nExitCode))
		{
			if (nExitCode != STILL_ACTIVE)
            {
                goOn = false;
				break; // EOF condition
            }
		}
		//else
			//break;
	}
	//_exitCode = nExitCode;

	if(!::SetEvent(hListenerEvent))
	{
		systemMessage(TEXT("Thread stdout listener"));
	}
}

void Process::error(const TCHAR *txt2display, BOOL & returnCode, int errCode)
{
	systemMessage(txt2display);
	returnCode = FALSE;
	throw int(errCode);
}
