// Scintilla source code edit control
/** @file LexAVS.cxx
 ** Lexer for AviSynth.
 **/
// Copyright 2012 by Bruno Barbieri <brunorex@gmail.com>
// Heavily based on LexPOV by Neil Hodgson
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

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return isalpha(ch) || (ch != ' ' && ch != '\n' && ch != '(' && ch != '.' && ch != ',');
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
			(isdigit(ch) || ch == '.' || ch == '-' || ch == '+');
}

static void ColouriseAvsDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &filters = *keywordlists[1];
	WordList &plugins = *keywordlists[2];
	WordList &functions = *keywordlists[3];
	WordList &clipProperties = *keywordlists[4];
	WordList &userDefined = *keywordlists[5];

	Sci_Position currentLine = styler.GetLine(startPos);
	// Initialize the block comment nesting level, if we are inside such a comment.
	int blockCommentLevel = 0;
	if (initStyle == SCE_AVS_COMMENTBLOCK || initStyle == SCE_AVS_COMMENTBLOCKN) {
		blockCommentLevel = styler.GetLineState(currentLine - 1);
	}

	// Do not leak onto next line
	if (initStyle == SCE_AVS_COMMENTLINE) {
		initStyle = SCE_AVS_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			if (sc.state == SCE_AVS_COMMENTBLOCK || sc.state == SCE_AVS_COMMENTBLOCKN) {
				// Inside a block comment, we set the line state
				styler.SetLineState(currentLine, blockCommentLevel);
			} else {
				// Reset the line state
				styler.SetLineState(currentLine, 0);
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_AVS_OPERATOR) {
			sc.SetState(SCE_AVS_DEFAULT);
		} else if (sc.state == SCE_AVS_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-sign char
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_AVS_DEFAULT);
			}
		} else if (sc.state == SCE_AVS_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (keywords.InList(s)) {
					sc.ChangeState(SCE_AVS_KEYWORD);
				} else if (filters.InList(s)) {
					sc.ChangeState(SCE_AVS_FILTER);
				} else if (plugins.InList(s)) {
					sc.ChangeState(SCE_AVS_PLUGIN);
				} else if (functions.InList(s)) {
					sc.ChangeState(SCE_AVS_FUNCTION);
				} else if (clipProperties.InList(s)) {
					sc.ChangeState(SCE_AVS_CLIPPROP);
				} else if (userDefined.InList(s)) {
					sc.ChangeState(SCE_AVS_USERDFN);
				}
				sc.SetState(SCE_AVS_DEFAULT);
			}
		} else if (sc.state == SCE_AVS_COMMENTBLOCK) {
			if (sc.Match('/', '*')) {
				blockCommentLevel++;
				sc.Forward();
			} else if (sc.Match('*', '/') && blockCommentLevel > 0) {
				blockCommentLevel--;
				sc.Forward();
				if (blockCommentLevel == 0) {
					sc.ForwardSetState(SCE_AVS_DEFAULT);
				}
			}
		} else if (sc.state == SCE_AVS_COMMENTBLOCKN) {
			if (sc.Match('[', '*')) {
				blockCommentLevel++;
				sc.Forward();
			} else if (sc.Match('*', ']') && blockCommentLevel > 0) {
				blockCommentLevel--;
				sc.Forward();
				if (blockCommentLevel == 0) {
					sc.ForwardSetState(SCE_AVS_DEFAULT);
				}
			}
		} else if (sc.state == SCE_AVS_COMMENTLINE) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_AVS_DEFAULT);
			}
		} else if (sc.state == SCE_AVS_STRING) {
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_AVS_DEFAULT);
			}
		} else if (sc.state == SCE_AVS_TRIPLESTRING) {
			if (sc.Match("\"\"\"")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_AVS_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_AVS_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_AVS_NUMBER);
			} else 	if (IsADigit(sc.ch) || (sc.ch == ',' && IsADigit(sc.chNext))) {
				sc.Forward();
				sc.SetState(SCE_AVS_NUMBER);
			} else if (sc.Match('/', '*')) {
				blockCommentLevel = 1;
				sc.SetState(SCE_AVS_COMMENTBLOCK);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('[', '*')) {
				blockCommentLevel = 1;
				sc.SetState(SCE_AVS_COMMENTBLOCKN);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.ch == '#') {
				sc.SetState(SCE_AVS_COMMENTLINE);
			} else if (sc.ch == '\"') {
				if (sc.Match("\"\"\"")) {
					sc.SetState(SCE_AVS_TRIPLESTRING);
				} else {
					sc.SetState(SCE_AVS_STRING);
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_AVS_OPERATOR);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_AVS_IDENTIFIER);
			}
		}
	}

	// End of file: complete any pending changeState
	if (sc.state == SCE_AVS_IDENTIFIER) {
		if (!IsAWordChar(sc.ch)) {
			char s[100];
			sc.GetCurrentLowered(s, sizeof(s));

			if (keywords.InList(s)) {
				sc.ChangeState(SCE_AVS_KEYWORD);
			} else if (filters.InList(s)) {
				sc.ChangeState(SCE_AVS_FILTER);
			} else if (plugins.InList(s)) {
				sc.ChangeState(SCE_AVS_PLUGIN);
			} else if (functions.InList(s)) {
				sc.ChangeState(SCE_AVS_FUNCTION);
			} else if (clipProperties.InList(s)) {
				sc.ChangeState(SCE_AVS_CLIPPROP);
			} else if (userDefined.InList(s)) {
				sc.ChangeState(SCE_AVS_USERDFN);
			}
			sc.SetState(SCE_AVS_DEFAULT);
		}
	}

	sc.Complete();
}

static void FoldAvsDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int initStyle,
	WordList *[],
	Accessor &styler) {

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && style == SCE_AVS_COMMENTBLOCK) {
			if (stylePrev != SCE_AVS_COMMENTBLOCK) {
				levelCurrent++;
			} else if ((styleNext != SCE_AVS_COMMENTBLOCK) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}

		if (foldComment && style == SCE_AVS_COMMENTBLOCKN) {
			if (stylePrev != SCE_AVS_COMMENTBLOCKN) {
				levelCurrent++;
			} else if ((styleNext != SCE_AVS_COMMENTBLOCKN) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}

		if (style == SCE_AVS_OPERATOR) {
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}

		if (!isspacechar(ch))
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const avsWordLists[] = {
	"Keywords",
	"Filters",
	"Plugins",
	"Functions",
	"Clip properties",
	"User defined functions",
	0,
};

extern const LexerModule lmAVS(SCLEX_AVS, ColouriseAvsDoc, "avs", FoldAvsDoc, avsWordLists);
