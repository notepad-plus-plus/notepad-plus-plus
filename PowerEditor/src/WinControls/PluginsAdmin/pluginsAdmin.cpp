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

#include "json.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <shlobj.h>
#include <shlwapi.h>
#include "pluginsAdmin.h"
#include "ScintillaEditView.h"
#include "localization.h"
#include "Processus.h"
#include "PluginsManager.h"
#include "verifySignedfile.h"

#define TEXTFILE        256
#define IDR_PLUGINLISTJSONFILE  101

using namespace std;
using nlohmann::json;



wstring PluginUpdateInfo::describe()
{
	wstring desc;
	const wchar_t *EOL = L"\r\n";
	if (!_description.empty())
	{
		desc = _description;
		desc += EOL;
	}

	if (!_author.empty())
	{
		desc += L"Author: ";
		desc += _author;
		desc += EOL;
	}

	if (!_homepage.empty())
	{
		desc += L"Homepage: ";
		desc += _homepage;
		desc += EOL;
	}

	return desc;
}

/// Try to find in the Haystack the Needle - ignore case
bool findStrNoCase(const wstring & strHaystack, const wstring & strNeedle)
{
	auto it = std::search(
		strHaystack.begin(), strHaystack.end(),
		strNeedle.begin(), strNeedle.end(),
		[](wchar_t ch1, wchar_t ch2){return towupper(ch1) == towupper(ch2); }
	);
	return (it != strHaystack.end());
}

bool PluginsAdminDlg::isFoundInListFromIndex(const PluginViewList& inWhichList, int index, const wstring& str2search, bool inWhichPart) const
{
	const PluginUpdateInfo* pui = inWhichList.getPluginInfoFromUiIndex(index);
	wstring searchIn;
	if (inWhichPart == _inNames)
		searchIn = pui->_displayName;
	else //(inWhichPart == inDescs)
		searchIn = pui->_description;

	return (findStrNoCase(searchIn, str2search));
}

long PluginsAdminDlg::searchFromCurrentSel(const PluginViewList& inWhichList, const wstring& str2search, bool inWhichPart, bool isNextMode) const
{
	// search from curent selected item or from the beginning
	long currentIndex = inWhichList.getSelectedIndex();
	int nbItem = static_cast<int>(inWhichList.nbItem());
	if (currentIndex == -1)
	{
		// no selection, let's search from 0 to the end
		for (int i = 0; i < nbItem; ++i)
		{
			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
				return i;
		}
	}
	else // with selection, let's search from currentIndex
	{
		// from current position to the end
		for (int i = currentIndex + (isNextMode ? 1 : 0); i < nbItem; ++i)
		{
			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
				return i;
		}

		// from to begining to current position
		for (int i = 0; i < currentIndex + (isNextMode ? 1 : 0); ++i)
		{
			if (isFoundInListFromIndex(inWhichList, i, str2search, inWhichPart))
				return i;
		}
	}
	return -1;
}

