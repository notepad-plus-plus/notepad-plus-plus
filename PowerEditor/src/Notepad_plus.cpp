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

#include <time.h>
#include <shlwapi.h>
#include <wininet.h>
#include "Notepad_plus.h"
#include "Notepad_plus_Window.h"
#include "CustomFileDialog.h"
#include "Printer.h"
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
#include "fileBrowser.h"
#include "Common.h"
#include "NppDarkMode.h"

using namespace std;

enum tb_stat {tb_saved, tb_unsaved, tb_ro, tb_monitored};
#define DIR_LEFT true
#define DIR_RIGHT false

int docTabIconIDs[] = { IDI_SAVED_ICON,  IDI_UNSAVED_ICON,  IDI_READONLY_ICON,  IDI_MONITORING_ICON };
int docTabIconIDs_darkMode[] = { IDI_SAVED_DM_ICON,  IDI_UNSAVED_DM_ICON,  IDI_READONLY_DM_ICON,  IDI_MONITORING_DM_ICON };
int docTabIconIDs_alt[] = { IDI_SAVED_ALT_ICON, IDI_UNSAVED_ALT_ICON, IDI_READONLY_ALT_ICON, IDI_MONITORING_ICON };


ToolBarButtonUnit toolBarIcons[] = {
    {IDM_FILE_NEW,                     IDI_NEW_ICON,               IDI_NEW_ICON,                  IDI_NEW_ICON2,              IDI_NEW_ICON2,                 IDI_NEW_ICON_DM,               IDI_NEW_ICON_DM,                  IDI_NEW_ICON_DM2,              IDI_NEW_ICON_DM2,                 IDR_FILENEW},
    {IDM_FILE_OPEN,                    IDI_OPEN_ICON,              IDI_OPEN_ICON,                 IDI_OPEN_ICON2,             IDI_OPEN_ICON2,                IDI_OPEN_ICON_DM,              IDI_OPEN_ICON_DM,                 IDI_OPEN_ICON_DM2,             IDI_OPEN_ICON_DM2,                IDR_FILEOPEN},
    {IDM_FILE_SAVE,                    IDI_SAVE_ICON,              IDI_SAVE_DISABLE_ICON,         IDI_SAVE_ICON2,             IDI_SAVE_DISABLE_ICON2,        IDI_SAVE_ICON_DM,              IDI_SAVE_DISABLE_ICON_DM,         IDI_SAVE_ICON_DM2,             IDI_SAVE_DISABLE_ICON_DM2,        IDR_FILESAVE},
    {IDM_FILE_SAVEALL,                 IDI_SAVEALL_ICON,           IDI_SAVEALL_DISABLE_ICON,      IDI_SAVEALL_ICON2,          IDI_SAVEALL_DISABLE_ICON2,     IDI_SAVEALL_ICON_DM,           IDI_SAVEALL_DISABLE_ICON_DM,      IDI_SAVEALL_ICON_DM2,          IDI_SAVEALL_DISABLE_ICON_DM2,     IDR_SAVEALL},
    {IDM_FILE_CLOSE,                   IDI_CLOSE_ICON,             IDI_CLOSE_ICON,                IDI_CLOSE_ICON2,            IDI_CLOSE_ICON2,               IDI_CLOSE_ICON_DM,             IDI_CLOSE_ICON_DM,                IDI_CLOSE_ICON_DM2,            IDI_CLOSE_ICON_DM2,               IDR_CLOSEFILE},
    {IDM_FILE_CLOSEALL,                IDI_CLOSEALL_ICON,          IDI_CLOSEALL_ICON,             IDI_CLOSEALL_ICON2,         IDI_CLOSEALL_ICON2,            IDI_CLOSEALL_ICON_DM,          IDI_CLOSEALL_ICON_DM,             IDI_CLOSEALL_ICON_DM2,         IDI_CLOSEALL_ICON_DM2,            IDR_CLOSEALL},
    {IDM_FILE_PRINT,                   IDI_PRINT_ICON,             IDI_PRINT_ICON,                IDI_PRINT_ICON2,            IDI_PRINT_ICON2,               IDI_PRINT_ICON_DM,             IDI_PRINT_ICON_DM,                IDI_PRINT_ICON_DM2,            IDI_PRINT_ICON_DM2,               IDR_PRINT},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

    {IDM_EDIT_CUT,                     IDI_CUT_ICON,               IDI_CUT_DISABLE_ICON,          IDI_CUT_ICON2,              IDI_CUT_DISABLE_ICON2,         IDI_CUT_ICON_DM,               IDI_CUT_DISABLE_ICON_DM,          IDI_CUT_ICON_DM2,              IDI_CUT_DISABLE_ICON_DM2,         IDR_CUT},
    {IDM_EDIT_COPY,                    IDI_COPY_ICON,              IDI_COPY_DISABLE_ICON,         IDI_COPY_ICON2,             IDI_COPY_DISABLE_ICON2,        IDI_COPY_ICON_DM,              IDI_COPY_DISABLE_ICON_DM,         IDI_COPY_ICON_DM2,             IDI_COPY_DISABLE_ICON_DM2,        IDR_COPY},
    {IDM_EDIT_PASTE,                   IDI_PASTE_ICON,             IDI_PASTE_DISABLE_ICON,        IDI_PASTE_ICON2,            IDI_PASTE_DISABLE_ICON2,       IDI_PASTE_ICON_DM,             IDI_PASTE_DISABLE_ICON_DM,        IDI_PASTE_ICON_DM2,            IDI_PASTE_DISABLE_ICON_DM2,       IDR_PASTE},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

    {IDM_EDIT_UNDO,                    IDI_UNDO_ICON,              IDI_UNDO_DISABLE_ICON,         IDI_UNDO_ICON2,             IDI_UNDO_DISABLE_ICON2,        IDI_UNDO_ICON_DM,              IDI_UNDO_DISABLE_ICON_DM,         IDI_UNDO_ICON_DM2,             IDI_UNDO_DISABLE_ICON_DM2,        IDR_UNDO},
    {IDM_EDIT_REDO,                    IDI_REDO_ICON,              IDI_REDO_DISABLE_ICON,         IDI_REDO_ICON2,             IDI_REDO_DISABLE_ICON2,        IDI_REDO_ICON_DM,              IDI_REDO_DISABLE_ICON_DM,         IDI_REDO_ICON_DM2,             IDI_REDO_DISABLE_ICON_DM2,        IDR_REDO},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

    {IDM_SEARCH_FIND,                  IDI_FIND_ICON,              IDI_FIND_ICON,                 IDI_FIND_ICON2,             IDI_FIND_ICON2,                IDI_FIND_ICON_DM,              IDI_FIND_ICON_DM,                 IDI_FIND_ICON_DM2,             IDI_FIND_ICON_DM2,                IDR_FIND},
    {IDM_SEARCH_REPLACE,               IDI_REPLACE_ICON,           IDI_REPLACE_ICON,              IDI_REPLACE_ICON2,          IDI_REPLACE_ICON2,             IDI_REPLACE_ICON_DM,           IDI_REPLACE_ICON_DM,              IDI_REPLACE_ICON_DM2,          IDI_REPLACE_ICON_DM2,             IDR_REPLACE},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {IDM_VIEW_ZOOMIN,                  IDI_ZOOMIN_ICON,            IDI_ZOOMIN_ICON,               IDI_ZOOMIN_ICON2,           IDI_ZOOMIN_ICON2,              IDI_ZOOMIN_ICON_DM,            IDI_ZOOMIN_ICON_DM,               IDI_ZOOMIN_ICON_DM2,           IDI_ZOOMIN_ICON_DM2,              IDR_ZOOMIN},
    {IDM_VIEW_ZOOMOUT,                 IDI_ZOOMOUT_ICON,           IDI_ZOOMOUT_ICON,              IDI_ZOOMOUT_ICON2,          IDI_ZOOMOUT_ICON2,             IDI_ZOOMOUT_ICON_DM,           IDI_ZOOMOUT_ICON_DM,              IDI_ZOOMOUT_ICON_DM2,          IDI_ZOOMOUT_ICON_DM2,             IDR_ZOOMOUT},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {IDM_VIEW_SYNSCROLLV,              IDI_SYNCV_ICON,             IDI_SYNCV_DISABLE_ICON,        IDI_SYNCV_ICON2,            IDI_SYNCV_DISABLE_ICON2,       IDI_SYNCV_ICON_DM,             IDI_SYNCV_DISABLE_ICON_DM,        IDI_SYNCV_ICON_DM2,            IDI_SYNCV_DISABLE_ICON_DM2,       IDR_SYNCV},
    {IDM_VIEW_SYNSCROLLH,              IDI_SYNCH_ICON,             IDI_SYNCH_DISABLE_ICON,        IDI_SYNCH_ICON2,            IDI_SYNCH_DISABLE_ICON2,       IDI_SYNCH_ICON_DM,             IDI_SYNCH_DISABLE_ICON_DM,        IDI_SYNCH_ICON_DM2,            IDI_SYNCH_DISABLE_ICON_DM2,       IDR_SYNCH},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {IDM_VIEW_WRAP,                    IDI_VIEW_WRAP_ICON,         IDI_VIEW_WRAP_ICON,            IDI_VIEW_WRAP_ICON2,        IDI_VIEW_WRAP_ICON2,           IDI_VIEW_WRAP_ICON_DM,         IDI_VIEW_WRAP_ICON_DM,            IDI_VIEW_WRAP_ICON_DM2,        IDI_VIEW_WRAP_ICON_DM2,           IDR_WRAP},
    {IDM_VIEW_ALL_CHARACTERS,          IDI_VIEW_ALL_CHAR_ICON,     IDI_VIEW_ALL_CHAR_ICON,        IDI_VIEW_ALL_CHAR_ICON2,    IDI_VIEW_ALL_CHAR_ICON2,       IDI_VIEW_ALL_CHAR_ICON_DM,     IDI_VIEW_ALL_CHAR_ICON_DM,        IDI_VIEW_ALL_CHAR_ICON_DM2,    IDI_VIEW_ALL_CHAR_ICON_DM2,       IDR_INVISIBLECHAR},
    {IDM_VIEW_INDENT_GUIDE,            IDI_VIEW_INDENT_ICON,       IDI_VIEW_INDENT_ICON,          IDI_VIEW_INDENT_ICON2,      IDI_VIEW_INDENT_ICON2,         IDI_VIEW_INDENT_ICON_DM,       IDI_VIEW_INDENT_ICON_DM,          IDI_VIEW_INDENT_ICON_DM2,      IDI_VIEW_INDENT_ICON_DM2,         IDR_INDENTGUIDE},
    {IDM_LANG_USER_DLG,                IDI_VIEW_UD_DLG_ICON,       IDI_VIEW_UD_DLG_ICON,          IDI_VIEW_UD_DLG_ICON2,      IDI_VIEW_UD_DLG_ICON2,         IDI_VIEW_UD_DLG_ICON_DM,       IDI_VIEW_UD_DLG_ICON_DM,          IDI_VIEW_UD_DLG_ICON_DM2,      IDI_VIEW_UD_DLG_ICON_DM2,         IDR_SHOWPANNEL},
    {IDM_VIEW_DOC_MAP,                 IDI_VIEW_DOC_MAP_ICON,      IDI_VIEW_DOC_MAP_ICON,         IDI_VIEW_DOC_MAP_ICON2,     IDI_VIEW_DOC_MAP_ICON2,        IDI_VIEW_DOC_MAP_ICON_DM,      IDI_VIEW_DOC_MAP_ICON_DM,         IDI_VIEW_DOC_MAP_ICON_DM2,     IDI_VIEW_DOC_MAP_ICON_DM2,        IDR_DOCMAP},
    {IDM_VIEW_DOCLIST,                 IDI_VIEW_DOCLIST_ICON,      IDI_VIEW_DOCLIST_ICON,         IDI_VIEW_DOCLIST_ICON2,     IDI_VIEW_DOCLIST_ICON2,        IDI_VIEW_DOCLIST_ICON_DM,      IDI_VIEW_DOCLIST_ICON_DM,         IDI_VIEW_DOCLIST_ICON_DM2,     IDI_VIEW_DOCLIST_ICON_DM2,        IDR_DOCLIST},
    {IDM_VIEW_FUNC_LIST,               IDI_VIEW_FUNCLIST_ICON,     IDI_VIEW_FUNCLIST_ICON,        IDI_VIEW_FUNCLIST_ICON2,    IDI_VIEW_FUNCLIST_ICON2,       IDI_VIEW_FUNCLIST_ICON_DM,     IDI_VIEW_FUNCLIST_ICON_DM,        IDI_VIEW_FUNCLIST_ICON_DM2,    IDI_VIEW_FUNCLIST_ICON_DM2,       IDR_FUNC_LIST},
    {IDM_VIEW_FILEBROWSER,             IDI_VIEW_FILEBROWSER_ICON,  IDI_VIEW_FILEBROWSER_ICON,     IDI_VIEW_FILEBROWSER_ICON2, IDI_VIEW_FILEBROWSER_ICON2,    IDI_VIEW_FILEBROWSER_ICON_DM,  IDI_VIEW_FILEBROWSER_ICON_DM,     IDI_VIEW_FILEBROWSER_ICON_DM2, IDI_VIEW_FILEBROWSER_ICON_DM2,    IDR_FILEBROWSER},
    {IDM_VIEW_MONITORING,              IDI_VIEW_MONITORING_ICON,   IDI_VIEW_MONITORING_ICON,      IDI_VIEW_MONITORING_ICON2,  IDI_VIEW_MONITORING_ICON2,     IDI_VIEW_MONITORING_ICON_DM,   IDI_VIEW_MONITORING_ICON_DM,      IDI_VIEW_MONITORING_ICON_DM2,  IDI_VIEW_MONITORING_ICON_DM2,     IDR_FILEMONITORING},

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

    {IDM_MACRO_STARTRECORDINGMACRO,    IDI_STARTRECORD_ICON,       IDI_STARTRECORD_DISABLE_ICON,  IDI_STARTRECORD_ICON2,      IDI_STARTRECORD_DISABLE_ICON2, IDI_STARTRECORD_ICON_DM,       IDI_STARTRECORD_DISABLE_ICON_DM,  IDI_STARTRECORD_ICON_DM2,      IDI_STARTRECORD_DISABLE_ICON_DM2, IDR_STARTRECORD},
    {IDM_MACRO_STOPRECORDINGMACRO,     IDI_STOPRECORD_ICON,        IDI_STOPRECORD_DISABLE_ICON,   IDI_STOPRECORD_ICON2,       IDI_STOPRECORD_DISABLE_ICON2,  IDI_STOPRECORD_ICON_DM,        IDI_STOPRECORD_DISABLE_ICON_DM,   IDI_STOPRECORD_ICON_DM2,       IDI_STOPRECORD_DISABLE_ICON_DM2,  IDR_STOPRECORD},
    {IDM_MACRO_PLAYBACKRECORDEDMACRO,  IDI_PLAYRECORD_ICON,        IDI_PLAYRECORD_DISABLE_ICON,   IDI_PLAYRECORD_ICON2,       IDI_PLAYRECORD_DISABLE_ICON2,  IDI_PLAYRECORD_ICON_DM,        IDI_PLAYRECORD_DISABLE_ICON_DM,   IDI_PLAYRECORD_ICON_DM2,       IDI_PLAYRECORD_DISABLE_ICON_DM2,  IDR_PLAYRECORD},
    {IDM_MACRO_RUNMULTIMACRODLG,       IDI_MMPLAY_ICON,            IDI_MMPLAY_DIS_ICON,           IDI_MMPLAY_ICON2,           IDI_MMPLAY_DIS_ICON2,          IDI_MMPLAY_ICON_DM,            IDI_MMPLAY_DIS_ICON_DM,           IDI_MMPLAY_ICON_DM2,           IDI_MMPLAY_DIS_ICON_DM2,          IDR_M_PLAYRECORD},
    {IDM_MACRO_SAVECURRENTMACRO,       IDI_SAVERECORD_ICON,        IDI_SAVERECORD_DISABLE_ICON,   IDI_SAVERECORD_ICON2,       IDI_SAVERECORD_DISABLE_ICON2,  IDI_SAVERECORD_ICON_DM,        IDI_SAVERECORD_DISABLE_ICON_DM,   IDI_SAVERECORD_ICON_DM2,       IDI_SAVERECORD_DISABLE_ICON_DM2,  IDR_SAVERECORD}
};



Notepad_plus::Notepad_plus()
	: _autoCompleteMain(&_mainEditView)
	, _autoCompleteSub(&_subEditView)
	, _smartHighlighter(&_findReplaceDlg)
{
	ZeroMemory(&_prevSelectedRange, sizeof(_prevSelectedRange));

	NppParameters& nppParam = NppParameters::getInstance();
	TiXmlDocumentA *nativeLangDocRootA = nppParam.getNativeLangA();
    _nativeLangSpeaker.init(nativeLangDocRootA);

	LocalizationSwitcher & localizationSwitcher = nppParam.getLocalizationSwitcher();
    const char *fn = _nativeLangSpeaker.getFileName();
    if (fn)
    {
        localizationSwitcher.setFileName(fn);
    }

	nppParam.setNativeLangSpeaker(&_nativeLangSpeaker);

	TiXmlDocument *toolIconsDocRoot = nppParam.getCustomizedToolIcons();

	if (toolIconsDocRoot)
	{
        _toolBar.initTheme(toolIconsDocRoot);
    }

	// Determine if user is administrator.
	BOOL is_admin;
	winVer ver = nppParam.getWinVersion();
	if (ver >= WV_VISTA || ver == WV_UNKNOWN)
	{
		SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
		PSID AdministratorsGroup;
		is_admin = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
		if (is_admin)
		{
			if (!CheckTokenMembership(NULL, AdministratorsGroup, &is_admin))
				is_admin = FALSE;
			FreeSid(AdministratorsGroup);
		}
	}
	else
		is_admin = false;

	nppParam.setAdminMode(is_admin == TRUE);
	_isAdministrator = is_admin ? true : false;
}

Notepad_plus::~Notepad_plus()
{
	// ATTENTION : the order of the destruction is very important
	// because if the parent's window handle is destroyed before
	// the destruction of its children windows' handles,
	// its children windows' handles will be destroyed automatically!

	(NppParameters::getInstance()).destroyInstance();

	delete _pTrayIco;
	delete _pAnsiCharPanel;
	delete _pClipboardHistoryPanel;
	delete _pDocumentListPanel;
	delete _pProjectPanel_1;
	delete _pProjectPanel_2;
	delete _pProjectPanel_3;
	delete _pDocMap;
	delete _pFuncList;
	delete _pFileBrowser;
}

