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

#include "shortcutRc.h"
#include "Scintilla.h"
#include "StaticDialog.h"
#include "Common.h"
#include "menuCmdID.h"

constexpr int menuItemStrLenMax = 64 + 64;	// Add 64 "units" more for being compatible to the current localization file. See:
											// https://github.com/notepad-plus-plus/notepad-plus-plus/issues/13556#issuecomment-1518197329

class NppParameters;

void getKeyStrFromVal(UCHAR keyVal, std::string & str);
void getNameStrFromCmd(DWORD cmd, std::wstring & str);
static size_t keyTranslate(size_t keyIn) {
	switch (keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:			return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:		return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:		return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:		return SCK_RETURN;
		case VK_ADD:		return SCK_ADD;
		case VK_SUBTRACT:	return SCK_SUBTRACT;
		case VK_DIVIDE:		return SCK_DIVIDE;
		case VK_OEM_2:		return '/';
		case VK_OEM_3:		return '`';
		case VK_OEM_4:		return '[';
		case VK_OEM_5:		return '\\';
		case VK_OEM_6:		return ']';
		default:			return keyIn;
	}
}

struct KeyCombo {
	bool _isCtrl = false;
	bool _isAlt = false;
	bool _isShift = false;
	UCHAR _key = 0;
};

class Shortcut : public StaticDialog {
public:
	Shortcut(): _canModifyName(false) {
		setName("");
		_keyCombo._isCtrl = false;
		_keyCombo._isAlt = false;
		_keyCombo._isShift = false;
		_keyCombo._key = 0;
	};

	Shortcut(const char* name, bool isCtrl, bool isAlt, bool isShift, UCHAR key) : _canModifyName(false) {
		_name[0] = '\0';
		if (name) {
			setName(name);
		} else {
			setName("");
		}
		_keyCombo._isCtrl = isCtrl;
		_keyCombo._isAlt = isAlt;
		_keyCombo._isShift = isShift;
		_keyCombo._key = key;
	};

	Shortcut(const Shortcut & sc) {
		setName(sc.getMenuName(), sc.getName());
		_keyCombo = sc._keyCombo;
		_canModifyName = sc._canModifyName;
	}

	BYTE getAcceleratorModifiers() {
		return ( FVIRTKEY | (_keyCombo._isCtrl?FCONTROL:0) | (_keyCombo._isAlt?FALT:0) | (_keyCombo._isShift?FSHIFT:0) );
	};

	Shortcut & operator=(const Shortcut & sc) {
		//Do not allow setting empty names
		//So either we have an empty name or the other name has to be set
		if (_name[0] == 0 || sc._name[0] != 0) {
			setName(sc.getMenuName(), sc.getName());
		}
		_keyCombo = sc._keyCombo;
		this->_canModifyName = sc._canModifyName;
		return *this;
	}
	friend inline bool operator==(const Shortcut & a, const Shortcut & b) {
		return ((strcmp(a.getMenuName(), b.getMenuName()) == 0) && 
			(a._keyCombo._isCtrl == b._keyCombo._isCtrl) && 
			(a._keyCombo._isAlt == b._keyCombo._isAlt) && 
			(a._keyCombo._isShift == b._keyCombo._isShift) && 
			(a._keyCombo._key == b._keyCombo._key)
			);
	};

	friend inline bool operator!=(const Shortcut & a, const Shortcut & b) {
		return !(a == b);
	};

	virtual intptr_t doDialog()
	{
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUT_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
    };

	virtual bool isValid() const { //valid should only be used in cases where the shortcut isEnabled().
		if (_keyCombo._key == 0)
			return true;	//disabled _keyCombo always valid, just disabled

		//These keys need a modifier, else invalid
		if ( ((_keyCombo._key >= 'A') && (_keyCombo._key <= 'Z')) || ((_keyCombo._key >= '0') && (_keyCombo._key <= '9')) || _keyCombo._key == VK_SPACE || _keyCombo._key == VK_CAPITAL || _keyCombo._key == VK_BACK || _keyCombo._key == VK_RETURN) {
			return ((_keyCombo._isCtrl) || (_keyCombo._isAlt));
		}
		// the remaining keys are always valid
		return true;
	};
	virtual bool isEnabled() const {	//true if _keyCombo != 0, false if _keyCombo == 0, in which case no accelerator should be made
		return (_keyCombo._key != 0);
	};

