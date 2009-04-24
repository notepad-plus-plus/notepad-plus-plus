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

#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#include "resource.h"
#include "Parameters.h"
#include "PluginInterface.h"

typedef BOOL (__cdecl * PFUNCISUNICODE)();

struct PluginCommand {
	TCHAR _pluginName[64];
	int _funcID;
	PFUNCPLUGINCMD _pFunc;
	PluginCommand(const TCHAR *pluginName, int funcID, PFUNCPLUGINCMD pFunc): _funcID(funcID), _pFunc(pFunc){
		lstrcpy(_pluginName, pluginName);
	};
};

struct PluginInfo {
	PluginInfo() :_hLib(NULL), _pluginMenu(NULL), _pFuncSetInfo(NULL),\
		_pFuncGetFuncsArray(NULL), _pFuncGetName(NULL), _funcItems(NULL),\
		_nbFuncItem(0){};
	~PluginInfo(){
		if (_pluginMenu)
			::DestroyMenu(_pluginMenu);

		if (_hLib)
			::FreeLibrary(_hLib);
	};

	HINSTANCE _hLib;
	HMENU _pluginMenu;

	PFUNCSETINFO _pFuncSetInfo;
	PFUNCGETNAME _pFuncGetName;
	PBENOTIFIED	_pBeNotified;
	PFUNCGETFUNCSARRAY _pFuncGetFuncsArray;
	PMESSAGEPROC _pMessageProc;
	PFUNCISUNICODE _pFuncIsUnicode;
	
	FuncItem *_funcItems;
	int _nbFuncItem;
	TCHAR _moduleName[64];
};

class PluginsManager {
public:
	PluginsManager() : _hPluginsMenu(NULL), _isDisabled(false) {};
	~PluginsManager() {
		
		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
			delete _pluginInfos[i];

		if (_hPluginsMenu)
			DestroyMenu(_hPluginsMenu);
	};
	void init(const NppData & nppData) {
		_nppData = nppData;
	};
	bool loadPlugins(const TCHAR *dir = NULL);
	
	void runPluginCommand(size_t i) {
		if (i < _pluginsCommands.size())
			if (_pluginsCommands[i]._pFunc != NULL)
				_pluginsCommands[i]._pFunc();
	};

	void runPluginCommand(const TCHAR *pluginName, int commandID) {
		for (size_t i = 0 ; i < _pluginsCommands.size() ; i++)
		{
			if (!generic_stricmp(_pluginsCommands[i]._pluginName, pluginName))
			{
				if (_pluginsCommands[i]._funcID == commandID)
					_pluginsCommands[i]._pFunc();
			}
		}
	};

	void setMenu(HMENU hMenu, const TCHAR *menuName);
	bool getShortcutByCmdID(int cmdID, ShortcutKey *sk);

	void notify(SCNotification *notification) {
		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			// To avoid the plugin change the data in SCNotification
			// Each notification to pass to a plugin is a copy of SCNotification instance
			SCNotification scNotif = *notification;
			_pluginInfos[i]->_pBeNotified(&scNotif);
		}
	};

	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam) {
		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			_pluginInfos[i]->_pMessageProc(Message, wParam, lParam);
		}
	};

	bool relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam) {
		const TCHAR * moduleName = (const TCHAR *)wParam;
		if (!moduleName || !moduleName[0] || !lParam)
			return false;

		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			if (generic_stricmp(_pluginInfos[i]->_moduleName, moduleName) == 0)
			{
				_pluginInfos[i]->_pMessageProc(Message, wParam, lParam);
				return true;
			}
		}
		return false;
	};

	HMENU getMenuHandle() {
		return _hPluginsMenu;
	};

	void disable() {_isDisabled = true;};
	bool hasPlugins(){return (_pluginInfos.size()!= 0);};

private:
	NppData _nppData;
	HMENU _hPluginsMenu;
		
	vector<PluginInfo *> _pluginInfos;
	vector<PluginCommand> _pluginsCommands;
	bool _isDisabled;
};

#define EXT_LEXER_DECL __stdcall

// External Lexer function definitions...
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef void (EXT_LEXER_DECL *GetLexerStatusTextFn)(unsigned int Index, TCHAR *desc, int buflength);

#endif //PLUGINSMANAGER_H