LRESULT Notepad_plus::init(HWND hwnd)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = nppParam.getNppGUI();

	// Menu
	_mainMenuHandle = ::GetMenu(hwnd);
	int langPos2BeRemoved = MENUINDEX_LANGUAGE + 1;
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
	MainFileManager.init(this, &_fileEditView); //get it up and running asap.

	nppParam.setFontList(hwnd);


	_mainWindowStatus = WindowMainActive;
	_activeView = MAIN_VIEW;

	const ScintillaViewParams & svp = nppParam.getSVP();

	int tabBarStatus = nppGUI._tabStatus;

	_toReduceTabBar = ((tabBarStatus & TAB_REDUCE) != 0);
	int iconDpiDynamicalSize = nppParam._dpiManager.scaleY(_toReduceTabBar ? 13 : 20);
	_docTabIconList.create(iconDpiDynamicalSize, _pPublicInterface->getHinst(), docTabIconIDs, sizeof(docTabIconIDs) / sizeof(int));
	_docTabIconListAlt.create(iconDpiDynamicalSize, _pPublicInterface->getHinst(), docTabIconIDs_alt, sizeof(docTabIconIDs_alt) / sizeof(int));
	_docTabIconListDarkMode.create(iconDpiDynamicalSize, _pPublicInterface->getHinst(), docTabIconIDs_darkMode, sizeof(docTabIconIDs_darkMode) / sizeof(int));
	
	vector<IconList *> pIconListVector;
	pIconListVector.push_back(&_docTabIconList);        // 0
	pIconListVector.push_back(&_docTabIconListAlt);     // 1
	pIconListVector.push_back(&_docTabIconListDarkMode);// 2

	unsigned char indexDocTabIcon = (((tabBarStatus & TAB_ALTICONS) == TAB_ALTICONS) ? 1 : NppDarkMode::isEnabled() ? 2 : 0);
	
	_mainDocTab.init(_pPublicInterface->getHinst(), hwnd, &_mainEditView, pIconListVector, indexDocTabIcon);
	_subDocTab.init(_pPublicInterface->getHinst(), hwnd, &_subEditView, pIconListVector, indexDocTabIcon);

	_mainEditView.display();

	_invisibleEditView.init(_pPublicInterface->getHinst(), hwnd);
	_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
	_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);
	_invisibleEditView.wrap(false); // Make sure no slow down

	// Configuration of 2 scintilla views
	_mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp._lineNumberMarginShow);
	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp._lineNumberMarginShow);
	_mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp._bookMarkMarginShow);
	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp._bookMarkMarginShow);

	_mainEditView.showIndentGuideLine(svp._indentGuideLineShow);
	_subEditView.showIndentGuideLine(svp._indentGuideLineShow);

	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);

	_configStyleDlg.init(_pPublicInterface->getHinst(), hwnd);
	_preference.init(_pPublicInterface->getHinst(), hwnd);
	_pluginsAdminDlg.init(_pPublicInterface->getHinst(), hwnd);

	//Marker Margin config
	_mainEditView.setMakerStyle(svp._folderStyle);
	_subEditView.setMakerStyle(svp._folderStyle);
	_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
	_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());

	//Line wrap method
	_mainEditView.setWrapMode(svp._lineWrapMethod);
	_subEditView.setWrapMode(svp._lineWrapMethod);

	_mainEditView.execute(SCI_SETENDATLASTLINE, !svp._scrollBeyondLastLine);
	_subEditView.execute(SCI_SETENDATLASTLINE, !svp._scrollBeyondLastLine);

	if (svp._doSmoothFont)
	{
		_mainEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
		_subEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
	}

	_mainEditView.setBorderEdge(svp._showBorderEdge);
	_subEditView.setBorderEdge(svp._showBorderEdge);

	_mainEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
	_subEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);

	_mainEditView.wrap(svp._doWrap);
	_subEditView.wrap(svp._doWrap);

	::SendMessage(hwnd, NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);

	_mainEditView.showEOL(svp._eolShow);
	_subEditView.showEOL(svp._eolShow);

	_mainEditView.showWSAndTab(svp._whiteSpaceShow);
	_subEditView.showWSAndTab(svp._whiteSpaceShow);

	_mainEditView.showWrapSymbol(svp._wrapSymbolShow);
	_subEditView.showWrapSymbol(svp._wrapSymbolShow);

	_mainEditView.performGlobalStyles();
	_subEditView.performGlobalStyles();

	_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
	_mainEditView.execute(SCI_SETZOOM, svp._zoom);
	_subEditView.execute(SCI_SETZOOM, svp._zoom2);

	::SendMessage(hwnd, NPPM_INTERNAL_SETMULTISELCTION, 0, 0);

	// Make backspace or delete work with multiple selections
	_mainEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);
	_subEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);

	// Turn virtual space on
	int virtualSpaceOptions = SCVS_RECTANGULARSELECTION;
	if(svp._virtualSpace)
		virtualSpaceOptions |= SCVS_USERACCESSIBLE | SCVS_NOWRAPLINESTART;

	_mainEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);
	_subEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);

	// Turn multi-paste on
	_mainEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
	_subEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);

	// allow user to start selecting as a stream block, then switch to a column block by adding Alt keypress
	_mainEditView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);
	_subEditView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);

	// Let Scintilla deal with some of the folding functionality
	_mainEditView.execute(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);
	_subEditView.execute(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);

	// Set padding info
	_mainEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
	_mainEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
	_subEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
	_subEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);

	// Improvement of the switching into the wrapped long line document
	_mainEditView.execute(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);
	_subEditView.execute(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);

	TabBarPlus::doDragNDrop(true);

	if (_toReduceTabBar)
	{
		HFONT hf = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));

		if (hf)
		{
			::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
			::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
		}
		int tabDpiDynamicalHeight = nppParam._dpiManager.scaleY(22);
		int tabDpiDynamicalWidth = nppParam._dpiManager.scaleX(45);
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

	// Autocomplete list and calltip
	const Style* pStyle = NppParameters::getInstance().getGlobalStylers().findByID(STYLE_DEFAULT);
	if (pStyle)
	{
		NppParameters::getInstance().setCurrentDefaultFgColor(pStyle->_fgColor);
		NppParameters::getInstance().setCurrentDefaultBgColor(pStyle->_bgColor);
		drawAutocompleteColoursFromTheme(pStyle->_fgColor, pStyle->_bgColor);
	}
	AutoCompletion::drawAutocomplete(_pEditView);
	AutoCompletion::drawAutocomplete(_pNonEditView);

	// Document Map
	drawDocumentMapColoursFromStylerArray();

	//--Splitter Section--//
	bool isVertical = (nppGUI._splitterPos == POS_VERTICAL);

	int splitterSizeDyn = nppParam._dpiManager.scaleX(splitterSize);
	_subSplitter.init(_pPublicInterface->getHinst(), hwnd);
	_subSplitter.create(&_mainDocTab, &_subDocTab, splitterSizeDyn, SplitterMode::DYNAMIC, 50, isVertical);

	//--Status Bar Section--//
	bool willBeShown = nppGUI._statusBarShow;
	_statusBar.init(_pPublicInterface->getHinst(), hwnd, 6);
	_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, nppParam._dpiManager.scaleX(220));
	_statusBar.setPartWidth(STATUSBAR_CUR_POS, nppParam._dpiManager.scaleX(260));
	_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, nppParam._dpiManager.scaleX(110));
	_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, nppParam._dpiManager.scaleX(120));
	_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, nppParam._dpiManager.scaleX(30));
	_statusBar.display(willBeShown);

	_pMainWindow = &_mainDocTab;

	_dockingManager.init(_pPublicInterface->getHinst(), hwnd, &_pMainWindow);

	if (nppGUI._isMinimizedToTray && _pTrayIco == NULL)
	{
		HICON icon = ::LoadIcon(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDI_M30ICON));
		_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, NPPM_INTERNAL_MINIMIZED_TRAY, icon, TEXT(""));
	}

	checkSyncState();

	// Plugin Manager
	NppData nppData;
	nppData._nppHandle = hwnd;
	nppData._scintillaMainHandle = _mainEditView.getHSelf();
	nppData._scintillaSecondHandle = _subEditView.getHSelf();

	_scintillaCtrls4Plugins.init(_pPublicInterface->getHinst(), hwnd);
	_pluginsManager.init(nppData);

	bool enablePluginAdmin = _pluginsAdminDlg.initFromJson();
	_pluginsManager.loadPlugins(nppParam.getPluginRootDir(), enablePluginAdmin ? &_pluginsAdminDlg.getAvailablePluginUpdateInfoList() : nullptr);
	_restoreButton.init(_pPublicInterface->getHinst(), hwnd);

	// ------------ //
	// Menu Section //
	// ------------ //

	setupColorSampleBitmapsOnMainMenuItems();

	// Macro Menu
	std::vector<MacroShortcut> & macros = nppParam.getMacroList();
	HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
	size_t const posBase = 6;
	size_t nbMacro = macros.size();
	if (nbMacro >= 1)
		::InsertMenu(hMacroMenu, posBase - 1, MF_BYPOSITION, static_cast<UINT>(-1), 0);

	for (size_t i = 0; i < nbMacro; ++i)
		::InsertMenu(hMacroMenu, static_cast<UINT>(posBase + i), MF_BYPOSITION, ID_MACRO + i, macros[i].toMenuItemString().c_str());

	if (nbMacro >= 1)
	{
		::InsertMenu(hMacroMenu, static_cast<UINT>(posBase + nbMacro + 1), MF_BYPOSITION, static_cast<UINT>(-1), 0);
		::InsertMenu(hMacroMenu, static_cast<UINT>(posBase + nbMacro + 2), MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_MACRO, TEXT("Modify Shortcut/Delete Macro..."));
	}

	// Run Menu
	std::vector<UserCommand> & userCommands = nppParam.getUserCommandList();
	HMENU hRunMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_RUN);
	int const runPosBase = 2;
	size_t nbUserCommand = userCommands.size();
	if (nbUserCommand >= 1)
		::InsertMenu(hRunMenu, runPosBase - 1, MF_BYPOSITION, static_cast<UINT>(-1), 0);

	for (size_t i = 0; i < nbUserCommand; ++i)
	{
		::InsertMenu(hRunMenu, static_cast<UINT>(runPosBase + i), MF_BYPOSITION, ID_USER_CMD + i, userCommands[i].toMenuItemString().c_str());
	}

	if (nbUserCommand >= 1)
	{
		::InsertMenu(hRunMenu, static_cast<UINT>(runPosBase + nbUserCommand + 1), MF_BYPOSITION, static_cast<UINT>(-1), 0);
		::InsertMenu(hRunMenu, static_cast<UINT>(runPosBase + nbUserCommand + 2), MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_RUN, TEXT("Modify Shortcut/Delete Command..."));
	}

	// Updater menu item
	if (!nppGUI._doesExistUpdater)
	{
		::DeleteMenu(_mainMenuHandle, IDM_UPDATE_NPP, MF_BYCOMMAND);
		::DeleteMenu(_mainMenuHandle, IDM_CONFUPDATERPROXY, MF_BYCOMMAND);
		HMENU hHelpMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_HELP);
		if (hHelpMenu)
			::DeleteMenu(hHelpMenu, 7, MF_BYPOSITION); // SEPARATOR
		::DrawMenuBar(hwnd);
	}
	//Languages Menu
	HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	// Add external languages to menu
	for (int i = 0; i < nppParam.getNbExternalLang(); ++i)
	{
		ExternalLangContainer & externalLangContainer = nppParam.getELCFromIndex(i);

		int numLangs = ::GetMenuItemCount(hLangMenu);
		const int bufferSize = 100;
		TCHAR buffer[bufferSize];
		const TCHAR* lexerNameW = wmc.char2wchar(externalLangContainer._name.c_str(), CP_ACP);

		int x = 0;
		for (; (x == 0 || lstrcmp(lexerNameW, buffer) > 0) && x < numLangs; ++x)
		{
			::GetMenuString(hLangMenu, x, buffer, bufferSize, MF_BYPOSITION);
		}

		::InsertMenu(hLangMenu, x - 1, MF_BYPOSITION, IDM_LANG_EXTERNAL + i, lexerNameW);
	}

	if (nppGUI._excludedLangList.size() > 0)
	{
		for (size_t i = 0, len = nppGUI._excludedLangList.size(); i < len; ++i)
		{
			int cmdID = nppParam.langTypeToCommandID(nppGUI._excludedLangList[i]._langType);
			const int itemSize = 256;
			TCHAR itemName[itemSize];
			::GetMenuString(hLangMenu, cmdID, itemName, itemSize, MF_BYCOMMAND);
			nppGUI._excludedLangList[i]._cmdID = cmdID;
			nppGUI._excludedLangList[i]._langName = itemName;
			::DeleteMenu(hLangMenu, cmdID, MF_BYCOMMAND);
			DrawMenuBar(hwnd);
		}
	}

	// Add User Defined Languages Entry
	int udlpos = ::GetMenuItemCount(hLangMenu) - 1;

	for (int i = 0, len = nppParam.getNbUserLang(); i < len; ++i)
	{
		UserLangContainer & userLangContainer = nppParam.getULCFromIndex(i);
		::InsertMenu(hLangMenu, udlpos + i, MF_BYPOSITION, IDM_LANG_USER + i + 1, userLangContainer.getName());
	}

	//Add recent files
	HMENU hFileMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FILE);
	int nbLRFile = nppParam.getNbLRFile();
	//int pos = IDM_FILEMENU_LASTONE - IDM_FILE + 1 /* +1 : because of  IDM_FILE_PRINTNOW */;

	_lastRecentFileList.initMenu(hFileMenu, IDM_FILEMENU_LASTONE + 1, IDM_FILEMENU_EXISTCMDPOSITION, &_accelerator, nppParam.putRecentFileInSubMenu());
	_lastRecentFileList.setLangEncoding(_nativeLangSpeaker.getLangEncoding());
	for (int i = 0; i < nbLRFile; ++i)
	{
		generic_string * stdStr = nppParam.getLRFile(i);
		if (!nppGUI._checkHistoryFiles || PathFileExists(stdStr->c_str()))
		{
			_lastRecentFileList.add(stdStr->c_str());
		}
	}

	//Plugin menu
	_pluginsAdminDlg.setPluginsManager(&_pluginsManager);
	_pluginsManager.initMenu(_mainMenuHandle, enablePluginAdmin);

	//Search menu
	//disable "Search Results Window" under Search Menu 
	::EnableMenuItem(_mainMenuHandle, IDM_FOCUS_ON_FOUND_RESULTS, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);

	//Main menu is loaded, now load context menu items
	nppParam.getContextMenuFromXmlTree(_mainMenuHandle, _pluginsManager.getMenuHandle());

	if (nppParam.hasCustomContextMenu())
	{
		_mainEditView.execute(SCI_USEPOPUP, FALSE);
		_subEditView.execute(SCI_USEPOPUP, FALSE);
	}

	_nativeLangSpeaker.changeMenuLang(_mainMenuHandle);
	::DrawMenuBar(hwnd);


	//Windows menu
	_windowsMenu.init(_mainMenuHandle);

	// Update context menu strings (translated)
	vector<MenuItemUnit> & tmp = nppParam.getContextMenuItems();
	size_t len = tmp.size();
	TCHAR menuName[64];
	for (size_t i = 0; i < len; ++i)
	{
		if (tmp[i]._itemName.empty())
		{
			::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
			tmp[i]._itemName = purgeMenuItemString(menuName);
		}
	}

	updateCommandShortcuts();

	//Translate non-menu shortcuts
	_nativeLangSpeaker.changeShortcutLang();

	//Update plugin shortcuts, all plugin commands should be available now
	nppParam.reloadPluginCmds();

	// Shortcut Accelerator : should be the last one since it will capture all the shortcuts
	_accelerator.init(_mainMenuHandle, hwnd);
	nppParam.setAccelerator(&_accelerator);

	// Scintilla key accelerator
	vector<HWND> scints;
	scints.push_back(_mainEditView.getHSelf());
	scints.push_back(_subEditView.getHSelf());
	_scintaccelerator.init(&scints, _mainMenuHandle, hwnd);

	nppParam.setScintillaAccelerator(&_scintaccelerator);
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

	_toolBar.init(_pPublicInterface->getHinst(), hwnd, tbStatus, toolBarIcons, sizeof(toolBarIcons) / sizeof(ToolBarButtonUnit));

	_rebarTop.init(_pPublicInterface->getHinst(), hwnd);
	_rebarBottom.init(_pPublicInterface->getHinst(), hwnd);
	_toolBar.addToRebar(&_rebarTop);
	_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, willBeShown);

	checkMacroState();

	//--Init dialogs--//
	_findReplaceDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_findInFinderDlg.init(_pPublicInterface->getHinst(), hwnd);
	_incrementFindDlg.init(_pPublicInterface->getHinst(), hwnd, &_findReplaceDlg, _nativeLangSpeaker.isRTL());
	_incrementFindDlg.addToRebar(&_rebarBottom);
	_goToLineDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_findCharsInRangeDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_colEditorDlg.init(_pPublicInterface->getHinst(), hwnd, &_pEditView);
	_aboutDlg.init(_pPublicInterface->getHinst(), hwnd);
	_debugInfoDlg.init(_pPublicInterface->getHinst(), hwnd, _isAdministrator, _pluginsManager.getLoadedPluginNames());
	_runDlg.init(_pPublicInterface->getHinst(), hwnd);
	_runMacroDlg.init(_pPublicInterface->getHinst(), hwnd);
	_documentPeeker.init(_pPublicInterface->getHinst(), hwnd);

	_md5FromFilesDlg.init(_pPublicInterface->getHinst(), hwnd);
	_md5FromFilesDlg.setHashType(hash_md5);
	_md5FromTextDlg.init(_pPublicInterface->getHinst(), hwnd);
	_md5FromTextDlg.setHashType(hash_md5);
	_sha2FromFilesDlg.init(_pPublicInterface->getHinst(), hwnd);
	_sha2FromFilesDlg.setHashType(hash_sha256);
	_sha2FromTextDlg.init(_pPublicInterface->getHinst(), hwnd);
	_sha2FromTextDlg.setHashType(hash_sha256);


	//--User Define Dialog Section--//
	int uddStatus = nppGUI._userDefineDlgStatus;
	UserDefineDialog *udd = _pEditView->getUserDefineDlg();

	bool uddShow = false;
	switch (uddStatus)
	{
		case UDD_SHOW: // show & undocked
		{
			udd->doDialog(true, _nativeLangSpeaker.isRTL());
			_nativeLangSpeaker.changeUserDefineLang(udd);
			uddShow = true;
			break;
		}
		case UDD_DOCKED: // hide & docked
		{
			_isUDDocked = true;
			break;
		}
		case (UDD_SHOW | UDD_DOCKED):    // show & docked
		{
			udd->doDialog(true, _nativeLangSpeaker.isRTL());
			_nativeLangSpeaker.changeUserDefineLang(udd);
			::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);
			uddShow = true;
			break;
		}

		default: // hide & undocked
			break;
	}

	//
	// Menu & toolbar for UserDefine Dialog
	//
	checkMenuItem(IDM_LANG_USER_DLG, uddShow);
	_toolBar.setCheck(IDM_LANG_USER_DLG, uddShow);

	//Hide or show the right shortcuts "＋" "▼" "✕" of main menu bar
	if (nppGUI._hideMenuRightShortcuts)
	{
		int nbRemoved = 0;
		const int bufferSize = 64;
		TCHAR buffer[bufferSize];
		int nbItem = GetMenuItemCount(_mainMenuHandle);
		for (int i = nbItem - 1; i >= 0; --i)
		{
			::GetMenuStringW(_mainMenuHandle, i, buffer, bufferSize, MF_BYPOSITION);
			if (lstrcmp(buffer, L"✕") == 0 || lstrcmp(buffer, L"▼") == 0 || lstrcmp(buffer, L"＋") == 0)
			{
				::RemoveMenu(_mainMenuHandle, i, MF_BYPOSITION);
				++nbRemoved;
			}
			if (nbRemoved == 3)
				break;
		}
		if (nbRemoved > 0)
			::DrawMenuBar(hwnd);
	}

	//
	// Initialize the default foreground & background color
	//
	{
		const Style * pStyle = nppParam.getGlobalStylers().findByID(STYLE_DEFAULT);
		if (pStyle)
		{
			nppParam.setCurrentDefaultFgColor(pStyle->_fgColor);
			nppParam.setCurrentDefaultBgColor(pStyle->_bgColor);
		}
	}

	//
	// launch the plugin dlg memorized at the last session
	//

	DockingManagerData& dmd = nppGUI._dockingData;

	_dockingManager.setDockedContSize(CONT_LEFT, nppGUI._dockingData._leftWidth);
	_dockingManager.setDockedContSize(CONT_RIGHT, nppGUI._dockingData._rightWidth);
	_dockingManager.setDockedContSize(CONT_TOP, nppGUI._dockingData._topHeight);
	_dockingManager.setDockedContSize(CONT_BOTTOM, nppGUI._dockingData._bottomHight);

	if (!nppGUI._isCmdlineNosessionActivated)
	{
		for (size_t i = 0, len = dmd._pluginDockInfo.size(); i < len; ++i)
		{
			PluginDlgDockingInfo& pdi = dmd._pluginDockInfo[i];
			if (pdi._isVisible)
			{
				if (pdi._name == NPP_INTERNAL_FUCTION_STR)
					_internalFuncIDs.push_back(pdi._internalID);
				else
					_pluginsManager.runPluginCommand(pdi._name.c_str(), pdi._internalID);
			}
		}

		for (size_t i = 0, len = dmd._containerTabInfo.size(); i < len; ++i)
		{
			ContainerTabInfo & cti = dmd._containerTabInfo[i];
			_dockingManager.setActiveTab(cti._cont, cti._activeTab);
		}
	}

	//Load initial docs into doctab
	loadBufferIntoView(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
	loadBufferIntoView(_subEditView.getCurrentBufferID(), SUB_VIEW);
	activateBuffer(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
	activateBuffer(_subEditView.getCurrentBufferID(), SUB_VIEW);

	_mainEditView.getFocus();

	if (_nativeLangSpeaker.isRTL())
	{
		_mainEditView.changeTextDirection(true);
		_subEditView.changeTextDirection(true);
	}

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
	NppParameters& nppParams = NppParameters::getInstance();
	NppGUI & nppGUI = nppParams.getNppGUI();
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
						(nppGUI._tabStatus & TAB_HIDE) | \
						(nppGUI._tabStatus & TAB_QUITONEMPTY) | \
						(nppGUI._tabStatus & TAB_ALTICONS);
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

	if (_findReplaceDlg.getHSelf() != NULL)
	{
		::GetWindowPlacement(_findReplaceDlg.getHSelf(), &posInfo);

		nppGUI._findWindowPos.left = posInfo.rcNormalPosition.left;
		nppGUI._findWindowPos.top = posInfo.rcNormalPosition.top;
		nppGUI._findWindowPos.right = posInfo.rcNormalPosition.right;
		nppGUI._findWindowPos.bottom = posInfo.rcNormalPosition.bottom;
	}

	saveDockingParams();
	nppParams.createXmlTreeFromGUIParams();
	return true;
}

bool Notepad_plus::saveProjectPanelsParams()
{
	NppParameters& nppParams = NppParameters::getInstance();

	if (_pProjectPanel_1)
	{
		if (!_pProjectPanel_1->checkIfNeedSave()) return false;
		nppParams.setWorkSpaceFilePath(0, _pProjectPanel_1->getWorkSpaceFilePath());
	}
	if (_pProjectPanel_2)
	{
		if (!_pProjectPanel_2->checkIfNeedSave()) return false;
		nppParams.setWorkSpaceFilePath(1, _pProjectPanel_2->getWorkSpaceFilePath());
	}
	if (_pProjectPanel_3)
	{
		if (!_pProjectPanel_3->checkIfNeedSave()) return false;
		nppParams.setWorkSpaceFilePath(2, _pProjectPanel_3->getWorkSpaceFilePath());
	}
	return nppParams.writeProjectPanelsSettings();
}

bool Notepad_plus::saveFileBrowserParam()
{
	if (_pFileBrowser)
	{
		vector<generic_string> rootPaths = _pFileBrowser->getRoots();
		generic_string selectedItemPath = _pFileBrowser->getSelectedItemPath();
		return (NppParameters::getInstance()).writeFileBrowserSettings(rootPaths, selectedItemPath);
	}
	return true; // nothing to save so true is returned
}

void Notepad_plus::saveDockingParams()
{
	NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();

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
				PluginDlgDockingInfo pddi(vDataVis[j]->pszModuleName, vDataVis[j]->dlgID, int32_t(i), vDataVis[j]->iPrevCont, true);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the hidden Tb's
		vector<tTbData *>	vDataAll	= vCont[i]->getDataOfAllTb();

		for (size_t j = 0, len3 = vDataAll.size(); j < len3 ; ++j)
		{
			if ((vDataAll[j]->pszName && vDataAll[j]->pszName[0]) && (!vCont[i]->isTbVis(vDataAll[j])))
			{
				PluginDlgDockingInfo pddi(vDataAll[j]->pszModuleName, vDataAll[j]->dlgID, int32_t(i), vDataAll[j]->iPrevCont, false);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the position, when container is a floated one
		if (i >= DOCKCONT_MAX)
		{
			RECT	rc;
			vCont[i]->getWindowRect(rc);
			FloatingWindowInfo fwi(int32_t(i), rc.left, rc.top, rc.right, rc.bottom);
			vFloatingWindowInfo.push_back(fwi);
		}

		// save the active tab
		ContainerTabInfo act(int32_t(i), vCont[i]->getActiveTb());
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

			if (floatCont >= 0)
			{
				if (floatContArray[floatCont] == 0)
				{
					RECT rc;
					if (nppGUI._dockingData.getFloatingRCFrom(floatCont, rc))
					{
						vFloatingWindowInfo.push_back(FloatingWindowInfo(floatCont, rc.left, rc.top, rc.right, rc.bottom));
					}
					floatContArray[floatCont] = 1;
				}
			}
			if (i < nppGUI._dockingData._pluginDockInfo.size()) // to prevent from crash in debug mode
				vPluginDockInfo.push_back(nppGUI._dockingData._pluginDockInfo[i]);
		}
	}

	nppGUI._dockingData._pluginDockInfo = vPluginDockInfo;
	nppGUI._dockingData._flaotingWindowInfo = vFloatingWindowInfo;
}


void Notepad_plus::saveUserDefineLangs()
{
	(NppParameters::getInstance()).writeNeed2SaveUDL();
}


void Notepad_plus::saveShortcuts()
{
	NppParameters::getInstance().writeShortcuts();
}


void Notepad_plus::saveFindHistory()
{
	_findReplaceDlg.saveFindHistory();
	(NppParameters::getInstance()).writeFindHistory();
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
	NppParameters& nppParamInst = NppParameters::getInstance();
	LangType langT = nppParamInst.getLangFromExt(ext);

	if ((langT != L_XML) && (langT != L_HTML))
		return -1;

	// Get the beginning of file data
	FILE *f = generic_fopen(fileName, TEXT("rb"));
	if (!f)
		return -1;
	const int blockSize = 1024; // To ensure that length is long enough to capture the encoding in html
	char data[blockSize];
	size_t lenFile = fread(data, 1, blockSize, f);
	fclose(f);

	// Put data in _invisibleEditView
	_invisibleEditView.execute(SCI_CLEARALL);
	_invisibleEditView.execute(SCI_APPENDTEXT, lenFile, reinterpret_cast<LPARAM>(data));

	const char *encodingAliasRegExpr = "[a-zA-Z0-9_-]+";
	const size_t encodingStrLen = 128;
	if (langT == L_XML)
	{
		// find encoding by RegExpr

		const char *xmlHeaderRegExpr = "<?xml[ \\t]+version[ \\t]*=[ \\t]*\"[^\"]+\"[ \\t]+encoding[ \\t]*=[ \\t]*\"[^\"]+\"[ \\t]*.*?>";

        size_t startPos = 0;
		size_t endPos = lenFile-1;
		_invisibleEditView.execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

		_invisibleEditView.execute(SCI_SETTARGETRANGE, startPos, endPos);

		auto posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(xmlHeaderRegExpr), reinterpret_cast<LPARAM>(xmlHeaderRegExpr));
		if (posFound >= 0)
		{
            const char *encodingBlockRegExpr = "encoding[ \\t]*=[ \\t]*\"[^\".]+\"";
			_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingBlockRegExpr), reinterpret_cast<LPARAM>(encodingBlockRegExpr));

            const char *encodingRegExpr = "\".+\"";
			_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingRegExpr), reinterpret_cast<LPARAM>(encodingRegExpr));

			_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingAliasRegExpr), reinterpret_cast<LPARAM>(encodingAliasRegExpr));

            startPos = _invisibleEditView.execute(SCI_GETTARGETSTART);
			endPos = _invisibleEditView.execute(SCI_GETTARGETEND);
			
			size_t len = endPos - startPos;
			if (len >= encodingStrLen)
			{
				return -1;
			}

            char encodingStr[encodingStrLen];
            _invisibleEditView.getText(encodingStr, startPos, endPos);

			EncodingMapper& em = EncodingMapper::getInstance();
            int enc = em.getEncodingFromString(encodingStr);
            return (enc == CP_ACP ? -1 : enc);
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

		intptr_t startPos = 0;
		auto endPos = lenFile - 1;
		_invisibleEditView.execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

		_invisibleEditView.execute(SCI_SETTARGETRANGE, startPos, endPos);

		auto posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(htmlHeaderRegExpr), reinterpret_cast<LPARAM>(htmlHeaderRegExpr));

		if (posFound < 0)
		{
			posFound = _invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(htmlHeaderRegExpr2), reinterpret_cast<LPARAM>(htmlHeaderRegExpr2));
			if (posFound < 0)
				return -1;
		}
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(charsetBlock), reinterpret_cast<LPARAM>(charsetBlock));
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(intermediaire), reinterpret_cast<LPARAM>(intermediaire));
		_invisibleEditView.execute(SCI_SEARCHINTARGET, strlen(encodingStrRE), reinterpret_cast<LPARAM>(encodingStrRE));

        startPos = _invisibleEditView.execute(SCI_GETTARGETSTART);
		endPos = _invisibleEditView.execute(SCI_GETTARGETEND);

		size_t len = endPos - startPos;
		if (len >= encodingStrLen)
		{
			return -1;
		}

        char encodingStr[encodingStrLen];
        _invisibleEditView.getText(encodingStr, startPos, endPos);

		EncodingMapper& em = EncodingMapper::getInstance();
		int enc = em.getEncodingFromString(encodingStr);
        return (enc == CP_ACP ? -1 : enc);
	}
}

void Notepad_plus::setCodePageForInvisibleView(Buffer const *pBuffer)
{
	intptr_t detectedCp = _invisibleEditView.execute(SCI_GETCODEPAGE);
	intptr_t cp2set = SC_CP_UTF8;
	if (pBuffer->getUnicodeMode() == uni8Bit)
	{
		cp2set = (detectedCp == SC_CP_UTF8 ? CP_ACP : detectedCp);
	}
	_invisibleEditView.execute(SCI_SETCODEPAGE, cp2set);
}

bool Notepad_plus::replaceInOpenedFiles()
{

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	Buffer * oldBuf = _invisibleEditView.getCurrentBuffer();	//for manually setting the buffer, so notifications can be handled properly

	Buffer * pBuf = NULL;

	int nbTotal = 0;
	const bool isEntireDoc = true;

    if (_mainWindowStatus & WindowMainActive)
    {
		for (size_t i = 0, len = _mainDocTab.nbItem(); i < len ; ++i)
	    {
			pBuf = MainFileManager.getBufferByID(_mainDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			_invisibleEditView.setCurrentBuffer(pBuf);

			_invisibleEditView.execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(ProcessReplaceAll, FindReplaceDlg::_env, isEntireDoc);
			_invisibleEditView.execute(SCI_ENDUNDOACTION);
		}
	}

	if (_mainWindowStatus & WindowSubActive)
	{
		for (size_t i = 0, len = _subDocTab.nbItem(); i < len; ++i)
		{
			pBuf = MainFileManager.getBufferByID(_subDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

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
	{
		generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-replaceinfiles-re-malformed", TEXT("Replace in Opened Files: The regular expression is malformed."));
		_findReplaceDlg.setStatusbarMessage(msg, FSNotFound);
	}
	else
	{
		if (nbTotal)
			enableCommand(IDM_FILE_SAVEALL, true, MENU | TOOLBAR);

		generic_string result;
		if (nbTotal == 1)
		{
			result = _nativeLangSpeaker.getLocalizedStrFromID("find-status-replaceinopenedfiles-1-replaced", TEXT("Replace in Opened Files: 1 occurrence was replaced."));
		}
		else
		{
			result = _nativeLangSpeaker.getLocalizedStrFromID("find-status-replaceinopenedfiles-nb-replaced", TEXT("Replace in Opened Files: $INT_REPLACE$ occurrences were replaced."));
			result = stringReplace(result, TEXT("$INT_REPLACE$"), std::to_wstring(nbTotal));
		}
		_findReplaceDlg.setStatusbarMessage(result, FSMessage);
	}
	return true;
}


void Notepad_plus::wsTabConvert(spaceTab whichWay)
{
	intptr_t tabWidth = _pEditView->execute(SCI_GETTABWIDTH);
	intptr_t currentPos = _pEditView->execute(SCI_GETCURRENTPOS);
    intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();
	intptr_t docLength = _pEditView->execute(SCI_GETLENGTH) + 1;
    if (docLength < 2)
        return;

    intptr_t count = 0;
    intptr_t column = 0;
    intptr_t newCurrentPos = 0;
	intptr_t tabStop = tabWidth - 1;   // remember, counting from zero !
    bool onlyLeading = false;
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
	_pEditView->execute(SCI_GETTEXT, docLength, reinterpret_cast<LPARAM>(source));

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
                    intptr_t insertTabs = tabWidth - (column % tabWidth);
                    for (int j = 0; j < insertTabs; ++j)
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
			int counter = 0;
			bool nonSpaceFound = false;
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
	_pEditView->execute(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(destination));
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
	auto mainSelStart = _pEditView->execute(SCI_GETSELECTIONSTART);
	auto mainSelEnd = _pEditView->execute(SCI_GETSELECTIONEND);
	auto mainSelLength = mainSelEnd - mainSelStart;
	bool isEntireDoc = mainSelLength == 0;
	env._isInSelection = !isEntireDoc;
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, isEntireDoc);

	// remove the last line if it's an empty line.
	if (isBlankContained)
	{
		env._str2Search = TEXT("(\\r\\n|\\r|\\n)^[\\t ]*$");
	}
	else
	{
		env._str2Search = TEXT("(\\r\\n|\\r|\\n)^$");
	}
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, isEntireDoc);
}

void Notepad_plus::removeDuplicateLines()
{
	// whichPart : line head or line tail
	FindOption env;

	env._str2Search = TEXT("^(.*(\\r?\\n|\\r))(\\1)+");
	env._str4Replace = TEXT("\\1");
	env._searchType = FindRegex;
	auto mainSelStart = _pEditView->execute(SCI_GETSELECTIONSTART);
	auto mainSelEnd = _pEditView->execute(SCI_GETSELECTIONEND);
	auto mainSelLength = mainSelEnd - mainSelStart;
	bool isEntireDoc = mainSelLength == 0;
	env._isInSelection = !isEntireDoc;
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, isEntireDoc);

	// remove the last line if it's a duplicate line.
	env._str2Search = TEXT("^(.+)(\\r?\\n|\\r)(\\1)$");
	_findReplaceDlg.processAll(ProcessReplaceAll, &env, isEntireDoc);
}

void Notepad_plus::getMatchedFileNames(const TCHAR *dir, size_t level, const vector<generic_string> & patterns, vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir)
{
	level++;
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
				if ((OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT(".")) != 0) && (OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT("..")) != 0) &&
					!matchInExcludeDirList(foundData.cFileName, patterns, level))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), level, patterns, fileNames, isRecursive, isInHiddenDir);
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
				if ((OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT(".")) != 0) && (OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT("..")) != 0) &&
					!matchInExcludeDirList(foundData.cFileName, patterns, level))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), level, patterns, fileNames, isRecursive, isInHiddenDir);
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

bool Notepad_plus::createFilelistForFiles(vector<generic_string> & fileNames)
{
	const TCHAR *dir2Search = _findReplaceDlg.getDir2Search();
	if (!dir2Search[0] || !::PathFileExists(dir2Search))
	{
		return false;
	}

	vector<generic_string> patterns2Match;
	_findReplaceDlg.getAndValidatePatterns(patterns2Match);

	bool isRecursive = _findReplaceDlg.isRecursive();
	bool isInHiddenDir = _findReplaceDlg.isInHiddenDir();
	getMatchedFileNames(dir2Search, 0, patterns2Match, fileNames, isRecursive, isInHiddenDir);
	return true;
}

bool Notepad_plus::createFilelistForProjects(vector<generic_string> & fileNames)
{
	vector<generic_string> patterns2Match;
	_findReplaceDlg.getAndValidatePatterns(patterns2Match);
	bool somethingIsSelected = false; // at least one Project Panel is open and checked

	if (_findReplaceDlg.isProjectPanel_1() && _pProjectPanel_1 && !_pProjectPanel_1->isClosed())
	{
		_pProjectPanel_1->enumWorkSpaceFiles (NULL, patterns2Match, fileNames);
		somethingIsSelected = true;
	}
	if (_findReplaceDlg.isProjectPanel_2() && _pProjectPanel_2 && !_pProjectPanel_2->isClosed())
	{
		_pProjectPanel_2->enumWorkSpaceFiles (NULL, patterns2Match, fileNames);
		somethingIsSelected = true;
	}
	if (_findReplaceDlg.isProjectPanel_3() && _pProjectPanel_3 && !_pProjectPanel_3->isClosed())
	{
		_pProjectPanel_3->enumWorkSpaceFiles (NULL, patterns2Match, fileNames);
		somethingIsSelected = true;
	}
	return somethingIsSelected;
}

std::mutex replaceInFiles_mutex;

bool Notepad_plus::replaceInFiles()
{
	std::lock_guard<std::mutex> lock(replaceInFiles_mutex);

	std::vector<generic_string> fileNames;
	if (!createFilelistForFiles(fileNames))
		return false;

	return replaceInFilelist(fileNames);
}

bool Notepad_plus::replaceInProjects()
{
	std::lock_guard<std::mutex> lock(replaceInFiles_mutex);

	std::vector<generic_string> fileNames;
	if (!createFilelistForProjects(fileNames))
		return false;

	return replaceInFilelist(fileNames);
}

bool Notepad_plus::replaceInFilelist(std::vector<generic_string> & fileNames)
{
	int nbTotal = 0;

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	Buffer * oldBuf = _invisibleEditView.getCurrentBuffer();	//for manually setting the buffer, so notifications can be handled properly

	Progress progress(_pPublicInterface->getHinst());
	size_t filesCount = fileNames.size();
	size_t filesPerPercent = 1;

	if (filesCount > 1)
	{
		if (filesCount >= 200)
			filesPerPercent = filesCount / 100;
		
		generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID(
			"replace-in-files-progress-title", TEXT("Replace In Files progress..."));
		progress.open(_findReplaceDlg.getHSelf(), msg.c_str());
	}

	for (size_t i = 0, updateOnCount = filesPerPercent; i < filesCount; ++i)
	{
		if (progress.isCancelled()) break;

		bool closeBuf = false;

		BufferID id = MainFileManager.getBufferFromName(fileNames.at(i).c_str());
		if (id == BUFFER_INVALID)
		{
			id = MainFileManager.loadFile(fileNames.at(i).c_str());
			closeBuf = true;
		}

		if (id != BUFFER_INVALID)
		{
			Buffer * pBuf = MainFileManager.getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			_invisibleEditView.setCurrentBuffer(pBuf);

			FindersInfo findersInfo;
			findersInfo._pFileName = fileNames.at(i).c_str();
			int nbReplaced = _findReplaceDlg.processAll(ProcessReplaceAll, FindReplaceDlg::_env, true, &findersInfo);
			nbTotal += nbReplaced;
			if (nbReplaced)
			{
				MainFileManager.saveBuffer(id, pBuf->getFullPathName());
			}

			if (closeBuf)
				MainFileManager.closeBuffer(id, _pEditView);
		}
		if (i == updateOnCount)
		{
			updateOnCount += filesPerPercent;
			progress.setPercent(int32_t((i * 100) / filesCount), fileNames.at(i).c_str());
		}
		else
		{
			progress.setInfo(fileNames.at(i).c_str());
		}
	}

	progress.close();

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_invisibleEditView.setCurrentBuffer(oldBuf);
	_pEditView = pOldView;

	generic_string result;
	if (nbTotal == 1)
	{
		result = _nativeLangSpeaker.getLocalizedStrFromID("find-status-replaceinfiles-1-replaced", TEXT("Replace in Files: 1 occurrence was replaced."));
	}
	else
	{
		result = _nativeLangSpeaker.getLocalizedStrFromID("find-status-replaceinfiles-nb-replaced", TEXT("Replace in Files: $INT_REPLACE$ occurrences were replaced."));
		result = stringReplace(result, TEXT("$INT_REPLACE$"), std::to_wstring(nbTotal));
	}
	_findReplaceDlg.setStatusbarMessage(result, FSMessage);

	return true;
}

bool Notepad_plus::findInFinderFiles(FindersInfo *findInFolderInfo)
{
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	vector<generic_string> fileNames = findInFolderInfo->_pSourceFinder->getResultFilePaths();

	findInFolderInfo->_pDestFinder->beginNewFilesSearch();
	findInFolderInfo->_pDestFinder->addSearchLine(findInFolderInfo->_findOption._str2Search.c_str());

	Progress progress(_pPublicInterface->getHinst());

	size_t filesCount = fileNames.size();
	size_t filesPerPercent = 1;

	if (filesCount > 1)
	{
		if (filesCount >= 200)
			filesPerPercent = filesCount / 100;
		
		generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID(
			"find-in-files-progress-title", TEXT("Find In Files progress..."));
		progress.open(_findReplaceDlg.getHSelf(), msg.c_str());
	}

	for (size_t i = 0, updateOnCount = filesPerPercent; i < filesCount; ++i)
	{
		if (progress.isCancelled()) break;

		bool closeBuf = false;
		BufferID id = MainFileManager.getBufferFromName(fileNames.at(i).c_str());
		if (id == BUFFER_INVALID)
		{
			id = MainFileManager.loadFile(fileNames.at(i).c_str());
			closeBuf = true;
		}

		if (id != BUFFER_INVALID)
		{
			Buffer * pBuf = MainFileManager.getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			findInFolderInfo->_pFileName = fileNames.at(i).c_str();
			
			nbTotal += _findReplaceDlg.processAll(ProcessFindInFinder, &(findInFolderInfo->_findOption), true, findInFolderInfo);
			
			if (closeBuf)
				MainFileManager.closeBuffer(id, _pEditView);
		}
		if (i == updateOnCount)
		{
			updateOnCount += filesPerPercent;
			progress.setPercent(int32_t((i * 100) / filesCount), fileNames.at(i).c_str());
		}
		else
		{
			progress.setInfo(fileNames.at(i).c_str());
		}
	}
	progress.close();

	const bool searchedInSelection = false;
	findInFolderInfo->_pDestFinder->finishFilesSearch(nbTotal, int(filesCount), findInFolderInfo->_findOption._isMatchLineNumber, !searchedInSelection);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	return true;
}

bool Notepad_plus::findInFiles()
{
	std::vector<generic_string> fileNames;
	if (! createFilelistForFiles(fileNames))
		return false;

	return findInFilelist(fileNames);
}

bool Notepad_plus::findInProjects()
{
	vector<generic_string> fileNames;
	if (! createFilelistForProjects(fileNames))
		return false;

	return findInFilelist(fileNames);
}

