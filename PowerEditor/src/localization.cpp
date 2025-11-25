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


#include "localization.h"

#include <windows.h>

#include <cstring>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>

#include <tinyxml2.h>

#include "Common.h"
#include "Notepad_plus.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "Window.h"
#include "menuCmdID.h"
#include "resource.h"


static constexpr MenuPosition g_menuFolderPositions[] = {
//==============================================
//	{L0,  L1, L2, id},
//==============================================
	{ 0,  -1, -1, "file" },
	{ 1,  -1, -1, "edit" },
	{ 2,  -1, -1, "search" },
	{ 3,  -1, -1, "view" },
	{ 4,  -1, -1, "encoding" },
	{ 5,  -1, -1, "language" },
	{ 6,  -1, -1, "settings" },
	{ 7,  -1, -1, "tools" },
	{ 8,  -1, -1, "macro" },
	{ 9,  -1, -1, "run" },
	{ 10, -1, -1, "Plugins" },
	{ 11, -1, -1, "Window" },

	{ 0,   2, -1, "file-openFolder" },
	{ 0,  13, -1, "file-closeMore" },
	{ 0,  22, -1, "file-recentFiles" },

	{ 1,  11, -1, "edit-insert" },
	{ 1,  12, -1, "edit-copyToClipboard" },
	{ 1,  13, -1, "edit-indent" },
	{ 1,  14, -1, "edit-convertCaseTo" },
	{ 1,  15, -1, "edit-lineOperations" },
	{ 1,  16, -1, "edit-comment" },
	{ 1,  17, -1, "edit-autoCompletion" },
	{ 1,  18, -1, "edit-eolConversion" },
	{ 1,  19, -1, "edit-blankOperations" },
	{ 1,  20, -1, "edit-pasteSpecial" },
	{ 1,  21, -1, "edit-onSelection" },
	{ 1,  23, -1, "edit-multiSelectALL" },
	{ 1,  24, -1, "edit-multiSelectNext" },
	{ 1,  33, -1, "edit-readonlyInNotepad++" },

	{ 2,  18, -1, "search-changeHistory" },
	{ 2,  20, -1, "search-markAll" },
	{ 2,  21, -1, "search-markOne" },
	{ 2,  22, -1, "search-unmarkAll" },
	{ 2,  23, -1, "search-jumpUp" },
	{ 2,  24, -1, "search-jumpDown" },
	{ 2,  25, -1, "search-copyStyledText" },
	{ 2,  27, -1, "search-bookmark" },

	{ 3,   5, -1, "view-currentFileIn" },
	{ 3,   7, -1, "view-showSymbol" },
	{ 3,   8, -1, "view-zoom" },
	{ 3,   9, -1, "view-moveCloneDocument" },
	{ 3,  10, -1, "view-tab" },
	{ 3,  19, -1, "view-collapseLevel" },
	{ 3,  20, -1, "view-uncollapseLevel" },
	{ 3,  24, -1, "view-project" },

	{ 4,   5, -1, "encoding-characterSets" },
	{ 4,   5,  0, "encoding-arabic" },
	{ 4,   5,  1, "encoding-baltic" },
	{ 4,   5,  2, "encoding-celtic" },
	{ 4,   5,  3, "encoding-cyrillic" },
	{ 4,   5,  4, "encoding-centralEuropean" },
	{ 4,   5,  5, "encoding-chinese" },
	{ 4,   5,  6, "encoding-easternEuropean" },
	{ 4,   5,  7, "encoding-greek" },
	{ 4,   5,  8, "encoding-hebrew" },
	{ 4,   5,  9, "encoding-japanese" },
	{ 4,   5, 10, "encoding-korean" },
	{ 4,   5, 11, "encoding-northEuropean" },
	{ 4,   5, 12, "encoding-thai" },
	{ 4,   5, 13, "encoding-turkish" },
	{ 4,   5, 14, "encoding-westernEuropean" },
	{ 4,   5, 15, "encoding-vietnamese" },

	{ 5,  25, -1, "language-userDefinedLanguage" },

	{ 6,   4, -1, "settings-import" },

	{ 7,   0, -1, "tools-md5" },
	{ 7,   1, -1, "tools-sha1" },
	{ 7,   2, -1, "tools-sha256" },
	{ 7,   3, -1, "tools-sha512" },

	{ 11,  0, -1, "window-sortby"},

	{ -1,  -1, -1, "" } // End of array
};

const MenuPosition& getMenuPosition(const char* id)
{
	int nbSubMenuPos = sizeof(g_menuFolderPositions) / sizeof(MenuPosition);

	for (int i = 0; i < nbSubMenuPos; ++i)
	{
		if (strcmp(g_menuFolderPositions[i]._id, id) == 0)
			return g_menuFolderPositions[i];
	}
	return g_menuFolderPositions[nbSubMenuPos - 1];
}

void NativeLangSpeaker::init(tinyxml2::XMLDocument* nativeLangDocRoot, bool loadIfEnglish)
{
	if (nativeLangDocRoot)
	{
		_nativeLang = nativeLangDocRoot->FirstChildElement("NotepadPlus");
		if (_nativeLang)
		{
			_nativeLang = _nativeLang->FirstChildElement("Native-Langue");
			if (_nativeLang)
			{
				tinyxml2::XMLElement* element = _nativeLang->ToElement();
				const char* rtl = element->Attribute("RTL");

				if (rtl)
				{
					_isRTL = (strcmp(rtl, "yes") == 0);

					if (_isRTL)
					{
						const char* editZoneRtl = element->Attribute("editZoneRTL");
						if (editZoneRtl)
							_isEditZoneRTL = !(strcmp(editZoneRtl, "no") == 0);
						else
							_isEditZoneRTL = true;
					}
					else
						_isEditZoneRTL = false;
				}
				else
				{
					_isRTL = false;
					_isEditZoneRTL = false;
				}


				// get original file name (defined by Notpad++) from the attribute
				_fileName = element->Attribute("filename");

				if (!loadIfEnglish && _fileName && _stricmp("english.xml", _fileName) == 0)
				{
					_nativeLang = nullptr;
					return;
				}
			}
		}
	}
}

