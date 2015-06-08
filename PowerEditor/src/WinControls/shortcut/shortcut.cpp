// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "shortcut.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "resource.h"
#include "Notepad_plus_Window.h"
#include "keys.h"

using namespace std;

const int KEY_STR_LEN = 16;

struct KeyIDNAME {
	const TCHAR * name;
	UCHAR id;
};

KeyIDNAME namedKeyArray[] = {
{TEXT("None"), VK_NULL},

{TEXT("Backspace"), VK_BACK},
{TEXT("Tab"), VK_TAB},
{TEXT("Enter"), VK_RETURN},
{TEXT("Esc"), VK_ESCAPE},
{TEXT("Spacebar"), VK_SPACE},

{TEXT("Page up"), VK_PRIOR},
{TEXT("Page down"), VK_NEXT},
{TEXT("End"), VK_END},
{TEXT("Home"), VK_HOME},
{TEXT("Left"), VK_LEFT},
{TEXT("Up"), VK_UP},
{TEXT("Right"), VK_RIGHT},
{TEXT("Down"), VK_DOWN},

{TEXT("INS"), VK_INSERT},
{TEXT("DEL"), VK_DELETE},

{TEXT("0"), VK_0},
{TEXT("1"), VK_1},
{TEXT("2"), VK_2},
{TEXT("3"), VK_3},
{TEXT("4"), VK_4},
{TEXT("5"), VK_5},
{TEXT("6"), VK_6},
{TEXT("7"), VK_7},
{TEXT("8"), VK_8},
{TEXT("9"), VK_9},
{TEXT("A"), VK_A},
{TEXT("B"), VK_B},
{TEXT("C"), VK_C},
{TEXT("D"), VK_D},
{TEXT("E"), VK_E},
{TEXT("F"), VK_F},
{TEXT("G"), VK_G},
{TEXT("H"), VK_H},
{TEXT("I"), VK_I},
{TEXT("J"), VK_J},
{TEXT("K"), VK_K},
{TEXT("L"), VK_L},
{TEXT("M"), VK_M},
{TEXT("N"), VK_N},
{TEXT("O"), VK_O},
{TEXT("P"), VK_P},
{TEXT("Q"), VK_Q},
{TEXT("R"), VK_R},
{TEXT("S"), VK_S},
{TEXT("T"), VK_T},
{TEXT("U"), VK_U},
{TEXT("V"), VK_V},
{TEXT("W"), VK_W},
{TEXT("X"), VK_X},
{TEXT("Y"), VK_Y},
{TEXT("Z"), VK_Z},

{TEXT("Numpad 0"), VK_NUMPAD0},
{TEXT("Numpad 1"), VK_NUMPAD1},
{TEXT("Numpad 2"), VK_NUMPAD2},
{TEXT("Numpad 3"), VK_NUMPAD3},
{TEXT("Numpad 4"), VK_NUMPAD4},
{TEXT("Numpad 5"), VK_NUMPAD5},
{TEXT("Numpad 6"), VK_NUMPAD6},
{TEXT("Numpad 7"), VK_NUMPAD7},
{TEXT("Numpad 8"), VK_NUMPAD8},
{TEXT("Numpad 9"), VK_NUMPAD9},
{TEXT("Num *"), VK_MULTIPLY},
{TEXT("Num +"), VK_ADD},
//{TEXT("Num Enter"), VK_SEPARATOR},	//this one doesnt seem to work
{TEXT("Num -"), VK_SUBTRACT},
{TEXT("Num ."), VK_DECIMAL},
{TEXT("Num /"), VK_DIVIDE},
{TEXT("F1"), VK_F1},
{TEXT("F2"), VK_F2},
{TEXT("F3"), VK_F3},
{TEXT("F4"), VK_F4},
{TEXT("F5"), VK_F5},
{TEXT("F6"), VK_F6},
{TEXT("F7"), VK_F7},
{TEXT("F8"), VK_F8},
{TEXT("F9"), VK_F9},
{TEXT("F10"), VK_F10},
{TEXT("F11"), VK_F11},
{TEXT("F12"), VK_F12},

{TEXT("~"), VK_OEM_3},
{TEXT("-"), VK_OEM_MINUS},
{TEXT("="), VK_OEM_PLUS},
{TEXT("["), VK_OEM_4},
{TEXT("]"), VK_OEM_6},
{TEXT(";"), VK_OEM_1},
{TEXT("'"), VK_OEM_7},
{TEXT("\\"), VK_OEM_5},
{TEXT(","), VK_OEM_COMMA},
{TEXT("."), VK_OEM_PERIOD},
{TEXT("/"), VK_OEM_2},

{TEXT("<>"), VK_OEM_102},
};

