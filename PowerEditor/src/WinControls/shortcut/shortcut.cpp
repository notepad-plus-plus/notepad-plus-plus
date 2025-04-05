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


#include <memory>
#include <algorithm>
#include <array>
#include "shortcut.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "resource.h"
#include "Notepad_plus_Window.h"
#include "keys.h"

using namespace std;

struct KeyIDNAME {
	const char * name = nullptr;
	UCHAR id = 0;
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
// {"Num Enter"), VK_SEPARATOR},	//this one doesnt seem to work: see below
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

{"F13", VK_F13},
{"F14", VK_F14},
{"F15", VK_F15},
{"F16", VK_F16},
{"F17", VK_F17},
{"F18", VK_F18},
{"F19", VK_F19},
{"F20", VK_F20},
{"F21", VK_F21},
{"F22", VK_F22},
{"F23", VK_F23},
{"F24", VK_F24},

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

// add placeholders for the VirtualKeys that aren't on the standard en-US keyboard
{"", VK_OEM_8},	// VK_OEM_8 not on US keyboards, despite being named
{"", VK_SEPARATOR}, // used for the ',' in "1,234,567.89", but it's not on most keyboards, so keep it in the custom section
{"", 0x88},		// RESERVED
{"", 0x89},		// RESERVED
{"", 0x8A},		// RESERVED
{"", 0x8B},		// RESERVED
{"", 0x8C},		// RESERVED
{"", 0x8D},		// RESERVED
{"", 0x8E},		// RESERVED
{"", 0x8F},		// RESERVED
{"", 0x92},		// OEM SPECIFIC
{"", 0x93},		// OEM SPECIFIC
{"", 0x94},		// OEM SPECIFIC
{"", 0x95},		// OEM SPECIFIC
{"", 0x96},		// OEM SPECIFIC
{"", 0x97},		// UNASSIGNED
{"", 0x98},		// UNASSIGNED
{"", 0x99},		// UNASSIGNED
{"", 0x9A},		// UNASSIGNED
{"", 0x9B},		// UNASSIGNED
{"", 0x9C},		// UNASSIGNED
{"", 0x9D},		// UNASSIGNED
{"", 0x9E},		// UNASSIGNED
{"", 0x9F},		// UNASSIGNED
{"", 0xB8},		// RESERVED
{"", 0xB9},		// RESERVED
{"", 0xC1},		// RESERVED: ABNT_C1
{"", 0xC2},		// RESERVED: ABNT_C2
{"", 0xC3},		// RESERVED
{"", 0xC4},		// RESERVED
{"", 0xC5},		// RESERVED
{"", 0xC6},		// RESERVED
{"", 0xC7},		// RESERVED
{"", 0xC8},		// RESERVED
{"", 0xC9},		// RESERVED
{"", 0xCA},		// RESERVED
{"", 0xCB},		// RESERVED
{"", 0xCC},		// RESERVED
{"", 0xCD},		// RESERVED
{"", 0xCE},		// RESERVED
{"", 0xCF},		// RESERVED
{"", 0xD0},		// RESERVED
{"", 0xD1},		// RESERVED
{"", 0xD2},		// RESERVED
{"", 0xD3},		// RESERVED
{"", 0xD4},		// RESERVED
{"", 0xD5},		// RESERVED
{"", 0xD6},		// RESERVED
{"", 0xD7},		// RESERVED
{"", 0xD8},		// RESERVED
{"", 0xD9},		// RESERVED
{"", 0xDA},		// RESERVED
{"", 0xE0},		// RESERVED
{"", 0xE1},		// OEM SPECIFIC
{"", 0xE3},		// OEM SPECIFIC
{"", 0xE4},		// OEM SPECIFIC
{"", 0xE6},		// OEM SPECIFIC
{"", 0xE8},		// UNASSIGNED
{"", 0xE9},		// OEM SPECIFIC
{"", 0xEA},		// OEM SPECIFIC
{"", 0xEB},		// OEM SPECIFIC
{"", 0xEC},		// OEM SPECIFIC
{"", 0xED},		// OEM SPECIFIC
{"", 0xEE},		// OEM SPECIFIC
{"", 0xEF},		// OEM SPECIFIC
{"", 0xF0},		// OEM SPECIFIC
{"", 0xF1},		// OEM SPECIFIC
{"", 0xF2},		// OEM SPECIFIC
{"", 0xF3},		// OEM SPECIFIC
{"", 0xF4},		// OEM SPECIFIC
{"", 0xF5},		// OEM SPECIFIC
};

#define nbKeys sizeof(namedKeyArray)/sizeof(KeyIDNAME)

