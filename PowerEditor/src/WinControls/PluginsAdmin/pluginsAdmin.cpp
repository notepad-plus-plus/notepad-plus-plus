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



generic_string PluginUpdateInfo::describe()
{
	generic_string desc;
	const TCHAR *EOL = TEXT("\r\n");
	if (!_description.empty())
	{
		desc = _description;
		desc += EOL;
	}

	if (!_author.empty())
	{
		desc += TEXT("Author: ");
		desc += _author;
		desc += EOL;
	}

	if (!_homepage.empty())
	{
		desc += TEXT("Homepage: ");
		desc += _homepage;
		desc += EOL;
	}

	return desc;
}

/// Try to find in the Haystack the Needle - ignore case
bool findStrNoCase(const generic_string & strHaystack, const generic_string & strNeedle)
{
	auto it = std::search(
		strHaystack.begin(), strHaystack.end(),
		strNeedle.begin(), strNeedle.end(),
		[](TCHAR ch1, TCHAR ch2){return _totupper(ch1) == _totupper(ch2); }
	);
	return (it != strHaystack.end());
}

bool PluginsAdminDlg::isFoundInAvailableListFromIndex(int index, const generic_string& str2search, bool inWhichPart) const
{
	PluginUpdateInfo* pui = _availableList.getPluginInfoFromUiIndex(index);
	generic_string searchIn;
	if (inWhichPart == _inNames)
		searchIn = pui->_displayName;
	else //(inWhichPart == inDescs)
		searchIn = pui->_description;

	return (findStrNoCase(searchIn, str2search));
}

