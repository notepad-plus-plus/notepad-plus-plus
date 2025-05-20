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

#include <cwctype>
#include <algorithm>
#include "StaticDialog.h"
#include "pluginsAdminRes.h"
#include "TabBar.h"
#include "ListView.h"
#include "tinyxml.h"
#include "URLCtrl.h"

class PluginsManager;

struct PluginUpdateInfo
{
	std::wstring _fullFilePath; // only for the installed Plugin

	std::wstring _folderName;   // plugin folder name - should be the same name with plugin and should be uniq among the plugins
	std::wstring _displayName;  // plugin description name
	Version _version;
	// Optional
	std::pair<Version, Version> _nppCompatibleVersions; // compatible to Notepad++ interval versions: <from, to> example: 
	                                                    // <0.0.0.0, 0.0.0.0>: plugin is compatible to all Notepad++ versions (due to invalid format set)
	                                                    // <6.9, 6.9>: plugin is compatible to only v6.9
	                                                    // <4.2, 6.6.6>: from v4.2 (included) to v6.6.6 (included)
	                                                    // <0.0.0.0, 8.2.1> all until v8.2.1 (included)
	                                                    // <8.3, 0.0.0.0> from v8.3 (included) to all

	// Optional
	std::pair<std::pair<Version, Version>, std::pair<Version, Version>> _oldVersionCompatibility; // Used only by Plugin Manager to filter plugins while loading plugins
	                                                                                              // The 1st interval versions are for old plugins' versions
	                                                                                              // The 2nd interval versions are for Notepad++ versions
	                                                                                              // which are compatible with the old plugins' versions given in the 1st interval
	
	std::wstring _homepage;
	std::wstring _sourceUrl;
	std::wstring _description;
	std::wstring _author;
	std::wstring _id;           // Plugin package ID: SHA-256 hash
	std::wstring _repository;
	bool _isVisible = true;       // if false then it should not be displayed 

	std::wstring describe();
	PluginUpdateInfo() = default;
	PluginUpdateInfo(const std::wstring& fullFilePath, const std::wstring& fileName);
};

struct NppCurrentStatus
{
	bool _isAdminMode = false;         // can launch gitup en Admin mode directly

	bool _isInProgramFiles = true;     // true: install/update/remove on "Program files" (ADMIN MODE)
	                                   // false: install/update/remove on NPP_INST or install on %APPDATA%, update/remove on %APPDATA% & NPP_INST (NORMAL MODE)
									
	bool _isAppDataPluginsAllowed = false;  // true: install on %APPDATA%, update / remove on %APPDATA% & "Program files" or NPP_INST

	std::wstring _nppInstallPath;
	std::wstring _appdataPath;

	// it should determine :
	// 1. deployment location : %ProgramFile%   %appdata%   %other%
	// 2. gitup launch mode:    ADM             ADM         NOMAL
	bool shouldLaunchInAdmMode() { return _isInProgramFiles; };
};

enum COLUMN_TYPE { COLUMN_PLUGIN, COLUMN_VERSION };
enum SORT_TYPE { DISPLAY_NAME_ALPHABET_ENCREASE, DISPLAY_NAME_ALPHABET_DECREASE };


struct SortDisplayNameDecrease final
{
	bool operator() (const PluginUpdateInfo* l, const PluginUpdateInfo* r)
	{
		return (l->_displayName.compare(r->_displayName) <= 0);
	}
};

class PluginViewList
{
friend class PluginsAdminDlg;

public:
	PluginViewList() = default;
	~PluginViewList() {
		_ui.destroy();
		for (auto& i : _list)
		{
			delete i;
		}
	};

