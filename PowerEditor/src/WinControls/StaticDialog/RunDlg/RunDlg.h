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
#include "RunDlg_rc.h"

#define CURRENTWORD_MAXLENGTH 2048

const wchar_t fullCurrentPath[] = L"FULL_CURRENT_PATH";
const wchar_t currentDirectory[] = L"CURRENT_DIRECTORY";
const wchar_t onlyFileName[] = L"FILE_NAME";
const wchar_t fileNamePart[] = L"NAME_PART";
const wchar_t fileExtPart[] = L"EXT_PART";
const wchar_t currentWord[] = L"CURRENT_WORD";
const wchar_t nppDir[] = L"NPP_DIRECTORY";
const wchar_t nppFullFilePath[] = L"NPP_FULL_FILE_PATH";
const wchar_t currentLine[] = L"CURRENT_LINE";
const wchar_t currentColumn[] = L"CURRENT_COLUMN";
const wchar_t currentLineStr[] = L"CURRENT_LINESTR";

int whichVar(wchar_t *str);
void expandNppEnvironmentStrs(const wchar_t *strSrc, wchar_t *stringDest, size_t strDestLen, HWND hWnd);

class Command {
public :
	Command() = default;
	explicit Command(const wchar_t *cmd) : _cmdLine(cmd){};
	explicit Command(const std::wstring& cmd) : _cmdLine(cmd){};
	HINSTANCE run(HWND hWnd);
	HINSTANCE run(HWND hWnd, const wchar_t* cwd);

protected :
	std::wstring _cmdLine;
private :
	void extractArgs(wchar_t *cmd2Exec, size_t cmd2ExecLen, wchar_t *args, size_t argsLen, const wchar_t *cmdEntier);
};

class RunDlg : public Command, public StaticDialog
{
public :
	RunDlg() = default;

	void doDialog(bool isRTL = false);
	void destroy() override {};

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	void addTextToCombo(const wchar_t *txt2Add) const;
	void removeTextFromCombo(const wchar_t *txt2Remove) const;
};

