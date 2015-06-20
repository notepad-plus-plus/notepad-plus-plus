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

#include <time.h>
#include <shlwapi.h>
#include "Notepad_plus.h"
#include "Notepad_plus_Window.h"
#include "FileDialog.h"
#include "printer.h"
#include "FileNameStringSplitter.h"
#include "lesDlgs.h"
#include "Utf8_16.h"
#include "regExtDlg.h"
#include "RunDlg.h"
#include "ShortcutMapper.h"
#include "preferenceDlg.h"
#include "TaskListDlg.h"
#include "xmlMatchedTagsHighlighter.h"
#include "EncodingMapper.h"
#include "ansiCharPanel.h"
#include "clipboardHistoryPanel.h"
#include "VerticalFileSwitcher.h"
#include "ProjectPanel.h"
#include "documentMap.h"
#include "functionListPanel.h"

using namespace std;

enum tb_stat {tb_saved, tb_unsaved, tb_ro};
#define DIR_LEFT true
#define DIR_RIGHT false

#define NPP_INTERNAL_FUCTION_STR TEXT("Notepad++::InternalFunction")

int docTabIconIDs[] = {IDI_SAVED_ICON, IDI_UNSAVED_ICON, IDI_READONLY_ICON};

ToolBarButtonUnit toolBarIcons[] = {
	{IDM_FILE_NEW,		IDI_NEW_OFF_ICON,		IDI_NEW_ON_ICON,		IDI_NEW_OFF_ICON, IDR_FILENEW},
	{IDM_FILE_OPEN,		IDI_OPEN_OFF_ICON,		IDI_OPEN_ON_ICON,		IDI_OPEN_OFF_ICON, IDR_FILEOPEN},
	{IDM_FILE_SAVE,		IDI_SAVE_OFF_ICON,		IDI_SAVE_ON_ICON,		IDI_SAVE_DISABLE_ICON, IDR_FILESAVE},
	{IDM_FILE_SAVEALL,	IDI_SAVEALL_OFF_ICON,	IDI_SAVEALL_ON_ICON,	IDI_SAVEALL_DISABLE_ICON, IDR_SAVEALL},
	{IDM_FILE_CLOSE,	IDI_CLOSE_OFF_ICON,		IDI_CLOSE_ON_ICON,		IDI_CLOSE_OFF_ICON, IDR_CLOSEFILE},
	{IDM_FILE_CLOSEALL,	IDI_CLOSEALL_OFF_ICON,	IDI_CLOSEALL_ON_ICON,	IDI_CLOSEALL_OFF_ICON, IDR_CLOSEALL},
	{IDM_FILE_PRINTNOW,	IDI_PRINT_OFF_ICON,		IDI_PRINT_ON_ICON,		IDI_PRINT_OFF_ICON, IDR_PRINT},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_EDIT_CUT,		IDI_CUT_OFF_ICON,		IDI_CUT_ON_ICON,		IDI_CUT_DISABLE_ICON, IDR_CUT},
	{IDM_EDIT_COPY,		IDI_COPY_OFF_ICON,		IDI_COPY_ON_ICON,		IDI_COPY_DISABLE_ICON, IDR_COPY},
	{IDM_EDIT_PASTE,	IDI_PASTE_OFF_ICON,		IDI_PASTE_ON_ICON,		IDI_PASTE_DISABLE_ICON, IDR_PASTE},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_EDIT_UNDO,		IDI_UNDO_OFF_ICON,		IDI_UNDO_ON_ICON,		IDI_UNDO_DISABLE_ICON, IDR_UNDO},
	{IDM_EDIT_REDO,		IDI_REDO_OFF_ICON,		IDI_REDO_ON_ICON,		IDI_REDO_DISABLE_ICON, IDR_REDO},
	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_SEARCH_FIND,		IDI_FIND_OFF_ICON,		IDI_FIND_ON_ICON,		IDI_FIND_OFF_ICON, IDR_FIND},
	{IDM_SEARCH_REPLACE,  IDI_REPLACE_OFF_ICON,	IDI_REPLACE_ON_ICON,	IDI_REPLACE_OFF_ICON, IDR_REPLACE},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_ZOOMIN,	IDI_ZOOMIN_OFF_ICON,	IDI_ZOOMIN_ON_ICON,		IDI_ZOOMIN_OFF_ICON, IDR_ZOOMIN},
	{IDM_VIEW_ZOOMOUT,	IDI_ZOOMOUT_OFF_ICON,	IDI_ZOOMOUT_ON_ICON,	IDI_ZOOMOUT_OFF_ICON, IDR_ZOOMOUT},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_SYNSCROLLV,	IDI_SYNCV_OFF_ICON,	IDI_SYNCV_ON_ICON,	IDI_SYNCV_DISABLE_ICON, IDR_SYNCV},
	{IDM_VIEW_SYNSCROLLH,	IDI_SYNCH_OFF_ICON,	IDI_SYNCH_ON_ICON,	IDI_SYNCH_DISABLE_ICON, IDR_SYNCH},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_WRAP,  IDI_VIEW_WRAP_OFF_ICON,	IDI_VIEW_WRAP_ON_ICON,	IDI_VIEW_WRAP_OFF_ICON, IDR_WRAP},
	{IDM_VIEW_ALL_CHARACTERS,  IDI_VIEW_ALL_CHAR_OFF_ICON,	IDI_VIEW_ALL_CHAR_ON_ICON,	IDI_VIEW_ALL_CHAR_OFF_ICON, IDR_INVISIBLECHAR},
	{IDM_VIEW_INDENT_GUIDE,  IDI_VIEW_INDENT_OFF_ICON,	IDI_VIEW_INDENT_ON_ICON,	IDI_VIEW_INDENT_OFF_ICON, IDR_INDENTGUIDE},
	{IDM_LANG_USER_DLG,  IDI_VIEW_UD_DLG_OFF_ICON,	IDI_VIEW_UD_DLG_ON_ICON,	IDI_VIEW_UD_DLG_OFF_ICON, IDR_SHOWPANNEL},
	{IDM_VIEW_DOC_MAP,  IDI_VIEW_UD_DLG_OFF_ICON, IDI_VIEW_UD_DLG_ON_ICON, IDI_VIEW_UD_DLG_OFF_ICON, IDR_DOCMAP},
	{IDM_VIEW_FUNC_LIST,  IDI_VIEW_UD_DLG_OFF_ICON, IDI_VIEW_UD_DLG_ON_ICON, IDI_VIEW_UD_DLG_OFF_ICON, IDR_FUNC_LIST},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_MACRO_STARTRECORDINGMACRO,		IDI_STARTRECORD_OFF_ICON,	IDI_STARTRECORD_ON_ICON,	IDI_STARTRECORD_DISABLE_ICON, IDR_STARTRECORD},
	{IDM_MACRO_STOPRECORDINGMACRO,		IDI_STOPRECORD_OFF_ICON,	IDI_STOPRECORD_ON_ICON,		IDI_STOPRECORD_DISABLE_ICON, IDR_STOPRECORD},
	{IDM_MACRO_PLAYBACKRECORDEDMACRO,	IDI_PLAYRECORD_OFF_ICON,	IDI_PLAYRECORD_ON_ICON,		IDI_PLAYRECORD_DISABLE_ICON, IDR_PLAYRECORD},
	{IDM_MACRO_RUNMULTIMACRODLG,			IDI_MMPLAY_OFF_ICON,		IDI_MMPLAY_ON_ICON,			IDI_MMPLAY_DIS_ICON, IDR_M_PLAYRECORD},
	{IDM_MACRO_SAVECURRENTMACRO,			IDI_SAVERECORD_OFF_ICON,	IDI_SAVERECORD_ON_ICON,		IDI_SAVERECORD_DISABLE_ICON, IDR_SAVERECORD}

};


Notepad_plus::Notepad_plus(): _mainWindowStatus(0), _pDocTab(NULL), _pEditView(NULL),
	_pMainSplitter(NULL),
    _recordingMacro(false), _pTrayIco(NULL), _isUDDocked(false), _pFileSwitcherPanel(NULL),
	_pProjectPanel_1(NULL), _pProjectPanel_2(NULL), _pProjectPanel_3(NULL), _pDocMap(NULL), _pFuncList(NULL),
	_linkTriggered(true), _isHotspotDblClicked(false), _isFolding(false), 
	_sysMenuEntering(false),
	_autoCompleteMain(&_mainEditView), _autoCompleteSub(&_subEditView), _smartHighlighter(&_findReplaceDlg),
	_isFileOpening(false), _pAnsiCharPanel(NULL), _pClipboardHistoryPanel(NULL)
{
	ZeroMemory(&_prevSelectedRange, sizeof(_prevSelectedRange));


	TiXmlDocumentA *nativeLangDocRootA = (NppParameters::getInstance())->getNativeLangA();
    _nativeLangSpeaker.init(nativeLangDocRootA);

	LocalizationSwitcher & localizationSwitcher = (NppParameters::getInstance())->getLocalizationSwitcher();
    const char *fn = _nativeLangSpeaker.getFileName();
    if (fn)
    {
        localizationSwitcher.setFileName(fn);
    }

	(NppParameters::getInstance())->setNativeLangSpeaker(&_nativeLangSpeaker);

	TiXmlDocument *toolIconsDocRoot = (NppParameters::getInstance())->getToolIcons();

	if (toolIconsDocRoot)
	{
        _toolBar.initTheme(toolIconsDocRoot);
    }

	// Determine if user is administrator.
	BOOL is_admin;
	winVer ver = NppParameters::getInstance()->getWinVersion();
	if (ver >= WV_VISTA || ver == WV_UNKNOWN)
	{
		SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
		PSID AdministratorsGroup;
		is_admin = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
		if(is_admin)
		{
			if(!CheckTokenMembership(NULL, AdministratorsGroup, &is_admin))
				is_admin = FALSE;
			FreeSid(AdministratorsGroup);
		}
	}
	else
		is_admin = false;

	_isAdministrator = is_admin ? true : false;
}

// ATTENTION : the order of the destruction is very important
// because if the parent's window handle is destroyed before
// the destruction of its children windows' handles,
// its children windows' handles will be destroyed automatically!
Notepad_plus::~Notepad_plus()
{
	(NppParameters::getInstance())->destroyInstance();
	MainFileManager->destroyInstance();
	(WcharMbcsConvertor::getInstance())->destroyInstance();
	if (_pTrayIco)
		delete _pTrayIco;

	if (_pAnsiCharPanel)
		delete _pAnsiCharPanel;

	if (_pClipboardHistoryPanel)
		delete _pClipboardHistoryPanel;

	if (_pFileSwitcherPanel)
		delete _pFileSwitcherPanel;

	if (_pProjectPanel_1)
	{
		delete _pProjectPanel_1;
	}
	if (_pProjectPanel_2)
	{
		delete _pProjectPanel_2;
	}
	if (_pProjectPanel_3)
	{
		delete _pProjectPanel_3;
	}
	if (_pDocMap)
	{
		delete _pDocMap;
	}
	if (_pFuncList)
	{
		delete _pFuncList;
	}
}




LRESULT Notepad_plus::init(HWND hwnd)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();

	// Menu
	_mainMenuHandle = ::GetMenu(hwnd);
	int langPos2BeRemoved = MENUINDEX_LANGUAGE+1;
	if (nppGUI._isLangMenuCompact)
		langPos2BeRemoved = MENUINDEX_LANGUAGE;
	::RemoveMenu(_mainMenuHandle, langPos2BeRemoved, MF_BYPOSITION);

	//Views
    _pDocTab = &_mainDocTab;
    _pEditView = &_mainEditView;
	_pNonDocTab = &_subDocTab;
	_pNonEditView = &_subEditView;

	_mainEditView.init(_pPublicInterface->getHinst(), hwnd);
	_subEditView.init(_pPublicInterface->getHinst(), hwnd);

	_fileEditView.init(_pPublicInterface->getHinst(), hwnd);
	MainFileManager->init(this, &_fileEditView);	//get it up and running asap.

	pNppParam->setFontList(hwnd);


	_mainWindowStatus = WindowMainActive;
	_activeView = MAIN_VIEW;

    const ScintillaViewParams & svp1 = pNppParam->getSVP();

	int tabBarStatus = nppGUI._tabStatus;
	_toReduceTabBar = ((tabBarStatus & TAB_REDUCE) != 0);
	int iconDpiDynamicalSize = NppParameters::getInstance()->_dpiManager.scaleY(_toReduceTabBar?13:20);
	_docTabIconList.create(iconDpiDynamicalSize, _pPublicInterface->getHinst(), docTabIconIDs, sizeof(docTabIconIDs)/sizeof(int));

	_mainDocTab.init(_pPublicInterface->getHinst(), hwnd, &_mainEditView, &_docTabIconList);
	_subDocTab.init(_pPublicInterface->getHinst(), hwnd, &_subEditView, &_docTabIconList);

	_mainEditView.display();

	_invisibleEditView.init(_pPublicInterface->getHinst(), hwnd);
	_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
	_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);
	_invisibleEditView.wrap(false); // Make sure no slow down

	// Configuration of 2 scintilla views
    _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp1._lineNumberMarginShow);
	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp1._lineNumberMarginShow);
    _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp1._bookMarkMarginShow);
	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp1._bookMarkMarginShow);

    _mainEditView.showIndentGuideLine(svp1._indentGuideLineShow);
    _subEditView.showIndentGuideLine(svp1._indentGuideLineShow);

	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);

	_configStyleDlg.init(_pPublicInterface->getHinst(), hwnd);
	_preference.init(_pPublicInterface->getHinst(), hwnd);

    //Marker Margin config
    _mainEditView.setMakerStyle(svp1._folderStyle);
    _subEditView.setMakerStyle(svp1._folderStyle);
	_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
	_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());

	//Line wrap method
	_mainEditView.setWrapMode(svp1._lineWrapMethod);
    _subEditView.setWrapMode(svp1._lineWrapMethod);

	_mainEditView.execute(SCI_SETCARETLINEVISIBLE, svp1._currentLineHilitingShow);
	_subEditView.execute(SCI_SETCARETLINEVISIBLE, svp1._currentLineHilitingShow);

	_mainEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
	_subEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);

	_mainEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
	_subEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);

	_mainEditView.wrap(svp1._doWrap);
	_subEditView.wrap(svp1._doWrap);

	_mainEditView.execute(SCI_SETEDGECOLUMN, svp1._edgeNbColumn);
	_mainEditView.execute(SCI_SETEDGEMODE, svp1._edgeMode);
	_subEditView.execute(SCI_SETEDGECOLUMN, svp1._edgeNbColumn);
	_subEditView.execute(SCI_SETEDGEMODE, svp1._edgeMode);

	_mainEditView.showEOL(svp1._eolShow);
	_subEditView.showEOL(svp1._eolShow);

	_mainEditView.showWSAndTab(svp1._whiteSpaceShow);
	_subEditView.showWSAndTab(svp1._whiteSpaceShow);

	_mainEditView.showWrapSymbol(svp1._wrapSymbolShow);
	_subEditView.showWrapSymbol(svp1._wrapSymbolShow);

	_mainEditView.performGlobalStyles();
	_subEditView.performGlobalStyles();

	_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
	_mainEditView.execute(SCI_SETZOOM, svp1._zoom);
	_subEditView.execute(SCI_SETZOOM, svp1._zoom2);

    ::SendMessage(hwnd, NPPM_INTERNAL_SETMULTISELCTION, 0, 0);

	// Make backspace or delete work with multiple selections
	_mainEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);
	_subEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);

	// Turn virtual space on
	_mainEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);
	_subEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);

	// Turn multi-paste on
	_mainEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
	_subEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);

	TabBarPlus::doDragNDrop(true);

	if (_toReduceTabBar)
	{
		HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

		if (hf)
		{
			::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
			::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
		}
		int tabDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(20);
		int tabDpiDynamicalWidth = NppParameters::getInstance()->_dpiManager.scaleX(45);
		TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
		TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
	}
	_mainDocTab.display();


	TabBarPlus::doDragNDrop((tabBarStatus & TAB_DRAGNDROP) != 0);
	TabBarPlus::setDrawTopBar((tabBarStatus & TAB_DRAWTOPBAR) != 0);
	TabBarPlus::setDrawInactiveTab((tabBarStatus & TAB_DRAWINACTIVETAB) != 0);
	TabBarPlus::setDrawTabCloseButton((tabBarStatus & TAB_CLOSEBUTTON) != 0);
	TabBarPlus::setDbClk2Close((tabBarStatus & TAB_DBCLK2CLOSE) != 0);
	TabBarPlus::setVertical((tabBarStatus & TAB_VERTICAL) != 0);
	drawTabbarColoursFromStylerArray();

    //--Splitter Section--//
	bool isVertical = (nppGUI._splitterPos == POS_VERTICAL);

    _subSplitter.init(_pPublicInterface->getHinst(), hwnd);
    _subSplitter.create(&_mainDocTab, &_subDocTab, 8, DYNAMIC, 50, isVertical);

    //--Status Bar Section--//
	bool willBeShown = nppGUI._statusBarShow;
    _statusBar.init(_pPublicInterface->getHinst(), hwnd, 6);
	_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, 200);
	_statusBar.setPartWidth(STATUSBAR_CUR_POS, 260);
	_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, 110);
	_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, 120);
	_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, 30);
    _statusBar.display(willBeShown);

    _pMainWindow = &_mainDocTab;

	_dockingManager.init(_pPublicInterface->getHinst(), hwnd, &_pMainWindow);

	if (nppGUI._isMinimizedToTray && _pTrayIco == NULL)
		_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

	checkSyncState();

	// Plugin Manager
	NppData nppData;
	nppData._nppHandle = hwnd;
	nppData._scintillaMainHandle = _mainEditView.getHSelf();
	nppData._scintillaSecondHandle = _subEditView.getHSelf();

	_scintillaCtrls4Plugins.init(_pPublicInterface->getHinst(), hwnd);
	_pluginsManager.init(nppData);

	// Load plugins firstly from "%APPDATA%/Notepad++/plugins" 
	// if Notepad++ is not in localConf mode. 
	// All the dll loaded are marked.
	bool isLoadFromAppDataAllow = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETAPPDATAPLUGINSALLOWED, 0, 0) == TRUE;
	const TCHAR *appDataNpp = pNppParam->getAppDataNppDir();
	if (appDataNpp[0] && isLoadFromAppDataAllow)
		_pluginsManager.loadPlugins(appDataNpp);

	// Load plugins from its installation directory.
	// All loaded dll will be ignored
	_pluginsManager.loadPlugins();
	

    _restoreButton.init(_pPublicInterface->getHinst(), hwnd);


	// ------------ //
	// Menu Section //
	// ------------ //

	// Macro Menu
	std::vector<MacroShortcut> & macros  = pNppParam->getMacroList();
	HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
	size_t const posBase = 6;
	size_t nbMacro = macros.size();
	if (nbMacro >= 1)
		::InsertMenu(hMacroMenu, posBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);

	for (size_t i = 0 ; i < nbMacro ; ++i)
	{
		::InsertMenu(hMacroMenu, posBase + i, MF_BYPOSITION, ID_MACRO + i, macros[i].toMenuItemString().c_str());
	}

    if (nbMacro >= 1)
    {
        ::InsertMenu(hMacroMenu, posBase + nbMacro + 1, MF_BYPOSITION, (unsigned int)-1, 0);
        ::InsertMenu(hMacroMenu, posBase + nbMacro + 2, MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_MACRO, TEXT("Modify Shortcut/Delete Macro..."));
    }
	// Run Menu
	std::vector<UserCommand> & userCommands = pNppParam->getUserCommandList();
	HMENU hRunMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_RUN);
	int const runPosBase = 2;
	size_t nbUserCommand = userCommands.size();
	if (nbUserCommand >= 1)
		::InsertMenu(hRunMenu, runPosBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);
	for (size_t i = 0 ; i < nbUserCommand ; ++i)
	{
		::InsertMenu(hRunMenu, runPosBase + i, MF_BYPOSITION, ID_USER_CMD + i, userCommands[i].toMenuItemString().c_str());
	}

    if (nbUserCommand >= 1)
    {
		::InsertMenu(hRunMenu, runPosBase + nbUserCommand + 1, MF_BYPOSITION, (unsigned int)-1, 0);
        ::InsertMenu(hRunMenu, runPosBase + nbUserCommand + 2, MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_RUN, TEXT("Modify Shortcut/Delete Command..."));
    }

	// Updater menu item
	if (!nppGUI._doesExistUpdater)
	{
		::DeleteMenu(_mainMenuHandle, IDM_UPDATE_NPP, MF_BYCOMMAND);
		::DeleteMenu(_mainMenuHandle, IDM_CONFUPDATERPROXY, MF_BYCOMMAND);
		HMENU hHelpMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_PLUGINS + 1);
		if (!hHelpMenu)
			hHelpMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_PLUGINS);
		if (hHelpMenu)
			::DeleteMenu(hHelpMenu, 7, MF_BYPOSITION); // SEPARATOR
		::DrawMenuBar(hwnd);
	}
	//Languages Menu
	HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);

	// Add external languages to menu
	for (int i = 0 ; i < pNppParam->getNbExternalLang() ; ++i)
	{
		ExternalLangContainer & externalLangContainer = pNppParam->getELCFromIndex(i);

		int numLangs = ::GetMenuItemCount(hLangMenu);
		const int bufferSize = 100;
		TCHAR buffer[bufferSize];

		int x;
		for(x = 0; (x == 0 || lstrcmp(externalLangContainer._name, buffer) > 0) && x < numLangs; ++x)
		{
			::GetMenuString(hLangMenu, x, buffer, bufferSize, MF_BYPOSITION);
		}

		::InsertMenu(hLangMenu, x-1, MF_BYPOSITION, IDM_LANG_EXTERNAL + i, externalLangContainer._name);
	}

	if (nppGUI._excludedLangList.size() > 0)
	{
		for (size_t i = 0, len = nppGUI._excludedLangList.size(); i < len ; ++i)
		{
			int cmdID = pNppParam->langTypeToCommandID(nppGUI._excludedLangList[i]._langType);
			const int itemSize = 256;
			TCHAR itemName[itemSize];
			::GetMenuString(hLangMenu, cmdID, itemName, itemSize, MF_BYCOMMAND);
			nppGUI._excludedLangList[i]._cmdID = cmdID;
			nppGUI._excludedLangList[i]._langName = itemName;
			::DeleteMenu(hLangMenu, cmdID, MF_BYCOMMAND);
			DrawMenuBar(hwnd);
		}
	}

	// Add User Define Languages Entry
	int udlpos = ::GetMenuItemCount(hLangMenu) - 1;

	for (int i = 0, len = pNppParam->getNbUserLang(); i < len ; ++i)
	{
		UserLangContainer & userLangContainer = pNppParam->getULCFromIndex(i);
		::InsertMenu(hLangMenu, udlpos + i, MF_BYPOSITION, IDM_LANG_USER + i + 1, userLangContainer.getName());
	}

	//Add recent files
	HMENU hFileMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FILE);
	int nbLRFile = pNppParam->getNbLRFile();
	//int pos = IDM_FILEMENU_LASTONE - IDM_FILE + 1 /* +1 : because of  IDM_FILE_PRINTNOW */;

	_lastRecentFileList.initMenu(hFileMenu, IDM_FILEMENU_LASTONE + 1, IDM_FILEMENU_EXISTCMDPOSITION, &_accelerator, pNppParam->putRecentFileInSubMenu());
	_lastRecentFileList.setLangEncoding(_nativeLangSpeaker.getLangEncoding());
	for (int i = 0 ; i < nbLRFile ; ++i)
	{
		generic_string * stdStr = pNppParam->getLRFile(i);
		if (!nppGUI._checkHistoryFiles || PathFileExists(stdStr->c_str()))
		{
			_lastRecentFileList.add(stdStr->c_str());
		}
	}

	//Plugin menu
	_pluginsManager.setMenu(_mainMenuHandle, NULL);

	//Main menu is loaded, now load context menu items
	pNppParam->getContextMenuFromXmlTree(_mainMenuHandle, _pluginsManager.getMenuHandle());

	if (pNppParam->hasCustomContextMenu())
	{
		_mainEditView.execute(SCI_USEPOPUP, FALSE);
		_subEditView.execute(SCI_USEPOPUP, FALSE);
	}

	generic_string pluginsTrans, windowTrans;
	_nativeLangSpeaker.changeMenuLang(_mainMenuHandle, pluginsTrans, windowTrans);
	::DrawMenuBar(hwnd);


	if (_pluginsManager.hasPlugins() && pluginsTrans != TEXT(""))
	{
		::ModifyMenu(_mainMenuHandle, MENUINDEX_PLUGINS, MF_BYPOSITION, 0, pluginsTrans.c_str());
	}
	//Windows menu
	_windowsMenu.init(_pPublicInterface->getHinst(), _mainMenuHandle, windowTrans.c_str());

	// Update context menu strings (translated)
	vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
	size_t len = tmp.size();
	TCHAR menuName[64];
	for (size_t i = 0 ; i < len ; ++i)
	{
		if (tmp[i]._itemName == TEXT(""))
		{
			::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
			tmp[i]._itemName = purgeMenuItemString(menuName);
		}
	}

	//Input all the menu item names into shortcut list
	//This will automatically do all translations, since menu translation has been done already
	vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
	len = shortcuts.size();

	for(size_t i = 0; i < len; ++i)
	{
		CommandShortcut & csc = shortcuts[i];
		if (!csc.getName()[0])
		{	//no predefined name, get name from menu and use that
			::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND);
			csc.setName(purgeMenuItemString(menuName, true).c_str());
		}
	}
	//Translate non-menu shortcuts
	_nativeLangSpeaker.changeShortcutLang();

	//Update plugin shortcuts, all plugin commands should be available now
	pNppParam->reloadPluginCmds();

	// Shortcut Accelerator : should be the last one since it will capture all the shortcuts
	_accelerator.init(_mainMenuHandle, hwnd);
	pNppParam->setAccelerator(&_accelerator);

	// Scintilla key accelerator
	vector<HWND> scints;
	scints.push_back(_mainEditView.getHSelf());
	scints.push_back(_subEditView.getHSelf());
	_scintaccelerator.init(&scints, _mainMenuHandle, hwnd);

	pNppParam->setScintillaAccelerator(&_scintaccelerator);
	_scintaccelerator.updateKeys();

	::DrawMenuBar(hwnd);


    //-- Tool Bar Section --//
	toolBarStatusType tbStatus = nppGUI._toolBarStatus;
	willBeShown = nppGUI._toolbarShow;

	// To notify plugins that toolbar icons can be registered
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_TBMODIFICATION;
	scnN.nmhdr.hwndFrom = hwnd;
	scnN.nmhdr.idFrom = 0;
	_pluginsManager.notify(&scnN);

	_toolBar.init(_pPublicInterface->getHinst(), hwnd, tbStatus, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit));

	changeToolBarIcons();

	_rebarTop.init(_pPublicInterface->getHinst(), hwnd);
	_rebarBottom.init(_pPublicInterface->getHinst(), hwnd);
	_toolBar.addToRebar(&_rebarTop);
	_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, willBeShown);

	//--Init dialogs--//
    _findReplaceDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_incrementFindDlg.init(_pPublicInterface->getHinst(), hwnd, &_findReplaceDlg, _nativeLangSpeaker.isRTL());
	_incrementFindDlg.addToRebar(&_rebarBottom);
    _goToLineDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_findCharsInRangeDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_colEditorDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
    _aboutDlg.init(_pPublicInterface->getHinst(), hwnd);
	_runDlg.init(_pPublicInterface->getHinst(), hwnd);
	_runMacroDlg.init(_pPublicInterface->getHinst(), hwnd);

    //--User Define Dialog Section--//
	int uddStatus = nppGUI._userDefineDlgStatus;
    UserDefineDialog *udd = _pEditView->getUserDefineDlg();

	bool uddShow = false;
	switch (uddStatus)
    {
        case UDD_SHOW :                 // show & undocked
			udd->doDialog(true, _nativeLangSpeaker.isRTL());
			_nativeLangSpeaker.changeUserDefineLang(udd);
			uddShow = true;
            break;
        case UDD_DOCKED : {              // hide & docked
			_isUDDocked = true;
            break;}
        case (UDD_SHOW | UDD_DOCKED) :    // show & docked
            udd->doDialog(true, _nativeLangSpeaker.isRTL());
			_nativeLangSpeaker.changeUserDefineLang(udd);
            ::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);
			uddShow = true;
            break;

		default :                        // hide & undocked
			break;
    }
    
	//
	// Menu & toolbar for UserDefine Dialog
	//
	checkMenuItem(IDM_LANG_USER_DLG, uddShow);
	_toolBar.setCheck(IDM_LANG_USER_DLG, uddShow);

	//
	// Initialize the default foreground & background color
	//
	StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();
	int i = globalStyles.getStylerIndexByID(STYLE_DEFAULT);
	if (i != -1)
	{
		Style & style = globalStyles.getStyler(i);
		(NppParameters::getInstance())->setCurrentDefaultFgColor(style._fgColor);
		(NppParameters::getInstance())->setCurrentDefaultBgColor(style._bgColor);
	}

	//
	// launch the plugin dlg memorized at the last session
	//
	DockingManagerData &dmd = nppGUI._dockingData;

	_dockingManager.setDockedContSize(CONT_LEFT  , nppGUI._dockingData._leftWidth);
	_dockingManager.setDockedContSize(CONT_RIGHT , nppGUI._dockingData._rightWidth);
	_dockingManager.setDockedContSize(CONT_TOP	 , nppGUI._dockingData._topHeight);
	_dockingManager.setDockedContSize(CONT_BOTTOM, nppGUI._dockingData._bottomHight);

	for (size_t i = 0, len = dmd._pluginDockInfo.size(); i < len ; ++i)
	{
		PluginDlgDockingInfo & pdi = dmd._pluginDockInfo[i];
		if (pdi._isVisible)
		{
			if (pdi._name == NPP_INTERNAL_FUCTION_STR)
			{
				_internalFuncIDs.push_back(pdi._internalID);
			}
			else
			{
				_pluginsManager.runPluginCommand(pdi._name.c_str(), pdi._internalID);
			}
		}
	}

	for (size_t i = 0, len = dmd._containerTabInfo.size(); i < len; ++i)
	{
		ContainerTabInfo & cti = dmd._containerTabInfo[i];
		_dockingManager.setActiveTab(cti._cont, cti._activeTab);
	}
	//Load initial docs into doctab
	loadBufferIntoView(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
	loadBufferIntoView(_subEditView.getCurrentBufferID(), SUB_VIEW);
	activateBuffer(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
	activateBuffer(_subEditView.getCurrentBufferID(), SUB_VIEW);
	//::SetFocus(_mainEditView.getHSelf());
	_mainEditView.getFocus();
	return TRUE;
}



void Notepad_plus::killAllChildren()
{
	_toolBar.destroy();
	_rebarTop.destroy();
	_rebarBottom.destroy();

    if (_pMainSplitter)
    {
        _pMainSplitter->destroy();
        delete _pMainSplitter;
    }

    _mainDocTab.destroy();
    _subDocTab.destroy();

	_mainEditView.destroy();
    _subEditView.destroy();
	_invisibleEditView.destroy();

    _subSplitter.destroy();
    _statusBar.destroy();

	_scintillaCtrls4Plugins.destroy();
	_dockingManager.destroy();
}

bool Notepad_plus::saveGUIParams()
{
	NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();
	nppGUI._toolbarShow = _rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
	nppGUI._toolBarStatus = _toolBar.getState();

	nppGUI._tabStatus = (TabBarPlus::doDragNDropOrNot()?TAB_DRAWTOPBAR:0) | \
						(TabBarPlus::drawTopBar()?TAB_DRAGNDROP:0) | \
						(TabBarPlus::drawInactiveTab()?TAB_DRAWINACTIVETAB:0) | \
						(_toReduceTabBar?TAB_REDUCE:0) | \
						(TabBarPlus::drawTabCloseButton()?TAB_CLOSEBUTTON:0) | \
						(TabBarPlus::isDbClk2Close()?TAB_DBCLK2CLOSE:0) | \
						(TabBarPlus::isVertical() ? TAB_VERTICAL:0) | \
						(TabBarPlus::isMultiLine() ? TAB_MULTILINE:0) |\
						(nppGUI._tabStatus & TAB_HIDE);
	nppGUI._splitterPos = _subSplitter.isVertical()?POS_VERTICAL:POS_HORIZOTAL;
	UserDefineDialog *udd = _pEditView->getUserDefineDlg();
	bool b = udd->isDocked();
	nppGUI._userDefineDlgStatus = (b?UDD_DOCKED:0) | (udd->isVisible()?UDD_SHOW:0);

	// When window is maximized GetWindowPlacement returns window's last non maximized coordinates.
	// Save them so that those will be used when window is restored next time.
	WINDOWPLACEMENT posInfo;
	posInfo.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(_pPublicInterface->getHSelf(), &posInfo);

	nppGUI._appPos.left   = posInfo.rcNormalPosition.left;
	nppGUI._appPos.top    = posInfo.rcNormalPosition.top;
	nppGUI._appPos.right  = posInfo.rcNormalPosition.right - posInfo.rcNormalPosition.left;
	nppGUI._appPos.bottom = posInfo.rcNormalPosition.bottom - posInfo.rcNormalPosition.top;
	nppGUI._isMaximized = ((IsZoomed(_pPublicInterface->getHSelf()) != 0) || (posInfo.flags & WPF_RESTORETOMAXIMIZED));
	
	saveDockingParams();

	return (NppParameters::getInstance())->writeGUIParams();
}

bool Notepad_plus::saveProjectPanelsParams()
{
	if (_pProjectPanel_1)
	{
		_pProjectPanel_1->checkIfNeedSave(TEXT("Project Panel 1"));
		(NppParameters::getInstance())->setWorkSpaceFilePath(0, _pProjectPanel_1->getWorkSpaceFilePath());
	}
	if (_pProjectPanel_2)
	{
		_pProjectPanel_2->checkIfNeedSave(TEXT("Project Panel 2"));
		(NppParameters::getInstance())->setWorkSpaceFilePath(1, _pProjectPanel_2->getWorkSpaceFilePath());
	}
	if (_pProjectPanel_3)
	{
		_pProjectPanel_3->checkIfNeedSave(TEXT("Project Panel 3"));
		(NppParameters::getInstance())->setWorkSpaceFilePath(2, _pProjectPanel_3->getWorkSpaceFilePath());
	}
	return (NppParameters::getInstance())->writeProjectPanelsSettings();
}

void Notepad_plus::saveDockingParams()
{
	NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();

	// Save the docking information
	nppGUI._dockingData._leftWidth		= _dockingManager.getDockedContSize(CONT_LEFT);
	nppGUI._dockingData._rightWidth		= _dockingManager.getDockedContSize(CONT_RIGHT);
	nppGUI._dockingData._topHeight		= _dockingManager.getDockedContSize(CONT_TOP);
	nppGUI._dockingData._bottomHight	= _dockingManager.getDockedContSize(CONT_BOTTOM);

	// clear the container tab information (active tab)
	nppGUI._dockingData._containerTabInfo.clear();

	// create a vector to save the current information
	vector<PluginDlgDockingInfo>	vPluginDockInfo;
	vector<FloatingWindowInfo>		vFloatingWindowInfo;

	// save every container
	vector<DockingCont*> vCont = _dockingManager.getContainerInfo();

	for (size_t i = 0, len = vCont.size(); i < len ; ++i)
	{
		// save at first the visible Tb's
		vector<tTbData *>	vDataVis	= vCont[i]->getDataOfVisTb();

		for (size_t j = 0, len2 = vDataVis.size(); j < len2 ; ++j)
		{
			if (vDataVis[j]->pszName && vDataVis[j]->pszName[0])
			{
				PluginDlgDockingInfo pddi(vDataVis[j]->pszModuleName, vDataVis[j]->dlgID, i, vDataVis[j]->iPrevCont, true);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the hidden Tb's
		vector<tTbData *>	vDataAll	= vCont[i]->getDataOfAllTb();

		for (size_t j = 0, len3 = vDataAll.size(); j < len3 ; ++j)
		{
			if ((vDataAll[j]->pszName && vDataAll[j]->pszName[0]) && (!vCont[i]->isTbVis(vDataAll[j])))
			{
				PluginDlgDockingInfo pddi(vDataAll[j]->pszModuleName, vDataAll[j]->dlgID, i, vDataAll[j]->iPrevCont, false);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the position, when container is a floated one
		if (i >= DOCKCONT_MAX)
		{
			RECT	rc;
			vCont[i]->getWindowRect(rc);
			FloatingWindowInfo fwi(i, rc.left, rc.top, rc.right, rc.bottom);
			vFloatingWindowInfo.push_back(fwi);
		}

		// save the active tab
		ContainerTabInfo act(i, vCont[i]->getActiveTb());
		nppGUI._dockingData._containerTabInfo.push_back(act);
	}

	// add the missing information and store it in nppGUI
	UCHAR floatContArray[50];
	memset(floatContArray, 0, 50);

	for (size_t i = 0, len4 = nppGUI._dockingData._pluginDockInfo.size(); i < len4 ; ++i)
	{
		BOOL	isStored = FALSE;
		for (size_t j = 0, len5 = vPluginDockInfo.size(); j < len5; ++j)
		{
			if (nppGUI._dockingData._pluginDockInfo[i] == vPluginDockInfo[j])
			{
				isStored = TRUE;
				break;
			}
		}

		if (isStored == FALSE)
		{
			int floatCont	= 0;

			if (nppGUI._dockingData._pluginDockInfo[i]._currContainer >= DOCKCONT_MAX)
				floatCont = nppGUI._dockingData._pluginDockInfo[i]._currContainer;
			else
				floatCont = nppGUI._dockingData._pluginDockInfo[i]._prevContainer;

			if (floatContArray[floatCont] == 0)
			{
				RECT rc;
				if (nppGUI._dockingData.getFloatingRCFrom(floatCont, rc))
				{
					vFloatingWindowInfo.push_back(FloatingWindowInfo(floatCont, rc.left, rc.top, rc.right, rc.bottom));
				}
				floatContArray[floatCont] = 1;
			}
			if (i < nppGUI._dockingData._pluginDockInfo.size()) // to prevent from crash in debug mode
				vPluginDockInfo.push_back(nppGUI._dockingData._pluginDockInfo[i]);
		}
	}

	nppGUI._dockingData._pluginDockInfo = vPluginDockInfo;
	nppGUI._dockingData._flaotingWindowInfo = vFloatingWindowInfo;
}


int Notepad_plus::getHtmlXmlEncoding(const TCHAR *fileName) const
{
	// Get Language type
	TCHAR *ext = PathFindExtension(fileName);
	if (*ext == '.') //extension found
	{
		ext += 1;
	}
	else
	{
		return -1;
	}
	NppParameters *pNppParamInst = NppParameters::getInstance();
	LangType langT = pNppParamInst->getLangFromExt(ext);

	if ((langT != L_XML) && (langT != L_HTML))
		return -1;

	// Get the beginning of file data
	FILE *f = generic_fopen(fileName, TEXT("rb"));
	if (!f)
		return -1;
	const int blockSize = 1024; // To ensure that length is long enough to capture the encoding in html
	char data[blockSize];
	int lenFile = fread(data, 1, blockSize, f);
	fclose(f);

	// Put data in _invisibleEditView
	_invisibleEditView.execute(SCI_CLEARALL);
    _invisibleEditView.execute(SCI_APPENDTEXT, lenFile, (LPARAM)data);

	const char *encodingAliasRegExpr = "[a-zA-Z0-9_-]+";

	if (langT == L_XML)
	{
		// find encoding by RegExpr

		const char *xmlHeaderRegExpr = "<?xml[ \\t]+version[ \\t]*=[ \\t]*\"[^\"]+\"[ \\t]+encoding[ \\t]*=[ \\t]*\"[^\"]+\"[ \\t]*.*?>";

        int startPos = 0;
		int endPos = lenFile-1;
		_invisibleEditView.execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

		_invisibleEditView.execute(SCI_SETTARGETSTART, startPos);
		_invisibleEditView.execute(SCI_SETTARGETEND, endPos);

		int posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(xmlHeaderRegExpr), (LPARAM)xmlHeaderRegExpr);
		if (posFound != -1 && posFound != -2)
		{
            const char *encodingBlockRegExpr = "encoding[ \\t]*=[ \\t]*\"[^\".]+\"";
            _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingBlockRegExpr), (LPARAM)encodingBlockRegExpr);

            const char *encodingRegExpr = "\".+\"";
            _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingRegExpr), (LPARAM)encodingRegExpr);

			_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingAliasRegExpr), (LPARAM)encodingAliasRegExpr);

            startPos = int(_invisibleEditView.execute(SCI_GETTARGETSTART));
			endPos = int(_invisibleEditView.execute(SCI_GETTARGETEND));

            char encodingStr[128];
            _invisibleEditView.getText(encodingStr, startPos, endPos);

			EncodingMapper *em = EncodingMapper::getInstance();
            int enc = em->getEncodingFromString(encodingStr);
            return (enc==CP_ACP?-1:enc);
		}
        return -1;
	}
	else // if (langT == L_HTML)
	{
		const char *htmlHeaderRegExpr  = "<meta[ \\t]+http-equiv[ \\t]*=[ \\t\"']*Content-Type[ \\t\"']*content[ \\t]*= *[\"']text/html;[ \\t]+charset[ \\t]*=[ \\t]*.+[\"'] */*>";
		const char *htmlHeaderRegExpr2 = "<meta[ \\t]+content[ \\t]*= *[\"']text/html;[ \\t]+charset[ \\t]*=[ \\t]*.+[ \\t\"']http-equiv[ \\t]*=[ \\t\"']*Content-Type[ \\t\"']*/*>";
		const char *charsetBlock = "charset[ \\t]*=[ \\t]*[^\"']+";
		const char *intermediaire = "=[ \\t]*.+";
		const char *encodingStrRE = "[^ \\t=]+";

        int startPos = 0;
		int endPos = lenFile-1;
		_invisibleEditView.execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

		_invisibleEditView.execute(SCI_SETTARGETSTART, startPos);
		_invisibleEditView.execute(SCI_SETTARGETEND, endPos);

		int posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(htmlHeaderRegExpr), (LPARAM)htmlHeaderRegExpr);

		if (posFound == -1 || posFound == -2)
		{
			posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(htmlHeaderRegExpr2), (LPARAM)htmlHeaderRegExpr2);
			if (posFound == -1 || posFound == -2)
				return -1;
		}
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(charsetBlock), (LPARAM)charsetBlock);
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(intermediaire), (LPARAM)intermediaire);
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingStrRE), (LPARAM)encodingStrRE);

        startPos = int(_invisibleEditView.execute(SCI_GETTARGETSTART));
		endPos = int(_invisibleEditView.execute(SCI_GETTARGETEND));

        char encodingStr[128];
        _invisibleEditView.getText(encodingStr, startPos, endPos);

		EncodingMapper *em = EncodingMapper::getInstance();
		int enc = em->getEncodingFromString(encodingStr);
        return (enc==CP_ACP?-1:enc);
	}
}