void PluginsAdminDlg::create(int dialogID, bool isRTL, bool msgDestParent)
{
	// get plugin installation path and launch mode (Admin or normal)
	collectNppCurrentStatusInfos();

	StaticDialog::create(dialogID, isRTL, msgDestParent);

	RECT rect{};
	getClientRect(rect);
	_tab.init(_hInst, _hSelf, false, true);
	NppDarkMode::subclassTabControl(_tab.getHSelf());

	const wchar_t *available = L"Available";
	const wchar_t *updates = L"Updates";
	const wchar_t *installed = L"Installed";
	const wchar_t *incompatible = L"Incompatible";

	_tab.insertAtEnd(available);
	_tab.insertAtEnd(updates);
	_tab.insertAtEnd(installed);
	_tab.insertAtEnd(incompatible);

	RECT rcDesc{};
	getMappedChildRect(IDC_PLUGINADM_EDIT, rcDesc);

	const long margeX = _dpiManager.getSystemMetricsForDpi(SM_CXEDGE);
	const long margeY = _dpiManager.scale(13);

	rect.bottom = rcDesc.bottom + margeY;
	_tab.reSizeTo(rect);
	_tab.display();

	RECT rcSearch{};
	getMappedChildRect(IDC_PLUGINADM_SEARCH_EDIT, rcSearch);

	RECT listRect{
		rcDesc.left - margeX,
		rcSearch.bottom + margeY,
		rcDesc.right + _dpiManager.getSystemMetricsForDpi(SM_CXVSCROLL) + margeX,
		rcDesc.top - margeY
	};

	NppParameters& nppParam = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
	wstring pluginStr = pNativeSpeaker->getAttrNameStr(L"Plugin", "PluginAdmin", "Plugin");
	wstring vesionStr = pNativeSpeaker->getAttrNameStr(L"Version", "PluginAdmin", "Version");

	const COLORREF fgColor = nppParam.getCurrentDefaultFgColor();
	const COLORREF bgColor = nppParam.getCurrentDefaultBgColor();

	const size_t szColVer = _dpiManager.scale(100);
	const size_t szColName = szColVer * 2;

	auto initListView = [&](PluginViewList& list) -> void {
		list.addColumn(columnInfo(pluginStr, szColName));
		list.addColumn(columnInfo(vesionStr, szColVer));
		list.setViewStyleOption(LVS_EX_CHECKBOXES);
		list.initView(_hInst, _hSelf);
		const HWND hList = list.getViewHwnd();
		ListView_SetBkColor(hList, bgColor);
		ListView_SetTextBkColor(hList, bgColor);
		ListView_SetTextColor(hList, fgColor);
		const auto style = ::GetWindowLongPtr(hList, GWL_STYLE);
		::SetWindowLongPtr(hList, GWL_STYLE, style | WS_TABSTOP);
		::SetWindowPos(hList, ::GetDlgItem(_hSelf, IDC_PLUGINADM_RESEARCH_NEXT), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // to allow tab switch
		list.reSizeView(listRect);
	};

	initListView(_availableList);
	initListView(_updateList);
	initListView(_installedList);
	initListView(_incompatibleList);

	switchDialog(0);

	NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
	NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

	HWND hPluginListVersionNumber = ::GetDlgItem(_hSelf, IDC_PLUGINLIST_VERSIONNUMBER_STATIC);
	::SetWindowText(hPluginListVersionNumber, _pluginListVersion.c_str());

	_repoLink.init(_hInst, _hSelf);
	_repoLink.create(::GetDlgItem(_hSelf, IDC_PLUGINLIST_ADDR), L"https://github.com/notepad-plus-plus/nppPluginList");

	goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
}

void PluginsAdminDlg::collectNppCurrentStatusInfos()
{
	NppParameters& nppParam = NppParameters::getInstance();
	_nppCurrentStatus._nppInstallPath = nppParam.getNppPath();

	_nppCurrentStatus._isAppDataPluginsAllowed = ::SendMessage(_hParent, NPPM_GETAPPDATAPLUGINSALLOWED, 0, 0) == TRUE;
	_nppCurrentStatus._appdataPath = nppParam.getAppDataNppDir();
	wstring programFilesPath = NppParameters::getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
	_nppCurrentStatus._isInProgramFiles = (_nppCurrentStatus._nppInstallPath.find(programFilesPath) == 0);

}

vector<PluginUpdateInfo*> PluginViewList::fromUiIndexesToPluginInfos(const std::vector<size_t>& uiIndexes) const
{
	std::vector<PluginUpdateInfo*> r;
	size_t nb = _ui.nbItem();

	for (const auto &i : uiIndexes)
	{
		if (i < nb)
		{
			r.push_back(getPluginInfoFromUiIndex(i));
		}
	}
	return r;
}

PluginsAdminDlg::PluginsAdminDlg()
{
	// Get wingup path
	NppParameters& nppParameters = NppParameters::getInstance();
	_updaterDir = nppParameters.getNppPath();
	pathAppend(_updaterDir, L"updater");
	_updaterFullPath = _updaterDir;
	pathAppend(_updaterFullPath, L"gup.exe");

	// get plugin-list path
	_pluginListFullPath = nppParameters.getPluginConfDir();

#ifdef DEBUG // if not debug, then it's release
	// load from nppPluginList.json instead of nppPluginList.dll
	pathAppend(_pluginListFullPath, L"nppPluginList.json");
#else //RELEASE
	pathAppend(_pluginListFullPath, L"nppPluginList.dll");
#endif
}

wstring PluginsAdminDlg::getPluginListVerStr() const
{
	Version v;
	v.setVersionFrom(_pluginListFullPath);
	return v.toString();
}

bool PluginsAdminDlg::exitToInstallRemovePlugins(Operation op, const vector<PluginUpdateInfo*>& puis)
{
	wstring opStr;
	if (op == pa_install)
		opStr = L"-unzipTo ";
	else if (op == pa_update)
		opStr = L"-unzipTo -clean ";
	else if (op == pa_remove)
		opStr = L"-clean ";
	else
		return false;

	NppParameters& nppParameters = NppParameters::getInstance();
	wstring updaterDir = nppParameters.getNppPath();
	updaterDir += L"\\updater\\";

	wstring updaterFullPath = updaterDir + L"gup.exe";

	wstring updaterParams = opStr;

	wchar_t nppFullPath[MAX_PATH]{};
	::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
	updaterParams += L"\"";
	updaterParams += nppFullPath;
	updaterParams += L"\" ";

	updaterParams += L"\"";
	updaterParams += nppParameters.getPluginRootDir();
	updaterParams += L"\"";

	for (const auto &i : puis)
	{
		if (op == pa_install || op == pa_update)
		{
			// add folder to operate
			updaterParams += L" \"";
			updaterParams += i->_folderName;
			updaterParams += L" ";
			updaterParams += i->_repository;
			updaterParams += L" ";
			updaterParams += i->_id;
			updaterParams += L"\"";
		}
		else // op == pa_remove
		{
			// add folder to operate
			updaterParams += L" \"";
			wstring folderName = i->_folderName;
			if (folderName.empty())
			{
				auto lastindex = i->_displayName.find_last_of(L".");
				if (lastindex != wstring::npos)
					folderName = i->_displayName.substr(0, lastindex);
				else
					folderName = i->_displayName;	// This case will never occur, but in case if it occurs too
													// just putting the plugin name, so that whole plugin system is not screewed.
			}
			updaterParams += folderName;
			updaterParams += L"\"";
		}
	}

	// Ask user's confirmation
	NativeLangSpeaker *pNativeSpeaker = nppParameters.getNativeLangSpeaker();
	auto res = pNativeSpeaker->messageBox("ExitToUpdatePlugins",
		_hSelf,
		L"If you click YES, you will quit Notepad++ to continue the operations.\nNotepad++ will be restarted after all the operations are terminated.\nContinue?",
		L"Notepad++ is about to exit",
		MB_YESNO | MB_APPLMODAL);

	if (res == IDYES)
	{
		NppParameters& nppParam = NppParameters::getInstance();

		// gup path: makes trigger ready
		nppParam.setWingupFullPath(updaterFullPath);

		// op: -clean or "-clean -unzip"
		// application path: Notepad++ path to be relaunched
		// plugin global path
		// plugin names or "plugin names + download url"
		nppParam.setWingupParams(updaterParams);

		// gup folder path
		nppParam.setWingupDir(updaterDir);

		// Quite Notepad++ so just before quitting Notepad++ launches gup with needed arguments  
		::PostMessage(_hParent, WM_COMMAND, IDM_FILE_EXIT, 0);
	}

	return true;
}

bool PluginsAdminDlg::installPlugins()
{
	// Need to exit Notepad++

	vector<size_t> indexes = _availableList.getCheckedIndexes();
	vector<PluginUpdateInfo*> puis = _availableList.fromUiIndexesToPluginInfos(indexes);

	return exitToInstallRemovePlugins(pa_install, puis);
}

bool PluginsAdminDlg::updatePlugins()
{
	// Need to exit Notepad++

	vector<size_t> indexes = _updateList.getCheckedIndexes();
	vector<PluginUpdateInfo*> puis = _updateList.fromUiIndexesToPluginInfos(indexes);

	return exitToInstallRemovePlugins(pa_update, puis);
}

bool PluginsAdminDlg::removePlugins()
{
	// Need to exit Notepad++

	vector<size_t> indexes = _installedList.getCheckedIndexes();
	vector<PluginUpdateInfo*> puis = _installedList.fromUiIndexesToPluginInfos(indexes);

	return exitToInstallRemovePlugins(pa_remove, puis);
}

void PluginsAdminDlg::changeTabName(LIST_TYPE index, const wchar_t *name2change)
{
	TCITEM tie{};
	tie.mask = TCIF_TEXT;
	tie.pszText = (wchar_t *)name2change;
	TabCtrl_SetItem(_tab.getHSelf(), index, &tie);

	wchar_t label[MAX_PATH]{};
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
}

void PluginsAdminDlg::changeColumnName(COLUMN_TYPE index, const wchar_t *name2change)
{
	_availableList.changeColumnName(index, name2change);
	_updateList.changeColumnName(index, name2change);
	_installedList.changeColumnName(index, name2change);
	_incompatibleList.changeColumnName(index, name2change);
}

void PluginViewList::changeColumnName(COLUMN_TYPE index, const wchar_t *name2change)
{
	_ui.setColumnText(index, name2change);
}

bool PluginViewList::removeFromFolderName(const wstring& folderName)
{

	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		const PluginUpdateInfo* pi = getPluginInfoFromUiIndex(i);
		if (pi->_folderName == folderName)
		{
			if (!_ui.removeFromIndex(i))
				return false;

			for (size_t j = 0; j < _list.size(); ++j)
			{
				if (_list[j] == pi)
				{
					_list.erase(_list.begin() + j);
					return true;
				}
			}
		}
	}
	return false;
}

