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


#include <shlwapi.h>
#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "Notepad_plus_msgs.h"
#include "localization.h"
#include "Common.h"
#include "Utf8.h"

using namespace std;

FindOption * FindReplaceDlg::_env;
FindOption FindReplaceDlg::_options;

#define SHIFTED 0x8000

const wstring noFoundPotentialReason = L"The given occurrence cannot be found. You may have forgotten to check \"Wrap around\" (to ON), \"Match case\" (to OFF), or \"Match whole word only\" (to OFF).";

void addText2Combo(const wchar_t * txt2add, HWND hCombo)
{
	if (!hCombo) return;
	if (!lstrcmp(txt2add, L"")) return;

	auto i = ::SendMessage(hCombo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(txt2add));
	if (i != CB_ERR) // found
	{
		::SendMessage(hCombo, CB_DELETESTRING, i, 0);
	}

	i = ::SendMessage(hCombo, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(txt2add));
	::SendMessage(hCombo, CB_SETCURSEL, i, 0);
}

wstring getTextFromCombo(HWND hCombo)
{
	wchar_t str[FINDREPLACE_MAXLENGTH] = { '\0' };
	::SendMessage(hCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, reinterpret_cast<LPARAM>(str));
	return wstring(str);
}

void delLeftWordInEdit(HWND hEdit)
{
	wchar_t str[FINDREPLACE_MAXLENGTH] = { '\0' };
	::SendMessage(hEdit, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, reinterpret_cast<LPARAM>(str));
	WORD cursor = 0;
	::SendMessage(hEdit, EM_GETSEL, (WPARAM)&cursor, 0);
	WORD wordstart = cursor;
	while (wordstart > 0) {
		wchar_t c = str[wordstart - 1];
		if (c != ' ' && c != '\t')
			break;
		--wordstart;
	}
	while (wordstart > 0) {
		wchar_t c = str[wordstart - 1];
		if (c == ' ' || c == '\t')
			break;
		--wordstart;
	}
	if (wordstart < cursor) {
		::SendMessage(hEdit, EM_SETSEL, (WPARAM)wordstart, (LPARAM)cursor);
		::SendMessage(hEdit, EM_REPLACESEL, (WPARAM)TRUE, reinterpret_cast<LPARAM>(L""));
	}
}

LRESULT run_swapButtonProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_RBUTTONUP:
		{
			::SendMessage(GetParent(hwnd), message, wParam, lParam);
			break;
		}

		default:
			break;
	}
	return ::CallWindowProc(oldEditProc, hwnd, message, wParam, lParam);
}

int Searching::convertExtendedToString(const wchar_t * query, wchar_t * result, int length)
{	//query may equal to result, since it always gets smaller
	int i = 0, j = 0;
	int charLeft = length;

	while (i < length)
	{	//because the backslash escape quences always reduce the size of the wstring, no overflow checks have to be made for target, assuming parameters are correct
		wchar_t current = query[i];
		--charLeft;
		if (current == '\\' && charLeft)
		{	//possible escape sequence
			++i;
			--charLeft;
			current = query[i];
			switch(current)
			{
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
				case 'u':
				{
					int size = 0, base = 0;
					if (current == 'b')
					{	//11111111
						size = 8, base = 2;
					}
					else if (current == 'o')
					{	//377
						size = 3, base = 8;
					}
					else if (current == 'd')
					{	//255
						size = 3, base = 10;
					}
					else if (current == 'x')
					{	//0xFF
						size = 2, base = 16;
					}
					else if (current == 'u')
					{	//0xCDCD
						size = 4, base = 16;
					}

					if (charLeft >= size)
					{
						int res = 0;
						if (Searching::readBase(query+(i+1), &res, base, size))
						{
							result[j] = static_cast<wchar_t>(res);
							i += size;
							break;
						}
					}
					//not enough chars to make parameter, use default method as fallback
					[[fallthrough]];
				}

				default:
				{	//unknown sequence, treat as regular text
					result[j] = '\\';
					++j;
					result[j] = current;
					break;
				}
			}
		}
		else
		{
			result[j] = query[i];
		}
		++i;
		++j;
	}
	result[j] = 0;
	return j;
}

bool Searching::readBase(const wchar_t * str, int * value, int base, int size)
{
	int i = 0, temp = 0;
	*value = 0;
	wchar_t max = '0' + static_cast<wchar_t>(base) - 1;

	while (i < size)
	{
		wchar_t current = str[i];
		if (current >= 'A')
		{
			current &= 0xdf;
			current -= ('A' - '0' - 10);
		}
		else if (current > '9')
			return false;

		if (current >= '0' && current <= max)
		{
			temp *= base;
			temp += (current - '0');
		}
		else
		{
			return false;
		}
		++i;
	}
	*value = temp;
	return true;
}

void Searching::displaySectionCentered(size_t posStart, size_t posEnd, ScintillaEditView * pEditView, bool isDownwards)
{
	// Make sure target lines are unfolded
	pEditView->execute(SCI_ENSUREVISIBLE, pEditView->execute(SCI_LINEFROMPOSITION, posStart));
	pEditView->execute(SCI_ENSUREVISIBLE, pEditView->execute(SCI_LINEFROMPOSITION, posEnd));

	// Jump-scroll to center, if current position is out of view
	pEditView->execute(SCI_SETVISIBLEPOLICY, CARET_JUMPS | CARET_EVEN);
	pEditView->execute(SCI_ENSUREVISIBLEENFORCEPOLICY, pEditView->execute(SCI_LINEFROMPOSITION, isDownwards ? posEnd : posStart));
	// When searching up, the beginning of the (possible multiline) result is important, when scrolling down the end
	pEditView->execute(SCI_GOTOPOS, isDownwards ? posEnd : posStart);
	pEditView->execute(SCI_SETVISIBLEPOLICY, CARET_EVEN);
	pEditView->execute(SCI_ENSUREVISIBLEENFORCEPOLICY, pEditView->execute(SCI_LINEFROMPOSITION, isDownwards ? posEnd : posStart));

	// Adjust so that we see the entire match; primarily horizontally
	pEditView->execute(SCI_SCROLLRANGE, posStart, posEnd);

	// Move cursor to end of result and select result
	pEditView->execute(SCI_GOTOPOS, posEnd);
	pEditView->execute(SCI_SETANCHOR, posStart);

	// Update Scintilla's knowledge about what column the caret is in, so that if user
	// does up/down arrow as first navigation after the search result is selected,
	// the caret doesn't jump to an unexpected column
	pEditView->execute(SCI_CHOOSECARETX);
}

WNDPROC FindReplaceDlg::originalFinderProc = nullptr;
WNDPROC FindReplaceDlg::originalComboEditProc = nullptr;

FindReplaceDlg::~FindReplaceDlg()
{
	_tab.destroy();
	delete _pFinder;
	for (int n = static_cast<int32_t>(_findersOfFinder.size()) - 1; n >= 0; n--)
	{
		delete _findersOfFinder[n];
		_findersOfFinder.erase(_findersOfFinder.begin() + n);
	}

	if (_shiftTrickUpTip)
		::DestroyWindow(_shiftTrickUpTip);

	if (_2ButtonsTip)
		::DestroyWindow(_2ButtonsTip);

	if (_dirFromActiveDocTip)
		::DestroyWindow(_dirFromActiveDocTip);

	if (_filterTip)
		::DestroyWindow(_filterTip);

	if (_hComboBoxFont)
		::DeleteObject(_hComboBoxFont);

	if (_hLargerBolderFont)
		::DeleteObject(_hLargerBolderFont);

	if (_hCourrierNewFont)
		::DeleteObject(_hCourrierNewFont);

	delete[] _uniFileName;
}

void FindReplaceDlg::create(int dialogID, bool isRTL, bool msgDestParent, bool toShow)
{
	StaticDialog::create(dialogID, isRTL, msgDestParent);
	fillFindHistory();
	_currentStatus = REPLACE_DLG;
	initOptionsFromDlg();

	_statusBar.init(GetModuleHandle(NULL), _hSelf, 0);
	_statusBar.display();

	setDpi();

	RECT rcClient{};
	getClientRect(rcClient);

	RECT rcCount{};
	getMappedChildRect(IDCCOUNTALL, rcCount);

	RECT rcOk{};
	getMappedChildRect(IDOK, rcOk);

	RECT rcTransGrpb{};
	getMappedChildRect(IDC_TRANSPARENT_GRPBOX, rcTransGrpb);

	RECT rcStatusBar{};
	::GetWindowRect(_statusBar.getHSelf(), &rcStatusBar);

	const LONG gap = (rcCount.top - rcOk.bottom);
	_lesssModeHeight = (rcCount.bottom + gap);

	const LONG padding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER);
	_szBorder.cx = (_dpiManager.getSystemMetricsForDpi(SM_CXFRAME) + padding) * 2;
	_szBorder.cy = (_dpiManager.getSystemMetricsForDpi(SM_CYFRAME) + padding) * 2
		+ _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION)
		+ (rcStatusBar.bottom - rcStatusBar.top);

	//fill min dialog size info
	_szMinDialog.cx = rcClient.right - rcClient.left;
	_szMinDialog.cy = rcTransGrpb.bottom + gap;

	_tab.init(_hInst, _hSelf, false, true);
	NppDarkMode::subclassTabControl(_tab.getHSelf());

	const wchar_t *find = L"Find";
	const wchar_t *replace = L"Replace";
	const wchar_t *findInFiles = L"Find in Files";
	const wchar_t *findInProjects = L"Find in Projects";
	const wchar_t *mark = L"Mark";

	_tab.insertAtEnd(find);
	_tab.insertAtEnd(replace);
	_tab.insertAtEnd(findInFiles);
	_tab.insertAtEnd(findInProjects);
	_tab.insertAtEnd(mark);

	_tab.reSizeTo(rcClient);
	_tab.display();

	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI& nppGUI = nppParam.getNppGUI();

	const UINT swpFlags = toShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
	if (nppGUI._findWindowPos.bottom - nppGUI._findWindowPos.top != 0)  // check height against 0 as a test of valid data from config
	{
		RECT rc = getViewablePositionRect(nppGUI._findWindowPos);
		::SetWindowPos(_hSelf, HWND_TOP, rc.left, rc.top, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOACTIVATE);
		::SetWindowPos(_hSelf, HWND_TOP, 0, 0, rc.right - rc.left, _rc.bottom - _rc.top, swpFlags | SWP_NOMOVE);

		if ((swpFlags & SWP_SHOWWINDOW) == SWP_SHOWWINDOW)
			::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
	}
	else
	{
		goToCenter(swpFlags);
	}

	if (nppGUI._findWindowLessMode)
	{
		// reverse the value of _findWindowLessMode because the value will be inversed again in IDD_RESIZE_TOGGLE_BUTTON
		nppGUI._findWindowLessMode = false;

		::SendMessage(_hSelf, WM_COMMAND, IDD_RESIZE_TOGGLE_BUTTON, 0);
	}
}

void FindReplaceDlg::fillFindHistory()
{
	NppParameters& nppParams = NppParameters::getInstance();
	FindHistory & findHistory = nppParams.getFindHistory();

	fillComboHistory(IDFINDWHAT, findHistory._findHistoryFinds);
	fillComboHistory(IDREPLACEWITH, findHistory._findHistoryReplaces);
	fillComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._findHistoryFilters);
	fillComboHistory(IDD_FINDINFILES_DIR_COMBO, findHistory._findHistoryPaths);

	::SendDlgItemMessage(_hSelf, IDWRAP, BM_SETCHECK, findHistory._isWrap, 0);
	::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, findHistory._isMatchWord, 0);
	::SendDlgItemMessage(_hSelf, IDMATCHCASE, BM_SETCHECK, findHistory._isMatchCase, 0);
	::SendDlgItemMessage(_hSelf, IDC_BACKWARDDIRECTION, BM_SETCHECK, !findHistory._isDirectionDown, 0);

	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_INHIDDENDIR_CHECK, BM_SETCHECK, findHistory._isFifInHiddenFolder, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_RECURSIVE_CHECK, BM_SETCHECK, findHistory._isFifRecuisive, 0);

	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT1_CHECK, BM_SETCHECK, findHistory._isFifProjectPanel_1, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT2_CHECK, BM_SETCHECK, findHistory._isFifProjectPanel_2, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT3_CHECK, BM_SETCHECK, findHistory._isFifProjectPanel_3, 0);

	::SendDlgItemMessage(_hSelf, IDNORMAL, BM_SETCHECK, findHistory._searchMode == FindHistory::normal, 0);
	::SendDlgItemMessage(_hSelf, IDEXTENDED, BM_SETCHECK, findHistory._searchMode == FindHistory::extended, 0);
	::SendDlgItemMessage(_hSelf, IDREGEXP, BM_SETCHECK, findHistory._searchMode == FindHistory::regExpr, 0);
	::SendDlgItemMessage(_hSelf, IDREDOTMATCHNL, BM_SETCHECK, findHistory._dotMatchesNewline, 0);

	::SendDlgItemMessage(_hSelf, IDC_MARKLINE_CHECK, BM_SETCHECK, findHistory._isBookmarkLine, 0);
	::SendDlgItemMessage(_hSelf, IDC_PURGE_CHECK, BM_SETCHECK, findHistory._isPurge, 0);

	::SendDlgItemMessage(_hSelf, IDC_2_BUTTONS_MODE, BM_SETCHECK, findHistory._isSearch2ButtonsMode, 0);

	if (findHistory._searchMode == FindHistory::regExpr)
	{
		//regex doesn't allow wholeword
		::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, BST_UNCHECKED, 0);
		enableFindDlgItem(IDWHOLEWORD, false);

		// regex upward search is disabled
		::SendDlgItemMessage(_hSelf, IDC_BACKWARDDIRECTION, BM_SETCHECK, BST_UNCHECKED, 0);
		enableFindDlgItem(IDC_BACKWARDDIRECTION, nppParams.regexBackward4PowerUser());
		enableFindDlgItem(IDC_FINDPREV, nppParams.regexBackward4PowerUser());

		// If the search mode from history is regExp then enable the checkbox (. matches newline)
		enableFindDlgItem(IDREDOTMATCHNL);
	}

	if (nppParams.isTransparentAvailable())
	{
		showFindDlgItem(IDC_TRANSPARENT_CHECK);
		showFindDlgItem(IDC_TRANSPARENT_GRPBOX);
		showFindDlgItem(IDC_TRANSPARENT_LOSSFOCUS_RADIO);
		showFindDlgItem(IDC_TRANSPARENT_ALWAYS_RADIO);
		showFindDlgItem(IDC_PERCENTAGE_SLIDER);

		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
		::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, findHistory._transparency);

		if (findHistory._transparencyMode == FindHistory::none)
		{
			enableFindDlgItem(IDC_TRANSPARENT_LOSSFOCUS_RADIO, false);
			enableFindDlgItem(IDC_TRANSPARENT_ALWAYS_RADIO, false);
			enableFindDlgItem(IDC_PERCENTAGE_SLIDER, false);
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
				(NppParameters::getInstance()).SetTransparent(_hSelf, findHistory._transparency);

			}
			::SendDlgItemMessage(_hSelf, id, BM_SETCHECK, TRUE, 0);
		}
	}
}

void FindReplaceDlg::fillComboHistory(int id, const vector<wstring> & strings)
{
	HWND hCombo = ::GetDlgItem(_hSelf, id);

	for (vector<wstring>::const_reverse_iterator i = strings.rbegin() ; i != strings.rend(); ++i)
	{
		addText2Combo(i->c_str(), hCombo);
	}

	//empty string is not added to CB items, so we need to set it manually
	if (!strings.empty() && strings.begin()->empty())
	{
		SetWindowText(hCombo, L"");
		return;
	}

	::SendMessage(hCombo, CB_SETCURSEL, 0, 0); // select first item
}


void FindReplaceDlg::saveFindHistory()
{
	if (! isCreated()) return;
	FindHistory& findHistory = (NppParameters::getInstance()).getFindHistory();

	saveComboHistory(IDD_FINDINFILES_DIR_COMBO, findHistory._nbMaxFindHistoryPath, findHistory._findHistoryPaths, false);
	saveComboHistory(IDD_FINDINFILES_FILTERS_COMBO, findHistory._nbMaxFindHistoryFilter, findHistory._findHistoryFilters, true);
	saveComboHistory(IDFINDWHAT,                    findHistory._nbMaxFindHistoryFind, findHistory._findHistoryFinds, false);
	saveComboHistory(IDREPLACEWITH,                 findHistory._nbMaxFindHistoryReplace, findHistory._findHistoryReplaces, true);
}

int FindReplaceDlg::saveComboHistory(int id, int maxcount, vector<wstring> & strings, bool saveEmpty)
{
	wchar_t text[FINDREPLACE_MAXLENGTH] = { '\0' };
	HWND hCombo = ::GetDlgItem(_hSelf, id);
	int count = static_cast<int32_t>(::SendMessage(hCombo, CB_GETCOUNT, 0, 0));
	count = std::min<int>(count, maxcount);

	if (count == CB_ERR) return 0;

	strings.clear();

	if (saveEmpty)
	{
		if (::GetWindowTextLength(hCombo) == 0)
		{
			strings.push_back(wstring());
		}
	}

	for (int i = 0 ; i < count ; ++i)
	{
		auto cbTextLen = ::SendMessage(hCombo, CB_GETLBTEXTLEN, i, 0);
		if (cbTextLen <= FINDREPLACE_MAXLENGTH - 1)
		{
			::SendMessage(hCombo, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(text));
			strings.push_back(wstring(text));
		}
	}
	return count;
}

void FindReplaceDlg::updateCombos()
{
	updateCombo(IDREPLACEWITH);
	updateCombo(IDFINDWHAT);
}

void FindReplaceDlg::updateCombo(int comboID)
{
	HWND hCombo = ::GetDlgItem(_hSelf, comboID);
	addText2Combo(getTextFromCombo(hCombo).c_str(), hCombo);
}

FoundInfo Finder::EmptyFoundInfo(0, 0, 0, L"");
SearchResultMarkingLine Finder::EmptySearchResultMarking;

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

			// WM_LBUTTONUP can go to a "File not found" messagebox instead of Scintilla here, because the mouse is not captured.
			// The missing message causes mouse cursor flicker as soon as the mouse cursor is moved to a position outside the text editing area.
			::SendMessage(_scintView.getHSelf(), WM_LBUTTONUP, 0, 0);

			size_t pos = notification->position;
			if (static_cast<intptr_t>(pos) == INVALID_POSITION)
				pos = _scintView.execute(SCI_GETLINEENDPOSITION, notification->line);
			_scintView.execute(SCI_SETSEL, pos, pos);

			std::pair<intptr_t, intptr_t> newPos = gotoFoundLine();
			auto lineStartAbsPos = _scintView.execute(SCI_POSITIONFROMLINE, notification->line);
			intptr_t lineEndAbsPos = _scintView.execute(SCI_GETLINEENDPOSITION, notification->line);

			intptr_t begin = newPos.first + lineStartAbsPos;
			intptr_t end = newPos.second + lineStartAbsPos;

			if (end > lineEndAbsPos)
				end = lineEndAbsPos;
			
			if (begin < end)
			{
				_scintView.execute(SCI_SETSEL, begin, end);
				_scintView.execute(SCI_SCROLLRANGE, begin, end);
			}

		}
		break;

		case SCN_PAINTED :
			if (isDoubleClicked)
			{
				(*_ppEditView)->grabFocus();
				isDoubleClicked = false;
			}
			break;

		case SCN_UPDATEUI:
			::SendMessage(_hParent, NPPM_INTERNAL_CHECKUNDOREDOSTATE, 0, 0);
			break;

	}
	return false;
}


std::pair<intptr_t, intptr_t> Finder::gotoFoundLine(size_t nOccurrence)
{
	std::pair<intptr_t, intptr_t> emptyResult(0, 0);
	auto currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	auto lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	auto start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	auto end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);

	if (start + 2 >= end) return emptyResult; // avoid empty lines

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		_scintView.execute(SCI_TOGGLEFOLD, lno);
		return  emptyResult;
	}

	const FoundInfo& fInfo = *(_pMainFoundInfos->begin() + lno);
	const SearchResultMarkingLine& markingLine = *(_pMainMarkings->begin() + lno);

	// Switch to another document
	if (!::SendMessage(_hParent, WM_DOOPEN, 0, reinterpret_cast<LPARAM>(fInfo._fullPath.c_str()))) return emptyResult;

	(*_ppEditView)->_positionRestoreNeeded = false;

	size_t index = 0;

	if (nOccurrence > 0)
	{
		index = nOccurrence - 1;
	}
	else // nOccurrence == 0 -> not used: use current line relative pos to check if it's inside of a marked occurrence
	{
		intptr_t currentPosInLine = currentPos - start;

		for (std::pair<intptr_t, intptr_t> range : markingLine._segmentPostions)
		{
			if (range.first <= currentPosInLine && currentPosInLine <= range.second)
				break;

			++index;
		}
	}

	if (index >= fInfo._ranges.size())
		index = 0;

	Searching::displaySectionCentered(fInfo._ranges[index].first, fInfo._ranges[index].second, *_ppEditView);

	return markingLine._segmentPostions[index];
}

void Finder::deleteResult()
{
	auto currentPos = _scintView.execute(SCI_GETCURRENTPOS); // yniq - add handling deletion of multiple lines?

	auto lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	auto start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	auto end = _scintView.execute(SCI_GETLINEENDPOSITION, lno);
	if (start + 2 >= end) return; // avoid empty lines

	if (_scintView.execute(SCI_GETLEXER) == SCLEX_NULL)
	{
		_scintView.setLexer(L_SEARCHRESULT, LIST_NONE); // Restore searchResult lexer in case the lexer was changed to SCLEX_NULL in GotoFoundLine()
	}

	if (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)  // delete a folder
	{
		auto endline = _scintView.execute(SCI_GETLASTCHILD, lno, -1) + 1;
		if ((size_t)endline > _pMainFoundInfos->size()) return;

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno, _pMainFoundInfos->begin() + endline); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno, _pMainMarkings->begin() + endline);

		auto end2 = _scintView.execute(SCI_POSITIONFROMLINE, endline);
		_scintView.execute(SCI_SETSEL, start, end2);
		setFinderReadOnly(false);
		_scintView.execute(SCI_CLEAR);
		setFinderReadOnly(true);
	}
	else // delete one line
	{
		if ((size_t)lno >= _pMainFoundInfos->size()) return;

		_pMainFoundInfos->erase(_pMainFoundInfos->begin() + lno); // remove found info
		_pMainMarkings->erase(_pMainMarkings->begin() + lno);

		setFinderReadOnly(false);
		_scintView.execute(SCI_LINEDELETE);
		setFinderReadOnly(true);
	}
	_markingsStruct._length = static_cast<long>(_pMainMarkings->size());

	assert(_pMainFoundInfos->size() == _pMainMarkings->size());
	assert(size_t(_scintView.execute(SCI_GETLINECOUNT)) == _pMainFoundInfos->size() + 1);
}

vector<wstring> Finder::getResultFilePaths(bool onlyInSelectedText) const
{
	size_t fromLine = 0, toLine = 0;

	if (onlyInSelectedText)
	{
		const pair<size_t, size_t> lineRange = _scintView.getSelectionLinesRange();
		fromLine = lineRange.first;
		toLine = lineRange.second;
	}
	else
	{
		toLine = _scintView.execute(SCI_GETLINECOUNT) - 1;
	}

	size_t len = _pMainFoundInfos->size();
	vector<wstring> paths;

	for (size_t line = fromLine; line <= toLine; ++line)
	{
		const int lineFoldLevel = _scintView.execute(SCI_GETFOLDLEVEL, line) & SC_FOLDLEVELNUMBERMASK;
		if (lineFoldLevel == fileHeaderLevel)
		{
			// fileHeaderLevel lines don't have path info; have to look into the NEXT line for it,
			// but only need to do something special here if we are on the LAST line of the selection
			if (line == toLine)
			{
				++line;
			}
		}

		if (line < len)
		{
			wstring& path2add = (*_pMainFoundInfos)[line]._fullPath;
			if (!path2add.empty())
			{
				// make sure that path is not already in
				if (std::find(paths.begin(), paths.end(), path2add) == paths.end())
				{
					paths.push_back(path2add);
				}
			}
		}
	}

	return paths;
}


bool Finder::canFind(const wchar_t *fileName, size_t lineNumber, size_t* indexToStartFrom) const
{
	size_t len = _pMainFoundInfos->size();
	for (size_t i = *indexToStartFrom; i < len; ++i)
	{
		if ((*_pMainFoundInfos)[i]._fullPath == fileName)
		{
			if (lineNumber == (*_pMainFoundInfos)[i]._lineNumber)
			{
				*indexToStartFrom = i;
				return true;
			}
		}
	}
	return false;
}

// Y     : current pos
// X     : current sel
// XXXXY : current sel + current pos
// 
//                occurrence 1   occur. 2         occur. 3  occur. 4              Status      auxiliaryInfo
// =========================================================================================================
// situation  1:     Y [     ]    [     ]          [     ]  [     ]            : pos_infront        -1
// situation  2:       [     ]    [     ]          [     ]  [  Y  ]            : pos_inside          4
// situation  3:       [     ]  XXY     ]          [     ]  [     ]            : pos_inside          2
// situation  4:       [     ]    [     ]          [     ]  [  XXXY            : pos_inside          4
// situation  5:       [     ]    [     ]          [XXXXXY  [     ]            : pos_inside          3
// situation  6:       [     Y    [     ]          [     ]  [     ]            : pos_between         1
// situation  7:       [     ]    [     ]          [     ]  Y     ]            : pos_between         3
// situation  8:       [     ]    [     ]          [     Y  [     ]            : pos_between         3
// situation  9:       [     ]    [     ]  Y       [     ]  [     ]            : pos_between         2
// situation 10:       [     ]    [     ]          [     ]  [     ]    Y       : pos_behind          4

Finder::CurrentPosInLineInfo Finder::getCurrentPosInLineInfo(intptr_t currentPosInLine, const SearchResultMarkingLine& markingLine) const
{
	CurrentPosInLineInfo cpili;
	size_t count = 0;
	intptr_t lastEnd = 0;
	auto selStart = _scintView.execute(SCI_GETSELECTIONSTART);
	auto selEnd = _scintView.execute(SCI_GETSELECTIONEND);
	bool hasSel = (selEnd - selStart) != 0;

	for (std::pair<intptr_t, intptr_t> range : markingLine._segmentPostions)
	{
		++count;

		if (lastEnd <= currentPosInLine && currentPosInLine < range.first)
		{
			if (count == 1)
			{
				cpili._status = pos_infront;
				break;
			}
			else
			{
				cpili._status = pos_between;
				cpili.auxiliaryInfo = count - 1;
				break;
			}
		}
		
		if (range.first <= currentPosInLine && currentPosInLine <= range.second)
		{
			if (currentPosInLine == range.first && !hasSel)
			{
				cpili._status = pos_between;
				cpili.auxiliaryInfo = count - 1; //  c1      c2
				                                 // [   ]  I[   ]      : I is recongnized with c2, so auxiliaryInfo should be c1 (c2-1)
			}
			else if (currentPosInLine == range.second && !hasSel)
			{
				cpili._status = pos_between;
				cpili.auxiliaryInfo = count;     //  c1      c2
				                                 // [   ]I  [   ]      : I is recongnized with c1, so auxiliaryInfo should be c1
			}
			else
			{
				cpili._status = pos_inside;
				cpili.auxiliaryInfo = count;
			}
			break;
		}

		if (range.second < currentPosInLine)
		{
			if (markingLine._segmentPostions.size() == count)
			{
				cpili._status = pos_behind;
				cpili.auxiliaryInfo = count;
				break;
			}
		}

		lastEnd = range.second;
	}

	return cpili;
}

void Finder::anchorWithNoHeaderLines(intptr_t& currentL, intptr_t initL, intptr_t minL, intptr_t maxL, int direction)
{
	if (currentL > maxL && direction == 0)
		currentL = minL;

	while (_scintView.execute(SCI_GETFOLDLEVEL, currentL) & SC_FOLDLEVELHEADERFLAG)
	{
		currentL += direction == -1 ? -1 : 1;

		if (currentL > maxL)
			currentL = minL;
		else if (currentL < minL)
			currentL = maxL;

		if (currentL == initL)
			break;
	}

	auto extremityAbsoltePos = _scintView.execute(direction == -1 ? SCI_GETLINEENDPOSITION : SCI_POSITIONFROMLINE, currentL);
	_scintView.execute(SCI_SETSEL, extremityAbsoltePos, extremityAbsoltePos);
}

