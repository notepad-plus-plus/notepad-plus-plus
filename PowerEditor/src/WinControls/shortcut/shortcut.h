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


#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#ifndef IDD_SHORTCUT_DLG
#include "shortcutRc.h"
#endif //IDD_SHORTCUT_DLG

#ifndef SCINTILLA_H
#include "Scintilla.h"
#endif //SCINTILLA_H

#include "StaticDialog.h"
#include "Common.h"

const size_t nameLenMax = 64;

class NppParameters;

void getKeyStrFromVal(UCHAR keyVal, generic_string & str);
void getNameStrFromCmd(DWORD cmd, generic_string & str);
static int keyTranslate(int keyIn) {
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
};

struct KeyCombo {
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	UCHAR _key;
};

class Shortcut  : public StaticDialog {
public:
	Shortcut(): _canModifyName(false) {
		setName(TEXT(""));
		_keyCombo._isCtrl = false;
		_keyCombo._isAlt = false;
		_keyCombo._isShift = false;
		_keyCombo._key = 0;
	};

	Shortcut(const TCHAR *name, bool isCtrl, bool isAlt, bool isShift, UCHAR key) : _canModifyName(false) {
		_name[0] = '\0';
		if (name) {
			setName(name);
		} else {
			setName(TEXT(""));
		}
		_keyCombo._isCtrl = isCtrl;
		_keyCombo._isAlt = isAlt;
		_keyCombo._isShift = isShift;
		_keyCombo._key = key;
	};

	Shortcut(const Shortcut & sc) {
		setName(sc.getMenuName());
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
			setName(sc.getMenuName());
		}
		_keyCombo = sc._keyCombo;
		this->_canModifyName = sc._canModifyName;
		return *this;
	}
	friend inline const bool operator==(const Shortcut & a, const Shortcut & b) {
		return ((lstrcmp(a.getMenuName(), b.getMenuName()) == 0) && 
			(a._keyCombo._isCtrl == b._keyCombo._isCtrl) && 
			(a._keyCombo._isAlt == b._keyCombo._isAlt) && 
			(a._keyCombo._isShift == b._keyCombo._isShift) && 
			(a._keyCombo._key == b._keyCombo._key)
			);
	};

	friend inline const bool operator!=(const Shortcut & a, const Shortcut & b) {
		return !(a == b);
	};

	virtual int doDialog() {
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUT_DLG), _hParent,  dlgProc, (LPARAM)this);
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

	virtual generic_string toString() const;					//the hotkey part
	generic_string toMenuItemString() const {					//generic_string suitable for menu
		generic_string str = _menuName;
		if(isEnabled())
		{
			str += TEXT("\t");
			str += toString();
		}
		return str;
	};
	const KeyCombo & getKeyCombo() const {
		return _keyCombo;
	};

	const TCHAR * getName() const {
		return _name;
	};

	const TCHAR * getMenuName() const {
		return _menuName;
	}

	void setName(const TCHAR * name);

protected :
	KeyCombo _keyCombo;
	virtual INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	bool _canModifyName;
	TCHAR _name[nameLenMax];		//normal name is plain text (for display purposes)
	TCHAR _menuName[nameLenMax];	//menu name has ampersands for quick keys
};
		 
class CommandShortcut : public Shortcut {
public:
	CommandShortcut(Shortcut sc, long id) :	Shortcut(sc), _id(id) {};
	unsigned long getID() const {return _id;};
	void setID(unsigned long id) { _id = id;};

private :
	unsigned long _id;
};


class ScintillaKeyMap : public Shortcut {
public:
	ScintillaKeyMap(Shortcut sc, unsigned long scintillaKeyID, unsigned long id): Shortcut(sc), _menuCmdID(id), _scintillaKeyID(scintillaKeyID) {
		_keyCombos.clear();
		_keyCombos.push_back(_keyCombo);
		_keyCombo._key = 0;
		size = 1;
	};
	unsigned long getScintillaKeyID() const {return _scintillaKeyID;};
	int getMenuCmdID() const {return _menuCmdID;};
	int toKeyDef(int index) const {
		KeyCombo kc = _keyCombos[index];
		int keymod = (kc._isCtrl?SCMOD_CTRL:0) | (kc._isAlt?SCMOD_ALT:0) | (kc._isShift?SCMOD_SHIFT:0);
		return keyTranslate((int)kc._key) + (keymod << 16);
	};

	KeyCombo getKeyComboByIndex(int index) const;
	void setKeyComboByIndex(int index, KeyCombo combo);
	void removeKeyComboByIndex(int index);
	void clearDups() {
		if (size > 1)
			_keyCombos.erase(_keyCombos.begin()+1, _keyCombos.end());
		size = 1;
	};
	int addKeyCombo(KeyCombo combo);
	bool isEnabled() const;
	size_t getSize() const;

	generic_string toString() const;
	generic_string toString(int index) const;

