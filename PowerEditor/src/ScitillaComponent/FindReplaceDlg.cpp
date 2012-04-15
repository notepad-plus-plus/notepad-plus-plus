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


#include "precompiledHeaders.h"
#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"
#include "UniConversion.h"

FindOption * FindReplaceDlg::_env;
FindOption FindReplaceDlg::_options;

int Searching::convertExtendedToString(const TCHAR * query, TCHAR * result, int length) {	//query may equal to result, since it always gets smaller
	int i = 0, j = 0;
	int charLeft = length;
	bool isGood = true;
	TCHAR current;
	while(i < length) {	//because the backslash escape quences always reduce the size of the generic_string, no overflow checks have to be made for target, assuming parameters are correct
		current = query[i];
		charLeft--;
		if (current == '\\' && charLeft) {	//possible escape sequence
			i++;
			charLeft--;
			current = query[i];
			switch(current) {
				case 'r':
					result[j] = '\r';
					break;
				case 'n':
					result[j] = '\n';
					break;
				case '0':
					result[j] = '\0';
					break;
				case 't':
					result[j] = '\t';
					break;
				case '\\':
					result[j] = '\\';
					break;
				case 'b':
				case 'd':
				case 'o':
				case 'x':
				case 'u': {
					int size = 0, base = 0;
					if (current == 'b') {			//11111111
						size = 8, base = 2;
					} else if (current == 'o') {	//377
						size = 3, base = 8;
					} else if (current == 'd') {	//255
						size = 3, base = 10;
					} else if (current == 'x') {	//0xFF
						size = 2, base = 16;
					} else if (current == 'u') {	//0xCDCD
						size = 4, base = 16;
					}
					if (charLeft >= size) {
						int res = 0;
						if (Searching::readBase(query+(i+1), &res, base, size)) {
							result[j] = (TCHAR)res;
							i+=size;
							break;
						}
					}
					//not enough chars to make parameter, use default method as fallback
					}
				default: {	//unknown sequence, treat as regular text
					result[j] = '\\';
					j++;
					result[j] = current;
					isGood = false;
					break;
				}
			}
		} else {
			result[j] = query[i];
		}
		i++;
		j++;
	}
	result[j] = 0;
	return j;
}

bool Searching::readBase(const TCHAR * str, int * value, int base, int size) {
	int i = 0, temp = 0;
	*value = 0;
	TCHAR max = '0' + (TCHAR)base - 1;
	TCHAR current;
	while(i < size) {
		current = str[i];
		if (current >= 'A') 
		{
			current &= 0xdf;
			current -= ('A' - '0' - 10);
		}
		else if (current > '9')
			return false;

		if (current >= '0' && current <= max) {
			temp *= base;
			temp += (current - '0');
		} else {
			return false;
		}
		i++;
	}
	*value = temp;
	return true;
}

void Searching::displaySectionCentered(int posStart, int posEnd, ScintillaEditView * pEditView, bool isDownwards) 
{
	// to make sure the found result is visible
	//When searching up, the beginning of the (possible multiline) result is important, when scrolling down the end
	int testPos = (isDownwards)?posEnd:posStart;
	pEditView->execute(SCI_SETCURRENTPOS, testPos);
	int currentlineNumberDoc = (int)pEditView->execute(SCI_LINEFROMPOSITION, testPos);
	int currentlineNumberVis = (int)pEditView->execute(SCI_VISIBLEFROMDOCLINE, currentlineNumberDoc);
	pEditView->execute(SCI_ENSUREVISIBLE, currentlineNumberDoc);	// make sure target line is unfolded

	int firstVisibleLineVis =	(int)pEditView->execute(SCI_GETFIRSTVISIBLELINE);
	int linesVisible =			(int)pEditView->execute(SCI_LINESONSCREEN) - 1;	//-1 for the scrollbar
	int lastVisibleLineVis =	(int)linesVisible + firstVisibleLineVis;
	
	//if out of view vertically, scroll line into (center of) view
	int linesToScroll = 0;
	if (currentlineNumberVis < firstVisibleLineVis)
	{
		linesToScroll = currentlineNumberVis - firstVisibleLineVis;
		//use center
		linesToScroll -= linesVisible/2;		
	}
	else if (currentlineNumberVis > lastVisibleLineVis)
	{
		linesToScroll = currentlineNumberVis - lastVisibleLineVis;
		//use center
		linesToScroll += linesVisible/2;
	}
	pEditView->scroll(0, linesToScroll);

	//Make sure the caret is visible, scroll horizontally (this will also fix wrapping problems)
	pEditView->execute(SCI_GOTOPOS, posStart);
	pEditView->execute(SCI_GOTOPOS, posEnd);

	pEditView->execute(SCI_SETANCHOR, posStart);	
}

LONG FindReplaceDlg::originalFinderProc = NULL;

#ifdef UNICODE
void FindReplaceDlg::addText2Combo(const TCHAR * txt2add, HWND hCombo, bool)
#else
void FindReplaceDlg::addText2Combo(const TCHAR * txt2add, HWND hCombo, bool isUTF8)
#endif
{	
	if (!hCombo) return;
	if (!lstrcmp(txt2add, TEXT(""))) return;
	
	int i = 0;

#ifdef UNICODE
	i = ::SendMessage(hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)txt2add);
	if (i != CB_ERR) // found
	{
		::SendMessage(hCombo, CB_DELETESTRING, i, 0);
	}

	i = ::SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)txt2add);

#else
	TCHAR text[FINDREPLACE_MAXLENGTH];
	bool isWin9x = _winVer <= WV_ME;
	wchar_t wchars2Add[FINDREPLACE_MAXLENGTH];
	wchar_t textW[FINDREPLACE_MAXLENGTH];
	int count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);

	if (isUTF8)
		::MultiByteToWideChar(CP_UTF8, 0, txt2add, -1, wchars2Add, FINDREPLACE_MAXLENGTH - 1);

	for ( ; i < count ; i++)
	{
		if (isUTF8)
		{
			if (!isWin9x)
				::SendMessageW(hCombo, CB_GETLBTEXT, i, (LPARAM)textW);

			else
			{
				::SendMessageA(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
				::MultiByteToWideChar(CP_ACP, 0, text, -1, textW, FINDREPLACE_MAXLENGTH - 1);
			}

			if (!wcscmp(wchars2Add, textW))
			{
				::SendMessage(hCombo, CB_DELETESTRING, i, 0);
				break;
			}
		}
		else
		{
			::SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)text);
			if (!strcmp(txt2add, text))
			{
				::SendMessage(hCombo, CB_DELETESTRING, i, 0);
				break;
			}
		}
	}

	if (!isUTF8)
		i = ::SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)txt2add);

	else
	{
		if (!isWin9x)
			i = ::SendMessageW(hCombo, CB_INSERTSTRING, 0, (LPARAM)wchars2Add);
		else
		{
			::WideCharToMultiByte(CP_ACP, 0, wchars2Add, -1, text, FINDREPLACE_MAXLENGTH - 1, NULL, NULL);
			i = ::SendMessageA(hCombo, CB_INSERTSTRING, 0, (LPARAM)text);
		}
	}
#endif
	::SendMessage(hCombo, CB_SETCURSEL, i, 0);
}
#ifdef UNICODE
generic_string FindReplaceDlg::getTextFromCombo(HWND hCombo, bool) const
#else
generic_string FindReplaceDlg::getTextFromCombo(HWND hCombo, bool isUnicode) const
#endif
{	
	TCHAR str[FINDREPLACE_MAXLENGTH];
#ifdef UNICODE
	::SendMessage(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)str);
#else
	bool isWin9x = _winVer <= WV_ME;
	if (isUnicode)
	{
		wchar_t wchars[FINDREPLACE_MAXLENGTH];
		if ( !isWin9x )
		{
			::SendMessageW(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)wchars);
		}
		else
		{
			char achars[FINDREPLACE_MAXLENGTH];
			::SendMessageA(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)achars);
			::MultiByteToWideChar(CP_ACP, 0, achars, -1, wchars, FINDREPLACE_MAXLENGTH - 1);
		}
		::WideCharToMultiByte(CP_UTF8, 0, wchars, -1, str, FINDREPLACE_MAXLENGTH - 1, NULL, NULL);
	}
	else
	{
		::SendMessage(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, (LPARAM)str);
	}

#endif
	return generic_string(str);
}


// important : to activate all styles
const int STYLING_MASK = 255;

FindReplaceDlg::~FindReplaceDlg()
{
	_tab.destroy();
	if (_pFinder)
		delete _pFinder;
	delete [] _uniFileName;
}

void FindReplaceDlg::create(int dialogID, bool isRTL) 
{
	StaticDialog::create(dialogID, isRTL);
	fillFindHistory();
	_currentStatus = REPLACE_DLG;
	initOptionsFromDlg();

	RECT rect;
	//::GetWindowRect(_hSelf, &rect);
	getClientRect(rect);
	_tab.init(_hInst, _hSelf, false, false, true);
	_tab.setFont(TEXT("Tahoma"), 13);
	
	const TCHAR *find = TEXT("Find");
	const TCHAR *replace = TEXT("Replace");
	const TCHAR *findInFiles = TEXT("Find in Files");
	const TCHAR *mark = TEXT("Mark");

	_tab.insertAtEnd(find);
	_tab.insertAtEnd(replace);
	_tab.insertAtEnd(findInFiles);
	_tab.insertAtEnd(mark);

	_tab.reSizeTo(rect);
	_tab.display();

	ETDTProc enableDlgTheme = (ETDTProc)::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
	if (enableDlgTheme)
		enableDlgTheme(_hSelf, ETDT_ENABLETAB);

	goToCenter();
}

