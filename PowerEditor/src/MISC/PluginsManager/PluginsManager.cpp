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


#include <shlwapi.h>
#include <DbgHelp.h>
#include <algorithm>
#include "PluginsManager.h"
#include "resource.h"

using namespace std;

const TCHAR * USERMSG = TEXT(" is not compatible with the current version of Notepad++.\n\n\
Do you want to remove this plugin from the plugins directory to prevent this message from the next launch?");

#ifdef _WIN64
#define ARCH_TYPE IMAGE_FILE_MACHINE_AMD64
const TCHAR *ARCH_ERR_MSG = TEXT("Cannot load 32-bit plugin.");
#else
#define ARCH_TYPE IMAGE_FILE_MACHINE_I386
const TCHAR *ARCH_ERR_MSG = TEXT("Cannot load 64-bit plugin.");
#endif




bool PluginsManager::unloadPlugin(int index, HWND nppHandle)
{
    SCNotification scnN;
	scnN.nmhdr.code = NPPN_SHUTDOWN;
	scnN.nmhdr.hwndFrom = nppHandle;
	scnN.nmhdr.idFrom = 0;
	_pluginInfos[index]->_pBeNotified(&scnN);

    //::DestroyMenu(_pluginInfos[index]->_pluginMenu);
    //_pluginInfos[index]->_pluginMenu = NULL;

	if (::FreeLibrary(_pluginInfos[index]->_hLib))
	{
		_pluginInfos[index]->_hLib = nullptr;
		printStr(TEXT("we're good"));
	}
    else
        printStr(TEXT("not ok"));

    //delete _pluginInfos[index];
//      printInt(index);
    //vector<PluginInfo *>::iterator it = _pluginInfos.begin() + index;
    //_pluginInfos.erase(it);
    //printStr(TEXT("remove"));
    return true;
}

static WORD GetBinaryArchitectureType(const TCHAR *filePath)
{
	WORD machine_type = IMAGE_FILE_MACHINE_UNKNOWN;
	HANDLE hMapping = NULL;
	LPVOID addrHeader = NULL;

	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;

	hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
	if (hMapping == NULL)
		goto cleanup;

	addrHeader = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (addrHeader == NULL)
		goto cleanup; // couldn't memory map the file

	PIMAGE_NT_HEADERS peHdr = ImageNtHeader(addrHeader);
	if (peHdr == NULL)
		goto cleanup; // couldn't read the header

	// Found the binary and architecture type
	machine_type = peHdr->FileHeader.Machine;

cleanup: // release all of our handles
	if (addrHeader != NULL)
		UnmapViewOfFile(addrHeader);

	if (hMapping != NULL)
		CloseHandle(hMapping);

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return machine_type;
}

