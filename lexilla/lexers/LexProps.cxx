// Scintilla source code edit control
/** @file LexProps.cxx
 ** Lexer for properties files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

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

namespace {

bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

constexpr bool isAssignChar(char ch) noexcept {
	return (ch == '=') || (ch == ':');
}

void ColourisePropsLine(
	const char *lineBuffer,
	Sci_PositionU lengthLine,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
	Accessor &styler,
	bool allowInitialSpaces) {

	Sci_PositionU i = 0;
	if (allowInitialSpaces) {
		while ((i < lengthLine) && isspacechar(lineBuffer[i]))	// Skip initial spaces
			i++;
	} else {
		if (isspacechar(lineBuffer[i])) // don't allow initial spaces
			i = lengthLine;
	}

	if (i < lengthLine) {
		if (lineBuffer[i] == '#' || lineBuffer[i] == '!' || lineBuffer[i] == ';') {
			styler.ColourTo(endPos, SCE_PROPS_COMMENT);
		} else if (lineBuffer[i] == '[') {
			styler.ColourTo(endPos, SCE_PROPS_SECTION);
		} else if (lineBuffer[i] == '@') {
			styler.ColourTo(startLine + i, SCE_PROPS_DEFVAL);
			if (isAssignChar(lineBuffer[i++]))
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
			styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
		} else {
			// Search for the '=' character
			while ((i < lengthLine) && !isAssignChar(lineBuffer[i]))
				i++;
			if ((i < lengthLine) && isAssignChar(lineBuffer[i])) {
				styler.ColourTo(startLine + i - 1, SCE_PROPS_KEY);
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			} else {
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			}
		}
	} else {
		styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
	}
}

void ColourisePropsDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;

	// property lexer.props.allow.initial.spaces
	//	For properties files, set to 0 to style all lines that start with whitespace in the default style.
	//	This is not suitable for SciTE .properties files which use indentation for flow control but
	//	can be used for RFC2822 text where indentation is used for continuation lines.
	const bool allowInitialSpaces = styler.GetPropertyInt("lexer.props.allow.initial.spaces", 1) != 0;

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line (or of line buffer) met, colourise it
			ColourisePropsLine(lineBuffer.c_str(), lineBuffer.length(), startLine, i, styler, allowInitialSpaces);
			lineBuffer.clear();
			startLine = i + 1;
		}
	}
	if (lineBuffer.length() > 0) {	// Last line does not have ending characters
		ColourisePropsLine(lineBuffer.c_str(), lineBuffer.length(), startLine, startPos + length - 1, styler, allowInitialSpaces);
	}
}

// adaption by ksc, using the "} else {" trick of 1.53
// 030721
void FoldPropsDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	const bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	const Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	bool headerPoint = false;
	int levelPrevious = (lineCurrent > 0) ? styler.LevelAt(lineCurrent - 1) : SC_FOLDLEVELBASE;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler[i+1];

		const int style = styler.StyleIndexAt(i);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_PROPS_SECTION) {
			headerPoint = true;
		}

		if (atEOL) {
			int lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
			if (headerPoint) {
				lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
					// previous section is empty
					styler.SetLevel(lineCurrent - 1, SC_FOLDLEVELBASE);
				}
			} else if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
				lev += 1;
			}

			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;
			visibleChars = 0;
			headerPoint = false;
			levelPrevious = lev;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}

	int level = levelPrevious & SC_FOLDLEVELNUMBERMASK;
	if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
		level += 1;
	}
	const int flagsNext = styler.LevelAt(lineCurrent);
	styler.SetLevel(lineCurrent, level | (flagsNext & ~SC_FOLDLEVELNUMBERMASK));
}

const char *const emptyWordListDesc[] = {
	nullptr
};

}

extern const LexerModule lmProps(SCLEX_PROPERTIES, ColourisePropsDoc, "props", FoldPropsDoc, emptyWordListDesc);