std::wstring NativeLangSpeaker::getSubMenuEntryName(const char* nodeName) const
{
	if (!_nativeLang) return L"";
	tinyxml2::XMLNode* mainMenu = _nativeLang->FirstChildElement("Menu");
	if (!mainMenu) return L"";
	mainMenu = mainMenu->FirstChildElement("Main");
	if (!mainMenu) return L"";
	tinyxml2::XMLNode* entriesRoot = mainMenu->FirstChildElement("SubEntries");
	if (!entriesRoot) return L"";

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	for (tinyxml2::XMLNode* childNode = entriesRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();

		const char* idName = element->Attribute("subMenuId");
		if (idName)
		{
			const char* name = element->Attribute("name");
			if (!strcmp(idName, nodeName))
			{
				return wmc.char2wchar(name, _nativeLangEncoding);
			}
		}
	}
	return L"";
}

static void purifyMenuString(std::string& s)
{
	// Remove & for CJK localization
	size_t posAndCJK = s.find("(&", 0);
	if (posAndCJK != std::string::npos)
	{
		if (posAndCJK + 3 < s.length())
		{
			if (s[posAndCJK + 3] == ')')
				s.erase(posAndCJK, 4);
		}
	}

	// Remove & and transform && to & for all localizations
	for (int i = static_cast<int>(s.length()) - 1; i >= 0; --i)
	{
		if (s[i] == '&')
		{
			if (i - 1 >= 0 && s[static_cast<size_t>(i) - 1] == '&')
			{
				s.erase(i, 1);
				i -= 1;
			}
			else
			{
				s.erase(i, 1);
			}
		}
	}

	// Remove ellipsis...
	size_t len = s.length();
	if (len <= 3)
		return;
	size_t posEllipsis = len - 3;
	if (s.substr(posEllipsis) == "...")
		s.erase(posEllipsis, 3);

}

std::wstring NativeLangSpeaker::getNativeLangMenuString(int itemID, std::wstring inCaseOfFailureStr, bool removeMarkAnd) const
{
	if (!_nativeLang)
		return inCaseOfFailureStr;

	tinyxml2::XMLNode* node = _nativeLang->FirstChildElement("Menu");
	if (!node) return inCaseOfFailureStr;

	node = node->FirstChildElement("Main");
	if (!node) return inCaseOfFailureStr;

	node = node->FirstChildElement("Commands");
	if (!node) return inCaseOfFailureStr;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	for (tinyxml2::XMLNode* childNode = node->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		if (int id = element->IntAttribute("id", 0); (id == itemID))
		{
			const char* name = element->Attribute("name");
			if (name)
			{
				std::string nameStr = name;

				if (removeMarkAnd)
				{
					purifyMenuString(nameStr);
				}
				return wmc.char2wchar(nameStr.c_str(), _nativeLangEncoding);
			}
		}
	}
	return inCaseOfFailureStr;
}

std::wstring NativeLangSpeaker::getShortcutNameString(int itemID) const
{
	if (!_nativeLang)
		return L"";

	tinyxml2::XMLNode* node = _nativeLang->FirstChildElement("Dialog");
	if (!node) return L"";

	node = node->FirstChildElement("ShortcutMapper");
	if (!node) return L"";

	node = node->FirstChildElement("MainCommandNames");
	if (!node) return L"";

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	for (tinyxml2::XMLNode* childNode = node->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		if (int id = element->IntAttribute("id", 0); (id == itemID))
		{
			const char* name = element->Attribute("name");
			if (name)
			{
				return wmc.char2wchar(name, _nativeLangEncoding);
			}
		}
	}
	return L"";
}

std::wstring NativeLangSpeaker::getLocalizedStrFromID(const char* strID, const std::wstring& defaultString) const
{
	if (!_nativeLang)
		return defaultString;

	if (!strID)
		return defaultString;

	tinyxml2::XMLNode* node = _nativeLang->FirstChildElement("MiscStrings");
	if (!node) return defaultString;

	node = node->FirstChildElement(strID);
	if (!node) return defaultString;

	tinyxml2::XMLElement* element = node->ToElement();

	const char* value = element->Attribute("value");
	if (!value) return defaultString;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	return wmc.char2wchar(value, _nativeLangEncoding);
}


// Get string from map.
// If string not found, get string from menu, then put it into map for the next use.
void NativeLangSpeaker::getMainMenuEntryName(std::wstring& dest, HMENU hMenu, const char* menuId, const wchar_t* defaultDest)
{
	const auto iter = _shortcutMenuEntryNameMap.find(menuId);
	if (iter == _shortcutMenuEntryNameMap.end())
	{
		const MenuPosition& menuPos = getMenuPosition(menuId);
		if (menuPos._x != -1 && menuPos._y == -1 && menuPos._z == -1)
		{
			wchar_t str[MAX_PATH];
			GetMenuString(hMenu, menuPos._x, str, MAX_PATH, MF_BYPOSITION);
			dest = str;
			dest.erase(std::remove(dest.begin(), dest.end(), '&'), dest.end());
			_shortcutMenuEntryNameMap[menuId] = dest;

		}
		else
		{
			if (strcmp(menuId, "about") == 0)
			{
				dest = getShortcutMapperLangStr("AboutCategory", defaultDest);
			}
			else
			{
				_shortcutMenuEntryNameMap[menuId] = defaultDest;
				dest = defaultDest;
			}
		}
	}
	else
	{
		dest = iter->second;
	}
}


