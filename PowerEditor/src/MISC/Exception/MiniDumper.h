//Adapted from http://www.codeproject.com/KB/debug/postmortemdebug_standalone1.aspx#_Reading_a_Minidump_with%20Visual%20Stud
//Modified for use by Npp
#ifndef MDUMP_H
#define MDUMP_H

#include <windows.h>
#include "dbghelp.h"

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