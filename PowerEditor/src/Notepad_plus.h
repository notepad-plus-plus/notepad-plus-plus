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
#include <window.h>
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
#include "Process.h"
#include "AutoCompletion.h"
#include "Buffer.h"
#include "SmartHighlighter.h"

#define MENU 0x01
#define TOOLBAR 0x02

enum FileTransferMode {
	TransferClone		= 0x01,
	TransferMove		= 0x02
};

enum WindowStatus {	//bitwise mask
	WindowMainActive	= 0x01,
	WindowSubActive		= 0x02,
	WindowBothActive	= 0x03,	//little helper shortcut
	WindowUserActive	= 0x04,
	WindowMask			= 0x07
};

/*
//Plugins rely on #define's
enum Views {
	MAIN_VIEW			= 0x00,
	SUB_VIEW			= 0x01
};
*/

struct TaskListInfo;
static TiXmlNodeA * searchDlgNode(TiXmlNodeA *node, const char *dlgTagName);

struct iconLocator {
	int listIndex;
	int iconIndex;
	std::generic_string iconLocation;

	iconLocator(int iList, int iIcon, const std::generic_string iconLoc) 
		: listIndex(iList), iconIndex(iIcon), iconLocation(iconLoc){};
};

struct VisibleGUIConf {
	bool isPostIt;
	bool isFullScreen;
	
	//Used by both views
	bool isMenuShown;
	//bool isToolbarShown;	//toolbar forcefully hidden by hiding rebar
	DWORD_PTR preStyle;

	//used by postit only
	bool isTabbarShown;
	bool isAlwaysOnTop;
	bool isStatusbarShown;

	//used by fullscreen only
	WINDOWPLACEMENT _winPlace;

	VisibleGUIConf() : isPostIt(false), isFullScreen(false),
		isAlwaysOnTop(false), isMenuShown(true), isTabbarShown(true),
		isStatusbarShown(true), preStyle(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)
	{
		_winPlace.length = 0;
	};
};

class FileDialog;

class Notepad_plus : public Window {
	enum comment_mode {cm_comment, cm_uncomment, cm_toggle};
public:
	Notepad_plus();
	virtual inline ~Notepad_plus();
	void init(HINSTANCE, HWND, const TCHAR *cmdLine, CmdLineParams *cmdLineParams);
	inline void killAllChildren();
	virtual inline void destroy();

    static const TCHAR * Notepad_plus::getClassName() {
		return _className;
	};
	
	void setTitle();
	void getTaskListInfo(TaskListInfo *tli);

	// For filtering the modeless Dialog message
	inline bool isDlgsMsg(MSG *msg, bool unicodeSupported) const {
		for (size_t i = 0; i < _hModelessDlgs.size(); i++)
		{
			if (unicodeSupported?(::IsDialogMessageW(_hModelessDlgs[i], msg)):(::IsDialogMessageA(_hModelessDlgs[i], msg)))
				return true;
		}
		return false;
	};

// fileOperations
	//The doXXX functions apply to a single buffer and dont need to worry about views, with the excpetion of doClose, since closing one view doesnt have to mean the document is gone
    BufferID doOpen(const TCHAR *fileName, bool isReadOnly = false);
	bool doReload(BufferID id, bool alert = true);
	bool doSave(BufferID, const TCHAR * filename, bool isSaveCopy = false);
	void doClose(BufferID, int whichOne);
	//bool doDelete(const TCHAR *fileName) const {return ::DeleteFile(fileName) != 0;};

	inline void fileNew();
	void fileOpen();
	inline bool fileReload();
	bool fileClose(BufferID id = BUFFER_INVALID, int curView = -1);	//use curView to override view to close from
	bool fileCloseAll();
	bool fileCloseAllButCurrent();
	bool fileSave(BufferID id = BUFFER_INVALID);
	bool fileSaveAll();
	bool fileSaveAs(BufferID id = BUFFER_INVALID, bool isSaveCopy = false);
	bool fileDelete(BufferID id = BUFFER_INVALID, int curView = -1);
	bool fileRename(BufferID id = BUFFER_INVALID, int curView = -1);

