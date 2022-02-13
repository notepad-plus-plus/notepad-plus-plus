// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "Parameters.h"
#include "Processus.h"


void Process::run(bool isElevationRequired) const
{
	const TCHAR *opVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
	::ShellExecute(NULL, opVerb, _command.c_str(), _args.c_str(), _curDir.c_str(), SW_SHOWNORMAL);
}

unsigned long Process::runSync(bool isElevationRequired) const
{
	SHELLEXECUTEINFO ShExecInfo = {};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
	ShExecInfo.lpFile = _command.c_str();
	ShExecInfo.lpParameters = _args.c_str();
	ShExecInfo.lpDirectory = _curDir.c_str();
	ShExecInfo.nShow = SW_SHOWNORMAL;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
	if (!ShExecInfo.hProcess)
	{
		// throw exception
		throw GetLastErrorAsString(GetLastError());
	}

	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

	unsigned long exitCode;
	if (::GetExitCodeProcess(ShExecInfo.hProcess, &exitCode) == FALSE)
	{
		// throw exception
		throw GetLastErrorAsString(GetLastError());
	}

	return exitCode;
}
