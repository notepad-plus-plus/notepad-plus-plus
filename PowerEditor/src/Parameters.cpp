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

#include "Parameters.h"
#include "ScintillaEditView.h"
#include <shlobj.h>

//#include <windows.h>


NppParameters * NppParameters::_pSelf = new NppParameters;

NppParameters::NppParameters() : _pXmlDoc(NULL),_pXmlUserDoc(NULL), _pXmlUserStylerDoc(NULL),\
								_pXmlUserLangDoc(NULL), _pXmlNativeLangDoc(NULL),\
								_nbLang(0), _nbFile(0), _nbMaxFile(10), _pXmlToolIconsDoc(NULL),\
								_pXmlShortcutDoc(NULL), _pXmlContextMenuDoc(NULL), _pXmlSessionDoc(NULL),\
								_nbUserLang(0), _hUser32(NULL), _hUXTheme(NULL),\
								_transparentFuncAddr(NULL), _enableThemeDialogTextureFuncAddr(NULL),\
								_isTaskListRBUTTONUP_Active(false)
{
	_appdataNppDir[0] = '\0';
}

void cutString(const char *str2cut, vector<string> & patternVect)
{
	char str2scan[MAX_PATH];
	strcpy(str2scan, str2cut);
	size_t len = strlen(str2scan);
	bool isProcessing = false;
	char *pBegin = NULL;
	for (size_t i = 0 ; i <= len ; i++)
	{
		switch(str2scan[i])
		{
			case ' ':
			case '\0':
			{
				if (isProcessing)
				{
					str2scan[i] = '\0';
					patternVect.push_back(pBegin);
					isProcessing = false;
				}
				break;
			}

			default :
				if (!isProcessing)
				{
					isProcessing = true;
					pBegin = str2scan+i;
				}
		}
	}
}


bool NppParameters::load(/*bool noUserPath*/)
{
	bool isAllLaoded = true;
	for (int i = 0 ; i < NB_LANG ; _langList[i] = NULL, i++);
	char nppPath[MAX_PATH];
	char userPath[MAX_PATH];
	
	// Prepare for default path
	::GetModuleFileName(NULL, nppPath, sizeof(nppPath));
	
	PathRemoveFileSpec(nppPath);
	strcpy(_nppPath, nppPath);

	// Make localConf.xml path
	char localConfPath[MAX_PATH];
	strcpy(localConfPath, _nppPath);
	PathAppend(localConfPath, localConfFile);

	// Test if localConf.xml exist
	bool isLocal = (PathFileExists(localConfPath) == TRUE);

	if (isLocal)
	{
		strcpy(userPath, _nppPath);
	}
	else
	{
		ITEMIDLIST *pidl;
		SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
		SHGetPathFromIDList(pidl, userPath);

		PathAppend(userPath, "Notepad++");

		strcpy(_appdataNppDir, userPath);

		if (!PathFileExists(userPath))
		{
			::CreateDirectory(userPath, NULL);
		}
	}

	//-------------------------------------//
	// Transparent function for w2k and xp //
	//-------------------------------------//
	_hUser32 = ::GetModuleHandle("User32");
	if (_hUser32)
		_transparentFuncAddr = (WNDPROC)::GetProcAddress(_hUser32, "SetLayeredWindowAttributes");
	
	//---------------------------------------------//
	// Dlg theme texture function for xp and vista //
	//---------------------------------------------//
	_hUXTheme = ::LoadLibrary("uxtheme.dll");
	if (_hUXTheme)
		_enableThemeDialogTextureFuncAddr = (WNDPROC)::GetProcAddress(_hUXTheme, "EnableThemeDialogTexture");

	//---------------------------------------//
	// langs.xml : for every user statically //
	//---------------------------------------//
	char langs_xml_path[MAX_PATH];
	strcpy(langs_xml_path, nppPath);
	
	PathAppend(langs_xml_path, "langs.xml");

	_pXmlDoc = new TiXmlDocument(langs_xml_path);
	bool loadOkay = _pXmlDoc->LoadFile();
	if (!loadOkay)
	{
		::MessageBox(NULL, "Load langs.xml failed!", "Configurator",MB_OK);
		delete _pXmlDoc;
		_pXmlDoc = NULL;
		isAllLaoded = false;
	}
	else
		getLangKeywordsFromXmlTree();

	//---------------------------//
	// config.xml : for per user //
	//---------------------------//
	char configPath[MAX_PATH];
	strcpy(configPath, userPath);
	PathAppend(configPath, "config.xml");

	if (!PathFileExists(configPath))
	{
		char srcConfigPath[MAX_PATH];
		strcpy(srcConfigPath, nppPath);
		PathAppend(srcConfigPath, "config.xml");

		::CopyFile(srcConfigPath, configPath, TRUE);
	}

	_pXmlUserDoc = new TiXmlDocument(configPath);
	loadOkay = _pXmlUserDoc->LoadFile();
	if (!loadOkay)
	{
		::MessageBox(NULL, "Load config.xml failed!", "Configurator",MB_OK);
		delete _pXmlUserDoc;
		_pXmlUserDoc = NULL;
		isAllLaoded = false;
	}
	else
		getUserParametersFromXmlTree();

	//----------------------------//
	// stylers.xml : for per user //
	//----------------------------//
	char stylerPath[MAX_PATH];
	strcpy(stylerPath, userPath);
	PathAppend(stylerPath, "stylers.xml");

	if (!PathFileExists(stylerPath))
	{
		char srcStylersPath[MAX_PATH];
		strcpy(srcStylersPath, nppPath);
		PathAppend(srcStylersPath, "stylers.xml");

		::CopyFile(srcStylersPath, stylerPath, TRUE);
	}

	_pXmlUserStylerDoc = new TiXmlDocument(stylerPath);
	loadOkay = _pXmlUserStylerDoc->LoadFile();
	if (!loadOkay)
	{
		::MessageBox(NULL, "Load stylers.xml failed!", "Configurator",MB_OK);
		delete _pXmlUserStylerDoc;
		_pXmlUserStylerDoc = NULL;
		isAllLaoded = false;
	}
	else
		getUserStylersFromXmlTree();

	//-----------------------------------//
	// userDefineLang.xml : for per user //
	//-----------------------------------//
	strcpy(_userDefineLangPath, userPath);
	PathAppend(_userDefineLangPath, "userDefineLang.xml");

	_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
	loadOkay = _pXmlUserLangDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlUserLangDoc;
		_pXmlUserLangDoc = NULL;
		isAllLaoded = false;
	}
	else
		getUserDefineLangsFromXmlTree();
	
	//----------------------------------------------//
	// nativeLang.xml : for per user                //
	// In case of absence of user's nativeLang.xml, //
	// We'll look in the Notepad++ Dir.             //
	//----------------------------------------------//
	char nativeLangPath[MAX_PATH];
	strcpy(nativeLangPath, userPath);
	PathAppend(nativeLangPath, "nativeLang.xml");

	if (!PathFileExists(nativeLangPath))
	{
		strcpy(nativeLangPath, nppPath);
		PathAppend(nativeLangPath, "nativeLang.xml");
	}

	_pXmlNativeLangDoc = new TiXmlDocument(nativeLangPath);
	loadOkay = _pXmlNativeLangDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlNativeLangDoc;
		_pXmlNativeLangDoc = NULL;
		isAllLaoded = false;
	}	
	
	//---------------------------------//
	// toolbarIcons.xml : for per user //
	//---------------------------------//
	char toolbarIconsPath[MAX_PATH];
	strcpy(toolbarIconsPath, userPath);
	PathAppend(toolbarIconsPath, "toolbarIcons.xml");

	_pXmlToolIconsDoc = new TiXmlDocument(toolbarIconsPath);
	loadOkay = _pXmlToolIconsDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlToolIconsDoc;
		_pXmlToolIconsDoc = NULL;
		isAllLaoded = false;
	}

	//------------------------------//
	// shortcuts.xml : for per user //
	//------------------------------//
	strcpy(_shortcutsPath, userPath);
	PathAppend(_shortcutsPath, "shortcuts.xml");

	if (!PathFileExists(_shortcutsPath))
	{
		char srcShortcutsPath[MAX_PATH];
		strcpy(srcShortcutsPath, nppPath);
		PathAppend(srcShortcutsPath, "shortcuts.xml");

		::CopyFile(srcShortcutsPath, _shortcutsPath, TRUE);
	}

	_pXmlShortcutDoc = new TiXmlDocument(_shortcutsPath);
	loadOkay = _pXmlShortcutDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlShortcutDoc;
		_pXmlShortcutDoc = NULL;
		isAllLaoded = false;
	}
	else
	{
		getShortcutsFromXmlTree();
		getMacrosFromXmlTree();
		getUserCmdsFromXmlTree();
		getPluginCmdsFromXmlTree();

		// fill out _scintillaModifiedKeys : 
		// those user defined Scintilla key will be used remap Scintilla Key Array
		getScintKeysFromXmlTree();

		// initialize entire Scintilla Key Array 
		initScintillaKeys();
	}

	//---------------------------------//
	// contextMenu.xml : for per user //
	//---------------------------------//
	strcpy(_contextMenuPath, userPath);
	PathAppend(_contextMenuPath, "contextMenu.xml");

	if (!PathFileExists(_contextMenuPath))
	{
		char srcContextMenuPath[MAX_PATH];
		strcpy(srcContextMenuPath, nppPath);
		PathAppend(srcContextMenuPath, "contextMenu.xml");

		::CopyFile(srcContextMenuPath, _contextMenuPath, TRUE);
	}

	_pXmlContextMenuDoc = new TiXmlDocument(_contextMenuPath);
	loadOkay = _pXmlContextMenuDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlContextMenuDoc;
		_pXmlContextMenuDoc = NULL;
		isAllLaoded = false;
	}
	else
		getContextMenuFromXmlTree();

	//----------------------------//
	// session.xml : for per user //
	//----------------------------//
	strcpy(_sessionPath, userPath);
	PathAppend(_sessionPath, "session.xml");

	_pXmlSessionDoc = new TiXmlDocument(_sessionPath);
	loadOkay = _pXmlSessionDoc->LoadFile();
	if (!loadOkay)
		isAllLaoded = false;
	else
		getSessionFromXmlTree();

	delete _pXmlSessionDoc;
	_pXmlSessionDoc = NULL;
	return isAllLaoded;
}

void NppParameters::destroyInstance()
{
	if (_pXmlDoc != NULL)
	{
		delete _pXmlDoc;
	}

	if (_pXmlUserDoc != NULL)
	{
		_pXmlUserDoc->SaveFile();
		delete _pXmlUserDoc;
	}
	if (_pXmlUserStylerDoc)
		delete _pXmlUserStylerDoc;

	if (_pXmlUserLangDoc)
		delete _pXmlUserLangDoc;

	if (_pXmlNativeLangDoc)
		delete _pXmlNativeLangDoc;

	if (_pXmlToolIconsDoc)
		delete _pXmlToolIconsDoc;

	if (_pXmlShortcutDoc)
		delete _pXmlShortcutDoc;

	if (_pXmlContextMenuDoc)
		delete _pXmlContextMenuDoc;

	if (_pXmlSessionDoc)
		delete _pXmlSessionDoc;

	delete _pSelf;
}

void NppParameters::setFontList(HWND hWnd)
{
	::AddFontResource(LINEDRAW_FONT); 

	//---------------//
	// Sys font list //
	//---------------//

	LOGFONT lf;
	_fontlist.push_back("");

	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfFaceName[0]='\0';
	HDC hDC = ::GetDC(hWnd);
	::EnumFontFamiliesEx(hDC, 
						&lf, 
						(FONTENUMPROC) EnumFontFamExProc, 
						(LPARAM) &_fontlist, 0);
}

void NppParameters::getLangKeywordsFromXmlTree()
{
	TiXmlNode *root = _pXmlDoc->FirstChild("NotepadPlus");
		if (!root) return;
	feedKeyWordsParameters(root);
}

bool NppParameters::getUserStylersFromXmlTree()
{
	TiXmlNode *root = _pXmlUserStylerDoc->FirstChild("NotepadPlus");
		if (!root) return false;
	return feedStylerArray(root);
}

