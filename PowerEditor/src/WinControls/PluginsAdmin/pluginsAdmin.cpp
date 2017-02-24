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

#include <algorithm>
#include <string>
#include <cctype>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include "pluginsAdmin.h"
#include "ScintillaEditView.h"
#include "localization.h"
#include "PluginsManager.h"
#include "md5.h"

using namespace std;

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
	std::wstring v = std::to_wstring(_major);
	v += TEXT(".");
	v += std::to_wstring(_minor);
	v += TEXT(".");
	v += std::to_wstring(_patch);
	v += TEXT(".");
	v += std::to_wstring(_build);
	return v;
}

generic_string PluginUpdateInfo::describe()
{
	generic_string desc;
	const TCHAR *EOL = TEXT("\r\n");
	if (not description.empty())
	{
		desc = description;
		desc += EOL;
	}

	if (not author.empty())
	{
		desc += TEXT("Author: ");
		desc += author;
		desc += EOL;
	}

	if (not homepage.empty())
	{
		desc += TEXT("Homepage: ");
		desc += homepage;
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
		[](char ch1, char ch2){return std::toupper(ch1) == std::toupper(ch2); }
	);
	return (it != strHaystack.end());
}

LoadedPluginInfo::LoadedPluginInfo(const generic_string & fullFilePath, const generic_string & filename)
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
				searchIn = _availablePluginList[j].name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j].description;

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
				searchIn = _availablePluginList[j].name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j].description;

			if (findStrNoCase(searchIn, str2search))
				return i;
		}

		// from to begining to current position
		for (int i = 0; i < currentIndex + (isNextMode ? 1 : 0); ++i)
		{
			size_t j = _availableListView.getLParamFromIndex(i);
			generic_string searchIn;
			if (inWhichPart == inNames)
				searchIn = _availablePluginList[j].name;
			else //(inWhichPart == inDescs)
				searchIn = _availablePluginList[j].description;

			if (findStrNoCase(searchIn, str2search))
				return i;
		}
	}
	return -1;
}

void PluginsAdminDlg::create(int dialogID, bool isRTL)
{
	// get plugin installation path and launch mode (Admin or normal)
	collectNppCurrentStatusInfos();

	StaticDialog::create(dialogID, isRTL);

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
	//_availableListView.display();
	
	_updateListView.addColumn(columnInfo(pluginStr, nppParam->_dpiManager.scaleX(200)));
	_updateListView.addColumn(columnInfo(vesionStr, nppParam->_dpiManager.scaleX(100)));
	_updateListView.addColumn(columnInfo(stabilityStr, nppParam->_dpiManager.scaleX(70)));
	_updateListView.setStyleOption(LVS_EX_CHECKBOXES);

	_updateListView.init(_hInst, _hSelf);
	_updateListView.reSizeTo(listRect);
	//_updateListView.display(false);

	_installedListView.addColumn(columnInfo(pluginStr, nppParam->_dpiManager.scaleX(200)));
	_installedListView.addColumn(columnInfo(vesionStr, nppParam->_dpiManager.scaleX(100)));
	_installedListView.addColumn(columnInfo(stabilityStr, nppParam->_dpiManager.scaleX(70)));
	_installedListView.setStyleOption(LVS_EX_CHECKBOXES);

	_installedListView.init(_hInst, _hSelf);
	_installedListView.reSizeTo(listRect);
	//_installedListView.display(false);

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
	return true;
}

bool PluginsAdminDlg::updatePlugins()
{
	return true;
}

bool PluginsAdminDlg::removePlugins()
{
	return true;
}

bool PluginsAdminDlg::downloadPluginList()
{
	// check on default location : %APPDATA%\Notepad++\plugins\config\pl\pl.json or NPP_INST_DIR\plugins\config\pl\pl.json


	// if absent then download it


	// check the update ofpl.json


	// download update if present

	// check integrity of pl.json

	// load pl.json

	generic_string pluginListXmlPath(TEXT("c:\\tmp\\pl.xml"));
	_pPluginsXmlDoc = new TiXmlDocument(pluginListXmlPath);
	if (not _pPluginsXmlDoc->LoadFile())
		return false;

	return true;
}