void FindReplaceDlg::fillFindHistory()
{
	NppParameters *nppParams = NppParameters::getInstance();
	FindHistory & findHistory = nppParams->getFindHistory();

	fillComboHistory(IDFINDWHAT, findHistory._findHistoryFinds);
	fillComboHistory(IDREPLACEWITH, findHistory._findHistoryReplaces);
	fillComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._findHistoryFilters);
    fillComboHistory(IDD_FINDINFILES_DIR_COMBO, findHistory._findHistoryPaths);

	::SendDlgItemMessage(_hSelf, IDWRAP, BM_SETCHECK, findHistory._isWrap, 0);
	::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, findHistory._isMatchWord, 0);
	::SendDlgItemMessage(_hSelf, IDMATCHCASE, BM_SETCHECK, findHistory._isMatchCase, 0);

	::SendDlgItemMessage(_hSelf, IDDIRECTIONUP, BM_SETCHECK, !findHistory._isDirectionDown, 0);
	::SendDlgItemMessage(_hSelf, IDDIRECTIONDOWN, BM_SETCHECK, findHistory._isDirectionDown, 0);

	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK, BM_SETCHECK, findHistory._isFifInHiddenFolder, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK, BM_SETCHECK, findHistory._isFifRecuisive, 0);
    ::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK, BM_SETCHECK, findHistory._isFolderFollowDoc, 0);

	::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, findHistory._searchMode == FindHistory::normal, 0);
	::SendDlgItemMessage(_hSelf, IDEXTENDED, BM_SETCHECK, findHistory._searchMode == FindHistory::extended, 0);
	::SendDlgItemMessage(_hSelf, IDREGEXP, BM_SETCHECK, findHistory._searchMode == FindHistory::regExpr, 0);
	::SendDlgItemMessage(_hSelf, IDREDOTMATCHNL, BM_SETCHECK, findHistory._dotMatchesNewline, 0);
	if (findHistory._searchMode == FindHistory::regExpr)
	{
		//regex doesnt allow wholeword
		::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, BST_UNCHECKED, 0);
		::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD), (BOOL)false);
	}
	
	if (nppParams->isTransparentAvailable())
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_CHECK), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), SW_SHOW);
		
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, findHistory._transparency);
		
		if (findHistory._transparencyMode == FindHistory::none)
		{
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), FALSE);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), FALSE);
		}
		else
		{
			::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_CHECK, BM_SETCHECK, TRUE, 0);
			
			int id;
			if (findHistory._transparencyMode == FindHistory::onLossingFocus)
			{
				id = IDC_TRANSPARENT_LOSSFOCUS_RADIO;
			}
			else
			{
				id = IDC_TRANSPARENT_ALWAYS_RADIO;
				(NppParameters::getInstance())->SetTransparent(_hSelf, findHistory._transparency);

			}
			::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);
		}
	}
}

void FindReplaceDlg::fillComboHistory(int id, const vector<generic_string> & strings)
{
	bool isUnicode = false;
	HWND hCombo = ::GetDlgItem(_hSelf, id);

	for (vector<generic_string>::const_reverse_iterator i = strings.rbegin() ; i != strings.rend(); i++)
	{
		addText2Combo(i->c_str(), hCombo, isUnicode);
	}
	::SendMessage(hCombo, CB_SETCURSEL, 0, 0); // select first item
}


void FindReplaceDlg::saveFindHistory()
{
	if (! isCreated()) return;
	FindHistory& findHistory = (NppParameters::getInstance())->getFindHistory();

	saveComboHistory(IDD_FINDINFILES_DIR_COMBO, findHistory._nbMaxFindHistoryPath, findHistory._findHistoryPaths);
	saveComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._nbMaxFindHistoryFilter, findHistory._findHistoryFilters);
	saveComboHistory(IDFINDWHAT,                    findHistory._nbMaxFindHistoryFind, findHistory._findHistoryFinds);
	saveComboHistory(IDREPLACEWITH,                 findHistory._nbMaxFindHistoryReplace, findHistory._findHistoryReplaces);
}

int FindReplaceDlg::saveComboHistory(int id, int maxcount, vector<generic_string> & strings)
{
	TCHAR text[FINDREPLACE_MAXLENGTH];
	HWND hCombo = ::GetDlgItem(_hSelf, id);
	int count = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	count = min(count, maxcount);

    if (count == CB_ERR) return 0;

    if (count)
        strings.clear();

    for (size_t i = 0 ; i < (size_t)count ; i++)
	{
		::SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM) text);
        strings.push_back(generic_string(text));
	}
    return count;
}

void FindReplaceDlg::updateCombos()
{
	updateCombo(IDREPLACEWITH);
	updateCombo(IDFINDWHAT);
}

FoundInfo Finder::EmptyFoundInfo(0, 0, TEXT(""));
SearchResultMarking Finder::EmptySearchResultMarking;

bool Finder::notify(SCNotification *notification)
{
	static bool isDoubleClicked = false;

	switch (notification->nmhdr.code)
	{
		case SCN_MARGINCLICK:
			if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
			{
				_scintView.marginClick(notification->position, notification->modifiers);
			}
			break;

		case SCN_DOUBLECLICK:
		{
			// remove selection from the finder
			isDoubleClicked = true;
			int pos = notification->position;
			if (pos == INVALID_POSITION)
				pos = _scintView.execute(SCI_GETLINEENDPOSITION, notification->line);
			_scintView.execute(SCI_SETSEL, pos, pos);

			GotoFoundLine();
		}
		break;

		case SCN_PAINTED :
			if (isDoubleClicked)
			{
				(*_ppEditView)->getFocus();
				isDoubleClicked = false;
			}
			break;
	}
	return false;
}


void Finder::GotoFoundLine()
{
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	int end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);
	if (start + 2 >= end) return; // avoid empty lines

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		_scintView.execute(SCI_TOGGLEFOLD, lno);
		return;
	}

	const FoundInfo fInfo = *(_pMainFoundInfos->begin() + lno);

	// Switch to another document
	::SendMessage(::GetParent(_hParent), WM_DOOPEN, 0, (LPARAM)fInfo._fullPath.c_str());
	Searching::displaySectionCentered(fInfo._start, fInfo._end, *_ppEditView);

	// Then we colourise the double clicked line
	setFinderStyle();
	//_scintView.execute(SCI_SETLEXER, SCLEX_NULL);   // yniq - this line causes a bug!!! (last line suddenly belongs to file header level (?) instead of having level=0x400)
													// later it affects DeleteResult and gotoNextFoundResult (assertions)
													// fixed by calling setFinderStyle() in DeleteResult()
	_scintView.execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_HIGHLIGHT_LINE, true);
	_scintView.execute(SCI_STARTSTYLING, start, STYLING_MASK);
	_scintView.execute(SCI_SETSTYLING, end - start + 2, SCE_SEARCHRESULT_HIGHLIGHT_LINE);
	_scintView.execute(SCI_COLOURISE, start, end + 1);
}

void Finder::DeleteResult()
{
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS); // yniq - add handling deletion of multiple lines?

	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	int end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);
	if (start + 2 >= end) return; // avoid empty lines

	_scintView.setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, 0); // Restore searchResult lexer in case the lexer was changed to SCLEX_NULL in GotoFoundLine()

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)  // delete a folder
	{
		int endline = _scintView.execute(SCI_GETLASTCHILD, lno, -1) + 1;
		assert((size_t) endline <= _pMainFoundInfos->size());

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno, _pMainFoundInfos->begin() + endline); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno, _pMainMarkings->begin() + endline);

		int end = _scintView.execute(SCI_POSITIONFROMLINE, endline);
		_scintView.execute(SCI_SETSEL, start, end);
		setFinderReadOnly(false);
		_scintView.execute(SCI_CLEAR);
		setFinderReadOnly(true);
	}
	else // delete one line
	{
		assert((size_t) lno < _pMainFoundInfos->size());

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno);

		setFinderReadOnly(false);
		_scintView.execute(SCI_LINEDELETE);
		setFinderReadOnly(true);
	}
	_MarkingsStruct._length = _pMainMarkings->size();

	assert(_pMainFoundInfos->size() == _pMainMarkings->size());
	assert(size_t(_scintView.execute(SCI_GETLINECOUNT)) == _pMainFoundInfos->size() + 1);
}

void Finder::gotoNextFoundResult(int direction)
{
	int increment = direction < 0 ? -1 : 1;
	int currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	int lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	int total_lines = _scintView.execute(SCI_GETLINECOUNT);
	if (total_lines <= 1) return;
	
	if (lno == total_lines - 1) lno--; // last line doesn't belong to any search, use last search

	int init_lno = lno;
	int max_lno = _scintView.execute(SCI_GETLASTCHILD, lno, searchHeaderLevel);

	assert(max_lno <= total_lines - 2);

	// get the line number of the current search (searchHeaderLevel)
	int level = _scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELNUMBERMASK;
	int min_lno = lno;
	while (level-- >= fileHeaderLevel)
	{
		min_lno = _scintView.execute(SCI_GETFOLDPARENT, min_lno);
		assert(min_lno >= 0);
	}

	if (min_lno < 0) min_lno = lno; // when lno is a search header line

	assert(min_lno <= max_lno);

	lno += increment;
	
	if      (lno > max_lno) lno = min_lno;
	else if (lno < min_lno) lno = max_lno;

	while (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		lno += increment;
		if      (lno > max_lno) lno = min_lno;
		else if (lno < min_lno) lno = max_lno;
		if (lno == init_lno) break;
	}

	if ((_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG) == 0)
	{
		int start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
		_scintView.execute(SCI_SETSEL, start, start);
		_scintView.execute(SCI_ENSUREVISIBLE, lno);
		_scintView.execute(SCI_SCROLLCARET);

		GotoFoundLine();
	}
}

