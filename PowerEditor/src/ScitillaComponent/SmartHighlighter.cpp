// this file is part of notepad++
// Copyright (C)2003 Harry <harrybharry@users.sourceforge.net>
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

#include <memory>
#include <algorithm>
#include "SmartHighlighter.h"
#include "ScintillaEditView.h"
#include "FindReplaceDlg.h"

#define MAXLINEHIGHLIGHT 400	//prevent highlighter from doing too much work when a lot is visible

SmartHighlighter::SmartHighlighter(FindReplaceDlg * pFRDlg)
: _pFRDlg(pFRDlg)
{
	//Nothing to do
}

void SmartHighlighter::highlightView(ScintillaEditView * pHighlightView)
{
	CharacterRange range = pHighlightView->getSelection();

	//Clear marks
	pHighlightView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
		return;

	const int textAlloclen = range.cpMax - range.cpMin + 1;
	assert(textAlloclen > 0);
	const size_t textLen = static_cast<size_t>(textAlloclen - 1);

	std::unique_ptr<char[]> text2Find = std::make_unique<char[]>(textAlloclen);
	pHighlightView->getSelectedText(text2Find.get(), textAlloclen, false);	//do not expand selection (false)
	
	//GETWORDCHARS for isQualifiedWord2() and isWordChar2()
	const int listCharIntegerSize = pHighlightView->execute(SCI_GETWORDCHARS, 0, 0);
	assert(listCharIntegerSize > 0);
	const size_t listCharSize = static_cast<size_t>(listCharIntegerSize);

	std::unique_ptr<char[]> listChar = std::make_unique<char[]>(listCharSize + 1);
	pHighlightView->execute(SCI_GETWORDCHARS, 0, (LPARAM)listChar.get());
	
	//The word has to consist if wordChars only, and the characters before and after something else
	if (!SmartHighlighter::isQualifiedWord(text2Find.get(), textLen, listChar.get(), listCharSize))
		return;

	UCHAR c = (UCHAR)pHighlightView->execute(SCI_GETCHARAT, range.cpMax);
	if (c)
	{
		if (isWordChar(char(c), listChar.get(), listCharSize))
			return;
	}
	c = (UCHAR)pHighlightView->execute(SCI_GETCHARAT, range.cpMin-1);
	if (c)
	{
		if (isWordChar(char(c), listChar.get(), listCharSize))
			return;
	}

	// save target locations for other search functions
	const int originalStartPos = (int)pHighlightView->execute(SCI_GETTARGETSTART);
	const int originalEndPos = (int)pHighlightView->execute(SCI_GETTARGETEND);

	// Get the range of text visible and highlight everything in it
	const int firstLine =		(int)pHighlightView->execute(SCI_GETFIRSTVISIBLELINE);
	const int nrLines =	min((int)pHighlightView->execute(SCI_LINESONSCREEN), MAXLINEHIGHLIGHT ) + 1;
	const int lastLine =		firstLine+nrLines;
	int prevDocLineChecked = -1;	//invalid start

	const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();

	FindOption fo;
	fo._isMatchCase = nppGUI._smartHiliteCaseSensitive;
	fo._isWholeWord = true;

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	const unsigned int cp = pHighlightView->execute(SCI_GETCODEPAGE); 
	const TCHAR * searchTextW = wmc->char2wchar(text2Find.get(), cp);

	for(int currentLine = firstLine; currentLine < lastLine; ++currentLine)
	{
		const int docLine = (int)pHighlightView->execute(SCI_DOCLINEFROMVISIBLE, currentLine);
		if (docLine == prevDocLineChecked)
			continue;	//still on same line (wordwrap)
		prevDocLineChecked = docLine;
		const int startPos = (int)pHighlightView->execute(SCI_POSITIONFROMLINE, docLine);
		int endPos = (int)pHighlightView->execute(SCI_POSITIONFROMLINE, docLine+1);
		if (endPos == -1) {	//past EOF
			endPos = pHighlightView->getCurrentDocLen() - 1;
			_pFRDlg->processRange(ProcessMarkAll_2, searchTextW, NULL, startPos, endPos, NULL, &fo);
			break;
		}
		_pFRDlg->processRange(ProcessMarkAll_2, searchTextW, NULL, startPos, endPos, NULL, &fo);
	}

	// restore the original targets to avoid conflicts with the search/replace functions
	pHighlightView->execute(SCI_SETTARGETSTART, originalStartPos);
	pHighlightView->execute(SCI_SETTARGETEND, originalEndPos);
}

bool SmartHighlighter::isQualifiedWord(_In_reads_(textSize) const char* const textToFind, const size_t textSize, _In_reads_(listCharSize) const char* const listChar, const size_t listCharSize) const
{
	for (size_t i = 0; i < textSize; ++i)
	{
		if (!isWordChar(textToFind[i], listChar, listCharSize))
			return false;
	}
	return true;
};

bool SmartHighlighter::isWordChar(const char ch, _In_reads_(listCharSize) const char* const listChar, const size_t listCharSize ) const
{
	return std::find(listChar, listChar + listCharSize, ch) != (listChar + listCharSize);
};