#define MAX_MAPSTR_CHARS 16
map<UCHAR, char[MAX_MAPSTR_CHARS+1]> oemVirtualKeyMap;
std::vector<UCHAR> oemVkUsedIDs;
void mapOemVirtualKeys()
{
	const LANGID EN_US = 0x0409;
	
	// this function should only be called once, so update the flag
	static bool isMapped = false;
	if (isMapped) return;
	
	// determine the active keyboard "language"
	LANGID current_lang_id = LOWORD(GetKeyboardLayout(0));
	
	std::vector<UCHAR> localizableNumpad{VK_MULTIPLY, VK_ADD, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE, VK_SEPARATOR};
	
	// for each of the namedKeyArray, check if it's VK_OEM_*, and update the map of VK_OEM names as needed
	for (size_t i = 0 ; i < nbKeys ; ++i)
	{
		// only need to map the VK_OEM_* keys, which are all at or above 0xA0, or any of the localizable keys on the keypad
		bool keyIsLocalizableOnNumpad = (std::find(localizableNumpad.begin(), localizableNumpad.end(), namedKeyArray[i].id) != localizableNumpad.end());
		if ((namedKeyArray[i].id >= 0xA0) || keyIsLocalizableOnNumpad)
		{
			// now update the STR in the mapping
			UINT v2c = MapVirtualKeyW((UINT)namedKeyArray[i].id, MAPVK_VK_TO_CHAR);
			
			// if it's a "dead key" (pre-accent modifier) or "ligature key", it will be marked with high order bits; mask those out...
			v2c = v2c & 0x1FFFFF;	// highest unicode is U+1F_FFFF, so that's the highest valid codepoint
			
			// if it's a normal character, just put it in the map; if the codepoint is higher than 0x7F, need to convert from codepoint to MultiByte UTF8-encoded text
			if (v2c < 0x80)
			{
				sprintf_s(oemVirtualKeyMap[namedKeyArray[i].id], MAX_MAPSTR_CHARS, "%c", v2c);
			}
			else 
			{
				// convert from codepoint to MultiByte UTF8-encoded text
				wchar_t v2w[2] = { (wchar_t)v2c, 0x00 };
				char bytes[8] = {0};
				int len = WideCharToMultiByte(CP_UTF8, 0, &v2w[0], -1, NULL, 0, NULL, NULL);
				if (len > 0)
				{
					WideCharToMultiByte(CP_UTF8, 0, &v2w[0], -1, &bytes[0], len, NULL, NULL);
				}
				sprintf_s(oemVirtualKeyMap[namedKeyArray[i].id], MAX_MAPSTR_CHARS, "%s", bytes);
			}

			// look for edge cases for naming
			if (current_lang_id == EN_US && oemVirtualKeyMap[namedKeyArray[i].id][0] == '`')
			{
				// en-US only: change from ` to ~, because that's what's historically been shown
				oemVirtualKeyMap[namedKeyArray[i].id][0] = '~';
			}
			else if (oemVirtualKeyMap[namedKeyArray[i].id][0] && keyIsLocalizableOnNumpad)
			{
				// make sure any localized numeric-keypad characters have the "Num" as a prefix
				char old_char = oemVirtualKeyMap[namedKeyArray[i].id][0];
				sprintf_s(oemVirtualKeyMap[namedKeyArray[i].id], MAX_MAPSTR_CHARS, "Num %c", old_char);
			}
			else if (oemVirtualKeyMap[namedKeyArray[i].id][0] == '.' && namedKeyArray[i].id != VK_DECIMAL && namedKeyArray[i].id != VK_OEM_PERIOD)
			{
				// the '.' being associated with something other than VK_DECIMAL and VK_OEM_PERIOD must be on the numeric keypad, probably as ABNT2_C2
				sprintf_s(oemVirtualKeyMap[namedKeyArray[i].id], MAX_MAPSTR_CHARS, "Num .");
			}
		}
	}
	isMapped = true;
}

string Shortcut::toString() const
{
	string sc;
	if (!isEnabled())
		return sc;

	if (_keyCombo._isCtrl)
		sc += "Ctrl+";
	if (_keyCombo._isAlt)
		sc += "Alt+";
	if (_keyCombo._isShift)
		sc += "Shift+";

	string keyString;
	getKeyStrFromVal(_keyCombo._key, keyString);
	sc += keyString;
	return sc;
}

void Shortcut::setName(const char* menuName, const char* shortcutName)
{
	lstrcpynA(_menuName, menuName, menuItemStrLenMax);
	char const * name = shortcutName ? shortcutName : menuName;
	size_t i = 0, j = 0;
	while (name[j] != 0 && i < (menuItemStrLenMax - 1))
	{
		if (name[j] != '&')
		{
			_name[i] = name[j];
			++i;
		}
		else //check if this ampersand is being escaped
		{
			if (name[j+1] == '&') //escaped ampersand
			{
				_name[i] = name[j];
				++i;
				++j;	//skip escaped ampersand
			}
		}
		++j;
	}
	_name[i] = 0;
}

string ScintillaKeyMap::toString() const
{
	string sc;
	size_t nbCombos = getSize();
	for (size_t combo = 0; combo < nbCombos; ++combo)
	{
		sc += toString(combo);
		if (combo < nbCombos - 1)
			sc += " / ";
	}
	return sc;
}

string ScintillaKeyMap::toString(size_t index) const
{
	string sc = "";
	if (!isEnabled())
		return sc;

	KeyCombo kc = _keyCombos[index];
	if (kc._isCtrl)
		sc += "Ctrl+";
	if (kc._isAlt)
		sc += "Alt+";
	if (kc._isShift)
		sc += "Shift+";

	string keyString;
	getKeyStrFromVal(kc._key, keyString);
	sc += keyString;
	return sc;
}

KeyCombo ScintillaKeyMap::getKeyComboByIndex(size_t index) const
{
	return _keyCombos[index];
}

void ScintillaKeyMap::setKeyComboByIndex(int index, KeyCombo combo)
{
	if (combo._key == 0 && (_size > 1))
	{	//remove the item if possible
		_keyCombos.erase(_keyCombos.begin() + index);
	}
	_keyCombos[index] = combo;
}

void ScintillaKeyMap::removeKeyComboByIndex(size_t index)
{
	if (_size > 1 && index < _size)
	{
		_keyCombos.erase(_keyCombos.begin() + index);
		_size--;
	}
}

