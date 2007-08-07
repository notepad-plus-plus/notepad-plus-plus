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

#include "RunDlg.h"
#include "FileDialog.h"
//#include "resource.h"
#include "Notepad_plus_msgs.h"
#include "shortcut.h"
#include "Parameters.h"
#include "Notepad_plus.h"


void Command::extractArgs(char *cmd2Exec, char *args, const char *cmdEntier)
{
	int i = 0;
	for ( ; i < int(strlen(cmdEntier)) ; i++)
	{
		if (cmdEntier[i] != ' ')
			cmd2Exec[i] = cmdEntier[i];
		else
			break;
	}
	cmd2Exec[i] = '\0';
	
	if (i < int(strlen(cmdEntier)))
	{
		for ( ; (i < int(strlen(cmdEntier))) && (cmdEntier[i] == ' ') ; i++);
		if (i < int(strlen(cmdEntier)))
		{
			for (int k = 0 ; i <= int(strlen(cmdEntier)) ; i++, k++)
			{
				args[k] = cmdEntier[i];
			}
		}

		int l = strlen(args);
		if (args[l-1] == ' ')
		{
			for (l -= 2 ; (l > 0) && (args[l] == ' ') ; l--);
			args[l+1] = '\0';
		}

	}
	else
		args[0] = '\0';
}


int whichVar(char *str)
{
	if (!strcmp(fullCurrentPath, str))
		return FULL_CURRENT_PATH;
	else if (!strcmp(currentDirectory, str))
		return CURRENT_DIRECTORY;
	else if (!strcmp(onlyFileName, str))
		return FILE_NAME;
	else if (!strcmp(fileNamePart, str))
		return NAME_PART;
	else if (!strcmp(fileExtPart, str))
		return EXT_PART;
	else if (!strcmp(currentWord, str))
		return CURRENT_WORD;
	else if (!strcmp(nppDir, str))
		return NPP_DIRECTORY;
	return VAR_NOT_RECOGNIZED;
}

// Since I'm sure the length will be 256, I won't check the strlen : watch out!
void expandNppEnvironmentStrs(const char *strSrc, char *stringDest, size_t strDestLen, HWND hWnd)
{
	size_t j = 0;
	for (size_t i = 0  ; i < strlen(strSrc) ; i++)
	{
		int iBegin = -1;
		int iEnd = -1;
		if ((strSrc[i] == '$') && (strSrc[i+1] == '('))
		{
			iBegin = i += 2;
			for ( ; i < strlen(strSrc) ; i++)
			{
				if (strSrc[i] == ')')
				{
					iEnd = i - 1;
					break;
				}
			}
		}
		if (iBegin != -1)
		{
			if (iEnd != -1)
			{
				char str[256];
				int m = 0;
				for (int k = iBegin  ; k <= iEnd ; k++)
					str[m++] = strSrc[k];
				str[m] = '\0';

				int internalVar = whichVar(str);
				if (internalVar == VAR_NOT_RECOGNIZED)
				{
					i = iBegin - 2;
					stringDest[j++] = strSrc[i];
				}
				else
				{
					char expandedStr[256];
					::SendMessage(hWnd, RUNCOMMAND_USER + internalVar, MAX_PATH, (LPARAM)expandedStr);
					for (size_t p = 0 ; p < strlen(expandedStr) ; p++)
						stringDest[j++] = expandedStr[p];
				}
			}
			else
			{
				i = iBegin - 2;
				stringDest[j++] = strSrc[i];
			}
		}
		else
			stringDest[j++] = strSrc[i];
	}
	stringDest[j] = '\0';
}