void PluginViewList::pushBack(PluginUpdateInfo* pi)
{
	_list.push_back(pi);

	vector<wstring> values2Add;
	values2Add.push_back(pi->_displayName);
	Version v = pi->_version;
	values2Add.push_back(v.toString());

	// add in order
	size_t i = _ui.findAlphabeticalOrderPos(pi->_displayName, _sortType == DISPLAY_NAME_ALPHABET_ENCREASE ? _ui.sortEncrease : _ui.sortDecrease);
	_ui.addLine(values2Add, reinterpret_cast<LPARAM>(pi), static_cast<int>(i));
}

// intervalVerStr format:
// 
// "6.9"          : exact version 6.9
// "[4.2,6.6.6]"  : from version 4.2 to 6.6.6 inclusive
// "[8.3,]"       : any version from 8.3 to the latest one
// "[,8.2.1]"     : 8.2.1 and any previous version
//
std::pair<Version, Version> getIntervalVersions(wstring intervalVerStr)
{
	std::pair<Version, Version> result;

	if (intervalVerStr.empty())
		return result;

	const size_t indexEnd = intervalVerStr.length() - 1;
	if (intervalVerStr[0] == '[' && intervalVerStr[indexEnd] == ']') // interval versions format
	{
		wstring cleanIntervalVerStr = intervalVerStr.substr(1, indexEnd - 1);
		vector<wstring> versionVect;
		cutStringBy(cleanIntervalVerStr.c_str(), versionVect, ',', true);
		if (versionVect.size() == 2)
		{
			if (!versionVect[0].empty() && !versionVect[1].empty()) // "[4.2,6.6.6]" : from version 4.2 to 6.6.6 inclusive
			{
				result.first = Version(versionVect[0]);
				result.second = Version(versionVect[1]);
			}
			else if (!versionVect[0].empty() && versionVect[1].empty()) // "[8.3,]" : any version from 8.3 to the latest one
			{
				result.first = Version(versionVect[0]);
			}
			else if (versionVect[0].empty() && !versionVect[1].empty()) // "[,8.2.1]" : 8.2.1 and any previous version
			{
				result.second = Version(versionVect[1]);
			}
		}
	}
	else if (intervalVerStr[0] != '[' && intervalVerStr[indexEnd] != ']') // one version format -> "6.9" : exact version 6.9
	{
		result.first = Version(intervalVerStr);
		result.second = Version(intervalVerStr);
	}
	else // invalid format
	{
		// do nothing
	}

	return result;
}