int PluginsManager::loadPlugin(const TCHAR *pluginFilePath, vector<generic_string> & dll2Remove)
{
	const TCHAR *pluginFileName = ::PathFindFileName(pluginFilePath);
	if (isInLoadedDlls(pluginFileName))
		return 0;

	NppParameters * nppParams = NppParameters::getInstance();

	PluginInfo *pi = new PluginInfo;
	try
	{
		pi->_moduleName = pluginFileName;

		if (GetBinaryArchitectureType(pluginFilePath) != ARCH_TYPE)
			throw generic_string(ARCH_ERR_MSG);

	    pi->_hLib = ::LoadLibrary(pluginFilePath);
        if (!pi->_hLib)
        {
			generic_string lastErrorMsg = GetLastErrorAsString();
            if (lastErrorMsg.empty())
                throw generic_string(TEXT("Load Library has failed.\nChanging the project's \"Runtime Library\" setting to \"Multi-threaded(/MT)\" might solve this problem."));
            else
                throw generic_string(lastErrorMsg.c_str());
        }
        
		pi->_pFuncIsUnicode = (PFUNCISUNICODE)GetProcAddress(pi->_hLib, "isUnicode");
		if (!pi->_pFuncIsUnicode || !pi->_pFuncIsUnicode())
			throw generic_string(TEXT("This ANSI plugin is not compatible with your Unicode Notepad++."));

		pi->_pFuncSetInfo = (PFUNCSETINFO)GetProcAddress(pi->_hLib, "setInfo");

		if (!pi->_pFuncSetInfo)
			throw generic_string(TEXT("Missing \"setInfo\" function"));

		pi->_pFuncGetName = (PFUNCGETNAME)GetProcAddress(pi->_hLib, "getName");
		if (!pi->_pFuncGetName)
			throw generic_string(TEXT("Missing \"getName\" function"));
		pi->_funcName = pi->_pFuncGetName();

		pi->_pBeNotified = (PBENOTIFIED)GetProcAddress(pi->_hLib, "beNotified");
		if (!pi->_pBeNotified)
			throw generic_string(TEXT("Missing \"beNotified\" function"));

		pi->_pMessageProc = (PMESSAGEPROC)GetProcAddress(pi->_hLib, "messageProc");
		if (!pi->_pMessageProc)
			throw generic_string(TEXT("Missing \"messageProc\" function"));

		pi->_pFuncSetInfo(_nppData);

		pi->_pFuncGetFuncsArray = (PFUNCGETFUNCSARRAY)GetProcAddress(pi->_hLib, "getFuncsArray");
		if (!pi->_pFuncGetFuncsArray)
			throw generic_string(TEXT("Missing \"getFuncsArray\" function"));

		pi->_funcItems = pi->_pFuncGetFuncsArray(&pi->_nbFuncItem);

		if ((!pi->_funcItems) || (pi->_nbFuncItem <= 0))
			throw generic_string(TEXT("Missing \"FuncItems\" array, or the nb of Function Item is not set correctly"));

		pi->_pluginMenu = ::CreateMenu();

		GetLexerCountFn GetLexerCount = (GetLexerCountFn)::GetProcAddress(pi->_hLib, "GetLexerCount");
		// it's a lexer plugin
		if (GetLexerCount)
		{
			GetLexerNameFn GetLexerName = (GetLexerNameFn)::GetProcAddress(pi->_hLib, "GetLexerName");
			if (!GetLexerName)
				throw generic_string(TEXT("Loading GetLexerName function failed."));

			GetLexerStatusTextFn GetLexerStatusText = (GetLexerStatusTextFn)::GetProcAddress(pi->_hLib, "GetLexerStatusText");

			if (!GetLexerStatusText)
				throw generic_string(TEXT("Loading GetLexerStatusText function failed."));

			// Assign a buffer for the lexer name.
			char lexName[MAX_EXTERNAL_LEXER_NAME_LEN];
			lexName[0] = '\0';
			TCHAR lexDesc[MAX_EXTERNAL_LEXER_DESC_LEN];
			lexDesc[0] = '\0';

			int numLexers = GetLexerCount();

			ExternalLangContainer *containers[30];

			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			for (int x = 0; x < numLexers; ++x)
			{
				GetLexerName(x, lexName, MAX_EXTERNAL_LEXER_NAME_LEN);
				GetLexerStatusText(x, lexDesc, MAX_EXTERNAL_LEXER_DESC_LEN);
				const TCHAR *pLexerName = wmc->char2wchar(lexName, CP_ACP);
				if (!nppParams->isExistingExternalLangName(pLexerName) && nppParams->ExternalLangHasRoom())
					containers[x] = new ExternalLangContainer(pLexerName, lexDesc);
				else
					containers[x] = NULL;
			}

			TCHAR xmlPath[MAX_PATH];
            lstrcpy(xmlPath, nppParams->getNppPath().c_str());
			PathAppend(xmlPath, TEXT("plugins\\Config"));
            PathAppend(xmlPath, pi->_moduleName.c_str());
			PathRemoveExtension(xmlPath);
			PathAddExtension(xmlPath, TEXT(".xml"));

			if (!PathFileExists(xmlPath))
			{
				lstrcpyn(xmlPath, TEXT("\0"), MAX_PATH );
				lstrcpy(xmlPath, nppParams->getAppDataNppDir() );
				PathAppend(xmlPath, TEXT("plugins\\Config"));
                PathAppend(xmlPath, pi->_moduleName.c_str());
				PathRemoveExtension( xmlPath );
				PathAddExtension( xmlPath, TEXT(".xml") );

				if (! PathFileExists( xmlPath ) )
				{
					throw generic_string(generic_string(xmlPath) + TEXT(" is missing."));
				}
			}

			TiXmlDocument *pXmlDoc = new TiXmlDocument(xmlPath);

			if (!pXmlDoc->LoadFile())
			{
				delete pXmlDoc;
				pXmlDoc = NULL;
				throw generic_string(generic_string(xmlPath) + TEXT(" failed to load."));
			}

			for (int x = 0; x < numLexers; ++x) // postpone adding in case the xml is missing/corrupt
			{
				if (containers[x] != NULL)
					nppParams->addExternalLangToEnd(containers[x]);
			}

			nppParams->getExternalLexerFromXmlTree(pXmlDoc);
			nppParams->getExternalLexerDoc()->push_back(pXmlDoc);
			const char *pDllName = wmc->wchar2char(pluginFilePath, CP_ACP);
			::SendMessage(_nppData._scintillaMainHandle, SCI_LOADLEXERLIBRARY, 0, reinterpret_cast<LPARAM>(pDllName));

		}
		addInLoadedDlls(pluginFilePath, pluginFileName);
		_pluginInfos.push_back(pi);
		return static_cast<int32_t>(_pluginInfos.size() - 1);
	}
	catch (std::exception& e)
	{
		::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
		return -1;
	}
	catch (generic_string s)
	{
		s += TEXT("\n\n");
		s += pluginFileName;
		s += USERMSG;
		if (::MessageBox(NULL, s.c_str(), pluginFilePath, MB_YESNO) == IDYES)
		{
			dll2Remove.push_back(pluginFilePath);
		}
		delete pi;
        return -1;
	}
	catch (...)
	{
		generic_string msg = TEXT("Failed to load");
		msg += TEXT("\n\n");
		msg += pluginFileName;
		msg += USERMSG;
		if (::MessageBox(NULL, msg.c_str(), pluginFilePath, MB_YESNO) == IDYES)
		{
			dll2Remove.push_back(pluginFilePath);
		}
		delete pi;
        return -1;
	}
}