HINSTANCE Command::run(HWND hWnd)
{
	char cmdPure[MAX_PATH];
	char cmdIntermediate[MAX_PATH];
	char cmd2Exec[MAX_PATH];
	char args[MAX_PATH];
	char argsIntermediate[MAX_PATH];
	char args2Exec[MAX_PATH];

	extractArgs(cmdPure, args, _cmdLine.c_str());
	::ExpandEnvironmentStrings(cmdPure, cmdIntermediate, sizeof(cmd2Exec));
	::ExpandEnvironmentStrings(args, argsIntermediate, sizeof(args));
	expandNppEnvironmentStrs(cmdIntermediate, cmd2Exec, sizeof(cmd2Exec), hWnd);
	expandNppEnvironmentStrs(argsIntermediate, args2Exec, sizeof(args2Exec), hWnd);

	return ::ShellExecute(hWnd, "open", cmd2Exec, args2Exec, ".", SW_SHOW);
}

BOOL CALLBACK RunDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{/*
		case WM_INITDIALOG :
		{
			getClientRect(_rc);
			return TRUE;
		}
		*/

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL :
					display(false);
					return TRUE;
				
				case IDOK :
				{
					char cmd[MAX_PATH];
					::GetDlgItemText(_hSelf, IDC_COMBO_RUN_PATH, cmd, MAX_PATH);
					_cmdLine = cmd;

					HINSTANCE hInst = run(_hParent);
					if (int(hInst) > 32)
					{
						addTextToCombo(_cmdLine.c_str());
						display(false);
					}
					else
					{
						removeTextFromCombo(_cmdLine.c_str());
					}
					return TRUE;
				}
				case IDC_BUTTON_SAVE :
				{
					std::vector<UserCommand> & theUserCmds = (NppParameters::getInstance())->getUserCommandList();

					char cmd[MAX_PATH];
					::GetDlgItemText(_hSelf, IDC_COMBO_RUN_PATH, cmd, MAX_PATH);
					UserCommand uc(cmd);
					uc.init(_hInst, _hSelf);

					if (uc.doDialog() != -1)
					{
						theUserCmds.push_back(uc);

						std::vector<UserCommand> & userCommands = (NppParameters::getInstance())->getUserCommandList();
						HMENU hRunMenu = ::GetSubMenu(::GetMenu(_hParent), MENUINDEX_RUN);
						int const posBase = 0;
						int nbCmd = userCommands.size();
						if (nbCmd == 1)
							::InsertMenu(hRunMenu, posBase + 1, MF_BYPOSITION, (unsigned int)-1, 0);
						//char menuString[64]; 
						//sprintf(menuString, "%s%s%s", uc._name, "\t", uc.toString().c_str());
						::InsertMenu(hRunMenu, posBase + 1 + nbCmd, MF_BYPOSITION, ID_USER_CMD + nbCmd - 1, uc.toMenuItemString().c_str());

						Accelerator *pAccel = (NppParameters::getInstance())->getAccelerator();
						pAccel->uptdateShortcuts();
						::SendMessage(_hParent, NPPM_INTERNAL_USERCMDLIST_MODIFIED, 0, 0);
					}
					return TRUE;
				}
				case IDC_BUTTON_FILE_BROWSER :
				{
					FileDialog fd(_hSelf, _hInst);
					fd.setExtFilter("Executable file : ", ".exe", ".com", ".cmd", ".bat", NULL);
					fd.setExtFilter("All files : ", ".*", NULL);

					if (const char *fn = fd.doOpenSingleFileDlg())
						addTextToCombo(fn);
					return TRUE;
				}

				default :
					break;
			}
		}
	}
	return FALSE;	
}

void RunDlg::addTextToCombo(const char *txt2Add) const
{
	HWND handle = ::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH);
	int i = ::SendMessage(handle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2Add);
	if (i == CB_ERR)
		i = ::SendMessage(handle, CB_ADDSTRING, 0, (LPARAM)txt2Add);
	::SendMessage(handle, CB_SETCURSEL, i, 0);
}
void RunDlg::removeTextFromCombo(const char *txt2Remove) const
{
	HWND handle = ::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH);
	int i = ::SendMessage(handle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2Remove);
	if (i == CB_ERR)
		return;
	::SendMessage(handle, CB_DELETESTRING, i, 0);
}

void RunDlg::doDialog(bool isRTL)
{
	if (!isCreated())
		create(IDD_RUN_DLG, isRTL);

    // Adjust the position in the center
	goToCenter();
	::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH));
};
