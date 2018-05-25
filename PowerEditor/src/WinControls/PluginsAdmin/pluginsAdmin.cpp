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

#include "json.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include "pluginsAdmin.h"
#include "ScintillaEditView.h"
#include "localization.h"
#include "Processus.h"
#include "PluginsManager.h"
#include "md5.h"

using namespace std;
using nlohmann::json;

void Version::setVersionFrom(generic_string filePath)
{
	if (not filePath.empty() && ::PathFileExists(filePath.c_str()))
	{
		DWORD handle;
		DWORD bufferSize = ::GetFileVersionInfoSize(filePath.c_str(), &handle);

		if (bufferSize <= 0)
			return;

		unsigned char* buffer = new unsigned char[bufferSize];
		::GetFileVersionInfo(filePath.c_str(), handle, bufferSize, buffer);

		VS_FIXEDFILEINFO* lpFileInfo;
		UINT cbFileInfo = 0;
		VerQueryValue(buffer, TEXT("\\"), (LPVOID*)&lpFileInfo, &cbFileInfo);
		if (cbFileInfo)
		{
			_major = (lpFileInfo->dwFileVersionMS & 0xFFFF0000) >> 16;
			_minor = lpFileInfo->dwFileVersionMS & 0x0000FFFF;
			_patch = (lpFileInfo->dwFileVersionLS & 0xFFFF0000) >> 16;
			_build = lpFileInfo->dwFileVersionLS & 0x0000FFFF;
		}
	}
}

generic_string Version::toString()
{
	if (_build == 0 && _patch == 0 && _minor == 0 && _major == 0) // ""
	{
		return TEXT("");
	}	
	else if (_build == 0 && _patch == 0 && _minor == 0) // "major"
	{
		return std::to_wstring(_major);
	}
	else if (_build == 0 && _patch == 0) // "major.minor"
	{
		std::wstring v = std::to_wstring(_major);
		v += TEXT(".");
		v += std::to_wstring(_minor);
		return v;
	}
	else if (_build == 0) // "major.minor.patch"
	{
		std::wstring v = std::to_wstring(_major);
		v += TEXT(".");
		v += std::to_wstring(_minor);
		v += TEXT(".");
		v += std::to_wstring(_patch);
		return v;
	}

	// "major.minor.patch.build"
	std::wstring ver = std::to_wstring(_major);
	ver += TEXT(".");
	ver += std::to_wstring(_minor);
	ver += TEXT(".");
	ver += std::to_wstring(_patch);
	ver += TEXT(".");
	ver += std::to_wstring(_build);

	return ver;
}

