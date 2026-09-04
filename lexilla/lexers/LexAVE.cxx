// SciTE - Scintilla based Text Editor
/** @file LexAVE.cxx
 ** Lexer for Avenue.
 **
  ** Written by Alexey Yutkin <yutkin@geol.msu.ru>.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
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
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}
static inline bool IsEnumChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch)|| ch == '_');
}
static inline bool IsANumberChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' );
}

inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

inline bool isAveOperator(char ch) {
	if (IsASCII(ch) && isalnum(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' ||
		ch == '{' || ch == '}' ||
		ch == '[' || ch == ']' || ch == ';' ||
		ch == '<' || ch == '>' || ch == ',' ||
		ch == '.'  )
		return true;
	return false;
}

static void ColouriseAveDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];

	// Do not leak onto next line
	if (initStyle == SCE_AVE_STRINGEOL) {
		initStyle = SCE_AVE_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			Sci_Position currentLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(currentLine, 0);
		}
		if (sc.atLineStart && (sc.state == SCE_AVE_STRING)) {
			// Prevent SCE_AVE_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_AVE_STRING);
		}


		// Determine if the current state should terminate.
		if (sc.state == SCE_AVE_OPERATOR) {
			sc.SetState(SCE_AVE_DEFAULT);
		} else if (sc.state == SCE_AVE_NUMBER) {
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_AVE_DEFAULT);
			}
		} else if (sc.state == SCE_AVE_ENUM) {
			if (!IsEnumChar(sc.ch)) {
				sc.SetState(SCE_AVE_DEFAULT);
			}
		} else if (sc.state == SCE_AVE_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) || (sc.ch == '.')) {
				char s[100];
				//sc.GetCurrent(s, sizeof(s));
				sc.GetCurrentLowered(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD4);
				} else if (keywords5.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD5);
				} else if (keywords6.InList(s)) {
					sc.ChangeState(SCE_AVE_WORD6);
				}
				sc.SetState(SCE_AVE_DEFAULT);
			}
		} else if (sc.state == SCE_AVE_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_AVE_DEFAULT);
			}
		} else if (sc.state == SCE_AVE_STRING) {
			 if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_AVE_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_AVE_STRINGEOL);
				sc.ForwardSetState(SCE_AVE_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_AVE_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_AVE_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_AVE_IDENTIFIER);
			} else if (sc.Match('\"')) {
				sc.SetState(SCE_AVE_STRING);
			} else if (sc.Match('\'')) {
				sc.SetState(SCE_AVE_COMMENT);
				sc.Forward();
			} else if (isAveOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_AVE_OPERATOR);
			} else if (sc.Match('#')) {
				sc.SetState(SCE_AVE_ENUM);
				sc.Forward();
			}
		}
	}
	sc.Complete();
}

static void FoldAveDoc(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
	Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = static_cast<char>(tolower(styler[startPos]));
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);
	char s[10] = "";

	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = static_cast<char>(tolower(chNext));
		chNext = static_cast<char>(tolower(styler.SafeGetCharAt(i + 1)));
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_AVE_WORD) {
			if (ch == 't' || ch == 'f' || ch == 'w' || ch == 'e') {
				for (unsigned int j = 0; j < 6; j++) {
					if (!iswordchar(styler[i + j])) {
						break;
					}
					s[j] = static_cast<char>(tolower(styler[i + j]));
					s[j + 1] = '\0';
				}

				if ((strcmp(s, "then") == 0) || (strcmp(s, "for") == 0) || (strcmp(s, "while") == 0)) {
					levelCurrent++;
				}
				if ((strcmp(s, "end") == 0) || (strcmp(s, "elseif") == 0)) {
					// Normally "elseif" and "then" will be on the same line and will cancel
					// each other out.  // As implemented, this does not support fold.at.else.
					levelCurrent--;
				}
			}
		} else if (style == SCE_AVE_OPERATOR) {
			if (ch == '{' || ch == '(') {
				levelCurrent++;
			} else if (ch == '}' || ch == ')') {
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
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

extern const LexerModule lmAVE(SCLEX_AVE, ColouriseAveDoc, "ave", FoldAveDoc);

