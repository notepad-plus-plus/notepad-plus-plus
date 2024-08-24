// Scintilla source code edit control
/** @file LexLisp.cxx
 ** Lexer for Lisp.
 ** Written by Alexey Yutkin.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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

#define SCE_LISP_CHARACTER 29
#define SCE_LISP_MACRO 30
#define SCE_LISP_MACRO_DISPATCH 31

static inline bool isLispoperator(char ch) {
	if (IsASCII(ch) && isalnum(ch))
		return false;
	if (ch == '\'' || ch == '`' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}')
		return true;
	return false;
}

static inline bool isLispwordstart(char ch) {
	return IsASCII(ch) && ch != ';'  && !isspacechar(ch) && !isLispoperator(ch) &&
		ch != '\n' && ch != '\r' &&  ch != '\"';
}


static void classifyWordLisp(Sci_PositionU start, Sci_PositionU end, WordList &keywords, WordList &keywords_kw, Accessor &styler) {
	assert(end >= start);
	char s[100];
	Sci_PositionU i;
	bool digit_flag = true;
	for (i = 0; (i < end - start + 1) && (i < 99); i++) {
		s[i] = styler[start + i];
		s[i + 1] = '\0';
		if (!isdigit(s[i]) && (s[i] != '.')) digit_flag = false;
	}
	char chAttr = SCE_LISP_IDENTIFIER;

	if(digit_flag) chAttr = SCE_LISP_NUMBER;
	else {
		if (keywords.InList(s)) {
			chAttr = SCE_LISP_KEYWORD;
		} else if (keywords_kw.InList(s)) {
			chAttr = SCE_LISP_KEYWORD_KW;
		} else if ((s[0] == '*' && s[i-1] == '*') ||
			   (s[0] == '+' && s[i-1] == '+')) {
			chAttr = SCE_LISP_SPECIAL;
		}
	}
	styler.ColourTo(end, chAttr);
	return;
}


static void ColouriseLispDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords_kw = *keywordlists[1];

	styler.StartAt(startPos);

	int state = initStyle, radix = -1;
	char chNext = styler[startPos];
	Sci_PositionU lengthDoc = startPos + length;
	styler.StartSegment(startPos);
	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			i += 1;
			continue;
		}

		if (state == SCE_LISP_DEFAULT) {
			if (ch == '#') {
				styler.ColourTo(i - 1, state);
				radix = -1;
				state = SCE_LISP_MACRO_DISPATCH;
			} else if (ch == ':' && isLispwordstart(chNext)) {
				styler.ColourTo(i - 1, state);
				state = SCE_LISP_SYMBOL;
			} else if (isLispwordstart(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_LISP_IDENTIFIER;
			}
			else if (ch == ';') {
				styler.ColourTo(i - 1, state);
				state = SCE_LISP_COMMENT;
			}
			else if (isLispoperator(ch) || ch=='\'') {
				styler.ColourTo(i - 1, state);
				styler.ColourTo(i, SCE_LISP_OPERATOR);
				if (ch=='\'' && isLispwordstart(chNext)) {
					state = SCE_LISP_SYMBOL;
				}
			}
			else if (ch == '\"') {
				styler.ColourTo(i - 1, state);
				state = SCE_LISP_STRING;
			}
		} else if (state == SCE_LISP_IDENTIFIER || state == SCE_LISP_SYMBOL) {
			if (!isLispwordstart(ch)) {
				if (state == SCE_LISP_IDENTIFIER) {
					classifyWordLisp(styler.GetStartSegment(), i - 1, keywords, keywords_kw, styler);
				} else {
					styler.ColourTo(i - 1, state);
				}
				state = SCE_LISP_DEFAULT;
			} /*else*/
			if (isLispoperator(ch) || ch=='\'') {
				styler.ColourTo(i - 1, state);
				styler.ColourTo(i, SCE_LISP_OPERATOR);
				if (ch=='\'' && isLispwordstart(chNext)) {
					state = SCE_LISP_SYMBOL;
				}
			}
		} else if (state == SCE_LISP_MACRO_DISPATCH) {
			if (!(IsASCII(ch) && isdigit(ch))) {
				if (ch != 'r' && ch != 'R' && (i - styler.GetStartSegment()) > 1) {
					state = SCE_LISP_DEFAULT;
				} else {
					switch (ch) {
						case '|': state = SCE_LISP_MULTI_COMMENT; break;
						case 'o':
						case 'O': radix = 8; state = SCE_LISP_MACRO; break;
						case 'x':
						case 'X': radix = 16; state = SCE_LISP_MACRO; break;
						case 'b':
						case 'B': radix = 2; state = SCE_LISP_MACRO; break;
						case '\\': state = SCE_LISP_CHARACTER; break;
						case ':':
						case '-':
						case '+': state = SCE_LISP_MACRO; break;
						case '\'': if (isLispwordstart(chNext)) {
								   state = SCE_LISP_SPECIAL;
							   } else {
								   styler.ColourTo(i - 1, SCE_LISP_DEFAULT);
								   styler.ColourTo(i, SCE_LISP_OPERATOR);
								   state = SCE_LISP_DEFAULT;
							   }
							   break;
						default: if (isLispoperator(ch)) {
								 styler.ColourTo(i - 1, SCE_LISP_DEFAULT);
								 styler.ColourTo(i, SCE_LISP_OPERATOR);
							 }
							 state = SCE_LISP_DEFAULT;
							 break;
					}
				}
			}
		} else if (state == SCE_LISP_MACRO) {
			if (isLispwordstart(ch) && (radix == -1 || IsADigit(ch, radix))) {
				state = SCE_LISP_SPECIAL;
			} else {
				state = SCE_LISP_DEFAULT;
			}
		} else if (state == SCE_LISP_CHARACTER) {
			if (isLispoperator(ch)) {
				styler.ColourTo(i, SCE_LISP_SPECIAL);
				state = SCE_LISP_DEFAULT;
			} else if (isLispwordstart(ch)) {
				styler.ColourTo(i, SCE_LISP_SPECIAL);
				state = SCE_LISP_SPECIAL;
			} else {
				state = SCE_LISP_DEFAULT;
			}
		} else if (state == SCE_LISP_SPECIAL) {
			if (!isLispwordstart(ch) || (radix != -1 && !IsADigit(ch, radix))) {
				styler.ColourTo(i - 1, state);
				state = SCE_LISP_DEFAULT;
			}
			if (isLispoperator(ch) || ch=='\'') {
				styler.ColourTo(i - 1, state);
				styler.ColourTo(i, SCE_LISP_OPERATOR);
				if (ch=='\'' && isLispwordstart(chNext)) {
					state = SCE_LISP_SYMBOL;
				}
			}
		} else {
			if (state == SCE_LISP_COMMENT) {
				if (atEOL) {
					styler.ColourTo(i - 1, state);
					state = SCE_LISP_DEFAULT;
				}
			} else if (state == SCE_LISP_MULTI_COMMENT) {
				if (ch == '|' && chNext == '#') {
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
					styler.ColourTo(i, state);
					state = SCE_LISP_DEFAULT;
				}
			} else if (state == SCE_LISP_STRING) {
				if (ch == '\\') {
					if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
						i++;
						chNext = styler.SafeGetCharAt(i + 1);
					}
				} else if (ch == '\"') {
					styler.ColourTo(i, state);
					state = SCE_LISP_DEFAULT;
				}
			}
		}

	}
	styler.ColourTo(lengthDoc - 1, state);
}

static void FoldLispDoc(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, WordList *[],
                            Accessor &styler) {
	Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_LISP_OPERATOR) {
			if (ch == '(' || ch == '[' || ch == '{') {
				levelCurrent++;
			} else if (ch == ')' || ch == ']' || ch == '}') {
				levelCurrent--;
			}
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
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const lispWordListDesc[] = {
	"Functions and special operators",
	"Keywords",
	0
};

extern const LexerModule lmLISP(SCLEX_LISP, ColouriseLispDoc, "lisp", FoldLispDoc, lispWordListDesc);