// twoIntervalVerStr format:
// "[4.2,6.6.6][6.4,8.9]"  : The 1st interval from version 4.2 to 6.6.6 inclusive, the 2nd interval from version 6.4 to 8.9
// "[8.3,][6.9,6.9]"       : The 1st interval any version from 8.3 to the latest version, the 2nd interval present only version 6.9
// "[,8.2.1][4.4,]"        : The 1st interval 8.2.1 and any previous version, , the 2nd interval any version from 4.4 to the latest version
std::pair<std::pair<Version, Version>, std::pair<Version, Version>> getTwoIntervalVersions(const wstring& twoIntervalVerStr)
{
	std::pair<std::pair<Version, Version>, std::pair<Version, Version>> r;
	wstring sep = L"][";
	wstring::size_type pos = twoIntervalVerStr.find(sep, 0);
	if (pos == string::npos)
		return r;

	wstring intervalStr1 = twoIntervalVerStr.substr(0, pos + 1);
	wstring intervalStr2 = twoIntervalVerStr.substr(pos + 1, twoIntervalVerStr.length() - pos + 1);

	r.first = getIntervalVersions(intervalStr1);
	r.second = getIntervalVersions(intervalStr2);

	return r;
}

bool loadFromJson(std::vector<PluginUpdateInfo*>& pl, wstring& verStr, const json& j)
{
	if (j.empty())
		return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	json jVerStr = j["version"];
	if (jVerStr.empty() || jVerStr.type() != json::value_t::string)
		return false;

	string s = jVerStr.get<std::string>();
	verStr = wmc.char2wchar(s.c_str(), CP_ACP);

	json jArray = j["npp-plugins"];
	if (jArray.empty() || jArray.type() != json::value_t::array)
		return false;
	
	for (const auto& i : jArray)
	{
		try {
			PluginUpdateInfo* pi = new PluginUpdateInfo();

			string valStr = i.at("folder-name").get<std::string>();
			pi->_folderName = wmc.char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("display-name").get<std::string>();
			pi->_displayName = wmc.char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("author").get<std::string>();
			pi->_author = wmc.char2wchar(valStr.c_str(), CP_UTF8);

			valStr = i.at("description").get<std::string>();
			pi->_description = wmc.char2wchar(valStr.c_str(), CP_UTF8);

			valStr = i.at("id").get<std::string>();
			pi->_id = wmc.char2wchar(valStr.c_str(), CP_ACP);

			try {
				valStr = i.at("version").get<std::string>();
				wstring newValStr(valStr.begin(), valStr.end());
				pi->_version = Version(newValStr);

				if (i.contains("npp-compatible-versions"))
				{
					json jNppCompatibleVer = i["npp-compatible-versions"];

					string versionsStr = jNppCompatibleVer.get<std::string>();
					wstring nppCompatibleVersionStr(versionsStr.begin(), versionsStr.end());
					pi->_nppCompatibleVersions = getIntervalVersions(nppCompatibleVersionStr);
				}

				if (i.contains("old-versions-compatibility"))
				{
					json jOldVerCompatibility = i["old-versions-compatibility"];

					string versionsStr = jOldVerCompatibility.get<std::string>();
					wstring oldVerCompatibilityStr(versionsStr.begin(), versionsStr.end());
					pi->_oldVersionCompatibility = getTwoIntervalVersions(oldVerCompatibilityStr);
				}
			}
			catch (const wstring& exceptionStr)
			{
				wstring msg = pi->_displayName;
				msg += L": ";
				throw msg + exceptionStr;
			}
			valStr = i.at("repository").get<std::string>();
			pi->_repository = wmc.char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("homepage").get<std::string>();
			pi->_homepage = wmc.char2wchar(valStr.c_str(), CP_ACP);

			pl.push_back(pi);
		}
#ifdef DEBUG
		catch (const wstring& exceptionStr)
		{
			::MessageBox(NULL, exceptionStr.c_str(), L"Exception caught in: PluginsAdmin loadFromJson()", MB_ICONERROR);
			continue;
		}

		catch (std::exception& e)
		{
			::MessageBoxA(NULL, e.what(), "Exception caught in: PluginsAdmin loadFromJson()", MB_ICONERROR);
			continue;
		}
#endif
		catch (...) // If one of mandatory properties is missing or with the incorrect format, an exception is thrown then this plugin will be ignored
		{
#ifdef DEBUG
			::MessageBoxA(NULL, "An unknown exception is just caught", "Unknown Exception", MB_OK);
#endif
			continue; 
		}
	}
	return true;
}