#define nrKeys sizeof(namedKeyArray)/sizeof(KeyIDNAME)

/*
TCHAR vKeyArray[][KEY_STR_LEN] = \
{TEXT(""), TEXT("BACKSPACE"), TEXT("TAB"), TEXT("ENTER"), TEXT("PAUSE"), TEXT("CAPS LOCK"), TEXT("ESC"), TEXT("SPACEBAR"), TEXT("PAGE UP"), TEXT("PAGE DOWN"),\
"END", TEXT("HOME"), TEXT("LEFT ARROW"), TEXT("UP ARROW"), TEXT("RIGHT ARROW"), TEXT("DOWN ARROW"), TEXT("INS"), TEXT("DEL"),\
"0", TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"),\
"A", TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F"), TEXT("G"), TEXT("H"), TEXT("I"), TEXT("J"), TEXT("K"), TEXT("L"), TEXT("M"),\
"N", TEXT("O"), TEXT("P"), TEXT("Q"), TEXT("R"), TEXT("S"), TEXT("T"), TEXT("U"), TEXT("V"), TEXT("W"), TEXT("X"), TEXT("Y"), TEXT("Z"),\
"NUMPAD0", TEXT("NUMPAD1"), TEXT("NUMPAD2"), TEXT("NUMPAD3"), TEXT("NUMPAD4"),\
"NUMPAD5", TEXT("NUMPAD6"), TEXT("NUMPAD7"), TEXT("NUMPAD8"), TEXT("NUMPAD9"),\
"F1", TEXT("F2"), TEXT("F3"), TEXT("F4"), TEXT("F5"), TEXT("F6"),\
"F7", TEXT("F8"), TEXT("F9"), TEXT("F10"), TEXT("F11"), TEXT("F12")};

UCHAR vkeyValue[] = {\
0x00, 0x08, 0x09, 0x0D, 0x13, 0x14, 0x1B, 0x20, 0x21, 0x22,\
0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E, 0x30, 0x31,\
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42,\
0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,\
0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,\
0x57, 0x58, 0x59, 0x5A, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,\
0x66, 0x67, 0x68, 0x69, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,\
0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B};
*/

generic_string Shortcut::toString() const
{
	generic_string sc = TEXT("");
	if (!isEnabled())
		return sc;

	if (_keyCombo._isCtrl)
		sc += TEXT("Ctrl+");
	if (_keyCombo._isAlt)
		sc += TEXT("Alt+");
	if (_keyCombo._isShift)
		sc += TEXT("Shift+");

	generic_string keyString;
	getKeyStrFromVal(_keyCombo._key, keyString);
	sc += keyString;
	return sc;
}

void Shortcut::setName(const TCHAR * name) {
	lstrcpyn(_menuName, name, nameLenMax);
	lstrcpyn(_name, name, nameLenMax);
	int i = 0, j = 0;
	while(name[j] != 0 && i < nameLenMax) {
		if (name[j] != '&') {
			_name[i] = name[j];
			++i;
		} else {	//check if this ampersand is being escaped
			if (name[j+1] == '&') {	//escaped ampersand
				_name[i] = name[j];
				++i;
				++j;	//skip escaped ampersand
			}
		}
		++j;
	}
	_name[i] = 0;
}

generic_string ScintillaKeyMap::toString() const {
	return toString(0);
}

generic_string ScintillaKeyMap::toString(int index) const {
	generic_string sc = TEXT("");
	if (!isEnabled())
		return sc;

	KeyCombo kc = _keyCombos[index];
	if (kc._isCtrl)
		sc += TEXT("Ctrl+");
	if (kc._isAlt)
		sc += TEXT("Alt+");
	if (kc._isShift)
		sc += TEXT("Shift+");

	generic_string keyString;
	getKeyStrFromVal(kc._key, keyString);
	sc += keyString;
	return sc;
}

KeyCombo ScintillaKeyMap::getKeyComboByIndex(int index) const {
	return _keyCombos[index];
}

