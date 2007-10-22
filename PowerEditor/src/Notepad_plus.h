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

#ifndef NOTEPAD_PLUS_H
#define NOTEPAD_PLUS_H

#include "Window.h"
#include "ScintillaEditView.h"
#include "ToolBar.h"
#include "ImageListSet.h"
#include "DocTabView.h"

#include "StaticDialog.h"
#include "SplitterContainer.h"
#include "FindReplaceDlg.h"
#include "AboutDlg.h"
#include "RunDlg.h"
#include "UserDefineDialog.h"
#include "StatusBar.h"
#include "Parameters.h"
#include "lastRecentFileList.h"
#include "GoToLineDlg.h"
#include "columnEditor.h"
#include "WordStyleDlg.h"
#include "constant.h"
#include "trayIconControler.h"
#include "ContextMenu.h"
#include "PluginsManager.h"
#include "Notepad_plus_msgs.h"
#include "preferenceDlg.h"
#include "WindowsDlg.h"
#include "RunMacroDlg.h"
#include "DockingManager.h"

#define NOTEPAD_PP_CLASS_NAME	"Notepad++"



#define MENU 0x01
#define TOOLBAR 0x02

//#define WM_LOADFILEBYPATH WM_USER

const bool MODE_TRANSFER = true;
const bool MODE_CLONE = false;

const unsigned char DOCK_MASK = 1;
const unsigned char TWO_VIEWS_MASK = 2;

const int blockSize = 128 * 1024 + 4;
struct TaskListInfo;
static TiXmlNode * searchDlgNode(TiXmlNode *node, const char *dlgTagName);
static winVer getWindowsVersion();
struct iconLocator {
	int listIndex;
	int iconIndex;
	std::string iconLocation;

	iconLocator(int iList, int iIcon, const std::string iconLoc) 
		: listIndex(iList), iconIndex(iIcon), iconLocation(iconLoc){};
};

class FileDialog;

class Notepad_plus : public Window {
public:
	Notepad_plus();

	void init(HINSTANCE, HWND, const char *cmdLine, CmdLineParams *cmdLineParams);

	// ATTENTION : the order of the destruction is very important
	// because if the parent's window hadle is destroyed before
	// the destruction of its childrens' windows handle, 
	// its childrens' windows handle will be destroyed automatically!
	virtual ~Notepad_plus(){
		(NppParameters::getInstance())->destroyInstance();
		if (_pTrayIco)
			delete _pTrayIco;
	};

