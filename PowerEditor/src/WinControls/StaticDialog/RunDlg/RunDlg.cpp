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

#include "StaticDialog.h"
#include "RunDlg.h"
#include "CustomFileDialog.h"
#include "Notepad_plus_msgs.h"
#include "shortcut.h"
#include "Parameters.h"
#include "Notepad_plus.h"
#include <strsafe.h>

using namespace std;

void Command::extractArgs(wchar_t* cmd2Exec, size_t cmd2ExecLen, wchar_t* args, size_t argsLen, const wchar_t* cmdEntier)
{
	size_t i = 0;
	bool quoted = false;

	size_t cmdEntierLen = lstrlen(cmdEntier);

	size_t shortest = std::min<size_t>(cmd2ExecLen, argsLen);

	if (cmdEntierLen > shortest)
		cmdEntierLen = shortest - 1;

	for (; i < cmdEntierLen; ++i)
	{
		if (cmdEntier[i] == ' ' && !quoted)
			break;

		if (cmdEntier[i]=='"')
			quoted = !quoted;

		cmd2Exec[i] = cmdEntier[i];
	}
	cmd2Exec[i] = '\0';
	
	if (i < cmdEntierLen)
	{
		for (size_t len = cmdEntierLen; (i < len) && (cmdEntier[i] == ' ') ; ++i);

		if (i < cmdEntierLen)
		{
			for (size_t k = 0, len2 = cmdEntierLen; i <= len2; ++i, ++k)
			{
				args[k] = cmdEntier[i];
			}
		}

		int l = lstrlen(args);
		if (args[l-1] == ' ')
		{
			for (l -= 2 ; (l > 0) && (args[l] == ' ') ; l--);
			args[l+1] = '\0';
		}
	}
	else
	{
		args[0] = '\0';
	}
}


int whichVar(wchar_t *str)
{
	if (!lstrcmp(fullCurrentPath, str))
		return FULL_CURRENT_PATH;
	else if (!lstrcmp(currentDirectory, str))
		return CURRENT_DIRECTORY;
	else if (!lstrcmp(onlyFileName, str))
		return FILE_NAME;
	else if (!lstrcmp(fileNamePart, str))
		return NAME_PART;
	else if (!lstrcmp(fileExtPart, str))
		return EXT_PART;
	else if (!lstrcmp(currentWord, str))
		return CURRENT_WORD;
	else if (!lstrcmp(nppDir, str))
		return NPP_DIRECTORY;
	else if (!lstrcmp(nppFullFilePath, str))
		return NPP_FULL_FILE_PATH;
	else if (!lstrcmp(currentLine, str))
		return CURRENT_LINE;
	else if (!lstrcmp(currentColumn, str))
		return CURRENT_COLUMN;
	else if (!lstrcmp(currentLineStr, str))
		return CURRENT_LINESTR;

	return VAR_NOT_RECOGNIZED;
}

// Since I'm sure the length will be 256, I won't check the lstrlen : watch out!
void expandNppEnvironmentStrs(const wchar_t *strSrc, wchar_t *stringDest, size_t strDestLen, HWND hWnd)
{
	size_t j = 0;
	for (int i = 0, len = lstrlen(strSrc); i < len; ++i)
	{
		int iBegin = -1;
		int iEnd = -1;
		if ((strSrc[i] == '$') && (strSrc[i+1] == '('))
		{
			iBegin = i += 2;
			for (size_t len2 = size_t(lstrlen(strSrc)); size_t(i) < len2 ; ++i)
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
				wchar_t str[MAX_PATH] = { '\0' };
				int m = 0;
				for (int k = iBegin  ; k <= iEnd ; ++k)
					str[m++] = strSrc[k];
				str[m] = '\0';

				int internalVar = whichVar(str);
				if (internalVar == VAR_NOT_RECOGNIZED)
				{
					i = iBegin - 2;
					if (j < (strDestLen-1))
						stringDest[j++] = strSrc[i];
					else
						break;
				}
				else
				{
					wchar_t expandedStr[CURRENTWORD_MAXLENGTH] = { '\0' };
					if (internalVar == CURRENT_LINE || internalVar == CURRENT_COLUMN)
					{
						size_t lineNumber = ::SendMessage(hWnd, RUNCOMMAND_USER + internalVar, 0, 0);
						std::wstring lineNumStr = std::to_wstring(lineNumber);
						StringCchCopyW(expandedStr, CURRENTWORD_MAXLENGTH, lineNumStr.c_str());
					}
					else
						::SendMessage(hWnd, RUNCOMMAND_USER + internalVar, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(expandedStr));

					for (size_t p = 0, len3 = size_t(lstrlen(expandedStr)); p < len3; ++p)
					{
						if (j < (strDestLen-1))
							stringDest[j++] = expandedStr[p];
						else
							break;
					}
				}
			}
			else
			{
				i = iBegin - 2;
				if (j < (strDestLen-1))
					stringDest[j++] = strSrc[i];
				else
					break;
			}
		}
		else
			if (j < (strDestLen-1))
				stringDest[j++] = strSrc[i];
			else
				break;
	}
	stringDest[j] = '\0';
}

