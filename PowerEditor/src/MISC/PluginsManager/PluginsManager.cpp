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


#include <shlwapi.h>
#include <dbghelp.h>
#include <algorithm>
#include <cinttypes>
#include "PluginsManager.h"
#include "resource.h"
#include "pluginsAdmin.h"
#include "ILexer.h"
#include "Lexilla.h"

using namespace std;

const wchar_t * USERMSG = L" is not compatible with the current version of Notepad++.\n\n\
Do you want to remove this plugin from the plugins directory to prevent this message from the next launch?";

static WORD getBinaryArchitectureType(const wchar_t *filePath)
{
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}

	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
	if (hMapping == NULL)
	{
		CloseHandle(hFile);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}

	LPVOID addrHeader = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (addrHeader == NULL) // couldn't memory map the file
	{
		CloseHandle(hFile);
		CloseHandle(hMapping);
		return IMAGE_FILE_MACHINE_UNKNOWN;
	}

	PIMAGE_NT_HEADERS peHdr = ImageNtHeader(addrHeader);

	// Found the binary and architecture type, if peHdr is !NULL
	WORD machine_type = (peHdr == NULL) ? IMAGE_FILE_MACHINE_UNKNOWN : peHdr->FileHeader.Machine;

	// release all of our handles
	UnmapViewOfFile(addrHeader);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return machine_type;
}

#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
	#define	LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR	0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
	#define	LOAD_LIBRARY_SEARCH_DEFAULT_DIRS	0x00001000
#endif

