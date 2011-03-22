// Copyright (c) 1990-2007, Scientific Toolworks, Inc.
// Author: Jason Haslam
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

static void GetRange(unsigned int start,
                     unsigned int end,
                     Accessor &styler,
                     char *s,
                     unsigned int len) {
	unsigned int i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static void ColourisePlmDoc(unsigned int startPos,
                            int length,
                            int initStyle,
                            WordList *keywordlists[],
                            Accessor &styler)
{
	unsigned int endPos = startPos + length;
	int state = initStyle;

	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = styler.SafeGetCharAt(i);
		char chNext = styler.SafeGetCharAt(i + 1);

		if (state == SCE_PLM_DEFAULT) {
			if (ch == '/' && chNext == '*') {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_COMMENT;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_STRING;
			} else if (isdigit(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_NUMBER;
			} else if (isalpha(ch)) {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_IDENTIFIER;
			} else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
			           ch == '=' || ch == '<' || ch == '>' || ch == ':') {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_OPERATOR;
			} else if (ch == '$') {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_CONTROL;
			}
		} else if (state == SCE_PLM_COMMENT) {
			if (ch == '*' && chNext == '/') {
				i++;
				styler.ColourTo(i, state);
				state = SCE_PLM_DEFAULT;
			}
		} else if (state == SCE_PLM_STRING) {
			if (ch == '\'') {
				if (chNext == '\'') {
					i++;
				} else {
					styler.ColourTo(i, state);
					state = SCE_PLM_DEFAULT;
				}
			}
		} else if (state == SCE_PLM_NUMBER) {
			if (!isdigit(ch) && !isalpha(ch) && ch != '$') {
				i--;
				styler.ColourTo(i, state);
				state = SCE_PLM_DEFAULT;
			}
		} else if (state == SCE_PLM_IDENTIFIER) {
			if (!isdigit(ch) && !isalpha(ch) && ch != '$') {
				// Get the entire identifier.
				char word[1024];
				int segmentStart = styler.GetStartSegment();
				GetRange(segmentStart, i - 1, styler, word, sizeof(word));

				i--;
				if (keywordlists[0]->InList(word))
					styler.ColourTo(i, SCE_PLM_KEYWORD);
				else
					styler.ColourTo(i, state);
				state = SCE_PLM_DEFAULT;
			}
		} else if (state == SCE_PLM_OPERATOR) {
			if (ch != '=' && ch != '>') {
				i--;
				styler.ColourTo(i, state);
				state = SCE_PLM_DEFAULT;
			}
		} else if (state == SCE_PLM_CONTROL) {
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, state);
				state = SCE_PLM_DEFAULT;
			}
		}
	}
	styler.ColourTo(endPos - 1, state);
}

static void FoldPlmDoc(unsigned int startPos,
                       int length,
                       int initStyle,
                       WordList *[],
                       Accessor &styler)
{
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
	int startKeyword = 0;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (stylePrev != SCE_PLM_KEYWORD && style == SCE_PLM_KEYWORD)
			startKeyword = i;

		if (style == SCE_PLM_KEYWORD && styleNext != SCE_PLM_KEYWORD) {
			char word[1024];
			GetRange(startKeyword, i, styler, word, sizeof(word));

			if (strcmp(word, "procedure") == 0 || strcmp(word, "do") == 0)
				levelCurrent++;
			else if (strcmp(word, "end") == 0)
				levelCurrent--;
		}

		if (foldComment) {
			if (stylePrev != SCE_PLM_COMMENT && style == SCE_PLM_COMMENT)
				levelCurrent++;
			else if (stylePrev == SCE_PLM_COMMENT && style != SCE_PLM_COMMENT)
				levelCurrent--;
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

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char *const plmWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmPLM(SCLEX_PLM, ColourisePlmDoc, "PL/M", FoldPlmDoc, plmWordListDesc);