HINSTANCE Command::run(HWND hWnd)
{
	return run(hWnd, L".");
}

HINSTANCE Command::run(HWND hWnd, const wchar_t* cwd)
{
	constexpr int argsIntermediateLen = MAX_PATH * 2;
	constexpr int args2ExecLen = CURRENTWORD_MAXLENGTH + MAX_PATH * 2;

	wchar_t cmdPure[MAX_PATH]{};
	wchar_t cmdIntermediate[MAX_PATH]{};
	wchar_t cmd2Exec[MAX_PATH]{};
	wchar_t args[MAX_PATH]{};
	wchar_t argsIntermediate[argsIntermediateLen]{};
	wchar_t args2Exec[args2ExecLen]{};

	extractArgs(cmdPure, MAX_PATH, args, MAX_PATH, _cmdLine.c_str());
	int nbTchar = ::ExpandEnvironmentStrings(cmdPure, cmdIntermediate, MAX_PATH);
	if (!nbTchar)
		wcscpy_s(cmdIntermediate, cmdPure);
	else if (nbTchar >= MAX_PATH)
		cmdIntermediate[MAX_PATH-1] = '\0';

	nbTchar = ::ExpandEnvironmentStrings(args, argsIntermediate, argsIntermediateLen);
	if (!nbTchar)
		wcscpy_s(argsIntermediate, args);
	else if (nbTchar >= argsIntermediateLen)
		argsIntermediate[argsIntermediateLen-1] = '\0';

	expandNppEnvironmentStrs(cmdIntermediate, cmd2Exec, MAX_PATH, hWnd);
	expandNppEnvironmentStrs(argsIntermediate, args2Exec, args2ExecLen, hWnd);

	wchar_t cwd2Exec[MAX_PATH]{};
	expandNppEnvironmentStrs(cwd, cwd2Exec, MAX_PATH, hWnd);
	
	HINSTANCE res = ::ShellExecute(hWnd, L"open", cmd2Exec, args2Exec, cwd2Exec, SW_SHOW);

	// As per MSDN (https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx)
	// If the function succeeds, it returns a value greater than 32.
	// If the function fails, it returns an error value that indicates the cause of the failure.
	int retResult = static_cast<int>(reinterpret_cast<intptr_t>(res));
	if (retResult <= 32)
	{
		wstring errorMsg;
		errorMsg += GetLastErrorAsString(retResult);
		errorMsg += L"An attempt was made to execute the below command.";
		errorMsg += L"\n----------------------------------------------------------";
		errorMsg += L"\nCommand: ";
		errorMsg += cmd2Exec;
		errorMsg += L"\nArguments: ";
		errorMsg += args2Exec;
		errorMsg += L"\nError Code: ";
		errorMsg += intToString(retResult);
		errorMsg += L"\n----------------------------------------------------------";

		::MessageBox(hWnd, errorMsg.c_str(), L"ShellExecute - ERROR", MB_ICONINFORMATION | MB_APPLMODAL);
	}

	return res;
}