int PluginsManager::loadPluginFromPath(const wchar_t *pluginFilePath)
{
	const wchar_t *pluginFileName = ::PathFindFileName(pluginFilePath);
	if (isInLoadedDlls(pluginFileName))
		return 0;

	NppParameters& nppParams = NppParameters::getInstance();

	PluginInfo *pi = new PluginInfo;
	try
	{
		pi->_moduleName = pluginFileName;
		int archType = nppParams.archType();
		if (getBinaryArchitectureType(pluginFilePath) != archType)
		{
			const wchar_t* archErrMsg = L"Cannot load plugin.";
			switch (archType)
			{
				case IMAGE_FILE_MACHINE_ARM64:
					archErrMsg = L"Cannot load ARM64 plugin.";
					break;
				case IMAGE_FILE_MACHINE_I386:
					archErrMsg = L"Cannot load 32-bit plugin.";
					break;
				case IMAGE_FILE_MACHINE_AMD64:
					archErrMsg = L"Cannot load 64-bit plugin.";
					break;
			}

			throw wstring(archErrMsg);
		}

        const DWORD dwFlags = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "AddDllDirectory") != NULL ? LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS : 0;
        pi->_hLib = ::LoadLibraryEx(pluginFilePath, NULL, dwFlags);
        if (!pi->_hLib)
        {
			wstring lastErrorMsg = GetLastErrorAsString();
            if (lastErrorMsg.empty())
                throw wstring(L"Load Library has failed.\nChanging the project's \"Runtime Library\" setting to \"Multi-threaded(/MT)\" might solve this problem.");
            else
                throw wstring(lastErrorMsg.c_str());
        }
        
		pi->_pFuncIsUnicode = (PFUNCISUNICODE)GetProcAddress(pi->_hLib, "isUnicode");
		if (!pi->_pFuncIsUnicode || !pi->_pFuncIsUnicode())
			throw wstring(L"This ANSI plugin is not compatible with your Unicode Notepad++.");

		pi->_pFuncSetInfo = (PFUNCSETINFO)GetProcAddress(pi->_hLib, "setInfo");

		if (!pi->_pFuncSetInfo)
			throw wstring(L"Missing \"setInfo\" function");

		pi->_pFuncGetName = (PFUNCGETNAME)GetProcAddress(pi->_hLib, "getName");
		if (!pi->_pFuncGetName)
			throw wstring(L"Missing \"getName\" function");
		pi->_funcName = pi->_pFuncGetName();

		pi->_pBeNotified = (PBENOTIFIED)GetProcAddress(pi->_hLib, "beNotified");
		if (!pi->_pBeNotified)
			throw wstring(L"Missing \"beNotified\" function");

		pi->_pMessageProc = (PMESSAGEPROC)GetProcAddress(pi->_hLib, "messageProc");
		if (!pi->_pMessageProc)
			throw wstring(L"Missing \"messageProc\" function");

		pi->_pFuncSetInfo(_nppData);

		pi->_pFuncGetFuncsArray = (PFUNCGETFUNCSARRAY)GetProcAddress(pi->_hLib, "getFuncsArray");
		if (!pi->_pFuncGetFuncsArray)
			throw wstring(L"Missing \"getFuncsArray\" function");

		pi->_funcItems = pi->_pFuncGetFuncsArray(&pi->_nbFuncItem);

		if ((!pi->_funcItems) || (pi->_nbFuncItem <= 0))
			throw wstring(L"Missing \"FuncItems\" array, or the nb of Function Item is not set correctly");

		pi->_pluginMenu = ::CreateMenu();

		Lexilla::GetLexerCountFn GetLexerCount = (Lexilla::GetLexerCountFn)::GetProcAddress(pi->_hLib, LEXILLA_GETLEXERCOUNT);
		// it's a lexer plugin
		if (GetLexerCount)
		{
			Lexilla::GetLexerNameFn GetLexerName = (Lexilla::GetLexerNameFn)::GetProcAddress(pi->_hLib, LEXILLA_GETLEXERNAME);
			if (!GetLexerName)
				throw wstring(L"Loading GetLexerName function failed.");

			//Lexilla::GetLexerFactoryFn GetLexerFactory = (Lexilla::GetLexerFactoryFn)::GetProcAddress(pi->_hLib, LEXILLA_GETLEXERFACTORY);
			//if (!GetLexerFactory)
				//throw wstring(L"Loading GetLexerFactory function failed.");

			Lexilla::CreateLexerFn CreateLexer = (Lexilla::CreateLexerFn)::GetProcAddress(pi->_hLib, LEXILLA_CREATELEXER);
			if (!CreateLexer)
				throw wstring(L"Loading CreateLexer function failed.");

			//Lexilla::GetLibraryPropertyNamesFn GetLibraryPropertyNames = (Lexilla::GetLibraryPropertyNamesFn)::GetProcAddress(pi->_hLib, LEXILLA_GETLIBRARYPROPERTYNAMES);
			//if (!GetLibraryPropertyNames)
				//throw wstring(L"Loading GetLibraryPropertyNames function failed.");

			//Lexilla::SetLibraryPropertyFn SetLibraryProperty = (Lexilla::SetLibraryPropertyFn)::GetProcAddress(pi->_hLib, LEXILLA_SETLIBRARYPROPERTY);
			//if (!SetLibraryProperty)
				//throw wstring(L"Loading SetLibraryProperty function failed.");

			//Lexilla::GetNameSpaceFn GetNameSpace = (Lexilla::GetNameSpaceFn)::GetProcAddress(pi->_hLib, LEXILLA_GETNAMESPACE);
			//if (!GetNameSpace)
				//throw wstring(L"Loading GetNameSpace function failed.");

			// Assign a buffer for the lexer name.
			char lexName[MAX_EXTERNAL_LEXER_NAME_LEN];
			lexName[0] = '\0';

			int numLexers = GetLexerCount();

			ExternalLangContainer* containers[30];

			for (int x = 0; x < numLexers; ++x)
			{
				GetLexerName(x, lexName, MAX_EXTERNAL_LEXER_NAME_LEN);
				if (!nppParams.isExistingExternalLangName(lexName) && nppParams.ExternalLangHasRoom())
				{
					containers[x] = new ExternalLangContainer;
					containers[x]->_name = lexName;
					containers[x]->fnCL = CreateLexer;
					//containers[x]->fnGLPN = GetLibraryPropertyNames;
					//containers[x]->fnSLP = SetLibraryProperty;
				}
				else
				{
					containers[x] = NULL;
				}
			}

			wchar_t xmlPath[MAX_PATH];
			wcscpy_s(xmlPath, nppParams.getNppPath().c_str());
			PathAppend(xmlPath, L"plugins\\Config");
            PathAppend(xmlPath, pi->_moduleName.c_str());
			PathRemoveExtension(xmlPath);
			PathAddExtension(xmlPath, L".xml");

			if (!doesFileExist(xmlPath))
			{
				lstrcpyn(xmlPath, L"\0", MAX_PATH );
				wcscpy_s(xmlPath, nppParams.getAppDataNppDir());
				PathAppend(xmlPath, L"plugins\\Config");
                PathAppend(xmlPath, pi->_moduleName.c_str());
				PathRemoveExtension(xmlPath);
				PathAddExtension(xmlPath, L".xml");

				if (!doesFileExist(xmlPath))
				{
					throw wstring(wstring(xmlPath) + L" is missing.");
				}
			}

			TiXmlDocument *pXmlDoc = new TiXmlDocument(xmlPath);

			if (!pXmlDoc->LoadFile())
			{
				delete pXmlDoc;
				pXmlDoc = NULL;
				throw wstring(wstring(xmlPath) + L" failed to load.");
			}

			for (int x = 0; x < numLexers; ++x) // postpone adding in case the xml is missing/corrupt
			{
				if (containers[x] != NULL)
					nppParams.addExternalLangToEnd(containers[x]);
			}

			nppParams.getExternalLexerFromXmlTree(pXmlDoc);
			nppParams.getExternalLexerDoc()->push_back(pXmlDoc);


			//const char *pDllName = wmc.wchar2char(pluginFilePath, CP_ACP);
			//::SendMessage(_nppData._scintillaMainHandle, SCI_LOADLEXERLIBRARY, 0, reinterpret_cast<LPARAM>(pDllName));


		}
		addInLoadedDlls(pluginFilePath, pluginFileName);
		_pluginInfos.push_back(pi);
		return static_cast<int32_t>(_pluginInfos.size() - 1);
	}
	catch (std::exception& e)
	{
		pluginExceptionAlert(pluginFileName, e);
		return -1;
	}
	catch (wstring& s)
	{
		if (pi && pi->_hLib)
		{
			::FreeLibrary(pi->_hLib);
		}

		s += L"\n\n";
		s += pluginFileName;
		s += USERMSG;
		if (::MessageBox(_nppData._nppHandle, s.c_str(), pluginFilePath, MB_YESNO) == IDYES)
		{

			::DeleteFile(pluginFilePath);
		}
		delete pi;
		return -1;
	}
	catch (...)
	{
		if (pi && pi->_hLib)
		{
			::FreeLibrary(pi->_hLib);
		}

		wstring msg = L"Failed to load";
		msg += L"\n\n";
		msg += pluginFileName;
		msg += USERMSG;
		if (::MessageBox(_nppData._nppHandle, msg.c_str(), pluginFilePath, MB_YESNO) == IDYES)
		{
			::DeleteFile(pluginFilePath);
		}
		delete pi;
		return -1;
	}
}