void NativeLangSpeaker::changeMenuLang(HMENU menuHandle)
{
	if (nullptr == _nativeLang)
		return;

	tinyxml2::XMLNode* mainMenu = _nativeLang->FirstChildElement("Menu");
	if (nullptr == mainMenu)
		return;

	mainMenu = mainMenu->FirstChildElement("Main");
	if (nullptr == mainMenu)
		return;

	tinyxml2::XMLNode* entriesRoot = mainMenu->FirstChildElement("Entries");
	if (nullptr == entriesRoot)
		return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	for (tinyxml2::XMLNode* childNode = entriesRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		const char* menuIdStr = element->Attribute("menuId");
		if (menuIdStr)
		{
			const MenuPosition& menuPos = getMenuPosition(menuIdStr);
			if (menuPos._x != -1)
			{
				const char* name = element->Attribute("name");
				const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
				::ModifyMenu(menuHandle, menuPos._x, MF_BYPOSITION, 0, nameW);
			}
		}
	}

	tinyxml2::XMLNode* menuCommandsRoot = mainMenu->FirstChildElement("Commands");
	for (tinyxml2::XMLNode* childNode = menuCommandsRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = element->IntAttribute("id", 0);
		const char* name = element->Attribute("name");

		const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(menuHandle, id, MF_BYCOMMAND, id, nameW);
	}

	tinyxml2::XMLNode* subEntriesRoot = mainMenu->FirstChildElement("SubEntries");

	for (tinyxml2::XMLNode* childNode = subEntriesRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		const char* subMenuIdStr = element->Attribute("subMenuId");
		const char* name = element->Attribute("name");

		if (nullptr == subMenuIdStr || nullptr == name)
			continue;

		const MenuPosition& menuPos = getMenuPosition(subMenuIdStr);
		int x = menuPos._x;
		int y = menuPos._y;
		int z = menuPos._z;

		HMENU hSubMenu = ::GetSubMenu(menuHandle, x);
		if (!hSubMenu)
			continue;

		HMENU hSubMenu2 = ::GetSubMenu(hSubMenu, y);
		if (!hSubMenu2)
			continue;

		HMENU hMenu = hSubMenu;
		int pos = y;

		//const char *zStr = element->Attribute("posZ", &z);
		if (z != -1)
		{
			HMENU hSubMenu3 = ::GetSubMenu(hSubMenu2, z);
			if (!hSubMenu3)
				continue;
			hMenu = hSubMenu2;
			pos = z;
		}

		const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(hMenu, pos, MF_BYPOSITION, 0, nameW);
	}
}


static constexpr int tabCmSubMenuEntryPos[] =
{
//   +-------------- The submenu entry item position on the top level of tab context menu
//   |
//   |       +------- Index order (CMDID: Context Menu submenu entry ID): in <TabBar> of english.xml - the number and the order of this array should be synchronized with <TabBar>
//   |       |
//   |       |
//   |       |
	 1,   // 0  Close Multiple Tabs
	 5,   // 1  Open into
	14,   // 2  Copy to Clipboard
	15,   // 3  Move Document
	16,   // 4  Apply Color to Tab
};


void NativeLangSpeaker::changeLangTabContextMenu(HMENU hCM) const
{
	if (_nativeLang != nullptr)
	{
		tinyxml2::XMLNode* tabBarMenu = _nativeLang->FirstChildElement("Menu");
		if (tabBarMenu)
		{
			tabBarMenu = tabBarMenu->FirstChildElement("TabBar");
			if (tabBarMenu)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				int nbSubEntry = sizeof(tabCmSubMenuEntryPos) / sizeof(int);

				for (tinyxml2::XMLNode* childNode = tabBarMenu->FirstChildElement("Item");
					childNode;
					childNode = childNode->NextSiblingElement("Item"))
				{
					tinyxml2::XMLElement* element = childNode->ToElement();
					int cmd = -1;
					if (element->QueryIntAttribute("CMDID", &cmd) != tinyxml2::XML_SUCCESS || cmd < 0)
						continue;

					const char* pName = element->Attribute("name");
					const wchar_t* pNameW = wmc.char2wchar(pName, _nativeLangEncoding);

					if (cmd > nbSubEntry) // menu item CMD
					{
						::ModifyMenu(hCM, cmd, MF_BYCOMMAND, cmd, pNameW);

						// Here CMDID are default Tab Context Menu commands.
						// User can always add any command beyond the default commands in tabContextMenu.xml file.
						// But such command won't be translated.
					}
					else // sub-menu entry id.
					{
						// The customized sub-menu entry cannot be translated.
						// User can use his/her native language as value of attribute "FolderName" in tabContextMenu.xml file.
						if (!NppParameters::getInstance().hasCustomTabContextMenu())
						{
							int subEntryIndex = cmd;
							int subEntrypos = tabCmSubMenuEntryPos[subEntryIndex];
							::ModifyMenu(hCM, subEntrypos, MF_BYPOSITION, cmd, pNameW);
						}
					}
				}
			}
		}
	}
}

void NativeLangSpeaker::getAlternativeNameFromTabContextMenu(std::wstring& output, int cmdID, bool isAlternative, const std::wstring& defaultValue) const
{
	if (_nativeLang != nullptr)
	{
		tinyxml2::XMLNode* tabBarMenu = _nativeLang->FirstChildElement("Menu");
		if (tabBarMenu)
		{
			tabBarMenu = tabBarMenu->FirstChildElement("TabBar");
			if (tabBarMenu)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

				for (tinyxml2::XMLNode* childNode = tabBarMenu->FirstChildElement("Item");
					childNode;
					childNode = childNode->NextSiblingElement("Item"))
				{
					tinyxml2::XMLElement* element = childNode->ToElement();
					int cmd = -1;
					if (element->QueryIntAttribute("CMDID", &cmd) == tinyxml2::XML_SUCCESS && cmd == cmdID)
					{
						const char* pName = element->Attribute(isAlternative ? "alternativeName" : "name");
						if (pName)
						{
							output = wmc.char2wchar(pName, _nativeLangEncoding);
							return;
						}
					}
				}
			}
		}
	}
	output = defaultValue;
}

