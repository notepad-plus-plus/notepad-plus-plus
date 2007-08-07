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

enum ShortcutType {TYPE_CMD, TYPE_MACRO, TYPE_USERCMD, TYPE_PLUGINCMD, TYPE_INVALID};

void getKeyStrFromVal(unsigned char keyVal, string & str);
ShortcutType getNameStrFromCmd(DWORD cmd, string & str);
static int keyTranslate(int keyIn) {
	switch (keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:		return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:	return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:	return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:	return SCK_RETURN;
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
struct Shortcut  : public StaticDialog {
	char _name[nameLenMax];
	bool _isCtrl;
	bool _isAlt;
	bool _isShift;
	unsigned char _key;
	bool _canModifyName;

	Shortcut():_isCtrl(false), _isAlt(false), _isShift(false), _key(0), _canModifyName(true) {_name[0] = '\0';};
	Shortcut(const Shortcut & shortcut) {
		this->_isCtrl = shortcut._isCtrl;
		this->_isAlt = shortcut._isAlt;
		this->_isShift = shortcut._isShift;
		this->_key = shortcut._key;
		strcpy(this->_name, shortcut._name);
	};
	Shortcut(const char *name, bool isCtrl, bool isAlt, bool isShift, unsigned char key) :\
		_isCtrl(isCtrl), _isAlt(isAlt), _isShift(isShift), _key(key){
		_name[0] = '\0';
		if (name)
			strcpy(_name, name);
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

	bool isValid() const {
		if (_key == 0)
			return false;
		// the following keys are always valid (NUMPAD & F1~F12 + ESC + HOME + END)
		if (((_key >= 0x60) && (_key <= 0x69)) || ((_key >= 0x70) && (_key <= 0x7B)) || (_key == 0x1B) || (_key == 0x24) || (_key == 0x23))
			return true;
		// the remain keys need at least Ctrl or Alt
		if ((_isCtrl) || (_isAlt))
			return true;
		return false;
	};
	void setNameReadOnly(bool canBeModified = false) {_canModifyName = canBeModified;};
	string toString() const;
	string toMenuItemString(int cmdID = 0) {
		string str = _name;
		if (cmdID)
			getNameStrFromCmd(cmdID, str);
		str += "\t";
		str += toString();
		return str;
	};
protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

};
		 
class CommandShortcut : public Shortcut {
public:
	CommandShortcut() : _id(0){};
	CommandShortcut(const char *name, unsigned long id, bool isCtrl, bool isAlt, bool isShift, unsigned char key) :\
		Shortcut(name, isCtrl, isAlt, isShift, key), _id(id) {};
	CommandShortcut(long id, Shortcut sc) :	Shortcut(sc), _id(id) {};
	unsigned long getID() const {return _id;};

protected :
	unsigned long _id;
};


class ScintillaKeyMap : public CommandShortcut {
public:
	ScintillaKeyMap():_scintillaKeyID(0), _menuCmdID(0){};
	ScintillaKeyMap(unsigned long id): _scintillaKeyID(0), _menuCmdID(0){ _id = id;};
	ScintillaKeyMap(const char *name, unsigned long id, unsigned long scintillaKeyID,\
		            bool isCtrl, bool isAlt, bool isShift, unsigned char key, int cmdID = 0) :\
		CommandShortcut(name, id, isCtrl, isAlt, isShift, key), _scintillaKeyID(scintillaKeyID), _menuCmdID(cmdID){};
	unsigned long getScintillaKeyID() const {return _scintillaKeyID;};
	int toKeyDef() const {
		int keymod = (_isCtrl?SCMOD_CTRL:0) | (_isAlt?SCMOD_ALT:0) | (_isShift?SCMOD_SHIFT:0);
		return keyTranslate((int)_key) + (keymod << 16);
	};
	unsigned long getScintillaKey() const {return _scintillaKeyID;};
	int getMenuCmdID() const {return _menuCmdID;};
	void setScintKey(int key) {_scintillaKeyID = key;};
	void setMenuID(int id) {_menuCmdID = id;};

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

class MacroShortcut : public Shortcut {
friend class NppParameters;
public:
	MacroShortcut(Shortcut sc) : Shortcut(sc) {};
	MacroShortcut(Macro macro) : _macro(macro) {};
	MacroShortcut(Shortcut sc, Macro macro) : Shortcut(sc), _macro(macro) {};
	Macro & getMacro() {return _macro;};
private:
	Macro _macro;
};


class UserCommand : public Shortcut {
friend class NppParameters;
public:
	UserCommand(Shortcut sc) : Shortcut(sc) {};
	UserCommand(char *cmd) : _cmd(cmd) {};
	UserCommand(Shortcut sc, char *cmd) : Shortcut(sc), _cmd(cmd) {};
	const char* getCmd() const {return _cmd.c_str();};
private:
	string _cmd;
};

class PluginCmdShortcut : public Shortcut {
friend class NppParameters;
public:
	PluginCmdShortcut(Shortcut sc) : Shortcut(sc), _id(0), _internalID(-1) {_moduleName[0] = '\0';};
	PluginCmdShortcut(Shortcut sc, int cmdID) : Shortcut(sc), _id(cmdID), _internalID(-1) {_moduleName[0] = '\0';};
	PluginCmdShortcut(Shortcut sc, int cmdID, const char *moduleName, unsigned short internalID) :\
		Shortcut(sc), _id(cmdID), _internalID(internalID) {
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

class Accelerator {
friend class ShortcutMapper;
public:
	Accelerator():_hAccTable(NULL), _didCopy(false), _pAccelArray(NULL), _nbAccelItems(0){
	};
	~Accelerator(){
		if (_didCopy)
			::DestroyAcceleratorTable(_hAccTable);
	};
	void init(HACCEL hAccel) {
		_hAccTable = hAccel;
		_nbOriginalAccelItem = ::CopyAcceleratorTable(_hAccTable, NULL, 0);
	};
	HACCEL getAccTable() const {return _hAccTable;};

	bool uptdateShortcuts(HWND nppHandle = NULL);

	void coloneAccelTable() {
		copyAccelArray();
		reNew();
	};


private:
	HACCEL _hAccTable;
	bool _didCopy;

	ACCEL *_pAccelArray;
	int _nbOriginalAccelItem;
	int _nbAccelItems;

	
	size_t copyAccelArray(int nbMacro2add = 0, int nbUserCmd2add = 0, int nbPluginCmd2add = 0) {
		int newSize = _nbOriginalAccelItem + nbMacro2add + nbUserCmd2add + nbPluginCmd2add;
		_nbAccelItems = newSize;

		if (_pAccelArray)
			delete [] _pAccelArray;
		_pAccelArray = new ACCEL[_nbAccelItems];

		::CopyAcceleratorTable(_hAccTable, _pAccelArray, _nbOriginalAccelItem);
		return newSize;
	};

	void reNew() {
		if (!_didCopy)
			_didCopy = true;
		else
			::DestroyAcceleratorTable(_hAccTable);

		_hAccTable = ::CreateAcceleratorTable(_pAccelArray, _nbAccelItems);
	};
};

#endif //SHORTCUTS_H
