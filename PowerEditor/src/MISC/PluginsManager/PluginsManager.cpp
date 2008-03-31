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

#include <shlwapi.h>
#include "PluginsManager.h"

#define USERMSG "This plugin is not compatible with current version of Notepad++.\n\
Remove this plugin from plugins directory if you don't want to see this message on the next launch time."

bool PluginsManager::loadPlugins(const char *dir)
{
	if (_isDisabled)
		return false;

	vector<string> dllNames;
	const char *pNppPath = (NppParameters::getInstance())->getNppPath();

	string pluginsFullPathFilter = (dir && dir[0])?dir:pNppPath;

	pluginsFullPathFilter += "\\plugins\\*.dll";

	WIN32_FIND_DATA foundData;
	HANDLE hFindFile = ::FindFirstFile(pluginsFullPathFilter.c_str(), &foundData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		string plugins1stFullPath = (dir && dir[0])?dir:pNppPath;
		plugins1stFullPath += "\\plugins\\";
		plugins1stFullPath += foundData.cFileName;
		dllNames.push_back(plugins1stFullPath);

		while (::FindNextFile(hFindFile, &foundData))
		{
			string fullPath = (dir && dir[0])?dir:pNppPath;
			fullPath += "\\plugins\\";
			fullPath += foundData.cFileName;
			dllNames.push_back(fullPath);
		}
		::FindClose(hFindFile);

		size_t i = 0;

		for ( ; i < dllNames.size() ; i++)
		{
			PluginInfo *pi = new PluginInfo;
			try {
				char tmpStr[MAX_PATH];
				strcpy(tmpStr, dllNames[i].c_str());
				strcpy(pi->_moduleName, PathFindFileName(tmpStr));
				
				pi->_hLib = ::LoadLibrary(dllNames[i].c_str());
				if (!pi->_hLib)
					throw string("Load Library is failed.\nMake \"Runtime Library\" setting of this project as \"Multi-threaded(/MT)\" may cure this problem.");

				pi->_pFuncSetInfo = (PFUNCSETINFO)GetProcAddress(pi->_hLib, "setInfo");
							
				if (!pi->_pFuncSetInfo)
					throw string("Missing \"setInfo\" function");

				pi->_pFuncGetName = (PFUNCGETNAME)GetProcAddress(pi->_hLib, "getName");
				if (!pi->_pFuncGetName)
					throw string("Missing \"getName\" function");

				pi->_pBeNotified = (PBENOTIFIED)GetProcAddress(pi->_hLib, "beNotified");
				if (!pi->_pBeNotified)
					throw string("Missing \"beNotified\" function");

				pi->_pMessageProc = (PMESSAGEPROC)GetProcAddress(pi->_hLib, "messageProc");
				if (!pi->_pMessageProc)
					throw string("Missing \"messageProc\" function");

				pi->_pFuncGetFuncsArray = (PFUNCGETFUNCSARRAY)GetProcAddress(pi->_hLib, "getFuncsArray");
				if (!pi->_pFuncGetFuncsArray) 
					throw string("Missing \"getFuncsArray\" function");

				pi->_funcItems = pi->_pFuncGetFuncsArray(&pi->_nbFuncItem);

				if ((!pi->_funcItems) || (pi->_nbFuncItem <= 0))
					throw string("Missing \"FuncItems\" array, or the nb of Function Item is not set correctly");
/*
				for (int c = 0 ; c < pi->_nbFuncItem ; c++)
					if (!pi->_funcItems[c]._pFunc)
						throw string("\"FuncItems\" array is not set correctly");
*/
				pi->_pluginMenu = ::CreateMenu();
				
				pi->_pFuncSetInfo(_nppData);

 
				GetLexerCountFn GetLexerCount = (GetLexerCountFn)::GetProcAddress(pi->_hLib, "GetLexerCount");
				// it's a lexer plugin
				if (GetLexerCount)
				{
					GetLexerNameFn GetLexerName = (GetLexerNameFn)::GetProcAddress(pi->_hLib, "GetLexerName");
					if (!GetLexerName)
						throw string("Loading GetLexerName function failed.");

					GetLexerStatusTextFn GetLexerStatusText = (GetLexerStatusTextFn)::GetProcAddress(pi->_hLib, "GetLexerStatusText");

					if (!GetLexerStatusText)
						throw string("Loading GetLexerStatusText function failed.");

					// Assign a buffer for the lexer name.
					char lexName[MAX_EXTERNAL_LEXER_NAME_LEN];
					strcpy(lexName, "");
					char lexDesc[MAX_EXTERNAL_LEXER_DESC_LEN];
					strcpy(lexDesc, "");

					int numLexers = GetLexerCount();

					NppParameters * nppParams = NppParameters::getInstance();

					ExternalLangContainer *containers[30];
					for (int x = 0; x < numLexers; x++)
					{
						GetLexerName(x, lexName, MAX_EXTERNAL_LEXER_NAME_LEN);
						GetLexerStatusText(x, lexDesc, MAX_EXTERNAL_LEXER_DESC_LEN);
						
						if (!nppParams->isExistingExternalLangName(lexName) && nppParams->ExternalLangHasRoom())
							containers[x] = new ExternalLangContainer(lexName, lexDesc);
						else
							containers[x] = NULL;
					}

					char xmlPath[MAX_PATH];
					strcpy(xmlPath, nppParams->getNppPath());
					PathAppend(xmlPath, "plugins\\Config");
					PathAppend(xmlPath, pi->_moduleName);
					PathRemoveExtension(xmlPath);
					PathAddExtension(xmlPath, ".xml");

					if (!PathFileExists(xmlPath))
					{
						throw string(string(xmlPath) + " is missing.");
					}

					TiXmlDocument *_pXmlDoc = new TiXmlDocument(xmlPath);

					if (!_pXmlDoc->LoadFile())
					{
						delete _pXmlDoc;
						_pXmlDoc = NULL;
						throw string(string(xmlPath) + " failed to load.");
					}
					
					for (int x = 0; x < numLexers; x++) // postpone adding in case the xml is missing/corrupt
						if (containers[x] != NULL)
							nppParams->addExternalLangToEnd(containers[x]);

					nppParams->getExternalLexerFromXmlTree(_pXmlDoc);
					nppParams->getExternalLexerDoc()->push_back(_pXmlDoc);

					::SendMessage(_nppData._scintillaMainHandle, SCI_LOADLEXERLIBRARY, 0, (LPARAM)dllNames[i].c_str());
				}
				_pluginInfos.push_back(pi);
			}
			catch(string s)
			{
				s += "\n\n";
				s += USERMSG;
				::MessageBox(NULL, s.c_str(), dllNames[i].c_str(), MB_OK);
				delete pi;
			}
			catch(...)
			{
				string msg = "Fail loaded";
				msg += "\n\n";
				msg += USERMSG;
				::MessageBox(NULL, msg.c_str(), dllNames[i].c_str(), MB_OK);
				delete pi;
			}
		}
	}

	return true;
}