int ScintillaKeyMap::addKeyCombo(KeyCombo combo)
{	//returns index where key is added, or -1 when invalid
	if (combo._key == 0)	//do not allow to add disabled keycombos
		return -1;
	if (!isEnabled())
	{	//disabled, override current combo with new enabled one
		_keyCombos[0] = combo;
		return 0;
	}

	for (size_t i = 0; i < _size; ++i)
	{	//if already in the list do not add it
		const KeyCombo& kc = _keyCombos[i];
		if (combo._key == kc._key && combo._isCtrl == kc._isCtrl && combo._isAlt == kc._isAlt && combo._isShift == kc._isShift)
			return static_cast<int32_t>(i);	//already in the list
	}
	_keyCombos.push_back(combo);
	++_size;
	return static_cast<int32_t>(_size - 1);
}

bool ScintillaKeyMap::isEnabled() const
{
	return (_keyCombos[0]._key != 0);
}

size_t ScintillaKeyMap::getSize() const
{
	return _size;
}

void getKeyStrFromVal(UCHAR keyVal, string & str)
{
	mapOemVirtualKeys();
	str = "";
	bool found = false;
	size_t i;
	for (i = 0; i < nbKeys; ++i)
	{
		if (keyVal == namedKeyArray[i].id)
		{
			found = true;
			break;
		}
	}
	if (found) 
	{
		// by default, assume it will just use the original name
		str = namedKeyArray[i].name;
		
		// but if that key is mapped on this keyboard, then use the mapped name (if the name is empty, it is not on the active keyboard)
		if (oemVirtualKeyMap.find(namedKeyArray[i].id) != oemVirtualKeyMap.end())
		{
			str = oemVirtualKeyMap[namedKeyArray[i].id];
		}
	}
	else 
		str = "Unlisted";
}

void getNameStrFromCmd(DWORD cmd, wstring & str)
{
	if ((cmd >= ID_MACRO) && (cmd < ID_MACRO_LIMIT))
	{
		const vector<MacroShortcut> & theMacros = (NppParameters::getInstance()).getMacroList();
		int i = cmd - ID_MACRO;
		str = string2wstring(theMacros[i].getName(), CP_UTF8);
	}
	else if ((cmd >= ID_USER_CMD) && (cmd < ID_USER_CMD_LIMIT))
	{
		const vector<UserCommand> & userCommands = (NppParameters::getInstance()).getUserCommandList();
		int i = cmd - ID_USER_CMD;
		str = string2wstring(userCommands[i].getName(), CP_UTF8);
	}
	else if ((cmd >= ID_PLUGINS_CMD) && (cmd < ID_PLUGINS_CMD_LIMIT))
	{
		const vector<PluginCmdShortcut> & pluginCmds = (NppParameters::getInstance()).getPluginCommandList();
		size_t i = 0;
		for (size_t j = 0, len = pluginCmds.size(); j < len ; ++j)
		{
			if (pluginCmds[j].getID() == cmd)
			{
				i = j;
				break;
			}
		}
		str = string2wstring(pluginCmds[i].getName(), CP_UTF8);
	}
	else
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		wchar_t cmdName[menuItemStrLenMax];
		HMENU m = reinterpret_cast<HMENU>(::SendMessage(hNotepad_plus, NPPM_INTERNAL_GETMENU, 0, 0));
		int nbChar = ::GetMenuString(m, cmd, cmdName, menuItemStrLenMax, MF_BYCOMMAND);
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

void Shortcut::updateConflictState(const bool endSession) const
{
	if (endSession)
	{
		// Clean up message for detached dialogs: save Macros/RunCommands
		::SendMessage(_hParent, NPPM_INTERNAL_FINDKEYCONFLICTS, 0, 0);
		return;
	}

	// Check for conflicts
	bool isConflict = false;
	::SendMessage(_hParent, NPPM_INTERNAL_FINDKEYCONFLICTS,
				  reinterpret_cast<WPARAM>(&_keyCombo), reinterpret_cast<LPARAM>(&isConflict));
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CONFLICT_STATIC), isConflict ? SW_SHOW : SW_HIDE);
}