generic_string PluginUpdateInfo::describe()
{
	generic_string desc;
	const TCHAR *EOL = TEXT("\r\n");
	if (not _description.empty())
	{
		desc = _description;
		desc += EOL;
	}

	if (not _author.empty())
	{
		desc += TEXT("Author: ");
		desc += _author;
		desc += EOL;
	}

	if (not _homepage.empty())
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


long PluginsAdminDlg::searchFromCurrentSel(generic_string str2search, bool inWhichPart, bool isNextMode) const
{
	// search from curent selected item or from the beginning
	long currentIndex = _availableListView.getSelectedIndex();
	int nbItem = static_cast<int>(_availableListView.nbItem());
	if (currentIndex == -1)
	{
		// no selection, let's search from 0 to the end
		for (int i = 0; i < nbItem; ++i)
		{
			size_t j = _availableListView.getLParamFromIndex(i);
			generic_string searchIn;
			if (inWhichPart == inNames)
				searchIn = _availablePluginList[j]._name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j]._description;

			if (findStrNoCase(searchIn, str2search))
				return i;
		}
	}
	else
	{
		// with selection, let's search from currentIndex

		// from current position to the end
		for (int i = currentIndex + (isNextMode ? 1 : 0); i < nbItem; ++i)
		{
			size_t j = _availableListView.getLParamFromIndex(i);
			generic_string searchIn;
			if (inWhichPart == inNames)
				searchIn = _availablePluginList[j]._name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j]._description;

			if (findStrNoCase(searchIn, str2search))
				return i;
		}

		// from to begining to current position
		for (int i = 0; i < currentIndex + (isNextMode ? 1 : 0); ++i)
		{
			size_t j = _availableListView.getLParamFromIndex(i);
			generic_string searchIn;
			if (inWhichPart == inNames)
				searchIn = _availablePluginList[j]._name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j]._description;

			if (findStrNoCase(searchIn, str2search))
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
	int tabDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(13);
	_tab.setFont(TEXT("Tahoma"), tabDpiDynamicalHeight);

	const TCHAR *available = TEXT("Available");
	const TCHAR *updates = TEXT("Updates");
	const TCHAR *installed = TEXT("Installed");

	_tab.insertAtEnd(available);
	_tab.insertAtEnd(updates);
	_tab.insertAtEnd(installed);

	rect.bottom -= 100;;
	_tab.reSizeTo(rect);
	_tab.display();

	const long marge = 10;

	const int topMarge = 42;

	HWND hResearchLabel = ::GetDlgItem(_hSelf, IDC_PLUGINADM_RESEARCH_STATIC);
	RECT researchLabelRect;
	::GetClientRect(hResearchLabel, &researchLabelRect);
	researchLabelRect.left = rect.left;
	researchLabelRect.top = topMarge + 2;
	::MoveWindow(hResearchLabel, researchLabelRect.left, researchLabelRect.top, researchLabelRect.right, researchLabelRect.bottom, TRUE);
	::InvalidateRect(hResearchLabel, nullptr, TRUE);

	HWND hResearchEdit = ::GetDlgItem(_hSelf, IDC_PLUGINADM_RESEARCH_EDIT);
	RECT researchEditRect;
	::GetClientRect(hResearchEdit, &researchEditRect);
	researchEditRect.left = researchLabelRect.right + marge;
	researchEditRect.top = topMarge;
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

	long actionZoneHeight = 50;
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

	NppParameters *nppParam = NppParameters::getInstance();
	NativeLangSpeaker *pNativeSpeaker = nppParam->getNativeLangSpeaker();
	generic_string pluginStr = pNativeSpeaker->getAttrNameStr(TEXT("Plugin"), "PluginAdmin", "Plugin");
	generic_string vesionStr = pNativeSpeaker->getAttrNameStr(TEXT("Version"), "PluginAdmin", "Version");
	generic_string stabilityStr = pNativeSpeaker->getAttrNameStr(TEXT("Stability"), "PluginAdmin", "Stability");

	_availableListView.addColumn(columnInfo(pluginStr, nppParam->_dpiManager.scaleX(200)));
	_availableListView.addColumn(columnInfo(vesionStr, nppParam->_dpiManager.scaleX(100)));
	_availableListView.addColumn(columnInfo(stabilityStr, nppParam->_dpiManager.scaleX(70)));
	_availableListView.setStyleOption(LVS_EX_CHECKBOXES);

	_availableListView.init(_hInst, _hSelf);
	_availableListView.reSizeTo(listRect);
	
	_updateListView.addColumn(columnInfo(pluginStr, nppParam->_dpiManager.scaleX(200)));
	_updateListView.addColumn(columnInfo(vesionStr, nppParam->_dpiManager.scaleX(100)));
	_updateListView.addColumn(columnInfo(stabilityStr, nppParam->_dpiManager.scaleX(70)));
	_updateListView.setStyleOption(LVS_EX_CHECKBOXES);

	_updateListView.init(_hInst, _hSelf);
	_updateListView.reSizeTo(listRect);

	_installedListView.addColumn(columnInfo(pluginStr, nppParam->_dpiManager.scaleX(200)));
	_installedListView.addColumn(columnInfo(vesionStr, nppParam->_dpiManager.scaleX(100)));
	_installedListView.addColumn(columnInfo(stabilityStr, nppParam->_dpiManager.scaleX(70)));
	_installedListView.setStyleOption(LVS_EX_CHECKBOXES);

	_installedListView.init(_hInst, _hSelf);
	_installedListView.reSizeTo(listRect);

	HWND hDesc = ::GetDlgItem(_hSelf, IDC_PLUGINADM_EDIT);
	::MoveWindow(hDesc, descRect.left, descRect.top, descRect.right, descRect.bottom, TRUE);
	::InvalidateRect(hDesc, nullptr, TRUE);

	switchDialog(0);

	ETDTProc enableDlgTheme = (ETDTProc)::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
	if (enableDlgTheme)
		enableDlgTheme(_hSelf, ETDT_ENABLETAB);

	goToCenter();
}