	void killAllChildren() {
		_toolBar.destroy();
		_rebar.destroy();

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
	};

	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};

    static const char * getClassName() {
        return _className;
    };
	
	void setTitleWith(const char *filePath);
	
	void getTaskListInfo(TaskListInfo *tli);

	// For filtering the modeless Dialog message
	bool isDlgsMsg(MSG *msg) const {
		for (size_t i = 0; i < _hModelessDlgs.size(); i++)
		{
			if (::IsDialogMessage(_hModelessDlgs[i], msg))
				return true;
		}

		return false;
	};
    bool doOpen(const char *fileName, bool isReadOnly = false);
	bool doSimpleOpen(const char *fileName);

    bool doReload(const char *fileName, bool alert = true);

	void saveScintillaParams(bool whichOne) {
		ScintillaViewParams svp;
		ScintillaEditView *pView = (whichOne == SCIV_PRIMARY)?&_mainEditView:&_subEditView;

		svp._lineNumberMarginShow = pView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER); 
		svp._bookMarkMarginShow = pView->hasMarginShowed(ScintillaEditView::_SC_MARGE_SYBOLE);
		svp._indentGuideLineShow = pView->isShownIndentGuide();
		svp._folderStyle = pView->getFolderStyle();
		svp._currentLineHilitingShow = pView->isCurrentLineHiLiting();
		svp._wrapSymbolShow = pView->isWrapSymbolVisible();
		svp._doWrap = pView->isWrap();
		svp._edgeMode = int(pView->execute(SCI_GETEDGEMODE));
		svp._edgeNbColumn = int(pView->execute(SCI_GETEDGECOLUMN));
		svp._zoom = int(pView->execute(SCI_GETZOOM));
		svp._whiteSpaceShow = pView->isInvisibleCharsShown();
		svp._eolShow = pView->isEolVisible();
		(NppParameters::getInstance())->writeScintillaParams(svp, whichOne);
	};

	void saveGUIParams(){
		NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();
		nppGUI._statusBarShow = _statusBar.isVisible();
		nppGUI._toolBarStatus = _toolBar.getState();

		nppGUI._tabStatus = (TabBarPlus::doDragNDropOrNot()?TAB_DRAWTOPBAR:0) | \
							(TabBarPlus::drawTopBar()?TAB_DRAGNDROP:0) | \
							(TabBarPlus::drawInactiveTab()?TAB_DRAWINACTIVETAB:0) | \
							(_toReduceTabBar?TAB_REDUCE:0) | \
							(TabBarPlus::drawTabCloseButton()?TAB_CLOSEBUTTON:0) | \
							(TabBarPlus::isDbClk2Close()?TAB_DBCLK2CLOSE:0);
		nppGUI._splitterPos = _subSplitter.isVertical()?POS_VERTICAL:POS_HORIZOTAL;
		UserDefineDialog *udd = _pEditView->getUserDefineDlg();
		bool b = udd->isDocked();
		nppGUI._userDefineDlgStatus = (b?UDD_DOCKED:0) | (udd->isVisible()?UDD_SHOW:0);
		
		// Save the position

		WINDOWPLACEMENT posInfo;

        posInfo.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(_hSelf, &posInfo);

		nppGUI._appPos.left = posInfo.rcNormalPosition.left;
		nppGUI._appPos.top = posInfo.rcNormalPosition.top;
		nppGUI._appPos.right = posInfo.rcNormalPosition.right - posInfo.rcNormalPosition.left;
		nppGUI._appPos.bottom = posInfo.rcNormalPosition.bottom - posInfo.rcNormalPosition.top;
		nppGUI._isMaximized = (IsZoomed(_hSelf) || (posInfo.flags & WPF_RESTORETOMAXIMIZED));

		saveDockingParams();

		(NppParameters::getInstance())->writeGUIParams();
	};

	void saveDockingParams() {
		NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();

		// Save the docking information
		nppGUI._dockingData._leftWidth		= _dockingManager.getDockedContSize(CONT_LEFT);
		nppGUI._dockingData._rightWidth		= _dockingManager.getDockedContSize(CONT_RIGHT); 
		nppGUI._dockingData._topHeight		= _dockingManager.getDockedContSize(CONT_TOP);	 
		nppGUI._dockingData._bottomHight	= _dockingManager.getDockedContSize(CONT_BOTTOM);

		// clear the conatainer tab information (active tab)
		nppGUI._dockingData._containerTabInfo.clear();

		// create a vector to save the current information
		vector<PlugingDlgDockingInfo>	vPluginDockInfo;
		vector<FloatingWindowInfo>		vFloatingWindowInfo;

		// save every container
		vector<DockingCont*> vCont = _dockingManager.getContainerInfo();

		for (size_t i = 0 ; i < vCont.size() ; i++)
		{
			// save at first the visible Tb's
			vector<tTbData *>	vDataVis	= vCont[i]->getDataOfVisTb();

			for (size_t j = 0 ; j < vDataVis.size() ; j++)
			{
				if (vDataVis[j]->pszName && vDataVis[j]->pszName[0])
				{
					PlugingDlgDockingInfo pddi(vDataVis[j]->pszModuleName, vDataVis[j]->dlgID, i, vDataVis[j]->iPrevCont, true);
					vPluginDockInfo.push_back(pddi);
				}
			}

			// save the hidden Tb's
			vector<tTbData *>	vDataAll	= vCont[i]->getDataOfAllTb();

			for (size_t j = 0 ; j < vDataAll.size() ; j++)
			{
				if ((vDataAll[j]->pszName && vDataAll[j]->pszName[0]) && (!vCont[i]->isTbVis(vDataAll[j])))
				{
					PlugingDlgDockingInfo pddi(vDataAll[j]->pszModuleName, vDataAll[j]->dlgID, i, vDataAll[j]->iPrevCont, false);
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
		unsigned char floatContArray[50];
		memset(floatContArray, 0, 50);

		for (size_t i = 0 ; i < nppGUI._dockingData._pluginDockInfo.size() ; i++)
		{
			BOOL	isStored = FALSE;
			for (size_t j = 0; j < vPluginDockInfo.size(); j++)
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
					RECT *pRc = nppGUI._dockingData.getFloatingRCFrom(floatCont);
					if (pRc)
						vFloatingWindowInfo.push_back(FloatingWindowInfo(floatCont, pRc->left, pRc->top, pRc->right, pRc->bottom));
					floatContArray[floatCont] = 1;
				}

				vPluginDockInfo.push_back(nppGUI._dockingData._pluginDockInfo[i]);
			}
		}

		nppGUI._dockingData._pluginDockInfo = vPluginDockInfo;
		nppGUI._dockingData._flaotingWindowInfo = vFloatingWindowInfo;
	};

	void saveUserDefineLangs() {
		if (ScintillaEditView::getUserDefineDlg()->isDirty())
			(NppParameters::getInstance())->writeUserDefinedLang();
	};

	void saveShortcuts() {
		if (_isCmdScModified || _isMacrosScModified || _isUserCmdScModified || _isScintillaKeyModified || _isPluginCmdScModified)
			(NppParameters::getInstance())->writeShortcuts(_isCmdScModified, _isMacrosScModified, _isUserCmdScModified, _isScintillaKeyModified, _isPluginCmdScModified);
	};

	void saveSession(const Session & session){
		(NppParameters::getInstance())->writeSession(session);
	};

	void getCurrentOpenedFiles(Session & session);

	void fileLoadSession(const char *fn = NULL);
	const char * fileSaveSession(size_t nbFile, char ** fileNames, const char *sessionFile2save);
	const char * fileSaveSession(size_t nbFile = 0, char ** fileNames = NULL);

	bool changeDlgLang(HWND hDlg, const char *dlgTagName, char *title = NULL);

	void changeConfigLang();
	void changeUserDefineLang();
	void changeMenuLang(string & pluginsTrans, string & windowTrans);
	void changePrefereceDlgLang();

	const char * getNativeTip(int btnID);
	void changeToolBarIcons();

	bool doBlockComment();
	bool doStreamComment();
	void doTrimTrailing() {
		_pEditView->execute(SCI_BEGINUNDOACTION);
		int nbLines = _pEditView->execute(SCI_GETLINECOUNT);
		for (int line = 0 ; line < nbLines ; line++)
		{
			int lineStart = _pEditView->execute(SCI_POSITIONFROMLINE,line);
			int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION,line);
			int i = lineEnd - 1;
			char c = (char)_pEditView->execute(SCI_GETCHARAT,i);

			for ( ; (i >= lineStart) && (c == ' ') || (c == '\t') ; c = (char)_pEditView->execute(SCI_GETCHARAT,i))
				i--;

			if (i < (lineEnd - 1))
			{
				_pEditView->execute(SCI_SETTARGETSTART, i + 1);
				_pEditView->execute(SCI_SETTARGETEND, lineEnd);
				_pEditView->execute(SCI_REPLACETARGET, 0, (LPARAM)"");
			}
		}
		_pEditView->execute(SCI_ENDUNDOACTION);
	};

	HACCEL getAccTable() const {
		return _accelerator.getAccTable();
	};

	bool addCurrentMacro();
	bool switchToFile(const char *fileName);
	void loadLastSession() {
		Session lastSession = (NppParameters::getInstance())->getSession();
		loadSession(lastSession);
	};
	bool loadSession(Session & session);

