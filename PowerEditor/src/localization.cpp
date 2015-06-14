// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#include "Notepad_plus.h"
#include "ShortcutMapper.h"
#include "EncodingMapper.h"
#include "localization.h"

using namespace std;



MenuPosition menuPos[] = {
	//==============================================
	//  {L0,  L1,  L2,    id},
	//==============================================
	{ 0, -1, -1, "file" },
	{ 1, -1, -1, "edit" },
	{ 2, -1, -1, "search" },
	{ 3, -1, -1, "view" },
	{ 4, -1, -1, "encoding" },
	{ 5, -1, -1, "language" },
	{ 6, -1, -1, "settings" },
	{ 7, -1, -1, "macro" },
	{ 8, -1, -1, "run" },

	{ 0, 2, -1, "file-openFolder" },
	{ 0, 11, -1, "file-closeMore" },
	{ 0, 20, -1, "file-recentFiles" },

	{ 1, 10, -1, "edit-copyToClipboard" },
	{ 1, 11, -1, "edit-indent" },
	{ 1, 12, -1, "edit-convertCaseTo" },
	{ 1, 13, -1, "edit-lineOperations" },
	{ 1, 14, -1, "edit-comment" },
	{ 1, 15, -1, "edit-autoCompletion" },
	{ 1, 16, -1, "edit-eolConversion" },
	{ 1, 17, -1, "edit-blankOperations" },
	{ 1, 18, -1, "edit-pasteSpecial" },

	{ 2, 18, -1, "search-markAll" },
	{ 2, 19, -1, "search-unmarkAll" },
	{ 2, 20, -1, "search-jumpUp" },
	{ 2, 21, -1, "search-jumpDown" },
	{ 2, 23, -1, "search-bookmark" },

	{ 3, 4, -1, "view-showSymbol" },
	{ 3, 5, -1, "view-zoom" },
	{ 3, 6, -1, "view-moveCloneDocument" },
	{ 3, 7, -1, "view-tab" },
	{ 3, 16, -1, "view-collapseLevel" },
	{ 3, 17, -1, "view-uncollapseLevel" },
	{ 3, 21, -1, "view-project" },

	{ 4, 5, -1, "encoding-characterSets" },
	{ 4, 5, 0, "encoding-arabic" },
	{ 4, 5, 1, "encoding-baltic" },
	{ 4, 5, 2, "encoding-celtic" },
	{ 4, 5, 3, "encoding-cyrillic" },
	{ 4, 5, 4, "encoding-centralEuropean" },
	{ 4, 5, 5, "encoding-chinese" },
	{ 4, 5, 6, "encoding-easternEuropean" },
	{ 4, 5, 7, "encoding-greek" },
	{ 4, 5, 8, "encoding-hebrew" },
	{ 4, 5, 9, "encoding-japanese" },
	{ 4, 5, 10, "encoding-korean" },
	{ 4, 5, 11, "encoding-northEuropean" },
	{ 4, 5, 12, "encoding-thai" },
	{ 4, 5, 13, "encoding-turkish" },
	{ 4, 5, 14, "encoding-westernEuropean" },
	{ 4, 5, 15, "encoding-vietnamese" },

	{ 6, 4, -1, "settings-import" },
	{ -1, -1, -1, "" } // End of array
};

void NativeLangSpeaker::init(TiXmlDocumentA *nativeLangDocRootA, bool loadIfEnglish)
{
	if (nativeLangDocRootA)
	{
		_nativeLangA =  nativeLangDocRootA->FirstChild("NotepadPlus");
		if (_nativeLangA)
		{
			_nativeLangA = _nativeLangA->FirstChild("Native-Langue");
			if (_nativeLangA)
			{
				TiXmlElementA *element = _nativeLangA->ToElement();
				const char *rtl = element->Attribute("RTL");
				if (rtl)
					_isRTL = (strcmp(rtl, "yes") == 0);
                else
                    _isRTL = false;

                // get original file name (defined by Notpad++) from the attribute
                _fileName = element->Attribute("filename");

				if (!loadIfEnglish && _fileName && stricmp("english.xml", _fileName) == 0)
                {
					_nativeLangA = NULL;
					return;
				}
				// get encoding
				TiXmlDeclarationA *declaration =  _nativeLangA->GetDocument()->FirstChild()->ToDeclaration();
				if (declaration)
				{
					const char * encodingStr = declaration->Encoding();
					EncodingMapper *em = EncodingMapper::getInstance();
                    int enc = em->getEncodingFromString(encodingStr);
                    _nativeLangEncoding = (enc != -1)?enc:CP_ACP;
				}
			}	
		}
    }
}

