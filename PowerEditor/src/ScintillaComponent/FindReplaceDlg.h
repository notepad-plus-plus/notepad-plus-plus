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

#include <map>
#include "FindReplaceDlg_rc.h"
#include "ScintillaEditView.h"
#include "DockingDlgInterface.h"
#include "BoostRegexSearch.h"
#include "StatusBar.h"

#define FIND_RECURSIVE 1
#define FIND_INHIDDENDIR 2

#define FIND_INVALID_REGULAR_EXPRESSION -2

#define FINDREPLACE_MAXLENGTH 2048

#define FINDTEMPSTRING_MAXSIZE 1024*1024

enum DIALOG_TYPE {FIND_DLG, REPLACE_DLG, FINDINFILES_DLG, FINDINPROJECTS_DLG, MARK_DLG};

#define DIR_DOWN true
#define DIR_UP false

#define FIND_STATUS_END_REACHED_TEXT L"Find: Reached document end, first occurrence from the top found."
#define FIND_STATUS_TOP_REACHED_TEXT L"Find:  Reached document beginning, first occurrence from the bottom found."
#define FIND_STATUS_REPLACE_END_REACHED_TEXT L"Replace: Reached document end, started from top."
#define FIND_STATUS_REPLACE_TOP_REACHED_TEXT L"Replace: Reached document beginning, started from bottom."

enum InWhat{ALL_OPEN_DOCS, FILES_IN_DIR, CURRENT_DOC, CURR_DOC_SELECTION, FILES_IN_PROJECTS};

struct FoundInfo {
	FoundInfo(intptr_t start, intptr_t end, size_t lineNumber, const wchar_t *fullPath)
		: _lineNumber(lineNumber), _fullPath(fullPath) {
		_ranges.push_back(std::pair<intptr_t, intptr_t>(start, end));
	};
	std::vector<std::pair<intptr_t, intptr_t>> _ranges;
	size_t _lineNumber = 0;
	std::wstring _fullPath;
};

struct TargetRange {
	int targetStart;
	int targetEnd;
};

enum SearchIncrementalType { NotIncremental, FirstIncremental, NextIncremental };
enum SearchType { FindNormal, FindExtended, FindRegex };
enum ProcessOperation { ProcessFindAll, ProcessReplaceAll, ProcessCountAll, ProcessMarkAll, ProcessMarkAll_2, ProcessMarkAll_IncSearch, ProcessMarkAllExt, ProcessFindInFinder };

struct FindOption
{
	bool _isWholeWord = true;
	bool _isMatchCase = true;
	bool _isWrapAround = true;
	bool _whichDirection = DIR_DOWN;
	SearchIncrementalType _incrementalType = NotIncremental;
	SearchType _searchType = FindNormal;
	bool _doPurge = false;
	bool _doMarkLine = false;
	bool _isInSelection = false;
	std::wstring _str2Search;
	std::wstring _str4Replace;
	std::wstring _filters;
	std::wstring _directory;
	bool _isRecursive = true;
	bool _isInHiddenDir = false;
	bool _isProjectPanel_1 = false;
	bool _isProjectPanel_2 = false;
	bool _isProjectPanel_3 = false;
	bool _dotMatchesNewline = false;
	bool _isMatchLineNumber = false; // always false for main search
};

//This class contains generic search functions as static functions for easy access
class Searching {
public:
	static int convertExtendedToString(const wchar_t * query, wchar_t * result, int length);
	static TargetRange t;
	static int buildSearchFlags(const FindOption * option) {
		return	(option->_isWholeWord ? SCFIND_WHOLEWORD : 0) |
				(option->_isMatchCase ? SCFIND_MATCHCASE : 0) |
				(option->_searchType == FindRegex ? SCFIND_REGEXP|SCFIND_POSIX : 0) |
				((option->_searchType == FindRegex && option->_dotMatchesNewline) ? SCFIND_REGEXP_DOTMATCHESNL : 0);
	};
	static void displaySectionCentered(size_t posStart, size_t posEnd, ScintillaEditView * pEditView, bool isDownwards = true);

private:
	static bool readBase(const wchar_t * str, int * value, int base, int size);

};

//Finder: Dockable window that contains search results
class Finder : public DockingDlgInterface {
friend class FindReplaceDlg;
public:

	Finder() : DockingDlgInterface(IDD_FINDRESULT) {
		_markingsStruct._length = 0;
		_markingsStruct._markings = NULL;
	};