void Finder::gotoNextFoundResult(int direction)
{
	//
	// Get currentLine & currentPosInLine from CurrentPos
	//
	auto currentPos = _scintView.execute(SCI_GETCURRENTPOS);
	intptr_t lno = _scintView.execute(SCI_LINEFROMPOSITION, currentPos);
	auto total_lines = _scintView.execute(SCI_GETLINECOUNT);
	if (total_lines <= 1) return;

	auto lineStartAbsPos = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	intptr_t currentPosInLine = currentPos - lineStartAbsPos;

	auto init_lno = lno;
	auto max_lno = _scintView.execute(SCI_GETLASTCHILD, lno, searchHeaderLevel);

	if (max_lno > total_lines - 2) return;

	// get the line number of the current search (searchHeaderLevel)
	int level = _scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELNUMBERMASK;
	auto min_lno = lno;
	while (level-- >= fileHeaderLevel)
	{
		min_lno = _scintView.execute(SCI_GETFOLDPARENT, min_lno);
		assert(min_lno >= 0);
	}

	if (min_lno < 0)
		min_lno = lno; // when lno is a search header line

	assert(min_lno <= max_lno);

	if (lno > max_lno && direction == 0)
		lno = min_lno;
	else if (lno < min_lno)
		lno = max_lno;

	//
	// Set anchor and make sure that achor is not on the last (empty) line or head lines
	//
	while (_scintView.execute(SCI_GETFOLDLEVEL, lno) & SC_FOLDLEVELHEADERFLAG)
	{
		lno += direction == -1 ? -1 : 1;

		if (lno > max_lno)
			lno = min_lno;
		else if (lno < min_lno)
			lno = max_lno;

		if (lno == init_lno)
			break;
	}

	if (lno != init_lno)
	{
		auto extremityAbsoltePos = _scintView.execute(direction == -1 ? SCI_GETLINEENDPOSITION : SCI_POSITIONFROMLINE, lno);
		_scintView.execute(SCI_SETSEL, extremityAbsoltePos, extremityAbsoltePos);
		currentPos = extremityAbsoltePos;
		auto start = _scintView.execute(SCI_POSITIONFROMLINE, lno);
		currentPosInLine = currentPos - start;
	}


	size_t occurrenceNumberInLine_base1 = 0;
	const SearchResultMarkingLine& markingLine = *(_pMainMarkings->begin() + lno);

	//
	// Determinate currentPosInLine status among pos_infront, pose_between, pos_inside and pos_behind
	//
	CurrentPosInLineInfo cpili  = getCurrentPosInLineInfo(currentPosInLine, markingLine);

	//
	// According CurrentPosInLineInfo and direction, set position and get number of occurrence
	//
	if (direction == 0) // Next
	{
		switch (cpili._status)
		{
			case pos_infront: // 2 situation: 
			{
				if (!markingLine._segmentPostions.size())
					return;

				if (markingLine._segmentPostions[0].second < SC_SEARCHRESULT_LINEBUFFERMAXLENGTH) // 1. The occurrence is displayed in the line
				{
					occurrenceNumberInLine_base1 = 1;
				}
				else // 2. The occurrence is NOT displayed in the line
				{
					lno++;
					anchorWithNoHeaderLines(lno, init_lno, min_lno, max_lno, direction);
					occurrenceNumberInLine_base1 = 1;
				}
			}
			break;

			case pos_between:
			case pos_inside:
			{
				occurrenceNumberInLine_base1 = cpili.auxiliaryInfo + 1;
				if (occurrenceNumberInLine_base1 > markingLine._segmentPostions.size())
				{
					lno++;
					anchorWithNoHeaderLines(lno, init_lno, min_lno, max_lno, direction);
					occurrenceNumberInLine_base1 = 1;
				}
			}
			break;

			case pos_behind:
			{
				lno++;
				anchorWithNoHeaderLines(lno, init_lno, min_lno, max_lno, direction);
				occurrenceNumberInLine_base1 = 1;
			}
			break;
		}
	}
	else if (direction == -1) // Previous
	{
		switch (cpili._status)
		{
			case pos_infront:
			{
				lno--;
				if (lno < 0) return;
				anchorWithNoHeaderLines(lno, init_lno, min_lno, max_lno, direction);
				const SearchResultMarkingLine& newMarkingLine = *(_pMainMarkings->begin() + lno);
				occurrenceNumberInLine_base1 = newMarkingLine._segmentPostions.size();
			}
			break;

			case pos_between:
			{
				occurrenceNumberInLine_base1 = cpili.auxiliaryInfo;
			}
			break;

			case pos_inside:
			{
				if (cpili.auxiliaryInfo > 1)
					occurrenceNumberInLine_base1 = cpili.auxiliaryInfo - 1;
				else
				{
					lno--;
					if (lno < 0) return;
					anchorWithNoHeaderLines(lno, init_lno, min_lno, max_lno, direction);
					const SearchResultMarkingLine& newMarkingLine = *(_pMainMarkings->begin() + lno);
					occurrenceNumberInLine_base1 = newMarkingLine._segmentPostions.size();
				}
			}
			break;

			case pos_behind:
			{
				occurrenceNumberInLine_base1 = cpili.auxiliaryInfo;
			}
			break;
		}
	}
	else // invalid
	{
		return;
	}

	_scintView.execute(SCI_ENSUREVISIBLE, lno);
	std::pair<intptr_t, intptr_t> newPos = gotoFoundLine(occurrenceNumberInLine_base1);

	lineStartAbsPos = _scintView.execute(SCI_POSITIONFROMLINE, lno);
	intptr_t lineEndAbsPos = _scintView.execute(SCI_GETLINEENDPOSITION, lno);

	intptr_t begin = newPos.first + lineStartAbsPos;
	intptr_t end = newPos.second + lineStartAbsPos;

	if (end > lineEndAbsPos)
		end = lineEndAbsPos;

	if (begin < end)
	{
		_scintView.execute(SCI_SETSEL, begin, end);
		_scintView.execute(SCI_SCROLLRANGE, begin, end);
	}
}

void FindInFinderDlg::initFromOptions()
{
	HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT_FIFOLDER);
	addText2Combo(_options._str2Search.c_str(), hFindCombo);

	setChecked(IDC_MATCHLINENUM_CHECK_FIFOLDER, _options._isMatchLineNumber);

	setChecked(IDWHOLEWORD_FIFOLDER, _options._searchType != FindRegex && _options._isWholeWord);
	::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD_FIFOLDER), _options._searchType != FindRegex);

	setChecked(IDMATCHCASE_FIFOLDER, _options._isMatchCase);

	setChecked(IDNORMAL_FIFOLDER, _options._searchType == FindNormal);
	setChecked(IDEXTENDED_FIFOLDER, _options._searchType == FindExtended);
	setChecked(IDREGEXP_FIFOLDER, _options._searchType == FindRegex);

	setChecked(IDREDOTMATCHNL_FIFOLDER, _options._dotMatchesNewline);
	::EnableWindow(::GetDlgItem(_hSelf, IDREDOTMATCHNL_FIFOLDER), _options._searchType == FindRegex);
}

void FindInFinderDlg::writeOptions()
{
	HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT_FIFOLDER);
	_options._str2Search = getTextFromCombo(hFindCombo);
	_options._isMatchLineNumber = isCheckedOrNot(IDC_MATCHLINENUM_CHECK_FIFOLDER);
	_options._isWholeWord = isCheckedOrNot(IDWHOLEWORD_FIFOLDER);
	_options._isMatchCase = isCheckedOrNot(IDMATCHCASE_FIFOLDER);
	_options._searchType = isCheckedOrNot(IDREGEXP_FIFOLDER) ? FindRegex : isCheckedOrNot(IDEXTENDED_FIFOLDER) ? FindExtended : FindNormal;
	_options._dotMatchesNewline = isCheckedOrNot(IDREDOTMATCHNL_FIFOLDER);
}

intptr_t CALLBACK FindInFinderDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			pNativeSpeaker->changeDlgLang(_hSelf, "FindInFinder");

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			initFromOptions();

			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);
		}
		return TRUE;

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc{};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDlgBackgroundBrush());
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				{
					::EndDialog(_hSelf, -1);
					return TRUE;
				}

				case IDOK:
				{
					writeOptions();
					::EndDialog(_hSelf, -1);
					FindersInfo findersInfo;
					findersInfo._pSourceFinder = _pFinder2Search;
					findersInfo._findOption = _options;
					::SendMessage(_hParent, WM_FINDALL_INCURRENTFINDER, reinterpret_cast<WPARAM>(&findersInfo), 0);
					return TRUE;
				}

				case IDNORMAL_FIFOLDER:
				case IDEXTENDED_FIFOLDER:
				case IDREGEXP_FIFOLDER:
				{
					if (isCheckedOrNot(IDREGEXP_FIFOLDER))
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD_FIFOLDER), false);
						setChecked(IDWHOLEWORD_FIFOLDER, false);
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL_FIFOLDER), true);
					}
					else if (isCheckedOrNot(IDEXTENDED_FIFOLDER))
					{
						::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD_FIFOLDER), true);
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL_FIFOLDER), false);
					}
					else
					{
						// normal mode
						::EnableWindow(::GetDlgItem(_hSelf, IDWHOLEWORD_FIFOLDER), true);
						::EnableWindow(GetDlgItem(_hSelf, IDREDOTMATCHNL_FIFOLDER), false);
					}

					return TRUE;
				}
			}
			return FALSE;
		}
		default:
			return FALSE;
	}
	return FALSE;
}


void FindReplaceDlg::resizeDialogElements()
{
	auto getRcWidth = [](const RECT& rc) -> int {
		return rc.right - rc.left;
		};

	auto setOrDeferWindowPos = [](HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) -> HDWP {
		if (hWinPosInfo != nullptr)
		{
			return ::DeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		}
		::SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		return nullptr;
		};

	const bool isLessModeOn = NppParameters::getInstance().getNppGUI()._findWindowLessMode;

	//elements that need to be resized horizontally (all edit/combo boxes etc.)
	const std::vector<int> resizeWindowIDs = { IDFINDWHAT, IDREPLACEWITH, IDD_FINDINFILES_FILTERS_COMBO, IDD_FINDINFILES_DIR_COMBO };
	const size_t nComboboxes = resizeWindowIDs.size();

	//elements that need to be moved
	const auto moveCheckIds = {
		IDD_FINDINFILES_RECURSIVE_CHECK, IDD_FINDINFILES_INHIDDENDIR_CHECK,
		IDD_FINDINFILES_PROJECT1_CHECK, IDD_FINDINFILES_PROJECT2_CHECK, IDD_FINDINFILES_PROJECT3_CHECK,
	};

	const auto moveBtnIDs = {
		IDCMARKALL, IDC_CLEAR_ALL, IDCCOUNTALL, IDC_FINDALL_OPENEDFILES, IDC_FINDALL_CURRENTFILE,
		IDREPLACE, IDREPLACEALL, IDC_REPLACE_OPENEDFILES, IDD_FINDINFILES_FIND_BUTTON, IDD_FINDINFILES_REPLACEINFILES, IDCANCEL,
		IDC_FINDPREV, IDC_COPY_MARKED_TEXT, IDD_FINDINFILES_REPLACEINPROJECTS, IDD_FINDINFILES_SETDIRFROMDOC_BUTTON,
	};

	const auto moveOtherCtrlsIDs = {
		IDC_REPLACEINSELECTION, IDD_RESIZE_TOGGLE_BUTTON, IDD_FINDREPLACE_SWAP_BUTTON
	};

	const auto moveLaterIDs = {
		IDC_FINDPREV, IDD_FINDINFILES_BROWSE_BUTTON
	};

	const auto moveTransIDs = {
		IDC_TRANSPARENT_CHECK, IDC_TRANSPARENT_LOSSFOCUS_RADIO, IDC_TRANSPARENT_ALWAYS_RADIO, IDC_PERCENTAGE_SLIDER
	};

	constexpr UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	RECT rcClient{};
	getClientRect(rcClient);

	RECT rcTmp{};

	RECT rcOkBtn{};
	HWND hOkBtn = ::GetDlgItem(_hSelf, IDOK);
	getMappedChildRect(hOkBtn, rcOkBtn);

	RECT rcSelCheck{};
	HWND hSelCheck = ::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK);
	getMappedChildRect(hSelCheck, rcSelCheck);

	RECT rc2ModeCheck{};
	HWND h2ModeCheck = ::GetDlgItem(_hSelf, IDC_2_BUTTONS_MODE);
	getMappedChildRect(h2ModeCheck, rc2ModeCheck);

	const int gap = rc2ModeCheck.left - rcOkBtn.right; // this value is important, same spacing is used almost everywhere in FindReplaceDlg.rc
	const int posSelCheck = rcOkBtn.left - rcSelCheck.left;

	::SetWindowPos(h2ModeCheck, nullptr, rcClient.right - gap - getRcWidth(rc2ModeCheck), rc2ModeCheck.top, 0, 0, SWP_NOSIZE | flags);
	getMappedChildRect(h2ModeCheck, rc2ModeCheck);

	::SetWindowPos(hOkBtn, nullptr, rc2ModeCheck.left - gap - getRcWidth(rcOkBtn), rcOkBtn.top, 0, 0, SWP_NOSIZE | flags);
	getMappedChildRect(hOkBtn, rcOkBtn);

	::SetWindowPos(hSelCheck, nullptr, rcOkBtn.left - posSelCheck, rcSelCheck.top, 0, 0, SWP_NOSIZE | flags);
	getMappedChildRect(hSelCheck, rcSelCheck);

	size_t nCtrls = moveCheckIds.size() + moveBtnIDs.size() + moveOtherCtrlsIDs.size();
	auto hdwp = ::BeginDeferWindowPos(static_cast<int>(nCtrls));

	RECT rcSelGrpb{};
	HWND hSelGrpb = ::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION);
	getMappedChildRect(hSelGrpb, rcSelGrpb);
	hdwp = setOrDeferWindowPos(hdwp, hSelGrpb, nullptr, rcSelCheck.left - (gap * 3) / 2, rcSelGrpb.top, 0, 0, SWP_NOSIZE | flags);

	for (int moveWndID : moveCheckIds)
	{
		HWND moveHwnd = ::GetDlgItem(_hSelf, moveWndID);
		getMappedChildRect(moveHwnd, rcTmp);
		hdwp = setOrDeferWindowPos(hdwp, moveHwnd, nullptr, rcOkBtn.left + gap / 2, rcTmp.top, 0, 0, SWP_NOSIZE | flags);
	}

	for (int moveWndID : moveBtnIDs)
	{
		HWND moveHwnd = ::GetDlgItem(_hSelf, moveWndID);
		getMappedChildRect(moveHwnd, rcTmp);
		hdwp = setOrDeferWindowPos(hdwp, moveHwnd, nullptr, rcOkBtn.left, rcTmp.top, 0, 0, SWP_NOSIZE | flags);
	}

	RECT rcRszBtn{};
	HWND hRszBtn = ::GetDlgItem(_hSelf, IDD_RESIZE_TOGGLE_BUTTON);
	getMappedChildRect(hRszBtn, rcRszBtn);
	hdwp = setOrDeferWindowPos(hdwp, hRszBtn, nullptr, rc2ModeCheck.left, rcRszBtn.top, 0, 0, SWP_NOSIZE | flags);

	RECT rcSwapBtn{};
	HWND hSwapBtn = ::GetDlgItem(_hSelf, IDD_FINDREPLACE_SWAP_BUTTON);
	getMappedChildRect(hSwapBtn, rcSwapBtn);
	hdwp = setOrDeferWindowPos(hdwp, hSwapBtn, nullptr, rcOkBtn.left - getRcWidth(rcSwapBtn) - gap, rcSwapBtn.top, 0, 0, SWP_NOSIZE | flags);

	if (hdwp)
		::EndDeferWindowPos(hdwp);

	getMappedChildRect(hSwapBtn, rcSwapBtn);

	nCtrls = nComboboxes + moveLaterIDs.size() + (isLessModeOn ? 0 : moveTransIDs.size()) + 1; // 1 is for tab control
	hdwp = ::BeginDeferWindowPos(static_cast<int>(nCtrls));

	std::vector<DWORD> endSelections(nComboboxes, 0);

	for (size_t i = 0; i < nComboboxes; ++i)
	{
		HWND resizeHwnd = ::GetDlgItem(_hSelf, resizeWindowIDs[i]);
		getMappedChildRect(resizeHwnd, rcTmp);

		// Combo box for some reasons selects text on resize. So let's check before resize if selection is present and clear it manually after resize.
		::SendMessage(resizeHwnd, CB_GETEDITSEL, 0, reinterpret_cast<LPARAM>(&endSelections[i]));

		hdwp = setOrDeferWindowPos(hdwp, resizeHwnd, nullptr, 0, 0, rcSwapBtn.left - rcTmp.left - gap, rcTmp.bottom - rcTmp.top, SWP_NOMOVE | flags);
	}

	RECT rcFPrevBtn{};
	HWND hFPrevBtn = ::GetDlgItem(_hSelf, IDC_FINDPREV);
	getMappedChildRect(hFPrevBtn, rcFPrevBtn);
	hdwp = setOrDeferWindowPos(hdwp, ::GetDlgItem(_hSelf, IDC_FINDNEXT), nullptr, rcFPrevBtn.right + gap, rcOkBtn.top, 0, 0, SWP_NOSIZE | flags);

	RECT rcBrowseBtn{};
	HWND hBrowseBtn = ::GetDlgItem(_hSelf, IDD_FINDINFILES_BROWSE_BUTTON);
	getMappedChildRect(hBrowseBtn, rcBrowseBtn);
	hdwp = setOrDeferWindowPos(hdwp, hBrowseBtn, nullptr, rcSwapBtn.left, rcBrowseBtn.top, 0, 0, SWP_NOSIZE | flags);

	if (!isLessModeOn)
	{
		RECT rcTransGrpb{};
		HWND hTransGrpb = ::GetDlgItem(_hSelf, IDC_TRANSPARENT_GRPBOX);
		getMappedChildRect(hTransGrpb, rcTransGrpb);

		RECT rcTransCheck{};
		HWND hTransCheck = ::GetDlgItem(_hSelf, IDC_TRANSPARENT_CHECK);
		getMappedChildRect(hTransCheck, rcTransCheck);

		RECT rcTransLFRadio{};
		HWND hTransLFRadio = ::GetDlgItem(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO);
		getMappedChildRect(hTransLFRadio, rcTransLFRadio);

		RECT rcTransARadio{};
		HWND hTransARadio = ::GetDlgItem(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO);
		getMappedChildRect(hTransARadio, rcTransARadio);

		RECT rcTransSlider{};
		HWND hTransSlider = ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER);
		getMappedChildRect(hTransSlider, rcTransSlider);

		const int gapTrans = rcTransGrpb.left - rcTransCheck.left;
		const int gapTransR = rcTransLFRadio.left - rcTransGrpb.left;
		const int gapTransS = rcTransSlider.left - rcTransGrpb.left;

		::SetWindowPos(hTransGrpb, nullptr, rc2ModeCheck.left - gap - getRcWidth(rcTransGrpb), rcTransGrpb.top, 0, 0, SWP_NOSIZE | flags);
		getMappedChildRect(hTransGrpb, rcTransGrpb);

		hdwp = setOrDeferWindowPos(hdwp, hTransCheck, nullptr, rcTransGrpb.left - gapTrans, rcTransCheck.top, 0, 0, SWP_NOSIZE | flags);
		hdwp = setOrDeferWindowPos(hdwp, hTransLFRadio, nullptr, rcTransGrpb.left + gapTransR, rcTransLFRadio.top, 0, 0, SWP_NOSIZE | flags);
		hdwp = setOrDeferWindowPos(hdwp, hTransARadio, nullptr, rcTransGrpb.left + gapTransR, rcTransARadio.top, 0, 0, SWP_NOSIZE | flags);
		hdwp = setOrDeferWindowPos(hdwp, hTransSlider, nullptr, rcTransGrpb.left + gapTransS, rcTransSlider.top, 0, 0, SWP_NOSIZE | flags);
	}

	auto hTab = _tab.getHSelf();
	::GetClientRect(hTab, &rcTmp);
	hdwp = setOrDeferWindowPos(hdwp, hTab, nullptr, 0, 0, rcClient.right, rcTmp.bottom, SWP_NOMOVE | flags);

	if (hdwp)
		::EndDeferWindowPos(hdwp);

	for (size_t i = 0; i < nComboboxes ; ++i)
	{
		if (endSelections[i] == 0)
		{
			HWND resizeHwnd = ::GetDlgItem(_hSelf, resizeWindowIDs[i]);
			::SendMessage(resizeHwnd, CB_SETEDITSEL, 0, 0);
		}
	}

	::SetWindowPos(::GetDlgItem(_hSelf, IDFINDWHAT), nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED | flags);
}

std::mutex findOps_mutex;