generic_string NativeLangSpeaker::getSpecialMenuEntryName(const char *entryName)
{
	if (!_nativeLangA) return TEXT("");
	TiXmlNodeA *mainMenu = _nativeLangA->FirstChild("Menu");
	if (!mainMenu) return TEXT("");
	mainMenu = mainMenu->FirstChild("Main");
	if (!mainMenu) return TEXT("");
	TiXmlNodeA *entriesRoot = mainMenu->FirstChild("Entries");
	if (!entriesRoot) return TEXT("");

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();

		const char *idName = element->Attribute("idName");
		if (idName)
		{
			const char *name = element->Attribute("name");
			if (!strcmp(idName, entryName))
			{
				return wmc->char2wchar(name, _nativeLangEncoding);
			}
		}
	}
	return TEXT("");
}

generic_string NativeLangSpeaker::getNativeLangMenuString(int itemID) 
{
	if (!_nativeLangA)
		return TEXT("");

	TiXmlNodeA *node = _nativeLangA->FirstChild("Menu");
	if (!node) return TEXT("");

	node = node->FirstChild("Main");
	if (!node) return TEXT("");

	node = node->FirstChild("Commands");
	if (!node) return TEXT("");

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	for (TiXmlNodeA *childNode = node->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		if (element->Attribute("id", &id) && (id == itemID))
		{
			const char *name = element->Attribute("name");
			if (name)
			{
				return wmc->char2wchar(name, _nativeLangEncoding);
			}
		}
	}
	return TEXT("");
}


MenuPosition & getMenuPosition(const char *id)
{

	int nbSubMenuPos = sizeof(menuPos)/sizeof(MenuPosition);

	for(int i = 0; i < nbSubMenuPos; ++i) 
	{
		if (strcmp(menuPos[i]._id, id) == 0)
			return menuPos[i];
	}
	return menuPos[nbSubMenuPos-1];
}

void NativeLangSpeaker::changeMenuLang(HMENU menuHandle, generic_string & pluginsTrans, generic_string & windowTrans)
{
	if (!_nativeLangA) return;
	TiXmlNodeA *mainMenu = _nativeLangA->FirstChild("Menu");
	if (!mainMenu) return;
	mainMenu = mainMenu->FirstChild("Main");
	if (!mainMenu) return;
	TiXmlNodeA *entriesRoot = mainMenu->FirstChild("Entries");
	if (!entriesRoot) return;
	const char *idName = NULL;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		const char *menuIdStr = element->Attribute("menuId");
		if (menuIdStr)
		{
			MenuPosition & menuPos = getMenuPosition(menuIdStr);
			if (menuPos._x != -1)
			{
				const char *name = element->Attribute("name");
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::ModifyMenu(menuHandle, menuPos._x, MF_BYPOSITION, 0, nameW);
			}
		}
		else 
		{
			idName = element->Attribute("idName");
			if (idName)
			{
				const char *name = element->Attribute("name");
				if (!strcmp(idName, "Plugins"))
				{
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					pluginsTrans = nameW;
				}
				else if (!strcmp(idName, "Window"))
				{
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					windowTrans = nameW;
				}
			}
		}
	}

	TiXmlNodeA *menuCommandsRoot = mainMenu->FirstChild("Commands");
	for (TiXmlNodeA *childNode = menuCommandsRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		element->Attribute("id", &id);
		const char *name = element->Attribute("name");

		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(menuHandle, id, MF_BYCOMMAND, id, nameW);
	}

	TiXmlNodeA *subEntriesRoot = mainMenu->FirstChild("SubEntries");

	for (TiXmlNodeA *childNode = subEntriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int x, y, z;
		//const char *xStr = element->Attribute("posX", &x);
		//const char *yStr = element->Attribute("posY", &y);
		const char *subMenuIdStr = element->Attribute("subMenuId");
		const char *name = element->Attribute("name");

		if (!subMenuIdStr || !name)
			continue;

		MenuPosition & menuPos = getMenuPosition(subMenuIdStr);
		x = menuPos._x;
		y = menuPos._y;
		z = menuPos._z;

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

		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(hMenu, pos, MF_BYPOSITION, 0, nameW);
	}
}