BOOL CALLBACK FindReplaceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			RECT arc;
			::GetWindowRect(::GetDlgItem(_hSelf, IDCANCEL), &arc);
			_findInFilesClosePos.bottom = _replaceClosePos.bottom = _findClosePos.bottom = arc.bottom - arc.top;
			_findInFilesClosePos.right = _replaceClosePos.right = _findClosePos.right = arc.right - arc.left;

			POINT p;
			p.x = arc.left;
			p.y = arc.top;
			::ScreenToClient(_hSelf, &p);

			_replaceClosePos.left = p.x;
			_replaceClosePos.top = p.y;

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDREPLACEALL));
			 _findInFilesClosePos.left = p.x;
			 _findInFilesClosePos.top = p.y;

			 p = getLeftTopPoint(::GetDlgItem(_hSelf, IDCANCEL));
			 _findClosePos.left = p.x;
			 _findClosePos.top = p.y + 10;

			return TRUE;
		}
		
		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER))
			{
				int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
				FindHistory & findHistory = (NppParameters::getInstance())->getFindHistory();
				findHistory._transparency = percent;
				if (isCheckedOrNot(IDC_TRANSPARENT_ALWAYS_RADIO))
				{
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
				}
			}
			return TRUE;
		}

		case WM_NOTIFY:
		{
			NMHDR *nmhdr = (NMHDR *)lParam;
			if (nmhdr->code == TCN_SELCHANGE)
			{
				HWND tabHandle = _tab.getHSelf();
				if (nmhdr->hwndFrom == tabHandle)
				{
					int indexClicked = int(::SendMessage(tabHandle, TCM_GETCURSEL, 0, 0));
					doDialog((DIALOG_TYPE)indexClicked);
				}
				return TRUE;
			}
			break;
		}

		case WM_ACTIVATE :
		{
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				CharacterRange cr = (*_ppEditView)->getSelection();
				int nbSelected = cr.cpMax - cr.cpMin;

				_options._isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK)?1:0;
				int checkVal = _options._isInSelection?BST_CHECKED:BST_UNCHECKED;
				
				if (!_options._isInSelection)
				{
					if (nbSelected <= 1024)
					{
						checkVal = BST_UNCHECKED;
						_options._isInSelection = false;
					}
					else
					{
						checkVal = BST_CHECKED;
						_options._isInSelection = true;
					}
				}
				// Searching/replacing in column selection is not allowed 
				if ((*_ppEditView)->execute(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE)
				{
					checkVal = BST_UNCHECKED;
					_options._isInSelection = false;
					nbSelected = 0;
				}
				::EnableWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), nbSelected);
                // uncheck if the control is disable
                if (!nbSelected)
                {
					checkVal = BST_UNCHECKED;
					_options._isInSelection = false;
                }
				::SendDlgItemMessage(_hSelf, IDC_IN_SELECTION_CHECK, BM_SETCHECK, checkVal, 0);
			}
			
			if (isCheckedOrNot(IDC_TRANSPARENT_LOSSFOCUS_RADIO))
			{
				if (LOWORD(wParam) == WA_INACTIVE && isVisible())
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
				}
				else
				{
					(NppParameters::getInstance())->removeTransparent(_hSelf);
				}
			}
			return TRUE;
		}

		case NPPM_MODELESSDIALOG :
			return ::SendMessage(_hParent, NPPM_MODELESSDIALOG, wParam, lParam);

		case WM_COMMAND : 
		{
			bool isMacroRecording = (::SendMessage(_hParent, WM_GETCURRENTMACROSTATUS,0,0) == MACRO_RECORDING_IN_PROGRESS);
			NppParameters *nppParamInst = NppParameters::getInstance();
			FindHistory & findHistory = nppParamInst->getFindHistory();
			switch (wParam)
			{
//Single actions
				case IDCANCEL:
					display(false);
					break;
				case IDOK : // Find Next : only for FIND_DLG and REPLACE_DLG
				{
					bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;

					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND);
					processFindNext(_options._str2Search.c_str());
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDREPLACE :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						_options._str4Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE);
						processReplace(_options._str2Search.c_str(), _options._str4Replace.c_str());
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;
//Process actions
				case IDC_FINDALL_OPENEDFILES :
				{
					if (_currentStatus == FIND_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
 						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
                        combo2ExtendedMode(IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						updateCombo(IDFINDWHAT);

						nppParamInst->_isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_GLOBAL);
						findAllIn(ALL_OPEN_DOCS);
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_FINDALL_CURRENTFILE :
				{
					bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
                    combo2ExtendedMode(IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_GLOBAL);
					findAllIn(CURRENT_DOC);
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDD_FINDINFILES_FIND_BUTTON :
				{
					const int filterSize = 256;
					TCHAR filters[filterSize+1];
					filters[filterSize] = '\0';
					TCHAR directory[MAX_PATH];
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_options._filters = filters;

					::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
					addText2Combo(directory, ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
					_options._directory = directory;
					
					if ((lstrlen(directory) > 0) && (directory[lstrlen(directory)-1] != '\\'))
						_options._directory += TEXT("\\");

 					bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
                   combo2ExtendedMode(IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
					updateCombo(IDFINDWHAT);

					nppParamInst->_isFindReplacing = true;
					if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_FIF);
					findAllIn(FILES_IN_DIR);
					nppParamInst->_isFindReplacing = false;
				}
				return TRUE;

				case IDD_FINDINFILES_REPLACEINFILES :
				{
					const int filterSize = 256;
					TCHAR filters[filterSize];
					TCHAR directory[MAX_PATH];
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_options._filters = filters;

					::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
					addText2Combo(directory, ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
					_options._directory = directory;
					
					if ((lstrlen(directory) > 0) && (directory[lstrlen(directory)-1] != '\\'))
						_options._directory += TEXT("\\");

					generic_string msg = TEXT("Are you sure you want to replace all occurrences in :\r");
					msg += _options._directory;
					msg += TEXT("\rfor file type : ");
					msg += _options._filters[0]?_options._filters:TEXT("*.*");
					
					if (::MessageBox(_hParent, msg.c_str(), TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str4Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombo(IDFINDWHAT);
						updateCombo(IDREPLACEWITH);

						nppParamInst->_isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE + FR_OP_FIF);
						::SendMessage(_hParent, WM_REPLACEINFILES, 0, 0);
						nppParamInst->_isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_REPLACE_OPENEDFILES :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str4Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE + FR_OP_GLOBAL);
						replaceAllInOpenedDocs();
						nppParamInst->_isFindReplacing = false;
					}
				}			
				return TRUE;

				case IDREPLACEALL :
				{
					if (_currentStatus == REPLACE_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str4Replace = getTextFromCombo(hReplaceCombo, isUnicode);
						updateCombos();

						nppParamInst->_isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE);
						(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
						int nbReplaced = processAll(ProcessReplaceAll, &_options);
						(*_ppEditView)->execute(SCI_ENDUNDOACTION);
						nppParamInst->_isFindReplacing = false;

						generic_string result = TEXT("");
						
						if (nbReplaced < 0)
							result = TEXT("The regular expression to search is formed badly");
						else
						{
							TCHAR moreInfo[64];
							wsprintf(moreInfo, TEXT("%d occurrences were replaced."), nbReplaced);
							result = moreInfo;
						}
						::MessageBox(_hParent, result.c_str(), TEXT("Replace All"), MB_OK);
						::SetFocus(_hSelf);
					}
				}
				return TRUE;

				case IDCCOUNTALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						updateCombo(IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);

						int nbCounted = processAll(ProcessCountAll, &_options);
						generic_string result = TEXT("");

						if (nbCounted < 0)
							result = TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
						else
						{
							TCHAR moreInfo[128];
							wsprintf(moreInfo, TEXT("%d match(es) to occurrence(s)"), nbCounted);
							result = moreInfo;
						}
						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND);
						::MessageBox(_hParent, result.c_str(), TEXT("Count"), MB_OK);
						::SetFocus(_hSelf);
					}
				}
				return TRUE;

				case IDCMARKALL :
				{
					if (_currentStatus == MARK_DLG)
					{
						bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo, isUnicode);
						updateCombo(IDFINDWHAT);

						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND);
						nppParamInst->_isFindReplacing = true;
						int nbMarked = processAll(ProcessMarkAll, &_options);
						nppParamInst->_isFindReplacing = false;
						generic_string result = TEXT("");
						if (nbMarked < 0)
							result = TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
						else
						{
							TCHAR moreInfo[128];
							wsprintf(moreInfo, TEXT("%d match(es) to occurrence(s)"), nbMarked);
							result = moreInfo;
						}
						::MessageBox(_hParent, result.c_str(), TEXT("Mark"), MB_OK);
						::SetFocus(_hSelf);
					}
				}
				return TRUE;

				case IDC_CLEAR_ALL :
				{
					if (_currentStatus == MARK_DLG)
					{
						(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
						(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
					}
				}
				return TRUE;
//Option actions
				case IDREDOTMATCHNL:
					findHistory._dotMatchesNewline = _options._dotMatchesNewline = isCheckedOrNot(IDREDOTMATCHNL);
					return TRUE;

				case IDWHOLEWORD :
					findHistory._isMatchWord = _options._isWholeWord = isCheckedOrNot(IDWHOLEWORD);
					return TRUE;

				case IDMATCHCASE :
					findHistory._isMatchCase = _options._isMatchCase = isCheckedOrNot(IDMATCHCASE);
					return TRUE;

				case IDNORMAL:
				case IDEXTENDED:
				case IDREGEXP : {
					if (isCheckedOrNot(IDREGEXP))
					{
						_options._searchType = FindRegex;
						findHistory._searchMode = FindHistory::regExpr;
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL), true);
					}
					else if (isCheckedOrNot(IDEXTENDED))
					{
						_options._searchType = FindExtended;
						findHistory._searchMode = FindHistory::extended;
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL), false);
					}
					else
					{
						_options._searchType = FindNormal;
						findHistory._searchMode = FindHistory::normal;
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL), false);
					}

					bool isRegex = (_options._searchType == FindRegex);
					if (isRegex) 
					{	
						//regex doesnt allow whole word
						_options._isWholeWord = false;
						::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, _options._isWholeWord?BST_CHECKED:BST_UNCHECKED, 0);

					}

					::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD), (BOOL)!isRegex);
					
					return TRUE; }

				case IDWRAP :
					findHistory._isWrap = _options._isWrapAround = isCheckedOrNot(IDWRAP);
					return TRUE;

				case IDDIRECTIONUP :
				case IDDIRECTIONDOWN :
					_options._whichDirection = (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), BM_GETCHECK, BST_CHECKED, 0));
					findHistory._isDirectionDown = _options._whichDirection == DIR_DOWN;
					return TRUE;

				case IDC_PURGE_CHECK :
				{
					if (_currentStatus == MARK_DLG)
						_options._doPurge = isCheckedOrNot(IDC_PURGE_CHECK);
				}
				return TRUE;

				case IDC_MARKLINE_CHECK :
				{
					if (_currentStatus == MARK_DLG)
						_options._doMarkLine = isCheckedOrNot(IDC_MARKLINE_CHECK);
				}
				return TRUE;

				case IDC_IN_SELECTION_CHECK :
				{
					if ((_currentStatus == REPLACE_DLG) || (_currentStatus == MARK_DLG))
						_options._isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK);
				}
				return TRUE;

				case IDC_TRANSPARENT_CHECK :
				{
					bool isChecked = isCheckedOrNot(IDC_TRANSPARENT_CHECK);

					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO), isChecked);
					::EnableWindow(::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER), isChecked);

					if (isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_CHECKED, 0);
						findHistory._transparencyMode = FindHistory::onLossingFocus;
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						(NppParameters::getInstance())->removeTransparent(_hSelf);
						findHistory._transparencyMode = FindHistory::none;
					}

					return TRUE;
				}

				case IDC_TRANSPARENT_ALWAYS_RADIO :
				{
					int percent = ::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
					(NppParameters::getInstance())->SetTransparent(_hSelf, percent);
					findHistory._transparencyMode = FindHistory::persistant;
				}
				return TRUE;

				case IDC_TRANSPARENT_LOSSFOCUS_RADIO :
				{
					(NppParameters::getInstance())->removeTransparent(_hSelf);
					findHistory._transparencyMode = FindHistory::onLossingFocus;
				}
				return TRUE;

				//
				// Find in Files
				//
				case IDD_FINDINFILES_RECURSIVE_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						findHistory._isFifRecuisive = _options._isRecursive = isCheckedOrNot(IDD_FINDINFILES_RECURSIVE_CHECK);
					
				}
				return TRUE;

				case IDD_FINDINFILES_INHIDDENDIR_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
						findHistory._isFifInHiddenFolder = _options._isInHiddenDir = isCheckedOrNot(IDD_FINDINFILES_INHIDDENDIR_CHECK);
					
				}
				return TRUE;

                case IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK :
				{
					if (_currentStatus == FINDINFILES_DLG)
                        findHistory._isFolderFollowDoc = isCheckedOrNot(IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK);

                    if (findHistory._isFolderFollowDoc)
                    {
                        NppParameters *pNppParam = NppParameters::getInstance();
                        const TCHAR * dir = pNppParam->getWorkingDir();
                        ::SetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, dir);
                    }
					
				}
				return TRUE;

				case IDD_FINDINFILES_BROWSE_BUTTON :
				{
					if (_currentStatus == FINDINFILES_DLG)
						folderBrowser(_hSelf, IDD_FINDINFILES_DIR_COMBO, _options._directory.c_str());	
				}
				return TRUE;

				default :
					break;
			}
			break;
		}
	}
	return FALSE;
}