void NativeLangSpeaker::changeLangTabDropContextMenu(HMENU hCM)
{
	const int POS_GO2VIEW = 0;
	const int POS_CLONE2VIEW = 1;

	if (_nativeLang)
	{
		const char* goToViewA = nullptr;
		const char* cloneToViewA = nullptr;

		tinyxml2::XMLNode* tabBarMenu = _nativeLang->FirstChildElement("Menu");
		if (tabBarMenu)
			tabBarMenu = tabBarMenu->FirstChildElement("TabBar");

		if (tabBarMenu)
		{
			for (tinyxml2::XMLNode* childNode = tabBarMenu->FirstChildElement("Item");
				childNode;
				childNode = childNode->NextSiblingElement("Item"))
			{
				tinyxml2::XMLElement* element = childNode->ToElement();
				int cmd = -1;
				if (element->QueryIntAttribute("CMDID", &cmd) == tinyxml2::XML_SUCCESS)
				{
					if (cmd == IDM_VIEW_GOTO_ANOTHER_VIEW)
						goToViewA = element->Attribute("name");
					else if (cmd == IDM_VIEW_CLONE_TO_ANOTHER_VIEW)
						cloneToViewA = element->Attribute("name");
				}
			}
		}

		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		if (goToViewA && goToViewA[0])
		{
			const wchar_t* goToViewG = wmc.char2wchar(goToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION | MF_STRING, cmdID, goToViewG);
		}

		if (cloneToViewA && cloneToViewA[0])
		{
			const wchar_t* cloneToViewG = wmc.char2wchar(cloneToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION | MF_STRING, cmdID, cloneToViewG);
		}
	}
}

void NativeLangSpeaker::changeLangTrayIconContexMenu(HMENU hCM)
{
	if (!_nativeLang) return;

	tinyxml2::XMLNode* tryIconMenu = _nativeLang->FirstChildElement("Menu");
	if (!tryIconMenu) return;

	tryIconMenu = tryIconMenu->FirstChildElement("TrayIcon");
	if (!tryIconMenu) return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	for (tinyxml2::XMLNode* childNode = tryIconMenu->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
			::ModifyMenu(hCM, id, MF_BYCOMMAND, id, nameW);
		}
	}
}

void NativeLangSpeaker::changeConfigLang(HWND hDlg)
{
	if (nullptr == _nativeLang)
		return;

	tinyxml2::XMLNode* styleConfDlgNode = _nativeLang->FirstChildElement("Dialog");
	if (!styleConfDlgNode)
		return;

	styleConfDlgNode = styleConfDlgNode->FirstChildElement("StyleConfig");
	if (!styleConfDlgNode) return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char* titre = (styleConfDlgNode->ToElement())->Attribute("title");

	if ((titre && titre[0]) && hDlg)
	{
		const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}
	for (tinyxml2::XMLNode* childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
				resizeCheckboxRadioBtn(hItem);
			}
		}
	}
	styleConfDlgNode = styleConfDlgNode->FirstChildElement("SubDialog");

	for (tinyxml2::XMLNode* childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
				resizeCheckboxRadioBtn(hItem);
			}
		}
	}
}


void NativeLangSpeaker::changeStyleCtrlsLang(HWND hDlg, int* idArray, const char** translatedText) const
{
	const int iColorStyle = 0;
	const int iUnderline = 8;

	HWND hItem;
	for (int i = iColorStyle; i < (iUnderline + 1); ++i)
	{
		if (translatedText[i] && translatedText[i][0])
		{
			hItem = ::GetDlgItem(hDlg, idArray[i]);
			if (hItem)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				const wchar_t* nameW = wmc.char2wchar(translatedText[i], _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
			}
		}
	}
}

void NativeLangSpeaker::changeUserDefineLangPopupDlg(HWND hDlg)
{
	if (!_nativeLang) return;

	tinyxml2::XMLNode* userDefineDlgNode = _nativeLang->FirstChildElement("Dialog");
	if (!userDefineDlgNode) return;

	userDefineDlgNode = userDefineDlgNode->FirstChildElement("UserDefine");
	if (!userDefineDlgNode) return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	tinyxml2::XMLNode* stylerDialogNode = userDefineDlgNode->FirstChildElement("StylerDialog");
	if (!stylerDialogNode) return;

	const char* titre = (stylerDialogNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
		const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}
	for (tinyxml2::XMLNode* childNode = stylerDialogNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
				resizeCheckboxRadioBtn(hItem);
			}
		}
	}
}

