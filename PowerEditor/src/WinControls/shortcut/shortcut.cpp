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

#include "shortcut.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "resource.h"
#include "Notepad_plus.h"

//const int NB_VKEY = 77;
const int KEY_STR_LEN = 16;

char vKeyArray[][KEY_STR_LEN] = \
{"", "BACKSPACE", "TAB", "ENTER", "PAUSE", "CAPS LOCK", "ESC", "SPACEBAR", "PAGE UP", "PAGE DOWN",\
"END", "HOME", "LEFT ARROW", "UP ARROW", "RIGHT ARROW", "DOWN ARROW", "INS", "DEL",\
"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",\
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",\
"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",\
"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4",\
"NUMPAD5", "NUMPAD6", "NUMPAD7", "NUMPAD8", "NUMPAD9",\
"F1", "F2", "F3", "F4", "F5", "F6",\
"F7", "F8", "F9", "F10", "F11", "F12"};

unsigned char vkeyValue[] = {\
0x00, 0x08, 0x09, 0x0D, 0x13, 0x14, 0x1B, 0x20, 0x21, 0x22,\
0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E, 0x30, 0x31,\
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42,\
0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,\
0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,\
0x57, 0x58, 0x59, 0x5A, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,\
0x66, 0x67, 0x68, 0x69, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,\
0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B};

string Shortcut::toString() const
{
	string sc = _isCtrl?"Ctrl+":"";
	sc += _isAlt?"Alt+":"";
	sc += _isShift?"Shift+":"";
	string key;
	getKeyStrFromVal(_key, key);
	sc += key;
	return sc;
}

void getKeyStrFromVal(unsigned char keyVal, string & str)
{
	str = "";
	bool found = false;
	int i = 0;
	for (; i < sizeof(vkeyValue) ; i++)
	{
		if (keyVal == vkeyValue[i])
		{
			found = true;
			break;
		}
	}
	if (found)
		str = vKeyArray[i];
}

ShortcutType getNameStrFromCmd(DWORD cmd, string & str)
{
	ShortcutType st;

	if ((cmd >= ID_MACRO) && (cmd < ID_MACRO_LIMIT))
	{
		vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();
		int i = cmd - ID_MACRO;
		str = theMacros[i]._name;
		st = TYPE_MACRO;
	}
	else if ((cmd >= ID_USER_CMD) && (cmd < ID_USER_CMD_LIMIT))
	{
		vector<UserCommand> & userCommands = (NppParameters::getInstance())->getUserCommandList();
		int i = cmd - ID_USER_CMD;
		str = userCommands[i]._name;
		st = TYPE_USERCMD;
	}
	else if ((cmd >= ID_PLUGINS_CMD) && (cmd < ID_PLUGINS_CMD_LIMIT))
	{
		vector<PluginCmdShortcut> & pluginCmds = (NppParameters::getInstance())->getPluginCommandList();
		int i = 0;
		for (size_t j = 0 ; j < pluginCmds.size() ; j++)
		{
			if (pluginCmds[j].getID() == cmd)
			{
				i = j;
				break;
			}
		}
		str = pluginCmds[i]._name;
		//printStr(str.c_str());
		st = TYPE_PLUGINCMD;
	}
	/*else if ((cmd >= IDCMD) && (cmd < IDCMD_LIMIT))
	{
		
		vector<string> & cmds = (NppParameters::getInstance())->getNoMenuCmdNames();
		
		size_t idcmdIndex = cmd - IDCMD;
		if (idcmdIndex < cmds.size())
			str = cmds[idcmdIndex];
		
		st = TYPE_CMD;
	}*/
	else
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL);
		char cmdName[64];
		char filteredCmdName[64];
		int nbChar = ::GetMenuString(::GetMenu(hNotepad_plus), cmd, cmdName, sizeof(cmdName), MF_BYCOMMAND);
		if (!nbChar)
			return TYPE_INVALID;
		bool fin = false;
		int j = 0;
		for (size_t i = 0 ; i < strlen(cmdName) ; i++)
		{
			switch(cmdName[i])
			{
				case '\t':
					filteredCmdName[j] = '\0';
					fin = true;
					break;

				case '&':
					break;

				default :
					filteredCmdName[j++] = cmdName[i];
			}
			if (fin)
				break;
		}
		filteredCmdName[j] = '\0';
		str = filteredCmdName;
		st = TYPE_CMD;
	}
	return st;
}