intptr_t CALLBACK FindReplaceDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_GETMINMAXINFO:
		{
			bool isLessModeOn = NppParameters::getInstance().getNppGUI()._findWindowLessMode;
			MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
			mmi->ptMinTrackSize.x = _szMinDialog.cx + _szBorder.cx;
			const LONG h = (isLessModeOn ? _lesssModeHeight : _szMinDialog.cy) + _szBorder.cy;
			mmi->ptMinTrackSize.y = h;
			mmi->ptMaxTrackSize.y = h;

			return TRUE;
		}

		case WM_SIZE:
		{
			resizeDialogElements();
			::SendMessage(_statusBar.getHSelf(), WM_SIZE, 0, 0); // pass WM_SIZE to status bar to automatically adjusts its size
			return TRUE;
		}

		case WM_NCHITTEST:
		{
			LRESULT lrHitTest = ::DefWindowProc(_hSelf, message, wParam, lParam);
			switch (lrHitTest)
			{
				case HTTOP:
				{
					lrHitTest = HTBORDER;
					break;
				}

				case HTTOPLEFT:
				{
					lrHitTest = HTLEFT;
					break;
				}

				case HTTOPRIGHT:
				{
					lrHitTest = HTRIGHT;
					break;
				}

				case HTBOTTOM:
				{
					lrHitTest = HTBORDER;
					break;
				}

				case HTBOTTOMLEFT:
				{
					lrHitTest = HTLEFT;
					break;
				}

				case HTBOTTOMRIGHT:
				{
					lrHitTest = HTRIGHT;
					break;
				}

				default:
					return FALSE;
			}

			::SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, lrHitTest);
			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc{};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDlgBackgroundBrush());
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::setDarkTooltips(_shiftTrickUpTip, NppDarkMode::ToolTipsType::tooltip);
			NppDarkMode::setDarkTooltips(_2ButtonsTip, NppDarkMode::ToolTipsType::tooltip);
			NppDarkMode::setDarkTooltips(_filterTip, NppDarkMode::ToolTipsType::tooltip);
			NppDarkMode::setDarkTooltips(_dirFromActiveDocTip, NppDarkMode::ToolTipsType::tooltip);
			
			if (_statusbarTooltipWnd)
			{
				NppDarkMode::setDarkTooltips(_statusbarTooltipWnd, NppDarkMode::ToolTipsType::tooltip);
			}

			HWND finder = getHFindResults();
			if (finder)
			{
				NppDarkMode::setDarkScrollBar(finder);
			}

			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_INITDIALOG :
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
			HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
			HWND hFiltersCombo = ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO);
			HWND hDirCombo = ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO);

			// Change handler of edit element in the comboboxes to support Ctrl+Backspace
			COMBOBOXINFO cbinfo{};
			cbinfo.cbSize = sizeof(COMBOBOXINFO);
			GetComboBoxInfo(hFindCombo, &cbinfo);
			if (!cbinfo.hwndItem) return FALSE;

			originalComboEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(comboEditProc)));
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cbinfo.hwndCombo));
			GetComboBoxInfo(hReplaceCombo, &cbinfo);
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(comboEditProc));
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cbinfo.hwndCombo));
			GetComboBoxInfo(hFiltersCombo, &cbinfo);
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(comboEditProc));
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cbinfo.hwndCombo));
			GetComboBoxInfo(hDirCombo, &cbinfo);
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(comboEditProc));
			SetWindowLongPtr(cbinfo.hwndItem, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cbinfo.hwndCombo));

			setDpi();

			HFONT hFont = nullptr;
			const bool isMonospaced = NppParameters::getInstance().getNppGUI()._monospacedFontFindDlg;
			if (isMonospaced)
			{
				hFont = createFont(L"Courier New", 8, false, _hSelf);
			}
			else
			{
				hFont = reinterpret_cast<HFONT>(::SendMessage(hFindCombo, WM_GETFONT, 0, 0));
			}
			
			// Change ComboBox height to accomodate High-DPI settings.
			// ComboBoxes are scaled using the font used in them, however this results in weird optics
			// on scaling > 200% (192 DPI). Using this method we accomodate these scalings way better
			// than the OS does with the current dpiAware.manifest...

			LOGFONT lf{};
			::GetObject(hFont, sizeof(lf), &lf);
			lf.lfHeight = -(_dpiManager.scale(16) - 5);
			_hComboBoxFont = ::CreateFontIndirect(&lf);

			for (const auto& hComboBox : { hFindCombo, hReplaceCombo, hFiltersCombo, hDirCombo })
			{
				::SendMessage(hComboBox, WM_SETFONT, reinterpret_cast<WPARAM>(_hComboBoxFont), MAKELPARAM(TRUE, 0));
			}

			if (isMonospaced && hFont != nullptr)
				::DeleteObject(hFont);

			 NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

			 wstring searchButtonTip = pNativeSpeaker->getLocalizedStrFromID("shift-change-direction-tip", L"Use Shift+Enter to search in the opposite direction.");
			 _shiftTrickUpTip = CreateToolTip(IDOK, _hSelf, _hInst, const_cast<PTSTR>(searchButtonTip.c_str()), _isRTL);

			 wstring checkboxTip = pNativeSpeaker->getLocalizedStrFromID("two-find-buttons-tip", L"2 find buttons mode");
			 _2ButtonsTip = CreateToolTip(IDC_2_BUTTONS_MODE, _hSelf, _hInst, const_cast<PTSTR>(checkboxTip.c_str()), _isRTL);

			 wstring findInFilesFilterTip = pNativeSpeaker->getLocalizedStrFromID("find-in-files-filter-tip", L"Find in cpp, cxx, h, hxx && hpp:\r*.cpp *.cxx *.h *.hxx *.hpp\r\rFind in all files except exe, obj && log:\r*.* !*.exe !*.obj !*.log\r\rFind in all files but exclude folders tests, bin && bin64:\r*.* !\\tests !\\bin*\r\rFind in all files but exclude all folders log or logs recursively:\r*.* !+\\log*");
			 _filterTip = CreateToolTip(IDD_FINDINFILES_FILTERS_STATIC, _hSelf, _hInst, const_cast<PTSTR>(findInFilesFilterTip.c_str()), _isRTL);

			 wstring dirFromActiveDocTip = pNativeSpeaker->getLocalizedStrFromID("find-in-files-dir-from-active-doc-tip", L"Fill directory field based on active document");
			 _dirFromActiveDocTip = CreateToolTip(IDD_FINDINFILES_SETDIRFROMDOC_BUTTON, _hSelf, _hInst, const_cast<PTSTR>(dirFromActiveDocTip.c_str()), _isRTL);

			::SetWindowTextW(::GetDlgItem(_hSelf, IDC_FINDPREV), L"▲");
			::SetWindowTextW(::GetDlgItem(_hSelf, IDC_FINDNEXT), L"▼ Find Next");

			_hSwapButton = ::GetDlgItem(_hSelf, IDD_FINDREPLACE_SWAP_BUTTON);
			::SetWindowLongPtr(_hSwapButton, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			_oldSwapButtonProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSwapButton, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(swapButtonProc)));
			::SetWindowTextW(_hSwapButton, L"⇅");

			::SetWindowTextW(::GetDlgItem(_hSelf, IDD_RESIZE_TOGGLE_BUTTON), L"˄");

			// "⇅" enlargement
			_hLargerBolderFont = createFont(L"Courier New", 14, true, _hSelf);
			::SendMessage(_hSwapButton, WM_SETFONT, reinterpret_cast<WPARAM>(_hLargerBolderFont), MAKELPARAM(TRUE, 0));

			// Make "˄" & "˅" look better
			_hCourrierNewFont = createFont(L"Courier New", 12, false, _hSelf);
			::SendDlgItemMessage(_hSelf, IDD_RESIZE_TOGGLE_BUTTON, WM_SETFONT, reinterpret_cast<WPARAM>(_hCourrierNewFont), MAKELPARAM(TRUE, 0));

			return TRUE;
		}

		case WM_DRAWITEM :
		{
			drawItem((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}

		case WM_HSCROLL :
		{
			if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(_hSelf, IDC_PERCENTAGE_SLIDER))
			{
				int percent = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
				FindHistory & findHistory = (NppParameters::getInstance()).getFindHistory();
				findHistory._transparency = percent;
				if (isCheckedOrNot(IDC_TRANSPARENT_ALWAYS_RADIO))
				{
					(NppParameters::getInstance()).SetTransparent(_hSelf, percent);
				}
			}
			return TRUE;
		}

		case WM_NOTIFY:
		{
			auto lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
			switch (lpnmhdr->code)
			{
				case TCN_SELCHANGE:
				{
					const HWND tabHandle = _tab.getHSelf();
					if (lpnmhdr->hwndFrom == tabHandle)
					{
						const auto indexClicked = static_cast<int>(::SendMessage(tabHandle, TCM_GETCURSEL, 0, 0));
						doDialog(static_cast<DIALOG_TYPE>(indexClicked));
						return TRUE;
					}
					break;
				}

				case BCN_DROPDOWN:
				{
					if (lpnmhdr->hwndFrom == _hSwapButton)
					{
						if (!_swapPopupMenu.isCreated())
						{
							vector<MenuItemUnit> itemUnitArray;
							itemUnitArray.push_back(MenuItemUnit(IDC_SWAP_FIND_REPLACE, L"⇅ Swap Find with Replace"));
							itemUnitArray.push_back(MenuItemUnit(IDC_COPY_FIND2REPLACE, L"⤵ Copy from Find to Replace"));
							itemUnitArray.push_back(MenuItemUnit(IDC_COPY_REPLACE2FIND, L"⤴ Copy from Replace to Find"));

							NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
							for (auto&& i : itemUnitArray)
							{
								i._itemName = pNativeSpeaker->getDlgLangMenuStr("Dialog", "Find", i._cmdID, i._itemName.c_str());
							}

							_swapPopupMenu.create(_hSelf, itemUnitArray);
						}

						RECT rc{};
						::GetClientRect(_hSwapButton, &rc);
						POINT p{};
						::ClientToScreen(_hSwapButton, &p);
						p.y += rc.bottom;
						_swapPopupMenu.display(p);

						return TRUE;
					}
					break;
				}
			}
			return FALSE;
		}

		case WM_ACTIVATE :
		{
			bool isInSelectionAutoChange = false;

			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				Sci_CharacterRangeFull cr = (*_ppEditView)->getSelection();
				intptr_t nbSelected = (*_ppEditView)->execute(SCI_COUNTCHARACTERS, cr.cpMin, cr.cpMax);

				bool inSelEnabled = nbSelected != 0;

				// Searching/replacing in multiple selections or column selection is not allowed
				if (((*_ppEditView)->execute(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE) ||
					((*_ppEditView)->execute(SCI_GETSELECTIONS) > 1))
				{
					inSelEnabled = false;
				}

				enableFindDlgItem(IDC_IN_SELECTION_CHECK, inSelEnabled);

				bool inSelChecked = isCheckedOrNot(IDC_IN_SELECTION_CHECK);
				bool origInSelChecked = inSelChecked;

				const NppGUI& nppGui = (NppParameters::getInstance()).getNppGUI();
				if (nppGui._inSelectionAutocheckThreshold != 0)
				{
					// code is allowed to change checkmark status of In-selection checkbox

					inSelChecked = inSelEnabled && (nbSelected >= nppGui._inSelectionAutocheckThreshold);

					setChecked(IDC_IN_SELECTION_CHECK, inSelChecked);
				}

				/*
				In the scenario where the user clicks the action button (Count, 
				Find All in Current Document, Replace All, Mark All, or Clear All marks) 
				without activating the Find/Replace dialog, the "In Selection" checkbox could 
				be auto-changed after the button click. To prevent the search from running with 
				this unintended state, the search message has been removed from the queue. 
				Then, launch a message box to alert the user that the search didn't run and 
				they need to verify the settings.
				*/
				if (inSelChecked != origInSelChecked)
				{
					const std::vector<int> inSelActionIds = { IDCCOUNTALL, IDC_FINDALL_CURRENTFILE, IDREPLACEALL, IDCMARKALL, IDC_CLEAR_ALL };
					for (const auto& id : inSelActionIds)
					{
						MSG msg;
						if (PeekMessage(&msg, ::GetDlgItem(_hSelf, id), 0, 0, PM_REMOVE))
						{
							isInSelectionAutoChange = true;
							break;
						}
					}
				}

				_options._isInSelection = inSelEnabled && inSelChecked;
			}

			if (isCheckedOrNot(IDC_TRANSPARENT_LOSSFOCUS_RADIO))
			{
				if (LOWORD(wParam) == WA_INACTIVE && isVisible())
				{
					int percent = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
					(NppParameters::getInstance()).SetTransparent(_hSelf, percent);
				}
				else
				{
					(NppParameters::getInstance()).removeTransparent(_hSelf);
				}
			}

			// At very first time (when find dlg is launched), search mode is Normal.
			// In that case, ". Matches newline" should be disabled as it applicable only for Regex
			if (isCheckedOrNot(IDREGEXP))
			{
				enableFindDlgItem(IDREDOTMATCHNL);
			}
			else
			{
				enableFindDlgItem(IDREDOTMATCHNL, false);
			}

			enableProjectCheckmarks();

			if (isInSelectionAutoChange)
			{
				NppParameters& nppParamInst = NppParameters::getInstance();
				(nppParamInst.getNativeLangSpeaker())->messageBox(
					"FindAutoChangeOfInSelectionWarning",
					_hSelf,
					L"The \"In selection\" checkbox state has been automatically modified.\r\n"
					L"Please verify the search condition before performing the action.",
					L"Search warning",
					MB_OK | MB_APPLMODAL);
			}

			return 0;
		}

		case NPPM_MODELESSDIALOG :
			return ::SendMessage(_hParent, NPPM_MODELESSDIALOG, wParam, lParam);

		case WM_GETDPISCALEDSIZE:
		{
			auto newSize = reinterpret_cast<SIZE*>(lParam);

			RECT rcClient{};
			getClientRect(rcClient);

			const UINT newDpi = static_cast<UINT>(wParam);
			const UINT prevDpi = _dpiManager.getDpi();

			const bool isLessModeOn = NppParameters::getInstance().getNppGUI()._findWindowLessMode;

			RECT rcStatusBar{};
			::GetWindowRect(_statusBar.getHSelf(), &rcStatusBar);

			rcClient.right = _dpiManager.scale(rcClient.right - rcClient.left, newDpi, prevDpi);
			rcClient.bottom = _dpiManager.scale((isLessModeOn ? _lesssModeHeight : _szMinDialog.cy) + (rcStatusBar.bottom - rcStatusBar.top), newDpi, prevDpi);

			LONG xBorder = 0;
			LONG yBorder = 0;

			const auto style = static_cast<DWORD>(::GetWindowLongPtr(_hSelf, GWL_STYLE));
			const auto exStyle = static_cast<DWORD>(::GetWindowLongPtr(_hSelf, GWL_EXSTYLE));
			if (_dpiManager.adjustWindowRectExForDpi(&rcClient, style, FALSE, exStyle, newDpi) == FALSE)
			{
				const LONG padding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER, newDpi);
				xBorder = (_dpiManager.getSystemMetricsForDpi(SM_CXFRAME, newDpi) + padding) * 2;
				yBorder = (_dpiManager.getSystemMetricsForDpi(SM_CYFRAME, newDpi) + padding) * 2 + _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION, newDpi);
			}

			newSize->cx = (rcClient.right - rcClient.left) + xBorder;
			newSize->cy = (rcClient.bottom - rcClient.top) + yBorder;

			if (prevDpi > newDpi)
			{
				const auto padding = static_cast<LONG>(_dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER, newDpi));
				newSize->cx += padding;
				newSize->cy += padding;
			}

			return TRUE;
		}

		case WM_DPICHANGED:
		{
			const UINT prevDpi = _dpiManager.getDpi();
			_dpiManager.setDpiWP(wParam);

			if (_hLargerBolderFont)
				::DeleteObject(_hLargerBolderFont);

			if (_hCourrierNewFont)
				::DeleteObject(_hCourrierNewFont);

			if (_hComboBoxFont)
				::DeleteObject(_hComboBoxFont);

			_hLargerBolderFont = createFont(L"Courier New", 14, true, _hSelf);
			::SendMessage(_hSwapButton, WM_SETFONT, reinterpret_cast<WPARAM>(_hLargerBolderFont), MAKELPARAM(TRUE, 0));

			_hCourrierNewFont = createFont(L"Courier New", 12, false, _hSelf);
			::SendDlgItemMessage(_hSelf, IDD_RESIZE_TOGGLE_BUTTON, WM_SETFONT, reinterpret_cast<WPARAM>(_hCourrierNewFont), MAKELPARAM(TRUE, 0));

			LOGFONT lf{};
			HFONT font = reinterpret_cast<HFONT>(::SendDlgItemMessage(_hSelf, IDFINDWHAT, WM_GETFONT, 0, 0));
			::GetObject(font, sizeof(lf), &lf);
			lf.lfHeight = -(_dpiManager.scale(16) - 5);
			_hComboBoxFont = ::CreateFontIndirect(&lf);

			for (auto idComboBox : { IDFINDWHAT, IDREPLACEWITH, IDD_FINDINFILES_FILTERS_COMBO, IDD_FINDINFILES_DIR_COMBO })
			{
				::SendDlgItemMessage(_hSelf, idComboBox, WM_SETFONT, reinterpret_cast<WPARAM>(_hComboBoxFont), MAKELPARAM(TRUE, 0));
			}

			RECT rcStatusBar{};
			::GetWindowRect(_statusBar.getHSelf(), &rcStatusBar);

			LONG padding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER);
			_szBorder.cx = ((_dpiManager.getSystemMetricsForDpi(SM_CXFRAME) + padding) * 2);
			_szBorder.cy = ((_dpiManager.getSystemMetricsForDpi(SM_CYFRAME) + padding) * 2
				+ _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION)
				+ (rcStatusBar.bottom - rcStatusBar.top));

			if (prevDpi > _dpiManager.getDpi())
			{
				padding = static_cast<LONG>(_dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER));
				_szBorder.cx += padding;
				_szBorder.cy += padding;
			}

			const UINT dpi = _dpiManager.getDpi();
			_szMinDialog.cx = _dpiManager.scale(_szMinDialog.cx, dpi, prevDpi);
			_szMinDialog.cy = _dpiManager.scale(_szMinDialog.cy, dpi, prevDpi);
			_lesssModeHeight = _dpiManager.scale(_lesssModeHeight, dpi, prevDpi);

			setPositionDpi(lParam, SWP_NOZORDER | SWP_NOACTIVATE);

			_rc.left = 0;
			_rc.top = 0;
			_rc.right = _szMinDialog.cx + _szBorder.cx;
			_rc.bottom = _szMinDialog.cy + _szBorder.cy;

			return TRUE;
		}


		case WM_COMMAND:
		{
			bool isMacroRecording = (static_cast<MacroStatus>(::SendMessage(_hParent, NPPM_GETCURRENTMACROSTATUS,0,0)) == MacroStatus::RecordInProgress);
			NppParameters& nppParamInst = NppParameters::getInstance();
			FindHistory & findHistory = nppParamInst.getFindHistory();
			switch (LOWORD(wParam))
			{
//Single actions
				case IDC_2_BUTTONS_MODE:
				{
					bool is2ButtonsMode = isCheckedOrNot(IDC_2_BUTTONS_MODE);
					findHistory._isSearch2ButtonsMode = is2ButtonsMode;

					showFindDlgItem(IDC_FINDPREV, is2ButtonsMode);
					showFindDlgItem(IDC_FINDNEXT, is2ButtonsMode);
					showFindDlgItem(IDOK, !is2ButtonsMode);
				}
				break;

				case IDCANCEL:
					(*_ppEditView)->execute(SCI_CALLTIPCANCEL);
					setStatusbarMessage(wstring(), FSNoMessage);
					display(false);
					break;

				case IDM_SEARCH_FINDNEXT:
				case IDM_SEARCH_FINDPREV:
				{
					if (HIWORD(wParam) != 1 ||
						(getCurrentStatus() != DIALOG_TYPE::FIND_DLG &&
						getCurrentStatus() != DIALOG_TYPE::REPLACE_DLG)
						|| (LOWORD(wParam) == IDM_SEARCH_FINDPREV &&
							(_options._searchType == FindRegex) &&
							!nppParamInst.regexBackward4PowerUser()))
					{
						return FALSE;
					}
					[[fallthrough]];
				}
				case IDC_FINDPREV:
				case IDC_FINDNEXT:
				case IDOK : // Find Next : only for FIND_DLG and REPLACE_DLG
				{
					setStatusbarMessage(wstring(), FSNoMessage);

					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo);
					updateCombo(IDFINDWHAT);

					nppParamInst._isFindReplacing = true;

					bool direction_bak = _options._whichDirection;

					if (LOWORD(wParam) == IDC_FINDPREV || LOWORD(wParam) == IDM_SEARCH_FINDPREV)
					{
						_options._whichDirection = DIR_UP;
					}
					else if (LOWORD(wParam) == IDC_FINDNEXT || LOWORD(wParam) == IDM_SEARCH_FINDNEXT)
					{
						_options._whichDirection = DIR_DOWN;
					}
					else // IDOK
					{
						// if shift-key is pressed, revert search direction
						// if shift-key is not pressed, use the normal setting
						SHORT shift = GetKeyState(VK_SHIFT);
						if (shift & SHIFTED)
						{
							_options._whichDirection = !_options._whichDirection;
						}
					}

					if ((_options._whichDirection == DIR_UP) && (_options._searchType == FindRegex) && !nppParamInst.regexBackward4PowerUser())
					{
						// this can only happen when shift-key was pressed
						// regex upward search is disabled
						// turn user action into a no-action step

						regexBackwardMsgBox();
					}
					else
					{
						if (isMacroRecording)
							saveInMacro(IDOK, FR_OP_FIND);

						FindStatus findStatus = FSFound;
						processFindNext(_options._str2Search.c_str(), _env, &findStatus);

						NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
						if (findStatus == FSEndReached)
						{
							wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-end-reached", FIND_STATUS_END_REACHED_TEXT);
							setStatusbarMessage(msg, FSEndReached);
						}
						else if (findStatus == FSTopReached)
						{
							wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-top-reached", FIND_STATUS_TOP_REACHED_TEXT);
							setStatusbarMessage(msg, FSTopReached);
						}
					}

					// restore search direction which may have been overwritten because shift-key was pressed
					_options._whichDirection = direction_bak;

					nppParamInst._isFindReplacing = false;
				}
				return TRUE;
					
				case IDC_SWAP_FIND_REPLACE:
				{
					if (_swapButtonStatus != swap)
					{
						_swapButtonStatus = swap;
						::SetWindowTextW(_hSwapButton, L"⇅");
						SendMessage(_hSwapButton, WM_SETFONT, (WPARAM)_hLargerBolderFont, MAKELPARAM(true, 0));
					}
					::SendMessage(_hSelf, WM_COMMAND, IDD_FINDREPLACE_SWAP_BUTTON, 0);
					return TRUE;
				}

				case IDC_COPY_FIND2REPLACE:
				{
					if (_swapButtonStatus != down)
					{
						_swapButtonStatus = down;
						::SetWindowTextW(_hSwapButton, L"⤵");
						SendMessage(_hSwapButton, WM_SETFONT, (WPARAM)_hLargerBolderFont, MAKELPARAM(true, 0));
					}
					::SendMessage(_hSelf, WM_COMMAND, IDD_FINDREPLACE_SWAP_BUTTON, 0);
					return TRUE;
				}

				case IDC_COPY_REPLACE2FIND:
				{
					if (_swapButtonStatus != up)
					{
						_swapButtonStatus = up;
						::SetWindowTextW(_hSwapButton, L"⤴");
						SendMessage(_hSwapButton, WM_SETFONT, (WPARAM)_hLargerBolderFont, MAKELPARAM(true, 0));
					}
					::SendMessage(_hSelf, WM_COMMAND, IDD_FINDREPLACE_SWAP_BUTTON, 0);
					return TRUE;
				}

				case IDD_FINDREPLACE_SWAP_BUTTON:
				{
					HWND hFindWhat = ::GetDlgItem(_hSelf, IDFINDWHAT);
					wstring findWhatText = getTextFromCombo(hFindWhat);
					HWND hPlaceWith = ::GetDlgItem(_hSelf, IDREPLACEWITH);
					wstring replaceWithText = getTextFromCombo(hPlaceWith);

					
					if (_swapButtonStatus == swap)
					{
						if ((!findWhatText.empty() || !replaceWithText.empty()) && (findWhatText != replaceWithText))
						{
							::SendMessage(hFindWhat, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(replaceWithText.c_str()));
							::SendMessage(hPlaceWith, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(findWhatText.c_str()));
						}
					}
					else if (_swapButtonStatus == down)
					{
						if (!findWhatText.empty() && (findWhatText != replaceWithText))
						{
							::SendMessage(hPlaceWith, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(findWhatText.c_str()));
						}
					}
					else // if (_swapButtonStatus == up)
					{
						if (!replaceWithText.empty() && (findWhatText != replaceWithText))
						{
							::SendMessage(hFindWhat, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(replaceWithText.c_str()));
						}
					}
				}
				return TRUE;

				case IDM_SEARCH_FIND:
					if (_currentStatus == FIND_DLG)
					{
						moveForDpiChange();
						goToCenter();
					}
					else
						enableReplaceFunc(false);
					return TRUE;

				case IDM_SEARCH_REPLACE:
					if (_currentStatus == REPLACE_DLG)
					{
						moveForDpiChange();
						goToCenter();
					}
					else
						enableReplaceFunc(true);
					return TRUE;

				case IDM_SEARCH_FINDINFILES:
					if (_currentStatus == FINDINFILES_DLG)
					{
						moveForDpiChange();
						goToCenter();
					}
					else
						enableFindInFilesFunc();
					return TRUE;

				case IDM_SEARCH_MARK:
					if (_currentStatus == MARK_DLG)
					{
						moveForDpiChange();
						goToCenter();
					}
					else
						enableMarkFunc();
					return TRUE;

				case IDC_NEXT_TAB:
				case IDC_PREV_TAB:
				{
					const int selTabIdx = _tab.getNextOrPrevTabIdx(LOWORD(wParam) == IDC_NEXT_TAB);

					switch (static_cast<DIALOG_TYPE>(selTabIdx))
					{
						case DIALOG_TYPE::FIND_DLG:
						{
							enableReplaceFunc(false);
							break;
						}

						case DIALOG_TYPE::REPLACE_DLG:
						{
							enableReplaceFunc(true);
							break;
						}

						case DIALOG_TYPE::FINDINFILES_DLG:
						{
							enableFindInFilesFunc();
							break;
						}

						case DIALOG_TYPE::FINDINPROJECTS_DLG:
						{
							enableFindInProjectsFunc();
							break;
						}

						case DIALOG_TYPE::MARK_DLG:
						{
							enableMarkFunc();
							break;
						}
					}

					return TRUE;
				}

				case IDREPLACE:
				{
					std::lock_guard<std::mutex> lock(findOps_mutex);

					if (_currentStatus == REPLACE_DLG)
					{
						setStatusbarMessage(L"", FSNoMessage);
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str2Search = getTextFromCombo(hFindCombo);
						_options._str4Replace = getTextFromCombo(hReplaceCombo);
						updateCombos();

						nppParamInst._isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE);
						processReplace(_options._str2Search.c_str(), _options._str4Replace.c_str());
						nppParamInst._isFindReplacing = false;
					}
				}
				return TRUE;
