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
#include <vector>
#include <assert.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "LexAccessor.h"
#include "PropSetSimple.h"
#include "Accessor.h"
#include "WordList.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

// The following definitions are a copy of the ones in FindReplaceDlg.h
enum { searchHeaderLevel = SC_FOLDLEVELBASE, fileHeaderLevel, resultLevel };


static inline bool AtEOL(Accessor &styler, size_t i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

static const char * const emptyWordListDesc[] = {
	0
};

static void ColouriseSearchResultLine(SearchResultMarkings* pMarkings, char *lineBuffer, size_t startLine, size_t endPos, Accessor &styler, int linenum) 
{
	// startLine and endPos are the absolute positions.

	if (lineBuffer[0] == ' ') // white space - file header
	{
		styler.ColourTo(endPos, SCE_SEARCHRESULT_FILE_HEADER);
	}
	else if (lineBuffer[0] == '	')// \t - line info
	{
		unsigned int firstTokenLen = 0;
		unsigned int currentPos = 0;

		
		for (; !(lineBuffer[firstTokenLen] >= '0' && lineBuffer[firstTokenLen] <= '9'); firstTokenLen++);
		
		styler.ColourTo(startLine + firstTokenLen - 1, SCE_SEARCHRESULT_DEFAULT);

		for (currentPos = firstTokenLen; lineBuffer[currentPos] != ':'; currentPos++);

		styler.ColourTo(startLine + currentPos - 1, SCE_SEARCHRESULT_LINE_NUMBER);
		
		int currentState = SCE_SEARCHRESULT_DEFAULT;

		if (linenum >= pMarkings->_length)
			return;
		SearchResultMarkingLine miLine = pMarkings->_markings[linenum];

		for (std::pair<intptr_t, intptr_t> mi : miLine._segmentPostions)
		{
			size_t match_start = startLine + mi.first - 1;
			size_t match_end = startLine + mi.second - 1;

			if (match_start <= endPos)
			{
				styler.ColourTo(match_start, SCE_SEARCHRESULT_DEFAULT);
				if (match_end <= endPos)
					styler.ColourTo(match_end, SCE_SEARCHRESULT_WORD2SEARCH);
				else
					currentState = SCE_SEARCHRESULT_WORD2SEARCH;
			}
		}

		styler.ColourTo(endPos, currentState);
	}
	else // every character - search header
	{
		styler.ColourTo(endPos, SCE_SEARCHRESULT_SEARCH_HEADER);
	}
}

static void ColouriseSearchResultDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {

	char lineBuffer[SC_SEARCHRESULT_LINEBUFFERMAXLENGTH];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	size_t startLine = startPos;

	const char* addrMarkingsStruct = (styler.pprops)->Get("@MarkingsStruct");
	if (!addrMarkingsStruct || !addrMarkingsStruct[0])
		return;

	SearchResultMarkings* pMarkings = NULL;
	sscanf(addrMarkingsStruct, "%p", (void**)&pMarkings);

	if (!pMarkings || !pMarkings->_markings)
		return;

	for (size_t i = startPos; i < startPos + length; i++)
	{
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1))
		{
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseSearchResultLine(pMarkings, lineBuffer, startLine, i, styler, styler.GetLine(startLine));
			linePos = 0;
			startLine = i + 1;

			while (!AtEOL(styler, i))
				i++;
		}
	}

	if (linePos > 0) // Last line does not have ending characters
	{
		ColouriseSearchResultLine(pMarkings, lineBuffer, startLine, startPos + length - 1, styler, styler.GetLine(startLine));
	}
}

static void FoldSearchResultDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	size_t endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int headerPoint = 0;
	int lev;

	for (size_t i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler[i+1];

		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\n') || (ch == '\r' && chNext != '\n');

		if (style == SCE_SEARCHRESULT_FILE_HEADER) 
		{
			headerPoint = fileHeaderLevel;
		}
		else if (style == SCE_SEARCHRESULT_SEARCH_HEADER) 
		{
			headerPoint = searchHeaderLevel;
		}

		if (atEOL) {
			lev = headerPoint ? SC_FOLDLEVELHEADERFLAG + headerPoint : resultLevel;
			headerPoint = 0;

			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;

			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
	styler.SetLevel(lineCurrent, SC_FOLDLEVELBASE);
}

LexerModule lmSearchResult(SCLEX_SEARCHRESULT, ColouriseSearchResultDoc, "searchResult", FoldSearchResultDoc, emptyWordListDesc);