void ScintillaKeyMap::setKeyComboByIndex(int index, KeyCombo combo) {
	if(combo._key == 0 && (size > 1)) {	//remove the item if possible
		_keyCombos.erase(_keyCombos.begin() + index);
	}
	_keyCombos[index] = combo;
}

void ScintillaKeyMap::removeKeyComboByIndex(int index) {
	if (size > 1 && index > -1 && index < int(size)) {
		_keyCombos.erase(_keyCombos.begin() + index);
		size--;
	}
}

int ScintillaKeyMap::addKeyCombo(KeyCombo combo) {	//returns index where key is added, or -1 when invalid
	if (combo._key == 0)	//do not allow to add disabled keycombos
		return -1;
	if (!isEnabled()) {	//disabled, override current combo with new enabled one
		_keyCombos[0] = combo;
		return 0;
	}
	for(size_t i = 0; i < size; ++i) {	//if already in the list do not add it
		KeyCombo & kc = _keyCombos[i];
		if (combo._key == kc._key && combo._isCtrl == kc._isCtrl && combo._isAlt == kc._isAlt && combo._isShift == kc._isShift)
			return i;	//already in the list
	}
	_keyCombos.push_back(combo);
	++size;
	return (size - 1);
}

bool ScintillaKeyMap::isEnabled() const {
	return (_keyCombos[0]._key != 0);
}

size_t ScintillaKeyMap::getSize() const {
	return size;
}

void getKeyStrFromVal(UCHAR keyVal, generic_string & str)
{
	str = TEXT("");
	bool found = false;
	int i;
	for (i = 0; i < nrKeys; ++i) {
		if (keyVal == namedKeyArray[i].id) {
			found = true;
			break;
		}
	}
	if (found)
		str = namedKeyArray[i].name;
	else 
		str = TEXT("Unlisted");
}

void getNameStrFromCmd(DWORD cmd, generic_string & str)
{
	if ((cmd >= ID_MACRO) && (cmd < ID_MACRO_LIMIT))
	{
		vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();
		int i = cmd - ID_MACRO;
		str = theMacros[i].getName();
	}
	else if ((cmd >= ID_USER_CMD) && (cmd < ID_USER_CMD_LIMIT))
	{
		vector<UserCommand> & userCommands = (NppParameters::getInstance())->getUserCommandList();
		int i = cmd - ID_USER_CMD;
		str = userCommands[i].getName();
	}
	else if ((cmd >= ID_PLUGINS_CMD) && (cmd < ID_PLUGINS_CMD_LIMIT))
	{
		vector<PluginCmdShortcut> & pluginCmds = (NppParameters::getInstance())->getPluginCommandList();
		int i = 0;
		for (size_t j = 0, len = pluginCmds.size(); j < len ; ++j)
		{
			if (pluginCmds[j].getID() == cmd)
			{
				i = j;
				break;
			}
		}
		str = pluginCmds[i].getName();
	}
	else
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		const int commandSize = 64;
		TCHAR cmdName[commandSize];
		int nbChar = ::GetMenuString((HMENU)::SendMessage(hNotepad_plus, NPPM_INTERNAL_GETMENU, 0, 0), cmd, cmdName, commandSize, MF_BYCOMMAND);
		if (!nbChar)
			return;
		bool fin = false;
		int j = 0;
		size_t len = lstrlen(cmdName);
		for (size_t i = 0 ; i < len; ++i)
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

INT_PTR CALLBACK Shortcut::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemText(_hSelf, IDC_NAME_EDIT, getMenuName());	//display the menu name, with ampersands
			if (!_canModifyName)
				::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, EM_SETREADONLY, TRUE, 0);
			int textlen = (int)::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);

			::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK, BM_SETCHECK, _keyCombo._isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK, BM_SETCHECK, _keyCombo._isAlt?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK, BM_SETCHECK, _keyCombo._isShift?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
			int iFound = -1;
			for (size_t i = 0 ; i < nrKeys ; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, (LPARAM)namedKeyArray[i].name);

				if (_keyCombo._key == namedKeyArray[i].id)
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
					_keyCombo._isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					return TRUE;

				case IDC_ALT_CHECK :
					_keyCombo._isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					return TRUE;

				case IDC_SHIFT_CHECK :
					_keyCombo._isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					return TRUE;

				case IDOK :
					if (!isEnabled()) {
						_keyCombo._isCtrl = _keyCombo._isAlt = _keyCombo._isShift = false;
					}
					if (_canModifyName) {
						TCHAR editName[nameLenMax];
						::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXT, nameLenMax, (LPARAM)editName);
						setName(editName);
					}
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
							_keyCombo._key = namedKeyArray[i].id;
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
}