void NativeLangSpeaker::changeUserDefineLang(UserDefineDialog* userDefineDlg)
{
	if (!_nativeLang) return;

	tinyxml2::XMLNode* userDefineDlgNode = _nativeLang->FirstChildElement("Dialog");
	if (!userDefineDlgNode) return;

	userDefineDlgNode = userDefineDlgNode->FirstChildElement("UserDefine");
	if (!userDefineDlgNode) return;

	HWND hDlg = userDefineDlg->getHSelf();

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char* titre = (userDefineDlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
		const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}

	for (tinyxml2::XMLNode* childNode = userDefineDlgNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			if (id > 30)
			{
				HWND hItem = ::GetDlgItem(hDlg, id);
				if (hItem)
				{
					if (id == IDC_DOCK_BUTTON && userDefineDlg->isDocked())
					{
						std::wstring undockStr = getAttrNameByIdStr(L"Undock", userDefineDlgNode, std::to_string(IDC_UNDOCK_BUTTON).c_str());
						::SetWindowText(hItem, undockStr.c_str());
					}
					else
					{
						const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
						::SetWindowText(hItem, nameW);
						resizeCheckboxRadioBtn(hItem);
					}
				}
			}
		}
	}
	const int nbDlg = 4;
	HWND hDlgArrary[nbDlg]{};
	hDlgArrary[0] = userDefineDlg->getFolderHandle();
	hDlgArrary[1] = userDefineDlg->getKeywordsHandle();
	hDlgArrary[2] = userDefineDlg->getCommentHandle();
	hDlgArrary[3] = userDefineDlg->getSymbolHandle();

	const char nodeNameArray[nbDlg][16] = { "Folder", "Keywords", "Comment", "Operator" };

	for (int i = 0; i < nbDlg; ++i)
	{
		tinyxml2::XMLNode* node = userDefineDlgNode->FirstChildElement(nodeNameArray[i]);

		if (node)
		{
			// Set Title
			titre = (node->ToElement())->Attribute("title");
			if (titre && titre[0])
			{
				const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
				userDefineDlg->setTabName(i, nameW);
			}
			for (tinyxml2::XMLNode* childNode = node->FirstChildElement("Item");
				childNode;
				childNode = childNode->NextSiblingElement("Item"))
			{
				tinyxml2::XMLElement* element = childNode->ToElement();
				int id = -1;
				const char* name = element->Attribute("name");
				if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
				{
					HWND hItem = ::GetDlgItem(hDlgArrary[i], id);
					if (hItem)
					{
						const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
						::SetWindowText(hItem, nameW);
						resizeCheckboxRadioBtn(hItem);
					}
				}
			}
		}
	}

	userDefineDlg->redraw();
}

void NativeLangSpeaker::changeFindReplaceDlgLang(FindReplaceDlg& findReplaceDlg)
{
	if (_nativeLang)
	{
		tinyxml2::XMLNode* dlgNode = _nativeLang->FirstChildElement("Dialog");
		if (dlgNode)
		{
			NppParameters& nppParam = NppParameters::getInstance();
			dlgNode = searchDlgNode(dlgNode, "Find");
			if (dlgNode)
			{
				const char* titre1 = (dlgNode->ToElement())->Attribute("titleFind");
				const char* titre2 = (dlgNode->ToElement())->Attribute("titleReplace");
				const char* titre3 = (dlgNode->ToElement())->Attribute("titleFindInFiles");
				const char* titre4 = (dlgNode->ToElement())->Attribute("titleFindInProjects");
				const char* titre5 = (dlgNode->ToElement())->Attribute("titleMark");

				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

				if (titre1 && titre1[0])
				{
					std::wstring nameW = wmc.char2wchar(titre1, _nativeLangEncoding);
					nppParam.getFindDlgTabTitles()._find = nameW;
					findReplaceDlg.changeTabName(FIND_DLG, nppParam.getFindDlgTabTitles()._find.c_str());
				}
				if (titre2 && titre2[0])
				{
					std::wstring nameW = wmc.char2wchar(titre2, _nativeLangEncoding);
					nppParam.getFindDlgTabTitles()._replace = nameW;
					findReplaceDlg.changeTabName(REPLACE_DLG, nppParam.getFindDlgTabTitles()._replace.c_str());
				}
				if (titre3 && titre3[0])
				{
					std::wstring nameW = wmc.char2wchar(titre3, _nativeLangEncoding);
					nppParam.getFindDlgTabTitles()._findInFiles = nameW;
					findReplaceDlg.changeTabName(FINDINFILES_DLG, nppParam.getFindDlgTabTitles()._findInFiles.c_str());
				}
				if (titre4 && titre4[0])
				{
					std::wstring nameW = wmc.char2wchar(titre4, _nativeLangEncoding);
					nppParam.getFindDlgTabTitles()._findInProjects = nameW;
					findReplaceDlg.changeTabName(FINDINPROJECTS_DLG, nppParam.getFindDlgTabTitles()._findInProjects.c_str());
				}
				if (titre5 && titre5[0])
				{
					std::wstring nameW = wmc.char2wchar(titre5, _nativeLangEncoding);
					nppParam.getFindDlgTabTitles()._mark = nameW;
					findReplaceDlg.changeTabName(MARK_DLG, nppParam.getFindDlgTabTitles()._mark.c_str());
				}
			}
		}
	}
	changeDlgLang(findReplaceDlg.getHSelf(), "Find");
}

void NativeLangSpeaker::changePluginsAdminDlgLang(PluginsAdminDlg& pluginsAdminDlg)
{
	if (_nativeLang)
	{
		tinyxml2::XMLNode* dlgNode = _nativeLang->FirstChildElement("Dialog");
		if (dlgNode)
		{
			dlgNode = searchDlgNode(dlgNode, "PluginsAdminDlg");
			if (dlgNode)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

				tinyxml2::XMLNode* ColumnPluginNode = dlgNode->FirstChildElement("ColumnPlugin");
				if (ColumnPluginNode)
				{
					const char* name = (ColumnPluginNode->ToElement())->Attribute("name");
					if (name && name[0])
					{
						std::wstring nameW = wmc.char2wchar(name, _nativeLangEncoding);
						pluginsAdminDlg.changeColumnName(COLUMN_PLUGIN, nameW.c_str());
					}
				}

				tinyxml2::XMLNode* ColumnVersionNode = dlgNode->FirstChildElement("ColumnVersion");
				if (ColumnVersionNode)
				{
					const char* name = (ColumnVersionNode->ToElement())->Attribute("name");
					if (name && name[0])
					{
						std::wstring nameW = wmc.char2wchar(name, _nativeLangEncoding);
						pluginsAdminDlg.changeColumnName(COLUMN_VERSION, nameW.c_str());
					}
				}

				const char* titre1 = (dlgNode->ToElement())->Attribute("titleAvailable");
				const char* titre2 = (dlgNode->ToElement())->Attribute("titleUpdates");
				const char* titre3 = (dlgNode->ToElement())->Attribute("titleInstalled");
				const char* titre4 = (dlgNode->ToElement())->Attribute("titleIncompatible");

				if (titre1 && titre1[0])
				{
					std::wstring nameW = wmc.char2wchar(titre1, _nativeLangEncoding);
					pluginsAdminDlg.changeTabName(AVAILABLE_LIST, nameW.c_str());
				}
				if (titre2 && titre2[0])
				{
					std::wstring nameW = wmc.char2wchar(titre2, _nativeLangEncoding);
					pluginsAdminDlg.changeTabName(UPDATES_LIST, nameW.c_str());
				}
				if (titre3 && titre3[0])
				{
					std::wstring nameW = wmc.char2wchar(titre3, _nativeLangEncoding);
					pluginsAdminDlg.changeTabName(INSTALLED_LIST, nameW.c_str());
				}
				if (titre4 && titre4[0])
				{
					std::wstring nameW = wmc.char2wchar(titre4, _nativeLangEncoding);
					pluginsAdminDlg.changeTabName(INCOMPATIBLE_LIST, nameW.c_str());
				}
			}

			changeDlgLang(pluginsAdminDlg.getHSelf(), "PluginsAdminDlg");
		}
	}
}