bool Notepad_plus::replaceInOpenedFiles() {

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	Buffer * oldBuf = _invisibleEditView.getCurrentBuffer();	//for manually setting the buffer, so notifications can be handled properly

	Buffer * pBuf = NULL;

	int nbTotal = 0;
	const bool isEntireDoc = true;

    if (_mainWindowStatus & WindowMainActive)
    {
		for (int i = 0, len = _mainDocTab.nbItem(); i < len ; ++i)
	    {
			pBuf = MainFileManager->getBufferByID(_mainDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
			_invisibleEditView.setCurrentBuffer(pBuf);
		    _invisibleEditView.execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(ProcessReplaceAll, FindReplaceDlg::_env, isEntireDoc);
			_invisibleEditView.execute(SCI_ENDUNDOACTION);
		}
	}

	if (_mainWindowStatus & WindowSubActive)
    {
		for (int i = 0, len = _subDocTab.nbItem(); i < len; ++i)
	    {
			pBuf = MainFileManager->getBufferByID(_subDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
			_invisibleEditView.setCurrentBuffer(pBuf);
		    _invisibleEditView.execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(ProcessReplaceAll, FindReplaceDlg::_env, isEntireDoc);
			_invisibleEditView.execute(SCI_ENDUNDOACTION);
		}
	}

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_invisibleEditView.setCurrentBuffer(oldBuf);
	_pEditView = pOldView;


	if (nbTotal < 0)
		_findReplaceDlg.setStatusbarMessage(TEXT("Replace in Opened Files: The regular expression to search is formed badly"), FSNotFound);
	else
	{
		if (nbTotal)
			enableCommand(IDM_FILE_SAVEALL, true, MENU | TOOLBAR);
		TCHAR result[64];
		wsprintf(result, TEXT("Replace in Opened Files: %d occurrences replaced."), nbTotal);
		_findReplaceDlg.setStatusbarMessage(result, FSMessage);
	}
	return true;
}

bool Notepad_plus::matchInList(const TCHAR *fileName, const vector<generic_string> & patterns)
{
	for (size_t i = 0, len = patterns.size() ; i < len ; ++i)
	{
		if (PathMatchSpec(fileName, patterns[i].c_str()))
			return true;
	}
	return false;
}


void Notepad_plus::wsTabConvert(spaceTab whichWay)
{
    int tabWidth = _pEditView->execute(SCI_GETTABWIDTH);
    int currentPos = _pEditView->execute(SCI_GETCURRENTPOS);
    int lastLine = _pEditView->lastZeroBasedLineNumber();
    int docLength = int(_pEditView->execute(SCI_GETLENGTH)) + 1;
    if (docLength < 2) 
        return;

    int count = 0;
    int column = 0;
    int counter = 0;
    int newCurrentPos = 0;
    int tabStop = tabWidth - 1;   // remember, counting from zero !
    bool onlyLeading = false;
    bool nonSpaceFound = false;
    vector<int> bookmarks;
    vector<int> folding;

    for (int i=0; i<lastLine; ++i)
    {
        if (bookmarkPresent(i))
            bookmarks.push_back(i);

        if ((_pEditView->execute(SCI_GETFOLDLEVEL, i) & SC_FOLDLEVELHEADERFLAG))
            if (_pEditView->execute(SCI_GETFOLDEXPANDED, i) == 0)
                folding.push_back(i);
    }

    char * source = new char[docLength];
    if (source == NULL)
        return;
    _pEditView->execute(SCI_GETTEXT, docLength, (LPARAM)source);

    if (whichWay == tab2Space)
    {
        // count how many tabs are there
        for (const char * ch=source; *ch; ++ch)
        {
            if (*ch == '\t')
                ++count;
        }
        if (count == 0)
        {
            delete [] source;
            return;
        }
    }
    // allocate tabwidth-1 chars extra per tab, just to be safe
    size_t newlen = docLength + count * (tabWidth - 1) + 1;
    char * destination = new char[newlen];
    if (destination == NULL)
    {
        delete [] source;
        return;
    }   
    char * dest = destination;

    switch (whichWay)
    {
        case tab2Space:
        {
            // rip through each line of the file
            for (int i = 0; source[i] != '\0'; ++i)
            {
                if (source[i] == '\t')
                {
                    size_t insertTabs = tabWidth - (column % tabWidth);
                    for (size_t j = 0; j<insertTabs; ++j)
                    {
                        *dest++ = ' ';
                        if (i <= currentPos)
                            ++newCurrentPos;
                    }
                    column += insertTabs;
                }
                else
                {
                    *dest++ = source[i];
                    if (i <= currentPos)
                        ++newCurrentPos;
                    if ((source[i] == '\n') || (source[i] == '\r'))
                        column = 0;
                    else if ((source[i] & 0xC0) != 0x80)  // UTF_8 support: count only bytes that don't start with 10......
                        ++column;
                }
            }
            *dest = '\0';
            break;
        }
        case space2TabLeading:
        {
            onlyLeading = true;
        }
        case space2TabAll:
        {
            bool nextChar = false;
            for (int i=0; source[i] != '\0'; ++i)
            {
                if (nonSpaceFound == false)
                {
                    while (source[i + counter] == ' ')
                    {
                        if ((column + counter) == tabStop)
                        {
                            tabStop += tabWidth;
                            if (counter >= 1)        // counter is counted from 0, so counter >= max-1
                            {
                                *dest++ = '\t';
                                i += counter;
                                column += counter + 1;
                                counter = 0;
                                nextChar = true;
                                if (i <= currentPos)
                                    ++newCurrentPos;                                
                                break;
                            }
                            else if (source[i+1] == ' ' || source[i+1] == '\t')  // if followed by space or TAB, convert even a single space to TAB
                            {
                                *dest++ = '\t';
                                i++;
                                column += 1;
                                counter = 0;
                                if (i <= currentPos)
                                    ++newCurrentPos;                                
                            }
                            else       // single space, don't convert it to TAB
                            {
                                *dest++ = source[i];
                                column += 1;
                                counter = 0;
                                nextChar = true;
                                if (i <= currentPos)
                                    ++newCurrentPos;                                
                                break;
                            }
                        }
                        else
                            ++counter;
                    }
                    
                    if (nextChar == true)
                    {
                        nextChar = false;
                        continue;
                    }
                    
                    if (source[i] == ' ' && source[i + counter] == '\t') // spaces "absorbed" by a TAB on the right
                    {
                        *dest++ = '\t';
                        i += counter;
                        column = tabStop + 1;
                        tabStop += tabWidth;
                        counter = 0;
                        if (i <= currentPos)
                            ++newCurrentPos;
                        continue;
                    }
                }
                
                if (onlyLeading == true && nonSpaceFound == false)
                    nonSpaceFound = true;

                if (source[i] == '\n' || source[i] == '\r')
                {
                    *dest++ = source[i]; 
                    column = 0;
                    tabStop = tabWidth - 1;
                    nonSpaceFound = false;
                }
                else if (source[i] == '\t')
                {
                    *dest++ = source[i];
                    column = tabStop + 1;
                    tabStop += tabWidth;
                    counter = 0;
                }
                else
                {
                    *dest++ = source[i];
                    counter = 0;
                    if ((source[i] & 0xC0) != 0x80)   // UTF_8 support: count only bytes that don't start with 10......
                    {
                        ++column;

                        if (column > 0 && column % tabWidth == 0)
                            tabStop += tabWidth;
                    }
                }
                
                if (i <= currentPos)
                    ++newCurrentPos;
            }
            *dest = '\0';
            break;
        }
    }

    _pEditView->execute(SCI_BEGINUNDOACTION);    
    _pEditView->execute(SCI_SETTEXT, 0, (LPARAM)destination);
    _pEditView->execute(SCI_GOTOPOS, newCurrentPos);

    for (size_t i=0; i<bookmarks.size(); ++i)
        _pEditView->execute(SCI_MARKERADD, bookmarks[i], MARK_BOOKMARK);

    for (size_t i=0; i<folding.size(); ++i)
        _pEditView->fold(folding[i], false);

    _pEditView->execute(SCI_ENDUNDOACTION);

    // clean up
    delete [] source;
    delete [] destination;
}

void Notepad_plus::doTrim(trimOp whichPart)
{
	// whichPart : line head or line tail
	FindOption env;
	if (whichPart == lineHeader)
	{
		env._str2Search = TEXT("^[	 ]+");
	}
	else if (whichPart == lineTail)
	{
		env._str2Search = TEXT("[	 ]+$");
	}
	else
		return;
	env._str4Replace = TEXT("");
    env._searchType = FindRegex;
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, true);
}

void Notepad_plus::removeEmptyLine(bool isBlankContained)
{
	// whichPart : line head or line tail
	FindOption env;
	if (isBlankContained)
	{
		env._str2Search = TEXT("^[\\t ]*$(\\r\\n|\\r|\\n)");
	}
	else
	{
		env._str2Search = TEXT("^$(\\r\\n|\\r|\\n)");
	}
	env._str4Replace = TEXT("");
    env._searchType = FindRegex;
	
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, true);


	// remove the last line if it's an empty line.
	if (isBlankContained)
	{
		env._str2Search = TEXT("(\\r\\n|\\r|\\n)^[\\t ]*$");
	}
	else
	{
		env._str2Search = TEXT("(\\r\\n|\\r|\\n)^$");
	}
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, true);
}

void Notepad_plus::getMatchedFileNames(const TCHAR *dir, const vector<generic_string> & patterns, vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir)
{
	generic_string dirFilter(dir);
	dirFilter += TEXT("*.*");
	WIN32_FIND_DATA foundData;

	HANDLE hFile = ::FindFirstFile(dirFilter.c_str(), &foundData);

	if (hFile != INVALID_HANDLE_VALUE)
	{

		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// do nothing
			}
			else if (isRecursive)
			{
				if ((lstrcmp(foundData.cFileName, TEXT("."))) && (lstrcmp(foundData.cFileName, TEXT(".."))))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive, isInHiddenDir);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				generic_string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	while (::FindNextFile(hFile, &foundData))
	{
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// do nothing
			}
			else if (isRecursive)
			{
				if ((lstrcmp(foundData.cFileName, TEXT("."))) && (lstrcmp(foundData.cFileName, TEXT(".."))))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive, isInHiddenDir);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				generic_string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	::FindClose(hFile);
}


bool Notepad_plus::replaceInFiles()
{
	const TCHAR *dir2Search = _findReplaceDlg.getDir2Search();
	if (!dir2Search[0] || !::PathFileExists(dir2Search))
	{
		return false;
	}

	bool isRecursive = _findReplaceDlg.isRecursive();
	bool isInHiddenDir = _findReplaceDlg.isInHiddenDir();
	int nbTotal = 0;

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	Buffer * oldBuf = _invisibleEditView.getCurrentBuffer();	//for manually setting the buffer, so notifications can be handled properly

	vector<generic_string> patterns2Match;
	_findReplaceDlg.getPatterns(patterns2Match);
	if (patterns2Match.size() == 0)
	{
		_findReplaceDlg.setFindInFilesDirFilter(NULL, TEXT("*.*"));
		_findReplaceDlg.getPatterns(patterns2Match);
	}
	vector<generic_string> fileNames;

	getMatchedFileNames(dir2Search, patterns2Match, fileNames, isRecursive, isInHiddenDir);

	Progress progress(_pPublicInterface->getHinst());
	size_t filesCount = fileNames.size();
	size_t filesPerPercent = 1;

	if (filesCount > 1)
	{
		if (filesCount >= 200)
			filesPerPercent = filesCount / 100;
		progress.open(_findReplaceDlg.getHSelf(), TEXT("Replace In Files progress..."));
	}

	for (size_t i = 0, updateOnCount = filesPerPercent; i < filesCount; ++i)
	{
		if (progress.isCancelled()) break;

		bool closeBuf = false;

		BufferID id = MainFileManager->getBufferFromName(fileNames.at(i).c_str());
		if (id == BUFFER_INVALID)
		{
			id = MainFileManager->loadFile(fileNames.at(i).c_str());
			closeBuf = true;
		}

		if (id != BUFFER_INVALID)
		{
			Buffer * pBuf = MainFileManager->getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
			_invisibleEditView.setCurrentBuffer(pBuf);

			int nbReplaced = _findReplaceDlg.processAll(ProcessReplaceAll, FindReplaceDlg::_env, true, fileNames.at(i).c_str());
			nbTotal += nbReplaced;
			if (nbReplaced)
			{
				MainFileManager->saveBuffer(id, pBuf->getFullPathName());
			}

			if (closeBuf)
				MainFileManager->closeBuffer(id, _pEditView);
		}
		if (i == updateOnCount)
		{
			updateOnCount += filesPerPercent;
			progress.setPercent((i * 100) / filesCount, fileNames.at(i).c_str());
			progress.flushCallerUserInput();
		}
		else
		{
			progress.setInfo(fileNames.at(i).c_str());
		}
	}

	progress.close();
	progress.flushCallerUserInput();

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_invisibleEditView.setCurrentBuffer(oldBuf);
	_pEditView = pOldView;

	TCHAR msg[128];
	wsprintf(msg, TEXT("Replace in Files: %d occurrences replaced"), nbTotal);
	_findReplaceDlg.setStatusbarMessage(msg, FSMessage);

	return true;
}

