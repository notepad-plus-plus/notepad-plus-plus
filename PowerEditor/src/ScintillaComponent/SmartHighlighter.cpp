// This file is part of Notepad++ project
// Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>

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

#include "SmartHighlighter.h"
#include "ScintillaEditView.h"
#include "FindReplaceDlg.h"

#define MAXLINEHIGHLIGHT 400	//prevent highlighter from doing too much work when a lot is visible

SmartHighlighter::SmartHighlighter(FindReplaceDlg * pFRDlg)
: _pFRDlg(pFRDlg)
{
	//Nothing to do
}

void SmartHighlighter::highlightViewWithWord(ScintillaEditView * pHighlightView, const generic_string & word2Hilite)
{
	// save target locations for other search functions
	auto originalStartPos = pHighlightView->execute(SCI_GETTARGETSTART);
	auto originalEndPos = pHighlightView->execute(SCI_GETTARGETEND);

	// Get the range of text visible and highlight everything in it
	auto firstLine = pHighlightView->execute(SCI_GETFIRSTVISIBLELINE);
	auto nbLineOnScreen = pHighlightView->execute(SCI_LINESONSCREEN);
	auto nbLines = min(nbLineOnScreen, MAXLINEHIGHLIGHT) + 1;
	auto lastLine = firstLine + nbLines;
	size_t startPos = 0;
	intptr_t endPos = 0;
	auto currentLine = firstLine;
	intptr_t prevDocLineChecked = -1;	//invalid start

	// Determine mode for SmartHighlighting
	bool isWordOnly = true;
	bool isCaseSensentive = true;

	const NppGUI & nppGUI = NppParameters::getInstance().getNppGUI();

	if (nppGUI._smartHiliteUseFindSettings)
	{
		// fetch find dialog's setting
		NppParameters& nppParams = NppParameters::getInstance();
		FindHistory &findHistory = nppParams.getFindHistory();
		isWordOnly = findHistory._isMatchWord;
		isCaseSensentive = findHistory._isMatchCase;
	}
	else
	{
		isWordOnly = nppGUI._smartHiliteWordOnly;
		isCaseSensentive = nppGUI._smartHiliteCaseSensitive;
	}

	FindOption fo;
	fo._isMatchCase = isCaseSensentive;
	fo._isWholeWord = isWordOnly;

	FindReplaceInfo frInfo;
	frInfo._txt2find = word2Hilite.c_str();

	for (; currentLine < lastLine; ++currentLine)
	{
		intptr_t docLine = pHighlightView->execute(SCI_DOCLINEFROMVISIBLE, currentLine);
		if (docLine == prevDocLineChecked)
			continue;	//still on same line (wordwrap)
		prevDocLineChecked = static_cast<intptr_t>(docLine);
		startPos = pHighlightView->execute(SCI_POSITIONFROMLINE, docLine);
		endPos = pHighlightView->execute(SCI_POSITIONFROMLINE, docLine + 1);
		
		frInfo._startRange = startPos;
		frInfo._endRange = endPos;
		if (endPos == -1)
		{	//past EOF
			frInfo._endRange = pHighlightView->getCurrentDocLen() - 1;
			_pFRDlg->processRange(ProcessMarkAll_2, frInfo, NULL, &fo, -1, pHighlightView);
			break;
		}
		else
		{
			_pFRDlg->processRange(ProcessMarkAll_2, frInfo, NULL, &fo, -1, pHighlightView);
		}
	}

	// restore the original targets to avoid conflicts with the search/replace functions
	pHighlightView->execute(SCI_SETTARGETRANGE, originalStartPos, originalEndPos);
}

void SmartHighlighter::highlightView(ScintillaEditView * pHighlightView, ScintillaEditView * unfocusView)
{
	// Clear marks
	pHighlightView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);

	const NppGUI & nppGUI = NppParameters::getInstance().getNppGUI();

	// If nothing selected or smart highlighting disabled, don't mark anything
	if ((!nppGUI._enableSmartHilite) || (pHighlightView->execute(SCI_GETSELECTIONEMPTY) == 1))
	{
		if (nppGUI._smartHiliteOnAnotherView && unfocusView && unfocusView->isVisible()
			&& unfocusView->getCurrentBufferID() != pHighlightView->getCurrentBufferID())
		{
			unfocusView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
		}
		return;
	}

	auto curPos = pHighlightView->execute(SCI_GETCURRENTPOS);
	auto range = pHighlightView->getSelection();
	intptr_t textlen = range.cpMax - range.cpMin;

	// Determine mode for SmartHighlighting
	bool isWordOnly = true;

	if (nppGUI._smartHiliteUseFindSettings)
	{
		// fetch find dialog's setting
		NppParameters& nppParams = NppParameters::getInstance();
		FindHistory &findHistory = nppParams.getFindHistory();
		isWordOnly = findHistory._isMatchWord;
	}
	else
	{
		isWordOnly = nppGUI._smartHiliteWordOnly;
	}

	// additional checks for wordOnly mode
	// Make sure the "word" positions match the current selection
	if (isWordOnly)
	{
		auto wordStart = pHighlightView->execute(SCI_WORDSTARTPOSITION, curPos, true);
		auto wordEnd = pHighlightView->execute(SCI_WORDENDPOSITION, wordStart, true);

		if (wordStart == wordEnd || wordStart != range.cpMin || wordEnd != range.cpMax)
			return;
	}
	else
	{
		auto line = pHighlightView->execute(SCI_LINEFROMPOSITION, curPos);
		auto lineLength = pHighlightView->execute(SCI_LINELENGTH, line);
		if (textlen > lineLength)
			return;
	}
	
	char * text2Find = new char[textlen + 1];
	pHighlightView->getSelectedText(text2Find, textlen + 1, false); //do not expand selection (false)

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(pHighlightView->execute(SCI_GETCODEPAGE));
	const TCHAR * text2FindW = wmc.char2wchar(text2Find, cp);

	highlightViewWithWord(pHighlightView, text2FindW);

	if (nppGUI._smartHiliteOnAnotherView && unfocusView && unfocusView->isVisible())
	{		
		// Clear the indicator only when the view is not a clone, or it will clear what we have already hightlighted in the pHighlightView
		if (unfocusView->getCurrentBufferID() != pHighlightView->getCurrentBufferID())
		{	
			unfocusView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
		}
		
		// Hightlight the unfocused view even if it's a clone, as it might be in a different area of the document
		highlightViewWithWord(unfocusView, text2FindW);
	}

	delete[] text2Find;
}
