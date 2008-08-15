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


void Command::extractArgs(wchar_t *cmd2Exec, wchar_t *args, const wchar_t *cmdEntier)
{
	int i = 0;
	bool quoted = false;
	for ( ; i < int(wcslen(cmdEntier)) ; i++)
	{
		if ((cmdEntier[i] == ' ') && (!quoted))
			break;
		if (cmdEntier[i]=='"')
			quoted = !quoted;

		cmd2Exec[i] = cmdEntier[i];
	}
	cmd2Exec[i] = '\0';
	
	if (i < int(wcslen(cmdEntier)))
	{
		for ( ; (i < int(wcslen(cmdEntier))) && (cmdEntier[i] == ' ') ; i++);
		if (i < int(wcslen(cmdEntier)))
		{
			for (int k = 0 ; i <= int(wcslen(cmdEntier)) ; i++, k++)
			{
				args[k] = cmdEntier[i];
			}
		}

		int l = wcslen(args);
		if (args[l-1] == ' ')
		{
			for (l -= 2 ; (l > 0) && (args[l] == ' ') ; l--);
			args[l+1] = '\0';
		}

	}
	else
		args[0] = '\0';
}


int whichVar(wchar_t *str)
{
	if (!wcscmp(fullCurrentPath, str))
		return FULL_CURRENT_PATH;
	else if (!wcscmp(currentDirectory, str))
		return CURRENT_DIRECTORY;
	else if (!wcscmp(onlyFileName, str))
		return FILE_NAME;
	else if (!wcscmp(fileNamePart, str))
		return NAME_PART;
	else if (!wcscmp(fileExtPart, str))
		return EXT_PART;
	else if (!wcscmp(currentWord, str))
		return CURRENT_WORD;
	else if (!wcscmp(nppDir, str))
		return NPP_DIRECTORY;
	else if (!wcscmp(currentLine, str))
		return CURRENT_LINE;
	else if (!wcscmp(currentColumn, str))
		return CURRENT_COLUMN;

	return VAR_NOT_RECOGNIZED;
}

// Since I'm sure the length will be 256, I won't check the strlen : watch out!
void expandNppEnvironmentStrs(const wchar_t *strSrc, wchar_t *stringDest, size_t strDestLen, HWND hWnd)
{
	size_t j = 0;
	for (size_t i = 0  ; i < wcslen(strSrc) ; i++)
	{
		int iBegin = -1;
		int iEnd = -1;
		if ((strSrc[i] == '$') && (strSrc[i+1] == '('))
		{
			iBegin = i += 2;
			for ( ; i < wcslen(strSrc) ; i++)
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
				wchar_t str[256];
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
					wchar_t expandedStr[256];
					if (internalVar == CURRENT_LINE || internalVar == CURRENT_COLUMN)
					{
						int lineNumber = ::SendMessage(hWnd, RUNCOMMAND_USER + internalVar, 0, 0);
						wsprintfW(expandedStr, L"%d", lineNumber);
					}
					else
						::SendMessage(hWnd, RUNCOMMAND_USER + internalVar, MAX_PATH, (LPARAM)expandedStr);

					for (size_t p = 0 ; p < wcslen(expandedStr) ; p++)
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
	wchar_t cmdPure[MAX_PATH];
	wchar_t cmdIntermediate[MAX_PATH];
	wchar_t cmd2Exec[MAX_PATH];
	wchar_t args[MAX_PATH];
	wchar_t argsIntermediate[MAX_PATH];
	wchar_t args2Exec[MAX_PATH];

	wstring cmdLineW = string2wstring(_cmdLine);
	extractArgs(cmdPure, args, cmdLineW.c_str());
	::ExpandEnvironmentStringsW(cmdPure, cmdIntermediate, sizeof(cmd2Exec));
	::ExpandEnvironmentStringsW(args, argsIntermediate, sizeof(args));
	expandNppEnvironmentStrs(cmdIntermediate, cmd2Exec, sizeof(cmd2Exec), hWnd);
	expandNppEnvironmentStrs(argsIntermediate, args2Exec, sizeof(args2Exec), hWnd);

	// cmd2Exec needs to be in UTF8 format for searches to work.
	char temp[MAX_PATH];
	wchar2char(cmd2Exec, temp);
	char2wchar(temp, cmd2Exec);

	return ::ShellExecuteW(hWnd, L"open", cmd2Exec, args2Exec, L".", SW_SHOW);
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
					wchar_t cmdW[MAX_PATH];
					::GetDlgItemTextW(_hSelf, IDC_COMBO_RUN_PATH, cmdW, MAX_PATH);
					wchar2char(cmdW, cmd);
					_cmdLine = cmd;
					wstring cmdLineW = string2wstring(_cmdLine);

					HINSTANCE hInst = run(_hParent);
					if (int(hInst) > 32)
					{
						addTextToCombo(cmdLineW.c_str());
						display(false);
					}
					else
					{
						removeTextFromCombo(cmdLineW.c_str());
					}
					return TRUE;
				}
				case IDC_BUTTON_SAVE :
				{
					std::vector<UserCommand> & theUserCmds = (NppParameters::getInstance())->getUserCommandList();

					int nbCmd = theUserCmds.size();

					int cmdID = ID_USER_CMD + nbCmd;
					char cmd[MAX_PATH];
					wchar_t cmdW[MAX_PATH];
					::GetDlgItemTextW(_hSelf, IDC_COMBO_RUN_PATH, cmdW, MAX_PATH);
					wchar2char(cmdW, cmd);
					UserCommand uc(Shortcut(), cmd, cmdID);
					uc.init(_hInst, _hSelf);

					if (uc.doDialog() != -1)
					{
						HMENU hRunMenu = ::GetSubMenu((HMENU)::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_RUN);
						int const posBase = 2;
						
						if (nbCmd == 0)
							::InsertMenu(hRunMenu, posBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);
						
						theUserCmds.push_back(uc);
						::InsertMenu(hRunMenu, posBase + nbCmd, MF_BYPOSITION, cmdID, uc.toMenuItemString().c_str());
						(NppParameters::getInstance())->getAccelerator()->updateShortcuts();
					}
					return TRUE;
				}
				case IDC_BUTTON_FILE_BROWSER :
				{
					FileDialog fd(_hSelf, _hInst);
					fd.setExtFilter("Executable file : ", ".exe", ".com", ".cmd", ".bat", NULL);
					fd.setExtFilter("All files : ", ".*", NULL);

					if (const char *fn = fd.doOpenSingleFileDlg())
					{
						wchar_t fnW[MAX_PATH];
						char2wchar(fn, fnW);
						addTextToCombo(fnW);
					}
					return TRUE;
				}

				default :
					break;
			}
		}
	}
	return FALSE;	
}

void RunDlg::addTextToCombo(const wchar_t *txt2Add) const
{
	HWND handle = ::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH);
	int i = ::SendMessageW(handle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2Add);
	if (i == CB_ERR)
		i = ::SendMessageW(handle, CB_ADDSTRING, 0, (LPARAM)txt2Add);
	::SendMessage(handle, CB_SETCURSEL, i, 0);
}
void RunDlg::removeTextFromCombo(const wchar_t *txt2Remove) const
{
	HWND handle = ::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH);
	int i = ::SendMessageW(handle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2Remove);
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