bool Notepad_plus::findInFiles()
{
	const TCHAR *dir2Search = _findReplaceDlg.getDir2Search();

	if (!dir2Search[0] || !::PathFileExists(dir2Search))
	{
		return false;
	}

	bool isRecursive = _findReplaceDlg.isRecursive();
	bool isInHiddenDir = _findReplaceDlg.isInHiddenDir();
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	vector<generic_string> patterns2Match;
	_findReplaceDlg.getPatterns(patterns2Match);
	if (patterns2Match.size() == 0)
	{
		_findReplaceDlg.setFindInFilesDirFilter(NULL, TEXT("*.*"));
		_findReplaceDlg.getPatterns(patterns2Match);
	}
	vector<generic_string> fileNames;
	getMatchedFileNames(dir2Search, patterns2Match, fileNames, isRecursive, isInHiddenDir);

	_findReplaceDlg.beginNewFilesSearch();

	Progress progress(_pPublicInterface->getHinst());

	size_t filesCount = fileNames.size();
	size_t filesPerPercent = 1;

	if (filesCount > 1)
	{
		if (filesCount >= 200)
			filesPerPercent = filesCount / 100;
		progress.open(_findReplaceDlg.getHSelf(), TEXT("Find In Files progress..."));
	}

	for (size_t i = 0, updateOnCount = filesPerPercent; i < filesCount; ++i)
	{
		if (progress.isCancelled()) break;

		bool closeBuf = false;
		BufferID id = MainFileManager->getBufferFromName(fileNames.at(i).c_str());
		if (id == BUFFER_INVALID)
		{
			id = MainFileManager->loadFile(fileNames.at(i).c_str());
			closeBuf = true;
		}

		if (id != BUFFER_INVALID)
		{
			Buffer * pBuf = MainFileManager->getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);

			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, true, fileNames.at(i).c_str());
			if (closeBuf)
				MainFileManager->closeBuffer(id, _pEditView);
		}
		if (i == updateOnCount)
		{
			updateOnCount += filesPerPercent;
			progress.setPercent((i * 100) / filesCount, fileNames.at(i).c_str());
			progress.flushCallerUserInput();
		}
		else
		{
			progress.setInfo(fileNames.at(i).c_str());
		}
	}

	progress.close();
	progress.flushCallerUserInput();

	_findReplaceDlg.finishFilesSearch(nbTotal);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	FindHistory & findHistory = (NppParameters::getInstance())->getFindHistory();
	if (nbTotal && !findHistory._isDlgAlwaysVisible)
		_findReplaceDlg.display(false);
	return true;
}


bool Notepad_plus::findInOpenedFiles()
{
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	Buffer * pBuf = NULL;

	const bool isEntireDoc = true;

	_findReplaceDlg.beginNewFilesSearch();

    if (_mainWindowStatus & WindowMainActive)
    {
		for (int i = 0, len = _mainDocTab.nbItem(); i < len ; ++i)
	    {
			pBuf = MainFileManager->getBufferByID(_mainDocTab.getBufferByIndex(i));
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, pBuf->getFullPathName());
	    }
    }

    if (_mainWindowStatus & WindowSubActive)
    {
		for (int i = 0, len2 = _subDocTab.nbItem(); i < len2 ; ++i)
	    {
			pBuf = MainFileManager->getBufferByID(_subDocTab.getBufferByIndex(i));
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, pBuf->getFullPathName());
	    }
    }

	_findReplaceDlg.finishFilesSearch(nbTotal);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	FindHistory & findHistory = (NppParameters::getInstance())->getFindHistory();
	if (nbTotal && !findHistory._isDlgAlwaysVisible)
		_findReplaceDlg.display(false);
	return true;
}


bool Notepad_plus::findInCurrentFile()
{
	int nbTotal = 0;
	Buffer * pBuf = _pEditView->getCurrentBuffer();
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	const bool isEntireDoc = true;

	_findReplaceDlg.beginNewFilesSearch();

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
	int cp = _invisibleEditView.execute(SCI_GETCODEPAGE);
	_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? cp : SC_CP_UTF8);
	nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, pBuf->getFullPathName());

	_findReplaceDlg.finishFilesSearch(nbTotal);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	FindHistory & findHistory = (NppParameters::getInstance())->getFindHistory();
	if (nbTotal && !findHistory._isDlgAlwaysVisible)
		_findReplaceDlg.display(false);
	return true;
}

void Notepad_plus::filePrint(bool showDialog)
{
	Printer printer;

	int startPos = int(_pEditView->execute(SCI_GETSELECTIONSTART));
	int endPos = int(_pEditView->execute(SCI_GETSELECTIONEND));

	printer.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pEditView, showDialog, startPos, endPos);
	printer.doPrint();
}

int Notepad_plus::doSaveOrNot(const TCHAR *fn)
{
	TCHAR pattern[64] = TEXT("Save file \"%s\" ?");
	TCHAR phrase[512];
	wsprintf(phrase, pattern, fn);
	return doActionOrNot(TEXT("Save"), phrase, MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
}

int Notepad_plus::doReloadOrNot(const TCHAR *fn, bool dirty)
{
	TCHAR* pattern = TEXT("%s\r\rThis file has been modified by another program.\rDo you want to reload it%s?");
	TCHAR* lose_info_str = dirty ? TEXT(" and lose the changes made in Notepad++") : TEXT("");
	TCHAR phrase[512];
	wsprintf(phrase, pattern, fn, lose_info_str);
	int icon = dirty ? MB_ICONEXCLAMATION : MB_ICONQUESTION;
	return doActionOrNot(TEXT("Reload"), phrase, MB_YESNO | MB_APPLMODAL | icon);
}

int Notepad_plus::doCloseOrNot(const TCHAR *fn)
{
	TCHAR pattern[128] = TEXT("The file \"%s\" doesn't exist anymore.\rKeep this file in editor?");
	TCHAR phrase[512];
	wsprintf(phrase, pattern, fn);
	return doActionOrNot(TEXT("Keep non existing file"), phrase, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
}

int Notepad_plus::doDeleteOrNot(const TCHAR *fn)
{
	TCHAR pattern[128] = TEXT("The file \"%s\"\rwill be moved to your Recycle Bin and this document will be closed.\rContinue?");
	TCHAR phrase[512];
	wsprintf(phrase, pattern, fn);
	return doActionOrNot(TEXT("Delete file"), phrase, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
}

int Notepad_plus::doActionOrNot(const TCHAR *title, const TCHAR *displayText, int type)
{
	return ::MessageBox(_pPublicInterface->getHSelf(), displayText, title, type);
}

void Notepad_plus::enableMenu(int cmdID, bool doEnable) const
{
	int flag = doEnable?MF_ENABLED | MF_BYCOMMAND:MF_DISABLED | MF_GRAYED | MF_BYCOMMAND;
	::EnableMenuItem(_mainMenuHandle, cmdID, flag);
}

void Notepad_plus::enableCommand(int cmdID, bool doEnable, int which) const
{
	if (which & MENU)
	{
		enableMenu(cmdID, doEnable);
	}
	if (which & TOOLBAR)
	{
		_toolBar.enable(cmdID, doEnable);
	}
}

void Notepad_plus::checkClipboard()
{
	bool hasSelection = (_pEditView->execute(SCI_GETSELECTIONSTART) != _pEditView->execute(SCI_GETSELECTIONEND));
	bool canPaste = (_pEditView->execute(SCI_CANPASTE) != 0);
	enableCommand(IDM_EDIT_CUT, hasSelection, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_COPY, hasSelection, MENU | TOOLBAR);

	enableCommand(IDM_EDIT_PASTE, canPaste, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_DELETE, hasSelection, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_UPPERCASE, hasSelection, MENU);
	enableCommand(IDM_EDIT_LOWERCASE, hasSelection, MENU);
}

void Notepad_plus::checkDocState()
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();

	bool isCurrentDirty = curBuf->isDirty();
	bool isSeveralDirty = isCurrentDirty;
	bool isFileExisting = PathFileExists(curBuf->getFullPathName()) != FALSE;
	if (!isCurrentDirty)
	{
		for(int i = 0; i < MainFileManager->getNrBuffers(); ++i)
		{
			if (MainFileManager->getBufferByIndex(i)->isDirty())
			{
				isSeveralDirty = true;
				break;
			}
		}
	}

	bool isCurrentUntitled = curBuf->isUntitled();
	enableCommand(IDM_FILE_SAVE, isCurrentDirty, MENU | TOOLBAR);
	enableCommand(IDM_FILE_SAVEALL, isSeveralDirty, MENU | TOOLBAR);
	enableCommand(IDM_VIEW_GOTO_NEW_INSTANCE, !(isCurrentDirty || isCurrentUntitled), MENU);
	enableCommand(IDM_VIEW_LOAD_IN_NEW_INSTANCE, !(isCurrentDirty || isCurrentUntitled), MENU);

	bool isSysReadOnly = curBuf->getFileReadOnly();
	if (isSysReadOnly)
	{
		::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | MF_UNCHECKED);
		enableCommand(IDM_EDIT_SETREADONLY, false, MENU);
		enableCommand(IDM_EDIT_CLEARREADONLY, true, MENU);
	}
	else
	{
		enableCommand(IDM_EDIT_SETREADONLY, true, MENU);
		enableCommand(IDM_EDIT_CLEARREADONLY, false, MENU);
		bool isUserReadOnly = curBuf->getUserReadOnly();
		::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | (isUserReadOnly?MF_CHECKED:MF_UNCHECKED));
	}
	enableCommand(IDM_FILE_DELETE, isFileExisting, MENU);
	enableCommand(IDM_FILE_RENAME, isFileExisting, MENU);

	enableConvertMenuItems(curBuf->getFormat());
	checkUnicodeMenuItems(/*curBuf->getUnicodeMode()*/);
	checkLangsMenu(-1);

	if (_pAnsiCharPanel)
		_pAnsiCharPanel->switchEncoding();
}

void Notepad_plus::checkUndoState()
{
	enableCommand(IDM_EDIT_UNDO, _pEditView->execute(SCI_CANUNDO) != 0, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_REDO, _pEditView->execute(SCI_CANREDO) != 0, MENU | TOOLBAR);
}

void Notepad_plus::checkMacroState()
{
	enableCommand(IDM_MACRO_STARTRECORDINGMACRO, !_recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_STOPRECORDINGMACRO, _recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_PLAYBACKRECORDEDMACRO, !_macro.empty() && !_recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_SAVECURRENTMACRO, !_macro.empty() && !_recordingMacro, MENU | TOOLBAR);

	enableCommand(IDM_MACRO_RUNMULTIMACRODLG, (!_macro.empty() && !_recordingMacro) || !((NppParameters::getInstance())->getMacroList()).empty(), MENU | TOOLBAR);
}

void Notepad_plus::checkSyncState()
{
	bool canDoSync = viewVisible(MAIN_VIEW) && viewVisible(SUB_VIEW);
	if (!canDoSync)
	{
		_syncInfo._isSynScollV = false;
		_syncInfo._isSynScollH = false;
		checkMenuItem(IDM_VIEW_SYNSCROLLV, false);
		checkMenuItem(IDM_VIEW_SYNSCROLLH, false);
		_toolBar.setCheck(IDM_VIEW_SYNSCROLLV, false);
		_toolBar.setCheck(IDM_VIEW_SYNSCROLLH, false);
	}
	enableCommand(IDM_VIEW_SYNSCROLLV, canDoSync, MENU | TOOLBAR);
	enableCommand(IDM_VIEW_SYNSCROLLH, canDoSync, MENU | TOOLBAR);
}

void Notepad_plus::checkLangsMenu(int id) const
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();
	if (id == -1)
	{
		id = (NppParameters::getInstance())->langTypeToCommandID(curBuf->getLangType());
		if (id == IDM_LANG_USER)
		{
			if (curBuf->isUserDefineLangExt())
			{
				const TCHAR *userLangName = curBuf->getUserDefineLangName();
				const int nbChar = 16;
				TCHAR menuLangName[nbChar];

				for (int i = IDM_LANG_USER + 1 ; i <= IDM_LANG_USER_LIMIT ; ++i)
				{
					if (::GetMenuString(_mainMenuHandle, i, menuLangName, nbChar-1, MF_BYCOMMAND))
						if (!lstrcmp(userLangName, menuLangName))
						{
							::CheckMenuRadioItem(_mainMenuHandle, IDM_LANG_C, IDM_LANG_USER_LIMIT, i, MF_BYCOMMAND);
							return;
						}
				}
			}
		}
	}
	::CheckMenuRadioItem(_mainMenuHandle, IDM_LANG_C, IDM_LANG_USER_LIMIT, id, MF_BYCOMMAND);
}

generic_string Notepad_plus::getLangDesc(LangType langType, bool getName)
{

	if ((langType >= L_EXTERNAL) && (langType < NppParameters::getInstance()->L_END))
	{
		ExternalLangContainer & elc = NppParameters::getInstance()->getELCFromIndex(langType - L_EXTERNAL);
		if (getName)
			return generic_string(elc._name);
		else
			return generic_string(elc._desc);
	}

	if (langType > L_EXTERNAL)
        langType = L_TEXT;

	generic_string str2Show;
	if (getName)
		str2Show = ScintillaEditView::langNames[langType].shortName;
	else
		str2Show = ScintillaEditView::langNames[langType].longName;

	if (langType == L_USER)
	{
		Buffer * currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf->isUserDefineLangExt())
		{
			str2Show += TEXT(" - ");
			str2Show += currentBuf->getUserDefineLangName();
		}
	}
	return str2Show;
}

void Notepad_plus::copyMarkedLines()
{
	int lastLine = _pEditView->lastZeroBasedLineNumber();
	generic_string globalStr = TEXT("");
	for (int i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i))
		{
			generic_string currentStr = getMarkedLine(i) + globalStr;
			globalStr = currentStr;
		}
	}
	str2Cliboard(globalStr);
}

void Notepad_plus::cutMarkedLines()
{
	int lastLine = _pEditView->lastZeroBasedLineNumber();
	generic_string globalStr = TEXT("");

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (int i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i))
		{
			generic_string currentStr = getMarkedLine(i) + globalStr;
			globalStr = currentStr;

			deleteMarkedline(i);
		}
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
	str2Cliboard(globalStr);
}

void Notepad_plus::deleteMarkedLines(bool isMarked)
{
	int lastLine = _pEditView->lastZeroBasedLineNumber();

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (int i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i) == isMarked)
			deleteMarkedline(i);
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::pasteToMarkedLines()
{
	int clipFormat;
	clipFormat = CF_UNICODETEXT;

	BOOL canPaste = ::IsClipboardFormatAvailable(clipFormat);
	if (!canPaste)
		return;
	int lastLine = _pEditView->lastZeroBasedLineNumber();

	::OpenClipboard(_pPublicInterface->getHSelf());
	HANDLE clipboardData = ::GetClipboardData(clipFormat);
	::GlobalSize(clipboardData);
	LPVOID clipboardDataPtr = ::GlobalLock(clipboardData);

	generic_string clipboardStr = (const TCHAR *)clipboardDataPtr;

	::GlobalUnlock(clipboardData);
	::CloseClipboard();

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (int i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i))
		{
			replaceMarkedline(i, clipboardStr.c_str());
		}
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::deleteMarkedline(int ln)
{
	int lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
	int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);

	bookmarkDelete(ln);
	TCHAR emptyString[2] = TEXT("");
	_pEditView->replaceTarget(emptyString, lineBegin, lineBegin + lineLen);
}

void Notepad_plus::inverseMarks()
{
	int lastLine = _pEditView->lastZeroBasedLineNumber();
	for (int i = 0 ; i <= lastLine  ; ++i)
	{
		if (bookmarkPresent(i))
		{
			bookmarkDelete(i);
		}
		else
		{
			bookmarkAdd(i);
		}
	}
}

void Notepad_plus::replaceMarkedline(int ln, const TCHAR *str)
{
	int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);
	int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, ln);

	_pEditView->replaceTarget(str, lineBegin, lineEnd);
}

generic_string Notepad_plus::getMarkedLine(int ln)
{
	int lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
	int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);

	TCHAR * buf = new TCHAR[lineLen+1];
	_pEditView->getGenericText(buf, lineLen + 1, lineBegin, lineBegin + lineLen);
	generic_string line = buf;
	delete [] buf;

	return line;
}

void Notepad_plus::findMatchingBracePos(int & braceAtCaret, int & braceOpposite)
{
	int caretPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	braceAtCaret = -1;
	braceOpposite = -1;
	TCHAR charBefore = '\0';

	int lengthDoc = int(_pEditView->execute(SCI_GETLENGTH));

	if ((lengthDoc > 0) && (caretPos > 0))
    {
		charBefore = TCHAR(_pEditView->execute(SCI_GETCHARAT, caretPos - 1, 0));
	}
	// Priority goes to character before caret
	if (charBefore && generic_strchr(TEXT("[](){}"), charBefore))
    {
		braceAtCaret = caretPos - 1;
	}

	if (lengthDoc > 0  && (braceAtCaret < 0))
    {
		// No brace found so check other side
		TCHAR charAfter = TCHAR(_pEditView->execute(SCI_GETCHARAT, caretPos, 0));
		if (charAfter && generic_strchr(TEXT("[](){}"), charAfter))
        {
			braceAtCaret = caretPos;
		}
	}
	if (braceAtCaret >= 0)
		braceOpposite = int(_pEditView->execute(SCI_BRACEMATCH, braceAtCaret, 0));
}

// return true if 1 or 2 (matched) brace(s) is found
bool Notepad_plus::braceMatch()
{
	int braceAtCaret = -1;
	int braceOpposite = -1;
	findMatchingBracePos(braceAtCaret, braceOpposite);

	if ((braceAtCaret != -1) && (braceOpposite == -1))
    {
		_pEditView->execute(SCI_BRACEBADLIGHT, braceAtCaret);
		_pEditView->execute(SCI_SETHIGHLIGHTGUIDE, 0);
	}
    else
    {
		_pEditView->execute(SCI_BRACEHIGHLIGHT, braceAtCaret, braceOpposite);

		if (_pEditView->isShownIndentGuide())
        {
            int columnAtCaret = int(_pEditView->execute(SCI_GETCOLUMN, braceAtCaret));
		    int columnOpposite = int(_pEditView->execute(SCI_GETCOLUMN, braceOpposite));
			_pEditView->execute(SCI_SETHIGHLIGHTGUIDE, (columnAtCaret < columnOpposite)?columnAtCaret:columnOpposite);
        }
    }

	const bool enable = (braceAtCaret != -1) && (braceOpposite != -1);
    enableCommand(IDM_SEARCH_GOTOMATCHINGBRACE, enable, MENU | TOOLBAR);
	enableCommand(IDM_SEARCH_SELECTMATCHINGBRACES, enable, MENU);
    return (braceAtCaret != -1);
}


void Notepad_plus::setDisplayFormat(formatType f)
{
	generic_string str;
	switch (f)
	{
		case MAC_FORMAT :
			str = TEXT("Macintosh");
			break;
		case UNIX_FORMAT :
			str = TEXT("UNIX");
			break;
		default :
			str = TEXT("Dos\\Windows");
	}
	_statusBar.setText(str.c_str(), STATUSBAR_EOF_FORMAT);
}

void Notepad_plus::setUniModeText()
{
	Buffer *buf = _pEditView->getCurrentBuffer();
	int encoding = buf->getEncoding();
	UniMode um = buf->getUnicodeMode();

	generic_string uniModeTextString;

	if (encoding == -1)
	{
		switch (um)
		{
			case uniUTF8:
				uniModeTextString = TEXT("UTF-8"); break;
			case uni16BE:
				uniModeTextString = TEXT("UCS-2 Big Endian"); break;
			case uni16LE:
				uniModeTextString = TEXT("UCS-2 Little Endian"); break;
			case uni16BE_NoBOM:
				uniModeTextString = TEXT("UCS-2 BE w/o BOM"); break;
			case uni16LE_NoBOM:
				uniModeTextString = TEXT("UCS-2 LE w/o BOM"); break;
			case uniCookie:
				uniModeTextString = TEXT("UTF-8 w/o BOM"); break;
			default :
				uniModeTextString = TEXT("ANSI");
		}
	}
	else
	{
		EncodingMapper *em = EncodingMapper::getInstance();
		int cmdID = em->getIndexFromEncoding(encoding);
		if (cmdID == -1)
		{
			//printStr(TEXT("Encoding problem. Encoding is not added in encoding_table?"));
			return;
		}
		cmdID += IDM_FORMAT_ENCODE;

		const int itemSize = 64;
		TCHAR uniModeText[itemSize];
		::GetMenuString(_mainMenuHandle, cmdID, uniModeText, itemSize, MF_BYCOMMAND);
		uniModeTextString = uniModeText;
	}
	_statusBar.setText(uniModeTextString.c_str(), STATUSBAR_UNICODE_TYPE);
}


void Notepad_plus::addHotSpot()
{
	int startPos = 0;
	int endPos = -1;
	int endStyle = _pEditView->execute(SCI_GETENDSTYLED);

	_pEditView->getVisibleStartAndEndPosition(&startPos, &endPos);

	_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

	_pEditView->execute(SCI_SETTARGETSTART, startPos);
	_pEditView->execute(SCI_SETTARGETEND, endPos);

	std::vector<unsigned char> hotspotPairs; //= _pEditView->GetHotspotPairs();

	unsigned char style_hotspot = 0;
	unsigned char mask = INDIC1_MASK;

	// INDIC2_MASK == 255 and it represents MSB bit 
	// only LEX_HTML and LEX_POSTSCRIPT use use INDIC2_MASK bit internally
	// LEX_HTML is using INDIC2_MASK bit even though it has only 127 states, so it is safe to overwrite 8th bit
	// INDIC2_MASK will be used for LEX_HTML

	// LEX_POSTSCRIPT is using INDIC2_MASK bit for "tokenization", and is using mask=31 in lexer,
	// therefore hotspot in LEX_POSTSCRIPT will be saved to 5th bit
	// there are only 15 states in LEX_POSTSCRIPT, so it is safe to overwrite 5th bit

	// rule of the thumb is, any lexet that calls: styler.StartAt(startPos, 255);
	// must have special processing here, all other lexers are fine with INDIC1_MASK (7th bit)

	LangType type = _pEditView->getCurrentBuffer()->getLangType();

	if (type == L_HTML || type == L_PHP || type == L_ASP || type == L_JSP)			
		mask = INDIC2_MASK;
	else if (type == L_PS)
		mask = 16;

	int posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(URL_REG_EXPR), (LPARAM)URL_REG_EXPR);

	while (posFound != -1 && posFound != -2)
	{
		int start = int(_pEditView->execute(SCI_GETTARGETSTART));
		int end = int(_pEditView->execute(SCI_GETTARGETEND));
		int foundTextLen = end - start;
		unsigned char idStyle = static_cast<unsigned char>(_pEditView->execute(SCI_GETSTYLEAT, posFound));

		// Search the style
		int fs = -1;
		for (size_t i = 0, len = hotspotPairs.size(); i < len ; ++i)
		{
			// make sure to ignore "hotspot bit" when comparing document style with archived hotspot style
			if ((hotspotPairs[i] & ~mask) == (idStyle & ~mask))
			{
				fs = hotspotPairs[i];
				_pEditView->execute(SCI_STYLEGETFORE, fs);
					break;
			}
		}

		// if we found it then use it to colourize
		if (fs != -1)
		{
			_pEditView->execute(SCI_STARTSTYLING, start, 0xFF);
			_pEditView->execute(SCI_SETSTYLING, foundTextLen, fs);
		}
		else // generalize a new style and add it into a array
		{
			style_hotspot = idStyle | mask;	// set "hotspot bit"
			hotspotPairs.push_back(style_hotspot);
			unsigned char idStyleMSBunset = idStyle & ~mask;
			char fontNameA[128];

			Style hotspotStyle;

			hotspotStyle._styleID = static_cast<int>(style_hotspot);
			_pEditView->execute(SCI_STYLEGETFONT, idStyleMSBunset, (LPARAM)fontNameA);
			TCHAR *generic_fontname = new TCHAR[128];

			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			const wchar_t * fontNameW = wmc->char2wchar(fontNameA, _nativeLangSpeaker.getLangEncoding());
			lstrcpy(generic_fontname, fontNameW);
			hotspotStyle._fontName = generic_fontname;

			hotspotStyle._fgColor = _pEditView->execute(SCI_STYLEGETFORE, idStyleMSBunset);
			hotspotStyle._bgColor = _pEditView->execute(SCI_STYLEGETBACK, idStyleMSBunset);
			hotspotStyle._fontSize = _pEditView->execute(SCI_STYLEGETSIZE, idStyleMSBunset);

			int isBold = _pEditView->execute(SCI_STYLEGETBOLD, idStyleMSBunset);
			int isItalic = _pEditView->execute(SCI_STYLEGETITALIC, idStyleMSBunset);
			int isUnderline = _pEditView->execute(SCI_STYLEGETUNDERLINE, idStyleMSBunset);
			hotspotStyle._fontStyle = (isBold?FONTSTYLE_BOLD:0) | (isItalic?FONTSTYLE_ITALIC:0) | (isUnderline?FONTSTYLE_UNDERLINE:0);

			int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
			if (urlAction == 2)
				hotspotStyle._fontStyle |= FONTSTYLE_UNDERLINE;

			_pEditView->setHotspotStyle(hotspotStyle);

			_pEditView->execute(SCI_STYLESETHOTSPOT, style_hotspot, TRUE);
			int activeFG = 0xFF0000;
			Style *urlHovered = getStyleFromName(TEXT("URL hovered"));
			if (urlHovered)
				activeFG = urlHovered->_fgColor;
			_pEditView->execute(SCI_SETHOTSPOTACTIVEFORE, TRUE, activeFG);
			_pEditView->execute(SCI_SETHOTSPOTSINGLELINE, style_hotspot, 0);

			// colourize it!
			_pEditView->execute(SCI_STARTSTYLING, start, 0xFF);
			_pEditView->execute(SCI_SETSTYLING, foundTextLen, style_hotspot);
		}

		_pEditView->execute(SCI_SETTARGETSTART, posFound + foundTextLen);
		_pEditView->execute(SCI_SETTARGETEND, endPos);

		posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(URL_REG_EXPR), (LPARAM)URL_REG_EXPR);
	}

	_pEditView->execute(SCI_STARTSTYLING, endStyle, 0xFF);
	_pEditView->execute(SCI_SETSTYLING, 0, 0);
}

bool Notepad_plus::isConditionExprLine(int lineNumber)
{
	if (lineNumber < 0 || lineNumber > _pEditView->execute(SCI_GETLINECOUNT))
		return false;

	int startPos = _pEditView->execute(SCI_POSITIONFROMLINE, lineNumber);
	int endPos = _pEditView->execute(SCI_GETLINEENDPOSITION, lineNumber);
	_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
	_pEditView->execute(SCI_SETTARGETSTART, startPos);
	_pEditView->execute(SCI_SETTARGETEND, endPos);

	const char ifElseForWhileExpr[] = "((else[ \t]+)?if|for|while)[ \t]*[(].*[)][ \t]*|else[ \t]*";

	int posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(ifElseForWhileExpr), (LPARAM)ifElseForWhileExpr);
	if (posFound != -1 && posFound != -2)
	{
		int end = int(_pEditView->execute(SCI_GETTARGETEND));
		if (end == endPos)
			return true;
	}

	return false;
}

int Notepad_plus::findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol)
{
	if (startPos == endPos)
		return -1;

	int balance = 0;

	if (startPos > endPos) // backward
	{
		for (int i = startPos; i >= int(endPos); --i)
		{
			char aChar = (char)_pEditView->execute(SCI_GETCHARAT, i);
			if (aChar == targetSymbol)
			{
				if (balance == 0)
					return i;
				--balance;
			}
			else if (aChar == matchedSymbol)
			{
				++balance;
			}
		}
	}
	else // forward - TODO
	{
	}
	return -1;
}