// return true if one of CommandShortcuts is deleted. Otherwise false.
void Accelerator::updateShortcuts() 
{
	vector<int> IFAccIds;
	IFAccIds.push_back(IDM_SEARCH_FINDNEXT);
	IFAccIds.push_back(IDM_SEARCH_FINDPREV);
	IFAccIds.push_back(IDM_SEARCH_FINDINCREMENT);

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
	vector<ACCEL> IFAcc;

	int offset = 0;
	size_t i = 0;
	//no validation performed, it might be that invalid shortcuts are being used by default. Allows user to 'hack', might be a good thing
	for(i = 0; i < nbMenu; ++i)
	{
		if (shortcuts[i].isEnabled())
		{
			_pAccelArray[offset].cmd = (WORD)(shortcuts[i].getID());
			_pAccelArray[offset].fVirt = shortcuts[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = shortcuts[i].getKeyCombo()._key;

			// Special extra handling for shortcuts shared by Incremental Find dialog
			if (std::find(IFAccIds.begin(), IFAccIds.end(), shortcuts[i].getID()) != IFAccIds.end())
				IFAcc.push_back(_pAccelArray[offset]);

			++offset;
		}
	}

	for(i = 0; i < nbMacro; ++i)
	{
		if (macros[i].isEnabled()) 
		{
			_pAccelArray[offset].cmd = (WORD)(macros[i].getID());
			_pAccelArray[offset].fVirt = macros[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = macros[i].getKeyCombo()._key;
			++offset;
		}
	}

	for(i = 0; i < nbUserCmd; ++i)
	{
		if (userCommands[i].isEnabled())
		{
			_pAccelArray[offset].cmd = (WORD)(userCommands[i].getID());
			_pAccelArray[offset].fVirt = userCommands[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = userCommands[i].getKeyCombo()._key;
			++offset;
		}
	}

	for(i = 0; i < nbPluginCmd; ++i)
	{
		if (pluginCommands[i].isEnabled())
		{
			_pAccelArray[offset].cmd = (WORD)(pluginCommands[i].getID());
			_pAccelArray[offset].fVirt = pluginCommands[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = pluginCommands[i].getKeyCombo()._key;
			++offset;
		}
	}

	_nbAccelItems = offset;

	updateFullMenu();
	
	//update the table
	if (_hAccTable)
		::DestroyAcceleratorTable(_hAccTable);
	_hAccTable = ::CreateAcceleratorTable(_pAccelArray, _nbAccelItems);

	if (_hIncFindAccTab)
		::DestroyAcceleratorTable(_hIncFindAccTab);

	size_t nb = IFAcc.size();
	ACCEL *tmpAccelArray = new ACCEL[nb];
	for (i = 0; i < nb; ++i)
	{
		tmpAccelArray[i] = IFAcc[i];
	}
	_hIncFindAccTab = ::CreateAcceleratorTable(tmpAccelArray, nb);
	delete [] tmpAccelArray;

	return;
}

void Accelerator::updateFullMenu() {
	NppParameters * pNppParam = NppParameters::getInstance();
	vector<CommandShortcut> commands = pNppParam->getUserShortcuts();
	for(size_t i = 0; i < commands.size(); ++i) {
		updateMenuItemByCommand(commands[i]);
	}

	vector<MacroShortcut> mcommands = pNppParam->getMacroList();
	for(size_t i = 0; i < mcommands.size(); ++i) {
		updateMenuItemByCommand(mcommands[i]);
	}

	vector<UserCommand> ucommands = pNppParam->getUserCommandList();
	for(size_t i = 0; i < ucommands.size(); ++i) {
		updateMenuItemByCommand(ucommands[i]);
	}

	vector<PluginCmdShortcut> pcommands = pNppParam->getPluginCommandList();
	for(size_t i = 0; i < pcommands.size(); ++i) {
		updateMenuItemByCommand(pcommands[i]);
	}

	::DrawMenuBar(_hMenuParent);
}

void Accelerator::updateMenuItemByCommand(CommandShortcut csc) {
	int cmdID = (int)csc.getID();
	
	//  Ensure that the menu item checks set prior to this update remain in affect.
	UINT cmdFlags = GetMenuState(_hAccelMenu, cmdID, MF_BYCOMMAND );
	cmdFlags = MF_BYCOMMAND | (cmdFlags&MF_CHECKED) ? ( MF_CHECKED ) : ( MF_UNCHECKED );
	::ModifyMenu(_hAccelMenu, cmdID, cmdFlags, cmdID, csc.toMenuItemString().c_str());
}

recordedMacroStep::recordedMacroStep(int iMessage, long wParam, long lParam, int codepage)
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
			case IDFINDWHAT:
			case IDREPLACEWITH:
			case IDD_FINDINFILES_DIR_COMBO:
			case IDD_FINDINFILES_FILTERS_COMBO:
			{
				char *ch = reinterpret_cast<char *>(lParameter);
				TCHAR tch[2];
				::MultiByteToWideChar(codepage, 0, ch, -1, tch, 2);
				sParameter = *tch;
				MacroType = mtUseSParameter;
				lParameter = 0;
			}
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
		char ansiBuffer[3];
		if (MacroType == mtUseSParameter)
		{
			::WideCharToMultiByte(pEditView->execute(SCI_GETCODEPAGE), 0, sParameter.c_str(), -1, ansiBuffer, 3, NULL, NULL);
			lParam = reinterpret_cast<LPARAM>(ansiBuffer);
		}

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
			if(sParameter.empty())
				scnN.ch = 0;
			else
				scnN.ch = sParameter.at(0);
			::SendMessage(pNotepad->getHSelf(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scnN));
		}
	}
}

void ScintillaAccelerator::init(vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent) {
	_hAccelMenu = hMenu;
	_hMenuParent = menuParent;
	size_t nr = vScintillas->size();
	for(size_t i = 0; i < nr; ++i) {
		_vScintillas.push_back(vScintillas->at(i));
	}
	_nrScintillas = (int)nr;
}

void ScintillaAccelerator::updateKeys() 
{
	NppParameters *pNppParam = NppParameters::getInstance();
	vector<ScintillaKeyMap> & map = pNppParam->getScintillaKeyList();
	size_t mapSize = map.size();
	size_t index;

	for(int i = 0; i < _nrScintillas; ++i)
	{
		::SendMessage(_vScintillas[i], SCI_CLEARALLCMDKEYS, 0, 0);
		for(size_t j = mapSize - 1; j >= 0; j--) //reverse order, top of the list has highest priority
		{	
			ScintillaKeyMap skm = map[j];
			if (skm.isEnabled()) 
			{		//no validating, scintilla accepts more keys
				size_t size = skm.getSize();
				for(index = 0; index < size; ++index)
					::SendMessage(_vScintillas[i], SCI_ASSIGNCMDKEY, skm.toKeyDef(index), skm.getScintillaKeyID());
			}
			if (skm.getMenuCmdID() != 0) 
			{
				updateMenuItemByID(skm, skm.getMenuCmdID());
			}
			if (j == 0)	//j is unsigned, so default method doesnt work
				break;
		}
	}
}

void ScintillaAccelerator::updateMenuItemByID(ScintillaKeyMap skm, int id) 
{
	const int commandSize = 64;
	TCHAR cmdName[commandSize];
	::GetMenuString(_hAccelMenu, id, cmdName, commandSize, MF_BYCOMMAND);
	int i = 0;
	while(cmdName[i] != 0)
	{
		if (cmdName[i] == '\t')
		{
			cmdName[i] = 0;
			break;
		}
		++i;
	}
	generic_string menuItem = cmdName;
	if (skm.isEnabled())
	{
		menuItem += TEXT("\t");
		//menuItem += TEXT("Sc:");	//sc: scintilla shortcut
		menuItem += skm.toString();
	}
	::ModifyMenu(_hAccelMenu, id, MF_BYCOMMAND, id, menuItem.c_str());
	::DrawMenuBar(_hMenuParent);
}

//This procedure uses _keyCombo as a temp. variable to store current settings which can then later be applied (by pressing OK)
void ScintillaKeyMap::applyToCurrentIndex() {
	int index = (int)::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0);
	if(index == LB_ERR)
		return;
	setKeyComboByIndex(index, _keyCombo);
	updateListItem(index);
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, index, 0);

}