void NativeLangSpeaker::changePreferenceDlgLang(PreferenceDlg & preference) 
{
	auto currentSel = preference.getListSelectedIndex();
	changeDlgLang(preference.getHSelf(), "Preference");

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const size_t titreMaxSize = 128;
	char titre[titreMaxSize];
	changeDlgLang(preference._generalSubDlg.getHSelf(), "Global", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Global", nameW);
	}

	changeDlgLang(preference._toolbarSubDlg.getHSelf(), "Toolbar", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Toolbar", nameW);
	}

	changeDlgLang(preference._tabbarSubDlg.getHSelf(), "Tabbar", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Tabbar", nameW);
	}

	changeDlgLang(preference._editingSubDlg.getHSelf(), "Scintillas", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Scintillas", nameW);
	}

	changeDlgLang(preference._editing2SubDlg.getHSelf(), "Scintillas2", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Scintillas2", nameW);
	}

	changeDlgLang(preference._darkModeSubDlg.getHSelf(), "DarkMode", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"DarkMode", nameW);
	}

	changeDlgLang(preference._marginsBorderEdgeSubDlg.getHSelf(), "MarginsBorderEdge", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"MarginsBorderEdge", nameW);
	}

	changeDlgLang(preference._newDocumentSubDlg.getHSelf(), "NewDoc", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"NewDoc", nameW);
	}

	changeDlgLang(preference._defaultDirectorySubDlg.getHSelf(), "DefaultDir", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"DefaultDir", nameW);
	}

	changeDlgLang(preference._recentFilesHistorySubDlg.getHSelf(), "RecentFilesHistory", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"RecentFilesHistory", nameW);
	}

	changeDlgLang(preference._fileAssocDlg.getHSelf(), "FileAssoc", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"FileAssoc", nameW);
	}

	changeDlgLang(preference._languageSubDlg.getHSelf(), "Language", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Language", nameW);
	}

	changeDlgLang(preference._indentationSubDlg.getHSelf(), "Indentation", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Indentation", nameW);
	}

	changeDlgLang(preference._highlightingSubDlg.getHSelf(), "Highlighting", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Highlighting", nameW);
	}

	changeDlgLang(preference._printSubDlg.getHSelf(), "Print", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Print", nameW);
	}

	changeDlgLang(preference._searchingSubDlg.getHSelf(), "Searching", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t* nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Searching", nameW);
	}

	changeDlgLang(preference._miscSubDlg.getHSelf(), "MISC", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"MISC", nameW);
	}
	changeDlgLang(preference._backupSubDlg.getHSelf(), "Backup", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Backup", nameW);
	}

	changeDlgLang(preference._autoCompletionSubDlg.getHSelf(), "AutoCompletion", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"AutoCompletion", nameW);
	}

	changeDlgLang(preference._multiInstanceSubDlg.getHSelf(), "MultiInstance", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"MultiInstance", nameW);
	}

	changeDlgLang(preference._delimiterSubDlg.getHSelf(), "Delimiter", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Delimiter", nameW);
	}

	changeDlgLang(preference._performanceSubDlg.getHSelf(), "Performance", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Performance", nameW);
	}

	changeDlgLang(preference._cloudAndLinkSubDlg.getHSelf(), "Cloud", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"Cloud", nameW);
	}

	changeDlgLang(preference._searchEngineSubDlg.getHSelf(), "SearchEngine", titre, titreMaxSize);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc.char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(L"SearchEngine", nameW);
	}

	preference._darkModeSubDlg.destroyResetMenu();

	preference.setListSelection(currentSel);
}