bool Notepad_plus::findInFilelist(std::vector<generic_string> & fileNames)
{
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	_findReplaceDlg.beginNewFilesSearch();

	Progress progress(_pPublicInterface->getHinst());

	size_t filesCount = fileNames.size();
	size_t filesPerPercent = 1;

	if (filesCount > 1)
	{
		if (filesCount >= 200)
			filesPerPercent = filesCount / 100;

		generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID(
			"find-in-files-progress-title", TEXT("Find In Files progress..."));
		progress.open(_findReplaceDlg.getHSelf(), msg.c_str());
	}

	const bool isEntireDoc = true;

	for (size_t i = 0, updateOnCount = filesPerPercent; i < filesCount; ++i)
	{
		if (progress.isCancelled()) break;

		bool closeBuf = false;
		BufferID id = MainFileManager.getBufferFromName(fileNames.at(i).c_str());
		if (id == BUFFER_INVALID)
		{
			id = MainFileManager.loadFile(fileNames.at(i).c_str());
			closeBuf = true;
		}

		if (id != BUFFER_INVALID)
		{
			Buffer * pBuf = MainFileManager.getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			FindersInfo findersInfo;
			findersInfo._pFileName = fileNames.at(i).c_str();

			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, &findersInfo);

			if (closeBuf)
				MainFileManager.closeBuffer(id, _pEditView);
		}
		if (i == updateOnCount)
		{
			updateOnCount += filesPerPercent;
			progress.setPercent(int32_t((i * 100) / filesCount), fileNames.at(i).c_str());
		}
		else
		{
			progress.setInfo(fileNames.at(i).c_str());
		}
	}

	progress.close();

	_findReplaceDlg.finishFilesSearch(nbTotal, int(filesCount), isEntireDoc);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	if (nbTotal != 0)
	{
		NppParameters& nppParam = NppParameters::getInstance();
		NppGUI& nppGui = nppParam.getNppGUI();
		if (!nppGui._findDlgAlwaysVisible)
		{
			_findReplaceDlg.display(false);
		}
	}

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
		for (size_t i = 0, len = _mainDocTab.nbItem(); i < len ; ++i)
		{
			pBuf = MainFileManager.getBufferByID(_mainDocTab.getBufferByIndex(i));
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			FindersInfo findersInfo;
			findersInfo._pFileName = pBuf->getFullPathName();
			
			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, &findersInfo);
		}
	}

	size_t nbUniqueBuffers = _mainDocTab.nbItem();

	if (_mainWindowStatus & WindowSubActive)
	{
		for (size_t i = 0, len2 = _subDocTab.nbItem(); i < len2 ; ++i)
		{
			pBuf = MainFileManager.getBufferByID(_subDocTab.getBufferByIndex(i));
			if (_mainDocTab.getIndexByBuffer(pBuf) != -1)
			{
				continue;  // clone was already searched in main; skip re-searching in sub
			}
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

			setCodePageForInvisibleView(pBuf);

			FindersInfo findersInfo;
			findersInfo._pFileName = pBuf->getFullPathName();

			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, &findersInfo);

			++nbUniqueBuffers;
		}
	}

	_findReplaceDlg.finishFilesSearch(nbTotal, int(nbUniqueBuffers), isEntireDoc);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	if (nbTotal != 0)
	{
		NppParameters& nppParam = NppParameters::getInstance();
		NppGUI& nppGui = nppParam.getNppGUI();
		if (!nppGui._findDlgAlwaysVisible)
		{
			_findReplaceDlg.display(false);
		}
	}

	return true;
}


bool Notepad_plus::findInCurrentFile(bool isEntireDoc)
{
	int nbTotal = 0;
	Buffer * pBuf = _pEditView->getCurrentBuffer();

	Sci_CharacterRange mainSelection = _pEditView->getSelection();  // remember selection before switching view

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	_findReplaceDlg.beginNewFilesSearch();

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());

	setCodePageForInvisibleView(pBuf);

	if (!isEntireDoc)
	{
		auto docLength = _invisibleEditView.execute(SCI_GETLENGTH);

		if ((mainSelection.cpMin > 0) || (mainSelection.cpMax < docLength))
		{
			_invisibleEditView.execute(SCI_SETSELECTIONSTART, mainSelection.cpMin);
			_invisibleEditView.execute(SCI_SETSELECTIONEND, mainSelection.cpMax);
		}
		else
		{
			isEntireDoc = true;
		}
	}

	FindersInfo findersInfo;
	findersInfo._pFileName = pBuf->getFullPathName();
	nbTotal += _findReplaceDlg.processAll(ProcessFindAll, FindReplaceDlg::_env, isEntireDoc, &findersInfo);

	_findReplaceDlg.finishFilesSearch(nbTotal, 1, isEntireDoc);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);

	if (nbTotal != 0)
	{
		NppParameters& nppParam = NppParameters::getInstance();
		NppGUI& nppGui = nppParam.getNppGUI();
		if (!nppGui._findDlgAlwaysVisible)
		{
			_findReplaceDlg.display(false);
		}
	}

	return true;
}

void Notepad_plus::filePrint(bool showDialog)
{
	Printer printer;

	intptr_t startPos = _pEditView->execute(SCI_GETSELECTIONSTART);
	intptr_t endPos = _pEditView->execute(SCI_GETSELECTIONEND);

	printer.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pEditView, showDialog, startPos, endPos, _nativeLangSpeaker.isRTL());
	printer.doPrint();
}

int Notepad_plus::doSaveOrNot(const TCHAR* fn, bool isMulti)
{
	// In case Notepad++ is iconized into notification zone
	if (!::IsWindowVisible(_pPublicInterface->getHSelf()))
	{
		::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);

		// Send sizing info to make window fit (specially to show tool bar.)
		::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
	}

	if (!isMulti)
	{
		generic_string title, msg;

		if (!_nativeLangSpeaker.getDoSaveOrNotStrings(title, msg))
		{
			title = TEXT("Save");
			msg = TEXT("Save file \"$STR_REPLACE$\" ?");
		}

		msg = stringReplace(msg, TEXT("$STR_REPLACE$"), fn);

		return ::MessageBox(_pPublicInterface->getHSelf(), msg.c_str(), title.c_str(), MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
	}

	DoSaveOrNotBox doSaveOrNotBox;
	doSaveOrNotBox.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), fn, isMulti);
	doSaveOrNotBox.doDialog(_nativeLangSpeaker.isRTL());
	int buttonID = doSaveOrNotBox.getClickedButtonId();
	doSaveOrNotBox.destroy();

	return buttonID;
}

int Notepad_plus::doSaveAll()
{
	// In case Notepad++ is iconized into notification zone
	if (!::IsWindowVisible(_pPublicInterface->getHSelf()))
	{
		::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);

		// Send sizing info to make window fit (specially to show tool bar.)
		::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
	}

	DoSaveAllBox doSaveAllBox;
	doSaveAllBox.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());
	doSaveAllBox.doDialog(_nativeLangSpeaker.isRTL());
	int buttonID = doSaveAllBox.getClickedButtonId();
	doSaveAllBox.destroy();

	return buttonID;
}

int Notepad_plus::doReloadOrNot(const TCHAR *fn, bool dirty)
{
	if (dirty)
		return _nativeLangSpeaker.messageBox("DoReloadOrNotAndLooseChange",
			_pPublicInterface->getHSelf(),
			TEXT("\"$STR_REPLACE$\"\r\rThis file has been modified by another program.\rDo you want to reload it and lose the changes made in Notepad++?"),
			TEXT("Reload"),
			MB_YESNO | MB_APPLMODAL | MB_ICONEXCLAMATION,
			0, // not used
			fn);
	else
		return _nativeLangSpeaker.messageBox("DoReloadOrNot",
			_pPublicInterface->getHSelf(),
			TEXT("\"$STR_REPLACE$\"\r\rThis file has been modified by another program.\rDo you want to reload it?"),
			TEXT("Reload"),
			MB_YESNO | MB_APPLMODAL | MB_ICONQUESTION,
			0, // not used
			fn);
}

int Notepad_plus::doCloseOrNot(const TCHAR *fn)
{
	return _nativeLangSpeaker.messageBox("DoCloseOrNot",
		_pPublicInterface->getHSelf(),
		TEXT("The file \"$STR_REPLACE$\" doesn't exist anymore.\rKeep this file in editor?"),
		TEXT("Keep non existing file"),
		MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL,
		0, // not used
		fn);
}

int Notepad_plus::doDeleteOrNot(const TCHAR *fn)
{
	return _nativeLangSpeaker.messageBox("DoDeleteOrNot",
		_pPublicInterface->getHSelf(),
		TEXT("The file \"$STR_REPLACE$\"\rwill be moved to your Recycle Bin and this document will be closed.\rContinue?"),
		TEXT("Delete file"),
		MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL,
		0, // not used
		fn);
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
	enableCommand(IDM_EDIT_PROPERCASE_FORCE, hasSelection, MENU);
	enableCommand(IDM_EDIT_PROPERCASE_BLEND, hasSelection, MENU);
	enableCommand(IDM_EDIT_SENTENCECASE_FORCE, hasSelection, MENU);
	enableCommand(IDM_EDIT_SENTENCECASE_BLEND, hasSelection, MENU);
	enableCommand(IDM_EDIT_INVERTCASE, hasSelection, MENU);
	enableCommand(IDM_EDIT_RANDOMCASE, hasSelection, MENU);
}

void Notepad_plus::checkDocState()
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();

	bool isCurrentDirty = curBuf->isDirty();
	bool isSeveralDirty = isCurrentDirty;
	bool isFileExisting = PathFileExists(curBuf->getFullPathName()) != FALSE;
	if (!isCurrentDirty)
	{
		for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
		{
			if (MainFileManager.getBufferByIndex(i)->isDirty())
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
	enableCommand(IDM_EDIT_CLEARREADONLY, isSysReadOnly, MENU);

	bool doEnable = !(curBuf->isMonitoringOn() || isSysReadOnly);
	enableCommand(IDM_EDIT_SETREADONLY, doEnable, MENU);

	bool isUserReadOnly = curBuf->getUserReadOnly();
	::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | (isUserReadOnly ? MF_CHECKED : MF_UNCHECKED));

	enableCommand(IDM_FILE_DELETE, isFileExisting, MENU);
	//enableCommand(IDM_FILE_RENAME, isFileExisting, MENU);
	enableCommand(IDM_FILE_OPEN_CMD, isFileExisting, MENU);
	enableCommand(IDM_FILE_OPEN_FOLDER, isFileExisting, MENU);
	enableCommand(IDM_FILE_RELOAD, isFileExisting, MENU);

	enableCommand(IDM_FILE_OPEN_DEFAULT_VIEWER, isAssoCommandExisting(curBuf->getFullPathName()), MENU);

	enableCommand(IDM_VIEW_IN_FIREFOX, isFileExisting, MENU);
	enableCommand(IDM_VIEW_IN_CHROME, isFileExisting, MENU);
	enableCommand(IDM_VIEW_IN_IE, isFileExisting, MENU);
	enableCommand(IDM_VIEW_IN_EDGE, isFileExisting, MENU);

	enableConvertMenuItems(curBuf->getEolFormat());
	checkUnicodeMenuItems();
	checkLangsMenu(-1);

	if (_pAnsiCharPanel)
		_pAnsiCharPanel->switchEncoding();

	enableCommand(IDM_VIEW_MONITORING, !curBuf->isUntitled(), MENU | TOOLBAR);
	checkMenuItem(IDM_VIEW_MONITORING, curBuf->isMonitoringOn());
	_toolBar.setCheck(IDM_VIEW_MONITORING, curBuf->isMonitoringOn());
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
	enableCommand(IDM_MACRO_SAVECURRENTMACRO, !_macro.empty() && !_recordingMacro && !_recordingSaved, MENU | TOOLBAR);

	enableCommand(IDM_MACRO_RUNMULTIMACRODLG, (!_macro.empty() && !_recordingMacro) || !((NppParameters::getInstance()).getMacroList()).empty(), MENU | TOOLBAR);
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

void Notepad_plus::setupColorSampleBitmapsOnMainMenuItems()
{
	// set up bitmaps on menu items
	// style-related bitmaps of color samples

	struct
	{
		int firstOfThisColorMenuId;
		int styleIndic;
		std::vector<int> sameColorMenuIds;
	}
	bitmapOnStyleMenuItemsInfo[] =
	{
		{ IDM_SEARCH_GONEXTMARKER5, SCE_UNIVERSAL_FOUND_STYLE_EXT5, { IDM_SEARCH_MARKALLEXT5, IDM_SEARCH_MARKONEEXT5, IDM_SEARCH_UNMARKALLEXT5, IDM_SEARCH_GOPREVMARKER5, IDM_SEARCH_STYLE5TOCLIP } },
		{ IDM_SEARCH_GONEXTMARKER4, SCE_UNIVERSAL_FOUND_STYLE_EXT4, { IDM_SEARCH_MARKALLEXT4, IDM_SEARCH_MARKONEEXT4, IDM_SEARCH_UNMARKALLEXT4, IDM_SEARCH_GOPREVMARKER4, IDM_SEARCH_STYLE4TOCLIP } },
		{ IDM_SEARCH_GONEXTMARKER3, SCE_UNIVERSAL_FOUND_STYLE_EXT3, { IDM_SEARCH_MARKALLEXT3, IDM_SEARCH_MARKONEEXT3, IDM_SEARCH_UNMARKALLEXT3, IDM_SEARCH_GOPREVMARKER3, IDM_SEARCH_STYLE3TOCLIP } },
		{ IDM_SEARCH_GONEXTMARKER2, SCE_UNIVERSAL_FOUND_STYLE_EXT2, { IDM_SEARCH_MARKALLEXT2, IDM_SEARCH_MARKONEEXT2, IDM_SEARCH_UNMARKALLEXT2, IDM_SEARCH_GOPREVMARKER2, IDM_SEARCH_STYLE2TOCLIP } },
		{ IDM_SEARCH_GONEXTMARKER1, SCE_UNIVERSAL_FOUND_STYLE_EXT1, { IDM_SEARCH_MARKALLEXT1, IDM_SEARCH_MARKONEEXT1, IDM_SEARCH_UNMARKALLEXT1, IDM_SEARCH_GOPREVMARKER1, IDM_SEARCH_STYLE1TOCLIP } },
		{ IDM_SEARCH_GONEXTMARKER_DEF, SCE_UNIVERSAL_FOUND_STYLE, { IDM_SEARCH_GOPREVMARKER_DEF, IDM_SEARCH_MARKEDTOCLIP } }
	};

	for (size_t j = 0; j < sizeof(bitmapOnStyleMenuItemsInfo) / sizeof(bitmapOnStyleMenuItemsInfo[0]); ++j)
	{
		const Style * pStyle = NppParameters::getInstance().getMiscStylerArray().findByID(bitmapOnStyleMenuItemsInfo[j].styleIndic);
		if (pStyle)
		{

			HDC hDC = GetDC(NULL);
			const int bitmapXYsize = 16;
			HBITMAP hNewBitmap = CreateCompatibleBitmap(hDC, bitmapXYsize, bitmapXYsize);
			HDC hDCn = CreateCompatibleDC(hDC);
			HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hDCn, hNewBitmap));
			RECT rc = { 0, 0, bitmapXYsize, bitmapXYsize };

			// paint full-size black square
			HBRUSH hBlackBrush = CreateSolidBrush(RGB(0,0,0));
			FillRect(hDCn, &rc, hBlackBrush);
			DeleteObject(hBlackBrush);

			// overpaint a slightly smaller colored square
			rc.left = rc.top = 1;
			rc.right = rc.bottom = bitmapXYsize - 1;
			HBRUSH hColorBrush = CreateSolidBrush(pStyle->_bgColor);
			FillRect(hDCn, &rc, hColorBrush);
			DeleteObject(hColorBrush);

			// restore old bitmap so we can delete it to avoid leak
			SelectObject(hDCn, hOldBitmap);
			DeleteDC(hDCn);
			
			SetMenuItemBitmaps(_mainMenuHandle, bitmapOnStyleMenuItemsInfo[j].firstOfThisColorMenuId, MF_BYCOMMAND, hNewBitmap, hNewBitmap);
			for (int relatedMenuId : bitmapOnStyleMenuItemsInfo[j].sameColorMenuIds)
			{
				SetMenuItemBitmaps(_mainMenuHandle, relatedMenuId, MF_BYCOMMAND, hNewBitmap, NULL);
			}
		}
	}
}

bool doCheck(HMENU mainHandle, int id)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_SUBMENU | MIIM_FTYPE | MIIM_ID | MIIM_STATE;

	bool found = false;
	int count = ::GetMenuItemCount(mainHandle);
	for (int i = 0; i < count; i++)
	{
		::GetMenuItemInfo(mainHandle, i, MF_BYPOSITION, &mii);
		if (mii.fType == MFT_RADIOCHECK || mii.fType == MFT_STRING)
		{
			bool checked = mii.hSubMenu ? doCheck(mii.hSubMenu, id) : (mii.wID == (unsigned int)id);
			if (checked)
			{
				::CheckMenuRadioItem(mainHandle, 0, count, i, MF_BYPOSITION);
				found = true;
			}
			else
			{
				mii.fState = 0;
				::SetMenuItemInfo(mainHandle, i, MF_BYPOSITION, &mii);
			}
		}
	}
	return found;
}

void Notepad_plus::checkLangsMenu(int id) const
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();
	if (id == -1)
	{
		id = (NppParameters::getInstance()).langTypeToCommandID(curBuf->getLangType());
		if (id == IDM_LANG_USER)
		{
			if (curBuf->isUserDefineLangExt())
			{
				const TCHAR *userLangName = curBuf->getUserDefineLangName();
				TCHAR menuLangName[langNameLenMax];

				for (int i = IDM_LANG_USER + 1 ; i <= IDM_LANG_USER_LIMIT ; ++i)
				{
					if (::GetMenuString(_mainMenuHandle, i, menuLangName, langNameLenMax, MF_BYCOMMAND))
					{
						if (!lstrcmp(userLangName, menuLangName))
						{
							HMENU _langMenuHandle = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);
							doCheck(_langMenuHandle, i);
							return;
						}
					}
				}
			}
		}
	}
	HMENU _langMenuHandle = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);
	doCheck(_langMenuHandle, id);
}

generic_string Notepad_plus::getLangDesc(LangType langType, bool getName)
{
	NppParameters& nppParams = NppParameters::getInstance();

	if ((langType >= L_EXTERNAL) && (langType < nppParams.L_END))
	{
		ExternalLangContainer & elc = nppParams.getELCFromIndex(langType - L_EXTERNAL);
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const TCHAR* lexerNameW = wmc.char2wchar(elc._name.c_str(), CP_ACP);
		return generic_string(lexerNameW);
	}

	if (langType > L_EXTERNAL)
        langType = L_TEXT;

	generic_string str2Show;
	if (getName)
		str2Show = ScintillaEditView::_langNameInfoArray[langType]._shortName;
	else
		str2Show = ScintillaEditView::_langNameInfoArray[langType]._longName;

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
	intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();
	generic_string globalStr = TEXT("");
	for (intptr_t i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i))
		{
			generic_string currentStr = getMarkedLine(i) + globalStr;
			globalStr = currentStr;
		}
	}
	str2Cliboard(globalStr);
}

std::mutex mark_mutex;

void Notepad_plus::cutMarkedLines()
{
	std::lock_guard<std::mutex> lock(mark_mutex);

	intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();
	generic_string globalStr = TEXT("");

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (intptr_t i = lastLine ; i >= 0 ; i--)
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
	std::lock_guard<std::mutex> lock(mark_mutex);

	intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (intptr_t i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i) == isMarked)
			deleteMarkedline(i);
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::pasteToMarkedLines()
{
	std::lock_guard<std::mutex> lock(mark_mutex);

	int clipFormat;
	clipFormat = CF_UNICODETEXT;

	BOOL canPaste = ::IsClipboardFormatAvailable(clipFormat);
	if (!canPaste)
		return;
	intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();

	::OpenClipboard(_pPublicInterface->getHSelf());
	HANDLE clipboardData = ::GetClipboardData(clipFormat);
	::GlobalSize(clipboardData);
	LPVOID clipboardDataPtr = ::GlobalLock(clipboardData);
	if (!clipboardDataPtr) return;

	generic_string clipboardStr = (const TCHAR *)clipboardDataPtr;

	::GlobalUnlock(clipboardData);
	::CloseClipboard();

	_pEditView->execute(SCI_BEGINUNDOACTION);
	for (intptr_t i = lastLine ; i >= 0 ; i--)
	{
		if (bookmarkPresent(i))
		{
			replaceMarkedline(i, clipboardStr.c_str());
		}
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::deleteMarkedline(size_t ln)
{
	intptr_t lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
	intptr_t lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);

	bookmarkDelete(ln);
	TCHAR emptyString[2] = TEXT("");
	_pEditView->replaceTarget(emptyString, lineBegin, lineBegin + lineLen);
}

void Notepad_plus::inverseMarks()
{
	intptr_t lastLine = _pEditView->lastZeroBasedLineNumber();
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

void Notepad_plus::replaceMarkedline(size_t ln, const TCHAR *str)
{
	intptr_t lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);
	intptr_t lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, ln);

	_pEditView->replaceTarget(str, lineBegin, lineEnd);
}

generic_string Notepad_plus::getMarkedLine(size_t ln)
{
	auto lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
	auto lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);

	TCHAR * buf = new TCHAR[lineLen+1];
	_pEditView->getGenericText(buf, lineLen + 1, lineBegin, lineBegin + lineLen);
	generic_string line = buf;
	delete [] buf;

	return line;
}

void Notepad_plus::findMatchingBracePos(intptr_t& braceAtCaret, intptr_t& braceOpposite)
{
	intptr_t caretPos = _pEditView->execute(SCI_GETCURRENTPOS);
	braceAtCaret = -1;
	braceOpposite = -1;
	TCHAR charBefore = '\0';

	intptr_t lengthDoc = _pEditView->execute(SCI_GETLENGTH);

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
		braceOpposite = _pEditView->execute(SCI_BRACEMATCH, braceAtCaret, 0);
}

// return true if 1 or 2 (matched) brace(s) is found
bool Notepad_plus::braceMatch()
{
	Buffer* currentBuf = _pEditView->getCurrentBuffer();
	if (currentBuf->isLargeFile())
		return false;

	intptr_t braceAtCaret = -1;
	intptr_t braceOpposite = -1;
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
            intptr_t columnAtCaret = _pEditView->execute(SCI_GETCOLUMN, braceAtCaret);
		    intptr_t columnOpposite = _pEditView->execute(SCI_GETCOLUMN, braceOpposite);
			_pEditView->execute(SCI_SETHIGHLIGHTGUIDE, (columnAtCaret < columnOpposite)?columnAtCaret:columnOpposite);
        }
    }

	const bool enable = (braceAtCaret != -1) && (braceOpposite != -1);
    enableCommand(IDM_SEARCH_GOTOMATCHINGBRACE, enable, MENU | TOOLBAR);
	enableCommand(IDM_SEARCH_SELECTMATCHINGBRACES, enable, MENU);
    return (braceAtCaret != -1);
}


void Notepad_plus::setLangStatus(LangType langType)
{
	_statusBar.setText(getLangDesc(langType).c_str(), STATUSBAR_DOC_TYPE);
}


void Notepad_plus::setDisplayFormat(EolType format)
{
	const TCHAR* str = TEXT("??");
	switch (format)
	{
		case EolType::windows: str = TEXT("Windows (CR LF)"); break;
		case EolType::macos:   str = TEXT("Macintosh (CR)"); break;
		case EolType::unix:    str = TEXT("Unix (LF)"); break;
		case EolType::unknown: str = TEXT("Unknown"); assert(false);  break;
	}
	_statusBar.setText(str, STATUSBAR_EOF_FORMAT);
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
				uniModeTextString = TEXT("UTF-8-BOM"); break;
			case uni16BE:
				uniModeTextString = TEXT("UTF-16 BE BOM"); break;
			case uni16LE:
				uniModeTextString = TEXT("UTF-16 LE BOM"); break;
			case uni16BE_NoBOM:
				uniModeTextString = TEXT("UTF-16 Big Endian"); break;
			case uni16LE_NoBOM:
				uniModeTextString = TEXT("UTF-16 Little Endian"); break;
			case uniCookie:
				uniModeTextString = TEXT("UTF-8"); break;
			default :
				uniModeTextString = TEXT("ANSI");
		}
	}
	else
	{
		EncodingMapper& em = EncodingMapper::getInstance();
		int cmdID = em.getIndexFromEncoding(encoding);
		if (cmdID == -1)
		{
			assert(!"Encoding problem. Encoding is not added in encoding_table?");
			return;
		}
		cmdID += IDM_FORMAT_ENCODE;

		const int itemSize = 64;
		TCHAR uniModeText[itemSize] = {};
		::GetMenuString(_mainMenuHandle, cmdID, uniModeText, itemSize, MF_BYCOMMAND);
		uniModeTextString = uniModeText;
		// Remove the shortcut text from the menu text.
		const size_t tabPos = uniModeTextString.find_last_of('\t');
		if (tabPos != generic_string::npos)
			uniModeTextString.resize(tabPos);
	}
	_statusBar.setText(uniModeTextString.c_str(), STATUSBAR_UNICODE_TYPE);
}

bool isUrlSchemeStartChar(TCHAR const c)
{
	return ((c >= 'A') && (c <= 'Z'))
		|| ((c >= 'a') && (c <= 'z'));
}

bool isUrlSchemeDelimiter(TCHAR const c) // characters allowed immedeately before scheme
{
	return   ! (((c >= '0') && (c <= '9'))
			 || ((c >= 'A') && (c <= 'Z'))
			 || ((c >= 'a') && (c <= 'z'))
			 ||  (c == '_'));
}

bool isUrlTextChar(TCHAR const c)
{
	if (c <= ' ') return false;
	switch (c)
	{
		case '"':
		case '#':
		case '<':
		case '>':
		case '{':
		case '}':
		case '?':
		case '\u007F':
			return false;
	}
	return true;
}

bool isUrlQueryDelimiter(TCHAR const c)
{
	switch(c)
	{
		case '&':
		case '+':
		case '=':
		case ';':
			return true;
	}
	return false;
}

bool isUrlSchemeSupported(INTERNET_SCHEME s, TCHAR *url)
{
	switch (s)
	{
		case INTERNET_SCHEME_FTP:
		case INTERNET_SCHEME_HTTP:
		case INTERNET_SCHEME_HTTPS:
		case INTERNET_SCHEME_MAILTO:
		case INTERNET_SCHEME_FILE:
			return true;
	}
	generic_string const mySchemes = (NppParameters::getInstance()).getNppGUI()._uriSchemes + TEXT(" ");
	TCHAR *p = (TCHAR *)mySchemes.c_str();
	while (*p)
	{
		int i = 0;
		while (p [i] && (p [i] != ' ')) i++;
		if (i == 0) return false;
		if (generic_strnicmp (url, p, i) == 0) return true;
		p += i;
		while (*p == ' ') p++;
	}
	return false;
}

// scanToUrlStart searches for a possible URL in <text>.
// If a possible URL is found, then:
// - True is returned.
// - The number of characters between <text[start]> and the beginning of the URL candidate is stored in <distance>.
// - The length of the URL scheme is stored in <schemeLength>.
// If no URL is found, then:
// - False is returned.
// - The number of characters between <text[start]> and the end of text is stored in <distance>.
bool scanToUrlStart(TCHAR *text, int textLen, int start, int* distance, int* schemeLength)
{
	int p = start;
	int p0 = 0;
	enum {sUnknown, sScheme} s = sUnknown;
	while (p < textLen)
	{
		switch (s)
		{
			case sUnknown:
				if (isUrlSchemeStartChar(text [p]) && ((p == 0) || isUrlSchemeDelimiter(text [p - 1])))
				{
					p0 = p;
					s = sScheme;
				}
				break;

			case sScheme:
				if (text [p] == ':')
				{
					*distance = p0 - start;
					*schemeLength = p - p0 + 1;
					return true;
				}
				if (!isUrlSchemeStartChar(text [p]))
					s = sUnknown;
				break;
		}
		p++;
	}
	*schemeLength = 0;
	*distance = p - start;
	return false;
}

// scanToUrlEnd searches the end of an URL, coarsly parsing its main parts HostAndPath, Query and Fragment.
//
// In the query part, a simple pattern is enforced, to avoid that everything goes through as a query.
// The pattern is kept simple, since there seem to be many different forms of queries used in the world.
// The objective here is not to detect whether or not a query is malformed. The objective is, to let through
// most of the real world's queries, and to sort out what is certainly not a query.
//
// The approach is:
// - A query begins with '?', followed by any number of values,
//   which are separated by a single delimiter character '&', '+', '=' or ';'.
// - Each value may be enclosed in single or double quotes.
//
// The query pattern going through looks like this:
// - ?abc;def;fgh="i j k"&'l m n'+opq
//
void scanToUrlEnd(TCHAR *text, int textLen, int start, int* distance)
{
	int p = start;
	TCHAR q = 0;
	enum {sHostAndPath, sQuery, sQueryAfterDelimiter, sQueryQuotes, sQueryAfterQuotes, sFragment} s = sHostAndPath;
	while (p < textLen)
	{
		switch (s)
		{
			case sHostAndPath: 
				if (text [p] == '?')
					s = sQuery;
				else if (text [p] == '#')
					s = sFragment;
				else if (!isUrlTextChar (text [p]))
				{
					*distance = p - start;
					return;
				}
				break;

			case sQuery:
				if (text [p] == '#')
					s = sFragment;
				else if (isUrlQueryDelimiter (text [p]))
					s = sQueryAfterDelimiter;
				else if (!isUrlTextChar(text [p]))
				{
					*distance = p - start;
					return;
				}
				break;

			case sQueryAfterDelimiter:
				if ((text [p] == '\'') || (text [p] == '"') || (text [p] == '`'))
				{
					q = text [p];
					s = sQueryQuotes;
				}
				else if (text [p] == '(')
				{
					q = ')';
					s = sQueryQuotes;
				}
				else if (text [p] == '[')
				{
					q = ']';
					s = sQueryQuotes;
				}
				else if (text [p] == '{')
				{
					q = '}';
					s = sQueryQuotes;
				}
				else if (isUrlTextChar(text [p]))
					s = sQuery;
				else
				{
					*distance = p - start;
					return;
				}
				break;

			case sQueryQuotes:
				if (text [p] < ' ')
				{
					*distance = p - start;
					return;
				}
				if (text [p] == q)
					s = sQueryAfterQuotes;
				break;
	
			case sQueryAfterQuotes:
				if (isUrlQueryDelimiter (text [p]))
					s = sQueryAfterDelimiter;
				else
				{
					*distance = p - start;
					return;
				}
				break;

			case sFragment:
				if (!isUrlTextChar(text [p]))
				{
					*distance = p - start;
					return;
				}
				break;
		}
		p++;
	}
	*distance = p - start;
}

// removeUnwantedTrailingCharFromUrl removes a single unwanted trailing character from an URL.
// It has to be called repeatedly, until it returns false, meaning that all unwanted characters are gone.
bool removeUnwantedTrailingCharFromUrl (TCHAR const *text, int* length)
{
	int l = *length - 1;
	if (l <= 0) return false;
	{ // remove unwanted single characters
		const TCHAR *singleChars = L".,:;?!#";
		for (int i = 0; singleChars [i]; i++)
			if (text [l] == singleChars [i])
			{
				*length = l;
				return true;
			}
	}
	{ // remove unwanted closing parenthesis
		const TCHAR *closingParenthesis = L")]";
		const TCHAR *openingParenthesis = L"([";
		for (int i = 0; closingParenthesis [i]; i++)
			if (text [l] == closingParenthesis [i])
			{
				int count = 0;
				for (int j = l - 1; j >= 0; j--)
				{
					if (text [j] == closingParenthesis [i])
						count++;
					if (text [j] == openingParenthesis [i])
						if (count > 0)
							count--;
						else
							return false;
				}
				if (count != 0)
					return false;
				*length = l;
				return true;
			}
	}
	return false;
}

// isUrl checks, whether there is a valid URL at <text [start]>.
// If yes:
// - True is returned.
// - The length of the URL is stored in <segmentLen>.
// If no:
// - False is returned.
// - The number of characters between <text[start]> and the next URL is stored in <segementLen>.
// - If no URL is found at all, then the number of characters between <text[start]> and the end of text is stored in <segmentLen>.
bool isUrl(TCHAR * text, int textLen, int start, int* segmentLen)
{
	int dist = 0, schemeLen = 0;
	if (scanToUrlStart(text, textLen, start, & dist, & schemeLen))
	{
		if (dist)
		{
			*segmentLen = dist;
			return false;
		}
		int len = 0;
		scanToUrlEnd (text, textLen, start + schemeLen, & len);
		if (len)
		{
			len += schemeLen;
			URL_COMPONENTS url;
			memset (& url, 0, sizeof(url));
			url.dwStructSize = sizeof(url);
			bool r  = InternetCrackUrl(& text [start], len, 0, & url) && isUrlSchemeSupported(url.nScheme, & text [start]);
			if (r)
			{
				while (removeUnwantedTrailingCharFromUrl (& text [start], & len));
				*segmentLen = len;
				return true;
			}
		}
		len = 1;
		int lMax = textLen - start;
		while (isUrlSchemeStartChar(text[start+len]) && (len < lMax)) len++;
		*segmentLen = len;
		return false;
	}
	*segmentLen = dist;
	return false;
}