bool PluginsAdminDlg::readFromXml()
{
	TiXmlNode *root = _pPluginsXmlDoc->FirstChild(TEXT("NotepadPlus"));
	if (not root)
		return false;

	_availablePluginList.clear();

	for (TiXmlNode *childNode = root->FirstChildElement(TEXT("plugin"));
		childNode;
		childNode = childNode->NextSibling(TEXT("plugin")))
	{
		PluginUpdateInfo pui;
		const TCHAR *name = (childNode->ToElement())->Attribute(TEXT("name"));
		if (name)
		{
			pui.name = name;
		}
		else
		{
			continue;
		}
			
		const TCHAR *version = (childNode->ToElement())->Attribute(TEXT("version"));
		if (version)
		{
			pui.version = version;
		}
		else
		{
			continue;
		}
		const TCHAR *homepage = (childNode->ToElement())->Attribute(TEXT("homepage"));
		if (homepage)
		{
			pui.homepage = homepage;
		}
		const TCHAR *sourceUrl = (childNode->ToElement())->Attribute(TEXT("sourceUrl"));
		if (sourceUrl)
		{
			pui.sourceUrl = sourceUrl;
		}
		const TCHAR *description = (childNode->ToElement())->Attribute(TEXT("description"));
		if (description)
		{
			pui.description = description;
		}
		const TCHAR *author = (childNode->ToElement())->Attribute(TEXT("author"));
		if (author)
		{
			pui.author = author;
		}
		else
		{
			continue;
		}
		const TCHAR *md5 = (childNode->ToElement())->Attribute(TEXT("md5"));
		if (md5)
		{
			pui.md5 = md5;
		}
		else
		{
			continue;
		}
		const TCHAR *alias = (childNode->ToElement())->Attribute(TEXT("alias"));
		if (alias)
		{
			pui.alias = alias;
		}
		const TCHAR *download = (childNode->ToElement())->Attribute(TEXT("download"));
		if (download)
		{
			pui.download = download;
		}
		else
		{
			continue;
		}

		_availablePluginList.push_back(pui);
	}
	return true;
}

bool PluginsAdminDlg::loadFomList()
{
	if (not _pPluginsXmlDoc)
		return false;

	if (not readFromXml())
		return false;


	size_t i = 0;
	// all - installed = available
	for (auto it = _availablePluginList.begin(); it != _availablePluginList.end(); ++it)
	{
		vector<generic_string> values2Add;

		values2Add.push_back(it->name);
		values2Add.push_back(it->version);
		values2Add.push_back(TEXT("Yes"));

		_availableListView.addLine(values2Add, i++);
	}

	getLoadedPluginInfos();

	return true;
}

bool PluginsAdminDlg::getLoadedPluginInfos()
{
	if (not _pPluginsManager)
		return false;

	for (auto it = _pPluginsManager->_loadedDlls.begin(); it != _pPluginsManager->_loadedDlls.end(); ++it)
	{
		LoadedPluginInfo lpi(it->_fullFilePath, it->_fileName);
		_loadedPluginInfos.push_back(lpi);
	}

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
	bool showAvailable, showUpdate, showInstalled;
	switch (indexToSwitch)
	{
		case 0: // available plugins
			showAvailable = true;
			showUpdate = false;
			showInstalled = false;
			break;

		case 1: // to be updated plugins
			showAvailable = false;
			showUpdate = true;
			showInstalled = false;
			break;

		case 2: // installed plugin
			showAvailable = false;
			showUpdate = false;
			showInstalled = true;
			break;

		default:
			return;
	}

	HWND hInstallButton = ::GetDlgItem(_hSelf, IDC_PLUGINADM_INSTALL);
	HWND hUpdateButton  = ::GetDlgItem(_hSelf, IDC_PLUGINADM_UPDATE);
	HWND hRemoveButton  = ::GetDlgItem(_hSelf, IDC_PLUGINADM_REMOVE);
	
	::ShowWindow(hInstallButton, showAvailable ? SW_SHOW : SW_HIDE);
	::EnableWindow(hInstallButton, showAvailable);

	::ShowWindow(hUpdateButton, showUpdate ? SW_SHOW : SW_HIDE);
	::EnableWindow(hUpdateButton, showUpdate);

	::ShowWindow(hRemoveButton, showInstalled ? SW_SHOW : SW_HIDE);
	::EnableWindow(hRemoveButton, showInstalled);

	_availableListView.display(showAvailable);
	_updateListView.display(showUpdate);
	_installedListView.display(showInstalled);
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
					removePlugins();
					return true;


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
			else if (pnmh->hwndFrom == _availableListView.getHSelf() && pnmh->code == LVN_ITEMCHANGED)
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				if (pnmv->uChanged & LVIF_STATE)
				{
					if (pnmv->uNewState & LVIS_SELECTED)
					{
						size_t infoIndex = _availableListView.getLParamFromIndex(pnmv->iItem);
						generic_string desc = _availablePluginList[infoIndex].describe();
						::SetDlgItemText(_hSelf, IDC_PLUGINADM_EDIT, desc.c_str());
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