PluginUpdateInfo::PluginUpdateInfo(const wstring& fullFilePath, const wstring& filename)
{
	if (!doesFileExist(fullFilePath.c_str()))
		return;

	_fullFilePath = fullFilePath;
	_displayName = filename;

	std::string content = getFileContent(fullFilePath.c_str());
	if (content.empty())
		return;

	_version.setVersionFrom(fullFilePath);
}

typedef const char * (__cdecl * PFUNCGETPLUGINLIST)();


bool PluginsAdminDlg::initFromJson()
{
	// GUP.exe doesn't work under XP
	winVer winVersion = (NppParameters::getInstance()).getWinVersion();
	if (winVersion <= WV_XP)
	{
		return false;
	}

	if (!doesFileExist(_pluginListFullPath.c_str()))
	{
		return false;
	}

	if (!doesFileExist(_updaterFullPath.c_str()))
	{
		return false;
	}

	json j;

#ifdef DEBUG // if not debug, then it's release
	
	// load from nppPluginList.json instead of nppPluginList.dll
#ifdef __MINGW32__
	ifstream nppPluginListJson(wstring2string(_pluginListFullPath, CP_UTF8));
#else // MSVC supports UTF-16 path names in file stream constructors 
	ifstream nppPluginListJson(_pluginListFullPath);
#endif
	nppPluginListJson >> j;

#else //RELEASE

	// check the signature on default location : %APPDATA%\Notepad++\plugins\config\pl\nppPluginList.dll or NPP_INST_DIR\plugins\config\pl\nppPluginList.dll
	
	SecurityGuard securityGuard;
	bool isSecured = securityGuard.checkModule(_pluginListFullPath, nm_pluginList);

	if (!isSecured)
		return false;

	isSecured = securityGuard.checkModule(_updaterFullPath, nm_gup);

	if (isSecured)
	{
		HMODULE hLib = NULL;
		hLib = ::LoadLibraryEx(_pluginListFullPath.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);

		if (!hLib)
		{
			// Error treatment
			//printStr(L"LoadLibrary PB!!!");
			return false;
		}

		HRSRC rc = ::FindResource(hLib, MAKEINTRESOURCE(IDR_PLUGINLISTJSONFILE), MAKEINTRESOURCE(TEXTFILE));
		if (!rc)
		{
			::FreeLibrary(hLib);
			return false;
		}

		HGLOBAL rcData = ::LoadResource(hLib, rc);
		if (!rcData)
		{
			::FreeLibrary(hLib);
			return false;
		}

		auto size = ::SizeofResource(hLib, rc);
		auto data = static_cast<const char*>(::LockResource(rcData));

		char* buffer = new char[size + 1];
		::memcpy(buffer, data, size);
		buffer[size] = '\0';

		j = j.parse(buffer);

		delete[] buffer;

		::FreeLibrary(hLib);
	}
#endif

	
	return loadFromJson(_availableList._list, _pluginListVersion, j);
}

bool PluginsAdminDlg::updateList()
{
	// initialize the primary view with the plugin list loaded from json 
	initAvailablePluginsViewFromList();

	// initialize update list view
	checkUpdates();

	// initialize incompatible list view
	initIncompatiblePluginList();

	// initialize installed list view
	loadFromPluginInfos();

	return true;
}


bool PluginsAdminDlg::initAvailablePluginsViewFromList()
{
	wchar_t nppFullPathName[MAX_PATH]{};
	GetModuleFileName(NULL, nppFullPathName, MAX_PATH);

	Version nppVer;
	nppVer.setVersionFrom(nppFullPathName);

	for (const auto& i : _availableList._list)
	{
		bool isCompatible = nppVer.isCompatibleTo(i->_nppCompatibleVersions.first, i->_nppCompatibleVersions.second);

		if (isCompatible)
		{
			vector<wstring> values2Add;
			values2Add.push_back(i->_displayName);
			Version v = i->_version;
			values2Add.push_back(v.toString());

			// add in order
			size_t j = _availableList._ui.findAlphabeticalOrderPos(i->_displayName, _availableList._sortType == DISPLAY_NAME_ALPHABET_ENCREASE ? ListView::sortEncrease : ListView::sortDecrease);
			_availableList._ui.addLine(values2Add, reinterpret_cast<LPARAM>(i), static_cast<int>(j));
		}
	}

	return true;
}


