// Scintilla source code edit control
/** @file LexDiff.cxx
 ** Lexer for diff results.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

static inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

#define DIFF_BUFFER_START_SIZE 16
// Note that ColouriseDiffLine analyzes only the first DIFF_BUFFER_START_SIZE
// characters of each line to classify the line.

static void ColouriseDiffLine(char *lineBuffer, Sci_Position endLine, Accessor &styler) {
	// It is needed to remember the current state to recognize starting
	// comment lines before the first "diff " or "--- ". If a real
	// difference starts then each line starting with ' ' is a whitespace
	// otherwise it is considered a comment (Only in..., Binary file...)
	if (0 == strncmp(lineBuffer, "diff ", 5)) {
		styler.ColourTo(endLine, SCE_DIFF_COMMAND);
	} else if (0 == strncmp(lineBuffer, "Index: ", 7)) {  // For subversion's diff
		styler.ColourTo(endLine, SCE_DIFF_COMMAND);
	} else if (0 == strncmp(lineBuffer, "---", 3) && lineBuffer[3] != '-') {
		// In a context diff, --- appears in both the header and the position markers
		if (lineBuffer[3] == ' ' && atoi(lineBuffer + 4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else if (lineBuffer[3] == '\r' || lineBuffer[3] == '\n')
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else if (lineBuffer[3] == ' ')
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
		else
			styler.ColourTo(endLine, SCE_DIFF_DELETED);
	} else if (0 == strncmp(lineBuffer, "+++ ", 4)) {
		// I don't know of any diff where "+++ " is a position marker, but for
		// consistency, do the same as with "--- " and "*** ".
		if (atoi(lineBuffer+4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "====", 4)) {  // For p4's diff
		styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "***", 3)) {
		// In a context diff, *** appears in both the header and the position markers.
		// Also ******** is a chunk header, but here it's treated as part of the
		// position marker since there is no separate style for a chunk header.
		if (lineBuffer[3] == ' ' && atoi(lineBuffer+4) && !strchr(lineBuffer, '/'))
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else if (lineBuffer[3] == '*')
			styler.ColourTo(endLine, SCE_DIFF_POSITION);
		else
			styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (0 == strncmp(lineBuffer, "? ", 2)) {    // For difflib
		styler.ColourTo(endLine, SCE_DIFF_HEADER);
	} else if (lineBuffer[0] == '@') {
		styler.ColourTo(endLine, SCE_DIFF_POSITION);
	} else if (lineBuffer[0] >= '0' && lineBuffer[0] <= '9') {
		styler.ColourTo(endLine, SCE_DIFF_POSITION);
	} else if (0 == strncmp(lineBuffer, "++", 2)) {
		styler.ColourTo(endLine, SCE_DIFF_PATCH_ADD);
	} else if (0 == strncmp(lineBuffer, "+-", 2)) {
		styler.ColourTo(endLine, SCE_DIFF_PATCH_DELETE);
	} else if (0 == strncmp(lineBuffer, "-+", 2)) {
		styler.ColourTo(endLine, SCE_DIFF_REMOVED_PATCH_ADD);
	} else if (0 == strncmp(lineBuffer, "--", 2)) {
		styler.ColourTo(endLine, SCE_DIFF_REMOVED_PATCH_DELETE);
	} else if (lineBuffer[0] == '-' || lineBuffer[0] == '<') {
		styler.ColourTo(endLine, SCE_DIFF_DELETED);
	} else if (lineBuffer[0] == '+' || lineBuffer[0] == '>') {
		styler.ColourTo(endLine, SCE_DIFF_ADDED);
	} else if (lineBuffer[0] == '!') {
		styler.ColourTo(endLine, SCE_DIFF_CHANGED);
	} else if (lineBuffer[0] != ' ') {
		styler.ColourTo(endLine, SCE_DIFF_COMMENT);
	} else {
		styler.ColourTo(endLine, SCE_DIFF_DEFAULT);
	}
}

static void ColouriseDiffDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	char lineBuffer[DIFF_BUFFER_START_SIZE] = "";
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		if (AtEOL(styler, i)) {
			if (linePos < DIFF_BUFFER_START_SIZE) {
				lineBuffer[linePos] = 0;
			}
			ColouriseDiffLine(lineBuffer, i, styler);
			linePos = 0;
		} else if (linePos < DIFF_BUFFER_START_SIZE - 1) {
			lineBuffer[linePos++] = styler[i];
		} else if (linePos == DIFF_BUFFER_START_SIZE - 1) {
			lineBuffer[linePos++] = 0;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		if (linePos < DIFF_BUFFER_START_SIZE) {
			lineBuffer[linePos] = 0;
		}
		ColouriseDiffLine(lineBuffer, startPos + length - 1, styler);
	}
}

static void FoldDiffDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	Sci_Position curLine = styler.GetLine(startPos);
	Sci_Position curLineStart = styler.LineStart(curLine);
	int prevLevel = curLine > 0 ? styler.LevelAt(curLine - 1) : SC_FOLDLEVELBASE;
	int nextLevel;

	do {
		const int lineType = styler.StyleAt(curLineStart);
		if (lineType == SCE_DIFF_COMMAND)
			nextLevel = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
		else if (lineType == SCE_DIFF_HEADER)
			nextLevel = (SC_FOLDLEVELBASE + 1) | SC_FOLDLEVELHEADERFLAG;
		else if (lineType == SCE_DIFF_POSITION && styler[curLineStart] != '-')
			nextLevel = (SC_FOLDLEVELBASE + 2) | SC_FOLDLEVELHEADERFLAG;
		else if (prevLevel & SC_FOLDLEVELHEADERFLAG)
			nextLevel = (prevLevel & SC_FOLDLEVELNUMBERMASK) + 1;
		else
			nextLevel = prevLevel;

		if ((nextLevel & SC_FOLDLEVELHEADERFLAG) && (nextLevel == prevLevel))
			styler.SetLevel(curLine-1, prevLevel & ~SC_FOLDLEVELHEADERFLAG);

		styler.SetLevel(curLine, nextLevel);
		prevLevel = nextLevel;

		curLineStart = styler.LineStart(++curLine);
	} while (static_cast<Sci_Position>(startPos)+length > curLineStart);
}

static const char *const emptyWordListDesc[] = {
	0
};

LexerModule lmDiff(SCLEX_DIFF, ColouriseDiffDoc, "diff", FoldDiffDoc, emptyWordListDesc);
