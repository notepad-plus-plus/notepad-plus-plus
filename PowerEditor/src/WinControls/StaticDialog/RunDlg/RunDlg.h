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

#ifndef RUN_DLG_H
#define RUN_DLG_H

#ifndef RUN_DLG_RC_H
#include "RunDlg_rc.h"
#endif //RUN_DLG_RC_H

using namespace std;

#define CURRENTWORD_MAXLENGTH 2048

const TCHAR fullCurrentPath[] = TEXT("FULL_CURRENT_PATH");
const TCHAR currentDirectory[] = TEXT("CURRENT_DIRECTORY");
const TCHAR onlyFileName[] = TEXT("FILE_NAME");
const TCHAR fileNamePart[] = TEXT("NAME_PART");
const TCHAR fileExtPart[] = TEXT("EXT_PART");
const TCHAR currentWord[] = TEXT("CURRENT_WORD");
const TCHAR nppDir[] = TEXT("NPP_DIRECTORY");
const TCHAR currentLine[] = TEXT("CURRENT_LINE");
const TCHAR currentColumn[] = TEXT("CURRENT_COLUMN");

int whichVar(TCHAR *str);
void expandNppEnvironmentStrs(const TCHAR *strSrc, TCHAR *stringDest, size_t strDestLen, HWND hWnd);

class Command {
public :
	Command(){};
	Command(TCHAR *cmd) : _cmdLine(cmd){};
	Command(generic_string cmd) : _cmdLine(cmd){};
	HINSTANCE run(HWND hWnd);

protected :
	generic_string _cmdLine;
private :
	void extractArgs(TCHAR *cmd2Exec, TCHAR *args, const TCHAR *cmdEntier);
};

class RunDlg : public Command, public StaticDialog
{
public :
	RunDlg() : StaticDialog() {};

	void doDialog(bool isRTL = false);

    virtual void destroy() {

    };

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
	void addTextToCombo(const TCHAR *txt2Add) const;
	void removeTextFromCombo(const TCHAR *txt2Remove) const;
};

#endif //RUN_DLG_H