	virtual std::string toString() const;					//the hotkey part
	std::string toMenuItemString() const {					//generic_string suitable for menu
		std::string str = _menuName;
		if (isEnabled())
		{
			str += "\t";
			str += toString();
		}
		return str;
	};
	const KeyCombo & getKeyCombo() const {
		return _keyCombo;
	};

	const char* getName() const {
		return _name;
	};

	const char* getMenuName() const {
		return _menuName;
	}

	void setName(const char* menuName, const char* shortcutName = NULL);

	void clear(){
		_keyCombo._isCtrl = false;
		_keyCombo._isAlt = false;
		_keyCombo._isShift = false;
		_keyCombo._key = 0;
		return;
	}

protected :
	KeyCombo _keyCombo;
	virtual intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	bool _canModifyName = false;
	char _name[menuItemStrLenMax] {};		//normal name is plain text (for display purposes)
	char _menuName[menuItemStrLenMax] {};	//menu name has ampersands for quick keys
	void updateConflictState(const bool endSession = false) const;
};
		 
class CommandShortcut : public Shortcut {
public:
	CommandShortcut(const Shortcut& sc, long id);
	unsigned long getID() const {return _id;};
	void setID(unsigned long id) { _id = id;};
	const TCHAR * getCategory() const { return _category.c_str(); };
	const TCHAR * getShortcutName() const { return _shortcutName.c_str(); };

private :
	unsigned long _id;
	generic_string _category;
	generic_string _shortcutName;
};


class ScintillaKeyMap : public Shortcut {
public:
	ScintillaKeyMap(const Shortcut& sc, unsigned long scintillaKeyID, unsigned long id): Shortcut(sc), _scintillaKeyID(scintillaKeyID), _menuCmdID(id) {
		_keyCombos.clear();
		_keyCombos.push_back(_keyCombo);
		_keyCombo._key = 0;
		_size = 1;
	};
	unsigned long getScintillaKeyID() const {return _scintillaKeyID;};
	int getMenuCmdID() const {return _menuCmdID;};
	size_t toKeyDef(size_t index) const {
		KeyCombo kc = _keyCombos[index];
		size_t keymod = (kc._isCtrl ? SCMOD_CTRL : 0) | (kc._isAlt ? SCMOD_ALT : 0) | (kc._isShift ? SCMOD_SHIFT : 0);
		return keyTranslate(kc._key) + (keymod << 16);
	};

	KeyCombo getKeyComboByIndex(size_t index) const;
	void setKeyComboByIndex(int index, KeyCombo combo);
	void removeKeyComboByIndex(size_t index);
	void clearDups() {
		if (_size > 1)
			_keyCombos.erase(_keyCombos.begin()+1, _keyCombos.end());
		_size = 1;
	};
	int addKeyCombo(KeyCombo combo);
	bool isEnabled() const;
	size_t getSize() const;

	std::string toString() const;
	std::string toString(size_t index) const;

	intptr_t doDialog()
	{
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUTSCINT_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
    };

	//only compares the internal KeyCombos, nothing else
	friend inline bool operator==(const ScintillaKeyMap & a, const ScintillaKeyMap & b) {
		bool equal = a._size == b._size;
		if (!equal)
			return false;
		size_t i = 0;
		while (equal && (i < a._size))
		{
			equal = 
				(a._keyCombos[i]._isCtrl	== b._keyCombos[i]._isCtrl) && 
				(a._keyCombos[i]._isAlt		== b._keyCombos[i]._isAlt) && 
				(a._keyCombos[i]._isShift	== b._keyCombos[i]._isShift) && 
				(a._keyCombos[i]._key		== b._keyCombos[i]._key);
			++i;
		}
		return equal;
	};

	friend inline bool operator!=(const ScintillaKeyMap & a, const ScintillaKeyMap & b) {
		return !(a == b);
	};

private:
	unsigned long _scintillaKeyID;
	int _menuCmdID;
	std::vector<KeyCombo> _keyCombos;
	size_t _size;
	void applyToCurrentIndex();
	void validateDialog();
	void showCurrentSettings();
	void updateListItem(int index);
protected :
	intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};


class Window;
class ScintillaEditView;