intptr_t CALLBACK Shortcut::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			mapOemVirtualKeys();

			::SetDlgItemText(_hSelf, IDC_NAME_EDIT, _canModifyName ? string2wstring(getMenuName(), CP_UTF8).c_str() : string2wstring(getName(), CP_UTF8).c_str());	//display the menu name, with ampersands, for macros
			if (!_canModifyName)
				::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, EM_SETREADONLY, TRUE, 0);
			auto textlen = ::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);

			::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK, BM_SETCHECK, _keyCombo._isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK, BM_SETCHECK, _keyCombo._isAlt?BST_CHECKED:BST_UNCHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK, BM_SETCHECK, _keyCombo._isShift?BST_CHECKED:BST_UNCHECKED, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));

			// while buiding the IDC_KEY_COMBO keys, update the map to which VirtualKey is used for each index
			int iFound = -1;
			for (size_t i = 0 ; i < nbKeys ; ++i)
			{
				const char* nameStr = namedKeyArray[i].name;
				
				// use the virtual key value, if it's been mapped
				if (oemVirtualKeyMap.find(namedKeyArray[i].id) != oemVirtualKeyMap.end()) 
				{
					nameStr = oemVirtualKeyMap[namedKeyArray[i].id];
				} 
				
				// only add a key to the IDC_KEY_COMBO list if the string is not empty
				if (nameStr[0])
				{
					::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(string2wstring(nameStr, CP_UTF8).c_str()));
					oemVkUsedIDs.push_back(namedKeyArray[i].id);
				}

				if (_keyCombo._key == namedKeyArray[i].id)
					iFound = static_cast<int32_t>(oemVkUsedIDs.size());
			}

			if (iFound != -1)
				::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_SETCURSEL, iFound, 0);
			
			// Hide this warning on startup
			::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), SW_HIDE);
			
			updateConflictState();
			NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
			nativeLangSpeaker->changeDlgLang(_hSelf, "ShortcutMapperSubDialg");
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (dlgCtrlID == IDC_NAME_EDIT)
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
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

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			auto textlen = ::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);
			switch (LOWORD(wParam))
			{
				case IDC_CTRL_CHECK :
					_keyCombo._isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					updateConflictState();
					return TRUE;

				case IDC_ALT_CHECK :
					_keyCombo._isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
					updateConflictState();
					return TRUE;

				case IDC_SHIFT_CHECK :
					_keyCombo._isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					updateConflictState();
					return TRUE;

				case IDOK :
					if (!isEnabled())
					{
						_keyCombo._isCtrl = _keyCombo._isAlt = _keyCombo._isShift = false;
					}

					if (_canModifyName)
					{
						wchar_t editName[menuItemStrLenMax]{};
						::SendDlgItemMessage(_hSelf, IDC_NAME_EDIT, WM_GETTEXT, menuItemStrLenMax, reinterpret_cast<LPARAM>(editName));
						setName(wstring2string(editName, CP_UTF8).c_str());
					}
					::EndDialog(_hSelf, 0);
					updateConflictState(true);
					return TRUE;

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
					updateConflictState(true);
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
							auto iSel = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
							_keyCombo._key = oemVkUsedIDs[iSel];
							::EnableWindow(::GetDlgItem(_hSelf, IDOK), isValid() && (textlen > 0 || !_canModifyName));
							::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isEnabled()?SW_HIDE:SW_SHOW);
							updateConflictState();
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
void Accelerator::updateShortcuts() 
{
	const array<unsigned long, 3> incrFindAccIds = { IDM_SEARCH_FINDNEXT, IDM_SEARCH_FINDPREV, IDM_SEARCH_FINDINCREMENT };

	NppParameters& nppParam = NppParameters::getInstance();

	vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
	vector<MacroShortcut> & macros  = nppParam.getMacroList();
	vector<UserCommand> & userCommands = nppParam.getUserCommandList();
	vector<PluginCmdShortcut> & pluginCommands = nppParam.getPluginCommandList();

	size_t nbMenu = shortcuts.size();
	size_t nbMacro = macros.size();
	size_t nbUserCmd = userCommands.size();
	size_t nbPluginCmd = pluginCommands.size();

	delete [] _pAccelArray;
	_pAccelArray = new ACCEL[nbMenu + nbMacro + nbUserCmd + nbPluginCmd];
	vector<ACCEL> incrFindAcc;
	vector<ACCEL> findReplaceAcc;
	int offset = 0;
	size_t i = 0;
	//no validation performed, it might be that invalid shortcuts are being used by default. Allows user to 'hack', might be a good thing
	for (i = 0; i < nbMenu; ++i)
	{
		if (shortcuts[i].isEnabled())
		{
			_pAccelArray[offset].cmd = static_cast<WORD>(shortcuts[i].getID());
			_pAccelArray[offset].fVirt = shortcuts[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = shortcuts[i].getKeyCombo()._key;

			// Special extra handling for shortcuts shared by Incremental Find dialog
			if (std::find(incrFindAccIds.begin(), incrFindAccIds.end(), shortcuts[i].getID()) != incrFindAccIds.end())
				incrFindAcc.push_back(_pAccelArray[offset]);

			if (shortcuts[i].getID() == IDM_SEARCH_FIND || shortcuts[i].getID() == IDM_SEARCH_REPLACE ||
				shortcuts[i].getID() == IDM_SEARCH_FINDINFILES || shortcuts[i].getID() == IDM_SEARCH_MARK ||
				shortcuts[i].getID() == IDM_SEARCH_FINDNEXT || shortcuts[i].getID() == IDM_SEARCH_FINDPREV)
				findReplaceAcc.push_back(_pAccelArray[offset]);

			++offset;
		}
	}

	for (i = 0; i < nbMacro; ++i)
	{
		if (macros[i].isEnabled()) 
		{
			_pAccelArray[offset].cmd = (WORD)(macros[i].getID());
			_pAccelArray[offset].fVirt = macros[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = macros[i].getKeyCombo()._key;
			++offset;
		}
	}

	for (i = 0; i < nbUserCmd; ++i)
	{
		if (userCommands[i].isEnabled())
		{
			_pAccelArray[offset].cmd = (WORD)(userCommands[i].getID());
			_pAccelArray[offset].fVirt = userCommands[i].getAcceleratorModifiers();
			_pAccelArray[offset].key = userCommands[i].getKeyCombo()._key;
			++offset;
		}
	}

	for (i = 0; i < nbPluginCmd; ++i)
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
	size_t nb = incrFindAcc.size();
	ACCEL *tmpIncrFindAccelArray = new ACCEL[nb];
	for (i = 0; i < nb; ++i)
	{
		tmpIncrFindAccelArray[i] = incrFindAcc[i];
	}
	_hIncFindAccTab = ::CreateAcceleratorTable(tmpIncrFindAccelArray, static_cast<int32_t>(nb));
	delete [] tmpIncrFindAccelArray;

	if (_hAccTabSwitch)
		::DestroyAcceleratorTable(_hAccTabSwitch);

	ACCEL accNextTab{ BYTE{FVIRTKEY | FCONTROL}, VK_TAB, IDC_NEXT_TAB };
	ACCEL accPrevTab{ BYTE{FVIRTKEY | FCONTROL | FSHIFT}, VK_TAB, IDC_PREV_TAB };
	vector<ACCEL> tabSwitchAcc{ accNextTab, accPrevTab };
	const size_t nbTabSwitchAcc = tabSwitchAcc.size();
	if (nbTabSwitchAcc > 0)
	{
		ACCEL* tmpTabSwitchAcc = new ACCEL[nbTabSwitchAcc];
		for (i = 0; i < nbTabSwitchAcc; ++i)
			tmpTabSwitchAcc[i] = tabSwitchAcc.at(i);
		_hAccTabSwitch = ::CreateAcceleratorTable(tmpTabSwitchAcc, static_cast<int>(nbTabSwitchAcc));
		delete[] tmpTabSwitchAcc;
	}

	if (_hFindAccTab)
		::DestroyAcceleratorTable(_hFindAccTab);

	size_t nbFindReplaceAcc = findReplaceAcc.size();
	if (nbFindReplaceAcc)
	{
		ACCEL* tmpFindAccelArray = new ACCEL[nbFindReplaceAcc];

		for (size_t j = 0; j < nbFindReplaceAcc; ++j)
			tmpFindAccelArray[j] = findReplaceAcc[j];

		_hFindAccTab = ::CreateAcceleratorTable(tmpFindAccelArray, static_cast<int>(nbFindReplaceAcc));
		delete[] tmpFindAccelArray;
	}

	return;
}

void Accelerator::updateFullMenu()
{
	NppParameters& nppParam = NppParameters::getInstance();
	vector<CommandShortcut> commands = nppParam.getUserShortcuts();
	for (size_t i = 0; i < commands.size(); ++i)
	{
		updateMenuItemByCommand(commands[i]);
	}

	vector<MacroShortcut> mcommands = nppParam.getMacroList();
	for (size_t i = 0; i < mcommands.size(); ++i)
	{
		updateMenuItemByCommand(mcommands[i]);
	}

	vector<UserCommand> ucommands = nppParam.getUserCommandList();
	for (size_t i = 0; i < ucommands.size(); ++i)
	{
		updateMenuItemByCommand(ucommands[i]);
	}

	vector<PluginCmdShortcut> pcommands = nppParam.getPluginCommandList();
	for (size_t i = 0; i < pcommands.size(); ++i)
	{
		updateMenuItemByCommand(pcommands[i]);
	}

	::DrawMenuBar(_hMenuParent);
}

void Accelerator::updateMenuItemByCommand(const CommandShortcut& csc)
{
	int cmdID = csc.getID();
	
	// Ensure that the menu item checks set prior to this update remain in affect.
	// Ensure that the menu item state is also maintained
	UINT cmdFlags = GetMenuState(_hAccelMenu, cmdID, MF_BYCOMMAND );
	cmdFlags = MF_BYCOMMAND | ((cmdFlags&MF_CHECKED) ? MF_CHECKED : MF_UNCHECKED) | ((cmdFlags&MF_DISABLED) ? MF_DISABLED : MF_ENABLED);
	::ModifyMenu(_hAccelMenu, cmdID, cmdFlags, cmdID, string2wstring(csc.toMenuItemString(), CP_UTF8).c_str());
}

recordedMacroStep::recordedMacroStep(int iMessage, uptr_t wParam, uptr_t lParam)
	: _message(iMessage), _wParameter(wParam), _lParameter(lParam), _macroType(mtUseLParameter)
{ 
	if (_lParameter)
	{
		switch (_message)
		{
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
				char *ch = reinterpret_cast<char *>(_lParameter);
				_sParameter = ch;
				_macroType = mtUseSParameter;
				_lParameter = 0;
			}
			break;

				
			default : // for all other messages, use _lParameter "as is"
				break;
		}
	}
}

// code comes from Scintilla's Editor.cxx:
// void Editor::NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam)
bool recordedMacroStep::isMacroable() const
{
	// Enumerates all macroable messages
	switch (_message)
	{
		case SCI_REPLACESEL: // (<unused>, const char *text)
		case SCI_ADDTEXT:    // (int length, const char *s)
		case SCI_INSERTTEXT: // (int pos, const char *text)
		case SCI_APPENDTEXT: // (int length, const char *s)
		case SCI_SEARCHNEXT: // (int searchFlags, const char *text)
		case SCI_SEARCHPREV: // (int searchFlags, const char *text)
		{
			if (_macroType == mtUseSParameter)
				return true;
			else
				return false;
		}

		case SCI_GOTOLINE:   // (int line)
		case SCI_GOTOPOS:    // (int position)
		case SCI_SETSELECTIONMODE:  // (int mode)
		case SCI_CUT:
		case SCI_COPY:
		case SCI_PASTE:
		case SCI_CLEAR:
		case SCI_CLEARALL:
		case SCI_SELECTALL:
		case SCI_SEARCHANCHOR:
		case SCI_LINEDOWN:
		case SCI_LINEDOWNEXTEND:
		case SCI_PARADOWN:
		case SCI_PARADOWNEXTEND:
		case SCI_LINEUP:
		case SCI_LINEUPEXTEND:
		case SCI_PARAUP:
		case SCI_PARAUPEXTEND:
		case SCI_CHARLEFT:
		case SCI_CHARLEFTEXTEND:
		case SCI_CHARRIGHT:
		case SCI_CHARRIGHTEXTEND:
		case SCI_WORDLEFT:
		case SCI_WORDLEFTEXTEND:
		case SCI_WORDRIGHT:
		case SCI_WORDRIGHTEXTEND:
		case SCI_WORDPARTLEFT:
		case SCI_WORDPARTLEFTEXTEND:
		case SCI_WORDPARTRIGHT:
		case SCI_WORDPARTRIGHTEXTEND:
		case SCI_WORDLEFTEND:
		case SCI_WORDLEFTENDEXTEND:
		case SCI_WORDRIGHTEND:
		case SCI_WORDRIGHTENDEXTEND:
		case SCI_HOME:
		case SCI_HOMEEXTEND:
		case SCI_LINEEND:
		case SCI_LINEENDEXTEND:
		case SCI_HOMEWRAP:
		case SCI_HOMEWRAPEXTEND:
		case SCI_LINEENDWRAP:
		case SCI_LINEENDWRAPEXTEND:
		case SCI_DOCUMENTSTART:
		case SCI_DOCUMENTSTARTEXTEND:
		case SCI_DOCUMENTEND:
		case SCI_DOCUMENTENDEXTEND:
		case SCI_STUTTEREDPAGEUP:
		case SCI_STUTTEREDPAGEUPEXTEND:
		case SCI_STUTTEREDPAGEDOWN:
		case SCI_STUTTEREDPAGEDOWNEXTEND:
		case SCI_PAGEUP:
		case SCI_PAGEUPEXTEND:
		case SCI_PAGEDOWN:
		case SCI_PAGEDOWNEXTEND:
		case SCI_EDITTOGGLEOVERTYPE:
		case SCI_CANCEL:
		case SCI_DELETEBACK:
		case SCI_TAB:
		case SCI_BACKTAB:
		case SCI_FORMFEED:
		case SCI_VCHOME:
		case SCI_VCHOMEEXTEND:
		case SCI_VCHOMEWRAP:
		case SCI_VCHOMEWRAPEXTEND:
		case SCI_VCHOMEDISPLAY:
		case SCI_VCHOMEDISPLAYEXTEND:
		case SCI_DELWORDLEFT:
		case SCI_DELWORDRIGHT:
		case SCI_DELWORDRIGHTEND:
		case SCI_DELLINELEFT:
		case SCI_DELLINERIGHT:
		case SCI_LINECOPY:
		case SCI_LINECUT:
		case SCI_LINEDELETE:
		case SCI_LINETRANSPOSE:
		case SCI_LINEDUPLICATE:
		case SCI_LOWERCASE:
		case SCI_UPPERCASE:
		case SCI_LINESCROLLDOWN:
		case SCI_LINESCROLLUP:
		case SCI_DELETEBACKNOTLINE:
		case SCI_HOMEDISPLAY:
		case SCI_HOMEDISPLAYEXTEND:
		case SCI_LINEENDDISPLAY:
		case SCI_LINEENDDISPLAYEXTEND:
		case SCI_LINEDOWNRECTEXTEND:
		case SCI_LINEUPRECTEXTEND:
		case SCI_CHARLEFTRECTEXTEND:
		case SCI_CHARRIGHTRECTEXTEND:
		case SCI_HOMERECTEXTEND:
		case SCI_VCHOMERECTEXTEND:
		case SCI_LINEENDRECTEXTEND:
		case SCI_PAGEUPRECTEXTEND:
		case SCI_PAGEDOWNRECTEXTEND:
		case SCI_SELECTIONDUPLICATE:
		case SCI_COPYALLOWLINE:
		case SCI_VERTICALCENTRECARET:
		case SCI_MOVESELECTEDLINESUP:
		case SCI_MOVESELECTEDLINESDOWN:
		case SCI_SCROLLTOSTART:
		case SCI_SCROLLTOEND:
		case SCI_SETVIRTUALSPACEOPTIONS:
		case SCI_SETCARETLINEBACKALPHA:
		case SCI_NEWLINE:
		{
			if (_macroType == mtUseLParameter)
				return true;
			else
				return false;
		}

		// Filter out all others like display changes.
		default:
			return false;
	}
}

void recordedMacroStep::PlayBack(Window* pNotepad, ScintillaEditView *pEditView)
{
	if (_macroType == mtMenuCommand)
	{
		::SendMessage(pNotepad->getHSelf(), WM_COMMAND, _wParameter, 0);
	}
	else
	{
		// Ensure it's macroable message before send it
		if (!isMacroable())
			return;

		if (_macroType == mtUseSParameter) 
		{
			/*
			int byteBufferLength = ::WideCharToMultiByte(static_cast<UINT>(pEditView->execute(SCI_GETCODEPAGE)), 0, _sParameter.c_str(), -1, NULL, 0, NULL, NULL);
			auto byteBuffer = std::make_unique< char[] >(byteBufferLength);
			::WideCharToMultiByte(static_cast<UINT>(pEditView->execute(SCI_GETCODEPAGE)), 0, _sParameter.c_str(), -1, byteBuffer.get(), byteBufferLength, NULL, NULL);
			auto lParam = reinterpret_cast<LPARAM>(byteBuffer.get());
			pEditView->execute(_message, _wParameter, lParam);
			*/
			pEditView->execute(_message, _wParameter, (LPARAM)_sParameter.c_str());
		}
		else
		{
			pEditView->execute(_message, _wParameter, _lParameter);
		}

		// If text content has been modified in Scintilla,
		// then notify Notepad++
		if ( (_message == SCI_SETTEXT)
			|| (_message == SCI_REPLACESEL) 
			|| (_message == SCI_ADDTEXT) 
			|| (_message == SCI_ADDSTYLEDTEXT) 
			|| (_message == SCI_INSERTTEXT) 
			|| (_message == SCI_APPENDTEXT) )
		{
			SCNotification scnN{};
			scnN.nmhdr.code = SCN_CHARADDED;
			scnN.nmhdr.hwndFrom = pEditView->getHSelf();
			scnN.nmhdr.idFrom = 0;
			if (_sParameter.empty())
				scnN.ch = 0;
			else
				scnN.ch = _sParameter.at(0);

			::SendMessage(pNotepad->getHSelf(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scnN));
		}
	}
}

void ScintillaAccelerator::init(vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent)
{
	_hAccelMenu = hMenu;
	_hMenuParent = menuParent;
	size_t nbScintilla = vScintillas->size();
	for (size_t i = 0; i < nbScintilla; ++i)
	{
		_vScintillas.push_back(vScintillas->at(i));
	}
}

void ScintillaAccelerator::updateKeys() 
{
	NppParameters& nppParam = NppParameters::getInstance();
	const vector<ScintillaKeyMap> & map = nppParam.getScintillaKeyList();
	size_t mapSize = map.size();
	size_t index;
	size_t nb = nbScintillas();
	for (size_t i = 0; i < nb; ++i)
	{
		::SendMessage(_vScintillas[i], SCI_CLEARALLCMDKEYS, 0, 0);
		for (int32_t j = static_cast<int32_t>(mapSize) - 1; j >= 0; j--) //reverse order, top of the list has highest priority
		{	
			ScintillaKeyMap skm = map[j];
			if (skm.isEnabled()) 
			{		//no validating, scintilla accepts more keys
				size_t size = skm.getSize();
				for (index = 0; index < size; ++index)
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

void ScintillaAccelerator::updateMenuItemByID(const ScintillaKeyMap& skm, int id)
{
	wchar_t cmdName[menuItemStrLenMax];
	::GetMenuString(_hAccelMenu, id, cmdName, menuItemStrLenMax, MF_BYCOMMAND);
	int i = 0;
	while (cmdName[i] != 0)
	{
		if (cmdName[i] == '\t')
		{
			cmdName[i] = 0;
			break;
		}
		++i;
	}
	wstring menuItem = cmdName;
	if (skm.isEnabled())
	{
		menuItem += L"\t";
		menuItem += string2wstring(skm.toString(), CP_UTF8);
	}
	::ModifyMenu(_hAccelMenu, id, MF_BYCOMMAND, id, menuItem.c_str());
	::DrawMenuBar(_hMenuParent);
}

//This procedure uses _keyCombo as a temp. variable to store current settings which can then later be applied (by pressing OK)
void ScintillaKeyMap::applyToCurrentIndex()
{
	int index = static_cast<int>(::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0));
	if (index == LB_ERR)
		return;
	setKeyComboByIndex(static_cast<int>(index), _keyCombo);
	updateListItem(index);
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, index, 0);

}

void ScintillaKeyMap::validateDialog()
{
	bool valid = isValid();	//current combo valid?
	bool isDisabling = _keyCombo._key == 0;	//true if this keycombo were to disable the shortcut
	bool isDisabled = !isEnabled();	//true if this shortcut already is 
	bool isDuplicate = false; //true if already in the list

	for (size_t i = 0; i < _size; ++i) 
	{
		if (_keyCombo._key   == _keyCombos[i]._key   && _keyCombo._isCtrl  == _keyCombos[i]._isCtrl &&
			_keyCombo._isAlt == _keyCombos[i]._isAlt && _keyCombo._isShift == _keyCombos[i]._isShift)
		{
			isDuplicate = true;
			break;
		}
	}

	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_ADD), valid && !isDisabling && !isDuplicate);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_APPLY), valid && (!isDisabling || _size == 1) && !isDuplicate);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RMVE), (_size > 1)?TRUE:FALSE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isDisabled?SW_SHOW:SW_HIDE);
	updateConflictState();
}

