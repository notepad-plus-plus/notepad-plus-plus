// Scintilla source code edit control
/** @file Lexr.cxx
 ** Lexer for R, S, SPlus Statistics Program (Heavily derived from CPP Lexer).
 **
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

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
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAnOperator(const int ch) {
	if (isascii(ch) && isalnum(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '-' || ch == '+' || ch == '!' || ch == '~' ||
	        ch == '?' || ch == ':' || ch == '*' || ch == '/' ||
	        ch == '^' || ch == '<' || ch == '>' || ch == '=' ||
	        ch == '&' || ch == '|' || ch == '$' || ch == '(' ||
	        ch == ')' || ch == '}' || ch == '{' || ch == '[' ||
		ch == ']')
		return true;
	return false;
}

static void ColouriseRDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords   = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];


	// Do not leak onto next line
	if (initStyle == SCE_R_INFIXEOL)
		initStyle = SCE_R_DEFAULT;


	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart && (sc.state == SCE_R_STRING)) {
			// Prevent SCE_R_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_R_STRING);
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_R_OPERATOR) {
			sc.SetState(SCE_R_DEFAULT);
		} else if (sc.state == SCE_R_NUMBER) {
			if (!IsADigit(sc.ch) && !(sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_R_DEFAULT);
			}
		} else if (sc.state == SCE_R_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_R_KWORD);
				} else if  (keywords2.InList(s)) {
					sc.ChangeState(SCE_R_BASEKWORD);
				} else if  (keywords3.InList(s)) {
					sc.ChangeState(SCE_R_OTHERKWORD);
				}
				sc.SetState(SCE_R_DEFAULT);
			}
		} else if (sc.state == SCE_R_COMMENT) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_R_DEFAULT);
			}
		} else if (sc.state == SCE_R_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_R_DEFAULT);
			}
		} else if (sc.state == SCE_R_INFIX) {
			if (sc.ch == '%') {
				sc.ForwardSetState(SCE_R_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_R_INFIXEOL);
				sc.ForwardSetState(SCE_R_DEFAULT);
			}
		}else if (sc.state == SCE_R_STRING2) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_R_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_R_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_R_NUMBER);
			} else if (IsAWordStart(sc.ch) ) {
				sc.SetState(SCE_R_IDENTIFIER);
			} else if (sc.Match('#')) {
					sc.SetState(SCE_R_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_R_STRING);
			} else if (sc.ch == '%') {
				sc.SetState(SCE_R_INFIX);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_R_STRING2);
			} else if (IsAnOperator(sc.ch)) {
				sc.SetState(SCE_R_OPERATOR);
			}
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldRDoc(unsigned int startPos, int length, int, WordList *[],
                       Accessor &styler) {
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
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_R_OPERATOR) {
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


static const char * const RWordLists[] = {
            "Language Keywords",
            "Base / Default package function",
            "Other Package Functions",
            "Unused",
            "Unused",
            0,
        };



LexerModule lmR(SCLEX_R, ColouriseRDoc, "r", FoldRDoc, RWordLists);
