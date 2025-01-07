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

#pragma once

#include "ScintillaEditView.h"
#include "DocTabView.h"
#include "SplitterContainer.h"
#include "FindReplaceDlg.h"
#include "AboutDlg.h"
#include "RunDlg.h"
#include "StatusBar.h"
#include "lastRecentFileList.h"
#include "GoToLineDlg.h"
#include "FindCharsInRange.h"
#include "columnEditor.h"
#include "WordStyleDlg.h"
#include "trayIconControler.h"
#include "PluginsManager.h"
#include "preferenceDlg.h"
#include "WindowsDlg.h"
#include "RunMacroDlg.h"
#include "DockingManager.h"
#include "Processus.h"
#include "AutoCompletion.h"
#include "SmartHighlighter.h"
#include "ScintillaCtrls.h"
#include "lesDlgs.h"
#include "pluginsAdmin.h"
#include "localization.h"
#include "documentSnapshot.h"
#include "md5Dlgs.h"
#include <vector>
#include <iso646.h>
#include <chrono>
#include <atomic>

extern std::chrono::steady_clock::time_point g_nppStartTimePoint;
extern std::chrono::steady_clock::duration g_pluginsLoadingTime;

extern std::atomic<bool> g_bNppExitFlag;


#define MENU 0x01
#define TOOLBAR 0x02

#define MAIN_EDIT_ZONE true

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

enum trimOp {
	lineHeader = 0,
	lineTail = 1,
	lineBoth = 2,
	lineEol = 3
};

enum spaceTab {
	tab2Space = 0,
	space2TabLeading = 1,
	space2TabAll = 2
};

struct TaskListInfo;


struct VisibleGUIConf final
{
	bool _isPostIt = false;
	bool _isFullScreen = false;
	bool _isDistractionFree = false;

	//Used by postit & fullscreen
	bool _isMenuShown = true;
	DWORD_PTR _preStyle = (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);

	//used by postit
	bool _isTabbarShown = true;
	bool _isAlwaysOnTop = false;
	bool _isStatusbarShown = true;

	//used by fullscreen
	WINDOWPLACEMENT _winPlace = {};

	//used by distractionFree
	bool _was2ViewModeOn = false;
	std::vector<DockingCont*> _pVisibleDockingContainers;
};

struct QuoteParams
{
	enum Speed { slow = 0, rapid, speedOfLight };

	QuoteParams() {};
	QuoteParams(const wchar_t* quoter, Speed speed, bool shouldBeTrolling, int encoding, LangType lang, const wchar_t* quote) :
		_quoter(quoter), _speed(speed), _shouldBeTrolling(shouldBeTrolling), _encoding(encoding), _lang(lang), _quote(quote) {}

	void reset() {
		_quoter = nullptr;
		_speed = rapid;
		_shouldBeTrolling = false;
		_encoding = SC_CP_UTF8;
		_lang = L_TEXT;
		_quote = nullptr;
	};

	const wchar_t* _quoter = nullptr;
	Speed _speed = rapid;
	bool _shouldBeTrolling = false;
	int _encoding = SC_CP_UTF8;
	LangType _lang = L_TEXT;
	const wchar_t* _quote = nullptr;
};

class CustomFileDialog;
class Notepad_plus_Window;
class AnsiCharPanel;
class ClipboardHistoryPanel;
class VerticalFileSwitcher;
class ProjectPanel;
class DocumentMap;
class FunctionListPanel;
class FileBrowser;
struct QuoteParams;

class Notepad_plus final
{
friend class Notepad_plus_Window;
friend class FileManager;

public:
	Notepad_plus();
	~Notepad_plus();

