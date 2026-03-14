     1|// This file is part of npminmin project
     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|#pragma once
    18|
    19|#include "ScintillaEditView.h"
    20|#include "DocTabView.h"
    21|#include "SplitterContainer.h"
    22|#include "FindReplaceDlg.h"
    23|#include "AboutDlg.h"
    24|#include "RunDlg.h"
    25|#include "StatusBar.h"
    26|#include "lastRecentFileList.h"
    27|#include "GoToLineDlg.h"
    28|#include "FindCharsInRange.h"
    29|#include "columnEditor.h"
    30|#include "WordStyleDlg.h"
    31|#include "trayIconControler.h"
    32|#include "PluginsManager.h"
    33|#include "preferenceDlg.h"
    34|#include "WindowsDlg.h"
    35|#include "RunMacroDlg.h"
    36|#include "DockingManager.h"
    37|#include "Processus.h"
    38|#include "AutoCompletion.h"
    39|#include "SmartHighlighter.h"
    40|#include "ScintillaCtrls.h"
    41|#include "lesDlgs.h"
    42|#include "pluginsAdmin.h"
    43|#include "localization.h"
    44|#include "documentSnapshot.h"
    45|#include "md5Dlgs.h"
    46|#include <vector>
    47|#include <iso646.h>
    48|#include <chrono>
    49|#include <atomic>
    50|
    51|extern std::chrono::steady_clock::time_point g_nppStartTimePoint;
    52|extern std::chrono::steady_clock::duration g_pluginsLoadingTime;
    53|
    54|extern std::atomic<bool> g_bNppExitFlag;
    55|
    56|
    57|#define MENU 0x01
    58|#define TOOLBAR 0x02
    59|
    60|#define MAIN_EDIT_ZONE true
    61|
    62|enum FileTransferMode {
    63|	TransferClone		= 0x01,
    64|	TransferMove		= 0x02
    65|};
    66|
    67|enum WindowStatus {	//bitwise mask
    68|	WindowMainActive	= 0x01,
    69|	WindowSubActive		= 0x02,
    70|	WindowBothActive	= 0x03,	//little helper shortcut
    71|	WindowUserActive	= 0x04,
    72|	WindowMask			= 0x07
    73|};
    74|
    75|enum trimOp {
    76|	lineHeader = 0,
    77|	lineTail = 1,
    78|	lineBoth = 2,
    79|	lineEol = 3
    80|};
    81|
    82|enum spaceTab {
    83|	tab2Space = 0,
    84|	space2TabLeading = 1,
    85|	space2TabAll = 2
    86|};
    87|
    88|struct TaskListInfo;
    89|
    90|
    91|struct VisibleGUIConf final
    92|{
    93|	bool _isPostIt = false;
    94|	bool _isFullScreen = false;
    95|	bool _isDistractionFree = false;
    96|
    97|	//Used by postit & fullscreen
    98|	bool _isMenuShown = true;
    99|	DWORD_PTR _preStyle = (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
   100|
   101|	//used by postit
   102|	bool _isTabbarShown = true;
   103|	bool _isAlwaysOnTop = false;
   104|	bool _isStatusbarShown = true;
   105|
   106|	//used by fullscreen
   107|	WINDOWPLACEMENT _winPlace = {};
   108|
   109|	//used by distractionFree
   110|	bool _was2ViewModeOn = false;
   111|	std::vector<DockingCont*> _pVisibleDockingContainers;
   112|};
   113|
   114|struct QuoteParams
   115|{
   116|	enum Speed { slow = 0, rapid, speedOfLight };
   117|
   118|	QuoteParams() {}
   119|	QuoteParams(const wchar_t* quoter, Speed speed, bool shouldBeTrolling, int encoding, LangType lang, const wchar_t* quote) :
   120|		_quoter(quoter), _speed(speed), _shouldBeTrolling(shouldBeTrolling), _encoding(encoding), _lang(lang), _quote(quote) {}
   121|
   122|	void reset() {
   123|		_quoter = nullptr;
   124|		_speed = rapid;
   125|		_shouldBeTrolling = false;
   126|		_encoding = SC_CP_UTF8;
   127|		_lang = L_TEXT;
   128|		_quote = nullptr;
   129|	}
   130|
   131|	const wchar_t* _quoter = nullptr;
   132|	Speed _speed = rapid;
   133|	bool _shouldBeTrolling = false;
   134|	int _encoding = SC_CP_UTF8;
   135|	LangType _lang = L_TEXT;
   136|	const wchar_t* _quote = nullptr;
   137|};
   138|
   139|class CustomFileDialog;
   140|class Notepad_plus_Window;
   141|class AnsiCharPanel;
   142|class ClipboardHistoryPanel;
   143|class VerticalFileSwitcher;
   144|class ProjectPanel;
   145|class DocumentMap;
   146|class FunctionListPanel;
   147|class FileBrowser;
   148|struct QuoteParams;
   149|
   150|class Notepad_plus final
   151|{
   152|friend class Notepad_plus_Window;
   153|friend class FileManager;
   154|
   155|public:
   156|	Notepad_plus();
   157|	~Notepad_plus();
   158|
   159|	LRESULT init(HWND hwnd);
   160|	LRESULT process(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
   161|	void killAllChildren();
   162|
   163|	enum comment_mode {cm_comment, cm_uncomment, cm_toggle};
   164|
   165|	void setTitle();
   166|	void getTaskListInfo(TaskListInfo *tli);
   167|	size_t getNbDirtyBuffer(int view);
   168|
   169|
   170|	// The following functions apply to a single buffer and don't need to worry about views, with the exception of doClose,
   171|	// since closing one view doesn't have to mean the document is gone
   172|	BufferID doOpen(const std::wstring& fileName, bool isRecursive = false, bool isReadOnly = false, int encoding = -1, const wchar_t *backupFileName = NULL, FILETIME fileNameTimestamp = {});
   173|	bool doReload(BufferID id, bool alert = true);
   174|	bool doSave(BufferID, const wchar_t * filename, bool isSaveCopy = false);
   175|	void doClose(BufferID, int whichOne, bool doDeleteBackup = false);
   176|
   177|
   178|	void fileOpen();
   179|	void fileNew();
   180|    bool fileReload();
   181|	bool fileClose(BufferID id = BUFFER_INVALID, int curView = -1);	//use curView to override view to close from
   182|	bool fileCloseAll(bool doDeleteBackup, bool isSnapshotMode = false);
   183|	bool fileCloseAllButCurrent();
   184|	void fileCloseAllButPinned();
   185|	bool fileCloseAllGiven(const std::vector<BufferViewInfo>& krvecBuffer);
   186|	bool fileCloseAllToLeft();
   187|	bool fileCloseAllToRight();
   188|	bool fileCloseAllUnchanged();
   189|	bool fileSave(BufferID id = BUFFER_INVALID);
   190|	bool fileSaveAllConfirm();
   191|	bool fileSaveAll();
   192|	bool fileSaveSpecific(const std::wstring& fileNameToSave);
   193|	bool fileSaveAs(BufferID id = BUFFER_INVALID, bool isSaveCopy = false);
   194|	bool fileDelete(BufferID id = BUFFER_INVALID);
   195|	bool fileRename(BufferID id = BUFFER_INVALID);
   196|	bool fileRenameUntitledPluginAPI(BufferID id, const wchar_t* tabNewName);
   197|	bool useFirstLineAsTabName(BufferID id);
   198|
   199|	void unPinnedForAllBuffers();
   200|	bool switchToFile(BufferID buffer);			//find buffer in active view then in other view.
   201|	//@}
   202|
   203|	bool isFileSession(const wchar_t * filename);
   204|	bool isFileWorkspace(const wchar_t * filename);
   205|	void filePrint(bool showDialog);
   206|	void saveScintillasZoom();
   207|
   208|	bool saveGUIParams();
   209|	bool saveProjectPanelsParams();
   210|	bool saveFileBrowserParam();
   211|	bool saveColumnEditorParams();
   212|	void saveDockingParams();
   213|    void saveUserDefineLangs();
   214|    void saveShortcuts();
   215|	void saveSession(const Session & session);
   216|	void saveCurrentSession();
   217|	void saveFindHistory();
   218|
   219|	void getCurrentOpenedFiles(Session& session, bool includeUntitledDoc = false);
   220|
   221|	bool fileLoadSession(const wchar_t* fn = nullptr);
   222|	const wchar_t * fileSaveSession(size_t nbFile, wchar_t ** fileNames, const wchar_t *sessionFile2save, bool includeFileBrowser = false);
   223|	const wchar_t * fileSaveSession(size_t nbFile = 0, wchar_t** fileNames = nullptr);
   224|
   225|	bool doBlockComment(comment_mode currCommentMode);
   226|	bool doStreamComment();
   227|	bool undoStreamComment(bool tryBlockComment = true);
   228|
   229|	bool addCurrentMacro();
   230|	void macroPlayback(Macro macro, std::vector<Document>* pDocs4EndUAIn = nullptr);
   231|
   232|    void loadLastSession();
   233|	bool loadSession(Session & session, bool isSnapshotMode = false, const wchar_t* userCreatedSessionName = nullptr);
   234|
   235|	void prepareBufferChangedDialog(Buffer * buffer);
   236|	void notifyBufferChanged(Buffer * buffer, int mask);
   237|	bool findInFinderFiles(FindersInfo *findInFolderInfo);
   238|
   239|	bool createFilelistForFiles(std::vector<std::wstring> & fileNames);
   240|	bool createFilelistForProjects(std::vector<std::wstring> & fileNames);
   241|	bool findInFiles();
   242|	bool findInProjects();
   243|	bool findInFilelist(std::vector<std::wstring> & fileList);
   244|	bool replaceInFiles();
   245|	bool replaceInProjects();
   246|	bool replaceInFilelist(std::vector<std::wstring> & fileList);
   247|
   248|	void setFindReplaceFolderFilter(const wchar_t *dir, const wchar_t *filters);
   249|	std::vector<std::wstring> addNppComponents(const wchar_t *destDir, const wchar_t *extFilterName, const wchar_t *extFilter);
   250|	std::vector<std::wstring> addNppPlugins(const wchar_t *extFilterName, const wchar_t *extFilter);
   251|    int getHtmlXmlEncoding(const wchar_t *fileName) const;
   252|
   253|	HACCEL getAccTable() const {
   254|		return _accelerator.getAccTable();
   255|	}
   256|
   257|	bool emergency(const std::wstring& emergencySavedDir);
   258|
   259|	Buffer* getCurrentBuffer()	{
   260|		return _pEditView->getCurrentBuffer();
   261|	}
   262|
   263|	void launchDocumentBackupTask();
   264|	int getQuoteIndexFrom(const wchar_t* quoter) const;
   265|	void showQuoteFromIndex(int index) const;
   266|	void showQuote(const QuoteParams* quote) const;
   267|
   268|	std::wstring getPluginListVerStr() const {
   269|		return _pluginsAdminDlg.getPluginListVerStr();
   270|	}
   271|
   272|	void minimizeDialogs();
   273|	void restoreMinimizeDialogs();
   274|
   275|	void refreshDarkMode(bool resetStyle = false);
   276|
   277|	void refreshInternalPanelIcons();
   278|
   279|	void changeReadOnlyUserModeForAllOpenedTabs(const bool ro);
   280|
   281|private:
   282|	Notepad_plus_Window* _pPublicInterface = nullptr;
   283|    Window* _pMainWindow = nullptr;
   284|	DockingManager _dockingManager;
   285|	std::vector<int> _internalFuncIDs;
   286|
   287|	AutoCompletion _autoCompleteMain;
   288|	AutoCompletion _autoCompleteSub; // each Scintilla has its own autoComplete
   289|
   290|	SmartHighlighter _smartHighlighter;
   291|    NativeLangSpeaker _nativeLangSpeaker;
   292|    DocTabView _mainDocTab;
   293|    DocTabView _subDocTab;
   294|    DocTabView* _pDocTab = nullptr;
   295|	DocTabView* _pNonDocTab = nullptr;
   296|
   297|    ScintillaEditView _subEditView = ScintillaEditView(MAIN_EDIT_ZONE);  // only _mainEditView and _subEditView are MAIN_EDIT_ZONE comparing with other Scintilla controls
   298|    ScintillaEditView _mainEditView = ScintillaEditView(MAIN_EDIT_ZONE); // only _mainEditView and _subEditView are MAIN_EDIT_ZONE comparing with other Scintilla controls
   299|	ScintillaEditView _invisibleEditView; // for searches
   300|	ScintillaEditView _fileEditView;      // for FileManager
   301|    ScintillaEditView* _pEditView = nullptr;
   302|	ScintillaEditView* _pNonEditView = nullptr;
   303|
   304|    SplitterContainer* _pMainSplitter = nullptr;
   305|    SplitterContainer _subSplitter;
   306|
   307|    ContextMenu _tabPopupMenu;
   308|	ContextMenu _tabPopupDropMenu;
   309|	ContextMenu _fileSwitcherMultiFilePopupMenu;
   310|
   311|	ToolBar	_toolBar;
   312|
   313|    StatusBar _statusBar;
   314|	ReBar _rebarTop;
   315|	ReBar _rebarBottom;
   316|
   317|	// Dialog
   318|	FindReplaceDlg _findReplaceDlg;
   319|	FindInFinderDlg _findInFinderDlg;
   320|
   321|	FindIncrementDlg _incrementFindDlg;
   322|    AboutDlg _aboutDlg;
   323|	DebugInfoDlg _debugInfoDlg;
   324|	CmdLineArgsDlg _cmdLineArgsDlg;
   325|	RunDlg _runDlg;
   326|	HashFromFilesDlg _md5FromFilesDlg;
   327|	HashFromTextDlg _md5FromTextDlg;
   328|	HashFromFilesDlg _sha2FromFilesDlg;
   329|	HashFromTextDlg _sha2FromTextDlg;
   330|	HashFromFilesDlg _sha1FromFilesDlg;
   331|	HashFromTextDlg _sha1FromTextDlg;
   332|	HashFromFilesDlg _sha512FromFilesDlg;
   333|	HashFromTextDlg _sha512FromTextDlg;
   334|    GoToLineDlg _goToLineDlg;
   335|	ColumnEditorDlg _colEditorDlg;
   336|	WordStyleDlg _configStyleDlg;
   337|	PreferenceDlg _preference;
   338|	FindCharsInRangeDlg _findCharsInRangeDlg;
   339|	PluginsAdminDlg _pluginsAdminDlg;
   340|	DocumentPeeker _documentPeeker;
   341|
   342|	// a handle list of all the npminmin dialogs
   343|	std::vector<HWND> _hModelessDlgs;
   344|
   345|	LastRecentFileList _lastRecentFileList;
   346|
   347|	WindowsMenu _windowsMenu;
   348|	HMENU _mainMenuHandle = NULL;
   349|
   350|	bool _sysMenuEntering = false;
   351|
   352|	// make sure we don't recursively call doClose when closing the last file with -quitOnEmpty
   353|	bool _isAttemptingCloseOnQuit = false;
   354|
   355|	// For FullScreen/PostIt/DistractionFree features
   356|	VisibleGUIConf	_beforeSpecialView;
   357|	void fullScreenToggle();
   358|	void postItToggle();
   359|	void distractionFreeToggle();
   360|
   361|	// Keystroke macro recording and playback
   362|	Macro _macro;
   363|	bool _recordingMacro = false;
   364|	bool _playingBackMacro = false;
   365|	bool _recordingSaved = false;
   366|	RunMacroDlg _runMacroDlg;
   367|
   368|	// For conflict detection when saving Macros or RunCommands
   369|	ShortcutMapper* _pShortcutMapper = nullptr;
   370|
   371|	// For hotspot
   372|	bool _linkTriggered = true;
   373|	bool _isFolding = false;
   374|
   375|	//For Dynamic selection highlight
   376|	Sci_CharacterRangeFull _prevSelectedRange;
   377|
   378|	//Synchronized Scrolling
   379|	struct SyncInfo final
   380|	{
   381|		intptr_t _line = 0;
   382|		intptr_t _column = 0;
   383|		bool _isSynScrollV = false;
   384|		bool _isSynScrollH = false;
   385|
   386|		bool doSync() const {return (_isSynScrollV || _isSynScrollH); }
   387|	}
   388|	_syncInfo;
   389|
   390|	bool _isUDDocked = false;
   391|
   392|	trayIconControler* _pTrayIco = nullptr;
   393|	intptr_t _zoomOriginalValue = 0;
   394|
   395|	Accelerator _accelerator;
   396|	ScintillaAccelerator _scintaccelerator;
   397|
   398|	PluginsManager _pluginsManager;
   399|    ButtonDlg _restoreButton;
   400|
   401|	bool _isFileOpening = false;
   402|	bool _isAdministrator = false;
   403|
   404|	bool _isNppSessionSavedAtExit = false; // guard flag, it prevents emptying of the npminmin session.xml in case of multiple WM_ENDSESSION or WM_CLOSE messages
   405|
   406|	ScintillaCtrls _scintillaCtrls4Plugins;
   407|
   408|	std::vector<std::pair<int, int> > _hideLinesMarks;
   409|	StyleArray _hotspotStyles;
   410|
   411|	AnsiCharPanel* _pAnsiCharPanel = nullptr;
   412|	ClipboardHistoryPanel* _pClipboardHistoryPanel = nullptr;
   413|	VerticalFileSwitcher* _pDocumentListPanel = nullptr;
   414|	ProjectPanel* _pProjectPanel_1 = nullptr;
   415|	ProjectPanel* _pProjectPanel_2 = nullptr;
   416|	ProjectPanel* _pProjectPanel_3 = nullptr;
   417|
   418|	FileBrowser* _pFileBrowser = nullptr;
   419|
   420|	DocumentMap* _pDocMap = nullptr;
   421|	FunctionListPanel* _pFuncList = nullptr;
   422|
   423|	std::vector<HWND> _sysTrayHiddenHwnd;
   424|
   425|	BOOL notify(SCNotification *notification);
   426|	void command(int id);
   427|
   428|//Document management
   429|	UCHAR _mainWindowStatus = 0; //For 2 views and user dialog if docked
   430|	int _activeView = MAIN_VIEW;
   431|
   432|	int _multiSelectFlag = 0; // For skipping current Multi-select comment 
   433|
   434|	//User dialog docking
   435|	void dockUserDlg();
   436|    void undockUserDlg();
   437|
   438|	//View visibility
   439|	void showView(int whichOne);
   440|	bool viewVisible(int whichOne);
   441|	void hideView(int whichOne);
   442|	void hideCurrentView();
   443|	bool bothActive() { return (_mainWindowStatus & WindowBothActive) == WindowBothActive; }
   444|	bool reloadLang();
   445|	bool loadStyles();
   446|
   447|	int currentView() {
   448|		return _activeView;
   449|	}
   450|
   451|	int otherView() {
   452|		return (_activeView == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
   453|	}
   454|
   455|	int otherFromView(int whichOne) {
   456|		return (whichOne == MAIN_VIEW?SUB_VIEW:MAIN_VIEW);
   457|	}
   458|
   459|	bool canHideView(int whichOne);	//true if view can safely be hidden (no open docs etc)
   460|
   461|	bool isEmpty(); // true if we have 1 view with 1 clean, untitled doc
   462|
   463|	int switchEditViewTo(int gid);	//activate other view (set focus etc)
   464|
   465|	void docGotoAnotherEditView(FileTransferMode mode);	//TransferMode
   466|	void docOpenInNewInstance(FileTransferMode mode, int x = 0, int y = 0);
   467|
   468|	void loadBufferIntoView(BufferID id, int whichOne, bool dontClose = false);		//Doesn't _activate_ the buffer
   469|	bool removeBufferFromView(BufferID id, int whichOne);	//Activates alternative of possible, or creates clean document if not clean already
   470|
   471|	bool activateBuffer(BufferID id, int whichOne, bool forceApplyHilite = false);			//activate buffer in that view if found
   472|	void notifyBufferActivated(BufferID bufid, int view);
   473|	void performPostReload(int whichOne);
   474|//END: Document management
   475|
   476|	int doSaveOrNot(const wchar_t *fn, bool isMulti = false);
   477|	int doReloadOrNot(const wchar_t *fn, bool dirty);
   478|	int doCloseOrNot(const wchar_t *fn);
   479|	int doDeleteOrNot(const wchar_t *fn);
   480|	int doSaveAll();
   481|
   482|	void enableMenu(int cmdID, bool doEnable) const;
   483|	void enableCommand(int cmdID, bool doEnable, int which) const;
   484|	void checkClipboard();
   485|	void checkDocState();
   486|	void checkUndoState();
   487|	void checkMacroState();
   488|	void checkSyncState();
   489|	void setupColorSampleBitmapsOnMainMenuItems();
   490|	void dropFiles(HDROP hdrop);
   491|	void checkModifiedDocument(bool bCheckOnlyCurrentBuffer);
   492|
   493|    void getMainClientRect(RECT & rc) const;
   494|	void staticCheckMenuAndTB() const;
   495|	void dynamicCheckMenuAndTB() const;
   496|	void enableConvertMenuItems(EolType f) const;
   497|	void checkUnicodeMenuItems() const;
   498|
   499|	std::wstring getLangDesc(LangType langType, bool getName = false);
   500|
   501|