bool NppParameters::getUserParametersFromXmlTree()
{
	if (!_pXmlUserDoc)
		return false;

	TiXmlNode *root = _pXmlUserDoc->FirstChild("NotepadPlus");
	if (!root) return false;

	// GUI
	feedGUIParameters(root);

	//History
	feedFileListParameters(root);

	// Raser tout
	TiXmlNode *node = root->FirstChildElement("History");
	root->RemoveChild(node);

	// Repartir de zero
	TiXmlElement HistoryNode("History");

	root->InsertEndChild(HistoryNode);
	return true;
}

bool NppParameters::getUserDefineLangsFromXmlTree()
{
	if (!_pXmlUserLangDoc)
		return false;
	
	TiXmlNode *root = _pXmlUserLangDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedUserLang(root);
	return true;
}

bool NppParameters::getShortcutsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;
	
	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedShortcut(root);
	return true;
}

bool NppParameters::getMacrosFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;
	
	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedMacros(root);
	return true;
}

bool NppParameters::getUserCmdsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;
	
	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedUserCmds(root);
	return true;
}


bool NppParameters::getPluginCmdsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;
	
	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedPluginCustomizedCmds(root);
	return true;
}


bool NppParameters::getScintKeysFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;
	
	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;

	feedScintKeys(root);
	return true;
}

void NppParameters::initScintillaKeys()
{
	// Cut/Copy/Paste
	_scintillaKeyCommands.push_back(ScintillaKeyMap("CUT", IDSCINTILLA_KEY_CUT, SCI_CUT, true, false, false, 0x58/*VK_X*/, IDM_EDIT_CUT));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("COPY", IDSCINTILLA_KEY_COPY, SCI_COPY, true, false, false, 0x43/*VK_C*/, IDM_EDIT_COPY));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("PASTE", IDSCINTILLA_KEY_PASTE, SCI_PASTE, true, false, false, 0x56/*VK_V*/, IDM_EDIT_PASTE));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("DEL", IDSCINTILLA_KEY_DEL, SCI_CLEAR, false, false, false, VK_DELETE, IDM_EDIT_DELETE));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("SELECT ALL", IDSCINTILLA_KEY_SELECTALL, SCI_SELECTALL, true, false, false, 0x41/*VK_A*/, IDM_EDIT_SELECTALL));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("OUTDENT", IDSCINTILLA_KEY_OUTDENT, SCI_BACKTAB, false, false, true, VK_TAB, IDM_EDIT_RMV_TAB));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("UNDO", IDSCINTILLA_KEY_UNDO, SCI_UNDO, true, false, false, 0x5A/*VK_Z*/, IDM_EDIT_UNDO));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("REDO", IDSCINTILLA_KEY_REDO, SCI_REDO, true, false, false, 0x59/*VK_Y*/, IDM_EDIT_REDO));

	// Line operation
	_scintillaKeyCommands.push_back(ScintillaKeyMap("DUPLICATE LINE", IDSCINTILLA_KEY_LINE_DUP, SCI_LINEDUPLICATE, true, false, false, 0x44/*VK_D*/, IDM_EDIT_DUP_LINE));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("CUT LINE", IDSCINTILLA_KEY_LINE_CUT, SCI_LINECUT, true, false, false, 0x4C/*VK_L*/));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("DELETE LINE", IDSCINTILLA_KEY_LINE_DEL, SCI_LINEDELETE, true, false, true, 0x4C/*VK_L*/));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("TRANSPOSE LINE", IDSCINTILLA_KEY_LINE_TRANS, SCI_LINETRANSPOSE, true, false, false, 0x54/*VK_T*/));
	_scintillaKeyCommands.push_back(ScintillaKeyMap("COPY LINE", IDSCINTILLA_KEY_LINE_COPY, SCI_LINECOPY, true, false, true, 0x54/*VK_T*/));
	//SCI_DELETEBACK
	//SCI_DELETEBACKNOTLINE
	
	//SCI_DELWORDLEFT
	//SCI_DELWORDRIGHT
	//SCI_DELLINELEFT
	//SCI_DELLINERIGHT
}

bool NppParameters::getContextMenuFromXmlTree()
{
	if (!_pXmlContextMenuDoc)
		return false;
	
	TiXmlNode *root = _pXmlContextMenuDoc->FirstChild("NotepadPlus");
	if (!root) 
		return false;
	
	TiXmlNode *contextMenuRoot = root->FirstChildElement("ScintillaContextMenu");
	if (contextMenuRoot)
	{
		for (TiXmlNode *childNode = contextMenuRoot->FirstChildElement("Item");
			childNode ;
			childNode = childNode->NextSibling("Item") )
		{
			int id;
			const char *idStr = (childNode->ToElement())->Attribute("id", &id);
			if (idStr)
			{
				_contextMenuItems.push_back(MenuItemUnit(id, ""));
			}
		}
	}

	return true;
}

bool NppParameters::loadSession(Session & session, const char *sessionFileName)
{
	TiXmlDocument *pXmlSessionDocument = new TiXmlDocument(sessionFileName);
	bool loadOkay = pXmlSessionDocument->LoadFile();
	if (loadOkay)
		loadOkay = getSessionFromXmlTree(pXmlSessionDocument, &session);

	delete pXmlSessionDocument;
	return loadOkay;
}

bool NppParameters::getSessionFromXmlTree(TiXmlDocument *pSessionDoc, Session *pSession)
{
	if ((pSessionDoc) && (!pSession))
		return false;

	TiXmlDocument **ppSessionDoc = &_pXmlSessionDoc;
	Session *ptrSession = &_session;

	if (pSessionDoc)
	{
		ppSessionDoc = &pSessionDoc;
		ptrSession = pSession;
	}

	if (!*ppSessionDoc)
		return false;
	
	TiXmlNode *root = (*ppSessionDoc)->FirstChild("NotepadPlus");
	if (!root) 
		return false;
	
	TiXmlNode *sessionRoot = root->FirstChildElement("Session");
	if (!sessionRoot)
		return false;

	
	TiXmlElement *actIndex = sessionRoot->ToElement();
	size_t index;
	const char *str = actIndex->Attribute("actifIndex", (int *)&index);
	if (str)
	{
		(*ptrSession)._actifIndex = index;
	}
	for (TiXmlNode *childNode = sessionRoot->FirstChildElement("File");
		childNode ;
		childNode = childNode->NextSibling("File") )
	{
		TiXmlNode *fnNode = childNode->FirstChild();
		const char *fileName = fnNode->Value();
		Position position;
		(childNode->ToElement())->Attribute("firstVisibleLine", &position._firstVisibleLine);
		(childNode->ToElement())->Attribute("xOffset", &position._xOffset);
		(childNode->ToElement())->Attribute("startPos", &position._startPos);
		(childNode->ToElement())->Attribute("endPos", &position._endPos);

		sessionFileInfo sfi(fileName, position);

		for (TiXmlNode *markNode = fnNode->NextSibling("Mark");
			markNode ;
			markNode = markNode->NextSibling("Mark") )
		{
			int lineNumber;
			const char *lineNumberStr = (markNode->ToElement())->Attribute("line", &lineNumber);
			if (lineNumberStr)
			{
				sfi.marks.push_back(lineNumber);
				//::MessageBox(NULL, "coucou", "", MB_OK);
			}
		}

		if (fileName)
			(*ptrSession)._files.push_back(sfi);
	}
	
	return true;
}
void NppParameters::feedFileListParameters(TiXmlNode *node)
{
	_nbMaxFile = 10;

	TiXmlNode *historyRoot = node->FirstChildElement("History");
	if (!historyRoot) return;

	(historyRoot->ToElement())->Attribute("nbMaxFile", &_nbMaxFile);
	if ((_nbMaxFile < 0) || (_nbMaxFile > 30))
		return;

	for (TiXmlNode *childNode = historyRoot->FirstChildElement("File");
		childNode && (_nbFile < NB_MAX_LRF_FILE);
		childNode = childNode->NextSibling("File") )
	{
		_LRFileList[_nbFile] = new string((childNode->FirstChild())->Value());
		_nbFile++;
	}
}

void NppParameters::feedShortcut(TiXmlNode *node)
{
	TiXmlNode *shortcutsRoot = node->FirstChildElement("InternalCommands");
	if (!shortcutsRoot) return;

	for (TiXmlNode *childNode = shortcutsRoot->FirstChildElement("Shortcut");
		childNode ;
		childNode = childNode->NextSibling("Shortcut") )
	{
		int id;
		const char *idStr = (childNode->ToElement())->Attribute("id", &id);
		if (idStr)
		{
			Shortcut sc;
			if (getShortcuts(childNode, sc) && sc.isValid())
			{
				_shortcuts.push_back(CommandShortcut(id, sc));
			}
		}
	}
}

void NppParameters::feedMacros(TiXmlNode *node)
{
	TiXmlNode *macrosRoot = node->FirstChildElement("Macros");
	if (!macrosRoot) return;

	for (TiXmlNode *childNode = macrosRoot->FirstChildElement("Macro");
		childNode ;
		childNode = childNode->NextSibling("Macro") )
	{
		Shortcut sc;
		if (getShortcuts(childNode, sc) && sc.isValid())
		{
			MacroShortcut ms(sc);
			getActions(childNode, ms);
			if (ms.isValid())
				_macros.push_back(ms);
		}
	}
}


void NppParameters::getActions(TiXmlNode *node, MacroShortcut & macroShortcut)
{
	for (TiXmlNode *childNode = node->FirstChildElement("Action");
		childNode ;
		childNode = childNode->NextSibling("Action") )
	{
		int type;
		const char *typeStr = (childNode->ToElement())->Attribute("type", &type);
		if ((!typeStr) || (type > 2))
			continue;

		int msg = 0;
		const char *msgStr = (childNode->ToElement())->Attribute("message", &msg);

		int wParam = 0;
		const char *wParamStr = (childNode->ToElement())->Attribute("wParam", &wParam);

		int lParam = 0;
		const char *lParamStr = (childNode->ToElement())->Attribute("lParam", &lParam);

		const char *sParam = (childNode->ToElement())->Attribute("sParam");
		if (!sParam)
			sParam = "";
		recordedMacroStep step(type, msg, wParam, lParam, sParam);
		if (step.isValid())
			(macroShortcut.getMacro()).push_back(step);

	}
}

void NppParameters::feedUserCmds(TiXmlNode *node)
{
	TiXmlNode *userCmdsRoot = node->FirstChildElement("UserDefinedCommands");
	if (!userCmdsRoot) return;

	for (TiXmlNode *childNode = userCmdsRoot->FirstChildElement("Command");
		childNode ;
		childNode = childNode->NextSibling("Command") )
	{
		Shortcut sc;
		if (getShortcuts(childNode, sc) && sc.isValid())
		{
			UserCommand uc(sc);
			uc._cmd = (childNode->FirstChild())->Value();
			if (uc.isValid())
				_userCommands.push_back(uc);
		}
	}
}

void NppParameters::feedPluginCustomizedCmds(TiXmlNode *node)
{
	TiXmlNode *pluginCustomizedCmdsRoot = node->FirstChildElement("PluginCommands");
	if (!pluginCustomizedCmdsRoot) return;

	for (TiXmlNode *childNode = pluginCustomizedCmdsRoot->FirstChildElement("PluginCommand");
		childNode ;
		childNode = childNode->NextSibling("PluginCommand") )
	{
		Shortcut sc;
		if (getShortcuts(childNode, sc) && sc.isValid())
		{
			const char *moduleName = (childNode->ToElement())->Attribute("moduleName");
			if (!moduleName)
				moduleName = "";

			int internalID = -1;
			const char *internalIDStr = (childNode->ToElement())->Attribute("internalID", &internalID);

			PluginCmdShortcut pcs(sc, -1, moduleName, internalID);
			if (pcs.isValid())
				_pluginCustomizedCmds.push_back(pcs);
		}
	}
}

void NppParameters::feedScintKeys(TiXmlNode *node)
{
	TiXmlNode *scintKeysRoot = node->FirstChildElement("ScintillaKeys");
	if (!scintKeysRoot) return;

	for (TiXmlNode *childNode = scintKeysRoot->FirstChildElement("ScintKey");
		childNode ;
		childNode = childNode->NextSibling("ScintKey") )
	{
		int id;
		const char *idStr = (childNode->ToElement())->Attribute("id", &id);
		if (idStr)
		{
			ScintillaKeyMap skmm(id);
			if (getScintKey(childNode, skmm) && skmm.isValid())
			{
				_scintillaModifiedKeys.push_back(skmm);
			}
		}
	}
}