// return value :
// true  : the text2find is found
// false : the text2find is not found

bool FindReplaceDlg::processFindNext(const TCHAR *txt2find, const FindOption *options, FindStatus *oFindStatus)
{
	if (oFindStatus)
		*oFindStatus = FSFound;

	if (!txt2find || !txt2find[0])
		return false;

	const FindOption *pOptions = options?options:_env;

	int stringSizeFind = lstrlen(txt2find);
	TCHAR *pText = new TCHAR[stringSizeFind + 1];
	lstrcpy(pText, txt2find);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pText, stringSizeFind);
	}

	int docLength = int((*_ppEditView)->execute(SCI_GETLENGTH));
	CharacterRange cr = (*_ppEditView)->getSelection();

	//The search "zone" is relative to the selection, so search happens 'outside'
	int startPosition = cr.cpMax;
	int endPosition = docLength;

	if (pOptions->_whichDirection == DIR_UP)
	{
		//When searching upwards, start is the lower part, end the upper, for backwards search
		startPosition = cr.cpMin - 1;
		endPosition = 0;
	}

	if (FirstIncremental==pOptions->_incrementalType)
	{
		// the text to find is modified so use the current position
		startPosition = cr.cpMin;
		endPosition = docLength;
	}
	else if (NextIncremental==pOptions->_incrementalType)
	{
		// text to find is not modified, so use current position +1
		startPosition = cr.cpMin +1;
		endPosition = docLength;	
		if (pOptions->_whichDirection == DIR_UP)
		{
			//When searching upwards, start is the lower part, end the upper, for backwards search
			startPosition = cr.cpMax - 1;
			endPosition = 0;
		}
	}

	int flags = Searching::buildSearchFlags(pOptions);

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);

	int posFind = (*_ppEditView)->searchInTarget(pText, stringSizeFind, startPosition, endPosition);
	if (posFind == -1) //no match found in target, check if a new target should be used
	{
		if (pOptions->_isWrapAround) 
		{
			//when wrapping, use the rest of the document (entire document is usable)
			if (pOptions->_whichDirection == DIR_DOWN)
			{
				startPosition = 0;
				endPosition = docLength;
				if (oFindStatus)
					*oFindStatus = FSEndReached;
			}
			else
			{
				startPosition = docLength;
				endPosition = 0;
				if (oFindStatus)
					*oFindStatus = FSTopReached;
			}

			//new target, search again
			posFind = (*_ppEditView)->searchInTarget(pText, stringSizeFind, startPosition, endPosition);
		}
		if (posFind == -1)
		{
			if (oFindStatus)
				*oFindStatus = FSNotFound;
			//failed, or failed twice with wrap
			if (NotIncremental==pOptions->_incrementalType) //incremental search doesnt trigger messages
			{	
				generic_string msg = TEXT("Can't find the text:\r\n\"");
				msg += txt2find;
				msg += TEXT("\"");
				::MessageBox(_hParent, msg.c_str(), TEXT("Find"), MB_OK);
				// if the dialog is not shown, pass the focus to his parent(ie. Notepad++)
				if (!::IsWindowVisible(_hSelf))
				{
					::SetFocus((*_ppEditView)->getHSelf());
				}
				else
				{
					::SetFocus(::GetDlgItem(_hSelf, IDFINDWHAT));
				}
			}
			delete [] pText;
			return false;
		}
	}
	else if (posFind == -2) // Invalid Regular expression
	{
		::MessageBox(_hParent, TEXT("Invalid regular expression"), TEXT("Find"), MB_ICONERROR | MB_OK);
		return false;
	}
	int start =	posFind;
	int end = int((*_ppEditView)->execute(SCI_GETTARGETEND));

	// to make sure the found result is visible:
	// prevent recording of absolute positioning commands issued in the process
	(*_ppEditView)->execute(SCI_STOPRECORD);
	Searching::displaySectionCentered(start, end, *_ppEditView, pOptions->_whichDirection == DIR_DOWN);
	if (::SendMessage(_hParent, WM_GETCURRENTMACROSTATUS,0,0) == MACRO_RECORDING_IN_PROGRESS)
		(*_ppEditView)->execute(SCI_STARTRECORD);

	delete [] pText;
	return true;
}

// return value :
// true  : the text is replaced, and find the next occurrence
// false : the text2find is not found, so the text is NOT replace
//      || the text is replaced, and do NOT find the next occurrence
bool FindReplaceDlg::processReplace(const TCHAR *txt2find, const TCHAR *txt2replace, const FindOption *options)
{
	if (!txt2find || !txt2find[0] || !txt2replace)
		return false;

	const FindOption *pOptions = options?options:_env;

	if ((*_ppEditView)->getCurrentBuffer()->isReadOnly()) return false;

	int stringSizeFind = lstrlen(txt2find);
	int stringSizeReplace = lstrlen(txt2replace);
	TCHAR *pTextFind = new TCHAR[stringSizeFind + 1];
	TCHAR *pTextReplace = new TCHAR[stringSizeReplace + 1];
	lstrcpy(pTextFind, txt2find);
	lstrcpy(pTextReplace, txt2replace);

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(txt2find, pTextFind, stringSizeFind);
		stringSizeReplace = Searching::convertExtendedToString(txt2replace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);
	CharacterRange cr = (*_ppEditView)->getSelection();
	
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int posFind = (*_ppEditView)->searchInTarget(pTextFind, stringSizeFind, cr.cpMin, cr.cpMax);
	if (posFind != -1)
	{
		if (isRegExp)
		{
			//For the rare re exp case. ex: replace ^ by AAA
			int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));

			int replacedLen = (*_ppEditView)->replaceTargetRegExMode(pTextReplace);
			(*_ppEditView)->execute(SCI_SETSEL, start, start + replacedLen);
		}
		else
		{
			int start = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
			int replacedLen = (*_ppEditView)->replaceTarget(pTextReplace);
			(*_ppEditView)->execute(SCI_SETSEL, start, start + replacedLen);
		}
	}
	else if (posFind == -2) // Invalid Regular expression
	{
		::MessageBox(_hParent, TEXT("Invalid regular expression"), TEXT("Find"), MB_ICONERROR | MB_OK);
		return false;
	}

	delete [] pTextFind;
	delete [] pTextReplace;
	return processFindNext(txt2find, pOptions);	//after replacing, find the next section for selection
}


int FindReplaceDlg::markAll(const TCHAR *txt2find, int styleID)
{
	FindOption opt;
	opt._isMatchCase = true;
	opt._isWholeWord = false;
	opt._str2Search = txt2find;

	int nbFound = processAll(ProcessMarkAllExt, &opt, true, NULL, styleID);
	return nbFound;
}

/*
int FindReplaceDlg::markAll2(const TCHAR *txt2find)
{
	FindOption opt;
	opt._isMatchCase = false;
	opt._isWholeWord = true;
	int nbFound = processAll(ProcessMarkAll_2, txt2find, NULL, true, NULL, &opt);
	return nbFound;
}
*/


int FindReplaceDlg::markAllInc(const FindOption *opt)
{
	int nbFound = processAll(ProcessMarkAll_IncSearch, opt,  true);
	return nbFound;
}

int FindReplaceDlg::processAll(ProcessOperation op, const FindOption *opt, bool isEntire, const TCHAR *fileName, int colourStyleID)
{
	const FindOption *pOptions = opt?opt:_env;
	const TCHAR *txt2find = pOptions->_str2Search.c_str();
	const TCHAR *txt2replace = pOptions->_str4Replace.c_str();

	CharacterRange cr = (*_ppEditView)->getSelection();
	int docLength = int((*_ppEditView)->execute(SCI_GETLENGTH));

	// Default : 
	//        direction : down
	//        begin at : 0
	//        end at : end of doc
	int startPosition = 0;
	int endPosition = docLength;

	bool direction = pOptions->_whichDirection;

	//first try limiting scope by direction
	if (direction == DIR_UP)	
	{
		startPosition = 0;
		endPosition = cr.cpMax;
	}
	else
	{
		startPosition = cr.cpMin;
		endPosition = docLength;
	}

	//then adjust scope if the full document needs to be changed
	if (pOptions->_isWrapAround || isEntire || (op == ProcessCountAll))	//entire document needs to be scanned
	{		
		startPosition = 0;
		endPosition = docLength;
	}
	
	//then readjust scope if the selection override is active and allowed
	if ((pOptions->_isInSelection) && ((op == ProcessMarkAll) || ((op == ProcessReplaceAll) && (!isEntire))))	//if selection limiter and either mark all or replace all w/o entire document override
	{
		startPosition = cr.cpMin;
		endPosition = cr.cpMax;
	}

	if ((op == ProcessMarkAllExt) && (colourStyleID != -1))
	{
		startPosition = 0;
		endPosition = docLength;
	}

	return processRange(op, txt2find, txt2replace, startPosition, endPosition, fileName, pOptions, colourStyleID);
}