private:
	static const char _className[32];
	char _nppPath[MAX_PATH];
    Window *_pMainWindow;
	DockingManager _dockingManager;

	TiXmlNode *_nativeLang, *_toolIcons;

    unsigned char _mainWindowStatus;

    DocTabView _mainDocTab;
    DocTabView _subDocTab;
    DocTabView *_pDocTab;

    ScintillaEditView _subEditView;
    ScintillaEditView _mainEditView;

	ScintillaEditView _invisibleEditView;

    ScintillaEditView *_pEditView;

    SplitterContainer *_pMainSplitter;
    SplitterContainer _subSplitter;

    ContextMenu _tabPopupMenu, _tabPopupDropMenu;

	ToolBar	_toolBar;
	IconList _docTabIconList;
	
    StatusBar _statusBar;
	bool _toReduceTabBar;
	ReBar _rebar;

	// Dialog
	FindReplaceDlg _findReplaceDlg;
	FindIncrementDlg _incrementFindDlg;
    AboutDlg _aboutDlg;
	RunDlg _runDlg;
    GoToLineDlg _goToLineDlg;
	ColumnEditorDlg _colEditorDlg;
	WordStyleDlg _configStyleDlg;
	PreferenceDlg _preference;
	
	// a handle list of all the Notepad++ dialogs
	vector<HWND> _hModelessDlgs;

	LastRecentFileList _lastRecentFileList;

	vector<iconLocator> _customIconVect;

	WindowsMenu _windowsMenu;

	// For FullScreen feature
	bool _isfullScreen;
	RECT _rcWorkArea;
	WINDOWPLACEMENT _winPlace;
	void fullScreenToggle();

	// For restore real window size
	//RECT _normalWindowRect;

	// Keystroke macro recording and playback
	Macro _macro;
	bool _recordingMacro;
	RunMacroDlg _runMacroDlg;

	// For hotspot
	bool _linkTriggered;
	bool _isDocModifing;
	bool _isHotspotDblClicked;

	struct ActivateAppInfo {
		bool _isActivated;
		int _x;
		int _y;
		ActivateAppInfo() : _isActivated(false), _x(0), _y(0){};
	} _activeAppInf;

	//Synchronized Scolling
	
	struct SyncInfo {
		int _line;
		int _column;
		bool _isSynScollV;
		bool _isSynScollH;
		SyncInfo():_line(0), _column(0), _isSynScollV(false), _isSynScollH(false){};
		bool doSync() const {return (_isSynScollV || _isSynScollH); };
	} _syncInfo;

	bool _isUDDocked;

	trayIconControler *_pTrayIco;
	int _zoomOriginalValue;

	Accelerator _accelerator;

	bool _isCmdScModified;
	bool _isMacrosScModified;
	bool _isUserCmdScModified;
	bool _isScintillaKeyModified;
	bool _isPluginCmdScModified;

	PluginsManager _pluginsManager;

	bool _isRTL;
	winVer _winVersion;

	class ScintillaCtrls {
	public :
		//ScintillaCtrls();
		void init(HINSTANCE hInst, HWND hNpp) {
			_hInst = hInst;
			_hParent = hNpp;
		};

		HWND createSintilla(HWND hParent) {
			//if (hParent)
				_hParent = hParent;
			
			ScintillaEditView *scint = new ScintillaEditView;
			scint->init(_hInst, _hParent);
			_scintVector.push_back(scint);
			return scint->getHSelf();
		};
		bool destroyScintilla(HWND handle2Destroy) {
			for (size_t i = 0 ; i < _scintVector.size() ; i++)
			{
				if (_scintVector[i]->getHSelf() == handle2Destroy)
				{
					_scintVector[i]->destroy();
					delete _scintVector[i];

					vector<ScintillaEditView *>::iterator it2delete = _scintVector.begin()+ i;
					_scintVector.erase(it2delete);
					return true;
				}
			}
			return false;
		};
		void destroy() {
			for (size_t i = 0 ; i < _scintVector.size() ; i++)
			{
				delete _scintVector[i];
			}
		};
	private:
		vector<ScintillaEditView *> _scintVector;
		HINSTANCE _hInst;
		HWND _hParent;
	} _scintillaCtrls4Plugins;

	vector<pair<int, int>> _hideLinesMarks;

	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL notify(SCNotification *notification);
	void specialCmd(int id, int param);
	void command(int id);

	void fileNew(){
		setTitleWith(_pDocTab->newDoc(NULL));
		//updateStatusBar();
		//dynamicCheckMenuAndTB();
		//checkDocState();
		//setLangStatus(_pEditView->getCurrentDocType());
		setWorkingDir(NULL);
	};

	void fileOpen();
	bool fileReload() {
		const char * fn = _pEditView->getCurrentTitle();
		if (Buffer::isUntitled(fn)) return false;
		if (::MessageBox(_hSelf, "Do you want to reload the current file?", "Reload", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
			reload(fn);
		return true;
	};
	bool fileClose();
	bool fileCloseAll();
	bool fileCloseAllButCurrent();

	void hideCurrentView();

	int doSaveOrNot(const char *fn) {
		char phrase[512] = "Save file \"";
		strcat(strcat(phrase, fn), "\" ?");
		return ::MessageBox(_hSelf, phrase, "Save", MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
	};
	
	int doReloadOrNot(const char *fn) {
		char phrase[512] = "The file \"";
		strcat(strcat(phrase, fn), "\" is modified by another program. Reload this file?");
		return ::MessageBox(_hSelf, phrase, "Reload", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
	};

	int doCloseOrNot(const char *fn) {
		char phrase[512] = "The file \"";
		strcat(strcat(phrase, fn), "\" doesn't exist anymore. Keep this file in editor ?");
		return ::MessageBox(_hSelf, phrase, "Keep non existing file", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
	};
	
	bool fileSave();
	bool fileSaveAll();
	bool fileSaveAs();
	void filePrint(bool showDialog);
	bool doSave(const char *filename, UniMode mode);
	void enableMenu(int cmdID, bool doEnable) const {
		int flag = doEnable?MF_ENABLED | MF_BYCOMMAND:MF_DISABLED | MF_GRAYED | MF_BYCOMMAND;
		::EnableMenuItem(::GetMenu(_hSelf), cmdID, flag);
	}
	void enableCommand(int cmdID, bool doEnable, int which) const;
	void checkClipboard();
	void checkDocState();
	void checkUndoState();
	void checkMacroState();
	void checkSyncState();
	void dropFiles(HDROP hdrop);
	void checkModifiedDocument();
	void reload(const char *fileName);

    void docGotoAnotherEditView(bool mode);
    void dockUserDlg();
    void undockUserDlg();

    void getToolBarClientRect(RECT &rc) const;
    void getMainClientRect(RECT & rc) const;
    void getStatusBarClientRect(RECT & rc) const;

    int switchEditViewTo(int gid);
	
	void dynamicCheckMenuAndTB() const;

	void enableConvertMenuItems(formatType f) const {
		enableCommand(IDM_FORMAT_TODOS, (f != WIN_FORMAT), MENU);
		enableCommand(IDM_FORMAT_TOUNIX, (f != UNIX_FORMAT), MENU);
		enableCommand(IDM_FORMAT_TOMAC, (f != MAC_FORMAT), MENU);
	};

	void checkUnicodeMenuItems(UniMode um) const;

    int getCurrentView() const {
        return (_pEditView == &_mainEditView)?MAIN_VIEW:SUB_VIEW;
    };

	int getNonCurrentView() const {
        return (_pEditView == &_mainEditView)?SUB_VIEW:MAIN_VIEW;
    };

    DocTabView * getNonCurrentDocTab() {
        return (_pDocTab == &_mainDocTab)?&_subDocTab:&_mainDocTab;
    };

    ScintillaEditView * getCurrentEditView() {
        return (_pEditView == &_mainEditView)?&_mainEditView:&_subEditView;
    };

    ScintillaEditView * getNonCurrentEditView() {
        return (_pEditView == &_mainEditView)?&_subEditView:&_mainEditView;
    };

    void synchronise();

	string getLangDesc(LangType langType, bool shortDesc = false);

	void setLangStatus(LangType langType){
		_statusBar.setText(getLangDesc(langType).c_str(), STATUSBAR_DOC_TYPE);
	};

	void setDisplayFormat(formatType f) {
		std::string str;
		switch (f)
		{
			case MAC_FORMAT :
				str = "MAC";
				break;
			case UNIX_FORMAT :
				str = "UNIX";
				break;
			default :
				str = "Dos\\Windows";
		}
		_statusBar.setText(str.c_str(), STATUSBAR_EOF_FORMAT);
	};

	void setUniModeText(UniMode um)
	{
		char *uniModeText;
		switch (um)
		{
			case uniUTF8:
				uniModeText = "UTF-8"; break;
			case uni16BE:
				uniModeText = "UCS-2 Big Endian"; break;
			case uni16LE:
				uniModeText = "UCS-2 little Endian"; break;
			case uniCookie:
				uniModeText = "ANSI as UTF-8"; break;
			default :
				uniModeText = "ANSI";
		}
		_statusBar.setText(uniModeText, STATUSBAR_UNICODE_TYPE);
	};

	void checkLangsMenu(int id) const ;

    void setLanguage(int id, LangType langType) {
        _pEditView->setCurrentDocType(langType);
        setLangStatus(langType);
		checkLangsMenu(id);
    };

    int getToolBarState() const {
        HMENU hMenu = ::GetMenu(_hSelf);

        if (::GetMenuState(hMenu, IDM_VIEW_TOOLBAR_HIDE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_TOOLBAR_HIDE;
        
        if (::GetMenuState(hMenu, IDM_VIEW_TOOLBAR_REDUCE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_TOOLBAR_REDUCE;

        if (::GetMenuState(hMenu, IDM_VIEW_TOOLBAR_ENLARGE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_TOOLBAR_ENLARGE;

        if (::GetMenuState(hMenu, IDM_VIEW_TOOLBAR_STANDARD, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_TOOLBAR_STANDARD;

		return -1;
    };

    int getFolderMarginStyle() const {
        HMENU hMenu = ::GetMenu(_hSelf);

        if (::GetMenuState(hMenu, IDM_VIEW_FOLDERMAGIN_SIMPLE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_SIMPLE;
        
        if (::GetMenuState(hMenu, IDM_VIEW_FOLDERMAGIN_ARROW, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_ARROW;

        if (::GetMenuState(hMenu, IDM_VIEW_FOLDERMAGIN_CIRCLE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_CIRCLE;

        if (::GetMenuState(hMenu, IDM_VIEW_FOLDERMAGIN_BOX, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_BOX;

		return 0;
    };

    void checkToolBarMenu(int id2Check) const {
        ::CheckMenuRadioItem(::GetMenu(_hSelf), IDM_VIEW_TOOLBAR_HIDE, IDM_VIEW_TOOLBAR_STANDARD, id2Check, MF_BYCOMMAND);
    };

	void checkFolderMarginStyleMenu(int id2Check) const {
		::CheckMenuRadioItem(::GetMenu(_hSelf), IDM_VIEW_FOLDERMAGIN_SIMPLE, IDM_VIEW_FOLDERMAGIN_BOX, id2Check, MF_BYCOMMAND);
	};

    int getFolderMaginStyleIDFrom(folderStyle fStyle) const {
        switch (fStyle)
        {
            case FOLDER_STYLE_SIMPLE : return IDM_VIEW_FOLDERMAGIN_SIMPLE;
            case FOLDER_STYLE_ARROW : return IDM_VIEW_FOLDERMAGIN_ARROW;
            case FOLDER_STYLE_CIRCLE : return IDM_VIEW_FOLDERMAGIN_CIRCLE;
            case FOLDER_STYLE_BOX : return IDM_VIEW_FOLDERMAGIN_BOX;
			default : return FOLDER_TYPE;
        }
        //return 
    };

	void checkMenuItem(int itemID, bool willBeChecked) const {
		::CheckMenuItem(::GetMenu(_hSelf), itemID, MF_BYCOMMAND | (willBeChecked?MF_CHECKED:MF_UNCHECKED));
	};
	void charAdded(char chAdded);
	void MaintainIndentation(char ch);
	
	void addHotSpot(bool docIsModifing = false);
	//void styleHotSpotForWholeDoc();
	//void styleHotSpotModifyMode();

    void bookmarkAdd(int lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		if (!bookmarkPresent(lineno))
			_pEditView->execute(SCI_MARKERADD, lineno, MARK_BOOKMARK);
	};
    void bookmarkDelete(int lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		if ( bookmarkPresent(lineno))
			_pEditView->execute(SCI_MARKERDELETE, lineno, MARK_BOOKMARK);
	};
    bool bookmarkPresent(int lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		LRESULT state = _pEditView->execute(SCI_MARKERGET, lineno);
		return ((state & (1 << MARK_BOOKMARK)) != 0);
	};
    void bookmarkToggle(int lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();

		if (bookmarkPresent(lineno))
			bookmarkDelete(lineno);
		else
    		bookmarkAdd(lineno);
	};
    void bookmarkNext(bool forwardScan);
	void bookmarkClearAll() const {
		_pEditView->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
	};

    int hideLinesMarkPresent(int lineno) const {
		LRESULT state = _pEditView->execute(SCI_MARKERGET, lineno);
		if ((state & (1 << MARK_HIDELINESBEGIN)) != 0)
			return MARK_HIDELINESBEGIN;
		else if ((state & (1 << MARK_HIDELINESEND)) != 0)
			return MARK_HIDELINESEND;
		return 0;
	};

	void hideLinesMarkDelete(int lineno, int which) const {
		_pEditView->execute(SCI_MARKERDELETE, lineno, which);
	};

	bool showLines(int lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		int hideLinesMark = hideLinesMarkPresent(lineno);
		if (!hideLinesMark)
			return false;

		//
		int start = 0;
		int end = 0;
		if (hideLinesMark == MARK_HIDELINESEND)
		{
			end = lineno;
			int i = lineno - 1;
			for ( ; i >= 0 ; i--)
			{
				if (_pEditView->execute(SCI_GETLINEVISIBLE, i))
					break;
				hideLinesMarkDelete(i, MARK_HIDELINESBEGIN);
				hideLinesMarkDelete(i, MARK_HIDELINESEND);
			}
			start = i;
		}
		else if (hideLinesMark == MARK_HIDELINESBEGIN)
		{
			long nbLine = _pEditView->getNbLine();
			start = lineno;
			int i = lineno + 1;
			for ( ; i < nbLine ; i++)
			{
				if (_pEditView->execute(SCI_GETLINEVISIBLE, i))
					break;
				hideLinesMarkDelete(i, MARK_HIDELINESBEGIN);
				hideLinesMarkDelete(i, MARK_HIDELINESEND);
			}
			end = i;
		}
		_pEditView->execute(SCI_SHOWLINES, start+1, end-1);
		hideLinesMarkDelete(start, MARK_HIDELINESBEGIN);
		hideLinesMarkDelete(end, MARK_HIDELINESEND);
		return true;
	};

    void findMatchingBracePos(int & braceAtCaret, int & braceOpposite);
    void braceMatch();
   
    void activateNextDoc(bool direction);
	void activateDoc(int pos);

	void updateStatusBar();
	void showAutoComp();
	void autoCompFromCurrentFile(bool autoInsert = true);
	void getApiFileName(LangType langType, std::string &fn);

	void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
	bool replaceAllFiles();
	bool findInOpenedFiles();
	bool findInFiles(bool isRecursive);

	bool matchInList(const char *fileName, const vector<string> & patterns);
	void getMatchedFileNames(const char *dir, const vector<string> & patterns, vector<string> & fileNames, bool isRecursive);

	void doSynScorll(HWND hW);
	void changeMenuShortcut(unsigned long cmdID, const char *shortcutStr);
	void setWorkingDir(char *dir) {
		if (NppParameters::getInstance()->getNppGUI()._saveOpenKeepInSameDir)
			return;

		if (!dir || !PathIsDirectory(dir))
		{
			return;
		}
		else
			::SetCurrentDirectory(dir);
	}
	bool str2Cliboard(const char *str2cpy);
	void mkPosIncFindDlg() {
		if (!_incrementFindDlg.isCreated())
			return;
		if (!_incrementFindDlg.isVisible())
			return;
		_incrementFindDlg.goToLowerLeft();
	};

	bool getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible);
	
	void setLangFromName(const char * langName)
	{
		int	id	= 0;
		char	menuLangName[ 16 ];

		for ( int i = IDM_LANG_C; i <= IDM_LANG_USER; i++ )
			if ( ::GetMenuString( ::GetMenu( _hSelf ), i, menuLangName, sizeof( menuLangName ), MF_BYCOMMAND ) )
				if ( !strcmp( langName, menuLangName ) )
				{
					id	= i;
					break;
				}

		if ( id == 0 )
		{
			for ( int i = IDM_LANG_USER + 1; i <= IDM_LANG_USER_LIMIT; i++ )
				if ( ::GetMenuString( ::GetMenu( _hSelf ), i, menuLangName, sizeof( menuLangName ), MF_BYCOMMAND ) )
					if ( !strcmp( langName, menuLangName ) )
					{
						id	= i;
						break;
					}
		}

		if ( id != 0 )
			command( id );
	}

	string getLangFromMenu(Buffer buf)
	{
		int	id;
		const char * userLangName;
		char	menuLangName[32];

		id = (NppParameters::getInstance())->langTypeToCommandID( buf.getLangType() );

		if ( ( id != IDM_LANG_USER ) || !( buf.isUserDefineLangExt() ) )
		{
			( ::GetMenuString( ::GetMenu( _hSelf ), id, menuLangName, sizeof( menuLangName ), MF_BYCOMMAND ) );
			userLangName = (char *)menuLangName;
		}
		else
		{
			userLangName = buf.getUserDefineLangName();
		}
		return	userLangName;
	}

	void removeHideLinesBookmarks() {
		for (size_t i = 0 ; i < _hideLinesMarks.size() ; i++)
		{
			hideLinesMarkDelete(_hideLinesMarks[i].first, MARK_HIDELINESBEGIN);
			hideLinesMarkDelete(_hideLinesMarks[i].second, MARK_HIDELINESEND);
		}
	};

	void setFileOpenSaveDlgFilters(FileDialog & fDlg);
};

#endif //NOTEPAD_PLUS_H