void Notepad_plus::addHotSpot(ScintillaEditView* view)
{
	Buffer* currentBuf = _pEditView->getCurrentBuffer();
	if (currentBuf->isLargeFile())
		return;

	ScintillaEditView* pView = view ? view : _pEditView;

	int urlAction = (NppParameters::getInstance()).getNppGUI()._styleURL;
	LPARAM indicStyle = (urlAction == urlNoUnderLineFg) || (urlAction == urlNoUnderLineBg) ? INDIC_HIDDEN : INDIC_PLAIN;
	LPARAM indicHoverStyle = (urlAction == urlNoUnderLineBg) || (urlAction == urlUnderLineBg) ? INDIC_FULLBOX : INDIC_EXPLORERLINK;
	LPARAM indicStyleCur = pView->execute(SCI_INDICGETSTYLE, URL_INDIC);
	LPARAM indicHoverStyleCur = pView->execute(SCI_INDICGETHOVERSTYLE, URL_INDIC);

	if ((indicStyleCur != indicStyle) || (indicHoverStyleCur != indicHoverStyle))
	{
		pView->execute(SCI_INDICSETSTYLE, URL_INDIC, indicStyle);
		pView->execute(SCI_INDICSETHOVERSTYLE, URL_INDIC, indicHoverStyle);
		pView->execute(SCI_INDICSETALPHA, URL_INDIC, 70);
		pView->execute(SCI_INDICSETFLAGS, URL_INDIC, SC_INDICFLAG_VALUEFORE);
	}

	intptr_t startPos = 0;
	intptr_t endPos = -1;
	pView->getVisibleStartAndEndPosition(&startPos, &endPos);
	if (startPos >= endPos) return;
	pView->execute(SCI_SETINDICATORCURRENT, URL_INDIC);
	if (urlAction == urlDisable)
	{
		pView->execute(SCI_INDICATORCLEARRANGE, startPos, endPos - startPos);
		return;
	}

	LRESULT indicFore = pView->execute(SCI_STYLEGETFORE, STYLE_DEFAULT);
	pView->execute(SCI_SETINDICATORVALUE, indicFore);

	UINT cp = static_cast<UINT>(pView->execute(SCI_GETCODEPAGE));
	char *encodedText = new char[endPos - startPos + 1];
	pView->getText(encodedText, startPos, endPos);
	TCHAR *wideText = new TCHAR[endPos - startPos + 1];
	int wideTextLen = MultiByteToWideChar(cp, 0, encodedText, static_cast<int>(endPos - startPos + 1), (LPWSTR) wideText, static_cast<int>(endPos - startPos + 1)) - 1;
	delete[] encodedText;
	if (wideTextLen > 0)
	{
		int startWide = 0;
		int lenWide = 0;
		int startEncoded = 0;
		int lenEncoded = 0;
		while (true)
		{
			bool r = isUrl(wideText, wideTextLen, startWide, & lenWide);
			if (lenWide <= 0)
				break;
			assert ((startWide + lenWide) <= wideTextLen);
			lenEncoded = WideCharToMultiByte(cp, 0, & wideText [startWide], lenWide, NULL, 0, NULL, NULL);
			if (r)
				pView->execute(SCI_INDICATORFILLRANGE, startEncoded + startPos, lenEncoded);
			else
				pView->execute(SCI_INDICATORCLEARRANGE, startEncoded + startPos, lenEncoded);
			startWide += lenWide;
			startEncoded += lenEncoded;
			if ((startWide >= wideTextLen) || ((startEncoded + startPos) >= endPos))
				break;
		}
		assert ((startEncoded + startPos) == endPos);
		assert (startWide == wideTextLen);
	}
	delete[] wideText;
}

bool Notepad_plus::isConditionExprLine(intptr_t lineNumber)
{
	if (lineNumber < 0 || lineNumber > _pEditView->execute(SCI_GETLINECOUNT))
		return false;

	auto startPos = _pEditView->execute(SCI_POSITIONFROMLINE, lineNumber);
	auto endPos = _pEditView->execute(SCI_GETLINEENDPOSITION, lineNumber);
	_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
	_pEditView->execute(SCI_SETTARGETRANGE, startPos, endPos);

	const char ifElseForWhileExpr[] = "((else[ \t]+)?if|for|while)[ \t]*[(].*[)][ \t]*|else[ \t]*";

	auto posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(ifElseForWhileExpr), reinterpret_cast<LPARAM>(ifElseForWhileExpr));
	if (posFound >= 0)
	{
		auto end = _pEditView->execute(SCI_GETTARGETEND);
		if (end == endPos)
			return true;
	}

	return false;
}

intptr_t Notepad_plus::findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol)
{
	if (startPos == endPos)
		return -1;

	if (startPos > endPos) // backward
	{
		int balance = 0;
		for (intptr_t i = startPos; i >= static_cast<intptr_t>(endPos); --i)
		{
			char aChar = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, i));
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
	else // forward
	{
	}
	return -1;
}

void Notepad_plus::maintainIndentation(TCHAR ch)
{
	intptr_t eolMode = _pEditView->execute(SCI_GETEOLMODE);
	intptr_t curLine = _pEditView->getCurrentLineNumber();
	intptr_t prevLine = curLine - 1;
	intptr_t indentAmountPrevLine = 0;
	intptr_t tabWidth = _pEditView->execute(SCI_GETTABWIDTH);

	LangType type = _pEditView->getCurrentBuffer()->getLangType();
	ExternalLexerAutoIndentMode autoIndentMode = ExternalLexerAutoIndentMode::Standard;

	// For external languages, query for custom auto-indentation funcionality
	if (type >= L_EXTERNAL)
	{
		NppParameters& nppParam = NppParameters::getInstance();
		autoIndentMode = nppParam.getELCFromIndex(type - L_EXTERNAL)._autoIndentMode;
		if (autoIndentMode == ExternalLexerAutoIndentMode::Custom)
			return;
	}

	// Do not alter indentation if we were at the beginning of the line and we pressed Enter
	if ((((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		(eolMode == SC_EOL_CR && ch == '\r')) && prevLine >= 0 && _pEditView->getLineLength(prevLine) == 0)
		return;

	if (type == L_C || type == L_CPP || type == L_JAVA || type == L_CS || type == L_OBJC ||
		type == L_PHP || type == L_JS || type == L_JAVASCRIPT || type == L_JSP || type == L_CSS || type == L_PERL || 
		type == L_RUST || type == L_POWERSHELL || type == L_JSON || autoIndentMode == ExternalLexerAutoIndentMode::C_Like)
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
			intptr_t prevPos = _pEditView->execute(SCI_GETCURRENTPOS) - (eolMode == SC_EOL_CRLF ? 3 : 2);
			UCHAR prevChar = (UCHAR)_pEditView->execute(SCI_GETCHARAT, prevPos);
			auto curPos = _pEditView->execute(SCI_GETCURRENTPOS);
			UCHAR nextChar = (UCHAR)_pEditView->execute(SCI_GETCHARAT, curPos);

			if (prevChar == '{')
			{
				if (nextChar == '}')
				{
					const char *eolChars;
					if (eolMode == SC_EOL_CRLF)
						eolChars = "\r\n";
					else if (eolMode == SC_EOL_LF)
						eolChars = "\n";
					else
						eolChars = "\r";

					_pEditView->execute(SCI_INSERTTEXT, _pEditView->execute(SCI_GETCURRENTPOS), reinterpret_cast<LPARAM>(eolChars));
					_pEditView->setLineIndent(curLine + 1, indentAmountPrevLine);
				}
				_pEditView->setLineIndent(curLine, indentAmountPrevLine + tabWidth);
			}
			else if (nextChar == '{')
			{
				_pEditView->setLineIndent(curLine, indentAmountPrevLine);
			}
			// These languages do no support single line control structures without braces.
			else if (type == L_PERL || type == L_RUST || type == L_POWERSHELL || type == L_JSON)
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
			auto startPos = _pEditView->execute(SCI_POSITIONFROMLINE, curLine);
			LRESULT endPos = _pEditView->execute(SCI_GETCURRENTPOS);

			for (LRESULT i = endPos - 2; i > 0 && i > startPos; --i)
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

				auto startPos2 = _pEditView->execute(SCI_POSITIONFROMLINE, prevLine);
				auto endPos2 = _pEditView->execute(SCI_GETLINEENDPOSITION, prevLine);
				_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
				_pEditView->execute(SCI_SETTARGETRANGE, startPos2, endPos2);

				const char braceExpr[] = "[ \t]*\\{.*";

				intptr_t posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(braceExpr), reinterpret_cast<LPARAM>(braceExpr));
				if (posFound >= 0)
				{
					auto end = _pEditView->execute(SCI_GETTARGETEND);
					if (end == endPos2)
						indentAmountPrevLine += tabWidth;
				}
			}

			_pEditView->setLineIndent(curLine, indentAmountPrevLine);

		}
		else if (ch == '}')
		{
			// Look backward for the pair {
			intptr_t startPos = _pEditView->execute(SCI_GETCURRENTPOS);
			if (startPos != 0)
				startPos -= 1;
			intptr_t posFound = findMachedBracePos(startPos - 1, 0, '{', '}');

			// if no { found, do nothing
			if (posFound == -1)
				return;

			// if { is in the same line, do nothing
			intptr_t matchedPairLine = _pEditView->execute(SCI_LINEFROMPOSITION, posFound);
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

BOOL Notepad_plus::processFindAccel(MSG *msg) const
{
	if (!::IsChild(_findReplaceDlg.getHSelf(), ::GetFocus()))
		return FALSE;
	return ::TranslateAccelerator(_findReplaceDlg.getHSelf(), _accelerator.getFindAccTable(), msg);
}

BOOL Notepad_plus::processIncrFindAccel(MSG *msg) const
{
	if (!::IsChild(_incrementFindDlg.getHSelf(), ::GetFocus()))
		return FALSE;
	return ::TranslateAccelerator(_incrementFindDlg.getHSelf(), _accelerator.getIncrFindAccTable(), msg);
}

void Notepad_plus::setLanguage(LangType langType)
{
	//Add logic to prevent changing a language when a document is shared between two views
	//If so, release one document
	bool reset = false;
	Document prev = 0;
	if (bothActive())
	{
		if (_mainEditView.getCurrentBufferID() == _subEditView.getCurrentBufferID())
		{
			reset = true;
			_subEditView.saveCurrentPos();
			prev = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
		}
	}
	
	if (reset)
	{
		_mainEditView.getCurrentBuffer()->setLangType(langType);
	}
	else
	{
		_pEditView->getCurrentBuffer()->setLangType(langType);
	}

	if (reset)
	{
		_subEditView.execute(SCI_SETDOCPOINTER, 0, prev);
		_subEditView.restoreCurrentPosPreStep();
	}
};

LangType Notepad_plus::menuID2LangType(int cmdID)
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
			return L_JAVASCRIPT;
		case IDM_LANG_JSON:
			return L_JSON;
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
		case IDM_LANG_FORTRAN_77 :
			return L_FORTRAN_77;
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
		case IDM_LANG_BAANC:
			return L_BAANC;
		case IDM_LANG_SREC :
            return L_SREC;
		case IDM_LANG_IHEX :
            return L_IHEX;
		case IDM_LANG_TEHEX :
            return L_TEHEX;
		case IDM_LANG_SWIFT:
			return L_SWIFT;
        case IDM_LANG_ASN1 :
            return L_ASN1;
        case IDM_LANG_AVS :
            return L_AVS;
        case IDM_LANG_BLITZBASIC :
            return L_BLITZBASIC;
        case IDM_LANG_PUREBASIC :
            return L_PUREBASIC;
        case IDM_LANG_FREEBASIC :
            return L_FREEBASIC;
        case IDM_LANG_CSOUND :
            return L_CSOUND;
        case IDM_LANG_ERLANG :
            return L_ERLANG;
        case IDM_LANG_ESCRIPT :
            return L_ESCRIPT;
        case IDM_LANG_FORTH :
            return L_FORTH;
        case IDM_LANG_LATEX :
            return L_LATEX;
        case IDM_LANG_MMIXAL :
            return L_MMIXAL;
        case IDM_LANG_NIM :
            return L_NIM;
        case IDM_LANG_NNCRONTAB :
            return L_NNCRONTAB;
        case IDM_LANG_OSCRIPT :
            return L_OSCRIPT;
        case IDM_LANG_REBOL :
            return L_REBOL;
        case IDM_LANG_REGISTRY :
            return L_REGISTRY;
        case IDM_LANG_RUST :
            return L_RUST;
        case IDM_LANG_SPICE :
            return L_SPICE;
        case IDM_LANG_TXT2TAGS :
            return L_TXT2TAGS;
        case IDM_LANG_VISUALPROLOG:
            return L_VISUALPROLOG;
        case IDM_LANG_TYPESCRIPT:
            return L_TYPESCRIPT;
        case IDM_LANG_USER:
            return L_USER;
		default:
		{
			if (cmdID >= IDM_LANG_USER && cmdID <= IDM_LANG_USER_LIMIT)
			{
				return L_USER;
			}
			break; 
		}
	}
	return L_EXTERNAL;
}


void Notepad_plus::setTitle()
{
	NppParameters& nppParams = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParams.getNppGUI();
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

	if (_isAdministrator)
		result += TEXT(" [Administrator]");

	generic_string tbAdd = nppParams.getTitleBarAdd();
	if (!tbAdd.empty())
	{
		result += TEXT(" - ");
		result += tbAdd;
	}

	::SendMessage(_pPublicInterface->getHSelf(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(result.c_str()));
}

void Notepad_plus::activateNextDoc(bool direction)
{
	int nbDoc = static_cast<int32_t>(_pDocTab->nbItem());

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
			nbDoc = static_cast<int32_t>(_pDocTab->nbItem());
		}
		curIndex = nbDoc - 1;
	}

	BufferID id = _pDocTab->getBufferByIndex(curIndex);
	activateBuffer(id, currentView());
}

void Notepad_plus::activateDoc(size_t pos)
{
	size_t nbDoc = _pDocTab->nbItem();
	if (pos == static_cast<size_t>(_pDocTab->getCurrentTabIndex()))
	{
		Buffer * buf = _pEditView->getCurrentBuffer();
		buf->increaseRecentTag();
		return;
	}

	if (pos < nbDoc)
	{
		BufferID id = _pDocTab->getBufferByIndex(pos);
		activateBuffer(id, currentView());
	}
}


static const char utflen[] = {1,1,2,3};