void NativeLangSpeaker::changeShortcutLang()
{
	if (!_nativeLang) return;

	NppParameters& nppParam = NppParameters::getInstance();
	std::vector<CommandShortcut>& mainshortcuts = nppParam.getUserShortcuts();
	std::vector<ScintillaKeyMap>& scinshortcuts = nppParam.getScintillaKeyList();

	tinyxml2::XMLNode* shortcuts = _nativeLang->FirstChildElement("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChildElement("Main");
	if (!shortcuts) return;

	tinyxml2::XMLNode* entriesRoot = shortcuts->FirstChildElement("Entries");
	if (!entriesRoot) return;

	for (tinyxml2::XMLNode* childNode = entriesRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int index = -1;
		int id = -1;
		if (element->QueryIntAttribute("index", &index) == tinyxml2::XML_SUCCESS && element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS)
		{
			if (index > -1 && static_cast<size_t>(index) < mainshortcuts.size()) //valid index only
			{
				const char* name = element->Attribute("name");
				CommandShortcut& csc = mainshortcuts[index];
				if (csc.getID() == (unsigned long)id)
				{
					csc.setName(name);
				}
			}
		}
	}

	//Scintilla
	shortcuts = _nativeLang->FirstChildElement("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChildElement("Scintilla");
	if (!shortcuts) return;

	entriesRoot = shortcuts->FirstChildElement("Entries");
	if (!entriesRoot) return;

	for (tinyxml2::XMLNode* childNode = entriesRoot->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int index = -1;
		if (element->QueryIntAttribute("index", &index) == tinyxml2::XML_SUCCESS)
		{
			if (index > -1 && static_cast<size_t>(index) < scinshortcuts.size()) //valid index only
			{
				const char* name = element->Attribute("name");
				ScintillaKeyMap& skm = scinshortcuts[index];
				skm.setName(name);
			}
		}
	}

}

std::wstring NativeLangSpeaker::getShortcutMapperLangStr(const char* nodeName, const wchar_t* defaultStr) const
{
	if (!_nativeLang) return defaultStr;

	tinyxml2::XMLNode* targetNode = _nativeLang->FirstChildElement("Dialog");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChildElement("ShortcutMapper");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChildElement(nodeName);
	if (!targetNode) return defaultStr;

	const char* name = (targetNode->ToElement())->Attribute("name");
	if (name && name[0])
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(name, _nativeLangEncoding);
	}

	return defaultStr;
}


tinyxml2::XMLNode* NativeLangSpeaker::searchDlgNode(tinyxml2::XMLNode* node, const char* dlgTagName)
{
	tinyxml2::XMLNode* dlgNode = node->FirstChildElement(dlgTagName);
	if (dlgNode) return dlgNode;
	for (tinyxml2::XMLNode* childNode = node->FirstChildElement();
		childNode;
		childNode = childNode->NextSibling())
	{
		dlgNode = searchDlgNode(childNode, dlgTagName);
		if (dlgNode) return dlgNode;
	}
	return nullptr;
}

bool NativeLangSpeaker::getDoSaveOrNotStrings(std::wstring& title, std::wstring& msg)
{
	if (!_nativeLang) return false;

	tinyxml2::XMLNode* dlgNode = _nativeLang->FirstChildElement("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, "DoSaveOrNot");
	if (!dlgNode) return false;

	const char* title2set = (dlgNode->ToElement())->Attribute("title");
	if (!title2set || !title2set[0]) return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const wchar_t* titleW = wmc.char2wchar(title2set, _nativeLangEncoding);
	title = titleW;

	for (tinyxml2::XMLNode* childNode = dlgNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			if (id == IDC_DOSAVEORNOTTEXT)
			{
				const wchar_t* msgW = wmc.char2wchar(name, _nativeLangEncoding);
				msg = msgW;

				return true;
			}
		}
	}
	return false;
}

bool NativeLangSpeaker::changeDlgLang(HWND hDlg, const char* dlgTagName, char* title, size_t titleMaxSize)
{
	if (title)
		title[0] = '\0';

	if (!_nativeLang) return false;

	tinyxml2::XMLNode* dlgNode = _nativeLang->FirstChildElement("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, dlgTagName);
	if (!dlgNode) return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char* title2set = (dlgNode->ToElement())->Attribute("title");
	if ((title2set && title2set[0]) && hDlg)
	{
		const wchar_t* nameW = wmc.char2wchar(title2set, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);

		if (title && titleMaxSize)
			strncpy(title, title2set, titleMaxSize - 1);
	}

	// Set the text of child control
	for (tinyxml2::XMLNode* childNode = dlgNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		const char* name = element->Attribute("name");
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
				resizeCheckboxRadioBtn(hItem);
			}
		}
	}

	// Set the text of child control
	for (tinyxml2::XMLNode* childNode = dlgNode->FirstChildElement("ComboBox");
		childNode;
		childNode = childNode->NextSiblingElement("ComboBox"))
	{
		std::vector<std::wstring> comboElms;
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		element->QueryIntAttribute("id", &id);
		HWND hCombo = ::GetDlgItem(hDlg, id);
		if (!hCombo) return false;

		for (tinyxml2::XMLNode* gChildNode = childNode->FirstChildElement("Element");
			gChildNode;
			gChildNode = gChildNode->NextSiblingElement("Element"))
		{
			tinyxml2::XMLElement* comBoelement = gChildNode->ToElement();
			const char* name = comBoelement->Attribute("name");
			const wchar_t* nameW = wmc.char2wchar(name, _nativeLangEncoding);
			comboElms.push_back(nameW);
		}

		size_t count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);
		if (count == comboElms.size())
		{
			// get selected index
			auto selIndex = ::SendMessage(hCombo, CB_GETCURSEL, 0, 0);

			// remove all old items
			::SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

			// add translated entries
			for (const auto& i : comboElms)
			{
				::SendMessage(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(i.c_str()));
			}

			// restore selected index
			::SendMessage(hCombo, CB_SETCURSEL, selIndex, 0);
		}
	}
	return true;
}

bool NativeLangSpeaker::getMsgBoxLang(const char* msgBoxTagName, std::wstring& title, std::wstring& message)
{
	title = L"";
	message = L"";

	if (!_nativeLang) return false;

	tinyxml2::XMLNode* msgBoxNode = _nativeLang->FirstChildElement("MessageBox");
	if (!msgBoxNode) return false;

	msgBoxNode = searchDlgNode(msgBoxNode, msgBoxTagName);
	if (!msgBoxNode) return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char* titre = (msgBoxNode->ToElement())->Attribute("title");
	const char* msg = (msgBoxNode->ToElement())->Attribute("message");
	if ((titre && titre[0]) && (msg && msg[0]))
	{
		title = wmc.char2wchar(titre, _nativeLangEncoding);
		message = wmc.char2wchar(msg, _nativeLangEncoding);
		return true;
	}
	return false;
}