int FindReplaceDlg::processRange(ProcessOperation op, const TCHAR *txt2find, const TCHAR *txt2replace, int startRange, int endRange, const TCHAR *fileName, const FindOption *opt, int colourStyleID)
{
	int nbProcessed = 0;
	
	if (!isCreated() && !txt2find)
		return nbProcessed;

	if ((op == ProcessReplaceAll) && (*_ppEditView)->getCurrentBuffer()->isReadOnly())
		return nbProcessed;

	if (startRange == endRange)
		return nbProcessed;

	if (!fileName)
		fileName = TEXT("");

	const FindOption *pOptions = opt?opt:_env;
	//bool isUnicode = (*_ppEditView)->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
	bool isUnicode = ((*_ppEditView)->execute(SCI_GETCODEPAGE) == SC_CP_UTF8);

	int stringSizeFind = 0;
	int stringSizeReplace = 0;

	TCHAR *pTextFind = NULL;
	if (!txt2find)
	{
		HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
		generic_string str2Search = getTextFromCombo(hFindCombo, isUnicode);
		stringSizeFind = str2Search.length();
		pTextFind = new TCHAR[stringSizeFind + 1];
		lstrcpy(pTextFind, str2Search.c_str());
	}
	else
	{
		stringSizeFind = lstrlen(txt2find);
		pTextFind = new TCHAR[stringSizeFind + 1];
		lstrcpy(pTextFind, txt2find);
	}

	if (!pTextFind[0]) 
	{
		delete [] pTextFind;
		return nbProcessed;
	}

	TCHAR *pTextReplace = NULL;
	if (op == ProcessReplaceAll)
	{
		if (!txt2replace)
		{
			HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
			generic_string str2Replace = getTextFromCombo(hReplaceCombo, isUnicode);
			stringSizeReplace = str2Replace.length();
			pTextReplace = new TCHAR[stringSizeReplace + 1];
			lstrcpy(pTextReplace, str2Replace.c_str());
		}
		else
		{
			stringSizeReplace = lstrlen(txt2replace);
			pTextReplace = new TCHAR[stringSizeReplace + 1];
			lstrcpy(pTextReplace, txt2replace);
		}
	}	

	if (pOptions->_searchType == FindExtended) {
		stringSizeFind = Searching::convertExtendedToString(pTextFind, pTextFind, stringSizeFind);
		if (op == ProcessReplaceAll)
			stringSizeReplace = Searching::convertExtendedToString(pTextReplace, pTextReplace, stringSizeReplace);
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions);



	if (op == ProcessMarkAll && colourStyleID == -1)	//if marking, check if purging is needed
	{
		if (_env->_doPurge) {
			(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
			if (_env->_doMarkLine)
				(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
		}
	}

	int targetStart = 0;
	int targetEnd = 0;

	//Initial range for searching
	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	targetStart = (*_ppEditView)->searchInTarget(pTextFind, stringSizeFind, startRange, endRange);
	
	if ((targetStart >= 0) && (op == ProcessFindAll))	//add new filetitle if this file results in hits
	{
		_pFinder->addFileNameTitle(fileName);
	}

	while (targetStart != -1 && targetStart != -2)
	{
		//int posFindBefore = posFind;
		targetStart = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
		targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));
		if (targetEnd > endRange) {	//we found a result but outside our range, therefore do not process it
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		int replaceDelta = 0;

		// Search resulted in empty token, possible with RE
		/*
		if (!foundTextLen) {
			delete [] pTextFind;
			delete [] pTextReplace;
			return -1;
		}
		*/
		
		switch (op)
		{
			case ProcessFindAll: 
			{
				int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, targetStart);
				int lend = (*_ppEditView)->execute(SCI_GETLINEENDPOSITION, lineNumber);
				int lstart = (*_ppEditView)->execute(SCI_POSITIONFROMLINE, lineNumber);
				int nbChar = lend - lstart;

				// use the static buffer
				TCHAR lineBuf[1024];

				if (nbChar > 1024 - 3)
					lend = lstart + 1020;

				int start_mark = targetStart - lstart;
				int end_mark = targetEnd - lstart;

				(*_ppEditView)->getGenericText(lineBuf, lstart, lend, &start_mark, &end_mark);
				generic_string line;
#ifdef UNICODE
				line = lineBuf;
#else
				UINT cp = (*_ppEditView)->execute(SCI_GETCODEPAGE);
				if (cp != SC_CP_UTF8)
				{
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *pTextW = wmc->char2wchar(lineBuf, ::GetACP());
					const char *pTextA = wmc->wchar2char(pTextW, SC_CP_UTF8);
					line = pTextA;
				}
				else
					line = lineBuf;
#endif
				line += TEXT("\r\n");
				SearchResultMarking srm;
				srm._start = start_mark;
				srm._end = end_mark;
				_pFinder->add(FoundInfo(targetStart, targetEnd,  fileName), srm, line.c_str(), lineNumber + 1);

				break; 
			}

			case ProcessReplaceAll: 
			{
				int replacedLength;
				if (isRegExp)
					replacedLength = (*_ppEditView)->replaceTargetRegExMode(pTextReplace);
				else
					replacedLength = (*_ppEditView)->replaceTarget(pTextReplace);

				replaceDelta = replacedLength - foundTextLen;
				break; 
			}

			case ProcessMarkAll: 
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_FOUND_STYLE);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);

				if (_env->_doMarkLine)
				{
					int lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, targetStart);
					int state = (*_ppEditView)->execute(SCI_MARKERGET, lineNumber);

					if (!(state & (1 << MARK_BOOKMARK)))
						(*_ppEditView)->execute(SCI_MARKERADD, lineNumber, MARK_BOOKMARK);
				}
				break; 
			}
			
			case ProcessMarkAllExt:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  colourStyleID);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessMarkAll_2:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_SMART);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessMarkAll_IncSearch:
			{
				(*_ppEditView)->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_INC);
				(*_ppEditView)->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				break;
			}

			case ProcessCountAll: 
			{
				//Nothing to do
				break;
			}

			default:
			{
				delete [] pTextFind;
				delete [] pTextReplace;
				return nbProcessed;
			}
			
		}	
		nbProcessed++;

        // After the processing of the last string occurence the search loop should be stopped
        // This helps to avoid the endless replacement during the EOL ("$") searching
        if( targetStart + foundTextLen == endRange )
            break;

		if (!foundTextLen && !replaceDelta)
			startRange = targetStart + foundTextLen + 1; // find a empty string so just step forword
		else
			startRange = targetStart + foundTextLen + replaceDelta;		//search from result onwards
		endRange += replaceDelta;									//adjust end of range in case of replace

		targetStart = (*_ppEditView)->searchInTarget(pTextFind, stringSizeFind, startRange, endRange);
	}
	delete [] pTextFind;
	delete [] pTextReplace;

	if (nbProcessed > 0 && op == ProcessFindAll) 
		_pFinder->addFileHitCount(nbProcessed);

	return nbProcessed;
}

void FindReplaceDlg::replaceAllInOpenedDocs()
{
	::SendMessage(_hParent, WM_REPLACEALL_INOPENEDDOC, 0, 0);
}

void FindReplaceDlg::findAllIn(InWhat op)
{
	bool doSetMarkingStruct = false;
	if (!_pFinder)
	{
		_pFinder = new Finder();
		_pFinder->init(_hInst, _hSelf, _ppEditView);
		
		tTbData	data = {0};
		_pFinder->create(&data, false);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_pFinder->getHSelf());
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO;
		data.hIconTab = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_FIND_RESULT_ICON), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		data.pszAddInfo = _findAllResultStr;

		data.pszModuleName = TEXT("dummy");

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		data.dlgID = 0;
		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);

		_pFinder->_scintView.init(_hInst, _pFinder->getHSelf());

		// Subclass the ScintillaEditView for the Finder (Scintilla doesn't notify all key presses)
		originalFinderProc = SetWindowLongPtr(_pFinder->_scintView.getHSelf(), GWLP_WNDPROC, (LONG_PTR)finderProc);

		_pFinder->setFinderReadOnly(true);
		_pFinder->_scintView.execute(SCI_SETCODEPAGE, SC_CP_UTF8);
		_pFinder->_scintView.execute(SCI_USEPOPUP, FALSE);
		_pFinder->_scintView.execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
		_pFinder->_scintView.execute(SCI_SETCARETLINEVISIBLE, 1);
		_pFinder->_scintView.execute(SCI_SETCARETWIDTH, 0);
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_FOLDER, true);

		// get the width of FindDlg
		RECT findRect;
		::GetWindowRect(_pFinder->getHSelf(), &findRect);

		// overwrite some default settings
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, false);
		_pFinder->_scintView.setMakerStyle(FOLDER_STYLE_SIMPLE);

		_pFinder->_scintView.display();
		_pFinder->display();
		doSetMarkingStruct = true;
	}
	_pFinder->setFinderStyle();

	if (doSetMarkingStruct)
	{
		// Send the address of _MarkingsStruct to the lexer
		char ptrword[sizeof(void*)*2+1];
		sprintf(ptrword, "%p", &_pFinder->_MarkingsStruct);
		_pFinder->_scintView.execute(SCI_SETPROPERTY, (WPARAM)"@MarkingsStruct", (LPARAM)ptrword);
	}
	
	::SendMessage(_pFinder->getHSelf(), WM_SIZE, 0, 0);

	int cmdid = 0;
	if (op == ALL_OPEN_DOCS)
		cmdid = WM_FINDALL_INOPENEDDOC;
	else if (op == FILES_IN_DIR)
		cmdid = WM_FINDINFILES;
	else if (op == CURRENT_DOC)
		cmdid = WM_FINDALL_INCURRENTDOC;

	if (!cmdid) return;

	if (::SendMessage(_hParent, cmdid, 0, 0))
	{
		wsprintf(_findAllResultStr, TEXT("%d hits"), _findAllResult);
		if (_findAllResult) 
		{
			focusOnFinder();
		}
		else
			getFocus(); // no hits
	}
	else // error - search folder doesn't exist
		::SendMessage(_hSelf, WM_NEXTDLGCTL, (WPARAM)::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO), TRUE);
}


void FindReplaceDlg::setSearchText(TCHAR * txt2find) {
	HWND hCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
	if (txt2find && txt2find[0])
	{
		// We got a valid search string
		::SendMessage(hCombo, CB_SETCURSEL, (WPARAM)-1, 0); // remove selection - to allow using down arrow to get to last searched word
		::SetDlgItemText(_hSelf, IDFINDWHAT, txt2find);
	}
	::SendMessage(hCombo, CB_SETEDITSEL, 0, MAKELPARAM(0, -1)); // select all text - fast edit
}

