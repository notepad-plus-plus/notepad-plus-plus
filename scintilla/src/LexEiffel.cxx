// Scintilla source code edit control
/** @file LexEiffel.cxx
 ** Lexer for Eiffel.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

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

static inline bool isEiffelOperator(unsigned int ch) {
	// '.' left out as it is used to make up numbers
	return ch == '*' || ch == '/' || ch == '\\' || ch == '-' || ch == '+' ||
	        ch == '(' || ch == ')' || ch == '=' ||
	        ch == '{' || ch == '}' || ch == '~' ||
	        ch == '[' || ch == ']' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' ||
	        ch == '.' || ch == '^' || ch == '%' || ch == ':' ||
		ch == '!' || ch == '@' || ch == '?';
}

static inline bool IsAWordChar(unsigned int  ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(unsigned int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static void ColouriseEiffelDoc(unsigned int startPos,
                            int length,
                            int initStyle,
                            WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords = *keywordlists[0];

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_EIFFEL_STRINGEOL) {
			if (sc.ch != '\r' && sc.ch != '\n') {
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		} else if (sc.state == SCE_EIFFEL_OPERATOR) {
			sc.SetState(SCE_EIFFEL_DEFAULT);
		} else if (sc.state == SCE_EIFFEL_WORD) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (!keywords.InList(s)) {
					sc.ChangeState(SCE_EIFFEL_IDENTIFIER);
				}
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		} else if (sc.state == SCE_EIFFEL_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		} else if (sc.state == SCE_EIFFEL_COMMENTLINE) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		} else if (sc.state == SCE_EIFFEL_STRING) {
			if (sc.ch == '%') {
				sc.Forward();
			} else if (sc.ch == '\"') {
				sc.Forward();
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		} else if (sc.state == SCE_EIFFEL_CHARACTER) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_EIFFEL_STRINGEOL);
			} else if (sc.ch == '%') {
				sc.Forward();
			} else if (sc.ch == '\'') {
				sc.Forward();
				sc.SetState(SCE_EIFFEL_DEFAULT);
			}
		}

		if (sc.state == SCE_EIFFEL_DEFAULT) {
			if (sc.ch == '-' && sc.chNext == '-') {
				sc.SetState(SCE_EIFFEL_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_EIFFEL_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_EIFFEL_CHARACTER);
			} else if (IsADigit(sc.ch) || (sc.ch == '.')) {
				sc.SetState(SCE_EIFFEL_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_EIFFEL_WORD);
			} else if (isEiffelOperator(sc.ch)) {
				sc.SetState(SCE_EIFFEL_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static bool IsEiffelComment(Accessor &styler, int pos, int len) {
	return len>1 && styler[pos]=='-' && styler[pos+1]=='-';
}

static void FoldEiffelDocIndent(unsigned int startPos, int length, int,
						   WordList *[], Accessor &styler) {
	int lengthDoc = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsEiffelComment);
	char chNext = styler[startPos];
	for (int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == lengthDoc)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsEiffelComment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsEiffelComment);
					if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK)) {
						lev |= SC_FOLDLEVELHEADERFLAG;
					}
				}
			}
			indentCurrent = indentNext;
			styler.SetLevel(lineCurrent, lev);
			lineCurrent++;
		}
	}
}

static void FoldEiffelDocKeyWords(unsigned int startPos, int length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
	unsigned int lengthDoc = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int stylePrev = 0;
	int styleNext = styler.StyleAt(startPos);
	// lastDeferred should be determined by looking back to last keyword in case
	// the "deferred" is on a line before "class"
	bool lastDeferred = false;
	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if ((stylePrev != SCE_EIFFEL_WORD) && (style == SCE_EIFFEL_WORD)) {
			char s[20];
			unsigned int j = 0;
			while ((j < (sizeof(s) - 1)) && (iswordchar(styler[i + j]))) {
				s[j] = styler[i + j];
				j++;
			}
			s[j] = '\0';

			if (
				(strcmp(s, "check") == 0) ||
				(strcmp(s, "debug") == 0) ||
				(strcmp(s, "deferred") == 0) ||
				(strcmp(s, "do") == 0) ||
				(strcmp(s, "from") == 0) ||
				(strcmp(s, "if") == 0) ||
				(strcmp(s, "inspect") == 0) ||
				(strcmp(s, "once") == 0)
			)
				levelCurrent++;
			if (!lastDeferred && (strcmp(s, "class") == 0))
				levelCurrent++;
			if (strcmp(s, "end") == 0)
				levelCurrent--;
			lastDeferred = strcmp(s, "deferred") == 0;
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0)
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
		stylePrev = style;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const eiffelWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmEiffel(SCLEX_EIFFEL, ColouriseEiffelDoc, "eiffel", FoldEiffelDocIndent, eiffelWordListDesc);
LexerModule lmEiffelkw(SCLEX_EIFFELKW, ColouriseEiffelDoc, "eiffelkw", FoldEiffelDocKeyWords, eiffelWordListDesc);
