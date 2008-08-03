// This file is modified from LexOthers.cxx of Scintilla source code edit control
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>

// The License.txt file describes the conditions under which this software may be distributed.
//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org >
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

static inline bool AtEOL(Accessor &styler, unsigned int i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

static const char * const emptyWordListDesc[] = {
	0
};

inline bool isSpaceChar(char ch) {
	return ((ch == ' ')  || (ch == '	'));
};

// return value : false if the end of line is reached, otherwise true
inline bool eatWhiteSpaces(const char *line, unsigned int & pos) {
	if (pos >= strlen(line)) return false;

	//int i = pos;
	for ( ; line[pos] && isSpaceChar(line[pos]) ; pos++);

	return (pos < strlen(line));
};

static void ColouriseSearchResultLine(WordList *keywordlists[], char *lineBuffer, unsigned int lengthLine, unsigned int startLine, unsigned int endPos, Accessor &styler) 
{

	// startLine and endPos are the absolute positions.

	WordList &word2Search = *keywordlists[0];
	WordList &keywords1 = *keywordlists[1];
	WordList &keywords2 = *keywordlists[2];
	WordList &keywords3 = *keywordlists[3];

	if (lineBuffer[0] == '[') 
	{
		styler.ColourTo(endPos, SCE_SEARCHRESULT_HEARDER);
	} 
	else
	{
		const unsigned int firstTokenLen = 4;
		styler.ColourTo(startLine + firstTokenLen, SCE_SEARCHRESULT_DEFAULT);

		unsigned int currentPos = firstTokenLen;
		for ( ; lineBuffer[currentPos] != ':' ; currentPos++);
		styler.ColourTo(startLine + currentPos - 1, SCE_SEARCHRESULT_NUMBER);
		
		
		//StyleContext sc(startPos, length, initStyle, styler);
		int currentStat = SCE_SEARCHRESULT_DEFAULT;


		const int maxWordSize = 4096;
		char word[maxWordSize];
		
		bool isEndReached = eatWhiteSpaces(lineBuffer, currentPos);

		styler.ColourTo(startLine + currentPos - 1, SCE_SEARCHRESULT_DEFAULT);

		while (currentPos < lengthLine)
		{
			for (int j = 0 ; j < maxWordSize ; currentPos++, j++)
			{
				
				if (currentPos >= lengthLine)
				{
					isEndReached = true;
					break;
				}

				char ch = lineBuffer[currentPos];

				if ((ch == ' ') ||  (ch == 0x0A) || (ch == 0x0D))
				{
					if (j == 0)
						goto end;

					word[j] = '\0';

					if ((word2Search) && (word2Search.InList(word)))
					{
						currentStat = SCE_SEARCHRESULT_WORD2SEARCH;
					}
					else if ((keywords1) && (keywords1.InList(word)))
					{
						currentStat = SCE_SEARCHRESULT_KWORD1;
					}
					else if ((keywords2) && (keywords2.InList(word)))
					{
						currentStat = SCE_SEARCHRESULT_KWORD2;
					}
					else if ((keywords3) && (keywords3.InList(word)))
					{
						currentStat = SCE_SEARCHRESULT_KWORD3;
					}
					else
					{
						currentStat = SCE_SEARCHRESULT_DEFAULT;
					}
					styler.ColourTo(startLine + currentPos - 1, currentStat);
					currentStat = SCE_SEARCHRESULT_DEFAULT;

					isEndReached = !eatWhiteSpaces(lineBuffer, currentPos);
					break;
				}
				else
					word[j] = ch;
			}

			if (isEndReached)
			{
				styler.ColourTo(endPos, SCE_SEARCHRESULT_DEFAULT);
			}
			else
			{
				styler.ColourTo(startLine + currentPos - 1, currentStat);
			}
		}
end :
		styler.ColourTo(endPos, SCE_SEARCHRESULT_DEFAULT);
	}
}

static void ColouriseSearchResultDoc(unsigned int startPos, int length, int, WordList *keywordlists[], Accessor &styler) {
	char lineBuffer[SC_SEARCHRESULT_LINEBUFFERMAXLENGTH];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;
	for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseSearchResultLine(keywordlists, lineBuffer, linePos, startLine, i, styler);
			linePos = 0;
			startLine = i + 1;
			while (!AtEOL(styler, i)) i++;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColouriseSearchResultLine(keywordlists, lineBuffer, linePos, startLine, startPos + length - 1, styler);
	}
}

// adaption by ksc, using the "} else {" trick of 1.53
// 030721
static void FoldSearchResultDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	bool headerPoint = false;
	int lev;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler[i+1];

		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\n') || (ch == '\r' && chNext != '\n');

		if (style == SCE_SEARCHRESULT_HEARDER) 
		{
			headerPoint = true;
		}

		if (atEOL) {
			lev = SC_FOLDLEVELBASE;

			if (lineCurrent > 0) {
				int levelPrevious = styler.LevelAt(lineCurrent - 1);

				if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
					lev = SC_FOLDLEVELBASE + 1;
				} else {
					lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
				}
			}

			if (headerPoint) {
				lev = SC_FOLDLEVELBASE;
			}
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;

			if (headerPoint) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;
			visibleChars = 0;
			headerPoint = false;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}

	if (lineCurrent > 0) {
		int levelPrevious = styler.LevelAt(lineCurrent - 1);
		if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
			lev = SC_FOLDLEVELBASE + 1;
		} else {
			lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
		}
	} else {
		lev = SC_FOLDLEVELBASE;
	}
	int flagsNext = styler.LevelAt(lineCurrent);
	styler.SetLevel(lineCurrent, lev | flagsNext & ~SC_FOLDLEVELNUMBERMASK);
}

LexerModule lmSearchResult(SCLEX_SEARCHRESULT, ColouriseSearchResultDoc, "searchResult", FoldSearchResultDoc, emptyWordListDesc);