bool PluginsManager::loadPlugins(const wchar_t* dir, const PluginViewList* pluginUpdateInfoList, PluginViewList* pluginIncompatibleList)
{
	if (_isDisabled)
		return false;

	vector<wstring> dllNames;

	NppParameters& nppParams = NppParameters::getInstance();
	wstring nppPath = nppParams.getNppPath();
	
	wstring pluginsFolder;
	if (dir && dir[0])
	{
		pluginsFolder = dir;
	}
	else
	{
		pluginsFolder = nppPath;
		pathAppend(pluginsFolder, L"plugins");
	}
	wstring pluginsFolderFilter = pluginsFolder;
	pathAppend(pluginsFolderFilter, L"*.*");
	
	WIN32_FIND_DATA foundData;
	HANDLE hFindFolder = ::FindFirstFile(pluginsFolderFilter.c_str(), &foundData);
	HANDLE hFindDll = INVALID_HANDLE_VALUE;

	// Get Notepad++ current version
	wchar_t nppFullPathName[MAX_PATH];
	GetModuleFileName(NULL, nppFullPathName, MAX_PATH);
	Version nppVer;
	nppVer.setVersionFrom(nppFullPathName);

	// get plugin folder
	if (hFindFolder != INVALID_HANDLE_VALUE && (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		const wchar_t* incompatibleWarning = L"%s's version %s is not compatible to this version of Notepad++ (v%s).\r\nAs a result the plugin cannot be loaded.";
		const wchar_t* incompatibleWarningWithSolution = L"%s's version %s is not compatible to this version of Notepad++ (v%s).\r\nAs a result the plugin cannot be loaded.\r\n\r\nGo to Updates section and update your plugin to %s for solving the compatibility issue.";

		wstring foundFileName = foundData.cFileName;
		if (foundFileName != L"." && foundFileName != L".." && wcsicmp(foundFileName.c_str(), L"Config") != 0)
		{
			wstring pluginsFullPathFilter = pluginsFolder;
			pathAppend(pluginsFullPathFilter, foundFileName);
			wstring  dllName = foundFileName;
			dllName += L".dll";
			pathAppend(pluginsFullPathFilter, dllName);

			// get plugin
			hFindDll = ::FindFirstFile(pluginsFullPathFilter.c_str(), &foundData);
			if (hFindDll != INVALID_HANDLE_VALUE && !(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// - foundFileName: folder-name
				// _ pluginsFullPathFilter: version
				// 
				// Find plugin update info of current plugin and check if it's compatible to Notepad++ current versions
				bool isCompatible = true;

				if (pluginUpdateInfoList)
				{
					int index = 0;
					PluginUpdateInfo* pui = pluginUpdateInfoList->findPluginInfoFromFolderName(foundFileName, index);
					if (pui)
					{
						// Find plugin version
						Version v;
						v.setVersionFrom(pluginsFullPathFilter);
						if (v == pui->_version)
						{
							// Find compatible Notepad++ versions
							isCompatible = nppVer.isCompatibleTo(pui->_nppCompatibleVersions.first, pui->_nppCompatibleVersions.second);

							if (!isCompatible && pluginIncompatibleList)
							{
								PluginUpdateInfo* incompatiblePlg = new PluginUpdateInfo(*pui);
								incompatiblePlg->_version = v;
								wchar_t msg[1024];
								wsprintf(msg, incompatibleWarning, incompatiblePlg->_displayName.c_str(), v.toString().c_str(), nppVer.toString().c_str());
								incompatiblePlg->_description = msg;
								pluginIncompatibleList->pushBack(incompatiblePlg);
							}
						}
						else if (v < pui->_version && // If dll version is older, and _oldVersionCompatibility is valid (not empty), we search in "_oldVersionCompatibility"
							!(pui->_oldVersionCompatibility.first.first.empty() && pui->_oldVersionCompatibility.first.second.empty()) && // first version interval is valid
							!(pui->_oldVersionCompatibility.first.second.empty() && pui->_oldVersionCompatibility.second.second.empty())) // second version interval is valid
						{
							if (v.isCompatibleTo(pui->_oldVersionCompatibility.first.first, pui->_oldVersionCompatibility.first.second)) // dll older version found
							{
								isCompatible = nppVer.isCompatibleTo(pui->_oldVersionCompatibility.second.first, pui->_oldVersionCompatibility.second.second);

								if (!isCompatible && pluginIncompatibleList)
								{
									PluginUpdateInfo* incompatiblePlg = new PluginUpdateInfo(*pui);
									incompatiblePlg->_version = v;
									wchar_t msg[1024];
									wsprintf(msg, incompatibleWarningWithSolution, incompatiblePlg->_displayName.c_str(), v.toString().c_str(), nppVer.toString().c_str(), pui->_version.toString().c_str());
									incompatiblePlg->_description = msg;
									pluginIncompatibleList->pushBack(incompatiblePlg);
								}
							}
						}
					}
				}
				
				if (isCompatible)
					dllNames.push_back(pluginsFullPathFilter);
			}
		}
		// get plugin folder
		while (::FindNextFile(hFindFolder, &foundData))
		{
			wstring foundFileName2 = foundData.cFileName;
			if (foundFileName2 != L"." && foundFileName2 != L".." && wcsicmp(foundFileName2.c_str(), L"Config") != 0)
			{
				wstring pluginsFullPathFilter2 = pluginsFolder;
				pathAppend(pluginsFullPathFilter2, foundFileName2);
				wstring  dllName2 = foundFileName2;
				dllName2 += L".dll";
				pathAppend(pluginsFullPathFilter2, dllName2);

				// get plugin
				if (hFindDll && (hFindDll != INVALID_HANDLE_VALUE))
				{
					::FindClose(hFindDll);
					hFindDll = INVALID_HANDLE_VALUE;
				}
				hFindDll = ::FindFirstFile(pluginsFullPathFilter2.c_str(), &foundData);
				if (hFindDll != INVALID_HANDLE_VALUE && !(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// - foundFileName2: folder-name
					// _ pluginsFullPathFilter2: version
					// 
					// Find plugin update info of current plugin and check if it's compatible to Notepad++ current versions
					bool isCompatible2 = true;

					if (pluginUpdateInfoList)
					{
						int index2 = 0;
						PluginUpdateInfo* pui2 = pluginUpdateInfoList->findPluginInfoFromFolderName(foundFileName2, index2);
						if (pui2)
						{
							// Find plugin version
							Version v2;
							v2.setVersionFrom(pluginsFullPathFilter2);
							if (v2 == pui2->_version)
							{
								// Find compatible Notepad++ versions
								isCompatible2 = nppVer.isCompatibleTo(pui2->_nppCompatibleVersions.first, pui2->_nppCompatibleVersions.second);

								if (!isCompatible2 && pluginIncompatibleList)
								{
									PluginUpdateInfo* incompatiblePlg = new PluginUpdateInfo(*pui2);
									incompatiblePlg->_version = v2;
									wchar_t msg[1024];
									wsprintf(msg, incompatibleWarning, incompatiblePlg->_displayName.c_str(), v2.toString().c_str(), nppVer.toString().c_str());
									incompatiblePlg->_description = msg;
									pluginIncompatibleList->pushBack(incompatiblePlg);
								}
							}
							else if (v2 < pui2->_version && // If dll version is older, and _oldVersionCompatibility is valid (not empty), we search in "_oldVersionCompatibility"
								!(pui2->_oldVersionCompatibility.first.first.empty() && pui2->_oldVersionCompatibility.first.second.empty()) && // first version interval is valid
								!(pui2->_oldVersionCompatibility.first.second.empty() && pui2->_oldVersionCompatibility.second.second.empty())) // second version interval is valid
							{
								if (v2.isCompatibleTo(pui2->_oldVersionCompatibility.first.first, pui2->_oldVersionCompatibility.first.second)) // dll older version found
								{
									isCompatible2 = nppVer.isCompatibleTo(pui2->_oldVersionCompatibility.second.first, pui2->_oldVersionCompatibility.second.second);

									if (!isCompatible2 && pluginIncompatibleList)
									{
										PluginUpdateInfo* incompatiblePlg = new PluginUpdateInfo(*pui2);
										incompatiblePlg->_version = v2;
										wchar_t msg[1024];
										wsprintf(msg, incompatibleWarningWithSolution, incompatiblePlg->_displayName.c_str(), v2.toString().c_str(), nppVer.toString().c_str(), pui2->_version.toString().c_str());
										incompatiblePlg->_description = msg;
										pluginIncompatibleList->pushBack(incompatiblePlg);
									}
								}
							}
						}
					}

					if (isCompatible2)
						dllNames.push_back(pluginsFullPathFilter2);
				}
			}
		}

	}

	if (hFindFolder && (hFindFolder != INVALID_HANDLE_VALUE))
		::FindClose(hFindFolder);

	if (hFindDll && (hFindDll != INVALID_HANDLE_VALUE))
		::FindClose(hFindDll);

	for (size_t i = 0, len = dllNames.size(); i < len; ++i)
	{
		loadPluginFromPath(dllNames[i].c_str());
	}

	return true;
}

// return true if cmdID found and its shortcut is enable
// false otherwise
bool PluginsManager::getShortcutByCmdID(int cmdID, ShortcutKey *sk)
{
	if (cmdID == 0 || !sk)
		return false;

	const vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance()).getPluginCommandList();

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

// returns false if cmdID not provided, true otherwise
bool PluginsManager::removeShortcutByCmdID(int cmdID)
{
	if (cmdID == 0)
		return false;

	NppParameters& nppParam = NppParameters::getInstance();
	vector<PluginCmdShortcut> & pluginCmdSCList = nppParam.getPluginCommandList();

	for (size_t i = 0, len = pluginCmdSCList.size(); i < len; ++i)
	{
		if (pluginCmdSCList[i].getID() == (unsigned long)cmdID)
		{
			//remove shortcut
			pluginCmdSCList[i].clear();

			// inform accelerator instance
			nppParam.getAccelerator()->updateShortcuts();

			// set dirty flag to force writing shortcuts.xml on shutdown
			nppParam.setShortcutDirty();
			break;
		}
	}
	return true;
}

void PluginsManager::addInMenuFromPMIndex(int i)
{
    vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance()).getPluginCommandList();
	::InsertMenu(_hPluginsMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_pluginInfos[i]->_pluginMenu, _pluginInfos[i]->_funcName.c_str());

    unsigned short j = 0;
	for ( ; j < _pluginInfos[i]->_nbFuncItem ; ++j)
	{
		if (_pluginInfos[i]->_funcItems[j]._pFunc == NULL)
		{
			::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
			continue;
		}

        _pluginsCommands.push_back(PluginCommand(_pluginInfos[i]->_moduleName.c_str(), j, _pluginInfos[i]->_funcItems[j]._pFunc));

		int cmdID = ID_PLUGINS_CMD + static_cast<int32_t>(_pluginsCommands.size() - 1);
		_pluginInfos[i]->_funcItems[j]._cmdID = cmdID;
		string itemName = wstring2string(_pluginInfos[i]->_funcItems[j]._itemName, CP_UTF8);

		if (_pluginInfos[i]->_funcItems[j]._pShKey)
		{
			ShortcutKey & sKey = *(_pluginInfos[i]->_funcItems[j]._pShKey);
            PluginCmdShortcut pcs(Shortcut(itemName.c_str(), sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key), cmdID, wstring2string(_pluginInfos[i]->_moduleName, CP_UTF8).c_str(), j);
			pluginCmdSCList.push_back(pcs);
			itemName += "\t";
			itemName += pcs.toString();
		}
		else
		{	//no ShortcutKey is provided, add an disabled shortcut (so it can still be mapped, Paramaters class can still index any changes and the toolbar wont funk out
            Shortcut sc(itemName.c_str(), false, false, false, 0x00);
            PluginCmdShortcut pcs(sc, cmdID, wstring2string(_pluginInfos[i]->_moduleName, CP_UTF8).c_str(), j);	//VK_NULL and everything disabled, the menu name is left alone
			pluginCmdSCList.push_back(pcs);
		}
		::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, cmdID, string2wstring(itemName, CP_UTF8).c_str());

		if (_pluginInfos[i]->_funcItems[j]._init2Check)
			::CheckMenuItem(_hPluginsMenu, cmdID, MF_BYCOMMAND | MF_CHECKED);
	}
}

HMENU PluginsManager::initMenu(HMENU hMenu, bool enablePluginAdmin)
{
	size_t nbPlugin = _pluginInfos.size();

	if (!_hPluginsMenu)
	{
		_hPluginsMenu = ::GetSubMenu(hMenu, MENUINDEX_PLUGINS);

		if (nbPlugin > 0)
			::InsertMenu(_hPluginsMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, L"");

		if (enablePluginAdmin)
		{
			int i = 1;
			::InsertMenu(_hPluginsMenu, i++, MF_BYPOSITION, IDM_SETTING_PLUGINADM, L"Plugins Admin...");
			::InsertMenu(_hPluginsMenu, i++, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
		}
	}

	for (size_t i = 0; i < nbPlugin; ++i)
	{
		addInMenuFromPMIndex(static_cast<int32_t>(i));
	}
	return _hPluginsMenu;
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
				constexpr size_t bufSize = 128;
				wchar_t funcInfo[bufSize] = { '\0' };
				swprintf(funcInfo, bufSize, L"runPluginCommand(size_t i : %zd)", i);
				pluginCrashAlert(_pluginsCommands[i]._pluginName.c_str(), funcInfo);
			}
		}
	}
}


