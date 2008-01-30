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

#include "keys.h"
const int KEY_STR_LEN = 16;

struct KeyIDNAME {
	const char * name;
	unsigned char id;
};

KeyIDNAME namedKeyArray[] = {
{"None", VK_NULL},

{"Backspace", VK_BACK},
{"Tab", VK_TAB},
{"Enter", VK_RETURN},
{"Esc", VK_ESCAPE},
{"Spacebar", VK_SPACE},

{"Page up", VK_PRIOR},
{"Page down", VK_NEXT},
{"End", VK_END},
{"Home", VK_HOME},
{"Left", VK_LEFT},
{"Up", VK_UP},
{"Right", VK_RIGHT},
{"Down", VK_DOWN},

{"INS", VK_INSERT},
{"DEL", VK_DELETE},

{"0", VK_0},
{"1", VK_1},
{"2", VK_2},
{"3", VK_3},
{"4", VK_4},
{"5", VK_5},
{"6", VK_6},
{"7", VK_7},
{"8", VK_8},
{"9", VK_9},
{"A", VK_A},
{"B", VK_B},
{"C", VK_C},
{"D", VK_D},
{"E", VK_E},
{"F", VK_F},
{"G", VK_G},
{"H", VK_H},
{"I", VK_I},
{"J", VK_J},
{"K", VK_K},
{"L", VK_L},
{"M", VK_M},
{"N", VK_N},
{"O", VK_O},
{"P", VK_P},
{"Q", VK_Q},
{"R", VK_R},
{"S", VK_S},
{"T", VK_T},
{"U", VK_U},
{"V", VK_V},
{"W", VK_W},
{"X", VK_X},
{"Y", VK_Y},
{"Z", VK_Z},

{"Numpad 0", VK_NUMPAD0},
{"Numpad 1", VK_NUMPAD1},
{"Numpad 2", VK_NUMPAD2},
{"Numpad 3", VK_NUMPAD3},
{"Numpad 4", VK_NUMPAD4},
{"Numpad 5", VK_NUMPAD5},
{"Numpad 6", VK_NUMPAD6},
{"Numpad 7", VK_NUMPAD7},
{"Numpad 8", VK_NUMPAD8},
{"Numpad 9", VK_NUMPAD9},
{"Num *", VK_MULTIPLY},
{"Num +", VK_ADD},
//{"Num Enter", VK_SEPARATOR},	//this one doesnt seem to work
{"Num -", VK_SUBTRACT},
{"Num .", VK_DECIMAL},
{"Num /", VK_DIVIDE},
{"F1", VK_F1},
{"F2", VK_F2},
{"F3", VK_F3},
{"F4", VK_F4},
{"F5", VK_F5},
{"F6", VK_F6},
{"F7", VK_F7},
{"F8", VK_F8},
{"F9", VK_F9},
{"F10", VK_F10},
{"F11", VK_F11},
{"F12", VK_F12},

{"~", VK_OEM_3},
{"-", VK_OEM_MINUS},
{"=", VK_OEM_PLUS},
{"[", VK_OEM_4},
{"]", VK_OEM_6},
{";", VK_OEM_1},
{"'", VK_OEM_7},
{"\\", VK_OEM_5},
{",", VK_OEM_COMMA},
{".", VK_OEM_PERIOD},
{"/", VK_OEM_2},

{"<>", VK_OEM_102},
};

#define nrKeys sizeof(namedKeyArray)/sizeof(KeyIDNAME)

/*
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
*/

string Shortcut::toString() const
{
	string sc = "";
	if (!isEnabled())
		return sc;

	if (_isCtrl)
		sc += "Ctrl+";
	if (_isAlt)
		sc += "Alt+";
	if (_isShift)
		sc += "Shift+";

	string key;
	getKeyStrFromVal(_key, key);
	sc += key;
	return sc;
}

void getKeyStrFromVal(unsigned char keyVal, string & str)
{
	str = "";
	bool found = false;
	int i;
	for (i = 0; i < nrKeys; i++) {
		if (keyVal == namedKeyArray[i].id) {
			found = true;
			break;
		}
	}
	if (found)
		str = namedKeyArray[i].name;
	else 
		str = "Unlisted";
}