bool NppParameters::getShortcuts(TiXmlNode *node, Shortcut & sc)
{
	if (!node) return false;

	const char *name = (node->ToElement())->Attribute("name");
	if (!name)
		name = "";

	bool isCtrl = false;
	const char *isCtrlStr = (node->ToElement())->Attribute("Ctrl");
	if (isCtrlStr)
		isCtrl = !strcmp("yes", isCtrlStr);

	bool isAlt = false;
	const char *isAltStr = (node->ToElement())->Attribute("Alt");
	if (isAltStr)
		isAlt = !strcmp("yes", isAltStr);

	bool isShift = false;
	const char *isShiftStr = (node->ToElement())->Attribute("Shift");
	if (isShiftStr)
		isShift = !strcmp("yes", isShiftStr);

	int key;
	const char *keyStr = (node->ToElement())->Attribute("Key", &key);
	if (!keyStr)
		return false;

	strcpy(sc._name, name);
	sc._isCtrl = isCtrl;
	sc._isAlt = isAlt;
	sc._isShift = isShift;
	sc._key = (unsigned char)key;
	return true;
}

bool NppParameters::getScintKey(TiXmlNode *node, ScintillaKeyMap & skm)
{
	if (getShortcuts(node, skm))
	{
		int scintKey;
		const char *keyStr = (node->ToElement())->Attribute("ScintID", &scintKey);
		if (!keyStr)
			return false;

		int menuID;
		keyStr = (node->ToElement())->Attribute("menuCmdID", &menuID);
		if (!keyStr)
			return false;
		skm.setScintKey(scintKey);
		skm.setMenuID(menuID);
		return true;
	}
	return false;
}

const int loadFailed = 100;
const int missingName = 101;
void NppParameters::feedUserLang(TiXmlNode *node)
{
	for (TiXmlNode *childNode = node->FirstChildElement("UserLang");
		childNode && (_nbUserLang < NB_MAX_USER_LANG);
		childNode = childNode->NextSibling("UserLang") )
	{
		const char *name = (childNode->ToElement())->Attribute("name");
		const char *ext = (childNode->ToElement())->Attribute("ext");
		try {
			if (!name || !name[0] || !ext) throw int(missingName);

			_userLangArray[_nbUserLang] = new UserLangContainer(name, ext);
			_nbUserLang++;

			TiXmlNode *settingsRoot = childNode->FirstChildElement("Settings");
			if (!settingsRoot) throw int(loadFailed);
			feedUserSettings(settingsRoot);

			TiXmlNode *keywordListsRoot = childNode->FirstChildElement("KeywordLists");
			if (!keywordListsRoot) throw int(loadFailed);
			feedUserKeywordList(keywordListsRoot);

			TiXmlNode *stylesRoot = childNode->FirstChildElement("Styles");
			if (!stylesRoot) throw int(loadFailed);
			feedUserStyles(stylesRoot);

		} catch (int e) {
			if (e == loadFailed)
				delete _userLangArray[--_nbUserLang];
		}
	}
}

void NppParameters::writeUserDefinedLang()
{
	if (!_pXmlUserLangDoc)
	{
		//do the treatment
		_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
	}

	//before remove the branch, we allocate and copy the char * which will be destroyed
	stylerStrOp(DUP);

	TiXmlNode *root = _pXmlUserLangDoc->FirstChild("NotepadPlus");
	if (root) 
	{
		_pXmlUserLangDoc->RemoveChild(root);
	}
	
	_pXmlUserLangDoc->InsertEndChild(TiXmlElement("NotepadPlus"));

	root = _pXmlUserLangDoc->FirstChild("NotepadPlus");

	for (int i = 0 ; i < _nbUserLang ; i++)
	{
		insertUserLang2Tree(root, _userLangArray[i]);
	}
	_pXmlUserLangDoc->SaveFile();
	stylerStrOp(FREE);
}

void NppParameters::insertCmd(TiXmlNode *shortcutsRoot, const CommandShortcut & cmd)
{
	TiXmlNode *sc = shortcutsRoot->InsertEndChild(TiXmlElement("Shortcut"));
	sc->ToElement()->SetAttribute("name", cmd._name);
	sc->ToElement()->SetAttribute("id", cmd.getID());
	sc->ToElement()->SetAttribute("Ctrl", cmd._isCtrl?"yes":"no");
	sc->ToElement()->SetAttribute("Alt", cmd._isAlt?"yes":"no");
	sc->ToElement()->SetAttribute("Shift", cmd._isShift?"yes":"no");
	sc->ToElement()->SetAttribute("Key", cmd._key);
}

