//this file is part of notepad++
//Copyright (C)2003 Harry <harrybharry@users.sourceforge.net>
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

#include "SmartHighlighter.h"
#include "Parameters.h"

#define MAXLINEHIGHLIGHT 400	//prevent highlighter from doing too much work when a lot is visible

SmartHighlighter::SmartHighlighter(FindReplaceDlg * pFRDlg)
: _pFRDlg(pFRDlg)
{
	//Nothing to do
}

void SmartHighlighter::highlightView(ScintillaEditView * pHighlightView)
{
	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	if (!nppGUI._enableSmartHilite)
		return;

	//Get selection
	CharacterRange range = pHighlightView->getSelection();

	//Clear marks
	pHighlightView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_2);

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
	{
		return;
	}

	int textlen = range.cpMax - range.cpMin + 1;

	char * text2Find = new char[textlen];
	pHighlightView->getSelectedText(text2Find, textlen, false);	//do not expand selection (false)

	bool valid = true;
	//The word has to consist if wordChars only, and the characters before and after something else
	if (!isQualifiedWord(text2Find))
		valid = false;
	else
	{
		unsigned char c = (unsigned char)pHighlightView->execute(SCI_GETCHARAT, range.cpMax);
		if (c)
		{
			if (isWordChar(char(c)))
				valid = false;
		}
		c = (unsigned char)pHighlightView->execute(SCI_GETCHARAT, range.cpMin-1);
		if (c)
		{
			if (isWordChar(char(c)))
				valid = false;
		}
	}
	if (!valid) {
		delete [] text2Find;
		return;
	}

	// save target locations for other search functions
	int originalStartPos = (int)pHighlightView->execute(SCI_GETTARGETSTART);
	int originalEndPos = (int)pHighlightView->execute(SCI_GETTARGETEND);

	// Get the range of text visible and highlight everything in it
	int firstLine =		(int)pHighlightView->execute(SCI_GETFIRSTVISIBLELINE);
	int nrLines =	min((int)pHighlightView->execute(SCI_LINESONSCREEN), MAXLINEHIGHLIGHT ) + 1;
	int lastLine =		(int)pHighlightView->execute(SCI_DOCLINEFROMVISIBLE, firstLine + nrLines);
	int startPos =		(int)pHighlightView->execute(SCI_POSITIONFROMLINE, firstLine);
	int endPos =		(int)pHighlightView->execute(SCI_POSITIONFROMLINE, lastLine);
	if (endPos == -1) {	//past EOF
		endPos =		(int)pHighlightView->getCurrentDocLen() - 1;
	}

	FindOption fo;
	fo._isMatchCase = true;
	fo._isWholeWord = true;
	_pFRDlg->processRange(ProcessMarkAll_2, text2Find, NULL, startPos, endPos, NULL, &fo);

	// restore the original targets to avoid conflicts with the search/replace functions
	pHighlightView->execute(SCI_SETTARGETSTART, originalStartPos);
	pHighlightView->execute(SCI_SETTARGETEND, originalEndPos);
}

bool SmartHighlighter::isQualifiedWord(const char *str) const
{
	for (size_t i = 0 ; i < strlen(str) ; i++)
	{
		if (!isWordChar(str[i]))
			return false;
	}
	return true;
};

bool SmartHighlighter::isWordChar(char ch) const
{
	if ((unsigned char)ch < 0x20) 
		return false;
	
	switch(ch)
	{
		case ' ':
		case '	':
		case '\n':
		case '\r':
		case '.':
		case ',':
		case '?':
		case ';':
		case ':':
		case '!':
		case '(':
		case ')':
		case '[':
		case ']':
		case '+':
		case '-':
		case '*':
		case '/':
		case '#':
		case '@':
		case '^':
		case '%':
		case '$':
		case '"':
		case '\'':
		case '~':
		case '&':
		case '{':
		case '}':
		case '|':
		case '=':
		case '<':
		case '>':
		case '\\':
			return false;
	}
	return true;
};