int tabContextMenuItemPos[] = {
0, // 0 : Close 
1, // 1 : Close ALL BUT This
4, // 2 : Save
5, // 3 : Save As
9, // 4 : Print
21,// 5 : Move to Other View
22,// 6 : Clone to Other View
17,// 7 : Full File Path to Clipboard
18,// 8 : Filename to Clipboard
19,// 9 : Current Dir. Path to Clipboard
6, // 10: Rename
7, // 11: Move to Recycle Bin
14,// 12: Read-Only
15,// 13: Clear Read-Only Flag
23,// 14: Move to New Instance
24,// 15: Open to New Instance
8, // 16: Reload
2, // 17: Close ALL to the Left
3, // 18: Close ALL to the Right
11,// 19: Open Containing Folder in Explorer
12,// 20: Open Containing Folder in cmd
-1 //-------End
};

void NativeLangSpeaker::changeLangTabContextMenu(HMENU hCM)
{
	if (_nativeLangA)
	{
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu) 
		{
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
			if (tabBarMenu)
			{
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				int nbCMItems = sizeof(tabContextMenuItemPos)/sizeof(int);

				for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
					childNode ;
					childNode = childNode->NextSibling("Item") )
				{
					TiXmlElementA *element = childNode->ToElement();
					int index;
					const char *indexStr = element->Attribute("CMID", &index);
					if (!indexStr || (index < 0 || index >= nbCMItems-1))
						continue;

					int pos = tabContextMenuItemPos[index];
					const char *pName = element->Attribute("name");
					if (pName)
					{
						const wchar_t *pNameW = wmc->char2wchar(pName, _nativeLangEncoding);
						int cmdID = ::GetMenuItemID(hCM, pos);
						::ModifyMenu(hCM, pos, MF_BYPOSITION, cmdID, pNameW);
					}
				}
			}
		}
	}
}

void NativeLangSpeaker::changeLangTabDrapContextMenu(HMENU hCM)
{
	const int POS_GO2VIEW = 0;
	const int POS_CLONE2VIEW = 1;

	if (_nativeLangA)
	{
		const char *goToViewA = NULL;
		const char *cloneToViewA = NULL;
		
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu)
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
		if (tabBarMenu)
		{
			for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int ordre;
				element->Attribute("CMID", &ordre);
				if (ordre == 5)
					goToViewA = element->Attribute("name");
				else if (ordre == 6)
					cloneToViewA = element->Attribute("name");
			}
		}

		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		if (goToViewA && goToViewA[0])
		{
			const wchar_t *goToViewG = wmc->char2wchar(goToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION|MF_STRING, cmdID, goToViewG);
		}
		if (cloneToViewA && cloneToViewA[0])
		{
			const wchar_t *cloneToViewG = wmc->char2wchar(cloneToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION|MF_STRING, cmdID, cloneToViewG);
		}
	}
}

void NativeLangSpeaker::changeConfigLang(HWND hDlg)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *styleConfDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!styleConfDlgNode) return;	
	
	styleConfDlgNode = styleConfDlgNode->FirstChild("StyleConfig");
	if (!styleConfDlgNode) return;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char *titre = (styleConfDlgNode->ToElement())->Attribute("title");

	if ((titre && titre[0]) && hDlg)
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}
	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
			}
		}
	}
	styleConfDlgNode = styleConfDlgNode->FirstChild("SubDialog");
	
	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
			}
		}
	}
}


void NativeLangSpeaker::changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText)
{
	const int iColorStyle = 0;
	const int iUnderline = 8;

	HWND hItem;
	for (int i = iColorStyle ; i < (iUnderline + 1) ; ++i)
	{
		if (translatedText[i] && translatedText[i][0])
		{
			hItem = ::GetDlgItem(hDlg, idArray[i]);
			if (hItem)
			{
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t *nameW = wmc->char2wchar(translatedText[i], _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
			}
		}
	}
}

void NativeLangSpeaker::changeUserDefineLangPopupDlg(HWND hDlg)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *userDefineDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!userDefineDlgNode) return;	
	
	userDefineDlgNode = userDefineDlgNode->FirstChild("UserDefine");
	if (!userDefineDlgNode) return;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	TiXmlNodeA *stylerDialogNode = userDefineDlgNode->FirstChild("StylerDialog");
	if (!stylerDialogNode) return;

	const char *titre = (stylerDialogNode->ToElement())->Attribute("title");
	if (titre &&titre[0])
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}
	for (TiXmlNodeA *childNode = stylerDialogNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);

			}
		}
	}
}

