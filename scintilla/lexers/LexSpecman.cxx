// Scintilla source code edit control
/** @file LexSpecman.cxx
 ** Lexer for Specman E language.
 ** Written by Avi Yegudin, based on C++ lexer by Neil Hodgson
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch == '\'');
}

static inline bool IsANumberChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '\'');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '`');
}

static void ColouriseSpecmanDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler, bool caseSensitive) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	// Do not leak onto next line
	if (initStyle == SCE_SN_STRINGEOL)
		initStyle = SCE_SN_CODE;

	int visibleChars = 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart && (sc.state == SCE_SN_STRING)) {
			// Prevent SCE_SN_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_SN_STRING);
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_SN_OPERATOR) {
			sc.SetState(SCE_SN_CODE);
		} else if (sc.state == SCE_SN_NUMBER) {
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_SN_CODE);
			}
		} else if (sc.state == SCE_SN_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) || (sc.ch == '.')) {
				char s[100];
				if (caseSensitive) {
					sc.GetCurrent(s, sizeof(s));
				} else {
					sc.GetCurrentLowered(s, sizeof(s));
				}
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_SN_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_SN_WORD2);
				} else if (keywords3.InList(s)) {
                                        sc.ChangeState(SCE_SN_WORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_SN_USER);
				}
				sc.SetState(SCE_SN_CODE);
			}
		} else if (sc.state == SCE_SN_PREPROCESSOR) {
                        if (IsASpace(sc.ch)) {
                                sc.SetState(SCE_SN_CODE);
                        }
		} else if (sc.state == SCE_SN_DEFAULT) {
			if (sc.Match('<', '\'')) {
				sc.Forward();
				sc.ForwardSetState(SCE_SN_CODE);
			}
		} else if (sc.state == SCE_SN_COMMENTLINE || sc.state == SCE_SN_COMMENTLINEBANG) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_SN_CODE);
				visibleChars = 0;
			}
		} else if (sc.state == SCE_SN_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_SN_CODE);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_SN_STRINGEOL);
				sc.ForwardSetState(SCE_SN_CODE);
				visibleChars = 0;
			}
		} else if (sc.state == SCE_SN_SIGNAL) {
			if (sc.atLineEnd) {
				sc.ChangeState(SCE_SN_STRINGEOL);
				sc.ForwardSetState(SCE_SN_CODE);
				visibleChars = 0;
			} else if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_SN_CODE);
			}
		} else if (sc.state == SCE_SN_REGEXTAG) {
			if (!IsADigit(sc.ch)) {
				sc.SetState(SCE_SN_CODE);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_SN_CODE) {
			if (sc.ch == '$' && IsADigit(sc.chNext)) {
				sc.SetState(SCE_SN_REGEXTAG);
                                sc.Forward();
			} else if (IsADigit(sc.ch)) {
                                sc.SetState(SCE_SN_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_SN_IDENTIFIER);
			} else if (sc.Match('\'', '>')) {
                                sc.SetState(SCE_SN_DEFAULT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if (sc.Match("//!"))	// Nice to have a different comment style
					sc.SetState(SCE_SN_COMMENTLINEBANG);
				else
					sc.SetState(SCE_SN_COMMENTLINE);
			} else if (sc.Match('-', '-')) {
				if (sc.Match("--!"))	// Nice to have a different comment style
					sc.SetState(SCE_SN_COMMENTLINEBANG);
				else
					sc.SetState(SCE_SN_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_SN_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_SN_SIGNAL);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_SN_PREPROCESSOR);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_SN_CODE);
				}
			} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '@') {
				sc.SetState(SCE_SN_OPERATOR);
			}
		}

		if (sc.atLineEnd) {
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
		}
		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldNoBoxSpecmanDoc(unsigned int startPos, int length, int,
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		//int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && (style == SCE_SN_COMMENTLINE)) {
			if (((ch == '/') && (chNext == '/')) ||
                            ((ch == '-') && (chNext == '-'))) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelNext++;
				} else if (chNext2 == '}') {
					levelNext--;
				}
			}
		}
		if (style == SCE_SN_OPERATOR) {
			if (ch == '{') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}
		if (atEOL) {
			int levelUse = levelCurrent;
			if (foldAtElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
}

static void FoldSpecmanDoc(unsigned int startPos, int length, int initStyle, WordList *[],
                       Accessor &styler) {
	FoldNoBoxSpecmanDoc(startPos, length, initStyle, styler);
}

static const char * const specmanWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Sequence keywords and identifiers",
            "User defined keywords and identifiers",
            "Unused",
            0,
        };

static void ColouriseSpecmanDocSensitive(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                                     Accessor &styler) {
	ColouriseSpecmanDoc(startPos, length, initStyle, keywordlists, styler, true);
}


LexerModule lmSpecman(SCLEX_SPECMAN, ColouriseSpecmanDocSensitive, "specman", FoldSpecmanDoc, specmanWordLists);