void NppParameters::insertMacro(TiXmlNode *macrosRoot, const MacroShortcut & macro)
{
	TiXmlNode *macroRoot = macrosRoot->InsertEndChild(TiXmlElement("Macro"));

	macroRoot->ToElement()->SetAttribute("name", macro._name);
	macroRoot->ToElement()->SetAttribute("Ctrl", macro._isCtrl?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Alt", macro._isAlt?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Shift", macro._isShift?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Key", macro._key);
	for (size_t i = 0 ; i < macro._macro.size() ; i++)
	{
		TiXmlNode *actionNode = macroRoot->InsertEndChild(TiXmlElement("Action"));
		const recordedMacroStep & action = macro._macro[i];
		actionNode->ToElement()->SetAttribute("type", action.MacroType);
		actionNode->ToElement()->SetAttribute("message", action.message);
		actionNode->ToElement()->SetAttribute("wParam", action.wParameter);
		actionNode->ToElement()->SetAttribute("lParam", action.lParameter);
		actionNode->ToElement()->SetAttribute("sParam", action.sParameter.c_str());
	}
}

void NppParameters::insertUserCmd(TiXmlNode *userCmdRoot, const UserCommand & userCmd)
{
	TiXmlNode *cmdRoot = userCmdRoot->InsertEndChild(TiXmlElement("Command"));
	cmdRoot->ToElement()->SetAttribute("name", userCmd._name);
	cmdRoot->ToElement()->SetAttribute("Ctrl", userCmd._isCtrl?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Alt", userCmd._isAlt?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Shift", userCmd._isShift?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Key", userCmd._key);
	cmdRoot->InsertEndChild(TiXmlText(userCmd._cmd.c_str()));
}

void NppParameters::insertPluginCmd(TiXmlNode *pluginCmdRoot, const PluginCmdShortcut & pluginCmd)
{
	TiXmlNode *pluginCmdNode = pluginCmdRoot->InsertEndChild(TiXmlElement("PluginCommand"));
	pluginCmdNode->ToElement()->SetAttribute("moduleName", pluginCmd._moduleName);
	pluginCmdNode->ToElement()->SetAttribute("internalID", pluginCmd._internalID);
	pluginCmdNode->ToElement()->SetAttribute("Ctrl", pluginCmd._isCtrl?"yes":"no");
	pluginCmdNode->ToElement()->SetAttribute("Alt", pluginCmd._isAlt?"yes":"no");
	pluginCmdNode->ToElement()->SetAttribute("Shift", pluginCmd._isShift?"yes":"no");
	pluginCmdNode->ToElement()->SetAttribute("Key", pluginCmd._key);
}

void NppParameters::insertScintKey(TiXmlNode *scintKeyRoot, const ScintillaKeyMap & scintKeyMap)
{
	TiXmlNode *keyRoot = scintKeyRoot->InsertEndChild(TiXmlElement("ScintKey"));

	keyRoot->ToElement()->SetAttribute("name", scintKeyMap._name);
	keyRoot->ToElement()->SetAttribute("id", scintKeyMap.getID());
	keyRoot->ToElement()->SetAttribute("Ctrl", scintKeyMap._isCtrl?"yes":"no");
	keyRoot->ToElement()->SetAttribute("Alt", scintKeyMap._isAlt?"yes":"no");
	keyRoot->ToElement()->SetAttribute("Shift", scintKeyMap._isShift?"yes":"no");
	keyRoot->ToElement()->SetAttribute("Key", scintKeyMap._key);
	keyRoot->ToElement()->SetAttribute("ScintID", scintKeyMap.getScintillaKey());
	keyRoot->ToElement()->SetAttribute("menuCmdID", scintKeyMap.getMenuCmdID());
}

void NppParameters::writeSession(const Session & session, const char *fileName)
{
	const char *pathName = fileName?fileName:_sessionPath;

	_pXmlSessionDoc = new TiXmlDocument(pathName);
	TiXmlNode *root = _pXmlSessionDoc->InsertEndChild(TiXmlElement("NotepadPlus"));

	if (root)
	{
		TiXmlNode *sessionNode = root->InsertEndChild(TiXmlElement("Session"));
		(sessionNode->ToElement())->SetAttribute("actifIndex", (int)session._actifIndex);
		for (size_t i = 0 ; i < session._files.size() ; i++)
		{
			TiXmlNode *fileNameNode = sessionNode->InsertEndChild(TiXmlElement("File"));
			
			(fileNameNode->ToElement())->SetAttribute("firstVisibleLine", session._files[i]._firstVisibleLine);
			(fileNameNode->ToElement())->SetAttribute("xOffset", session._files[i]._xOffset);
			(fileNameNode->ToElement())->SetAttribute("startPos", session._files[i]._startPos);
			(fileNameNode->ToElement())->SetAttribute("endPos", session._files[i]._endPos);

			TiXmlText fileNameFullPath(session._files[i]._fileName.c_str());
			fileNameNode->InsertEndChild(fileNameFullPath);
			for (size_t j = 0 ; j < session._files[i].marks.size() ; j++)
			{
				size_t markLine = session._files[i].marks[j];
				TiXmlNode *markNode = fileNameNode->InsertEndChild(TiXmlElement("Mark"));
				markNode->ToElement()->SetAttribute("line", markLine);
			}
		}
	}
	_pXmlSessionDoc->SaveFile();

}

void NppParameters::writeShortcuts(bool rewriteCmdSc, bool rewriteMacrosSc, bool rewriteUserCmdSc, bool rewriteScintillaKey, bool rewritePluginCmdSc)
{
	if (!_pXmlShortcutDoc)
	{
		//do the treatment
		_pXmlShortcutDoc = new TiXmlDocument(_shortcutsPath);
	}

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	if (!root)
	{
		root = _pXmlShortcutDoc->InsertEndChild(TiXmlElement("NotepadPlus"));
		//root = _pXmlShortcutDoc->FirstChild("NotepadPlus");
	}

	if (rewriteCmdSc)
	{
		TiXmlNode *cmdRoot = root->FirstChild("InternalCommands");
		if (cmdRoot)
			root->RemoveChild(cmdRoot);

		cmdRoot = root->InsertEndChild(TiXmlElement("InternalCommands"));

		for (size_t i = 0 ; i < _shortcuts.size() ; i++)
		{
			insertCmd(cmdRoot, _shortcuts[i]);
		}
	}

	if (rewriteMacrosSc)
	{
		TiXmlNode *macrosRoot = root->FirstChild("Macros");
		if (macrosRoot)
			root->RemoveChild(macrosRoot);

		macrosRoot = root->InsertEndChild(TiXmlElement("Macros"));
		
		for (size_t i = 0 ; i < _macros.size() ; i++)
		{
			insertMacro(macrosRoot, _macros[i]);
		}
	}

	if (rewriteUserCmdSc)
	{
		TiXmlNode *userCmdRoot = root->FirstChild("UserDefinedCommands");
		if (userCmdRoot)
			root->RemoveChild(userCmdRoot);
		
		userCmdRoot = root->InsertEndChild(TiXmlElement("UserDefinedCommands"));
		
		for (size_t i = 0 ; i < _userCommands.size() ; i++)
		{
			insertUserCmd(userCmdRoot, _userCommands[i]);
		}
	}

	if (rewriteScintillaKey)
	{
		TiXmlNode *scitillaKeyRoot = root->FirstChild("ScintillaKeys");
		if (scitillaKeyRoot)
			root->RemoveChild(scitillaKeyRoot);

		scitillaKeyRoot = root->InsertEndChild(TiXmlElement("ScintillaKeys"));
		for (size_t i = 0 ; i < _scintillaModifiedKeys.size() ; i++)
		{
			insertScintKey(scitillaKeyRoot, _scintillaModifiedKeys[i]);
		}
	}

	if (rewritePluginCmdSc)
	{
		TiXmlNode *pluginCmdRoot = root->FirstChild("PluginCommands");
		if (pluginCmdRoot)
			root->RemoveChild(pluginCmdRoot);

		pluginCmdRoot = root->InsertEndChild(TiXmlElement("PluginCommands"));
		for (size_t i = 0 ; i < _pluginCustomizedCmds.size() ; i++)
		{
			insertPluginCmd(pluginCmdRoot, _pluginCustomizedCmds[i]);
		}
	}
	if (rewriteCmdSc || rewriteMacrosSc || rewriteUserCmdSc || rewriteScintillaKey || rewritePluginCmdSc)
		_pXmlShortcutDoc->SaveFile();
}

int NppParameters::addUserLangToEnd(const UserLangContainer & userLang, const char *newName)
{
	if (isExistingUserLangName(newName))
		return -1;
	_userLangArray[_nbUserLang] = new UserLangContainer();
	*(_userLangArray[_nbUserLang]) = userLang;
	strcpy(_userLangArray[_nbUserLang]->_name, newName);
	_nbUserLang++;
	return _nbUserLang-1;
}

void NppParameters::removeUserLang(int index)
{
	if (index >= _nbUserLang )
		return;
	delete _userLangArray[index];
	for (int i = index ; i < (_nbUserLang - 1) ; i++)
		_userLangArray[i] = _userLangArray[i+1];
	_nbUserLang--;
}

int NppParameters::getIndexFromKeywordListName(const char *name)
{
	if (!name) return -1;
	if (!strcmp(name, "Folder+"))	return 1;
	else if (!strcmp(name, "Folder-"))	return 2;
	else if (!strcmp(name, "Operators"))return 3;
	else if (!strcmp(name, "Comment"))	return 4;
	else if (!strcmp(name, "Words1"))	return 5;
	else if (!strcmp(name, "Words2"))	return 6;
	else if (!strcmp(name, "Words3"))	return 7;
	else if (!strcmp(name, "Words4"))	return 8;
	else if (!strcmp(name, "Delimiters"))	return 0;
	else return -1;
}
void NppParameters::feedUserSettings(TiXmlNode *settingsRoot)
{
	const char *boolStr;
	TiXmlNode *globalSettingNode = settingsRoot->FirstChildElement("Global");
	if (globalSettingNode)
	{
		boolStr = (globalSettingNode->ToElement())->Attribute("caseIgnored");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_isCaseIgnored = !strcmp("yes", boolStr);
	}
	TiXmlNode *treatAsSymbolNode = settingsRoot->FirstChildElement("TreatAsSymbol");
	if (treatAsSymbolNode)
	{
		boolStr = (treatAsSymbolNode->ToElement())->Attribute("comment");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_isCommentSymbol = !strcmp("yes", boolStr);

		boolStr = (treatAsSymbolNode->ToElement())->Attribute("commentLine");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_isCommentLineSymbol = !strcmp("yes", boolStr);
	}
	TiXmlNode *prefixNode = settingsRoot->FirstChildElement("Prefix");
	if (prefixNode)
	{
		char names[nbPrefixListAllowed][7] = {"words1","words2","words3","words4"};
		for (int i = 0 ; i < nbPrefixListAllowed ; i++)
		{
			boolStr = (prefixNode->ToElement())->Attribute(names[i]);
			if (boolStr)
				_userLangArray[_nbUserLang - 1]->_isPrefix[i] = !strcmp("yes", boolStr);
		}
	}
}

void NppParameters::feedUserKeywordList(TiXmlNode *node)
{
	for (TiXmlNode *childNode = node->FirstChildElement("Keywords");
		childNode ;
		childNode = childNode->NextSibling("Keywords"))
	{
		const char *keywordsName = (childNode->ToElement())->Attribute("name");
		int i = getIndexFromKeywordListName(keywordsName);
		if (i != -1)
		{
			TiXmlNode *valueNode = childNode->FirstChild();
			const char *kwl = (valueNode)?valueNode->Value():(strcmp(keywordsName, "Delimiters")?"":"000000");
			strcpy(_userLangArray[_nbUserLang - 1]->_keywordLists[i], kwl);
		}
	}
}

void NppParameters::feedUserStyles(TiXmlNode *node)
{
	for (TiXmlNode *childNode = node->FirstChildElement("WordsStyle");
		childNode ;
		childNode = childNode->NextSibling("WordsStyle"))
	{
		int id;
		const char *styleIDStr = (childNode->ToElement())->Attribute("styleID", &id);
		if (styleIDStr)
		{
			_userLangArray[_nbUserLang - 1]->_styleArray.addStyler(id, childNode);
		}
	}
}

bool NppParameters::feedStylerArray(TiXmlNode *node)
{
    TiXmlNode *styleRoot = node->FirstChildElement("LexerStyles");
    if (!styleRoot) return false;

    // For each lexer
    for (TiXmlNode *childNode = styleRoot->FirstChildElement("LexerType");
		 childNode ;
		 childNode = childNode->NextSibling("LexerType") )
    {
     	if (!_lexerStylerArray.hasEnoughSpace()) return false;

	    TiXmlElement *element = childNode->ToElement();
	    const char *lexerName = element->Attribute("name");
		const char *lexerDesc = element->Attribute("desc");
		const char *lexerUserExt = element->Attribute("ext");
	    if (lexerName)
	    {
		    _lexerStylerArray.addLexerStyler(lexerName, lexerDesc, lexerUserExt, childNode);
	    }
    }

    // The global styles for all lexers
    TiXmlNode *globalStyleRoot = node->FirstChildElement("GlobalStyles");
    if (!globalStyleRoot) return false;

    for (TiXmlNode *childNode = globalStyleRoot->FirstChildElement("WidgetStyle");
		 childNode ;
		 childNode = childNode->NextSibling("WidgetStyle") )
    {
     	if (!_widgetStyleArray.hasEnoughSpace()) return false;

	    TiXmlElement *element = childNode->ToElement();
	    const char *styleIDStr = element->Attribute("styleID");

        int styleID = -1;
		if ((styleID = decStrVal(styleIDStr)) != -1)
		{
		    _widgetStyleArray.addStyler(styleID, childNode);
        }
    }
	return true;
}

void LexerStylerArray::addLexerStyler(const char *lexerName, const char *lexerDesc, const char *lexerUserExt , TiXmlNode *lexerNode)
{
    LexerStyler & ls = _lexerStylerArray[_nbLexerStyler++];
    ls.setLexerName(lexerName);
	if (lexerDesc)
		ls.setLexerDesc(lexerDesc);

	if (lexerUserExt)
		ls.setLexerUserExt(lexerUserExt);
    
    for (TiXmlNode *childNode = lexerNode->FirstChildElement("WordsStyle");
		 childNode ;
		 childNode = childNode->NextSibling("WordsStyle") )
    {
	        
		if (!ls.hasEnoughSpace()) return;

		TiXmlElement *element = childNode->ToElement();
		const char *styleIDStr = element->Attribute("styleID");
		
		if (styleIDStr)
		{
			int styleID = -1;
			if ((styleID = decStrVal(styleIDStr)) != -1)
			{
				ls.addStyler(styleID, childNode);
			}
		}
    }
}

void StyleArray::addStyler(int styleID, TiXmlNode *styleNode)
{
	_styleArray[_nbStyler]._styleID = styleID;
	
	if (styleNode)
	{
		TiXmlElement *element = styleNode->ToElement();
		
		// Pour _fgColor, _bgColor :
		// RGB() | (result & 0xFF000000) c'est pour le cas de -1 (0xFFFFFFFF)
		// retourné par hexStrVal(str)
		const char *str = element->Attribute("name");
		if (str)
		{
			_styleArray[_nbStyler]._styleDesc = str;
		}

		str = element->Attribute("fgColor");
		if (str)
		{
			unsigned long result = hexStrVal(str);
			_styleArray[_nbStyler]._fgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);
	            
		}
		
		str = element->Attribute("bgColor");
		if (str)
		{
			unsigned long result = hexStrVal(str);
			_styleArray[_nbStyler]._bgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);
		}
		
		str = element->Attribute("fontName");
		_styleArray[_nbStyler]._fontName = str;
		
		str = element->Attribute("fontStyle");
		if (str)
		{
			_styleArray[_nbStyler]._fontStyle = decStrVal(str);
		}
		
		str = element->Attribute("fontSize");
		if (str)
		{
			_styleArray[_nbStyler]._fontSize = decStrVal(str);
		}

		str = element->Attribute("keywordClass");
		if (str)
		{
			_styleArray[_nbStyler]._keywordClass = getKwClassFromName(str);
		}

		TiXmlNode *v = styleNode->FirstChild();
		if (v)
		{
			//const char *keyWords = v->Value();
			//if (keyWords)
				_styleArray[_nbStyler]._keywords = new string(v->Value());
		}
	}
	_nbStyler++;
}

void NppParameters::writeHistory(const char *fullpath)
{
	TiXmlNode *historyNode = (_pXmlUserDoc->FirstChild("NotepadPlus"))->FirstChildElement("History");
	TiXmlElement recentFileNode("File");
	TiXmlText fileNameFullPath(fullpath);
	recentFileNode.InsertEndChild(fileNameFullPath);

	(historyNode->ToElement())->InsertEndChild(recentFileNode);
}

TiXmlNode * NppParameters::getChildElementByAttribut(TiXmlNode *pere, const char *childName,\
			const char *attributName, const char *attributVal) const 
{
	for (TiXmlNode *childNode = pere->FirstChildElement(childName);
		childNode ;
		childNode = childNode->NextSibling(childName))
	{
		TiXmlElement *element = childNode->ToElement();
		const char *val = element->Attribute(attributName);
		if (val)
		{
			if (!strcmp(val, attributVal))
				return childNode;
		}
	}
	return NULL;
}

// 2 restes : L_H, L_USER
LangType NppParameters::getLangIDFromStr(const char *langName)
{
	if (!strcmp("c", langName))	return L_C;
	if (!strcmp("cpp", langName)) return L_CPP;
	if (!strcmp("java", langName)) return L_JAVA;
	if (!strcmp("cs", langName)) return L_CS;
	if (!strcmp("objc", langName)) return L_OBJC;
	if (!strcmp("rc", langName)) return L_RC;
	if (!strcmp("html", langName)) return L_HTML;
	if (!strcmp("javascript", langName)) return L_JS;
	if (!strcmp("php", langName)) return L_PHP;
	if (!strcmp("vb", langName)) return L_VB;
    if (!strcmp("sql", langName)) return L_SQL;
	if (!strcmp("xml", langName)) return L_XML;
	if (!strcmp("asp", langName)) return L_ASP;
	if (!strcmp("perl", langName)) return L_PERL;
	if (!strcmp("pascal", langName)) return L_PASCAL;
	if (!strcmp("python", langName)) return L_PYTHON;
	if (!strcmp("css", langName)) return L_CSS;
	if (!strcmp("lua", langName)) return L_LUA;
	if (!strcmp("batch", langName)) return L_BATCH;
	if (!strcmp("ini", langName)) return L_INI;
	if (!strcmp("nfo", langName)) return L_NFO;
	if (!strcmp("makefile", langName)) return L_MAKEFILE;
	if (!strcmp("tex", langName)) return L_TEX;
	if (!strcmp("fortran", langName)) return L_FORTRAN;
	if (!strcmp("bash", langName)) return L_BASH;
	if (!strcmp("actionscript", langName)) return L_FLASH;	
	if (!strcmp("nsis", langName)) return L_NSIS;
	if (!strcmp("tcl", langName)) return L_TCL;

	if (!strcmp("lisp", langName)) return L_LISP;
	if (!strcmp("scheme", langName)) return L_SCHEME;
	if (!strcmp("asm", langName)) return L_ASM;
	if (!strcmp("diff", langName)) return L_DIFF;
	if (!strcmp("props", langName)) return L_PROPS;
	if (!strcmp("postscript", langName)) return L_PS;
	if (!strcmp("ruby", langName)) return L_RUBY;
	if (!strcmp("smalltalk", langName)) return L_SMALLTALK;
	if (!strcmp("vhdl", langName)) return L_VHDL;

	if (!strcmp("caml", langName)) return L_CAML;
	if (!strcmp("verilog", langName)) return L_VERILOG;
	if (!strcmp("kix", langName)) return L_KIX;
	if (!strcmp("autoit", langName)) return L_AU3;
	if (!strcmp("ada", langName)) return L_ADA;
	if (!strcmp("matlab", langName)) return L_MATLAB;
	if (!strcmp("haskell", langName)) return L_HASKELL;
	if (!strcmp("inno", langName)) return L_INNO;
	if (!strcmp("searchResult", langName)) return L_SEARCHRESULT;
	if (!strcmp("cmake", langName)) return L_CMAKE;
	return L_TXT;
}

