//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
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

#ifndef FIND_REPLACE_DLG_H
#define FIND_REPLACE_DLG_H

#include "StaticDialog.h"
#include "FindReplaceDlg_rc.h"
#include "Buffer.h"
#include "ScintillaEditView.h"
#include "StatusBar.h"
#include "DockingDlgInterface.h"

/*
typedef bool DIALOG_TYPE;
#define REPLACE true
#define FIND false
*/
enum DIALOG_TYPE {FIND_DLG, REPLACE_DLG, FINDINFILES_DLG};

#define DIR_DOWN true
#define DIR_UP false

//#define FIND_REPLACE_STR_MAX 256

typedef bool InWhat;
#define ALL_OPEN_DOCS true
#define FILES_IN_DIR false

struct FoundInfo {
	FoundInfo(int start, int end, const char *foundLine, const char *fullPath, size_t lineNum)
		: _start(start), _end(end), _foundLine(foundLine), _fullPath(fullPath), _scintLineNumber(lineNum){};
	int _start;
	int _end;
	std::string _foundLine;
	std::string _fullPath;
	size_t _scintLineNumber;
};

struct TargetRange {
	int targetStart;
	int targetEnd;
};

enum SearchType { FindNormal, FindExtended, FindRegex };
enum ProcessOperation { ProcessFindAll, ProcessReplaceAll, ProcessCountAll, ProcessMarkAll, ProcessMarkAll_2 };

struct FindOption {
	bool _isWholeWord;
	bool _isMatchCase;
	bool _isWrapAround;
	bool _whichDirection;
	bool _isIncremental;
	SearchType _searchType;
	FindOption() :_isWholeWord(true), _isMatchCase(true), _searchType(FindNormal),\
		_isWrapAround(true), _whichDirection(DIR_DOWN), _isIncremental(false){};
};

//This class contains generic search functions as static functions for easy access
class Searching {
public:
	static int convertExtendedToString(const char * query, char * result, int length);
	static TargetRange t;
	static int buildSearchFlags(FindOption * option) {
		return	(option->_isWholeWord ? SCFIND_WHOLEWORD : 0) |
				(option->_isMatchCase ? SCFIND_MATCHCASE : 0) |
				(option->_searchType == FindRegex ? SCFIND_REGEXP|SCFIND_POSIX : 0);
	};
private:
	static bool readBase(const char * string, int * value, int base, int size);

};

//Finder: Dockable window that contains search results
class Finder : public DockingDlgInterface {
friend class FindReplaceDlg;
public:
	Finder() : DockingDlgInterface(IDD_FINDRESULT), _markedLine(-1), _lineCounter(0) {};

	~Finder() {
		_scintView.destroy();
	}
	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
	};

	void addFileNameTitle(const char *fileName) {
		string str = "[";
		str += fileName;
		str += "]\n";

		_scintView.execute(SCI_APPENDTEXT, str.length(), (LPARAM)str.c_str());
		_lineCounter++;
	};

	void add(FoundInfo fi, int lineNb) {
		_foundInfos.push_back(fi);
		std::string str = "Line ";

		char lnb[16];
		str += itoa(lineNb, lnb, 10);
		str += " : ";
		str += fi._foundLine;
		_scintView.execute(SCI_APPENDTEXT, str.length(), (LPARAM)str.c_str());
		_lineCounter++;
	};

	void setFinderStyle();

	void removeAll() {
		_markedLine = -1;
		_foundInfos.clear();
		_scintView.execute(SCI_CLEARALL);
		_lineCounter = 0;
	};

	FoundInfo & getInfo(int curLineNum) {
		int nbInfo = _foundInfos.size();

		for (size_t i = (nbInfo <= curLineNum)?nbInfo -1:curLineNum ; i > 0 ; i--)
		{
			if (_foundInfos[i]._scintLineNumber == curLineNum)
				return _foundInfos[i];
		}
		return _foundInfos[0]; // should never be reached
	};

	bool isEmpty() const {
		return _foundInfos.empty();
	};

	int getCurrentMarkedLine() const {return _markedLine;};
	void setCurrentMarkedLine(int line) {_markedLine = line;};
	InWhat getMode() const {return _mode;};
	void setMode(InWhat mode) {_mode = mode;};
	
	void setSearchWord(const char *word2search) {
		_scintView.setKeywords(L_SEARCHRESULT, word2search, 0);
	};



protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool notify(SCNotification *notification);

private:
	ScintillaEditView **_ppEditView;
	std::vector<FoundInfo> _foundInfos;
	ScintillaEditView _scintView;
	int _markedLine;
	InWhat _mode;
	size_t _lineCounter;
};

//FindReplaceDialog: standard find/replace window
class FindReplaceDlg : public StaticDialog
{
friend class FindIncrementDlg;
public :
	FindReplaceDlg() : StaticDialog(), _pFinder(NULL), _isRTL(false), _isRecursive(true), _maxNbCharAllocated(1024), _fileNameLenMax(1024) {
		_line = new char[_maxNbCharAllocated + 3];
		_uniCharLine = new char[(_maxNbCharAllocated + 3) * 2];
		_uniFileName = new char[(_fileNameLenMax + 3) * 2];
		_winVer = (NppParameters::getInstance())->getWinVersion();
		//strcpy(_findAllResultStr, FIND_RESULT_DEFAULT_TITLE);
	};
	~FindReplaceDlg() {
		_tab.destroy();
		if (_pFinder)
			delete _pFinder;

		delete [] _line;	
		delete [] _uniCharLine;
		delete [] _uniFileName;
	};

	void init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView) {
		Window::init(hInst, hPere);
		if (!ppEditView)
			throw int(9900);
		_ppEditView = ppEditView;
	};

	virtual void create(int dialogID, bool isRTL = false);
	
	void initOptionsFromDlg()	{
		_options._isWholeWord = isCheckedOrNot(IDWHOLEWORD);
		_options._isMatchCase = isCheckedOrNot(IDMATCHCASE);
		_options._searchType = isCheckedOrNot(IDREGEXP)?FindRegex:isCheckedOrNot(IDEXTENDED)?FindExtended:FindNormal;
		_options._isWrapAround = isCheckedOrNot(IDWRAP);
		_isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK);

		// Set Direction : Down by default
		_options._whichDirection = DIR_DOWN;
		::SendMessage(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), BM_SETCHECK, BST_CHECKED, 0);

		_doPurge = isCheckedOrNot(IDC_PURGE_CHECK);
		_doMarkLine = isCheckedOrNot(IDC_MARKLINE_CHECK);
		_doStyleFoundToken = isCheckedOrNot(IDC_STYLEFOUND_CHECK);

		::EnableWindow(::GetDlgItem(_hSelf, IDCMARKALL), (_doMarkLine || _doStyleFoundToken));
	};

	void doDialog(DIALOG_TYPE whichType, bool isRTL = false) {
		if (!isCreated())
		{
			create(IDD_FIND_REPLACE_DLG, isRTL);
			_isRTL = isRTL;
		}

		if (whichType == FINDINFILES_DLG)
			enableFindInFilesFunc();
		else
			enableReplaceFunc(whichType == REPLACE_DLG);

		::SetFocus(::GetDlgItem(_hSelf, IDFINDWHAT));
		display();
	};
	bool processFindNext(const char *txt2find, FindOption *options = NULL);
	bool processReplace(const char *txt2find, const char *txt2replace, FindOption *options = NULL);

	int markAll(const char *str2find);
	int markAll2(const char *str2find);

	int processAll(ProcessOperation op, const char *txt2find, const char *txt2replace, bool isEntire = false, const char *fileName = NULL);
	void replaceAllInOpenedDocs();
	void findAllIn(InWhat op);
	void setSearchText(const char * txt2find, bool isUTF8 = false) {
		addText2Combo(txt2find, ::GetDlgItem(_hSelf, IDFINDWHAT), isUTF8);
	}

	void setFinderReadOnly(bool isReadOnly = true) {
		_pFinder->_scintView.execute(SCI_SETREADONLY, isReadOnly);
	};

	bool isFinderEmpty() const {
		return _pFinder->isEmpty();
	};

	void clearFinder() {
		_pFinder->removeAll();
	};

	void putFindResult(int result) {
		_findAllResult = result;
	};

	void setSearchWord2Finder(){
		string str2Search = getText2search();
		_pFinder->setSearchWord(str2Search.c_str());
	};

	/// Sets the direction in which to search.
	/// \param dir Direction to search (DIR_UP or DIR_DOWN)
	///
	void setSearchDirection(bool dir) {
		_options._whichDirection = dir;
	};

	const char * getDir2Search() const {return _directory.c_str();};

	void getPatterns(vector<string> & patternVect);

	void launchFindInFilesDlg() {
		//::SendMessage(_hSelf, WM_COMMAND, IDC_FINDINFILES, 0);
		doDialog(FINDINFILES_DLG);
	};

	void setFindInFilesDirFilter(const char *dir, const char *filters) {
		if (dir)
		{
			_directory = dir;
			::SetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, dir);
		}
		if (filters)
		{
			_filters = filters;
			::SetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters);
		}
	};

	string getText2search() const {
		bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;
		return getTextFromCombo(::GetDlgItem(_hSelf, IDFINDWHAT), isUnicode);
	};

	const string & getFilters() const {return _filters;};
	const string & getDirectory() const {return _directory;};

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void addText2Combo(const char * txt2add, HWND comboID, bool isUTF8 = false);
	string getTextFromCombo(HWND hCombo, bool isUnicode) const;

