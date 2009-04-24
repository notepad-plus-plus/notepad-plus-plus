// Scintilla source code edit control
/** @file LexLout.cxx
 ** Lexer for the Basser Lout (>= version 3) typesetting language
 **/
// Copyright 2003 by Kein-Hong Man <mkh@pl.jaring.my>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalpha(ch) || ch == '@' || ch == '_');
}

static inline bool IsAnOther(const int ch) {
	return (ch < 0x80) && (ch == '{' || ch == '}' ||
	ch == '!' || ch == '$' || ch == '%' || ch == '&' || ch == '\'' ||
	ch == '(' || ch == ')' || ch == '*' || ch == '+' || ch == ',' ||
	ch == '-' || ch == '.' || ch == '/' || ch == ':' || ch == ';' ||
	ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '[' ||
	ch == ']' || ch == '^' || ch == '`' || ch == '|' || ch == '~');
}

static void ColouriseLoutDoc(unsigned int startPos, int length, int initStyle,
			     WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];

	int visibleChars = 0;
	int firstWordInLine = 0;
	int leadingAtSign = 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart && (sc.state == SCE_LOUT_STRING)) {
			// Prevent SCE_LOUT_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_LOUT_STRING);
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_LOUT_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_LOUT_DEFAULT);
				visibleChars = 0;
			}
		} else if (sc.state == SCE_LOUT_NUMBER) {
			if (!IsADigit(sc.ch) && sc.ch != '.') {
				sc.SetState(SCE_LOUT_DEFAULT);
			}
		} else if (sc.state == SCE_LOUT_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_LOUT_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_LOUT_STRINGEOL);
				sc.ForwardSetState(SCE_LOUT_DEFAULT);
				visibleChars = 0;
			}
		} else if (sc.state == SCE_LOUT_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));

				if (leadingAtSign) {
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_LOUT_WORD);
					} else {
						sc.ChangeState(SCE_LOUT_WORD4);
					}
				} else if (firstWordInLine && keywords3.InList(s)) {
					sc.ChangeState(SCE_LOUT_WORD3);
				}
				sc.SetState(SCE_LOUT_DEFAULT);
			}
		} else if (sc.state == SCE_LOUT_OPERATOR) {
			if (!IsAnOther(sc.ch)) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));

				if (keywords2.InList(s)) {
					sc.ChangeState(SCE_LOUT_WORD2);
				}
				sc.SetState(SCE_LOUT_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_LOUT_DEFAULT) {
			if (sc.ch == '#') {
				sc.SetState(SCE_LOUT_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_LOUT_STRING);
			} else if (IsADigit(sc.ch) ||
				  (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_LOUT_NUMBER);
			} else if (IsAWordChar(sc.ch)) {
				firstWordInLine = (visibleChars == 0);
				leadingAtSign = (sc.ch == '@');
				sc.SetState(SCE_LOUT_IDENTIFIER);
			} else if (IsAnOther(sc.ch)) {
				sc.SetState(SCE_LOUT_OPERATOR);
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

static void FoldLoutDoc(unsigned int startPos, int length, int, WordList *[],
                        Accessor &styler) {

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);
	char s[10];

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_LOUT_WORD) {
			if (ch == '@') {
				for (unsigned int j = 0; j < 8; j++) {
					if (!IsAWordChar(styler[i + j])) {
						break;
					}
					s[j] = styler[i + j];
					s[j + 1] = '\0';
				}
				if (strcmp(s, "@Begin") == 0) {
					levelCurrent++;
				} else if (strcmp(s, "@End") == 0) {
					levelCurrent--;
				}
			}
		} else if (style == SCE_LOUT_OPERATOR) {
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}
		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
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

static const char * const loutWordLists[] = {
            "Predefined identifiers",
            "Predefined delimiters",
            "Predefined keywords",
            0,
        };

LexerModule lmLout(SCLEX_LOUT, ColouriseLoutDoc, "lout", FoldLoutDoc, loutWordLists);