long PluginsAdminDlg::searchFromCurrentSel(const generic_string& str2search, bool inWhichPart, bool isNextMode) const
{
	// search from curent selected item or from the beginning
	long currentIndex = _availableList.getSelectedIndex();
	int nbItem = static_cast<int>(_availableList.nbItem());
	if (currentIndex == -1)
	{
		// no selection, let's search from 0 to the end
		for (int i = 0; i < nbItem; ++i)
		{
			if (isFoundInAvailableListFromIndex(i, str2search, inWhichPart))
				return i;
		}
	}
	else // with selection, let's search from currentIndex
	{
		// from current position to the end
		for (int i = currentIndex + (isNextMode ? 1 : 0); i < nbItem; ++i)
		{
			if (isFoundInAvailableListFromIndex(i, str2search, inWhichPart))
				return i;
		}

		// from to begining to current position
		for (int i = 0; i < currentIndex + (isNextMode ? 1 : 0); ++i)
		{
			if (isFoundInAvailableListFromIndex(i, str2search, inWhichPart))
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

	RECT rect;
	getClientRect(rect);
	_tab.init(_hInst, _hSelf, false, true);
	NppDarkMode::subclassTabControl(_tab.getHSelf());
	DPIManager& dpiManager = NppParameters::getInstance()._dpiManager;

	int tabDpiDynamicalHeight = dpiManager.scaleY(13);
	_tab.setFont(TEXT("Tahoma"), tabDpiDynamicalHeight);

	const TCHAR *available = TEXT("Available");
	const TCHAR *updates = TEXT("Updates");
	const TCHAR *installed = TEXT("Installed");

	_tab.insertAtEnd(available);
	_tab.insertAtEnd(updates);
	_tab.insertAtEnd(installed);

	rect.bottom -= dpiManager.scaleX(100);
	_tab.reSizeTo(rect);
	_tab.display();

	const long marge = dpiManager.scaleX(10);
	const int topMarge = dpiManager.scaleY(42);

	HWND hResearchLabel = ::GetDlgItem(_hSelf, IDC_PLUGINADM_SEARCH_STATIC);
	RECT researchLabelRect;
	::GetClientRect(hResearchLabel, &researchLabelRect);
	researchLabelRect.left = rect.left + marge;
	researchLabelRect.top = topMarge + dpiManager.scaleY(4);
	::MoveWindow(hResearchLabel, researchLabelRect.left, researchLabelRect.top, researchLabelRect.right, researchLabelRect.bottom, TRUE);
	::InvalidateRect(hResearchLabel, nullptr, TRUE);

	HWND hResearchEdit = ::GetDlgItem(_hSelf, IDC_PLUGINADM_SEARCH_EDIT);
	RECT researchEditRect;
	::GetClientRect(hResearchEdit, &researchEditRect);
	researchEditRect.left = researchLabelRect.right + marge;
	researchEditRect.top = topMarge + dpiManager.scaleX(2);
	::MoveWindow(hResearchEdit, researchEditRect.left, researchEditRect.top, researchEditRect.right, researchEditRect.bottom, TRUE);
	::InvalidateRect(hResearchEdit, nullptr, TRUE);

	HWND hNextButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_RESEARCH_NEXT);
	RECT researchNextRect;
	::GetClientRect(hNextButton, &researchNextRect);
	researchNextRect.left = researchEditRect.left + researchEditRect.right + marge;
	researchNextRect.top = topMarge;
	::MoveWindow(hNextButton, researchNextRect.left, researchNextRect.top, researchNextRect.right, researchNextRect.bottom, TRUE);
	::InvalidateRect(hNextButton, nullptr, TRUE);

	HWND hActionButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_INSTALL);
	RECT actionRect;
	::GetClientRect(hActionButton, &actionRect);
	long w = actionRect.right - actionRect.left;
	actionRect.left = rect.right - w - marge;
	actionRect.top = topMarge;
	::MoveWindow(hActionButton, actionRect.left, actionRect.top, actionRect.right, actionRect.bottom, TRUE);
	::InvalidateRect(hActionButton, nullptr, TRUE);

	hActionButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_UPDATE);
	::MoveWindow(hActionButton, actionRect.left, actionRect.top, actionRect.right, actionRect.bottom, TRUE);
	::InvalidateRect(hActionButton, nullptr, TRUE);

	hActionButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_REMOVE);
	::MoveWindow(hActionButton, actionRect.left, actionRect.top, actionRect.right, actionRect.bottom, TRUE);
	::InvalidateRect(hActionButton, nullptr, TRUE);

	long actionZoneHeight = dpiManager.scaleY(50);
	rect.top += actionZoneHeight;
	rect.bottom -= actionZoneHeight;

	RECT listRect = rect;
	RECT descRect = rect;

	long descHeight = rect.bottom / 3 - marge * 2;
	long listHeight = (rect.bottom / 3) * 2 - marge * 3;

	listRect.top += marge;
	listRect.bottom = listHeight;
	listRect.left += marge;
	listRect.right -= marge * 2;

	descRect.top += listHeight + marge * 3;
	descRect.bottom = descHeight;
	descRect.left += marge;
	descRect.right -= marge * 2;

	NppParameters& nppParam = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
	generic_string pluginStr = pNativeSpeaker->getAttrNameStr(TEXT("Plugin"), "PluginAdmin", "Plugin");
	generic_string vesionStr = pNativeSpeaker->getAttrNameStr(TEXT("Version"), "PluginAdmin", "Version");
	//generic_string stabilityStr = pNativeSpeaker->getAttrNameStr(TEXT("Stability"), "PluginAdmin", "Stability");

	_availableList.addColumn(columnInfo(pluginStr, nppParam._dpiManager.scaleX(200)));
	_availableList.addColumn(columnInfo(vesionStr, nppParam._dpiManager.scaleX(100)));
	//_availableList.addColumn(columnInfo(stabilityStr, nppParam._dpiManager.scaleX(70)));
	_availableList.setViewStyleOption(LVS_EX_CHECKBOXES);

	COLORREF fgColor = (NppParameters::getInstance()).getCurrentDefaultFgColor();
	COLORREF bgColor = (NppParameters::getInstance()).getCurrentDefaultBgColor();

	_availableList.initView(_hInst, _hSelf);

	ListView_SetBkColor(_availableList.getViewHwnd(), bgColor);
	ListView_SetTextBkColor(_availableList.getViewHwnd(), bgColor);
	ListView_SetTextColor(_availableList.getViewHwnd(), fgColor);

	_availableList.reSizeView(listRect);
	
	_updateList.addColumn(columnInfo(pluginStr, nppParam._dpiManager.scaleX(200)));
	_updateList.addColumn(columnInfo(vesionStr, nppParam._dpiManager.scaleX(100)));
	//_updateList.addColumn(columnInfo(stabilityStr, nppParam._dpiManager.scaleX(70)));
	_updateList.setViewStyleOption(LVS_EX_CHECKBOXES);

	_updateList.initView(_hInst, _hSelf);

	ListView_SetBkColor(_updateList.getViewHwnd(), bgColor);
	ListView_SetTextBkColor(_updateList.getViewHwnd(), bgColor);
	ListView_SetTextColor(_updateList.getViewHwnd(), fgColor);

	_updateList.reSizeView(listRect);

	_installedList.addColumn(columnInfo(pluginStr, nppParam._dpiManager.scaleX(200)));
	_installedList.addColumn(columnInfo(vesionStr, nppParam._dpiManager.scaleX(100)));
	//_installedList.addColumn(columnInfo(stabilityStr, nppParam._dpiManager.scaleX(70)));
	_installedList.setViewStyleOption(LVS_EX_CHECKBOXES);

	_installedList.initView(_hInst, _hSelf);

	ListView_SetBkColor(_installedList.getViewHwnd(), bgColor);
	ListView_SetTextBkColor(_installedList.getViewHwnd(), bgColor);
	ListView_SetTextColor(_installedList.getViewHwnd(), fgColor);

	_installedList.reSizeView(listRect);

	HWND hDesc = ::GetDlgItem(_hSelf, IDC_PLUGINADM_EDIT);
	::MoveWindow(hDesc, descRect.left, descRect.top, descRect.right, descRect.bottom, TRUE);
	::InvalidateRect(hDesc, nullptr, TRUE);

	switchDialog(0);

	NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
	NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

	goToCenter();
}

