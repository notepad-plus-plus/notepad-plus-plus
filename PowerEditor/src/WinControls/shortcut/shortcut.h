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
#ifndef SHORTCUTS_H
#define SHORTCUTS_H

//#include "Parameters.h"
#include <vector>
#include <string>
#include <windows.h>
#include "shortcutRc.h"
#include "StaticDialog.h"
#include "Scintilla.h"

using namespace std;

const size_t nameLenMax = 64;

class NppParameters;

void getKeyStrFromVal(unsigned char keyVal, string & str);
void getNameStrFromCmd(DWORD cmd, string & str);
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

class Shortcut  : public StaticDialog {
public:
	char _name[nameLenMax];
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	unsigned char _key;
	bool _canModifyName;

	Shortcut():_isCtrl(false), _isAlt(false), _isShift(false), _key(0), _canModifyName(false) {_name[0] = '\0';};
	Shortcut(const Shortcut & shortcut) {
		this->_isCtrl = shortcut._isCtrl;
		this->_isAlt = shortcut._isAlt;
		this->_isShift = shortcut._isShift;
		this->_key = shortcut._key;
		strcpy(this->_name, shortcut._name);
		this->_canModifyName = shortcut._canModifyName;
	};
	Shortcut(const char *name, bool isCtrl, bool isAlt, bool isShift, unsigned char key) :\
		_isCtrl(isCtrl), _isAlt(isAlt), _isShift(isShift), _key(key) {
		_name[0] = '\0';
		if (name)
			strcpy(_name, name);
		this->_canModifyName = false;
	};

	friend inline const bool operator==(const Shortcut & a, const Shortcut & b) {
		return ((strcmp(a._name, b._name) == 0) && (a._isCtrl == b._isCtrl) && (a._isAlt == b._isAlt) && (a._isShift == b._isShift) && (a._key == b._key));
	};

	friend inline const bool operator!=(const Shortcut & a, const Shortcut & b) {
		return !((strcmp(a._name, b._name) == 0) && (a._isCtrl == b._isCtrl) && (a._isAlt == b._isAlt) && (a._isShift == b._isShift) && (a._key == b._key));
	};

	void copyShortcut(const Shortcut & sc) {
        if (this != &sc)
        {
			strcpy(this->_name, sc._name);
			this->_isAlt = sc._isAlt;
			this->_isCtrl = sc._isCtrl;
			this->_isShift = sc._isShift;
			this->_key = sc._key;
			this->_canModifyName = sc._canModifyName;
        }
    };

	int doDialog() {
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SHORTCUT_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
    };

	bool isValid() const { //valid should only be used in cases where the shortcut isEnabled().
		if (_key == 0)
			return true;	//disabled key always valid, just disabled

		//These keys need a modifier, else invalid
		if ( ((_key >= 'A') && (_key <= 'Z')) || ((_key >= '0') && (_key <= '9')) || _key == VK_SPACE || _key == VK_CAPITAL || _key == VK_BACK || _key == VK_RETURN) {
			return ((_isCtrl) || (_isAlt));
		}
		// the remaining keys are always valid
		return true;
	};
	bool isEnabled() const {	//true if key != 0, false if key == 0, in which case no accelerator should be made
		return (_key != 0);
	};

	string toString() const;					//the hotkey part
	string toMenuItemString(int cmdID = 0) {	//string suitable for menu, uses menu to retrieve name if command is specified
		string str = _name;
		if(isEnabled()) 
		{
			str += "\t";
			str += toString();
		}
		return str;
	};
protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

};
		 
class CommandShortcut : public Shortcut {
public:
	CommandShortcut(Shortcut sc, long id) :	Shortcut(sc), _id(id) {};
	unsigned long getID() const {return _id;};
	void setID(unsigned long id) { _id = id;};

protected :
	unsigned long _id;
};


class ScintillaKeyMap : public Shortcut {
public:
	ScintillaKeyMap(Shortcut sc, unsigned long scintillaKeyID, unsigned long id): Shortcut(sc), _menuCmdID(id), _scintillaKeyID(scintillaKeyID) {};
	unsigned long getScintillaKeyID() const {return _scintillaKeyID;};
	int getMenuCmdID() const {return _menuCmdID;};
	int toKeyDef() const {
		int keymod = (_isCtrl?SCMOD_CTRL:0) | (_isAlt?SCMOD_ALT:0) | (_isShift?SCMOD_SHIFT:0);
		return keyTranslate((int)_key) + (keymod << 16);
	};

private:
	unsigned long _scintillaKeyID;
	int _menuCmdID;
};