//Process actions
				case IDC_FINDALL_OPENEDFILES :
				{
					if (_currentStatus == FIND_DLG)
					{
						setStatusbarMessage(L"", FSNoMessage);
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						combo2ExtendedMode(IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo);
						updateCombo(IDFINDWHAT);

						nppParamInst._isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_GLOBAL);
						findAllIn(ALL_OPEN_DOCS);
						nppParamInst._isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_FINDALL_CURRENTFILE :
				{
					setStatusbarMessage(L"", FSNoMessage);
					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
					combo2ExtendedMode(IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo);
					updateCombo(IDFINDWHAT);

					nppParamInst._isFindReplacing = true;
					if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_GLOBAL);
					findAllIn(_options._isInSelection ? CURR_DOC_SELECTION : CURRENT_DOC);
					nppParamInst._isFindReplacing = false;
				}
				return TRUE;

				case IDD_FINDINFILES_FIND_BUTTON:
				{
					setStatusbarMessage(L"", FSNoMessage);
					constexpr int filterSize = 512;
					wchar_t filters[filterSize + 1] = { '\0' };
					filters[filterSize] = '\0';
					wchar_t directory[MAX_PATH]{};
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_options._filters = filters;

					HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
					combo2ExtendedMode(IDFINDWHAT);
					_options._str2Search = getTextFromCombo(hFindCombo);
					updateCombo(IDFINDWHAT);

					if (_currentStatus == FINDINFILES_DLG)
					{
						::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
						_options._directory = directory;
						trim(_options._directory);
						if (!_options._directory.empty())
						{
							addText2Combo(_options._directory.c_str(), ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
							if (_options._directory.back() != L'\\')
							{
								_options._directory += L"\\";
							}
							nppParamInst._isFindReplacing = true;
							if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND + FR_OP_FIF);
							findAllIn(FILES_IN_DIR);
							nppParamInst._isFindReplacing = false;
						}
						else
						{
							// move input focus to "Directory:" edit control
							::SendMessage(_hSelf, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO)), TRUE);
						}
					}
					else if (_currentStatus == FINDINPROJECTS_DLG)
					{
						if (_options._isProjectPanel_1 || _options._isProjectPanel_2 || _options._isProjectPanel_3)
						{
							nppParamInst._isFindReplacing = true;
							if (isMacroRecording) saveInMacro(IDD_FINDINFILES_FINDINPROJECTS, FR_OP_FIND + FR_OP_FIP);
							findAllIn(FILES_IN_PROJECTS);
							nppParamInst._isFindReplacing = false;
						}
					}
				}
				return TRUE;

				case IDD_FINDINFILES_REPLACEINFILES:
				{
					std::lock_guard<std::mutex> lock(findOps_mutex);

					setStatusbarMessage(L"", FSNoMessage);
					constexpr int filterSize = 512;
					wchar_t filters[filterSize]{};
					wchar_t directory[MAX_PATH]{};
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_options._filters = filters;

					::GetDlgItemText(_hSelf, IDD_FINDINFILES_DIR_COMBO, directory, MAX_PATH);
					_options._directory = directory;
					trim(_options._directory);
					if (!_options._directory.empty())
					{
						if (replaceInFilesConfirmCheck(_options._directory, _options._filters))
						{
							addText2Combo(_options._directory.c_str(), ::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO));
							if (_options._directory.back() != L'\\')
							{
								_options._directory += L"\\";
							}

							HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
							_options._str2Search = getTextFromCombo(hFindCombo);
							HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
							_options._str4Replace = getTextFromCombo(hReplaceCombo);
							updateCombo(IDFINDWHAT);
							updateCombo(IDREPLACEWITH);

							nppParamInst._isFindReplacing = true;
							if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE + FR_OP_FIF);
							::SendMessage(_hParent, WM_REPLACEINFILES, 0, 0);
							nppParamInst._isFindReplacing = false;
						}
					}
					else
					{
						// move input focus to "Directory:" edit control
						::SendMessage(_hSelf, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO)), TRUE);
					}
				}
				return TRUE;

				case IDD_FINDINFILES_REPLACEINPROJECTS:
				{
					std::lock_guard<std::mutex> lock(findOps_mutex);

					setStatusbarMessage(L"", FSNoMessage);
					constexpr int filterSize = 512;
					wchar_t filters[filterSize]{};
					::GetDlgItemText(_hSelf, IDD_FINDINFILES_FILTERS_COMBO, filters, filterSize);
					addText2Combo(filters, ::GetDlgItem(_hSelf, IDD_FINDINFILES_FILTERS_COMBO));
					_options._filters = filters;
					if (replaceInProjectsConfirmCheck())
					{
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str4Replace = getTextFromCombo(hReplaceCombo);
						updateCombo(IDFINDWHAT);
						updateCombo(IDREPLACEWITH);

						nppParamInst._isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE + FR_OP_FIP);
						::SendMessage(_hParent, WM_REPLACEINPROJECTS, 0, 0);
						nppParamInst._isFindReplacing = false;
					}
				}
				return TRUE;

				case IDC_REPLACE_OPENEDFILES :
				{
					std::lock_guard<std::mutex> lock(findOps_mutex);

					if (_currentStatus == REPLACE_DLG)
					{
						NppParameters& nppParam = NppParameters::getInstance();
						const NppGUI& nppGui = nppParam.getNppGUI();
						if (!nppGui._confirmReplaceInAllOpenDocs || replaceInOpenDocsConfirmCheck())
						{
							setStatusbarMessage(L"", FSNoMessage);
							HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
							_options._str2Search = getTextFromCombo(hFindCombo);
							HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
							_options._str4Replace = getTextFromCombo(hReplaceCombo);
							updateCombos();

							nppParamInst._isFindReplacing = true;
							if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE + FR_OP_GLOBAL);
							replaceAllInOpenedDocs();
							nppParamInst._isFindReplacing = false;
						}
					}
				}
				return TRUE;

				case IDREPLACEALL :
				{
					std::lock_guard<std::mutex> lock(findOps_mutex);

					if (_currentStatus == REPLACE_DLG)
					{
						setStatusbarMessage(L"", FSNoMessage);
						if ((*_ppEditView)->getCurrentBuffer()->isReadOnly())
						{
							NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
							wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replace-readonly", L"Replace: Cannot replace text. The current document is read only.");
							setStatusbarMessage(msg, FSNotFound);
							return TRUE;
						}

						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo);
						HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
						_options._str4Replace = getTextFromCombo(hReplaceCombo);
						updateCombos();

						nppParamInst._isFindReplacing = true;
						if (isMacroRecording) saveInMacro(wParam, FR_OP_REPLACE);
						(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
						int nbReplaced = processAll(ProcessReplaceAll, &_options);
						(*_ppEditView)->execute(SCI_ENDUNDOACTION);
						nppParamInst._isFindReplacing = false;

						
						NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
						if (nbReplaced == FIND_INVALID_REGULAR_EXPRESSION)
						{
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							wstring result;
							if (nbReplaced == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-replaceall-1-replaced", L"Replace All: 1 occurrence was replaced");
							}
							else
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-replaceall-nb-replaced", L"Replace All: $INT_REPLACE$ occurrences were replaced");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbReplaced));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(&_options);

							wstring reasonMsg;
							bool isTheMostLaxMode = _options._isWrapAround && !_options._isMatchCase && !_options._isWholeWord;
							if (nbReplaced == 0 && !isTheMostLaxMode)
							{
								reasonMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find-pebkac-maybe", noFoundPotentialReason);
							}

							setStatusbarMessage(result, FSMessage, reasonMsg);
						}
						grabFocus();
					}
				}
				return TRUE;

				case IDCCOUNTALL :
				{
					if (_currentStatus == FIND_DLG)
					{
						setStatusbarMessage(L"", FSNoMessage);
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						updateCombo(IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo);

						int nbCounted = processAll(ProcessCountAll, &_options);

						NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
						if (nbCounted == FIND_INVALID_REGULAR_EXPRESSION)
						{
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							wstring result;
							if (nbCounted == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-count-1-match", L"Count: 1 match");
							}
							else //if (nbCounted == 0 || nbCounted > 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-count-nb-matches", L"Count: $INT_REPLACE$ matches");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbCounted));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(&_options);

							wstring reasonMsg;
							bool isTheMostLaxMode = _options._isWrapAround && !_options._isMatchCase && !_options._isWholeWord;
							if (nbCounted == 0 && !isTheMostLaxMode)
							{
								reasonMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find-pebkac-maybe", noFoundPotentialReason);
							}

							setStatusbarMessage(result, FSMessage, reasonMsg);
						}

						if (isMacroRecording)
							saveInMacro(wParam, FR_OP_FIND);
						
						grabFocus();
					}
				}
				return TRUE;

				case IDCMARKALL :
				{
					if (_currentStatus == MARK_DLG)
					{
						setStatusbarMessage(L"", FSNoMessage);
						HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
						_options._str2Search = getTextFromCombo(hFindCombo);
						updateCombo(IDFINDWHAT);

						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND);
						nppParamInst._isFindReplacing = true;
						int nbMarked = processAll(ProcessMarkAll, &_options);
						nppParamInst._isFindReplacing = false;

						
						NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
						if (nbMarked == FIND_INVALID_REGULAR_EXPRESSION)
						{
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							std::wstring result;
							if (nbMarked == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-mark-1-match", L"Mark: 1 match");
							}
							else //if (nbMarked == 0 || nbMarked > 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-mark-nb-matches", L"Mark: $INT_REPLACE$ matches");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbMarked));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(&_options);
							
							wstring reasonMsg;
							bool isTheMostLaxMode = _options._isWrapAround && !_options._isMatchCase && !_options._isWholeWord;
							if (nbMarked == 0 && !isTheMostLaxMode)
							{
								reasonMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find-pebkac-maybe", noFoundPotentialReason);
							}

							setStatusbarMessage(result, FSMessage, reasonMsg);
						}
						
						grabFocus();
					}
				}
				return TRUE;

				case IDC_CLEAR_ALL :
				{
					if (_currentStatus == MARK_DLG)
					{
						if (isMacroRecording) saveInMacro(wParam, FR_OP_FIND);
						clearMarks(_options);
					}
				}
				return TRUE;

				case IDC_COPY_MARKED_TEXT:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_SEARCH_MARKEDTOCLIP, 0);
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
				case IDREGEXP :
				{
					if (isCheckedOrNot(IDREGEXP))
					{
						_options._searchType = FindRegex;
						findHistory._searchMode = FindHistory::regExpr;
						enableFindDlgItem(IDREDOTMATCHNL);
					}
					else if (isCheckedOrNot(IDEXTENDED))
					{
						_options._searchType = FindExtended;
						findHistory._searchMode = FindHistory::extended;
						enableFindDlgItem(IDREDOTMATCHNL, false);
					}
					else
					{
						_options._searchType = FindNormal;
						findHistory._searchMode = FindHistory::normal;
						enableFindDlgItem(IDREDOTMATCHNL, false);
					}

					bool isRegex = (_options._searchType == FindRegex);
					if (isRegex)
					{
						//regex doesn't allow whole word
						_options._isWholeWord = false;
						::SendDlgItemMessage(_hSelf, IDWHOLEWORD, BM_SETCHECK, _options._isWholeWord?BST_CHECKED:BST_UNCHECKED, 0);

						//regex upward search is disabled
						if (!nppParamInst.regexBackward4PowerUser())
						{
							::SendDlgItemMessage(_hSelf, IDC_BACKWARDDIRECTION, BM_SETCHECK, BST_UNCHECKED, 0);
							_options._whichDirection = DIR_DOWN;
						}
					}

					enableFindDlgItem(IDWHOLEWORD, !isRegex);

					// regex upward search is disabled
					bool doEnable = true;
					if (isRegex && !nppParamInst.regexBackward4PowerUser())
					{
						doEnable = false;
					}
					enableFindDlgItem(IDC_BACKWARDDIRECTION, doEnable);
					enableFindDlgItem(IDC_FINDPREV, doEnable);

					return TRUE;
				}

				case IDWRAP :
					findHistory._isWrap = _options._isWrapAround = isCheckedOrNot(IDWRAP);
					return TRUE;

				case IDC_BACKWARDDIRECTION:
					_options._whichDirection = isCheckedOrNot(IDC_BACKWARDDIRECTION) ? DIR_UP : DIR_DOWN;
					findHistory._isDirectionDown = _options._whichDirection == DIR_DOWN;
					return TRUE;

				case IDC_PURGE_CHECK :
				{
					if (_currentStatus == MARK_DLG)
						findHistory._isPurge = _options._doPurge = isCheckedOrNot(IDC_PURGE_CHECK);
				}
				return TRUE;

				case IDC_MARKLINE_CHECK :
				{
					if (_currentStatus == MARK_DLG)
						findHistory._isBookmarkLine = _options._doMarkLine = isCheckedOrNot(IDC_MARKLINE_CHECK);
				}
				return TRUE;

				case IDC_IN_SELECTION_CHECK :
				{
					if ((_currentStatus == FIND_DLG) || (_currentStatus == REPLACE_DLG) || (_currentStatus == MARK_DLG))
					{
						_options._isInSelection = ::IsWindowEnabled(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK)) &&
							isCheckedOrNot(IDC_IN_SELECTION_CHECK);
					}
				}
				return TRUE;

				case IDC_TRANSPARENT_CHECK :
				{
					bool isChecked = isCheckedOrNot(IDC_TRANSPARENT_CHECK);

					enableFindDlgItem(IDC_TRANSPARENT_LOSSFOCUS_RADIO, isChecked);
					enableFindDlgItem(IDC_TRANSPARENT_ALWAYS_RADIO, isChecked);
					enableFindDlgItem(IDC_PERCENTAGE_SLIDER, isChecked);

					if (isChecked)
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_CHECKED, 0);
						findHistory._transparencyMode = FindHistory::onLossingFocus;
					}
					else
					{
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_LOSSFOCUS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						::SendDlgItemMessage(_hSelf, IDC_TRANSPARENT_ALWAYS_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
						(NppParameters::getInstance()).removeTransparent(_hSelf);
						findHistory._transparencyMode = FindHistory::none;
					}

					return TRUE;
				}

				case IDC_TRANSPARENT_ALWAYS_RADIO :
				{
					int percent = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
					(NppParameters::getInstance()).SetTransparent(_hSelf, percent);
					findHistory._transparencyMode = FindHistory::persistant;
				}
				return TRUE;

				case IDC_TRANSPARENT_LOSSFOCUS_RADIO :
				{
					(NppParameters::getInstance()).removeTransparent(_hSelf);
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

				case IDD_FINDINFILES_PROJECT1_CHECK:
				case IDD_FINDINFILES_PROJECT2_CHECK:
				case IDD_FINDINFILES_PROJECT3_CHECK:
				{
					if (_currentStatus == FINDINPROJECTS_DLG)
					{
						switch (LOWORD(wParam))
						{
							case IDD_FINDINFILES_PROJECT1_CHECK:
								findHistory._isFifProjectPanel_1 = _options._isProjectPanel_1 = isCheckedOrNot(IDD_FINDINFILES_PROJECT1_CHECK);
								break;
							case IDD_FINDINFILES_PROJECT2_CHECK:
								findHistory._isFifProjectPanel_2 = _options._isProjectPanel_2 = isCheckedOrNot(IDD_FINDINFILES_PROJECT2_CHECK);
								break;
							case IDD_FINDINFILES_PROJECT3_CHECK:
								findHistory._isFifProjectPanel_3 = _options._isProjectPanel_3 = isCheckedOrNot(IDD_FINDINFILES_PROJECT3_CHECK);
								break;
						}
						bool enable = _options._isProjectPanel_1 || _options._isProjectPanel_2 || _options._isProjectPanel_3;
						enableFindDlgItem(IDD_FINDINFILES_FIND_BUTTON, enable);
						enableFindDlgItem(IDD_FINDINFILES_REPLACEINPROJECTS, enable);
					}
				}
				return TRUE;

				case IDD_FINDINFILES_SETDIRFROMDOC_BUTTON :
				{
					wstring currPath;
					const Buffer* buf = (*_ppEditView)->getCurrentBuffer();
					if (!(buf->getStatus() & (DOC_UNNAMED | DOC_DELETED)))
					{
						currPath = buf->getFullPathName();
						pathRemoveFileSpec(currPath);
					}

					if (!currPath.empty() && doesDirectoryExist(currPath.c_str()))
					{
						setFindInFilesDirFilter(currPath.c_str(), NULL);
					}
				}
				return TRUE;

				case IDD_FINDINFILES_BROWSE_BUTTON :
				{
					if (_currentStatus == FINDINFILES_DLG)
					{
						NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
						const wstring title = pNativeSpeaker->getLocalizedStrFromID("find-in-files-select-folder", L"Select a folder to search from");
						folderBrowser(_hSelf, title, IDD_FINDINFILES_DIR_COMBO, _options._directory.c_str());
					}
				}
				return TRUE;

				case IDD_RESIZE_TOGGLE_BUTTON:
				{
					RECT rc{};
					getWindowRect(rc);
					LONG w = rc.right - rc.left;
					bool& isLessModeOn = NppParameters::getInstance().getNppGUI()._findWindowLessMode;
					isLessModeOn = !isLessModeOn;
					long dlgH = (isLessModeOn ? _lesssModeHeight : _szMinDialog.cy) + _szBorder.cy;

					DIALOG_TYPE dlgT = getCurrentStatus();
					calcAndSetCtrlsPos(dlgT, true);

					::SetWindowPos(_hSelf, nullptr, 0, 0, w, dlgH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

					hideOrShowCtrl4reduceOrNormalMode(dlgT);

					::SetDlgItemText(_hSelf, IDD_RESIZE_TOGGLE_BUTTON, isLessModeOn ? L"˅" : L"˄");
					
					redraw();
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

bool FindReplaceDlg::processFindNext(const wchar_t *txt2find, const FindOption *options, FindStatus *oFindStatus, FindNextType findNextType /* = FINDNEXTTYPE_FINDNEXT */)
{
	if (oFindStatus)
		*oFindStatus = FSFound;

	if (!txt2find || !txt2find[0])
		return false;

	const FindOption *pOptions = options?options:_env;

	(*_ppEditView)->execute(SCI_CALLTIPCANCEL);

	int stringSizeFind = lstrlen(txt2find);
	wchar_t *pText = new wchar_t[stringSizeFind + 1];
	wcscpy_s(pText, stringSizeFind + 1, txt2find);

	if (pOptions->_searchType == FindExtended)
	{
		stringSizeFind = Searching::convertExtendedToString(txt2find, pText, stringSizeFind);
	}

	intptr_t docLength = (*_ppEditView)->execute(SCI_GETLENGTH);
	Sci_CharacterRangeFull cr = (*_ppEditView)->getSelection();


	//The search "zone" is relative to the selection, so search happens 'outside'
	intptr_t startPosition = cr.cpMax;
	intptr_t endPosition = docLength;


	if (pOptions->_whichDirection == DIR_UP)
	{
		//When searching upwards, start is the lower part, end the upper, for backwards search
		startPosition = cr.cpMax - 1;
		endPosition = 0;
	}

	if (FirstIncremental==pOptions->_incrementalType)
	{
		// the text to find is modified so use the current position
		startPosition = cr.cpMin;
		endPosition = docLength;

		if (pOptions->_whichDirection == DIR_UP)
		{
			//When searching upwards, start is the lower part, end the upper, for backwards search
			startPosition = cr.cpMax;
			endPosition = 0;
		}
	}
	else if (NextIncremental==pOptions->_incrementalType)
	{
		// text to find is not modified, so use current position +1
		startPosition = cr.cpMin + 1;
		endPosition = docLength;

		if (pOptions->_whichDirection == DIR_UP)
		{
			//When searching upwards, start is the lower part, end the upper, for backwards search
			startPosition = cr.cpMax - 1;
			endPosition = 0;
		}
	}

	int flags = Searching::buildSearchFlags(pOptions);
	switch (findNextType)
	{
		case FINDNEXTTYPE_FINDNEXT:
			flags |= SCFIND_REGEXP_EMPTYMATCH_ALL | SCFIND_REGEXP_SKIPCRLFASONE;
		break;

		case FINDNEXTTYPE_REPLACENEXT:
			flags |= SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH | SCFIND_REGEXP_SKIPCRLFASONE;
			break;

		case FINDNEXTTYPE_FINDNEXTFORREPLACE:
			flags |= SCFIND_REGEXP_EMPTYMATCH_ALL | SCFIND_REGEXP_EMPTYMATCH_ALLOWATSTART | SCFIND_REGEXP_SKIPCRLFASONE;
			break;
	}

	intptr_t start, end;
	intptr_t posFind;

	// Never allow a zero length match in the middle of a line end marker
	if ((*_ppEditView)->execute(SCI_GETCHARAT, startPosition - 1) == '\r'
		&& (*_ppEditView)->execute(SCI_GETCHARAT, startPosition) == '\n')
	{
		flags = (flags & ~SCFIND_REGEXP_EMPTYMATCH_MASK) | SCFIND_REGEXP_EMPTYMATCH_NONE;
	}

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);


	posFind = (*_ppEditView)->searchInTarget(pText, stringSizeFind, startPosition, endPosition);
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
		{ // not found
			if (oFindStatus)
				*oFindStatus = FSNotFound;
			//failed, or failed twice with wrap
			if (pOptions->_incrementalType == NotIncremental) //incremental search doesnt trigger messages
			{
				NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				wstring warningMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find", L"Find: Can't find the text \"$STR_REPLACE$\"");
				wstring newTxt2find = stringReplace(txt2find, L"&", L"&&");

				if (newTxt2find.length() > 32) // truncate the search string to display, if the search string is too long
				{
					newTxt2find.erase(28);
					newTxt2find += L"...";
				}

				warningMsg = stringReplace(warningMsg, L"$STR_REPLACE$", newTxt2find);

				warningMsg += L" ";
				warningMsg += getScopeInfoForStatusBar(&_options);

				wstring reasonMsg;
				bool isTheMostLaxMode = _options._isWrapAround && !_options._isMatchCase && !_options._isWholeWord;
				if (!isTheMostLaxMode)
				{
					reasonMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find-pebkac-maybe", noFoundPotentialReason);
				}
				setStatusbarMessage(warningMsg, FSNotFound, reasonMsg);

				// if the dialog is not shown, pass the focus to his parent(ie. Notepad++)
				if (!::IsWindowVisible(_hSelf))
				{
					(*_ppEditView)->grabFocus();
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
	else if (posFind == FIND_INVALID_REGULAR_EXPRESSION)
	{ // error
		setStatusbarMessageWithRegExprErr(*_ppEditView);
		return false;
	}

	start =	posFind;
	end = (*_ppEditView)->execute(SCI_GETTARGETEND);

	setStatusbarMessage(L"", FSNoMessage);

	// to make sure the found result is visible:
	// prevent recording of absolute positioning commands issued in the process
	(*_ppEditView)->execute(SCI_STOPRECORD);
	Searching::displaySectionCentered(start, end, *_ppEditView, pOptions->_whichDirection == DIR_DOWN);
	// Show a calltip for a zero length match
	if (start == end)
	{
		NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-regex-zero-length-match", L"zero length match");
		msg = L"^ " + msg;
		(*_ppEditView)->showCallTip(start, msg.c_str());
	}
	if (static_cast<MacroStatus>(::SendMessage(_hParent, NPPM_GETCURRENTMACROSTATUS,0,0)) == MacroStatus::RecordInProgress)
		(*_ppEditView)->execute(SCI_STARTRECORD);

	delete [] pText;

	return true;
}

// return value :
// true  : the text is replaced, and find the next occurrence
// false : the text2find is not found, so the text is NOT replace
//      || the text is replaced, and do NOT find the next occurrence
bool FindReplaceDlg::processReplace(const wchar_t *txt2find, const wchar_t *txt2replace, const FindOption *options)
{
	bool moreMatches;

	if (!txt2find || !txt2find[0] || !txt2replace)
		return false;

	if ((*_ppEditView)->getCurrentBuffer()->isReadOnly())
	{
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replace-readonly", L"Replace: Cannot replace text. The current document is read only.");
		setStatusbarMessage(msg, FSNotFound);
		return false;
	}

	FindOption replaceOptions = options ? *options : *_env;
	replaceOptions._incrementalType = FirstIncremental;

	Sci_CharacterRangeFull currentSelection = (*_ppEditView)->getSelection();
	FindStatus status;
	moreMatches = processFindNext(txt2find, &replaceOptions, &status, FINDNEXTTYPE_FINDNEXTFORREPLACE);

	if (moreMatches)
	{
		Sci_CharacterRangeFull nextFind = (*_ppEditView)->getSelection();

		// If the next find is the same as the last, then perform the replacement
		if (nextFind.cpMin == currentSelection.cpMin && nextFind.cpMax == currentSelection.cpMax)
		{
			bool isRegExp = replaceOptions._searchType == FindRegex;

			intptr_t start = currentSelection.cpMin;
			intptr_t replacedLen = 0;
			if (isRegExp)
			{
				replacedLen = (*_ppEditView)->replaceTargetRegExMode(txt2replace);
			}
			else
			{
				if (replaceOptions._searchType == FindExtended)
				{
					int stringSizeReplace = lstrlen(txt2replace);
					wchar_t *pText2ReplaceExtended = new wchar_t[stringSizeReplace + 1];
					Searching::convertExtendedToString(txt2replace, pText2ReplaceExtended, stringSizeReplace);

					replacedLen = (*_ppEditView)->replaceTarget(pText2ReplaceExtended);

					delete[] pText2ReplaceExtended;
				}
				else
				{
					replacedLen = (*_ppEditView)->replaceTarget(txt2replace);
				}
			}
			(*_ppEditView)->execute(SCI_SETSEL, start + replacedLen, start + replacedLen);

			NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

			NppParameters& nppParam = NppParameters::getInstance();
			const NppGUI& nppGui = nppParam.getNppGUI();

			if (nppGui._replaceStopsWithoutFindingNext)
			{
				wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replaced-without-continuing", L"Replace: 1 occurrence was replaced.");
				setStatusbarMessage(msg, FSMessage);
			}
			else
			{
				// Do the next find
				moreMatches = processFindNext(txt2find, &replaceOptions, &status, FINDNEXTTYPE_REPLACENEXT);

				if (status == FSEndReached)
				{
					wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replace-end-reached", FIND_STATUS_REPLACE_END_REACHED_TEXT);
					setStatusbarMessage(msg, FSEndReached);
				}
				else if (status == FSTopReached)
				{
					wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replace-top-reached", FIND_STATUS_REPLACE_TOP_REACHED_TEXT);
					setStatusbarMessage(msg, FSTopReached);
				}
				else
				{
					wstring msg;
					if (moreMatches)
						msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replaced-next-found", L"Replace: 1 occurrence was replaced. The next occurrence found.");
					else
						msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replaced-next-not-found", L"Replace: 1 occurrence was replaced. No more occurrences were found.");

					setStatusbarMessage(msg, FSMessage);
				}
			}
		}
	}
	else
	{
		if (_statusbarTooltipMsg.empty()) // Tooltip message non-empty means there's a find problem - so we keep the message as it is and not erase it
		{
			NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replace-not-found", L"Replace: no occurrence was found");

			msg += L" ";
			msg += getScopeInfoForStatusBar(&_options);

			wstring reasonMsg;
			bool isTheMostLaxMode = _options._isWrapAround && !_options._isMatchCase && !_options._isWholeWord;
			if (!isTheMostLaxMode)
			{
				reasonMsg = pNativeSpeaker->getLocalizedStrFromID("find-status-cannot-find-pebkac-maybe", noFoundPotentialReason);
			}
			setStatusbarMessage(msg, FSNotFound, reasonMsg);
		}
	}

	return moreMatches;
}



int FindReplaceDlg::markAll(const wchar_t *txt2find, int styleID)
{
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
	FindOption markAllOpt;

	markAllOpt._isMatchCase = nppGUI._markAllCaseSensitive;
	markAllOpt._isWholeWord = nppGUI._markAllWordOnly;
	markAllOpt._str2Search = txt2find;

	int nbFound = processAll(ProcessMarkAllExt, &markAllOpt, true, NULL, styleID);
	return nbFound;
}


int FindReplaceDlg::markAllInc(const FindOption *opt)
{
	int nbFound = processAll(ProcessMarkAll_IncSearch, opt,  true);
	return nbFound;
}

int FindReplaceDlg::processAll(ProcessOperation op, const FindOption *opt, bool isEntire, const FindersInfo *pFindersInfo, int colourStyleID)
{
	if (op == ProcessReplaceAll && (*_ppEditView)->getCurrentBuffer()->isReadOnly())
	{
		NppParameters& nppParam = NppParameters::getInstance();
		NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
		wstring msg = pNativeSpeaker->getLocalizedStrFromID("find-status-replaceall-readonly", L"Replace All: Cannot replace text. The current document is read only.");
		setStatusbarMessage(msg, FSNotFound);
		return 0;
	}

	// Turn OFF all the notification of modification (SCN_MODIFIED) for the sake of performance
	LRESULT notifFlag = (*_ppEditView)->execute(SCI_GETMODEVENTMASK);
	(*_ppEditView)->execute(SCI_SETMODEVENTMASK, 0);



	const FindOption *pOptions = opt?opt:_env;
	const wchar_t *txt2find = pOptions->_str2Search.c_str();
	const wchar_t *txt2replace = pOptions->_str4Replace.c_str();

	Sci_CharacterRangeFull cr = (*_ppEditView)->getSelection();
	size_t docLength = (*_ppEditView)->execute(SCI_GETLENGTH);

	// Default :
	//        direction : down
	//        begin at : 0
	//        end at : end of doc
	size_t startPosition = 0;
	size_t endPosition = docLength;

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
	if (op == ProcessCountAll && pOptions->_isInSelection)
	{
		startPosition = cr.cpMin;
		endPosition = cr.cpMax;
	}
	else if (pOptions->_isWrapAround || isEntire)	//entire document needs to be scanned
	{
		startPosition = 0;
		endPosition = docLength;
	}

	//then readjust scope if the selection override is active and allowed
	if ((pOptions->_isInSelection) && ((op == ProcessMarkAll) || ((op == ProcessReplaceAll || op == ProcessFindAll) && (!isEntire))))
		//if selection limiter and either mark all or replace all or find all w/o entire document override
	{
		startPosition = cr.cpMin;
		endPosition = cr.cpMax;
	}

	if ((op == ProcessMarkAllExt) && (colourStyleID != -1))
	{
		startPosition = 0;
		endPosition = docLength;
	}

	FindReplaceInfo findReplaceInfo;
	findReplaceInfo._txt2find = txt2find;
	findReplaceInfo._txt2replace = txt2replace;
	findReplaceInfo._startRange = startPosition;
	findReplaceInfo._endRange = endPosition;

	int nbProcessed = processRange(op, findReplaceInfo, pFindersInfo, pOptions, colourStyleID);


	// Turn ON the notifications after operations
	(*_ppEditView)->execute(SCI_SETMODEVENTMASK, notifFlag);
	if (op == ProcessReplaceAll && nbProcessed > 0) // All the notification of modification (SCN_MODIFIED) were removed during the operations, so we set modified status true here
	{
		Buffer* buf = (*_ppEditView)->getCurrentBuffer();
		buf->setModifiedStatus(true);
		::SendMessage(_hParent, NPPM_INTERNAL_DOCMODIFIEDBYREPLACEALL, reinterpret_cast<WPARAM>(buf), 0);
	}


	if (nbProcessed == FIND_INVALID_REGULAR_EXPRESSION)
		return FIND_INVALID_REGULAR_EXPRESSION;

	if (nbProcessed > 0 && op == ProcessReplaceAll && pOptions->_isInSelection)
	{
		size_t newDocLength = (*_ppEditView)->execute(SCI_GETLENGTH);
		endPosition += newDocLength - docLength;
		(*_ppEditView)->execute(SCI_SETSELECTION, endPosition, startPosition);
		(*_ppEditView)->execute(SCI_SCROLLRANGE, startPosition, endPosition);
		if (startPosition == endPosition)
		{
			const NppGUI& nppGui = (NppParameters::getInstance()).getNppGUI();
			if (nppGui._inSelectionAutocheckThreshold != 0)
			{
				setChecked(IDC_IN_SELECTION_CHECK, false);
			}
			enableFindDlgItem(IDC_IN_SELECTION_CHECK, false);
		}
	}
	return nbProcessed;
}

int FindReplaceDlg::processRange(ProcessOperation op, FindReplaceInfo & findReplaceInfo, const FindersInfo * pFindersInfo, const FindOption *opt, int colourStyleID, ScintillaEditView *view2Process)
{
	int nbProcessed = 0;

	if (!isCreated() && !findReplaceInfo._txt2find)
		return 0;

	if (!_ppEditView)
		return 0;

	ScintillaEditView *pEditView = *_ppEditView;

	if (view2Process)
		pEditView = view2Process;

	if ((op == ProcessReplaceAll) && pEditView->getCurrentBuffer()->isReadOnly())
		return nbProcessed;

	if (findReplaceInfo._startRange == findReplaceInfo._endRange)
		return nbProcessed;

	const FindOption *pOptions = opt ? opt : _env;

	LRESULT stringSizeFind = 0;
	LRESULT stringSizeReplace = 0;

	wchar_t *pTextFind = NULL;
	if (!findReplaceInfo._txt2find)
	{
		HWND hFindCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
		wstring str2Search = getTextFromCombo(hFindCombo);
		stringSizeFind = str2Search.length();
		pTextFind = new wchar_t[stringSizeFind + 1];
		wcscpy_s(pTextFind, stringSizeFind + 1, str2Search.c_str());
	}
	else
	{
		stringSizeFind = lstrlen(findReplaceInfo._txt2find);
		pTextFind = new wchar_t[stringSizeFind + 1];
		wcscpy_s(pTextFind, stringSizeFind + 1, findReplaceInfo._txt2find);
	}

	if (!pTextFind[0])
	{
		delete [] pTextFind;
		return nbProcessed;
	}

	wchar_t *pTextReplace = NULL;
	if (op == ProcessReplaceAll)
	{
		if (!findReplaceInfo._txt2replace)
		{
			HWND hReplaceCombo = ::GetDlgItem(_hSelf, IDREPLACEWITH);
			wstring str2Replace = getTextFromCombo(hReplaceCombo);
			stringSizeReplace = str2Replace.length();
			pTextReplace = new wchar_t[stringSizeReplace + 1];
			wcscpy_s(pTextReplace, stringSizeReplace + 1, str2Replace.c_str());
		}
		else
		{
			stringSizeReplace = lstrlen(findReplaceInfo._txt2replace);
			pTextReplace = new wchar_t[stringSizeReplace + 1];
			wcscpy_s(pTextReplace, stringSizeReplace + 1, findReplaceInfo._txt2replace);
		}
	}

	if (pOptions->_searchType == FindExtended)
	{
		stringSizeFind = Searching::convertExtendedToString(pTextFind, pTextFind, static_cast<int32_t>(stringSizeFind));
		if (op == ProcessReplaceAll)
			stringSizeReplace = Searching::convertExtendedToString(pTextReplace, pTextReplace, static_cast<int32_t>(stringSizeReplace));
	}

	bool isRegExp = pOptions->_searchType == FindRegex;
	int flags = Searching::buildSearchFlags(pOptions) | SCFIND_REGEXP_SKIPCRLFASONE;

	// Allow empty matches, but not immediately after previous match for replace all or find all.
	// Other search types should ignore empty matches completely.
	if (op == ProcessReplaceAll || op == ProcessFindAll)
		flags |= SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH;



	if (op == ProcessMarkAll && colourStyleID == -1)	//if marking, check if purging is needed
	{
		if (_env->_doPurge)
		{
			clearMarks(*_env);
		}
	}

	intptr_t targetStart = 0;

	//Initial range for searching
	pEditView->execute(SCI_SETSEARCHFLAGS, flags);


	bool findAllFileNameAdded = false;

	// A temporary string which is used to populate the search result window
	std::unique_ptr<std::string> text2AddUtf8(new std::string());
	size_t indexBuffer = 0;

	while (targetStart >= 0)
	{
		targetStart = pEditView->searchInTarget(pTextFind, stringSizeFind, findReplaceInfo._startRange, findReplaceInfo._endRange);

		// If we've not found anything, just break out of the loop
		if (targetStart == -1)
			break;

		if (targetStart == FIND_INVALID_REGULAR_EXPRESSION)
			return FIND_INVALID_REGULAR_EXPRESSION;

		intptr_t targetEnd = pEditView->execute(SCI_GETTARGETEND);

		if (targetEnd > findReplaceInfo._endRange)
		{
			//we found a result but outside our range, therefore do not process it
			break;
		}

		intptr_t foundTextLen = targetEnd - targetStart;
		intptr_t replaceDelta = 0;
		bool processed = true;

		switch (op)
		{
			case ProcessFindAll:
			{
				const wchar_t *pFileName = L"";
				if (pFindersInfo && pFindersInfo->_pFileName)
					pFileName = pFindersInfo->_pFileName;

				if (!findAllFileNameAdded)	//add new filetitle in hits if we haven't already
				{
					_pFinder->addFileNameTitle(pFileName);
					findAllFileNameAdded = true;
				}

				auto totalLineNumber = pEditView->execute(SCI_GETLINECOUNT);
				auto lineNumber = pEditView->execute(SCI_LINEFROMPOSITION, targetStart);
				intptr_t lend = pEditView->execute(SCI_GETLINEENDPOSITION, lineNumber);
				intptr_t lstart = pEditView->execute(SCI_POSITIONFROMLINE, lineNumber);
				intptr_t nbChar = lend - lstart;

				// use the static buffer
				wchar_t lineBuf[SC_SEARCHRESULT_LINEBUFFERMAXLENGTH]{};

				if (nbChar > SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - 3)
					lend = lstart + SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - 4;

				intptr_t start_mark = targetStart - lstart;
				intptr_t end_mark = targetEnd - lstart;

				pEditView->getGenericText(lineBuf, SC_SEARCHRESULT_LINEBUFFERMAXLENGTH, lstart, lend, &start_mark, &end_mark);

				wstring line = lineBuf;
				line += L"\r\n";

				SearchResultMarkingLine srml;
				srml._segmentPostions.push_back(std::pair<intptr_t, intptr_t>(start_mark, end_mark));
				text2AddUtf8->append(_pFinder->foundLine(FoundInfo(targetStart, targetEnd, lineNumber + 1, pFileName), srml, line.c_str(), totalLineNumber));

				if (text2AddUtf8->length() > FINDTEMPSTRING_MAXSIZE)
				{
					_pFinder->setFinderReadOnly(false);
					_pFinder->_scintView.execute(SCI_ADDTEXT, text2AddUtf8->length(), reinterpret_cast<LPARAM>(text2AddUtf8->c_str()));
					_pFinder->setFinderReadOnly(true);
					text2AddUtf8->clear();
				}
				break;
			}

			case ProcessFindInFinder:
			{
				if (!pFindersInfo || !pFindersInfo->_pSourceFinder || !pFindersInfo->_pDestFinder)
					break;

				const wchar_t *pFileName = pFindersInfo->_pFileName ? pFindersInfo->_pFileName : L"";

				auto totalLineNumber = pEditView->execute(SCI_GETLINECOUNT);
				auto lineNumber = pEditView->execute(SCI_LINEFROMPOSITION, targetStart);
				intptr_t lend = pEditView->execute(SCI_GETLINEENDPOSITION, lineNumber);
				intptr_t lstart = pEditView->execute(SCI_POSITIONFROMLINE, lineNumber);
				intptr_t nbChar = lend - lstart;

				// use the static buffer
				wchar_t lineBuf[SC_SEARCHRESULT_LINEBUFFERMAXLENGTH]{};

				if (nbChar > SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - 3)
					lend = lstart + SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - 4;

				intptr_t start_mark = targetStart - lstart;
				intptr_t end_mark = targetEnd - lstart;

				pEditView->getGenericText(lineBuf, SC_SEARCHRESULT_LINEBUFFERMAXLENGTH, lstart, lend, &start_mark, &end_mark);

				wstring line = lineBuf;
				line += L"\r\n";
				SearchResultMarkingLine srml;
				srml._segmentPostions.push_back(std::pair<intptr_t, intptr_t>(start_mark, end_mark));

				processed = (!pOptions->_isMatchLineNumber) || (pFindersInfo->_pSourceFinder->canFind(pFileName, lineNumber + 1, &indexBuffer));
				if (processed)
				{
					if (!findAllFileNameAdded)	//add new filetitle in hits if we haven't already
					{
						pFindersInfo->_pDestFinder->addFileNameTitle(pFileName);
						findAllFileNameAdded = true;
					}
					text2AddUtf8->append(pFindersInfo->_pDestFinder->foundLine(FoundInfo(targetStart, targetEnd, lineNumber + 1, pFileName), srml, line.c_str(), totalLineNumber));

					if (text2AddUtf8->length() > FINDTEMPSTRING_MAXSIZE)
					{
						pFindersInfo->_pDestFinder->setFinderReadOnly(false);
						pFindersInfo->_pDestFinder->_scintView.execute(SCI_ADDTEXT, text2AddUtf8->length(), reinterpret_cast<LPARAM>(text2AddUtf8->c_str()));
						pFindersInfo->_pDestFinder->setFinderReadOnly(true);
						text2AddUtf8->clear();
					}
				}
				break;
			}

			case ProcessReplaceAll:
			{
				intptr_t replacedLength;
				if (isRegExp)
					replacedLength = pEditView->replaceTargetRegExMode(pTextReplace);
				else
					replacedLength = pEditView->replaceTarget(pTextReplace);

				replaceDelta = replacedLength - foundTextLen;
				break;
			}

			case ProcessMarkAll:
			{
				// In theory, we can't have empty matches for a ProcessMarkAll, but because scintilla
				// gets upset if we call INDICATORFILLRANGE with a length of 0, we protect against it here.
				// At least in version 2.27, after calling INDICATORFILLRANGE with length 0, further indicators
				// on the same line would simply not be shown.  This may have been fixed in later version of Scintilla.
				if (foundTextLen > 0)
				{
					pEditView->execute(SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_FOUND_STYLE);
					pEditView->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				}

				if (_env->_doMarkLine)
				{
					auto lineNumber = pEditView->execute(SCI_LINEFROMPOSITION, targetStart);
					auto lineNumberEnd = pEditView->execute(SCI_LINEFROMPOSITION, targetEnd - 1);

					for (auto i = lineNumber; i <= lineNumberEnd; ++i)
					{
						auto state = pEditView->execute(SCI_MARKERGET, i);

						if (!(state & (1 << MARK_BOOKMARK)))
							pEditView->execute(SCI_MARKERADD, i, MARK_BOOKMARK);
					}
				}
				break;
			}

			case ProcessMarkAllExt:
			{
				// See comment by ProcessMarkAll
				if (foundTextLen > 0)
				{
					pEditView->execute(SCI_SETINDICATORCURRENT,  colourStyleID);
					pEditView->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				}
				break;
			}

			case ProcessMarkAll_2:
			{
				// See comment by ProcessMarkAll
				if (foundTextLen > 0)
				{
					pEditView->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_SMART);
					pEditView->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				}
				break;
			}

			case ProcessMarkAll_IncSearch:
			{
				// See comment by ProcessMarkAll
				if (foundTextLen > 0)
				{
					pEditView->execute(SCI_SETINDICATORCURRENT,  SCE_UNIVERSAL_FOUND_STYLE_INC);
					pEditView->execute(SCI_INDICATORFILLRANGE,  targetStart, foundTextLen);
				}
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
		if (processed) ++nbProcessed;

		// After the processing of the last string occurrence the search loop should be stopped
		// This helps to avoid the endless replacement during the EOL ("$") searching
		if (targetStart + foundTextLen == findReplaceInfo._endRange)
			break;

		findReplaceInfo._startRange = targetStart + foundTextLen + replaceDelta;		//search from result onwards
		findReplaceInfo._endRange += replaceDelta;									//adjust end of range in case of replace
	}

	delete [] pTextFind;
	delete [] pTextReplace;

	if (nbProcessed > 0)
	{
		Finder *pFinder = nullptr;
		if (op == ProcessFindAll)
		{
			_pFinder->setFinderReadOnly(false);
			_pFinder->_scintView.execute(SCI_ADDTEXT, text2AddUtf8->length(), reinterpret_cast<LPARAM>(text2AddUtf8->c_str()));
			_pFinder->setFinderReadOnly(true);
			text2AddUtf8->clear();
			pFinder = _pFinder;
		}
		else if (op == ProcessFindInFinder)
		{
			if (pFindersInfo && pFindersInfo->_pDestFinder)
			{
				pFindersInfo->_pDestFinder->setFinderReadOnly(false);
				pFindersInfo->_pDestFinder->_scintView.execute(SCI_ADDTEXT, text2AddUtf8->length(), reinterpret_cast<LPARAM>(text2AddUtf8->c_str()));
				pFindersInfo->_pDestFinder->setFinderReadOnly(true);
				text2AddUtf8->clear();
				pFinder = pFindersInfo->_pDestFinder;
			}
			else
				pFinder = _pFinder;
		}

		if (pFinder != nullptr)
			pFinder->addFileHitCount(nbProcessed);
	}
	return nbProcessed;
}

void FindReplaceDlg::replaceAllInOpenedDocs()
{
	::SendMessage(_hParent, WM_REPLACEALL_INOPENEDDOC, 0, 0);
}

void FindReplaceDlg::findAllIn(InWhat op)
{
	bool justCreated = false;
	if (!_pFinder)
	{
		NppParameters& nppParam = NppParameters::getInstance();

		_pFinder = new Finder();
		_pFinder->init(_hInst, (*_ppEditView)->getHParent(), _ppEditView);
		_pFinder->setVolatiled(false);

		tTbData	data{};
		_pFinder->create(&data);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(_pFinder->getHSelf()));
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO | DWS_USEOWNDARKMODE;

		int icoID = IDR_FIND_RESULT_ICO2;
		if (nppParam.getNppGUI()._tbIconInfo._tbIconSet == TB_STANDARD)
			icoID = IDI_FIND_RESULT_ICON;
		else if (NppDarkMode::isEnabled())
			icoID = IDR_FIND_RESULT_ICO_DM;

		const int iconSize = DPIManagerV2::scale(g_dockingContTabIconSize, _pFinder->getHSelf());
		DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSize, iconSize, &data.hIconTab, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);

		data.pszAddInfo = _findAllResultStr;

		data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		data.dlgID = 0;

		NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
		wstring text = pNativeSpeaker->getLocalizedStrFromID("find-result-caption", L"");

		if (!text.empty())
		{
			_findResTitle = text;
			data.pszName = _findResTitle.c_str();
		}

		::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

		_pFinder->_scintView.init(_hInst, _pFinder->getHSelf());

		// Subclass the ScintillaEditView for the Finder (Scintilla doesn't notify all key presses)
		originalFinderProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(_pFinder->_scintView.getHSelf(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(finderProc)));

		_pFinder->setFinderReadOnly(true);
		_pFinder->_scintView.execute(SCI_SETCODEPAGE, SC_CP_UTF8);
		_pFinder->_scintView.execute(SCI_USEPOPUP, SC_POPUP_NEVER);
		_pFinder->_scintView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF); // Turn off the modification event
		_pFinder->_scintView.execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
		_pFinder->_scintView.execute(SCI_SETCARETWIDTH, 1);
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_FOLDER, true);

		_pFinder->_scintView.execute(SCI_SETUSETABS, true);
		_pFinder->_scintView.execute(SCI_SETTABWIDTH, 4);

		const NppGUI& nppGUI = nppParam.getNppGUI();
		_pFinder->_longLinesAreWrapped = nppGUI._finderLinesAreCurrentlyWrapped;
		_pFinder->_scintView.wrap(_pFinder->_longLinesAreWrapped);
		_pFinder->_scintView.setWrapMode(LINEWRAP_INDENT);
		_pFinder->_scintView.showWrapSymbol(true);

		_pFinder->_purgeBeforeEverySearch = nppGUI._finderPurgeBeforeEverySearch;

		// allow user to start selecting as a stream block, then switch to a column block by adding Alt keypress
		_pFinder->_scintView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);

		// get the width of FindDlg
		RECT findRect;
		::GetWindowRect(_pFinder->getHSelf(), &findRect);

		// overwrite some default settings
		_pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_SYMBOL, false);
		_pFinder->_scintView.setMakerStyle(FOLDER_STYLE_SIMPLE);

		NppDarkMode::setBorder(_pFinder->_scintView.getHSelf());

		_pFinder->_scintView.display();
		_pFinder->setFinderStyle();
		_pFinder->setFinderStyleForNpc();

		_pFinder->display(false);
		::UpdateWindow(_hParent);
		justCreated = true;
		
	}
	
	if (_pFinder->_purgeBeforeEverySearch)
	{
		_pFinder->removeAll();
	}

	if (justCreated)
	{
		// Send the address of _MarkingsStruct to the lexer
		char ptrword[sizeof(void*) * 2 + 1];
		sprintf(ptrword, "%p", static_cast<void*>(&_pFinder->_markingsStruct));
		_pFinder->_scintView.execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("@MarkingsStruct"), reinterpret_cast<LPARAM>(ptrword));

		//enable "Search Results Window" under Search Menu
		::EnableMenuItem(::GetMenu(_hParent), IDM_FOCUS_ON_FOUND_RESULTS, MF_ENABLED | MF_BYCOMMAND);
		::EnableMenuItem(::GetMenu(_hParent), IDM_SEARCH_GOTONEXTFOUND, MF_ENABLED | MF_BYCOMMAND);
		::EnableMenuItem(::GetMenu(_hParent), IDM_SEARCH_GOTOPREVFOUND, MF_ENABLED | MF_BYCOMMAND);
	}

	::SendMessage(_pFinder->getHSelf(), WM_SIZE, 0, 0);

	bool toRTL = (*_ppEditView)->isTextDirectionRTL();
	bool isRTL = _pFinder->_scintView.isTextDirectionRTL();

	if ((toRTL && !isRTL) || (!toRTL && isRTL))
		_pFinder->_scintView.changeTextDirection(toRTL);

	int cmdid = 0;
	if (op == ALL_OPEN_DOCS)
		cmdid = WM_FINDALL_INOPENEDDOC;
	else if (op == FILES_IN_DIR)
		cmdid = WM_FINDINFILES;
	else if (op == FILES_IN_PROJECTS)
		cmdid = WM_FINDINPROJECTS;
	else if ((op == CURRENT_DOC) || (op == CURR_DOC_SELECTION))
		cmdid = WM_FINDALL_INCURRENTDOC;

	if (!cmdid) return;

	bool limitSearchScopeToSelection = op == CURR_DOC_SELECTION;
	if (::SendMessage(_hParent, cmdid, static_cast<WPARAM>(limitSearchScopeToSelection ? 1 : 0), 0))
	{
		wstring text = _pFinder->getHitsString(_findAllResult);
		wsprintf(_findAllResultStr, text.c_str());

		if (_findAllResult)
		{
			focusOnFinder();
		}
		else
		{
			// Show finder
			_pFinder->display();
			grabFocus(); // no hits
		}
	}
	else // error - search folder doesn't exist
		::SendMessage(_hSelf, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(::GetDlgItem(_hSelf, IDD_FINDINFILES_DIR_COMBO)), TRUE);
}