void Notepad_plus::maintainIndentation(TCHAR ch)
{
	int eolMode = int(_pEditView->execute(SCI_GETEOLMODE));
	int curLine = int(_pEditView->getCurrentLineNumber());
	int prevLine = curLine - 1;
	int indentAmountPrevLine = 0;
	int tabWidth = _pEditView->execute(SCI_GETTABWIDTH);

	LangType type = _pEditView->getCurrentBuffer()->getLangType();

	if (type == L_C || type == L_CPP || type == L_JAVA || type == L_CS || type == L_OBJC ||
		type == L_PHP || type == L_JS || type == L_JSP)
	{
		if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
			(eolMode == SC_EOL_CR && ch == '\r'))
		{
			// Search the non-empty previous line
			while (prevLine >= 0 && _pEditView->getLineLength(prevLine) == 0)
				prevLine--;

			// Get previous line's Indent
			if (prevLine >= 0)
			{
				indentAmountPrevLine = _pEditView->getLineIndent(prevLine);
			}

			// get previous char from current line
			int prevPos = _pEditView->execute(SCI_GETCURRENTPOS) - (eolMode == SC_EOL_CRLF ? 3 : 2);
			UCHAR prevChar = (UCHAR)_pEditView->execute(SCI_GETCHARAT, prevPos);
			int curPos = _pEditView->execute(SCI_GETCURRENTPOS);
			UCHAR nextChar = (UCHAR)_pEditView->execute(SCI_GETCHARAT, curPos);

			if (prevChar == '{')
			{
				if (nextChar == '}')
				{
					char *eolChars;
					if (eolMode == SC_EOL_CRLF)
						eolChars = "\r\n";
					else if (eolMode == SC_EOL_LF)
						eolChars = "\n";
					else
						eolChars = "\r";

					_pEditView->execute(SCI_INSERTTEXT, _pEditView->execute(SCI_GETCURRENTPOS), (LPARAM)eolChars);
					_pEditView->setLineIndent(curLine + 1, indentAmountPrevLine);
				}
				_pEditView->setLineIndent(curLine, indentAmountPrevLine + tabWidth);
			}
			else if (nextChar == '{')
			{
				_pEditView->setLineIndent(curLine, indentAmountPrevLine);
			}
			else if (isConditionExprLine(prevLine))
			{
				_pEditView->setLineIndent(curLine, indentAmountPrevLine + tabWidth);
			}
			else
			{
				if (indentAmountPrevLine > 0)
				{
					if (prevLine > 0 && isConditionExprLine(prevLine - 1))
						_pEditView->setLineIndent(curLine, indentAmountPrevLine - tabWidth);
					else
						_pEditView->setLineIndent(curLine, indentAmountPrevLine);
				}
			}
		}
		else if (ch == '{')
		{
			// if no character in front of {, aligned with prev line's indentation
			int startPos = _pEditView->execute(SCI_POSITIONFROMLINE, curLine);
			int endPos = _pEditView->execute(SCI_GETCURRENTPOS);

			for (int i = endPos - 2; i > 0 && i > startPos; --i)
			{
				UCHAR aChar = (UCHAR)_pEditView->execute(SCI_GETCHARAT, i);
				if (aChar != ' ' && aChar != '\t')
					return;
			}

			// Search the non-empty previous line
			while (prevLine >= 0 && _pEditView->getLineLength(prevLine) == 0)
				prevLine--;

			// Get previous line's Indent
			if (prevLine >= 0)
			{
				indentAmountPrevLine = _pEditView->getLineIndent(prevLine);

				int startPos = _pEditView->execute(SCI_POSITIONFROMLINE, prevLine);
				int endPos = _pEditView->execute(SCI_GETLINEENDPOSITION, prevLine);
				_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
				_pEditView->execute(SCI_SETTARGETSTART, startPos);
				_pEditView->execute(SCI_SETTARGETEND, endPos);

				const char braceExpr[] = "[ \t]*\\{.*";

				int posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(braceExpr), (LPARAM)braceExpr);
				if (posFound != -1 && posFound != -2)
				{
					int end = int(_pEditView->execute(SCI_GETTARGETEND));
					if (end == endPos)
						indentAmountPrevLine += tabWidth;
				}
			}

			_pEditView->setLineIndent(curLine, indentAmountPrevLine);
			
		}
		else if (ch == '}')
		{
			// Look backward for the pair {
			int startPos = _pEditView->execute(SCI_GETCURRENTPOS);
			if (startPos != 0)
				startPos -= 1;
			int posFound = findMachedBracePos(startPos - 1, 0, '{', '}');

			// if no { found, do nothing
			if (posFound == -1)
				return;

			// if { is in the same line, do nothing
			int matchedPairLine = _pEditView->execute(SCI_LINEFROMPOSITION, posFound);
			if (matchedPairLine == curLine)
				return;

			// { is in another line, get its indentation
			indentAmountPrevLine = _pEditView->getLineIndent(matchedPairLine);

			// aligned } indent with {
			_pEditView->setLineIndent(curLine, indentAmountPrevLine);

			/*
			// indent lines from { to }
			for (int i = matchedPairLine + 1; i < curLine; ++i)
				_pEditView->setLineIndent(i, indentAmountPrevLine + tabWidth);
			*/
		}
	}
	else // Basic indentation mode
	{
		if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
			(eolMode == SC_EOL_CR && ch == '\r'))
		{
			// Search the non-empty previous line
			while (prevLine >= 0 && _pEditView->getLineLength(prevLine) == 0)
				prevLine--;

			if (prevLine >= 0)
			{
				indentAmountPrevLine = _pEditView->getLineIndent(prevLine);
			}

			if (indentAmountPrevLine > 0)
			{
				_pEditView->setLineIndent(curLine, indentAmountPrevLine);
			}
		}
	}
}

void Notepad_plus::specialCmd(int id)
{
	NppParameters *pNppParam = NppParameters::getInstance();

	switch (id)
	{
        case IDM_VIEW_LINENUMBER:
        case IDM_VIEW_SYMBOLMARGIN:
		case IDM_VIEW_DOCCHANGEMARGIN:
        {
            int margin;
            if (id == IDM_VIEW_LINENUMBER)
                margin = ScintillaEditView::_SC_MARGE_LINENUMBER;
            else //if (id == IDM_VIEW_SYMBOLMARGIN)
                margin = ScintillaEditView::_SC_MARGE_SYBOLE;

			if (_mainEditView.hasMarginShowed(margin))
			{
                _mainEditView.showMargin(margin, false);
				_subEditView.showMargin(margin, false);
			}
            else
			{
				_mainEditView.showMargin(margin);
                _subEditView.showMargin(margin);
			}
			break;
        }

        case IDM_VIEW_FOLDERMAGIN_SIMPLE:
        case IDM_VIEW_FOLDERMAGIN_ARROW:
        case IDM_VIEW_FOLDERMAGIN_CIRCLE:
        case IDM_VIEW_FOLDERMAGIN_BOX:
		case IDM_VIEW_FOLDERMAGIN:
        {
            folderStyle fStyle = (id == IDM_VIEW_FOLDERMAGIN_SIMPLE)?FOLDER_STYLE_SIMPLE:\
								 (id == IDM_VIEW_FOLDERMAGIN_ARROW)?FOLDER_STYLE_ARROW:\
								 (id == IDM_VIEW_FOLDERMAGIN_CIRCLE)?FOLDER_STYLE_CIRCLE:\
								 (id == IDM_VIEW_FOLDERMAGIN)?FOLDER_STYLE_NONE:FOLDER_STYLE_BOX;

            _mainEditView.setMakerStyle(fStyle);
			_subEditView.setMakerStyle(fStyle);
            break;
        }

		case IDM_VIEW_CURLINE_HILITING:
		{
            COLORREF colour = pNppParam->getCurLineHilitingColour();
			_mainEditView.setCurrentLineHiLiting(!_pEditView->isCurrentLineHiLiting(), colour);
			_subEditView.setCurrentLineHiLiting(!_pEditView->isCurrentLineHiLiting(), colour);
			break;
		}

		case IDM_VIEW_EDGEBACKGROUND:
		case IDM_VIEW_EDGELINE:
		case IDM_VIEW_EDGENONE:
		{
			int mode;
			switch (id)
			{
				case IDM_VIEW_EDGELINE:
				{
					mode = EDGE_LINE;
					break;
				}
				case IDM_VIEW_EDGEBACKGROUND:
				{
					mode = EDGE_BACKGROUND;
					break;
				}
				default :
					mode = EDGE_NONE;
			}
			_mainEditView.execute(SCI_SETEDGEMODE, mode);
			_subEditView.execute(SCI_SETEDGEMODE, mode);
			break;
		}

		case IDM_VIEW_LWDEF:
		case IDM_VIEW_LWALIGN:
		case IDM_VIEW_LWINDENT:
		{
			int mode = (id == IDM_VIEW_LWALIGN)?SC_WRAPINDENT_SAME:\
				(id == IDM_VIEW_LWINDENT)?SC_WRAPINDENT_INDENT:SC_WRAPINDENT_FIXED;
			_mainEditView.execute(SCI_SETWRAPINDENTMODE, mode);
			_subEditView.execute(SCI_SETWRAPINDENTMODE, mode);
			break;
		}
	}
}

BOOL Notepad_plus::processIncrFindAccel(MSG *msg) const
{
	if (!::IsChild(_incrementFindDlg.getHSelf(), ::GetFocus()))
		return FALSE;
	return ::TranslateAccelerator(_incrementFindDlg.getHSelf(), _accelerator.getIncrFindAccTable(), msg);
}

void Notepad_plus::setLanguage(LangType langType) {
	//Add logic to prevent changing a language when a document is shared between two views
	//If so, release one document
	bool reset = false;
	Document prev = 0;
	if (bothActive()) {
		if (_mainEditView.getCurrentBufferID() == _subEditView.getCurrentBufferID()) {
			reset = true;
			_subEditView.saveCurrentPos();
			prev = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
		}
	}
	if (reset) {
		_mainEditView.getCurrentBuffer()->setLangType(langType);
	} else {
		/*
		int mode = _pEditView->execute(SCI_GETMODEVENTMASK, 0, 0);
		_pEditView->execute(SCI_SETMODEVENTMASK, 0, 0);
		_pEditView->getCurrentBuffer()->setLangType(langType);
		_pEditView->execute(SCI_SETMODEVENTMASK, mode, 0);
		*/
		_pEditView->getCurrentBuffer()->setLangType(langType);
	}

	if (reset) {
		_subEditView.execute(SCI_SETDOCPOINTER, 0, prev);
		_subEditView.restoreCurrentPos();
	}
};

enum LangType Notepad_plus::menuID2LangType(int cmdID)
{
	switch (cmdID)
	{
        case IDM_LANG_C	:
            return L_C;
        case IDM_LANG_CPP :
            return L_CPP;
        case IDM_LANG_JAVA :
            return L_JAVA;
        case IDM_LANG_CS :
            return L_CS;
        case IDM_LANG_HTML :
            return L_HTML;
        case IDM_LANG_XML :
            return L_XML;
        case IDM_LANG_JS :
            return L_JS;
        case IDM_LANG_PHP :
            return L_PHP;
        case IDM_LANG_ASP :
            return L_ASP;
        case IDM_LANG_JSP :
            return L_JSP;
        case IDM_LANG_CSS :
            return L_CSS;
        case IDM_LANG_LUA :
            return L_LUA;
        case IDM_LANG_PERL :
            return L_PERL;
        case IDM_LANG_PYTHON :
            return L_PYTHON;
        case IDM_LANG_PASCAL :
            return L_PASCAL;
        case IDM_LANG_BATCH :
            return L_BATCH;
        case IDM_LANG_OBJC :
            return L_OBJC;
        case IDM_LANG_VB :
            return L_VB;
        case IDM_LANG_SQL :
            return L_SQL;
        case IDM_LANG_ASCII :
            return L_ASCII;
        case IDM_LANG_TEXT :
            return L_TEXT;
        case IDM_LANG_RC :
            return L_RC;
        case IDM_LANG_MAKEFILE :
            return L_MAKEFILE;
        case IDM_LANG_INI :
            return L_INI;
        case IDM_LANG_TEX :
            return L_TEX;
        case IDM_LANG_FORTRAN :
            return L_FORTRAN;
        case IDM_LANG_BASH :
            return L_BASH;
        case IDM_LANG_FLASH :
            return L_FLASH;
		case IDM_LANG_NSIS :
            return L_NSIS;
		case IDM_LANG_TCL :
            return L_TCL;
		case IDM_LANG_LISP :
			return L_LISP;
		case IDM_LANG_SCHEME :
			return L_SCHEME;
		case IDM_LANG_ASM :
            return L_ASM;
		case IDM_LANG_DIFF :
            return L_DIFF;
		case IDM_LANG_PROPS :
            return L_PROPS;
		case IDM_LANG_PS:
            return L_PS;
		case IDM_LANG_RUBY:
            return L_RUBY;
		case IDM_LANG_SMALLTALK:
            return L_SMALLTALK;
		case IDM_LANG_VHDL :
            return L_VHDL;
        case IDM_LANG_KIX :
            return L_KIX;
        case IDM_LANG_CAML :
            return L_CAML;
        case IDM_LANG_ADA :
            return L_ADA;
        case IDM_LANG_VERILOG :
            return L_VERILOG;
		case IDM_LANG_MATLAB :
            return L_MATLAB;
		case IDM_LANG_HASKELL :
            return L_HASKELL;
        case IDM_LANG_AU3 :
            return L_AU3;
		case IDM_LANG_INNO :
            return L_INNO;
		case IDM_LANG_CMAKE :
            return L_CMAKE;
		case IDM_LANG_YAML :
			return L_YAML;
        case IDM_LANG_COBOL :
            return L_COBOL;
        case IDM_LANG_D :
            return L_D;
        case IDM_LANG_GUI4CLI :
            return L_GUI4CLI;
        case IDM_LANG_POWERSHELL :
            return L_POWERSHELL;
        case IDM_LANG_R :
            return L_R;
		case IDM_LANG_COFFEESCRIPT :
            return L_COFFEESCRIPT;

		case IDM_LANG_USER :
            return L_USER;
		default: {
			if (cmdID >= IDM_LANG_USER && cmdID <= IDM_LANG_USER_LIMIT) {
				return L_USER;
			}
			break; }
	}
	return L_EXTERNAL;
}


void Notepad_plus::setTitle()
{
	const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();
	//Get the buffer
	Buffer * buf = _pEditView->getCurrentBuffer();

	generic_string result = TEXT("");
	if (buf->isDirty())
	{
		result += TEXT("*");
	}

	if (nppGUI._shortTitlebar)
	{
		result += buf->getFileName();
	}
	else
	{
		result += buf->getFullPathName();
	}
	result += TEXT(" - ");
	result += _pPublicInterface->getClassName();

	if(_isAdministrator)
		result += TEXT(" [Administrator]");

	::SendMessage(_pPublicInterface->getHSelf(), WM_SETTEXT, 0, (LPARAM)result.c_str());
}

void Notepad_plus::activateNextDoc(bool direction)
{
	int nbDoc = _pDocTab->nbItem();

    int curIndex = _pDocTab->getCurrentTabIndex();
    curIndex += (direction == dirUp)?-1:1;

	if (curIndex >= nbDoc)
	{
		if (viewVisible(otherView()))
			switchEditViewTo(otherView());
		curIndex = 0;
	}
	else if (curIndex < 0)
	{
		if (viewVisible(otherView()))
		{
			switchEditViewTo(otherView());
			nbDoc = _pDocTab->nbItem();
		}
		curIndex = nbDoc - 1;
	}

	BufferID id = _pDocTab->getBufferByIndex(curIndex);
	activateBuffer(id, currentView());
}

void Notepad_plus::activateDoc(int pos)
{
	int nbDoc = _pDocTab->nbItem();
	if (pos == _pDocTab->getCurrentTabIndex())
	{
		Buffer * buf = _pEditView->getCurrentBuffer();
		buf->increaseRecentTag();
		return;
	}

	if (pos >= 0 && pos < nbDoc)
	{
		BufferID id = _pDocTab->getBufferByIndex(pos);
		activateBuffer(id, currentView());
	}
}


static const char utflen[] = {1,1,2,3};

size_t Notepad_plus::getSelectedCharNumber(UniMode u)
{
	size_t result = 0;
	int numSel = _pEditView->execute(SCI_GETSELECTIONS);
	if (u == uniUTF8 || u == uniCookie)
	{
		for (int i=0; i < numSel; ++i)
		{
			size_t line1 = _pEditView->execute(SCI_LINEFROMPOSITION, _pEditView->execute(SCI_GETSELECTIONNSTART, i));
			size_t line2 = _pEditView->execute(SCI_LINEFROMPOSITION, _pEditView->execute(SCI_GETSELECTIONNEND, i));
			for (size_t j = line1; j <= line2; ++j)
			{
				size_t stpos = _pEditView->execute(SCI_GETLINESELSTARTPOSITION, j);
				if (stpos != INVALID_POSITION)
				{
					size_t endpos = _pEditView->execute(SCI_GETLINESELENDPOSITION, j);
					for (size_t pos = stpos; pos < endpos; ++pos)
					{
						unsigned char c = 0xf0 & (unsigned char)_pEditView->execute(SCI_GETCHARAT, pos);
						if (c >= 0xc0)
							pos += utflen[(c & 0x30) >>  4];
						++result;
					}
				}
			}
		}
	}
	else
	{
		for (int i=0; i < numSel; ++i)
		{
			size_t stpos = _pEditView->execute(SCI_GETSELECTIONNSTART, i);
			size_t endpos = _pEditView->execute(SCI_GETSELECTIONNEND, i);
			result += (endpos - stpos);
			size_t line1 = _pEditView->execute(SCI_LINEFROMPOSITION, stpos);
			size_t line2 = _pEditView->execute(SCI_LINEFROMPOSITION, endpos);
			line2 -= line1;
			if (_pEditView->execute(SCI_GETEOLMODE) == SC_EOL_CRLF) line2 *= 2;
			result -= line2;
		}
		if (u != uni8Bit && u != uni7Bit) result *= 2;
	}
	return result;
}


#ifdef _OPENMP
#include <omp.h>
#endif
static inline size_t countUtf8Characters(unsigned char *buf, int pos, int endpos)
{
	size_t result = 0;
	while(pos < endpos)
	{
		unsigned char c = buf[pos++];
		if ((c&0xc0) == 0x80 // do not count unexpected continuation bytes (this handles the case where an UTF-8 character is split in the middle)
			|| c == '\n' || c == '\r') continue; // do not count end of lines
		if (c >= 0xc0) pos += utflen[(c & 0x30) >>  4];
		++result;
	}
	return result;
}


size_t Notepad_plus::getCurrentDocCharCount(UniMode u)
{
	if (u != uniUTF8 && u != uniCookie)
	{
		size_t numLines = _pEditView->execute(SCI_GETLINECOUNT);
		int result = _pEditView->execute(SCI_GETLENGTH);
		size_t lines = numLines==0?0:numLines-1;
		if (_pEditView->execute(SCI_GETEOLMODE) == SC_EOL_CRLF) lines *= 2;
		result -= lines;
		return ((int)result < 0)?0:result;
	}
 	else
 	{
		// Note that counting is not well defined for invalid UTF-8 characters.
		// This method is O(filelength) regardless of the number of characters we count (due to SCI_GETCHARACTERPOINTER);
		// it would not be appropriate for counting characters in a small selection.
		size_t result = 0;

		size_t endpos = _pEditView->execute(SCI_GETLENGTH);
		unsigned char* buf = (unsigned char*)_pEditView->execute(SCI_GETCHARACTERPOINTER); // Scintilla doc said the pointer can be invalidated by any other "execute"

#ifdef _OPENMP // parallel counting of characters with OpenMP
		if(endpos > 50000) // starting threads takes time; for small files it is better to simply count in one thread
		{
			#pragma omp parallel reduction(+: result)
			{
				// split in chunks of same size (except last chunk if it's not evenly divisible)
				unsigned int num_threads = omp_get_num_threads();
				unsigned int thread_num = omp_get_thread_num();
				size_t chunk_size = endpos/num_threads;
				size_t pos = chunk_size*thread_num;
				size_t endpos_local = (thread_num == num_threads-1) ? endpos : pos+chunk_size;
				result = countUtf8Characters(buf, pos, endpos_local);
			}
		}
		else
#endif
		{
			result = countUtf8Characters(buf, 0, endpos);
		}
 		return result;
 	}
}


bool Notepad_plus::isFormatUnicode(UniMode u)
{
	return (u != uni8Bit && u != uni7Bit && u != uniUTF8 && u != uniCookie);
}

int Notepad_plus::getBOMSize(UniMode u)
{
	switch(u)
	{
		case uni16LE:
		case uni16BE:
			return 2;
		case uniUTF8:
			return 3;
		default:
			return 0;
	}
}

int Notepad_plus::getSelectedAreas()
{
	int numSel = _pEditView->execute(SCI_GETSELECTIONS);
	if (numSel == 1) // either 0 or 1 selection
		return (_pEditView->execute(SCI_GETSELECTIONNSTART, 0) == _pEditView->execute(SCI_GETSELECTIONNEND, 0)) ? 0 : 1;
	return (_pEditView->execute(SCI_SELECTIONISRECTANGLE)) ? 1 : numSel;
}

size_t Notepad_plus::getSelectedBytes()
{
	int numSel = _pEditView->execute(SCI_GETSELECTIONS);
	size_t result = 0;
	for (int i = 0; i < numSel; ++i)
		result += (_pEditView->execute(SCI_GETSELECTIONNEND, i) - _pEditView->execute(SCI_GETSELECTIONNSTART, i));
	return result;
}

int Notepad_plus::wordCount()
{
    FindOption env;
    env._str2Search = TEXT("[^ 	\\\\.,;:!?()+\\-\\*/=\\]\\[{}&~\"'`|@$%<>\\^]+");
    env._searchType = FindRegex;
    return _findReplaceDlg.processAll(ProcessCountAll, &env, true);
}


void Notepad_plus::updateStatusBar()
{
    TCHAR strLnCol[128];
	TCHAR strSel[64];
	int selByte = 0;
	int selLine = 0;

	_pEditView->getSelectedCount(selByte, selLine);

	long selected_length = _pEditView->getSelectedLength();
	if (selected_length != -1)
		wsprintf(strSel, TEXT("Sel : %d | %d"), selected_length, selLine);
	else
		wsprintf(strSel, TEXT("Sel : %s"), TEXT("N/A"));

    wsprintf(strLnCol, TEXT("Ln : %d    Col : %d    %s"),\
        (_pEditView->getCurrentLineNumber() + 1), \
        (_pEditView->getCurrentColumnNumber() + 1),\
        strSel);

    _statusBar.setText(strLnCol, STATUSBAR_CUR_POS);

    TCHAR strDocLen[256];
	wsprintf(strDocLen, TEXT("length : %d    lines : %d"), _pEditView->getCurrentDocLen(), _pEditView->execute(SCI_GETLINECOUNT));
    _statusBar.setText(strDocLen, STATUSBAR_DOC_SIZE);
    _statusBar.setText(_pEditView->execute(SCI_GETOVERTYPE) ? TEXT("OVR") : TEXT("INS"), STATUSBAR_TYPING_MODE);
}

void Notepad_plus::dropFiles(HDROP hdrop)
{
	if (hdrop)
	{
		// Determinate in which view the file(s) is (are) dropped
		POINT p;
		::DragQueryPoint(hdrop, &p);
		HWND hWin = ::RealChildWindowFromPoint(_pPublicInterface->getHSelf(), p);
		if (!hWin) return;

		if ((_subEditView.getHSelf() == hWin) || (_subDocTab.getHSelf() == hWin))
			switchEditViewTo(SUB_VIEW);
		else
			switchEditViewTo(MAIN_VIEW);

		int filesDropped = ::DragQueryFile(hdrop, 0xffffffff, NULL, 0);
		BufferID lastOpened = BUFFER_INVALID;
		for (int i = 0 ; i < filesDropped ; ++i)
		{
			TCHAR pathDropped[MAX_PATH];
			::DragQueryFile(hdrop, i, pathDropped, MAX_PATH);
			BufferID test = doOpen(pathDropped);
			if (test != BUFFER_INVALID)
				lastOpened = test;
            //setLangStatus(_pEditView->getCurrentDocType());
		}
		if (lastOpened != BUFFER_INVALID) {
			switchToFile(lastOpened);
		}
		::DragFinish(hdrop);
		// Put Notepad_plus to forefront
		// May not work for Win2k, but OK for lower versions
		// Note: how to drop a file to an iconic window?
		// Actually, it is the Send To command that generates a drop.
		if (::IsIconic(_pPublicInterface->getHSelf()))
		{
			::ShowWindow(_pPublicInterface->getHSelf(), SW_RESTORE);
		}
		::SetForegroundWindow(_pPublicInterface->getHSelf());
	}
}

void Notepad_plus::checkModifiedDocument()
{
	//this will trigger buffer updates. If the status changes, Notepad++ will be informed and can do its magic
	MainFileManager->checkFilesystemChanges();
}

void Notepad_plus::getMainClientRect(RECT &rc) const
{
    _pPublicInterface->getClientRect(rc);
	rc.top += _rebarTop.getHeight();
	rc.bottom -= rc.top + _rebarBottom.getHeight() + _statusBar.getHeight();
}

void Notepad_plus::showView(int whichOne) {
	if (viewVisible(whichOne))	//no use making visible view visible
		return;

	if (_mainWindowStatus & WindowUserActive) {
		 _pMainSplitter->setWin0(&_subSplitter);
		 _pMainWindow = _pMainSplitter;
	} else {
		_pMainWindow = &_subSplitter;
	}

	if (whichOne == MAIN_VIEW) {
		_mainEditView.display(true);
		_mainDocTab.display(true);
	} else if (whichOne == SUB_VIEW) {
		_subEditView.display(true);
		_subDocTab.display(true);
	}
	_pMainWindow->display(true);

	_mainWindowStatus |= (whichOne==MAIN_VIEW)?WindowMainActive:WindowSubActive;

	//Send sizing info to make windows fit
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
}

bool Notepad_plus::viewVisible(int whichOne) {
	int viewToCheck = (whichOne == SUB_VIEW?WindowSubActive:WindowMainActive);
	return (_mainWindowStatus & viewToCheck) != 0;
}

void Notepad_plus::hideCurrentView()
{
	hideView(currentView());
}

void Notepad_plus::hideView(int whichOne)
{
	if (!(bothActive()))	//cannot close if not both views visible
		return;

	Window * windowToSet = (whichOne == MAIN_VIEW)?&_subDocTab:&_mainDocTab;
	if (_mainWindowStatus & WindowUserActive)
	{
		_pMainSplitter->setWin0(windowToSet);
	}
	else // otherwise the main window is the spltter container that we just created
		_pMainWindow = windowToSet;

	_subSplitter.display(false);	//hide splitter
	//hide scintilla and doctab
	if (whichOne == MAIN_VIEW) {
		_mainEditView.display(false);
		_mainDocTab.display(false);
	} else if (whichOne == SUB_VIEW) {
		_subEditView.display(false);
		_subDocTab.display(false);
	}

	// resize the main window
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);

	switchEditViewTo(otherFromView(whichOne));
	int viewToDisable = (whichOne == SUB_VIEW?WindowSubActive:WindowMainActive);
	_mainWindowStatus &= ~viewToDisable;
}

bool Notepad_plus::loadStyles()
{
	NppParameters *pNppParam = NppParameters::getInstance();
	return pNppParam->reloadStylers();
}

bool Notepad_plus::canHideView(int whichOne)
{
	if (!viewVisible(whichOne))
		return false;	//cannot hide hidden view
	if (!bothActive())
		return false;	//cannot hide only window
	DocTabView * tabToCheck = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	Buffer * buf = MainFileManager->getBufferByID(tabToCheck->getBufferByIndex(0));
	bool canHide = ((tabToCheck->nbItem() == 1) && !buf->isDirty() && buf->isUntitled());
	return canHide;
}

void Notepad_plus::loadBufferIntoView(BufferID id, int whichOne, bool dontClose)
{
	DocTabView * tabToOpen = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	ScintillaEditView * viewToOpen = (whichOne == MAIN_VIEW)?&_mainEditView:&_subEditView;

	//check if buffer exists
	int index = tabToOpen->getIndexByBuffer(id);
	if (index != -1)	//already open, done
		return;

	BufferID idToClose = BUFFER_INVALID;
	//Check if the tab has a single clean buffer. Close it if so
	if (!dontClose && tabToOpen->nbItem() == 1)
	{
		idToClose = tabToOpen->getBufferByIndex(0);
		Buffer * buf = MainFileManager->getBufferByID(idToClose);
		if (buf->isDirty() || !buf->isUntitled())
		{
			idToClose = BUFFER_INVALID;
		}
	}

	MainFileManager->addBufferReference(id, viewToOpen);

	//close clean doc. Use special logic to prevent flicker of tab showing then hiding
	if (idToClose != BUFFER_INVALID)
	{
		tabToOpen->setBuffer(0, id);	//index 0 since only one open
		activateBuffer(id, whichOne);	//activate. DocTab already activated but not a problem
		MainFileManager->closeBuffer(idToClose, viewToOpen);	//delete the buffer
		if (_pFileSwitcherPanel)
			_pFileSwitcherPanel->closeItem((int)idToClose, whichOne);
	}
	else
	{
		tabToOpen->addBuffer(id);
	}
}

bool Notepad_plus::removeBufferFromView(BufferID id, int whichOne)
{
	DocTabView * tabToClose = (whichOne == MAIN_VIEW) ? &_mainDocTab : &_subDocTab;
	ScintillaEditView * viewToClose = (whichOne == MAIN_VIEW) ? &_mainEditView : &_subEditView;

	//check if buffer exists
	int index = tabToClose->getIndexByBuffer(id);
	if (index == -1)        //doesn't exist, done
		return false;

	Buffer * buf = MainFileManager->getBufferByID(id);

	//Cannot close doc if last and clean
	if (tabToClose->nbItem() == 1)
	{
		if (!buf->isDirty() && buf->isUntitled())
		{
			return false;
		}
	}

	int active = tabToClose->getCurrentTabIndex();
	if (active == index) //need an alternative (close real doc, put empty one back)
	{
		if (tabToClose->nbItem() == 1)  //need alternative doc, add new one. Use special logic to prevent flicker of adding new tab then closing other
		{
			BufferID newID = MainFileManager->newEmptyDocument();
			MainFileManager->addBufferReference(newID, viewToClose);
			tabToClose->setBuffer(0, newID);        //can safely use id 0, last (only) tab open
			activateBuffer(newID, whichOne);        //activate. DocTab already activated but not a problem
		}
		else
		{
			int toActivate = 0;
			//activate next doc, otherwise prev if not possible
			if (active == tabToClose->nbItem() - 1) //prev
			{
				toActivate = active - 1;
			}
			else
			{
				toActivate = active;    //activate the 'active' index. Since we remove the tab first, the indices shift (on the right side)
			}
			tabToClose->deletItemAt((size_t)index); //delete first
			activateBuffer(tabToClose->getBufferByIndex(toActivate), whichOne);     //then activate. The prevent jumpy tab behaviour
		}
	}
	else
	{
		tabToClose->deletItemAt((size_t)index);
	}

	MainFileManager->closeBuffer(id, viewToClose);
	return true;
}