std::wstring NativeLangSpeaker::getDlgLangMenuStr(const char* firstLevelNodeName, const char* secondLevelNodeName, int cmdID, const wchar_t* defaultStr) const
{
	if (!_nativeLang) return defaultStr;

	tinyxml2::XMLNode* targetNode = _nativeLang->FirstChildElement(firstLevelNodeName);
	if (!targetNode) return defaultStr;

	if (secondLevelNodeName && secondLevelNodeName[0])
	{
		targetNode = targetNode->FirstChildElement(secondLevelNodeName);
		if (!targetNode) return defaultStr;
	}

	targetNode = targetNode->FirstChildElement("Menu");
	if (!targetNode) return defaultStr;

	const char* name = nullptr;
	for (tinyxml2::XMLNode* childNode = targetNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && id == cmdID)
		{
			name = element->Attribute("name");
			break;
		}
	}

	if (name && name[0])
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

std::wstring NativeLangSpeaker::getCmdLangStr(std::vector<const char*> nodeNames, int cmdID, const wchar_t* defaultStr) const
{
	if (!_nativeLang) return defaultStr;
	tinyxml2::XMLNode* targetNode = _nativeLang->FirstChildElement(nodeNames.at(0));
	if (targetNode == nullptr)
		return defaultStr;

	auto it = nodeNames.begin();
	++it;

	for (auto end = nodeNames.end(); it != end; ++it)
	{
		targetNode = targetNode->FirstChildElement(*it);
		if (targetNode == nullptr)
			return defaultStr;
	}

	if (targetNode == nullptr)
		return defaultStr;

	const char* name = nullptr;
	for (tinyxml2::XMLNode* childNode = targetNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && id == cmdID)
		{
			name = element->Attribute("name");
			break;
		}
	}

	if (name && name[0])
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

std::wstring NativeLangSpeaker::getProjectPanelLangMenuStr(const char* nodeName, int cmdID, const wchar_t* defaultStr) const
{
	if (!_nativeLang) return defaultStr;

	tinyxml2::XMLNode* targetNode = _nativeLang->FirstChildElement("ProjectManager");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChildElement("Menus");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChildElement(nodeName);
	if (!targetNode) return defaultStr;

	const char* name = nullptr;
	for (tinyxml2::XMLNode* childNode = targetNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		int id = -1;
		if (element->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS && id == cmdID)
		{
			name = element->Attribute("name");
			break;
		}
	}

	if (name && name[0])
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

std::wstring NativeLangSpeaker::getAttrNameStr(const wchar_t* defaultStr, const char* nodeL1Name, const char* nodeL2Name, const char* nodeL3Name) const
{
	if (!_nativeLang) return defaultStr;

	tinyxml2::XMLNode* targetNode = _nativeLang->FirstChildElement(nodeL1Name);
	if (!targetNode) return defaultStr;
	if (nodeL2Name)
		targetNode = targetNode->FirstChildElement(nodeL2Name);

	if (!targetNode) return defaultStr;

	const char* name = (targetNode->ToElement())->Attribute(nodeL3Name);
	if (name && name[0])
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

std::wstring NativeLangSpeaker::getAttrNameByIdStr(const wchar_t* defaultStr, tinyxml2::XMLNode* targetNode, const char* nodeL1Value, const char* nodeL1Name, const char* nodeL2Name) const
{
	if (!targetNode) return defaultStr;

	for (tinyxml2::XMLNode* childNode = targetNode->FirstChildElement("Item");
		childNode;
		childNode = childNode->NextSiblingElement("Item"))
	{
		tinyxml2::XMLElement* element = childNode->ToElement();
		const char* id = element->Attribute(nodeL1Name);
		if (id && id[0] && !strcmp(id, nodeL1Value))
		{
			const char* name = element->Attribute(nodeL2Name);
			if (name && name[0])
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				return wmc.char2wchar(name, _nativeLangEncoding);
			}
		}
	}
	return defaultStr;
}

int NativeLangSpeaker::messageBox(const char* msgBoxTagName, HWND hWnd, const wchar_t* defaultMessage, const wchar_t* defaultTitle, int msgBoxType, int intInfo, const wchar_t* strInfo)
{
	if ((NppParameters::getInstance()).isEndSessionCritical())
		return IDCANCEL; // simulate Esc-key or Cancel-button as there should not be any big delay / code-flow block

	std::wstring msg;
	std::wstring title;
	if (!getMsgBoxLang(msgBoxTagName, title, msg))
	{
		title = defaultTitle;
		msg = defaultMessage;
	}
	title = stringReplace(title, L"$INT_REPLACE$", std::to_wstring(intInfo));
	msg = stringReplace(msg, L"$INT_REPLACE$", std::to_wstring(intInfo));
	if (strInfo)
	{
		title = stringReplace(title, L"$STR_REPLACE$", strInfo);
		msg = stringReplace(msg, L"$STR_REPLACE$", strInfo);
	}
	if (_isRTL)
	{
		msgBoxType |= MB_RTLREADING | MB_RIGHT;
	}
	return ::MessageBox(hWnd, msg.c_str(), (title.empty() || wcscmp(title.c_str(), L"0") == 0) ? nullptr : title.c_str(), msgBoxType);
}

// Default English localization during Notepad++ launch
// is handled in NppDarkMode::subclassButtonControl.
void NativeLangSpeaker::resizeCheckboxRadioBtn(HWND hWnd)
{
	constexpr size_t classNameLen = 32;
	wchar_t className[classNameLen]{};
	::GetClassNameW(hWnd, className, classNameLen);
	if (wcscmp(className, WC_BUTTON) == 0)
	{
		const auto nBtnStyle = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
		switch (nBtnStyle & BS_TYPEMASK)
		{
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
			{
				if ((nBtnStyle & BS_MULTILINE) != BS_MULTILINE)
				{
					::SendMessageW(hWnd, NppDarkMode::WM_SETBUTTONIDEALSIZE, 0, 0);
				}
				break;
			}

			default:
				break;
		}
	}
}