class Window;
class ScintillaEditView;

struct recordedMacroStep {
	enum MacroTypeIndex {mtUseLParameter, mtUseSParameter, mtMenuCommand};
	
	int message;
	long wParameter;
	long lParameter;
	string sParameter;
	MacroTypeIndex MacroType;
	
	recordedMacroStep(int iMessage, long wParam, long lParam);
	recordedMacroStep(int iCommandID) : message(0), wParameter(iCommandID), lParameter(0), MacroType(mtMenuCommand) {};

	recordedMacroStep(int type, int iMessage, long wParam, long lParam, const char *sParam)
		: message(iMessage), wParameter(wParam), lParameter(lParam), MacroType(MacroTypeIndex(type)){
		sParameter = *reinterpret_cast<const char *>(sParam);	
	};
	bool isValid() const {
		return true;
	};

	void PlayBack(Window* pNotepad, ScintillaEditView *pEditView);
};

typedef vector<recordedMacroStep> Macro;

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
	UserCommand(Shortcut sc, const char *cmd, int id) : CommandShortcut(sc, id), _cmd(cmd) {_canModifyName = true;};
	const char* getCmd() const {return _cmd.c_str();};
private:
	string _cmd;
};

class PluginCmdShortcut : public CommandShortcut {
friend class NppParameters;
public:
	PluginCmdShortcut(Shortcut sc, int id, const char *moduleName, unsigned short internalID) :\
		CommandShortcut(sc, id), _id(id), _internalID(internalID) {
		strcpy(_moduleName, moduleName);
	};
	bool isValid() const {
		if (!Shortcut::isValid())
			return false;
		if ((!_moduleName[0]) || (_internalID == -1))
			return false;
		return true;
	}
	const char * getModuleName() const {return _moduleName;};
	int getInternalID() const {return _internalID;};
	unsigned long getID() const {return _id;};

protected :
	unsigned long _id;
	char _moduleName[nameLenMax];
	int _internalID;
};

class Accelerator { //Handles accelerator keys for Notepad++ menu, including custom commands
friend class ShortcutMapper;
public:
	Accelerator():_hAccelMenu(NULL), _hMenuParent(NULL), _hAccTable(NULL), _didCopy(false), _pAccelArray(NULL), _nbAccelItems(0){};
	~Accelerator(){
		if (_didCopy)
			::DestroyAcceleratorTable(_hAccTable);
		if (_pAccelArray)
			delete [] _pAccelArray;
	};
	void init(HACCEL hAccel, HMENU hMenu, HWND menuParent) {
		_hAccTable = hAccel;
		_hAccelMenu = hMenu;
		_hMenuParent = menuParent;
		_nbOriginalAccelItem = ::CopyAcceleratorTable(_hAccTable, NULL, 0);
	};
	HACCEL getAccTable() const {return _hAccTable;};

	bool updateShortcuts(/*HWND nppHandle = NULL*/);
	//bool updateCommand(CommandShortcut & csc);
private:
	HMENU _hAccelMenu;
	HWND _hMenuParent;
	HACCEL _hAccTable;
	bool _didCopy;

	ACCEL *_pAccelArray;
	int _nbOriginalAccelItem;
	int _nbAccelItems;

	void reNew() {
		if (!_didCopy)
			_didCopy = true;
		else
			::DestroyAcceleratorTable(_hAccTable);

		_hAccTable = ::CreateAcceleratorTable(_pAccelArray, _nbAccelItems);
	};
	void updateFullMenu();
	void updateMenuItemByCommand(CommandShortcut csc);
};

class ScintillaAccelerator {	//Handles accelerator keys for scintilla
public:
	ScintillaAccelerator() : _nrScintillas(0) {};
	void init(vector<HWND> * vScintillas, HMENU hMenu, HWND menuParent);
	void updateKeys();
	void updateKey(ScintillaKeyMap skmOld, ScintillaKeyMap skm);

private:
	HMENU _hAccelMenu;
	HWND _hMenuParent;
	vector<HWND> _vScintillas;
	int _nrScintillas;

	void updateMenuItemByID(ScintillaKeyMap skm, int id);
};

#endif //SHORTCUTS_H