Finder* FindReplaceDlg::createFinder()
{
	NppParameters& nppParam = NppParameters::getInstance();

	Finder* pFinder = new Finder();
	pFinder->init(_hInst, (*_ppEditView)->getHParent(), _ppEditView);

	tTbData	data{};
	bool isRTL = _pFinder->_scintView.isTextDirectionRTL();
	pFinder->create(&data, isRTL);
	::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<WPARAM>(pFinder->getHSelf()));
	// define the default docking behaviour
	data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB | DWS_ADDINFO | DWS_USEOWNDARKMODE;

	int icoID = IDR_FIND_RESULT_ICO2;
	if (nppParam.getNppGUI()._tbIconInfo._tbIconSet == TB_STANDARD)
		icoID = IDI_FIND_RESULT_ICON;
	else if (NppDarkMode::isEnabled())
		icoID = IDR_FIND_RESULT_ICO_DM;

	const int iconSize = DPIManagerV2::scale(g_dockingContTabIconSize, _pFinder->getHSelf());
	DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSize, iconSize, &data.hIconTab, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);

	data.pszAddInfo = _findAllResultStr;

	data.pszModuleName = NPP_INTERNAL_FUCTION_STR;

	// the dlgDlg should be the index of funcItem where the current function pointer is
	// in this case is DOCKABLE_DEMO_INDEX
	data.dlgID = 0;

	NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
	wstring text = pNativeSpeaker->getLocalizedStrFromID("find-result-caption", L"");
	if (!text.empty())
	{
		_findResTitle = text;
		data.pszName = _findResTitle.c_str();
	}

	::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

	pFinder->_scintView.init(_hInst, pFinder->getHSelf());
	if (isRTL)
		pFinder->_scintView.changeTextDirection(true);

	// Subclass the ScintillaEditView for the Finder (Scintilla doesn't notify all key presses)
	originalFinderProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(pFinder->_scintView.getHSelf(), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(finderProc)));

	pFinder->setFinderReadOnly(true);
	pFinder->_scintView.execute(SCI_SETCODEPAGE, SC_CP_UTF8);
	pFinder->_scintView.execute(SCI_USEPOPUP, SC_POPUP_NEVER);
	pFinder->_scintView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF); // Turn off the modification event
	pFinder->_scintView.execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
	pFinder->_scintView.execute(SCI_SETCARETWIDTH, 1);
	pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_FOLDER, true);

	pFinder->_scintView.execute(SCI_SETUSETABS, true);
	pFinder->_scintView.execute(SCI_SETTABWIDTH, 4);

	// inherit setting from current state of main finder:
	pFinder->_longLinesAreWrapped = _pFinder->_longLinesAreWrapped;
	pFinder->_scintView.wrap(pFinder->_longLinesAreWrapped);
	pFinder->_scintView.setWrapMode(LINEWRAP_INDENT);
	pFinder->_scintView.showWrapSymbol(true);

	// allow user to start selecting as a stream block, then switch to a column block by adding Alt keypress
	pFinder->_scintView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);

	// get the width of FindDlg
	RECT findRect;
	::GetWindowRect(pFinder->getHSelf(), &findRect);

	// overwrite some default settings
	pFinder->_scintView.showMargin(ScintillaEditView::_SC_MARGE_SYMBOL, false);
	pFinder->_scintView.setMakerStyle(FOLDER_STYLE_SIMPLE);

	pFinder->_scintView.display();
	::UpdateWindow(_hParent);

	NppDarkMode::setBorder(pFinder->_scintView.getHSelf());
	pFinder->setFinderStyle();
	pFinder->setFinderStyleForNpc();

	// Send the address of _MarkingsStruct to the lexer
	char ptrword[sizeof(void*) * 2 + 1];
	sprintf(ptrword, "%p", static_cast<void*>(&pFinder->_markingsStruct));
	pFinder->_scintView.execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("@MarkingsStruct"), reinterpret_cast<LPARAM>(ptrword));

	_findersOfFinder.push_back(pFinder);

	::SendMessage(pFinder->getHSelf(), WM_SIZE, 0, 0);

	// Show finder
	pFinder->display();
	pFinder->_scintView.grabFocus();

	return pFinder;
}

bool FindReplaceDlg::removeFinder(Finder *finder2remove)
{
	for (vector<Finder *>::iterator i = _findersOfFinder.begin(); i != _findersOfFinder.end(); ++i)
	{
		if (*i == finder2remove)
		{
			delete finder2remove;
			_findersOfFinder.erase(i);
			return true;
		}
	}
	return false;
}

Finder* FindReplaceDlg::getFinderFrom(HWND hwnd)
{
	if (_pFinder && _pFinder->isCreated())
	{
		if (_pFinder->_scintView.getHSelf() == hwnd)
			return _pFinder;

		if (!_findersOfFinder.empty())
		{
			for (const auto& finder : _findersOfFinder)
			{
				if (finder->_scintView.getHSelf() == hwnd)
					return finder;
			}
		}
	}

	return nullptr;
}

int FindReplaceDlg::regexBackwardMsgBox()
{
	NppParameters& nppParam = NppParameters::getInstance();

	const int msgboxID = nppParam.getNativeLangSpeaker()->messageBox("FindRegexBackwardDisabled",
		(*_ppEditView)->getHParent(),
		L"By default, backward regex searching is disabled due to potentially unexpected results. " \
		L"To perform a backward search, open the Find dialog and select either normal or extended search mode instead of regular expression.\r\n" \
		L"Press the OK button to open the Find dialog or set focus on it.\r\n" \
		L"\r\n" \
		L"If you require the backward regex searching feature, consult the user manual for instructions on enabling it.",
		L"Regex backward search disabled",
		MB_OKCANCEL | MB_APPLMODAL | MB_ICONINFORMATION);

	switch (msgboxID)
	{
		case IDOK:
		{
			doDialog(FIND_DLG, nppParam.getNativeLangSpeaker()->isRTL());
			goToCenter();
			::SetFocus(::GetDlgItem(_hSelf, IDREGEXP));
			break;
		}

		case IDCANCEL:
		default:
			break;
	}

	return msgboxID;
}

void FindReplaceDlg::setSearchText(wchar_t * txt2find)
{
	HWND hCombo = ::GetDlgItem(_hSelf, IDFINDWHAT);
	if (txt2find && txt2find[0])
	{
		// We got a valid search string
		::SendMessage(hCombo, CB_SETCURSEL, static_cast<WPARAM>(-1), 0); // remove selection - to allow using down arrow to get to last searched word
		::SetDlgItemText(_hSelf, IDFINDWHAT, txt2find);
	}
	::SendMessage(hCombo, CB_SETEDITSEL, 0, MAKELPARAM(0, -1)); // select all text - fast edit
}

void FindReplaceDlg::enableFindDlgItem(int dlgItemID, bool isEnable /* = true*/)
{
	HWND h = ::GetDlgItem(_hSelf, dlgItemID);
	if (!h) return;

	::EnableWindow(h, isEnable ? TRUE : FALSE);

	// remember the real state of this control being enabled/disabled
	_controlEnableMap[dlgItemID] = isEnable;
}

void FindReplaceDlg::showFindDlgItem(int dlgItemID, bool isShow /* = true*/)
{
	HWND h = ::GetDlgItem(_hSelf, dlgItemID);
	if (!h) return;

	::ShowWindow(h, isShow ? SW_SHOW : SW_HIDE);

	// when hiding a control to make it user-inaccessible, it can still be manipulated via a keyboard accelerator!
	// so disable it in addition to hiding it to prevent such user interaction
	// but that causes trouble when unhiding it; we don't know if it should be enabled or disabled,
	// so used the remembered state when we last enabled/disabled this control to determine it

	if (dlgItemID == IDOK)
	{
		// do not disable the standard Find-Next button (IDOK);
		// keeping it enabled allows Enter (and Shift+Enter) to work when in 2-button-find-mode
		// and IDC_FINDNEXT does not have focus
		return;
	}

	int enable = isShow ? TRUE : FALSE;

	if (isShow)
	{
		const auto iter = _controlEnableMap.find(dlgItemID);
		if (iter == _controlEnableMap.end())
		{
			// if control's state was never previously recorded, assume it was enabled
			enable = TRUE;
			_controlEnableMap[dlgItemID] = true;
		}
		else
		{
			enable = iter->second ? TRUE : FALSE;
		}
	}

	::EnableWindow(h, enable);
}

void FindReplaceDlg::enableReplaceFunc(bool isEnable)
{
	_currentStatus = isEnable ? REPLACE_DLG : FIND_DLG;

	enableFindInFilesControls(false, false);
	enableMarkAllControls(false);
	// replace controls
	showFindDlgItem(ID_STATICTEXT_REPLACE, isEnable);
	showFindDlgItem(IDREPLACE, isEnable);
	showFindDlgItem(IDREPLACEWITH, isEnable);
	showFindDlgItem(IDD_FINDREPLACE_SWAP_BUTTON, isEnable);
	showFindDlgItem(IDREPLACEALL, isEnable);
	showFindDlgItem(IDC_REPLACE_OPENEDFILES, isEnable);
	showFindDlgItem(IDC_REPLACEINSELECTION);
	showFindDlgItem(IDC_IN_SELECTION_CHECK);
	showFindDlgItem(IDC_2_BUTTONS_MODE);
	bool is2ButtonMode = isCheckedOrNot(IDC_2_BUTTONS_MODE);
	showFindDlgItem(IDOK, !is2ButtonMode);
	showFindDlgItem(IDC_FINDPREV, is2ButtonMode);
	showFindDlgItem(IDC_FINDNEXT, is2ButtonMode);


	// find controls
	showFindDlgItem(IDC_FINDALL_OPENEDFILES, !isEnable);
	showFindDlgItem(IDCCOUNTALL, !isEnable);
	showFindDlgItem(IDC_FINDALL_CURRENTFILE, !isEnable);

	gotoCorrectTab();
	calcAndSetCtrlsPos(_currentStatus);

	wchar_t label[MAX_PATH] = { '\0' };
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);

	setDefaultButton(IDOK);

	hideOrShowCtrl4reduceOrNormalMode(_currentStatus);
}

void FindReplaceDlg::enableMarkAllControls(bool isEnable)
{
	showFindDlgItem(IDCMARKALL, isEnable);
	showFindDlgItem(IDC_MARKLINE_CHECK, isEnable);
	showFindDlgItem(IDC_PURGE_CHECK, isEnable);
	showFindDlgItem(IDC_CLEAR_ALL, isEnable);
	showFindDlgItem(IDC_IN_SELECTION_CHECK, isEnable);
	showFindDlgItem(IDC_COPY_MARKED_TEXT, isEnable);
}

void FindReplaceDlg::enableFindInFilesControls(bool isEnable, bool projectPanels)
{
	// Hide Items
	showFindDlgItem(IDC_BACKWARDDIRECTION, !isEnable);
	showFindDlgItem(IDWRAP, !isEnable);
	showFindDlgItem(IDCCOUNTALL, !isEnable);
	showFindDlgItem(IDC_FINDALL_OPENEDFILES, !isEnable);
	showFindDlgItem(IDC_FINDALL_CURRENTFILE, !isEnable);

	if (isEnable)
	{
		showFindDlgItem(IDC_2_BUTTONS_MODE, false);
		showFindDlgItem(IDOK, false);
		showFindDlgItem(IDC_FINDPREV, false);
		showFindDlgItem(IDC_FINDNEXT, false);
	}
	else
	{
		showFindDlgItem(IDC_2_BUTTONS_MODE);
		bool is2ButtonMode = isCheckedOrNot(IDC_2_BUTTONS_MODE);
		showFindDlgItem(IDOK, !is2ButtonMode);
		showFindDlgItem(IDC_FINDPREV, is2ButtonMode);
		showFindDlgItem(IDC_FINDNEXT, is2ButtonMode);
	}

	showFindDlgItem(IDC_MARKLINE_CHECK, !isEnable);
	showFindDlgItem(IDC_PURGE_CHECK, !isEnable);
	showFindDlgItem(IDC_IN_SELECTION_CHECK, !isEnable);
	showFindDlgItem(IDC_CLEAR_ALL, !isEnable);
	showFindDlgItem(IDCMARKALL, !isEnable);
	showFindDlgItem(IDC_COPY_MARKED_TEXT, !isEnable);

	showFindDlgItem(IDREPLACE, !isEnable);
	showFindDlgItem(IDC_REPLACEINSELECTION, !isEnable);
	showFindDlgItem(IDREPLACEALL, !isEnable);
	showFindDlgItem(IDC_REPLACE_OPENEDFILES, !isEnable);

	// Show Items
	if (isEnable)
	{
		showFindDlgItem(ID_STATICTEXT_REPLACE);
		showFindDlgItem(IDREPLACEWITH);
		showFindDlgItem(IDD_FINDREPLACE_SWAP_BUTTON);
	}
	showFindDlgItem(IDD_FINDINFILES_REPLACEINFILES, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_REPLACEINPROJECTS, isEnable && projectPanels);
	showFindDlgItem(IDD_FINDINFILES_FILTERS_STATIC, isEnable);
	showFindDlgItem(IDD_FINDINFILES_FILTERS_COMBO, isEnable);
	showFindDlgItem(IDD_FINDINFILES_DIR_STATIC, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_DIR_COMBO, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_BROWSE_BUTTON, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_FIND_BUTTON, isEnable);
	showFindDlgItem(IDD_FINDINFILES_RECURSIVE_CHECK, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_INHIDDENDIR_CHECK, isEnable && (!projectPanels));
	showFindDlgItem(IDD_FINDINFILES_PROJECT1_CHECK, isEnable && projectPanels);
	showFindDlgItem(IDD_FINDINFILES_PROJECT2_CHECK, isEnable && projectPanels);
	showFindDlgItem(IDD_FINDINFILES_PROJECT3_CHECK, isEnable && projectPanels);
	showFindDlgItem(IDD_FINDINFILES_SETDIRFROMDOC_BUTTON, isEnable && (!projectPanels));
}

void FindReplaceDlg::getPatterns(vector<wstring> & patternVect)
{
	cutString(_env->_filters.c_str(), patternVect);
}

void FindReplaceDlg::getAndValidatePatterns(vector<wstring> & patternVect)
{
	getPatterns(patternVect);
	if (patternVect.size() == 0)
	{
		setFindInFilesDirFilter(NULL, L"*.*");
		getPatterns(patternVect);
	}
	else if (allPatternsAreExclusion(patternVect))
	{
		patternVect.insert(patternVect.begin(), L"*.*");
	}
}

