//this file is part of notepad++
//Copyright (C)2010 Don HO <don.h@free.fr>
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

#include "precompiledHeaders.h"
#include "Notepad_plus.h"
#include "TaskListDlg.h"
#include "ImageListSet.h"

struct SortTaskListPred
{
	DocTabView *_views[2];

	SortTaskListPred(DocTabView &p, DocTabView &s)
	{
		_views[MAIN_VIEW] = &p;
		_views[SUB_VIEW] = &s;
	}

	bool operator()(const TaskLstFnStatus &l, const TaskLstFnStatus &r) const {
		BufferID lID = _views[l._iView]->getBufferByIndex(l._docIndex);
		BufferID rID = _views[r._iView]->getBufferByIndex(r._docIndex);
		Buffer * bufL = MainFileManager->getBufferByID(lID);
		Buffer * bufR = MainFileManager->getBufferByID(rID);
		return bufL->getRecentTag() > bufR->getRecentTag();
	}
};

int docTabIconIDs[] = {IDI_SAVED_ICON, IDI_UNSAVED_ICON, IDI_READONLY_ICON};

ToolBarButtonUnit toolBarIcons[] = {
	{IDM_FILE_NEW,		IDI_NEW_OFF_ICON,		IDI_NEW_ON_ICON,		IDI_NEW_OFF_ICON, IDR_FILENEW},
	{IDM_FILE_OPEN,		IDI_OPEN_OFF_ICON,		IDI_OPEN_ON_ICON,		IDI_NEW_OFF_ICON, IDR_FILEOPEN},
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
	{IDM_VIEW_USER_DLG,  IDI_VIEW_UD_DLG_OFF_ICON,	IDI_VIEW_UD_DLG_ON_ICON,	IDI_VIEW_UD_DLG_OFF_ICON, IDR_SHOWPANNEL},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_MACRO_STARTRECORDINGMACRO,		IDI_STARTRECORD_OFF_ICON,	IDI_STARTRECORD_ON_ICON,	IDI_STARTRECORD_DISABLE_ICON, IDR_STARTRECORD},
	{IDM_MACRO_STOPRECORDINGMACRO,		IDI_STOPRECORD_OFF_ICON,	IDI_STOPRECORD_ON_ICON,		IDI_STOPRECORD_DISABLE_ICON, IDR_STOPRECORD},
	{IDM_MACRO_PLAYBACKRECORDEDMACRO,	IDI_PLAYRECORD_OFF_ICON,	IDI_PLAYRECORD_ON_ICON,		IDI_PLAYRECORD_DISABLE_ICON, IDR_PLAYRECORD},
	{IDM_MACRO_RUNMULTIMACRODLG,			IDI_MMPLAY_OFF_ICON,		IDI_MMPLAY_ON_ICON,			IDI_MMPLAY_DIS_ICON, IDR_M_PLAYRECORD},
	{IDM_MACRO_SAVECURRENTMACRO,			IDI_SAVERECORD_OFF_ICON,	IDI_SAVERECORD_ON_ICON,		IDI_SAVERECORD_DISABLE_ICON, IDR_SAVERECORD}
	
};



LRESULT CALLBACK Notepad_plus::Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{

  static bool isFirstGetMinMaxInfoMsg = true;

  switch(Message)
  {
    case WM_NCCREATE : // First message we get the ptr of instantiated object
                       // then stock it into GWL_USERDATA index in order to retrieve afterward
	{
		Notepad_plus *pM30ide = (Notepad_plus *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		pM30ide->_hSelf = hwnd;
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pM30ide);

		return TRUE;
	}

    default :
    {
      return ((Notepad_plus *)::GetWindowLongPtr(hwnd, GWL_USERDATA))->runProc(hwnd, Message, wParam, lParam);
    }
  }
}



