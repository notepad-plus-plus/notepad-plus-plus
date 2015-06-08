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


#ifndef FIND_REPLACE_DLG_H
#define FIND_REPLACE_DLG_H

#ifndef FINDREPLACE_DLG_H
#include "FindReplaceDlg_rc.h"
#endif //FINDREPLACE_DLG_H

#ifndef SCINTILLA_EDIT_VIEW_H
#include "ScintillaEditView.h"
#endif //SCINTILLA_EDIT_VIEW_H

#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "BoostRegexSearch.h"
#include "StatusBar.h"

#define FIND_RECURSIVE 1
#define FIND_INHIDDENDIR 2

#define FINDREPLACE_MAXLENGTH 2048

enum DIALOG_TYPE {FIND_DLG, REPLACE_DLG, FINDINFILES_DLG, MARK_DLG};

#define DIR_DOWN true
#define DIR_UP false

//#define FIND_REPLACE_STR_MAX 256

enum InWhat{ALL_OPEN_DOCS, FILES_IN_DIR, CURRENT_DOC};

struct FoundInfo {
	FoundInfo(int start, int end, const TCHAR *fullPath)
		: _start(start), _end(end), _fullPath(fullPath) {};
	int _start;
	int _end;
	generic_string _fullPath;
};

struct TargetRange {
	int targetStart;
	int targetEnd;
};

enum SearchIncrementalType { NotIncremental, FirstIncremental, NextIncremental };
enum SearchType { FindNormal, FindExtended, FindRegex };
enum ProcessOperation { ProcessFindAll, ProcessReplaceAll, ProcessCountAll, ProcessMarkAll, ProcessMarkAll_2, ProcessMarkAll_IncSearch, ProcessMarkAllExt };

struct FindOption
{
	bool _isWholeWord;
	bool _isMatchCase;
	bool _isWrapAround;
	bool _whichDirection;
	SearchIncrementalType _incrementalType;
	SearchType _searchType;
	bool _doPurge;
	bool _doMarkLine;
	bool _isInSelection;
	generic_string _str2Search;
	generic_string _str4Replace;
	generic_string _filters;
	generic_string _directory;
	bool _isRecursive;
	bool _isInHiddenDir;
	bool _dotMatchesNewline;
	FindOption() : _isWholeWord(true), _isMatchCase(true), _searchType(FindNormal),\
		_isWrapAround(true), _whichDirection(DIR_DOWN), _incrementalType(NotIncremental), 
		_doPurge(false), _doMarkLine(false),
		_isInSelection(false),  _isRecursive(true), _isInHiddenDir(false),
		_dotMatchesNewline(false),
		_filters(TEXT("")), _directory(TEXT("")) {};
};

//This class contains generic search functions as static functions for easy access
class Searching {
public:
	static int convertExtendedToString(const TCHAR * query, TCHAR * result, int length);
	static TargetRange t;
	static int buildSearchFlags(const FindOption * option) {
		return	(option->_isWholeWord ? SCFIND_WHOLEWORD : 0) |
				(option->_isMatchCase ? SCFIND_MATCHCASE : 0) |
				(option->_searchType == FindRegex ? SCFIND_REGEXP|SCFIND_POSIX : 0) |
				((option->_searchType == FindRegex && option->_dotMatchesNewline) ? SCFIND_REGEXP_DOTMATCHESNL : 0);
	};
	static void displaySectionCentered(int posStart, int posEnd, ScintillaEditView * pEditView, bool isDownwards = true);

private:
	static bool readBase(const TCHAR * str, int * value, int base, int size);

};

//Finder: Dockable window that contains search results
class Finder : public DockingDlgInterface {
friend class FindReplaceDlg;
public:
	Finder() : DockingDlgInterface(IDD_FINDRESULT), _pMainFoundInfos(&_foundInfos1), _pMainMarkings(&_markings1) {
		_MarkingsStruct._length = 0;
		_MarkingsStruct._markings = NULL;
	};

