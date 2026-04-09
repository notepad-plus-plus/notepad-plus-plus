// plugin_manager.mm — macOS plugin loading and management
// Loads .dylib plugins from ~/Library/Application Support/MacNote++/plugins/

#import <Cocoa/Cocoa.h>
#include "plugin_manager.h"
#include "handle_registry.h"

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <dlfcn.h>

// Command ID ranges from upstream resource.h
#define ID_PLUGINS_CMD                 22000
#define ID_PLUGINS_CMD_LIMIT           22999
#define ID_PLUGINS_CMD_DYNAMIC         23000
#define ID_PLUGINS_CMD_DYNAMIC_LIMIT   24999
#define MARKER_PLUGINS                 1
#define MARKER_PLUGINS_LIMIT           15
#define INDICATOR_PLUGINS              9
#define INDICATOR_PLUGINS_LIMIT        20

using PFUNCISUNICODE = BOOL (__cdecl*)();

// ============================================================
// MacPluginInfo
// ============================================================

MacPluginInfo::~MacPluginInfo()
{
	if (_pluginMenu)
		::DestroyMenu(_pluginMenu);
	if (_hLib)
		::FreeLibrary(_hLib);
}

// ============================================================
// Helpers
// ============================================================

static std::string wideToUTF8(const std::wstring& wide)
{
	if (wide.empty()) return "";
	NSString* str = [[NSString alloc] initWithBytes:wide.data()
	                                         length:wide.size() * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ? std::string([str UTF8String]) : "";
}

static std::wstring utf8ToWide(const char* utf8)
{
	if (!utf8) return L"";
	NSString* str = [NSString stringWithUTF8String:utf8];
	if (!str) return L"";
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	if (!data || data.length < sizeof(wchar_t)) return L"";
	return std::wstring(reinterpret_cast<const wchar_t*>(data.bytes),
	                    data.length / sizeof(wchar_t));
}

// Validate that a Mach-O binary matches the current architecture
static bool validateMachOArchitecture(const std::string& path)
{
	FILE* f = fopen(path.c_str(), "rb");
	if (!f)
		return false;

	uint32_t magic = 0;
	if (fread(&magic, sizeof(magic), 1, f) != 1)
	{
		fclose(f);
		return false;
	}

	bool valid = false;

#ifdef __arm64__
	cpu_type_t expectedCpu = CPU_TYPE_ARM64;
#else
	cpu_type_t expectedCpu = CPU_TYPE_X86_64;
#endif

	if (magic == MH_MAGIC_64)
	{
		// 64-bit Mach-O: read cputype from header
		fseek(f, 0, SEEK_SET);
		struct mach_header_64 header;
		if (fread(&header, sizeof(header), 1, f) == 1)
			valid = (header.cputype == expectedCpu);
	}
	else if (magic == FAT_MAGIC || magic == FAT_CIGAM)
	{
		// Universal binary: check if it contains our architecture
		fseek(f, 0, SEEK_SET);
		struct fat_header fh;
		if (fread(&fh, sizeof(fh), 1, f) == 1)
		{
			uint32_t nArch = (magic == FAT_CIGAM) ? OSSwapInt32(fh.nfat_arch) : fh.nfat_arch;
			for (uint32_t i = 0; i < nArch && !valid; ++i)
			{
				struct fat_arch arch;
				if (fread(&arch, sizeof(arch), 1, f) == 1)
				{
					cpu_type_t cpu = (magic == FAT_CIGAM) ? OSSwapInt32(arch.cputype) : arch.cputype;
					if (cpu == expectedCpu)
						valid = true;
				}
			}
		}
	}

	fclose(f);
	return valid;
}

// ============================================================
// MacPluginManager
// ============================================================

MacPluginManager::MacPluginManager()
	: _staticCmdAlloc(ID_PLUGINS_CMD, ID_PLUGINS_CMD_LIMIT)
	, _dynamicCmdAlloc(ID_PLUGINS_CMD_DYNAMIC, ID_PLUGINS_CMD_DYNAMIC_LIMIT)
	, _markerAlloc(MARKER_PLUGINS, MARKER_PLUGINS_LIMIT)
	, _indicatorAlloc(INDICATOR_PLUGINS, INDICATOR_PLUGINS_LIMIT + 1)
{
}

int MacPluginManager::loadPluginFromPath(const std::wstring& pluginFilePath)
{
	std::string utf8Path = wideToUTF8(pluginFilePath);

	// Extract filename from path
	NSString* nsPath = [NSString stringWithUTF8String:utf8Path.c_str()];
	NSString* fileName = [nsPath lastPathComponent];
	std::wstring wFileName = utf8ToWide([fileName UTF8String]);

	auto pi = std::make_unique<MacPluginInfo>();
	pi->_moduleName = wFileName;
	// Display name: strip .dylib extension
	NSString* displayName = [fileName stringByDeletingPathExtension];
	pi->_displayName = utf8ToWide([displayName UTF8String]);

	// Validate architecture
	if (!validateMachOArchitecture(utf8Path))
	{
		NSLog(@"Plugin %@ has incompatible architecture, skipping.", fileName);
		return -1;
	}

	// Load the dylib
	pi->_hLib = ::LoadLibraryExW(pluginFilePath.c_str(), nullptr, 0);
	if (!pi->_hLib)
	{
		NSLog(@"Failed to load plugin %@: dlopen error", fileName);
		return -1;
	}

	// Resolve isUnicode (optional check)
	auto pIsUnicode = reinterpret_cast<PFUNCISUNICODE>(::GetProcAddress(pi->_hLib, "isUnicode"));
	if (pIsUnicode && !pIsUnicode())
	{
		NSLog(@"Plugin %@ is ANSI-only, skipping.", fileName);
		return -1;
	}

	// Resolve required exports
	pi->_pFuncSetInfo = reinterpret_cast<PFUNCSETINFO>(::GetProcAddress(pi->_hLib, "setInfo"));
	if (!pi->_pFuncSetInfo)
	{
		NSLog(@"Plugin %@ missing setInfo export.", fileName);
		return -1;
	}

	pi->_pFuncGetName = reinterpret_cast<PFUNCGETNAME>(::GetProcAddress(pi->_hLib, "getName"));
	if (!pi->_pFuncGetName)
	{
		NSLog(@"Plugin %@ missing getName export.", fileName);
		return -1;
	}

	pi->_pBeNotified = reinterpret_cast<PBENOTIFIED>(::GetProcAddress(pi->_hLib, "beNotified"));
	if (!pi->_pBeNotified)
	{
		NSLog(@"Plugin %@ missing beNotified export.", fileName);
		return -1;
	}

	pi->_pMessageProc = reinterpret_cast<PMESSAGEPROC>(::GetProcAddress(pi->_hLib, "messageProc"));
	if (!pi->_pMessageProc)
	{
		NSLog(@"Plugin %@ missing messageProc export.", fileName);
		return -1;
	}

	pi->_pFuncGetFuncsArray = reinterpret_cast<PFUNCGETFUNCSARRAY>(::GetProcAddress(pi->_hLib, "getFuncsArray"));
	if (!pi->_pFuncGetFuncsArray)
	{
		NSLog(@"Plugin %@ missing getFuncsArray export.", fileName);
		return -1;
	}

	// Initialize plugin
	pi->_pFuncSetInfo(_nppData);

	// Get the plugin's display name
	const wchar_t* pluginName = pi->_pFuncGetName();
	if (pluginName && pluginName[0])
		pi->_displayName = pluginName;

	// Get function items
	pi->_funcItems = pi->_pFuncGetFuncsArray(&pi->_nbFuncItem);
	if (!pi->_funcItems || pi->_nbFuncItem <= 0)
	{
		NSLog(@"Plugin %@ returned no function items.", fileName);
		return -1;
	}

	// Allocate command IDs from the static range and register commands
	pi->_cmdIdBase = static_cast<int>(_pluginsCommands.size());
	for (int i = 0; i < pi->_nbFuncItem; ++i)
	{
		// Skip separators (null _pFunc) — upstream only assigns IDs to real commands
		if (!pi->_funcItems[i]._pFunc)
			continue;

		int cmdId = _staticCmdAlloc.allocate(1);
		if (cmdId < 0)
		{
			NSLog(@"Exhausted static plugin command IDs.");
			break;
		}
		pi->_funcItems[i]._cmdID = cmdId;

		PluginCommand cmd;
		cmd._pluginName = pi->_displayName;
		cmd._pFunc = pi->_funcItems[i]._pFunc;
		_pluginsCommands.push_back(cmd);
	}

	// Create plugin submenu
	pi->_pluginMenu = ::CreateMenu();

	NSLog(@"Loaded plugin: %@", [NSString stringWithUTF8String:wideToUTF8(pi->_displayName).c_str()]);

	_pluginInfos.push_back(std::move(pi));
	return static_cast<int>(_pluginInfos.size() - 1);
}

bool MacPluginManager::loadPlugins()
{
	@autoreleasepool {
		NSString* pluginDir = [@"~/Library/Application Support/MacNote++/plugins"
			stringByExpandingTildeInPath];

		NSFileManager* fm = [NSFileManager defaultManager];

		// Create directory if it doesn't exist
		[fm createDirectoryAtPath:pluginDir withIntermediateDirectories:YES attributes:nil error:nil];

		NSArray<NSString*>* contents = [fm contentsOfDirectoryAtPath:pluginDir error:nil];
		if (!contents)
			return false;

		for (NSString* item in contents)
		{
			NSString* itemPath = [pluginDir stringByAppendingPathComponent:item];
			BOOL isDir = NO;
			if ([fm fileExistsAtPath:itemPath isDirectory:&isDir] && isDir)
			{
				// Look for <Name>/<Name>.dylib
				NSString* dylibName = [item stringByAppendingPathExtension:@"dylib"];
				NSString* dylibPath = [itemPath stringByAppendingPathComponent:dylibName];

				if ([fm fileExistsAtPath:dylibPath])
				{
					std::wstring wPath = utf8ToWide([dylibPath UTF8String]);
					loadPluginFromPath(wPath);
				}
			}
		}
	}

	return !_pluginInfos.empty();
}

HMENU MacPluginManager::initMenu(HMENU hPluginsMenu)
{
	_hPluginsMenu = hPluginsMenu;
	if (!_hPluginsMenu)
		return nullptr;

	for (auto& pi : _pluginInfos)
	{
		// Create per-plugin submenu
		HMENU pluginSub = ::CreatePopupMenu();
		pi->_pluginMenu = pluginSub;

		for (int i = 0; i < pi->_nbFuncItem; ++i)
		{
			FuncItem& fi = pi->_funcItems[i];
			if (!fi._pFunc)
			{
				// Null _pFunc indicates a separator (upstream convention)
				::AppendMenuW(pluginSub, MF_SEPARATOR, 0, nullptr);
			}
			else
			{
				UINT flags = MF_STRING;
				if (fi._init2Check)
					flags |= MF_CHECKED;
				::AppendMenuW(pluginSub, flags, static_cast<UINT_PTR>(fi._cmdID), fi._itemName);
			}
		}

		// Add plugin submenu to main Plugins menu
		::AppendMenuW(_hPluginsMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(pluginSub),
		              pi->_displayName.c_str());
	}

	return _hPluginsMenu;
}

void MacPluginManager::runPluginCommand(int index)
{
	if (index < 0 || index >= static_cast<int>(_pluginsCommands.size()))
		return;

	auto& cmd = _pluginsCommands[index];
	if (cmd._pFunc)
	{
		try
		{
			cmd._pFunc();
		}
		catch (...)
		{
			NSLog(@"Plugin command crashed: %@",
			      [NSString stringWithUTF8String:wideToUTF8(cmd._pluginName).c_str()]);
		}
	}
}

void MacPluginManager::notify(const SCNotification* notification)
{
	if (_noMoreNotification)
		return;

	if (notification->nmhdr.code == NPPN_SHUTDOWN)
		_noMoreNotification = true;

	for (auto& pi : _pluginInfos)
	{
		if (pi->_hLib && pi->_pBeNotified)
		{
			try
			{
				pi->_pBeNotified(const_cast<SCNotification*>(notification));
			}
			catch (...)
			{
				NSLog(@"Plugin beNotified crashed: %@",
				      [NSString stringWithUTF8String:wideToUTF8(pi->_moduleName).c_str()]);
			}
		}
	}
}

void MacPluginManager::relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam)
{
	for (auto& pi : _pluginInfos)
	{
		if (pi->_hLib && pi->_pMessageProc)
		{
			try
			{
				pi->_pMessageProc(Message, wParam, lParam);
			}
			catch (...)
			{
				NSLog(@"Plugin messageProc crashed: %@",
				      [NSString stringWithUTF8String:wideToUTF8(pi->_moduleName).c_str()]);
			}
		}
	}
}

bool MacPluginManager::allocateCmdID(int numberRequired, int* start)
{
	*start = _dynamicCmdAlloc.allocate(numberRequired);
	return (*start >= 0);
}

bool MacPluginManager::allocateMarker(int numberRequired, int* start)
{
	*start = _markerAlloc.allocate(numberRequired);
	return (*start >= 0);
}

bool MacPluginManager::allocateIndicator(int numberRequired, int* start)
{
	*start = _indicatorAlloc.allocate(numberRequired);
	return (*start >= 0);
}

// ============================================================
// Singleton
// ============================================================

MacPluginManager& pluginManager()
{
	static MacPluginManager instance;
	return instance;
}