void NativeLangSpeaker::changeUserDefineLang(UserDefineDialog *userDefineDlg)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *userDefineDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!userDefineDlgNode) return;	
	
	userDefineDlgNode = userDefineDlgNode->FirstChild("UserDefine");
	if (!userDefineDlgNode) return;

	HWND hDlg = userDefineDlg->getHSelf();

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char *titre = (userDefineDlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
	}
	// for each control
	const int nbControl = 9;
	const char *translatedText[nbControl];
	for (int i = 0 ; i < nbControl ; ++i)
		translatedText[i] = NULL;

	for (TiXmlNodeA *childNode = userDefineDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		
		if (sentinel && (name && name[0]))
		{
			if (id > 30)
			{
				HWND hItem = ::GetDlgItem(hDlg, id);
				if (hItem)
				{
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					::SetWindowText(hItem, nameW);
				}
			}
			else
			{
				switch(id)
				{
					case 0: case 1: case 2: case 3: case 4:
					case 5: case 6: case 7: case 8: 
 						translatedText[id] = name; break;
				}
			}
		}
	}
	const int nbDlg = 4;
	HWND hDlgArrary[nbDlg];
	hDlgArrary[0] = userDefineDlg->getFolderHandle();
	hDlgArrary[1] = userDefineDlg->getKeywordsHandle();
	hDlgArrary[2] = userDefineDlg->getCommentHandle();
	hDlgArrary[3] = userDefineDlg->getSymbolHandle();

	const char nodeNameArray[nbDlg][16] = {"Folder", "Keywords", "Comment", "Operator"};

	for (int i = 0 ; i < nbDlg ; ++i)
	{
		TiXmlNodeA *node = userDefineDlgNode->FirstChild(nodeNameArray[i]);
		
		if (node) 
		{
			// Set Title
			titre = (node->ToElement())->Attribute("title");
			if (titre &&titre[0])
			{
				const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
				userDefineDlg->setTabName(i, nameW);
			}
			for (TiXmlNodeA *childNode = node->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int id;
				const char *sentinel = element->Attribute("id", &id);
				const char *name = element->Attribute("name");
				if (sentinel && (name && name[0]))
				{
					HWND hItem = ::GetDlgItem(hDlgArrary[i], id);
					if (hItem)
					{
						const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
						::SetWindowText(hItem, nameW);
					}
				}
			}
		}
	}
}

