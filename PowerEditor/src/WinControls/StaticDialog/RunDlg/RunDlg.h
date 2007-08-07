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

#include "StaticDialog.h"
#include "RunDlg_rc.h"
#include <string>

//static void extractArgs(char *cmd2Exec, char *args, const char *cmdEntier);

using namespace std;

const char fullCurrentPath[] = "FULL_CURRENT_PATH";
const char currentDirectory[] = "CURRENT_DIRECTORY";
const char onlyFileName[] = "FILE_NAME";
const char fileNamePart[] = "NAME_PART";
const char fileExtPart[] = "EXT_PART";
const char currentWord[] = "CURRENT_WORD";
const char nppDir[] = "NPP_DIRECTORY";

int whichVar(char *str);
void expandNppEnvironmentStrs(const char *strSrc, char *stringDest, size_t strDestLen, HWND hWnd);

class Command {
public :
	Command(){};
	Command(char *cmd) : _cmdLine(cmd){};
	Command(string cmd) : _cmdLine(cmd){};
	HINSTANCE run(HWND hWnd);

protected :
	string _cmdLine;
private :
	void extractArgs(char *cmd2Exec, char *args, const char *cmdEntier);
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
	void addTextToCombo(const char *txt2Add) const;
	void removeTextFromCombo(const char *txt2Remove) const;
};

#endif //RUN_DLG_H