void PluginsManager::runPluginCommand(const wchar_t *pluginName, int commandID)
{
	for (size_t i = 0, len = _pluginsCommands.size() ; i < len ; ++i)
	{
		if (!wcsicmp(_pluginsCommands[i]._pluginName.c_str(), pluginName))
		{
			if (_pluginsCommands[i]._funcID == commandID)
			{
				try
				{
					_pluginsCommands[i]._pFunc();
				}
				catch (std::exception& e)
				{
					pluginExceptionAlert(_pluginsCommands[i]._pluginName.c_str(), e);
				}
				catch (...)
				{
					constexpr size_t bufSize = 128;
					wchar_t funcInfo[bufSize] = { '\0' };
					swprintf(funcInfo, bufSize, L"runPluginCommand(const wchar_t *pluginName : %s, int commandID : %d)", pluginName, commandID);
					pluginCrashAlert(_pluginsCommands[i]._pluginName.c_str(), funcInfo);
				}
			}
		}
	}
}

// send the notification to a specific plugin
void PluginsManager::notify(size_t indexPluginInfo, const SCNotification *notification)
{
	if (indexPluginInfo >= _pluginInfos.size())
		return;

	if (_pluginInfos[indexPluginInfo]->_hLib)
	{
		// To avoid the plugin change the data in SCNotification
		// Each notification to pass to a plugin is a copy of SCNotification instance
		SCNotification scNotif = *notification;
		try
		{
			_pluginInfos[indexPluginInfo]->_pBeNotified(&scNotif);
		}
		catch (std::exception& e)
		{
			pluginExceptionAlert(_pluginInfos[indexPluginInfo]->_moduleName.c_str(), e);
		}
		catch (...)
		{
			constexpr size_t bufSize = 256;
			wchar_t funcInfo[bufSize] = { '\0' };
			swprintf(funcInfo, bufSize, L"notify(SCNotification *notification) : \r notification->nmhdr.code == %d\r notification->nmhdr.hwndFrom == %p\r notification->nmhdr.idFrom == %" PRIuPTR, \
				scNotif.nmhdr.code, scNotif.nmhdr.hwndFrom, scNotif.nmhdr.idFrom);
			pluginCrashAlert(_pluginInfos[indexPluginInfo]->_moduleName.c_str(), funcInfo);
		}
	}
}