size_t Notepad_plus::getSelectedCharNumber(UniMode u)
{
	size_t result = 0;
	size_t numSel = _pEditView->execute(SCI_GETSELECTIONS);
	if (u == uniUTF8 || u == uniCookie)
	{
		for (size_t i = 0; i < numSel; ++i)
		{
			size_t line1 = _pEditView->execute(SCI_LINEFROMPOSITION, _pEditView->execute(SCI_GETSELECTIONNSTART, i));
			size_t line2 = _pEditView->execute(SCI_LINEFROMPOSITION, _pEditView->execute(SCI_GETSELECTIONNEND, i));
			for (size_t j = line1; j <= line2; ++j)
			{
				size_t stpos = _pEditView->execute(SCI_GETLINESELSTARTPOSITION, j);
				if (static_cast<intptr_t>(stpos) != INVALID_POSITION)
				{
					size_t endpos = _pEditView->execute(SCI_GETLINESELENDPOSITION, j);
					for (size_t pos = stpos; pos < endpos; ++pos)
					{
						unsigned char c = 0xf0 & static_cast<unsigned char>(_pEditView->execute(SCI_GETCHARAT, pos));
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
		for (size_t i = 0; i < numSel; ++i)
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
static inline size_t countUtf8Characters(unsigned char *buf, size_t pos, size_t endpos)
{
	size_t result = 0;
	while (pos < endpos)
	{
		unsigned char c = buf[pos++];
		if ((c&0xc0) == 0x80 // do not count unexpected continuation bytes (this handles the case where an UTF-8 character is split in the middle)
			|| c == '\n' || c == '\r') continue; // do not count end of lines
		if (c >= 0xc0) 
			pos += utflen[(c & 0x30) >>  4];
		++result;
	}
	return result;
}


size_t Notepad_plus::getCurrentDocCharCount(UniMode u)
{
	if (u != uniUTF8 && u != uniCookie)
	{
		size_t numLines = _pEditView->execute(SCI_GETLINECOUNT);
		auto result = _pEditView->execute(SCI_GETLENGTH);
		size_t lines = numLines==0?0:numLines-1;
		if (_pEditView->execute(SCI_GETEOLMODE) == SC_EOL_CRLF) lines *= 2;
		result -= lines;
		return (result < 0) ? 0 : result;
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
		if (endpos > 50000) // starting threads takes time; for small files it is better to simply count in one thread
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

size_t Notepad_plus::getSelectedAreas()
{
	size_t numSel = _pEditView->execute(SCI_GETSELECTIONS);
	if (numSel == 1) // either 0 or 1 selection
		return (_pEditView->execute(SCI_GETSELECTIONNSTART, 0) == _pEditView->execute(SCI_GETSELECTIONNEND, 0)) ? 0 : 1;
	return (_pEditView->execute(SCI_SELECTIONISRECTANGLE)) ? 1 : numSel;
}

size_t Notepad_plus::getSelectedBytes()
{
	size_t numSel = _pEditView->execute(SCI_GETSELECTIONS);
	size_t result = 0;
	for (size_t i = 0; i < numSel; ++i)
		result += (_pEditView->execute(SCI_GETSELECTIONNEND, i) - _pEditView->execute(SCI_GETSELECTIONNSTART, i));
	return result;
}

int Notepad_plus::wordCount()
{
    FindOption env;
    env._str2Search = TEXT("[^ 	\\\\.,;:!?()+\\r\\n\\-\\*/=\\]\\[{}&~\"'`|@$%<>\\^]+");
    env._searchType = FindRegex;
    return _findReplaceDlg.processAll(ProcessCountAll, &env, true);
}

void Notepad_plus::updateStatusBar()
{
	// these sections of status bar NOT updated by this function:
	// STATUSBAR_DOC_TYPE , STATUSBAR_EOF_FORMAT , STATUSBAR_UNICODE_TYPE

	TCHAR strDocLen[256];
	size_t docLen = _pEditView->getCurrentDocLen();
	intptr_t nbLine = _pEditView->execute(SCI_GETLINECOUNT);
	wsprintf(strDocLen, TEXT("length : %s    lines : %s"),
		commafyInt(docLen).c_str(),
		commafyInt(nbLine).c_str());
	_statusBar.setText(strDocLen, STATUSBAR_DOC_SIZE);

	TCHAR strSel[64];

	size_t numSelections = _pEditView->execute(SCI_GETSELECTIONS);
	if (numSelections == 1)
	{
		if (_pEditView->execute(SCI_GETSELECTIONEMPTY))
		{
			size_t currPos = _pEditView->execute(SCI_GETCURRENTPOS);
			wsprintf(strSel, TEXT("Pos : %s"), commafyInt(currPos + 1).c_str());
		}
		else
		{
			const std::pair<size_t, size_t> oneSelCharsAndLines = _pEditView->getSelectedCharsAndLinesCount();
			wsprintf(strSel, TEXT("Sel : %s | %s"),
				commafyInt(oneSelCharsAndLines.first).c_str(),
				commafyInt(oneSelCharsAndLines.second).c_str());
		}
	}
	else if (_pEditView->execute(SCI_SELECTIONISRECTANGLE))
	{
		const std::pair<size_t, size_t> rectSelCharsAndLines = _pEditView->getSelectedCharsAndLinesCount();

		bool sameCharCountOnEveryLine = true;
		size_t maxLineCharCount = 0;

		for (size_t sel = 0; sel < numSelections; ++sel)
		{
			size_t start = _pEditView->execute(SCI_GETSELECTIONNSTART, sel);
			size_t end = _pEditView->execute(SCI_GETSELECTIONNEND, sel);
			size_t lineCharCount = _pEditView->execute(SCI_COUNTCHARACTERS, start, end);

			if (sel == 0)
			{
				maxLineCharCount = lineCharCount;
			}
			else 
			{
				if (lineCharCount != maxLineCharCount)
				{
					sameCharCountOnEveryLine = false;
					if (lineCharCount > maxLineCharCount)
					{
						maxLineCharCount = lineCharCount;
					}
				}
			}
		}

		wsprintf(strSel, TEXT("Sel : %sx%s %s %s"),
			commafyInt(numSelections).c_str(),  // lines (rows) in rectangular selection
			commafyInt(maxLineCharCount).c_str(),  // show maximum width for columns
			sameCharCountOnEveryLine ? TEXT("=") : TEXT("->"),
			commafyInt(rectSelCharsAndLines.first).c_str());
	}
	else  // multiple stream selections
	{
		const int maxSelsToProcessLineCount = 99;  // limit the number of selections to process, for performance reasons
		const std::pair<size_t, size_t> multipleSelCharsAndLines = _pEditView->getSelectedCharsAndLinesCount(maxSelsToProcessLineCount);

		wsprintf(strSel, TEXT("Sel %s : %s | %s"),
			commafyInt(numSelections).c_str(),
			commafyInt(multipleSelCharsAndLines.first).c_str(),
			numSelections <= maxSelsToProcessLineCount ?
				commafyInt(multipleSelCharsAndLines.second).c_str() :
				TEXT("..."));  // show ellipsis for line count if too many selections are active
	}

	TCHAR strLnColSel[128];
	intptr_t curLN = _pEditView->getCurrentLineNumber();
	intptr_t curCN = _pEditView->getCurrentColumnNumber();
	wsprintf(strLnColSel, TEXT("Ln : %s    Col : %s    %s"),
		commafyInt(curLN + 1).c_str(),
		commafyInt(curCN + 1).c_str(),
		strSel);
	_statusBar.setText(strLnColSel, STATUSBAR_CUR_POS);

	_statusBar.setText(_pEditView->execute(SCI_GETOVERTYPE) ? TEXT("OVR") : TEXT("INS"), STATUSBAR_TYPING_MODE);
}

void Notepad_plus::dropFiles(HDROP hdrop)
{
	if (hdrop)
	{
		// Determinate in which view the file(s) is (are) dropped
		POINT p;
		::DragQueryPoint(hdrop, &p);
		HWND hWin = ::ChildWindowFromPointEx(_pPublicInterface->getHSelf(), p, CWP_SKIPINVISIBLE);
		if (!hWin) return;

		if ((_subEditView.getHSelf() == hWin) || (_subDocTab.getHSelf() == hWin))
			switchEditViewTo(SUB_VIEW);
		else
			switchEditViewTo(MAIN_VIEW);

		int filesDropped = ::DragQueryFile(hdrop, 0xffffffff, NULL, 0);

		vector<generic_string> folderPaths;
		vector<generic_string> filePaths;
		for (int i = 0; i < filesDropped; ++i)
		{
			TCHAR pathDropped[MAX_PATH];
			::DragQueryFile(hdrop, i, pathDropped, MAX_PATH);
			if (::PathIsDirectory(pathDropped))
			{
				size_t len = lstrlen(pathDropped);
				if (len > 0 && pathDropped[len - 1] != TCHAR('\\'))
				{
					pathDropped[len] = TCHAR('\\');
					pathDropped[len + 1] = TCHAR('\0');
				}
				folderPaths.push_back(pathDropped);
			}
			else
			{
				filePaths.push_back(pathDropped);
			}
		}
		
		NppParameters& nppParam = NppParameters::getInstance();
		bool isOldMode = nppParam.getNppGUI()._isFolderDroppedOpenFiles;

		if (isOldMode || folderPaths.size() == 0) // old mode or new mode + only files
		{
			BufferID lastOpened = BUFFER_INVALID;
			for (int i = 0; i < filesDropped; ++i)
			{
				TCHAR pathDropped[MAX_PATH];
				::DragQueryFile(hdrop, i, pathDropped, MAX_PATH);
				BufferID test = doOpen(pathDropped);
				if (test != BUFFER_INVALID)
					lastOpened = test;
			}

			if (lastOpened != BUFFER_INVALID)
			{
				switchToFile(lastOpened);
			}
		}
		else if (!isOldMode && (folderPaths.size() != 0 && filePaths.size() != 0)) // new mode && both folders & files
		{
			// display error & do nothing
			_nativeLangSpeaker.messageBox("DroppingFolderAsProjectModeWarning",
				_pPublicInterface->getHSelf(),
				TEXT("You can only drop files or folders but not both, because you're in dropping Folder as Project mode.\ryou have to enable \"Open all files of folder instead of launching Folder as Workspace on folder dropping\" in \"Default Directory\" section of Preferences dialog to make this operation work."),
				TEXT("Invalid action"),
				MB_OK | MB_APPLMODAL);
		}
		else if (!isOldMode && (folderPaths.size() != 0 && filePaths.size() == 0)) // new mode && only folders
		{
			// process new mode
			generic_string emptyStr;
			launchFileBrowser(folderPaths, emptyStr);
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

void Notepad_plus::checkModifiedDocument(bool bCheckOnlyCurrentBuffer)
{
	//this will trigger buffer updates. If the status changes, Notepad++ will be informed and can do its magic
	MainFileManager.checkFilesystemChanges(bCheckOnlyCurrentBuffer);
}

void Notepad_plus::getMainClientRect(RECT &rc) const
{
    _pPublicInterface->getClientRect(rc);
	rc.top += _rebarTop.getHeight();
	rc.bottom -= rc.top + _rebarBottom.getHeight() + _statusBar.getHeight();
}

void Notepad_plus::showView(int whichOne)
{
	if (viewVisible(whichOne))	//no use making visible view visible
		return;

	if (_mainWindowStatus & WindowUserActive)
	{
		 _pMainSplitter->setWin0(&_subSplitter);
		 _pMainWindow = _pMainSplitter;
	}
	else
	{
		_pMainWindow = &_subSplitter;
	}

	if (whichOne == MAIN_VIEW)
	{
		_mainEditView.display(true);
		_mainDocTab.display(true);
	}
	else if (whichOne == SUB_VIEW)
	{
		_subEditView.display(true);
		_subDocTab.display(true);
	}
	_pMainWindow->display(true);

	_mainWindowStatus |= (whichOne==MAIN_VIEW)?WindowMainActive:WindowSubActive;

	//Send sizing info to make windows fit
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
}

bool Notepad_plus::viewVisible(int whichOne)
{
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
	else
	{
		// otherwise the main window is the spltter container that we just created
		_pMainWindow = windowToSet;
	}

	_subSplitter.display(false);	//hide splitter
	//hide scintilla and doctab
	if (whichOne == MAIN_VIEW)
	{
		_mainEditView.display(false);
		_mainDocTab.display(false);
	}
	else if (whichOne == SUB_VIEW)
	{
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
	NppParameters& nppParam = NppParameters::getInstance();
	return nppParam.reloadStylers();
}

bool Notepad_plus::canHideView(int whichOne)
{
	if (!viewVisible(whichOne))
		return false;	//cannot hide hidden view
	if (!bothActive())
		return false;	//cannot hide only window
	DocTabView * tabToCheck = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	Buffer * buf = MainFileManager.getBufferByID(tabToCheck->getBufferByIndex(0));
	bool canHide = ((tabToCheck->nbItem() == 1) && !buf->isDirty() && buf->isUntitled());
	return canHide;
}

bool Notepad_plus::isEmpty()
{
	if (bothActive()) return false;

	DocTabView * tabToCheck = (_mainWindowStatus & WindowMainActive) ? &_mainDocTab : &_subDocTab;
	
	Buffer * buf = MainFileManager.getBufferByID(tabToCheck->getBufferByIndex(0));
	bool isEmpty = ((tabToCheck->nbItem() == 1) && !buf->isDirty() && buf->isUntitled());
	return isEmpty;
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
		Buffer * buf = MainFileManager.getBufferByID(idToClose);
		if (buf->isDirty() || !buf->isUntitled())
		{
			idToClose = BUFFER_INVALID;
		}
	}

	MainFileManager.addBufferReference(id, viewToOpen);

	//close clean doc. Use special logic to prevent flicker of tab showing then hiding
	if (idToClose != BUFFER_INVALID)
	{
		tabToOpen->setBuffer(0, id);	//index 0 since only one open
		activateBuffer(id, whichOne);	//activate. DocTab already activated but not a problem
		MainFileManager.closeBuffer(idToClose, viewToOpen);	//delete the buffer
		if (_pDocumentListPanel)
			_pDocumentListPanel->closeItem(idToClose, whichOne);
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

	Buffer * buf = MainFileManager.getBufferByID(id);

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
			BufferID newID = MainFileManager.newEmptyDocument();
			MainFileManager.addBufferReference(newID, viewToClose);
			tabToClose->setBuffer(0, newID);        //can safely use id 0, last (only) tab open
			activateBuffer(newID, whichOne);        //activate. DocTab already activated but not a problem
		}
		else
		{
			int toActivate = 0;
			//activate next doc, otherwise prev if not possible
			if (size_t(active) == tabToClose->nbItem() - 1) //prev
			{
				toActivate = active - 1;
			}
			else
			{
				toActivate = active;    //activate the 'active' index. Since we remove the tab first, the indices shift (on the right side)
			}

			if (NppParameters::getInstance().getNppGUI()._styleMRU)
			{
				// After closing a file choose the file to activate based on MRU list and not just last file in the list.
				TaskListInfo taskListInfo;
				::SendMessage(_pPublicInterface->getHSelf(), WM_GETTASKLISTINFO, reinterpret_cast<WPARAM>(&taskListInfo), 0);
				size_t i, n = taskListInfo._tlfsLst.size();
				for (i = 0; i < n; i++)
				{
					TaskLstFnStatus& tfs = taskListInfo._tlfsLst[i];
					if (tfs._iView != whichOne || tfs._bufID == id)
						continue;
					toActivate = tfs._docIndex >= active ? tfs._docIndex - 1 : tfs._docIndex;
					break;
				}
			}

			tabToClose->deletItemAt((size_t)index); //delete first
			_isFolding = true; // So we can ignore events while folding is taking place
			activateBuffer(tabToClose->getBufferByIndex(toActivate), whichOne);     //then activate. The prevent jumpy tab behaviour
			_isFolding = false;
		}
	}
	else
	{
		tabToClose->deletItemAt((size_t)index);
	}

	MainFileManager.closeBuffer(id, viewToClose);
	return true;
}

int Notepad_plus::switchEditViewTo(int gid)
{
	if (currentView() == gid)
	{
		//make sure focus is ok, then leave
		_pEditView->getFocus();	//set the focus
		return gid;
	}

	if (!viewVisible(gid))
		return currentView();	//cannot activate invisible view

	int oldView = currentView();
	int newView = otherView();

	_activeView = newView;
	//Good old switcheroo
	std::swap(_pDocTab, _pNonDocTab);
	std::swap(_pEditView, _pNonEditView);

	_pEditView->beSwitched();
    _pEditView->getFocus();	//set the focus

	if (_pDocMap)
	{
		_pDocMap->initWrapMap();
	}

	if (NppParameters::getInstance().getNppGUI().isSnapshotMode())
	{
		// Before switching off, synchronize backup file
		MainFileManager.backupCurrentBuffer();
	}

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
		int splitterSizeDyn = NppParameters::getInstance()._dpiManager.scaleX(splitterSize);
        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), splitterSizeDyn, SplitterMode::RIGHT_FIX, 45);
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
	Buffer * buf = MainFileManager.getBufferByID(bufferID);
	if (buf->isUntitled() || buf->isDirty())
		return;

	TCHAR nppName[MAX_PATH];
	::GetModuleFileName(NULL, nppName, MAX_PATH);
	generic_string command = TEXT("\"");
	command += nppName;
	command += TEXT("\"");
	command += TEXT(" \"$(FULL_CURRENT_PATH)\" -multiInst -nosession");

	if (x)
	{
		TCHAR pX[10];
		generic_itoa(x, pX, 10);
		command += TEXT(" -x");
		command += pX;
	}

	if (y)
	{
		TCHAR pY[10];
		generic_itoa(y, pY, 10);
		command += TEXT(" -y");
		command += pY;
	}

	LangType lt = buf->getLangType();

	// In case of UDL, "-lLANG" argument part is ignored.
	// We let new instance detect the user lang type via file extension -
	// it works in the most of case, except if user applies an UDL manually. 
	// For example,  this workaround won't work under the following situation:
	// user applies Markdown to a file named "myMarkdown.abc".
	if (lt != L_USER)
	{
		command += TEXT(" -l");
		command += ScintillaEditView::_langNameInfoArray[lt]._langName;
	}
	command += TEXT(" -n");
	command += to_wstring(_pEditView->getCurrentLineNumber() + 1);
	command += TEXT(" -c");
	command += to_wstring(_pEditView->getCurrentColumnNumber() + 1);

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
		// If both the views are visible then first save the position of non-edit view
		// So that moving document between views does not lose caret position
		// How it works =>
		//		non-edit view becomes edit view as document from edit view is sent to non edit view
		//		restoreCurrentPos is called on non-edit view, which will restore the position of
		//		active document/tab on non-edit view  (whatever position we set in below if condition)
		if (_pEditView->isVisible() && _pNonEditView->isVisible())
		{
			_pNonEditView->saveCurrentPos();
		}

		loadBufferIntoView(current, viewToGo);
		Buffer *buf = MainFileManager.getBufferByID(current);
		_pEditView->saveCurrentPos();	//allow copying of position
		buf->setPosition(buf->getPosition(_pEditView), _pNonEditView);
		_pNonEditView->restoreCurrentPosPreStep();	//set position
		activateBuffer(current, viewToGo);
	}

	//Open the view if it was hidden
	int viewToOpen = (viewToGo == SUB_VIEW?WindowSubActive:WindowMainActive);
	if (!(_mainWindowStatus & viewToOpen))
	{
		showView(viewToGo);
	}

	bool monitoringWasOn = false;

	//Close the document if we transfered the document instead of cloning it
	if (mode == TransferMove)
	{
		Buffer *buf = MainFileManager.getBufferByID(current);
		monitoringWasOn = buf->isMonitoringOn();

		//just close the activate document, since thats the one we moved (no search)
		doClose(_pEditView->getCurrentBufferID(), currentView());
	} // else it was cone, so leave it

	//Activate the other view since thats where the document went
	switchEditViewTo(viewToGo);

	if (monitoringWasOn)
	{
		command(IDM_VIEW_MONITORING);
	}
}

bool Notepad_plus::activateBuffer(BufferID id, int whichOne, bool forceApplyHilite)
{
	NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	bool isSnapshotMode = nppGui.isSnapshotMode();
	if (isSnapshotMode)
	{
		// Before switching off, synchronize backup file
		MainFileManager.backupCurrentBuffer();
	}
	Buffer * pBuf = MainFileManager.getBufferByID(id);
	bool reload = pBuf->getNeedReload();
	if (reload)
	{
		MainFileManager.reloadBuffer(id);
		pBuf->setNeedReload(false);
	}
	if (whichOne == MAIN_VIEW)
	{
		if (_mainDocTab.activateBuffer(id))	//only activate if possible
		{
			_isFolding = true;
			_mainEditView.activateBuffer(id, forceApplyHilite);
			_isFolding = false;
		}
		else
			return false;
	}
	else
	{
		if (_subDocTab.activateBuffer(id))
		{
			_isFolding = true;
			_subEditView.activateBuffer(id, forceApplyHilite);
			_isFolding = false;
		}
		else
			return false;
	}

	if (reload)
	{
		performPostReload(whichOne);
	}

	notifyBufferActivated(id, whichOne);

	bool isCurrBuffDetection = (nppGui._fileAutoDetection & cdEnabledNew) ? true : false;
	if (!reload && isCurrBuffDetection)
	{
		// Buffer has been activated, now check for file modification
		// If enabled for current buffer
		pBuf->checkFileState();
	}
	return true;
}

void Notepad_plus::performPostReload(int whichOne)
{
	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParam.getNppGUI();
	bool toEnd = (nppGUI._fileAutoDetection & cdGo2end) ? true : false;
	if (!toEnd)
		return;
	if (whichOne == MAIN_VIEW)
	{
		_mainEditView.setPositionRestoreNeeded(false);
		_mainEditView.execute(SCI_DOCUMENTEND);
	}
	else
	{
		_subEditView.setPositionRestoreNeeded(false);
		_subEditView.execute(SCI_DOCUMENTEND);
	}
}

void Notepad_plus::bookmarkNext(bool forwardScan)
{
	size_t lineno = _pEditView->getCurrentLineNumber();
	int sci_marker = SCI_MARKERNEXT;
	size_t lineStart = lineno + 1;	//Scan starting from next line
	intptr_t lineRetry = 0;				//If not found, try from the beginning
	if (!forwardScan)
    {
		lineStart = lineno - 1;		//Scan starting from previous line
		lineRetry = _pEditView->execute(SCI_GETLINECOUNT);	//If not found, try from the end
		sci_marker = SCI_MARKERPREVIOUS;
	}
	intptr_t nextLine = _pEditView->execute(sci_marker, lineStart, 1 << MARK_BOOKMARK);
	if (nextLine < 0)
		nextLine = _pEditView->execute(sci_marker, lineRetry, 1 << MARK_BOOKMARK);

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
	enableConvertMenuItems(_pEditView->getCurrentBuffer()->getEolFormat());
	checkUnicodeMenuItems();
}


void Notepad_plus::enableConvertMenuItems(EolType format) const
{
	enableCommand(IDM_FORMAT_TODOS, (format != EolType::windows), MENU);
	enableCommand(IDM_FORMAT_TOUNIX, (format != EolType::unix), MENU);
	enableCommand(IDM_FORMAT_TOMAC, (format != EolType::macos), MENU);
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
		case uni16BE   : id = IDM_FORMAT_UTF_16BE; break;
		case uni16LE   : id = IDM_FORMAT_UTF_16LE; break;
		case uniCookie : id = IDM_FORMAT_AS_UTF_8; break;
		case uni8Bit   : id = IDM_FORMAT_ANSI; break;
	}

	if (encoding == -1)
	{
		// Uncheck all in the sub encoding menu
        HMENU _formatMenuHandle = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FORMAT);
        doCheck(_formatMenuHandle, -1);

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
		EncodingMapper& em = EncodingMapper::getInstance();
		int cmdID = em.getIndexFromEncoding(encoding);
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
        HMENU _formatMenuHandle = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FORMAT);
        doCheck(_formatMenuHandle, cmdID);
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
	const size_t extractedLen = 128;
	TCHAR extracted[extractedLen] = {'\0'};

	for (size_t i = 0, j = 0, len = lstrlen(str2extract) ; i < len && j < extractedLen - 1; ++i)
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
	Buffer * buf = _pEditView->getCurrentBuffer();
	// Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly())
		return false;

	//-- BlockToStreamComment:
	const TCHAR *commentStart;
	const TCHAR *commentEnd;
	generic_string symbolStart;
	generic_string symbolEnd;

	const TCHAR *commentLineSymbol;
	generic_string symbol;

	//Single Line Comment/Uncomment/Toggle can have two modes:
	// * a NORMAL MODE which uses a commentLineSymbol to comment/uncomment code per line, and
	// * an ADVANCED MODE which uses commentStart and commentEnd symbols to comment/uncomment code per line.
	//The NORMAL MODE is used for all Lexers which have a commentLineSymbol defined.
	//The ADVANCED MODE is only available for Lexers which do not have a commentLineSymbol but commentStreamSymbols (start/end) defined.
	//The ADVANCED MODE behaves the same way as the NORMAL MODE (comment/uncomment every single line in the selection range separately)
	//but uses two symbols to accomplish this.
	bool isSingleLineAdvancedMode = false;

	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance().getULCFromName(buf->getUserDefineLangName());
		if (!userLangContainer)
			return false;

		symbol = extractSymbol('0', '0', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentLineSymbol = symbol.c_str();
		// BlockToStreamComment: Needed to decide, if stream-comment can be called below!
		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentLineSymbol = buf->getCommentLineSymbol();
		// BlockToStreamComment: Needed to decide, if stream-comment can be called below!
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	if ((!commentLineSymbol) || (!commentLineSymbol[0]) || (commentLineSymbol == NULL))
	{
	// BlockToStreamComment: If there is no block-comment symbol, try the stream comment:
		if (!(!commentStart || !commentStart[0] || commentStart == NULL || !commentEnd || !commentEnd[0] || commentEnd == NULL))
		{
			if (currCommentMode == cm_comment)
			{
				//Do an advanced "Single Line Comment" by using stream-comment symbols (start/end) per line in this case.
				isSingleLineAdvancedMode = true;
				//return doStreamComment(); //Use "Block Comment" for this.
			}
			else if (currCommentMode == cm_uncomment)
			{
				//"undoStreamComment()" can be more flexible than "isSingleLineAdvancedMode = true", 
				//since it can uncomment more embedded levels at once and the commentEnd symbol can be located everywhere. 
				//But, depending on the selection start/end position, the first/last selected line may not be uncommented properly!
				return undoStreamComment(false);
				//isSingleLineAdvancedMode = true;
			}
			else if (currCommentMode == cm_toggle)
			{
				//Do an advanced "Toggle Single Line Comment" by using stream-comment symbols (start/end) per line in this case.
				isSingleLineAdvancedMode = true;
			}
			else
				return false;
		}
		else
			return false;
	}

	//For Single Line NORMAL MODE
	generic_string comment;
	size_t comment_length = 0;

	//For Single Line ADVANCED MODE
	generic_string advCommentStart;
	generic_string advCommentEnd;
	size_t advCommentStart_length = 0;
	size_t advCommentEnd_length = 0;

	const TCHAR aSpace[] { TEXT(" ") };

	//Only values that have passed through will be assigned, to be sure they are valid!
	if (!isSingleLineAdvancedMode)
	{
		comment = commentLineSymbol;

		if (!(buf->getLangType() == L_BAANC)) // BaanC standardization - no space.
			comment += aSpace;

		comment_length = comment.length();
	}
	else // isSingleLineAdvancedMode
	{
		advCommentStart = commentStart;
		advCommentStart += aSpace;
		advCommentEnd = aSpace;
		advCommentEnd += commentEnd;

		advCommentStart_length = advCommentStart.length();
		advCommentEnd_length = advCommentEnd.length();
	}

    size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
    size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
    size_t caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
    // checking if caret is located in _beginning_ of selected block
    bool move_caret = caretPosition < selectionEnd;
	intptr_t selStartLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
	intptr_t selEndLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionEnd);
	intptr_t lines = selEndLine - selStartLine;
    // "caret return" is part of the last selected line
    if ((lines > 0) && (selectionEnd == static_cast<size_t>(_pEditView->execute(SCI_POSITIONFROMLINE, selEndLine))))
		selEndLine--;
	// count lines which were un-commented to decide if undoStreamComment() shall be called.
	int nUncomments = 0;
	//Some Lexers need line-comments at the beginning of a line.
	const bool avoidIndent = (buf->getLangType() == L_FORTRAN_77 || buf->getLangType() == L_BAANC);
	//Some Lexers comment blank lines, per their standards.
	const bool commentEmptyLines = (buf->getLangType() == L_BAANC);

    _pEditView->execute(SCI_BEGINUNDOACTION);

    for (intptr_t i = selStartLine; i <= selEndLine; ++i)
	{
		size_t lineStart = _pEditView->execute(SCI_POSITIONFROMLINE, i);
		size_t lineIndent = _pEditView->execute(SCI_GETLINEINDENTPOSITION, i);
		size_t lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, i);

		// empty lines are not commented, unless required by the language.
		if (lineIndent == lineEnd && !commentEmptyLines)
			continue;

		if (avoidIndent)
			lineIndent = lineStart;

		size_t linebufferSize = lineEnd - lineIndent + 1;
		TCHAR* linebuf = new TCHAR[linebufferSize];

		_pEditView->getGenericText(linebuf, linebufferSize, lineIndent, lineEnd);

        generic_string linebufStr = linebuf;
		delete [] linebuf;

   		if (currCommentMode != cm_comment) // uncomment/toggle
		{
			if (!isSingleLineAdvancedMode)
			{
				// In order to do get case insensitive comparison use strnicmp() instead case-sensitive comparison.
				//      Case insensitive comparison is needed e.g. for "REM" and "rem" in Batchfiles.
				if (generic_strnicmp(linebufStr.c_str(), comment.c_str(), !(buf->getLangType() == L_BAANC) ? comment_length - 1 : comment_length) == 0)
				{
					size_t len = linebufStr[comment_length - 1] == aSpace[0] ? comment_length : !(buf->getLangType() == L_BAANC) ? comment_length - 1 : comment_length;

					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + len);
					_pEditView->replaceSelWith("");

					// SELECTION RANGE ADJUSTMENT .......................
					if (i == selStartLine) // first selected line
					{
						if (selectionStart > lineIndent + len)
							selectionStart -= len;
						else if (selectionStart > lineIndent)
							selectionStart = lineIndent;
					} // ................................................
					if (i == selEndLine) // last selected line
					{
						if (selectionEnd > lineIndent + len)
							selectionEnd -= len;
						else if (selectionEnd > lineIndent)
						{
							selectionEnd = lineIndent;
							if (lineIndent == lineStart && i != selStartLine)
								++selectionEnd; // avoid caret return in this case
						}
					} // ................................................
					else // every iteration except the last selected line
						selectionEnd -= len;
					// ..................................................

					++nUncomments;
					continue;
				}
			}
			else // isSingleLineAdvancedMode
			{
				if ((generic_strnicmp(linebufStr.c_str(), advCommentStart.c_str(), advCommentStart_length - 1) == 0) &&
					(generic_strnicmp(linebufStr.substr(linebufStr.length() - advCommentEnd_length + 1, advCommentEnd_length - 1).c_str(), advCommentEnd.substr(1, advCommentEnd_length - 1).c_str(), advCommentEnd_length - 1) == 0))
				{
					size_t startLen = linebufStr[advCommentStart_length - 1] == aSpace[0] ? advCommentStart_length : advCommentStart_length - 1;
					size_t endLen = linebufStr[linebufStr.length() - advCommentEnd_length] == aSpace[0] ? advCommentEnd_length : advCommentEnd_length - 1;

					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + startLen);
					_pEditView->replaceSelWith("");
					_pEditView->execute(SCI_SETSEL, lineEnd - startLen - endLen, lineEnd - startLen);
					_pEditView->replaceSelWith("");

					// SELECTION RANGE ADJUSTMENT .......................
					if (i == selStartLine) // first selected line
					{
						if (selectionStart > lineEnd - endLen)
							selectionStart = lineEnd - startLen - endLen;
						else if (selectionStart > lineIndent + startLen)
							selectionStart -= startLen;
						else if (selectionStart > lineIndent)
							selectionStart = lineIndent;
					} // ................................................
					if (i == selEndLine) // last selected line
					{
						if (selectionEnd > lineEnd)
							selectionEnd -= (startLen + endLen);
						else if (selectionEnd > lineEnd - endLen)
							selectionEnd = lineEnd - startLen - endLen;
						else if (selectionEnd > lineIndent + startLen)
							selectionEnd -= startLen;
						else if (selectionEnd > lineIndent)
						{
							selectionEnd = lineIndent;
							if (lineIndent == lineStart && i != selStartLine)
								++selectionEnd; // avoid caret return in this case
						}
					} // ................................................
					else // every iteration except the last selected line
						selectionEnd -= (startLen + endLen);
					// ..................................................

					++nUncomments;
					continue;
				}
			}
		} // uncomment/toggle

		if (currCommentMode != cm_uncomment) // comment/toggle
		{
			if (!isSingleLineAdvancedMode)
			{
				_pEditView->insertGenericTextFrom(lineIndent, comment.c_str());

				// SELECTION RANGE ADJUSTMENT .......................
				if (i == selStartLine) // first selected line
				{
					if (selectionStart >= lineIndent)
						selectionStart += comment_length;
				} // ................................................
				if (i == selEndLine) // last selected line
				{
					if (selectionEnd >= lineIndent)
						selectionEnd += comment_length;
				} // ................................................
				else // every iteration except the last selected line
					selectionEnd += comment_length;
				// ..................................................
			}
			else // isSingleLineAdvancedMode
			{
				_pEditView->insertGenericTextFrom(lineIndent, advCommentStart.c_str());
				_pEditView->insertGenericTextFrom(lineEnd + advCommentStart_length, advCommentEnd.c_str());

				// SELECTION RANGE ADJUSTMENT .......................
				if (i == selStartLine) // first selected line
				{
					if (selectionStart >= lineIndent)
						selectionStart += advCommentStart_length;
				} // ................................................
				if (i == selEndLine) // last selected line
				{
					if (selectionEnd > lineEnd)
						selectionEnd += (advCommentStart_length + advCommentEnd_length);
					else if (selectionEnd >= lineIndent)
						selectionEnd += advCommentStart_length;
				} // ................................................
				else // every iteration except the last selected line
					selectionEnd += (advCommentStart_length + advCommentEnd_length);
				// ..................................................
			}
		} // comment/toggle
	} // for (...)

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

	// undoStreamComment: If there were no block-comments to un-comment try uncommenting of stream-comment.
	if ((currCommentMode == cm_uncomment) && (nUncomments == 0))
	{
		return undoStreamComment(false);
	}
    return true;
}

bool Notepad_plus::doStreamComment()
{
	const TCHAR *commentStart;
	const TCHAR *commentEnd;

	generic_string symbolStart;
	generic_string symbolEnd;

	// BlockToStreamComment:
	const TCHAR *commentLineSymbol;
	generic_string symbol;

	Buffer * buf = _pEditView->getCurrentBuffer();
	// Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly())
		return false;

	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance().getULCFromName(buf->getUserDefineLangName());

		if (!userLangContainer)
			return false;

		// BlockToStreamComment: Next two lines needed to decide, if block-comment can be called below!
		symbol = extractSymbol('0', '0', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentLineSymbol = symbol.c_str();

		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		// BlockToStreamComment: Next line needed to decide, if block-comment can be called below!
		commentLineSymbol = buf->getCommentLineSymbol();
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	// BlockToStreamComment: If there is no stream-comment symbol, try the block comment:
	if ((!commentStart) || (!commentStart[0]) || (commentStart == NULL) || (!commentEnd) || (!commentEnd[0]) || (commentEnd == NULL))
	{
		if (!(!commentLineSymbol || !commentLineSymbol[0] || commentLineSymbol == NULL))
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
		auto selLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
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

void Notepad_plus::saveScintillasZoom()
{
	NppParameters& nppParam = NppParameters::getInstance();
	ScintillaViewParams & svp = (ScintillaViewParams &)nppParam.getSVP();
	svp._zoom = _mainEditView.execute(SCI_GETZOOM);
	svp._zoom2 = _subEditView.execute(SCI_GETZOOM);
}

bool Notepad_plus::addCurrentMacro()
{
	NppParameters& nppParams = NppParameters::getInstance();
	vector<MacroShortcut> & theMacros = nppParams.getMacroList();

	int nbMacro = static_cast<int32_t>(theMacros.size());

	int cmdID = ID_MACRO + nbMacro;
	MacroShortcut ms(Shortcut(), _macro, cmdID);
	ms.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());

	if (ms.doDialog() != -1)
	{
		HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
		int const posBase = 6;	//separator at index 5
		if (nbMacro == 0)
		{
			::InsertMenu(hMacroMenu, posBase-1, MF_BYPOSITION, static_cast<UINT>(-1), 0);	//no separator yet, add one

            // Insert the separator and modify/delete command
			::InsertMenu(hMacroMenu, posBase + nbMacro + 1, MF_BYPOSITION, static_cast<UINT>(-1), 0);

			NativeLangSpeaker *pNativeLangSpeaker = nppParams.getNativeLangSpeaker();
			generic_string nativeLangShortcutMapperMacro = pNativeLangSpeaker->getNativeLangMenuString(IDM_SETTING_SHORTCUT_MAPPER_MACRO);
			if (nativeLangShortcutMapperMacro == TEXT(""))
				nativeLangShortcutMapperMacro = TEXT("Modify Shortcut/Delete Macro...");

			::InsertMenu(hMacroMenu, posBase + nbMacro + 2, MF_BYCOMMAND, IDM_SETTING_SHORTCUT_MAPPER_MACRO, nativeLangShortcutMapperMacro.c_str());
        }
		theMacros.push_back(ms);
		::InsertMenu(hMacroMenu, posBase + nbMacro, MF_BYPOSITION, cmdID, ms.toMenuItemString().c_str());
		_accelerator.updateShortcuts();
		nppParams.setShortcutDirty();
		return true;
	}
	return false;
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
	int currentNbDoc = static_cast<int32_t>(_pDocTab->nbItem());
	int nonCurrentNbDoc = static_cast<int32_t>(_pNonDocTab->nbItem());

	tli->_currentIndex = 0;

	if (!viewVisible(otherView()))
		nonCurrentNbDoc = 0;

	for (int i = 0 ; i < currentNbDoc ; ++i)
	{
		BufferID bufID = _pDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager.getBufferByID(bufID);
		int status = b->isMonitoringOn()?tb_monitored:(b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved));
		tli->_tlfsLst.push_back(TaskLstFnStatus(currentView(), i, b->getFullPathName(), status, (void *)bufID));
	}
	for (int i = 0 ; i < nonCurrentNbDoc ; ++i)
	{
		BufferID bufID = _pNonDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager.getBufferByID(bufID);
		int status = b->isMonitoringOn()?tb_monitored:(b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved));
		tli->_tlfsLst.push_back(TaskLstFnStatus(otherView(), i, b->getFullPathName(), status, (void *)bufID));
	}
}


bool Notepad_plus::goToPreviousIndicator(int indicID2Search, bool isWrap) const
{
    auto position = _pEditView->execute(SCI_GETCURRENTPOS);
	auto docLen = _pEditView->getCurrentDocLen();

    bool isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  position) != 0;
    size_t posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  position);
    size_t posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search,  position);

	// pre-condition
	if ((posStart == 0) && (posEnd == docLen - 1))
		return false;

    if (posStart <= 0)
	{
		if (!isWrap)
			return false;

		isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  docLen - 1) != 0;
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

    auto newPos = posStart - 1;
    posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search, newPos);
    posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, newPos);

	// found
	if (_pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search, posStart))
	{
		NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
		nppGUI._disableSmartHiliteTmp = true;

        auto currentline = _pEditView->execute(SCI_LINEFROMPOSITION, posEnd);
	    _pEditView->execute(SCI_ENSUREVISIBLE, currentline);	// make sure target line is unfolded

		_pEditView->execute(SCI_SETSEL, posEnd, posStart);
		_pEditView->execute(SCI_SCROLLCARET);
		return true;
	}
	return false;
}

bool Notepad_plus::goToNextIndicator(int indicID2Search, bool isWrap) const
{
    size_t position = _pEditView->execute(SCI_GETCURRENTPOS);
	size_t docLen = _pEditView->getCurrentDocLen();

    bool isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  position) != 0;
    size_t posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search,  position);
    size_t posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search,  position);

	// pre-condition
	if ((posStart == 0) && (posEnd == docLen - 1))
		return false;

    if (posEnd >= docLen)
	{
		if (!isWrap)
			return false;

		isInIndicator = _pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search,  0) != 0;
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
    auto newPos = posEnd;
    posStart = _pEditView->execute(SCI_INDICATORSTART, indicID2Search, newPos);
    posEnd = _pEditView->execute(SCI_INDICATOREND, indicID2Search, newPos);

	// found
	if (_pEditView->execute(SCI_INDICATORVALUEAT, indicID2Search, posStart))
	{
		NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
		nppGUI._disableSmartHiliteTmp = true;

        auto currentline = _pEditView->execute(SCI_LINEFROMPOSITION, posEnd);
	    _pEditView->execute(SCI_ENSUREVISIBLE, currentline);	// make sure target line is unfolded

		_pEditView->execute(SCI_SETSEL, posStart, posEnd);
		_pEditView->execute(SCI_SCROLLCARET);
		return true;
	}
	return false;
}

void Notepad_plus::fullScreenToggle()
{
	if (!_beforeSpecialView._isFullScreen)	//toggle fullscreen on
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
		if (_beforeSpecialView._isPostIt)
        {
            bs |= buttonStatus_postit;
        }
        else
		{
			//only change the GUI if not already done by postit
			_beforeSpecialView._isMenuShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISMENUHIDDEN, 0, 0) != TRUE;
			if (_beforeSpecialView._isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, TRUE);

			//Hide rebar
			_rebarTop.display(false);
			_rebarBottom.display(false);
		}
        _restoreButton.setButtonStatus(bs);

		//Hide window so windows can properly update it
		::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);

		//Set popup style for fullscreen window and store the old style
		if (!_beforeSpecialView._isPostIt)
		{
			_beforeSpecialView._preStyle = ::SetWindowLongPtr(_pPublicInterface->getHSelf(), GWL_STYLE, WS_POPUP);
			if (!_beforeSpecialView._preStyle)
			{
				//something went wrong, use default settings
				_beforeSpecialView._preStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
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
		if (!_beforeSpecialView._isPostIt)
		{
			//only change the GUI if postit isnt active
			if (_beforeSpecialView._isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, FALSE);

			//Show rebar
			_rebarTop.display(true);
			_rebarBottom.display(true);
		}

		//Set old style if not fullscreen
		if (!_beforeSpecialView._isPostIt)
		{
			::SetWindowLongPtr( _pPublicInterface->getHSelf(), GWL_STYLE, _beforeSpecialView._preStyle);
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
	_beforeSpecialView._isFullScreen = !_beforeSpecialView._isFullScreen;
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
    if (_beforeSpecialView._isPostIt)
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
	if (!_beforeSpecialView._isPostIt)	// PostIt disabled, enable it
	{
		NppGUI & nppGUI = NppParameters::getInstance().getNppGUI();
		// get current status before switch to postIt
		//check these always
		{
			_beforeSpecialView._isAlwaysOnTop = ::GetMenuState(_mainMenuHandle, IDM_VIEW_ALWAYSONTOP, MF_BYCOMMAND) == MF_CHECKED;
			_beforeSpecialView._isTabbarShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISTABBARHIDDEN, 0, 0) != TRUE;
			_beforeSpecialView._isStatusbarShown = nppGUI._statusBarShow;
			if (nppGUI._statusBarShow)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDESTATUSBAR, 0, TRUE);
			if (_beforeSpecialView._isTabbarShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDETABBAR, 0, TRUE);
			if (!_beforeSpecialView._isAlwaysOnTop)
				::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);
		}
		//Only check these if not fullscreen
        int bs = buttonStatus_postit;
		if (_beforeSpecialView._isFullScreen)
        {
            bs |= buttonStatus_fullscreen;
        }
        else
		{
			_beforeSpecialView._isMenuShown = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_ISMENUHIDDEN, 0, 0) != TRUE;
			if (_beforeSpecialView._isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, TRUE);

			//Hide rebar
			_rebarTop.display(false);
			_rebarBottom.display(false);
		}
        _restoreButton.setButtonStatus(bs);

		// PostIt!

		//Set popup style for fullscreen window and store the old style
		if (!_beforeSpecialView._isFullScreen)
		{
			//Hide window so windows can properly update it
			::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);
			_beforeSpecialView._preStyle = ::SetWindowLongPtr( _pPublicInterface->getHSelf(), GWL_STYLE, WS_POPUP );
			if (!_beforeSpecialView._preStyle)
			{
				//something went wrong, use default settings
				_beforeSpecialView._preStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
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
		if (!_beforeSpecialView._isFullScreen)
		{
			//only change the these parts of GUI if not already done by fullscreen
			if (_beforeSpecialView._isMenuShown)
				::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDEMENU, 0, FALSE);

			//Show rebar
			_rebarTop.display(true);
			_rebarBottom.display(true);
		}
		//Do this GUI config always
		if (_beforeSpecialView._isStatusbarShown)
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDESTATUSBAR, 0, FALSE);
		if (_beforeSpecialView._isTabbarShown)
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_HIDETABBAR, 0, FALSE);
		if (!_beforeSpecialView._isAlwaysOnTop)
			::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);

		//restore window style if not fullscreen
		if (!_beforeSpecialView._isFullScreen)
		{
			//dwStyle |= (WS_CAPTION | WS_SIZEBOX);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_HIDE);
			::SetWindowLongPtr(_pPublicInterface->getHSelf(), GWL_STYLE, _beforeSpecialView._preStyle);

			//Redraw the window and refresh windowmanager cache, dont do anything else, sizing is done later on
			::SetWindowPos(_pPublicInterface->getHSelf(), HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_DRAWFRAME|SWP_FRAMECHANGED);
			::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
		}
	}

	_beforeSpecialView._isPostIt = !_beforeSpecialView._isPostIt;
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
}

// Distraction Free mode uses full screen mode + post-it mode + setting padding on the both left & right sides.
// In order to keep the coherence of data, when full screen mode or (and) post-it mode is (are) active,
// Distraction Free mode should be innaccible, and vice versa.
void Notepad_plus::distractionFreeToggle()
{
	// Toggle Distraction Free Mode
	fullScreenToggle();
	postItToggle();

	// Get padding info
	const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
	int paddingLeft = 0;
	int paddingRight = 0;
	

	// Enable or disable Distraction Free Mode
	if (_beforeSpecialView._isDistractionFree) // disable it
	{
		// restore another view if 2 views mode was on
		if (_beforeSpecialView._was2ViewModeOn)
		{
			showView(otherView());
			_beforeSpecialView._was2ViewModeOn = false;
		}

		// restore dockable panels
		for (auto i : _beforeSpecialView._pVisibleDockingContainers)
		{
			i->display();
		}
		_dockingManager.resize();

		// restore padding
		paddingLeft = svp._paddingLeft;
		paddingRight = svp._paddingRight;

		// hide restore button
		_restoreButton.setButtonStatus(0);
		_restoreButton.display(false);
	}
	else // enable it
	{
		// check if 2 views mode is on
		ScintillaEditView & nonFocusedView = (otherView() == MAIN_VIEW) ? _mainEditView : _subEditView;
		if (nonFocusedView.isVisible())
		{
			hideView(otherView());
			_beforeSpecialView._was2ViewModeOn = true;
		}
		else
		{
			_beforeSpecialView._was2ViewModeOn = false;
		}

		// check if any dockable panel is visible
		std::vector<DockingCont*> & container = _dockingManager.getContainerInfo();
		_beforeSpecialView._pVisibleDockingContainers.clear();
		for (auto i : container)
		{
			if (i->isVisible())
			{
				_beforeSpecialView._pVisibleDockingContainers.push_back(i);
			}
		}
		
		for (auto i : _beforeSpecialView._pVisibleDockingContainers)
		{
			i->display(false);
		}
		_dockingManager.resize();

		// set padding
		paddingLeft = paddingRight = svp.getDistractionFreePadding(_pEditView->getWidth());

		// set state of restore button (it's already shown by fullScreen & postIt toggling)
		_restoreButton.setButtonStatus(buttonStatus_distractionFree);
	}

	_beforeSpecialView._isDistractionFree = !_beforeSpecialView._isDistractionFree;

	// set Distraction Free Mode paddin or restore the normal padding
	_pEditView->execute(SCI_SETMARGINLEFT, 0, paddingLeft);
	_pEditView->execute(SCI_SETMARGINRIGHT, 0, paddingRight);
}