bool PluginsManager::loadPlugins(const TCHAR *dir)
{
	if (_isDisabled)
		return false;

	vector<generic_string> dllNames;
	vector<generic_string> dll2Remove;
	NppParameters * nppParams = NppParameters::getInstance();
    generic_string nppPath = nppParams->getNppPath();
	generic_string pluginsFullPathFilter = (dir && dir[0])?dir:nppPath;

	pluginsFullPathFilter += TEXT("\\plugins\\*.dll");

	WIN32_FIND_DATA foundData;
	HANDLE hFindFile = ::FindFirstFile(pluginsFullPathFilter.c_str(), &foundData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		generic_string plugins1stFullPath = (dir && dir[0])?dir:nppPath;
		plugins1stFullPath += TEXT("\\plugins\\");
		plugins1stFullPath += foundData.cFileName;
		dllNames.push_back(plugins1stFullPath);

		while (::FindNextFile(hFindFile, &foundData))
		{
            bool isInBlackList = nppParams->isInBlackList(foundData.cFileName);
            if (!isInBlackList)
            {
			    generic_string fullPath = (dir && dir[0])?dir:nppPath;
			    fullPath += TEXT("\\plugins\\");

			    fullPath += foundData.cFileName;
			    dllNames.push_back(fullPath);
            }
            PluginList & pl = nppParams->getPluginList();
            pl.add(foundData.cFileName, isInBlackList);
		}
		::FindClose(hFindFile);


		for (size_t i = 0, len = dllNames.size(); i < len ; ++i)
		{
            loadPlugin(dllNames[i].c_str(),  dll2Remove);
		}

	}

	for (size_t j = 0, len = dll2Remove.size() ; j < len ; ++j)
		::DeleteFile(dll2Remove[j].c_str());

	std::sort(_pluginInfos.begin(), _pluginInfos.end(), [](const PluginInfo *a, const PluginInfo *b) { return a->_funcName < b->_funcName; });

	return true;
}

