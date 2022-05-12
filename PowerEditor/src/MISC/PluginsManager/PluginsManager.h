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

#include "resource.h"
#include "Parameters.h"
#include "PluginInterface.h"
#include "IDAllocator.h"

typedef BOOL (__cdecl * PFUNCISUNICODE)();
class PluginViewList;

struct PluginCommand
{
	generic_string _pluginName;
	int _funcID = 0;
	PFUNCPLUGINCMD _pFunc = nullptr;
	PluginCommand(const TCHAR *pluginName, int funcID, PFUNCPLUGINCMD pFunc): _funcID(funcID), _pFunc(pFunc), _pluginName(pluginName){};
};

struct PluginInfo
{
	PluginInfo() = default;
	~PluginInfo()
	{
		if (_pluginMenu)
			::DestroyMenu(_pluginMenu);

		if (_hLib)
			::FreeLibrary(_hLib);
	}

	HINSTANCE _hLib = nullptr;
	HMENU _pluginMenu = nullptr;

	PFUNCSETINFO _pFuncSetInfo = nullptr;
	PFUNCGETNAME _pFuncGetName = nullptr;
	PBENOTIFIED	_pBeNotified = nullptr;
	PFUNCGETFUNCSARRAY _pFuncGetFuncsArray = nullptr;
	PMESSAGEPROC _pMessageProc = nullptr;
	PFUNCISUNICODE _pFuncIsUnicode = nullptr;

	FuncItem *_funcItems = nullptr;
	int _nbFuncItem = 0;
	generic_string _moduleName;
	generic_string _funcName;
};

struct LoadedDllInfo
{
	generic_string _fullFilePath;
	generic_string _fileName;
	generic_string _displayName;

	LoadedDllInfo(const generic_string & fullFilePath, const generic_string & fileName) : _fullFilePath(fullFilePath), _fileName(fileName)
	{
		// the plugin module's name, without '.dll'
		_displayName = fileName.substr(0, fileName.find_last_of('.'));
	};
};

class PluginsManager
{
friend class PluginsAdminDlg;
public:
	PluginsManager() : _dynamicIDAlloc(ID_PLUGINS_CMD_DYNAMIC, ID_PLUGINS_CMD_DYNAMIC_LIMIT),
					   _markerAlloc(MARKER_PLUGINS, MARKER_PLUGINS_LIMIT)	{}
	~PluginsManager()
	{
		for (size_t i = 0, len = _pluginInfos.size(); i < len; ++i)
			delete _pluginInfos[i];
	}

	void init(const NppData & nppData)
	{
		_nppData = nppData;
	}

	bool loadPlugins(const TCHAR *dir = NULL, const PluginViewList* pluginUpdateInfoList = nullptr);

    bool unloadPlugin(int index, HWND nppHandle);

	void runPluginCommand(size_t i);
	void runPluginCommand(const TCHAR *pluginName, int commandID);

    void addInMenuFromPMIndex(int i);
	HMENU initMenu(HMENU hMenu, bool enablePluginAdmin = false);
	bool getShortcutByCmdID(int cmdID, ShortcutKey *sk);
	bool removeShortcutByCmdID(int cmdID);

	void notify(size_t indexPluginInfo, const SCNotification *notification); // to a plugin
	void notify(const SCNotification *notification); // broadcast
	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam);
	bool relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam);

	HMENU getMenuHandle() const { return _hPluginsMenu; }

	void disable() {_isDisabled = true;}
	bool hasPlugins() {return (_pluginInfos.size()!= 0);}

	bool allocateCmdID(int numberRequired, int *start);
	bool inDynamicRange(int id) { return _dynamicIDAlloc.isInRange(id); }

	bool allocateMarker(int numberRequired, int *start);
	generic_string getLoadedPluginNames() const;

private:
	NppData _nppData;
	HMENU _hPluginsMenu = NULL;

	std::vector<PluginInfo *> _pluginInfos;
	std::vector<PluginCommand> _pluginsCommands;
	std::vector<LoadedDllInfo> _loadedDlls;
	bool _isDisabled = false;
	IDAllocator _dynamicIDAlloc;
	IDAllocator _markerAlloc;
	bool _noMoreNotification = false;

	int loadPluginFromPath(const TCHAR* pluginFilePath);

	void pluginCrashAlert(const TCHAR *pluginName, const TCHAR *funcSignature)
	{
		generic_string msg = pluginName;
		msg += TEXT(" just crashed in\r");
		msg += funcSignature;
		::MessageBox(NULL, msg.c_str(), TEXT("Plugin Crash"), MB_OK|MB_ICONSTOP);
	}

	void pluginExceptionAlert(const TCHAR *pluginName, const std::exception& e)
	{
		generic_string msg = TEXT("An exception occurred due to plugin: ");
		msg += pluginName;
		msg += TEXT("\r\n\r\nException reason: ");
		msg += s2ws(e.what());

		::MessageBox(NULL, msg.c_str(), TEXT("Plugin Exception"), MB_OK);
	}

	bool isInLoadedDlls(const TCHAR *fn) const
	{
		for (size_t i = 0; i < _loadedDlls.size(); ++i)
			if (generic_stricmp(fn, _loadedDlls[i]._fileName.c_str()) == 0)
				return true;
		return false;
	}

	void addInLoadedDlls(const TCHAR *fullPath, const TCHAR *fn) {
		_loadedDlls.push_back(LoadedDllInfo(fullPath, fn));
	}
};
