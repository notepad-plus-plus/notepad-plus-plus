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

#ifndef PROCESSUS_H
#define PROCESSUS_H

#include <windows.h>
#include <string>
using namespace std;

class Process 
{
public:
	Process() {};
	Process(const char *cmd, const char *cDir/*, unsigned int id = 0*/)
		: _stdoutStr(""), _stderrStr(""), _hPipeOutR(NULL),
		_hPipeErrR(NULL), _hProcess(NULL), _hProcessThread(NULL) {

		strcpy(_command, cmd);
		strcpy(_curDir, cDir);
		//_pid = id;
	};

	BOOL run();

	const char * getStdout() const {
		return _stdoutStr.c_str();
	};
	
	const char * getStderr() const {
		return _stderrStr.c_str();
	};

	int getExitCode() const {
		return _exitCode;
	};

	bool hasStdout() {
		return _stdoutStr.compare("");
	};

	bool hasStderr() {
		return _stderrStr.compare("");
	};

protected:
	// LES ENTREES
    char _command[256];
	char _curDir[256];
	
	// LES SORTIES
	string _stdoutStr;
	string _stderrStr;
	int _exitCode;

	// LES HANDLES
    HANDLE _hPipeOutR;
	HANDLE _hPipeErrR;
	HANDLE _hProcess;
	HANDLE _hProcessThread;

    //UINT _pid;   // process ID assigned by caller
	
	static DWORD WINAPI staticListenerStdOut(void * myself){
		((Process *)myself)->listenerStdOut();
		return 0;
	};
	static DWORD WINAPI staticListenerStdErr(void * myself) {
		((Process *)myself)->listenerStdErr();
		return 0;
	};
	void listenerStdOut();
	void listenerStdErr();
	void error(const char *txt2display, BOOL & returnCode, int errCode);
};

#endif //PROCESSUS_H