	int doDialog() {
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUTSCINT_DLG), _hParent,  dlgProc, (LPARAM)this);
    };

	//only compares the internal KeyCombos, nothing else
	friend inline const bool operator==(const ScintillaKeyMap & a, const ScintillaKeyMap & b) {
		bool equal = a.size == b.size;
		if (!equal)
			return false;
		size_t i = 0;
		while(equal && (i < a.size)) {
			equal = 
				(a._keyCombos[i]._isCtrl	== b._keyCombos[i]._isCtrl) && 
				(a._keyCombos[i]._isAlt		== b._keyCombos[i]._isAlt) && 
				(a._keyCombos[i]._isShift	== b._keyCombos[i]._isShift) && 
				(a._keyCombos[i]._key		== b._keyCombos[i]._key);
			++i;
		}
		return equal;
	};

	friend inline const bool operator!=(const ScintillaKeyMap & a, const ScintillaKeyMap & b) {
		return !(a == b);
	};

private:
	unsigned long _scintillaKeyID;
	int _menuCmdID;
	std::vector<KeyCombo> _keyCombos;
	size_t size;
	void applyToCurrentIndex();
	void validateDialog();
	void showCurrentSettings();
	void updateListItem(int index);
protected :
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
};


class Window;
class ScintillaEditView;

struct recordedMacroStep {
	enum MacroTypeIndex {mtUseLParameter, mtUseSParameter, mtMenuCommand, mtSavedSnR};
	
	int message;
	long wParameter;
	long lParameter;
	generic_string sParameter;
	MacroTypeIndex MacroType;
	
	recordedMacroStep(int iMessage, long wParam, long lParam, int codepage);
	recordedMacroStep(int iCommandID) : message(0), wParameter(iCommandID), lParameter(0), MacroType(mtMenuCommand) {};

	recordedMacroStep(int iMessage, long wParam, long lParam, const TCHAR *sParam, int type)
		: message(iMessage), wParameter(wParam), lParameter(lParam), MacroType(MacroTypeIndex(type)){
			sParameter = (sParam)?generic_string(sParam):TEXT("");	
	};

	bool isValid() const {
		return true;
	};
	bool isPlayable() const {return MacroType <= mtMenuCommand;};

	void PlayBack(Window* pNotepad, ScintillaEditView *pEditView);
};

typedef std::vector<recordedMacroStep> Macro;

class MacroShortcut : public CommandShortcut {
friend class NppParameters;
public:
	MacroShortcut(Shortcut sc, Macro macro, int id) : CommandShortcut(sc, id), _macro(macro) {_canModifyName = true;};
	Macro & getMacro() {return _macro;};
private:
	Macro _macro;
};


class UserCommand : public CommandShortcut {
friend class NppParameters;
public:
	UserCommand(Shortcut sc, const TCHAR *cmd, int id) : CommandShortcut(sc, id), _cmd(cmd) {_canModifyName = true;};
	const TCHAR* getCmd() const {return _cmd.c_str();};
private:
	generic_string _cmd;
};

class PluginCmdShortcut : public CommandShortcut {
//friend class NppParameters;
public:
	PluginCmdShortcut(Shortcut sc, int id, const TCHAR *moduleName, unsigned short internalID) :\
		CommandShortcut(sc, id), _id(id), _moduleName(moduleName), _internalID(internalID) {};
	bool isValid() const {
		if (!Shortcut::isValid())
			return false;
		if ((!_moduleName[0]) || (_internalID == -1))
			return false;
		return true;
	}
	const TCHAR * getModuleName() const {return _moduleName.c_str();};
	int getInternalID() const {return _internalID;};
	unsigned long getID() const {return _id;};

private :
	unsigned long _id;
	generic_string _moduleName;
	int _internalID;
};

class Accelerator { //Handles accelerator keys for Notepad++ menu, including custom commands
friend class ShortcutMapper;
public:
	Accelerator() :_hAccelMenu(NULL), _hMenuParent(NULL), _hAccTable(NULL), _hIncFindAccTab(NULL), _pAccelArray(NULL), _nbAccelItems(0){};
	~Accelerator() {
		if (_hAccTable)
			::DestroyAcceleratorTable(_hAccTable);
		if (_hIncFindAccTab)
			::DestroyAcceleratorTable(_hIncFindAccTab);
		if (_pAccelArray)
			delete [] _pAccelArray;
	};
	void init(HMENU hMenu, HWND menuParent) {
		_hAccelMenu = hMenu;
		_hMenuParent = menuParent;
		updateShortcuts();
	};
	HACCEL getAccTable() const {return _hAccTable;};
	HACCEL getIncrFindAccTable() const { return _hIncFindAccTab; };

	void updateShortcuts();
	void updateFullMenu();

private:
	HMENU _hAccelMenu;
	HWND _hMenuParent;
	HACCEL _hAccTable;
	HACCEL _hIncFindAccTab;
	ACCEL *_pAccelArray;
	int _nbAccelItems;

	void updateMenuItemByCommand(CommandShortcut csc);
};

class ScintillaAccelerator {	//Handles accelerator keys for scintilla
public:
	ScintillaAccelerator() : _nrScintillas(0) {};
	void init(std::vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent);
	void updateKeys();
	void updateKey(ScintillaKeyMap skmOld, ScintillaKeyMap skm);
private:
	HMENU _hAccelMenu;
	HWND _hMenuParent;
	std::vector<HWND> _vScintillas;
	int _nrScintillas;

	void updateMenuItemByID(ScintillaKeyMap skm, int id);
};

#endif //SHORTCUTS_H
