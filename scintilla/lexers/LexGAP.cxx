// Scintilla source code edit control
/** @file LexGAP.cxx
 ** Lexer for the GAP language. (The GAP System for Computational Discrete Algebra)
 ** http://www.gap-system.org
 **/
// Copyright 2007 by Istvan Szollosi ( szteven <at> gmail <dot> com )
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

static inline bool IsGAPOperator(char ch) {
	if (IsASCII(ch) && isalnum(ch)) return false;
	if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
		ch == '^' || ch == ',' || ch == '!' || ch == '.' ||
		ch == '=' || ch == '<' || ch == '>' || ch == '(' ||
		ch == ')' || ch == ';' || ch == '[' || ch == ']' ||
		ch == '{' || ch == '}' || ch == ':' )
		return true;
	return false;
}

static void GetRange(Sci_PositionU start, Sci_PositionU end, Accessor &styler, char *s, Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(styler[start + i]);
		i++;
	}
	s[i] = '\0';
}

static void ColouriseGAPDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler) {

	WordList &keywords1 = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	// Do not leak onto next line
	if (initStyle == SCE_GAP_STRINGEOL) initStyle = SCE_GAP_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		// Prevent SCE_GAP_STRINGEOL from leaking back to previous line
		if ( sc.atLineStart ) {
			if (sc.state == SCE_GAP_STRING) sc.SetState(SCE_GAP_STRING);
			if (sc.state == SCE_GAP_CHAR) sc.SetState(SCE_GAP_CHAR);
		}

		// Handle line continuation generically
		if (sc.ch == '\\' ) {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate
		switch (sc.state) {
			case SCE_GAP_OPERATOR :
				sc.SetState(SCE_GAP_DEFAULT);
				break;

			case SCE_GAP_NUMBER :
				if (!IsADigit(sc.ch)) {
					if (sc.ch == '\\') {
						if (!sc.atLineEnd) {
							if (!IsADigit(sc.chNext)) {
								sc.Forward();
								sc.ChangeState(SCE_GAP_IDENTIFIER);
							}
						}
					} else if (isalpha(sc.ch) || sc.ch == '_') {
						sc.ChangeState(SCE_GAP_IDENTIFIER);
					}
					else sc.SetState(SCE_GAP_DEFAULT);
				}
				break;

			case SCE_GAP_IDENTIFIER :
				if (!(iswordstart(static_cast<char>(sc.ch)) || sc.ch == '$')) {
					if (sc.ch == '\\') sc.Forward();
					else {
						char s[1000];
						sc.GetCurrent(s, sizeof(s));
						if (keywords1.InList(s)) {
							sc.ChangeState(SCE_GAP_KEYWORD);
						} else if (keywords2.InList(s)) {
							sc.ChangeState(SCE_GAP_KEYWORD2);
						} else if (keywords3.InList(s)) {
							sc.ChangeState(SCE_GAP_KEYWORD3);
						} else if (keywords4.InList(s)) {
							sc.ChangeState(SCE_GAP_KEYWORD4);
						}
						sc.SetState(SCE_GAP_DEFAULT);
					}
				}
				break;

			case SCE_GAP_COMMENT :
				if (sc.atLineEnd) {
					sc.SetState(SCE_GAP_DEFAULT);
				}
				break;

			case SCE_GAP_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_GAP_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_GAP_DEFAULT);
				}
				break;

			case SCE_GAP_CHAR:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_GAP_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_GAP_DEFAULT);
				}
				break;

			case SCE_GAP_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_GAP_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered
		if (sc.state == SCE_GAP_DEFAULT) {
			if (IsGAPOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_GAP_OPERATOR);
			}
			else if (IsADigit(sc.ch)) {
				sc.SetState(SCE_GAP_NUMBER);
			} else if (isalpha(sc.ch) || sc.ch == '_' || sc.ch == '\\' || sc.ch == '$' || sc.ch == '~') {
				sc.SetState(SCE_GAP_IDENTIFIER);
				if (sc.ch == '\\') sc.Forward();
			} else if (sc.ch == '#') {
				sc.SetState(SCE_GAP_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_GAP_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_GAP_CHAR);
			}
		}

	}
	sc.Complete();
}

static int ClassifyFoldPointGAP(const char* s) {
	int level = 0;
	if (strcmp(s, "function") == 0 ||
		strcmp(s, "do") == 0 ||
		strcmp(s, "if") == 0 ||
		strcmp(s, "repeat") == 0 ) {
		level = 1;
	} else if (strcmp(s, "end") == 0 ||
			strcmp(s, "od") == 0 ||
			strcmp(s, "fi") == 0 ||
			strcmp(s, "until") == 0 ) {
		level = -1;
	}
	return level;
}

static void FoldGAPDoc( Sci_PositionU startPos, Sci_Position length, int initStyle,   WordList** , Accessor &styler) {
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	Sci_Position lastStart = 0;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (stylePrev != SCE_GAP_KEYWORD && style == SCE_GAP_KEYWORD) {
			// Store last word start point.
			lastStart = i;
		}

		if (stylePrev == SCE_GAP_KEYWORD) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[100];
				GetRange(lastStart, i, styler, s, sizeof(s));
				levelCurrent += ClassifyFoldPointGAP(s);
			}
		}

		if (atEOL) {
			int lev = levelPrev;
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

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const GAPWordListDesc[] = {
	"Keywords 1",
	"Keywords 2",
	"Keywords 3 (unused)",
	"Keywords 4 (unused)",
	0
};

LexerModule lmGAP(
   SCLEX_GAP,
   ColouriseGAPDoc,
   "gap",
   FoldGAPDoc,
   GAPWordListDesc);