int NppParameters::getIndexFromStr(const char *indexName) const
{
	string str(indexName);

	if (str == "instre1")
		return LANG_INDEX_INSTR;
	else if (str == "instre2")
		return LANG_INDEX_INSTR2;
	else if (str == "type1")
		return LANG_INDEX_TYPE;
	else if (str == "type2")
		return LANG_INDEX_TYPE2;
	else if (str == "type3")
		return LANG_INDEX_TYPE3;
	else if (str == "type4")
		return LANG_INDEX_TYPE4;
	else
		return -1;
}

void NppParameters::feedKeyWordsParameters(TiXmlNode *node)
{
	
	TiXmlNode *langRoot = node->FirstChildElement("Languages");
	if (!langRoot) return;

	for (TiXmlNode *langNode = langRoot->FirstChildElement("Language");
		langNode ;
		langNode = langNode->NextSibling("Language") )
	{
		if (_nbLang < NB_LANG)
		{
			TiXmlElement *element = langNode->ToElement();
			const char *name = element->Attribute("name");
			if (name)
			{
				_langList[_nbLang] = new Lang(getLangIDFromStr(name), name);
				_langList[_nbLang]->setDefaultExtList(element->Attribute("ext"));
				_langList[_nbLang]->setCommentLineSymbol(element->Attribute("commentLine"));
				_langList[_nbLang]->setCommentStart(element->Attribute("commentStart"));
				_langList[_nbLang]->setCommentEnd(element->Attribute("commentEnd"));

				for (TiXmlNode *kwNode = langNode->FirstChildElement("Keywords");
					kwNode ;
					kwNode = kwNode->NextSibling("Keywords") )
				{
					const char *indexName = (kwNode->ToElement())->Attribute("name");
					TiXmlNode *kwVal = kwNode->FirstChild();
					const char *keyWords = "";
					if ((indexName) && (kwVal))
						keyWords = kwVal->Value();

					int i = getIndexFromStr(indexName);

					if (i != -1)
						_langList[_nbLang]->setWords(keyWords, i);
				}
				_nbLang++;
			}
		}
	}
}

void NppParameters::feedGUIParameters(TiXmlNode *node)
{
	TiXmlNode *GUIRoot = node->FirstChildElement("GUIConfigs");
	if (!GUIRoot) return;

	for (TiXmlNode *childNode = GUIRoot->FirstChildElement("GUIConfig");
		childNode ;
		childNode = childNode->NextSibling("GUIConfig") )
	{
		TiXmlElement *element = childNode->ToElement();
		const char *nm = element->Attribute("name");
		if (!nm) return;

		const char *val;

		if (!strcmp(nm, "ToolBar"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "hide"))
				_nppGUI._toolBarStatus = TB_HIDE;
			else if (!strcmp(val, "small"))
				_nppGUI._toolBarStatus = TB_SMALL;
			else if (!strcmp(val, "large"))
				_nppGUI._toolBarStatus = TB_LARGE;
			else if (!strcmp(val, "standard"))
				_nppGUI._toolBarStatus = TB_STANDARD;
		}
		else if (!strcmp(nm, "StatusBar"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "hide"))
				_nppGUI._statusBarShow = false;
			else if (!strcmp(val, "show"))
				_nppGUI._statusBarShow = true;
		}
		else if (!strcmp(nm, "TabBar"))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._tabStatus;
			val = element->Attribute("dragAndDrop");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus = TAB_DRAGNDROP;
				else if (!strcmp(val, "no"))
					_nppGUI._tabStatus = 0;
				else
					isFailed = true;
			}

			val = element->Attribute("drawTopBar");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus |= TAB_DRAWTOPBAR;
				else if (!strcmp(val, "no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute("drawInactiveTab");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus |= TAB_DRAWINACTIVETAB;
				else if (!strcmp(val, "no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute("reduce");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus |= TAB_REDUCE;
				else if (!strcmp(val, "no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute("closeButton");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus |= TAB_CLOSEBUTTON;
			}

			val = element->Attribute("doubleClick2Close");
			if (val)
			{
				if (!strcmp(val, "yes"))
					_nppGUI._tabStatus |= TAB_DBCLK2CLOSE;
			}
			if (isFailed)
				_nppGUI._tabStatus = oldValue;
		}
		else if (!strcmp(nm, "Auto-detection"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "no"))
				_nppGUI._fileAutoDetection = cdDisabled;
			else if (!strcmp(val, "auto"))
            	_nppGUI._fileAutoDetection = cdAutoUpdate;
         	else
            	_nppGUI._fileAutoDetection = cdEnabled;
		}
		else if (!strcmp(nm, "TrayIcon"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "yes"))
				_nppGUI._isMinimizedToTray = true;
		}
		else if (!strcmp(nm, "RememberLastSession"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "yes"))
				_nppGUI._rememberLastSession = true;
			else
				_nppGUI._rememberLastSession = false;
		}

		else if (!strcmp(nm, "MaitainIndent"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "yes"))
				_nppGUI._maitainIndent = true;
			else
				_nppGUI._maitainIndent = false;
		}

		else if (!strcmp(nm, "TaskList"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;
			_nppGUI._doTaskList = (!strcmp(val, "yes"))?true:false;
		}

		else if (!strcmp(nm, "SaveOpenFileInSameDir"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;
			_nppGUI._saveOpenKeepInSameDir = (!strcmp(val, "yes"))?true:false;
		}

		else if (!strcmp(nm, "MRU"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;
			_nppGUI._styleMRU = (!strcmp(val, "yes"))?true:false;
		}

		else if (!strcmp(nm, "URL"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;
			if (!strcmp(val, "1"))
				_nppGUI._styleURL = 1;
			else if (!strcmp(val, "2"))
				_nppGUI._styleURL = 2;
			else
				_nppGUI._styleURL = 0;
		}

		else if (!strcmp(nm, "CheckHistoryFiles"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "no"))
				_nppGUI._checkHistoryFiles = false;
		}
		else if (!strcmp(nm, "ScintillaViewsSplitter"))
		{
			val = (childNode->FirstChild())->Value();
			if (!val) return;

			if (!strcmp(val, "vertical"))
				_nppGUI._splitterPos = POS_VERTICAL;
			else if (!strcmp(val, "horizontal"))
				_nppGUI._splitterPos = POS_HORIZOTAL;
		}
		else if (!strcmp(nm, "UserDefineDlg"))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._userDefineDlgStatus;

			val = (childNode->FirstChild())->Value();
			if (val) 
			{
				if (!strcmp(val, "hide"))
					_nppGUI._userDefineDlgStatus = 0;
				else if (!strcmp(val, "show"))
					_nppGUI._userDefineDlgStatus = UDD_SHOW;
				else 
					isFailed = true;
			}
			val = element->Attribute("position");
			if (val) 
			{
				if (!strcmp(val, "docked"))
					_nppGUI._userDefineDlgStatus |= UDD_DOCKED;
				else if (!strcmp(val, "undocked"))
					_nppGUI._userDefineDlgStatus |= 0;
				else 
					isFailed = true;
			}
			if (isFailed)
				_nppGUI._userDefineDlgStatus = oldValue;
		}
		else if (!strcmp(nm, "TabSetting"))
		{
			val = element->Attribute("size");
			if (val)
				_nppGUI._tabSize = decStrVal(val);

			if ((_nppGUI._tabSize == -1) || (_nppGUI._tabSize == 0))
				_nppGUI._tabSize = 8;
			//bool isFailed = false;
			val = element->Attribute("replaceBySpace");
			if (val)
				_nppGUI._tabReplacedBySpace = (!strcmp(val, "yes"));
		}
		else if (!strcmp(nm, "AppPosition"))
		{
			RECT oldRect = _nppGUI._appPos;
			bool fuckUp = true;
			int i;

			if (element->Attribute("x", &i))
			{
				_nppGUI._appPos.left = i;

				if (element->Attribute("y", &i))
				{
					_nppGUI._appPos.top = i;

					if (element->Attribute("width", &i))
					{
						_nppGUI._appPos.right = i;

						if (element->Attribute("height", &i))
						{
							_nppGUI._appPos.bottom = i;
							fuckUp = false;
						}
					}
				}
			}
			if (fuckUp)
				_nppGUI._appPos = oldRect;

			
			if (val = element->Attribute("isMaximized"))
			{
				_nppGUI._isMaximized = (strcmp(val, "yes") == 0);
			}
		}
		else if (!strcmp(nm, "NewDocDefaultSettings"))
		{
			int i;
			if (element->Attribute("format", &i))
				_nppGUI._newDocDefaultSettings._format = (formatType)i;

			if (element->Attribute("encoding", &i))
				_nppGUI._newDocDefaultSettings._encoding = (UniMode)i;

			if (element->Attribute("lang", &i))
				_nppGUI._newDocDefaultSettings._lang = (LangType)i;
		}
		else if (!strcmp(nm, "langsExcluded"))
		{
			int g0 = 0; // up to  8
			int g1 = 0; // up to 16
			int g2 = 0; // up to 24
			int g3 = 0; // up to 32
			int g4 = 0; // up to 40
			int g5 = 0; // up to 48
			int g6 = 0; // up to 56
			int g7 = 0; // up to 64
			const int nbMax = 64;

			int i;
			if (element->Attribute("gr0", &i))
				if (i <= 255)
					g0 = i;
			if (element->Attribute("gr1", &i))
				if (i <= 255)
					g1 = i;
			if (element->Attribute("gr2", &i))
				if (i <= 255)
					g2 = i;
			if (element->Attribute("gr3", &i))
				if (i <= 255)
					g3 = i;
			if (element->Attribute("gr4", &i))
				if (i <= 255)
					g4 = i;
			if (element->Attribute("gr5", &i))
				if (i <= 255)
					g5 = i;
			if (element->Attribute("gr6", &i))
				if (i <= 255)
					g6 = i;
			if (element->Attribute("gr7", &i))
				if (i <= 255)
					g7 = i;

			bool langArray[nbMax];
			for (int i = 0 ; i < nbMax ; i++) langArray[i] = false;
			
			unsigned char mask = 1;
			for (int i = 0 ; i < 8 ; i++) 
			{
				if (mask & g0)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 8 ; i < 16 ; i++) 
			{
				if (mask & g1)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 16 ; i < 24 ; i++) 
			{
				if (mask & g2)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 24 ; i < 32 ; i++) 
			{
				if (mask & g3)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 32 ; i < 40 ; i++) 
			{
				if (mask & g4)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 40 ; i < 48 ; i++) 
			{
				if (mask & g5)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 48 ; i < 56 ; i++) 
			{
				if (mask & g6)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 56 ; i < 64 ; i++) 
			{
				if (mask & g7)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}
		}

		else if (!strcmp(nm, "Print"))
		{
			val = element->Attribute("lineNumber");
			if (val)
				_nppGUI._printSettings._printLineNumber = (!strcmp(val, "yes"));

			int i;
			if (element->Attribute("printOption", &i))
				_nppGUI._printSettings._printOption = i;

			val = element->Attribute("headerLeft");
			if (val)
				_nppGUI._printSettings._headerLeft = val;

			val = element->Attribute("headerMiddle");
			if (val)
				_nppGUI._printSettings._headerMiddle = val;

			val = element->Attribute("headerRight");
			if (val)
				_nppGUI._printSettings._headerRight = val;


			val = element->Attribute("footerLeft");
			if (val)
				_nppGUI._printSettings._footerLeft = val;

			val = element->Attribute("footerMiddle");
			if (val)
				_nppGUI._printSettings._footerMiddle = val;

			val = element->Attribute("footerRight");
			if (val)
				_nppGUI._printSettings._footerRight = val;


			val = element->Attribute("headerFontName");
			if (val)
				_nppGUI._printSettings._headerFontName = val;

			val = element->Attribute("footerFontName");
			if (val)
				_nppGUI._printSettings._footerFontName = val;

			if (element->Attribute("headerFontStyle", &i))
				_nppGUI._printSettings._headerFontStyle = i;

			if (element->Attribute("footerFontStyle", &i))
				_nppGUI._printSettings._footerFontStyle = i;

			if (element->Attribute("headerFontSize", &i))
				_nppGUI._printSettings._headerFontSize = i;

			if (element->Attribute("footerFontSize", &i))
				_nppGUI._printSettings._footerFontSize = i;


			if (element->Attribute("margeLeft", &i))
				_nppGUI._printSettings._marge.left = i;

			if (element->Attribute("margeTop", &i))
				_nppGUI._printSettings._marge.top = i;

			if (element->Attribute("margeRight", &i))
				_nppGUI._printSettings._marge.right = i;

			if (element->Attribute("margeBottom", &i))
				_nppGUI._printSettings._marge.bottom = i;
		}

		else if (!strcmp(nm, "ScintillaPrimaryView"))
		{
			feedScintillaParam(SCIV_PRIMARY, element);
		}
		else if (!strcmp(nm, "ScintillaSecondaryView"))
		{
			feedScintillaParam(SCIV_SECOND, element);
		}
		else if (!strcmp(nm, "Backup"))
		{
			int i;
			if (element->Attribute("action", &i))
				_nppGUI._backup = (BackupFeature)i;

			const char *bDir = element->Attribute("useCustumDir");
			if (bDir && !strcmp(bDir, "yes")) 
			{
				_nppGUI._useDir = true;
			}
			const char *pDir = element->Attribute("dir");
			if (pDir)
				strcpy(_nppGUI._backupDir, pDir);
		}
		else if (!strcmp(nm, "DockingManager"))
		{
			feedDockingManager(element);
		}
	}
}