bool PluginsAdminDlg::initIncompatiblePluginList()
{
	wchar_t nppFullPathName[MAX_PATH]{};
	GetModuleFileName(NULL, nppFullPathName, MAX_PATH);

	Version nppVer;
	nppVer.setVersionFrom(nppFullPathName);

	for (const auto& i : _incompatibleList._list)
	{
		vector<wstring> values2Add;
		values2Add.push_back(i->_displayName);
		Version v = i->_version;
		values2Add.push_back(v.toString());

		// add in order
		size_t j = _incompatibleList._ui.findAlphabeticalOrderPos(i->_displayName, _incompatibleList._sortType == DISPLAY_NAME_ALPHABET_ENCREASE ? ListView::sortEncrease : ListView::sortDecrease);
		_incompatibleList._ui.addLine(values2Add, reinterpret_cast<LPARAM>(i), static_cast<int>(j));
	}

	return true;
}

bool PluginsAdminDlg::loadFromPluginInfos()
{
	if (!_pPluginsManager)
		return false;

	// Search from loaded plugins, if loaded plugins are in the available list,
	// add them into installed plugins list, and hide them from the available list
	for (const auto& i : _pPluginsManager->_loadedDlls)
	{
		if (i._fileName.length() >= MAX_PATH)
			continue;

		// user file name (without ext. to find whole info in available list
		wchar_t fnNoExt[MAX_PATH]{};
		wcscpy_s(fnNoExt, i._fileName.c_str());
		::PathRemoveExtension(fnNoExt);

		int listIndex;
		PluginUpdateInfo* foundInfo = _availableList.findPluginInfoFromFolderName(fnNoExt, listIndex);
		if (!foundInfo)
		{
			PluginUpdateInfo* pui = new PluginUpdateInfo(i._fullFilePath, i._fileName);
			_installedList.pushBack(pui);
		}
		else
		{
			// Add new updated info to installed list
			PluginUpdateInfo* pui = new PluginUpdateInfo(*foundInfo);
			pui->_fullFilePath = i._fullFilePath;
			pui->_version.setVersionFrom(i._fullFilePath);
			_installedList.pushBack(pui);

			// Hide it from the available list
			_availableList.hideFromListIndex(listIndex);

			// if the installed plugin version is smaller than the one on the available list,
			// put it in the update list as well.
			if (pui->_version < foundInfo->_version)
			{
				PluginUpdateInfo* pui2 = new PluginUpdateInfo(*foundInfo);
				_updateList.pushBack(pui2);
			}
		}
	}

	// Search from unloaded incompatible plugins
	for (size_t j = 0, nb = _incompatibleList.nbItem(); j < nb; j++)
	{
		const PluginUpdateInfo* incompatiblePluginInfo = _incompatibleList.getPluginInfoFromUiIndex(j);
		int listIndex;
		PluginUpdateInfo* foundInfoOfAvailable = _availableList.findPluginInfoFromFolderName(incompatiblePluginInfo->_folderName, listIndex);

		if (foundInfoOfAvailable)
		{
			// if the incompatible plugin version is smaller than the one on the available list, put it in the update list
			if (foundInfoOfAvailable->_version > incompatiblePluginInfo->_version)
			{
				// Hide it from the available list
				_availableList.hideFromListIndex(listIndex);

				PluginUpdateInfo* pui = new PluginUpdateInfo(*foundInfoOfAvailable);
				_updateList.pushBack(pui);
			}
		}
	}

	return true;
}

PluginUpdateInfo* PluginViewList::findPluginInfoFromFolderName(const wstring& folderName, int& index) const
{
	index = 0;
	for (const auto& i : _list)
	{
		if (lstrcmpi(i->_folderName.c_str(), folderName.c_str()) == 0)
			return i;
		++index;
	}
	index = -1;
	return nullptr;
}

bool PluginViewList::removeFromUiIndex(size_t index2remove)
{
	if (index2remove >= _ui.nbItem())
		return false;
	return _ui.removeFromIndex(index2remove);
}

bool PluginViewList::removeFromListIndex(size_t index2remove)
{
	if (index2remove >= _list.size())
		return false;

	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		if (_ui.getLParamFromIndex(static_cast<int>(i)) == reinterpret_cast<LPARAM>(_list[index2remove]))
		{
			if (!_ui.removeFromIndex(i))
				return false;
		}
	}
	
	_list.erase(_list.begin() + index2remove);

	return true;
}

bool PluginViewList::removeFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide)
{
	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		if (_ui.getLParamFromIndex(static_cast<int>(i)) == reinterpret_cast<LPARAM>(pluginInfo2hide))
		{
			if (!_ui.removeFromIndex(static_cast<int>(i)))
			{
				return false;
			}
		}
	}

	for (size_t j = 0; j < _list.size(); ++j)
	{
		if (_list[j] == pluginInfo2hide)
		{
			_list.erase(_list.begin() + j);
			return true;
		}
	}

	return false;
}