void FindReplaceDlg::enableReplaceFunc(bool isEnable) 
{
	_currentStatus = isEnable?REPLACE_DLG:FIND_DLG;
	int hideOrShow = isEnable?SW_SHOW:SW_HIDE;
	RECT *pClosePos = isEnable?&_replaceClosePos:&_findClosePos;

	enableFindInFilesControls(false);
	enableMarkAllControls(false);
	// replace controls
	::ShowWindow(::GetDlgItem(_hSelf, ID_STATICTEXT_REPLACE),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACE),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEALL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEINSEL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), hideOrShow);

	// find controls
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), !hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL),!hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_CURRENTFILE),!hideOrShow);

	gotoCorrectTab();

	::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), pClosePos->left, pClosePos->top, pClosePos->right, pClosePos->bottom, TRUE);

	TCHAR label[MAX_PATH];
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);

	setDefaultButton(IDOK);
}

void FindReplaceDlg::enableMarkAllControls(bool isEnable)
{
	int hideOrShow = isEnable?SW_SHOW:SW_HIDE;
	::ShowWindow(::GetDlgItem(_hSelf, IDCMARKALL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_MARKLINE_CHECK),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PURGE_CHECK),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CLEAR_ALL),hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), hideOrShow);

	::ShowWindow(::GetDlgItem(_hSelf, IDC_DIR_STATIC), !hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), !hideOrShow);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), !hideOrShow);
}

void FindReplaceDlg::enableFindInFilesControls(bool isEnable)
{
	// Hide Items
	::ShowWindow(::GetDlgItem(_hSelf, IDWRAP), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_CURRENTFILE), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDOK), isEnable?SW_HIDE:SW_SHOW);

	::ShowWindow(::GetDlgItem(_hSelf, IDC_MARKLINE_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_PURGE_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CLEAR_ALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDCMARKALL), isEnable?SW_HIDE:SW_SHOW);

	::ShowWindow(::GetDlgItem(_hSelf, IDC_DIR_STATIC), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONUP), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDDIRECTIONDOWN), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACE), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEALL), isEnable?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES), isEnable?SW_HIDE:SW_SHOW);

	// Show Items
	if (isEnable)
	{
		::ShowWindow(::GetDlgItem(_hSelf, ID_STATICTEXT_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH), SW_SHOW);
	}
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_REPLACEINFILES), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_STATIC), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_BROWSE_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FIND_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_GOBACK_BUTTON), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK), isEnable?SW_SHOW:SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK), isEnable?SW_SHOW:SW_HIDE);
    ::ShowWindow(::GetDlgItem(_hSelf, IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK), isEnable?SW_SHOW:SW_HIDE);
}

void FindReplaceDlg::getPatterns(vector<generic_string> & patternVect)
{
	cutString(_env->_filters.c_str(), patternVect);
}

void FindReplaceDlg::saveInMacro(int cmd, int cmdType)
{
	int booleans = 0;
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_INIT, 0);
	::SendMessage(_hParent, WM_FRSAVE_STR, IDFINDWHAT,  reinterpret_cast<LPARAM>(_options._str2Search.c_str()));
	booleans |= _options._isWholeWord?IDF_WHOLEWORD:0;
	booleans |= _options._isMatchCase?IDF_MATCHCASE:0;
	booleans |= _options._dotMatchesNewline?IDF_REDOTMATCHNL:0;

	::SendMessage(_hParent, WM_FRSAVE_INT, IDNORMAL, _options._searchType);
	if (cmd == IDCMARKALL)
	{
		booleans |= _options._doPurge?IDF_PURGE_CHECK:0;
		booleans |= _options._doMarkLine?IDF_MARKLINE_CHECK:0;
	}
	if (cmdType & FR_OP_REPLACE)
		::SendMessage(_hParent, WM_FRSAVE_STR, IDREPLACEWITH, (LPARAM)_options._str4Replace.c_str());
	if (cmdType & FR_OP_FIF)
	{
		::SendMessage(_hParent, WM_FRSAVE_STR, IDD_FINDINFILES_DIR_COMBO, (LPARAM)_options._directory.c_str());
		::SendMessage(_hParent, WM_FRSAVE_STR, IDD_FINDINFILES_FILTERS_COMBO, (LPARAM)_options._filters.c_str());
		booleans |= _options._isRecursive?IDF_FINDINFILES_RECURSIVE_CHECK:0;
		booleans |= _options._isInHiddenDir?IDF_FINDINFILES_INHIDDENDIR_CHECK:0;
	}
	else if (!(cmdType & FR_OP_GLOBAL))
	{
		booleans |= _options._isInSelection?IDF_IN_SELECTION_CHECK:0;
		booleans |= _options._isWrapAround?IDF_WRAP:0;
		booleans |= _options._whichDirection?IDF_WHICH_DIRECTION:0;
	}
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_BOOLEANS, booleans);
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_EXEC, cmd);
}

void FindReplaceDlg::execSavedCommand(int cmd, int intValue, generic_string stringValue)
{
	switch(cmd)
	{
		case IDC_FRCOMMAND_INIT:
			_env = new FindOption;
			break;
		case IDFINDWHAT:
			_env->_str2Search = stringValue;
			break;
		case IDC_FRCOMMAND_BOOLEANS:
			_env->_isWholeWord = ((intValue & IDF_WHOLEWORD)> 0);
			_env->_isMatchCase = ((intValue & IDF_MATCHCASE)> 0);
			_env->_isRecursive = ((intValue & IDF_FINDINFILES_RECURSIVE_CHECK)> 0);
			_env->_isInHiddenDir = ((intValue & IDF_FINDINFILES_INHIDDENDIR_CHECK)> 0);
			_env->_doPurge = ((intValue & IDF_PURGE_CHECK)> 0);
			_env->_doMarkLine = ((intValue & IDF_MARKLINE_CHECK)> 0);
			_env->_isInSelection = ((intValue & IDF_IN_SELECTION_CHECK)> 0);
			_env->_isWrapAround = ((intValue & IDF_WRAP)> 0);
			_env->_whichDirection = ((intValue & IDF_WHICH_DIRECTION)> 0);
			_env->_dotMatchesNewline = ((intValue & IDF_REDOTMATCHNL)> 0);
			break;
		case IDNORMAL:
			_env->_searchType = (SearchType)intValue;
			break;
		case IDREPLACEWITH:
			_env->_str4Replace = stringValue;
			break;
		case IDD_FINDINFILES_DIR_COMBO:
			_env->_directory = stringValue;
			break;
		case IDD_FINDINFILES_FILTERS_COMBO:
			_env->_filters = stringValue;
			break;
		case IDC_FRCOMMAND_EXEC:
		{
			NppParameters *nppParamInst = NppParameters::getInstance();
			switch(intValue)
			{
				case IDOK:
					nppParamInst->_isFindReplacing = true;
					processFindNext(_env->_str2Search.c_str());
					nppParamInst->_isFindReplacing = false;
					break;
				case IDREPLACE:
					nppParamInst->_isFindReplacing = true;
					processReplace(_env->_str2Search.c_str(), _env->_str4Replace.c_str(), _env);
					nppParamInst->_isFindReplacing = false;
					break;
				case IDC_FINDALL_OPENEDFILES:
					nppParamInst->_isFindReplacing = true;
					findAllIn(ALL_OPEN_DOCS);
					nppParamInst->_isFindReplacing = false;
					break;
				case IDC_FINDALL_CURRENTFILE:
					nppParamInst->_isFindReplacing = true;
					findAllIn(FILES_IN_DIR);
					nppParamInst->_isFindReplacing = false;
					break;
				case IDC_REPLACE_OPENEDFILES :
					nppParamInst->_isFindReplacing = true;
					replaceAllInOpenedDocs();
					nppParamInst->_isFindReplacing = false;
					break;
				case IDD_FINDINFILES_FIND_BUTTON :
					nppParamInst->_isFindReplacing = true;
					findAllIn(FILES_IN_DIR);
					nppParamInst->_isFindReplacing = false;
					break;

				case IDD_FINDINFILES_REPLACEINFILES :
				{
					generic_string msg = TEXT("Are you sure you want to replace all occurrences in :\r");
					msg += _env->_directory;
					msg += TEXT("\rfor file type : ");
					msg += (_env->_filters[0])?_env->_filters:TEXT("*.*");
					
					if (::MessageBox(_hParent, msg.c_str(), TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						nppParamInst->_isFindReplacing = true;
						::SendMessage(_hParent, WM_REPLACEINFILES, 0, 0);
						nppParamInst->_isFindReplacing = false;
					}
					break;
				}
				case IDREPLACEALL :
				{
					nppParamInst->_isFindReplacing = true;
					(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
					int nbReplaced = processAll(ProcessReplaceAll, _env);
					(*_ppEditView)->execute(SCI_ENDUNDOACTION);
					nppParamInst->_isFindReplacing = false;

					generic_string result = TEXT("");
					
					if (nbReplaced < 0)
						result = TEXT("The regular expression to search is formed badly");
					else
					{
						TCHAR moreInfo[64];
						wsprintf(moreInfo, TEXT("%d occurrences were replaced."), nbReplaced);
						result = moreInfo;
					}
					::MessageBox(_hParent, result.c_str(), TEXT("Replace All"), MB_OK);
					break;
				}
				case IDCCOUNTALL :
				{
					int nbCounted = processAll(ProcessCountAll, _env);
					generic_string result = TEXT("");

					if (nbCounted < 0)
						result = TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
					else
					{
						TCHAR moreInfo[128];
						wsprintf(moreInfo, TEXT("%d match(es) to occurrence(s)"), nbCounted);
						result = moreInfo;
					}
					::MessageBox(_hParent, result.c_str(), TEXT("Count"), MB_OK);
					break;
				}
				case IDCMARKALL:
				{
					nppParamInst->_isFindReplacing = true;
					int nbMarked = processAll(ProcessMarkAll, _env);
					nppParamInst->_isFindReplacing = false;
					generic_string result = TEXT("");
					if (nbMarked < 0)
						result = TEXT("The regular expression to search is formed badly.\r\nIs it resulting in nothing?");
					else
					{
						TCHAR moreInfo[128];
						wsprintf(moreInfo, TEXT("%d match(es) to occurrence(s)"), nbMarked);
						result = moreInfo;
					}
					::MessageBox(_hParent, result.c_str(), TEXT("Mark"), MB_OK);
					break;
				}
				default:
					throw std::runtime_error("Internal error: unknown saved command!");
			}
			delete _env;
			_env = &_options;
			break;
		}
		default:
			throw std::runtime_error("Internal error: unknown SnR command!");
	}
}

void FindReplaceDlg::setFindInFilesDirFilter(const TCHAR *dir, const TCHAR *filters)
{
	if (dir)
	{
		_options._directory = dir;
		::SetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, dir);
	}
	if (filters)
	{
		_options._filters = filters;
		::SetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters);
	}
}

