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


#ifndef PROCESSUS_H
#define PROCESSUS_H

#include <windows.h>
//#include <string>
using namespace std;

class Process 
{
public:
	Process() {};
	Process(const TCHAR *cmd, const TCHAR *cDir)
		: _stdoutStr(TEXT("")), _stderrStr(TEXT("")), _hPipeOutR(NULL),
		_hPipeErrR(NULL), _hProcess(NULL), _hProcessThread(NULL) {

		lstrcpy(_command, cmd);
		lstrcpy(_curDir, cDir);
	};

	BOOL run();

	const TCHAR * getStdout() const {
		return _stdoutStr.c_str();
	};
	
	const TCHAR * getStderr() const {
		return _stderrStr.c_str();
	};

	int getExitCode() const {
		return _exitCode;
	};

	bool hasStdout() {
		return (_stdoutStr.compare(TEXT("")) != 0);
	};

	bool hasStderr() {
		return (_stderrStr.compare(TEXT("")) != 0);
	};

protected:
	// LES ENTREES
    TCHAR _command[256];
	TCHAR _curDir[256];
	
	// LES SORTIES
	generic_string _stdoutStr;
	generic_string _stderrStr;
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
	void error(const TCHAR *txt2display, BOOL & returnCode, int errCode);
};

#endif //PROCESSUS_H