	~Finder() {
		_scintView.destroy();
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

	void addSearchLine(const wchar_t *searchName);
	void addFileNameTitle(const wchar_t * fileName);
	void addFileHitCount(int count);
	void addSearchResultInfo(int count, int countSearched, bool searchedEntireNotSelection, const FindOption *pFindOpt);
	const char* foundLine(FoundInfo fi, SearchResultMarkingLine mi, const wchar_t* foundline, size_t totalLineNumber);
	void setFinderStyle();
	void setFinderStyleForNpc(bool onlyColor = false);
	void removeAll();
	void openAll();
	void wrapLongLinesToggle();
	void purgeToggle();
	void copy();
	void copyPathnames();
	void beginNewFilesSearch();
	void finishFilesSearch(int count, int searchedCount, bool searchedEntireNotSelection, const FindOption *pFindOpt);
	
	void gotoNextFoundResult(int direction);
	std::pair<intptr_t, intptr_t> gotoFoundLine(size_t nOccurrence = 0); // value 0 means this argument is not used
	void deleteResult();
	std::vector<std::wstring> getResultFilePaths(bool onlyInSelectedText) const;
	bool canFind(const wchar_t *fileName, size_t lineNumber, size_t* indexToStartFrom) const;
	void setVolatiled(bool val) { _canBeVolatiled = val; };
	std::wstring getHitsString(int count) const;

	LRESULT scintillaExecute(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const {
		return _scintView.execute(msg, wParam, lParam);
	}
protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	bool notify(SCNotification *notification);

private:
	enum { searchHeaderLevel = SC_FOLDLEVELBASE, fileHeaderLevel, resultLevel };

	enum CurrentPosInLineStatus { pos_infront, pos_between, pos_inside, pos_behind };

	struct CurrentPosInLineInfo {
		CurrentPosInLineStatus _status = pos_infront;
		intptr_t auxiliaryInfo = -1; // according the status - see getCurrentPosInLineInfo member function definition's above comment
	};

	ScintillaEditView **_ppEditView = nullptr;
	std::vector<FoundInfo> _foundInfos1;
	std::vector<FoundInfo> _foundInfos2;
	std::vector<FoundInfo>* _pMainFoundInfos = &_foundInfos1;
	std::vector<SearchResultMarkingLine> _markings1;
	std::vector<SearchResultMarkingLine> _markings2;
	std::vector<SearchResultMarkingLine>* _pMainMarkings = &_markings1;
	SearchResultMarkings _markingsStruct;
	intptr_t _previousLineNumber = -1;

	ScintillaEditView _scintView;
	unsigned int _nbFoundFiles = 0;

	intptr_t _lastFileHeaderPos = 0;
	intptr_t _lastSearchHeaderPos = 0;

	bool _canBeVolatiled = true;
	bool _longLinesAreWrapped = false;
	bool _purgeBeforeEverySearch = false;

	std::wstring _prefixLineStr;

	void setFinderReadOnly(bool isReadOnly) {
		_scintView.execute(SCI_SETREADONLY, isReadOnly);
	};

	bool isLineActualSearchResult(const std::wstring & s) const;
	std::wstring & prepareStringForClipboard(std::wstring & s) const;

	static FoundInfo EmptyFoundInfo;
	static SearchResultMarkingLine EmptySearchResultMarking;

	CurrentPosInLineInfo getCurrentPosInLineInfo(intptr_t currentPosInLine, const SearchResultMarkingLine& markingLine) const;
	void anchorWithNoHeaderLines(intptr_t& currentL, intptr_t initL, intptr_t minL, intptr_t maxL, int direction);
};


enum FindStatus { FSFound, FSNotFound, FSTopReached, FSEndReached, FSMessage, FSNoMessage};

enum FindNextType {
	FINDNEXTTYPE_FINDNEXT,
	FINDNEXTTYPE_REPLACENEXT,
	FINDNEXTTYPE_FINDNEXTFORREPLACE
};

struct FindReplaceInfo
{
	const wchar_t *_txt2find = nullptr;
	const wchar_t *_txt2replace = nullptr;
	intptr_t _startRange = -1;
	intptr_t _endRange = -1;
};

struct FindersInfo
{
	Finder *_pSourceFinder = nullptr;
	Finder *_pDestFinder = nullptr;
	const wchar_t *_pFileName = nullptr;

	FindOption _findOption;
};

class FindInFinderDlg : public StaticDialog
{
public:
	void doDialog(Finder *launcher, bool isRTL = false);
	FindOption & getOption() { return _options; }
	FindInFinderDlg() {
		_options._isMatchCase = false;
		_options._isWholeWord = false;
		_options._isMatchLineNumber = true;
	};

private:
	Finder  *_pFinder2Search = nullptr;
	FindOption _options;
	
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void initFromOptions();
	void writeOptions();
};

LRESULT run_swapButtonProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class FindReplaceDlg : public StaticDialog
{
friend class FindIncrementDlg;
public :
	static FindOption _options;
	static FindOption* _env;
	FindReplaceDlg() {
		_uniFileName = new char[(_fileNameLenMax + 3) * 2];
		_winVer = (NppParameters::getInstance()).getWinVersion();
		_env = &_options;
	};

	~FindReplaceDlg();

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw std::runtime_error("FindIncrementDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	};

	void create(int dialogID, bool isRTL = false, bool msgDestParent = true, bool toShow = true);
	
	void initOptionsFromDlg();

	void doDialog(DIALOG_TYPE whichType, bool isRTL = false, bool toShow = true);
	bool processFindNext(const wchar_t *txt2find, const FindOption *options = NULL, FindStatus *oFindStatus = NULL, FindNextType findNextType = FINDNEXTTYPE_FINDNEXT);
	bool processReplace(const wchar_t *txt2find, const wchar_t *txt2replace, const FindOption *options = NULL);

	int markAll(const wchar_t *txt2find, int styleID);
	int markAllInc(const FindOption *opt);
	

	int processAll(ProcessOperation op, const FindOption *opt, bool isEntire = false, const FindersInfo *pFindersInfo = nullptr, int colourStyleID = -1);
	int processRange(ProcessOperation op, FindReplaceInfo & findReplaceInfo, const FindersInfo *pFindersInfo, const FindOption *opt = nullptr, int colourStyleID = -1, ScintillaEditView *view2Process = nullptr);

	void replaceAllInOpenedDocs();
	void findAllIn(InWhat op);
	void setSearchText(wchar_t * txt2find);

	void gotoNextFoundResult(int direction = 0) const {
		if (_pFinder) _pFinder->gotoNextFoundResult(direction);
	};

	void putFindResult(int result) {
		_findAllResult = result;
	};
	const wchar_t * getDir2Search() const {return _env->_directory.c_str();};

	void getPatterns(std::vector<std::wstring> & patternVect);
	void getAndValidatePatterns(std::vector<std::wstring> & patternVect);

	void launchFindInFilesDlg() {
		doDialog(FINDINFILES_DLG);
	};

	void launchFindInProjectsDlg() {
		doDialog(FINDINPROJECTS_DLG);
	};

	void setFindInFilesDirFilter(const wchar_t *dir, const wchar_t *filters);
	void setProjectCheckmarks(FindHistory *findHistory, int Msk);
	void enableProjectCheckmarks();

	std::wstring getText2search() const {
		return _env->_str2Search;
	};

	const std::wstring & getFilters() const {return _env->_filters;};
	const std::wstring & getDirectory() const {return _env->_directory;};
	const FindOption & getCurrentOptions() const {return *_env;};
	bool isRecursive() const { return _env->_isRecursive; };
	bool isInHiddenDir() const { return _env->_isInHiddenDir; };
	bool isProjectPanel_1() const { return _env->_isProjectPanel_1; };
	bool isProjectPanel_2() const { return _env->_isProjectPanel_2; };
	bool isProjectPanel_3() const { return _env->_isProjectPanel_3; };
	void saveFindHistory();
	void changeTabName(DIALOG_TYPE index, const wchar_t *name2change) {
		TCITEM tie{};
		tie.mask = TCIF_TEXT;
		tie.pszText = (wchar_t *)name2change;
		TabCtrl_SetItem(_tab.getHSelf(), index, &tie);

		wchar_t label[MAX_PATH]{};
		_tab.getCurrentTitle(label, MAX_PATH);
		::SetWindowText(_hSelf, label);
	}
	void beginNewFilesSearch()
	{
		_pFinder->beginNewFilesSearch();
		_pFinder->addSearchLine(getText2search().c_str());
	}

	void finishFilesSearch(int count, int searchedCount, bool searchedEntireNotSelection)
	{
		_pFinder->finishFilesSearch(count, searchedCount, searchedEntireNotSelection, _env);
	}

	void focusOnFinder() {
		// Show finder and set focus
		if (_pFinder) 
		{
			_pFinder->display();
			_pFinder->_scintView.grabFocus();
		}
	};

	bool allowCopyAction() {
		HWND focusedHwnd = GetFocus();
		Finder* finder = getFinderFrom(focusedHwnd);

		if (finder)
		{
			return finder->_scintView.hasSelection();
		}
		return false;
	};

	HWND getHFindResults() {
		if (_pFinder)
			return _pFinder->_scintView.getHSelf();
		return NULL;
	}

	void updateFinderScintilla() {
		if (_pFinder && _pFinder->isCreated())
		{
			_pFinder->setFinderStyle();

			if (!_findersOfFinder.empty())
			{
				for (const auto& finder : _findersOfFinder)
				{
					finder->setFinderStyle();
				}
			}
		}
	};

	void updateFinderScintillaForNpc(bool onlyColor = false) {
		if (_pFinder && _pFinder->isCreated())
		{
			_pFinder->setFinderStyleForNpc(onlyColor);

			if (!_findersOfFinder.empty())
			{
				for (const auto& finder : _findersOfFinder)
				{
					finder->setFinderStyleForNpc();
				}
			}
		}
	};

	void execSavedCommand(int cmd, uptr_t intValue, const std::wstring& stringValue);
	void clearMarks(const FindOption& opt);
	void setStatusbarMessage(const std::wstring & msg, FindStatus staus, const std::wstring& tooltipMsg = L"");
	void setStatusbarMessageWithRegExprErr(ScintillaEditView* pEditView);
	std::wstring getScopeInfoForStatusBar(FindOption const *pFindOpt) const;
	Finder * createFinder();
	bool removeFinder(Finder *finder2remove);
	DIALOG_TYPE getCurrentStatus() {return _currentStatus;};
	Finder* getFinderFrom(HWND hwnd);
	int regexBackwardMsgBox();

protected :
	void resizeDialogElements();
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	static WNDPROC originalFinderProc;
	static WNDPROC originalComboEditProc;

	static LRESULT FAR PASCAL comboEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Window procedure for the finder
	static LRESULT FAR PASCAL finderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void combo2ExtendedMode(int comboID);

private:
	SIZE _szMinDialog{};
	SIZE _szBorder{};

	LONG _lesssModeHeight = 0;

	DIALOG_TYPE _currentStatus = DIALOG_TYPE::FIND_DLG;

	ScintillaEditView** _ppEditView = nullptr;
	Finder  *_pFinder = nullptr;
	std::wstring _findResTitle;

	std::vector<Finder*> _findersOfFinder{};

	HWND _shiftTrickUpTip = nullptr;
	HWND _2ButtonsTip = nullptr;
	HWND _filterTip = nullptr;
	HWND _dirFromActiveDocTip = nullptr;

	bool _isRTL = false;

	int _findAllResult = 0;
	wchar_t _findAllResultStr[1024] = {'\0'};

	int _fileNameLenMax = 1024;
	char *_uniFileName = nullptr;

	TabBar _tab;
	winVer _winVer = winVer::WV_UNKNOWN;
	StatusBar _statusBar;
	FindStatus _statusbarFindStatus;

	std::wstring _statusbarTooltipMsg;
	HWND _statusbarTooltipWnd = nullptr;
	HICON _statusbarTooltipIcon = nullptr;
	int _statusbarTooltipIconSize = 0;

	HFONT _hComboBoxFont = nullptr;
	HFONT _hLargerBolderFont = nullptr;
	HFONT _hCourrierNewFont = nullptr;

	std::map<int, bool> _controlEnableMap;

	std::vector<int> _reduce2hide_find = { IDC_IN_SELECTION_CHECK, IDC_REPLACEINSELECTION, IDC_FINDALL_CURRENTFILE, IDCANCEL };
	std::vector<int> _reduce2hide_findReplace = { IDC_IN_SELECTION_CHECK, IDC_REPLACEINSELECTION, IDREPLACEALL, IDCANCEL };
	std::vector<int> _reduce2hide_fif = { IDD_FINDINFILES_FILTERS_STATIC, IDD_FINDINFILES_FILTERS_COMBO, IDCANCEL };
	std::vector<int> _reduce2hide_fip = { IDD_FINDINFILES_FILTERS_STATIC, IDD_FINDINFILES_FILTERS_COMBO, IDCANCEL };
	std::vector<int> _reduce2hide_mark = { IDC_MARKLINE_CHECK, IDC_PURGE_CHECK, IDC_IN_SELECTION_CHECK, IDC_REPLACEINSELECTION, IDC_COPY_MARKED_TEXT, IDCANCEL };

	void enableFindDlgItem(int dlgItemID, bool isEnable = true);
	void showFindDlgItem(int dlgItemID, bool isShow = true);

	void enableReplaceFunc(bool isEnable);
	void enableFindInFilesControls(bool isEnable, bool projectPanels);
	void enableFindInFilesFunc();
	void enableFindInProjectsFunc();
	void enableMarkAllControls(bool isEnable);
	void enableMarkFunc();
	void hideOrShowCtrl4reduceOrNormalMode(DIALOG_TYPE dlgT);
	void calcAndSetCtrlsPos(DIALOG_TYPE dlgT, bool fromColBtn = false);

	void setDefaultButton(int nID) {
		SendMessage(_hSelf, DM_SETDEFID, nID, 0L);
	};

	void gotoCorrectTab() {
		auto currentIndex = _tab.getCurrentTabIndex();
		if (currentIndex != _currentStatus)
			_tab.activateAt(_currentStatus);
	};
	
	FindStatus getFindStatus() {
		return _statusbarFindStatus;
	}

	void updateCombos();
	void updateCombo(int comboID);
	void fillFindHistory();
	void fillComboHistory(int id, const std::vector<std::wstring> & strings);
	int saveComboHistory(int id, int maxcount, std::vector<std::wstring> & strings, bool saveEmpty);
	static const int FR_OP_FIND = 1;
	static const int FR_OP_REPLACE = 2;
	static const int FR_OP_FIF = 4;
	static const int FR_OP_GLOBAL = 8;
	static const int FR_OP_FIP = 16;
	void saveInMacro(size_t cmd, int cmdType);
	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	bool replaceInFilesConfirmCheck(const std::wstring& directory, const std::wstring& fileTypes);
	bool replaceInProjectsConfirmCheck();
	bool replaceInOpenDocsConfirmCheck(void);

	ContextMenu _swapPopupMenu;
	enum SwapButtonStatus {swap, down, up} _swapButtonStatus = swap;
	HWND _hSwapButton = nullptr;
	static LRESULT CALLBACK swapButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (FindReplaceDlg*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_swapButtonProc(dlg->_oldSwapButtonProc, hwnd, message, wParam, lParam));
	};
	WNDPROC _oldSwapButtonProc = nullptr;
};

//FindIncrementDlg: incremental search dialog, docked in rebar
class FindIncrementDlg : public StaticDialog
{
public :
	FindIncrementDlg() = default;
	void init(HINSTANCE hInst, HWND hPere, FindReplaceDlg *pFRDlg, bool isRTL = false);
	void destroy() override;
	void display(bool toShow = true) const override;

	void setSearchText(const wchar_t* txt2find, bool) {
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(txt2find));
	};

	void setFindStatus(FindStatus iStatus, int nbCounted);
	
	FindStatus getFindStatus() {
		return _findStatus;
	}

	void addToRebar(ReBar* rebar);
private :
	bool _isRTL = false;
	FindReplaceDlg *_pFRDlg = nullptr;
	FindStatus _findStatus = FSFound;

	ReBar* _pRebar = nullptr;
	REBARBANDINFO _rbBand{};

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void markSelectedTextInc(bool enable, FindOption *opt = NULL);
};


class Progress
{
public:
	explicit Progress(HINSTANCE hInst);
	~Progress();