void getNameStrFromCmd(DWORD cmd, string & str)
{
	if ((cmd >= ID_MACRO) && (cmd < ID_MACRO_LIMIT))
	{
		vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();
		int i = cmd - ID_MACRO;
		str = theMacros[i]._name;
	}
	else if ((cmd >= ID_USER_CMD) && (cmd < ID_USER_CMD_LIMIT))
	{
		vector<UserCommand> & userCommands = (NppParameters::getInstance())->getUserCommandList();
		int i = cmd - ID_USER_CMD;
		str = userCommands[i]._name;
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
	}
	else
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus::getClassName(), NULL);
		char cmdName[64];
		int nbChar = ::GetMenuString(::GetMenu(hNotepad_plus), cmd, cmdName, sizeof(cmdName), MF_BYCOMMAND);
		if (!nbChar)
			return;
		bool fin = false;
		int j = 0;
		size_t len = strlen(cmdName);
		for (size_t i = 0 ; i < len; i++)
		{
			switch(cmdName[i])
			{
				case '\t':
					cmdName[j] = '\0';
					fin = true;
					break;

				case '&':
					break;

				default :
					cmdName[j++] = cmdName[i];
			}
			if (fin)
				break;
		}
		cmdName[j] = '\0';
		str = cmdName;
	}
	return;
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
			int textlen = (int)::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);

			::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK, BM_SETCHECK, _isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK, BM_SETCHECK, _isAlt?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK, BM_SETCHECK, _isShift?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
			int iFound = -1;
			for (size_t i = 0 ; i < nrKeys ; i++)
			{
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, (LPARAM)namedKeyArray[i].name);

				if (_key == namedKeyArray[i].id)
					iFound = i;
			}

			if (iFound != -1)
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_SETCURSEL, iFound, 0);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isEnabled()?SW_HIDE:SW_SHOW);

			goToCenter();
			return TRUE;
		}

		case WM_COMMAND : 
		{
			int textlen = (int)::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);
			switch (wParam)
			{
				case IDC_CTRL_CHECK :
					_isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					return TRUE;

				case IDC_ALT_CHECK :
					_isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					return TRUE;

				case IDC_SHIFT_CHECK :
					_isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					return TRUE;

				case IDOK :
					if (!isEnabled()) {
						_isCtrl = _isAlt = _isShift = false;
					}
					if (_canModifyName)
						::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXT, nameLenMax, (LPARAM)_name);
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
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
							return TRUE;
						}
					}
					else if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						if (LOWORD(wParam) == IDC_KEY_COMBO)
						{
							int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
							_key = namedKeyArray[i].id;
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
							::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isEnabled()?SW_HIDE:SW_SHOW);
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
bool Accelerator::updateShortcuts(/*HWND nppHandle*/) 
{
	bool isCmdScModified = false;
	NppParameters *pNppParam = NppParameters::getInstance();

	vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
	vector<MacroShortcut> & macros  = pNppParam->getMacroList();
	vector<UserCommand> & userCommands = pNppParam->getUserCommandList();
	vector<PluginCmdShortcut> & pluginCommands = pNppParam->getPluginCommandList();

	size_t nbMenu = shortcuts.size();
	size_t nbMacro = macros.size();
	size_t nbUserCmd = userCommands.size();
	size_t nbPluginCmd = pluginCommands.size();

	if (_pAccelArray)
		delete [] _pAccelArray;

	_pAccelArray = new ACCEL[nbMenu+nbMacro+nbUserCmd+nbPluginCmd];
	int offset = 0;
	size_t i = 0;
	//no validation performed, it might be that invalid shortcuts are being used by default. Allows user to 'hack', might be a good thing
	for(i = 0; i < nbMenu; i++) {
		if (shortcuts[i].isEnabled()) {// && shortcuts[i].isValid()) {
			_pAccelArray[offset].cmd = (WORD)(shortcuts[i].getID());
			_pAccelArray[offset].fVirt = FVIRTKEY | (shortcuts[i]._isCtrl?FCONTROL:0) | (shortcuts[i]._isAlt?FALT:0) | (shortcuts[i]._isShift?FSHIFT:0);
			_pAccelArray[offset].key = shortcuts[i]._key;
			offset++;
		}
	}

	for(i = 0; i < nbMacro; i++) {
		if (macros[i].isEnabled()) {// && macros[i].isValid()) {
			_pAccelArray[offset].cmd = (WORD)(macros[i].getID());
			_pAccelArray[offset].fVirt = FVIRTKEY | (macros[i]._isCtrl?FCONTROL:0) | (macros[i]._isAlt?FALT:0) | (macros[i]._isShift?FSHIFT:0);
			_pAccelArray[offset].key = macros[i]._key;
			offset++;
		}
	}

	for(i = 0; i < nbUserCmd; i++) {
		if (userCommands[i].isEnabled()) {// && userCommands[i].isValid()) {
			_pAccelArray[offset].cmd = (WORD)(userCommands[i].getID());
			_pAccelArray[offset].fVirt = FVIRTKEY | (userCommands[i]._isCtrl?FCONTROL:0) | (userCommands[i]._isAlt?FALT:0) | (userCommands[i]._isShift?FSHIFT:0);
			_pAccelArray[offset].key = userCommands[i]._key;
			offset++;
		}
	}

	for(i = 0; i < nbPluginCmd; i++) {
		if (pluginCommands[i].isEnabled()) {// && pluginCommands[i].isValid()) {
			_pAccelArray[offset].cmd = (WORD)(pluginCommands[i].getID());
			_pAccelArray[offset].fVirt = FVIRTKEY | (pluginCommands[i]._isCtrl?FCONTROL:0) | (pluginCommands[i]._isAlt?FALT:0) | (pluginCommands[i]._isShift?FSHIFT:0);
			_pAccelArray[offset].key = pluginCommands[i]._key;
			offset++;
		}
	}

	_nbAccelItems = offset;

	updateFullMenu();
	reNew();	//update the table
	return isCmdScModified;
}

