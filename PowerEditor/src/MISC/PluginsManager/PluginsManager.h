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
	std::wstring _pluginName;
	int _funcID = 0;
	PFUNCPLUGINCMD _pFunc = nullptr;
	PluginCommand(const wchar_t *pluginName, int funcID, PFUNCPLUGINCMD pFunc): _pluginName(pluginName), _funcID(funcID), _pFunc(pFunc) {};
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
	std::wstring _moduleName;
	std::wstring _funcName;
};

struct LoadedDllInfo
{
	std::wstring _fullFilePath;
	std::wstring _fileName;
	std::wstring _displayName;

	LoadedDllInfo(const std::wstring & fullFilePath, const std::wstring & fileName) : _fullFilePath(fullFilePath), _fileName(fileName)
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
					   _markerAlloc(MARKER_PLUGINS, MARKER_PLUGINS_LIMIT),
					   _indicatorAlloc(INDICATOR_PLUGINS, INDICATOR_PLUGINS_LIMIT + 1)	{}
	~PluginsManager()
	{
		for (size_t i = 0, len = _pluginInfos.size(); i < len; ++i)
			delete _pluginInfos[i];
	}

	void init(const NppData & nppData)
	{
		_nppData = nppData;
	}

	bool loadPlugins(const wchar_t *dir = NULL, const PluginViewList* pluginUpdateInfoList = nullptr, PluginViewList* pluginImcompatibleList = nullptr);

	void runPluginCommand(size_t i);
	void runPluginCommand(const wchar_t *pluginName, int commandID);

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

	bool allocateMarker(int numberRequired, int* start);
	bool allocateIndicator(int numberRequired, int* start);
	std::wstring getLoadedPluginNames() const;

private:
	NppData _nppData;
	HMENU _hPluginsMenu = NULL;

	std::vector<PluginInfo *> _pluginInfos;
	std::vector<PluginCommand> _pluginsCommands;
	std::vector<LoadedDllInfo> _loadedDlls;
	bool _isDisabled = false;
	IDAllocator _dynamicIDAlloc;
	IDAllocator _markerAlloc;
	IDAllocator _indicatorAlloc;
	bool _noMoreNotification = false;

	int loadPluginFromPath(const wchar_t* pluginFilePath);

	void pluginCrashAlert(const wchar_t *pluginName, const wchar_t *funcSignature) {
		std::wstring msg = pluginName;
		msg += L" just crashed in\r";
		msg += funcSignature;
		::MessageBox(NULL, msg.c_str(), L"Plugin Crash", MB_OK|MB_ICONSTOP);
	}

	void pluginExceptionAlert(const wchar_t *pluginName, const std::exception& e) {
		std::wstring msg = L"An exception occurred due to plugin: ";
		msg += pluginName;
		msg += L"\r\n\r\nException reason: ";
		msg += string2wstring(e.what(), CP_UTF8);

		::MessageBox(NULL, msg.c_str(), L"Plugin Exception", MB_OK);
	}

	bool isInLoadedDlls(const wchar_t *fn) const {
		for (size_t i = 0; i < _loadedDlls.size(); ++i)
			if (wcsicmp(fn, _loadedDlls[i]._fileName.c_str()) == 0)
				return true;
		return false;
	}

	void addInLoadedDlls(const wchar_t *fullPath, const wchar_t *fn) {
		_loadedDlls.push_back(LoadedDllInfo(fullPath, fn));
	}
};