bool PluginsManager::loadPluginsV2(const TCHAR* dir)
{
	if (_isDisabled)
		return false;


	vector<generic_string> dllNames;
	vector<generic_string> dll2Remove;

	NppParameters * nppParams = NppParameters::getInstance();
	generic_string nppPath = nppParams->getNppPath();
	
	generic_string pluginsFolder;
	if (dir && dir[0])
	{
		pluginsFolder = dir;
	}
	else
	{
		pluginsFolder = nppPath;
		PathAppend(pluginsFolder, TEXT("plugins"));
	}
	generic_string pluginsFolderFilter = pluginsFolder;
	PathAppend(pluginsFolderFilter, TEXT("*.*"));
	
	WIN32_FIND_DATA foundData;
	HANDLE hFindFolder = ::FindFirstFile(pluginsFolderFilter.c_str(), &foundData);
	HANDLE hFindDll = INVALID_HANDLE_VALUE;

	// get plugin folder
	if (hFindFolder != INVALID_HANDLE_VALUE && (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		generic_string pluginsFullPathFilter = pluginsFolder;
		PathAppend(pluginsFullPathFilter, foundData.cFileName);
		generic_string pluginsFolderPath = pluginsFullPathFilter;
		generic_string  dllName = foundData.cFileName;
		dllName += TEXT(".dll");
		PathAppend(pluginsFullPathFilter, dllName);

		// get plugin
		hFindDll = ::FindFirstFile(pluginsFullPathFilter.c_str(), &foundData);
		if (hFindDll != INVALID_HANDLE_VALUE && !(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			dllNames.push_back(pluginsFullPathFilter);

			PluginList & pl = nppParams->getPluginList();
			pl.add(foundData.cFileName, false);
		}

		// get plugin folder
		while (::FindNextFile(hFindFolder, &foundData))
		{
			generic_string pluginsFullPathFilter2 = pluginsFolder;
			PathAppend(pluginsFullPathFilter2, foundData.cFileName);
			generic_string pluginsFolderPath2 = pluginsFullPathFilter2;
			generic_string  dllName2 = foundData.cFileName;
			dllName2 += TEXT(".dll");
			PathAppend(pluginsFullPathFilter2, dllName2);
			// get plugin
			hFindDll = ::FindFirstFile(pluginsFullPathFilter2.c_str(), &foundData);
			if (hFindDll != INVALID_HANDLE_VALUE && !(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				dllNames.push_back(pluginsFullPathFilter2);

				PluginList & pl = nppParams->getPluginList();
				pl.add(foundData.cFileName, false);
			}
		}

	}
	::FindClose(hFindFolder);
	::FindClose(hFindDll);

	for (size_t i = 0, len = dllNames.size(); i < len; ++i)
	{
		loadPlugin(dllNames[i].c_str(), dll2Remove);
	}

	return true;
}

// return true if cmdID found and its shortcut is enable
// false otherwise
bool PluginsManager::getShortcutByCmdID(int cmdID, ShortcutKey *sk)
{
	if (cmdID == 0 || !sk)
		return false;

	const vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();

	for (size_t i = 0, len = pluginCmdSCList.size(); i < len ; ++i)
	{
		if (pluginCmdSCList[i].getID() == (unsigned long)cmdID)
		{
			const KeyCombo & kc = pluginCmdSCList[i].getKeyCombo();
			if (kc._key == 0x00)
				return false;

			sk->_isAlt = kc._isAlt;
			sk->_isCtrl = kc._isCtrl;
			sk->_isShift = kc._isShift;
			sk->_key = kc._key;
			return true;
		}
	}
	return false;
}


void PluginsManager::addInMenuFromPMIndex(int i)
{
    vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();
	::InsertMenu(_hPluginsMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_pluginInfos[i]->_pluginMenu, _pluginInfos[i]->_funcName.c_str());

    unsigned short j = 0;
	for ( ; j < _pluginInfos[i]->_nbFuncItem ; ++j)
	{
		if (_pluginInfos[i]->_funcItems[j]._pFunc == NULL)
		{
			::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
			continue;
		}

        _pluginsCommands.push_back(PluginCommand(_pluginInfos[i]->_moduleName.c_str(), j, _pluginInfos[i]->_funcItems[j]._pFunc));

		int cmdID = ID_PLUGINS_CMD + static_cast<int32_t>(_pluginsCommands.size() - 1);
		_pluginInfos[i]->_funcItems[j]._cmdID = cmdID;
		generic_string itemName = _pluginInfos[i]->_funcItems[j]._itemName;

		if (_pluginInfos[i]->_funcItems[j]._pShKey)
		{
			ShortcutKey & sKey = *(_pluginInfos[i]->_funcItems[j]._pShKey);
            PluginCmdShortcut pcs(Shortcut(itemName.c_str(), sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key), cmdID, _pluginInfos[i]->_moduleName.c_str(), j);
			pluginCmdSCList.push_back(pcs);
			itemName += TEXT("\t");
			itemName += pcs.toString();
		}
		else
		{	//no ShortcutKey is provided, add an disabled shortcut (so it can still be mapped, Paramaters class can still index any changes and the toolbar wont funk out
            Shortcut sc(itemName.c_str(), false, false, false, 0x00);
            PluginCmdShortcut pcs(sc, cmdID, _pluginInfos[i]->_moduleName.c_str(), j);	//VK_NULL and everything disabled, the menu name is left alone
			pluginCmdSCList.push_back(pcs);
		}
		::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, cmdID, itemName.c_str());

		if (_pluginInfos[i]->_funcItems[j]._init2Check)
			::CheckMenuItem(_hPluginsMenu, cmdID, MF_BYCOMMAND | MF_CHECKED);
	}
	/*UNLOAD
    ::InsertMenu(_pluginInfos[i]->_pluginMenu, j++, MF_BYPOSITION | MF_SEPARATOR, 0, TEXT(""));
    ::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, ID_PLUGINS_REMOVING + i, TEXT("Remove this plugin"));
	*/
}

HMENU PluginsManager::setMenu(HMENU hMenu, const TCHAR *menuName)
{
	if (hasPlugins())
	{
		const TCHAR *nom_menu = (menuName && menuName[0])?menuName:TEXT("&Plugins");

        if (!_hPluginsMenu)
        {
		    _hPluginsMenu = ::CreateMenu();
		    ::InsertMenu(hMenu,  MENUINDEX_PLUGINS, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_hPluginsMenu, nom_menu);
        }

		for (size_t i = 0, len = _pluginInfos.size() ; i < len ; ++i)
		{
			addInMenuFromPMIndex(static_cast<int32_t>(i));
		}
        return _hPluginsMenu;
	}
	return NULL;
}


void PluginsManager::runPluginCommand(size_t i)
{
	if (i < _pluginsCommands.size())
	{
		if (_pluginsCommands[i]._pFunc != NULL)
		{
			try
			{
				_pluginsCommands[i]._pFunc();
			}
			catch (std::exception& e)
			{
				::MessageBoxA(NULL, e.what(), "PluginsManager::runPluginCommand Exception", MB_OK);
			}
			catch (...)
			{
				TCHAR funcInfo[128];
				generic_sprintf(funcInfo, TEXT("runPluginCommand(size_t i : %d)"), i);
				pluginCrashAlert(_pluginsCommands[i]._pluginName.c_str(), funcInfo);
			}
		}
	}
}


void PluginsManager::runPluginCommand(const TCHAR *pluginName, int commandID)
{
	for (size_t i = 0, len = _pluginsCommands.size() ; i < len ; ++i)
	{
		if (!generic_stricmp(_pluginsCommands[i]._pluginName.c_str(), pluginName))
		{
			if (_pluginsCommands[i]._funcID == commandID)
			{
				try
				{
					_pluginsCommands[i]._pFunc();
				}
				catch (std::exception& e)
				{
					::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
				}
				catch (...)
				{
					TCHAR funcInfo[128];
					generic_sprintf(funcInfo, TEXT("runPluginCommand(const TCHAR *pluginName : %s, int commandID : %d)"), pluginName, commandID);
					pluginCrashAlert(_pluginsCommands[i]._pluginName.c_str(), funcInfo);
				}
			}
		}
	}
}


void PluginsManager::notify(const SCNotification *notification)
{
	if (_noMoreNotification) // this boolean should be enabled after NPPN_SHUTDOWN has been sent
		return;
	_noMoreNotification = notification->nmhdr.code == NPPN_SHUTDOWN;

	for (size_t i = 0, len = _pluginInfos.size() ; i < len ; ++i)
	{
        if (_pluginInfos[i]->_hLib)
        {
			// To avoid the plugin change the data in SCNotification
			// Each notification to pass to a plugin is a copy of SCNotification instance
			SCNotification scNotif = *notification;
			try
			{
				_pluginInfos[i]->_pBeNotified(&scNotif);
			}
			catch (std::exception& e)
			{
				::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
			}
			catch (...)
			{
				TCHAR funcInfo[256];
				generic_sprintf(funcInfo, TEXT("notify(SCNotification *notification) : \r notification->nmhdr.code == %d\r notification->nmhdr.hwndFrom == %p\r notification->nmhdr.idFrom == %d"),\
					scNotif.nmhdr.code, scNotif.nmhdr.hwndFrom, scNotif.nmhdr.idFrom);
				pluginCrashAlert(_pluginInfos[i]->_moduleName.c_str(), funcInfo);
			}
		}
	}
}


void PluginsManager::relayNppMessages(UINT Message, WPARAM wParam, LPARAM lParam)
{
	for (size_t i = 0, len = _pluginInfos.size(); i < len ; ++i)
	{
        if (_pluginInfos[i]->_hLib)
		{
			try
			{
				_pluginInfos[i]->_pMessageProc(Message, wParam, lParam);
			}
			catch (std::exception& e)
			{
				::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
			}
			catch (...)
			{
				TCHAR funcInfo[128];
				generic_sprintf(funcInfo, TEXT("relayNppMessages(UINT Message : %d, WPARAM wParam : %d, LPARAM lParam : %d)"), Message, wParam, lParam);
				pluginCrashAlert(_pluginInfos[i]->_moduleName.c_str(), funcInfo);
			}
		}
	}
}


bool PluginsManager::relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam)
{
	const TCHAR * moduleName = (const TCHAR *)wParam;
	if (!moduleName || !moduleName[0] || !lParam)
		return false;

	for (size_t i = 0, len = _pluginInfos.size() ; i < len ; ++i)
	{
        if (_pluginInfos[i]->_moduleName == moduleName)
		{
            if (_pluginInfos[i]->_hLib)
			{
				try
				{
					_pluginInfos[i]->_pMessageProc(Message, wParam, lParam);
				}
				catch (std::exception& e)
				{
					::MessageBoxA(NULL, e.what(), "Exception", MB_OK);
				}
				catch (...)
				{
					TCHAR funcInfo[128];
					generic_sprintf(funcInfo, TEXT("relayPluginMessages(UINT Message : %d, WPARAM wParam : %d, LPARAM lParam : %d)"), Message, wParam, lParam);
					pluginCrashAlert(_pluginInfos[i]->_moduleName.c_str(), funcInfo);
				}
				return true;
            }
		}
	}
	return false;
}


bool PluginsManager::allocateCmdID(int numberRequired, int *start)
{
	bool retVal = true;

	*start = _dynamicIDAlloc.allocate(numberRequired);

	if (-1 == *start)
	{
		*start = 0;
		retVal = false;
	}
	return retVal;
}

bool PluginsManager::allocateMarker(int numberRequired, int *start)
{
	bool retVal = true;
	*start = _markerAlloc.allocate(numberRequired);
	if (-1 == *start)
	{
		*start = 0;
		retVal = false;
	}
	return retVal;
}

generic_string PluginsManager::getLoadedPluginNames() const
{
	generic_string pluginPaths;
	for (size_t i = 0; i < _loadedDlls.size(); ++i)
	{
		pluginPaths += _loadedDlls[i]._fileName;
		pluginPaths += TEXT(" ");
	}
	return pluginPaths;
}