BOOL CALLBACK Shortcut::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) 
{

	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemText(_hSelf, IDC_NAME_EDIT, _name);
			if (!_canModifyName)
				::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, EM_SETREADONLY, TRUE, 0);

			::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK, BM_SETCHECK, _isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK, BM_SETCHECK, _isAlt?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK, BM_SETCHECK, _isShift?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid());
			int iFound = -1;
			for (size_t i = 0 ; i < sizeof(vkeyValue) ; i++)
			{
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, (LPARAM)vKeyArray[i]);

				if (_key == vkeyValue[i])
					iFound = i;
			}

			if (iFound != -1)
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_SETCURSEL, iFound, 0);

			goToCenter();
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_CTRL_CHECK :
					_isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid());
					return TRUE;

				case IDC_ALT_CHECK :
					_isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid());
					return TRUE;

				case IDC_SHIFT_CHECK :
					_isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					return TRUE;

				case IDOK :
					::EndDialog(_hSelf, 0);
					return TRUE;

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
					return TRUE;

				default:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						if (LOWORD(wParam) == IDC_NAME_EDIT)
						{
							::SendDlgItemMessage(_hSelf, LOWORD(wParam), WM_GETTEXT, nameLenMax, (LPARAM)_name);
							return TRUE;
						}
					}
					else if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						if (LOWORD(wParam) == IDC_KEY_COMBO)
						{
							int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
							_key = vkeyValue[i];
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid());
							return TRUE;
						}
					}
					return FALSE;
			}
		}
		default :
			return FALSE;
	}

	return FALSE;
}

