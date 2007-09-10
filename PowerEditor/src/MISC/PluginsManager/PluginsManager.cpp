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

				getCustomizedShortcuts(pi->_moduleName, pi->_funcItems, pi->_nbFuncItem);

				for (int i = 0 ; i < pi->_nbFuncItem ; i++)
					if (!pi->_funcItems[i]._pFunc)
						throw string("\"FuncItems\" array is not set correctly");

				pi->_pluginMenu = ::CreateMenu();
				
				pi->_pFuncSetInfo(_nppData);

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
				//printInt(cmdID);
				_pluginInfos[i]->_funcItems[j]._cmdID = cmdID;
				string itemName = _pluginInfos[i]->_funcItems[j]._itemName;

				if (_pluginInfos[i]->_funcItems[j]._pShKey)
				{
					ShortcutKey & sKey = *(_pluginInfos[i]->_funcItems[j]._pShKey);
					//CommandShortcut cmdShortcut(itemName.c_str(), cmdID, sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key);
					//printInt(cmdID);
					PluginCmdShortcut pcs(Shortcut(itemName.c_str(), sKey._isCtrl, sKey._isAlt, sKey._isShift, sKey._key), cmdID, _pluginInfos[i]->_moduleName, j);
					pluginCmdSCList.push_back(pcs);
					itemName += "\t";
					itemName += pcs.toString();
				}
				::InsertMenu(_pluginInfos[i]->_pluginMenu, j, MF_BYPOSITION, cmdID, itemName.c_str());
				if (_pluginInfos[i]->_funcItems[j]._init2Check)
					::CheckMenuItem(_hPluginsMenu, cmdID, MF_BYCOMMAND | MF_CHECKED);
			}
		}
	}
}