	~Finder() {
		_scintView.destroy();
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

	void addSearchLine(const TCHAR *searchName);
	void addFileNameTitle(const TCHAR * fileName);
	void addFileHitCount(int count);
	void addSearchHitCount(int count);
	void add(FoundInfo fi, SearchResultMarking mi, const TCHAR* foundline, int lineNb);
	void setFinderStyle();
	void removeAll();
	void openAll();
	void copy();
	void beginNewFilesSearch();
	void finishFilesSearch(int count);
	void gotoNextFoundResult(int direction);
	void GotoFoundLine();
	void DeleteResult();

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool notify(SCNotification *notification);

private:

	enum { searchHeaderLevel = SC_FOLDLEVELBASE + 1, fileHeaderLevel, resultLevel };

	ScintillaEditView **_ppEditView;
	std::vector<FoundInfo> _foundInfos1;
	std::vector<FoundInfo> _foundInfos2;
	std::vector<FoundInfo>* _pMainFoundInfos;
	std::vector<SearchResultMarking> _markings1;
	std::vector<SearchResultMarking> _markings2;
	std::vector<SearchResultMarking>* _pMainMarkings;
	SearchResultMarkings _MarkingsStruct;

	ScintillaEditView _scintView;
	unsigned int nFoundFiles;

	int _lastFileHeaderPos;
	int _lastSearchHeaderPos;

	void setFinderReadOnly(bool isReadOnly) {
		_scintView.execute(SCI_SETREADONLY, isReadOnly);
	};

	bool isLineActualSearchResult(int line) const;
	generic_string prepareStringForClipboard(generic_string s) const;

	static FoundInfo EmptyFoundInfo;
	static SearchResultMarking EmptySearchResultMarking;
};


enum FindStatus { FSFound, FSNotFound, FSTopReached, FSEndReached, FSMessage, FSNoMessage};

enum FindNextType {
	FINDNEXTTYPE_FINDNEXT,
	FINDNEXTTYPE_REPLACENEXT,
	FINDNEXTTYPE_FINDNEXTFORREPLACE
};


class FindReplaceDlg : public StaticDialog
{
friend class FindIncrementDlg;
public :
	static FindOption _options;
	static FindOption* _env;
	FindReplaceDlg() : StaticDialog(), _pFinder(NULL), _isRTL(false),\
		_fileNameLenMax(1024) {
		_uniFileName = new char[(_fileNameLenMax + 3) * 2];
		_winVer = (NppParameters::getInstance())->getWinVersion();
		_env = &_options;
	};
	~FindReplaceDlg();

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw std::runtime_error("FindIncrementDlg::init : ppEditView is null.");
		_ppEditView = ppEditView;
	};

	virtual void create(int dialogID, bool isRTL = false);
	
	void initOptionsFromDlg();

	void doDialog(DIALOG_TYPE whichType, bool isRTL = false, bool toShow = true);
	bool processFindNext(const TCHAR *txt2find, const FindOption *options = NULL, FindStatus *oFindStatus = NULL, FindNextType findNextType = FINDNEXTTYPE_FINDNEXT);
	bool processReplace(const TCHAR *txt2find, const TCHAR *txt2replace, const FindOption *options = NULL);

	int markAll(const TCHAR *txt2find, int styleID, bool isWholeWordSelected);
	//int markAll2(const TCHAR *str2find);
	int markAllInc(const FindOption *opt);
	

	int processAll(ProcessOperation op, const FindOption *opt, bool isEntire = false, const TCHAR *fileName = NULL, int colourStyleID = -1);
//	int processAll(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, bool isEntire = false, const TCHAR *fileName = NULL, const FindOption *opt = NULL, int colourStyleID = -1);
	int processRange(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, int startRange, int endRange, const TCHAR *fileName = NULL, const FindOption *opt = NULL, int colourStyleID = -1);
	void replaceAllInOpenedDocs();
	void findAllIn(InWhat op);
	void setSearchText(TCHAR * txt2find);

	void gotoNextFoundResult(int direction = 0) {if (_pFinder) _pFinder->gotoNextFoundResult(direction);};

	void putFindResult(int result) {
		_findAllResult = result;
	};
	const TCHAR * getDir2Search() const {return _env->_directory.c_str();};

	void getPatterns(std::vector<generic_string> & patternVect);

	void launchFindInFilesDlg() {
		doDialog(FINDINFILES_DLG);
	};

	void setFindInFilesDirFilter(const TCHAR *dir, const TCHAR *filters);

	generic_string getText2search() const {
		return _env->_str2Search;
	};

	const generic_string & getFilters() const {return _env->_filters;};
	const generic_string & getDirectory() const {return _env->_directory;};
	const FindOption & getCurrentOptions() const {return *_env;};
	bool isRecursive() const { return _env->_isRecursive; };
	bool isInHiddenDir() const { return _env->_isInHiddenDir; };
	void saveFindHistory();
	void changeTabName(DIALOG_TYPE index, const TCHAR *name2change) {
		TCITEM tie;
		tie.mask = TCIF_TEXT;
		tie.pszText = (TCHAR *)name2change;
		TabCtrl_SetItem(_tab.getHSelf(), index, &tie);
	}
	void beginNewFilesSearch()
	{
		_pFinder->beginNewFilesSearch();
		_pFinder->addSearchLine(getText2search().c_str());
	}

	void finishFilesSearch(int count)
	{
		_pFinder->finishFilesSearch(count);
	}

	void focusOnFinder() {
		// Show finder and set focus
		if (_pFinder) 
		{
			::SendMessage(_hParent, NPPM_DMMSHOW, 0, (LPARAM)_pFinder->getHSelf());
			_pFinder->_scintView.getFocus();
		}
	};

	HWND getHFindResults() {
		if (_pFinder)
			return _pFinder->_scintView.getHSelf();
		return NULL;
	}

