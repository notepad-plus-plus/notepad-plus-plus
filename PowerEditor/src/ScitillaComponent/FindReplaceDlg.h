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


#define FIND_RECURSIVE 1
#define FIND_INHIDDENDIR 2

enum DIALOG_TYPE {FIND_DLG, REPLACE_DLG, FINDINFILES_DLG};

#define DIR_DOWN true
#define DIR_UP false

//#define FIND_REPLACE_STR_MAX 256

typedef bool InWhat;
#define ALL_OPEN_DOCS true
#define FILES_IN_DIR false

struct FoundInfo {
	FoundInfo(int start, int end, const TCHAR *foundLine, const TCHAR *fullPath, size_t lineNum)
		: _start(start), _end(end), _foundLine(foundLine), _fullPath(fullPath), _scintLineNumber(lineNum){};
	int _start;
	int _end;
	std::generic_string _foundLine;
	std::generic_string _fullPath;
	size_t _scintLineNumber;
};

struct TargetRange {
	int targetStart;
	int targetEnd;
};

enum SearchType { FindNormal, FindExtended, FindRegex };
enum ProcessOperation { ProcessFindAll, ProcessReplaceAll, ProcessCountAll, ProcessMarkAll, ProcessMarkAll_2, ProcessMarkAll_IncSearch };

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
	static int convertExtendedToString(const TCHAR * query, TCHAR * result, int length);
	static TargetRange t;
	static int buildSearchFlags(FindOption * option) {
		return	(option->_isWholeWord ? SCFIND_WHOLEWORD : 0) |
				(option->_isMatchCase ? SCFIND_MATCHCASE : 0) |
				(option->_searchType == FindRegex ? SCFIND_REGEXP|SCFIND_POSIX : 0);
	};
	static void displaySectionCentered(int posStart, int posEnd, ScintillaEditView * pEditView, bool isDownwards = true);

