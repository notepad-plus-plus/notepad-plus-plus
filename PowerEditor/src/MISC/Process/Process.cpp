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

#include "precompiledHeaders.h"
#include "Parameters.h"
#include "process.h"


void Process::run()
{
	TCHAR *opVerb = TEXT("open");
	winVer winVersion = (NppParameters::getInstance())->getWinVersion();
	if (winVersion == WV_VISTA || winVersion == WV_WIN7)
		opVerb = TEXT("runas");
	::ShellExecute(NULL, opVerb, _command.c_str(), _args.c_str(), _curDir.c_str(), SW_SHOWNORMAL);
}