void FindReplaceDlg::saveInMacro(size_t cmd, int cmdType)
{
	int booleans = 0;
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_INIT, 0);
	::SendMessage(_hParent, WM_FRSAVE_STR, IDFINDWHAT,  reinterpret_cast<LPARAM>(cmd == IDC_CLEAR_ALL ? "" : wstring2string(_options._str2Search, CP_UTF8).c_str()));
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
		::SendMessage(_hParent, WM_FRSAVE_STR, IDREPLACEWITH, reinterpret_cast<LPARAM>(wstring2string(_options._str4Replace, CP_UTF8).c_str()));
	if (cmdType & FR_OP_FIF)
	{
		::SendMessage(_hParent, WM_FRSAVE_STR, IDD_FINDINFILES_DIR_COMBO, reinterpret_cast<LPARAM>(wstring2string(_options._directory, CP_UTF8).c_str()));
		::SendMessage(_hParent, WM_FRSAVE_STR, IDD_FINDINFILES_FILTERS_COMBO, reinterpret_cast<LPARAM>(wstring2string(_options._filters, CP_UTF8).c_str()));
		booleans |= _options._isRecursive?IDF_FINDINFILES_RECURSIVE_CHECK:0;
		booleans |= _options._isInHiddenDir?IDF_FINDINFILES_INHIDDENDIR_CHECK:0;
	}
	if (cmdType & FR_OP_FIP)
	{
		::SendMessage(_hParent, WM_FRSAVE_STR, IDD_FINDINFILES_FILTERS_COMBO, reinterpret_cast<LPARAM>(wstring2string(_options._filters, CP_UTF8).c_str()));
		booleans |= _options._isProjectPanel_1?IDF_FINDINFILES_PROJECT1_CHECK:0;
		booleans |= _options._isProjectPanel_2?IDF_FINDINFILES_PROJECT2_CHECK:0;
		booleans |= _options._isProjectPanel_3?IDF_FINDINFILES_PROJECT3_CHECK:0;
	}
	else if (!(cmdType & FR_OP_GLOBAL))
	{
		booleans |= _options._isInSelection?IDF_IN_SELECTION_CHECK:0;
		booleans |= _options._isWrapAround?IDF_WRAP:0;
		booleans |= _options._whichDirection?IDF_WHICH_DIRECTION:0;
	}
	else if (cmdType == FR_OP_FIND + FR_OP_GLOBAL)
	{
		// find all in curr doc can be limited by selected text
		booleans |= _options._isInSelection ? IDF_IN_SELECTION_CHECK : 0;
	}
	if (cmd == IDC_CLEAR_ALL)
	{
		booleans = _options._doMarkLine ? IDF_MARKLINE_CHECK : 0;
		booleans |= _options._isInSelection ? IDF_IN_SELECTION_CHECK : 0;
	}
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_BOOLEANS, booleans);
	::SendMessage(_hParent, WM_FRSAVE_INT, IDC_FRCOMMAND_EXEC, cmd);
}

void FindReplaceDlg::setStatusbarMessage(const wstring & msg, FindStatus staus, const wstring& tooltipMsg)
{
	if (_statusbarTooltipWnd)
	{
		::DestroyWindow(_statusbarTooltipWnd);
		_statusbarTooltipWnd = nullptr;
	}

	_statusbarTooltipMsg = tooltipMsg;

	if (staus == FSNotFound)
	{
		if (!NppParameters::getInstance().getNppGUI()._muteSounds)
			::MessageBeep(0xFFFFFFFF);

		FLASHWINFO flashInfo{};
		flashInfo.cbSize = sizeof(FLASHWINFO);
		flashInfo.hwnd = isVisible()?_hSelf:GetParent(_hSelf);
		flashInfo.uCount = 3;
		flashInfo.dwTimeout = 100;
		flashInfo.dwFlags = FLASHW_ALL;
		FlashWindowEx(&flashInfo);
	}
	else if (staus == FSTopReached || staus == FSEndReached)
	{
		if (!isVisible())
		{
			FLASHWINFO flashInfo{};
			flashInfo.cbSize = sizeof(FLASHWINFO);
			flashInfo.hwnd = GetParent(_hSelf);
			flashInfo.uCount = 2;
			flashInfo.dwTimeout = 100;
			flashInfo.dwFlags = FLASHW_ALL;
			FlashWindowEx(&flashInfo);
		}
	}

	if (isVisible())
	{
		_statusbarFindStatus = staus;
		_statusBar.setOwnerDrawText(msg.c_str());
	}
}

void FindReplaceDlg::setStatusbarMessageWithRegExprErr(ScintillaEditView* pEditView)
{
	if (!pEditView)
		return;

	char msg[511] {};
	pEditView->execute(SCI_GETBOOSTREGEXERRMSG, _countof(msg), reinterpret_cast<LPARAM>(msg));

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	std::wstring result = pNativeSpeaker->getLocalizedStrFromID("find-status-invalid-re", L"Find: Invalid Regular Expression");
	string s = msg;
	
	setStatusbarMessage(result, FSNotFound, string2wstring(s, CP_UTF8));
}

wstring FindReplaceDlg::getScopeInfoForStatusBar(FindOption const *pFindOpt) const
{
	wstring scope;

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	if (pFindOpt->_isInSelection)
	{
		scope += pNativeSpeaker->getLocalizedStrFromID("find-status-scope-selection", L"in selected text");
	}
	else if (pFindOpt->_isWrapAround)
	{
		scope += pNativeSpeaker->getLocalizedStrFromID("find-status-scope-all", L"in entire file");
	}
	else if (pFindOpt->_whichDirection == DIR_UP)
	{
		scope += pNativeSpeaker->getLocalizedStrFromID("find-status-scope-backward", L"from start-of-file to caret");
	}
	else
	{
		scope += pNativeSpeaker->getLocalizedStrFromID("find-status-scope-forward", L"from caret to end-of-file");
	}

	return scope;
}

void FindReplaceDlg::execSavedCommand(int cmd, uptr_t intValue, const wstring& stringValue)
{
	try
	{
		switch (cmd)
		{
			case IDC_FRCOMMAND_INIT:
				_env = new FindOption;
				break;
			case IDFINDWHAT:
				_env->_str2Search = stringValue;
				break;
			case IDC_FRCOMMAND_BOOLEANS:
				_env->_isWholeWord = ((intValue & IDF_WHOLEWORD) > 0);
				_env->_isMatchCase = ((intValue & IDF_MATCHCASE) > 0);
				_env->_isRecursive = ((intValue & IDF_FINDINFILES_RECURSIVE_CHECK) > 0);
				_env->_isInHiddenDir = ((intValue & IDF_FINDINFILES_INHIDDENDIR_CHECK) > 0);
				_env->_isProjectPanel_1 = ((intValue & IDF_FINDINFILES_PROJECT1_CHECK) > 0);
				_env->_isProjectPanel_2 = ((intValue & IDF_FINDINFILES_PROJECT2_CHECK) > 0);
				_env->_isProjectPanel_3 = ((intValue & IDF_FINDINFILES_PROJECT3_CHECK) > 0);
				_env->_doPurge = ((intValue & IDF_PURGE_CHECK) > 0);
				_env->_doMarkLine = ((intValue & IDF_MARKLINE_CHECK) > 0);
				_env->_isInSelection = ((intValue & IDF_IN_SELECTION_CHECK) > 0);
				_env->_isWrapAround = ((intValue & IDF_WRAP) > 0);
				_env->_whichDirection = ((intValue & IDF_WHICH_DIRECTION) > 0);
				_env->_dotMatchesNewline = ((intValue & IDF_REDOTMATCHNL) > 0);
				break;
			case IDNORMAL:
				_env->_searchType = static_cast<SearchType>(intValue);
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
				NppParameters& nppParamInst = NppParameters::getInstance();
				switch (intValue)
				{
					case IDOK:
						if ((_env->_whichDirection == DIR_UP) && (_env->_searchType == FindRegex) && !nppParamInst.regexBackward4PowerUser())
						{
							// regex upward search is disabled
							// this macro step could have been recorded in an earlier version before it was not allowed, or hand-edited
							// make this a no-action macro step

							regexBackwardMsgBox();
						}
						else
						{
							nppParamInst._isFindReplacing = true;
							processFindNext(_env->_str2Search.c_str());
							nppParamInst._isFindReplacing = false;
						}
						break;

					case IDC_FINDNEXT:
						// IDC_FINDNEXT will not be recorded into new macros recorded with 7.8.5 and later
						// stay playback compatible with 7.5.5 - 7.8.4 where IDC_FINDNEXT was allowed but unneeded/undocumented
						nppParamInst._isFindReplacing = true;
						_env->_whichDirection = DIR_DOWN;
						processFindNext(_env->_str2Search.c_str());
						nppParamInst._isFindReplacing = false;
						break;

					case IDC_FINDPREV:
						// IDC_FINDPREV will not be recorded into new macros recorded with 7.8.5 and later
						// stay playback compatible with 7.5.5 - 7.8.4 where IDC_FINDPREV was allowed but unneeded/undocumented
						if (_env->_searchType == FindRegex && !nppParamInst.regexBackward4PowerUser())
						{
							// regex upward search is disabled
							// this macro step could have been recorded in an earlier version before it was not allowed, or hand-edited
							// make this a no-action macro step

							regexBackwardMsgBox();
						}
						else
						{
							_env->_whichDirection = DIR_UP;
							nppParamInst._isFindReplacing = true;
							processFindNext(_env->_str2Search.c_str());
							nppParamInst._isFindReplacing = false;
						}
						break;

					case IDREPLACE:
						if ((_env->_whichDirection == DIR_UP) && (_env->_searchType == FindRegex && !nppParamInst.regexBackward4PowerUser()))
						{
							// regex upward search is disabled
							// this macro step could have been recorded in an earlier version before it was disabled, or hand-edited
							// make this a no-action macro step

							regexBackwardMsgBox();
						}
						else
						{
							nppParamInst._isFindReplacing = true;
							processReplace(_env->_str2Search.c_str(), _env->_str4Replace.c_str(), _env);
							nppParamInst._isFindReplacing = false;
						}
						break;
					case IDC_FINDALL_OPENEDFILES:
						nppParamInst._isFindReplacing = true;
						findAllIn(ALL_OPEN_DOCS);
						nppParamInst._isFindReplacing = false;
						break;
					case IDC_FINDALL_CURRENTFILE:
						nppParamInst._isFindReplacing = true;
						findAllIn(_env->_isInSelection ? CURR_DOC_SELECTION : CURRENT_DOC);
						nppParamInst._isFindReplacing = false;
						break;
					case IDC_REPLACE_OPENEDFILES:
					{
						NppParameters& nppParam = NppParameters::getInstance();
						const NppGUI& nppGui = nppParam.getNppGUI();
						if (!nppGui._confirmReplaceInAllOpenDocs || replaceInOpenDocsConfirmCheck())
						{
							nppParamInst._isFindReplacing = true;
							replaceAllInOpenedDocs();
							nppParamInst._isFindReplacing = false;
						}
						break;
					}
					case IDD_FINDINFILES_FIND_BUTTON:
						nppParamInst._isFindReplacing = true;
						findAllIn(FILES_IN_DIR);
						nppParamInst._isFindReplacing = false;
						break;
					case IDD_FINDINFILES_FINDINPROJECTS:
						nppParamInst._isFindReplacing = true;
						findAllIn(FILES_IN_PROJECTS);
						nppParamInst._isFindReplacing = false;
						break;

					case IDD_FINDINFILES_REPLACEINFILES:
					{
						if (replaceInFilesConfirmCheck(_env->_directory, _env->_filters))
						{
							nppParamInst._isFindReplacing = true;
							::SendMessage(_hParent, WM_REPLACEINFILES, 0, 0);
							nppParamInst._isFindReplacing = false;
						}
						break;
					}

					case IDD_FINDINFILES_REPLACEINPROJECTS:
					{
						if (replaceInProjectsConfirmCheck())
						{
							nppParamInst._isFindReplacing = true;
							::SendMessage(_hParent, WM_REPLACEINPROJECTS, 0, 0);
							nppParamInst._isFindReplacing = false;
						}
						break;
					}

					case IDREPLACEALL:
					{
						nppParamInst._isFindReplacing = true;
						(*_ppEditView)->execute(SCI_BEGINUNDOACTION);
						int nbReplaced = processAll(ProcessReplaceAll, _env);
						(*_ppEditView)->execute(SCI_ENDUNDOACTION);
						nppParamInst._isFindReplacing = false;

						if (nbReplaced == FIND_INVALID_REGULAR_EXPRESSION)
						{	
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							wstring result;
							NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

							if (nbReplaced == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-replaceall-1-replaced", L"Replace All: 1 occurrence was replaced");
							}
							else //if (nbReplaced == 0 || nbReplaced > 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-replaceall-nb-replaced", L"Replace All: $INT_REPLACE$ occurrences were replaced");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbReplaced));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(_env);

							setStatusbarMessage(result, FSMessage);
						}

						break;
					}

					case IDCCOUNTALL:
					{
						int nbCounted = processAll(ProcessCountAll, _env);

						if (nbCounted == FIND_INVALID_REGULAR_EXPRESSION)
						{
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							wstring result;
							NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

							if (nbCounted == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-count-1-match", L"Count: 1 match");
							}
							else
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-count-nb-matches", L"Count: $INT_REPLACE$ matches");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbCounted));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(_env);

							setStatusbarMessage(result, FSMessage);
						}
						break;
					}

					case IDCMARKALL:
					{
						nppParamInst._isFindReplacing = true;
						int nbMarked = processAll(ProcessMarkAll, _env);
						nppParamInst._isFindReplacing = false;

						if (nbMarked == FIND_INVALID_REGULAR_EXPRESSION)
						{
							setStatusbarMessageWithRegExprErr(*_ppEditView);
						}
						else
						{
							wstring result;
							NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
							if (nbMarked == 1)
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-mark-1-match", L"Mark: 1 match");
							}
							else
							{
								result = pNativeSpeaker->getLocalizedStrFromID("find-status-mark-nb-matches", L"Mark: $INT_REPLACE$ matches");
								result = stringReplace(result, L"$INT_REPLACE$", std::to_wstring(nbMarked));
							}
							result += L" ";
							result += getScopeInfoForStatusBar(_env);

							setStatusbarMessage(result, FSMessage);
						}

						break;
					}

					case IDC_CLEAR_ALL:
					{
						clearMarks(*_env);
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
	catch (const std::runtime_error& err)
	{
		MessageBoxA(NULL, err.what(), "Play Macro Exception", MB_OK);
	}
}

void FindReplaceDlg::clearMarks(const FindOption& opt)
{
	if (opt._isInSelection)
	{
		Sci_CharacterRangeFull cr = (*_ppEditView)->getSelection();

		intptr_t startPosition = cr.cpMin;
		intptr_t endPosition = cr.cpMax;

		(*_ppEditView)->execute(SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_FOUND_STYLE);
		(*_ppEditView)->execute(SCI_INDICATORCLEARRANGE, startPosition, endPosition - startPosition);

		if (opt._doMarkLine)
		{
			auto lineNumber = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, startPosition);
			auto lineNumberEnd = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, endPosition - 1);

			for (auto i = lineNumber; i <= lineNumberEnd; ++i)
			{
				auto state = (*_ppEditView)->execute(SCI_MARKERGET, i);

				if (state & (1 << MARK_BOOKMARK))
					(*_ppEditView)->execute(SCI_MARKERDELETE, i, MARK_BOOKMARK);
			}
		}
	}
	else
	{
		(*_ppEditView)->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
		if (opt._doMarkLine)
		{
			(*_ppEditView)->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
		}
	}

	setStatusbarMessage(L"", FSNoMessage);
}

void FindReplaceDlg::setFindInFilesDirFilter(const wchar_t *dir, const wchar_t *filters)
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

void FindReplaceDlg::enableProjectCheckmarks()
{
	NppParameters& nppParams = NppParameters::getInstance();
	FindHistory & findHistory = nppParams.getFindHistory();
	HMENU hMenu = (HMENU) ::SendMessage (_hParent, NPPM_INTERNAL_GETMENU, 0, 0);
	if (hMenu)
	{
		int   idm [3] = {IDM_VIEW_PROJECT_PANEL_1, IDM_VIEW_PROJECT_PANEL_2, IDM_VIEW_PROJECT_PANEL_3};
		int   idd [3] = {IDD_FINDINFILES_PROJECT1_CHECK, IDD_FINDINFILES_PROJECT2_CHECK, IDD_FINDINFILES_PROJECT3_CHECK};
		bool *opt [3] = {&_options._isProjectPanel_1, &_options._isProjectPanel_2, &_options._isProjectPanel_3};
		bool *hst [3] = {&findHistory._isFifProjectPanel_1, &findHistory._isFifProjectPanel_2, &findHistory._isFifProjectPanel_3};
		bool enable = false;
		for (int i = 0; i < 3; i++)
		{
			UINT s = GetMenuState (hMenu, idm [i], MF_BYCOMMAND);
			if (s != ((UINT)-1))
			{
				if (s & MF_CHECKED)
				{
					enableFindDlgItem (idd [i], true);
					if (BST_CHECKED == ::SendDlgItemMessage(_hSelf, idd [i], BM_GETCHECK, 0, 0))
						enable = true;
				}
				else
				{
					*opt [i] = 0;
					*hst [i] = 0;
					::SendDlgItemMessage(_hSelf, idd [i], BM_SETCHECK, 0, 0);
					enableFindDlgItem (idd [i], false);
				}
			}
		}
		enableFindDlgItem (IDD_FINDINFILES_FIND_BUTTON, enable || (_currentStatus != FINDINPROJECTS_DLG));
		enableFindDlgItem (IDD_FINDINFILES_REPLACEINPROJECTS, enable);
	}
}

void FindReplaceDlg::setProjectCheckmarks(FindHistory *findHistory, int msk)
{
	_options._isProjectPanel_1 = (msk & 1) ? true : false;
	_options._isProjectPanel_2 = (msk & 2) ? true : false;
	_options._isProjectPanel_3 = (msk & 4) ? true : false;
	FindHistory *fh = findHistory;
	if (! fh)
	{
		NppParameters& nppParams = NppParameters::getInstance();
		fh = & nppParams.getFindHistory();
	}

	if (fh)
	{
		fh->_isFifProjectPanel_1 = _options._isProjectPanel_1;
		fh->_isFifProjectPanel_2 = _options._isProjectPanel_2;
		fh->_isFifProjectPanel_3 = _options._isProjectPanel_3;
	}
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT1_CHECK, BM_SETCHECK, _options._isProjectPanel_1, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT2_CHECK, BM_SETCHECK, _options._isProjectPanel_2, 0);
	::SendDlgItemMessage(_hSelf, IDD_FINDINFILES_PROJECT3_CHECK, BM_SETCHECK, _options._isProjectPanel_3, 0);

	if (_currentStatus == FINDINPROJECTS_DLG)
	{
		enableFindDlgItem (IDD_FINDINFILES_FIND_BUTTON, msk != 0);
		enableFindDlgItem (IDD_FINDINFILES_REPLACEINPROJECTS, msk != 0);
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

	_options._whichDirection = isCheckedOrNot(IDC_BACKWARDDIRECTION) ? DIR_UP : DIR_DOWN;

	_options._isRecursive = isCheckedOrNot(IDD_FINDINFILES_RECURSIVE_CHECK);
	_options._isInHiddenDir = isCheckedOrNot(IDD_FINDINFILES_INHIDDENDIR_CHECK);
	_options._isProjectPanel_1 =  isCheckedOrNot(IDD_FINDINFILES_PROJECT1_CHECK);
	_options._isProjectPanel_2 =  isCheckedOrNot(IDD_FINDINFILES_PROJECT2_CHECK);
	_options._isProjectPanel_3 =  isCheckedOrNot(IDD_FINDINFILES_PROJECT3_CHECK);
}

void FindInFinderDlg::doDialog(Finder *launcher, bool isRTL)
{
	_pFinder2Search = launcher;
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(IDD_FINDINFINDER_DLG, &pMyDlgTemplate);
		::DialogBoxIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_FINDINFINDER_DLG), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));

}

void FindReplaceDlg::doDialog(DIALOG_TYPE whichType, bool isRTL, bool toShow)
{
	if (!isCreated())
	{
		_isRTL = isRTL;
		create(IDD_FIND_REPLACE_DLG, isRTL, true, toShow);

		::EnableMenuItem(::GetMenu(_hParent), IDM_SEARCH_FINDNEXT, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(::GetMenu(_hParent), IDM_SEARCH_FINDPREV, MF_BYCOMMAND | MF_ENABLED);
	}

	if (whichType == FINDINFILES_DLG)
		enableFindInFilesFunc();
	else if (whichType == FINDINPROJECTS_DLG)
		enableFindInProjectsFunc();
	else if (whichType == MARK_DLG)
		enableMarkFunc();
	else
		enableReplaceFunc(whichType == REPLACE_DLG);

	::SetFocus(toShow ? ::GetDlgItem(_hSelf, IDFINDWHAT) : (*_ppEditView)->getHSelf());
	display(toShow, true);
}

LRESULT FAR PASCAL FindReplaceDlg::finderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN && (wParam == VK_DELETE || wParam == VK_RETURN || wParam == VK_ESCAPE))
	{
		ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		Finder *pFinder = (Finder *)(::GetWindowLongPtr(pScint->getHParent(), GWLP_USERDATA));
		if (wParam == VK_RETURN)
		{
			std::pair<intptr_t, intptr_t> newPos = pFinder->gotoFoundLine();

			auto currentPos = pFinder->_scintView.execute(SCI_GETCURRENTPOS);
			intptr_t lno = pFinder->_scintView.execute(SCI_LINEFROMPOSITION, currentPos);
			intptr_t lineStartAbsPos = pFinder->_scintView.execute(SCI_POSITIONFROMLINE, lno);
			intptr_t lineEndAbsPos = pFinder->_scintView.execute(SCI_GETLINEENDPOSITION, lno);

			intptr_t begin = newPos.first + lineStartAbsPos;
			intptr_t end = newPos.second + lineStartAbsPos;

			if (end > lineEndAbsPos)
				end = lineEndAbsPos;

			pFinder->_scintView.execute(SCI_SETSEL, begin, end);
			pFinder->_scintView.execute(SCI_SCROLLRANGE, begin, end);
		}
		else if (wParam == VK_ESCAPE)
			pFinder->display(false);
		else // VK_DELETE
			pFinder->deleteResult();
		return 0;
	}
	else
		// Call default (original) window procedure
		return CallWindowProc(originalFinderProc, hwnd, message, wParam, lParam);
}

LRESULT FAR PASCAL FindReplaceDlg::comboEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndCombo = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	bool isDropped = ::SendMessage(hwndCombo, CB_GETDROPPEDSTATE, 0, 0) != 0;

	static wchar_t draftString[FINDREPLACE_MAXLENGTH]{};

	if (isDropped && (message == WM_KEYDOWN) && (wParam == VK_DELETE))
	{
		auto curSel = ::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
		if (curSel != CB_ERR)
		{
			auto itemsRemaining = ::SendMessage(hwndCombo, CB_DELETESTRING, curSel, 0);
			// if we close the dropdown and reopen it, it will be correctly-sized for remaining items
			::SendMessage(hwndCombo, CB_SHOWDROPDOWN, FALSE, 0);
			if (itemsRemaining > 0)
			{
				if (itemsRemaining == curSel)
				{
					--curSel;
				}
				::SendMessage(hwndCombo, CB_SETCURSEL, curSel, 0);
				::SendMessage(hwndCombo, CB_SHOWDROPDOWN, TRUE, 0);
			}
			return 0;
		}
	}
	else if (message == WM_CHAR && wParam == 0x7F) // ASCII "DEL" (Ctrl+Backspace)
	{
		delLeftWordInEdit(hwnd);
		return 0;
	}
	else if (message == WM_SETFOCUS)
	{
		draftString[0] = '\0';
	}
	else if ((message == WM_KEYDOWN) && (wParam == VK_DOWN) && (::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0) == CB_ERR))
	{
		// down key on unselected combobox item -> store current edit text as draft
		::SendMessage(hwndCombo, WM_GETTEXT, FINDREPLACE_MAXLENGTH - 1, reinterpret_cast<LPARAM>(draftString));
	}
	else if ((message == WM_KEYDOWN) && (wParam == VK_UP) && (::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0) == CB_ERR))
	{
		// up key on unselected combobox item -> no change but select current edit text
		::SendMessage(hwndCombo, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
		return 0;
	}
	else if ((message == WM_KEYDOWN) && (wParam == VK_UP) && (::SendMessage(hwndCombo, CB_GETCURSEL, 0, 0) == 0) && std::wcslen(draftString) > 0)
	{
		// up key on top selected combobox item -> restore draft to edit text
		::SendMessage(hwndCombo, CB_SETCURSEL, WPARAM(-1), 0);
		::SendMessage(hwndCombo, WM_SETTEXT, FINDREPLACE_MAXLENGTH - 1, reinterpret_cast<LPARAM>(draftString));
		::SendMessage(hwndCombo, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
		return 0;

	}
	return CallWindowProc(originalComboEditProc, hwnd, message, wParam, lParam);
}

void FindReplaceDlg::hideOrShowCtrl4reduceOrNormalMode(DIALOG_TYPE dlgT)
{
	bool isLessModeOn = NppParameters::getInstance().getNppGUI()._findWindowLessMode;
	if (dlgT == FIND_DLG)
	{
		for (int id : _reduce2hide_find)
			::ShowWindow(::GetDlgItem(_hSelf, id), isLessModeOn ? SW_HIDE : SW_SHOW);
	}
	else if (dlgT == REPLACE_DLG)
	{
		for (int id : _reduce2hide_findReplace)
			::ShowWindow(::GetDlgItem(_hSelf, id), isLessModeOn ? SW_HIDE : SW_SHOW);
	}
	else if (dlgT == FINDINFILES_DLG)
	{
		for (int id : _reduce2hide_fif)
			::ShowWindow(::GetDlgItem(_hSelf, id), isLessModeOn ? SW_HIDE : SW_SHOW);
	}
	else if (dlgT == FINDINPROJECTS_DLG)
	{
		for (int id : _reduce2hide_fip)
			::ShowWindow(::GetDlgItem(_hSelf, id), isLessModeOn ? SW_HIDE : SW_SHOW);
	}
	else // MARK_DLG
	{
		for (int id : _reduce2hide_mark)
			::ShowWindow(::GetDlgItem(_hSelf, id), isLessModeOn ? SW_HIDE : SW_SHOW);
	}
}

void FindReplaceDlg::calcAndSetCtrlsPos(DIALOG_TYPE dlgT, bool fromColBtn)
{
	const bool isNotLessMode = !NppParameters::getInstance().getNppGUI()._findWindowLessMode;

	RECT rcBtn2ndPos{};
	RECT rcBtn3rdPos{};

	getMappedChildRect(IDREPLACE, rcBtn2ndPos);
	getMappedChildRect(IDREPLACEALL, rcBtn3rdPos);
	
	const LONG btnGap = rcBtn3rdPos.top - rcBtn2ndPos.bottom;
	constexpr UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	if (isNotLessMode)
	{
		const LONG btnGapDbl = 2 * btnGap;
		const LONG btnGapOneHalf = 3 * btnGap / 2;

		LONG yFrame = -btnGapOneHalf;
		LONG hFrame = btnGapDbl;
		int ySelCheck = rcBtn3rdPos.top;
		RECT rcToUse{};

		switch (dlgT)
		{
			case FIND_DLG:
			{
				getMappedChildRect(IDC_FINDALL_OPENEDFILES, rcToUse);

				RECT rcBtn3rdPosLarge{};
				getMappedChildRect(IDC_FINDALL_CURRENTFILE, rcBtn3rdPosLarge);
				yFrame += rcBtn2ndPos.top;
				hFrame += (rcBtn3rdPosLarge.bottom - rcBtn2ndPos.top);
				break;
			}

			case REPLACE_DLG:
			{
				getMappedChildRect(IDC_REPLACE_OPENEDFILES, rcToUse);

				yFrame += rcBtn3rdPos.top;
				hFrame += (rcBtn3rdPos.bottom - rcBtn3rdPos.top);
				break;
			}

			case FINDINFILES_DLG:
			case FINDINPROJECTS_DLG:
			{
				rcToUse = rcBtn2ndPos;
				break;
			}

			case MARK_DLG:
			{

				RECT rcBtn1stPos{};
				getMappedChildRect(IDOK, rcBtn1stPos);
				yFrame += rcBtn1stPos.top;
				hFrame += (rcBtn2ndPos.bottom - rcBtn1stPos.top);
				ySelCheck = rcBtn2ndPos.top;

				rcToUse = rcBtn3rdPos;
				break;
			}
		}
		
		::SetWindowPos(::GetDlgItem(_hSelf, IDCANCEL), nullptr, rcToUse.left, (rcToUse.bottom + btnGap), 0, 0, SWP_NOSIZE | flags);

		if (dlgT == FIND_DLG || dlgT == REPLACE_DLG || dlgT == MARK_DLG)
		{
			RECT rcCheckBtn{};
			getMappedChildRect(IDC_IN_SELECTION_CHECK, rcCheckBtn);
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_IN_SELECTION_CHECK), nullptr, rcCheckBtn.left, ySelCheck + btnGap / 2, 0, 0, SWP_NOSIZE | flags);

			const LONG xFrame = rcCheckBtn.left - btnGapOneHalf;
			const LONG wFrame = (rcBtn2ndPos.right - rcCheckBtn.left) + btnGapDbl;
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_REPLACEINSELECTION), nullptr, xFrame, yFrame, wFrame, hFrame, flags);
		}
	}

	if (fromColBtn)
	{
		RECT rc2ModeCheck{};
		getMappedChildRect(IDC_2_BUTTONS_MODE, rc2ModeCheck);
		LONG yColBtn = btnGap / 2;
		if (isNotLessMode)
		{
			RECT rcSlider{};
			getMappedChildRect(IDC_PERCENTAGE_SLIDER, rcSlider);
			yColBtn += rcSlider.top;
		}
		else
		{
			yColBtn += rcBtn2ndPos.top;
		}
		::SetWindowPos(::GetDlgItem(_hSelf, IDD_RESIZE_TOGGLE_BUTTON), nullptr, rc2ModeCheck.left, yColBtn, 0, 0, SWP_NOSIZE | flags);
	}
}

