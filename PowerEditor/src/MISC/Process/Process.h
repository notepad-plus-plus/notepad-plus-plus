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

using namespace std;

enum progType {WIN32_PROG, CONSOLE_PROG};

class Process 
{
public:
    Process(const TCHAR *cmd, const TCHAR *args, const TCHAR *cDir)
		:_command(cmd), _args(args), _curDir(cDir){};

	void run();
 
protected:
    generic_string _command;
	generic_string _args;
	generic_string _curDir;
};

#endif //PROCESSUS_H

