//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
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
//Adapted from http://www.codeproject.com/KB/debug/postmortemdebug_standalone1.aspx#_Reading_a_Minidump_with%20Visual%20Stud
//Modified for use by Npp

#ifndef MDUMP_H
#define MDUMP_H


// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									const PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									const PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									const PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

class MiniDumper {
public:
	MiniDumper();
	bool writeDump(EXCEPTION_POINTERS * pExceptionInfo);
};

#endif //MDUMP_H