void PluginsManager::setMenu(HMENU hMenu, const char *menuName)
{
	if (hasPlugins())
	{
		vector<PluginCmdShortcut> & pluginCmdSCList = (NppParameters::getInstance())->getPluginCommandList();
		const char *nom_menu = (menuName && menuName[0])?menuName:"Plugins";

		_hPluginsMenu = ::CreateMenu();
		::InsertMenu(hMenu, 9, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_hPluginsMenu, nom_menu);

		for (size_t i = 0 ; i < _pluginInfos.size() ; i++)
		{
			::InsertMenu(_hPluginsMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)_pluginInfos[i]->_pluginMenu, _pluginInfos[i]->_pFuncGetName());

			for (int j = 0 ; j < _pluginInfos[i]->_nbFuncItem ; j++)
			{
				_pluginsCommands.push_back(PluginCommand(_pluginInfos[i]->_moduleName, j, _pluginInfos[i]->_funcItems[j]._pFunc));
				int cmdID = ID_PLUGINS_CMD + (_pluginsCommands.size() - 1);
				_pluginInfos[i]->_funcItems[j]._cmdID = (_pluginInfos[i]->_funcItems[j]._pFunc == NULL)?0:cmdID;
				string itemName = _pluginInfos[i]->_funcItems[j]._itemName;

				if (_pluginInfos[i]->_funcItems[j]._pShKey)
				{
					ShortcutKey & sKey = *(_pluginInfos[i]->_funcItems[j]._pShKey);
					PluginCmdShortcut pcs(Shortcut(itemName.c_str(), sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key), cmdID, _pluginInfos[i]->_moduleName, j);
					pluginCmdSCList.push_back(pcs);
					itemName += "\t";
					itemName += pcs.toString();
				}
				else
				{	//no ShortcutKey is provided, add an disabled shortcut (so it can still be mapped, Paramaters class can still index any changes and the toolbar wont funk out
					PluginCmdShortcut pcs(Shortcut(itemName.c_str(), false, false, false, 0x00), cmdID, _pluginInfos[i]->_moduleName, j);	//VK_NULL and everything disabled, the menu name is left alone
					pluginCmdSCList.push_back(pcs);
				}
				unsigned int flag = MF_BYPOSITION | ((_pluginInfos[i]->_funcItems[j]._pFunc == NULL)?MF_SEPARATOR:0);
				::InsertMenu(_pluginInfos[i]->_pluginMenu, j, flag, (_pluginInfos[i]->_funcItems[j]._pFunc == NULL)?0:cmdID, itemName.c_str());

				if (_pluginInfos[i]->_funcItems[j]._init2Check)
					::CheckMenuItem(_hPluginsMenu, cmdID, MF_BYCOMMAND | MF_CHECKED);
			}
		}
	}
}