void PluginsAdminDlg::collectNppCurrentStatusInfos()
{
	NppParameters& nppParam = NppParameters::getInstance();
	_nppCurrentStatus._nppInstallPath = nppParam.getNppPath();

	_nppCurrentStatus._isAppDataPluginsAllowed = ::SendMessage(_hParent, NPPM_GETAPPDATAPLUGINSALLOWED, 0, 0) == TRUE;
	_nppCurrentStatus._appdataPath = nppParam.getAppDataNppDir();
	generic_string programFilesPath = NppParameters::getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
	_nppCurrentStatus._isInProgramFiles = (_nppCurrentStatus._nppInstallPath.find(programFilesPath) == 0);

}

vector<PluginUpdateInfo*> PluginViewList::fromUiIndexesToPluginInfos(const std::vector<size_t>& uiIndexes) const
{
	std::vector<PluginUpdateInfo*> r;
	size_t nb = _ui.nbItem();

	for (auto i : uiIndexes)
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
	pathAppend(_updaterDir, TEXT("updater"));
	_updaterFullPath = _updaterDir;
	pathAppend(_updaterFullPath, TEXT("gup.exe"));

	// get plugin-list path
	_pluginListFullPath = nppParameters.getPluginConfDir();

#ifdef DEBUG // if not debug, then it's release
	// load from nppPluginList.json instead of nppPluginList.dll
	pathAppend(_pluginListFullPath, TEXT("nppPluginList.json"));
#else //RELEASE
	pathAppend(_pluginListFullPath, TEXT("nppPluginList.dll"));
#endif
}

generic_string PluginsAdminDlg::getPluginListVerStr() const
{
	Version v;
	v.setVersionFrom(_pluginListFullPath);
	return v.toString();
}