LRESULT Notepad_plus::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = FALSE;

	NppParameters *pNppParam = NppParameters::getInstance();
	switch (Message)
	{
		case WM_NCACTIVATE:
		{
			// Note: lParam is -1 to prevent endless loops of calls
			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, (LPARAM)-1);
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}
		case WM_CREATE:
		{
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

			_mainEditView.init(_hInst, hwnd);
			_subEditView.init(_hInst, hwnd);

			_fileEditView.init(_hInst, hwnd);
			MainFileManager->init(this, &_fileEditView);	//get it up and running asap.

			pNppParam->setFontList(hwnd);
			

			_mainWindowStatus = WindowMainActive;
			_activeView = MAIN_VIEW;

            const ScintillaViewParams & svp1 = pNppParam->getSVP(SCIV_PRIMARY);
			const ScintillaViewParams & svp2 = pNppParam->getSVP(SCIV_SECOND);

			int tabBarStatus = nppGUI._tabStatus;
			_toReduceTabBar = ((tabBarStatus & TAB_REDUCE) != 0);
			_docTabIconList.create(_toReduceTabBar?13:20, _hInst, docTabIconIDs, sizeof(docTabIconIDs)/sizeof(int));

			_mainDocTab.init(_hInst, hwnd, &_mainEditView, &_docTabIconList);
			_subDocTab.init(_hInst, hwnd, &_subEditView, &_docTabIconList);

			_mainEditView.display();

			_invisibleEditView.init(_hInst, hwnd);
			_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
			_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);
			_invisibleEditView.wrap(false); // Make sure no slow down

			// Configuration of 2 scintilla views
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp1._lineNumberMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp2._lineNumberMarginShow);
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp1._bookMarkMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp2._bookMarkMarginShow);

            _mainEditView.showIndentGuideLine(svp1._indentGuideLineShow);
            _subEditView.showIndentGuideLine(svp2._indentGuideLineShow);
			
			::SendMessage(hwnd, NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
			::SendMessage(hwnd, NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);

			_configStyleDlg.init(_hInst, hwnd);
			_preference.init(_hInst, hwnd);
			
            //Marker Margin config
            _mainEditView.setMakerStyle(svp1._folderStyle);
            _subEditView.setMakerStyle(svp2._folderStyle);

			_mainEditView.execute(SCI_SETCARETLINEVISIBLE, svp1._currentLineHilitingShow);
			_subEditView.execute(SCI_SETCARETLINEVISIBLE, svp2._currentLineHilitingShow);
			
			_mainEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
			_subEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);

			_mainEditView.wrap(svp1._doWrap);
			_subEditView.wrap(svp2._doWrap);

			_mainEditView.execute(SCI_SETEDGECOLUMN, svp1._edgeNbColumn);
			_mainEditView.execute(SCI_SETEDGEMODE, svp1._edgeMode);
			_subEditView.execute(SCI_SETEDGECOLUMN, svp2._edgeNbColumn);
			_subEditView.execute(SCI_SETEDGEMODE, svp2._edgeMode);

			_mainEditView.showEOL(svp1._eolShow);
			_subEditView.showEOL(svp2._eolShow);

			_mainEditView.showWSAndTab(svp1._whiteSpaceShow);
			_subEditView.showWSAndTab(svp2._whiteSpaceShow);

			_mainEditView.showWrapSymbol(svp1._wrapSymbolShow);
			_subEditView.showWrapSymbol(svp2._wrapSymbolShow);

			_mainEditView.performGlobalStyles();
			_subEditView.performGlobalStyles();

			_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
			_mainEditView.execute(SCI_SETZOOM, svp1._zoom);
			_subEditView.execute(SCI_SETZOOM, svp2._zoom);

            ::SendMessage(hwnd, NPPM_INTERNAL_SETMULTISELCTION, 0, 0);

			_mainEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);
			_subEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);

			_mainEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);
			_subEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);

			TabBarPlus::doDragNDrop(true);

			if (_toReduceTabBar)
			{
				HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

				if (hf)
				{
					::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
					::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
				}
				TabCtrl_SetItemSize(_mainDocTab.getHSelf(), 45, 20);
				TabCtrl_SetItemSize(_subDocTab.getHSelf(), 45, 20);
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

            _subSplitter.init(_hInst, hwnd);
            _subSplitter.create(&_mainDocTab, &_subDocTab, 8, DYNAMIC, 50, isVertical);

            //--Status Bar Section--//
			bool willBeShown = nppGUI._statusBarShow;
            _statusBar.init(_hInst, hwnd, 6);
			_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, 250);
			_statusBar.setPartWidth(STATUSBAR_CUR_POS, 300);
			_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, 80);
			_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, 100);
			_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, 30);
            _statusBar.display(willBeShown);

            _pMainWindow = &_mainDocTab;

			_dockingManager.init(_hInst, hwnd, &_pMainWindow);

			if (nppGUI._isMinimizedToTray && _pTrayIco == NULL)
				_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

			checkSyncState();

			// Plugin Manager
			NppData nppData;
			nppData._nppHandle = hwnd;
			nppData._scintillaMainHandle = _mainEditView.getHSelf();
			nppData._scintillaSecondHandle = _subEditView.getHSelf();

			_scintillaCtrls4Plugins.init(_hInst, hwnd);
			_pluginsManager.init(nppData);
			_pluginsManager.loadPlugins();
			const TCHAR *appDataNpp = pNppParam->getAppDataNppDir();
			if (appDataNpp[0])
				_pluginsManager.loadPlugins(appDataNpp);

		    _restoreButton.init(_hInst, hwnd);
		    

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
			for (size_t i = 0 ; i < nbMacro ; i++)
			{
				::InsertMenu(hMacroMenu, posBase + i, MF_BYPOSITION, ID_MACRO + i, macros[i].toMenuItemString().c_str());
			}
			// Run Menu
			std::vector<UserCommand> & userCommands = pNppParam->getUserCommandList();
			HMENU hRunMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_RUN);
			int const runPosBase = 2;
			size_t nbUserCommand = userCommands.size();
			if (nbUserCommand >= 1)
				::InsertMenu(hRunMenu, runPosBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);
			for (size_t i = 0 ; i < nbUserCommand ; i++)
			{
				::InsertMenu(hRunMenu, runPosBase + i, MF_BYPOSITION, ID_USER_CMD + i, userCommands[i].toMenuItemString().c_str());
			}

			// Updater menu item
			if (!nppGUI._doesExistUpdater)
			{
				//::MessageBox(NULL, TEXT("pas de updater"), TEXT(""), MB_OK);
				::DeleteMenu(_mainMenuHandle, IDM_UPDATE_NPP, MF_BYCOMMAND);
				::DrawMenuBar(hwnd);
			}
			//Languages Menu
			HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);

			// Add external languages to menu
			for (int i = 0 ; i < pNppParam->getNbExternalLang() ; i++)
			{
				ExternalLangContainer & externalLangContainer = pNppParam->getELCFromIndex(i);

				int numLangs = ::GetMenuItemCount(hLangMenu);
				const int bufferSize = 100;
				TCHAR buffer[bufferSize];

				int x;
				for(x = 0; (x == 0 || lstrcmp(externalLangContainer._name, buffer) > 0) && x < numLangs; x++)
				{
					::GetMenuString(hLangMenu, x, buffer, bufferSize, MF_BYPOSITION);
				}

				::InsertMenu(hLangMenu, x-1, MF_BYPOSITION, IDM_LANG_EXTERNAL + i, externalLangContainer._name);
			}

			if (nppGUI._excludedLangList.size() > 0)
			{
				for (size_t i = 0 ; i < nppGUI._excludedLangList.size() ; i++)
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

			for (int i = 0 ; i < pNppParam->getNbUserLang() ; i++)
			{
				UserLangContainer & userLangContainer = pNppParam->getULCFromIndex(i);
				::InsertMenu(hLangMenu, udlpos + i, MF_BYPOSITION, IDM_LANG_USER + i + 1, userLangContainer.getName());
			}

			//Add recent files
			HMENU hFileMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FILE);
			int nbLRFile = pNppParam->getNbLRFile();
			int pos = IDM_FILEMENU_LASTONE - IDM_FILE + 2;

			_lastRecentFileList.initMenu(hFileMenu, IDM_FILEMENU_LASTONE + 1, pos);
			_lastRecentFileList.setLangEncoding(_nativeLangEncoding);
			for (int i = 0 ; i < nbLRFile ; i++)
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
			pNppParam->getContextMenuFromXmlTree(_mainMenuHandle);

			if (pNppParam->hasCustomContextMenu())
			{
				_mainEditView.execute(SCI_USEPOPUP, FALSE);
				_subEditView.execute(SCI_USEPOPUP, FALSE);
			}

			generic_string pluginsTrans, windowTrans;
			changeMenuLang(pluginsTrans, windowTrans);
			
			if (_pluginsManager.hasPlugins() && pluginsTrans != TEXT(""))
			{
				::ModifyMenu(_mainMenuHandle, MENUINDEX_PLUGINS, MF_BYPOSITION, 0, pluginsTrans.c_str());
			}
			//Windows menu
			_windowsMenu.init(_hInst, _mainMenuHandle, windowTrans.c_str());

			// Update context menu strings
			vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
			size_t len = tmp.size();
			TCHAR menuName[64];
			for (size_t i = 0 ; i < len ; i++)
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

			for(size_t i = 0; i < len; i++) 
			{
				CommandShortcut & csc = shortcuts[i];
				if (!csc.getName()[0]) 
				{	//no predefined name, get name from menu and use that
					::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND);
					csc.setName(purgeMenuItemString(menuName, true).c_str());
				}
			}
			//Translate non-menu shortcuts
			changeShortcutLang();

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

			_toolBar.init(_hInst, hwnd, tbStatus, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit));

			changeToolBarIcons();

			_rebarTop.init(_hInst, hwnd);
			_rebarBottom.init(_hInst, hwnd);
			_toolBar.addToRebar(&_rebarTop);
			_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, willBeShown);

			//--Init dialogs--//
            _findReplaceDlg.init(_hInst, hwnd, &_pEditView);
			_incrementFindDlg.init(_hInst, hwnd, &_findReplaceDlg, _isRTL);
			_incrementFindDlg.addToRebar(&_rebarBottom);
            _goToLineDlg.init(_hInst, hwnd, &_pEditView);
			_colEditorDlg.init(_hInst, hwnd, &_pEditView);
            _aboutDlg.init(_hInst, hwnd);
			_runDlg.init(_hInst, hwnd);
			_runMacroDlg.init(_hInst, hwnd);

            //--User Define Dialog Section--//
			int uddStatus = nppGUI._userDefineDlgStatus;
		    UserDefineDialog *udd = _pEditView->getUserDefineDlg();

			bool uddShow = false;
			switch (uddStatus)
            {
                case UDD_SHOW :                 // show & undocked
					udd->doDialog(true, _isRTL);
					changeUserDefineLang();
					uddShow = true;
                    break;
                case UDD_DOCKED : {              // hide & docked
					_isUDDocked = true;
                    break;}
                case (UDD_SHOW | UDD_DOCKED) :    // show & docked
		            udd->doDialog(true, _isRTL);
					changeUserDefineLang();
		            ::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);
					uddShow = true;
                    break;

				default :                        // hide & undocked
					break;
            }
            		// UserDefine Dialog
			
			checkMenuItem(IDM_VIEW_USER_DLG, uddShow);
			_toolBar.setCheck(IDM_VIEW_USER_DLG, uddShow);

			//launch the plugin dlg memorized at the last session
			DockingManagerData &dmd = nppGUI._dockingData;

			_dockingManager.setDockedContSize(CONT_LEFT  , nppGUI._dockingData._leftWidth);
			_dockingManager.setDockedContSize(CONT_RIGHT , nppGUI._dockingData._rightWidth);
			_dockingManager.setDockedContSize(CONT_TOP	 , nppGUI._dockingData._topHeight);
			_dockingManager.setDockedContSize(CONT_BOTTOM, nppGUI._dockingData._bottomHight);

			for (size_t i = 0 ; i < dmd._pluginDockInfo.size() ; i++)
			{
				PlugingDlgDockingInfo & pdi = dmd._pluginDockInfo[i];

				if (pdi._isVisible)
					_pluginsManager.runPluginCommand(pdi._name.c_str(), pdi._internalID);
			}

			for (size_t i = 0 ; i < dmd._containerTabInfo.size() ; i++)
			{
				ContainerTabInfo & cti = dmd._containerTabInfo[i];
				_dockingManager.setActiveTab(cti._cont, cti._activeTab);
			}
			//Load initial docs into doctab
			loadBufferIntoView(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
			loadBufferIntoView(_subEditView.getCurrentBufferID(), SUB_VIEW);
			activateBuffer(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
			activateBuffer(_subEditView.getCurrentBufferID(), SUB_VIEW);
			MainFileManager->increaseDocNr();	//so next doc starts at 2

			::SetFocus(_mainEditView.getHSelf());
			result = TRUE;
		}
		break;

		case WM_DRAWITEM :
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			if (dis->CtlType == ODT_TAB)
			{
				return ::SendMessage(dis->hwndItem, WM_DRAWITEM, wParam, lParam);
			}
		}

		case WM_DOCK_USERDEFINE_DLG:
		{
			dockUserDlg();
			return TRUE;
		}

        case WM_UNDOCK_USERDEFINE_DLG:
		{
            undockUserDlg();
			return TRUE;
		}

		case WM_REMOVE_USERLANG:
		{
			TCHAR *userLangName = (TCHAR *)lParam;
			if (!userLangName || !userLangName[0])
				return FALSE;
            generic_string name(userLangName);

			//loop through buffers and reset the language (L_USER, TEXT("")) if (L_USER, name)
			Buffer * buf;
			for(int i = 0; i < MainFileManager->getNrBuffers(); i++)
			{
				buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && name == buf->getUserDefineLangName())
					buf->setLangType(L_USER, TEXT(""));
			}
			return TRUE;
		}

        case WM_RENAME_USERLANG:
		{
			if (!lParam || !(((TCHAR *)lParam)[0]) || !wParam || !(((TCHAR *)wParam)[0]))
				return FALSE;

            generic_string oldName((TCHAR *)lParam);
			generic_string newName((TCHAR *)wParam);

			//loop through buffers and reset the language (L_USER, newName) if (L_USER, oldName)
			Buffer * buf;
			for(int i = 0; i < MainFileManager->getNrBuffers(); i++)
			{
				buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && oldName == buf->getUserDefineLangName())
					buf->setLangType(L_USER, newName.c_str());
			}
			return TRUE;
		}

		case WM_CLOSE_USERDEFINE_DLG :
		{
			checkMenuItem(IDM_VIEW_USER_DLG, false);
			_toolBar.setCheck(IDM_VIEW_USER_DLG, false);
			return TRUE;
		}

		case WM_REPLACEALL_INOPENEDDOC :
		{
			replaceAllFiles();
			return TRUE;
		}

		case WM_FINDALL_INOPENEDDOC :
		{
			findInOpenedFiles();
			return TRUE;
		}

		case WM_FINDALL_INCURRENTDOC :
		{
			findInCurrentFile();
			return TRUE;
		}
		
		case WM_FINDINFILES :
		{
			return findInFiles();
		}

		case WM_REPLACEINFILES :
		{
			replaceInFiles();
			return TRUE;
		}
		case NPPM_LAUNCHFINDINFILESDLG :
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _isRTL);

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			if (isFirstTime)
				changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
			_findReplaceDlg.launchFindInFilesDlg();
			setFindReplaceFolderFilter((const TCHAR*) wParam, (const TCHAR*) lParam);
			return TRUE;
		}

		case NPPM_DOOPEN:
		case WM_DOOPEN:
		{
			BufferID id = doOpen((const TCHAR *)lParam);
			if (id != BUFFER_INVALID)
			{
				return switchToFile(id);
			}
		}
		break;

		case NPPM_INTERNAL_SETFILENAME:
		{
			if (!lParam && !wParam)
				return FALSE;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			if (b && b->getStatus() == DOC_UNNAMED) {
				b->setFileName((const TCHAR*)lParam);
				return TRUE;
			}
			return FALSE;
		}
		break;

		case NPPM_GETBUFFERLANGTYPE:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getLangType();
		}
		break;

		case NPPM_SETBUFFERLANGTYPE:
		{
			if (!wParam)
				return FALSE;
			if (lParam < L_TEXT || lParam >= L_EXTERNAL || lParam == L_USER)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setLangType((LangType)lParam);
			return TRUE;
		}
		break;

		case NPPM_GETBUFFERENCODING:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getUnicodeMode();
		}
		break;

		case NPPM_SETBUFFERENCODING:
		{
			if (!wParam)
				return FALSE;
			if (lParam < uni8Bit || lParam >= uniEnd)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			if (b->getStatus() != DOC_UNNAMED || b->isDirty())	//do not allow to change the encoding if the file has any content
				return FALSE;
			b->setUnicodeMode((UniMode)lParam);
			return TRUE;
		}
		break;

		case NPPM_GETBUFFERFORMAT:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getFormat();
		}
		break;

		case NPPM_SETBUFFERFORMAT:
		{
			if (!wParam)
				return FALSE;
			if (lParam < WIN_FORMAT || lParam >= UNIX_FORMAT)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setFormat((formatType)lParam);
			return TRUE;
		}
		break;

		case NPPM_GETBUFFERIDFROMPOS:
		{
			DocTabView * pView = NULL;
			if (lParam == MAIN_VIEW) {
				pView = &_mainDocTab;
			} else if (lParam == SUB_VIEW) {
				pView = &_subDocTab;
			} else {
				return (LRESULT)BUFFER_INVALID;
			}
			if ((int)wParam < pView->nbItem()) {
				return (LRESULT)(pView->getBufferByIndex((int)wParam));
			}
			return (LRESULT)BUFFER_INVALID;
		}
		break;

		case NPPM_GETCURRENTBUFFERID:
		{
			return (LRESULT)(_pEditView->getCurrentBufferID());
		}
		break;

		case NPPM_RELOADBUFFERID:
		{
			if (!wParam)
				return FALSE;
			return doReload((BufferID)wParam, lParam != 0);
		}
		break;

		case NPPM_RELOADFILE:
		{
			BufferID id = MainFileManager->getBufferFromName((const TCHAR *)lParam);
			if (id != BUFFER_INVALID)
				doReload(id, wParam != 0);
		}
		break;

		case NPPM_SWITCHTOFILE :
		{
			BufferID id = MainFileManager->getBufferFromName((const TCHAR *)lParam);
			if (id != BUFFER_INVALID)
				return switchToFile(id);
			return false;
		}

		case NPPM_SAVECURRENTFILE:
		{
			return fileSave();
		}
		break;

		case NPPM_SAVEALLFILES:
		{
			return fileSaveAll();
		}
		break;

		case NPPM_INTERNAL_DOCORDERCHANGED :
		{
			BufferID id = _pEditView->getCurrentBufferID();

			// Notify plugins that current file is about to be closed
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_DOCORDERCHANGED;
			scnN.nmhdr.hwndFrom = (void *)lParam;
			scnN.nmhdr.idFrom = (uptr_t)id;
			_pluginsManager.notify(&scnN);
			return TRUE;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			getClientRect(rc);
			if (lParam == 0) {
				lParam = MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top);
			}

			::MoveWindow(_rebarTop.getHSelf(), 0, 0, rc.right, _rebarTop.getHeight(), TRUE);
			_statusBar.adjustParts(rc.right);
			::SendMessage(_statusBar.getHSelf(), WM_SIZE, wParam, lParam);

			int rebarBottomHeight = _rebarBottom.getHeight();
			int statusBarHeight = _statusBar.getHeight();
			::MoveWindow(_rebarBottom.getHSelf(), 0, rc.bottom - rebarBottomHeight - statusBarHeight, rc.right, rebarBottomHeight, TRUE);
			
			getMainClientRect(rc);
			_dockingManager.reSizeTo(rc);

			result = TRUE;
		}
		break;

		case WM_MOVE:
		{
			result = TRUE;
		}
		break;

		case WM_MOVING:
		{
			result = FALSE;
		}
		break;

		case WM_SIZING:
		{
			result = FALSE;
		}
		break;
		
		case WM_COPYDATA :
        {
            COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT *)lParam;

			switch (pCopyData->dwData)
			{
				case COPYDATA_PARAMS :
				{
                    CmdLineParams *cmdLineParam = (CmdLineParams *)pCopyData->lpData;
					pNppParam->setCmdlineParam(*cmdLineParam);
                    _rememberThisSession = !cmdLineParam->_isNoSession;
					break;
				}

				case COPYDATA_FILENAMESA :
				{
					char *fileNamesA = (char *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *fileNamesW = wmc->char2wchar(fileNamesA, CP_ACP);
					loadCommandlineParams(fileNamesW, &cmdLineParams);
#else
					loadCommandlineParams(fileNamesA, &cmdLineParams);
#endif
					break;
				}

				case COPYDATA_FILENAMESW :
				{
					wchar_t *fileNamesW = (wchar_t *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
					
#ifdef UNICODE
					loadCommandlineParams(fileNamesW, &cmdLineParams);
#else
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const char *fileNamesA = wmc->wchar2char(fileNamesW, CP_ACP);
					loadCommandlineParams(fileNamesA, &cmdLineParams);
#endif
					break;
				}
			}

            return TRUE;
        }

		case WM_COMMAND:
            if (HIWORD(wParam) == SCEN_SETFOCUS)
            {
				HWND hMain = _mainEditView.getHSelf(), hSec = _subEditView.getHSelf();
				HWND hFocus = (HWND)lParam;
				if (hMain == hFocus)
					switchEditViewTo(MAIN_VIEW);
				else if (hSec == hFocus)
					switchEditViewTo(SUB_VIEW);
				else {
					//Other Scintilla, ignore
				}
				return TRUE;
            }
            else
			{
				if ((lParam == 1) || (lParam == 2))
				{
					specialCmd(LOWORD(wParam), lParam);
				}
				else
					command(LOWORD(wParam));
			}
			return TRUE;

		case NPPM_INTERNAL_RELOADNATIVELANG:
		{
			reloadLang();
		}
		return TRUE;

		case NPPM_INTERNAL_RELOADSTYLERS:
		{
			loadStyles();
		}
		return TRUE;

		case NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED:
		{
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_SHORTCUTREMAPPED;
			scnN.nmhdr.hwndFrom = (void *)lParam; // ShortcutKey structure
			scnN.nmhdr.idFrom = (uptr_t)wParam; // cmdID
			_pluginsManager.notify(&scnN);
		}
		return TRUE;

		case NPPM_GETSHORTCUTBYCMDID:
		{
			int cmdID = wParam; // cmdID
			ShortcutKey *sk = (ShortcutKey *)lParam; // ShortcutKey structure

			return _pluginsManager.getShortcutByCmdID(cmdID, sk);
		}

		case NPPM_MENUCOMMAND :
			command(lParam);
			return TRUE;

		case NPPM_GETFULLCURRENTPATH :
		case NPPM_GETCURRENTDIRECTORY :
		case NPPM_GETFILENAME :
		case NPPM_GETNAMEPART :
		case NPPM_GETEXTPART :
		{
			TCHAR str[MAX_PATH];
			// par defaut : NPPM_GETCURRENTDIRECTORY
			TCHAR *fileStr = lstrcpy(str, _pEditView->getCurrentBuffer()->getFullPathName());

			if (Message == NPPM_GETCURRENTDIRECTORY)
				PathRemoveFileSpec(str);
			else if (Message == NPPM_GETFILENAME)
				fileStr = PathFindFileName(str);
			else if (Message == NPPM_GETNAMEPART)
			{
				fileStr = PathFindFileName(str);
				PathRemoveExtension(fileStr);
			}
			else if (Message == NPPM_GETEXTPART)
				fileStr = PathFindExtension(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(fileStr) >= int(wParam))
				{
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD :
		{
			const int strSize = CURRENTWORD_MAXLENGTH; 
			TCHAR str[strSize];

			_pEditView->getGenericSelectedText((TCHAR *)str, strSize);
			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))	//buffer too small
				{
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETCURRENTWORD error"), MB_OK);
					return FALSE;
				}
				else //buffer large enough, perform safe copy
				{
					lstrcpyn((TCHAR *)lParam, str, wParam);
					return TRUE;
				}
			}

			lstrcpy((TCHAR *)lParam, str);
			return TRUE;
		}

		case NPPM_GETNPPDIRECTORY :
		{
			const int strSize = MAX_PATH;
			TCHAR str[strSize];

			::GetModuleFileName(NULL, str, strSize);
			PathRemoveFileSpec(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))
				{
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETNPPDIRECTORY error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, str);
			return TRUE;
		}

		case NPPM_GETCURRENTLINE :
		{
			return _pEditView->getCurrentLineNumber();
		}

		case NPPM_GETCURRENTCOLUMN :
		{
			return _pEditView->getCurrentColumnNumber();
		}

		case NPPM_GETCURRENTSCINTILLA : 
		{ 
			if (_pEditView == &_mainEditView) 
				*((int *)lParam) = MAIN_VIEW; 
			else if (_pEditView == &_subEditView) 
				*((int *)lParam) = SUB_VIEW; 
			else 
				*((int *)lParam) = -1; 
			return TRUE; 
		} 

		case NPPM_GETCURRENTLANGTYPE :
		{
			*((LangType *)lParam) = _pEditView->getCurrentBuffer()->getLangType();
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE :
		{
			_pEditView->getCurrentBuffer()->setLangType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETNBOPENFILES :
		{
			int nbDocPrimary = _mainDocTab.nbItem();
			int nbDocSecond = _subDocTab.nbItem();
			if (lParam == ALL_OPEN_FILES)
				return nbDocPrimary + nbDocSecond;
			else if (lParam == PRIMARY_VIEW)
				return  nbDocPrimary;
			else if (lParam == SECOND_VIEW)
				return  nbDocSecond;
		}

		case NPPM_GETOPENFILENAMESPRIMARY :
		case NPPM_GETOPENFILENAMESSECOND :
		case NPPM_GETOPENFILENAMES :
		{
			if (!wParam) return 0;

			TCHAR **fileNames = (TCHAR **)wParam;
			int nbFileNames = lParam;

			int j = 0;
			if (Message != NPPM_GETOPENFILENAMESSECOND) {
				for (int i = 0 ; i < _mainDocTab.nbItem() && j < nbFileNames ; i++)
				{
					BufferID id = _mainDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}
			if (Message != NPPM_GETOPENFILENAMESPRIMARY) {
				for (int i = 0 ; i < _subDocTab.nbItem() && j < nbFileNames ; i++)
				{
					BufferID id = _subDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}
			return j;
		}

		case WM_GETTASKLISTINFO :
		{
			if (!wParam) return 0;
			TaskListInfo * tli = (TaskListInfo *)wParam;
			getTaskListInfo(tli);

			if (NppParameters::getInstance()->getNppGUI()._styleMRU)
			{
				tli->_currentIndex = 0;
				std::sort(tli->_tlfsLst.begin(),tli->_tlfsLst.end(),SortTaskListPred(_mainDocTab,_subDocTab));
			}
			else
			{
				for(int idx = 0; idx < (int)tli->_tlfsLst.size(); ++idx)
				{
					if(tli->_tlfsLst[idx]._iView == currentView() &&
						tli->_tlfsLst[idx]._docIndex == _pDocTab->getCurrentTabIndex())
					{
						tli->_currentIndex = idx;
						break;
					}
				}
			}
			return TRUE;
		}

		case WM_MOUSEWHEEL :
		{
			if (LOWORD(wParam) & MK_RBUTTON)
			{
				// redirect to the IDC_PREV_DOC or IDC_NEXT_DOC so that we have the unified process

				pNppParam->_isTaskListRBUTTONUP_Active = true;
				short zDelta = (short) HIWORD(wParam);
				return ::SendMessage(_hSelf, WM_COMMAND, zDelta>0?IDC_PREV_DOC:IDC_NEXT_DOC, 0);
			}
			return TRUE;
		}
		
		case WM_APPCOMMAND :
		{
			switch(GET_APPCOMMAND_LPARAM(lParam))
			{
				case APPCOMMAND_BROWSER_BACKWARD :
				case APPCOMMAND_BROWSER_FORWARD :
					int nbDoc = viewVisible(MAIN_VIEW)?_mainDocTab.nbItem():0;
					nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
					if (nbDoc > 1)
						activateNextDoc((GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)?dirDown:dirUp);
					_linkTriggered = true;
			}
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case NPPM_GETNBSESSIONFILES :
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			if ((!sessionFileName) || (sessionFileName[0] == '\0')) return 0;
			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				return session2Load.nbMainFiles() + session2Load.nbSubFiles();
			}
			return 0;
		}
		
		case NPPM_GETSESSIONFILES :
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			TCHAR **sessionFileArray = (TCHAR **)wParam;

			if ((!sessionFileName) || (sessionFileName[0] == '\0')) return FALSE;

			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				size_t i = 0;
				for ( ; i < session2Load.nbMainFiles() ; )
				{
					const TCHAR *pFn = session2Load._mainViewFiles[i]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}

				for (size_t j = 0 ; j < session2Load.nbSubFiles() ; j++)
				{
					const TCHAR *pFn = session2Load._subViewFiles[j]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_DECODESCI:
		{
			// convert to ASCII
			Utf8_16_Write     UnicodeConvertor;
			UINT            length  = 0;
			char*            buffer  = NULL;
			ScintillaEditView *pSci;

			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;
			

			// get text of current scintilla
			length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

			// convert here      
			UniMode unicodeMode = pSci->getCurrentBuffer()->getUnicodeMode();
			UnicodeConvertor.setEncoding(unicodeMode);
			length = UnicodeConvertor.convert(buffer, length-1);

			// set text in target
			pSci->execute(SCI_CLEARALL);
			pSci->addText(length, UnicodeConvertor.getNewBuf());
			pSci->execute(SCI_EMPTYUNDOBUFFER);

			pSci->execute(SCI_SETCODEPAGE);

			// set cursor position
			pSci->execute(SCI_GOTOPOS);

			// clean buffer
			delete [] buffer;

			return unicodeMode;
		}

		case NPPM_ENCODESCI:
		{
			// convert
			Utf8_16_Read    UnicodeConvertor;
			UINT            length  = 0;
			char*            buffer  = NULL;
			ScintillaEditView *pSci;

			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;

			// get text of current scintilla
			length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

			length = UnicodeConvertor.convert(buffer, length-1);

			// set text in target
			pSci->execute(SCI_CLEARALL);
			pSci->addText(length, UnicodeConvertor.getNewBuf());

			

			pSci->execute(SCI_EMPTYUNDOBUFFER);

			// set cursor position
			pSci->execute(SCI_GOTOPOS);

			// clean buffer
			delete [] buffer;

			// set new encoding if BOM was changed by other programms
			UniMode um = UnicodeConvertor.getEncoding();
			(pSci->getCurrentBuffer())->setUnicodeMode(um);
			(pSci->getCurrentBuffer())->setDirty(true);
			return um;
		}

		case NPPM_ACTIVATEDOC :
		case NPPM_TRIGGERTABBARCONTEXTMENU:
		{
			// similar to NPPM_ACTIVEDOC
			int whichView = ((wParam != MAIN_VIEW) && (wParam != SUB_VIEW))?currentView():wParam;
			int index = lParam;

			switchEditViewTo(whichView);
			activateDoc(index);

			if (Message == NPPM_TRIGGERTABBARCONTEXTMENU)
			{
				// open here tab menu
				NMHDR	nmhdr;
				nmhdr.code = NM_RCLICK;

				nmhdr.hwndFrom = (whichView == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();

				nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
				::SendMessage(_hSelf, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
			}
			return TRUE;
		}

		case NPPM_GETNPPVERSION:
		{
			const TCHAR * verStr = VERSION_VALUE;
			TCHAR mainVerStr[16];
			TCHAR auxVerStr[16];
			bool isDot = false;
			int j =0;
			int k = 0;
			for (int i = 0 ; verStr[i] ; i++)
			{
				if (verStr[i] == '.')
					isDot = true;
				else
				{
					if (!isDot)
						mainVerStr[j++] = verStr[i];
					else
						auxVerStr[k++] = verStr[i];
				}
			}
			mainVerStr[j] = '\0';
			auxVerStr[k] = '\0';

			int mainVer = 0, auxVer = 0;
			if (mainVerStr)
				mainVer = generic_atoi(mainVerStr);
			if (auxVerStr)
				auxVer = generic_atoi(auxVerStr);

			return MAKELONG(auxVer, mainVer);
		}

		case WM_ISCURRENTMACRORECORDED :
			return (!_macro.empty() && !_recordingMacro);

		case WM_MACRODLGRUNMACRO:
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				int times = 1;
				if (_runMacroDlg.getMode() == RM_RUN_MULTI)
				{
					times = _runMacroDlg.getTimes();
				}
				else if (_runMacroDlg.getMode() == RM_RUN_EOF)
				{
					times = -1;
				}
				else
				{
					break;
				}
                          
				int counter = 0;
				int lastLine = int(_pEditView->execute(SCI_GETLINECOUNT)) - 1;
				int currLine = _pEditView->getCurrentLineNumber();
				int indexMacro = _runMacroDlg.getMacro2Exec();
				int deltaLastLine = 0;
				int deltaCurrLine = 0;

				Macro m = _macro;

				if (indexMacro != -1)
				{
					vector<MacroShortcut> & ms = pNppParam->getMacroList();
					m = ms[indexMacro].getMacro();
				}

				_pEditView->execute(SCI_BEGINUNDOACTION);
				for(;;)
				{
					for (Macro::iterator step = m.begin(); step != m.end(); step++)
						step->PlayBack(this, _pEditView);
						
					counter++;
					if ( times >= 0 )
					{
						if ( counter >= times ) break;
					}
					else // run until eof
					{
						bool cursorMovedUp = deltaCurrLine < 0;
						deltaLastLine = int(_pEditView->execute(SCI_GETLINECOUNT)) - 1 - lastLine;
						deltaCurrLine = _pEditView->getCurrentLineNumber() - currLine;

						if (( deltaCurrLine == 0 )	// line no. not changed?
							&& (deltaLastLine >= 0))  // and no lines removed?
							break; // exit

						// Update the line count, but only if the number of lines remaining is shrinking.
						// Otherwise, the macro playback may never end.
						if (deltaLastLine < deltaCurrLine)
							lastLine += deltaLastLine;

						// save current line
						currLine += deltaCurrLine;

						// eof?
						if ((currLine >= lastLine) || (currLine < 0) 
							|| ((deltaCurrLine == 0) && (currLine == 0) && ((deltaLastLine >= 0) || cursorMovedUp)))
							break;
					}
				}
				_pEditView->execute(SCI_ENDUNDOACTION);
			}
		}
		break;

		case NPPM_CREATESCINTILLAHANDLE :
		{
			return (LRESULT)_scintillaCtrls4Plugins.createSintilla((lParam == NULL?_hSelf:(HWND)lParam));
		}
		
		case NPPM_DESTROYSCINTILLAHANDLE :
		{
			return _scintillaCtrls4Plugins.destroyScintilla((HWND)lParam);
		}

		case NPPM_GETNBUSERLANG :
		{
			if (lParam)
				*((int *)lParam) = IDM_LANG_USER;
			return pNppParam->getNbUserLang();
		}

		case NPPM_GETCURRENTDOCINDEX :
		{
			if (lParam == SUB_VIEW)
			{
				if (!viewVisible(SUB_VIEW))
					return -1;
				return _subDocTab.getCurrentTabIndex();
			}
			else //MAIN_VIEW
			{
				if (!viewVisible(MAIN_VIEW))
					return -1;
				return _mainDocTab.getCurrentTabIndex();
			}
		}

		case NPPM_SETSTATUSBAR :
		{
			TCHAR *str2set = (TCHAR *)lParam;
			if (!str2set || !str2set[0])
				return FALSE;

			switch (wParam)
			{
				case STATUSBAR_DOC_TYPE :
				case STATUSBAR_DOC_SIZE :
				case STATUSBAR_CUR_POS :
				case STATUSBAR_EOF_FORMAT :
				case STATUSBAR_UNICODE_TYPE :
				case STATUSBAR_TYPING_MODE :
					_statusBar.setText(str2set, wParam);
					return TRUE;
				default :
					return FALSE;
			}
		}

		case NPPM_GETMENUHANDLE :
		{
			if (wParam == NPPPLUGINMENU)
				return (LRESULT)_pluginsManager.getMenuHandle();
			else
				return NULL;
		}

		case NPPM_LOADSESSION :
		{
			fileLoadSession((const TCHAR *)lParam);
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION :
		{
			return (LRESULT)fileSaveSession(0, NULL, (const TCHAR *)lParam);
		}

		case NPPM_SAVESESSION :
		{
			sessionInfo *pSi = (sessionInfo *)lParam;
			return (LRESULT)fileSaveSession(pSi->nbFile, pSi->files, pSi->sessionFilePathName);
		}

		case NPPM_INTERNAL_CLEARSCINTILLAKEY :
		{
			_mainEditView.execute(SCI_CLEARCMDKEY, wParam);
			_subEditView.execute(SCI_CLEARCMDKEY, wParam);
			return TRUE;		
		}
		case NPPM_INTERNAL_BINDSCINTILLAKEY :
		{
			_mainEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			_subEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			
			return TRUE;
		}
		case NPPM_INTERNAL_CMDLIST_MODIFIED :
		{
			//changeMenuShortcut(lParam, (const TCHAR *)wParam);
			::DrawMenuBar(_hSelf);
			return TRUE;
		}

		case NPPM_INTERNAL_MACROLIST_MODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_USERCMDLIST_MODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETWIDTH :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();

			if (nppGUI._caretWidth < 4)
			{
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_mainEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
				_subEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
			}
			else
			{
				_mainEditView.execute(SCI_SETCARETWIDTH, 1);
				_subEditView.execute(SCI_SETCARETWIDTH, 1);
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
			}
			return TRUE;
		}

        case NPPM_INTERNAL_SETMULTISELCTION :
        {
            NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
            _mainEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			_subEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
            return TRUE;
        }

		case NPPM_INTERNAL_SETCARETBLINKRATE :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			_mainEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			_subEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			return TRUE;
		}

		case NPPM_INTERNAL_ISTABBARREDUCED :
		{
			return _toReduceTabBar?TRUE:FALSE;
		}

		// ADD: success->hwnd; failure->NULL
		// REMOVE: success->NULL; failure->hwnd
		case NPPM_MODELESSDIALOG :
		{
			if (wParam == MODELESSDIALOGADD)
			{
				for (size_t i = 0 ; i < _hModelessDlgs.size() ; i++)
					if (_hModelessDlgs[i] == (HWND)lParam)
						return NULL;
				_hModelessDlgs.push_back((HWND)lParam);
				return lParam;
			}
			else if (wParam == MODELESSDIALOGREMOVE)
			{
				for (size_t i = 0 ; i < _hModelessDlgs.size() ; i++)
					if (_hModelessDlgs[i] == (HWND)lParam)
					{
						vector<HWND>::iterator hDlg = _hModelessDlgs.begin() + i;
						_hModelessDlgs.erase(hDlg);
						return NULL;
					}
				return lParam;
			}
			return TRUE;
		}

		case WM_CONTEXTMENU :
		{
			if (pNppParam->_isTaskListRBUTTONUP_Active)
			{
				pNppParam->_isTaskListRBUTTONUP_Active = false;
			}
			else
			{
				if ((HWND(wParam) == _mainEditView.getHSelf()) || (HWND(wParam) == _subEditView.getHSelf()))
				{
					if ((HWND(wParam) == _mainEditView.getHSelf())) {
						switchEditViewTo(MAIN_VIEW);
					} else {
						switchEditViewTo(SUB_VIEW);
					}
					POINT p;
					::GetCursorPos(&p);
					ContextMenu scintillaContextmenu;
					vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
					vector<bool> isEnable;
					for (size_t i = 0 ; i < tmp.size() ; i++)
					{
						isEnable.push_back((::GetMenuState(_mainMenuHandle, tmp[i]._cmdID, MF_BYCOMMAND)&MF_DISABLED) == 0);
					}
					scintillaContextmenu.create(_hSelf, tmp);
					for (size_t i = 0 ; i < isEnable.size() ; i++)
						scintillaContextmenu.enableItem(tmp[i]._cmdID, isEnable[i]);

					scintillaContextmenu.display(p);
					return TRUE;
				}
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case WM_NOTIFY:
		{
			checkClipboard();
			checkUndoState();
			checkMacroState();
			_pluginsManager.notify(reinterpret_cast<SCNotification *>(lParam));
			return notify(reinterpret_cast<SCNotification *>(lParam));
		}

		case NPPM_INTERNAL_CHECKDOCSTATUS :
		case WM_ACTIVATEAPP :
		{
			if (wParam == TRUE) // if npp is about to be activated
			{
				const NppGUI & nppgui = pNppParam->getNppGUI();
				if (LOWORD(wParam) && (nppgui._fileAutoDetection != cdDisabled))
				{
					_activeAppInf._isActivated = true;
					checkModifiedDocument();
					return FALSE;
				}
			}
			break;
		}

		case NPPM_INTERNAL_GETCHECKDOCOPT :
		{
			return (LRESULT)((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection;
		}

		case NPPM_INTERNAL_SETCHECKDOCOPT :
		{
			// If nothing is changed by user, then we allow to set this value
			if (((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection == ((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetectionOriginalValue)
				((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection = (ChangeDetect)wParam;
			return TRUE;
		}

		case NPPM_GETPOSFROMBUFFERID :
		{
			int i;

			if ((i = _mainDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
			{
				long view = MAIN_VIEW;
				view <<= 30;
				return view|i;
			}
			if ((i = _subDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
			{
				long view = SUB_VIEW;
				view <<= 30;
				return view|i;
			}
			return -1;
		}

		case NPPM_GETFULLPATHFROMBUFFERID :
		{
			return MainFileManager->getFileNameFromBuffer((BufferID)wParam, (TCHAR *)lParam);
		}
		
		case NPPM_INTERNAL_ENABLECHECKDOCOPT:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (wParam == CHECKDOCOPT_NONE)
				nppgui._fileAutoDetection = cdDisabled;
			else if (wParam == CHECKDOCOPT_UPDATESILENTLY)
				nppgui._fileAutoDetection = cdAutoUpdate;
			else if (wParam == CHECKDOCOPT_UPDATEGO2END)
				nppgui._fileAutoDetection = cdGo2end;
			else if (wParam == (CHECKDOCOPT_UPDATESILENTLY | CHECKDOCOPT_UPDATEGO2END))
				nppgui._fileAutoDetection = cdAutoUpdateGo2end;

			return TRUE;
		}
		
		case WM_ACTIVATE :
			_pEditView->getFocus();
			return TRUE;

		case WM_DROPFILES:
		{
			dropFiles(reinterpret_cast<HDROP>(wParam));
			return TRUE;
		}

		case WM_UPDATESCINTILLAS:
		{
			//reset styler for change in Stylers.xml
			_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
			_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
			_mainEditView.performGlobalStyles();
			_subEditView.performGlobalStyles();
			_findReplaceDlg.updateFinderScintilla();

			drawTabbarColoursFromStylerArray();

			// Notify plugins of update to styles xml
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_WORDSTYLESUPDATED;
			scnN.nmhdr.hwndFrom = _hSelf;
			scnN.nmhdr.idFrom = (uptr_t) _pEditView->getCurrentBufferID();
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			if (_isPrelaunch)
			{
				SendMessage(_hSelf, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
            else
            {
                if (_pTrayIco)
                    _pTrayIco->doTrayIcon(REMOVE);

			    const NppGUI & nppgui = pNppParam->getNppGUI();
			    Session currentSession;
			    if (nppgui._rememberLastSession) 
			    {
				    getCurrentOpenedFiles(currentSession);
				    //Lock the recent file list so it isnt populated with opened files
				    //Causing them to show on restart even though they are loaded by session
				    _lastRecentFileList.setLock(true);	//only lock when the session is remembered
			    }
			    bool allClosed = fileCloseAll();	//try closing files before doing anything else
    			
			    if (nppgui._rememberLastSession) 
			    {
				    _lastRecentFileList.setLock(false);	//only lock when the session is remembered
			    }

			    if (!allClosed) 
			    {
				    //User cancelled the shutdown
				    return FALSE;
			    }

			    if (_beforeSpecialView.isFullScreen)	//closing, return to windowed mode
				    fullScreenToggle();
			    if (_beforeSpecialView.isPostIt)		//closing, return to windowed mode
				    postItToggle();

			    if (_configStyleDlg.isCreated() && ::IsWindowVisible(_configStyleDlg.getHSelf()))
				    _configStyleDlg.restoreGlobalOverrideValues();

			    SCNotification scnN;
			    scnN.nmhdr.code = NPPN_SHUTDOWN;
			    scnN.nmhdr.hwndFrom = _hSelf;
			    scnN.nmhdr.idFrom = 0;
			    _pluginsManager.notify(&scnN);

			    saveFindHistory();

			    _lastRecentFileList.saveLRFL();
			    saveScintillaParams(SCIV_PRIMARY);
			    saveScintillaParams(SCIV_SECOND);
			    saveGUIParams();
			    saveUserDefineLangs();
			    saveShortcuts();
			    if (nppgui._rememberLastSession && _rememberThisSession)
				    saveSession(currentSession);

                //Sends WM_DESTROY, Notepad++ will end
			    ::DestroyWindow(hwnd);
			}
			return TRUE;
		}

		case WM_DESTROY:
		{	
			killAllChildren();	
			::PostQuitMessage(0);
			gNppHWND = NULL;
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if ((nppgui._isMinimizedToTray || _isPrelaunch) && (wParam == SC_MINIMIZE))
			{
				if (!_pTrayIco)
					_pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

				_pTrayIco->doTrayIcon(ADD);
				::ShowWindow(hwnd, SW_HIDE);
				return TRUE;
			}
			
			if (wParam == SC_KEYMENU && lParam == VK_SPACE)
			{
				_sysMenuEntering = true;
			}
			else if (wParam == 0xF093) //it should be SC_MOUSEMENU. A bug?
			{
				_sysMenuEntering = true;
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case WM_LBUTTONDBLCLK:
		{
			::SendMessage(_hSelf, WM_COMMAND, IDM_FILE_NEW, 0);
			return TRUE;
		}

		case IDC_MINIMIZED_TRAY:
		{
			switch (lParam)
			{
				//case WM_LBUTTONDBLCLK:
				case WM_LBUTTONUP :
					_pEditView->getFocus();
					::ShowWindow(_hSelf, SW_SHOW);
					if (!_isPrelaunch)
						_pTrayIco->doTrayIcon(REMOVE);
					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					return TRUE;

				case WM_MBUTTONUP:
					command(IDM_SYSTRAYPOPUP_NEW_AND_PASTE);
					return TRUE;

 				case WM_RBUTTONUP:
 				{
 					POINT p;
 					GetCursorPos(&p);

					HMENU hmenu;            // menu template          
					HMENU hTrayIconMenu;  // shortcut menu   
					hmenu = ::LoadMenu(_hInst, MAKEINTRESOURCE(IDR_SYSTRAYPOPUP_MENU));
					hTrayIconMenu = ::GetSubMenu(hmenu, 0); 
					SetForegroundWindow(_hSelf);
					TrackPopupMenu(hTrayIconMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
					PostMessage(_hSelf, WM_NULL, 0, 0);
					DestroyMenu(hmenu); 
 					return TRUE; 
 				}

			}
			return TRUE;
		}

		case NPPM_DMMSHOW:
		{
			_dockingManager.showDockableDlg((HWND)lParam, SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMHIDE:
		{
			_dockingManager.showDockableDlg((HWND)lParam, SW_HIDE);
			return TRUE;
		}

		case NPPM_DMMUPDATEDISPINFO:
		{
			if (::IsWindowVisible((HWND)lParam))
				_dockingManager.updateContainerInfo((HWND)lParam);
			return TRUE;
		}

		case NPPM_DMMREGASDCKDLG:
		{
			tTbData *pData	= (tTbData *)lParam;
			int		iCont	= -1;
			bool	isVisible	= false;

			getIntegralDockingData(*pData, iCont, isVisible);
			_dockingManager.createDockableDlg(*pData, iCont, isVisible);
			return TRUE;
		}

		case NPPM_DMMVIEWOTHERTAB:
		{
			_dockingManager.showDockableDlg((TCHAR*)lParam, SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMGETPLUGINHWNDBYNAME : //(const TCHAR *windowName, const TCHAR *moduleName)
		{
			if (!lParam) return NULL;

			TCHAR *moduleName = (TCHAR *)lParam;
			TCHAR *windowName = (TCHAR *)wParam;
			vector<DockingCont *> dockContainer = _dockingManager.getContainerInfo();
			for (size_t i = 0 ; i < dockContainer.size() ; i++)
			{
				vector<tTbData *> tbData = dockContainer[i]->getDataOfAllTb();
				for (size_t j = 0 ; j < tbData.size() ; j++)
				{
					if (generic_stricmp(moduleName, tbData[j]->pszModuleName) == 0)
					{
						if (!windowName)
							return (LRESULT)tbData[j]->hClient;
						else if (generic_stricmp(windowName, tbData[j]->pszName) == 0)
							return (LRESULT)tbData[j]->hClient;
					}
				}
			}
			return NULL;
		}

		case NPPM_ADDTOOLBARICON:
		{
			_toolBar.registerDynBtn((UINT)wParam, (toolbarIcons*)lParam);
			return TRUE;
		}

		case NPPM_SETMENUITEMCHECK:
		{
			::CheckMenuItem(_mainMenuHandle, (UINT)wParam, MF_BYCOMMAND | ((BOOL)lParam ? MF_CHECKED : MF_UNCHECKED));
			_toolBar.setCheck((int)wParam, bool(lParam != 0));
			return TRUE;
		}

		case NPPM_GETWINDOWSVERSION:
		{
			return _winVersion;
		}

		case NPPM_MAKECURRENTBUFFERDIRTY :
		{
			_pEditView->getCurrentBuffer()->setDirty(true);
			return TRUE;
		}

		case NPPM_GETENABLETHEMETEXTUREFUNC :
		{
			return (LRESULT)pNppParam->getEnableThemeDlgTexture();
		}

		case NPPM_GETPLUGINSCONFIGDIR :
		{
			if (!lParam || !wParam)
				return FALSE;

			generic_string pluginsConfigDirPrefix = pNppParam->getAppDataNppDir();
			
			if (pluginsConfigDirPrefix == TEXT(""))
				pluginsConfigDirPrefix = pNppParam->getNppPath();

			const TCHAR *secondPart = TEXT("plugins\\Config");
			
			size_t len = wParam;
			if (len < pluginsConfigDirPrefix.length() + lstrlen(secondPart))
				return FALSE;

			TCHAR *pluginsConfigDir = (TCHAR *)lParam;			
			lstrcpy(pluginsConfigDir, pluginsConfigDirPrefix.c_str());

			::PathAppend(pluginsConfigDir, secondPart);
			return TRUE;
		}

		case NPPM_MSGTOPLUGIN :
		{
			return _pluginsManager.relayPluginMessages(Message, wParam, lParam);
		}

		case NPPM_HIDETABBAR :
		{
			bool hide = (lParam != 0);
			bool oldVal = DocTabView::getHideTabBarStatus();
			if (hide == oldVal) return oldVal;

			DocTabView::setHideTabBarStatus(hide);
			::SendMessage(_hSelf, WM_SIZE, 0, 0);

			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			if (hide)
				nppGUI._tabStatus |= TAB_HIDE;
			else
				nppGUI._tabStatus &= ~TAB_HIDE;

			return oldVal;
		}
		case NPPM_ISTABBARHIDDEN :
		{
			return _mainDocTab.getHideTabBarStatus();
		}


		case NPPM_HIDETOOLBAR :
		{
			bool show = (lParam != TRUE);
			bool currentStatus = _rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
			if (show != currentStatus)
				_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, show);
			return currentStatus;
		}
		case NPPM_ISTOOLBARHIDDEN :
		{
			return !_rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
		}

		case NPPM_HIDEMENU :
		{
			bool hide = (lParam == TRUE);
			bool isHidden = ::GetMenu(_hSelf) == NULL;
			if (hide == isHidden)
				return isHidden;

			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			nppGUI._menuBarShow = !hide;
			if (nppGUI._menuBarShow)
				::SetMenu(_hSelf, _mainMenuHandle);
			else
				::SetMenu(_hSelf, NULL);

			return isHidden;
		}
		case NPPM_ISMENUHIDDEN :
		{
			return (::GetMenu(_hSelf) == NULL);
		}

		case NPPM_HIDESTATUSBAR:
		{
			bool show = (lParam != TRUE);
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			bool oldVal = nppGUI._statusBarShow;
			if (show == oldVal)
			{
				return oldVal;
			}
            RECT rc;
			getClientRect(rc);
			
			nppGUI._statusBarShow = show;
            _statusBar.display(nppGUI._statusBarShow);
            ::SendMessage(_hSelf, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.bottom, rc.right));
            return oldVal;
        }

		case NPPM_ISSTATUSBARHIDDEN :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			return !nppGUI._statusBarShow;
		}

/*
		case NPPM_ADDREBAR :
		{
			if (!lParam)
				return FALSE;
			_rebarTop.addBand((REBARBANDINFO*)lParam, false);
			return TRUE;
		}

		case NPPM_UPDATEREBAR :
		{
			if (!lParam || wParam < REBAR_BAR_EXTERNAL)
				return FALSE;
			_rebarTop.reNew((int)wParam, (REBARBANDINFO*)lParam);
			return TRUE;
		}

		case NPPM_REMOVEREBAR :
		{
			if (wParam < REBAR_BAR_EXTERNAL)
				return FALSE;
			_rebarTop.removeBand((int)wParam);
			return TRUE;
		}
*/
		case NPPM_INTERNAL_ISFOCUSEDTAB :
		{
			HWND hTabToTest = (currentView() == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();
			return (HWND)lParam == hTabToTest;
		}

		case NPPM_INTERNAL_GETMENU :
		{
			return (LRESULT)_mainMenuHandle;
		}
	
		case NPPM_INTERNAL_CLEARINDICATOR :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
			return TRUE;
		}
		case NPPM_INTERNAL_CLEARINDICATORTAGMATCH :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGMATCH);
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}
		case NPPM_INTERNAL_CLEARINDICATORTAGATTR :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}

		case NPPM_INTERNAL_SWITCHVIEWFROMHWND :
		{
			HWND handle = (HWND)lParam;
			if (_mainEditView.getHSelf() == handle || _mainDocTab.getHSelf() == handle)
			{
				switchEditViewTo(MAIN_VIEW);
			}
			else if (_subEditView.getHSelf() == handle || _subDocTab.getHSelf() == handle)
			{
				switchEditViewTo(SUB_VIEW);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_UPDATETITLEBAR :
		{
			setTitle();
			return TRUE;
		}

		case NPPM_INTERNAL_DISABLEAUTOUPDATE :
		{
			//printStr(TEXT("you've got me"));
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			nppGUI._autoUpdateOpt._doAutoUpdate = false;
			return TRUE;
		}

		case WM_INITMENUPOPUP:
		{
			_windowsMenu.initPopupMenu((HMENU)wParam, _pDocTab);
			return TRUE;
		}

		case WM_ENTERMENULOOP:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(_hSelf, _mainMenuHandle);
				
			return TRUE;
		}

		case WM_EXITMENULOOP:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(_hSelf, NULL);
			_sysMenuEntering = false;
			return FALSE;
		}

		default:
		{
			if (Message == WDN_NOTIFY)
			{
				NMWINDLG* nmdlg = (NMWINDLG*)lParam;
				switch (nmdlg->type)
				{
					case WDT_ACTIVATE:
						activateDoc(nmdlg->curSel);
						nmdlg->processed = TRUE;
						break;
					case WDT_SAVE:
					{
						//loop through nmdlg->nItems, get index and save it
						for (int i = 0; i < (int)nmdlg->nItems; i++) {
							fileSave(_pDocTab->getBufferByIndex(i));
						}
						nmdlg->processed = TRUE;
					}
					break;
					case WDT_CLOSE:
					{
                        bool closed;

						//loop through nmdlg->nItems, get index and close it
						for (int i = 0; i < (int)nmdlg->nItems; i++)
                        {
							closed = fileClose(_pDocTab->getBufferByIndex(nmdlg->Items[i]), currentView());
							UINT pos = nmdlg->Items[i];
							// The window list only needs to be rearranged when the file was actually closed
							if (closed)
                            {
								nmdlg->Items[i] = 0xFFFFFFFF; // indicate file was closed

								// Shift the remaining items downward to fill the gap
								for (int j = i + 1; j < (int)nmdlg->nItems; j++)
                                {
									if (nmdlg->Items[j] > pos)
                                    {
										nmdlg->Items[j]--;
									}
								}
							}
						}
						nmdlg->processed = TRUE;
					}
					break;
					case WDT_SORT:
						if (nmdlg->nItems != (unsigned int)_pDocTab->nbItem())	//sanity check, if mismatch just abort
							break;
						//Collect all buffers
						std::vector<BufferID> tempBufs;
						for(int i = 0; i < (int)nmdlg->nItems; i++) {
							tempBufs.push_back(_pDocTab->getBufferByIndex(i));
						}
						//Reset buffers
						for(int i = 0; i < (int)nmdlg->nItems; i++) {
							_pDocTab->setBuffer(i, tempBufs[nmdlg->Items[i]]);
						}
						activateBuffer(_pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex()), currentView());
						break;
				}
				return TRUE;
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}
	}

	_pluginsManager.relayNppMessages(Message, wParam, lParam);
	return result;
}

