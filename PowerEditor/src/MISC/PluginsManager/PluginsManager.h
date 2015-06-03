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


#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#ifndef RESOURCE_H
#include "resource.h"
#endif //RESOURCE_H

#ifndef PARAMETERS_H
#include "Parameters.h"
#endif //PARAMETERS_H

#ifndef PLUGININTERFACE_H
#include "PluginInterface.h"
#endif //PLUGININTERFACE_H

#ifndef IDALLOCATOR_H
#include "IDAllocator.h"
#endif // IDALLOCATOR_H

typedef BOOL (__cdecl * PFUNCISUNICODE)();

struct PluginCommand {
	generic_string _pluginName;
	int _funcID;
	PFUNCPLUGINCMD _pFunc;
	PluginCommand(const TCHAR *pluginName, int funcID, PFUNCPLUGINCMD pFunc): _funcID(funcID), _pFunc(pFunc), _pluginName(pluginName){};
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
	generic_string _moduleName;
};

class PluginsManager {
public:
	PluginsManager() : _hPluginsMenu(NULL), _isDisabled(false), _dynamicIDAlloc(ID_PLUGINS_CMD_DYNAMIC, ID_PLUGINS_CMD_DYNAMIC_LIMIT),
					   _markerAlloc(MARKER_PLUGINS, MARKER_PLUGINS_LIMIT)	{};
	~PluginsManager() {
		
		for (size_t i = 0, len = _pluginInfos.size(); i < len; ++i)
			delete _pluginInfos[i];

		if (_hPluginsMenu)
			DestroyMenu(_hPluginsMenu);
	};
	void init(const NppData & nppData) {
		_nppData = nppData;
	};

    int loadPlugin(const TCHAR *pluginFilePath, std::vector<generic_string> & dll2Remove);
	bool loadPlugins(const TCHAR *dir = NULL);
	
    bool unloadPlugin(int index, HWND nppHandle);

	void runPluginCommand(size_t i);
	void runPluginCommand(const TCHAR *pluginName, int commandID);

    void addInMenuFromPMIndex(int i);
	HMENU setMenu(HMENU hMenu, const TCHAR *menuName);
	bool getShortcutByCmdID(int cmdID, ShortcutKey *sk);

	void notify(const SCNotification *notification);
	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam);
	bool relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam);

	HMENU getMenuHandle() {
		return _hPluginsMenu;
	};

	void disable() {_isDisabled = true;};
	bool hasPlugins(){return (_pluginInfos.size()!= 0);};

	bool allocateCmdID(int numberRequired, int *start);
	bool inDynamicRange(int id) { return _dynamicIDAlloc.isInRange(id); }

	bool allocateMarker(int numberRequired, int *start);

private:
	NppData _nppData;
	HMENU _hPluginsMenu;

	std::vector<PluginInfo *> _pluginInfos;
	std::vector<PluginCommand> _pluginsCommands;
	std::vector<generic_string> _loadedDlls;
	bool _isDisabled;
	IDAllocator _dynamicIDAlloc;
	IDAllocator _markerAlloc;
	void pluginCrashAlert(const TCHAR *pluginName, const TCHAR *funcSignature) {
		generic_string msg = pluginName;
		msg += TEXT(" just crash in\r");
		msg += funcSignature;
		::MessageBox(NULL, msg.c_str(), TEXT(" just crash in\r"), MB_OK|MB_ICONSTOP);
	};
	bool isInLoadedDlls(const TCHAR *fn) const {
		for (size_t i = 0; i < _loadedDlls.size(); ++i)
			if (generic_stricmp(fn, _loadedDlls[i].c_str()) == 0)
				return true;
		return false;
	};

	void addInLoadedDlls(const TCHAR *fn) {
		_loadedDlls.push_back(fn);
	};
};

#define EXT_LEXER_DECL __stdcall

// External Lexer function definitions...
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef void (EXT_LEXER_DECL *GetLexerStatusTextFn)(unsigned int Index, TCHAR *desc, int buflength);

#endif //PLUGINSMANAGER_H