void PluginsAdminDlg::collectNppCurrentStatusInfos()
{
	NppParameters *pNppParam = NppParameters::getInstance();
	_nppCurrentStatus._nppInstallPath = pNppParam->getNppPath();

	_nppCurrentStatus._isAppDataPluginsAllowed = ::SendMessage(_hParent, NPPM_GETAPPDATAPLUGINSALLOWED, 0, 0) == TRUE;
	_nppCurrentStatus._appdataPath = pNppParam->getAppDataNppDir();
	generic_string programFilesPath = NppParameters::getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
	_nppCurrentStatus._isInProgramFiles = (_nppCurrentStatus._nppInstallPath.find(programFilesPath) == 0);

}

bool PluginsAdminDlg::installPlugins()
{
	vector<size_t> indexes = _availableListView.getCheckedIndexes();

	NppParameters *pNppParameters = NppParameters::getInstance();
	generic_string updaterDir = pNppParameters->getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");
	generic_string updaterParams = TEXT("-unzipTo -clean ");

	for (auto i : indexes)
	{
		//printStr(_availablePluginList[i]._name .c_str());

		// add folder to operate
		generic_string destFolder = pNppParameters->getAppDataNppDir();
		PathAppend(destFolder, _availablePluginList[i]._name);
		
		updaterParams += destFolder;

		// add zipFile's url
		updaterParams += TEXT(" ");
		updaterParams += _availablePluginList[i]._repository;

		Process updater(updaterFullPath.c_str(), updaterParams.c_str(), updaterDir.c_str());
		updater.run();
	}
	return true;
}

bool PluginsAdminDlg::updatePlugins()
{
	vector<size_t> indexes = _updateListView.getCheckedIndexes();

	NppParameters *pNppParameters = NppParameters::getInstance();
	generic_string updaterDir = pNppParameters->getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");
	generic_string updaterParams = TEXT("-unzipTo -clean ");

	for (auto i : indexes)
	{
		// add folder to operate
		generic_string destFolder = pNppParameters->getAppDataNppDir();
		PathAppend(destFolder, _availablePluginList[i]._name);

		updaterParams += destFolder;

		// add zipFile's url
		updaterParams += TEXT(" ");
		updaterParams += _availablePluginList[i]._repository;

		Process updater(updaterFullPath.c_str(), updaterParams.c_str(), updaterDir.c_str());
		updater.run();
	}
	return true;
}

bool PluginsAdminDlg::removePlugins()
{
	vector<size_t> indexes = _installedListView.getCheckedIndexes();

	NppParameters *pNppParameters = NppParameters::getInstance();
	generic_string updaterDir = pNppParameters->getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");
	generic_string updaterParams = TEXT("-clean ");

	for (auto i : indexes)
	{
		//printStr(_installedPluginList[i]._fullFilePath.c_str());

		// add folder to operate
		generic_string destFolder = pNppParameters->getAppDataNppDir();
		PathAppend(destFolder, _availablePluginList[i]._name);

		updaterParams += destFolder;

		Process updater(updaterFullPath.c_str(), updaterParams.c_str(), updaterDir.c_str());
		updater.run();
	}
	return true;
}