void FindReplaceDlg::enableFindInFilesFunc()
{
	enableFindInFilesControls(true, false);
	_currentStatus = FINDINFILES_DLG;
	gotoCorrectTab();
	calcAndSetCtrlsPos(_currentStatus);
	wchar_t label[MAX_PATH]{};
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
	setDefaultButton(IDD_FINDINFILES_FIND_BUTTON);
	enableFindDlgItem (IDD_FINDINFILES_FIND_BUTTON, true);
	hideOrShowCtrl4reduceOrNormalMode(_currentStatus);
}

void FindReplaceDlg::enableFindInProjectsFunc()
{
	enableFindInFilesControls(true, true);
	_currentStatus = FINDINPROJECTS_DLG;
	gotoCorrectTab();
	calcAndSetCtrlsPos(_currentStatus);
	wchar_t label[MAX_PATH]{};
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
	setDefaultButton(IDD_FINDINFILES_FIND_BUTTON);
	bool enable = _options._isProjectPanel_1 || _options._isProjectPanel_2 || _options._isProjectPanel_3;
	enableFindDlgItem (IDD_FINDINFILES_FIND_BUTTON, enable);
	enableFindDlgItem (IDD_FINDINFILES_REPLACEINPROJECTS, enable);
	hideOrShowCtrl4reduceOrNormalMode(_currentStatus);
}

void FindReplaceDlg::enableMarkFunc()
{
	enableFindInFilesControls(false, false);
	enableMarkAllControls(true);

	// Replace controls to hide
	showFindDlgItem(ID_STATICTEXT_REPLACE, false);
	showFindDlgItem(IDREPLACE, false);
	showFindDlgItem(IDREPLACEWITH, false);
	showFindDlgItem(IDD_FINDREPLACE_SWAP_BUTTON, false);
	showFindDlgItem(IDREPLACEALL, false);
	showFindDlgItem(IDC_REPLACE_OPENEDFILES, false);

	// find controls to hide
	showFindDlgItem(IDC_FINDALL_OPENEDFILES, false);
	showFindDlgItem(IDCCOUNTALL, false);
	showFindDlgItem(IDC_FINDALL_CURRENTFILE, false);
	showFindDlgItem(IDOK, false);
	showFindDlgItem(IDC_2_BUTTONS_MODE, false);
	showFindDlgItem(IDC_FINDPREV, false);
	showFindDlgItem(IDC_FINDNEXT, false);

	_currentStatus = MARK_DLG;
	gotoCorrectTab();
	calcAndSetCtrlsPos(_currentStatus);

	wchar_t label[MAX_PATH]{};
	_tab.getCurrentTitle(label, MAX_PATH);
	::SetWindowText(_hSelf, label);
	setDefaultButton(IDCMARKALL);

	hideOrShowCtrl4reduceOrNormalMode(_currentStatus);
}

void FindReplaceDlg::combo2ExtendedMode(int comboID)
{
	HWND hFindCombo = ::GetDlgItem(_hSelf, comboID);
	if (!hFindCombo) return;

	wstring str2transform = getTextFromCombo(hFindCombo);

	// Count the number of character '\n' and '\r'
	size_t nbEOL = 0;
	size_t str2transformLen = lstrlen(str2transform.c_str());
	for (size_t i = 0 ; i < str2transformLen ; ++i)
	{
		if (str2transform[i] == '\r' || str2transform[i] == '\n')
			++nbEOL;
	}

	if (nbEOL)
	{
		wchar_t * newBuffer = new wchar_t[str2transformLen + nbEOL*2 + 1];
		int j = 0;
		for (size_t i = 0 ; i < str2transformLen ; ++i)
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

void FindReplaceDlg::drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	//printStr(L"OK"));
	COLORREF fgColor = black; // black by default
	PCTSTR ptStr =(PCTSTR)lpDrawItemStruct->itemData;
	NppParameters& nppParamInst = NppParameters::getInstance();
	
	if (_statusbarFindStatus == FSNotFound)
	{
		fgColor = nppParamInst.getFindDlgStatusMsgColor(0);
	}
	else if (_statusbarFindStatus == FSMessage)
	{
		fgColor = nppParamInst.getFindDlgStatusMsgColor(1);
	}
	else if (_statusbarFindStatus == FSTopReached || _statusbarFindStatus == FSEndReached)
	{
		fgColor = nppParamInst.getFindDlgStatusMsgColor(2);
	}
	else if (_statusbarFindStatus == FSNoMessage)
	{
		ptStr = L"";
	}

	if (NppDarkMode::isEnabled())
	{
		fgColor = NppDarkMode::getTextColor();

		if (_statusbarFindStatus == FSNotFound)
		{
			HLSColour hls(nppParamInst.getFindDlgStatusMsgColor(0));
			fgColor = hls.toRGB4DarkMod();
		}
		else if (_statusbarFindStatus == FSMessage)
		{
			HLSColour hls(nppParamInst.getFindDlgStatusMsgColor(1));
			fgColor = hls.toRGB4DarkMod();
		}
		else if (_statusbarFindStatus == FSTopReached || _statusbarFindStatus == FSEndReached)
		{
			HLSColour hls(nppParamInst.getFindDlgStatusMsgColor(2));
			fgColor = hls.toRGB4DarkMod();
		}
	}

	SetTextColor(lpDrawItemStruct->hDC, fgColor);
	::SetBkMode(lpDrawItemStruct->hDC, TRANSPARENT);

	RECT rect{};
	_statusBar.getClientRect(rect);

	if (NppDarkMode::isEnabled())
	{
		rect.left += 2;
	}

	::DrawText(lpDrawItemStruct->hDC, ptStr, lstrlen(ptStr), &rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

	if (_statusbarTooltipMsg.empty()) return;

	SIZE size{};
	::GetTextExtentPoint32(lpDrawItemStruct->hDC, ptStr, lstrlen(ptStr), &size);
	int s = (rect.bottom - rect.top) & 0x70; // limit s to available icon sizes and avoid uneven scalings
	if (s > 0)
	{
		if (_statusbarTooltipIcon && (_statusbarTooltipIconSize != s))
		{
			DestroyIcon (_statusbarTooltipIcon);
			_statusbarTooltipIcon = nullptr;
		}

		if (!_statusbarTooltipIcon)
			_statusbarTooltipIcon = (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_GET_INFO_FROM_TOOLTIP), IMAGE_ICON, s, s, 0);

		if (_statusbarTooltipIcon)
		{
			_statusbarTooltipIconSize = s;
			rect.left = rect.left + size.cx + s / 2;
			rect.top  = (rect.top + rect.bottom - s) / 2;
			DrawIconEx (lpDrawItemStruct->hDC, rect.left, rect.top, _statusbarTooltipIcon, s, s, 0, NULL, DI_NORMAL);
			if (!_statusbarTooltipWnd)
			{
				rect.right = rect.left + s;
				rect.bottom = rect.top + s;
				_statusbarTooltipWnd = CreateToolTipRect(1, _statusBar.getHSelf(), _hInst, const_cast<PTSTR>(_statusbarTooltipMsg.c_str()), rect);

				NppDarkMode::setDarkTooltips(_statusbarTooltipWnd, NppDarkMode::ToolTipsType::tooltip);
			}
		}
		else
		{
			_statusbarTooltipIconSize = 0;
		}
	}
}

bool FindReplaceDlg::replaceInFilesConfirmCheck(const wstring& directory, const wstring& fileTypes)
{
	bool confirmed = false;

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	wstring title = pNativeSpeaker->getLocalizedStrFromID("replace-in-files-confirm-title", L"Are you sure?");

	wstring msg = pNativeSpeaker->getLocalizedStrFromID("replace-in-files-confirm-directory", L"Are you sure you want to replace all occurrences in:");
	msg += L"\r\r";
	msg += directory;

	msg += L"\r\r";

	wstring msg2 = pNativeSpeaker->getLocalizedStrFromID("replace-in-files-confirm-filetype", L"For file type:");
	msg2 += L"\r\r";
	msg2 += fileTypes[0] ? fileTypes : L"*.*";

	msg += msg2;

	int res = ::MessageBox(NULL, msg.c_str(), title.c_str(), MB_OKCANCEL | MB_DEFBUTTON2 | MB_TASKMODAL);

	if (res == IDOK)
	{
		confirmed = true;
	}

	return confirmed;
}

bool FindReplaceDlg::replaceInProjectsConfirmCheck()
{
	bool confirmed = false;

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	wstring title = pNativeSpeaker->getLocalizedStrFromID("replace-in-projects-confirm-title", L"Are you sure?");

	wstring msg = pNativeSpeaker->getLocalizedStrFromID("replace-in-projects-confirm-message", L"Do you want to replace all occurrences in all documents in the selected Project Panel(s)?");
	int res = ::MessageBox(NULL, msg.c_str(), title.c_str(), MB_OKCANCEL | MB_DEFBUTTON2 | MB_TASKMODAL);

	if (res == IDOK)
	{
		confirmed = true;
	}

	return confirmed;
}

bool FindReplaceDlg::replaceInOpenDocsConfirmCheck(void)
{
	bool confirmed = false;

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	wstring title = pNativeSpeaker->getLocalizedStrFromID("replace-in-open-docs-confirm-title", L"Are you sure?");
	wstring msg = pNativeSpeaker->getLocalizedStrFromID("replace-in-open-docs-confirm-message", L"Are you sure you want to replace all occurrences in all open documents?");

	int res = ::MessageBox(NULL, msg.c_str(), title.c_str(), MB_OKCANCEL | MB_DEFBUTTON2 | MB_TASKMODAL);

	if (res == IDOK)
	{
		confirmed = true;
	}

	return confirmed;
}

wstring Finder::getHitsString(int count) const
{
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	wstring text = pNativeSpeaker->getLocalizedStrFromID("find-result-hits", L"");

	if (text.empty())
	{
		if (count == 1)
		{
			text = L"(1 hit)";
		}
		else
		{
			text = L"(";
			text += std::to_wstring(count);
			text += L" hits)";
		}
	}
	else
	{
		text = stringReplace(text, L"$INT_REPLACE$", std::to_wstring(count));
	}

	return text;
}

void Finder::addSearchLine(const wchar_t *searchName)
{
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	wstring str = pNativeSpeaker->getLocalizedStrFromID("find-result-title", L"Search");
	str += L" \"";
	str += searchName;
	str += L"\" \r\n";

	setFinderReadOnly(false);
	_scintView.addGenericText(str.c_str());
	setFinderReadOnly(true);
	_lastSearchHeaderPos = _scintView.execute(SCI_GETCURRENTPOS) - 2;

	_pMainFoundInfos->push_back(EmptyFoundInfo);
	_pMainMarkings->push_back(EmptySearchResultMarking);
	_previousLineNumber = -1;
}

void Finder::addFileNameTitle(const wchar_t * fileName)
{
	wstring str = L"  ";
	str += fileName;
	str += L"\r\n";

	setFinderReadOnly(false);
	_scintView.addGenericText(str.c_str());
	setFinderReadOnly(true);
	_lastFileHeaderPos = _scintView.execute(SCI_GETCURRENTPOS) - 2;

	_pMainFoundInfos->push_back(EmptyFoundInfo);
	_pMainMarkings->push_back(EmptySearchResultMarking);
	_previousLineNumber = -1;
}

void Finder::addFileHitCount(int count)
{
	wstring text = L" ";
	text += getHitsString(count);
	setFinderReadOnly(false);
	_scintView.insertGenericTextFrom(_lastFileHeaderPos, text.c_str());
	setFinderReadOnly(true);
	++_nbFoundFiles;
}

void Finder::addSearchResultInfo(int count, int countSearched, bool searchedEntireNotSelection, const FindOption* pFindOpt)
{
	wstring nbResStr = std::to_wstring(count);
	wstring nbFoundFilesStr = std::to_wstring(_nbFoundFiles);
	wstring nbSearchedFilesStr = std::to_wstring(countSearched);

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	wstring text = pNativeSpeaker->getLocalizedStrFromID(searchedEntireNotSelection ? "find-result-title-info" : "find-result-title-info-selections", L"");

	if (text.empty())
	{
		// create default output: "(1 hit in 1 file of 1 searched)"
		//  variation             "(1 hit in 2 files of 2 searched)"
		//  variation             "(2 hits in 1 file of 3 searched)"
		//  variation             "(0 hits in 0 files of 1 searched)"
		// selection variations:  "(1 hit in 1 selection of 1 searched)"     " (0 hits in 0 selections of 1 searched)"
		// line filter variation: "(1 hit in 1 file of 1 searched) - Line Filter Mode: only display the filtered results"

		wstring hitsIn = count == 1 ? L"hit" : L"hits";

		wstring fileOrSelection = searchedEntireNotSelection ? L"file" : L"selection";
		if (_nbFoundFiles != 1)
		{
			fileOrSelection += L"s";
		}

		text = L"(" + nbResStr + L" " + hitsIn + L" in " + nbFoundFilesStr + L" " +
			fileOrSelection + L" of " + nbSearchedFilesStr + L" searched" + L")";
	}
	else
	{
		text = stringReplace(text, L"$INT_REPLACE1$", nbResStr);
		text = stringReplace(text, L"$INT_REPLACE2$", nbFoundFilesStr);
		text = stringReplace(text, L"$INT_REPLACE3$", nbSearchedFilesStr);
	}

	wstring searchModeText;
	if (pFindOpt->_searchType == FindExtended)
	{
		searchModeText += pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-options-searchmode-extended", L"Extended");
	}
	else if (pFindOpt->_searchType == FindRegex)
	{
		searchModeText += pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-options-searchmode-regexp", L"RegEx");
		if (pFindOpt->_dotMatchesNewline) searchModeText += L".";
	}
	else
	{
		searchModeText += pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-options-searchmode-normal", L"Normal");
	}

	wstring searchOptionsText;
	if (pFindOpt->_isMatchCase)
	{
		searchOptionsText += pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-options-case", L"Case");
	}
	if (pFindOpt->_isWholeWord)
	{
		if (!searchOptionsText.empty()) searchOptionsText += L"/";
		searchOptionsText += pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-options-word", L"Word");
	}

	if (!searchOptionsText.empty()) searchModeText += L": ";
	
	text += L" [" + searchModeText + searchOptionsText + L"]";

	if (pFindOpt->_isMatchLineNumber)
	{
		wstring lineFilterModeInfo = pNativeSpeaker->getLocalizedStrFromID("find-result-title-info-extra", L" - Line Filter Mode: only display the filtered results");
		text += lineFilterModeInfo;
	}

	setFinderReadOnly(false);
	_scintView.insertGenericTextFrom(_lastSearchHeaderPos, text.c_str());
	setFinderReadOnly(true);
}

const char* Finder::foundLine(FoundInfo fi, SearchResultMarkingLine miLine, const wchar_t* foundline, size_t totalLineNumber)
{
	bool isRepeatedLine = false;

	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI& nppGUI = nppParam.getNppGUI();

	if (nppGUI._finderShowOnlyOneEntryPerFoundLine)
	{
		if (_previousLineNumber == -1)
		{
			_previousLineNumber = fi._lineNumber;
		}
		else if (_previousLineNumber == static_cast<intptr_t>(fi._lineNumber))
		{
			isRepeatedLine = true;
		}
		else // previousLine != fi._lineNumber
		{
			_previousLineNumber = fi._lineNumber;
		}
	}

	wstring headerStr = L"\t";
	headerStr += _prefixLineStr;
	headerStr += L" ";

	size_t totalLineNumberDigit = static_cast<size_t>(nbDigitsFromNbLines(totalLineNumber) + 1);
	size_t currentLineNumberDigit = static_cast<size_t>(nbDigitsFromNbLines(fi._lineNumber) + 1);

	wstring lineNumberStr = L"";
	lineNumberStr.append(totalLineNumberDigit - currentLineNumberDigit, ' ');
	lineNumberStr.append(std::to_wstring(fi._lineNumber));
	headerStr += lineNumberStr;
	headerStr += L": ";

	miLine._segmentPostions[0].first += headerStr.length();
	miLine._segmentPostions[0].second += headerStr.length();
	headerStr += foundline;
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const char* text2AddUtf8 = wmc.wchar2char(headerStr.c_str(), SC_CP_UTF8, &miLine._segmentPostions[0].first, &miLine._segmentPostions[0].second); // certainly utf8 here
	size_t text2AddUtf8Len = strlen(text2AddUtf8);

	// if current line is the repeated line of previous one, and settings make per found line show ONCE in the result even there are several found occurences in the same line, for:
	if ((isRepeatedLine && 
		((text2AddUtf8Len < SC_SEARCHRESULT_LINEBUFFERMAXLENGTH) ||                                                                              // 1. All displayed whole lines, in which it contains all the occurrences.
		((text2AddUtf8Len >= SC_SEARCHRESULT_LINEBUFFERMAXLENGTH && miLine._segmentPostions[0].second < SC_SEARCHRESULT_LINEBUFFERMAXLENGTH))))) // 2. or the cut lines but the segments are displayed. For the segments displayed beyond displayed (non-cut part), go to default mode.
	{
		// Add start and end markers into the previous line's info for colourizing 
		_pMainMarkings->back()._segmentPostions.push_back(std::pair<intptr_t, intptr_t>(miLine._segmentPostions[0].first, miLine._segmentPostions[0].second));
		_pMainFoundInfos->back()._ranges.push_back(fi._ranges.back());

		return "";
	}
	else // default mode: allow same found line has several entries in search result if the searched occurrence is matched several times in the same line
	{
		_pMainFoundInfos->push_back(fi);


		if (text2AddUtf8Len >= SC_SEARCHRESULT_LINEBUFFERMAXLENGTH)
		{
			const char* endOfLongLine = " ...\r\n"; // perfectly Utf8-encoded already
			size_t lenEndOfLongLine = strlen(endOfLongLine);
			size_t cut = SC_SEARCHRESULT_LINEBUFFERMAXLENGTH - lenEndOfLongLine - 1;

			while ((cut > 0) && (!Utf8::isValid(&text2AddUtf8[cut], (int)(text2AddUtf8Len - cut))))
				cut--;

			memcpy((void*)&text2AddUtf8[cut], endOfLongLine, lenEndOfLongLine + 1);
			text2AddUtf8Len = cut + lenEndOfLongLine;
		}

		_pMainMarkings->push_back(miLine);

		return text2AddUtf8;
	}
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
	for (auto&& path : getResultFilePaths(true))
	{
		::SendMessage(_hParent, WM_DOOPEN, 0, reinterpret_cast<LPARAM>(path.c_str()));
	}
}

void Finder::copyPathnames()
{
	wstring toClipboard;
	for (auto&& path : getResultFilePaths(true))
	{
		toClipboard += path + L"\r\n";
	}
	if (!toClipboard.empty())
	{
		if (!str2Clipboard(toClipboard, _hSelf))
		{
			assert(false);
			::MessageBox(NULL, L"Error placing pathnames into clipboard.", L"Notepad++", MB_ICONINFORMATION);
		}
	}
}

void Finder::wrapLongLinesToggle()
{
	_longLinesAreWrapped = !_longLinesAreWrapped;
	_scintView.wrap(_longLinesAreWrapped);

	if (!_canBeVolatiled)
	{
		// only remember this setting from the original finder
		NppParameters& nppParam = NppParameters::getInstance();
		NppGUI& nppGUI = nppParam.getNppGUI();
		nppGUI._finderLinesAreCurrentlyWrapped = _longLinesAreWrapped;
	}
}

void Finder::purgeToggle()
{
	_purgeBeforeEverySearch = !_purgeBeforeEverySearch;

	if (!_canBeVolatiled)
	{
		// only remember this setting from the original finder
		NppParameters& nppParam = NppParameters::getInstance();
		NppGUI& nppGUI = nppParam.getNppGUI();
		nppGUI._finderPurgeBeforeEverySearch = _purgeBeforeEverySearch;
	}
}

bool Finder::isLineActualSearchResult(const wstring & s) const
{
	// actual-search-result lines are the only type that start with a tab character
	// sample: "\tLine 123: xxxxxxHITxxxxxx"
	return (s.find(L"\t") == 0);
}

wstring & Finder::prepareStringForClipboard(wstring & s) const
{
	// Input: a string like "\tLine 3: search result".
	// Output: "search result"
	s = stringReplace(s, L"\r", L"");
	s = stringReplace(s, L"\n", L"");
	const auto firstColon = s.find(L':');
	if (firstColon == std::string::npos)
	{
		// Should never happen.
		assert(false);
		return s;
	}
	else
	{
		// Plus 2 in order to deal with ": ".
		s = s.substr(2 + firstColon);
		return s;
	}
}

void Finder::copy()
{
	const pair<size_t, size_t> lineRange = _scintView.getSelectionLinesRange();
	size_t fromLine = lineRange.first;
	size_t toLine = lineRange.second;

	if (_scintView.execute(SCI_GETSELECTIONEMPTY) || fromLine == toLine)
	{
		const int selectedLineFoldLevel = _scintView.execute(SCI_GETFOLDLEVEL, fromLine) & SC_FOLDLEVELNUMBERMASK;
		if (selectedLineFoldLevel != resultLevel)
		{
			// caret on Search "..." header line
			// or
			// caret is on a line with a pathname
			
			// locate the final resultLevel line under its parent grouping
			toLine = _scintView.execute(SCI_GETLASTCHILD, fromLine, selectedLineFoldLevel);
			const int toLineFoldLevel = _scintView.execute(SCI_GETFOLDLEVEL, toLine) & SC_FOLDLEVELNUMBERMASK;
			if (toLineFoldLevel != resultLevel)
			{
				return;  // the search had 0 hits, so no resultLevel lines, nothing to copy
			}
		}
	}

	std::vector<wstring> lines;
	wstring previousResultLineStr(L"");
	for (size_t line = fromLine; line <= toLine; ++line)
	{
		wstring lineStr = _scintView.getLine(line);
		if (isLineActualSearchResult(lineStr))
		{
			if (lineStr != previousResultLineStr)
			{
				previousResultLineStr = lineStr;
				lines.push_back(prepareStringForClipboard(lineStr));
			}
		}
	}
	wstring toClipboard;
	stringJoin(lines, L"\r\n", toClipboard);
	toClipboard += L"\r\n";
	if (!toClipboard.empty())
	{
		if (!str2Clipboard(toClipboard, _hSelf))
		{
			assert(false);
			::MessageBox(NULL, L"Error placing text in clipboard.", L"Notepad++", MB_ICONINFORMATION);
		}
	}
}

void Finder::beginNewFilesSearch()
{
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	_prefixLineStr = pNativeSpeaker->getLocalizedStrFromID("find-result-line-prefix", L"Line");

	// Use SCI_SETSEL(0, 0) instead of SCI_SETCURRENTPOS(0) to workaround
	// an eventual regression or a change of behaviour in Scintilla 4.4.6
	// ref: https://github.com/notepad-plus-plus/notepad-plus-plus/issues/9595#issuecomment-789824579
	//
	_scintView.execute(SCI_SETSEL, 0, 0);
	//_scintView.execute(SCI_SETCURRENTPOS, 0);

	_pMainFoundInfos = _pMainFoundInfos == &_foundInfos1 ? &_foundInfos2 : &_foundInfos1;
	_pMainMarkings = _pMainMarkings == &_markings1 ? &_markings2 : &_markings1;
	_nbFoundFiles = 0;

	// fold all old searches (1st level only)
	_scintView.foldLevel(searchHeaderLevel - SC_FOLDLEVELBASE, fold_collapse);
}

void Finder::finishFilesSearch(int count, int searchedCount, bool searchedEntireNotSelection, const FindOption* pFindOpt)
{
	std::vector<FoundInfo>* _pOldFoundInfos;
	std::vector<SearchResultMarkingLine>* _pOldMarkings;
	_pOldFoundInfos = _pMainFoundInfos == &_foundInfos1 ? &_foundInfos2 : &_foundInfos1;
	_pOldMarkings = _pMainMarkings == &_markings1 ? &_markings2 : &_markings1;

	_pOldFoundInfos->insert(_pOldFoundInfos->begin(), _pMainFoundInfos->begin(), _pMainFoundInfos->end());
	_pOldMarkings->insert(_pOldMarkings->begin(), _pMainMarkings->begin(), _pMainMarkings->end());
	_pMainFoundInfos->clear();
	_pMainMarkings->clear();
	_pMainFoundInfos = _pOldFoundInfos;
	_pMainMarkings = _pOldMarkings;

	_markingsStruct._length = static_cast<long>(_pMainMarkings->size());
	if (_pMainMarkings->size() > 0)
		_markingsStruct._markings = &((*_pMainMarkings)[0]);

	addSearchResultInfo(count, searchedCount, searchedEntireNotSelection, pFindOpt);
	_scintView.execute(SCI_SETSEL, 0, 0);

	//SCI_SETILEXER resets the lexer property @MarkingsStruct and then no data could be exchanged with the searchResult lexer
	char ptrword[sizeof(void*) * 2 + 1];
	sprintf(ptrword, "%p", static_cast<void*>(&_markingsStruct));
	_scintView.execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("@MarkingsStruct"), reinterpret_cast<LPARAM>(ptrword));

	//previous code: _scintView.execute(SCI_SETILEXER, 0, reinterpret_cast<LPARAM>(CreateLexer("searchResult")));
	_scintView.execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));

	_previousLineNumber = -1;
}


void Finder::setFinderStyle()
{
	// Set global styles for the finder
	_scintView.performGlobalStyles();

	NppDarkMode::setDarkScrollBar(_scintView.getHSelf());

	// Set current line background color for the finder
	const wchar_t * lexerName = ScintillaEditView::_langNameInfoArray[L_SEARCHRESULT]._langName;
	LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		const Style * pStyle = pStyler->findByID(SCE_SEARCHRESULT_CURRENT_LINE);
		if (pStyle)
		{
			_scintView.setElementColour(SC_ELEMENT_CARET_LINE_BACK, pStyle->_bgColor);
			_scintView.execute(SCI_SETCARETLINEFRAME, 0);
			_scintView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
		}
	}
	_scintView.setSearchResultLexer();

	// Override foreground & background colour by default foreground & background coulour
	StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();
	Style * pStyleDefault = stylers.findByID(STYLE_DEFAULT);
	if (pStyleDefault)
	{
		_scintView.setStyle(*pStyleDefault);

		GlobalOverride & go = NppParameters::getInstance().getGlobalOverrideStyle();
		if (go.isEnable())
		{
			const Style * pStyleGlobalOverride = stylers.findByName(L"Global override");
			if (pStyleGlobalOverride)
			{
				if (go.enableFg)
				{
					pStyleDefault->_fgColor = pStyleGlobalOverride->_fgColor;
				}
				if (go.enableBg)
				{
					pStyleDefault->_bgColor = pStyleGlobalOverride->_bgColor;
				}
			}
		}

		_scintView.execute(SCI_STYLESETFORE, SCE_SEARCHRESULT_DEFAULT, pStyleDefault->_fgColor);
		_scintView.execute(SCI_STYLESETBACK, SCE_SEARCHRESULT_DEFAULT, pStyleDefault->_bgColor);
	}

	_scintView.execute(SCI_COLOURISE, 0, -1);

	// finder fold style follows user preference but use box when user selects none
	const ScintillaViewParams& svp = (ScintillaViewParams&)NppParameters::getInstance().getSVP();
	_scintView.setMakerStyle(svp._folderStyle == FOLDER_STYLE_NONE ? FOLDER_STYLE_BOX : svp._folderStyle);
}