bool PluginViewList::hideFromPluginInfoPtr(PluginUpdateInfo* pluginInfo2hide)
{
	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		if (_ui.getLParamFromIndex(static_cast<int>(i)) == reinterpret_cast<LPARAM>(pluginInfo2hide))
		{
			if (!_ui.removeFromIndex(static_cast<int>(i)))
			{
				return false;
			}
			else
			{
				pluginInfo2hide->_isVisible = false;
				return true;
			}
		}
	}
	return false;
}

bool PluginViewList::restore(const wstring& folderName)
{
	for (const auto &i : _list)
	{
		if (i->_folderName == folderName)
		{
			vector<wstring> values2Add;
			values2Add.push_back(i->_displayName);
			Version v = i->_version;
			values2Add.push_back(v.toString());
			values2Add.push_back(L"Yes");
			_ui.addLine(values2Add, reinterpret_cast<LPARAM>(i));

			i->_isVisible = true;

			return true;
		}
	}
	return false;
}

bool PluginViewList::hideFromListIndex(size_t index2hide)
{
	if (index2hide >= _list.size())
		return false;

	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		if (_ui.getLParamFromIndex(static_cast<int>(i)) == reinterpret_cast<LPARAM>(_list[index2hide]))
		{
			if (!_ui.removeFromIndex(static_cast<int>(i)))
				return false;
		}
	}

	_list[index2hide]->_isVisible = false;

	return true;
}

bool PluginsAdminDlg::checkUpdates()
{
	return true;
}

// begin insentive-case search from the second key-in character
bool PluginsAdminDlg::searchInPlugins(bool isNextMode) const
{
	constexpr int maxLen = 256;
	wchar_t txt2search[maxLen]{};
	::GetDlgItemText(_hSelf, IDC_PLUGINADM_SEARCH_EDIT, txt2search, maxLen);
	if (lstrlen(txt2search) < 2)
		return false;

	HWND tabHandle = _tab.getHSelf();
	int inWhichTab = int(::SendMessage(tabHandle, TCM_GETCURSEL, 0, 0));

	const PluginViewList* inWhichList = nullptr;
	switch (inWhichTab)
	{
	case 3:
		inWhichList = &_incompatibleList;
		break;
	case 2:
		inWhichList = &_installedList;
		break;
	case 1:
		inWhichList = &_updateList;
		break;
	case 0:
	default:
		inWhichList = &_availableList;
		break;
	}


	long foundIndex = searchInNamesFromCurrentSel(*inWhichList, txt2search, isNextMode);
	if (foundIndex == -1)
		foundIndex = searchInDescsFromCurrentSel(*inWhichList, txt2search, isNextMode);

	if (foundIndex == -1)
		return false;

	inWhichList->setSelection(foundIndex);

	return true;
}

void PluginsAdminDlg::switchDialog(int indexToSwitch)
{
	wstring desc;
	bool showAvailable, showUpdate, showInstalled, showIncompatibile;
	switch (indexToSwitch)
	{
		case 0: // available plugins
		{
			showAvailable = true;
			showUpdate = false;
			showInstalled = false;
			showIncompatibile = false;

			long infoIndex = _availableList.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_availableList.nbItem()))
				desc = _availableList.getPluginInfoFromUiIndex(infoIndex)->describe();
		}
		break;

		case 1: // to be updated plugins
		{
			showAvailable = false;
			showUpdate = true;
			showInstalled = false;
			showIncompatibile = false;
			
			long infoIndex = _updateList.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_updateList.nbItem()))
				desc = _updateList.getPluginInfoFromUiIndex(infoIndex)->describe();
		}
		break;

		case 2: // installed plugin
		{
			showAvailable = false;
			showUpdate = false;
			showInstalled = true;
			showIncompatibile = false;

			long infoIndex = _installedList.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_installedList.nbItem()))
				desc = _installedList.getPluginInfoFromUiIndex(infoIndex)->describe();
		}
		break;

		case 3: // incompability plugins
		{
			showAvailable = false;
			showUpdate = false;
			showInstalled = false;
			showIncompatibile = true;

			long infoIndex = _incompatibleList.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_incompatibleList.nbItem()))
				desc = _incompatibleList.getPluginInfoFromUiIndex(infoIndex)->_description;
		}
		break;

		default:
			return;
	}

	_availableList.displayView(showAvailable);
	_updateList.displayView(showUpdate);
	_installedList.displayView(showInstalled);
	_incompatibleList.displayView(showIncompatibile);

	::SetDlgItemText(_hSelf, IDC_PLUGINADM_EDIT, desc.c_str());

	HWND hInstallButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_INSTALL);
	HWND hUpdateButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_UPDATE);
	HWND hRemoveButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_REMOVE);

	::ShowWindow(hInstallButton, showAvailable ? SW_SHOW : SW_HIDE);
	if (showAvailable)
	{
		vector<size_t> checkedArray = _availableList.getCheckedIndexes();
		showAvailable = checkedArray.size() > 0;
	}
	::EnableWindow(hInstallButton, showAvailable);

	::ShowWindow(hUpdateButton, showUpdate ? SW_SHOW : SW_HIDE);
	if (showUpdate)
	{
		vector<size_t> checkedArray = _updateList.getCheckedIndexes();
		showUpdate = checkedArray.size() > 0;
	}
	::EnableWindow(hUpdateButton, showUpdate);

	::ShowWindow(hRemoveButton, showInstalled ? SW_SHOW : SW_HIDE);
	if (showInstalled)
	{
		vector<size_t> checkedArray = _installedList.getCheckedIndexes();
		showInstalled = checkedArray.size() > 0;
	}
	::EnableWindow(hRemoveButton, showInstalled);
}