void ScintillaKeyMap::validateDialog() {
	bool valid = isValid();	//current combo valid?
	bool isDisabling = _keyCombo._key == 0;	//true if this keycombo were to disable the shortcut
	bool isDisabled = !isEnabled();	//true if this shortcut already is 

	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_ADD), valid && !isDisabling);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_APPLY), valid && (!isDisabling || size == 1));
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RMVE), (size > 1)?TRUE:FALSE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isDisabled?SW_SHOW:SW_HIDE);
}

void ScintillaKeyMap::showCurrentSettings() {
	int keyIndex = ::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0);
	_keyCombo = _keyCombos[keyIndex];
	::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK,	BM_SETCHECK, _keyCombo._isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK,		BM_SETCHECK, _keyCombo._isAlt?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK,	BM_SETCHECK, _keyCombo._isShift?BST_CHECKED:BST_UNCHECKED, 0);
	for (size_t i = 0 ; i < nrKeys ; ++i)
	{
		if (_keyCombo._key == namedKeyArray[i].id)
		{
			::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_SETCURSEL, i, 0);
			break;
		}
	}
}

void ScintillaKeyMap::updateListItem(int index) {
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_INSERTSTRING, index, (LPARAM)toString(index).c_str());
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_DELETESTRING, index+1, 0);
}