	bool addBufferToView(BufferID id, int whichOne);
	bool moveBuffer(BufferID id, int whereTo);	//assumes whereFrom is otherView(whereTo)
	bool switchToFile(BufferID buffer);			//find buffer in active view then in other view.
// end fileOperations

	bool isFileSession(const TCHAR * filename);
	void filePrint(bool showDialog);
	bool saveScintillaParams(bool whichOne);

	inline bool saveGUIParams();

	inline void saveDockingParams();

	inline void saveUserDefineLangs();

	inline void saveShortcuts();

	inline void saveSession(const Session & session);

	inline void saveFindHistory();

	void getCurrentOpenedFiles(Session & session);

	bool fileLoadSession(const TCHAR *fn = NULL);
	const TCHAR * fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save);
	const TCHAR * fileSaveSession(size_t nbFile = 0, TCHAR ** fileNames = NULL);

	bool changeDlgLang(HWND hDlg, const char *dlgTagName, char *title = NULL);
	void changeFindReplaceDlgLang();
	void changeConfigLang();
	void changeUserDefineLang();
	void changeMenuLang(generic_string & pluginsTrans, generic_string & windowTrans);
	void changeLangTabContextMenu();
	void changeLangTabDrapContextMenu();
	void changePrefereceDlgLang();
	void changeShortcutLang();
	void changeShortcutmapperLang(ShortcutMapper * sm);

	const TCHAR * getNativeTip(int btnID);
	void changeToolBarIcons();

	bool doBlockComment(comment_mode currCommentMode);
	bool doStreamComment();
	inline void doTrimTrailing();

	inline HACCEL getAccTable() const{
		return _accelerator.getAccTable();
	};

	bool addCurrentMacro();
	inline void loadLastSession();
	bool loadSession(Session & session);
	winVer getWinVersion() const {return _winVersion;};

	bool emergency(generic_string emergencySavedDir);

	void notifyBufferChanged(Buffer * buffer, int mask);
	bool findInFiles();
	bool replaceInFiles();
	void setFindReplaceFolderFilter(const TCHAR *dir, const TCHAR *filters);

	static HWND gNppHWND;	//static handle to Notepad++ window, NULL if non-existant
private:
	static const TCHAR _className[32];
	TCHAR _nppPath[MAX_PATH];
    Window *_pMainWindow;
	DockingManager _dockingManager;

	AutoCompletion _autoCompleteMain;
	AutoCompletion _autoCompleteSub;	//each Scintilla has its own autoComplete

	SmartHighlighter _smartHighlighter;

	TiXmlNode *_toolIcons;
	TiXmlNodeA *_nativeLangA;

	int _nativeLangEncoding;

    DocTabView _mainDocTab;
    DocTabView _subDocTab;
    DocTabView *_pDocTab;
	DocTabView *_pNonDocTab;

    ScintillaEditView _subEditView;
    ScintillaEditView _mainEditView;
	ScintillaEditView _invisibleEditView;	//for searches
	ScintillaEditView _fileEditView;		//for FileManager

    ScintillaEditView *_pEditView;
	ScintillaEditView *_pNonEditView;

    SplitterContainer *_pMainSplitter;
    SplitterContainer _subSplitter;

    ContextMenu _tabPopupMenu, _tabPopupDropMenu;

	ToolBar	_toolBar;
	IconList _docTabIconList;
	
    StatusBar _statusBar;
	bool _toReduceTabBar;
	ReBar _rebarTop;
	ReBar _rebarBottom;

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
	HMENU _mainMenuHandle;
	bool _sysMenuEntering;

	// For FullScreen/PostIt features
	VisibleGUIConf	_beforeSpecialView;
	void fullScreenToggle();
	void postItToggle();

	// Keystroke macro recording and playback
	Macro _macro;
	bool _recordingMacro;
	RunMacroDlg _runMacroDlg;

	// For hotspot
	bool _linkTriggered;
	bool _isDocModifing;
	bool _isHotspotDblClicked;

	//For Dynamic selection highlight
	CharacterRange _prevSelectedRange;

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
	ScintillaAccelerator _scintaccelerator;

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
				_scintVector[i]->destroy();
				delete _scintVector[i];
			}
		};
	private:
		vector<ScintillaEditView *> _scintVector;
		HINSTANCE _hInst;
		HWND _hParent;
	} _scintillaCtrls4Plugins;

	vector<pair<int, int> > _hideLinesMarks;

	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL notify(SCNotification *notification);
	void specialCmd(int id, int param);
	void command(int id);