void FindReplaceDlg::initOptionsFromDlg()
{
	_options._isWholeWord = isCheckedOrNot(IDWHOLEWORD);
	_options._isMatchCase = isCheckedOrNot(IDMATCHCASE);
	_options._searchType = isCheckedOrNot(IDREGEXP)?FindRegex:isCheckedOrNot(IDEXTENDED)?FindExtended:FindNormal;
	_options._isWrapAround = isCheckedOrNot(IDWRAP);
	_options._isInSelection = isCheckedOrNot(IDC_IN_SELECTION_CHECK);

	_options._dotMatchesNewline = isCheckedOrNot(IDREDOTMATCHNL);
	_options._doPurge = isCheckedOrNot(IDC_PURGE_CHECK);
	_options._doMarkLine = isCheckedOrNot(IDC_MARKLINE_CHECK);

	_options._whichDirection = isCheckedOrNot(IDDIRECTIONDOWN);
	
	_options._isRecursive = isCheckedOrNot(IDD_FINDINFILES_RECURSIVE_CHECK);
	_options._isInHiddenDir = isCheckedOrNot(IDD_FINDINFILES_INHIDDENDIR_CHECK);
}

void FindReplaceDlg::doDialog(DIALOG_TYPE whichType, bool isRTL, bool toShow)
{
	if (!isCreated())
	{
		create(IDD_FIND_REPLACE_DLG, isRTL);
		_isRTL = isRTL;
	}

	if (whichType == FINDINFILES_DLG)
		enableFindInFilesFunc();
	else if (whichType == MARK_DLG)
		enableMarkFunc();
	else
		enableReplaceFunc(whichType == REPLACE_DLG);

	::SetFocus(::GetDlgItem(_hSelf, IDFINDWHAT));
    display(toShow);
}

LRESULT FAR PASCAL FindReplaceDlg::finderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN && (wParam == VK_DELETE || wParam == VK_RETURN))
	{
		ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA));
		Finder *pFinder = (Finder *)(::GetWindowLongPtr(pScint->getHParent(), GWL_USERDATA));
		if (wParam == VK_RETURN)
			pFinder->GotoFoundLine();
		else // VK_DELETE
			pFinder->DeleteResult();
		return 0;
	}
	else
		// Call default (original) window procedure
		return CallWindowProc((WNDPROC) originalFinderProc, hwnd, message, wParam, lParam);
}

void FindReplaceDlg::enableFindInFilesFunc()
{
	enableFindInFilesControls();
	_currentStatus = FINDINFILES_DLG;
	gotoCorrectTab();
	::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), _findInFilesClosePos.left, _findInFilesClosePos.top, _findInFilesClosePos.right, _findInFilesClosePos.bottom, TRUE);
	TCHAR label[MAX_PATH];
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
	setDefaultButton(IDD_FINDINFILES_FIND_BUTTON);
}

void FindReplaceDlg::enableMarkFunc()
{
	enableFindInFilesControls(false);
	enableMarkAllControls(true);

	// Replace controls to hide
	::ShowWindow(::GetDlgItem(_hSelf, ID_STATICTEXT_REPLACE),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACE),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEWITH),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEALL),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDREPLACEINSEL),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE_OPENEDFILES),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION),SW_HIDE);

	// find controls to hide
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_OPENEDFILES), SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDCCOUNTALL),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_FINDALL_CURRENTFILE),SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDOK),SW_HIDE);

	_currentStatus = MARK_DLG;
	gotoCorrectTab();
	::MoveWindow(::GetDlgItem(_hSelf, IDCANCEL), _findInFilesClosePos.left, _findInFilesClosePos.top, _findInFilesClosePos.right, _findInFilesClosePos.bottom, TRUE);
	TCHAR label[MAX_PATH];
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
	setDefaultButton(IDCMARKALL);
}
void FindReplaceDlg::combo2ExtendedMode(int comboID)
{
	HWND hFindCombo = ::GetDlgItem(_hSelf, comboID);
	if (!hFindCombo) return;
	
	generic_string str2transform = getTextFromCombo(hFindCombo);
		
    // Count the number of character '\n' and '\r'
    size_t nbEOL = 0;
    size_t str2transformLen = lstrlen(str2transform.c_str());
    for (size_t i = 0 ; i < str2transformLen ; i++)
    {
        if (str2transform[i] == '\r' || str2transform[i] == '\n')
            nbEOL++;
    }

    if (nbEOL)
    {
		TCHAR * newBuffer = new TCHAR[str2transformLen + nbEOL*2 + 1];
        int j = 0;
        for (size_t i = 0 ; i < str2transformLen ; i++)
        {
            if (str2transform[i] == '\r')
            {
                newBuffer[j++] = '\\';
                newBuffer[j++] = 'r';
            }
            else if (str2transform[i] == '\n')
            {
                newBuffer[j++] = '\\';
                newBuffer[j++] = 'n';
            }
            else
            {
                newBuffer[j++] = str2transform[i];
            }
        }
        newBuffer[j++] = '\0';
		setSearchText(newBuffer);

        _options._searchType = FindExtended;
		::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, FALSE, 0);
		::SendDlgItemMessage(_hSelf, IDEXTENDED, BM_SETCHECK, TRUE, 0);
		::SendDlgItemMessage(_hSelf, IDREGEXP, BM_SETCHECK, FALSE, 0);

		delete [] newBuffer;
    }
}

void Finder::addSearchLine(const TCHAR *searchName)
{
	generic_string str = TEXT("Search \"");
	str += searchName;
	str += TEXT("\"\r\n");

	setFinderReadOnly(false);
	_scintView.addGenericText(str.c_str());
	setFinderReadOnly(true);
	_lastSearchHeaderPos = _scintView.execute(SCI_GETCURRENTPOS) - 2;

	_pMainFoundInfos->push_back(EmptyFoundInfo);
	_pMainMarkings->push_back(EmptySearchResultMarking);
}

void Finder::addFileNameTitle(const TCHAR * fileName)
{
	generic_string str = TEXT("  ");
	str += fileName;
	str += TEXT("\r\n");

	setFinderReadOnly(false);
	_scintView.addGenericText(str.c_str());
	setFinderReadOnly(true);
	_lastFileHeaderPos = _scintView.execute(SCI_GETCURRENTPOS) - 2;

	_pMainFoundInfos->push_back(EmptyFoundInfo);
	_pMainMarkings->push_back(EmptySearchResultMarking);
}

void Finder::addFileHitCount(int count)
{
	TCHAR text[20];
	wsprintf(text, TEXT(" (%i hits)"), count);
	setFinderReadOnly(false);
	_scintView.insertGenericTextFrom(_lastFileHeaderPos, text);
	setFinderReadOnly(true);
	nFoundFiles++;
}

void Finder::addSearchHitCount(int count)
{
	TCHAR text[50];
	wsprintf(text, TEXT(" (%i hits in %i files)"), count, nFoundFiles);
	setFinderReadOnly(false);
	_scintView.insertGenericTextFrom(_lastSearchHeaderPos, text);
	setFinderReadOnly(true);
}


void Finder::add(FoundInfo fi, SearchResultMarking mi, const TCHAR* foundline, int lineNb)
{
	_pMainFoundInfos->push_back(fi);
	generic_string str = TEXT("\tLine ");

	TCHAR lnb[16];
	wsprintf(lnb, TEXT("%d"), lineNb);
	str += lnb;
	str += TEXT(": ");
	mi._start += str.length();
	mi._end += str.length();
	str += foundline;

	if (str.length() >= SC_SEARCHRESULT_LINEBUFFERMAXLENGTH)
	{
		const TCHAR * endOfLongLine = TEXT("...\r\n");
		str = str.substr(0, SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - lstrlen(endOfLongLine) - 1);
		str += endOfLongLine;
	}
	setFinderReadOnly(false);
	_scintView.addGenericText(str.c_str(), &mi._start, &mi._end);
	setFinderReadOnly(true);
	_pMainMarkings->push_back(mi);
}

void Finder::removeAll() 
{
	_pMainFoundInfos->clear();
	_pMainMarkings->clear();
	setFinderReadOnly(false);
	_scintView.execute(SCI_CLEARALL);
	setFinderReadOnly(true);
}

void Finder::openAll()
{
	size_t sz = _pMainFoundInfos->size();

	for (size_t i = 0; i < sz; i++)
	{
		::SendMessage(::GetParent(_hParent), WM_DOOPEN, 0, (LPARAM)_pMainFoundInfos->at(i)._fullPath.c_str());
	}
}

void Finder::beginNewFilesSearch()
{
	//_scintView.execute(SCI_SETLEXER, SCLEX_NULL);

	_scintView.execute(SCI_SETCURRENTPOS, 0);
	_pMainFoundInfos = _pMainFoundInfos == &_foundInfos1 ? &_foundInfos2 : &_foundInfos1;
	_pMainMarkings = _pMainMarkings == &_markings1 ? &_markings2 : &_markings1;
	nFoundFiles = 0;

	// fold all old searches (1st level only)
	_scintView.collapse(searchHeaderLevel - SC_FOLDLEVELBASE, fold_collapse);
}

void Finder::finishFilesSearch(int count)
{
	std::vector<FoundInfo>* _pOldFoundInfos;
	std::vector<SearchResultMarking>* _pOldMarkings;
	_pOldFoundInfos = _pMainFoundInfos == &_foundInfos1 ? &_foundInfos2 : &_foundInfos1;
	_pOldMarkings = _pMainMarkings == &_markings1 ? &_markings2 : &_markings1;
	
	_pOldFoundInfos->insert(_pOldFoundInfos->begin(), _pMainFoundInfos->begin(), _pMainFoundInfos->end());
	_pOldMarkings->insert(_pOldMarkings->begin(), _pMainMarkings->begin(), _pMainMarkings->end());
	_pMainFoundInfos->clear();
	_pMainMarkings->clear();
	_pMainFoundInfos = _pOldFoundInfos;
	_pMainMarkings = _pOldMarkings;
	
	_MarkingsStruct._length = _pMainMarkings->size();
	_MarkingsStruct._markings = &((*_pMainMarkings)[0]);

	addSearchHitCount(count);
	_scintView.execute(SCI_SETSEL, 0, 0);

	_scintView.execute(SCI_SETLEXER, SCLEX_SEARCHRESULT);
	_scintView.execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
}