private:
	static bool readBase(const TCHAR * str, int * value, int base, int size);

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

	void addFileNameTitle(const TCHAR * fileName) {
		generic_string str = TEXT("[");
		str += fileName;
		str += TEXT("]\n");

		setFinderReadOnly(false);
		_scintView.appandGenericText(str.c_str());
		setFinderReadOnly(true);
		_lineCounter++;
	};

	void add(FoundInfo fi, int lineNb) {
		_foundInfos.push_back(fi);
		std::generic_string str = TEXT("Line ");

		TCHAR lnb[16];
		wsprintf(lnb, TEXT("%d"), lineNb);
		str += lnb;
		str += TEXT(" : ");
		str += fi._foundLine;

		size_t len = str.length();
		if (len >= SC_SEARCHRESULT_LINEBUFFERMAXLENGTH)
		{
			const TCHAR * endOfLongLine = TEXT("...\r\n");
			str = str.substr(0, SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - lstrlen(endOfLongLine) - 1);
			str += endOfLongLine;
		}
		else
		{
			// Make sure we have EOL. We might not have one for example when searching in non-text files.
			// This can happen because Scintilla line endings (\n) are not the same as 
			// string line endings (\0). In this case we will see only a part of the line
			// in the find result window.
			if (str[len-1] != '\n')
				str += TEXT("\n");
		}
		setFinderReadOnly(false);
		_scintView.appandGenericText(str.c_str());
		setFinderReadOnly(true);
		_lineCounter++;
	};

	void setFinderStyle();

	void removeAll() {
		_markedLine = -1;
		_foundInfos.clear();
		setFinderReadOnly(false);
		_scintView.execute(SCI_CLEARALL);
		setFinderReadOnly(true);
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
	
	void setSearchWord(const TCHAR *word2search) {
		_scintView.setHiLiteResultWords(word2search);
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

	void setFinderReadOnly(bool isReadOnly) {
		_scintView.execute(SCI_SETREADONLY, isReadOnly);
	};
};

//FindReplaceDialog: standard find/replace window
class FindReplaceDlg : public StaticDialog
{
friend class FindIncrementDlg;
public :
	FindReplaceDlg() : StaticDialog(), _pFinder(NULL), _isRTL(false), _isRecursive(true),_isInHiddenDir(false),\
		_fileNameLenMax(1024) {
		_uniFileName = new char[(_fileNameLenMax + 3) * 2];
		_winVer = (NppParameters::getInstance())->getWinVersion();
	};
	~FindReplaceDlg() {
		_tab.destroy();
		if (_pFinder)
			delete _pFinder;
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
	bool processFindNext(const TCHAR *txt2find, FindOption *options = NULL);
	bool processReplace(const TCHAR *txt2find, const TCHAR *txt2replace, FindOption *options = NULL);

	int markAll(const TCHAR *str2find);
	int markAll2(const TCHAR *str2find);
	int markAllInc(const TCHAR *str2find, FindOption *opt);

	int processAll(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, bool isEntire = false, const TCHAR *fileName = NULL, FindOption *opt = NULL);
	int processRange(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, int startRange, int endRange, const TCHAR *fileName = NULL, FindOption *opt = NULL);
	void replaceAllInOpenedDocs();
	void findAllIn(InWhat op);

	void setSearchText(TCHAR * txt2find, bool isUTF8 = false) {
		HWND hCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
		if (txt2find && txt2find[0])
		{
			// We got a valid search string
			::SendMessage(hCombo, CB_SETCURSEL, -1, 0); // remove selection - to allow using down arrow to get to last searched word
			::SetDlgItemText(_hSelf, IDFINDWHAT, txt2find);
		}
		::SendMessage(hCombo, CB_SETEDITSEL, 0, MAKELPARAM(0, -1)); // select all text - fast edit
	}

	bool isFinderEmpty() const {
		return _pFinder->isEmpty();
	};

	void clearFinder() {
		_pFinder->removeAll();
	};

	void putFindResult(int result) {
		_findAllResult = result;
	};
	void putFindResultStr(const TCHAR *text);

	void refresh();

	void setSearchWord2Finder(){
		generic_string str2Search = getText2search();
		_pFinder->setSearchWord(str2Search.c_str());
	};

	const TCHAR * getDir2Search() const {return _directory.c_str();};

	void getPatterns(vector<generic_string> & patternVect);

	void launchFindInFilesDlg() {
		doDialog(FINDINFILES_DLG);
	};

	void setFindInFilesDirFilter(const TCHAR *dir, const TCHAR *filters) {
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

	generic_string getText2search() const {
		return getTextFromCombo(::GetDlgItem(_hSelf, IDFINDWHAT));
	};

	const generic_string & getFilters() const {return _filters;};
	const generic_string & getDirectory() const {return _directory;};
	const FindOption & getCurrentOptions() const {return _options;};
	bool isRecursive() const { return _isRecursive; };
	bool isInHiddenDir() const { return _isInHiddenDir; };
	void saveFindHistory();
	void changeTabName(DIALOG_TYPE index, const TCHAR *name2change) {
		TCITEM tie;
		tie.mask = TCIF_TEXT;
		tie.pszText = (TCHAR *)name2change;
		TabCtrl_SetItem(_tab.getHSelf(), index, &tie);
	}

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void addText2Combo(const TCHAR * txt2add, HWND comboID, bool isUTF8 = false);
	generic_string getTextFromCombo(HWND hCombo, bool isUnicode = false) const;

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
	bool _isRTL;

	int _findAllResult;
	TCHAR _findAllResultStr[1024];

	generic_string _filters;
	generic_string _directory;
	bool _isRecursive;
	bool _isInHiddenDir;

	int _fileNameLenMax;
	char *_uniFileName;

	TabBar _tab;
	winVer _winVer;

	void enableReplaceFunc(bool isEnable);
	void enableFindInFilesControls(bool isEnable = true);
	void enableFindInFilesFunc() {
		enableFindInFilesControls();

		_currentStatus = FINDINFILES_DLG;
		gotoCorrectTab();
		::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), _findInFilesClosePos.left, _findInFilesClosePos.top, _findInFilesClosePos.right, _findInFilesClosePos.bottom, TRUE);

		TCHAR label[MAX_PATH];
		_tab.getCurrentTitle(label, MAX_PATH);
		::SetWindowText(_hSelf, label);

		setDefaultButton(IDD_FINDINFILES_FIND_BUTTON);
	};

	//////////////////

	void setDefaultButton(int nID)
	{
#if 0
		// Where is a problem when you:
		// 1. open the find dialog
		// 2. press the "close" buttom
		// 3. open it again
		// 4. search for a non existing text
		// 5. when the "Can't find the text:" messagebox appears, hit "OK"
		// 6. now, the "Close" button looks like the default button. (but it only looks like that)
		//    if you hit "Enter" the "Find" button will be "pressed".
		// I thought this code might fix this but it doesn't
		// See: http://msdn.microsoft.com/en-us/library/ms645413(VS.85).aspx

		HWND pButton;
		DWORD dwDefInfo = SendMessage(_hSelf, DM_GETDEFID, 0, 0L);
		if (HIWORD(dwDefInfo) == DC_HASDEFID && (int)LOWORD(dwDefInfo) != nID)
		{
			// Reset 'DefButton' style
			pButton = GetDlgItem(_hSelf, (int)LOWORD(dwDefInfo));
			if (pButton)
				SendMessage(pButton, BM_SETSTYLE, LOWORD(BS_PUSHBUTTON | BS_RIGHT ), MAKELPARAM(TRUE, 0));
		}

		SendMessage(_hSelf, DM_SETDEFID, (WPARAM)nID, 0L);
		pButton = GetDlgItem(_hSelf, nID);
		if (pButton)
		{
			SendMessage(pButton, BM_SETSTYLE, LOWORD(BS_DEFPUSHBUTTON), MAKELPARAM(TRUE, 0));
		}
#endif
		SendMessage(_hSelf, DM_SETDEFID, (WPARAM)nID, 0L);
	}
	////////////////////////

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
		bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
		HWND hCombo = ::GetDlgItem(_hSelf, comboID);
		addText2Combo(getTextFromCombo(hCombo, isUnicode).c_str(), hCombo, isUnicode);
	};
	void fillFindHistory();
	void fillComboHistory(int id, int count, generic_string **pStrings);
	void saveComboHistory(int id, int maxcount, int& oldcount, generic_string **pStrings);
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

	void setSearchText(const TCHAR * txt2find, bool isUTF8 = false) {
		_doSearchFromBegin = false;
#ifdef UNICODE
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
#else
		if (!isUTF8)
		{
			::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)txt2find);
			return;
		}
		const int wideBufferSize = 256;
		WCHAR wchars[wideBufferSize];
		::MultiByteToWideChar(CP_UTF8, 0, txt2find, -1, wchars, wideBufferSize);
		winVer winVersion = NppParameters::getInstance()->getWinVersion();
		if (winVersion <= WV_ME) {
			//Cannot simply take txt2find since its UTF8
			char ansiBuffer[wideBufferSize];	//Assuming no more than 2 bytes for each wchar (SBCS or DBCS, no UTF8 and sorts)
			::WideCharToMultiByte(CP_ACP, 0, wchars, -1, ansiBuffer, wideBufferSize, NULL, NULL);
			::SendDlgItemMessageA(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)ansiBuffer);
		} else {
			::SendDlgItemMessageW(_hSelf, IDC_INCFINDTEXT, WM_SETTEXT, 0, (LPARAM)wchars);
		}
#endif
	}

	void addToRebar(ReBar * rebar);
private :
	bool _isRTL;
	FindReplaceDlg *_pFRDlg;

	ReBar * _pRebar;
	REBARBANDINFO _rbBand;

	bool _doSearchFromBegin;
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void markSelectedTextInc(bool enable, FindOption *opt = NULL);
};

#endif //FIND_REPLACE_DLG_H