void Accelerator::updateFullMenu() {
	NppParameters * pNppParam = NppParameters::getInstance();
	vector<CommandShortcut> commands = pNppParam->getUserShortcuts();
	for(size_t i = 0; i < commands.size(); i++) {
		updateMenuItemByCommand(commands[i]);
	}

	vector<MacroShortcut> mcommands = pNppParam->getMacroList();
	for(size_t i = 0; i < mcommands.size(); i++) {
		updateMenuItemByCommand(mcommands[i]);
	}

	vector<UserCommand> ucommands = pNppParam->getUserCommandList();
	for(size_t i = 0; i < ucommands.size(); i++) {
		updateMenuItemByCommand(ucommands[i]);
	}

	vector<PluginCmdShortcut> pcommands = pNppParam->getPluginCommandList();
	for(size_t i = 0; i < pcommands.size(); i++) {
		updateMenuItemByCommand(pcommands[i]);
	}

	::DrawMenuBar(_hMenuParent);
}

void Accelerator::updateMenuItemByCommand(CommandShortcut csc) {
	int cmdID = (int)csc.getID();
	::ModifyMenu(_hAccelMenu, cmdID, MF_BYCOMMAND, cmdID, csc.toMenuItemString().c_str());
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
		if ( (message == SCI_SETTEXT)
			|| (message == SCI_REPLACESEL) 
			|| (message == SCI_ADDTEXT) 
			|| (message == SCI_ADDSTYLEDTEXT) 
			|| (message == SCI_INSERTTEXT) 
			|| (message == SCI_APPENDTEXT) ) {
			SCNotification scnN;
			scnN.nmhdr.code = SCN_CHARADDED;
			scnN.nmhdr.hwndFrom = pEditView->getHSelf();
			scnN.nmhdr.idFrom = 0;
			scnN.ch = sParameter.at(0);
			::SendMessage(pNotepad->getHSelf(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scnN));
		}
	}
}

void ScintillaAccelerator::init(vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent) {
	_hAccelMenu = hMenu;
	_hMenuParent = menuParent;
	size_t nr = vScintillas->size();
	for(size_t i = 0; i < nr; i++) {
		_vScintillas.push_back(vScintillas->at(i));
	}
	_nrScintillas = (int)nr;
}

void ScintillaAccelerator::updateKeys() {
	NppParameters *pNppParam = NppParameters::getInstance();
	vector<ScintillaKeyMap> & map = pNppParam->getScintillaKeyList();
	size_t mapSize = map.size();

	for(int i = 0; i < _nrScintillas; i++) {
		::SendMessage(_vScintillas[i], SCI_CLEARALLCMDKEYS, 0, 0);
		for(size_t j = mapSize - 1; j >= 0; j--) {	//reverse order, top of the list has highest priority
			ScintillaKeyMap skm = map[j];
			if (skm.isEnabled()) {		//no validating, scintilla accepts more keys
				::SendMessage(_vScintillas[i], SCI_ASSIGNCMDKEY, skm.toKeyDef(), skm.getScintillaKeyID());
			}
			if (skm.getMenuCmdID() != 0) {
				updateMenuItemByID(skm, skm.getMenuCmdID());
			}
			if (j == 0)	//j is unsigned, so default method doesnt work
				break;
		}
	}
}

void ScintillaAccelerator::updateKey(ScintillaKeyMap skmOld, ScintillaKeyMap skmNew) {
	updateKeys();	//do a full update, double mappings can make this work badly
	return;
	for(int i = 0; i < _nrScintillas; i++) {
		::SendMessage(_vScintillas[i], SCI_CLEARCMDKEY, skmOld.toKeyDef(), 0);
		::SendMessage(_vScintillas[i], SCI_ASSIGNCMDKEY, skmNew.toKeyDef(), skmNew.getScintillaKeyID());
	}
}

void ScintillaAccelerator::updateMenuItemByID(ScintillaKeyMap skm, int id) {
	NppParameters *pNppParam = NppParameters::getInstance();
	char cmdName[64];
	::GetMenuString(_hAccelMenu, id, cmdName, sizeof(cmdName), MF_BYCOMMAND);
	int i = 0;
	while(cmdName[i] != 0) {
		if (cmdName[i] == '\t') {
			cmdName[i] = 0;
			break;
		}
		i++;
	}
	string menuItem = cmdName;
	if (skm.isEnabled()) {
		menuItem += "\t";
		//menuItem += "Sc:";	//sc: scintilla shortcut
		menuItem += skm.toString();
	}
	::ModifyMenu(_hAccelMenu, id, MF_BYCOMMAND, id, menuItem.c_str());
	::DrawMenuBar(_hMenuParent);
}
