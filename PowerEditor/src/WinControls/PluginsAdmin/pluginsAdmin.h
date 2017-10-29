// This file is part of Notepad++ project
// Copyright (C)2017 Don HO <don.h@free.fr>
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


#pragma once

#include "StaticDialog.h"
#include "pluginsAdminRes.h"
#include "TabBar.h"
#include "ListView.h"
#include "tinyxml.h"

class PluginsManager;

struct Version
{
	unsigned long _major = 0;
	unsigned long _minor = 0;
	unsigned long _patch = 0;
	unsigned long _build = 0;
	void setVersionFrom(generic_string filePath);
	generic_string toString();
};

struct PluginUpdateInfo
{
	generic_string _fullFilePath;

	generic_string _id;
	generic_string _name;
	Version _version;
	generic_string _homepage;
	generic_string _sourceUrl;
	generic_string _description;
	generic_string _author;
	generic_string _md5;
	generic_string _alias;
	generic_string _repository;

	generic_string describe();
	PluginUpdateInfo() {};
	PluginUpdateInfo(const generic_string& fullFilePath, const generic_string& fileName);
};

struct NppCurrentStatus
{
	bool _isAdminMode;              // can launch gitup en Admin mode directly

	bool _isInProgramFiles;         // true: install/update/remove on "Program files" (ADMIN MODE)
									// false: install/update/remove on NPP_INST or install on %APPDATA%, update/remove on %APPDATA% & NPP_INST (NORMAL MODE)
									
	bool _isAppDataPluginsAllowed;  // true: install on %APPDATA%, update / remove on %APPDATA% & "Program files" or NPP_INST

	generic_string _nppInstallPath;
	generic_string _appdataPath;

	// it should determinate :
	// 1. deployment location : %ProgramFile%   %appdata%   %other%
	// 2. gitup launch mode:    ADM             ADM         NOMAL
	bool shouldLaunchInAdmMode() { return _isInProgramFiles; };
};

class PluginsAdminDlg final : public StaticDialog
{
public :
	PluginsAdminDlg() {};
	~PluginsAdminDlg() {
		_availableListView.destroy();
		_updateListView.destroy();
		_installedListView.destroy();
	}
    void init(HINSTANCE hInst, HWND parent)	{
        Window::init(hInst, parent);
	};

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);

    void doDialog(bool isRTL = false) {
    	if (!isCreated())
		{
			create(IDD_PLUGINSADMIN_DLG, isRTL);
		}

		if (!::IsWindowVisible(_hSelf))
		{

		}
	    display();
    };

	void switchDialog(int indexToSwitch);

	bool updateListAndLoadFromJson(); // call GitUup for the 1st time
	void updateAvailableListView();
	void updateInstalledListView();
	void updateUpdateListView();

	void setPluginsManager(PluginsManager *pluginsManager) { _pPluginsManager = pluginsManager; };
	void setAdminMode(bool isAdm) { _nppCurrentStatus._isAdminMode = isAdm; };

	bool installPlugins();
	bool updatePlugins();
	bool removePlugins();

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
	TabBar _tab;

	ListView _availableListView;
	ListView _updateListView;
	ListView _installedListView;
	std::vector<PluginUpdateInfo> _availablePluginList; // All plugins (pluginList.json) - installed plugins 
	std::vector<PluginUpdateInfo> _updatePluginList;    // A list returned by gitup.exe
	std::vector<PluginUpdateInfo> _installedPluginList; // for each installed plugin, check its json file

	PluginsManager *_pPluginsManager = nullptr;
	NppCurrentStatus _nppCurrentStatus;

	void collectNppCurrentStatusInfos();
	bool searchInPlugins(bool isNextMode) const;
	const bool inNames = true;
	const bool inDescs = false;
	long searchFromCurrentSel(generic_string str2search, bool inWhichPart, bool isNextMode) const;
	long searchInNamesFromCurrentSel(generic_string str2search, bool isNextMode) const {
		return searchFromCurrentSel(str2search, inNames, isNextMode);
	};

	long searchInDescsFromCurrentSel(generic_string str2search, bool isNextMode) const {
		return searchFromCurrentSel(str2search, inDescs, isNextMode);
	};

	bool loadFromPluginInfos();
	bool checkUpdates();
};

