// plugin_manager.h — macOS plugin loading and management
// Source-code-compatible plugin system for MacNote++.
// Loads .dylib plugins using the same PluginInterface.h contract as Notepad++.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "windows.h"
#include "PluginInterface.h"
#include "IDAllocator.h"

struct MacPluginInfo
{
	MacPluginInfo() = default;
	~MacPluginInfo();

	HINSTANCE _hLib = nullptr;
	HMENU _pluginMenu = nullptr;

	PFUNCSETINFO _pFuncSetInfo = nullptr;
	PFUNCGETNAME _pFuncGetName = nullptr;
	PBENOTIFIED _pBeNotified = nullptr;
	PFUNCGETFUNCSARRAY _pFuncGetFuncsArray = nullptr;
	PMESSAGEPROC _pMessageProc = nullptr;

	FuncItem* _funcItems = nullptr;
	int _nbFuncItem = 0;
	std::wstring _moduleName;
	std::wstring _displayName;

	// Index of first command in the global _pluginsCommands vector
	int _cmdIdBase = 0;
};

struct PluginCommand
{
	std::wstring _pluginName;
	PFUNCPLUGINCMD _pFunc = nullptr;
};

class MacPluginManager
{
public:
	MacPluginManager();
	~MacPluginManager() = default;

	void init(const NppData& nppData) { _nppData = nppData; }

	bool loadPlugins();
	int loadPluginFromPath(const std::wstring& pluginFilePath);

	HMENU initMenu(HMENU hPluginsMenu);

	void runPluginCommand(int index);

	void notify(const SCNotification* notification);
	void relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam);

	bool inDynamicRange(int id) const { return _dynamicCmdAlloc.isInRange(id); }

	bool allocateCmdID(int numberRequired, int* start);
	bool allocateMarker(int numberRequired, int* start);
	bool allocateIndicator(int numberRequired, int* start);

	bool hasPlugins() const { return !_pluginInfos.empty(); }

private:
	NppData _nppData{};
	HMENU _hPluginsMenu = nullptr;

	std::vector<std::unique_ptr<MacPluginInfo>> _pluginInfos;
	std::vector<PluginCommand> _pluginsCommands;

	IDAllocator _staticCmdAlloc;
	IDAllocator _dynamicCmdAlloc;
	IDAllocator _markerAlloc;
	IDAllocator _indicatorAlloc;

	bool _noMoreNotification = false;
};

// Singleton accessor
MacPluginManager& pluginManager();
