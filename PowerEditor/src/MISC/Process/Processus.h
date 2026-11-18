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


#pragma once

#include "Common.h"

enum progType {WIN32_PROG, CONSOLE_PROG};

class Process
{
public:
    Process(const wchar_t* cmd, const wchar_t* args, const wchar_t* cDir)
		:_command(cmd), _args(args), _curDir(cDir){}

	void run(bool isElevationRequired = false) const;
	unsigned long runSync(bool isElevationRequired = false) const;

protected:
    std::wstring _command;
	std::wstring _args;
	std::wstring _curDir;
};