struct recordedMacroStep {
	enum MacroTypeIndex {mtUseLParameter, mtUseSParameter, mtMenuCommand, mtSavedSnR};

	int _message = 0;
	uptr_t _wParameter = 0;
	uptr_t _lParameter = 0;
	std::string _sParameter;
	MacroTypeIndex _macroType = mtMenuCommand;
	
	recordedMacroStep(int iMessage, uptr_t wParam, uptr_t lParam);
	explicit recordedMacroStep(int iCommandID): _wParameter(iCommandID) {};

	recordedMacroStep(int iMessage, uptr_t wParam, uptr_t lParam, const char *sParam, int type)
		: _message(iMessage), _wParameter(wParam), _lParameter(lParam), _macroType(MacroTypeIndex(type)){
			_sParameter = (sParam) ? std::string(sParam) : "";	
	};

	bool isValid() const {
		return true;
	};
	bool isScintillaMacro() const {return _macroType <= mtMenuCommand;};
	bool isMacroable() const;

	void PlayBack(Window* pNotepad, ScintillaEditView *pEditView);
};

typedef std::vector<recordedMacroStep> Macro;

class MacroShortcut : public CommandShortcut {
friend class NppParameters;
public:
	MacroShortcut(const Shortcut& sc, const Macro& macro, int id) : CommandShortcut(sc, id), _macro(macro) {_canModifyName = true;};
	Macro & getMacro() {return _macro;};
private:
	Macro _macro;
};


class UserCommand : public CommandShortcut {
friend class NppParameters;
public:
	UserCommand(const Shortcut& sc, const char *cmd, int id) : CommandShortcut(sc, id), _cmd(cmd) {_canModifyName = true;};
	const char* getCmd() const {return _cmd.c_str();};
private:
	std::string _cmd;
};

class PluginCmdShortcut : public CommandShortcut {
//friend class NppParameters;
public:
	PluginCmdShortcut(const Shortcut& sc, int id, const char*moduleName, unsigned short internalID) :\
		CommandShortcut(sc, id), _id(id), _moduleName(moduleName), _internalID(internalID) {};
	bool isValid() const {
		if (!Shortcut::isValid())
			return false;
		if ((!_moduleName[0]) || (_internalID == -1))
			return false;
		return true;
	}
	const char* getModuleName() const {return _moduleName.c_str();};
	int getInternalID() const {return _internalID;};
	unsigned long getID() const {return _id;};

private :
	unsigned long _id;
	std::string _moduleName;
	int _internalID;
};

class Accelerator { //Handles accelerator keys for Notepad++ menu, including custom commands
friend class ShortcutMapper;
public:
	Accelerator() = default;
	~Accelerator() {
		if (_hAccTable)
			::DestroyAcceleratorTable(_hAccTable);
		if (_hIncFindAccTab)
			::DestroyAcceleratorTable(_hIncFindAccTab);
		if (_hFindAccTab)
			::DestroyAcceleratorTable(_hFindAccTab);
		delete [] _pAccelArray;
	};
	void init(HMENU hMenu, HWND menuParent) {
		_hAccelMenu = hMenu;
		_hMenuParent = menuParent;
		updateShortcuts();
	};
	HACCEL getAccTable() const {return _hAccTable;};
	HACCEL getIncrFindAccTable() const { return _hIncFindAccTab; };
	HACCEL getFindAccTable() const { return _hFindAccTab; };

	void updateShortcuts();
	void updateFullMenu();

private:
	HMENU _hAccelMenu = nullptr;
	HWND _hMenuParent = nullptr;
	HACCEL _hAccTable = nullptr;
	HACCEL _hIncFindAccTab = nullptr;
	HACCEL _hFindAccTab = nullptr;
	ACCEL *_pAccelArray = nullptr;
	int _nbAccelItems = 0;

	void updateMenuItemByCommand(const CommandShortcut& csc);
};

class ScintillaAccelerator {	//Handles accelerator keys for scintilla
public:
	ScintillaAccelerator() = default;
	void init(std::vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent);
	void updateKeys();
	size_t nbScintillas() { return _vScintillas.size(); };
private:
	HMENU _hAccelMenu = nullptr;
	HWND _hMenuParent = nullptr;
	std::vector<HWND> _vScintillas;

	void updateMenuItemByID(const ScintillaKeyMap& skm, int id);
};