	LRESULT init(HWND hwnd);
	LRESULT process(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void killAllChildren();

	enum comment_mode {cm_comment, cm_uncomment, cm_toggle};

	void setTitle();
	void getTaskListInfo(TaskListInfo *tli);
	size_t getNbDirtyBuffer(int view);
	// For filtering the modeless Dialog message

	//! \name File Operations
	//@{
	//The doXXX functions apply to a single buffer and dont need to worry about views, with the excpetion of doClose, since closing one view doesnt have to mean the document is gone
	BufferID doOpen(const std::wstring& fileName, bool isRecursive = false, bool isReadOnly = false, int encoding = -1, const wchar_t *backupFileName = NULL, FILETIME fileNameTimestamp = {});
	bool doReload(BufferID id, bool alert = true);
	bool doSave(BufferID, const wchar_t * filename, bool isSaveCopy = false);
	void doClose(BufferID, int whichOne, bool doDeleteBackup = false);
	//bool doDelete(const wchar_t *fileName) const {return ::DeleteFile(fileName) != 0;};

	void fileOpen();
	void fileNew();
    bool fileReload();
	bool fileClose(BufferID id = BUFFER_INVALID, int curView = -1);	//use curView to override view to close from
	bool fileCloseAll(bool doDeleteBackup, bool isSnapshotMode = false);
	bool fileCloseAllButCurrent();
	void fileCloseAllButPinned();
	bool fileCloseAllGiven(const std::vector<BufferViewInfo>& krvecBuffer);
	bool fileCloseAllToLeft();
	bool fileCloseAllToRight();
	bool fileCloseAllUnchanged();
	bool fileSave(BufferID id = BUFFER_INVALID);
	bool fileSaveAllConfirm();
	bool fileSaveAll();
	bool fileSaveSpecific(const std::wstring& fileNameToSave);
	bool fileSaveAs(BufferID id = BUFFER_INVALID, bool isSaveCopy = false);
	bool fileDelete(BufferID id = BUFFER_INVALID);
	bool fileRename(BufferID id = BUFFER_INVALID);
	bool fileRenameUntitledPluginAPI(BufferID id, const wchar_t* tabNewName);

	void unPinnedForAllBuffers();
	bool switchToFile(BufferID buffer);			//find buffer in active view then in other view.
	//@}

	bool isFileSession(const wchar_t * filename);
	bool isFileWorkspace(const wchar_t * filename);
	void filePrint(bool showDialog);
	void saveScintillasZoom();

	bool saveGUIParams();
	bool saveProjectPanelsParams();
	bool saveFileBrowserParam();
	bool saveColumnEditorParams();
	void saveDockingParams();
    void saveUserDefineLangs();
    void saveShortcuts();
	void saveSession(const Session & session);
	void saveCurrentSession();
	void saveFindHistory();

	void getCurrentOpenedFiles(Session& session, bool includUntitledDoc = false);

	bool fileLoadSession(const wchar_t* fn = nullptr);
	const wchar_t * fileSaveSession(size_t nbFile, wchar_t ** fileNames, const wchar_t *sessionFile2save, bool includeFileBrowser = false);
	const wchar_t * fileSaveSession(size_t nbFile = 0, wchar_t** fileNames = nullptr);

	bool doBlockComment(comment_mode currCommentMode);
	bool doStreamComment();
	bool undoStreamComment(bool tryBlockComment = true);

	bool addCurrentMacro();
	void macroPlayback(Macro);

    void loadLastSession();
	bool loadSession(Session & session, bool isSnapshotMode = false, const wchar_t* userCreatedSessionName = nullptr);

	void prepareBufferChangedDialog(Buffer * buffer);
	void notifyBufferChanged(Buffer * buffer, int mask);
	bool findInFinderFiles(FindersInfo *findInFolderInfo);

	bool createFilelistForFiles(std::vector<std::wstring> & fileNames);
	bool createFilelistForProjects(std::vector<std::wstring> & fileNames);
	bool findInFiles();
	bool findInProjects();
	bool findInFilelist(std::vector<std::wstring> & fileList);
	bool replaceInFiles();
	bool replaceInProjects();
	bool replaceInFilelist(std::vector<std::wstring> & fileList);

	void setFindReplaceFolderFilter(const wchar_t *dir, const wchar_t *filters);
	std::vector<std::wstring> addNppComponents(const wchar_t *destDir, const wchar_t *extFilterName, const wchar_t *extFilter);
	std::vector<std::wstring> addNppPlugins(const wchar_t *extFilterName, const wchar_t *extFilter);
    int getHtmlXmlEncoding(const wchar_t *fileName) const;

	HACCEL getAccTable() const{
		return _accelerator.getAccTable();
	};

	bool emergency(const std::wstring& emergencySavedDir);

	Buffer* getCurrentBuffer()	{
		return _pEditView->getCurrentBuffer();
	};

	void launchDocumentBackupTask();
	int getQuoteIndexFrom(const wchar_t* quoter) const;
	void showQuoteFromIndex(int index) const;
	void showQuote(const QuoteParams* quote) const;

	std::wstring getPluginListVerStr() const {
		return _pluginsAdminDlg.getPluginListVerStr();
	};

	void minimizeDialogs();
	void restoreMinimizeDialogs();

	void refreshDarkMode(bool resetStyle = false);

private:
	Notepad_plus_Window* _pPublicInterface = nullptr;
    Window* _pMainWindow = nullptr;
	DockingManager _dockingManager;
	std::vector<int> _internalFuncIDs;

	AutoCompletion _autoCompleteMain;
	AutoCompletion _autoCompleteSub; // each Scintilla has its own autoComplete

	SmartHighlighter _smartHighlighter;
    NativeLangSpeaker _nativeLangSpeaker;
    DocTabView _mainDocTab;
    DocTabView _subDocTab;
    DocTabView* _pDocTab = nullptr;
	DocTabView* _pNonDocTab = nullptr;

    ScintillaEditView _subEditView = ScintillaEditView(MAIN_EDIT_ZONE);  // only _mainEditView and _subEditView are MAIN_EDIT_ZONE comparing with other Scintilla controls
    ScintillaEditView _mainEditView = ScintillaEditView(MAIN_EDIT_ZONE); // only _mainEditView and _subEditView are MAIN_EDIT_ZONE comparing with other Scintilla controls
	ScintillaEditView _invisibleEditView; // for searches
	ScintillaEditView _fileEditView;      // for FileManager
    ScintillaEditView* _pEditView = nullptr;
	ScintillaEditView* _pNonEditView = nullptr;

    SplitterContainer* _pMainSplitter = nullptr;
    SplitterContainer _subSplitter;

    ContextMenu _tabPopupMenu;
	ContextMenu _tabPopupDropMenu;
	ContextMenu _fileSwitcherMultiFilePopupMenu;

	ToolBar	_toolBar;

    StatusBar _statusBar;
	ReBar _rebarTop;
	ReBar _rebarBottom;

	// Dialog
	FindReplaceDlg _findReplaceDlg;
	FindInFinderDlg _findInFinderDlg;

	FindIncrementDlg _incrementFindDlg;
    AboutDlg _aboutDlg;
	DebugInfoDlg _debugInfoDlg;
	RunDlg _runDlg;
	HashFromFilesDlg _md5FromFilesDlg;
	HashFromTextDlg _md5FromTextDlg;
	HashFromFilesDlg _sha2FromFilesDlg;
	HashFromTextDlg _sha2FromTextDlg;
	HashFromFilesDlg _sha1FromFilesDlg;
	HashFromTextDlg _sha1FromTextDlg;
	HashFromFilesDlg _sha512FromFilesDlg;
	HashFromTextDlg _sha512FromTextDlg;
    GoToLineDlg _goToLineDlg;
	ColumnEditorDlg _colEditorDlg;
	WordStyleDlg _configStyleDlg;
	PreferenceDlg _preference;
	FindCharsInRangeDlg _findCharsInRangeDlg;
	PluginsAdminDlg _pluginsAdminDlg;
	DocumentPeeker _documentPeeker;

	// a handle list of all the Notepad++ dialogs
	std::vector<HWND> _hModelessDlgs;

	LastRecentFileList _lastRecentFileList;

	WindowsMenu _windowsMenu;
	HMENU _mainMenuHandle = NULL;

	bool _sysMenuEntering = false;

	// make sure we don't recursively call doClose when closing the last file with -quitOnEmpty
	bool _isAttemptingCloseOnQuit = false;

	// For FullScreen/PostIt/DistractionFree features
	VisibleGUIConf	_beforeSpecialView;
	void fullScreenToggle();
	void postItToggle();
	void distractionFreeToggle();

	// Keystroke macro recording and playback
	Macro _macro;
	bool _recordingMacro = false;
	bool _playingBackMacro = false;
	bool _recordingSaved = false;
	RunMacroDlg _runMacroDlg;

	// For conflict detection when saving Macros or RunCommands
	ShortcutMapper* _pShortcutMapper = nullptr;

	// For hotspot
	bool _linkTriggered = true;
	bool _isFolding = false;

	//For Dynamic selection highlight
	Sci_CharacterRangeFull _prevSelectedRange;

	//Synchronized Scolling
	struct SyncInfo final
	{
		intptr_t _line = 0;
		intptr_t _column = 0;
		bool _isSynScollV = false;
		bool _isSynScollH = false;

		bool doSync() const {return (_isSynScollV || _isSynScollH); }
	}
	_syncInfo;

	bool _isUDDocked = false;

	trayIconControler* _pTrayIco = nullptr;
	intptr_t _zoomOriginalValue = 0;

	Accelerator _accelerator;
	ScintillaAccelerator _scintaccelerator;

	PluginsManager _pluginsManager;
    ButtonDlg _restoreButton;

	bool _isFileOpening = false;
	bool _isAdministrator = false;

	bool _isNppSessionSavedAtExit = false; // guard flag, it prevents emptying of the Notepad++ session.xml in case of multiple WM_ENDSESSION or WM_CLOSE messages

	ScintillaCtrls _scintillaCtrls4Plugins;

	std::vector<std::pair<int, int> > _hideLinesMarks;
	StyleArray _hotspotStyles;

	AnsiCharPanel* _pAnsiCharPanel = nullptr;
	ClipboardHistoryPanel* _pClipboardHistoryPanel = nullptr;
	VerticalFileSwitcher* _pDocumentListPanel = nullptr;
	ProjectPanel* _pProjectPanel_1 = nullptr;
	ProjectPanel* _pProjectPanel_2 = nullptr;
	ProjectPanel* _pProjectPanel_3 = nullptr;

	FileBrowser* _pFileBrowser = nullptr;

	DocumentMap* _pDocMap = nullptr;
	FunctionListPanel* _pFuncList = nullptr;

	std::vector<HWND> _sysTrayHiddenHwnd;

	BOOL notify(SCNotification *notification);
	void command(int id);

//Document management
	UCHAR _mainWindowStatus = 0; //For 2 views and user dialog if docked
	int _activeView = MAIN_VIEW;

	int _multiSelectFlag = 0; // For sktpping current Multi-select comment 

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
	bool loadStyles();

	int currentView() {
		return _activeView;
	}

	int otherView() {
		return (_activeView == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	}

	int otherFromView(int whichOne) {
		return (whichOne == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
	}

	bool canHideView(int whichOne);	//true if view can safely be hidden (no open docs etc)

	bool isEmpty(); // true if we have 1 view with 1 clean, untitled doc

	int switchEditViewTo(int gid);	//activate other view (set focus etc)

	void docGotoAnotherEditView(FileTransferMode mode);	//TransferMode
	void docOpenInNewInstance(FileTransferMode mode, int x = 0, int y = 0);

	void loadBufferIntoView(BufferID id, int whichOne, bool dontClose = false);		//Doesnt _activate_ the buffer
	bool removeBufferFromView(BufferID id, int whichOne);	//Activates alternative of possible, or creates clean document if not clean already

	bool activateBuffer(BufferID id, int whichOne, bool forceApplyHilite = false);			//activate buffer in that view if found
	void notifyBufferActivated(BufferID bufid, int view);
	void performPostReload(int whichOne);
//END: Document management

	int doSaveOrNot(const wchar_t *fn, bool isMulti = false);
	int doReloadOrNot(const wchar_t *fn, bool dirty);
	int doCloseOrNot(const wchar_t *fn);
	int doDeleteOrNot(const wchar_t *fn);
	int doSaveAll();

	void enableMenu(int cmdID, bool doEnable) const;
	void enableCommand(int cmdID, bool doEnable, int which) const;
	void checkClipboard();
	void checkDocState();
	void checkUndoState();
	void checkMacroState();
	void checkSyncState();
	void setupColorSampleBitmapsOnMainMenuItems();
	void dropFiles(HDROP hdrop);
	void checkModifiedDocument(bool bCheckOnlyCurrentBuffer);

    void getMainClientRect(RECT & rc) const;
	void staticCheckMenuAndTB() const;
	void dynamicCheckMenuAndTB() const;
	void enableConvertMenuItems(EolType f) const;
	void checkUnicodeMenuItems() const;

	std::wstring getLangDesc(LangType langType, bool getName = false);

	void setLangStatus(LangType langType);

	void setDisplayFormat(EolType f);
	void setUniModeText();
	void checkLangsMenu(int id) const ;
    void setLanguage(LangType langType);
	LangType menuID2LangType(int cmdID);

	BOOL processIncrFindAccel(MSG *msg) const;
	BOOL processFindAccel(MSG *msg) const;
	BOOL processTabSwitchAccel(MSG* msg) const;

	void checkMenuItem(int itemID, bool willBeChecked) const {
		::CheckMenuItem(_mainMenuHandle, itemID, MF_BYCOMMAND | (willBeChecked?MF_CHECKED:MF_UNCHECKED));
	}

	bool isConditionExprLine(intptr_t lineNumber);
	intptr_t findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol);
	void maintainIndentation(wchar_t ch);

	void addHotSpot(ScintillaEditView* view = nullptr);
	void removeAllHotSpot();

    void bookmarkAdd(intptr_t lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		if (!bookmarkPresent(lineno))
			_pEditView->execute(SCI_MARKERADD, lineno, MARK_BOOKMARK);
	}

    void bookmarkDelete(size_t lineno) const {
		if (lineno == static_cast<size_t>(-1))
			lineno = _pEditView->getCurrentLineNumber();
		while (bookmarkPresent(lineno))
			_pEditView->execute(SCI_MARKERDELETE, lineno, MARK_BOOKMARK);
	}

    bool bookmarkPresent(intptr_t lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();
		LRESULT state = _pEditView->execute(SCI_MARKERGET, lineno);
		return ((state & (1 << MARK_BOOKMARK)) != 0);
	}

    void bookmarkToggle(intptr_t lineno) const {
		if (lineno == -1)
			lineno = _pEditView->getCurrentLineNumber();

		if (bookmarkPresent(lineno))
			bookmarkDelete(lineno);
		else
			bookmarkAdd(lineno);
	}

    void bookmarkNext(bool forwardScan);
	void bookmarkClearAll() const {
		_pEditView->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
	}

	void copyMarkedLines();
	void cutMarkedLines();
	void deleteMarkedLines(bool isMarked);
	void pasteToMarkedLines();
	void deleteMarkedline(size_t ln);
	void inverseMarks();
	void replaceMarkedline(size_t ln, const wchar_t *str);
	std::wstring getMarkedLine(size_t ln);
    void findMatchingBracePos(intptr_t& braceAtCaret, intptr_t& braceOpposite);
    bool braceMatch();

    void activateNextDoc(bool direction);
	void activateDoc(size_t pos);

	void updateStatusBar();
	size_t getSelectedCharNumber(UniMode);
	size_t getCurrentDocCharCount(UniMode u);
	size_t getSelectedAreas();
	size_t getSelectedBytes();
	bool isFormatUnicode(UniMode);
	int getBOMSize(UniMode);

	void showAutoComp();
	void autoCompFromCurrentFile(bool autoInsert = true);
	void showFunctionComp();
	void showPathCompletion();
	void showFunctionNextHint(bool isNext = true);

	//void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
	void setCodePageForInvisibleView(Buffer const* pBuffer);
	bool replaceInOpenedFiles();
	bool findInOpenedFiles();
	bool findInCurrentFile(bool isEntireDoc);

	void getMatchedFileNames(const wchar_t *dir, size_t level, const std::vector<std::wstring> & patterns, std::vector<std::wstring> & fileNames, bool isRecursive, bool isInHiddenDir);
	void doSynScorll(HWND hW);
	void setWorkingDir(const wchar_t *dir);
	bool str2Cliboard(const std::wstring & str2cpy);

	bool getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible);
	int getLangFromMenuName(const wchar_t * langName);
	std::wstring getLangFromMenu(const Buffer * buf);

    std::wstring exts2Filters(const std::wstring& exts, int maxExtsLen = -1) const; // maxExtsLen default value -1 makes no limit of whole exts length
	int setFileOpenSaveDlgFilters(CustomFileDialog & fDlg, bool showAllExt, int langType = -1); // showAllExt should be true if it's used for open file dialog - all set exts should be used for filtering files
	Style * getStyleFromName(const wchar_t *styleName);
	bool dumpFiles(const wchar_t * outdir, const wchar_t * fileprefix = L"");	//helper func
	void drawTabbarColoursFromStylerArray();
	void drawAutocompleteColoursFromTheme(COLORREF fgColor, COLORREF bgColor);
	void drawDocumentMapColoursFromStylerArray();

	std::vector<std::wstring> loadCommandlineParams(const wchar_t * commandLine, const CmdLineParams * pCmdParams) {
		const CmdLineParamsDTO dto = CmdLineParamsDTO::FromCmdLineParams(*pCmdParams);
		return loadCommandlineParams(commandLine, &dto);
	}
	std::vector<std::wstring> loadCommandlineParams(const wchar_t * commandLine, const CmdLineParamsDTO * pCmdParams);
	bool noOpenedDoc() const;
	bool goToPreviousIndicator(int indicID2Search, bool isWrap = true) const;
	bool goToNextIndicator(int indicID2Search, bool isWrap = true) const;
	int wordCount();

	void wsTabConvert(spaceTab whichWay);
	void doTrim(trimOp whichPart);
	void eol2ws();
	void removeEmptyLine(bool isBlankContained);
	void removeDuplicateLines();
	void launchAnsiCharPanel();
	void launchClipboardHistoryPanel();
	void launchDocumentListPanel(bool changeFromBtnCmd = false);
	void changeDocumentListIconSet(bool changeFromBtnCmd);
	void checkProjectMenuItem();
	void launchProjectPanel(int cmdID, ProjectPanel ** pProjPanel, int panelID);
	void launchDocMap();
	void launchFunctionList();
	void launchFileBrowser(const std::vector<std::wstring> & folders, const std::wstring& selectedItemPath, bool fromScratch = false);
	void showAllQuotes() const;
	static DWORD WINAPI threadTextPlayer(void *text2display);
	static DWORD WINAPI threadTextTroller(void *params);
	static int getRandomAction(int ranNum);
	static bool deleteBack(ScintillaEditView *pCurrentView, BufferID targetBufID);
	static bool deleteForward(ScintillaEditView *pCurrentView, BufferID targetBufID);
	static bool selectBack(ScintillaEditView *pCurrentView, BufferID targetBufID);

	static int getRandomNumber(int rangeMax = -1) {
		int randomNumber = rand();
		if (rangeMax == -1)
			return randomNumber;
		return (rand() % rangeMax);
	}

	static DWORD WINAPI backupDocument(void *params);

	static DWORD WINAPI monitorFileOnChange(void * params);
	struct MonitorInfo final {
		MonitorInfo(Buffer *buf, HWND nppHandle) :
			_buffer(buf), _nppHandle(nppHandle) {};
		Buffer *_buffer = nullptr;
		HWND _nppHandle = nullptr;
	};

	void monitoringStartOrStopAndUpdateUI(Buffer* pBuf, bool isStarting);
	void createMonitoringThread(Buffer* pBuf);
	void updateCommandShortcuts();

	HBITMAP generateSolidColourMenuItemIcon(COLORREF colour);

	void clearChangesHistory();
	void changedHistoryGoTo(int idGoTo);

	HMENU createMenuFromMenu(HMENU hSourceMenu, const std::vector<int>& commandIds);
	BOOL notifyTBShowMenu(LPNMTOOLBARW lpnmtb, const char* menuPosId);
	BOOL notifyTBShowMenu(LPNMTOOLBARW lpnmtb, const char* menuPosId, const std::vector<int>& cmdIDs);
};
