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

#include <oleacc.h>
#include "Common.h"
#include "RunDlg_rc.h"

#define CURRENTWORD_MAXLENGTH 2048

const TCHAR fullCurrentPath[] = TEXT("FULL_CURRENT_PATH");
const TCHAR currentDirectory[] = TEXT("CURRENT_DIRECTORY");
const TCHAR onlyFileName[] = TEXT("FILE_NAME");
const TCHAR fileNamePart[] = TEXT("NAME_PART");
const TCHAR fileExtPart[] = TEXT("EXT_PART");
const TCHAR currentWord[] = TEXT("CURRENT_WORD");
const TCHAR nppDir[] = TEXT("NPP_DIRECTORY");
const TCHAR nppFullFilePath[] = TEXT("NPP_FULL_FILE_PATH");
const TCHAR currentLine[] = TEXT("CURRENT_LINE");
const TCHAR currentColumn[] = TEXT("CURRENT_COLUMN");
const TCHAR currentLineStr[] = TEXT("CURRENT_LINESTR");

int whichVar(TCHAR *str);
void expandNppEnvironmentStrs(const TCHAR *strSrc, TCHAR *stringDest, size_t strDestLen, HWND hWnd);

class Command {
public :
	Command() = default;
	explicit Command(const TCHAR *cmd) : _cmdLine(cmd){};
	explicit Command(const generic_string& cmd) : _cmdLine(cmd){};
	HINSTANCE run(HWND hWnd);
	HINSTANCE run(HWND hWnd, const TCHAR* cwd);

protected :
	generic_string _cmdLine;
private :
	void extractArgs(TCHAR *cmd2Exec, size_t cmd2ExecLen, TCHAR *args, size_t argsLen, const TCHAR *cmdEntier);
};

class RunDlg : public Command, public StaticDialog
{
public :
	RunDlg() = default;

	void doDialog(bool isRTL = false);
    virtual void destroy() {};

protected :
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
	void addTextToCombo(const TCHAR *txt2Add) const;
	void removeTextFromCombo(const TCHAR *txt2Remove) const;
};