	void pushBack(PluginUpdateInfo* pi);
	HWND getViewHwnd() { return _ui.getHSelf(); };
	void displayView(bool doShow) const { _ui.display(doShow); };
	std::vector<size_t> getCheckedIndexes() const { return _ui.getCheckedIndexes(); };
	std::vector<PluginUpdateInfo*> fromUiIndexesToPluginInfos(const std::vector<size_t>& ) const;
	long getSelectedIndex() const { return _ui.getSelectedIndex(); };
	void setSelection(int index) const { _ui.setSelection(index); };
	void initView(HINSTANCE hInst, HWND parent) { _ui.init(hInst, parent); };
	void addColumn(const columnInfo & column2Add) { _ui.addColumn(column2Add); };
	void reSizeView(RECT & rc) { _ui.reSizeToWH(rc); }
	void setViewStyleOption(int32_t extraStyle) { _ui.setStyleOption(extraStyle); };
	size_t nbItem() const { return _ui.nbItem(); };
	PluginUpdateInfo* getPluginInfoFromUiIndex(size_t index) const { return reinterpret_cast<PluginUpdateInfo*>(_ui.getLParamFromIndex(static_cast<int>(index))); };
	PluginUpdateInfo* findPluginInfoFromFolderName(const std::wstring& folderName, int& index) const;
	bool removeFromListIndex(size_t index2remove);
	bool hideFromListIndex(size_t index2Hide);
	bool removeFromFolderName(const std::wstring& folderName);
	bool removeFromUiIndex(size_t index2remove);
	bool hideFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide);
	bool restore(const std::wstring& folderName);
	bool removeFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide);
	void changeColumnName(COLUMN_TYPE index, const wchar_t *name2change);

private:
	// _list & _ui should keep being synchronized
	std::vector<PluginUpdateInfo*> _list;
	ListView _ui;

	SORT_TYPE _sortType = DISPLAY_NAME_ALPHABET_ENCREASE;
};

enum LIST_TYPE { AVAILABLE_LIST, UPDATES_LIST, INSTALLED_LIST, INCOMPATIBLE_LIST };


class PluginsAdminDlg final : public StaticDialog
{
public :
	PluginsAdminDlg();
	~PluginsAdminDlg() = default;

	void create(int dialogID, bool isRTL = false, bool msgDestParent = true) override;

	void doDialog(bool isRTL = false) {
		if (!isCreated())
		{
			create(IDD_PLUGINSADMIN_DLG, isRTL);
		}
		display();
	};

	bool initFromJson();

	void switchDialog(int indexToSwitch);
	void setPluginsManager(PluginsManager *pluginsManager) { _pPluginsManager = pluginsManager; };

	bool updateList();
	void setAdminMode(bool isAdm) { _nppCurrentStatus._isAdminMode = isAdm; };

	bool installPlugins();
	bool updatePlugins();
	bool removePlugins();

	void changeTabName(LIST_TYPE index, const wchar_t *name2change);
	void changeColumnName(COLUMN_TYPE index, const wchar_t *name2change);
	std::wstring getPluginListVerStr() const;
	const PluginViewList & getAvailablePluginUpdateInfoList() const {
		return _availableList;
	};
	
	PluginViewList & getIncompatibleList() {
		return _incompatibleList;
	};

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	std::wstring _updaterDir;
	std::wstring _updaterFullPath;
	std::wstring _pluginListFullPath;

	TabBar _tab;

	std::wstring _pluginListVersion;
	URLCtrl _repoLink;

	PluginViewList _availableList;    // A permanent list, once it's loaded (no removal - only hide or show) 
	PluginViewList _updateList;       // A dynamical list, items are removable
	PluginViewList _installedList;    // A dynamical list, items are removable
	PluginViewList _incompatibleList; // A permanent list, once it's loaded (no removal - only hide or show) 

	PluginsManager *_pPluginsManager = nullptr;
	NppCurrentStatus _nppCurrentStatus;

	void collectNppCurrentStatusInfos();
	bool searchInPlugins(bool isNextMode) const;
	const bool _inNames = true;
	const bool _inDescs = false;
	bool isFoundInListFromIndex(const PluginViewList& inWhichList,int index, const std::wstring& str2search, bool inWhichPart) const;
	long searchFromCurrentSel(const PluginViewList& inWhichList, const std::wstring& str2search, bool inWhichPart, bool isNextMode) const;
	long searchInNamesFromCurrentSel(const PluginViewList& inWhichList, const std::wstring& str2search, bool isNextMode) const {
		return searchFromCurrentSel(inWhichList, str2search, _inNames, isNextMode);
	};

	long searchInDescsFromCurrentSel(const PluginViewList& inWhichList, const std::wstring& str2search, bool isNextMode) const {
		return searchFromCurrentSel(inWhichList, str2search, _inDescs, isNextMode);
	};
	
	bool initAvailablePluginsViewFromList();
	bool initIncompatiblePluginList();
	bool loadFromPluginInfos();
	bool checkUpdates();

	enum Operation {
		pa_install = 0,
		pa_update = 1,
		pa_remove = 2
	};
	bool exitToInstallRemovePlugins(Operation op, const std::vector<PluginUpdateInfo*>& puis);
};