int Notepad_plus::switchEditViewTo(int gid)
{
	if (currentView() == gid) {	//make sure focus is ok, then leave
		_pEditView->getFocus();	//set the focus
		return gid;
	}
	if (!viewVisible(gid))
		return currentView();	//cannot activate invisible view

	int oldView = currentView();
	int newView = otherView();

	_activeView = newView;
	//Good old switcheroo
	DocTabView * tempTab = _pDocTab;
	_pDocTab = _pNonDocTab;
	_pNonDocTab = tempTab;
	ScintillaEditView * tempView = _pEditView;
	_pEditView = _pNonEditView;
	_pNonEditView = tempView;

	_pEditView->beSwitched();
    _pEditView->getFocus();	//set the focus

	if (_pDocMap)
	{
		_pDocMap->initWrapMap();
	}

	// Before switching off, synchronize backup file
	MainFileManager->backupCurrentBuffer();

	notifyBufferActivated(_pEditView->getCurrentBufferID(), currentView());
	return oldView;
}

void Notepad_plus::dockUserDlg()
{
    if (!_pMainSplitter)
    {
        _pMainSplitter = new SplitterContainer;
		_pMainSplitter->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());

        Window *pWindow;
		if (_mainWindowStatus & (WindowMainActive | WindowSubActive))
            pWindow = &_subSplitter;
        else
            pWindow = _pDocTab;

        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45);
    }

    if (bothActive())
        _pMainSplitter->setWin0(&_subSplitter);
    else
        _pMainSplitter->setWin0(_pDocTab);

    _pMainSplitter->display();

    _mainWindowStatus |= WindowUserActive;
    _pMainWindow = _pMainSplitter;

	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
}

void Notepad_plus::undockUserDlg()
{
    // a cause de surchargement de "display"
    ::ShowWindow(_pMainSplitter->getHSelf(), SW_HIDE);

    if (bothActive())
        _pMainWindow = &_subSplitter;
    else
        _pMainWindow = _pDocTab;

    ::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);

    _mainWindowStatus &= ~WindowUserActive;
    (ScintillaEditView::getUserDefineDlg())->display();
}

void Notepad_plus::docOpenInNewInstance(FileTransferMode mode, int x, int y)
{
	BufferID bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	if (buf->isUntitled() || buf->isDirty())
		return;

	TCHAR nppName[MAX_PATH];
	::GetModuleFileName(NULL, nppName, MAX_PATH);
	generic_string command = TEXT("\"");
	command += nppName;
	command += TEXT("\"");

	command += TEXT(" \"$(FULL_CURRENT_PATH)\" -multiInst -nosession -x");
	TCHAR pX[10], pY[10];
	generic_itoa(x, pX, 10);
	generic_itoa(y, pY, 10);

	command += pX;
	command += TEXT(" -y");
	command += pY;

	Command cmd(command);
	cmd.run(_pPublicInterface->getHSelf());
	if (mode == TransferMove)
	{
		doClose(bufferID, currentView());
		if (noOpenedDoc())
			::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
	}
}

void Notepad_plus::docGotoAnotherEditView(FileTransferMode mode)
{
	// Test if it's only doc to transfer on the hidden view
	// If so then do nothing
	if (mode == TransferMove)
	{
		if (_pDocTab->nbItem() == 1)
		{
			ScintillaEditView *pOtherView = NULL;
			if (_pEditView == &_mainEditView)
			{
				pOtherView = &_subEditView;
			}
			else if (_pEditView == &_subEditView)
			{
				pOtherView = &_mainEditView;
			}
			else
				return;

			if (!pOtherView->isVisible())
				return;
		}
	}

	//First put the doc in the other view if not present (if it is, activate it).
	//Then if needed close in the original tab
	BufferID current = _pEditView->getCurrentBufferID();
	int viewToGo = otherView();
	int indexFound = _pNonDocTab->getIndexByBuffer(current);
	if (indexFound != -1)	//activate it
	{
		activateBuffer(current, otherView());
	}
	else	//open the document, also copying the position
	{
		loadBufferIntoView(current, viewToGo);
		Buffer * buf = MainFileManager->getBufferByID(current);
		_pEditView->saveCurrentPos();	//allow copying of position
		buf->setPosition(buf->getPosition(_pEditView), _pNonEditView);
		_pNonEditView->restoreCurrentPos();	//set position
		activateBuffer(current, viewToGo);
	}

	//Open the view if it was hidden
	int viewToOpen = (viewToGo == SUB_VIEW?WindowSubActive:WindowMainActive);
	if (!(_mainWindowStatus & viewToOpen))
	{
		showView(viewToGo);
	}

	//Close the document if we transfered the document instead of cloning it
	if (mode == TransferMove)
	{
		//just close the activate document, since thats the one we moved (no search)
		doClose(_pEditView->getCurrentBufferID(), currentView());
		/*
		if (noOpenedDoc())
			::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
		*/
	} // else it was cone, so leave it

	//Activate the other view since thats where the document went
	switchEditViewTo(viewToGo);
}

bool Notepad_plus::activateBuffer(BufferID id, int whichOne)
{
	bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
	if (isSnapshotMode)
	{
		// Before switching off, synchronize backup file
		MainFileManager->backupCurrentBuffer();
	}
	Buffer * pBuf = MainFileManager->getBufferByID(id);
	bool reload = pBuf->getNeedReload();
	if (reload)
	{
		MainFileManager->reloadBuffer(id);
		pBuf->setNeedReload(false);
	}
	if (whichOne == MAIN_VIEW)
	{
		if (_mainDocTab.activateBuffer(id))	//only activate if possible
			_mainEditView.activateBuffer(id);
		else
			return false;
	}
	else
	{
		if (_subDocTab.activateBuffer(id))
			_subEditView.activateBuffer(id);
		else
			return false;
	}

	if (reload)
	{
		performPostReload(whichOne);
	}

	notifyBufferActivated(id, whichOne);

	//scnN.nmhdr.code = NPPN_DOCSWITCHINGIN;		//superseeded by NPPN_BUFFERACTIVATED
	return true;
}

void Notepad_plus::performPostReload(int whichOne) {
	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();
	bool toEnd = (nppGUI._fileAutoDetection == cdAutoUpdateGo2end) || (nppGUI._fileAutoDetection == cdGo2end);
	if (!toEnd)
		return;
	if (whichOne == MAIN_VIEW) {
		_mainEditView.execute(SCI_GOTOLINE, _mainEditView.execute(SCI_GETLINECOUNT) -1);
	} else {
		_subEditView.execute(SCI_GOTOLINE, _subEditView.execute(SCI_GETLINECOUNT) -1);
	}
}

void Notepad_plus::bookmarkNext(bool forwardScan)
{
	int lineno = _pEditView->getCurrentLineNumber();
	int sci_marker = SCI_MARKERNEXT;
	int lineStart = lineno + 1;	//Scan starting from next line
	int lineRetry = 0;				//If not found, try from the beginning
	if (!forwardScan)
    {
		lineStart = lineno - 1;		//Scan starting from previous line
		lineRetry = int(_pEditView->execute(SCI_GETLINECOUNT));	//If not found, try from the end
		sci_marker = SCI_MARKERPREVIOUS;
	}
	int nextLine = int(_pEditView->execute(sci_marker, lineStart, 1 << MARK_BOOKMARK));
	if (nextLine < 0)
		nextLine = int(_pEditView->execute(sci_marker, lineRetry, 1 << MARK_BOOKMARK));

	if (nextLine < 0)
		return;

    _pEditView->execute(SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine);
	_pEditView->execute(SCI_GOTOLINE, nextLine);
}

void Notepad_plus::staticCheckMenuAndTB() const
{
	// Visibility of invisible characters
	bool wsTabShow = _pEditView->isInvisibleCharsShown();
	bool eolShow = _pEditView->isEolVisible();

	bool onlyWS = false;
	bool onlyEOL = false;
	bool bothWSEOL = false;
	if (wsTabShow)
	{
		if (eolShow)
		{
			bothWSEOL = true;
		}
		else
		{
			onlyWS = true;
		}
	}
	else if (eolShow)
	{
		onlyEOL = true;
	}

	checkMenuItem(IDM_VIEW_TAB_SPACE, onlyWS);
	checkMenuItem(IDM_VIEW_EOL, onlyEOL);
	checkMenuItem(IDM_VIEW_ALL_CHARACTERS, bothWSEOL);
	_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, bothWSEOL);

	// Visibility of the indentation guide line
	bool b = _pEditView->isShownIndentGuide();
	checkMenuItem(IDM_VIEW_INDENT_GUIDE, b);
	_toolBar.setCheck(IDM_VIEW_INDENT_GUIDE, b);


	// Wrap
	b = _pEditView->isWrap();
	checkMenuItem(IDM_VIEW_WRAP, b);
	_toolBar.setCheck(IDM_VIEW_WRAP, b);
	checkMenuItem(IDM_VIEW_WRAP_SYMBOL, _pEditView->isWrapSymbolVisible());
}

void Notepad_plus::dynamicCheckMenuAndTB() const
{
	//Format conversion
	enableConvertMenuItems(_pEditView->getCurrentBuffer()->getFormat());
	checkUnicodeMenuItems();
}

void Notepad_plus::enableConvertMenuItems(formatType f) const
{
	enableCommand(IDM_FORMAT_TODOS, (f != WIN_FORMAT), MENU);
	enableCommand(IDM_FORMAT_TOUNIX, (f != UNIX_FORMAT), MENU);
	enableCommand(IDM_FORMAT_TOMAC, (f != MAC_FORMAT), MENU);
}

void Notepad_plus::checkUnicodeMenuItems() const
{
	Buffer *buf = _pEditView->getCurrentBuffer();
	UniMode um = buf->getUnicodeMode();
	int encoding = buf->getEncoding();

	int id = -1;
	switch (um)
	{
		case uniUTF8   : id = IDM_FORMAT_UTF_8; break;
		case uni16BE   : id = IDM_FORMAT_UCS_2BE; break;
		case uni16LE   : id = IDM_FORMAT_UCS_2LE; break;
		case uniCookie : id = IDM_FORMAT_AS_UTF_8; break;
		case uni8Bit   : id = IDM_FORMAT_ANSI; break;
	}

	if (encoding == -1)
	{
		// Uncheck all in the sub encoding menu
		::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ENCODE, IDM_FORMAT_ENCODE_END, IDM_FORMAT_ENCODE, MF_BYCOMMAND);
		::CheckMenuItem(_mainMenuHandle, IDM_FORMAT_ENCODE, MF_UNCHECKED | MF_BYCOMMAND);

		if (id == -1) //um == uni16BE_NoBOM || um == uni16LE_NoBOM
		{
			// Uncheck all in the main encoding menu
			::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ANSI, IDM_FORMAT_AS_UTF_8, IDM_FORMAT_ANSI, MF_BYCOMMAND);
			::CheckMenuItem(_mainMenuHandle, IDM_FORMAT_ANSI, MF_UNCHECKED | MF_BYCOMMAND);
		}
		else
		{
			::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ANSI, IDM_FORMAT_AS_UTF_8, id, MF_BYCOMMAND);
		}
	}
	else
	{
		EncodingMapper *em = EncodingMapper::getInstance();
		int cmdID = em->getIndexFromEncoding(encoding);
		if (cmdID == -1)
		{
			//printStr(TEXT("Encoding problem. Encoding is not added in encoding_table?"));
			return;
		}
		cmdID += IDM_FORMAT_ENCODE;

		// Uncheck all in the main encoding menu
		::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ANSI, IDM_FORMAT_AS_UTF_8, IDM_FORMAT_ANSI, MF_BYCOMMAND);
		::CheckMenuItem(_mainMenuHandle, IDM_FORMAT_ANSI, MF_UNCHECKED | MF_BYCOMMAND);

		// Check the encoding item
		::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ENCODE, IDM_FORMAT_ENCODE_END, cmdID, MF_BYCOMMAND);
	}
}

void Notepad_plus::showAutoComp()
{
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showApiComplete();
}

void Notepad_plus::showPathCompletion()
{
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showPathCompletion();
}

void Notepad_plus::autoCompFromCurrentFile(bool autoInsert)
{
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showWordComplete(autoInsert);
}

void Notepad_plus::showFunctionComp()
{
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showFunctionComplete();
}

static generic_string extractSymbol(TCHAR firstChar, TCHAR secondChar, const TCHAR *str2extract)
{
	bool found = false;
	TCHAR extracted[128] = TEXT("");

	for (size_t i = 0, j = 0, len = lstrlen(str2extract) ; i < len ; ++i)
	{
		if (found)
		{
			if (!str2extract[i] || str2extract[i] == ' ')
			{
				extracted[j] = '\0';
				return generic_string(extracted);
			}
			extracted[j++] = str2extract[i];

		}
		else
		{
			if (!str2extract[i])
				return TEXT("");

			if (str2extract[i] == firstChar && str2extract[i+1] == secondChar)
			{
				found = true;
				++i;
			}
		}
	}
	return  generic_string(extracted);
};

bool Notepad_plus::doBlockComment(comment_mode currCommentMode)
{
	//--LS: BlockToStreamComment:
	const TCHAR *commentStart;
	const TCHAR *commentEnd;
	generic_string symbolStart;
	generic_string symbolEnd;

	const TCHAR *commentLineSybol;
	generic_string symbol;

	Buffer * buf = _pEditView->getCurrentBuffer();
	//--FLS: Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly()) 
		return false;
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance()->getULCFromName(buf->getUserDefineLangName());
		if (!userLangContainer)
			return false;

		symbol = extractSymbol('0', '0', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentLineSybol = symbol.c_str();
		//--FLS: BlockToStreamComment: Needed to decide, if stream-comment can be called below!
		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentLineSybol = buf->getCommentLineSymbol();
		//--FLS: BlockToStreamComment: Needed to decide, if stream-comment can be called below!
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	if ((!commentLineSybol) || (!commentLineSybol[0]) || (commentLineSybol == NULL)) 
	{
	//--FLS: BlockToStreamComment: If there is no block-comment symbol, try the stream comment:
		if (!(!commentStart || !commentStart[0] || commentStart == NULL || !commentEnd || !commentEnd[0] || commentEnd == NULL)) 
		{
			if ((currCommentMode == cm_comment)) 
			{
				return doStreamComment();
			}
			else if (currCommentMode == cm_uncomment) 
			{
				return undoStreamComment();
			}
			else 
				return false;
		}
		else
			return false;
	}

    generic_string comment(commentLineSybol);
    comment += TEXT(" ");

    size_t comment_length = comment.length();
    size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
    size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
    size_t caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
    // checking if caret is located in _beginning_ of selected block
    bool move_caret = caretPosition < selectionEnd;
    int selStartLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
    int selEndLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionEnd);
    int lines = selEndLine - selStartLine;
    size_t firstSelLineStart = _pEditView->execute(SCI_POSITIONFROMLINE, selStartLine);
    // "caret return" is part of the last selected line
    if ((lines > 0) && (selectionEnd == static_cast<size_t>(_pEditView->execute(SCI_POSITIONFROMLINE, selEndLine))))
		selEndLine--;
	//--FLS: count lines which were un-commented to decide if undoStreamComment() shall be called.
	int nUncomments = 0;
    _pEditView->execute(SCI_BEGINUNDOACTION);

    for (int i = selStartLine; i <= selEndLine; ++i)
	{
		int lineStart = _pEditView->execute(SCI_POSITIONFROMLINE, i);
        int lineIndent = lineStart;
        int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, i);

		size_t linebufferSize = lineEnd - lineStart + 2;
		TCHAR* linebuf = new TCHAR[linebufferSize];

		Lang *lang = _pEditView->getCurrentBuffer()->getCurrentLang();
		bool isFortran = lang == NULL?false:lang->_langID == L_FORTRAN;
		if (!isFortran)
			lineIndent = _pEditView->execute(SCI_GETLINEINDENTPOSITION, i);
		_pEditView->getGenericText(linebuf, linebufferSize, lineIndent, lineEnd);

        generic_string linebufStr = linebuf;
		delete [] linebuf;

        // empty lines are not commented
        if (linebufStr.length() < 1)
			continue;

   		if (currCommentMode != cm_comment)
		{
			//--FLS: In order to do get case insensitive comparison use strnicmp() instead case-sensitive comparison.
			//      Case insensitive comparison is needed e.g. for "REM" and "rem" in Batchfiles.
			//if (linebufStr.substr(0, comment_length - 1) == comment.substr(0, comment_length - 1))
			if (generic_strnicmp(linebufStr.c_str(), comment.c_str(), comment_length -1) == 0)
			{
				int len = (generic_strnicmp(linebufStr.substr(0, comment_length).c_str(),comment.c_str(), comment_length) == 0)?comment_length:comment_length - 1;

				_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + len);
					_pEditView->replaceSelWith("");

					if (i == selStartLine) // is this the first selected line?
					selectionStart -= len;
				selectionEnd -= len; // every iteration
				++nUncomments;
					continue;
			}
		}
		if ((currCommentMode == cm_toggle) || (currCommentMode == cm_comment))
		{
			if (i == selStartLine) // is this the first selected line?
				selectionStart += comment_length;
			selectionEnd += comment_length; // every iteration
			_pEditView->insertGenericTextFrom(lineIndent, comment.c_str());
		}
     }
    // after uncommenting selection may promote itself to the lines
    // before the first initially selected line;
    // another problem - if only comment symbol was selected;
    if (selectionStart < firstSelLineStart)
	{
        if (selectionStart >= selectionEnd - (comment_length - 1))
			selectionEnd = firstSelLineStart;
        selectionStart = firstSelLineStart;
    }
    if (move_caret)
	{
        // moving caret to the beginning of selected block
        _pEditView->execute(SCI_GOTOPOS, selectionEnd);
        _pEditView->execute(SCI_SETCURRENTPOS, selectionStart);
    }
	else
	{
        _pEditView->execute(SCI_SETSEL, selectionStart, selectionEnd);
    }
    _pEditView->execute(SCI_ENDUNDOACTION);
	
	//--FLS: undoStreamComment: If there were no block-comments to un-comment try uncommenting of stream-comment.
	if ((currCommentMode == cm_uncomment) && (nUncomments == 0)) {
		return undoStreamComment();
	}
    return true;
}

bool Notepad_plus::doStreamComment()
{
	const TCHAR *commentStart;
	const TCHAR *commentEnd;

	generic_string symbolStart;
	generic_string symbolEnd;

	//--FLS: BlockToStreamComment:
	const TCHAR *commentLineSybol;
	generic_string symbol;

	Buffer * buf = _pEditView->getCurrentBuffer();
	//--FLS: Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly()) 
		return false;
		
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance()->getULCFromName(buf->getUserDefineLangName());

		if (!userLangContainer)
			return false;

		//--FLS: BlockToStreamComment: Next two lines needed to decide, if block-comment can be called below!
		symbol = extractSymbol('0', '0', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentLineSybol = symbol.c_str();

		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		//--FLS: BlockToStreamComment: Next line needed to decide, if block-comment can be called below!
		commentLineSybol = buf->getCommentLineSymbol();
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	// if ((!commentStart) || (!commentStart[0]))
	// 		return false;
	// if ((!commentEnd) || (!commentEnd[0]))
	// 		return false;
	//--FLS: BlockToStreamComment: If there is no stream-comment symbol, try the block comment:
	if ((!commentStart) || (!commentStart[0]) || (commentStart == NULL) || (!commentEnd) || (!commentEnd[0]) || (commentEnd == NULL)) {
		if (!(!commentLineSybol || !commentLineSybol[0] || commentLineSybol == NULL)) 
			return doBlockComment(cm_comment);
		else
		return false;
	}

	generic_string start_comment(commentStart);
	generic_string end_comment(commentEnd);
	generic_string white_space(TEXT(" "));

	start_comment += white_space;
	white_space += end_comment;
	end_comment = white_space;
	size_t start_comment_length = start_comment.length();
	size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
	size_t caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
	// checking if caret is located in _beginning_ of selected block
	bool move_caret = caretPosition < selectionEnd;

	// if there is no selection?
	if (selectionEnd - selectionStart <= 0)
	{
		int selLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
		selectionStart = _pEditView->execute(SCI_GETLINEINDENTPOSITION, selLine);
		selectionEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, selLine);
	}
	_pEditView->execute(SCI_BEGINUNDOACTION);
	_pEditView->insertGenericTextFrom(selectionStart, start_comment.c_str());
	selectionEnd += start_comment_length;
	selectionStart += start_comment_length;
	_pEditView->insertGenericTextFrom(selectionEnd, end_comment.c_str());
	if (move_caret)
	{
		// moving caret to the beginning of selected block
		_pEditView->execute(SCI_GOTOPOS, selectionEnd);
		_pEditView->execute(SCI_SETCURRENTPOS, selectionStart);
	}
	else
	{
		_pEditView->execute(SCI_SETSEL, selectionStart, selectionEnd);
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
	return true;
}

bool Notepad_plus::saveScintillaParams()
{
	NppParameters * pNppParam = NppParameters::getInstance();
	ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP();
	svp._zoom = int(_mainEditView.execute(SCI_GETZOOM));
	svp._zoom2 = int(_subEditView.execute(SCI_GETZOOM));
	return (NppParameters::getInstance())->writeScintillaParams(svp);
}

bool Notepad_plus::addCurrentMacro()
{
	vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();

	int nbMacro = theMacros.size();

	int cmdID = ID_MACRO + nbMacro;
	MacroShortcut ms(Shortcut(), _macro, cmdID);
	ms.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());

	if (ms.doDialog() != -1)
	{
		HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
		int const posBase = 6;	//separator at index 5
		if (nbMacro == 0)
		{
			::InsertMenu(hMacroMenu, posBase-1, MF_BYPOSITION, (unsigned int)-1, 0);	//no separator yet, add one

            // Insert the separator and modify/delete command
			::InsertMenu(hMacroMenu, posBase + nbMacro + 1, MF_BYPOSITION, (unsigned int)-1, 0);

			NativeLangSpeaker *pNativeLangSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
			generic_string nativeLangShortcutMapperMacro = pNativeLangSpeaker->getNativeLangMenuString(IDM_SETTING_SHORTCUT_MAPPER_MACRO);
			if (nativeLangShortcutMapperMacro == TEXT(""))
				nativeLangShortcutMapperMacro = TEXT("Modify Shortcut/Delete Macro...");

			::InsertMenu(hMacroMenu, posBase + nbMacro + 2, MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_MACRO, nativeLangShortcutMapperMacro.c_str());
        }
		theMacros.push_back(ms);
		::InsertMenu(hMacroMenu, posBase + nbMacro, MF_BYPOSITION, cmdID, ms.toMenuItemString().c_str());
		_accelerator.updateShortcuts();
		return true;
	}
	return false;
}

void Notepad_plus::changeToolBarIcons()
{
    _toolBar.changeIcons();
}

bool Notepad_plus::switchToFile(BufferID id)
{
	int i = 0;
	int iView = currentView();
	if (id == BUFFER_INVALID)
		return false;

	if ((i = _pDocTab->getIndexByBuffer(id)) != -1)
	{
		iView = currentView();
	}
	else if ((i = _pNonDocTab->getIndexByBuffer(id)) != -1)
	{
		iView = otherView();
	}

	if (i != -1)
	{
		switchEditViewTo(iView);
		activateBuffer(id, currentView());
		return true;
	}
	return false;
}

void Notepad_plus::getTaskListInfo(TaskListInfo *tli)
{
	size_t currentNbDoc = _pDocTab->nbItem();
	size_t nonCurrentNbDoc = _pNonDocTab->nbItem();

	tli->_currentIndex = 0;

	if (!viewVisible(otherView()))
		nonCurrentNbDoc = 0;

	for (size_t i = 0 ; i < currentNbDoc ; ++i)
	{
		BufferID bufID = _pDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager->getBufferByID(bufID);
		int status = b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(currentView(), i, b->getFullPathName(), status, (void *)bufID));
	}
	for (size_t i = 0 ; i < nonCurrentNbDoc ; ++i)
	{
		BufferID bufID = _pNonDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager->getBufferByID(bufID);
		int status = b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(otherView(), i, b->getFullPathName(), status, (void *)bufID));
	}
}


bool Notepad_plus::goToPreviousIndicator(int indicID2Search, bool isWrap) const
{
    int position = _pEditView->execute(SCI_GETCURRENTPOS);
	int docLen = _pEditView->getCurrentDocLen();

    BOOL isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  position);
    int posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  position);
    int posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search,  position);

	// pre-condition
	if ((posStart == 0) && (posEnd == docLen - 1))
		return false;

    if (posStart <= 0)
	{
		if (!isWrap)
			return false;

		isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  docLen - 1);
		posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  docLen - 1);
	}

    if (isInIndicator) // try to get out of indicator
    {
        posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search, posStart - 1);
        if (posStart <= 0)
		{
			if (!isWrap)
				return false;
			posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  docLen - 1);
		}
	}

    int newPos = posStart - 1;
    posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search, newPos);
    posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, newPos);

	// found
	if (_pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search, posStart))
	{
		NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
		nppGUI._disableSmartHiliteTmp = true;

        int currentline = _pEditView->execute(SCI_LINEFROMPOSITION, posEnd);
	    _pEditView->execute(SCI_ENSUREVISIBLE, currentline);	// make sure target line is unfolded

		_pEditView->execute(SCI_SETSEL, posEnd, posStart);
		_pEditView->execute(SCI_SCROLLCARET);
		return true;
	}
	return false;
}

bool Notepad_plus::goToNextIndicator(int indicID2Search, bool isWrap) const
{
    int position = _pEditView->execute(SCI_GETCURRENTPOS);
	int docLen = _pEditView->getCurrentDocLen();

    BOOL isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  position);
    int posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  position);
    int posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search,  position);

	// pre-condition
	if ((posStart == 0) && (posEnd == docLen - 1))
		return false;

    if (posEnd >= docLen)
	{
		if (!isWrap)
			return false;

		isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  0);
		posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, 0);
	}

    if (isInIndicator) // try to get out of indicator
    {
        posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, posEnd);

        if (posEnd >= docLen)
		{
			if (!isWrap)
				return false;
			posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, 0);
		}
    }
    int newPos = posEnd;
    posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search, newPos);
    posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, newPos);

	// found
	if (_pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search, posStart))
	{
		NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
		nppGUI._disableSmartHiliteTmp = true;

        int currentline = _pEditView->execute(SCI_LINEFROMPOSITION, posEnd);
	    _pEditView->execute(SCI_ENSUREVISIBLE, currentline);	// make sure target line is unfolded

		_pEditView->execute(SCI_SETSEL, posStart, posEnd);
		_pEditView->execute(SCI_SCROLLCARET);
		return true;
	}
	return false;
}