private :
	DIALOG_TYPE _currentStatus;
	FindOption _options;

	bool _doPurge;
	bool _doMarkLine;
	bool _doStyleFoundToken;
	bool _isInSelection;


	RECT _findClosePos, _replaceClosePos, _findInFilesClosePos;

	ScintillaEditView **_ppEditView;
	Finder  *_pFinder;
	//StatusBar _statusBar;
	bool _isRTL;
	//FindInFilesDlg _findInFilesDlg;

	int _findAllResult;
	char _findAllResultStr[128];

	string _filters;
	string _directory;
	bool _isRecursive;

	int _maxNbCharAllocated;
	int _fileNameLenMax;
	char *_line;
	char *_uniCharLine;
	char *_uniFileName;

	TabBar _tab;
	winVer _winVer;

	void enableReplaceFunc(bool isEnable);
	void enableFindInFilesControls(bool isEnable = true);
	void enableFindInFilesFunc() {
		enableFindInFilesControls();

		//::EnableWindow(::GetDlgItem(_hSelf, IDOK), FALSE);

		_currentStatus = FINDINFILES_DLG;
		gotoCorrectTab();
		::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), _findInFilesClosePos.left, _findInFilesClosePos.top, _findInFilesClosePos.right, _findInFilesClosePos.bottom, TRUE);
	};

	void gotoCorrectTab() {
		int currentIndex = _tab.getCurrentTabIndex();
		if (currentIndex != _currentStatus)
			_tab.activateAt(_currentStatus);
	};

	bool isCheckedOrNot(int checkControlID) const {
		return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
	};
	
	void updateCombos();
	void updateCombo(int comboID) {
		bool isUnicode = (*_ppEditView)->getCurrentBuffer().getUnicodeMode() != uni8Bit;
		HWND hCombo = ::GetDlgItem(_hSelf, comboID);
		addText2Combo(getTextFromCombo(hCombo, isUnicode).c_str(), hCombo, isUnicode);
	};
};

//FindIncrementDlg: incremental search dialog, docked in rebar
class FindIncrementDlg : public StaticDialog
{
public :
	FindIncrementDlg() : _pFRDlg(NULL), _pRebar(NULL) {};
	void init(HINSTANCE hInst, HWND hPere, FindReplaceDlg *pFRDlg, bool isRTL = false) {
		Window::init(hInst, hPere);
		if (!pFRDlg)
			throw int(9910);
		_pFRDlg = pFRDlg;
		create(IDD_INCREMENT_FIND, isRTL);
		_isRTL = isRTL;
	};
	virtual void destroy();
	virtual void display(bool toShow = true) const;

	void setSearchText(const char * txt2find, bool isUTF8 = false) {
		_doSearchFromBegin = false;
		if (!isUTF8)
		{
			::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
			return;
		}
		WCHAR wchars[256];
		::MultiByteToWideChar(CP_UTF8, 0, txt2find, -1, wchars, 256 / sizeof(WCHAR));
		::SendDlgItemMessageW(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)wchars);
	}

	void addToRebar(ReBar * rebar);
private :
	bool _isRTL;
	FindReplaceDlg *_pFRDlg;

	ReBar * _pRebar;
	REBARBANDINFO _rbBand;

	HWND _hEditBox, _hSearchPrev, _hSearchNext, _hCheckCase;

	bool _doSearchFromBegin;
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //FIND_REPLACE_DLG_H