bool PluginsAdminDlg::exitToInstallRemovePlugins(Operation op, const vector<PluginUpdateInfo*>& puis)
{
	generic_string opStr;
	if (op == pa_install)
		opStr = TEXT("-unzipTo ");
	else if (op == pa_update)
		opStr = TEXT("-unzipTo -clean ");
	else if (op == pa_remove)
		opStr = TEXT("-clean ");
	else
		return false;

	NppParameters& nppParameters = NppParameters::getInstance();
	generic_string updaterDir = nppParameters.getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");

	generic_string updaterParams = opStr;

	TCHAR nppFullPath[MAX_PATH];
	::GetModuleFileName(NULL, nppFullPath, MAX_PATH);
	updaterParams += TEXT("\"");
	updaterParams += nppFullPath;
	updaterParams += TEXT("\" ");

	updaterParams += TEXT("\"");
	updaterParams += nppParameters.getPluginRootDir();
	updaterParams += TEXT("\"");

	for (auto i : puis)
	{
		if (op == pa_install || op == pa_update)
		{
			// add folder to operate
			updaterParams += TEXT(" \"");
			updaterParams += i->_folderName;
			updaterParams += TEXT(" ");
			updaterParams += i->_repository;
			updaterParams += TEXT(" ");
			updaterParams += i->_id;
			updaterParams += TEXT("\"");
		}
		else // op == pa_remove
		{
			// add folder to operate
			updaterParams += TEXT(" \"");
			generic_string folderName = i->_folderName;
			if (folderName.empty())
			{
				auto lastindex = i->_displayName.find_last_of(TEXT("."));
				if (lastindex != generic_string::npos)
					folderName = i->_displayName.substr(0, lastindex);
				else
					folderName = i->_displayName;	// This case will never occur, but in case if it occurs too
													// just putting the plugin name, so that whole plugin system is not screewed.
			}
			updaterParams += folderName;
			updaterParams += TEXT("\"");
		}
	}

	// Ask user's confirmation
	NativeLangSpeaker *pNativeSpeaker = nppParameters.getNativeLangSpeaker();
	auto res = pNativeSpeaker->messageBox("ExitToUpdatePlugins",
		_hSelf,
		TEXT("If you click YES, you will quit Notepad++ to continue the operations.\nNotepad++ will be restarted after all the operations are terminated.\nContinue?"),
		TEXT("Notepad++ is about to exit"),
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

void PluginsAdminDlg::changeTabName(LIST_TYPE index, const TCHAR *name2change)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = (TCHAR *)name2change;
	TabCtrl_SetItem(_tab.getHSelf(), index, &tie);

	TCHAR label[MAX_PATH];
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
}

void PluginsAdminDlg::changeColumnName(COLUMN_TYPE index, const TCHAR *name2change)
{
	_availableList.changeColumnName(index, name2change);
	_updateList.changeColumnName(index, name2change);
	_installedList.changeColumnName(index, name2change);
}

void PluginViewList::changeColumnName(COLUMN_TYPE index, const TCHAR *name2change)
{
	_ui.setColumnText(index, name2change);
}

bool PluginViewList::removeFromFolderName(const generic_string& folderName)
{

	for (size_t i = 0; i < _ui.nbItem(); ++i)
	{
		PluginUpdateInfo* pi = getPluginInfoFromUiIndex(i);
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

	vector<generic_string> values2Add;
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
std::pair<Version, Version> getIntervalVersions(generic_string intervalVerStr)
{
	std::pair<Version, Version> result;

	if (intervalVerStr.empty())
		return result;

	const size_t indexEnd = intervalVerStr.length() - 1;
	if (intervalVerStr[0] == '[' && intervalVerStr[indexEnd] == ']') // interval versions format
	{
		generic_string cleanIntervalVerStr = intervalVerStr.substr(1, indexEnd - 1);
		vector<generic_string> versionVect;
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
std::pair<std::pair<Version, Version>, std::pair<Version, Version>> getTwoIntervalVersions(generic_string twoIntervalVerStr)
{
	std::pair<std::pair<Version, Version>, std::pair<Version, Version>> r;
	generic_string sep = TEXT("][");
	generic_string::size_type pos = twoIntervalVerStr.find(sep, 0);
	if (pos == string::npos)
		return r;

	generic_string intervalStr1 = twoIntervalVerStr.substr(0, pos + 1);
	generic_string intervalStr2 = twoIntervalVerStr.substr(pos + 1, twoIntervalVerStr.length() - pos + 1);

	r.first = getIntervalVersions(intervalStr1);
	r.second = getIntervalVersions(intervalStr2);

	return r;
}

bool loadFromJson(std::vector<PluginUpdateInfo*>& pl, const json& j)
{
	if (j.empty())
		return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

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
				generic_string newValStr(valStr.begin(), valStr.end());
				pi->_version = Version(newValStr);

				if (i.contains("npp-compatible-versions"))
				{
					json jNppCompatibleVer = i["npp-compatible-versions"];

					string versionsStr = jNppCompatibleVer.get<std::string>();
					generic_string nppCompatibleVersionStr(versionsStr.begin(), versionsStr.end());
					pi->_nppCompatibleVersions = getIntervalVersions(nppCompatibleVersionStr);
				}

				if (i.contains("old-versions-compatibility"))
				{
					json jOldVerCompatibility = i["old-versions-compatibility"];

					string versionsStr = jOldVerCompatibility.get<std::string>();
					generic_string oldVerCompatibilityStr(versionsStr.begin(), versionsStr.end());
					pi->_oldVersionCompatibility = getTwoIntervalVersions(oldVerCompatibilityStr);
				}
			}
			catch (const wstring& s)
			{
				wstring msg = pi->_displayName;
				msg += L": ";
				throw msg + s;
			}
			valStr = i.at("repository").get<std::string>();
			pi->_repository = wmc.char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("homepage").get<std::string>();
			pi->_homepage = wmc.char2wchar(valStr.c_str(), CP_ACP);

			pl.push_back(pi);
		}
#ifdef DEBUG
		catch (const wstring& s)
		{
			::MessageBox(NULL, s.c_str(), TEXT("Exception caught in: PluginsAdmin loadFromJson()"), MB_ICONERROR);
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

PluginUpdateInfo::PluginUpdateInfo(const generic_string& fullFilePath, const generic_string& filename)
{
	if (!::PathFileExists(fullFilePath.c_str()))
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

	if (!::PathFileExists(_pluginListFullPath.c_str()))
	{
		return false;
	}

	if (!::PathFileExists(_updaterFullPath.c_str()))
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
			//printStr(TEXT("LoadLibrary PB!!!"));
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

	
	return loadFromJson(_availableList._list, j);
}

bool PluginsAdminDlg::updateList()
{
	// initialize the primary view with the plugin list loaded from json 
	initAvailablePluginsViewFromList();

	// initialize update list view
	checkUpdates();

	// initialize installed list view
	loadFromPluginInfos();

	return true;
}


bool PluginsAdminDlg::initAvailablePluginsViewFromList()
{
	TCHAR nppFullPathName[MAX_PATH];
	GetModuleFileName(NULL, nppFullPathName, MAX_PATH);

	Version nppVer;
	nppVer.setVersionFrom(nppFullPathName);

	for (const auto& i : _availableList._list)
	{
		bool isCompatible = nppVer.isCompatibleTo(i->_nppCompatibleVersions.first, i->_nppCompatibleVersions.second);

		if (isCompatible)
		{
			vector<generic_string> values2Add;
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
		TCHAR fnNoExt[MAX_PATH];
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

	return true;
}

PluginUpdateInfo* PluginViewList::findPluginInfoFromFolderName(const generic_string& folderName, int& index) const
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

bool PluginViewList::restore(const generic_string& folderName)
{
	for (auto i : _list)
	{
		if (i->_folderName == folderName)
		{
			vector<generic_string> values2Add;
			values2Add.push_back(i->_displayName);
			Version v = i->_version;
			values2Add.push_back(v.toString());
			values2Add.push_back(TEXT("Yes"));
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
	const int maxLen = 256;
	TCHAR txt2search[maxLen];
	::GetDlgItemText(_hSelf, IDC_PLUGINADM_SEARCH_EDIT, txt2search, maxLen);
	if (lstrlen(txt2search) < 2)
		return false;

	long foundIndex = searchInNamesFromCurrentSel(txt2search, isNextMode);
	if (foundIndex == -1)
		foundIndex = searchInDescsFromCurrentSel(txt2search, isNextMode);

	if (foundIndex == -1)
		return false;

	_availableList.setSelection(foundIndex);

	return true;
}

void PluginsAdminDlg::switchDialog(int indexToSwitch)
{
	generic_string desc;
	bool showAvailable, showUpdate, showInstalled;
	switch (indexToSwitch)
	{
		case 0: // available plugins
		{
			showAvailable = true;
			showUpdate = false;
			showInstalled = false;

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

			long infoIndex = _installedList.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_installedList.nbItem()))
				desc = _installedList.getPluginInfoFromUiIndex(infoIndex)->describe();
		}
		break;

		default:
			return;
	}

	_availableList.displayView(showAvailable);
	_updateList.displayView(showUpdate);
	_installedList.displayView(showInstalled);

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
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				HWND hwnd = reinterpret_cast<HWND>(lParam);
				if (hwnd == ::GetDlgItem(_hSelf, IDC_PLUGINADM_EDIT))
				{
					return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
				}
				else
				{
					return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
				}
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

		case WM_COMMAND :
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
				}
			}

			switch (wParam)
			{
				case IDOK:
					if (::GetFocus() == ::GetDlgItem(_hSelf, IDC_PLUGINADM_SEARCH_EDIT))
						::PostMessage(_hSelf, WM_NEXTDLGCTL, 0, 0L);
					return TRUE;

				case IDCANCEL:
					display(false);
					return TRUE;

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
                     pnmh->hwndFrom == _installedList.getViewHwnd())
			{
				PluginViewList* pViewList;
				int buttonID;

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
				else // pnmh->hwndFrom == _installedList.getViewHwnd()
				{
					pViewList = &_installedList;
					buttonID = IDC_PLUGINADM_REMOVE;
				}

				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

				if (pnmh->code == LVN_ITEMCHANGED)
				{
					if (pnmv->uChanged & LVIF_STATE)
					{
						if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2) || // checked
							(pnmv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))   // unchecked
						{
							HWND hButton = ::GetDlgItem(_hSelf, buttonID);
							vector<size_t> checkedArray = pViewList->getCheckedIndexes();
							bool showButton = checkedArray.size() > 0;

							::EnableWindow(hButton, showButton);
						}
						else if (pnmv->uNewState & LVIS_SELECTED)
						{
							PluginUpdateInfo* pui = pViewList->getPluginInfoFromUiIndex(pnmv->iItem);
							generic_string desc = pui->describe();
							::SetDlgItemText(_hSelf, IDC_PLUGINADM_EDIT, desc.c_str());
						}
					}
				}
			}

			return TRUE;
		}

		case WM_DESTROY :
		{
			return TRUE;
		}
	}
	return FALSE;
}