// broadcast the notification to all plugins
void PluginsManager::notify(const SCNotification *notification)
{
	if (_noMoreNotification) // this boolean should be enabled after NPPN_SHUTDOWN has been sent
		return;
	_noMoreNotification = notification->nmhdr.code == NPPN_SHUTDOWN;

	for (size_t i = 0, len = _pluginInfos.size() ; i < len ; ++i)
	{
		notify(i, notification);
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
				pluginExceptionAlert(_pluginInfos[i]->_moduleName.c_str(), e);
			}
			catch (...)
			{
				constexpr size_t bufSize = 128;
				wchar_t funcInfo[bufSize] = { '\0' };
				swprintf(funcInfo, bufSize, L"relayNppMessages(UINT Message : %u, WPARAM wParam : %" PRIuPTR ", LPARAM lParam : %" PRIiPTR ")", Message, wParam, lParam);
				pluginCrashAlert(_pluginInfos[i]->_moduleName.c_str(), funcInfo);
			}
		}
	}
}


bool PluginsManager::relayPluginMessages(UINT Message, WPARAM wParam, LPARAM lParam)
{
	const wchar_t * moduleName = (const wchar_t *)wParam;
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
					pluginExceptionAlert(_pluginInfos[i]->_moduleName.c_str(), e);
				}
				catch (...)
				{
					constexpr size_t bufSize = 128;
					wchar_t funcInfo[bufSize] = { '\0' };
					swprintf(funcInfo, bufSize, L"relayPluginMessages(UINT Message : %u, WPARAM wParam : %" PRIuPTR ", LPARAM lParam : %" PRIiPTR ")", Message, wParam, lParam);
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

	if (*start == -1)
	{
		*start = 0;
		retVal = false;
	}
	return retVal;
}

bool PluginsManager::allocateMarker(int numberRequired, int* start)
{
	bool retVal = true;
	*start = _markerAlloc.allocate(numberRequired);
	if (*start == -1)
	{
		*start = 0;
		retVal = false;
	}
	return retVal;
}

bool PluginsManager::allocateIndicator(int numberRequired, int* start)
{
	bool retVal = false;
	int possibleStart = _indicatorAlloc.allocate(numberRequired);
	if (possibleStart != -1)
	{
		*start = possibleStart;
		retVal = true;
	}
	return retVal;
}

wstring PluginsManager::getLoadedPluginNames() const
{
	wstring pluginPaths;
	PluginUpdateInfo pl;
	for (const auto &dll : _loadedDlls)
	{
		pl = PluginUpdateInfo(dll._fullFilePath, dll._fileName);
		pluginPaths += L"\r\n    ";
		pluginPaths += dll._displayName;
		pluginPaths += L" (";
		pluginPaths += pl._version.toString();
		pluginPaths += L")";
	}
	return pluginPaths;
}