void Finder::setFinderStyleForNpc(bool onlyColor)
{
	NppParameters& nppParam = NppParameters::getInstance();
	const bool isShown = nppParam.getSVP()._npcShow;
	if (!onlyColor)
	{
		_scintView.showNpc(isShown, true);
	}
	else if (isShown)
	{
		_scintView.setNpcAndCcUniEOL();
	}
}

intptr_t CALLBACK Finder::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND :
		{
			if (HIWORD(wParam) == SCEN_SETFOCUS)
			{
				::SendMessage(_hParent, NPPM_INTERNAL_CHECKUNDOREDOSTATE, 0, 0);
				return TRUE;
			}

			switch (wParam)
			{
				case NPPM_INTERNAL_FINDINFINDERDLG:
				{
					::SendMessage(_hParent, NPPM_INTERNAL_FINDINFINDERDLG, reinterpret_cast<WPARAM>(this), 0);
					return TRUE;
				}

				case NPPM_INTERNAL_REMOVEFINDER:
				{
					if (_canBeVolatiled)
					{
						::SendMessage(_hParent, NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf));
						setClosed(true);
					}
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERCOLLAPSE :
				{
					_scintView.foldAll(fold_collapse);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERUNCOLLAPSE :
				{
					_scintView.foldAll(fold_expand);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERCOPY :
				{
					copy();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERCOPYVERBATIM:
				{
					_scintView.execute(SCI_COPY);

					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERCOPYPATHS:
				{
					copyPathnames();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERSELECTALL :
				{
					_scintView.execute(SCI_SELECTALL);
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERCLEARALL:
				{
					removeAll();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDEROPENALL:
				{
					openAll();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERWRAP:
				{
					wrapLongLinesToggle();
					return TRUE;
				}

				case NPPM_INTERNAL_SCINTILLAFINDERPURGE:
				{
					purgeToggle();
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

				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

				wstring findInFinder = pNativeSpeaker->getLocalizedStrFromID("finder-find-in-finder", L"Find in these search results...");
				wstring closeThis = pNativeSpeaker->getLocalizedStrFromID("finder-close-this", L"Close these search results");
				wstring collapseAll = pNativeSpeaker->getLocalizedStrFromID("finder-collapse-all", L"Fold all");
				wstring uncollapseAll = pNativeSpeaker->getLocalizedStrFromID("finder-uncollapse-all", L"Unfold all");
				wstring copyLines = pNativeSpeaker->getLocalizedStrFromID("finder-copy", L"Copy Selected Line(s)");
				wstring copyVerbatim = pNativeSpeaker->getNativeLangMenuString(IDM_EDIT_COPY, L"Copy", true);
				copyVerbatim += L"\tCtrl+C";
				wstring copyPaths = pNativeSpeaker->getLocalizedStrFromID("finder-copy-selected-paths", L"Copy Selected Pathname(s)");
				wstring selectAll = pNativeSpeaker->getNativeLangMenuString(IDM_EDIT_SELECTALL, L"Select all", true);
				selectAll += L"\tCtrl+A";
				wstring clearAll = pNativeSpeaker->getLocalizedStrFromID("finder-clear-all", L"Clear all");
				wstring purgeForEverySearch = pNativeSpeaker->getLocalizedStrFromID("finder-purge-for-every-search", L"Purge for every search");
				wstring openAll = pNativeSpeaker->getLocalizedStrFromID("finder-open-selected-paths", L"Open Selected Pathname(s)");
				wstring wrapLongLines = pNativeSpeaker->getLocalizedStrFromID("finder-wrap-long-lines", L"Word wrap long lines");

				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_FINDINFINDERDLG, findInFinder));
				if (_canBeVolatiled)
					tmp.push_back(MenuItemUnit(NPPM_INTERNAL_REMOVEFINDER, closeThis));
				tmp.push_back(MenuItemUnit(0, L"Separator"));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERCOLLAPSE, collapseAll));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERUNCOLLAPSE, uncollapseAll));
				tmp.push_back(MenuItemUnit(0, L"Separator"));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERCOPYVERBATIM, copyVerbatim));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERCOPY, copyLines));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERCOPYPATHS, copyPaths));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERSELECTALL, selectAll));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERCLEARALL, clearAll));
				tmp.push_back(MenuItemUnit(0, L"Separator"));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDEROPENALL, openAll));
				// configuration items go at the bottom:
				tmp.push_back(MenuItemUnit(0, L"Separator"));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERWRAP, wrapLongLines));
				tmp.push_back(MenuItemUnit(NPPM_INTERNAL_SCINTILLAFINDERPURGE, purgeForEverySearch));

				scintillaContextmenu.create(_hSelf, tmp);

				bool hasSomeSelectedText = _scintView.getSelectedTextCount() > 0;
				scintillaContextmenu.enableItem(NPPM_INTERNAL_SCINTILLAFINDERCOPYVERBATIM, hasSomeSelectedText);

				scintillaContextmenu.enableItem(NPPM_INTERNAL_SCINTILLAFINDERCLEARALL, !_canBeVolatiled);

				scintillaContextmenu.enableItem(NPPM_INTERNAL_SCINTILLAFINDERPURGE, !_canBeVolatiled);
				scintillaContextmenu.checkItem(NPPM_INTERNAL_SCINTILLAFINDERPURGE, _purgeBeforeEverySearch && !_canBeVolatiled);

				scintillaContextmenu.checkItem(NPPM_INTERNAL_SCINTILLAFINDERWRAP, _longLinesAreWrapped);

				::TrackPopupMenu(scintillaContextmenu.getMenuHandle(),
					NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
					p.x, p.y, 0, _hSelf, NULL);
				return TRUE;
			}
			return ::DefWindowProc(_hSelf, message, wParam, lParam);
		}

		case WM_SIZE :
		{
			RECT rc{};
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

intptr_t CALLBACK FindIncrementDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
		// Make edit field red if not found
		case WM_CTLCOLOREDIT :
		{
			auto hdc = reinterpret_cast<HDC>(wParam);

			if (NppDarkMode::isEnabled())
			{
				if (FSNotFound != getFindStatus())
				{
					return NppDarkMode::onCtlColorCtrl(hdc);
				}
				else // text not found
				{
					return NppDarkMode::onCtlColorError(hdc);
				}
			}

			// if the text not found modify the background color of the editor
			static HBRUSH hBrushBackground = CreateSolidBrush(BCKGRD_COLOR);

			if (FSNotFound != getFindStatus())
				return FALSE; // text found, use the default color

			// text not found
			SetTextColor(hdc, TXT_COLOR);
			SetBkColor(hdc, BCKGRD_COLOR);
			return reinterpret_cast<LRESULT>(hBrushBackground);
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(getHSelf());
			return TRUE;
		}

		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(getHSelf());
			return TRUE;
		}

		case WM_COMMAND :
		{
			bool updateSearch = false;
			bool forward = true;
			bool advance = false;
			bool updateHiLight = false;
			bool updateCase = false;

			switch (LOWORD(wParam))
			{
				case IDCANCEL :
					(*(_pFRDlg->_ppEditView))->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_INC);
					(*(_pFRDlg->_ppEditView))->grabFocus();
					display(false);
					return TRUE;

				case IDM_SEARCH_FINDINCREMENT:	// Accel table: Start incremental search
					// if focus is on a some other control, return it to the edit field
					if (::GetFocus() != ::GetDlgItem(_hSelf, IDC_INCFINDTEXT))
					{
						HWND hFindTxt = ::GetDlgItem(_hSelf, IDC_INCFINDTEXT);
						::PostMessage(_hSelf, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(hFindTxt), TRUE);
						return TRUE;
					}
					// otherwise, repeat the search
					[[fallthrough]];
				case IDM_SEARCH_FINDPREV:		// Accel table: find prev
				case IDM_SEARCH_FINDNEXT:		// Accel table: find next
				case IDC_INCFINDPREVOK:
				case IDC_INCFINDNXTOK:
				case IDOK:
					updateSearch = true;
					advance = true;
					forward = (LOWORD(wParam) == IDC_INCFINDNXTOK) ||
						(LOWORD(wParam) == IDM_SEARCH_FINDNEXT) ||
						(LOWORD(wParam) == IDM_SEARCH_FINDINCREMENT) ||
						((LOWORD(wParam) == IDOK) && !(GetKeyState(VK_SHIFT) & SHIFTED));
					break;

				case IDC_INCFINDMATCHCASE:
					updateSearch = true;
					updateCase = true;
					updateHiLight = true;
					break;

				case IDC_INCFINDHILITEALL:
					updateHiLight = true;
					break;

				case IDC_INCFINDTEXT:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						updateSearch = true;
						updateHiLight = isCheckedOrNot(IDC_INCFINDHILITEALL);
						updateCase = isCheckedOrNot(IDC_INCFINDMATCHCASE);
						break;
					}
					// treat other edit notifications as unhandled
					[[fallthrough]];
				default:
					return FALSE;
			}
			FindOption fo;
			fo._isWholeWord = false;
			fo._incrementalType = advance ? NextIncremental : FirstIncremental;
			fo._whichDirection = forward ? DIR_DOWN : DIR_UP;
			fo._isMatchCase = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDMATCHCASE, BM_GETCHECK, 0, 0));

			wstring str2Search = getTextFromCombo(::GetDlgItem(_hSelf, IDC_INCFINDTEXT));
			if (updateSearch)
			{
				FindStatus findStatus = FSFound;
				bool isFound = _pFRDlg->processFindNext(str2Search.c_str(), &fo, &findStatus);

				fo._str2Search = str2Search;
				int nbCounted = _pFRDlg->processAll(ProcessCountAll, &fo);
				setFindStatus(findStatus, nbCounted);

				// If case-sensitivity changed (to Match=yes), there may have been a matched selection that
				// now does not match; so if Not Found, clear selection and put caret at beginning of what was
				// selected (no change, if there was no selection)
				if (updateCase && !isFound)
				{
					Sci_CharacterRangeFull range = (*(_pFRDlg->_ppEditView))->getSelection();
					(*(_pFRDlg->_ppEditView))->execute(SCI_SETSEL, static_cast<WPARAM>(-1), range.cpMin);
				}
			}

			if (updateHiLight)
			{
				bool highlight = !str2Search.empty() &&
					(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_INCFINDHILITEALL, BM_GETCHECK, 0, 0));
				markSelectedTextInc(highlight, &fo);
			}
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rcClient{};
				GetClientRect(_hSelf, &rcClient);
				::FillRect(reinterpret_cast<HDC>(wParam), &rcClient, NppDarkMode::getDlgBackgroundBrush());
				return TRUE;
			}
			else
			{
				HWND hParent = ::GetParent(_hSelf);
				HDC winDC = (HDC)wParam;
				//RTL handling
				POINT pt = { 0, 0 }, ptOrig = { 0, 0 };
				::MapWindowPoints(_hSelf, hParent, &pt, 1);
				::OffsetWindowOrgEx((HDC)wParam, pt.x, pt.y, &ptOrig);
				LRESULT lResult = SendMessage(hParent, WM_ERASEBKGND, reinterpret_cast<WPARAM>(winDC), 0);
				::SetWindowOrgEx(winDC, ptOrig.x, ptOrig.y, NULL);
				return (BOOL)lResult;
			}
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
	Sci_CharacterRangeFull range = (*(_pFRDlg->_ppEditView))->getSelection();

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
		return;

	wchar_t text2Find[FINDREPLACE_MAXLENGTH]{};
	(*(_pFRDlg->_ppEditView))->getGenericSelectedText(text2Find, FINDREPLACE_MAXLENGTH, false);	//do not expand selection (false)
	opt->_str2Search = text2Find;
	_pFRDlg->markAllInc(opt);
}

void FindIncrementDlg::setFindStatus(FindStatus iStatus, int nbCounted)
{
	wstring statusStr2Display;

	static const wchar_t* const strFSNotFound = L"Phrase not found";
	static const wchar_t* const strFSTopReached = L"Reached top of page, continued from bottom";
	static const wchar_t* const strFSEndReached = L"Reached end of page, continued from top";

	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	if (nbCounted >= 0)
	{
		statusStr2Display = pNativeSpeaker->getLocalizedStrFromID("IncrementalFind-FSFound", L"");

		if (statusStr2Display.empty())
		{
			wchar_t strFindFSFound[128] = L"";

			if (nbCounted == 1)
				wsprintf(strFindFSFound, L"%d match", nbCounted);
			else
				wsprintf(strFindFSFound, L"%s matches", commafyInt(nbCounted).c_str());
			statusStr2Display = strFindFSFound;
		}
		else
		{
			statusStr2Display = stringReplace(statusStr2Display, L"$INT_REPLACE$", std::to_wstring(nbCounted));
		}
	}


	switch (iStatus)
	{
		case FindStatus::FSNotFound:
			statusStr2Display = pNativeSpeaker->getLocalizedStrFromID("IncrementalFind-FSNotFound", strFSNotFound);
			break;

		case FindStatus::FSTopReached:
			statusStr2Display = pNativeSpeaker->getLocalizedStrFromID("IncrementalFind-FSTopReached", strFSTopReached);
			break;

		case FindStatus::FSEndReached:
			statusStr2Display = pNativeSpeaker->getLocalizedStrFromID("IncrementalFind-FSEndReached", strFSEndReached);
			break;

		case FindStatus::FSFound:
			break;

		default:
			return; // out of range
	}

	_findStatus = iStatus;

	// get the HWND of the editor
	HWND hEditor = ::GetDlgItem(_hSelf, IDC_INCFINDTEXT);

	// invalidate the editor rect
	::InvalidateRect(hEditor, NULL, TRUE);

	::SendDlgItemMessage(_hSelf, IDC_INCFINDSTATUS, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(statusStr2Display.c_str()));
}

void FindIncrementDlg::addToRebar(ReBar * rebar)
{
	if (_pRebar)
		return;

	_pRebar = rebar;
	RECT client{};
	getClientRect(client);

	ZeroMemory(&_rbBand, REBARBAND_SIZE);
	_rbBand.cbSize  = REBARBAND_SIZE;

	_rbBand.fMask   = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
					  RBBIM_SIZE | RBBIM_ID;

	_rbBand.fStyle = RBBS_HIDDEN | RBBS_NOGRIPPER;
	_rbBand.hwndChild	= getHSelf();
	_rbBand.wID			= REBAR_BAR_SEARCH;	//ID REBAR_BAR_SEARCH for search dialog
	_rbBand.cxMinChild	= 0;
	_rbBand.cyIntegral	= 1;
	_rbBand.cyMinChild	= _rbBand.cyMaxChild	= client.bottom-client.top;
	_rbBand.cxIdeal		= _rbBand.cx			= client.right-client.left;

	_pRebar->addBand(&_rbBand, true);
	_pRebar->setGrayBackground(_rbBand.wID);
}

const wchar_t Progress::cClassName[] = L"NppProgressClass";
const wchar_t Progress::cDefaultHeader[] = L"Operation progress...";
const int Progress::cBackgroundColor = COLOR_3DFACE;
const SIZE Progress::_szClient = { 565, 95 };

volatile LONG Progress::refCount = 0;


Progress::Progress(HINSTANCE hInst) : _hwnd(NULL), _hCallerWnd(NULL)
{
	if (::InterlockedIncrement(&refCount) == 1)
	{
		_hInst = hInst;

		WNDCLASSEX wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = wndProc;
		wcex.hInstance = _hInst;
		wcex.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDI_M30ICON));
		wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = ::GetSysColorBrush(cBackgroundColor);
		wcex.lpszClassName = cClassName;

		::RegisterClassEx(&wcex);

		INITCOMMONCONTROLSEX icex{};
		icex.dwSize = sizeof(icex);
		icex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

		::InitCommonControlsEx(&icex);
	}
}


Progress::~Progress()
{
	close();

	if (::InterlockedDecrement(&refCount) == 0)
		::UnregisterClass(cClassName, _hInst);
}


HWND Progress::open(HWND hCallerWnd, const wchar_t* header)
{
	if (_hwnd)
		return _hwnd;

	// Create manually reset non-signalled event
	_hActiveState = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!_hActiveState)
		return NULL;

	if (!hCallerWnd)
		return NULL;

	_hCallerWnd = hCallerWnd;

	for (HWND hwnd = _hCallerWnd; hwnd; hwnd = ::GetParent(hwnd))
		::UpdateWindow(hwnd);

	if (header)
		wcscpy_s(_header, _countof(_header), header);
	else
		wcscpy_s(_header, _countof(_header), cDefaultHeader);

	_hThread = ::CreateThread(NULL, 0, threadFunc, this, 0, NULL);
	if (!_hThread)
	{
		::CloseHandle(_hActiveState);
		return NULL;
	}

	// Wait for the progress window to be created
	::WaitForSingleObject(_hActiveState, INFINITE);

	// On progress window create fail
	if (!_hwnd)
	{
		::WaitForSingleObject(_hThread, INFINITE);
		::CloseHandle(_hThread);
		::CloseHandle(_hActiveState);
	}

	return _hwnd;
}


void Progress::close()
{
	if (_hwnd)
	{
		::PostMessage(_hwnd, WM_CLOSE, 0, 0);
		_hwnd = NULL;
		::WaitForSingleObject(_hThread, INFINITE);

		::CloseHandle(_hThread);
		::CloseHandle(_hActiveState);
	}

	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}
}


void Progress::setPercent(unsigned percent, const wchar_t* fileName, int nbHitsSoFar) const
{
	if (_hwnd)
	{
		::PostMessage(_hPBar, PBM_SETPOS, percent, 0);
		::SendMessage(_hPathText, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(fileName));
		wchar_t str[16]{};
		_itow(nbHitsSoFar, str, 10);
		::SendMessage(_hRunningHitsText, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str));
	}
}


void Progress::setInfo(const wchar_t* info, int nbHitsSoFar) const
{
	if (_hwnd)
	{
		::SendMessage(_hPathText, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(info));
		if (nbHitsSoFar != -1)
		{
			wchar_t str[16]{};
			_itow(nbHitsSoFar, str, 10);
			::SendMessage(_hRunningHitsText, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str));
		}
	}
}


DWORD WINAPI Progress::threadFunc(LPVOID data)
{
	Progress* pw = static_cast<Progress*>(data);
	return (DWORD)pw->thread();
}


int Progress::thread()
{
	BOOL r = createProgressWindow();
	::SetEvent(_hActiveState);
	if (r)
		return r;

	// Window message loop
	MSG msg{};
	while ((r = ::GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return r;
}


int Progress::createProgressWindow()
{
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	_dpiManager.setDpi(_hCallerWnd);
	const UINT dpi = _dpiManager.getDpi();

	RECT rcWindow = getDpiScaledWindowRect(dpi);

	int xWindow = rcWindow.right - rcWindow.left;
	int yWindow = rcWindow.bottom - rcWindow.top;

	const DWORD style = WS_POPUP | WS_CAPTION;
	const DWORD exStyle = WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_OVERLAPPEDWINDOW | (pNativeSpeaker->isRTL() ? WS_EX_LAYOUTRTL : 0);

	_hwnd = ::CreateWindowEx(exStyle, cClassName, _header, style,
		rcWindow.left, rcWindow.top, xWindow, yWindow,
		nullptr, nullptr, _hInst, static_cast<LPVOID>(this));

	if (!_hwnd)
		return -1;

	_hPathText = ::CreateWindowEx(0, WC_STATIC, L"",
		WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS,
		0, 0, 0, 0,
		_hwnd, nullptr, _hInst, nullptr);

	wstring hits = pNativeSpeaker->getLocalizedStrFromID("progress-hits-title", L"Hits:");
	_hRunningHitsStaticText = ::CreateWindowEx(0, WC_STATIC, hits.c_str(),
		WS_CHILD | WS_VISIBLE | SS_RIGHT,
		0, 0, 0, 0,
		_hwnd, nullptr, _hInst, nullptr);

	_hRunningHitsText = ::CreateWindowEx(0, WC_STATIC, L"",
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, nullptr, _hInst, nullptr);

	_hPBar = ::CreateWindowEx(0, PROGRESS_CLASS, L"Progress Bar",
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		0, 0, 0, 0,
		_hwnd, nullptr, _hInst, nullptr);

	::SendMessage(_hPBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

	// Set border so user can distinguish easier progress bar,
	// especially, when getBackgroundColor is very similar or same 
	// as getDlgBackgroundColor
	NppDarkMode::setBorder(_hPBar, NppDarkMode::isEnabled());
	NppDarkMode::disableVisualStyle(_hPBar, NppDarkMode::isEnabled());
	if (NppDarkMode::isEnabled())
	{
		::SendMessage(_hPBar, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(NppDarkMode::getBackgroundColor()));
		::SendMessage(_hPBar, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(NppDarkMode::getDarkerTextColor()));
	}

	wstring cancel = pNativeSpeaker->getLocalizedStrFromID("common-cancel", L"Cancel");
	_hBtn = ::CreateWindowEx(0, WC_BUTTON, cancel.c_str(),
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		0, 0, 0, 0,
		_hwnd, nullptr, _hInst, nullptr);

	setCtrlsPos();
	setFont();

	NppDarkMode::autoSubclassAndThemeChildControls(_hwnd);
	NppDarkMode::setDarkTitleBar(_hwnd);

	POINT center{};
	RECT callerRect{};
	::GetWindowRect(_hCallerWnd, &callerRect);
	center.x = (callerRect.left + callerRect.right) / 2;
	center.y = (callerRect.top + callerRect.bottom) / 2;

	int x = center.x - xWindow / 2;
	int y = center.y - yWindow / 2;

	::SetWindowPos(_hwnd, nullptr, x, y, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
	::SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, xWindow, yWindow, SWP_NOMOVE | SWP_SHOWWINDOW);

	::ShowWindow(_hwnd, SW_SHOWNORMAL);
	::UpdateWindow(_hwnd);

	return 0;
}


RECT Progress::getDpiScaledWindowRect(UINT dpi) const
{
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	const int borderPadding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
	const int xBorderPadding = (_dpiManager.getSystemMetricsForDpi(SM_CXFRAME, dpi) + borderPadding) * 2;
	const int yBorderPadding = (_dpiManager.getSystemMetricsForDpi(SM_CYFRAME, dpi) + borderPadding) * 2 + _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION, dpi);

	const int xClient = _dpiManager.scale(_szClient.cx, dpi);
	const int yClient = _dpiManager.scale(_szClient.cy, dpi);

	RECT rc{ 0, 0, xClient, yClient };

	const DWORD style = WS_POPUP | WS_CAPTION;
	const DWORD exStyle = WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_OVERLAPPEDWINDOW | (pNativeSpeaker->isRTL() ? WS_EX_LAYOUTRTL : 0);
	if (_dpiManager.adjustWindowRectExForDpi(&rc, style, FALSE, exStyle, dpi) == FALSE)
	{
		rc.right = xClient + xBorderPadding;
		rc.bottom = yClient + yBorderPadding;
	}

	return rc;
}

void Progress::setCtrlsPos()
{
	auto setOrDeferWindowPos = [](HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) -> HDWP {
		if (hWinPosInfo != nullptr)
		{
			return ::DeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		}
		::SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		return nullptr;
		};

	constexpr int heightBar = 10;
	constexpr int heightText = 18;
	constexpr int widthTextHits = 150;
	constexpr int widthBtn = 80;
	constexpr int heightBtn = 25;

	const int padding = _dpiManager.scale(5);

	const int xClientPadded = _dpiManager.scale(_szClient.cx) - padding * 2;
	const int yBar = _dpiManager.scale(heightBar);
	const int yText = _dpiManager.scale(heightText);
	const int xTextHits = _dpiManager.scale(widthTextHits);
	const int xBtn = _dpiManager.scale(widthBtn);
	const int yBtn = _dpiManager.scale(heightBtn);

	const int xStartPos = padding;
	int yCtrlPos = padding;

	constexpr UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	constexpr int nCtrls = 5;
	auto hdwp = ::BeginDeferWindowPos(nCtrls);

	hdwp = setOrDeferWindowPos(hdwp, _hPathText, nullptr, xStartPos, yCtrlPos, xClientPadded, yText, flags);
	yCtrlPos += yText;
	hdwp = setOrDeferWindowPos(hdwp, _hRunningHitsStaticText, nullptr, (xClientPadded - padding) / 2 - xTextHits, yCtrlPos, xTextHits, yText, flags);
	hdwp = setOrDeferWindowPos(hdwp, _hRunningHitsText, nullptr, (xClientPadded + padding) / 2, yCtrlPos, xTextHits, yText, flags);
	yCtrlPos += yText;
	hdwp = setOrDeferWindowPos(hdwp, _hPBar, nullptr, xStartPos, yCtrlPos, xClientPadded, yBar, flags);
	yCtrlPos += yText;
	hdwp = setOrDeferWindowPos(hdwp, _hBtn, nullptr, (xClientPadded - xBtn) / 2, yCtrlPos, xBtn, yBtn, flags);

	if (hdwp)
		::EndDeferWindowPos(hdwp);
}

void Progress::setFont()
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}

	LOGFONT lf{ _dpiManager.getDefaultGUIFontForDpi() };
	_hFont = ::CreateFontIndirect(&lf);

	if (_hFont != nullptr)
	{
		const auto& wpFont = reinterpret_cast<WPARAM>(_hFont);
		::SendMessage(_hPathText, WM_SETFONT, wpFont, TRUE);
		::SendMessage(_hRunningHitsStaticText, WM_SETFONT, wpFont, TRUE);
		::SendMessage(_hRunningHitsText, WM_SETFONT, wpFont, TRUE);
		::SendMessage(_hBtn, WM_SETFONT, wpFont, TRUE);
	}
}


LRESULT APIENTRY Progress::wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		case WM_CREATE:
		{
			Progress* pw = static_cast<Progress*>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pw));
			return 0;
		}

		case WM_CTLCOLORDLG:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wparam));
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wparam));
			}
			// transparent background for text, same as main window background
			return reinterpret_cast<LRESULT>(::GetSysColorBrush(NULL_BRUSH));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc{};
				::GetClientRect(hwnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wparam), &rc, NppDarkMode::getDlgBackgroundBrush());
				return TRUE;
			}
			break;
		}

		case WM_SETFOCUS:
		{
			Progress* pw =	reinterpret_cast<Progress*>(static_cast<LONG_PTR>
			(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
			::SetFocus(pw->_hBtn);
			return 0;
		}

		case WM_GETDPISCALEDSIZE:
		{
			const auto pw = reinterpret_cast<Progress*>(static_cast<LONG_PTR>(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));

			const UINT newDpi = static_cast<UINT>(wparam);
			RECT rcWindow = pw->getDpiScaledWindowRect(newDpi);

			auto newSize = reinterpret_cast<SIZE*>(lparam);
			newSize->cx = rcWindow.right - rcWindow.left;
			newSize->cy = rcWindow.bottom - rcWindow.top;

			return TRUE;
		}

		case WM_DPICHANGED:
		{
			auto pw = reinterpret_cast<Progress*>(static_cast<LONG_PTR>(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
			pw->_dpiManager.setDpiWP(wparam);

			pw->setCtrlsPos();
			pw->setFont();

			pw->_dpiManager.setPositionDpi(lparam, hwnd, SWP_NOZORDER | SWP_NOACTIVATE);

			return 0;
		}

		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				Progress* pw = reinterpret_cast<Progress*>(static_cast<LONG_PTR>(::GetWindowLongPtr(hwnd, GWLP_USERDATA)));
				::ResetEvent(pw->_hActiveState);
				::EnableWindow(pw->_hBtn, FALSE);
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				wstring info = pNativeSpeaker->getLocalizedStrFromID("progress-cancel-info", L"Cancelling operation, please wait...");
				pw->setInfo(info.c_str());
				return 0;
			}
			break;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
	}

	return ::DefWindowProc(hwnd, umsg, wparam, lparam);
}