void Notepad_plus::fullScreenToggle()
{
	if (!_beforeSpecialView.isFullScreen)	//toggle fullscreen on
	{
		_beforeSpecialView._winPlace.length = sizeof(_beforeSpecialView._winPlace);
		::GetWindowPlacement(_pPublicInterface->getHSelf(), &_beforeSpecialView._winPlace);

		RECT fullscreenArea;		//RECT used to calculate window fullscreen size
		//Preset view area, in case something fails, primary monitor values
		fullscreenArea.top = 0;
		fullscreenArea.left = 0;
		fullscreenArea.right = GetSystemMetrics(SM_CXSCREEN);
		fullscreenArea.bottom = GetSystemMetrics(SM_CYSCREEN);

		//if (_winVersion != WV_NT)
		{
			HMONITOR currentMonitor;	//Handle to monitor where fullscreen should go
			MONITORINFO mi;				//Info of that monitor
			//Caution, this will not work on windows 95, so probably add some checking of some sorts like Unicode checks, IF 95 were to be supported
			currentMonitor = ::MonitorFromWindow(_pPublicInterface->getHSelf(), MONITOR_DEFAULTTONEAREST);	//should always be valid monitor handle
			mi.cbSize = sizeof(MONITORINFO);
			if (::GetMonitorInfo(currentMonitor, &mi) != FALSE)
			{
				fullscreenArea = mi.rcMonitor;
				fullscreenArea.right -= fullscreenArea.left;
				fullscreenArea.bottom -= fullscreenArea.top;
			}
		}

		//Setup GUI
        int bs = buttonStatus_fullscreen;
		if (_beforeSpecialView.isPostIt)
        {
            bs |= buttonStatus_postit;
        }
        else
		{
			//only change the GUI if not already done by postit
			_beforeSpecialView.isMenuShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISMENUHIDDEN, 0, 0) != TRUE;
			if (_beforeSpecialView.isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, TRUE);

			//Hide rebar
			_rebarTop.display(false);
			_rebarBottom.display(false);
		}
        _restoreButton.setButtonStatus(bs);

		//Hide window so windows can properly update it
		::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);

		//Set popup style for fullscreen window and store the old style
		if (!_beforeSpecialView.isPostIt)
		{
			_beforeSpecialView.preStyle = ::SetWindowLongPtr(_pPublicInterface->getHSelf(), GWL_STYLE, WS_POPUP);
			if (!_beforeSpecialView.preStyle) {	//something went wrong, use default settings
				_beforeSpecialView.preStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
			}
		}

		//Set fullscreen window, highest non-top z-order, show the window and redraw it (refreshing the windowmanager cache aswell)
		::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		::SetWindowPos(_pPublicInterface->getHSelf(), HWND_TOP, fullscreenArea.left, fullscreenArea.top, fullscreenArea.right, fullscreenArea.bottom, SWP_NOZORDER|SWP_DRAWFRAME|SWP_FRAMECHANGED);
		::SetForegroundWindow(_pPublicInterface->getHSelf());

        // show restore button
        _restoreButton.doDialog(_nativeLangSpeaker.isRTL());

        RECT rect;
        GetWindowRect(_restoreButton.getHSelf(), &rect);
	    int w = rect.right - rect.left;
	    int h = rect.bottom - rect.top;

        RECT nppRect;
        GetWindowRect(_pPublicInterface->getHSelf(), &nppRect);
        int x = nppRect.right - w;
        int y = nppRect.top;
        ::MoveWindow(_restoreButton.getHSelf(), x, y, w, h, FALSE);

        _pEditView->getFocus();
	}
	else	//toggle fullscreen off
	{
		//Hide window for updating, restore style and menu then restore position and Z-Order
		::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);

        _restoreButton.setButtonStatus(buttonStatus_fullscreen ^ _restoreButton.getButtonStatus());
        _restoreButton.display(false);

		//Setup GUI
		if (!_beforeSpecialView.isPostIt)
		{
			//only change the GUI if postit isnt active
			if (_beforeSpecialView.isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, FALSE);

			//Show rebar
			_rebarTop.display(true);
			_rebarBottom.display(true);
		}

		//Set old style if not fullscreen
		if (!_beforeSpecialView.isPostIt)
		{
			::SetWindowLongPtr( _pPublicInterface->getHSelf(), GWL_STYLE, _beforeSpecialView.preStyle);
			//Redraw the window and refresh windowmanager cache, dont do anything else, sizing is done later on
			::SetWindowPos(_pPublicInterface->getHSelf(), HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_DRAWFRAME|SWP_FRAMECHANGED);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		}

		if (_beforeSpecialView._winPlace.length)
		{
			if (_beforeSpecialView._winPlace.showCmd == SW_SHOWMAXIMIZED)
			{
				::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOWMAXIMIZED);
			}
			else
			{
				::SetWindowPlacement(_pPublicInterface->getHSelf(), &_beforeSpecialView._winPlace);
			}
		}
		else	//fallback
		{
			::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		}
	}
	//::SetForegroundWindow(_pPublicInterface->getHSelf());
	_beforeSpecialView.isFullScreen = !_beforeSpecialView.isFullScreen;
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
    if (_beforeSpecialView.isPostIt)
    {
        // show restore button on the right position
        RECT rect;
        GetWindowRect(_restoreButton.getHSelf(), &rect);
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;

        RECT nppRect;
        GetWindowRect(_pPublicInterface->getHSelf(), &nppRect);
        int x = nppRect.right - w - w;
        int y = nppRect.top + 1;
        ::MoveWindow(_restoreButton.getHSelf(), x, y, w, h, FALSE);
    }
}

void Notepad_plus::postItToggle()
{
	NppParameters * pNppParam = NppParameters::getInstance();
	if (!_beforeSpecialView.isPostIt)	// PostIt disabled, enable it
	{
		NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
		// get current status before switch to postIt
		//check these always
		{
			_beforeSpecialView.isAlwaysOnTop = ::GetMenuState(_mainMenuHandle, IDM_VIEW_ALWAYSONTOP, MF_BYCOMMAND) == MF_CHECKED;
			_beforeSpecialView.isTabbarShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISTABBARHIDDEN, 0, 0) != TRUE;
			_beforeSpecialView.isStatusbarShown = nppGUI._statusBarShow;
			if (nppGUI._statusBarShow)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDESTATUSBAR, 0, TRUE);
			if (_beforeSpecialView.isTabbarShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDETABBAR, 0, TRUE);
			if (!_beforeSpecialView.isAlwaysOnTop)
				::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);
		}
		//Only check these if not fullscreen
        int bs = buttonStatus_postit;
		if (_beforeSpecialView.isFullScreen)
        {
            bs |= buttonStatus_fullscreen;
        }
        else
		{
			_beforeSpecialView.isMenuShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISMENUHIDDEN, 0, 0) != TRUE;
			if (_beforeSpecialView.isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, TRUE);

			//Hide rebar
			_rebarTop.display(false);
			_rebarBottom.display(false);
		}
        _restoreButton.setButtonStatus(bs);

		// PostIt!

		//Set popup style for fullscreen window and store the old style
		if (!_beforeSpecialView.isFullScreen)
		{
			//Hide window so windows can properly update it
			::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);
			_beforeSpecialView.preStyle = ::SetWindowLongPtr( _pPublicInterface->getHSelf(), GWL_STYLE, WS_POPUP );
			if (!_beforeSpecialView.preStyle) {	//something went wrong, use default settings
				_beforeSpecialView.preStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
			}
			//Redraw the window and refresh windowmanager cache, dont do anything else, sizing is done later on
			::SetWindowPos(_pPublicInterface->getHSelf(), HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_DRAWFRAME|SWP_FRAMECHANGED);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		}

        // show restore button
        _restoreButton.doDialog(_nativeLangSpeaker.isRTL());

        RECT rect;
        GetWindowRect(_restoreButton.getHSelf(), &rect);
	    int w = rect.right - rect.left;
	    int h = rect.bottom - rect.top;

        RECT nppRect;
        GetWindowRect(_pPublicInterface->getHSelf(), &nppRect);
        int x = nppRect.right - w - w;
        int y = nppRect.top + 1;
        ::MoveWindow(_restoreButton.getHSelf(), x, y, w, h, FALSE);

        _pEditView->getFocus();
	}
	else	//PostIt enabled, disable it
	{
        _restoreButton.setButtonStatus(buttonStatus_postit ^ _restoreButton.getButtonStatus());
        _restoreButton.display(false);

		//Setup GUI
		if (!_beforeSpecialView.isFullScreen)
		{
			//only change the these parts of GUI if not already done by fullscreen
			if (_beforeSpecialView.isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, FALSE);

			//Show rebar
			_rebarTop.display(true);
			_rebarBottom.display(true);
		}
		//Do this GUI config always
		if (_beforeSpecialView.isStatusbarShown)
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDESTATUSBAR, 0, FALSE);
		if (_beforeSpecialView.isTabbarShown)
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDETABBAR, 0, FALSE);
		if (!_beforeSpecialView.isAlwaysOnTop)
			::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);

		//restore window style if not fullscreen
		if (!_beforeSpecialView.isFullScreen)
		{
			//dwStyle |= (WS_CAPTION | WS_SIZEBOX);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);
			::SetWindowLongPtr(_pPublicInterface->getHSelf(), GWL_STYLE, _beforeSpecialView.preStyle);

			//Redraw the window and refresh windowmanager cache, dont do anything else, sizing is done later on
			::SetWindowPos(_pPublicInterface->getHSelf(), HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_DRAWFRAME|SWP_FRAMECHANGED);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		}
	}

	_beforeSpecialView.isPostIt = !_beforeSpecialView.isPostIt;
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
}

void Notepad_plus::doSynScorll(HWND whichView)
{
	int column = 0;
	int line = 0;
	ScintillaEditView *pView;

	// var for Line
	int mainCurrentLine, subCurrentLine;

	// var for Column
	int mxoffset, sxoffset;
	int pixel;
	int mainColumn, subColumn;

    if (whichView == _mainEditView.getHSelf())
	{
		if (_syncInfo._isSynScollV)
		{
			// Compute for Line
			mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
			subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
			line = mainCurrentLine - _syncInfo._line - subCurrentLine;
		}
		if (_syncInfo._isSynScollH)
		{
			// Compute for Column
			mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
			pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			subColumn = sxoffset/pixel;
			column = mainColumn - _syncInfo._column - subColumn;
		}
		pView = &_subEditView;
    }
    else if (whichView == _subEditView.getHSelf())
    {
		if (_syncInfo._isSynScollV)
		{
			// Compute for Line
			mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
			subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
			line = subCurrentLine + _syncInfo._line - mainCurrentLine;
		}
		if (_syncInfo._isSynScollH)
		{
			// Compute for Column
			mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
			pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			subColumn = sxoffset/pixel;
			column = subColumn + _syncInfo._column - mainColumn;
		}
		pView = &_mainEditView;
    }
    else
        return;

	pView->scroll(column, line);
}

bool Notepad_plus::getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible)
{
	DockingManagerData & dockingData = (DockingManagerData &)(NppParameters::getInstance())->getNppGUI()._dockingData;

	for (size_t i = 0, len = dockingData._pluginDockInfo.size(); i < len ; ++i)
	{
		const PluginDlgDockingInfo & pddi = dockingData._pluginDockInfo[i];

		if (!generic_stricmp(pddi._name.c_str(), dockData.pszModuleName) && (pddi._internalID == dockData.dlgID))
		{
			iCont				= pddi._currContainer;
			isVisible			= pddi._isVisible;
			dockData.iPrevCont	= pddi._prevContainer;

			if (dockData.iPrevCont != -1)
			{
				int cont = (pddi._currContainer < DOCKCONT_MAX ? pddi._prevContainer : pddi._currContainer);
				RECT rc;
				if (dockingData.getFloatingRCFrom(cont, rc))
					dockData.rcFloat = rc;
			}
			return true;
		}
	}
	return false;
}


void Notepad_plus::getCurrentOpenedFiles(Session & session, bool includUntitledDoc)
{
	_mainEditView.saveCurrentPos();	//save position so itll be correct in the session
	_subEditView.saveCurrentPos();	//both views
	session._activeView = currentView();
	session._activeMainIndex = _mainDocTab.getCurrentTabIndex();
	session._activeSubIndex = _subDocTab.getCurrentTabIndex();

	//Use _invisibleEditView to temporarily open documents to retrieve markers
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	const int nbElem = 2;
	DocTabView *docTab[nbElem];
	docTab[0] = &_mainDocTab;
	docTab[1] = &_subDocTab;
	for (size_t k = 0; k < nbElem; ++k)
	{
		for (int i = 0, len = docTab[k]->nbItem(); i < len ; ++i)
		{
			BufferID bufID = docTab[k]->getBufferByIndex(i);
			ScintillaEditView *editView = k == 0?&_mainEditView:&_subEditView;
			int activeIndex = int(k == 0?session._activeMainIndex:session._activeSubIndex);
			vector<sessionFileInfo> *viewFiles = (vector<sessionFileInfo> *)(k == 0?&(session._mainViewFiles):&(session._subViewFiles));

			Buffer * buf = MainFileManager->getBufferByID(bufID);

			if (buf->isUntitled() && buf->docLength() == 0)
				continue;

			if (!includUntitledDoc)
				if (!PathFileExists(buf->getFullPathName()))
					continue;

			
			generic_string	languageName = getLangFromMenu(buf);
			const TCHAR *langName = languageName.c_str();
			sessionFileInfo sfi(buf->getFullPathName(), langName, buf->getEncoding(), buf->getPosition(editView), buf->getBackupFileName().c_str(), int(buf->getLastModifiedTimestamp()));

			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			int maxLine = _invisibleEditView.execute(SCI_GETLINECOUNT);

			for (int j = 0 ; j < maxLine ; ++j)
			{
				if ((_invisibleEditView.execute(SCI_MARKERGET, j)&(1 << MARK_BOOKMARK)) != 0)
				{
					sfi._marks.push_back(j);
				}
			}

			if (i == activeIndex)
			{
				editView->getCurrentFoldStates(sfi._foldStates);
			}
			else
			{
				sfi._foldStates = buf->getHeaderLineState(editView);
			}
			viewFiles->push_back(sfi);
		}
	}
	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
}

bool Notepad_plus::str2Cliboard(const generic_string & str2cpy)
{
	return str2Clipboard(str2cpy, _pPublicInterface->getHSelf());
}

//ONLY CALL IN CASE OF EMERGENCY: EXCEPTION
//This function is destructive
bool Notepad_plus::emergency(generic_string emergencySavedDir)
{
    ::CreateDirectory(emergencySavedDir.c_str(), NULL);
	return dumpFiles(emergencySavedDir.c_str(), TEXT("File"));
}

bool Notepad_plus::dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix) {
	//start dumping unsaved files to recovery directory
	bool somethingsaved = false;
	bool somedirty = false;
	TCHAR savePath[MAX_PATH] = {0};

	//rescue primary
	for(int i = 0; i < MainFileManager->getNrBuffers(); ++i) {
		Buffer * docbuf = MainFileManager->getBufferByIndex(i);
		if (!docbuf->isDirty())	//skip saved documents
			continue;
		else
			somedirty = true;

		const TCHAR * unitext = (docbuf->getUnicodeMode() != uni8Bit)?TEXT("_utf8"):TEXT("");
		wsprintf(savePath, TEXT("%s\\%s%03d%s.dump"), outdir, fileprefix, i, unitext);

		bool res = MainFileManager->saveBuffer(docbuf->getID(), savePath);

		somethingsaved |= res;
	}

	return somethingsaved || !somedirty;
}

void Notepad_plus::drawTabbarColoursFromStylerArray()
{
	Style *stActText = getStyleFromName(TABBAR_ACTIVETEXT);
	if (stActText && stActText->_fgColor != -1)
		TabBarPlus::setColour(stActText->_fgColor, TabBarPlus::activeText);

	Style *stActfocusTop = getStyleFromName(TABBAR_ACTIVEFOCUSEDINDCATOR);
	if (stActfocusTop && stActfocusTop->_fgColor != -1)
		TabBarPlus::setColour(stActfocusTop->_fgColor, TabBarPlus::activeFocusedTop);

	Style *stActunfocusTop = getStyleFromName(TABBAR_ACTIVEUNFOCUSEDINDCATOR);
	if (stActunfocusTop && stActunfocusTop->_fgColor != -1)
		TabBarPlus::setColour(stActunfocusTop->_fgColor, TabBarPlus::activeUnfocusedTop);

	Style *stInact = getStyleFromName(TABBAR_INACTIVETEXT);
	if (stInact && stInact->_fgColor != -1)
		TabBarPlus::setColour(stInact->_fgColor, TabBarPlus::inactiveText);
	if (stInact && stInact->_bgColor != -1)
		TabBarPlus::setColour(stInact->_bgColor, TabBarPlus::inactiveBg);
}

void Notepad_plus::notifyBufferChanged(Buffer * buffer, int mask)
{
	// To avoid to crash while MS-DOS style is set as default language,
	// Checking the validity of current instance is necessary.
	if (!this) return;

	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();

	_mainEditView.bufferUpdated(buffer, mask);
	_subEditView.bufferUpdated(buffer, mask);
	_mainDocTab.bufferUpdated(buffer, mask);
	_subDocTab.bufferUpdated(buffer, mask);

	bool mainActive = (_mainEditView.getCurrentBuffer() == buffer);
	bool subActive = (_subEditView.getCurrentBuffer() == buffer);

	//Only event that applies to non-active Buffers
	if (mask & BufferChangeStatus)
	{	//reload etc
		bool didDialog = false;
		bool doCloseDoc = false;
		switch(buffer->getStatus())
		{
			case DOC_UNNAMED: 	//nothing todo
			{
				break;
			}
			case DOC_REGULAR: 	//nothing todo
			{
				break;
			}
			case DOC_MODIFIED:	//ask for reloading
			{
				bool autoUpdate = (nppGUI._fileAutoDetection == cdAutoUpdate) || (nppGUI._fileAutoDetection == cdAutoUpdateGo2end);
				if (!autoUpdate || buffer->isDirty())
				{
                    // if file updating is not silently, we switch to the file to update.
                    int index = _pDocTab->getIndexByBuffer(buffer->getID());
				    int iView = currentView();
				    if (index == -1)
					    iView = otherView();
				    activateBuffer(buffer->getID(), iView);	//activate the buffer in the first view possible

                    // Then we ask user to update
					didDialog = true;
					if (doReloadOrNot(buffer->getFullPathName(), buffer->isDirty()) != IDYES)
						break;	//abort
				}
				// Set _isLoadedDirty false so when the document clean state is reached the icon will be set to blue
				buffer->setLoadedDirty(false);
				doReload(buffer->getID(), false);
				if (mainActive || subActive)
				{
					performPostReload(mainActive?MAIN_VIEW:SUB_VIEW);
				}
				break;
			}
			case DOC_DELETED: 	//ask for keep
			{
				SCNotification scnN;
				scnN.nmhdr.code = NPPN_FILEDELETED;
				scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
				scnN.nmhdr.idFrom = (uptr_t)buffer->getID();
				_pluginsManager.notify(&scnN);

				int index = _pDocTab->getIndexByBuffer(buffer->getID());
				int iView = currentView();
				if (index == -1)
					iView = otherView();

				activateBuffer(buffer->getID(), iView);	//activate the buffer in the first view possible
				didDialog = true;
				doCloseDoc = doCloseOrNot(buffer->getFullPathName()) == IDNO;
				if (doCloseDoc)
				{
					//close in both views, doing current view last since that has to remain opened
					bool isSnapshotMode = nppGUI.isSnapshotMode();
					doClose(buffer->getID(), otherView(), isSnapshotMode);
					doClose(buffer->getID(), currentView(), isSnapshotMode);
				}
				break;
			}
		}

		if (didDialog)
		{
			int curPos = _pEditView->execute(SCI_GETCURRENTPOS);
			::PostMessage(_pEditView->getHSelf(), WM_LBUTTONUP, 0, 0);
			::PostMessage(_pEditView->getHSelf(), SCI_SETSEL, curPos, curPos);
			if (::IsIconic(_pPublicInterface->getHSelf()))
				::ShowWindow(_pPublicInterface->getHSelf(), SW_RESTORE);

			if (doCloseDoc) // buffer has been deleted, cannot (and no need to) go on
				return;
		}
	}

    if (mask & (BufferChangeReadonly))
	{
		checkDocState();

		bool isSysReadOnly = buffer->getFileReadOnly();
		bool isUserReadOnly = buffer->getUserReadOnly();
		bool isDirty = buffer->isDirty();

		// To notify plugins ro status is changed
		SCNotification scnN;
		scnN.nmhdr.hwndFrom = (void *)buffer->getID();
		scnN.nmhdr.idFrom = (uptr_t)  ((isSysReadOnly || isUserReadOnly? DOCSTAUS_READONLY : 0) | (isDirty ? DOCSTAUS_BUFFERDIRTY : 0));
		scnN.nmhdr.code = NPPN_READONLYCHANGED;
		_pluginsManager.notify(&scnN);

	}

	if (_pFileSwitcherPanel)
		_pFileSwitcherPanel->setItemIconStatus((int)buffer);

	if (!mainActive && !subActive)
	{
		return;
	}

	if (mask & (BufferChangeLanguage))
	{
		if (mainActive)
			_autoCompleteMain.setLanguage(buffer->getLangType());
		if (subActive)
			_autoCompleteSub.setLanguage(buffer->getLangType());
	}

	if ((currentView() == MAIN_VIEW) && !mainActive)
		return;

	if ((currentView() == SUB_VIEW) && !subActive)
		return;

	if (mask & (BufferChangeDirty|BufferChangeFilename))
	{
		if(mask & BufferChangeFilename)
			command(IDM_VIEW_REFRESHTABAR);
		checkDocState();
		setTitle();
		generic_string dir(buffer->getFullPathName());
		PathRemoveFileSpec(dir);
		setWorkingDir(dir.c_str());
	}

	if (mask & (BufferChangeLanguage))
	{
		checkLangsMenu(-1);	//let N++ do search for the item
		setLangStatus(buffer->getLangType());
		if (_mainEditView.getCurrentBuffer() == buffer)
			_autoCompleteMain.setLanguage(buffer->getLangType());
		else if (_subEditView.getCurrentBuffer() == buffer)
			_autoCompleteSub.setLanguage(buffer->getLangType());

		SCNotification scnN;
		scnN.nmhdr.code = NPPN_LANGCHANGED;
		scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
		scnN.nmhdr.idFrom = (uptr_t)_pEditView->getCurrentBufferID();
		_pluginsManager.notify(&scnN);
	}

	if (mask & (BufferChangeFormat|BufferChangeLanguage|BufferChangeUnicode))
	{
		updateStatusBar();
		checkUnicodeMenuItems(/*buffer->getUnicodeMode()*/);
		setUniModeText();
		setDisplayFormat(buffer->getFormat());
		enableConvertMenuItems(buffer->getFormat());
	}
}

void Notepad_plus::notifyBufferActivated(BufferID bufid, int view)
{
	Buffer * buf = MainFileManager->getBufferByID(bufid);
	buf->increaseRecentTag();

	if (view == MAIN_VIEW)
	{
		_autoCompleteMain.setLanguage(buf->getLangType());
	}
	else if (view == SUB_VIEW)
	{
		_autoCompleteSub.setLanguage(buf->getLangType());
	}

	if (view != currentView())
		return;	//dont care if another view did something


	checkDocState();
	dynamicCheckMenuAndTB();
	setLangStatus(buf->getLangType());
	updateStatusBar();
	checkUnicodeMenuItems(/*buf->getUnicodeMode()*/);
	setUniModeText();
	setDisplayFormat(buf->getFormat());
	enableConvertMenuItems(buf->getFormat());
	generic_string dir(buf->getFullPathName());
	PathRemoveFileSpec(dir);
	setWorkingDir(dir.c_str());
	setTitle();
	//Make sure the colors of the tab controls match
	::InvalidateRect(_mainDocTab.getHSelf(), NULL, FALSE);
	::InvalidateRect(_subDocTab.getHSelf(), NULL, FALSE);

	SCNotification scnN;
	scnN.nmhdr.code = NPPN_BUFFERACTIVATED;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)bufid;
	_pluginsManager.notify(&scnN);

	if (_pFileSwitcherPanel)
	{
		_pFileSwitcherPanel->activateItem((int)bufid, currentView());
	}

	if (_pDocMap && (!_pDocMap->isClosed()) && _pDocMap->isVisible())
	{
		_pDocMap->reloadMap();
		_pDocMap->setSyntaxHiliting();
	}

	if (_pFuncList && (!_pFuncList->isClosed()) && _pFuncList->isVisible())
	{
		_pFuncList->reload();
	}

	_linkTriggered = true;
}

void Notepad_plus::loadCommandlineParams(const TCHAR * commandLine, CmdLineParams * pCmdParams) {
	if (!commandLine || ! pCmdParams)
		return;

	NppParameters *nppParams = NppParameters::getInstance();
	FileNameStringSplitter fnss(commandLine);
	const TCHAR *pFn = NULL;

	// loading file as session file is allowed only when there is only one file
	if (pCmdParams->_isSessionFile && fnss.size() == 1) 
	{
		Session session2Load;
		if ((NppParameters::getInstance())->loadSession(session2Load, fnss.getFileName(0)))
		{
			loadSession(session2Load);
		}
		return;
	}

 	LangType lt = pCmdParams->_langType;
	int ln =  pCmdParams->_line2go;
    int cn = pCmdParams->_column2go;
	bool recursive = pCmdParams->_isRecursive;
	bool readOnly = pCmdParams->_isReadOnly;

	BufferID lastOpened = BUFFER_INVALID;
	for (int i = 0, len = fnss.size(); i < len ; ++i)
	{
		pFn = fnss.getFileName(i);
		if (!pFn) return;

		BufferID bufID = doOpen(pFn, recursive, readOnly);
		if (bufID == BUFFER_INVALID)	//cannot open file
			continue;

		lastOpened = bufID;

		if (lt != L_EXTERNAL && lt < nppParams->L_END)
		{
			Buffer * pBuf = MainFileManager->getBufferByID(bufID);
			pBuf->setLangType(lt);
		}

		if (ln != -1)
		{	//we have to move the cursor manually
			int iView = currentView();	//store view since fileswitch can cause it to change
			switchToFile(bufID);	//switch to the file. No deferred loading, but this way we can easily move the cursor to the right position

            if (cn == -1)
			_pEditView->execute(SCI_GOTOLINE, ln-1);
            else
            {
                int pos = _pEditView->execute(SCI_FINDCOLUMN, ln-1, cn-1);
                _pEditView->execute(SCI_GOTOPOS, pos);
            }
			switchEditViewTo(iView);	//restore view
		}
	}
	if (lastOpened != BUFFER_INVALID)
    {
		switchToFile(lastOpened);
	}
}


void Notepad_plus::setFindReplaceFolderFilter(const TCHAR *dir, const TCHAR *filter)
{
	generic_string fltr;
	NppParameters *pNppParam = NppParameters::getInstance();
	FindHistory & findHistory = pNppParam->getFindHistory();

	// get current directory in case it's not provided.
	if (!dir && findHistory._isFolderFollowDoc)
	{
		dir = pNppParam->getWorkingDir();
	}

	// get current language file extensions in case it's not provided.
	if (!filter && findHistory._isFilterFollowDoc)
	{
		// Get current language file extensions
		const TCHAR *ext = NULL;
		LangType lt = _pEditView->getCurrentBuffer()->getLangType();

		if (lt == L_USER)
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			UserLangContainer * userLangContainer = pNppParam->getULCFromName(buf->getUserDefineLangName());
			if (userLangContainer)
				ext = userLangContainer->getExtention();
		}
		else
		{
			ext = NppParameters::getInstance()->getLangExtFromLangType(lt);
		}

		if (ext && ext[0])
		{
			fltr = TEXT("");
			vector<generic_string> vStr;
			cutString(ext, vStr);
			for (size_t i = 0 ,len = vStr.size(); i < len; ++i)
			{
				fltr += TEXT("*.");
				fltr += vStr[i] + TEXT(" ");
			}
		}
		else
		{
			fltr = TEXT("*.*");
		}
		filter = fltr.c_str();
	}
	_findReplaceDlg.setFindInFilesDirFilter(dir, filter);
}

vector<generic_string> Notepad_plus::addNppComponents(const TCHAR *destDir, const TCHAR *extFilterName, const TCHAR *extFilter)
{
	FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());
    fDlg.setExtFilter(extFilterName, extFilter, NULL);

    vector<generic_string> copiedFiles;

    if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
    {
        // Get plugins dir
		generic_string destDirName = (NppParameters::getInstance())->getNppPath();
        PathAppend(destDirName, destDir);

        if (!::PathFileExists(destDirName.c_str()))
        {
            ::CreateDirectory(destDirName.c_str(), NULL);
        }

        destDirName += TEXT("\\");

        size_t sz = pfns->size();
        for (size_t i = 0 ; i < sz ; ++i)
        {
            if (::PathFileExists(pfns->at(i).c_str()))
            {
                // copy to plugins directory
                generic_string destName = destDirName;
                destName += ::PathFindFileName(pfns->at(i).c_str());
                if (::CopyFile(pfns->at(i).c_str(), destName.c_str(), FALSE))
                    copiedFiles.push_back(destName.c_str());
            }
        }
    }
    return copiedFiles;
}

void Notepad_plus::setWorkingDir(const TCHAR *dir)
{
	NppParameters * params = NppParameters::getInstance();
	if (params->getNppGUI()._openSaveDir == dir_last)
		return;
	if (params->getNppGUI()._openSaveDir == dir_userDef)
	{
		params->setWorkingDir(NULL);
	}
	else if (dir && PathIsDirectory(dir))
	{
		params->setWorkingDir(dir);
	}
}

