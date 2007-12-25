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

struct PluginCommand {
	char _pluginName[64];
	int _funcID;
	PFUNCPLUGINCMD _pFunc;
	PluginCommand(const char *pluginName, int funcID, PFUNCPLUGINCMD pFunc): _funcID(funcID), _pFunc(pFunc){
		strcpy(_pluginName, pluginName);
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
	
	FuncItem *_funcItems;
	int _nbFuncItem;
	char _moduleName[64];
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
	bool loadPlugins(const char *dir = NULL);
	
	void runPluginCommand(size_t i) {
		if (i < _pluginsCommands.size())
			_pluginsCommands[i]._pFunc();
	};

	void runPluginCommand(const char *pluginName, int commandID) {
		for (size_t i = 0 ; i < _pluginsCommands.size() ; i++)
		{
			if (!stricmp(_pluginsCommands[i]._pluginName, pluginName))
			{
				if (_pluginsCommands[i]._funcID == commandID)
					_pluginsCommands[i]._pFunc();
			}
		}
	};

	void setMenu(HMENU hMenu, const char *menuName);

	void notify(SCNotification *notification) {
		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			_pluginInfos[i]->_pBeNotified(notification);
		}
	};

	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam) {
		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			_pluginInfos[i]->_pMessageProc(Message, wParam, lParam);
		}
	};

	bool relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam) {
		const char * moduleName = (const char *)wParam;
		if (!moduleName || !moduleName[0] || !lParam)
			return false;

		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			if (stricmp(_pluginInfos[i]->_moduleName, moduleName) == 0)
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

	void getCustomizedShortcuts(char *pluginName, FuncItem *funcItems, int nbFuncItem) {
		vector<PluginCmdShortcut> & pluginCustomizedCmds = (NppParameters::getInstance())->getPluginCustomizedCmds();

		for (size_t i = 0 ; i < pluginCustomizedCmds.size() ; i++)
		{
			if (strcmp(pluginName, pluginCustomizedCmds[i].getModuleName()) == 0)
			{
				int id = pluginCustomizedCmds[i].getInternalID();
				if ((id < nbFuncItem) && (funcItems[id]._pShKey != NULL))
				{
					funcItems[id]._pShKey->_isAlt = pluginCustomizedCmds[i]._isAlt;
					funcItems[id]._pShKey->_isCtrl = pluginCustomizedCmds[i]._isCtrl;
					funcItems[id]._pShKey->_isShift = pluginCustomizedCmds[i]._isShift;
					funcItems[id]._pShKey->_key = pluginCustomizedCmds[i]._key;
				}
			}
		}
	};
	void disable() {_isDisabled = true;};

private:
	NppData _nppData;
	
	HMENU _hPluginsMenu;
		
	vector<PluginInfo *> _pluginInfos;
	vector<PluginCommand> _pluginsCommands;
	bool _isDisabled;

	bool hasPlugins(){return (_pluginInfos.size()!= 0);};
};

#define EXT_LEXER_DECL __stdcall

// External Lexer function definitions...
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef void (EXT_LEXER_DECL *GetLexerStatusTextFn)(unsigned int Index, char *desc, int buflength);

#endif //PLUGINSMANAGER_H