void Finder::setFinderStyle()
{
	// Set global styles for the finder
	_scintView.performGlobalStyles();
	
	// Set current line background color for the finder
	const TCHAR * lexerName = ScintillaEditView::langNames[L_SEARCHRESULT].lexerName;
	LexerStyler *pStyler = (_scintView._pParameter->getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		int i = pStyler->getStylerIndexByID(SCE_SEARCHRESULT_CURRENT_LINE);
		if (i != -1)
		{
			Style & style = pStyler->getStyler(i);
			_scintView.execute(SCI_SETCARETLINEBACK, style._bgColor);
		}
	}

	_scintView.setSearchResultLexer();
	_scintView.execute(SCI_COLOURISE, 0, -1);
}

BOOL CALLBACK Finder::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE :
				{
					_scintView.foldAll(fold_collapse);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE :
				{
					_scintView.foldAll(fold_uncollapse);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERCOPY :
				{
					_scintView.execute(SCI_COPY);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERSELECTALL :
				{
					_scintView.execute(SCI_SELECTALL);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFERCLEARALL:
				{
					removeAll();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINFEROPENALL:
				{
					openAll();
					return TRUE;
				}

				default :
				{
					return FALSE;
				}
			}
		}
		
		case WM_CONTEXTMENU :
		{
			if (HWND(wParam) == _scintView.getHSelf())
			{
				POINT p;
				::GetCursorPos(&p);
				ContextMenu scintillaContextmenu;
				vector<MenuItemUnit> tmp;
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE, TEXT("Collapse all")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE, TEXT("Uncollapse all")));
				tmp.push_back(MenuItemUnit(0, TEXT("Separator")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCOPY, TEXT("Copy")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERSELECTALL, TEXT("Select All")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFERCLEARALL, TEXT("Clear All")));
				tmp.push_back(MenuItemUnit(0, TEXT("Separator")));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINFEROPENALL, TEXT("Open All")));

				scintillaContextmenu.create(_hSelf, tmp);

				scintillaContextmenu.display(p);
				return TRUE;
			}
			return ::DefWindowProc(_hSelf, message, wParam, lParam);
		}

		case WM_SIZE :
		{
			RECT rc;
			getClientRect(rc);
			_scintView.reSizeTo(rc);
			break;
		}

		case WM_NOTIFY:
		{
			notify(reinterpret_cast<SCNotification *>(lParam));
			return FALSE;
		}
		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return FALSE;
}

void FindIncrementDlg::init(HINSTANCE hInst, HWND hPere, FindReplaceDlg *pFRDlg, bool isRTL)
{
	Window::init(hInst, hPere);
	if (!pFRDlg)
		throw std::runtime_error("FindIncrementDlg::init : Parameter pFRDlg is null");

	_pFRDlg = pFRDlg;
	create(IDD_INCREMENT_FIND, isRTL);
	_isRTL = isRTL;
}

void FindIncrementDlg::destroy()
{
	if (_pRebar) 
	{
		_pRebar->removeBand(_rbBand.wID);
		_pRebar = NULL;
	}
}

void FindIncrementDlg::display(bool toShow) const
{
	if (!_pRebar)
	{
		Window::display(toShow);
		return;
	}
	if (toShow)
	{
		::SetFocus(::GetDlgItem(_hSelf, IDC_INCFINDTEXT));
		// select the whole find editor text
		::SendDlgItemMessage(_hSelf, IDC_INCFINDTEXT, EM_SETSEL, 0, -1);
	}
	_pRebar->setIDVisible(_rbBand.wID, toShow);
}

#define SHIFTED 0x8000
#define BCKGRD_COLOR (RGB(255,102,102))
#define TXT_COLOR    (RGB(255,255,255))

BOOL CALLBACK FindIncrementDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message)
	{
		// Make edit field red if not found
		case WM_CTLCOLOREDIT :
		{
			// if the text not found modify the background color of the editor
			static HBRUSH hBrushBackground = CreateSolidBrush(BCKGRD_COLOR);
			if (FSNotFound != getFindStatus())
				return FALSE; // text found, use the default color

			// text not found
			SetTextColor((HDC)wParam, TXT_COLOR);
			SetBkColor((HDC)wParam, BCKGRD_COLOR);
			return (LRESULT)hBrushBackground;
		}

		case WM_COMMAND : 
		{
			bool isUnicode = (*(_pFRDlg->_ppEditView))->getCurrentBuffer()->getUnicodeMode() != uni8Bit;
			FindStatus findStatus = FSFound;
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
					(*(_pFRDlg->_ppEditView))->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_INC);
					::SetFocus((*(_pFRDlg->_ppEditView))->getHSelf());

					display(false);
					return TRUE;

				case IDC_INCFINDPREVOK :
				case IDC_INCFINDNXTOK :
				case IDOK :
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._incrementalType = NextIncremental;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));
					if (LOWORD(wParam) == IDC_INCFINDPREVOK)
						fo._whichDirection = DIR_UP;
					else if (LOWORD(wParam) == IDOK)
					{
						SHORT nVirtKey = GetKeyState(VK_SHIFT);
						if (nVirtKey & SHIFTED)
							fo._whichDirection = DIR_UP;
					}
					
					generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					_pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
					setFindStatus(findStatus);
				}
				return TRUE;

				case IDC_INCFINDTEXT :
				{
					switch(HIWORD(wParam))
					{
						case EN_CHANGE :
						{
							FindOption fo;
							fo._isWholeWord = false;
							fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));
							fo._incrementalType = FirstIncremental;
							
							generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
							_pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
							setFindStatus(findStatus);
						}
					return TRUE;
					case EN_KILLFOCUS :
					case EN_SETFOCUS :
						break;
					}
				}
				return TRUE;

				case IDC_INCFINDMATCHCASE:
					{
						FindOption fo;
						fo._isWholeWord = false;
					fo._incrementalType = FirstIncremental;
						fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

						generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
					bool isFound = _pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);
					setFindStatus(findStatus);
						if (!isFound)
						{
							CharacterRange range = (*(_pFRDlg->_ppEditView))->getSelection();
							(*(_pFRDlg->_ppEditView))->execute(SCI_SETSEL, (WPARAM)-1, range.cpMin);
						}
				}

				case IDC_INCFINDHILITEALL :
				{
					FindOption fo;
					fo._isWholeWord = false;
					fo._incrementalType = FirstIncremental;
					fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

					generic_string str2Search = _pFRDlg->getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT), isUnicode);
						bool isHiLieAll = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDHILITEALL, BM_GETCHECK, 0, 0));
						if (str2Search == TEXT(""))
							isHiLieAll = false;
						markSelectedTextInc(isHiLieAll, &fo);
					}
				return TRUE;
			}
		}

		case WM_ERASEBKGND:
		{
			HWND hParent = ::GetParent(_hSelf);
			HDC winDC = (HDC)wParam;
			//RTL handling
			POINT pt = {0, 0}, ptOrig = {0, 0};
			::MapWindowPoints(_hSelf, hParent, &pt, 1);
			::OffsetWindowOrgEx((HDC)wParam, pt.x, pt.y, &ptOrig);
			LRESULT lResult = SendMessage(hParent, WM_ERASEBKGND,(WPARAM)winDC, 0);
			::SetWindowOrgEx(winDC, ptOrig.x, ptOrig.y, NULL);
			return (BOOL)lResult;
			break; 
		}

		case WM_MOVE:
		{
			::InvalidateRect(_hSelf, NULL, TRUE);	//when moving, force background redraw
			return FALSE;
			break;
		}
	}
	return FALSE;
}

void FindIncrementDlg::markSelectedTextInc(bool enable, FindOption *opt)
{
	(*(_pFRDlg->_ppEditView))->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_INC);

	if (!enable)
		return;

	//Get selection
	CharacterRange range = (*(_pFRDlg->_ppEditView))->getSelection();

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
		return;

	TCHAR text2Find[FINDREPLACE_MAXLENGTH];
	(*(_pFRDlg->_ppEditView))->getGenericSelectedText(text2Find, FINDREPLACE_MAXLENGTH, false);	//do not expand selection (false)
	opt->_str2Search = text2Find;
	_pFRDlg->markAllInc(opt);
}

void FindIncrementDlg::setFindStatus(FindStatus iStatus) 
{
	static TCHAR *findStatus[] = { TEXT(""), // FSFound
	                               TEXT("Phrase not found"), //FSNotFound
	                               TEXT("Reached top of page, continued from bottom"), // FSTopReached
	                               TEXT("Reached end of page, continued from top")}; // FSEndReached
	if (iStatus<0 || iStatus >= sizeof(findStatus)/sizeof(findStatus[0]))
		return; // out of range

	_FindStatus = iStatus;

	// get the HWND of the editor
	HWND hEditor = ::GetDlgItem(_hSelf, IDC_INCFINDTEXT);

	// invalidate the editor rect
	::InvalidateRect(hEditor, NULL, TRUE);
	::SendDlgItemMessage(_hSelf, IDC_INCFINDSTATUS, WM_SETTEXT, 0, (LPARAM)findStatus[iStatus]);
}

void FindIncrementDlg::addToRebar(ReBar * rebar) 
{
	if(_pRebar)
		return;

	_pRebar = rebar;
	RECT client;
	getClientRect(client);

	ZeroMemory(&_rbBand, REBARBAND_SIZE);
	_rbBand.cbSize  = REBARBAND_SIZE;

	_rbBand.fMask   = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
					  RBBIM_SIZE | RBBIM_ID;

	_rbBand.fStyle		= RBBS_HIDDEN;
	_rbBand.hwndChild	= getHSelf();
	_rbBand.wID			= REBAR_BAR_SEARCH;	//ID REBAR_BAR_SEARCH for search dialog
	_rbBand.cxMinChild	= 0;
	_rbBand.cyIntegral	= 1;
	_rbBand.cyMinChild	= _rbBand.cyMaxChild	= client.bottom-client.top;
	_rbBand.cxIdeal		= _rbBand.cx			= client.right-client.left;

	_pRebar->addBand(&_rbBand, true);
	_pRebar->setGrayBackground(_rbBand.wID);
}