bool loadFromJson(vector<PluginUpdateInfo> & pl, const json& j)
{
	if (j.empty())
		return false;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	json jArray = j["npp-plugins"];
	if (jArray.empty() || jArray.type() != json::value_t::array)
		return false;
	
	for (const auto& i : jArray)
	{
		try {
			PluginUpdateInfo pi;

			string valStr = i.at("folder-name").get<std::string>();
			pi._name = wmc->char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("display-name").get<std::string>();
			pi._alias = wmc->char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("author").get<std::string>();
			pi._author = wmc->char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("description").get<std::string>();
			pi._description = wmc->char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("repository").get<std::string>();
			pi._repository = wmc->char2wchar(valStr.c_str(), CP_ACP);

			valStr = i.at("homepage").get<std::string>();
			pi._homepage = wmc->char2wchar(valStr.c_str(), CP_ACP);


			pl.push_back(pi);
		}
		catch (...) // Every field is mandatory. If one of property is missing, an exception is thrown then this plugin will be ignored
		{
			continue; 
		}
	}
	return true;
}



bool PluginsAdminDlg::updateListAndLoadFromJson()
{
	// check on default location : %APPDATA%\Notepad++\plugins\config\pl\pl.json or NPP_INST_DIR\plugins\config\pl\pl.json


	// if absent then download it


	// check the update of pl.json


	// download update if present

	// check integrity of pl.json

	// load pl.json
	// 
	generic_string nppPluginListJsonPath = TEXT("C:\\tmp\\nppPluginList.json");

	if (!::PathFileExists(nppPluginListJsonPath.c_str()))
		return false;

	ifstream nppPluginListJson(nppPluginListJsonPath);
	json pluginsJson;
	nppPluginListJson >> pluginsJson;

	// initialize available list view
	loadFromJson(_availablePluginList, pluginsJson);
	updateAvailableListView();

	// initialize update list view
	checkUpdates();
	updateUpdateListView();

	// initialize installed list view
	loadFromPluginInfos();
	updateInstalledListView();

	return true;
}

void PluginsAdminDlg::updateAvailableListView()
{
	size_t i = 0;
	//
	for (const auto& pui : _availablePluginList)
	{
		vector<generic_string> values2Add;
		values2Add.push_back(pui._name);
		Version v = pui._version;
		values2Add.push_back(v.toString());
		values2Add.push_back(TEXT("Yes"));
		_availableListView.addLine(values2Add, i++);
	}
}

void PluginsAdminDlg::updateInstalledListView()
{
	size_t i = 0;
	//
	for (const auto& lpi : _installedPluginList)
	{
		vector<generic_string> values2Add;
		values2Add.push_back(lpi._name);
		Version v = lpi._version;
		values2Add.push_back(v.toString());
		values2Add.push_back(TEXT("Yes"));
		_installedListView.addLine(values2Add, i++);
	}
}

void PluginsAdminDlg::updateUpdateListView()
{
	size_t i = 0;
	//
	for (const auto& pui : _updatePluginList)
	{
		vector<generic_string> values2Add;
		values2Add.push_back(pui._name);
		Version v = pui._version;
		values2Add.push_back(v.toString());
		values2Add.push_back(TEXT("Yes"));
		_updateListView.addLine(values2Add, i++);
	}
}

PluginUpdateInfo::PluginUpdateInfo(const generic_string& fullFilePath, const generic_string& filename)
{
	if (not::PathFileExists(fullFilePath.c_str()))
		return;

	_fullFilePath = fullFilePath;
	_name = filename;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	const char *path = wmc->wchar2char(fullFilePath.c_str(), CP_ACP);
	MD5 md5;
	_id = wmc->char2wchar(md5.digestFile(path), CP_ACP);

	_version.setVersionFrom(fullFilePath);

}