	// Disable copy construction and operator=
	Progress(const Progress&) = delete;
	const Progress& operator=(const Progress&) = delete;

	HWND open(HWND hCallerWnd, const wchar_t* header = NULL);
	void close();

	bool isCancelled() const
	{
		if (_hwnd)
			return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
		return false;
	}

	void setPercent(unsigned percent, const wchar_t* fileName, int nbHitsSoFar) const;
	void setInfo(const wchar_t* info, int nbHitsSoFar = -1) const;

private:
	static const wchar_t cClassName[];
	static const wchar_t cDefaultHeader[];
	static const int cBackgroundColor;
	static const SIZE _szClient;

	static volatile LONG refCount;

	static DWORD WINAPI threadFunc(LPVOID data);
	static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	int thread();
	int createProgressWindow();

	RECT getDpiScaledWindowRect(UINT dpi) const;
	void setCtrlsPos();
	void setFont();

	HINSTANCE _hInst = nullptr;
	volatile HWND _hwnd = nullptr;
	HWND _hCallerWnd = nullptr;
	wchar_t _header[128] = {'\0'};
	HANDLE _hThread = nullptr;
	HANDLE _hActiveState = nullptr;
	HWND _hPathText = nullptr;
	HWND _hRunningHitsStaticText = nullptr;
	HWND _hRunningHitsText = nullptr;
	HWND _hPBar = nullptr;
	HWND _hBtn = nullptr;
	HFONT _hFont = nullptr;

	DPIManagerV2 _dpiManager;
};