// return true if one of CommandShortcuts is deleted. Otherwise false.
bool Accelerator::uptdateShortcuts(HWND nppHandle) 
{
	bool isCmdScModified = false;
	NppParameters *pNppParam = NppParameters::getInstance();

	vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
	vector<MacroShortcut> & macros  = pNppParam->getMacroList();
	vector<UserCommand> & userCommands = pNppParam->getUserCommandList();
	vector<PluginCmdShortcut> & pluginCommands = pNppParam->getPluginCommandList();

	vector<ScintillaKeyMap> & scintillaScs = pNppParam->getScintillaKeyList();
	vector<ScintillaKeyMap> & scintillaModifScs = pNppParam->getScintillaModifiedKeys();

	size_t nbMacro = macros.size();
	size_t nbUserCmd = userCommands.size();
	size_t nbPluginCmd = pluginCommands.size();

	size_t totalShorcut = copyAccelArray(nbMacro, nbUserCmd, nbPluginCmd);

	int m = 0;
	size_t i = totalShorcut - nbMacro - nbUserCmd - nbPluginCmd;
	for (vector<MacroShortcut>::iterator ms = macros.begin() ; i < (totalShorcut - nbUserCmd - nbPluginCmd) ; i++, ms++)
	{
		_pAccelArray[i].cmd = ID_MACRO + m++;
		_pAccelArray[i].fVirt = FVIRTKEY | (ms->_isCtrl?FCONTROL:0) | (ms->_isAlt?FALT:0) | (ms->_isShift?FSHIFT:0);
		_pAccelArray[i].key = ms->_key;
	}

	m = 0;
	for (vector<UserCommand>::iterator uc = userCommands.begin() ; i < (totalShorcut - nbPluginCmd); i++, uc++)
	{
		_pAccelArray[i].cmd = ID_USER_CMD + m++;
		_pAccelArray[i].fVirt = FVIRTKEY | (uc->_isCtrl?FCONTROL:0) | (uc->_isAlt?FALT:0) | (uc->_isShift?FSHIFT:0);
		_pAccelArray[i].key = uc->_key;
	}

	for (vector<PluginCmdShortcut>::iterator pc = pluginCommands.begin() ; i < totalShorcut; i++, pc++)
	{
		_pAccelArray[i].cmd = (unsigned short)pc->getID();
		_pAccelArray[i].fVirt = FVIRTKEY | (pc->_isCtrl?FCONTROL:0) | (pc->_isAlt?FALT:0) | (pc->_isShift?FSHIFT:0);
		_pAccelArray[i].key = pc->_key;
	}

	// search all the command shortcuts modified by user from xml
	for (vector<CommandShortcut>::iterator csc = shortcuts.begin() ; csc != shortcuts.end() ; )
	{
		bool found = false;
		int nbCmdShortcut = totalShorcut - nbMacro - nbUserCmd - nbPluginCmd;
		for (int j = 0 ; j < nbCmdShortcut ; j++)
		{
			if (_pAccelArray[j].cmd == csc->getID())
			{
				_pAccelArray[j].fVirt = FVIRTKEY | (csc->_isCtrl?FCONTROL:0)\
					| (csc->_isAlt?FALT:0) | (csc->_isShift?FSHIFT:0);
				_pAccelArray[j].key = csc->_key;
				found = true;
				break;
			}
		}
		if (!found)
		{
			csc = shortcuts.erase(csc);
			isCmdScModified = true;
		}
		else
			csc++;
	}


	if (nppHandle)
	{
		for (vector<ScintillaKeyMap>::iterator skmm = scintillaModifScs.begin() ; skmm != scintillaModifScs.end() ; skmm++)
		{
			for (vector<ScintillaKeyMap>::iterator skm = scintillaScs.begin() ; skm != scintillaScs.end() ; skm++)
			{
				if (skmm->getScintillaKey() == skm->getScintillaKey())
				{
					// remap the key
					::SendMessage(nppHandle, NPPM_INTERNAL_BINDSCINTILLAKEY, skmm->toKeyDef(), skmm->getScintillaKey());
					::SendMessage(nppHandle, NPPM_INTERNAL_CLEARSCINTILLAKEY, skm->toKeyDef(), 0);

					// update the global ScintillaKeyList
					skm->copyShortcut(*skmm);

					// if this shortcut is linked to a menu item, change to shortcut string in menu item
					if (int cmdID = skm->getMenuCmdID())
					{
						HMENU hMenu = ::GetMenu(nppHandle);
						::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID, skm->toMenuItemString(cmdID).c_str());
					}
					break;
				}
			}
		}
	}
	reNew();
	return isCmdScModified;
}

recordedMacroStep::recordedMacroStep(int iMessage, long wParam, long lParam)
	: message(iMessage), wParameter(wParam), lParameter(lParam), MacroType(mtUseLParameter)
{ 
	if (lParameter) {
		switch (message) {
			case SCI_SETTEXT :
			case SCI_REPLACESEL :
			case SCI_REPLACETARGET :
			case SCI_REPLACETARGETRE :
			case SCI_SEARCHINTARGET :
			case SCI_ADDTEXT :
			case SCI_ADDSTYLEDTEXT :
			case SCI_INSERTTEXT :
			case SCI_APPENDTEXT :
			case SCI_SETWORDCHARS :
			case SCI_SETWHITESPACECHARS :
			case SCI_SETSTYLINGEX :
			case SCI_TEXTWIDTH :
			case SCI_STYLESETFONT :
			case SCI_SEARCHNEXT :
			case SCI_SEARCHPREV :
				sParameter = *reinterpret_cast<char *>(lParameter);
				MacroType = mtUseSParameter;
				lParameter = 0;
				break;
				
			default : // for all other messages, use lParameter "as is"
				break;
		}
	}
}

void recordedMacroStep::PlayBack(Window* pNotepad, ScintillaEditView *pEditView)
{
	if (MacroType == mtMenuCommand)
		::SendMessage(pNotepad->getHSelf(), WM_COMMAND, wParameter, 0);

	else
	{
		long lParam = lParameter;
		if (MacroType == mtUseSParameter)
			lParam = reinterpret_cast<long>(sParameter.c_str());
		pEditView->execute(message, wParameter, lParam);
	}
}