INT_PTR CALLBACK ScintillaKeyMap::run_dlgProc(UINT Message, WPARAM wParam, LPARAM) 
{
	
	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SetDlgItemText(_hSelf, IDC_NAME_EDIT, _name);
			_keyCombo = _keyCombos[0];

			for (size_t i = 0 ; i < nrKeys ; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, (LPARAM)namedKeyArray[i].name);
			}

			for(size_t i = 0; i < size; ++i) {
				::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_ADDSTRING, 0, (LPARAM)toString(i).c_str());
			}
			::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, 0, 0);

			showCurrentSettings();
			validateDialog();

			goToCenter();
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_CTRL_CHECK :
					_keyCombo._isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					//applyToCurrentIndex();
					validateDialog();
					return TRUE;

				case IDC_ALT_CHECK :
					_keyCombo._isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					//applyToCurrentIndex();
					validateDialog();
					return TRUE;

				case IDC_SHIFT_CHECK :
					_keyCombo._isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, wParam, BM_GETCHECK, 0, 0);
					//applyToCurrentIndex();
					return TRUE;

				case IDOK :
					//Cleanup
					_keyCombo._key = 0;
					_keyCombo._isCtrl = _keyCombo._isAlt = _keyCombo._isShift = false;
					::EndDialog(_hSelf, 0);
					return TRUE;

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
					return TRUE;

				case IDC_BUTTON_ADD: {
					int oldsize = size;
					int res = addKeyCombo(_keyCombo);
					if (res > -1) {
						if (res == oldsize) {
							::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)toString(res).c_str());
						}else {	//update current generic_string, can happen if it was disabled
							updateListItem(res);
						}
						::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, res, 0);
					}
					showCurrentSettings();
					validateDialog();
					return TRUE; }

				case IDC_BUTTON_RMVE: {
					if (size == 1)	//cannot delete last shortcut
						return TRUE;
					int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0);
					removeKeyComboByIndex(i);
					::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_DELETESTRING, i, 0);
					if (i == (int)size)
						i = size - 1;
					::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, i, 0);
					showCurrentSettings();
					validateDialog();
					return TRUE; }

				case IDC_BUTTON_APPLY: {
					applyToCurrentIndex();
					validateDialog();
					return TRUE; }

				default:
					if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == LBN_SELCHANGE)
					{
						switch(LOWORD(wParam)) {
							case IDC_KEY_COMBO:
							{
								int i = ::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_GETCURSEL, 0, 0);
								_keyCombo._key = namedKeyArray[i].id;
								//applyToCurrentIndex();
								validateDialog();
								return TRUE;
							}
							case IDC_LIST_KEYS:
							{
								showCurrentSettings();
								return TRUE;
							}
						}
					}
					return FALSE;
			}
		}
		default :
			return FALSE;
	}

	//return FALSE;
}