void NppParameters::feedScintillaParam(bool whichOne, TiXmlNode *node)
{
    TiXmlElement *element = node->ToElement();

    // Line Number Margin
    const char *nm = element->Attribute("lineNumberMargin");
	if (nm) 
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._lineNumberMarginShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._lineNumberMarginShow = false;
	}

	// Bookmark Margin
	nm = element->Attribute("bookMarkMargin");
	if (nm) 
	{

		if (!strcmp(nm, "show"))
			_svp[whichOne]._bookMarkMarginShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._bookMarkMarginShow = false;
	}

    // Indent GuideLine 
    nm = element->Attribute("indentGuideLine");
	if (nm)
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._indentGuideLineShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._indentGuideLineShow= false;
	}

    // Folder Mark Style
    nm = element->Attribute("folderMarkStyle");
	if (nm)
	{
		if (!strcmp(nm, "box"))
			_svp[whichOne]._folderStyle = FOLDER_STYLE_BOX;
		else if (!strcmp(nm, "circle"))
			_svp[whichOne]._folderStyle = FOLDER_STYLE_CIRCLE;
		else if (!strcmp(nm, "arrow"))
			_svp[whichOne]._folderStyle = FOLDER_STYLE_ARROW;
		else if (!strcmp(nm, "simple"))
			_svp[whichOne]._folderStyle = FOLDER_STYLE_SIMPLE;
	}

    // Current Line Highlighting State
    nm = element->Attribute("currentLineHilitingShow");
	if (nm)
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._currentLineHilitingShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._currentLineHilitingShow = false;
	}

    // Current wrap symbol visibility State
    nm = element->Attribute("wrapSymbolShow");
	if (nm)
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._wrapSymbolShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._wrapSymbolShow = false;
	}

	// Do Wrap
    nm = element->Attribute("Wrap");
	if (nm)
	{
		if (!strcmp(nm, "yes"))
			_svp[whichOne]._doWrap = true;
		else if (!strcmp(nm, "no"))
			_svp[whichOne]._doWrap = false;
	}

	// Do Edge
    nm = element->Attribute("edge");
	if (nm)
	{
		if (!strcmp(nm, "background"))
			_svp[whichOne]._edgeMode = EDGE_BACKGROUND;
		else if (!strcmp(nm, "line"))
			_svp[whichOne]._edgeMode = EDGE_LINE;
		else
			_svp[whichOne]._edgeMode = EDGE_NONE;
	}
	
	int val;
	nm = element->Attribute("edgeNbColumn", &val);
	if (nm)
	{
		_svp[whichOne]._edgeNbColumn = val;
	}

	nm = element->Attribute("zoom", &val);
	if (nm)
	{
		_svp[whichOne]._zoom = val;
	}

	// White Space visibility State
    nm = element->Attribute("whiteSpaceShow");
	if (nm)
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._whiteSpaceShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._whiteSpaceShow = false;
	}

	// EOL visibility State
    nm = element->Attribute("eolShow");
	if (nm)
	{
		if (!strcmp(nm, "show"))
			_svp[whichOne]._eolShow = true;
		else if (!strcmp(nm, "hide"))
			_svp[whichOne]._eolShow = false;
	}
}