void ScintillaKeyMap::showCurrentSettings()
{
	auto keyIndex = ::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0);
	_keyCombo = _keyCombos[keyIndex];
	::SendDlgItemMessage(_hSelf, IDC_CTRL_CHECK,	BM_SETCHECK, _keyCombo._isCtrl?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_ALT_CHECK,		BM_SETCHECK, _keyCombo._isAlt?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_SHIFT_CHECK,	BM_SETCHECK, _keyCombo._isShift?BST_CHECKED:BST_UNCHECKED, 0);
	for (size_t i = 0 ; i < nbKeys ; ++i)
	{
		if (_keyCombo._key == namedKeyArray[i].id)
		{
			::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_SETCURSEL, i, 0);
			break;
		}
	}
}

void ScintillaKeyMap::updateListItem(int index)
{
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_INSERTSTRING, index, reinterpret_cast<LPARAM>(string2wstring(toString(index), CP_UTF8).c_str()));
	::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_DELETESTRING, index+1, 0);
}

intptr_t CALLBACK ScintillaKeyMap::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) 
{
	
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			mapOemVirtualKeys();

			::SetDlgItemText(_hSelf, IDC_NAME_EDIT, string2wstring(_name, CP_UTF8).c_str());
			_keyCombo = _keyCombos[0];

			for (size_t i = 0 ; i < nbKeys ; ++i)
			{
				const char* nameStr = namedKeyArray[i].name;
				
				if (oemVirtualKeyMap.find(namedKeyArray[i].id) != oemVirtualKeyMap.end()) 
				{
					nameStr = oemVirtualKeyMap[namedKeyArray[i].id];
				} 
				
				// only add a key to the IDC_KEY_COMBO list if the string is not empty
				if (nameStr[0])
				{
					::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(string2wstring(nameStr, CP_UTF8).c_str()));
					oemVkUsedIDs.push_back(namedKeyArray[i].id);
				}
			}

			for (size_t i = 0; i < _size; ++i)
			{
				::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(string2wstring(toString(i), CP_UTF8).c_str()));
			}
			::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, 0, 0);

			showCurrentSettings();
			validateDialog();

			// Hide this warning on startup
			::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), SW_HIDE);

			NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
			nativeLangSpeaker->changeDlgLang(_hSelf, "ShortcutMapperSubDialg");
			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColorListbox(wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			auto dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (dlgCtrlID == IDC_NAME_EDIT)
			{
				return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
			}
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

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_CTRL_CHECK :
					_keyCombo._isCtrl = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					validateDialog();
					return TRUE;

				case IDC_ALT_CHECK :
					_keyCombo._isAlt = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					validateDialog();
					return TRUE;

				case IDC_SHIFT_CHECK :
					_keyCombo._isShift = BST_CHECKED == ::SendDlgItemMessage(_hSelf, static_cast<int32_t>(wParam), BM_GETCHECK, 0, 0);
					validateDialog();
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

				case IDC_BUTTON_ADD:
				{
					size_t oldsize = _size;
					int res = addKeyCombo(_keyCombo);
					if (res > -1)
					{
						if (res == static_cast<int32_t>(oldsize))
						{
							::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_INSERTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(string2wstring(toString(res), CP_UTF8).c_str()));
						}
						else
						{	//update current string, can happen if it was disabled
							updateListItem(res);
						}
						::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, res, 0);
					}
					showCurrentSettings();
					validateDialog();
					return TRUE; 
				}

				case IDC_BUTTON_RMVE:
				{
					if (_size == 1)	//cannot delete last shortcut
						return TRUE;
					auto i = ::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_GETCURSEL, 0, 0);
					removeKeyComboByIndex(i);
					::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_DELETESTRING, i, 0);
					if (static_cast<size_t>(i) == _size)
						i = _size - 1;
					::SendDlgItemMessage(_hSelf, IDC_LIST_KEYS, LB_SETCURSEL, i, 0);
					showCurrentSettings();
					validateDialog();
					return TRUE; 
				}

				case IDC_BUTTON_APPLY:
				{
					applyToCurrentIndex();
					validateDialog();
					return TRUE;
				}

				default:
					if (HIWORD(wParam) == CBN_SELCHANGE) // LBN_SELCHANGE has same value 1
					{
						switch(LOWORD(wParam))
						{
							case IDC_KEY_COMBO:
							{
								auto iSel = ::SendDlgItemMessage(_hSelf, IDC_KEY_COMBO, CB_GETCURSEL, 0, 0);
								_keyCombo._key = oemVkUsedIDs[iSel];
								::ShowWindow(::GetDlgItem(_hSelf, IDC_WARNING_STATIC), isEnabled() ? SW_HIDE : SW_SHOW);
								validateDialog();
								return TRUE;
							}
							case IDC_LIST_KEYS:
							{
								showCurrentSettings();
								validateDialog();
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
	return FALSE;
}

void CommandShortcut::setCategoryFromMenu(HMENU hMenu)
{
	NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();

	if (_id >= IDM_WINDOW_WINDOWS && _id <= IDM_WINDOW_SORT_FS_DSC)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "Window", L"Window");
	else if ( _id >= IDM_VIEW_GOTO_ANOTHER_VIEW && _id <= IDM_VIEW_GOTO_END)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "view", L"View");
	else if (_id == IDM_EDIT_LTR || _id == IDM_EDIT_RTL)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "view", L"View");
	else if (_id == IDC_PREV_DOC || _id == IDC_NEXT_DOC)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "view", L"View");
	else if (_id == IDM_FORMAT_TODOS || _id == IDM_FORMAT_TOUNIX || _id == IDM_FORMAT_TOMAC)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "edit", L"Edit");
	else if (_id == IDM_EDIT_AUTOCOMPLETE || _id == IDM_EDIT_AUTOCOMPLETE_CURRENTFILE || _id == IDM_EDIT_FUNCCALLTIP ||
		_id == IDM_EDIT_AUTOCOMPLETE_PATH || _id == IDM_EDIT_FUNCCALLTIP_PREVIOUS || _id == IDM_EDIT_FUNCCALLTIP_NEXT)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "edit", L"Edit");
	else if (_id == IDM_LANGSTYLE_CONFIG_DLG)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "settings", L"Settings");
	else if (_id == IDM_MACRO_STARTRECORDINGMACRO  ||_id == IDM_MACRO_STOPRECORDINGMACRO  || _id == IDM_MACRO_RUNMULTIMACRODLG ||
		_id == IDM_MACRO_PLAYBACKRECORDEDMACRO  ||_id == IDM_MACRO_SAVECURRENTMACRO || _id == IDC_EDIT_TOGGLEMACRORECORDING)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "macro", L"Macro");


	else if ( _id < IDM_EDIT)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "file", L"File");
	else if ( _id < IDM_SEARCH)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "edit",L"Edit");
	else if ( _id < IDM_VIEW)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "search", L"Search");
	else if ( _id < IDM_FORMAT)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "view", L"View");
	else if ( _id < IDM_LANG)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "encoding", L"Encoding");
	else if ( _id < IDM_ABOUT)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "language", L"Language");
	else if ( _id < IDM_SETTING)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "about", L"About");
	else if ( _id < IDM_TOOL)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "settings", L"Settings");
	else if ( _id < IDM_EXECUTE)
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "tools", L"Tools");
	else
		pNativeSpeaker->getMainMenuEntryName(_category, hMenu, "run", L"Run");
}

CommandShortcut& CommandShortcut::operator = (const Shortcut& sct)
{
	if (this != &sct)
	{
		strcpy(this->_name, sct.getName());
		this->_keyCombo = sct.getKeyCombo();
	}
	return *this;
}