int Notepad_plus::getLangFromMenuName(const TCHAR * langName)
{
	int	id	= 0;
	const int menuSize = 64;
	TCHAR menuLangName[menuSize];

	for ( int i = IDM_LANG_C; i <= IDM_LANG_USER; ++i )
		if ( ::GetMenuString( _mainMenuHandle, i, menuLangName, menuSize, MF_BYCOMMAND ) )
			if ( !lstrcmp( langName, menuLangName ) )
			{
				id	= i;
				break;
			}

	if ( id == 0 )
	{
		for ( int i = IDM_LANG_USER + 1; i <= IDM_LANG_USER_LIMIT; ++i )
			if ( ::GetMenuString( _mainMenuHandle, i, menuLangName, menuSize, MF_BYCOMMAND ) )
				if ( !lstrcmp( langName, menuLangName ) )
				{
					id	= i;
					break;
				}
	}

	return id;
}

generic_string Notepad_plus::getLangFromMenu(const Buffer * buf)
{

	int	id;
	generic_string userLangName;
	const int nbChar = 32;
	TCHAR menuLangName[nbChar];

	id = (NppParameters::getInstance())->langTypeToCommandID( buf->getLangType() );
	if ( ( id != IDM_LANG_USER ) || !( buf->isUserDefineLangExt() ) )
	{
		::GetMenuString(_mainMenuHandle, id, menuLangName, nbChar-1, MF_BYCOMMAND);
		userLangName = menuLangName;
	}
	else
	{
		userLangName = buf->getUserDefineLangName();
	}
	return	userLangName;
}

Style * Notepad_plus::getStyleFromName(const TCHAR *styleName)
{
	StyleArray & stylers = (NppParameters::getInstance())->getMiscStylerArray();

	int i = stylers.getStylerIndexByName(styleName);
	Style * st = NULL;
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		st = &style;
	}
	return st;
}

bool Notepad_plus::noOpenedDoc() const
{
	if (_mainDocTab.isVisible() && _subDocTab.isVisible())
		return false;
	if (_pDocTab->nbItem() == 1)
	{
		BufferID buffer = _pDocTab->getBufferByIndex(0);
		Buffer * buf = MainFileManager->getBufferByID(buffer);
		if (!buf->isDirty() && buf->isUntitled())
			return true;
	}
	return false;
}

bool Notepad_plus::reloadLang()
{
	NppParameters *pNppParam = NppParameters::getInstance();

	if (!pNppParam->reloadLang())
	{
		return false;
	}

	TiXmlDocumentA *nativeLangDocRootA = pNppParam->getNativeLangA();
	if (!nativeLangDocRootA)
	{
		return false;
	}

    _nativeLangSpeaker.init(nativeLangDocRootA, true);

    pNppParam->reloadContextMenuFromXmlTree(_mainMenuHandle, _pluginsManager.getMenuHandle());

	generic_string pluginsTrans, windowTrans;
	_nativeLangSpeaker.changeMenuLang(_mainMenuHandle, pluginsTrans, windowTrans);
    ::DrawMenuBar(_pPublicInterface->getHSelf());

	int indexWindow = ::GetMenuItemCount(_mainMenuHandle) - 3;

	if (_pluginsManager.hasPlugins() && pluginsTrans != TEXT(""))
	{
		::ModifyMenu(_mainMenuHandle, indexWindow - 1, MF_BYPOSITION, 0, pluginsTrans.c_str());
	}

	if (windowTrans != TEXT(""))
	{
		::ModifyMenu(_mainMenuHandle, indexWindow, MF_BYPOSITION, 0, windowTrans.c_str());
		windowTrans += TEXT("...");
		::ModifyMenu(_mainMenuHandle, IDM_WINDOW_WINDOWS, MF_BYCOMMAND, IDM_WINDOW_WINDOWS, windowTrans.c_str());
	}
	// Update scintilla context menu strings
	vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
	size_t len = tmp.size();
	TCHAR menuName[64];
	for (size_t i = 0 ; i < len ; ++i)
	{
		if (tmp[i]._itemName == TEXT(""))
		{
			::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
			tmp[i]._itemName = purgeMenuItemString(menuName);
		}
	}

	vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
	len = shortcuts.size();

	for(size_t i = 0; i < len; ++i)
	{
		CommandShortcut & csc = shortcuts[i];
		::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND);
		csc.setName(purgeMenuItemString(menuName, true).c_str());
	}
	_accelerator.updateFullMenu();

	_scintaccelerator.updateKeys();


	if (_tabPopupMenu.isCreated())
	{
		_nativeLangSpeaker.changeLangTabContextMenu(_tabPopupMenu.getMenuHandle());
	}
	if (_tabPopupDropMenu.isCreated())
	{
		_nativeLangSpeaker.changeLangTabDrapContextMenu(_tabPopupDropMenu.getMenuHandle());
	}
	if (_fileSwitcherMultiFilePopupMenu.isCreated())
	{
		//_nativeLangSpeaker.changeLangTabDrapContextMenu(_fileSwitcherMultiFilePopupMenu.getMenuHandle());
	}
	if (_preference.isCreated())
	{
		_nativeLangSpeaker.changePrefereceDlgLang(_preference);
	}

	if (_configStyleDlg.isCreated())
	{
        _nativeLangSpeaker.changeConfigLang(_configStyleDlg.getHSelf());
	}

	if (_findReplaceDlg.isCreated())
	{
		_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
	}

	if (_goToLineDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
	}

	if (_runDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_runDlg.getHSelf(), "Run");
	}

	if (_runMacroDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_runMacroDlg.getHSelf(), "MultiMacro");
	}

	if (_findCharsInRangeDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_findCharsInRangeDlg.getHSelf(), "FindCharsInRange");
	}

	if (_colEditorDlg.isCreated())
	{
        _nativeLangSpeaker.changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
	}

	UserDefineDialog *udd = _pEditView->getUserDefineDlg();
	if (udd->isCreated())
	{
		_nativeLangSpeaker.changeUserDefineLang(udd);
	}

	_lastRecentFileList.setLangEncoding(_nativeLangSpeaker.getLangEncoding());
	return true;
}

void Notepad_plus::launchClipboardHistoryPanel()
{
	if (!_pClipboardHistoryPanel)
	{
		_pClipboardHistoryPanel = new ClipboardHistoryPanel();

		_pClipboardHistoryPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);

		tTbData	data = {0};
		_pClipboardHistoryPanel->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pClipboardHistoryPanel->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_CLIPBOARDPANEL_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_EDIT_CLIPBOARDHISTORY_PANEL;
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(CH_PROJECTPANELTITLE, "ClipboardHistory", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);

		COLORREF fgColor = (NppParameters::getInstance())->getCurrentDefaultFgColor();
		COLORREF bgColor = (NppParameters::getInstance())->getCurrentDefaultBgColor();

		_pClipboardHistoryPanel->setBackgroundColor(bgColor);
		_pClipboardHistoryPanel->setForegroundColor(fgColor);
	}

	_pClipboardHistoryPanel->display();
}

void Notepad_plus::launchFileSwitcherPanel()
{
	if (!_pFileSwitcherPanel)
	{
		_pFileSwitcherPanel = new VerticalFileSwitcher;
		HIMAGELIST hImgLst = _docTabIconList.getHandle();
		_pFileSwitcherPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), hImgLst);

		tTbData	data = {0};
		_pFileSwitcherPanel->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pFileSwitcherPanel->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_DOCSWITCHER_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_FILESWITCHER_PANEL;

		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(FS_PROJECTPANELTITLE, "DocSwitcher", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		
		COLORREF fgColor = (NppParameters::getInstance())->getCurrentDefaultFgColor();
		COLORREF bgColor = (NppParameters::getInstance())->getCurrentDefaultBgColor();

		_pFileSwitcherPanel->setBackgroundColor(bgColor);
		_pFileSwitcherPanel->setForegroundColor(fgColor);
	}
	_pFileSwitcherPanel->display();
}

void Notepad_plus::launchAnsiCharPanel()
{
	if (!_pAnsiCharPanel)
	{
		_pAnsiCharPanel = new AnsiCharPanel();

		_pAnsiCharPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);

		tTbData	data = {0};
		_pAnsiCharPanel->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pAnsiCharPanel->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_ASCIIPANEL_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_EDIT_CHAR_PANEL;
		
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(AI_PROJECTPANELTITLE, "AsciiInsertion", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		
		COLORREF fgColor = (NppParameters::getInstance())->getCurrentDefaultFgColor();
		COLORREF bgColor = (NppParameters::getInstance())->getCurrentDefaultBgColor();

		_pAnsiCharPanel->setBackgroundColor(bgColor);
		_pAnsiCharPanel->setForegroundColor(fgColor);
	}

	_pAnsiCharPanel->display();
}

void Notepad_plus::launchProjectPanel(int cmdID, ProjectPanel ** pProjPanel, int panelID)
{
	if (!(*pProjPanel))
	{
		NppParameters *pNppParam = NppParameters::getInstance();

		(*pProjPanel) = new ProjectPanel;
		(*pProjPanel)->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());
		(*pProjPanel)->setWorkSpaceFilePath(pNppParam->getworkSpaceFilePath(panelID));

		tTbData	data;
		memset(&data, 0, sizeof(data));
		(*pProjPanel)->create(&data);
		data.pszName = TEXT("ST");

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)(*pProjPanel)->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_PROJECTPANEL_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = cmdID;
		
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(PM_PROJECTPANELTITLE, "ProjectManager", "PanelTitle");

		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);

		COLORREF fgColor = (NppParameters::getInstance())->getCurrentDefaultFgColor();
		COLORREF bgColor = (NppParameters::getInstance())->getCurrentDefaultBgColor();

		(*pProjPanel)->setBackgroundColor(bgColor);
		(*pProjPanel)->setForegroundColor(fgColor);
	}
	(*pProjPanel)->display();
}

void Notepad_plus::launchDocMap()
{
	if (!(NppParameters::getInstance())->isTransparentAvailable())
	{
		::MessageBox(NULL, TEXT("It seems you still use a prehistoric system, This feature works only on a modern system, sorry."), TEXT(""), MB_OK);
		return;
	}

	if (!_pDocMap)
	{
		_pDocMap = new DocumentMap();
		_pDocMap->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);
		
		tTbData	data = {0};
		_pDocMap->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pDocMap->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_DOCMAP_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_DOC_MAP;

		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(DM_PANELTITLE, "DocumentMap", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
	}

	_pDocMap->initWrapMap();
	_pDocMap->wrapMap();
	_pDocMap->display();

	_pEditView->getFocus();
}

void Notepad_plus::launchFunctionList()
{
	if (!_pFuncList)
	{
		_pFuncList = new FunctionListPanel();
		_pFuncList->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);
		
		tTbData	data = {0};
		_pFuncList->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pFuncList->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB;
		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_FUNC_LIST_ICO), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_FUNC_LIST;

		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(FL_PANELTITLE, "FunctionList", "PanelTitle");

		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			lstrcpy(title, title_temp.c_str());
			data.pszName = title;
		}

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		COLORREF fgColor = (NppParameters::getInstance())->getCurrentDefaultFgColor();
		COLORREF bgColor = (NppParameters::getInstance())->getCurrentDefaultBgColor();

		_pFuncList->setBackgroundColor(bgColor);
		_pFuncList->setForegroundColor(fgColor);
	}
	
	_pFuncList->display();
	_pFuncList->reload();

	_pEditView->getFocus();
}

struct TextPlayerParams {
	HWND _nppHandle;
	ScintillaEditView *_pCurrentView;
	const char *_text2display;
	const char *_quoter;
	bool _shouldBeTrolling;
};

struct TextTrollerParams {
	ScintillaEditView *_pCurrentView;
	const char *_text2display;
	BufferID _targetBufID;
	HANDLE _mutex;
};

struct Quote{
	const char *_quoter;
	const char *_quote;
};

const int nbQuote = 203;
Quote quotes[nbQuote] = {
{"Notepad++", "Good programmers use Notepad++ to code.\nExtreme programmers use MS Word to code, in Comic Sans, center aligned."},
{"Martin Golding", "Always code as if the guy who ends up maintaining your code will be a violent psychopath who knows where you live."},
{"L. Peter Deutsch", "To iterate is human, to recurse divine."},
{"Seymour Cray", "The trouble with programmers is that you can never tell what a programmer is doing until it's too late."},
{"Brian Kernighan", "Debugging is twice as hard as writing the code in the first place. Therefore, if you write the code as cleverly as possible, you are, by definition, not smart enough to debug it."},
{"Alan Kay", "Most software today is very much like an Egyptian pyramid with millions of bricks piled on top of each other, with no structural integrity, but just done by brute force and thousands of slaves."},
{"Bill Gates", "Measuring programming progress by lines of code is like measuring aircraft building progress by weight."},
{"Christopher Thompson", "Sometimes it pays to stay in bed on Monday, rather than spending the rest of the week debugging Monday's code."},
{"Vidiu Platon", "I don't care if it works on your machine! We are not shipping your machine!"},
{"Edward V Berard", "Walking on water and developing software from a specification are easy if both are frozen."},
{"pixadel", "Fine, Java MIGHT be a good example of what a programming language should be like. But Java applications are good examples of what applications SHOULDN'T be like."},
{"Oktal", "I think Microsoft named .Net so it wouldn't show up in a Unix directory listing."},
{"Bjarne Stroustrup", "In C++ it's harder to shoot yourself in the foot, but when you do, you blow off your whole leg."},
{"Mosher's Law of Software Engineering", "Don't worry if it doesn't work right. If everything did, you'd be out of a job."},
{"Bob Gray", "Writing in C or C++ is like running a chain saw with all the safety guards removed."},
{"Roberto Waltman", "In the one and only true way. The object-oriented version of \"Spaghetti code\" is, of course, \"Lasagna code\". (Too many layers)"},
{"Gavin Russell Baker", "C++ : Where friends have access to your private members."},
{"Linus Torvalds", "Software is like sex: It's better when it's free."},
{"Cult of vi", "Emacs is a great operating system, lacking only a decent editor."},
{"Church of Emacs", "vi has two modes - \"beep repeatedly\" and \"break everything\"."},
{"Steve Jobs", "Picasso had a saying: \"Good artists copy, great artists steal.\".\nWe have always been shameless about stealing great ideas."},
{"brotips #1001", "Do everything for greatness, not money. Money follows greatness."},
{"brotips #1212", "Cheating is like eating fast food: you do it, you enjoy it, and then you feel like shit."},
{"Robin Williams", "God gave men both a penis and a brain, but unfortunately not enough blood supply to run both at the same time."},
{"Darth Vader", "You don't get to 500 million star systems without making a few enemies."},
{"Doug Linder", "A good programmer is someone who always looks both ways before crossing a one-way street."},
{"Jean-Claude van Damme", "A cookie has no soul, it's just a cookie. But before it was milk and eggs.\nAnd in eggs there's the potential for life."},
{"Michael Feldman", "Java is, in many ways, C++--."},
{"Don Ho", "Je mange donc je chie."},
{"Don Ho #2", "RTFM is the true path of every developer.\nBut it would happen only if there's no way out."},
{"Don Ho #3", "Smartphone is the best invention of 21st century for avoiding the eyes contact while crossing people you know on the street."},
{"Anonymous #1", "Does your ass ever get jealous of all the shit that comes out of your month?"},
{"Anonymous #2", "Before sex, you help each other get naked, after sex you only dress yourself.\nMoral of the story: in life no one helps you once you're fucked."},
{"Anonymous #3", "I'm not totally useless. I can be used as a bad example."},
{"Anonymous #4", "Life is too short to remove USB safely."},
{"Anonymous #5", "\"SEX\" is not the answer.\nSex is the question, \"YES\" is the answer."},
{"Anonymous #6", "Going to Mc Donald's for a salad is like going to a whore for a hug."},
{"Anonymous #7", "I need a six month holiday, TWICE A YEAR!"},
{"Anonymous #8", "Everything is a knife if you're strong enough."},
{"Anonymous #9", "I just read a list of \"the 100 things to do before you die\". I'm pretty surprised \"yell for help\" wasn't one of them..."},
{"Anonymous #10", "Roses are red,\nViolets are red,\nTulips are red,\nBushes are red,\nTrees are red,\nHOLY SHIT MY\nGARDEN'S ON FIRE!!"},
{"Anonymous #11", "We stopped checking for monsters under our bed, when we realized they were inside us."},
{"Anonymous #12", "I would rather check my facebook than face my checkbook."},
{"Anonymous #13", "Whoever says Paper beats Rock is an idiot. Next time I see someone say that I will throw a rock at them while they hold up a sheet of paper."},
{"Anonymous #14", "A better world is where chickens can cross the road without having their motives questioned."},
{"Anonymous #15", "If I didn't drink, how would my friends know I love them at 2 AM?"},
{"Anonymous #16", "What you do after sex?\n  A. Smoke a cigarette\n  B. Kiss your partener\n  C. Clear browser history\n"},
{"Anonymous #17", "All you need is love,\nall you want is sex,\nall you have is porn.\n"},
{"Anonymous #18", "Never get into fights with ugly people, they have nothing to lose."},
{"Anonymous #19", "F_CK: All I need is U."},
{"Anonymous #20", "Never make eye contact while eating a banana."},
{"Anonymous #21", "I love my sixpack so much, I protect it with a layer of fat."},
{"Anonymous #22", "\"It's impossible.\" said pride.\n\"It's risky.\" said experience.\n\"It's pointless.\" said reason.\n\"Give it a try.\" whispered the heart.\n...\n\"What the hell was that?!?!?!?!?!\" shouted the anus two minutes later."},
{"Anonymous #23", "Everybody talks about leaving a better planet for the children.\nWhy nobody tries to leave better children to the planet?"},
{"Anonymous #24", "An Architect's dream is an Engineer's nightmare."},
{"Anonymous #25", "In a way, I feel sorry for the kids of this generation.\nThey'll have parents who know how to check browser history."},
{"Anonymous #26", "I would never bungee jump.\nI came into this world because of a broken rubber, and I'm not going out cause of one."},
{"Anonymous #27", "I don't have a problem with caffeine.\nI have a problem without caffeine."},
{"Anonymous #28", "Why 6 afraid of 7?\nBecause 7 8 9 (seven ate nine) while 6 and 9 were flirting."},
{"Anonymous #30", "Why do Java developers wear glasses?\nBecause they don't C#."},
{"Anonymous #31", "A baby's laughter is one of the most beautiful sounds you will ever hear. Unless it's 3 AM. And you're home alone. And you don't have a baby."},
{"Anonymous #32", "Two bytes meet. The first byte asks, \"You look terrible. Are you OK?\"\nThe second byte replies, \"No, just feeling a bit off.\""},
{"Anonymous #33", "Programmer - an organism that turns coffee into software."},
{"Anonymous #34", "It's not a bug - it's an undocumented feature."},
{"Anonymous #35", "Should array index start at 0 or 1?\nMy compromised solution is 0.5"},
{"Anonymous #36", "Every single time when I'm about to hug someone extremely sexy, I hit the mirror."},
{"Anonymous #37", "My software never has bugs. It just develops random features."},
{"Anonymous #38", "LISP = Lots of Irritating Silly Parentheses."},
{"Anonymous #39", "Perl, the only language that looks the same before and after RSA encryption."},
{"Anonymous #40", "People ask me why, as an atheist, I still say: OH MY GOD.\nIt makes perfect sense: We say \"Oh my God\" when something is UNBELIEVABLE."},
{"Anonymous #41", "1. Dig a hole.\n2. Name it love.\n3. Watch people falling in love.\n"},
{"Anonymous #42", "Don't think of yourself as an ugly person.\nThink of yourself as a beautiful monkey."},
{"Anonymous #43", "Afraid to die alone?\nBecome a bus driver."},
{"Anonymous #44", "The first 5 days after the weekend are always the hardest."},
{"Anonymous #45", "Rhinos are just fat unicorns."},
{"Anonymous #46", "Today, I asked a girl out. She replied, \"Sorry, I'm suddenly a lesbian.\" FML"},
{"Anonymous #47", "Kids are like fart.\nYou can only stand yours."},
{"Anonymous #48", "If you were born in Israel, you'd probably be Jewish.\nIf you were born in Saudi Arabia, you'd probably be Muslim.\nIf you were born in India, you'd probably be Hindu.\nBut because you were born in North America, you're Christian.\nYour faith is not inspired by some divine, constant truth.\nIt's simply geography."},
{"Anonymous #49", "There are 2 types of people in this world:\nPeople who say they pee in the shower, and the dirty fucking liars."},
{"Anonymous #50", "London 2012 Olympic Games - A bunch of countries coming across the ocean to put their flags in britain and try to get a bunch of gold... it's like history but opposite."},
{"Anonymous #51", "I don't need a stable relationship,\nI just need a stable Internet connection."},
{"Anonymous #52", "What's the difference between religion and bullshit?\nThe bull."},
{"Anonymous #53", "Today, as I was waiting for my girlfriend in the street, I saw a woman who looked a lot like her. I ran towards her, my arms in the air ready to give her a hug, only to realise it wasn't her. I then had to pass the woman, my arms in the air, still running. FML"},
{"Anonymous #54", "Today, I finally got my hands on the new iPhone 5, after I pulled it out of a patient's rectum. FML"},
{"Anonymous #55", "Violent video games won't change our behaviour.\nIf people were influenced by video games, then the majority of Facebook users would be farmers right now."},
{"Anonymous #56", "Religion is like circumcision.\nIf you wait until someone is 21 to tell them about it they probably won't be interested."},
{"Anonymous #57", "No, no, no, I'm not insulting you.\nI'm describing you."},
{"Anonymous #58", "I bought a dog once. Named him \"Stay\".\n\"Come here, Stay.\"\nHe's insane now."},
{"Anonymous #60", "Yesterday I named my Wifi network \"hack me if you can\"\nToday when I woke up it was changed to \"challenge accepted\"."},
{"Anonymous #61", "Your mother is so fat,\nthe recursive function computing her mass causes a stack overflow."},
{"Anonymous #62", "Oral sex makes my day, but anal sex makes my hole weak."},
{"Anonymous #63", "I'm not saying I am Batman, I am just saying no one has ever seen me and Batman in the same room together."},
{"Anonymous #64", "I took a taxi today.\nThe driver told me \"I love my job, I own this car, I've got my own business, I'm my own boss, NO ONE tells me what to do!\"\nI said \"TURN LEFT HERE\".\n"},
{"Anonymous #65", "A man without God is like a fish without a bicycle."},
{"Anonymous #66", "I hate how spiders just sit there on the walls and act like they pay rent!"},
{"Anonymous #67", "Whenever someone starts a sentence by saying \"I'm not racist...\",they are about to say something super racist."},
{"Anonymous #68", "I'm not laughing at you, I'm laughing with you, you're just not laughing."},
{"Anonymous #69", "Women need a reason to have sex. Men just need a place."},
{"Anonymous #70", "If abortion is murder then are condoms kidnapping?"},
{"Anonymous #71", "Men also have feelings.\nFor example, they can feel hungry."},
{"Anonymous #72", "Project Manager:\nA person who thinks 9 women can deliver a baby in 1 month."},
{"Anonymous #73", "If you try and don't succeed, cheat. Repeat until caught. Then lie."},
{"Anonymous #74", "Olympics is the stupidest thing.\nPeople are so proud to be competing for their country.\nThey play their stupid song and raise some dumb flags.\nI'd love to see no flags raised, no song, no mention of country.\nOnly people."},
{"Anonymous #75", "I think therefore I am\nnot religious."},
{"Anonymous #76", "Even if being gay were a choice, so what?\nPeople choose to be assholes and they can get married."},
{"Anonymous #77", "Governments are like diapers.\nThey should be changed often, and for the same reason."},
{"Anonymous #78", "If you expect the world to be fair with you because you are fair, you're fooling yourself.\nThat's like expecting the lion not to eat you because you didn't eat him."},
{"Anonymous #79", "I'm a creationist.\nI believe man create God."},
{"Anonymous #80", "Let's eat kids.\nLet's eat, kids.\n\nUse a comma.\nSave lives."},
{"Anonymous #81", "A male engineering student was crossing a road one day when a frog called out to him and said, \"If you kiss me, I'll turn into a beautiful princess.\" He bent over, picked up the frog, and put it in his pocket.\n\nThe frog spoke up again and said, \"If you kiss me and turn me back into a beautiful princess, I will stay with you for one week.\" The engineering student took the frog out of his pocket, smiled at it; and returned it to his pocket.\n\nThe frog then cried out, \"If you kiss me and turn me back into a princess, I'll stay with you and do ANYTHING you want.\" Again the boy took the frog out, smiled at it, and put it back into his pocket.\n\nFinally, the frog asked, \"What is the matter? I've told you I'm a beautiful princess, that I'll stay with you for a week and do anything you want. Why won't you kiss me?\" The boy said, \"Look I'm an engineer. I don't have time for a girlfriend, but a talking frog is cool.\"\n"},
{"Anonymous #82", "Programmers never die.\nThey just go offline."},
{"Anonymous #83", "Copy from one, it's plagiarism.\nCopy from two, it's research."},
{"Anonymous #84", "Saying that Java is nice because it works on all OSes is like saying that anal sex is nice because it works on all genders."},
{"Anonymous #85", "Race, religion, ethnic pride and nationalism etc... does nothing but teach you how to hate people that you've never met."},
{"Anonymous #86", "Farts are just the ghosts of the things we eat."},
{"Anonymous #87", "I promised I would never kill someone who had my blood.\nBut that mosquito made me break my word."},
//{"Anonymous #88", ""},
{"Anonymous #89", "I'm drunk and you're still ugly."},
{"Anonymous #90", "Clapping:\n(verb)\nRepeatedly high-fiving yourself for someone else's accomplishments."},
{"Anonymous #91", "CV: ctrl-C, ctrl-V"},
{"Anonymous #92", "Mondays are not so bad.\nIt's your job that sucks."},
{"Anonymous #93", "[In a job interview]\nInterviewer: What's your greatest weakness?\nCandidate: Honesty.\nInterviewer: I don't think honesty is a weakness.\nCandidate: I don't give a fuck what you think."},
{"Anonymous #94", "Hey, I just met you\nAnd this is crazy\nHere's my number 127.0.0.1\nPing me maybe?"},
{"Anonymous #95", "YES!\nI'm a programmer, and\nNO!\nIt doesn't mean that I have to fix your PC!"},
{"Anonymous #96", "Code for 6 minutes, debug for 6 hours."},
{"Anonymous #97", "Real Programmers don't comment their code.\nIf it was hard to write, it should be hard to read."},
{"Anonymous #98", "My neighbours listen to good music.\nWhether they like it or not."},
{"Anonymous #99", "I've been using Vim for about 2 years now,\nmostly because I can't figure out how to exit it."},
{"Anonymous #100", "Dear YouTube,\nI can deal with Ads.\nI can deal with Buffer.\nBut when Ads buffer, I suffer."},
{"Anonymous #101", "It's always sad when a man and his dick share only one brain...\nand it turns out to be the dick's."},
{"Anonymous #102", "If IE is brave enough to ask you to set it as your default browser,\ndon't tell me you dare not ask a girl out."},
{"Anonymous #103", "Turn on your brain, turn off TV."},
{"Anonymous #104", "The main idea of \"Inception\":\nif you run a VM inside a VM inside a VM inside a VM inside a VM,\neverything will be very slow."},
//{"Anonymous #105", ""},
{"Anonymous #106", "When I die, I want to go peacefully like my grandfather did, in his sleep\n- not screaming, like the passengers in his car."},
{"Anonymous #107", "Remember, YOUR God is real.\nAll those other Gods are ridiculous, made-up nonsense.\nBut not yours.\nYour God is real. Whichever one that is."},
{"Anonymous #108", "I hope Bruce Willis dies of a Viagra overdose,\nThe way you can see the headline:\nBruce Willis, Died Hard"},
//{"Anonymous #109", ""},
{"Anonymous #110", "A programmer had a problem, so he decided to use threads.\nNow 2 has. He problems."},
{"Anonymous #111", "I love how the internet has improved people's grammar far more than any English teacher has.\nIf you write \"your\" instead of \"you're\" in English class, all you get is a red mark.\nMess up on the internet, and may God have mercy on your soul."},
{"Anonymous #112", "#hulk {\n    height: 200%;\n    width: 200%;\n    color: green;\n}"},
{"Anonymous #113", "Open source is communism.\nAt least it is what communism was meant to be."},
{"Anonymous #114", "How can you face your problem if your problem is your face?"},
{"Anonymous #115", "YOLOLO:\nYou Only LOL Once."},
{"Anonymous #116", "Every exit is an entrance to new experiences."},
{"Anonymous #117", "A Native American was asked:\n\"Do you celebrate Columbus day?\"\nHe replied:\n\"I don't know, do Jews celebrate Hitler's birthday?\""},
{"Anonymous #118", "I love necrophilia, but i can't stand the awkward silences."},
{"Anonymous #119", "\"I'm gonna Google that. BING that, Bing that, sorry.\"\n- The CEO of Bing (many times per day still)"},
{"Anonymous #120", "Life is what happens to you while you're looking at your smartphone."},
{"Anonymous #121", "Thing to do today:\n1. Get up\n2. Go back to bed"},
{"Anonymous #122", "Nerd?\nI prefer the term \"Intellectual badass\"."},
{"Anonymous #123", "How can you face your problem if your problem is your face?"},
{"Anonymous #124", "You don't need religion to have morals.\nIf you can't determine right from wrong then you lack empathy, not religion."},
{"Anonymous #125", "Pooping with the door opened is the meaning of true freedom."},
{"Anonymous #126", "Social media does not make people stupid.\nIt just makes stupid people more visible."},
{"Anonymous #127", "Don't give up your dreams.\nKeep sleeping."},
{"Anonymous #128", "I love sleep.\nNot because I'm lazy.\nBut because my dreams are better than my real life."},
//{"Anonymous #129", ""},
{"Anonymous #130", "Common sense is so rare, it's kinda like a superpower..."},
{"Anonymous #131", "The best thing about a boolean is even if you are wrong, you are only off by a bit."},
{"Anonymous #132", "Benchmarks don't lie, but liars do benchmarks."},
{"Anonymous #133", "Multitasking: Screwing up several things at once."},
{"Anonymous #134", "Linux is user friendly.\nIt's just picky about its friends."},
{"Anonymous #135", "Theory is when you know something, but it doesn't work.\nPractice is when something works, but you don't know why.\nProgrammers combine theory and practice: nothing works and they don't know why."},
{"Anonymous #136", "Documentation is like sex:\nwhen it's good, it's very, very good;\nwhen it's bad, it's better than nothing."},
{"Anonymous #137", "Home is where you poop most comfortably."},
{"Anonymous #138", "Laptop Speakers problem: too quiet for music, too loud for porn."},
{"Anonymous #139", "Chinese food to go: $16\nGas to go get the food: $2\nDrove home just to realize they forgot one of your containers: RICELESS"},
{"Anonymous #140", "MS Windows is like religion to most people: they are born into it, accept it as default, never consider switching to another."},
{"Anonymous #141", "To most religious people, the holy books are like a software license (EULA).\nNobody actually reads it. They just scroll to the bottom and click \"I agree\"."},
{"Anonymous #142", "You are nothing but a number of days,\nwhenever each day passes then part of you has gone."},
{"Anonymous #143", "If 666 is evil, does that make 25.8069758011 the root of all evil?"},
{"Anonymous #144", "I don't want to sound like a badass but...\nI eject my USB drive without removing it safely."},
{"Anonymous #145", "feet  (noun)\na device used for finding legos in the dark"},
{"Anonymous #146", "Buy a sheep\nName it \"Relation\"\nNow you have a Relationsheep\n"},
{"Anonymous #147", "I dig, you dig, we dig,\nhe dig, she dig, they dig...\n\nIt's not a beautiful poem,\nbut it's very deep."},
{"Anonymous #148", "UNIX command line Russian roulette:\n[ $[ $RANDOM % 6 ] == 0 ] && rm -rf /* || echo *Click*"},
{"Anonymous #149", "unzip, strip, top, less, touch, finger, grep, mount, fsck, more, yes, fsck, fsck, fsck, umount, sleep.\n\nNo, it's not porn. It's Unix."},
{"Anonymous #150", "To understand what recursion is, you must first understand recursion."},
{"Anonymous #151", "Q: What's the object-oriented way to become wealthy?\nA: Inheritance."},
{"Anonymous #152", "A SQL query goes into a bar, walks up to two tables and asks, \"Can I join you?\""},
{"Anonymous #153", "You are not fat, you are just more visible."},
{"Anonymous #154", "Minimalist\n (.   .)\n  )   (\n (  Y  )\nASCII Art"},
{"Louis C.K.", "I'm a good citizen. I'm a good father. I recycle and I masturbate."},
{"Mary Oliver", "Someone I loved once gave me a box full of darkness.\nIt took me years to understand that this, too, was a gift."},
{"Floor", "If you fall, I will be there."},
{"Simon Amstell", "If you have some problem in your life and need to deal with it, then use religion, that's fine.\nI use Google."},
{"James Bond", "James, James Bond."},
{"Albert Einstein", "Only 3 things are infinite:\n1. Universe.\n2. Human Stupidity.\n3. Winrar's free trial."},
{"Terry Pratchett", "Artificial Intelligence is no match for natural stupidity."},
{"Stewart Brand", "Once a new technology starts rolling, if you're not part of the steamroller,\nyou're part of the road."},
{"Sam Redwine", "Software and cathedrals are much the same - first we build them, then we pray."},
{"Jan L. A. van de Snepscheut", "In theory, there is no difference between theory and practice. But, in practice, there is."},
{"Jessica Gaston", "One man's crappy software is another man's full time job."},
{"Barack Obama", "Yes, we scan!"},
{"George W. Bush", "Where is my Nobel prize?\nI bombed people too."},
{"Gandhi", "Earth provides enough to satisfy every man's need, but not every man's greed."},
{"R. D. Laing", "Life is a sexually transmitted disease and the mortality rate is one hundred percent."},
{"Apple fan boy", "I'll buy a second iPhone 5 and buy a lot of iOS applications so that Apple will be able to buy Samsung (this shitty company)\nto shut it down and all the Apple haters will be forced to have an iPhone. Muhahaha..."},
{"Hustle Man", "Politicians are like sperm.\nOne in a million turn out to be an actual human being."},
{"Mark Twain", "Censorship is telling a man he can't have a steak just because a baby can't chew it."},
{"Friedrich Nietzsche", "There is not enough love and goodness in the world to permit giving any of it away to imaginary beings."},
{"Dhalsim", "Pain is a state of mind and I don't mind your pain."},
{"Elie Wiesel", "Human beings can be beautiful or more beautiful,\nthey can be fat or skinny, they can be right or wrong,\nbut illegal? How can a human being be illegal?"},
{"Dennis Ritchie", "Empty your memory, with a free(), like a pointer.\nIf you cast a pointer to a integer, it becomes the integer.\nIf you cast a pointer to a struct, it becomes the struct.\nThe pointer can crash, and can overflow.\nBe a pointer my friend."},
{"Chewbacca", "Uuuuuuuuuur Ahhhhrrrrrr\nUhrrrr Ahhhhrrrrrr\nAaaarhg..."},
{"#JeSuisCharlie", "Freedom of expression is like the air we breathe, we don't feel it, until people take it away from us.\n\nFor this reason, Je suis Charlie, not because I endorse everything they published, but because I cherish the right to speak out freely without risk even when it offends others.\nAnd no, you cannot just take someone's life for whatever he/she expressed.\n\nHence this \"Je suis Charlie\" edition.\n" }
};