//Document management
	UCHAR _mainWindowStatus;	//For 2 views and user dialog if docked
	int _activeView;

	//User dialog docking
	void dockUserDlg();
    void undockUserDlg();

	//View visibility
	void showView(int whichOne);
	bool viewVisible(int whichOne);
	void hideView(int whichOne);
	void hideCurrentView();
	bool bothActive() { return (_mainWindowStatus & WindowBothActive) == WindowBothActive; };
	bool reloadLang();
	int currentView(){
		return _activeView;
	};
	int otherView(){
		return (_activeView == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	};
	int otherFromView(int whichOne){
		return (whichOne == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	};
	bool canHideView(int whichOne);	//true if view can safely be hidden (no open docs etc)

	int switchEditViewTo(int gid);	//activate other view (set focus etc)

	void docGotoAnotherEditView(FileTransferMode mode);	//TransferMode
	void docOpenInNewInstance(FileTransferMode mode);

	void loadBufferIntoView(BufferID id, int whichOne, bool dontClose = false);		//Doesnt _activate_ the buffer
	void removeBufferFromView(BufferID id, int whichOne);	//Activates alternative of possible, or creates clean document if not clean already

	bool activateBuffer(BufferID id, int whichOne);			//activate buffer in that view if found
	void notifyBufferActivated(BufferID bufid, int view);
	void performPostReload(int whichOne);
//END: Document management

	int doSaveOrNot(const TCHAR *fn) {
		TCHAR pattern[64] = TEXT("Save file \"%s\" ?");
		TCHAR phrase[512];
		wsprintf(phrase, pattern, fn);
		return doActionOrNot(TEXT("Save"), phrase, MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
	};

	int doReloadOrNot(const TCHAR *fn, bool dirty) {
		TCHAR* pattern = TEXT("%s\r\rThis file has been modified by another program.\rDo you want to reload it%s?");
		TCHAR* lose_info_str = dirty ? TEXT(" and lose the changes made in Notepad++") : TEXT("");
		TCHAR phrase[512];
		wsprintf(phrase, pattern, fn, lose_info_str);
		int icon = dirty ? MB_ICONEXCLAMATION : MB_ICONQUESTION;
		return doActionOrNot(TEXT("Reload"), phrase, MB_YESNO | MB_APPLMODAL | icon);
	};

	int doCloseOrNot(const TCHAR *fn) {
		TCHAR pattern[128] = TEXT("The file \"%s\" doesn't exist anymore.\rKeep this file in editor?");
		TCHAR phrase[512];
		wsprintf(phrase, pattern, fn);
		return doActionOrNot(TEXT("Keep non existing file"), phrase, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
	};
	
	int doDeleteOrNot(const TCHAR *fn) {
		TCHAR pattern[128] = TEXT("The file \"%s\"\rwill be deleted from your disk and this document will be closed.\rContinue?");
		TCHAR phrase[512];
		wsprintf(phrase, pattern, fn);
		return doActionOrNot(TEXT("Delete file"), phrase, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
	};

	int doActionOrNot(const TCHAR *title, const TCHAR *displayText, int type) {
		return ::MessageBox(_hSelf, displayText, title, type);
	};
	void enableMenu(int cmdID, bool doEnable) const {
		int flag = doEnable?MF_ENABLED | MF_BYCOMMAND:MF_DISABLED | MF_GRAYED | MF_BYCOMMAND;
		::EnableMenuItem(_mainMenuHandle, cmdID, flag);
	}
	void enableCommand(int cmdID, bool doEnable, int which) const;
	void checkClipboard();
	void checkDocState();
	void checkUndoState();
	void checkMacroState();
	void checkSyncState();
	void dropFiles(HDROP hdrop);
	void checkModifiedDocument();

    void getMainClientRect(RECT & rc) const;

	void dynamicCheckMenuAndTB() const;

	void enableConvertMenuItems(formatType f) const {
		enableCommand(IDM_FORMAT_TODOS, (f != WIN_FORMAT), MENU);
		enableCommand(IDM_FORMAT_TOUNIX, (f != UNIX_FORMAT), MENU);
		enableCommand(IDM_FORMAT_TOMAC, (f != MAC_FORMAT), MENU);
	};

	void checkUnicodeMenuItems(UniMode um) const;

	generic_string getLangDesc(LangType langType, bool shortDesc = false);

	void setLangStatus(LangType langType){
		_statusBar.setText(getLangDesc(langType).c_str(), STATUSBAR_DOC_TYPE);
	};

	void setDisplayFormat(formatType f) {
		std::generic_string str;
		switch (f)
		{
			case MAC_FORMAT :
				str = TEXT("MAC");
				break;
			case UNIX_FORMAT :
				str = TEXT("UNIX");
				break;
			default :
				str = TEXT("Dos\\Windows");
		}
		_statusBar.setText(str.c_str(), STATUSBAR_EOF_FORMAT);
	};

	void setUniModeText(UniMode um)
	{
		TCHAR *uniModeText;
		switch (um)
		{
			case uniUTF8:
				uniModeText = TEXT("UTF-8"); break;
			case uni16BE:
				uniModeText = TEXT("UCS-2 Big Endian"); break;
			case uni16LE:
				uniModeText = TEXT("UCS-2 little Endian"); break;
			case uniCookie:
				uniModeText = TEXT("ANSI as UTF-8"); break;
			default :
				uniModeText = TEXT("ANSI");
		}
		_statusBar.setText(uniModeText, STATUSBAR_UNICODE_TYPE);
	};

	void checkLangsMenu(int id) const ;

    void setLanguage(int id, LangType langType);

	enum LangType menuID2LangType(int cmdID);

    int getFolderMarginStyle() const {
        if (::GetMenuState(_mainMenuHandle, IDM_VIEW_FOLDERMAGIN_SIMPLE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_SIMPLE;
        
        if (::GetMenuState(_mainMenuHandle, IDM_VIEW_FOLDERMAGIN_ARROW, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_ARROW;

        if (::GetMenuState(_mainMenuHandle, IDM_VIEW_FOLDERMAGIN_CIRCLE, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_CIRCLE;

        if (::GetMenuState(_mainMenuHandle, IDM_VIEW_FOLDERMAGIN_BOX, MF_BYCOMMAND) == MF_CHECKED)
            return IDM_VIEW_FOLDERMAGIN_BOX;

		return 0;
    };

	void checkFolderMarginStyleMenu(int id2Check) const {
		::CheckMenuRadioItem(_mainMenuHandle, IDM_VIEW_FOLDERMAGIN_SIMPLE, IDM_VIEW_FOLDERMAGIN_BOX, id2Check, MF_BYCOMMAND);
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
		::CheckMenuItem(_mainMenuHandle, itemID, MF_BYCOMMAND | (willBeChecked?MF_CHECKED:MF_UNCHECKED));
	};
	void charAdded(TCHAR chAdded);
	void MaintainIndentation(TCHAR ch);
	
	void addHotSpot(bool docIsModifing = false);

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


	void copyMarkedLines() {
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
		str2Cliboard(globalStr.c_str());
	};

	void cutMarkedLines() {
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
		str2Cliboard(globalStr.c_str());
	};

	void deleteMarkedLines() {
		int lastLine = _pEditView->lastZeroBasedLineNumber();

		_pEditView->execute(SCI_BEGINUNDOACTION);
		for (int i = lastLine ; i >= 0 ; i--)
		{
			if (bookmarkPresent(i))
				deleteMarkedline(i);
		}
		_pEditView->execute(SCI_ENDUNDOACTION);
	};

	void pasteToMarkedLines() {
		int clipFormat;
	#ifdef UNICODE
		clipFormat = CF_UNICODETEXT;
	#else
		clipFormat = CF_TEXT;
	#endif
		BOOL canPaste = ::IsClipboardFormatAvailable(clipFormat);
		if (!canPaste)
			return;
		int lastLine = _pEditView->lastZeroBasedLineNumber();

		::OpenClipboard(_hSelf);
		HANDLE clipboardData = ::GetClipboardData(clipFormat);
		int len = ::GlobalSize(clipboardData);
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
	};

	void deleteMarkedline(int ln) {
		int lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
		int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);
		
		bookmarkDelete(ln);
		TCHAR emptyString[2] = TEXT("");
		_pEditView->replaceTarget(emptyString, lineBegin, lineBegin + lineLen);
	};

	void replaceMarkedline(int ln, const TCHAR *str) {
		int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);
		int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, ln);

		_pEditView->replaceTarget(str, lineBegin, lineEnd);
	};

	generic_string getMarkedLine(int ln) {
		int lineLen = _pEditView->execute(SCI_LINELENGTH, ln);
		int lineBegin = _pEditView->execute(SCI_POSITIONFROMLINE, ln);

		TCHAR * buf = new TCHAR[lineLen+1];
		_pEditView->getGenericText(buf, lineBegin, lineBegin + lineLen);
		generic_string line = buf;
		delete [] buf;

		return line;
	};

    void findMatchingBracePos(int & braceAtCaret, int & braceOpposite);
    void braceMatch();

    void activateNextDoc(bool direction);
	void activateDoc(int pos);

	void updateStatusBar();

	void showAutoComp();
	void autoCompFromCurrentFile(bool autoInsert = true);
	void showFunctionComp();

	void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
	bool replaceAllFiles();
	bool findInOpenedFiles();

	bool matchInList(const TCHAR *fileName, const vector<generic_string> & patterns);
	void getMatchedFileNames(const TCHAR *dir, const vector<generic_string> & patterns, vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir);

	void doSynScorll(HWND hW);
	void setWorkingDir(TCHAR *dir) {
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
	bool str2Cliboard(const TCHAR *str2cpy);
	bool bin2Cliboard(const UCHAR *uchar2cpy, size_t length);

	bool getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible);
	
	int getLangFromMenuName(const TCHAR * langName) {
		int	id	= 0;
		const int menuSize = 64;
		TCHAR menuLangName[menuSize];

		for ( int i = IDM_LANG_C; i <= IDM_LANG_USER; i++ )
			if ( ::GetMenuString( _mainMenuHandle, i, menuLangName, menuSize, MF_BYCOMMAND ) )
				if ( !lstrcmp( langName, menuLangName ) )
				{
					id	= i;
					break;
				}

		if ( id == 0 )
		{
			for ( int i = IDM_LANG_USER + 1; i <= IDM_LANG_USER_LIMIT; i++ )
				if ( ::GetMenuString( _mainMenuHandle, i, menuLangName, menuSize, MF_BYCOMMAND ) )
					if ( !lstrcmp( langName, menuLangName ) )
					{
						id	= i;
						break;
					}
		}

		return id;
	};

	generic_string getLangFromMenu(const Buffer * buf) {
		int	id;
		const TCHAR * userLangName;
		TCHAR	menuLangName[32];

		id = (NppParameters::getInstance())->langTypeToCommandID( buf->getLangType() );

		if ( ( id != IDM_LANG_USER ) || !( buf->isUserDefineLangExt() ) )
		{
			( ::GetMenuString( _mainMenuHandle, id, menuLangName, sizeof( menuLangName ), MF_BYCOMMAND ) );
			userLangName = (TCHAR *)menuLangName;
		}
		else
		{
			userLangName = buf->getUserDefineLangName();
		}
		return	userLangName;
	};

	void setFileOpenSaveDlgFilters(FileDialog & fDlg);
	void markSelectedTextInc(bool enable);

	Style * getStyleFromName(const TCHAR *styleName) {
		StyleArray & stylers = (NppParameters::getInstance())->getMiscStylerArray();

		int i = stylers.getStylerIndexByName(styleName);
		Style * st = NULL;
		if (i != -1)
		{
			Style & style = stylers.getStyler(i);
			st = &style;
		}
		return st;
	};

	bool dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix = TEXT(""));	//helper func
	void drawTabbarColoursFromStylerArray();

	void loadCommandlineParams(const TCHAR * commandLine, CmdLineParams * pCmdParams);
};

#endif //NOTEPAD_PLUS_H