void NativeLangSpeaker::changeFindReplaceDlgLang(FindReplaceDlg & findReplaceDlg)
{
	if (_nativeLangA)
	{
		TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
		if (dlgNode)
		{
			NppParameters *pNppParam = NppParameters::getInstance();
			dlgNode = searchDlgNode(dlgNode, "Find");
			if (dlgNode)
			{
				const char *titre1 = (dlgNode->ToElement())->Attribute("titleFind");
				const char *titre2 = (dlgNode->ToElement())->Attribute("titleReplace");
				const char *titre3 = (dlgNode->ToElement())->Attribute("titleFindInFiles");
				const char *titre4 = (dlgNode->ToElement())->Attribute("titleMark");

				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

				if (titre1 && titre1[0])
				{
					basic_string<wchar_t> nameW = wmc->char2wchar(titre1, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._find = nameW;
					findReplaceDlg.changeTabName(FIND_DLG, pNppParam->getFindDlgTabTitiles()._find.c_str());
				}
				if (titre2  && titre2[0])
				{
					basic_string<wchar_t> nameW = wmc->char2wchar(titre2, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._replace = nameW;
					findReplaceDlg.changeTabName(REPLACE_DLG, pNppParam->getFindDlgTabTitiles()._replace.c_str());
				}
				if (titre3 && titre3[0])
				{
					basic_string<wchar_t> nameW = wmc->char2wchar(titre3, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._findInFiles = nameW;
					findReplaceDlg.changeTabName(FINDINFILES_DLG, pNppParam->getFindDlgTabTitiles()._findInFiles.c_str());
				}
				if (titre4 && titre4[0])
				{
					basic_string<wchar_t> nameW = wmc->char2wchar(titre4, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._mark = nameW;
					findReplaceDlg.changeTabName(MARK_DLG, pNppParam->getFindDlgTabTitiles()._mark.c_str());
				}
			}
		}
	}
	changeDlgLang(findReplaceDlg.getHSelf(), "Find");
}

void NativeLangSpeaker::changePrefereceDlgLang(PreferenceDlg & preference) 
{
	int currentSel = preference.getListSelectedIndex();
	changeDlgLang(preference.getHSelf(), "Preference");

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	char titre[128];
	changeDlgLang(preference._barsDlg.getHSelf(), "Global", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Global"), nameW);
	}
	changeDlgLang(preference._marginsDlg.getHSelf(), "Scintillas", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Scintillas"), nameW);
	}

	changeDlgLang(preference._defaultNewDocDlg.getHSelf(), "NewDoc", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("NewDoc"), nameW);
	}

	changeDlgLang(preference._defaultDirectoryDlg.getHSelf(), "DefaultDir", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("DefaultDir"), nameW);
	}

	changeDlgLang(preference._recentFilesHistoryDlg.getHSelf(), "RecentFilesHistory", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("RecentFilesHistory"), nameW);
	}

	changeDlgLang(preference._fileAssocDlg.getHSelf(), "FileAssoc", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("FileAssoc"), nameW);
	}

	changeDlgLang(preference._langMenuDlg.getHSelf(), "LangMenu", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("LangMenu"), nameW);
	}

	changeDlgLang(preference._tabSettings.getHSelf(), "TabSettings", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("TabSettings"), nameW);
	}

	changeDlgLang(preference._printSettingsDlg.getHSelf(), "Print", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Print"), nameW);
	}
	changeDlgLang(preference._settingsDlg.getHSelf(), "MISC", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("MISC"), nameW);
	}
	changeDlgLang(preference._backupDlg.getHSelf(), "Backup", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Backup"), nameW);
	}

	changeDlgLang(preference._autoCompletionDlg.getHSelf(), "AutoCompletion", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("AutoCompletion"), nameW);
	}

	changeDlgLang(preference._multiInstDlg.getHSelf(), "MultiInstance", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("MultiInstance"), nameW);
	}

	changeDlgLang(preference._delimiterSettingsDlg.getHSelf(), "Delimiter", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Delimiter"), nameW);
	}

	changeDlgLang(preference._settingsOnCloudDlg.getHSelf(), "Cloud", titre);
	if (titre[0] != '\0')
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		preference.renameDialogTitle(TEXT("Cloud"), nameW);
	}

	preference.setListSelection(currentSel);
}

void NativeLangSpeaker::changeShortcutLang()
{
	if (!_nativeLangA) return;

	NppParameters * pNppParam = NppParameters::getInstance();
	vector<CommandShortcut> & mainshortcuts = pNppParam->getUserShortcuts();
	vector<ScintillaKeyMap> & scinshortcuts = pNppParam->getScintillaKeyList();
	int mainSize = (int)mainshortcuts.size();
	int scinSize = (int)scinshortcuts.size();

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Main");
	if (!shortcuts) return;

	TiXmlNodeA *entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index, id;
		if (element->Attribute("index", &index) && element->Attribute("id", &id))
		{
			if (index > -1 && index < mainSize) { //valid index only
				const char *name = element->Attribute("name");
				CommandShortcut & csc = mainshortcuts[index];
				if (csc.getID() == (unsigned long)id) 
				{
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
					csc.setName(nameW);
				}
			}
		}
	}

	//Scintilla
	shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Scintilla");
	if (!shortcuts) return;

	entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < scinSize) { //valid index only
				const char *name = element->Attribute("name");
				ScintillaKeyMap & skm = scinshortcuts[index];

				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				skm.setName(nameW);
			}
		}
	}

}

void NativeLangSpeaker::changeShortcutmapperLang(ShortcutMapper * sm)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Dialog");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("ShortcutMapper");
	if (!shortcuts) return;

	for (TiXmlNodeA *childNode = shortcuts->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < 5)  //valid index only
			{
				const char *name = element->Attribute("name");

				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				sm->translateTab(index, nameW);
			}
		}
	}
}