const int nbWtf = 6;
char *wtf[nbWtf] = {
"WTF?!",
"lol",
"FAP FAP FAP",
"ROFL",
"OMFG",
"Husband is not an ATM machine!!!"
};

const int nbIntervalTime = 5;
int intervalTimeArray[nbIntervalTime] = {30,30,30,30,200};
const int nbPauseTime = 3;
int pauseTimeArray[nbPauseTime] = {200,400,600};

const int act_doNothing = 0;
const int act_trolling = 1;
const int nbAct = 30;
int actionArray[nbAct] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0};
const int maxRange = 200;

int Notepad_plus::getRandomAction(int ranNum)
{
	return actionArray[ranNum % nbAct];
}

bool isInList(int elem, vector<int> elemList)
{
	for (size_t i = 0, len = elemList.size(); i < len; ++i)
	{
		if (elem == elemList[i])
			return true;
	}
	return false;
}

DWORD WINAPI Notepad_plus::threadTextPlayer(void *params)
{
	// random seed generation needs only one time. 
	srand((unsigned int)time(NULL));

	HWND hNpp = ((TextPlayerParams *)params)->_nppHandle;
	ScintillaEditView *pCurrentView = ((TextPlayerParams *)params)->_pCurrentView;
	const char *text2display = ((TextPlayerParams *)params)->_text2display;
	bool shouldBeTrolling = ((TextPlayerParams *)params)->_shouldBeTrolling;

	// Open a new document
    ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

	static TextTrollerParams trollerParams;
	trollerParams._pCurrentView = pCurrentView;
	BufferID targetBufID = pCurrentView->getCurrentBufferID();
	trollerParams._targetBufID = targetBufID;
	HANDLE mutex = ::CreateMutex(NULL, false, TEXT("nppTextWriter"));
	trollerParams._mutex = mutex;
	
    // Get the current scintilla
    HWND curScintilla = pCurrentView->getHSelf();
	const int nbMaxTrolling = 1;
	int nbTrolling = 0;
	vector<int> generatedRans;
	char previousChar = '\0';
	for (size_t i = 0, len = strlen(text2display); i < len ; ++i)
    {
		int ranNum = getRandomNumber(maxRange);

		int action = act_doNothing;

		if (shouldBeTrolling && (i > 20 && previousChar == ' ') && nbTrolling < nbMaxTrolling)
		{
			action = getRandomAction(ranNum);
			//char toto[64];
			//sprintf(toto, "i == %d    action : %d    current char == %c", i, action, text2display[i]);
			//writeLog(TEXT("c:\\tmp\\log.txt"), toto);
		}

		if (action == act_trolling)
		{
			int wtfIndex = getRandomNumber() % nbWtf;
			if (!isInList(wtfIndex, generatedRans))
			{
				//writeLog(TEXT("c:\\tmp\\log.txt"), "trolling begin");
				generatedRans.push_back(wtfIndex);
				++nbTrolling;
				trollerParams._text2display = wtf[wtfIndex];

				ReleaseMutex(mutex);

				HANDLE hThread = ::CreateThread(NULL, 0, threadTextTroller, &trollerParams, 0, NULL);

				::Sleep(1000);
				WaitForSingleObject(mutex, INFINITE);
				
				::CloseHandle(hThread);
				//writeLog(TEXT("c:\\tmp\\log.txt"), "trolling end");
			}	
		}

		char charToShow[2] = {text2display[i], '\0'};

		if (text2display[i] == ' ' || text2display[i] == '.')
			Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		else
			Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);

		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
			return TRUE;

        ::SendMessage(curScintilla, SCI_APPENDTEXT, 1, (LPARAM)charToShow);
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);
		
		previousChar = text2display[i];
		//char ch[64];
		//sprintf(ch, "writting char == %c", text2display[i]);
		//writeLog(TEXT("c:\\tmp\\log.txt"), ch);
    }
	//writeLog(TEXT("c:\\tmp\\log.txt"), "\n\n\n\n");
	const char * quoter = ((TextPlayerParams *)params)->_quoter;
	string quoter_str = quoter;
	size_t pos = quoter_str.find("Anonymous");
	if (pos == string::npos)
	{
		::SendMessage(curScintilla, SCI_APPENDTEXT, 3, (LPARAM)"\n- ");
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);

		// Display quoter
		for (size_t i = 0, len = strlen(quoter); i < len; ++i)
		{
			int ranNum = getRandomNumber(maxRange);
			
			char charToShow[2] = {quoter[i], '\0'};

			Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);

			BufferID currentBufID = pCurrentView->getCurrentBufferID();
			if (currentBufID != targetBufID)
				return TRUE;

			::SendMessage(curScintilla, SCI_APPENDTEXT, 1, (LPARAM)charToShow);
			::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);

		}
	}

    return TRUE;
}

DWORD WINAPI Notepad_plus::threadTextTroller(void *params)
{
	WaitForSingleObject(((TextTrollerParams *)params)->_mutex, INFINITE);

	// random seed generation needs only one time. 
	srand((unsigned int)time(NULL));

	ScintillaEditView *pCurrentView = ((TextTrollerParams *)params)->_pCurrentView;
	const char *text2display = ((TextTrollerParams *)params)->_text2display;
	HWND curScintilla = pCurrentView->getHSelf();
	BufferID targetBufID = ((TextTrollerParams *)params)->_targetBufID;
	//HANDLE mutex = ((TextTrollerParams *)params)->_mutex;

	for (size_t i = 0, len = strlen(text2display); i < len; ++i)
    {
		char charToShow[2] = {text2display[i], '\0'};
        int ranNum = getRandomNumber(maxRange);
		if (text2display[i] == ' ' || text2display[i] == '.')
			Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		else
			Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);

		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
		{
			ReleaseMutex(((TextTrollerParams *)params)->_mutex);
			return TRUE;
		}
        ::SendMessage(curScintilla, SCI_APPENDTEXT, 1, (LPARAM)charToShow);
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);
    }
	//writeLog(TEXT("c:\\tmp\\log.txt"), text2display);
	int n = getRandomNumber();
	int delMethod = n%4;
	if (delMethod == 0)
	{
		size_t len = strlen(text2display);
		for (size_t j = 0; j < len; ++j)
		{
			if (!deleteBack(pCurrentView, targetBufID))
				break;
		}	
	}
	else if (delMethod == 1)
	{
		size_t len = strlen(text2display);
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0) - len, 0);
		for (size_t j = 0; j < len; ++j)
		{
			if (!deleteForward(pCurrentView, targetBufID))
				break;
		}
	}
	else if (delMethod == 2)
	{
		for (size_t j = 0, len = strlen(text2display); j < len; ++j)
		{
			if (!selectBack(pCurrentView, targetBufID))
				break;
		}
		int ranNum = getRandomNumber(maxRange);
		::Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		::SendMessage(pCurrentView->getHSelf(), SCI_DELETEBACK, 0, 0);
	}
	else
	{
		int currentPos = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONSTART, 0, 0);
		::SendMessage(pCurrentView->getHSelf(), SCI_SETSELECTION, currentPos, currentPos - strlen(text2display));
		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
			return TRUE;
		int ranNum = getRandomNumber(maxRange);
		::Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		::SendMessage(pCurrentView->getHSelf(), SCI_DELETEBACK, 0, 0);
	}
	
	ReleaseMutex(((TextTrollerParams *)params)->_mutex);
	return TRUE;
}

bool Notepad_plus::deleteBack(ScintillaEditView *pCurrentView, BufferID targetBufID)
{
	int ranNum = getRandomNumber(maxRange - 100);
	BufferID currentBufID = pCurrentView->getCurrentBufferID();
	Sleep(ranNum);
	if (currentBufID != targetBufID)
		return false;
	::SendMessage(pCurrentView->getHSelf(), SCI_DELETEBACK, 0, 0);
	return true;
}

bool Notepad_plus::deleteForward(ScintillaEditView *pCurrentView, BufferID targetBufID)
{
	int ranNum = getRandomNumber(maxRange - 100);
	BufferID currentBufID = pCurrentView->getCurrentBufferID();
	Sleep(ranNum);
	if (currentBufID != targetBufID)
		return false;
	::SendMessage(pCurrentView->getHSelf(), SCI_GOTOPOS, ::SendMessage(pCurrentView->getHSelf(), SCI_GETCURRENTPOS, 0, 0) + 1, 0);
	::SendMessage(pCurrentView->getHSelf(), SCI_DELETEBACK, 0, 0);
	return true;
}

bool Notepad_plus::selectBack(ScintillaEditView *pCurrentView, BufferID targetBufID)
{
	int ranNum = getRandomNumber(maxRange - 100);
	BufferID currentBufID = pCurrentView->getCurrentBufferID();
	int currentPos = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONSTART, 0, 0);
	int currentAnchor = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONEND, 0, 0);
	Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);
	if (currentBufID != targetBufID)
		return false;
	
	::SendMessage(pCurrentView->getHSelf(), SCI_SETSELECTION, currentAnchor, --currentPos);
	return true;
}

int Notepad_plus::getQuoteIndexFrom(const char *quoter) const
{
	if (!quoter)
		return -1;
	if (stricmp(quoter, "Get them all!!!") == 0)
	{
		return -2;
	}
	
	if (stricmp(quoter, "random") == 0)
	{
		srand((unsigned int)time(NULL));
		return getRandomNumber(nbQuote);
	}

	for (int i = 0; i < nbQuote; ++i)
	{
		if (stricmp(quotes[i]._quoter, quoter) == 0)
			return i;
	}
	return -1;
}

void Notepad_plus::showAllQuotes() const
{

}

/*
void Notepad_plus::showQuoteFromIndex(int index) const
{
	if (index < 0 || index >= nbQuote) return;

    //TextPlayerParams *params = new TextPlayerParams();
	static TextPlayerParams params;
	params._nppHandle = Notepad_plus::_pPublicInterface->getHSelf();
	params._text2display = quotes[index]._quote;
	params._quoter = quotes[index]._quoter;
	params._pCurrentView = _pEditView;
	params._shouldBeTrolling = index < 20;
	HANDLE hThread = ::CreateThread(NULL, 0, threadTextPlayer, &params, 0, NULL);
    ::CloseHandle(hThread);
}
*/


void Notepad_plus::showQuoteFromIndex(int index) const
{
	if (index < 0 || index >= nbQuote) return;
	showQuote(quotes[index]._quote, quotes[index]._quoter, index < 20);
}

void Notepad_plus::showQuote(const char *quote, const char *quoter, bool doTrolling) const

{
	static TextPlayerParams params;
	params._nppHandle = Notepad_plus::_pPublicInterface->getHSelf();
	params._text2display = quote;
	params._quoter = quoter;
	params._pCurrentView = _pEditView;
	params._shouldBeTrolling = doTrolling;
	HANDLE hThread = ::CreateThread(NULL, 0, threadTextPlayer, &params, 0, NULL);
	::CloseHandle(hThread);
}

void Notepad_plus::launchDocumentBackupTask()
{
	HANDLE hThread = ::CreateThread(NULL, 0, backupDocument, NULL, 0, NULL);
	if (hThread)
		::CloseHandle(hThread);
}

DWORD WINAPI Notepad_plus::backupDocument(void * /*param*/)
{
	bool isSnapshotMode = true;
	while (isSnapshotMode)
	{
		NppParameters *nppParam = NppParameters::getInstance();
		if (!nppParam)
			return FALSE;

		size_t timer = nppParam->getNppGUI()._snapshotBackupTiming;
		if (timer < 1000)
			timer = 1000;

		::Sleep(timer);

		nppParam = NppParameters::getInstance();
		if (!nppParam)
			return FALSE;

		isSnapshotMode = nppParam->getNppGUI().isSnapshotMode();
		if (!isSnapshotMode)
			break;

		MainFileManager->backupCurrentBuffer();
	}
	return TRUE;
}

#pragma warning( disable : 4127 )
//--FLS: undoStreamComment: New function to undo stream comment around or within selection end-points.
bool Notepad_plus::undoStreamComment()
{
	const TCHAR *commentStart;
	const TCHAR *commentEnd;

	generic_string symbolStart;
	generic_string symbolEnd;
	const int charbufLen = 10;
    TCHAR charbuf[charbufLen];

	bool retVal = false;

	Buffer * buf = _pEditView->getCurrentBuffer();
	//--LS: Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly()) 
		return false;
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance()->getULCFromName(buf->getUserDefineLangName());
		if (!userLangContainer)
			return false;

		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	if ((!commentStart) || (!commentStart[0]))
		return false;
	if ((!commentEnd) || (!commentEnd[0]))
		return false;

	generic_string start_comment(commentStart);
	generic_string end_comment(commentEnd);
	generic_string white_space(TEXT(" "));
	int start_comment_length = start_comment.length();
	int end_comment_length = end_comment.length();

	do { // do as long as stream-comments are within selection

		int selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
		int selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
		int caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
		int docLength = (_pEditView->execute(SCI_GETLENGTH));

		// checking if caret is located in _beginning_ of selected block
		bool move_caret = caretPosition < selectionEnd;

		//-- Note: The caretPosition is either at selectionEnd or at selectionStart!! selectionStart is always before (smaller) than selectionEnd!!

		//-- First, search all start_comment and end_comment before and after the selectionStart and selectionEnd position.
		const int iSelStart=0, iSelEnd=1;
		const size_t N_CMNT = 2;
		int posStartCommentBefore[N_CMNT], posEndCommentBefore[N_CMNT], posStartCommentAfter[N_CMNT], posEndCommentAfter[N_CMNT];
		bool blnStartCommentBefore[N_CMNT], blnEndCommentBefore[N_CMNT], blnStartCommentAfter[N_CMNT], blnEndCommentAfter[N_CMNT];
		int posStartComment, posEndComment;
		int selectionStartMove, selectionEndMove;
		int flags;

		//-- Directly use Scintilla-Functions 
		//   rather than _findReplaceDlg.processFindNext()which does not return the find-position and is not quiet!
		flags = SCFIND_WORDSTART;
		_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
		//-- Find all start- and end-comments before and after the selectionStart position.
		//-- When searching upwards the start-position for searching must be moved one after the current position
		//   to find a search-string just starting before the current position!
		//-- Direction DIR_UP ---
		posStartCommentBefore[iSelStart] = _pEditView->searchInTarget(start_comment.c_str(), start_comment_length, selectionStart, 0);
		(posStartCommentBefore[iSelStart] == -1 ? blnStartCommentBefore[iSelStart] = false : blnStartCommentBefore[iSelStart] = true);
		posEndCommentBefore[iSelStart] = _pEditView->searchInTarget(end_comment.c_str(), end_comment_length, selectionStart, 0);
		(posEndCommentBefore[iSelStart] == -1 ? blnEndCommentBefore[iSelStart] = false : blnEndCommentBefore[iSelStart] = true);
		//-- Direction DIR_DOWN ---
		posStartCommentAfter[iSelStart] = _pEditView->searchInTarget(start_comment.c_str(), start_comment_length, selectionStart, docLength);
		(posStartCommentAfter[iSelStart] == -1 ? blnStartCommentAfter[iSelStart] = false : blnStartCommentAfter[iSelStart] = true);
		posEndCommentAfter[iSelStart] = _pEditView->searchInTarget(end_comment.c_str(), end_comment_length, selectionStart, docLength);
		(posEndCommentAfter[iSelStart] == -1 ? blnEndCommentAfter[iSelStart] = false : blnEndCommentAfter[iSelStart] = true);

		//-- Check, if selectionStart or selectionEnd is within a stream comment -----
		//   or if the selection includes a complete stream-comment!! ----------------
		bool blnCommentFound = false;
		//-- First, check if there is a stream-comment around the selectionStart position:
		if ((blnStartCommentBefore[iSelStart] && blnEndCommentAfter[iSelStart]) 
			&& (!blnEndCommentBefore[iSelStart] || (posStartCommentBefore[iSelStart] >= posEndCommentBefore[iSelStart])) 
			&& (!blnStartCommentAfter[iSelStart] || (posEndCommentAfter[iSelStart] <= posStartCommentAfter[iSelStart]))) {
				blnCommentFound = true;
				posStartComment = posStartCommentBefore[iSelStart];
				posEndComment   = posEndCommentAfter[iSelStart];
		}
		//-- Second, check if there is a stream-comment around the selectionEnd position:
		else {
			//-- Find all start- and end-comments before and after the selectionEnd position.
			//-- Direction DIR_UP ---
			posStartCommentBefore[iSelEnd] = _pEditView->searchInTarget(start_comment.c_str(), start_comment_length, selectionEnd, 0);
			(posStartCommentBefore[iSelEnd] == -1 ? blnStartCommentBefore[iSelEnd] = false : blnStartCommentBefore[iSelEnd] = true);
			posEndCommentBefore[iSelEnd] = _pEditView->searchInTarget(end_comment.c_str(), end_comment_length, selectionEnd, 0);
			(posEndCommentBefore[iSelEnd] == -1 ? blnEndCommentBefore[iSelEnd] = false : blnEndCommentBefore[iSelEnd] = true);
			//-- Direction DIR_DOWN ---
			posStartCommentAfter[iSelEnd] = _pEditView->searchInTarget(start_comment.c_str(), start_comment_length, selectionEnd, docLength);
			(posStartCommentAfter[iSelEnd] == -1 ? blnStartCommentAfter[iSelEnd] = false : blnStartCommentAfter[iSelEnd] = true);
			posEndCommentAfter[iSelEnd] = _pEditView->searchInTarget(end_comment.c_str(), end_comment_length, selectionEnd, docLength);
			(posEndCommentAfter[iSelEnd] == -1 ? blnEndCommentAfter[iSelEnd] = false : blnEndCommentAfter[iSelEnd] = true);
			if ((blnStartCommentBefore[iSelEnd] && blnEndCommentAfter[iSelEnd]) 
				&& (!blnEndCommentBefore[iSelEnd] || (posStartCommentBefore[iSelEnd] >= posEndCommentBefore[iSelEnd])) 
				&& (!blnStartCommentAfter[iSelEnd] || (posEndCommentAfter[iSelEnd] <= posStartCommentAfter[iSelEnd]))) {
					blnCommentFound = true;
					posStartComment = posStartCommentBefore[iSelEnd];
					posEndComment   = posEndCommentAfter[iSelEnd];
			}
			//-- Third, check if there is a stream-comment within the selected area:
			else if ( (blnStartCommentAfter[iSelStart] && (posStartCommentAfter[iSelStart] < selectionEnd))
				&& (blnEndCommentBefore[iSelEnd] && (posEndCommentBefore[iSelEnd] >  selectionStart))) {
					//-- If there are more than one stream-comment within the selection, take the first one after selectionStart!!
					blnCommentFound = true;
					posStartComment = posStartCommentAfter[iSelStart];
					posEndComment   = posEndCommentAfter[iSelStart];
			}
			//-- Finally, if there is no stream-comment, return
			else {
				return retVal;
			}
		}
		//-- Ok, there are valid start-comment and valid end-comment around the caret-position.
		//   Now, un-comment stream-comment:
		retVal = true;
		int startCommentLength = start_comment_length;
		int endCommentLength = end_comment_length;
		//-- First delete end-comment, so that posStartCommentBefore does not change!
		//-- Get character before end-comment to decide, if there is a white character before the end-comment, which will be removed too!
		_pEditView->getGenericText(charbuf, charbufLen, posEndComment-1, posEndComment);
		if (generic_strncmp(charbuf, white_space.c_str(), white_space.length()) == 0) {
			endCommentLength +=1;
			posEndComment-=1;
		}
		//-- Delete end stream-comment string ---------
		_pEditView->execute(SCI_BEGINUNDOACTION);
		_pEditView->execute(SCI_SETSEL, posEndComment, posEndComment + endCommentLength);
		_pEditView->execute(SCI_REPLACESEL, 0, (WPARAM)"");

		//-- Get character after start-comment to decide, if there is a white character after the start-comment, which will be removed too!
		_pEditView->getGenericText(charbuf, charbufLen, posStartComment+startCommentLength, posStartComment+startCommentLength+1);
		if (generic_strncmp(charbuf, white_space.c_str(), white_space.length()) == 0) {
			startCommentLength +=1;
		}
		//-- Delete starting stream-comment string ---------
		_pEditView->execute(SCI_SETSEL, posStartComment, posStartComment + startCommentLength);
		_pEditView->execute(SCI_REPLACESEL, 0, (WPARAM)"");
		_pEditView->execute(SCI_ENDUNDOACTION);

		//-- Reset selection before calling the routine
		//-- Determine selection movement
		//   selectionStart
		if (selectionStart > posStartComment) {
			if (selectionStart >= posStartComment+startCommentLength)
				selectionStartMove = -(int)startCommentLength;
			else
				selectionStartMove = -(int)(selectionStart - posStartComment);
		}
		else
			selectionStartMove = 0;
		//   selectionEnd
		if (selectionEnd >= posEndComment+endCommentLength)
			selectionEndMove = -(int)(startCommentLength+endCommentLength);
		else if (selectionEnd <= posEndComment)
				selectionEndMove = -(int)startCommentLength;
		else
				selectionEndMove = -(int)(startCommentLength + (selectionEnd - posEndComment));
		//-- Reset selection of text without deleted stream-comment-string
		if (move_caret)
		{
			// moving caret to the beginning of selected block
			_pEditView->execute(SCI_GOTOPOS, selectionEnd+selectionEndMove);
			_pEditView->execute(SCI_SETCURRENTPOS, selectionStart+selectionStartMove);
		}
		else
		{
			_pEditView->execute(SCI_SETSEL, selectionStart+selectionStartMove, selectionEnd+selectionEndMove);
		}
	} while(1); //do as long as stream-comments are within selection
	//return retVal;
} //----- undoStreamComment() -------------------------------

