// Scintilla source code edit control
/** @file LexBaan.cxx
 ** Lexer for Baan.
 ** Based heavily on LexCPP.cxx
 **/
// Copyright 2001- by Vamsi Potluru <vamsi@who.net> & Praveen Ambekar <ambekarpraveen@yahoo.com>
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

static inline bool IsAWordChar(const int  ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch == '$' || ch == ':');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static void ColouriseBaanDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	bool stylingWithinPreprocessor = styler.GetPropertyInt("styling.within.preprocessor") != 0;

	if (initStyle == SCE_BAAN_STRINGEOL)	// Does not leak onto next line
		initStyle = SCE_BAAN_DEFAULT;

	int visibleChars = 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_BAAN_OPERATOR) {
			sc.SetState(SCE_BAAN_DEFAULT);
		} else if (sc.state == SCE_BAAN_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_BAAN_DEFAULT);
			}
		} else if (sc.state == SCE_BAAN_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_BAAN_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_BAAN_WORD2);
				}
				sc.SetState(SCE_BAAN_DEFAULT);
			}
		} else if (sc.state == SCE_BAAN_PREPROCESSOR) {
			if (stylingWithinPreprocessor) {
				if (IsASpace(sc.ch)) {
					sc.SetState(SCE_BAAN_DEFAULT);
				}
			} else {
				if (sc.atLineEnd && (sc.chNext != '^')) {
					sc.SetState(SCE_BAAN_DEFAULT);
				}
			}
		} else if (sc.state == SCE_BAAN_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_BAAN_DEFAULT);
			}
		} else if (sc.state == SCE_BAAN_COMMENTDOC) {
			if (sc.MatchIgnoreCase("enddllusage")) {
				for (unsigned int i = 0; i < 10; i++){
					sc.Forward();
				}
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
			}
		} else if (sc.state == SCE_BAAN_STRING) {
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
			} else if ((sc.atLineEnd) && (sc.chNext != '^')) {
				sc.ChangeState(SCE_BAAN_STRINGEOL);
				sc.ForwardSetState(SCE_C_DEFAULT);
				visibleChars = 0;
			}
		}

		if (sc.state == SCE_BAAN_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_BAAN_NUMBER);
			} else if (sc.MatchIgnoreCase("dllusage")){
					sc.SetState(SCE_BAAN_COMMENTDOC);
					do {
						sc.Forward();
					} while ((!sc.atLineEnd) && sc.More());
			} else if (IsAWordStart(sc.ch)) {
					sc.SetState(SCE_BAAN_IDENTIFIER);
			} else if (sc.Match('|')){
					sc.SetState(SCE_BAAN_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_BAAN_STRING);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_BAAN_PREPROCESSOR);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while (IsASpace(sc.ch) && sc.More());
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_BAAN_OPERATOR);
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

static void FoldBaanDoc(unsigned int startPos, int length, int initStyle, WordList *[],
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment &&
			(style == SCE_BAAN_COMMENT || style == SCE_BAAN_COMMENTDOC)) {
			if (style != stylePrev) {
				levelCurrent++;
			} else if ((style != styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (style == SCE_BAAN_OPERATOR) {
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

LexerModule lmBaan(SCLEX_BAAN, ColouriseBaanDoc, "baan", FoldBaanDoc);