intptr_t CALLBACK PluginsAdminDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				const int dlgCtrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
				if (dlgCtrlID == IDC_PLUGINADM_EDIT)
				{
					return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
				}
				return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			_repoLink.destroy();

			const size_t szColVer = _dpiManager.scale(100);
			const size_t szColName = szColVer * 2;

			auto setListViewSize = [&](PluginViewList& list) -> void {
				ListView_SetColumnWidth(list.getViewHwnd(), 0, szColName);
				ListView_SetColumnWidth(list.getViewHwnd(), 1, szColVer);
				};
			
			setListViewSize(_availableList);
			setListViewSize(_updateList);
			setListViewSize(_installedList);
			setListViewSize(_incompatibleList);

			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_PLUGINADM_SEARCH_EDIT:
					{
						searchInPlugins(false);
						return TRUE;
					}

					default:
						return FALSE;
				}
			}

			switch (LOWORD(wParam))
			{
				case IDOK:
					if (::GetFocus() == ::GetDlgItem(_hSelf, IDC_PLUGINADM_SEARCH_EDIT))
						::PostMessage(_hSelf, WM_NEXTDLGCTL, 0, 0L);
					return TRUE;

				case IDCANCEL:
					display(false);
					return TRUE;

				case IDC_NEXT_TAB:
				case IDC_PREV_TAB:
				{
					const int selTabIdx = _tab.getNextOrPrevTabIdx(LOWORD(wParam) == IDC_NEXT_TAB);
					_tab.activateAt(selTabIdx);
					switchDialog(selTabIdx);

					return TRUE;
				}

				case IDC_PLUGINADM_RESEARCH_NEXT:
					searchInPlugins(true);
					return true;

				case IDC_PLUGINADM_INSTALL:
					installPlugins();
					return true;

				case IDC_PLUGINADM_UPDATE:
					updatePlugins();
					return true;

				case IDC_PLUGINADM_REMOVE:
				{
					removePlugins();
					return true;
				}

				default :
					break;
			}
			return FALSE;
		}

		case WM_NOTIFY :
		{
			LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
			if (pnmh->code == TCN_SELCHANGE)
			{
				HWND tabHandle = _tab.getHSelf();
				if (pnmh->hwndFrom == tabHandle)
				{
					int indexClicked = int(::SendMessage(tabHandle, TCM_GETCURSEL, 0, 0));
					switchDialog(indexClicked);
				}
			}
			else if (pnmh->hwndFrom == _availableList.getViewHwnd() ||
                     pnmh->hwndFrom == _updateList.getViewHwnd() ||
                     pnmh->hwndFrom == _installedList.getViewHwnd() ||
                     pnmh->hwndFrom == _incompatibleList.getViewHwnd())
			{
				const PluginViewList* pViewList = nullptr;
				int buttonID = 0;

				if (pnmh->hwndFrom == _availableList.getViewHwnd())
				{
					pViewList = &_availableList;
					buttonID = IDC_PLUGINADM_INSTALL;
				}
				else if (pnmh->hwndFrom == _updateList.getViewHwnd())
				{
					pViewList = &_updateList;
					buttonID = IDC_PLUGINADM_UPDATE;
				}
				else if (pnmh->hwndFrom == _installedList.getViewHwnd())
				{
					pViewList = &_installedList;
					buttonID = IDC_PLUGINADM_REMOVE;
				}
				else // pnmh->hwndFrom == _incompatibleList.getViewHwnd()
				{
					pViewList = &_incompatibleList;
					buttonID = 0;
				}

				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

				if (pnmh->code == LVN_ITEMCHANGED)
				{
					if (pnmv->uChanged & LVIF_STATE)
					{
						if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2) || // checked
							(pnmv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))   // unchecked
						{
							if (buttonID)
							{
								HWND hButton = ::GetDlgItem(_hSelf, buttonID);
								vector<size_t> checkedArray = pViewList->getCheckedIndexes();
								bool showButton = checkedArray.size() > 0;

								::EnableWindow(hButton, showButton);
							}
						}
						else if (pnmv->uNewState & LVIS_SELECTED)
						{
							PluginUpdateInfo* pui = pViewList->getPluginInfoFromUiIndex(pnmv->iItem);
							wstring desc = buttonID ? pui->describe() : pui->_description;
							::SetDlgItemText(_hSelf, IDC_PLUGINADM_EDIT, desc.c_str());
						}
					}
				}
			}

			return TRUE;
		}

		case WM_DESTROY:
		{
			_repoLink.destroy();
			return TRUE;
		}
	}
	return FALSE;
}