void Notepad_plus::doSynScorll(HWND whichView)
{
	intptr_t column = 0;
	intptr_t line = 0;
	ScintillaEditView *pView;

	// var for Line
	intptr_t mainCurrentLine, subCurrentLine;

	// var for Column
	intptr_t mxoffset, sxoffset;
	intptr_t pixel;
	intptr_t mainColumn, subColumn;

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
			pixel = _mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = _subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
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
			pixel = _mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = _subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
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
	DockingManagerData & dockingData = (DockingManagerData &)(NppParameters::getInstance()).getNppGUI()._dockingData;

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
		for (size_t i = 0, len = docTab[k]->nbItem(); i < len ; ++i)
		{
			BufferID bufID = docTab[k]->getBufferByIndex(i);
			ScintillaEditView *editView = k == 0 ? &_mainEditView : &_subEditView;
			size_t activeIndex = k == 0 ? session._activeMainIndex : session._activeSubIndex;
			vector<sessionFileInfo> *viewFiles = (vector<sessionFileInfo> *)(k == 0?&(session._mainViewFiles):&(session._subViewFiles));

			Buffer * buf = MainFileManager.getBufferByID(bufID);

			if (buf->isUntitled() && buf->docLength() == 0)
				continue;

			if (!includUntitledDoc)
				if (!PathFileExists(buf->getFullPathName()))
					continue;


			generic_string	languageName = getLangFromMenu(buf);
			const TCHAR *langName = languageName.c_str();
			sessionFileInfo sfi(buf->getFullPathName(), langName, buf->getEncoding(), buf->getUserReadOnly(), buf->getPosition(editView), buf->getBackupFileName().c_str(), buf->getLastModifiedTimestamp(), buf->getMapPosition());

			sfi._isMonitoring = buf->isMonitoringOn();

			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			size_t maxLine = static_cast<size_t>(_invisibleEditView.execute(SCI_GETLINECOUNT));

			for (size_t j = 0 ; j < maxLine ; ++j)
			{
				if ((_invisibleEditView.execute(SCI_MARKERGET, j) & (1 << MARK_BOOKMARK)) != 0)
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
bool Notepad_plus::emergency(const generic_string& emergencySavedDir)
{
    ::CreateDirectory(emergencySavedDir.c_str(), NULL);
	return dumpFiles(emergencySavedDir.c_str(), TEXT("File"));
}

bool Notepad_plus::dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix)
{
	//start dumping unsaved files to recovery directory
	bool somethingsaved = false;
	bool somedirty = false;
	TCHAR savePath[MAX_PATH] = { '\0' };

	//rescue primary
	for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
	{
		Buffer * docbuf = MainFileManager.getBufferByIndex(i);
		if (!docbuf->isDirty())	//skip saved documents
			continue;
		else
			somedirty = true;

		const TCHAR * unitext = (docbuf->getUnicodeMode() != uni8Bit)?TEXT("_utf8"):TEXT("");
		wsprintf(savePath, TEXT("%s\\%s%03d%s.dump"), outdir, fileprefix, static_cast<int>(i), unitext);

		SavingStatus res = MainFileManager.saveBuffer(docbuf->getID(), savePath);

		somethingsaved |= (res == SavingStatus::SaveOK);
	}

	return somethingsaved || !somedirty;
}

void Notepad_plus::drawTabbarColoursFromStylerArray()
{
	Style *stActText = getStyleFromName(TABBAR_ACTIVETEXT);
	if (stActText && static_cast<long>(stActText->_fgColor) != -1)
		TabBarPlus::setColour(stActText->_fgColor, TabBarPlus::activeText);

	Style *stActfocusTop = getStyleFromName(TABBAR_ACTIVEFOCUSEDINDCATOR);
	if (stActfocusTop && static_cast<long>(stActfocusTop->_fgColor) != -1)
		TabBarPlus::setColour(stActfocusTop->_fgColor, TabBarPlus::activeFocusedTop);

	Style *stActunfocusTop = getStyleFromName(TABBAR_ACTIVEUNFOCUSEDINDCATOR);
	if (stActunfocusTop && static_cast<long>(stActunfocusTop->_fgColor) != -1)
		TabBarPlus::setColour(stActunfocusTop->_fgColor, TabBarPlus::activeUnfocusedTop);

	Style *stInact = getStyleFromName(TABBAR_INACTIVETEXT);
	if (stInact && static_cast<long>(stInact->_fgColor) != -1)
		TabBarPlus::setColour(stInact->_fgColor, TabBarPlus::inactiveText);
	if (stInact && static_cast<long>(stInact->_bgColor) != -1)
		TabBarPlus::setColour(stInact->_bgColor, TabBarPlus::inactiveBg);
}

void Notepad_plus::drawAutocompleteColoursFromTheme(COLORREF fgColor, COLORREF bgColor)
{
	if (bgColor == 0xFFFFFF)
		return;

	int rbv = GetRValue(bgColor);
	int gbv = GetGValue(bgColor);
	int bbv = GetBValue(bgColor);

	int rfv = GetRValue(fgColor);
	int gfv = GetGValue(fgColor);
	int bfv = GetBValue(fgColor);

	COLORREF bgDarker = RGB(rbv - 20 <= 0 ? 0 : rbv - 20, gbv - 20 <= 0 ? 0 : gbv - 20, bbv - 20 <= 0 ? 0 : bbv - 20);

	if (bgColor == RGB(0, 0, 0)) // if the bg is pure black
		bgDarker = RGB(20, 20, 20); // make bgDarker lighter for distinguishing between both

	COLORREF fgDarker = RGB(rfv - 20 <= 0 ? 0 : rfv - 20, gfv - 20 <= 0 ? 0 : gfv - 20, bfv - 20 <= 0 ? 0 : bfv - 20);
	COLORREF fgLigher = RGB(rfv + 20 >= 255 ? 255 : rfv + 20, gfv + 20 >= 255 ? 255 : gfv + 20, bfv + 20 >= 255 ? 255 : bfv + 20);

	AutoCompletion::setColour(bgDarker, AutoCompletion::AutocompleteColorIndex::autocompleteBg);
	AutoCompletion::setColour(bgColor, AutoCompletion::AutocompleteColorIndex::selectedBg);
	AutoCompletion::setColour(fgDarker, AutoCompletion::AutocompleteColorIndex::autocompleteText);
	AutoCompletion::setColour(fgColor, AutoCompletion::AutocompleteColorIndex::selectedText);

	AutoCompletion::setColour(bgDarker, AutoCompletion::AutocompleteColorIndex::calltipBg);
	AutoCompletion::setColour(fgDarker, AutoCompletion::AutocompleteColorIndex::calltipText);
	AutoCompletion::setColour(fgLigher, AutoCompletion::AutocompleteColorIndex::calltipHighlight);
	
}

void Notepad_plus::drawDocumentMapColoursFromStylerArray()
{
	Style* docMap = getStyleFromName(VIEWZONE_DOCUMENTMAP);
	if (docMap && static_cast<long>(docMap->_fgColor) != -1)
		ViewZoneDlg::setColour(docMap->_fgColor, ViewZoneDlg::ViewZoneColorIndex::focus);
	if (docMap && static_cast<long>(docMap->_bgColor) != -1)
		ViewZoneDlg::setColour(docMap->_bgColor, ViewZoneDlg::ViewZoneColorIndex::frost);
}

void Notepad_plus::prepareBufferChangedDialog(Buffer * buffer)
{
	// immediately show window if it was minimized before
	if (::IsIconic(_pPublicInterface->getHSelf()))
		::ShowWindow(_pPublicInterface->getHSelf(), SW_RESTORE);

	// switch to the file that changed
	int index = _pDocTab->getIndexByBuffer(buffer->getID());
	int iView = currentView();
	if (index == -1)
		iView = otherView();
	activateBuffer(buffer->getID(), iView);	//activate the buffer in the first view possible

	// prevent flickering issue by "manually" clicking and activating the _pEditView
	// (mouse events seem to get lost / improperly handled when showing the dialog)
	auto curPos = _pEditView->execute(SCI_GETCURRENTPOS);
	::PostMessage(_pEditView->getHSelf(), WM_LBUTTONDOWN, 0, 0);
	::PostMessage(_pEditView->getHSelf(), WM_LBUTTONUP, 0, 0);
	::PostMessage(_pEditView->getHSelf(), SCI_SETSEL, curPos, curPos);
}

void Notepad_plus::notifyBufferChanged(Buffer * buffer, int mask)
{
	// To avoid to crash while MS-DOS style is set as default language,
	// Checking the validity of current instance is necessary.
	if (!this) return;

	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParam.getNppGUI();

	_mainEditView.bufferUpdated(buffer, mask);
	_subEditView.bufferUpdated(buffer, mask);
	_mainDocTab.bufferUpdated(buffer, mask);
	_subDocTab.bufferUpdated(buffer, mask);

	bool mainActive = (_mainEditView.getCurrentBuffer() == buffer);
	bool subActive = (_subEditView.getCurrentBuffer() == buffer);

	//Only event that applies to non-active Buffers
	if (mask & BufferChangeStatus)
	{	//reload etc
		switch(buffer->getStatus())
		{
			case DOC_UNNAMED: 	//nothing todo
			case DOC_REGULAR: 	//nothing todo
			{
				break;
			}
			case DOC_MODIFIED:	//ask for reloading
			{
				// Since it is being monitored DOC_NEEDRELOAD is going to handle the change.
				if (buffer->isMonitoringOn())
					break;

				bool autoUpdate = (nppGUI._fileAutoDetection & cdAutoUpdate) ? true : false;
				if (!autoUpdate || buffer->isDirty())
				{
					prepareBufferChangedDialog(buffer);

					// Then we ask user to update
					if (doReloadOrNot(buffer->getFullPathName(), buffer->isDirty()) != IDYES)
					{
						// Since the file content has changed but the user doesn't want to reload it, set state to dirty
						buffer->setDirty(true);

						// buffer in Notepad++ is not syncronized anymore with the file on disk
						buffer->setUnsync(true);

						break;	//abort
					}
				}
				// Set _isLoadedDirty false so when the document clean state is reached the icon will be set to blue
				buffer->setLoadedDirty(false);

				// buffer in Notepad++ is syncronized with the file on disk
				buffer->setUnsync(false);

				doReload(buffer->getID(), false);
				if (mainActive || subActive)
				{
					performPostReload(mainActive?MAIN_VIEW:SUB_VIEW);
				}
				break;
			}
			case DOC_NEEDRELOAD: // by log monitoring
			{
				doReload(buffer->getID(), false);

				// not only test main view
				if (buffer == _mainEditView.getCurrentBuffer())
				{
					_mainEditView.setPositionRestoreNeeded(false);
					_mainEditView.execute(SCI_DOCUMENTEND);
				}
				// but also test sub-view, because the buffer could be clonned
				if (buffer == _subEditView.getCurrentBuffer())
				{
					_subEditView.setPositionRestoreNeeded(false);
					_subEditView.execute(SCI_DOCUMENTEND);
				}

				break;
			}
			case DOC_DELETED: 	//ask for keep
			{
				prepareBufferChangedDialog(buffer);

				SCNotification scnN;
				scnN.nmhdr.code = NPPN_FILEDELETED;
				scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
				scnN.nmhdr.idFrom = (uptr_t)buffer->getID();
				_pluginsManager.notify(&scnN);

				int doCloseDoc = doCloseOrNot(buffer->getFullPathName()) == IDNO;
				if (doCloseDoc)
				{
					//close in both views, doing current view last since that has to remain opened
					bool isSnapshotMode = nppGUI.isSnapshotMode();
					doClose(buffer->getID(), otherView(), isSnapshotMode);
					doClose(buffer->getID(), currentView(), isSnapshotMode);
					return;
				}
				else
				{
					// buffer in Notepad++ is not syncronized anymore with the file on disk
					buffer->setUnsync(true);
				}

				break;
			}
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
		scnN.nmhdr.idFrom = (uptr_t)  ((isSysReadOnly || isUserReadOnly? DOCSTATUS_READONLY : 0) | (isDirty ? DOCSTATUS_BUFFERDIRTY : 0));
		scnN.nmhdr.code = NPPN_READONLYCHANGED;
		_pluginsManager.notify(&scnN);

	}

	if (_pDocumentListPanel)
		_pDocumentListPanel->setItemIconStatus(buffer);

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
		if (mask & BufferChangeFilename)
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
		setDisplayFormat(buffer->getEolFormat());
		enableConvertMenuItems(buffer->getEolFormat());
	}
}

void Notepad_plus::notifyBufferActivated(BufferID bufid, int view)
{
	Buffer * buf = MainFileManager.getBufferByID(bufid);
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
	setDisplayFormat(buf->getEolFormat());
	enableConvertMenuItems(buf->getEolFormat());
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

	if (_pDocumentListPanel)
	{
		_pDocumentListPanel->activateItem(bufid, currentView());
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

std::vector<generic_string> Notepad_plus::loadCommandlineParams(const TCHAR * commandLine, const CmdLineParamsDTO * pCmdParams)
{
	if (!commandLine || ! pCmdParams)
		return std::vector<generic_string>();

	NppParameters& nppParams = NppParameters::getInstance();
	const NppGUI& nppGUI = nppParams.getNppGUI();
	FileNameStringSplitter fnss(commandLine);

	// loading file as session file is allowed only when there is only one file
	if (pCmdParams->_isSessionFile && fnss.size() == 1)
	{
		Session session2Load;
		if (nppParams.loadSession(session2Load, fnss.getFileName(0)))
		{
			const bool isSnapshotMode = false;
			const bool shouldLoadFileBrowser = true;

			if ((nppGUI._multiInstSetting == multiInstOnSession) || (nppGUI._multiInstSetting == multiInst))
				nppParams.setLoadedSessionFilePath(fnss.getFileName(0));
			loadSession(session2Load, isSnapshotMode, shouldLoadFileBrowser);
		}
		return std::vector<generic_string>();
	}

 	LangType lt = pCmdParams->_langType;
	generic_string udl = pCmdParams->_udlName;
	intptr_t lineNumber =  pCmdParams->_line2go;
	intptr_t columnNumber = pCmdParams->_column2go;
	intptr_t positionNumber = pCmdParams->_pos2go;
	bool recursive = pCmdParams->_isRecursive;
	bool readOnly = pCmdParams->_isReadOnly;
	bool openFoldersAsWorkspace = pCmdParams->_openFoldersAsWorkspace;
	bool monitorFiles = pCmdParams->_monitorFiles;

	if (openFoldersAsWorkspace)
	{
		// All the filepath in argument will be used as folder in workspace
		// call launchFileBrowser later with fnss
		return fnss.getFileNames();
	}

	BufferID lastOpened = BUFFER_INVALID;
	for (int i = 0, len = fnss.size(); i < len ; ++i)
	{
		const TCHAR *pFn = fnss.getFileName(i);
		if (!pFn) return std::vector<generic_string>();

		BufferID bufID = doOpen(pFn, recursive, readOnly);
		if (bufID == BUFFER_INVALID)	//cannot open file
			continue;

		lastOpened = bufID;
		Buffer* pBuf = MainFileManager.getBufferByID(bufID);

		if (!udl.empty())
		{
			pBuf->setLangType(L_USER, udl.c_str());
		}
		else if (lt != L_EXTERNAL && lt < nppParams.L_END)
		{
			pBuf->setLangType(lt);
		}

		if (lineNumber >= 0 || positionNumber >= 0)
		{
			//we have to move the cursor manually
			int iView = currentView();	//store view since fileswitch can cause it to change
			switchToFile(bufID);	//switch to the file. No deferred loading, but this way we can easily move the cursor to the right position

			if (positionNumber >= 0)
			{
				if (positionNumber > 0)
				{
					// make sure not jumping into the middle of a multibyte character
					// or into the middle of a CR/LF pair for Windows files
					auto before = _pEditView->execute(SCI_POSITIONBEFORE, positionNumber);
					positionNumber = _pEditView->execute(SCI_POSITIONAFTER, before);
				}
				_pEditView->execute(SCI_GOTOPOS, positionNumber);
			}
			else if (columnNumber < 0)
			{
				_pEditView->execute(SCI_GOTOLINE, lineNumber - 1);
			}
			else
			{
				auto pos = _pEditView->execute(SCI_FINDCOLUMN, lineNumber - 1, columnNumber - 1);
				_pEditView->execute(SCI_GOTOPOS, pos);
			}

			_pEditView->scrollPosToCenter(_pEditView->execute(SCI_GETCURRENTPOS));

			switchEditViewTo(iView);	//restore view
		}

		if (monitorFiles)
		{
			monitoringStartOrStopAndUpdateUI(pBuf, true);
			createMonitoringThread(pBuf);
		}
	}
	if (lastOpened != BUFFER_INVALID)
    {
		switchToFile(lastOpened);
	}

	return fnss.getFileNames();
}


void Notepad_plus::setFindReplaceFolderFilter(const TCHAR *dir, const TCHAR *filter)
{
	generic_string fltr;
	NppParameters& nppParam = NppParameters::getInstance();
	FindHistory & findHistory = nppParam.getFindHistory();

	// get current directory in case it's not provided.
	if (!dir && findHistory._isFolderFollowDoc)
	{
		dir = nppParam.getWorkingDir();
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
			UserLangContainer * userLangContainer = nppParam.getULCFromName(buf->getUserDefineLangName());
			if (userLangContainer)
				ext = userLangContainer->getExtention();
		}
		else
		{
			ext = nppParam.getLangExtFromLangType(lt);
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
	CustomFileDialog fDlg(_pPublicInterface->getHSelf());
	fDlg.setExtFilter(extFilterName, extFilter);

    vector<generic_string> copiedFiles;

	const auto& fns = fDlg.doOpenMultiFilesDlg();
    if (!fns.empty())
    {
        // Get plugins dir
		generic_string destDirName = (NppParameters::getInstance()).getNppPath();
        pathAppend(destDirName, destDir);

        if (!::PathFileExists(destDirName.c_str()))
        {
            ::CreateDirectory(destDirName.c_str(), NULL);
        }

        destDirName += TEXT("\\");

        size_t sz = fns.size();
        for (size_t i = 0 ; i < sz ; ++i)
        {
            if (::PathFileExists(fns.at(i).c_str()))
            {
                // copy to plugins directory
                generic_string destName = destDirName;
                destName += ::PathFindFileName(fns.at(i).c_str());
                if (::CopyFile(fns.at(i).c_str(), destName.c_str(), FALSE))
                    copiedFiles.push_back(destName.c_str());
            }
        }
    }
    return copiedFiles;
}

vector<generic_string> Notepad_plus::addNppPlugins(const TCHAR *extFilterName, const TCHAR *extFilter)
{
	CustomFileDialog fDlg(_pPublicInterface->getHSelf());
    fDlg.setExtFilter(extFilterName, extFilter);

    vector<generic_string> copiedFiles;

	const auto& fns = fDlg.doOpenMultiFilesDlg();
	if (!fns.empty())
    {
        // Get plugins dir
		generic_string destDirName = (NppParameters::getInstance()).getPluginRootDir();

        if (!::PathFileExists(destDirName.c_str()))
        {
            ::CreateDirectory(destDirName.c_str(), NULL);
        }

        size_t sz = fns.size();
        for (size_t i = 0 ; i < sz ; ++i)
        {
            if (::PathFileExists(fns.at(i).c_str()))
            {
                // copy to plugins directory
                generic_string destName = destDirName;
				
				generic_string nameExt = ::PathFindFileName(fns.at(i).c_str());
				auto pos = nameExt.find_last_of(TEXT("."));
				if (pos == generic_string::npos)
					continue;

				generic_string name = nameExt.substr(0, pos);
				pathAppend(destName, name);
				if (!::PathFileExists(destName.c_str()))
				{
					::CreateDirectory(destName.c_str(), NULL);
				}
				pathAppend(destName, nameExt);

                if (::CopyFile(fns.at(i).c_str(), destName.c_str(), FALSE))
                    copiedFiles.push_back(destName.c_str());
            }
        }
    }
    return copiedFiles;
}

void Notepad_plus::setWorkingDir(const TCHAR *dir)
{
	NppParameters& params = NppParameters::getInstance();
	if (params.getNppGUI()._openSaveDir == dir_last)
		return;
	if (params.getNppGUI()._openSaveDir == dir_userDef)
	{
		params.setWorkingDir(NULL);
	}
	else if (dir && PathIsDirectory(dir))
	{
		params.setWorkingDir(dir);
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

	id = (NppParameters::getInstance()).langTypeToCommandID( buf->getLangType() );
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
	return NppParameters::getInstance().getMiscStylerArray().findByName(styleName);
}

bool Notepad_plus::noOpenedDoc() const
{
	if (_mainDocTab.isVisible() && _subDocTab.isVisible())
		return false;
	if (_pDocTab->nbItem() == 1)
	{
		BufferID buffer = _pDocTab->getBufferByIndex(0);
		Buffer * buf = MainFileManager.getBufferByID(buffer);
		if (!buf->isDirty() && buf->isUntitled())
			return true;
	}
	return false;
}

bool Notepad_plus::reloadLang()
{
	NppParameters& nppParam = NppParameters::getInstance();

	if (!nppParam.reloadLang())
	{
		return false;
	}

	TiXmlDocumentA *nativeLangDocRootA = nppParam.getNativeLangA();
	if (!nativeLangDocRootA)
	{
		return false;
	}

    _nativeLangSpeaker.init(nativeLangDocRootA, true);

    nppParam.reloadContextMenuFromXmlTree(_mainMenuHandle, _pluginsManager.getMenuHandle());

	_nativeLangSpeaker.changeMenuLang(_mainMenuHandle);
    ::DrawMenuBar(_pPublicInterface->getHSelf());

	// Update scintilla context menu strings
	vector<MenuItemUnit> & tmp = nppParam.getContextMenuItems();
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

	updateCommandShortcuts();

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

	if (_md5FromFilesDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_md5FromFilesDlg.getHSelf(), "MD5FromFilesDlg");
	}

	if (_md5FromTextDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_md5FromTextDlg.getHSelf(), "MD5FromTextDlg");
	}

	if (_sha2FromFilesDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_sha2FromFilesDlg.getHSelf(), "SHA256FromFilesDlg");
	}

	if (_sha2FromTextDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_sha2FromTextDlg.getHSelf(), "SHA256FromTextDlg");
	}

	if (_runMacroDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_runMacroDlg.getHSelf(), "MultiMacro");
	}

	if (_incrementFindDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_incrementFindDlg.getHSelf(), "IncrementalFind");
	}

	if (_findCharsInRangeDlg.isCreated())
	{
		_nativeLangSpeaker.changeDlgLang(_findCharsInRangeDlg.getHSelf(), "FindCharsInRange");
	}

	if (_colEditorDlg.isCreated())
	{
        _nativeLangSpeaker.changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
	}

	if (_pluginsAdminDlg.isCreated())
	{
		_nativeLangSpeaker.changePluginsAdminDlgLang(_pluginsAdminDlg);
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
	NppParameters& nppParams = NppParameters::getInstance();
	if (!_pClipboardHistoryPanel)
	{
		_pClipboardHistoryPanel = new ClipboardHistoryPanel();

		_pClipboardHistoryPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);

		NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
		bool isRTL = pNativeSpeaker->isRTL();
		tTbData	data = {};
		_pClipboardHistoryPanel->create(&data, isRTL);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pClipboardHistoryPanel->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_USEOWNDARKMODE;

		int icoID = IDR_CLIPBOARDPANEL_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_CLIPBOARDPANEL_ICO_DM;
		else if (nppParams.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_CLIPBOARDPANEL_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_EDIT_CLIPBOARDHISTORY_PANEL;

		generic_string title_temp = pNativeSpeaker->getAttrNameStr(CH_PROJECTPANELTITLE, "ClipboardHistory", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		COLORREF fgColor = nppParams.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParams.getCurrentDefaultBgColor();

		_pClipboardHistoryPanel->setBackgroundColor(bgColor);
		_pClipboardHistoryPanel->setForegroundColor(fgColor);
	}

	_pClipboardHistoryPanel->display();
}


void Notepad_plus::launchDocumentListPanel()
{
	if (!_pDocumentListPanel)
	{
		NppParameters& nppParams = NppParameters::getInstance();
		int tabBarStatus = nppParams.getNppGUI()._tabStatus;

		_pDocumentListPanel = new VerticalFileSwitcher;
		HIMAGELIST hImgLst = ((tabBarStatus & TAB_ALTICONS) ? _docTabIconListAlt.getHandle() : NppDarkMode::isEnabled() ? _docTabIconListDarkMode.getHandle() : _docTabIconList.getHandle());
		_pDocumentListPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), hImgLst);
		NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
		bool isRTL = pNativeSpeaker->isRTL();
		tTbData	data = {};
		_pDocumentListPanel->create(&data, isRTL);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pDocumentListPanel->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB | DWS_USEOWNDARKMODE;

		int icoID = IDR_DOCLIST_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_DOCLIST_ICO_DM;
		else if (nppParams.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_DOCLIST_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_DOCLIST;

		generic_string title_temp = pNativeSpeaker->getAttrNameStr(FS_PROJECTPANELTITLE, "DocList", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		COLORREF fgColor = nppParams.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParams.getCurrentDefaultBgColor();

		_pDocumentListPanel->setBackgroundColor(bgColor);
		_pDocumentListPanel->setForegroundColor(fgColor);
	}
	_pDocumentListPanel->display();
}


void Notepad_plus::launchAnsiCharPanel()
{
	if (!_pAnsiCharPanel)
	{
		_pAnsiCharPanel = new AnsiCharPanel();
		_pAnsiCharPanel->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);
		
		NppParameters& nppParams = NppParameters::getInstance();

		NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
		bool isRTL = pNativeSpeaker->isRTL();
		tTbData	data = {};
		_pAnsiCharPanel->create(&data, isRTL);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pAnsiCharPanel->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_USEOWNDARKMODE;

		int icoID = IDR_ASCIIPANEL_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_ASCIIPANEL_ICO_DM;
		else if (nppParams.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_ASCIIPANEL_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_EDIT_CHAR_PANEL;

		generic_string title_temp = pNativeSpeaker->getAttrNameStr(AI_PROJECTPANELTITLE, "AsciiInsertion", "PanelTitle");
		static TCHAR title[85];
		if (title_temp.length() < 85)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		COLORREF fgColor = nppParams.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParams.getCurrentDefaultBgColor();

		_pAnsiCharPanel->setBackgroundColor(bgColor);
		_pAnsiCharPanel->setForegroundColor(fgColor);
	}

	_pAnsiCharPanel->display();
}

void Notepad_plus::launchFileBrowser(const vector<generic_string> & folders, const generic_string& selectedItemPath, bool fromScratch)
{
	if (!_pFileBrowser)
	{
		_pFileBrowser = new FileBrowser;
		_pFileBrowser->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());

		tTbData	data;
		memset(&data, 0, sizeof(data));
		_pFileBrowser->create(&data, _nativeLangSpeaker.isRTL());
		data.pszName = TEXT("ST");

		NppParameters& nppParams = NppParameters::getInstance();

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pFileBrowser->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB | DWS_USEOWNDARKMODE;
		
		int icoID = IDR_FILEBROWSER_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_FILEBROWSER_ICO_DM;
		else if (nppParams.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_FILEBROWSER_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_FILEBROWSER;

		NativeLangSpeaker *pNativeSpeaker = nppParams.getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(FB_PANELTITLE, FOLDERASWORKSPACE_NODE, "PanelTitle");

		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		COLORREF fgColor = nppParams.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParams.getCurrentDefaultBgColor();

		_pFileBrowser->setBackgroundColor(bgColor);
		_pFileBrowser->setForegroundColor(fgColor);
	}

	if (fromScratch)
	{
		_pFileBrowser->deleteAllFromTree();
	}

	for (size_t i = 0; i <folders.size(); ++i)
	{
		_pFileBrowser->addRootFolder(folders[i]);
	}

	_pFileBrowser->display();
	_pFileBrowser->selectItemFromPath(selectedItemPath);

	checkMenuItem(IDM_VIEW_FILEBROWSER, true);
	_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
	_pFileBrowser->setClosed(false);
}

void Notepad_plus::checkProjectMenuItem()
{
	HMENU viewMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_VIEW);
	int viewMenuCount = ::GetMenuItemCount(viewMenu);
	for (int i = 0; i < viewMenuCount; i++)
	{
		HMENU subMenu = ::GetSubMenu(viewMenu, i);
		if (subMenu)
		{
			int subMenuCount = ::GetMenuItemCount(subMenu);
			bool found = false;
			bool checked = false;
			for (int j = 0; j < subMenuCount; j++)
			{
				UINT const ids [] = {IDM_VIEW_PROJECT_PANEL_1, IDM_VIEW_PROJECT_PANEL_2, IDM_VIEW_PROJECT_PANEL_3};
				UINT id = GetMenuItemID (subMenu, j);
				for (size_t k = 0; k < _countof(ids); k++)
				{
					if (id == ids [k])
					{
						found = true;
						UINT s = GetMenuState(subMenu, j, MF_BYPOSITION);
						if (s & MF_CHECKED)
						{
							checked = true;
							break;
						}
					}
				}
			}
			if (found)
			{
				CheckMenuItem(viewMenu, i, (checked ? MF_CHECKED : MF_UNCHECKED) | MF_BYPOSITION);
				break;
			}
		}
	}
}

void Notepad_plus::launchProjectPanel(int cmdID, ProjectPanel ** pProjPanel, int panelID)
{
	NppParameters& nppParam = NppParameters::getInstance();
	if (!(*pProjPanel))
	{
		(*pProjPanel) = new ProjectPanel;
		(*pProjPanel)->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), panelID);
		(*pProjPanel)->setWorkSpaceFilePath(nppParam.getWorkSpaceFilePath(panelID));
		NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
		bool isRTL = pNativeSpeaker->isRTL();
		tTbData	data;
		memset(&data, 0, sizeof(data));
		(*pProjPanel)->create(&data, isRTL);
		data.pszName = TEXT("ST");

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>((*pProjPanel)->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB | DWS_USEOWNDARKMODE;

		int icoID = IDR_PROJECTPANEL_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_PROJECTPANEL_ICO_DM;
		else if (nppParam.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_PROJECTPANEL_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = cmdID;

		generic_string title_no = to_wstring (panelID + 1);
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(PM_PROJECTPANELTITLE, "ProjectManager", "PanelTitle") + TEXT(" ") + title_no;
		(*pProjPanel)->setPanelTitle(title_temp);
		data.pszName = (*pProjPanel)->getPanelTitle();
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		COLORREF fgColor = nppParam.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParam.getCurrentDefaultBgColor();

		(*pProjPanel)->setBackgroundColor(bgColor);
		(*pProjPanel)->setForegroundColor(fgColor);
	}
	else
	{
		(*pProjPanel)->openWorkSpace(nppParam.getWorkSpaceFilePath(panelID));
	}
	(*pProjPanel)->display();

	checkMenuItem(cmdID, true);
	checkProjectMenuItem();
	(*pProjPanel)->setClosed(false);
}


void Notepad_plus::launchDocMap()
{
	NppParameters& nppParam = NppParameters::getInstance();
	if (!nppParam.isTransparentAvailable())
	{
		_nativeLangSpeaker.messageBox("PrehistoricSystemDetected",
			_pPublicInterface->getHSelf(),
			TEXT("It seems you still use a prehistoric system. This feature works only on a modern system, sorry."),
			TEXT("Prehistoric system detected"),
			MB_OK);

		return;
	}

	if (!_pDocMap)
	{
		_pDocMap = new DocumentMap();
		_pDocMap->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), &_pEditView);

		tTbData	data = {};
		_pDocMap->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pDocMap->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_USEOWNDARKMODE;

		int icoID = IDR_DOCMAP_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_DOCMAP_ICO_DM;
		else if (nppParam.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_DOCMAP_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_DOC_MAP;

		NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(DM_PANELTITLE, "DocumentMap", "PanelTitle");
		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}
		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));
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

		tTbData	data = {};
		_pFuncList->create(&data);

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pFuncList->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_USEOWNDARKMODE;
		
		NppParameters& nppParam = NppParameters::getInstance();

		int icoID = IDR_FUNC_LIST_ICO;
		if (NppDarkMode::isEnabled())
			icoID = IDR_FUNC_LIST_ICO_DM;
		else if (nppParam.getNppGUI()._toolBarStatus != TB_STANDARD)
			icoID = IDR_FUNC_LIST_ICO2;

		data.hIconTab = (HICON)::LoadImage(_pPublicInterface->getHinst(), MAKEINTRESOURCE(icoID), IMAGE_ICON, 14, 14, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		// In the case of Notepad++ internal function, it'll be the command ID which triggers this dialog
		data.dlgID = IDM_VIEW_FUNC_LIST;

		NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
		generic_string title_temp = pNativeSpeaker->getAttrNameStr(FL_PANELTITLE, FL_FUCTIONLISTROOTNODE, "PanelTitle");

		static TCHAR title[32];
		if (title_temp.length() < 32)
		{
			wcscpy_s(title, title_temp.c_str());
			data.pszName = title;
		}

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));
		COLORREF fgColor = nppParam.getCurrentDefaultFgColor();
		COLORREF bgColor = nppParam.getCurrentDefaultBgColor();

		_pFuncList->setBackgroundColor(bgColor);
		_pFuncList->setForegroundColor(fgColor);
	}

	_pFuncList->display();
}