intptr_t CALLBACK RunDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			return TRUE;
		}

		case NPPM_INTERNAL_FINDKEYCONFLICTS:
		{
			return ::SendMessage(_hParent, message, wParam, lParam);
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColorListbox(wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_CHANGEUISTATE:
		{
			if (NppDarkMode::isEnabled())
			{
				redrawDlgItem(IDC_MAINTEXT_STATIC);
			}

			return FALSE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);
			getWindowRect(_rc);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
					display(false);
					return TRUE;
				
				case IDOK :
				{
					wchar_t cmd[MAX_PATH]{};
					::GetDlgItemText(_hSelf, IDC_COMBO_RUN_PATH, cmd, MAX_PATH);
					_cmdLine = cmd;

					HINSTANCE hInst = run(_hParent);
					if (reinterpret_cast<intptr_t>(hInst) > 32)
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
					NppParameters& nppParams = NppParameters::getInstance();
					std::vector<UserCommand> & theUserCmds = nppParams.getUserCommandList();

					int nbCmd = static_cast<int32_t>(theUserCmds.size());
					int cmdID = ID_USER_CMD + nbCmd;

					DynamicMenu& runMenu = nppParams.getRunMenuItems();
					int nbTopLevelItem = runMenu.getTopLevelItemNumber();

					wchar_t cmd[MAX_PATH]{};
					::GetDlgItemText(_hSelf, IDC_COMBO_RUN_PATH, cmd, MAX_PATH);
					UserCommand uc(Shortcut(), wstring2string(cmd, CP_UTF8).c_str(), cmdID);
					uc.init(_hInst, _hSelf);

					if (uc.doDialog() != -1)
					{
						HMENU mainMenu = reinterpret_cast<HMENU>(::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0));
						HMENU hRunMenu = ::GetSubMenu(mainMenu, MENUINDEX_RUN);
						int const posBase = runMenu.getPosBase();
						
						if (nbTopLevelItem == 0)
							::InsertMenu(hRunMenu, posBase - 1, MF_BYPOSITION, static_cast<unsigned int>(-1), 0);
						
						theUserCmds.push_back(uc);
						runMenu.push_back(MenuItemUnit(cmdID, string2wstring(uc.getName(), CP_UTF8)));
						::InsertMenu(hRunMenu, posBase + nbTopLevelItem, MF_BYPOSITION, cmdID, string2wstring(uc.toMenuItemString(), CP_UTF8).c_str());

                        if (nbTopLevelItem == 0)
                        {
                            // Insert the separator and modify/delete command
							::InsertMenu(hRunMenu, posBase + nbTopLevelItem + 1, MF_BYPOSITION, static_cast<unsigned int>(-1), 0);
							NativeLangSpeaker *pNativeLangSpeaker = nppParams.getNativeLangSpeaker();
							wstring nativeLangShortcutMapperMacro = pNativeLangSpeaker->getNativeLangMenuString(IDM_SETTING_SHORTCUT_MAPPER_MACRO);
							if (nativeLangShortcutMapperMacro == L"")
								nativeLangShortcutMapperMacro = runMenu.getLastCmdLabel();

							::InsertMenu(hRunMenu, posBase + nbTopLevelItem + 2, MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_RUN, nativeLangShortcutMapperMacro.c_str());
                        }
						nppParams.getAccelerator()->updateShortcuts();
						nppParams.setShortcutDirty();
					}
					return TRUE;
				}

				case IDC_BUTTON_FILE_BROWSER:
				{
					CustomFileDialog fd(_hSelf);
					fd.setExtFilter(L"Executable File", { L".exe", L".com", L".cmd", L".bat" });
					fd.setExtFilter(L"All Files", L".*");

					wstring fn = fd.doOpenSingleFileDlg();
					if (!fn.empty())
					{
						if (fn.find(' ') != wstring::npos)
						{
							wstring fn_quotes(fn);
							fn_quotes = L"\"" + fn_quotes + L"\"";
							addTextToCombo(fn_quotes.c_str());
						}
						else
						{
							addTextToCombo(fn.c_str());
						}
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
	auto i = ::SendMessage(handle, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(txt2Add));
	if (i == CB_ERR)
		i = ::SendMessage(handle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(txt2Add));
	::SendMessage(handle, CB_SETCURSEL, i, 0);
}
void RunDlg::removeTextFromCombo(const wchar_t *txt2Remove) const
{
	HWND handle = ::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH);
	auto i = ::SendMessage(handle, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(txt2Remove));
	if (i == CB_ERR)
		return;
	::SendMessage(handle, CB_DELETESTRING, i, 0);
}

void RunDlg::doDialog(bool isRTL)
{
	if (!isCreated())
		create(IDD_RUN_DLG, isRTL);

	// Adjust the position in the center
	moveForDpiChange();
	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
	::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_RUN_PATH));
}