	void updateFinderScintilla() {
		if (_pFinder && _pFinder->isCreated() && _pFinder->isVisible())
		{
			_pFinder->setFinderStyle();
		}
	};

	void execSavedCommand(int cmd, int intValue, generic_string stringValue);
	void setStatusbarMessage(const generic_string & msg, FindStatus staus);


protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void addText2Combo(const TCHAR * txt2add, HWND comboID, bool isUTF8 = false);
	generic_string getTextFromCombo(HWND hCombo, bool isUnicode = false) const;
	static LONG originalFinderProc;

	// Window procedure for the finder
	static LRESULT FAR PASCAL finderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void combo2ExtendedMode(int comboID);

private :

	DIALOG_TYPE _currentStatus;
	RECT _findClosePos, _replaceClosePos, _findInFilesClosePos;

	ScintillaEditView **_ppEditView;
	Finder  *_pFinder;
	bool _isRTL;

	int _findAllResult;
	TCHAR _findAllResultStr[1024];

	int _fileNameLenMax;
	char *_uniFileName;

	TabBar _tab;
	winVer _winVer;
	StatusBar _statusBar;
	FindStatus _statusbarFindStatus;

	

	void enableReplaceFunc(bool isEnable);
	void enableFindInFilesControls(bool isEnable = true);
	void enableFindInFilesFunc();
	void enableMarkAllControls(bool isEnable);
	void enableMarkFunc();

	void setDefaultButton(int nID) {
		SendMessage(_hSelf, DM_SETDEFID, (WPARAM)nID, 0L);
	};

	void gotoCorrectTab() {
		int currentIndex = _tab.getCurrentTabIndex();
		if (currentIndex != _currentStatus)
			_tab.activateAt(_currentStatus);
	};
	
	FindStatus getFindStatus() {
		return this->_statusbarFindStatus;
	}

	void updateCombos();
	void updateCombo(int comboID) {
		bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
		HWND hCombo = ::GetDlgItem(_hSelf, comboID);
		addText2Combo(getTextFromCombo(hCombo, isUnicode).c_str(), hCombo, isUnicode);
	};
	void fillFindHistory();
    void fillComboHistory(int id, const std::vector<generic_string> & strings);
	int saveComboHistory(int id, int maxcount, std::vector<generic_string> & strings);
	static const int FR_OP_FIND = 1;
	static const int FR_OP_REPLACE = 2;
	static const int FR_OP_FIF = 4;
	static const int FR_OP_GLOBAL = 8;
	void saveInMacro(int cmd, int cmdType);
	void drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

};

//FindIncrementDlg: incremental search dialog, docked in rebar
class FindIncrementDlg : public StaticDialog
{
public :
	FindIncrementDlg() : _pFRDlg(NULL), _pRebar(NULL), _findStatus(FSFound) {};
	void init(HINSTANCE hInst, HWND hPere, FindReplaceDlg *pFRDlg, bool isRTL = false);
	virtual void destroy();
	virtual void display(bool toShow = true) const;

	void setSearchText(const TCHAR * txt2find, bool) {
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
	};

	void setFindStatus(FindStatus iStatus, int nbCounted);
	
	FindStatus getFindStatus() {
		return _findStatus;
	}

	void addToRebar(ReBar * rebar);
private :
	bool _isRTL;
	FindReplaceDlg *_pFRDlg;
	FindStatus _findStatus;

	ReBar * _pRebar;
	REBARBANDINFO _rbBand;

	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void markSelectedTextInc(bool enable, FindOption *opt = NULL);
};


class Progress
{
public:
	Progress(HINSTANCE hInst);
	~Progress();

	HWND open(HWND hCallerWnd = NULL, const TCHAR* header = NULL);
	void close();

	bool isCancelled() const
	{
		if (_hwnd)
			return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
		return false;
	}

	void setInfo(const TCHAR *info) const
	{
		if (_hwnd)
			::SendMessage(_hPText, WM_SETTEXT, 0, (LPARAM)info);
	}

	void setPercent(unsigned percent, const TCHAR *fileName) const;
	void flushCallerUserInput() const;

private:
	static const TCHAR cClassName[];
	static const TCHAR cDefaultHeader[];
	static const int cBackgroundColor;
	static const int cPBwidth;
	static const int cPBheight;
	static const int cBTNwidth;
	static const int cBTNheight;

	static volatile LONG refCount;

	static DWORD WINAPI threadFunc(LPVOID data);
	static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	// Disable copy construction and operator=
	Progress(const Progress&);
	const Progress& operator=(const Progress&);

	int thread();
	int createProgressWindow();
	RECT adjustSizeAndPos(int width, int height);

	HINSTANCE _hInst;
	volatile HWND _hwnd;
	HWND _hCallerWnd;
	TCHAR _header[128];
	HANDLE _hThread;
	HANDLE _hActiveState;
	HWND _hPText;
	HWND _hPBar;
	HWND _hBtn;
};

#endif //FIND_REPLACE_DLG_H