bool PluginsAdminDlg::loadFromPluginInfos()
{
	if (!_pPluginsManager)
		return false;

	for (const auto& i : _pPluginsManager->_loadedDlls)
	{
		PluginUpdateInfo pui(i._fullFilePath, i._fileName);
		_installedPluginList.push_back(pui);
	}

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
	::GetDlgItemText(_hSelf, IDC_PLUGINADM_RESEARCH_EDIT, txt2search, maxLen);
	if (lstrlen(txt2search) < 2)
		return false;

	long foundIndex = searchInNamesFromCurrentSel(txt2search, isNextMode);
	if (foundIndex == -1)
		foundIndex = searchInDescsFromCurrentSel(txt2search, isNextMode);

	if (foundIndex == -1)
		return false;

	_availableListView.setSelection(foundIndex);

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

			long infoIndex = _availableListView.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_availablePluginList.size()))
				desc = _availablePluginList.at(infoIndex).describe();
		}
		break;

		case 1: // to be updated plugins
		{
			showAvailable = false;
			showUpdate = true;
			showInstalled = false;
			
			long infoIndex = _updateListView.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_updatePluginList.size()))
				desc = _updatePluginList.at(infoIndex).describe();
		}
		break;

		case 2: // installed plugin
		{
			showAvailable = false;
			showUpdate = false;
			showInstalled = true;

			long infoIndex = _installedListView.getSelectedIndex();
			if (infoIndex != -1 && infoIndex < static_cast<long>(_installedPluginList.size()))
				desc = _installedPluginList.at(infoIndex).describe();
		}
		break;

		default:
			return;
	}

	_availableListView.display(showAvailable);
	_updateListView.display(showUpdate);
	_installedListView.display(showInstalled);

	::SetDlgItemText(_hSelf, IDC_PLUGINADM_EDIT, desc.c_str());

	HWND hInstallButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_INSTALL);
	HWND hUpdateButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_UPDATE);
	HWND hRemoveButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_REMOVE);

	::ShowWindow(hInstallButton, showAvailable ? SW_SHOW : SW_HIDE);
	if (showAvailable)
	{
		vector<size_t> checkedArray = _availableListView.getCheckedIndexes();
		showAvailable = checkedArray.size() > 0;
	}
	::EnableWindow(hInstallButton, showAvailable);

	::ShowWindow(hUpdateButton, showUpdate ? SW_SHOW : SW_HIDE);
	if (showUpdate)
	{
		vector<size_t> checkedArray = _updateListView.getCheckedIndexes();
		showUpdate = checkedArray.size() > 0;
	}
	::EnableWindow(hUpdateButton, showUpdate);

	::ShowWindow(hRemoveButton, showInstalled ? SW_SHOW : SW_HIDE);
	if (showInstalled)
	{
		vector<size_t> checkedArray = _installedListView.getCheckedIndexes();
		showInstalled = checkedArray.size() > 0;
	}
	::EnableWindow(hRemoveButton, showInstalled);
}

INT_PTR CALLBACK PluginsAdminDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_INITDIALOG :
		{

			return TRUE;
		}

		case WM_COMMAND :
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_PLUGINADM_RESEARCH_EDIT:
					{
						searchInPlugins(false);
						return TRUE;
					}
				}
			}

			switch (wParam)
			{
				case IDCANCEL :
				case IDOK :
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
			else if (pnmh->hwndFrom == _availableListView.getHSelf() || 
                     pnmh->hwndFrom == _updateListView.getHSelf() ||
                     pnmh->hwndFrom == _installedListView.getHSelf())
			{
				ListView* pListView;
				vector<PluginUpdateInfo>* pPluginInfos;
				int buttonID;

				if (pnmh->hwndFrom == _availableListView.getHSelf())
				{
					pListView = &_availableListView;
					pPluginInfos = &_availablePluginList;
					buttonID = IDC_PLUGINADM_INSTALL;
				}
				else if (pnmh->hwndFrom == _updateListView.getHSelf())
				{
					pListView = &_updateListView;
					pPluginInfos = &_updatePluginList;
					buttonID = IDC_PLUGINADM_UPDATE;
				}
				else // pnmh->hwndFrom == _installedListView.getHSelf()
				{
					pListView = &_installedListView;
					pPluginInfos = &_installedPluginList;
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
							vector<size_t> checkedArray = pListView->getCheckedIndexes();
							bool showButton = checkedArray.size() > 0;

							::EnableWindow(hButton, showButton);
						}
						else if (pnmv->uNewState & LVIS_SELECTED)
						{
							size_t infoIndex = pListView->getLParamFromIndex(pnmv->iItem);
							generic_string desc = pPluginInfos->at(infoIndex).describe();
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