TiXmlNodeA * NativeLangSpeaker::searchDlgNode(TiXmlNodeA *node, const char *dlgTagName)
{
	TiXmlNodeA *dlgNode = node->FirstChild(dlgTagName);
	if (dlgNode) return dlgNode;
	for (TiXmlNodeA *childNode = node->FirstChildElement();
		childNode ;
		childNode = childNode->NextSibling() )
	{
		dlgNode = searchDlgNode(childNode, dlgTagName);
		if (dlgNode) return dlgNode;
	}
	return NULL;
}

bool NativeLangSpeaker::changeDlgLang(HWND hDlg, const char *dlgTagName, char *title)
{
	if (title)
		title[0] = '\0';

	if (!_nativeLangA) return false;

	TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, dlgTagName);
	if (!dlgNode) return false;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char *titre = (dlgNode->ToElement())->Attribute("title");
	if ((titre && titre[0]) && hDlg)
	{
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);

		if (title)
			strcpy(title, titre);
	}

	// Set the text of child control
	for (TiXmlNodeA *childNode = dlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
			}
		}
	}
	return true;
}

bool NativeLangSpeaker::getMsgBoxLang(const char *msgBoxTagName, generic_string & title, generic_string & message)
{
	title = TEXT("");
	message = TEXT("");

	if (!_nativeLangA) return false;

	TiXmlNodeA *msgBoxNode = _nativeLangA->FirstChild("MessageBox");
	if (!msgBoxNode) return false;

	msgBoxNode = searchDlgNode(msgBoxNode, msgBoxTagName);
	if (!msgBoxNode) return false;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	// Set Title
	const char *titre = (msgBoxNode->ToElement())->Attribute("title");
	const char *msg = (msgBoxNode->ToElement())->Attribute("message");
	if ((titre && titre[0]) && (msg && msg[0]))
	{
		title = wmc->char2wchar(titre, _nativeLangEncoding);
		message = wmc->char2wchar(msg, _nativeLangEncoding);
		return true;
	}
	return false;
}

generic_string NativeLangSpeaker::getProjectPanelLangMenuStr(const char * nodeName, int cmdID, const TCHAR *defaultStr) const
{
	if (!_nativeLangA) return defaultStr;

	TiXmlNodeA *targetNode = _nativeLangA->FirstChild("ProjectManager");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChild("Menus");
	if (!targetNode) return defaultStr;

	targetNode = targetNode->FirstChild(nodeName);
	if (!targetNode) return defaultStr;

	const char *name = NULL;
	for (TiXmlNodeA *childNode = targetNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *idStr = element->Attribute("id", &id);

		if (idStr && id == cmdID)
		{
			name = element->Attribute("name");
			break;
		}
	}

	if (name && name[0])
	{
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		return wmc->char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

generic_string NativeLangSpeaker::getAttrNameStr(const TCHAR *defaultStr, const char *nodeL1Name, const char *nodeL2Name) const
{
	if (!_nativeLangA) return defaultStr;

	TiXmlNodeA *targetNode = _nativeLangA->FirstChild(nodeL1Name);
	if (!targetNode) return defaultStr;
	if (nodeL2Name)
		targetNode = targetNode->FirstChild(nodeL2Name);

	if (!targetNode) return defaultStr;

	const char *name = (targetNode->ToElement())->Attribute("name");
	if (name && name[0])
	{
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		return wmc->char2wchar(name, _nativeLangEncoding);
	}
	return defaultStr;
}

int NativeLangSpeaker::messageBox(const char *msgBoxTagName, HWND hWnd, const TCHAR *defaultMessage, const TCHAR *defaultTitle, int msgBoxType, int intInfo, const TCHAR *strInfo)
{
	generic_string msg, title;
	if (!getMsgBoxLang(msgBoxTagName, title, msg))
	{
		title = defaultTitle;
		msg = defaultMessage;
	}
	title = stringReplace(title, TEXT("$INT_REPLACE$"), std::to_wstring(intInfo));
	msg = stringReplace(msg, TEXT("$INT_REPLACE$"), std::to_wstring(intInfo));
	if (strInfo)
	{
		title = stringReplace(title, TEXT("$STR_REPLACE$"), strInfo);
		msg = stringReplace(msg, TEXT("$STR_REPLACE$"), strInfo);
	}
	return ::MessageBox(hWnd, msg.c_str(), title.c_str(), msgBoxType);
}