void NppParameters::feedDockingManager(TiXmlNode *node)
{
    TiXmlElement *element = node->ToElement();

    int i;
	if (element->Attribute("leftWidth", &i))
		_nppGUI._dockingData._leftWidth = i;

	if (element->Attribute("rightWidth", &i))
		_nppGUI._dockingData._rightWidth = i;

	if (element->Attribute("topHeight", &i))
		_nppGUI._dockingData._topHeight = i;

	if (element->Attribute("bottomHeight", &i))
		_nppGUI._dockingData._bottomHight = i;

	

	for (TiXmlNode *childNode = node->FirstChildElement("FloatingWindow");
		childNode ;
		childNode = childNode->NextSibling("FloatingWindow") )
	{
		TiXmlElement *floatElement = childNode->ToElement();
		int cont;
		if (floatElement->Attribute("cont", &cont))
		{
			int x = 0;
			int y = 0;
			int w = 100;
			int h = 100;

			floatElement->Attribute("x", &x);
			floatElement->Attribute("y", &y);
			floatElement->Attribute("width", &w);
			floatElement->Attribute("height", &h);
			_nppGUI._dockingData._flaotingWindowInfo.push_back(FloatingWindowInfo(cont, x, y, w, h));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement("PluginDlg");
		childNode ;
		childNode = childNode->NextSibling("PluginDlg") )
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		const char *name = dlgElement->Attribute("pluginName");
		
		int id;
		const char *idStr = dlgElement->Attribute("id", &id);
		if (name && idStr)
		{
			int curr = 0; // on left
			int prev = 0; // on left

			dlgElement->Attribute("curr", &curr);
			dlgElement->Attribute("prev", &prev);

			bool isVisible = false;
			const char *val = NULL;
			if (val = dlgElement->Attribute("isVisible"))
			{
				isVisible = (strcmp(val, "yes") == 0);
			}

			_nppGUI._dockingData._pluginDockInfo.push_back(PlugingDlgDockingInfo(name, id, curr, prev, isVisible));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement("ActiveTabs");
		childNode ;
		childNode = childNode->NextSibling("ActiveTabs") )
	{
		TiXmlElement *dlgElement = childNode->ToElement();
	
		int cont;
		if (dlgElement->Attribute("cont", &cont))
		{
			int activeTab = 0;
			dlgElement->Attribute("activeTab", &activeTab);
			_nppGUI._dockingData._containerTabInfo.push_back(ContainerTabInfo(cont, activeTab));
		}
	}
}

void NppParameters::writeScintillaParams(const ScintillaViewParams & svp, bool whichOne) 
{
	if (!_pXmlUserDoc) return;

	const char *pViewName = (whichOne == SCIV_PRIMARY)?"ScintillaPrimaryView":"ScintillaSecondaryView";
	TiXmlNode *configsRoot = (_pXmlUserDoc->FirstChild("NotepadPlus"))->FirstChildElement("GUIConfigs");
	TiXmlNode *scintNode = getChildElementByAttribut(configsRoot, "GUIConfig", "name", pViewName);
	if (scintNode)
	{
		(scintNode->ToElement())->SetAttribute("lineNumberMargin", svp._lineNumberMarginShow?"show":"hide");
		(scintNode->ToElement())->SetAttribute("bookMarkMargin", svp._bookMarkMarginShow?"show":"hide");
		(scintNode->ToElement())->SetAttribute("indentGuideLine", svp._indentGuideLineShow?"show":"hide");
		const char *pFolderStyleStr = (svp._folderStyle == FOLDER_STYLE_SIMPLE)?"simple":
										(svp._folderStyle == FOLDER_STYLE_ARROW)?"arrow":
											(svp._folderStyle == FOLDER_STYLE_CIRCLE)?"circle":"box";
		(scintNode->ToElement())->SetAttribute("folderMarkStyle", pFolderStyleStr);
		(scintNode->ToElement())->SetAttribute("currentLineHilitingShow", svp._currentLineHilitingShow?"show":"hide");
		(scintNode->ToElement())->SetAttribute("wrapSymbolShow", svp._wrapSymbolShow?"show":"hide");
		(scintNode->ToElement())->SetAttribute("Wrap", svp._doWrap?"yes":"no");
		char *edgeStr = NULL;
		if (svp._edgeMode == EDGE_NONE)
			edgeStr = "no";
		else if (svp._edgeMode == EDGE_LINE)
			edgeStr = "line";
		else
			edgeStr = "background";
		(scintNode->ToElement())->SetAttribute("edge", edgeStr);
		(scintNode->ToElement())->SetAttribute("edgeNbColumn", svp._edgeNbColumn);
		(scintNode->ToElement())->SetAttribute("zoom", svp._zoom);
		(scintNode->ToElement())->SetAttribute("whiteSpaceShow", svp._whiteSpaceShow?"show":"hide");
		(scintNode->ToElement())->SetAttribute("eolShow", svp._eolShow?"show":"hide");
	}
	else {/*create one*/}
}

void NppParameters::writeGUIParams()
{
	if (!_pXmlUserDoc) return;

	TiXmlNode *GUIRoot = (_pXmlUserDoc->FirstChild("NotepadPlus"))->FirstChildElement("GUIConfigs");
	if (!GUIRoot) 
	{
		return;
	}

	bool autoDetectionExist = false;
	bool checkHistoryFilesExist = false;
	bool trayIconExist = false;
	bool rememberLastSessionExist = false;
	bool newDocDefaultSettingsExist = false;
	bool langsExcludedLstExist = false;
	bool printSettingExist = false;
	bool doTaskListExist = false;
	bool maitainIndentExist = false;
	bool MRUExist = false;
	bool backExist = false;
	bool saveOpenFileInSameDirExist = false;
	bool URLExist = false;

	TiXmlNode *dockingParamNode = NULL;

	for (TiXmlNode *childNode = GUIRoot->FirstChildElement("GUIConfig");
		childNode ;
		childNode = childNode->NextSibling("GUIConfig"))
	{
		TiXmlElement *element = childNode->ToElement();
		const char *nm = element->Attribute("name");
		if (!nm) {return;}

		if (!strcmp(nm, "ToolBar"))
		{
			const char *pStr = _nppGUI._toolBarStatus == TB_HIDE?"hide":(_nppGUI._toolBarStatus == TB_SMALL?"small":(_nppGUI._toolBarStatus == TB_STANDARD?"standard":"large"));
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "StatusBar"))
		{
			const char *pStr = _nppGUI._statusBarShow?"show":"hide";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "TabBar"))
		{
			const char *pStr = (_nppGUI._tabStatus & TAB_DRAWTOPBAR)?"yes":"no";
			element->SetAttribute("dragAndDrop", pStr);

			pStr = (_nppGUI._tabStatus & TAB_DRAGNDROP)?"yes":"no";
			element->SetAttribute("drawTopBar", pStr);

			pStr = (_nppGUI._tabStatus & TAB_DRAWINACTIVETAB)?"yes":"no";
			element->SetAttribute("drawInactiveTab", pStr);

			pStr = (_nppGUI._tabStatus & TAB_REDUCE)?"yes":"no";
			element->SetAttribute("reduce", pStr);

			pStr = (_nppGUI._tabStatus & TAB_CLOSEBUTTON)?"yes":"no";
			element->SetAttribute("closeButton", pStr);

			pStr = (_nppGUI._tabStatus & TAB_DBCLK2CLOSE)?"yes":"no";
			element->SetAttribute("doubleClick2Close", pStr);
		}
		else if (!strcmp(nm, "ScintillaViewsSplitter"))
		{
			const char *pStr = _nppGUI._splitterPos == POS_VERTICAL?"vertical":"horizontal";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "UserDefineDlg"))
		{
			const char *pStr = _nppGUI._userDefineDlgStatus & UDD_SHOW?"show":"hide";
			(childNode->FirstChild())->SetValue(pStr);
			
			pStr = (_nppGUI._userDefineDlgStatus & UDD_DOCKED)?"docked":"undocked";
			element->SetAttribute("position", pStr);
		}
		else if (!strcmp(nm, "TabSetting"))
		{
			const char *pStr = _nppGUI._tabReplacedBySpace?"yes":"no";
			element->SetAttribute("replaceBySpace", pStr);
			element->SetAttribute("size", _nppGUI._tabSize);
		}
		else if (!strcmp(nm, "Auto-detection"))
		{
			autoDetectionExist = true;
			const char *pStr = (cdEnabled == _nppGUI._fileAutoDetection)?"yes":((cdAutoUpdate == _nppGUI._fileAutoDetection)?"auto":"no");
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "TrayIcon"))
		{
			trayIconExist = true;
			const char *pStr = _nppGUI._isMinimizedToTray?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "RememberLastSession"))
		{
			rememberLastSessionExist = true;
			const char *pStr = _nppGUI._rememberLastSession?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}

		else if (!strcmp(nm, "MaitainIndent"))
		{
			maitainIndentExist = true;
			const char *pStr = _nppGUI._maitainIndent?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}

		else if (!strcmp(nm, "SaveOpenFileInSameDir"))
		{
			saveOpenFileInSameDirExist = true;
			const char *pStr = _nppGUI._saveOpenKeepInSameDir?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}

		else if (!strcmp(nm, "TaskList"))
		{
			doTaskListExist = true;
			const char *pStr = _nppGUI._doTaskList?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "CheckHistoryFiles"))
		{
			checkHistoryFilesExist = true;
			const char *pStr = _nppGUI._checkHistoryFiles?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "AppPosition"))
		{
			element->SetAttribute("x", _nppGUI._appPos.left);
			element->SetAttribute("y", _nppGUI._appPos.top);
			element->SetAttribute("width", _nppGUI._appPos.right);
			element->SetAttribute("height", _nppGUI._appPos.bottom);
			element->SetAttribute("isMaximized", _nppGUI._isMaximized?"yes":"no");
		}
		else if (!strcmp(nm, "NewDocDefaultSettings"))
		{
			element->SetAttribute("format", _nppGUI._newDocDefaultSettings._format);
			element->SetAttribute("encoding", _nppGUI._newDocDefaultSettings._encoding);
			element->SetAttribute("lang", _nppGUI._newDocDefaultSettings._lang);
			newDocDefaultSettingsExist = true;
		}
		else if (!strcmp(nm, "langsExcluded"))
		{
			writeExcludedLangList(element);
			langsExcludedLstExist = true;
		}
		else if (!strcmp(nm, "Print"))
		{
			writePrintSetting(element);
			printSettingExist = true;
		}
		else if (!strcmp(nm, "Backup"))
		{
			element->SetAttribute("action", _nppGUI._backup);
			element->SetAttribute("useCustumDir", _nppGUI._useDir?"yes":"no");
			element->SetAttribute("dir", _nppGUI._backupDir);
			backExist = true;
		}
		else if (!strcmp(nm, "MRU"))
		{
			MRUExist = true;
			const char *pStr = _nppGUI._styleMRU?"yes":"no";
			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "URL"))
		{
			URLExist = true;
			const char *pStr = "0";
			if (_nppGUI._styleURL == 1)
				pStr = "1";
			else if (_nppGUI._styleURL == 2)
				pStr = "2";

			(childNode->FirstChild())->SetValue(pStr);
		}
		else if (!strcmp(nm, "DockingManager"))
		{
			dockingParamNode = childNode;
		}
	}

	if (!autoDetectionExist)
	{
		//insertGUIConfigBoolNode(GUIRoot, "Auto-detection", _nppGUI._fileAutoDetection);
		const char *pStr = _nppGUI._fileAutoDetection==0?"no":(_nppGUI._fileAutoDetection==1?"yes":"auto");
		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "Auto-detection");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}
	if (!checkHistoryFilesExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "CheckHistoryFiles", _nppGUI._checkHistoryFiles);
	}
	if (!trayIconExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "TrayIcon", _nppGUI._isMinimizedToTray);
	}
	
	if (!maitainIndentExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "MaitainIndent", _nppGUI._maitainIndent);
	}

	if (!rememberLastSessionExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "RememberLastSession", _nppGUI._rememberLastSession);
	}

	if (!newDocDefaultSettingsExist)
	{
		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "NewDocDefaultSettings");
		GUIConfigElement->SetAttribute("format", _nppGUI._newDocDefaultSettings._format);
		GUIConfigElement->SetAttribute("encoding", _nppGUI._newDocDefaultSettings._encoding);
		GUIConfigElement->SetAttribute("lang", _nppGUI._newDocDefaultSettings._lang);
	}

	if (!langsExcludedLstExist)
	{
		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "langsExcluded");
		writeExcludedLangList(GUIConfigElement);
	}
	if (!printSettingExist)
	{
		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "Print");
		writePrintSetting(GUIConfigElement);
	}
	if (!backExist)
	{
		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "Backup");
		GUIConfigElement->SetAttribute("action", _nppGUI._backup);
		GUIConfigElement->SetAttribute("useCustumDir", _nppGUI._useDir?"yes":"no");
		GUIConfigElement->SetAttribute("dir", _nppGUI._backupDir);
	}

	if (!doTaskListExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "TaskList", _nppGUI._doTaskList);
	}

	if (!MRUExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "MRU", _nppGUI._styleMRU);
	}

	if (!URLExist)
	{
		const char *pStr = "0";
		if (_nppGUI._styleURL == 1)
			pStr = "1";
		else if (_nppGUI._styleURL == 2)
			pStr = "2";

		TiXmlElement *GUIConfigElement = (GUIRoot->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute("name", "URL");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	if (!saveOpenFileInSameDirExist)
	{
		insertGUIConfigBoolNode(GUIRoot, "SaveOpenFileInSameDir", _nppGUI._saveOpenKeepInSameDir);
	}
	
	if (dockingParamNode)
	{
		// Rase tout
		GUIRoot->RemoveChild(dockingParamNode);
	}
	insertDockingParamNode(GUIRoot);

}

void NppParameters::insertDockingParamNode(TiXmlNode *GUIRoot)
{
	TiXmlElement DMNode("GUIConfig");
	DMNode.SetAttribute("name", "DockingManager");
	DMNode.SetAttribute("leftWidth", _nppGUI._dockingData._leftWidth);
	DMNode.SetAttribute("rightWidth", _nppGUI._dockingData._rightWidth);
	DMNode.SetAttribute("topHeight", _nppGUI._dockingData._topHeight);
	DMNode.SetAttribute("bottomHeight", _nppGUI._dockingData._bottomHight);
	
	for (size_t i = 0 ; i < _nppGUI._dockingData._flaotingWindowInfo.size() ; i++)
	{
		FloatingWindowInfo & fwi = _nppGUI._dockingData._flaotingWindowInfo[i];
		TiXmlElement FWNode("FloatingWindow");
		FWNode.SetAttribute("cont", fwi._cont);
		FWNode.SetAttribute("x", fwi._pos.left);
		FWNode.SetAttribute("y", fwi._pos.top);
		FWNode.SetAttribute("width", fwi._pos.right);
		FWNode.SetAttribute("height", fwi._pos.bottom);

		DMNode.InsertEndChild(FWNode);
	}

	for (size_t i = 0 ; i < _nppGUI._dockingData._pluginDockInfo.size() ; i++)
	{
		PlugingDlgDockingInfo & pdi = _nppGUI._dockingData._pluginDockInfo[i];
		TiXmlElement PDNode("PluginDlg");
		PDNode.SetAttribute("pluginName", pdi._name);
		PDNode.SetAttribute("id", pdi._internalID);
		PDNode.SetAttribute("curr", pdi._currContainer);
		PDNode.SetAttribute("prev", pdi._prevContainer);
		PDNode.SetAttribute("isVisible", pdi._isVisible?"yes":"no");

		DMNode.InsertEndChild(PDNode);
	}//printStr("in writeGUIParams");

	for (size_t i = 0 ; i < _nppGUI._dockingData._containerTabInfo.size() ; i++)
	{
		ContainerTabInfo & cti = _nppGUI._dockingData._containerTabInfo[i];
		TiXmlElement CTNode("ActiveTabs");
		CTNode.SetAttribute("cont", cti._cont);
		CTNode.SetAttribute("activeTab", cti._activeTab);
		DMNode.InsertEndChild(CTNode);
	}

	GUIRoot->InsertEndChild(DMNode);
}

void NppParameters::writePrintSetting(TiXmlElement *element)
{
	const char *pStr = _nppGUI._printSettings._printLineNumber?"yes":"no";
	element->SetAttribute("lineNumber", pStr);

	element->SetAttribute("printOption", _nppGUI._printSettings._printOption);

	element->SetAttribute("headerLeft", _nppGUI._printSettings._headerLeft.c_str());
	element->SetAttribute("headerMiddle", _nppGUI._printSettings._headerMiddle.c_str());
	element->SetAttribute("headerRight", _nppGUI._printSettings._headerRight.c_str());
	element->SetAttribute("footerLeft", _nppGUI._printSettings._footerLeft.c_str());
	element->SetAttribute("footerMiddle", _nppGUI._printSettings._footerMiddle.c_str());
	element->SetAttribute("footerRight", _nppGUI._printSettings._footerRight.c_str());

	element->SetAttribute("headerFontName", _nppGUI._printSettings._headerFontName.c_str());
	element->SetAttribute("headerFontStyle", _nppGUI._printSettings._headerFontStyle);
	element->SetAttribute("headerFontSize", _nppGUI._printSettings._headerFontSize);
	element->SetAttribute("footerFontName", _nppGUI._printSettings._footerFontName.c_str());
	element->SetAttribute("footerFontStyle", _nppGUI._printSettings._footerFontStyle);
	element->SetAttribute("footerFontSize", _nppGUI._printSettings._footerFontSize);

	element->SetAttribute("margeLeft", _nppGUI._printSettings._marge.left);
	element->SetAttribute("margeRight", _nppGUI._printSettings._marge.right);
	element->SetAttribute("margeTop", _nppGUI._printSettings._marge.top);
	element->SetAttribute("margeBottom", _nppGUI._printSettings._marge.bottom);
}

void NppParameters::writeExcludedLangList(TiXmlElement *element)
{
	int g0 = 0; // up to  8
	int g1 = 0; // up to 16
	int g2 = 0; // up to 24
	int g3 = 0; // up to 32
	int g4 = 0; // up to 40
	int g5 = 0; // up to 48
	int g6 = 0; // up to 56
	int g7 = 0; // up to 64

	for (size_t i = 0 ; i < _nppGUI._excludedLangList.size() ; i++)
	{
		int nGrp = _nppGUI._excludedLangList[i]._langType / 8;
		int nMask = 1 << _nppGUI._excludedLangList[i]._langType % 8;

		switch (nGrp)
		{
			case 0 :
				g0 |= nMask;
				break;
			case 1 :
				g1 |= nMask;
				break;
			case 2 :
				g2 |= nMask;
				break;
			case 3 :
				g3 |= nMask;
				break;
			case 4 :
				g4 |= nMask;
				break;
			case 5 :
				g5 |= nMask;
				break;
			case 6 :
				g6 |= nMask;
				break;
			case 7 :
				g7 |= nMask;
				break;
		}
	}

	element->SetAttribute("gr0", g0);
	element->SetAttribute("gr1", g1);
	element->SetAttribute("gr2", g2);
	element->SetAttribute("gr3", g3);
	element->SetAttribute("gr4", g4);
	element->SetAttribute("gr5", g5);
	element->SetAttribute("gr6", g6);
	element->SetAttribute("gr7", g7);
}

void NppParameters::insertGUIConfigBoolNode(TiXmlNode *r2w, const char *name, bool bVal)
{
	const char *pStr = bVal?"yes":"no";
	TiXmlElement *GUIConfigElement = (r2w->InsertEndChild(TiXmlElement("GUIConfig")))->ToElement();
	GUIConfigElement->SetAttribute("name", name);
	GUIConfigElement->InsertEndChild(TiXmlText(pStr));
}

int RGB2int(COLORREF color) {
    return (((((DWORD)color) & 0x0000FF) << 16) | ((((DWORD)color) & 0x00FF00)) | ((((DWORD)color) & 0xFF0000) >> 16));
}

int NppParameters::langTypeToCommandID(LangType lt) const 
{
	int id;
	switch (lt)
	{
		case L_C :	
			id = IDM_LANG_C; break;
		case L_CPP :
			id = IDM_LANG_CPP; break;
		case L_JAVA :
			id = IDM_LANG_JAVA;	break;
		case L_CS :
			id = IDM_LANG_CS; break;
		case L_OBJC :
			id = IDM_LANG_OBJC;	break;
		case L_HTML :
			id = IDM_LANG_HTML;	break;
		case L_XML : 
			id = IDM_LANG_XML; break;
		case L_JS :
			id = IDM_LANG_JS; break;
		case L_PHP :
			id = IDM_LANG_PHP; break;
		case L_ASP :
			id = IDM_LANG_ASP; break;
		case L_CSS :
			id = IDM_LANG_CSS; break;
		case L_LUA :
			id = IDM_LANG_LUA; break;
		case L_PERL :
			id = IDM_LANG_PERL; break;
		case L_PYTHON :
			id = IDM_LANG_PYTHON; break;
		case L_BATCH :
			id = IDM_LANG_BATCH; break;
		case L_PASCAL :
			id = IDM_LANG_PASCAL; break;
		case L_MAKEFILE :
			id = IDM_LANG_MAKEFILE;	break;
		case L_INI :
			id = IDM_LANG_INI; break;
		case L_NFO :
			id = IDM_LANG_ASCII; break;
		case L_RC :
			id = IDM_LANG_RC; break;
		case L_TEX :
			id = IDM_LANG_TEX; break;
		case L_FORTRAN : 
			id = IDM_LANG_FORTRAN; break;
		case L_BASH : 
			id = IDM_LANG_SH; break;
		case L_FLASH :
			id = IDM_LANG_FLASH; break;
		case L_NSIS :
			id = IDM_LANG_NSIS; break;
		case L_USER :
			id = IDM_LANG_USER; break;
        case L_SQL :
            id = IDM_LANG_SQL; break;
        case L_VB :
            id = IDM_LANG_VB; break;
		case L_TCL :
            id = IDM_LANG_TCL; break;

		case L_LISP :
            id = IDM_LANG_LISP; break;
		case L_SCHEME :
            id = IDM_LANG_SCHEME; break;
		case L_ASM :
            id = IDM_LANG_ASM; break;
		case L_DIFF :
            id = IDM_LANG_DIFF; break;
		case L_PROPS :
            id = IDM_LANG_PROPS; break;
		case L_PS :
            id = IDM_LANG_PS; break;
		case L_RUBY :
            id = IDM_LANG_RUBY; break;
		case L_SMALLTALK :
            id = IDM_LANG_SMALLTALK; break;
		case L_VHDL :
            id = IDM_LANG_VHDL; break;

		case L_ADA :
            id = IDM_LANG_ADA; break;
		case L_MATLAB :
            id = IDM_LANG_MATLAB; break;

		case L_HASKELL :
            id = IDM_LANG_HASKELL; break;

		case L_KIX :
            id = IDM_LANG_KIX; break;
		case L_AU3 :
            id = IDM_LANG_AU3; break;
		case L_VERILOG :
            id = IDM_LANG_VERILOG; break;
		case L_CAML :
            id = IDM_LANG_CAML; break;

		case L_INNO :
            id = IDM_LANG_INNO; break;

		case L_CMAKE :
            id = IDM_LANG_CMAKE; break;

		case L_SEARCHRESULT :
			id = -1;	break;

		case L_TXT :
			id = IDM_LANG_TEXT;	break;
		default :
			id = IDM_LANG_TEXT;
	}
	return id;
}

void NppParameters::writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers)
{
	TiXmlNode *lexersRoot = (_pXmlUserStylerDoc->FirstChild("NotepadPlus"))->FirstChildElement("LexerStyles");
	for (TiXmlNode *childNode = lexersRoot->FirstChildElement("LexerType");
		childNode ;
		childNode = childNode->NextSibling("LexerType"))
	{
		TiXmlElement *element = childNode->ToElement();
		const char *nm = element->Attribute("name");
		
		LexerStyler *pLs = _lexerStylerArray.getLexerStylerByName(nm);
        LexerStyler *pLs2 = lexersStylers.getLexerStylerByName(nm);

		const char *extStr = pLs->getLexerUserExt();
		//pLs2->setLexerUserExt(extStr);
	    element->SetAttribute("ext", extStr);


		if (pLs) 
		{
			for (TiXmlNode *grChildNode = childNode->FirstChildElement("WordsStyle");
					grChildNode ;
					grChildNode = grChildNode->NextSibling("WordsStyle"))
			{
				TiXmlElement *grElement = grChildNode->ToElement();
                const char *styleName = grElement->Attribute("name");

                int i = pLs->getStylerIndexByName(styleName);
                if (i != -1)
				{
                    Style & style = pLs->getStyler(i);
                    Style & style2Sync = pLs2->getStyler(i);

                    writeStyle2Element(style, style2Sync, grElement);
                }
			}
		}
	}

	TiXmlNode *globalStylesRoot = (_pXmlUserStylerDoc->FirstChild("NotepadPlus"))->FirstChildElement("GlobalStyles");

    for (TiXmlNode *childNode = globalStylesRoot->FirstChildElement("WidgetStyle");
		childNode ;
		childNode = childNode->NextSibling("WidgetStyle"))
    {
        TiXmlElement *pElement = childNode->ToElement();
        const char *styleName = pElement->Attribute("name");
        int i = _widgetStyleArray.getStylerIndexByName(styleName);

        if (i != -1)
		{
            Style & style = _widgetStyleArray.getStyler(i);
            Style & style2Sync = globalStylers.getStyler(i);

            writeStyle2Element(style, style2Sync, pElement);
        }
    }

	_pXmlUserStylerDoc->SaveFile();
}

