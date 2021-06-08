// Scintilla source code edit control
/** @file LexMPT.cxx
 ** Lexer for MPT specific files. Based on LexOthers.cxx
 ** LOT = the text log file created by the MPT application while running a test program
 ** Other MPT specific files to be added later.
 **/
// Copyright 2003 by Marius Gheorghe <mgheorghe@cabletest.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Scintilla;

static int GetLotLineState(std::string &line) {
	if (line.length()) {
		// Most of the time the first non-blank character in line determines that line's type
		// Now finds the first non-blank character
		unsigned i; // Declares counter here to make it persistent after the for loop
		for (i = 0; i < line.length(); ++i) {
			if (!(IsASCII(line[i]) && isspace(line[i])))
				break;
		}

		// Checks if it was a blank line
		if (i == line.length())
			return SCE_LOT_DEFAULT;

		switch (line[i]) {
		case '*': // Fail measurement
			return SCE_LOT_FAIL;

		case '+': // Header
		case '|': // Header
			return SCE_LOT_HEADER;

		case ':': // Set test limits
			return SCE_LOT_SET;

		case '-': // Section break
			return SCE_LOT_BREAK;

		default:  // Any other line
			// Checks for message at the end of lot file
			if (line.find("PASSED") != std::string::npos) {
				return SCE_LOT_PASS;
			}
			else if (line.find("FAILED") != std::string::npos) {
				return SCE_LOT_FAIL;
			}
			else if (line.find("ABORTED") != std::string::npos) {
				return SCE_LOT_ABORT;
			}
			else {
				return i ? SCE_LOT_PASS : SCE_LOT_DEFAULT;
			}
		}
	}
	else {
		return SCE_LOT_DEFAULT;
	}
}

static void ColourizeLotDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	bool atLineStart = true;// Arms the 'at line start' flag
	char chNext = styler.SafeGetCharAt(startPos);
	std::string line("");
	line.reserve(256);	// Lot lines are less than 256 chars long most of the time. This should avoid reallocations

	// Styles LOT document
	Sci_PositionU i;			// Declared here because it's used after the for loop
	for (i = startPos; i < startPos + length; ++i) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		line += ch;
		atLineStart = false;

		// LOT files are only used on the Win32 platform, thus EOL == CR+LF
		// Searches for the end of line
		if (ch == '\r' && chNext == '\n') {
			line += chNext; // Gets the '\n'
			++i; // Advances past the '\n'
			chNext = styler.SafeGetCharAt(i + 1); // Gets character of next line
			styler.ColourTo(i, GetLotLineState(line));
			line = "";
			atLineStart = true; // Arms flag for next line
		}
	}

	// Last line may not have a line ending
	if (!atLineStart) {
		styler.ColourTo(i - 1, GetLotLineState(line));
	}
}

// Folds an MPT LOT file: the blocks that can be folded are:
// sections (headed by a set line)
// passes (contiguous pass results within a section)
// fails (contiguous fail results within a section)
static void FoldLotDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	bool foldCompact = styler.GetPropertyInt("fold.compact", 0) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	char chNext = styler.SafeGetCharAt(startPos);
	int style = SCE_LOT_DEFAULT;
	int styleNext = styler.StyleAt(startPos);
	int lev = SC_FOLDLEVELBASE;

	// Gets style of previous line if not at the beginning of the document
	if (startPos > 1)
		style = styler.StyleAt(startPos - 2);

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if (ch == '\r' && chNext == '\n') {
			// TO DO:
			// Should really get the state of the previous line from the styler
			int stylePrev = style;
			style = styleNext;
			styleNext = styler.StyleAt(i + 2);

			switch (style) {
/*
			case SCE_LOT_SET:
				lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				break;
*/
			case SCE_LOT_FAIL:
/*
				if (stylePrev != SCE_LOT_FAIL)
					lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				else
					lev = SC_FOLDLEVELBASE + 1;
*/
				lev = SC_FOLDLEVELBASE;
				break;

			default:
				if (lineCurrent == 0 || stylePrev == SCE_LOT_FAIL)
					lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				else
					lev = SC_FOLDLEVELBASE + 1;

				if (visibleChars == 0 && foldCompact)
					lev |= SC_FOLDLEVELWHITEFLAG;
				break;
			}

			if (lev != styler.LevelAt(lineCurrent))
				styler.SetLevel(lineCurrent, lev);

			lineCurrent++;
			visibleChars = 0;
		}

		if (!isspacechar(ch))
			visibleChars++;
	}

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, lev | flagsNext);
}

static const char * const emptyWordListDesc[] = {
	0
};

LexerModule lmLot(SCLEX_LOT, ColourizeLotDoc, "lot", FoldLotDoc, emptyWordListDesc);