struct TextPlayerParams
{
	HWND _nppHandle = nullptr;
	ScintillaEditView* _pCurrentView = nullptr;
	QuoteParams* _quotParams = nullptr;
};

struct TextTrollerParams
{
	ScintillaEditView *_pCurrentView;
	const wchar_t*_text2display;
	BufferID _targetBufID;
	HANDLE _mutex;
};


static const QuoteParams quotes[] =
{
	{TEXT("Notepad++"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The creation of Notepad++ is due to my need for a decent editor to edit the source code of Notepad++")},
	{TEXT("Notepad++ #1"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I hate reading other people's code.\nSo I wrote mine, made it as open source project, and watch others suffer.")},
	{TEXT("Notepad++ #2"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Good programmers use Notepad++ to code.\nExtreme programmers use MS Word to code, in Comic Sans, center aligned.")},
	{TEXT("Notepad++ #3"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The best things in life are free.\nNotepad++ is free.\nSo Notepad++ is the best.\n")},
	{TEXT("Richard Stallman"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If I'm the Father of Open Source, it was conceived through artificial insemination using stolen sperm without my knowledge or consent.")},
	{TEXT("Martin Golding"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Always code as if the guy who ends up maintaining your code will be a violent psychopath who knows where you live.")},
	{TEXT("L. Peter Deutsch"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("To iterate is human, to recurse divine.")},
	{TEXT("Seymour Cray"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The trouble with programmers is that you can never tell what a programmer is doing until it's too late.")},
	{TEXT("Brian Kernighan"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Debugging is twice as hard as writing the code in the first place. Therefore, if you write the code as cleverly as possible, you are, by definition, not smart enough to debug it.")},
	{TEXT("Alan Kay"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Most software today is very much like an Egyptian pyramid with millions of bricks piled on top of each other, with no structural integrity, but just done by brute force and thousands of slaves.")},
	{TEXT("Bill Gates"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Measuring programming progress by lines of code is like measuring aircraft building progress by weight.")},
	{TEXT("Christopher Thompson"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Sometimes it pays to stay in bed on Monday, rather than spending the rest of the week debugging Monday's code.")},
	{TEXT("Vidiu Platon"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I don't care if it works on your machine! We are not shipping your machine!")},
	{TEXT("Edward V Berard"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Walking on water and developing software from a specification are easy if both are frozen.")},
	{TEXT("pixadel"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Fine, Java MIGHT be a good example of what a programming language should be like.\nBut Java applications are good examples of what applications SHOULDN'T be like.")},
	{TEXT("Oktal"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I think Microsoft named .Net so it wouldn't show up in a Unix directory listing.")},
	{TEXT("Bjarne Stroustrup"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("In C++ it's harder to shoot yourself in the foot, but when you do, you blow off your whole leg.")},
	{TEXT("Mosher's Law of Software Engineering"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Don't worry if it doesn't work right. If everything did, you'd be out of a job.")},
	{TEXT("Bob Gray"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Writing in C or C++ is like running a chain saw with all the safety guards removed.")},
	{TEXT("Roberto Waltman"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("In the one and only true way. The object-oriented version of \"Spaghetti code\" is, of course, \"Lasagna code\". (Too many layers)")},
	{TEXT("Gavin Russell Baker"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("C++ : Where friends have access to your private members.")},
	{TEXT("Linus Torvalds"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Software is like sex: It's better when it's free.")},
	{TEXT("Cult of vi"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Emacs is a great operating system, lacking only a decent editor.")},
	{TEXT("Church of Emacs"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("vi has two modes - \"beep repeatedly\" and \"break everything\".")},
	{TEXT("Steve Jobs"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Picasso had a saying: \"Good artists copy, great artists steal.\".\nWe have always been shameless about stealing great ideas.")},
	{TEXT("brotips #1001"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Do everything for greatness, not money. Money follows greatness.")},
	{TEXT("Robin Williams"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("God gave men both a penis and a brain, but unfortunately not enough blood supply to run both at the same time.")},
	{TEXT("Darth Vader"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Strong people don't put others down.\nThey lift them up.")},
	{TEXT("Darth Vader #2"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("You don't get to 500 million star systems without making a few enemies.")},
	{TEXT("Doug Linder"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A good programmer is someone who always looks both ways before crossing a one-way street.")},
	{TEXT("Jean-Claude van Damme"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A cookie has no soul, it's just a cookie. But before it was milk and eggs.\nAnd in eggs there's the potential for life.")},
	{TEXT("Michael Feldman"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Java is, in many ways, C++--.")},
	{TEXT("Don Ho"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Je mange donc je chie.")},
	{TEXT("Don Ho #2"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("RTFM is the true path of every developer.\nBut it would happen only if there's no way out.")},
	{TEXT("Don Ho #3"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Smartphone is the best invention of 21st century for avoiding the eyes contact while crossing people you know on the street.")},
	{TEXT("Don Ho #4"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Poor countries' museums vs. rich countries' museums:\nThe first show what they have left.\nThe second show what they have stolen.")},
	{TEXT("Anonymous #1"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("An opinion without 3.14 is just an onion.")},
	{TEXT("Anonymous #2"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Before sex, you help each other get naked, after sex you only dress yourself.\nMoral of the story: in life no one helps you once you're fucked.")},
	{TEXT("Anonymous #3"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("I'm not totally useless. I can be used as a bad example.")},
	{TEXT("Anonymous #4"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Life is too short to remove USB safely.")},
	{TEXT("Anonymous #5"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("\"SEX\" is not the answer.\nSex is the question, \"YES\" is the answer.")},
	{TEXT("Anonymous #6"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Going to McDonald's for a salad is like going to a whore for a hug.")},
	{TEXT("Anonymous #7"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("I need a six month holiday, TWICE A YEAR!")},
	{TEXT("Anonymous #8"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Everything is a knife if you're strong enough.")},
	{TEXT("Anonymous #9"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I'M A FUCKING ANIMAL IN BED.\nMore specifically a koala.")},
	{TEXT("Anonymous #10"), QuoteParams::slow, true, SC_CP_UTF8, L_TEXT, TEXT("Etc.\n\n(Abb.) End of Thinking Capacity.\n")},
	{TEXT("Anonymous #11"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("We stopped checking for monsters under our bed, when we realized they were inside us.")},
	{TEXT("Anonymous #12"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I would rather check my facebook than face my checkbook.")},
	{TEXT("Anonymous #13"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Whoever says Paper beats Rock is an idiot. Next time I see someone say that I will throw a rock at them while they hold up a sheet of paper.")},
	{TEXT("Anonymous #14"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A better world is where chickens can cross the road without having their motives questioned.")},
	{TEXT("Anonymous #15"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If I didn't drink, how would my friends know I love them at 2 AM?")},
	{TEXT("Anonymous #16"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Q: How do you generate a random string?\nA: Put a Windows user in front of vi, and tell him to exit.")},
	{TEXT("Anonymous #17"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Pros and cons of making food.\nPros: food\nCons : making\n")},
	{TEXT("Anonymous #18"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Never get into fights with ugly people, they have nothing to lose.")},
	{TEXT("Anonymous #19"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("People who say they give 110%\ndon't really understand how percentages work.")},
	{TEXT("Anonymous #20"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Never make eye contact while eating a banana.")},
	{TEXT("Anonymous #21"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I love my sixpack so much, I protect it with a layer of fat.")},
	{TEXT("Anonymous #22"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("\"It's impossible.\" said pride.\n\"It's risky.\" said experience.\n\"It's pointless.\" said reason.\n\"Give it a try.\" whispered the heart.\n...\n\"What the hell was that?!?!?!?!?!\" shouted the anus two minutes later.")},
	{TEXT("Anonymous #23"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("A programmer is told to \"go to hell\".\nHe finds the worst part of that statement is the \"go to\".")},
	{TEXT("Anonymous #24"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("An Architect's dream is an Engineer's nightmare.")},
	{TEXT("Anonymous #25"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("In a way, I feel sorry for the kids of this generation.\nThey'll have parents who know how to check browser history.")},
	{TEXT("Anonymous #26"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Q: What's the difference between git and github?\nA: It's the difference between porn and pornhub.\n")},
	{TEXT("Anonymous #27"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I don't have a problem with caffeine.\nI have a problem without caffeine.")},
	{TEXT("Anonymous #28"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Why 6 afraid of 7?\nBecause 7 8 9 while 6 and 9 were flirting.")},
	{TEXT("Anonymous #29"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("How do you comfort a JavaScript bug?\nYou console it.")},
	{TEXT("Anonymous #30"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Why do Java developers wear glasses?\nBecause they don't C#.")},
	{TEXT("Anonymous #31"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A baby's laughter is one of the most beautiful sounds you will ever hear. Unless it's 3 AM. And you're home alone. And you don't have a baby.")},
	{TEXT("Anonymous #32"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Two bytes meet. The first byte asks, \"You look terrible. Are you OK?\"\nThe second byte replies, \"No, just feeling a bit off.\"")},
	{TEXT("Anonymous #33"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Programmer - an organism that turns coffee into software.")},
	{TEXT("Anonymous #34"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("It's not a bug - it's an undocumented feature.")},
	{TEXT("Anonymous #35"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Should array index start at 0 or 1?\nMy compromised solution is 0.5")},
	{TEXT("Anonymous #36"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Every single time when I'm about to hug someone extremely sexy, I hit the mirror.")},
	{TEXT("Anonymous #37"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("My software never has bugs. It just develops random features.")},
	{TEXT("Anonymous #38"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("LISP = Lots of Irritating Silly Parentheses.")},
	{TEXT("Anonymous #39"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Perl, the only language that looks the same before and after RSA encryption.")},
	{TEXT("Anonymous #40"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("People ask me why, as an atheist, I still say: OH MY GOD.\nIt makes perfect sense: We say \"Oh my God\" when something is UNBELIEVABLE.")},
	{TEXT("Anonymous #41"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("1. Dig a hole.\n2. Name it love.\n3. Watch people falling in love.\n")},
	{TEXT("Anonymous #42"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Don't think of yourself as an ugly person.\nThink of yourself as a beautiful monkey.")},
	{TEXT("Anonymous #43"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Afraid to die alone?\nBecome a bus driver.")},
	{TEXT("Anonymous #44"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The first 5 days after the weekend are always the hardest.")},
	{TEXT("Anonymous #45"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Rhinos are just fat unicorns.")},
	{TEXT("Anonymous #46"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Sometimes when I'm writing Javascript I want to throw up my hands and say \"this is bullshit!\"\nbut I can never remember what \"this\" refers to.")},
	{TEXT("Anonymous #47"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Kids are like farts.\nYou can only stand yours.")},
	{TEXT("Anonymous #48"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("If you were born in Israel, you'd probably be Jewish.\nIf you were born in Saudi Arabia, you'd probably be Muslim.\nIf you were born in India, you'd probably be Hindu.\nBut because you were born in North America, you're Christian.\nYour faith is not inspired by some divine, constant truth.\nIt's simply geography.")},
	{TEXT("Anonymous #49"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("There are 2 types of people in this world:\nPeople who say they pee in the shower, and the dirty fucking liars.")},
	{TEXT("Anonymous #50"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("London 2012 Olympic Games - A bunch of countries coming across the ocean to put their flags in Britain and try to get a bunch of gold... it's like history but opposite.")},
	{TEXT("Anonymous #51"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I don't need a stable relationship,\nI just need a stable Internet connection.")},
	{TEXT("Anonymous #52"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("What's the difference between religion and bullshit?\nThe bull.")},
	{TEXT("Anonymous #53"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Today, as I was waiting for my girlfriend in the street, I saw a woman who looked a lot like her. I ran towards her, my arms in the air ready to give her a hug, only to realise it wasn't her. I then had to pass the woman, my arms in the air, still running. FML")},
	{TEXT("Anonymous #54"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Decimal: 1 + 1 = 2\nBinary:  1 + 1 = 10\nBoolean: 1 + 1 = 1\nJavaScript(hold my beer) : 1 + 1 = 11\n")},
	{TEXT("Anonymous #55"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Don't be ashamed of who you are.\nThat's your parents job.")},
	{TEXT("Anonymous #56"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Religion is like circumcision.\nIf you wait until someone is 21 to tell them about it they probably won't be interested.")},
	{TEXT("Anonymous #57"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("No, no, no, I'm not insulting you.\nI'm describing you.")},
	{TEXT("Anonymous #58"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I bought a dog once. Named him \"Stay\".\n\"Come here, Stay.\"\nHe's insane now.")},
	{TEXT("Anonymous #59"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Law of Software Quality:\n	errors = (more code)²\ne = mc²")},
	{TEXT("Anonymous #60"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Yesterday I named my Wifi network \"hack me if you can\"\nToday when I woke up it was changed to \"challenge accepted\".")},
	{TEXT("Anonymous #61"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Your mother is so fat,\nthe recursive function computing her mass causes a stack overflow.")},
	{TEXT("Anonymous #62"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Oral sex makes my day, but anal sex makes my hole weak.")},
	{TEXT("Anonymous #63"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I'm not saying I am Batman, I am just saying no one has ever seen me and Batman in the same room together.")},
	{TEXT("Anonymous #64"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I took a taxi today.\nThe driver told me \"I love my job, I own this car, I've got my own business, I'm my own boss, NO ONE tells me what to do!\"\nI said \"TURN LEFT HERE\".\n")},
	{TEXT("Anonymous #65"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("A man without God is like a fish without a bicycle.")},
	{TEXT("Anonymous #66"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I hate how spiders just sit there on the walls and act like they pay rent!")},
	{TEXT("Anonymous #67"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Whenever someone starts a sentence by saying \"I'm not racist...\"),they are about to say something super racist.")},
	{TEXT("Anonymous #68"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I'm not laughing at you, I'm laughing with you, you're just not laughing.\n")},
	{TEXT("Anonymous #69"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Women need a reason to have sex. Men just need a place.")},
	{TEXT("Anonymous #70"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("If abortion is murder then are condoms kidnapping?")},
	{TEXT("Anonymous #71"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Men also have feelings.\nFor example, they can feel hungry.")},
	{TEXT("Anonymous #72"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Project Manager:\nA person who thinks 9 women can deliver a baby in 1 month.")},
	{TEXT("Anonymous #73"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If you try and don't succeed, cheat. Repeat until caught. Then lie.")},
	{TEXT("Anonymous #74"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Olympics is the stupidest thing.\nPeople are so proud to be competing for their country.\nThey play their stupid song and raise some dumb flags.\nI'd love to see no flags raised, no song, no mention of country.\nOnly people.")},
	{TEXT("Anonymous #75"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("I think therefore I am\nnot religious.")},
	{TEXT("Anonymous #76"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Even if being gay were a choice, so what?\nPeople choose to be assholes and they can get married.")},
	{TEXT("Anonymous #77"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Governments are like diapers.\nThey should be changed often, and for the same reason.")},
	{TEXT("Anonymous #78"), QuoteParams::slow, true, SC_CP_UTF8, L_TEXT, TEXT("Mathématiquement, un cocu est un entier qui partage sa moitié avec un tiers.\n")},
	{TEXT("Anonymous #79"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("I'm a creationist.\nI believe man created God.\n")},
	{TEXT("Anonymous #80"), QuoteParams::slow, true, SC_CP_UTF8, L_TEXT, TEXT("Let's eat kids.\nLet's eat, kids.\n\nUse a comma.\nSave lives.\n")},
	{TEXT("Anonymous #81"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A male engineering student was crossing a road one day when a frog called out to him and said, \"If you kiss me, I'll turn into a beautiful princess.\" He bent over, picked up the frog, and put it in his pocket.\n\nThe frog spoke up again and said, \"If you kiss me and turn me back into a beautiful princess, I will stay with you for one week.\" The engineering student took the frog out of his pocket, smiled at it; and returned it to his pocket.\n\nThe frog then cried out, \"If you kiss me and turn me back into a princess, I'll stay with you and do ANYTHING you want.\" Again the boy took the frog out, smiled at it, and put it back into his pocket.\n\nFinally, the frog asked, \"What is the matter? I've told you I'm a beautiful princess, that I'll stay with you for a week and do anything you want. Why won't you kiss me?\" The boy said, \"Look I'm an engineer. I don't have time for a girlfriend, but a talking frog is cool.\"\n")},
	{TEXT("Anonymous #82"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Gamers never die.\nThey just go offline.\n")},
	{TEXT("Anonymous #83"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Copy from one, it's plagiarism.\nCopy from two, it's research.\n")},
	{TEXT("Anonymous #84"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Saying that Java is nice because it works on all OSes is like saying that anal sex is nice because it works on all genders.")},
	{TEXT("Anonymous #85"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Race, religion, ethnic pride and nationalism etc... does nothing but teach you how to hate people that you've never met.")},
	{TEXT("Anonymous #86"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Farts are just the ghosts of the things we eat.")},
	{TEXT("Anonymous #87"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I promised I would never kill someone who had my blood.\nBut that mosquito made me break my word.")},
	{TEXT("Anonymous #88"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Si un jour une chaise te dit que t'as un joli cul, tu trouveras ça bizarre mais c'est juste un compliment d'objet direct.")},
	{TEXT("Anonymous #89"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("The biggest step in any relationship isn't the first kiss.\nIt's the first fart.")},
	{TEXT("Anonymous #90"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Clapping:\n(verb)\nRepeatedly high-fiving yourself for someone else's accomplishments.")},
	{TEXT("Anonymous #91"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("CV: ctrl-C, ctrl-V")},
	{TEXT("Anonymous #92"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Mondays are not so bad.\nIt's your job that sucks.")},
	{TEXT("Anonymous #93"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("[In a job interview]\nInterviewer: What's your greatest weakness?\nCandidate: Honesty.\nInterviewer: I don't think honesty is a weakness.\nCandidate: I don't give a fuck what you think.")},
	{TEXT("Anonymous #94"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Hey, I just met you\nAnd this is crazy\nHere's my number 127.0.0.1\nPing me maybe?")},
	{TEXT("Anonymous #95"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("What if the spider you killed in your house had spent his entire life thinking you were his roommate?\nEver think about that?\nNo. You only think about yourself.\n")},
	{TEXT("Anonymous #96"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Code for 6 minutes, debug for 6 hours.")},
	{TEXT("Anonymous #97"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Real Programmers don't comment their code.\nIf it was hard to write, it should be hard to read.")},
	{TEXT("Anonymous #98"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("My neighbours listen to good music.\nWhether they like it or not.")},
	{TEXT("Anonymous #99"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I've been using Vim for about 2 years now,\nmostly because I can't figure out how to exit it.")},
	{TEXT("Anonymous #100"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Dear YouTube,\nI can deal with Ads.\nI can deal with Buffer.\nBut when Ads buffer, I suffer.")},
	{TEXT("Anonymous #101"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("It's always sad when a man and his dick share only one brain...\nand it turns out to be the dick's.")},
	{TEXT("Anonymous #102"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If IE is brave enough to ask you to set it as your default browser,\ndon't tell me you dare not ask a girl out.")},
	{TEXT("Anonymous #104"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The main idea of \"Inception\":\nif you run a VM inside a VM inside a VM inside a VM inside a VM,\neverything will be very slow.")},
	{TEXT("Anonymous #105"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("The best antivirus is common sense.")},
	{TEXT("Anonymous #106"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("When I die, I want to go peacefully like my grandfather did, in his sleep\n- not screaming, like the passengers in his car.")},
	{TEXT("Anonymous #107"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Remember, YOUR God is real.\nAll those other Gods are ridiculous, made-up nonsense.\nBut not yours.\nYour God is real. Whichever one that is.")},
	{TEXT("Anonymous #108"), QuoteParams::rapid, true, SC_CP_UTF8, L_CSS, TEXT("#your-mom {\n	width: 100000000000000000000px;\n	float: nope;\n}\n")},
	{TEXT("Anonymous #109"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("What's the best thing about UDP jokes?\nI don't care if you get them.")},
	{TEXT("Anonymous #110"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A programmer had a problem, so he decided to use threads.\nNow 2 has. He problems")},
	{TEXT("Anonymous #111"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I'VE NEVER BEEN VERY GOOD AT GEOGRAPHY.\nBUT I CAN NAME AT LEAST ONE CITY IN FRANCE, WHICH IS NICE.")},
	{TEXT("Anonymous #112"), QuoteParams::rapid, true, SC_CP_UTF8, L_CSS, TEXT("#hulk {\n    height: 200%;\n    width: 200%;\n    color: green;\n}\n")},
	{TEXT("Anonymous #113"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("A colon can completely change the meaning of a sentence. For example:\n- Jane ate her friend's sandwich.\n- Jane ate her friend's colon.")},
	{TEXT("Anonymous #114"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("How can you face your problem if your problem is your face?")},
	{TEXT("Anonymous #115"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("YOLOLO:\nYou Only LOL Once.")},
	{TEXT("Anonymous #116"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Every exit is an entrance to new experiences.")},
	{TEXT("Anonymous #117"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("A Native American was asked:\n\"Do you celebrate Columbus day?\"\nHe replied:\n\"I don't know, do Jews celebrate Hitler's birthday?\"")},
	{TEXT("Anonymous #118"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I love necrophilia, but i can't stand the awkward silences.")},
	{TEXT("Anonymous #119"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("\"I'm gonna Google that. BING that, Bing that, sorry.\"\n- The CEO of Bing (many times per day still)")},
	{TEXT("Anonymous #120"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("GRAMMAR\nThe difference between knowing your shit and knowing you're shit.")},
	{TEXT("Anonymous #121"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("5 out of 6 people agree that Russian roulette is completely safe.")},
	{TEXT("Anonymous #122"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Nerd?\nI prefer the term \"Intellectual badass\".")},
	{TEXT("Anonymous #123"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("I know every digit of π,\njust not in the right order.")},
	{TEXT("Anonymous #124"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("You don't need religion to have morals.\nIf you can't determine right from wrong then you lack empathy, not religion.")},
	{TEXT("Anonymous #125"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Pooping with the door opened is the meaning of true freedom.")},
	{TEXT("Anonymous #126"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Social media does not make people stupid.\nIt just makes stupid people more visible.")},
	{TEXT("Anonymous #127"), QuoteParams::rapid, false, SC_CP_UTF8, L_SQL, TEXT("SELECT finger\nFROM hand\nWHERE id = 2 ;\n")},
	{TEXT("Anonymous #128"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("3 men are on a boat. They have 4 cigarettes, but nothing to light them with.\nSo they throw a cigarette overboard and the whole boat becomes a cigarette lighter.")},
	{TEXT("Anonymous #129"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("What is the most used language in programming?\n\nProfanity\n")},
	{TEXT("Anonymous #130"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Common sense is so rare, it's kinda like a superpower...")},
	{TEXT("Anonymous #131"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("The best thing about a boolean is even if you are wrong, you are only off by a bit.")},
	{TEXT("Anonymous #132"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Benchmarks don't lie, but liars do benchmarks.")},
	{TEXT("Anonymous #133"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Multitasking: Screwing up several things at once.")},
	{TEXT("Anonymous #134"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Linux is user friendly.\nIt's just picky about its friends.")},
	{TEXT("Anonymous #135"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Theory is when you know something, but it doesn't work.\nPractice is when something works, but you don't know why.\nProgrammers combine theory and practice: nothing works and they don't know why.")},
	{TEXT("Anonymous #136"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Documentation is like sex:\nwhen it's good, it's very, very good;\nwhen it's bad, it's better than nothing.")},
	{TEXT("Anonymous #137"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Home is where you poop most comfortably.")},
	{TEXT("Anonymous #138"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Laptop Speakers problem: too quiet for music, too loud for porn.")},
	{TEXT("Anonymous #139"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Chinese food to go: $16\nGas to go get the food: $2\nDrove home just to realize they forgot one of your containers: RICELESS")},
	{TEXT("Anonymous #140"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("MS Windows is like religion to most people: they are born into it, accept it as default, never consider switching to another.")},
	{TEXT("Anonymous #141"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("To most religious people, the holy books are like a software license (EULA).\nNobody actually reads it. They just scroll to the bottom and click \"I agree\".")},
	{TEXT("Anonymous #142"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("You are nothing but a number of days,\nwhenever each day passes then part of you has gone.")},
	{TEXT("Anonymous #143"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If 666 is evil, does that make 25.8069758011 the root of all evil?")},
	{TEXT("Anonymous #144"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I don't want to sound like a badass but\nI eject my USB drive without removing it safely.")},
	{TEXT("Anonymous #145"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("feet  (noun)\na device used for finding legos in the dark")},
	{TEXT("Anonymous #146"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Buy a sheep\nName it \"Relation\"\nNow you have a Relationsheep\n")},
	{TEXT("Anonymous #147"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I dig, you dig, we dig,\nhe dig, she dig, they dig...\n\nIt's not a beautiful poem,\nbut it's very deep.")},
	{TEXT("Anonymous #148"), QuoteParams::rapid, false, SC_CP_UTF8, L_BASH, TEXT("# UNIX command line Russian roulette:\n[ $[ $RANDOM % 6 ] == 0 ] && rm -rf /* || echo *Click*\n")},
	{TEXT("Anonymous #149"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("unzip, strip, top, less, touch, finger, grep, mount, fsck, more, yes, fsck, fsck, fsck, umount, sleep.\n\nNo, it's not porn. It's Unix.")},
	{TEXT("Anonymous #150"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("To understand what recursion is, you must first understand recursion.")},
	{TEXT("Anonymous #151"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Q: What's the object-oriented way to become wealthy?\nA: Inheritance.")},
	{TEXT("Anonymous #152"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A SQL query goes into a bar, walks up to two tables and asks, \"Can I join you?\"")},
	{TEXT("Anonymous #153"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("You are not fat, you are just more visible.")},
	{TEXT("Anonymous #154"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Minimalist\n (.   .)\n  )   (\n (  Y  )\nASCII Art")},
	{TEXT("Anonymous #155"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Parallel lines have so much in common...\nIt's a shame that they'll never meet.")},
	{TEXT("Anonymous #156"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Declare variables, not war.\nExecute programs, not people.")},
	{TEXT("Anonymous #157"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("I can't see an end. I have no control and I don't think there's any escape. I don't even have a home anymore.\nI think it's time for a new keyboard.")},
	{TEXT("Anonymous #158"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("6.9\nA little fun interrupted by a period.")},
	{TEXT("Anonymous #159"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("I love anal\n-yzing all data before making assumptions.")},
	{TEXT("Anonymous #160"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("So my wife said\n\"take off my shirt\".\nI did as she said and take off her shirt.\nThen she said,\n\"Take off my skirt.\"\nI took off her skirt.\n\"Take off my shoes.\"\nI took off her shoes.\n\"Now take off my bra and panties.\"\nand so I took them off.\nThen she looked at me and said\n\"I don't want to catch you wearing my things ever again.\"")},
	{TEXT("Anonymous #161"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Do you know:\nSpiders are the only web developers in the world that enjoy finding bugs.") },
	{TEXT("Anonymous #162"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Psychologist: Lie down please.\n8: No, thank you. If I do, this session will never reach the end.") },
	{TEXT("Anonymous #163"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("I love the way the earth rotates,\nit really makes my day.") },
	{TEXT("Anonymous #164"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Homonyms are a waist of thyme.") },
	{TEXT("Anonymous #165"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("What's the difference between a police officer and a bullet?\nWhen a bullet kills someone else, you know it's been fired.") },
	{TEXT("Anonymous #166"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("What has 4 letters\nsometimes 9 letters\nbut never has 5 letters") },
	{TEXT("Anonymous #167"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("The 'h' in \"software development\" stands for \"happiness\".") },
	{TEXT("Anonymous #168"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Never let your computer know that you are in a hurry.\nComputers can smell fear.\nThey slow down if they know that you are running out of time.") },
	{TEXT("Anonymous #169"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("JavaScript is not a language.\nIt's a programming jokes generator.") },
	{TEXT("Anonymous #170"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("A journalist asked Linus Torvalds what makes code bad.\nHe replied : No comment.") },
	{TEXT("Anonymous #171"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If you spell \"Nothing\" backwards, it becomes \"Gnihton\" which also means nothing.") },
	{TEXT("Anonymous #172"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Programming in Javascript is like looking both ways before you cross the street, and then getting hit by an airplane.") },
	{TEXT("Anonymous #173"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Q: Why do we need a backend, why not just connect front end to database???\n\nA: Yeah! And why do we eat and go to the bathroom while we can throw the food directly in the toilet? Because stuff needs to get processed. ;)\n") },
	{TEXT("Anonymous #174"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Someday, once humans are extinct from covid-19. I hope whatever species rules Earth makes chicken nuggets in the shape of us, like we did for dinosaurs.") },
	{TEXT("Anonymous #175"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Linkedin is basically a reversed Tinder.\nHot girls write to nerd guys and they didn't reply.") },
	{TEXT("Anonymous #176"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("A vegan said to me, \"people who sell meat are gross!\"\nI said, \"people who sell fruits and vegetables are grocer.\"\n") },
	{TEXT("Anonymous #177"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Documentation is a love letter that you write to your future self.\n") },
	{TEXT("Anonymous #178"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("When I die, I hope it's early in the morning so I don't have to go to work that day for no reason.\n") },
	{TEXT("Anonymous #179"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Workers plaay football\nManagers play tennis\nCEOs play golf\n\nHigher the function, smaller the balls.\n") },
	{TEXT("Anonymous #180"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Friends are just like trees.\nThey fall down when you hit them multiple times with an axe.\n") },
	{TEXT("Anonymous #181"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("I met a magical Genie. He gave me one wish.\nI said: \"I wish I could be you.\"\nThe Genue saud: \"Weurd wush but U wull grant ut.\"\n") },
	{TEXT("Anonymous #182"), QuoteParams::slow, false, SC_CP_UTF8, L_CPP, TEXT("printf(\"%s%s\", \"\\\\o/\\n| |\\n| |8=\", \"=D\\n/ \\\\\\n\");\n") },
	{TEXT("Anonymous #183"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Dear Optimist, Pessimist and Realist,\n\nWhile you guys were busy arguing about\nthe glass of water, I drank it!\n\n\n					Sincerely,\n					The Opportunist\n") },
	{TEXT("Anonymous #184"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Dance like nobody's watching.\nEncrypt like everyone is.\n") },
	{TEXT("Anonymous #185"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Me: \"I'm 45 years old but I've got a 19 year-old young man's body\"\nHer: \"Show me\"\nI opened the freezer to show her the body.\nShe screamed.\nMe too.\n") },
	{TEXT("Anonymous #186"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Everyone complains about the weather,\nbut no one wants to sacrifice a virgin to change it.\n") },
	{TEXT("Anonymous #187"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("If you are alone at home and feel lonely:\nTurn off the lights, turn on the TV and watch a horror movie.\nThen you will have feeling that there are someone hidden in the kitchen, in the toilet\nand even under your bed.\n") },
	{TEXT("Anonymous #188"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("*Knock knock*\n\"Race condition\"\n\"Who's there?\"\n") },
	{TEXT("Anonymous #189"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("The two most difficult programming challenges are naming things, cache invalidation, and off-by-one errors.\n") },
	{TEXT("Anonymous #190"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("The greatest security vulnerability in any computer system is located between the keyboard and the chair.\n") },
	{TEXT("Anonymous #191"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("My biggest talent is always being able to tell what's in a wrapped present.\n\nIt's a gift.\n") },
	{TEXT("Anonymous #192"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("You can't force someone to love you.\nBut you can lock this person in the basement and wait for him/her to develop Stockholm syndrome.\n") },
	{TEXT("Anonymous #193"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Do you know:\nthere are more airplanes in the oceans, than submarines in the sky?\n") },
	{TEXT("Anonymous #194"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("If you hold a Unix shell up to your ear,\nyou might just be able to hear the C.\n") },
	{TEXT("xkcd"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Never have I felt so close to another soul\nAnd yet so helplessly alone\nAs when I Google an error\nAnd there's one result\nA thread by someone with the same problem\nAnd no answer\nLast posted to in 2003\n\n\"Who were you, DenverCoder9?\"\n\"What did you see?!\"\n\n(ref: https://xkcd.com/979/)") },
	{TEXT("A developer"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("No hugs & kisses.\nOnly bugs & fixes.") },
	{TEXT("Elon Musk"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Don't set your password as your child's name.\nName your child after your password.") },
	{TEXT("OOP"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("If you want to treat women as objects,\ndo it with class.")},
	{TEXT("Internet #404"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Quote not Found")},
	{TEXT("Mary Oliver"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Someone I loved once gave me a box full of darkness.\nIt took me years to understand that this, too, was a gift.")},
	{TEXT("Floor"), QuoteParams::slow, true, SC_CP_UTF8, L_TEXT, TEXT("If you fall, I will be there.")},
	{TEXT("Simon Amstell"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("If you have some problem in your life and need to deal with it, then use religion, that's fine.\nI use Google.")},
	{TEXT("Albert Einstein"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Only 3 things are infinite:\n1. Universe.\n2. Human Stupidity.\n3. Winrar's free trial.")},
	{TEXT("Terry Pratchett"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Artificial Intelligence is no match for natural stupidity.")},
	{TEXT("Stewart Brand"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Once a new technology starts rolling, if you're not part of the steamroller,\nyou're part of the road.")},
	{TEXT("Sam Redwine"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Software and cathedrals are much the same - first we build them, then we pray.")},
	{TEXT("Jan L. A. van de Snepscheut"),  QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("In theory, there is no difference between theory and practice. But, in practice, there is.")},
	{TEXT("Jessica Gaston"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("One man's crappy software is another man's full time job.")},
	{TEXT("Raymond Devos"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Mon pied droit est jaloux de mon pied gauche. Quand l'un avance, l'autre veut le dépasser.\nEt moi, comme un imbécile, je marche !")},
	{TEXT("xkcd.com"), QuoteParams::rapid, false, SC_CP_UTF8, L_C, TEXT("int getRandomNumber()\n{\n    return 4; //chosen by fair dice roll, guaranteed to be random.\n}\n")},
	{TEXT("Gandhi"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Earth provides enough to satisfy every man's need, but not every man's greed.")},
	{TEXT("R. D. Laing"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Life is a sexually transmitted disease and the mortality rate is one hundred percent.")},
	{TEXT("Hustle Man"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Politicians are like sperm.\nOne in a million turn out to be an actual human being.")},
	{TEXT("Mark Twain"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Censorship is telling a man he can't have a steak just because a baby can't chew it.")},
	{TEXT("Friedrich Nietzsche"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("There is not enough love and goodness in the world to permit giving any of it away to imaginary beings.")},
	{TEXT("Dhalsim"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Pain is a state of mind and I don't mind your pain.")},
	{TEXT("Elie Wiesel"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Human beings can be beautiful or more beautiful,\nthey can be fat or skinny, they can be right or wrong,\nbut illegal? How can a human being be illegal?")},
	{TEXT("Dennis Ritchie"), QuoteParams::rapid, true, SC_CP_UTF8, L_TEXT, TEXT("Empty your memory, with a free(), like a pointer.\nIf you cast a pointer to a integer, it becomes the integer.\nIf you cast a pointer to a struct, it becomes the struct.\nThe pointer can crash, and can overflow.\nBe a pointer my friend.")},
	{TEXT("Chewbacca"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Uuuuuuuuuur Ahhhhrrrrrr\nUhrrrr Ahhhhrrrrrr\nAaaarhg...")},
	{TEXT("Alexandria Ocasio-Cortez"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("No one ever makes a billion dollars.\nYou TAKE a billion dollars.")},
	{TEXT("Freddy Krueger"), QuoteParams::slow, false, SC_CP_UTF8, L_TEXT, TEXT("Never stop dreaming.\n")},
	{TEXT("Ricky Gervais"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Feel free to mock my lack of belief in any Gods.\nIt won't hurt my feelings.\nIt won't damage my faith in reason.\nAnd I won't kill you for it.")},
	{TEXT("Francis bacon"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Knowledge is power. France is bacon.\n\nWhen I was young my father said to me: \"Knowledge is power, Francis Bacon.\" I understood it as \"Knowledge is power, France is bacon.\"\n\nFor more than a decade I wondered over the meaning of the second part and what was the surreal linkage between the two. If I said the quote to someone, \"Knowledge is power, France is Bacon\", they nodded knowingly. Or someone might say, \"Knowledge is power\" and I'd finish the quote \"France is Bacon\" and they wouldn't look at me like I'd said something very odd, but thoughtfully agree. I did ask a teacher what did \"Knowledge is power, France is bacon\" mean and got a full 10-minute explanation of the \"knowledge is power\" bit but nothing on \"France is bacon\". When I prompted further explanation by saying \"France is bacon?\" in a questioning tone I just got a \"yes\". At 12 I didn't have the confidence to press it further. I just accepted it as something I'd never understand.\n\nIt wasn't until years later I saw it written down that the penny dropped.\n")},
	{TEXT("Space Invaders"), QuoteParams::speedOfLight, false, SC_CP_UTF8, L_TEXT, TEXT("\n\n       ▄██▄\n     ▄██████▄           █   █  █▀▀▀\n     ██▄██▄██           █   █  █▄▄\n      ▄▀▄▄▀▄            █ █ █  █\n     ▀ ▀  ▀ ▀           ▀▀ ▀▀  ▀▀▀▀\n\n      ▀▄   ▄▀           ▄█▀▀▀  ▄█▀▀█▄  █▀▄▀█  █▀▀▀\n     ▄█▀███▀█▄          █      █    █  █ ▀ █  █▄▄\n    █ █▀▀▀▀▀█ █         █▄     █▄  ▄█  █   █  █\n       ▀▀ ▀▀             ▀▀▀▀   ▀▀▀▀   ▀   ▀  ▀▀▀▀\n\n     ▄▄█████▄▄          ▀█▀  █▀▄  █\n    ██▀▀███▀▀██          █   █ ▀▄ █\n    ▀▀██▀▀▀██▀▀          █   █  ▀▄█\n    ▄█▀ ▀▀▀ ▀█▄         ▀▀▀  ▀   ▀▀\n\n      ▄▄████▄▄          █▀▀█  █▀▀▀  ▄▀▀▄  ▄█▀▀▀  █▀▀▀\n    ▄██████████▄        █▄▄█  █▄▄   █▄▄█  █      █▄▄ \n  ▄██▄██▄██▄██▄██▄      █     █     █  █  █▄     █   \n    ▀█▀  ▀▀  ▀█▀        ▀     ▀▀▀▀  ▀  ▀   ▀▀▀▀  ▀▀▀▀\n\n") },
	{TEXT("#JeSuisCharlie"), QuoteParams::rapid, false, SC_CP_UTF8, L_TEXT, TEXT("Freedom of expression is like the air we breathe, we don't feel it, until people take it away from us.\n\nFor this reason, Je suis Charlie, not because I endorse everything they published, but because I cherish the right to speak out freely without risk even when it offends others.\nAnd no, you cannot just take someone's life for whatever he/she expressed.\n\nHence this \"Je suis Charlie\" edition.\n")}
};



const int nbWtf = 5;
const wchar_t* wtf[nbWtf] =
{
	TEXT("WTF?!"),
	TEXT("lol"),
	TEXT("ROFL"),
	TEXT("OMFG"),
	TEXT("Husband is not an ATM machine!!!")
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
	srand(static_cast<UINT>(time(NULL)));

	TextPlayerParams* textPlayerParams = static_cast<TextPlayerParams*>(params);
	HWND hNpp = textPlayerParams->_nppHandle;
	ScintillaEditView *pCurrentView = textPlayerParams->_pCurrentView;
	QuoteParams* qParams = textPlayerParams->_quotParams;
	const wchar_t* text2display = qParams->_quote;
	bool shouldBeTrolling = qParams->_shouldBeTrolling;

	// Open a new document
    ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

	if (qParams->_encoding == SC_CP_UTF8)
		::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FORMAT_AS_UTF_8);
	else if (qParams->_encoding >= 0)
		pCurrentView->execute(SCI_SETCODEPAGE, qParams->_encoding);

	int langMenuId = (NppParameters::getInstance()).langTypeToCommandID(qParams->_lang);
	::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, langMenuId);

	int x = 2, y = 1;
	if (qParams->_speed == QuoteParams::slow)
	{
		x = 1;
		y = 1;
	}
	else if (qParams->_speed == QuoteParams::rapid)
	{
		x = 2;
		y = 1;
	}
	else if (qParams->_speed == QuoteParams::speedOfLight)
	{
		x = 1;
		y = 0;
	}

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
	wchar_t previousChar = '\0';

	for (size_t i = 0, len = lstrlen(text2display); i < len ; ++i)
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

				int sleepTime = 1000 / x * y;
				::Sleep(sleepTime);

				WaitForSingleObject(mutex, INFINITE);

				::CloseHandle(hThread);
				//writeLog(TEXT("c:\\tmp\\log.txt"), "trolling end");
			}
		}


		if (text2display[i] == ' ' || text2display[i] == '.')
		{
			int sleepTime = (ranNum + pauseTimeArray[ranNum%nbPauseTime]) / x * y;
			Sleep(sleepTime);
		}
		else
		{
			int sleepTime = (ranNum + intervalTimeArray[ranNum%nbIntervalTime]) / x * y;
			Sleep(sleepTime);
		}

		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
			return TRUE;

		char charToShow[4] = { '\0' };
		::WideCharToMultiByte(CP_UTF8, 0, text2display + i, 1, charToShow, sizeof(charToShow), NULL, NULL);
		::SendMessage(curScintilla, SCI_APPENDTEXT, strlen(charToShow), reinterpret_cast<LPARAM>(charToShow));
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);

		previousChar = text2display[i];
		//char ch[64];
		//sprintf(ch, "writing char == %c", text2display[i]);
		//writeLog(TEXT("c:\\tmp\\log.txt"), ch);
    }

	//writeLog(TEXT("c:\\tmp\\log.txt"), "\n\n\n\n");
	const wchar_t* quoter = qParams->_quoter;
	wstring quoter_str = quoter;
	size_t pos = quoter_str.find(TEXT("Anonymous"));
	if (pos == string::npos)
	{
		::SendMessage(curScintilla, SCI_APPENDTEXT, 3, reinterpret_cast<LPARAM>("\n- "));
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);

		// Display quoter
		for (size_t i = 0, len = lstrlen(quoter); i < len; ++i)
		{
			int ranNum = getRandomNumber(maxRange);

			int sleepTime = (ranNum + intervalTimeArray[ranNum%nbIntervalTime]) / x * y;
			Sleep(sleepTime);

			BufferID currentBufID = pCurrentView->getCurrentBufferID();
			if (currentBufID != targetBufID)
				return TRUE;

			char charToShow[4] = { '\0' };
			::WideCharToMultiByte(CP_UTF8, 0, quoter + i, 1, charToShow, sizeof(charToShow), NULL, NULL);

			::SendMessage(curScintilla, SCI_APPENDTEXT, 1, reinterpret_cast<LPARAM>(charToShow));
			::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);

		}
	}

    return TRUE;
}


DWORD WINAPI Notepad_plus::threadTextTroller(void *params)
{
	TextTrollerParams *textTrollerParams = static_cast<TextTrollerParams *>(params);
	WaitForSingleObject(textTrollerParams->_mutex, INFINITE);

	// random seed generation needs only one time.
	srand(static_cast<UINT>(time(NULL)));

	ScintillaEditView *pCurrentView = textTrollerParams->_pCurrentView;
	const wchar_t* text2display = textTrollerParams->_text2display;
	HWND curScintilla = pCurrentView->getHSelf();
	BufferID targetBufID = textTrollerParams->_targetBufID;

	for (size_t i = 0, len = lstrlen(text2display); i < len; ++i)
    {
        int ranNum = getRandomNumber(maxRange);
		if (text2display[i] == ' ' || text2display[i] == '.')
			Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		else
			Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);

		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
		{
			ReleaseMutex(textTrollerParams->_mutex);
			return TRUE;
		}

		char charToShow[64] = { '\0' };
		::WideCharToMultiByte(CP_UTF8, 0, text2display + i, 1, charToShow, sizeof(charToShow), NULL, NULL);
		::SendMessage(curScintilla, SCI_APPENDTEXT, 1, reinterpret_cast<LPARAM>(charToShow));
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);
    }
	//writeLog(TEXT("c:\\tmp\\log.txt"), text2display);
	int n = getRandomNumber();
	int delMethod = n%4;
	if (delMethod == 0)
	{
		size_t len = lstrlen(text2display);
		for (size_t j = 0; j < len; ++j)
		{
			if (!deleteBack(pCurrentView, targetBufID))
				break;
		}
	}
	else if (delMethod == 1)
	{
		size_t len = lstrlen(text2display);
		::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0) - len, 0);
		for (size_t j = 0; j < len; ++j)
		{
			if (!deleteForward(pCurrentView, targetBufID))
				break;
		}
	}
	else if (delMethod == 2)
	{
		for (size_t j = 0, len = lstrlen(text2display); j < len; ++j)
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
		auto currentPos = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONSTART, 0, 0);
		::SendMessage(pCurrentView->getHSelf(), SCI_SETSELECTION, currentPos, currentPos - lstrlen(text2display));
		BufferID currentBufID = pCurrentView->getCurrentBufferID();
		if (currentBufID != targetBufID)
			return TRUE;
		int ranNum = getRandomNumber(maxRange);
		::Sleep(ranNum + pauseTimeArray[ranNum%nbPauseTime]);
		::SendMessage(pCurrentView->getHSelf(), SCI_DELETEBACK, 0, 0);
	}

	ReleaseMutex(textTrollerParams->_mutex);
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
	auto currentPos = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONSTART, 0, 0);
	auto currentAnchor = ::SendMessage(pCurrentView->getHSelf(), SCI_GETSELECTIONEND, 0, 0);
	Sleep(ranNum + intervalTimeArray[ranNum%nbIntervalTime]);
	if (currentBufID != targetBufID)
		return false;

	::SendMessage(pCurrentView->getHSelf(), SCI_SETSELECTION, currentAnchor, --currentPos);
	return true;
}


int Notepad_plus::getQuoteIndexFrom(const wchar_t* quoter) const
{
	if (!quoter)
		return -1;

	if (wcsicmp(quoter, TEXT("Get them all!!!")) == 0)
		return -2;

	int nbQuote = sizeof(quotes) / sizeof(QuoteParams);
	if (wcsicmp(quoter, TEXT("random")) == 0)
	{
		srand(static_cast<UINT>(time(NULL)));
		return getRandomNumber(nbQuote);
	}

	for (int i = 0; i < nbQuote; ++i)
	{
		if (wcsicmp(quotes[i]._quoter, quoter) == 0)
			return i;
	}
	return -1;
}


void Notepad_plus::showAllQuotes() const
{
}


void Notepad_plus::showQuoteFromIndex(int index) const
{
	int nbQuote = sizeof(quotes) / sizeof(QuoteParams);
	if (index < 0 || index >= nbQuote) return;
	showQuote(&quotes[index]);
}

void Notepad_plus::showQuote(const QuoteParams* quote) const
{
	static TextPlayerParams params;
	params._quotParams = const_cast<QuoteParams*>(quote);
	params._nppHandle = Notepad_plus::_pPublicInterface->getHSelf();
	params._pCurrentView = _pEditView;

	HANDLE hThread = ::CreateThread(NULL, 0, threadTextPlayer, &params, 0, NULL);
	if (hThread)
		::CloseHandle(hThread);
}

void Notepad_plus::minimizeDialogs()
{
	static StaticDialog* modelessDlgs[] = {&_findReplaceDlg, &_aboutDlg, &_debugInfoDlg, &_runDlg, &_goToLineDlg, &_colEditorDlg, &_configStyleDlg,\
		&_preference, &_pluginsAdminDlg, &_findCharsInRangeDlg, &_md5FromFilesDlg, &_md5FromTextDlg, &_sha2FromFilesDlg, &_sha2FromTextDlg, &_runMacroDlg};
	
	static size_t nbModelessDlg = sizeof(modelessDlgs) / sizeof(StaticDialog*);

	for (size_t i = 0; i < nbModelessDlg; ++i)
	{
		StaticDialog* pDlg = modelessDlgs[i];
		if (pDlg->isCreated() && pDlg->isVisible())
		{
			pDlg->display(false);
			_sysTrayHiddenHwnd.push_back(pDlg->getHSelf());
		}
	}
}

void Notepad_plus::restoreMinimizeDialogs()
{
	size_t nbDialogs = _sysTrayHiddenHwnd.size();
	for (int i = (static_cast<int>(nbDialogs) - 1); i >= 0; i--)
	{
		::ShowWindow(_sysTrayHiddenHwnd[i], SW_SHOW);
		_sysTrayHiddenHwnd.erase(_sysTrayHiddenHwnd.begin() + i);
	}
}

void Notepad_plus::refreshDarkMode(bool resetStyle)
{
	if (resetStyle)
	{
		NppParameters& nppParams = NppParameters::getInstance();

		::SendMessage(_pPublicInterface->getHSelf(), NPPM_SETEDITORBORDEREDGE, 0, nppParams.getSVP()._showBorderEdge);

		::SendMessage(_subEditView.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		::SendMessage(_mainEditView.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);

		::SendMessage(_mainDocTab.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		::SendMessage(_subDocTab.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);

		::SendMessage(_findInFinderDlg.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		::RedrawWindow(_findInFinderDlg.getHSelf(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

		::SendMessage(_incrementFindDlg.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		::RedrawWindow(_pPublicInterface->getHSelf(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

		if (_pProjectPanel_1)
		{
			::SendMessage(_pProjectPanel_1->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}
		if (_pProjectPanel_2)
		{
			::SendMessage(_pProjectPanel_2->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}
		if (_pProjectPanel_3)
		{
			::SendMessage(_pProjectPanel_3->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}
		if (_pFuncList)
		{
			::SendMessage(_pFuncList->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}
		if (_pFileBrowser)
		{
			::SendMessage(_pFileBrowser->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}

		if (_pAnsiCharPanel)
		{
			::SendMessage(_pAnsiCharPanel->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}
		if (_pDocumentListPanel)
		{
			::SendMessage(_pDocumentListPanel->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}

		if (_pClipboardHistoryPanel)
		{
			::SendMessage(_pClipboardHistoryPanel->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
		}

		bool isChecked = _preference._generalSubDlg.isCheckedOrNot(IDC_CHECK_TAB_ALTICONS);
		if (!isChecked)
		{
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_CHANGETABBAEICONS, 0, NppDarkMode::isEnabled() ? 2 : 0);
		}

		toolBarStatusType state = _toolBar.getState();
		switch (state)
		{
			case TB_SMALL:
				_toolBar.reduce();
				break;

			case TB_LARGE:
				_toolBar.enlarge();
				break;

			case TB_SMALL2:
				_toolBar.reduceToSet2();
				break;

			case TB_LARGE2:
				_toolBar.enlargeToSet2();
				break;

			case TB_STANDARD:
				// Force standard colorful icon to Fluent UI small icon in dark mode
				if (NppDarkMode::isEnabled())
					_toolBar.reduce();
				break;
		}

		ThemeSwitcher& themeSwitcher = nppParams.getThemeSwitcher();
		generic_string themePath;
		generic_string themeName;
		const TCHAR darkModeXmlFileName[] = TEXT("DarkModeDefault.xml");
		if (NppDarkMode::isEnabled())
		{
			themePath = themeSwitcher.getThemeDirPath();
			pathAppend(themePath, darkModeXmlFileName);

			themeName = themeSwitcher.getThemeFromXmlFileName(themePath.c_str());
		}
		else
		{
			//use _stylerPath;

			pair<generic_string, generic_string>& themeInfo = themeSwitcher.getElementFromIndex(0);
			themePath = themeInfo.second;
			themeName = themeSwitcher.getDefaultThemeLabel();
		}

		if (::PathFileExists(themePath.c_str()))
		{
			nppParams.getNppGUI()._themeName = themePath;

			if (_configStyleDlg.isCreated())
			{
				_configStyleDlg.selectThemeByName(themeName.c_str());
			}
			else
			{
				nppParams.reloadStylers(themePath.c_str());
				::SendMessage(_pPublicInterface->getHSelf(), WM_UPDATESCINTILLAS, 0, 0);
			}
		}

		if (NppDarkMode::isExperimentalSupported())
		{
			NppDarkMode::allowDarkModeForApp(NppDarkMode::isEnabled());

			NppDarkMode::setDarkTitleBar(_pPublicInterface->getHSelf());
			::SetWindowPos(_pPublicInterface->getHSelf(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			for (auto& docCont : _dockingManager.getContainerInfo())
			{
				auto hwndDocCont = docCont->getCaptionWnd();
				NppDarkMode::setDarkTitleBar(hwndDocCont);
				::SetWindowPos(hwndDocCont, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			for (auto& hwndDlg : _hModelessDlgs)
			{
				NppDarkMode::setDarkTitleBar(hwndDlg);
				::SendMessage(hwndDlg, NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
				::RedrawWindow(hwndDlg, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
				::SetWindowPos(hwndDlg, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			NppDarkMode::setDarkTitleBar(_findInFinderDlg.getHSelf());
			::SetWindowPos(_findInFinderDlg.getHSelf(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		for (auto& docCont : _dockingManager.getContainerInfo())
		{
			auto hwndDocCont = docCont->getCaptionWnd();
			::RedrawWindow(hwndDocCont, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
		}

		for (auto& hwndDlg : _hModelessDlgs)
		{
			//::SendMessage(hwndDlg, NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			::RedrawWindow(hwndDlg, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
		}

		::RedrawWindow(_findInFinderDlg.getHSelf(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
		::RedrawWindow(_pPublicInterface->getHSelf(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
	}
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
		NppParameters& nppParam = NppParameters::getInstance();

		size_t timer = nppParam.getNppGUI()._snapshotBackupTiming;
		if (timer < 1000)
			timer = 1000;

		::Sleep(DWORD(timer));

		isSnapshotMode = nppParam.getNppGUI().isSnapshotMode();
		if (!isSnapshotMode)
			break;

		::PostMessage(Notepad_plus_Window::gNppHWND, NPPM_INTERNAL_SAVEBACKUP, 0, 0);
	}
	return TRUE;
}



#pragma warning( disable : 4127 )
//-- undoStreamComment: New function to undo stream comment around or within selection end-points.
bool Notepad_plus::undoStreamComment(bool tryBlockComment)
{
	const TCHAR *commentStart;
	const TCHAR *commentEnd;
	const TCHAR *commentLineSymbol;

	generic_string symbolStart;
	generic_string symbolEnd;
	generic_string symbol;

	const int charbufLen = 10;
    TCHAR charbuf[charbufLen];

	bool retVal = false;

	Buffer * buf = _pEditView->getCurrentBuffer();
	//-- Avoid side-effects (e.g. cursor moves number of comment-characters) when file is read-only.
	if (buf->isReadOnly())
		return false;
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance().getULCFromName(buf->getUserDefineLangName());
		if (!userLangContainer)
			return false;

		symbol = extractSymbol('0', '0', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentLineSymbol = symbol.c_str();
		symbolStart = extractSymbol('0', '3', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('0', '4', userLangContainer->_keywordLists[SCE_USER_KWLIST_COMMENTS]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentLineSymbol = buf->getCommentLineSymbol();
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}


	// BlockToStreamComment: If there is no stream-comment symbol and we came not from doBlockComment, try the block comment:
	if ((!commentStart) || (!commentStart[0]) || (commentStart == NULL) || (!commentEnd) || (!commentEnd[0]) || (commentEnd == NULL))
	{
		if (!(!commentLineSymbol || !commentLineSymbol[0] || commentLineSymbol == NULL) && tryBlockComment)
			return doBlockComment(cm_uncomment);
		else
			return false;
	}

	generic_string start_comment(commentStart);
	generic_string end_comment(commentEnd);
	generic_string white_space(TEXT(" "));
	size_t start_comment_length = start_comment.length();
	size_t end_comment_length = end_comment.length();

	// do as long as stream-comments are within selection
	do
	{
		auto selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
		auto selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
		auto caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
		auto docLength = _pEditView->execute(SCI_GETLENGTH);

		// checking if caret is located in _beginning_ of selected block
		bool move_caret = caretPosition < selectionEnd;

		//-- Note: The caretPosition is either at selectionEnd or at selectionStart!! selectionStart is always before (smaller) than selectionEnd!!

		//-- First, search all start_comment and end_comment before and after the selectionStart and selectionEnd position.
		const int iSelStart=0, iSelEnd=1;
		const size_t N_CMNT = 2;
		intptr_t posStartCommentBefore[N_CMNT], posEndCommentBefore[N_CMNT], posStartCommentAfter[N_CMNT], posEndCommentAfter[N_CMNT];
		bool blnStartCommentBefore[N_CMNT], blnEndCommentBefore[N_CMNT], blnStartCommentAfter[N_CMNT], blnEndCommentAfter[N_CMNT];
		intptr_t posStartComment, posEndComment;
		intptr_t selectionStartMove, selectionEndMove;
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

		//-- First, check if there is a stream-comment around the selectionStart position:
		if ((blnStartCommentBefore[iSelStart] && blnEndCommentAfter[iSelStart])
			&& (!blnEndCommentBefore[iSelStart] || (posStartCommentBefore[iSelStart] >= posEndCommentBefore[iSelStart]))
			&& (!blnStartCommentAfter[iSelStart] || (posEndCommentAfter[iSelStart] <= posStartCommentAfter[iSelStart])))
		{
				posStartComment = posStartCommentBefore[iSelStart];
				posEndComment   = posEndCommentAfter[iSelStart];
		}
		else //-- Second, check if there is a stream-comment around the selectionEnd position:
		{
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
				&& (!blnStartCommentAfter[iSelEnd] || (posEndCommentAfter[iSelEnd] <= posStartCommentAfter[iSelEnd])))
			{
					posStartComment = posStartCommentBefore[iSelEnd];
					posEndComment   = posEndCommentAfter[iSelEnd];
			}
			//-- Third, check if there is a stream-comment within the selected area:
			else if ( (blnStartCommentAfter[iSelStart] && (posStartCommentAfter[iSelStart] < selectionEnd))
				&& (blnEndCommentBefore[iSelEnd] && (posEndCommentBefore[iSelEnd] >  selectionStart)))
			{
					//-- If there are more than one stream-comment within the selection, take the first one after selectionStart!!
					posStartComment = posStartCommentAfter[iSelStart];
					posEndComment   = posEndCommentAfter[iSelStart];
			}
			//-- Finally, if there is no stream-comment, return
			else
				return retVal;
		}

		//-- Ok, there are valid start-comment and valid end-comment around the caret-position.
		//   Now, un-comment stream-comment:
		retVal = true;
		intptr_t startCommentLength = start_comment_length;
		intptr_t endCommentLength = end_comment_length;

		//-- First delete end-comment, so that posStartCommentBefore does not change!
		//-- Get character before end-comment to decide, if there is a white character before the end-comment, which will be removed too!
		_pEditView->getGenericText(charbuf, charbufLen, posEndComment-1, posEndComment);
		if (generic_strncmp(charbuf, white_space.c_str(), white_space.length()) == 0)
		{
			endCommentLength +=1;
			posEndComment-=1;
		}
		//-- Delete end stream-comment string ---------
		_pEditView->execute(SCI_BEGINUNDOACTION);
		_pEditView->execute(SCI_SETSEL, posEndComment, posEndComment + endCommentLength);
		_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));

		//-- Get character after start-comment to decide, if there is a white character after the start-comment, which will be removed too!
		_pEditView->getGenericText(charbuf, charbufLen, posStartComment+startCommentLength, posStartComment+startCommentLength+1);
		if (generic_strncmp(charbuf, white_space.c_str(), white_space.length()) == 0)
			startCommentLength +=1;

		//-- Delete starting stream-comment string ---------
		_pEditView->execute(SCI_SETSEL, posStartComment, posStartComment + startCommentLength);
		_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
		_pEditView->execute(SCI_ENDUNDOACTION);

		//-- Reset selection before calling the routine
		//-- Determine selection movement
		//   selectionStart
		if (selectionStart > posStartComment)
		{
			if (selectionStart >= posStartComment+startCommentLength)
				selectionStartMove = -startCommentLength;
			else
				selectionStartMove = -selectionStart - posStartComment;
		}
		else
			selectionStartMove = 0;

		//   selectionEnd
		if (selectionEnd >= posEndComment+endCommentLength)
			selectionEndMove = -startCommentLength+endCommentLength;
		else if (selectionEnd <= posEndComment)
			selectionEndMove = -startCommentLength;
		else
			selectionEndMove = -startCommentLength + (selectionEnd - posEndComment);

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
	}
	while (1); //do as long as stream-comments are within selection
}

void Notepad_plus::monitoringStartOrStopAndUpdateUI(Buffer* pBuf, bool isStarting)
{
	if (pBuf)
	{
		if (isStarting)
			pBuf->startMonitoring();
		else
			pBuf->stopMonitoring();

		checkMenuItem(IDM_VIEW_MONITORING, isStarting);
		_toolBar.setCheck(IDM_VIEW_MONITORING, isStarting);
		pBuf->setUserReadOnly(isStarting);
	}
}

void Notepad_plus::createMonitoringThread(Buffer* pBuf)
{
	MonitorInfo *monitorInfo = new Notepad_plus::MonitorInfo(pBuf, _pPublicInterface->getHSelf());
	HANDLE hThread = ::CreateThread(NULL, 0, monitorFileOnChange, (void *)monitorInfo, 0, NULL); // will be deallocated while quitting thread
	::CloseHandle(hThread);
}

// Fill names into the shortcut list.
// Each command shortcut has two names:
// - The menu name, to be displayed in the menu
// - The shortcut name, to be displayed in the shortcut list
//
// The names are filled in with the following priorities:
// * Menu name
//   1. From xml Menu/Main/Commands section
//   2. From menu resource in Notepad_plus.rc
//   3. From winKeyDefs[] table in Parameter.cpp
//   We don't use xml ShortCutMapper/MainCommandNames here
//
// * Shortcut name
//   1. From xml ShortCutMapper/MainCommandNames section
//   2. From xml file Menu/Main/Commands section
//   3. From the winKeyDefs[] table in Parameter.cpp
//   4. From the menu resource in Notepad_plus.rc

void Notepad_plus::updateCommandShortcuts()
{
	NppParameters& nppParam = NppParameters::getInstance();
	vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
	size_t len = shortcuts.size();

	for (size_t i = 0; i < len; ++i)
	{
		CommandShortcut & csc = shortcuts[i];
		unsigned long id = csc.getID();
		generic_string localizedMenuName = _nativeLangSpeaker.getNativeLangMenuString(id);
		generic_string menuName = localizedMenuName;
		generic_string shortcutName = _nativeLangSpeaker.getShortcutNameString(id);

		if (menuName.length() == 0)
		{
			TCHAR szMenuName[64];
			if (::GetMenuString(_mainMenuHandle, csc.getID(), szMenuName, _countof(szMenuName), MF_BYCOMMAND))
				menuName = purgeMenuItemString(szMenuName, true);
			else
				menuName = csc.getShortcutName();
		}

		if (shortcutName.length() == 0)
		{
			if (localizedMenuName.length() > 0)
				shortcutName = localizedMenuName;
			else if (csc.getShortcutName()[0])
				shortcutName = csc.getShortcutName();
			else
				shortcutName = menuName;
		}

		csc.setName(menuName.c_str(), shortcutName.c_str());
	}
}