void NppParameters::writeStyle2Element(Style & style2Wite, Style & style2Sync, TiXmlElement *element)
{
    const char *styleName = element->Attribute("name");

    if (HIBYTE(HIWORD(style2Wite._fgColor)) != 0xFF)
    {
        int rgbVal = RGB2int(style2Wite._fgColor);
	    char fgStr[7];
	    sprintf(fgStr, "%.6X", rgbVal);
	    element->SetAttribute("fgColor", fgStr);
    }

    if (HIBYTE(HIWORD(style2Wite._bgColor)) != 0xFF)
    {
        int rgbVal = RGB2int(style2Wite._bgColor);
	    char bgStr[7];
	    sprintf(bgStr, "%.6X", rgbVal);
	    element->SetAttribute("bgColor", bgStr);
    }

    if (style2Wite._fontName)
    {
        const char *oldFontName = element->Attribute("fontName");
        if (strcmp(oldFontName, style2Wite._fontName))
        {
		    element->SetAttribute("fontName", style2Wite._fontName);
            style2Sync._fontName = style2Wite._fontName = element->Attribute("fontName");
        }
    }

    if (style2Wite._fontSize != -1)
    {
        if (!style2Wite._fontSize)
            element->SetAttribute("fontSize", "");
        else
		    element->SetAttribute("fontSize", style2Wite._fontSize);
    }

    if (style2Wite._fontStyle != -1)
    {
	    element->SetAttribute("fontStyle", style2Wite._fontStyle);
    }

	
	if (style2Wite._keywords)
    {	
		TiXmlNode *teteDeNoeud = element->LastChild();

		if (teteDeNoeud)
			teteDeNoeud->SetValue(style2Wite._keywords->c_str());
		else 
			element->InsertEndChild(TiXmlText(style2Wite._keywords->c_str()));
    }
}

void NppParameters::insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang) 
{
	TiXmlElement *rootElement = (node->InsertEndChild(TiXmlElement("UserLang")))->ToElement();

	rootElement->SetAttribute("name", userLang->_name);
	rootElement->SetAttribute("ext", userLang->_ext);
	TiXmlElement *settingsElement = (rootElement->InsertEndChild(TiXmlElement("Settings")))->ToElement();
	{
		TiXmlElement *globalElement = (settingsElement->InsertEndChild(TiXmlElement("Global")))->ToElement();
		globalElement->SetAttribute("caseIgnored", userLang->_isCaseIgnored?"yes":"no");

		TiXmlElement *treatAsSymbolElement = (settingsElement->InsertEndChild(TiXmlElement("TreatAsSymbol")))->ToElement();
		treatAsSymbolElement->SetAttribute("comment", userLang->_isCommentSymbol?"yes":"no");
		treatAsSymbolElement->SetAttribute("commentLine", userLang->_isCommentLineSymbol?"yes":"no");

		TiXmlElement *prefixElement = (settingsElement->InsertEndChild(TiXmlElement("Prefix")))->ToElement();
		char names[nbPrefixListAllowed][7] = {"words1","words2","words3","words4"};
		for (int i = 0 ; i < nbPrefixListAllowed ; i++)
			prefixElement->SetAttribute(names[i], userLang->_isPrefix[i]?"yes":"no");
	}

	TiXmlElement *kwlElement = (rootElement->InsertEndChild(TiXmlElement("KeywordLists")))->ToElement();

	const int nbKWL = 9;
	char kwn[nbKWL][16] = {"Delimiters", "Folder+","Folder-","Operators","Comment","Words1","Words2","Words3","Words4"};
	for (int i = 0 ; i < nbKWL ; i++)
	{
		TiXmlElement *kwElement = (kwlElement->InsertEndChild(TiXmlElement("Keywords")))->ToElement();
		kwElement->SetAttribute("name", kwn[i]);
		kwElement->InsertEndChild(TiXmlText(userLang->_keywordLists[i]));
	}

	TiXmlElement *styleRootElement = (rootElement->InsertEndChild(TiXmlElement("Styles")))->ToElement();

	for (int i = 0 ; i < userLang->_styleArray.getNbStyler() ; i++)
	{
		TiXmlElement *styleElement = (styleRootElement->InsertEndChild(TiXmlElement("WordsStyle")))->ToElement();
		Style style2Wite = userLang->_styleArray.getStyler(i);

		styleElement->SetAttribute("name", style2Wite._styleDesc);

		styleElement->SetAttribute("styleID", style2Wite._styleID);

		//if (HIBYTE(HIWORD(style2Wite._fgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Wite._fgColor);
			char fgStr[7];
			sprintf(fgStr, "%.6X", rgbVal);
			styleElement->SetAttribute("fgColor", fgStr);
		}

		//if (HIBYTE(HIWORD(style2Wite._bgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Wite._bgColor);
			char bgStr[7];
			sprintf(bgStr, "%.6X", rgbVal);
			styleElement->SetAttribute("bgColor", bgStr);
		}

		if (style2Wite._fontName)
		{
			styleElement->SetAttribute("fontName", style2Wite._fontName);
		}

		if (style2Wite._fontStyle == -1)
		{
			styleElement->SetAttribute("fontStyle", "0");
		}
		else
		{
			styleElement->SetAttribute("fontStyle", style2Wite._fontStyle);
		}

		if (style2Wite._fontSize != -1)
		{
			if (!style2Wite._fontSize)
				styleElement->SetAttribute("fontSize", "");
			else
				styleElement->SetAttribute("fontSize", style2Wite._fontSize);
		}
	}
}

void NppParameters::stylerStrOp(bool op) 
{
	for (int i = 0 ; i < _nbUserLang ; i++)
	{
		int nbStyler = _userLangArray[i]->_styleArray.getNbStyler();
		for (int j = 0 ; j < nbStyler ; j++)
		{
			Style & style = _userLangArray[i]->_styleArray.getStyler(j);
			
			if (op == DUP)
			{
				char *str = new char[strlen(style._styleDesc) + 1];
				style._styleDesc = strcpy(str, style._styleDesc);
				if (style._fontName)
				{
					str = new char[strlen(style._fontName) + 1];
					style._fontName = strcpy(str, style._fontName);
				}
				else
				{
					str = new char[2];
					str[0] = str[1] = '\0';
					style._fontName = str;
				}
			}
			else
			{
				delete [] style._styleDesc;
				delete [] style._fontName;
			}
		}
	}
}

