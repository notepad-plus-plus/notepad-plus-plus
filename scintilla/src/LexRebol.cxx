// Scintilla source code edit control
/** @file LexRebol.cxx
 ** Lexer for REBOL.
 ** Written by Pascal Hurni, inspired from LexLua by Paul Winwood & Marcos E. Wurzius & Philippe Lhoste
 **
 ** History:
 **		2005-04-07	First release.
 **		2005-04-10	Closing parens and brackets go now in default style
 **					String and comment nesting should be more safe
 **/
// Copyright 2005 by Pascal Hurni <pascal_hurni@fastmail.fm>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "StyleContext.h"


static inline bool IsAWordChar(const int ch) {
	return (isalnum(ch) || ch == '?' || ch == '!' || ch == '.' || ch == '\'' || ch == '+' || ch == '-' || ch == '*' || ch == '&' || ch == '|' || ch == '=' || ch == '_' || ch == '~');
}

static inline bool IsAWordStart(const int ch, const int ch2) {
	return ((ch == '+' || ch == '-' || ch == '.') && !isdigit(ch2)) ||
		(isalpha(ch) || ch == '?' || ch == '!' || ch == '\'' || ch == '*' || ch == '&' || ch == '|' || ch == '=' || ch == '_' || ch == '~');
}

static inline bool IsAnOperator(const int ch, const int ch2, const int ch3) {
	// One char operators
	if (IsASpaceOrTab(ch2)) {
		return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '<' || ch == '>' || ch == '=' || ch == '?';
	}

	// Two char operators
	if (IsASpaceOrTab(ch3)) {
		return (ch == '*' && ch2 == '*') ||
			   (ch == '/' && ch2 == '/') ||
			   (ch == '<' && (ch2 == '=' || ch2 == '>')) ||
			   (ch == '>' && ch2 == '=') ||
			   (ch == '=' && (ch2 == '=' || ch2 == '?')) ||
			   (ch == '?' && ch2 == '?');
	}

	return false;
}

static inline bool IsBinaryStart(const int ch, const int ch2, const int ch3, const int ch4) {
	return (ch == '#' && ch2 == '{') ||
		   (IsADigit(ch) && ch2 == '#' && ch3 == '{' ) ||
		   (IsADigit(ch) && IsADigit(ch2) && ch3 == '#' && ch4 == '{' );
}


static void ColouriseRebolDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
	WordList &keywords8 = *keywordlists[7];

	int currentLine = styler.GetLine(startPos);
	// Initialize the braced string {.. { ... } ..} nesting level, if we are inside such a string.
	int stringLevel = 0;
	if (initStyle == SCE_REBOL_BRACEDSTRING || initStyle == SCE_REBOL_COMMENTBLOCK) {
		stringLevel = styler.GetLineState(currentLine - 1);
	}

	bool blockComment = initStyle == SCE_REBOL_COMMENTBLOCK;
	int dotCount = 0;

	// Do not leak onto next line
	if (initStyle == SCE_REBOL_COMMENTLINE) {
		initStyle = SCE_REBOL_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);
	if (startPos == 0) {
		sc.SetState(SCE_REBOL_PREFACE);
	}
	for (; sc.More(); sc.Forward()) {

		//--- What to do at line end ?
		if (sc.atLineEnd) {
			// Can be either inside a {} string or simply at eol
			if (sc.state != SCE_REBOL_BRACEDSTRING && sc.state != SCE_REBOL_COMMENTBLOCK &&
				sc.state != SCE_REBOL_BINARY && sc.state != SCE_REBOL_PREFACE)
				sc.SetState(SCE_REBOL_DEFAULT);

			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			switch (sc.state) {
			case SCE_REBOL_BRACEDSTRING:
			case SCE_REBOL_COMMENTBLOCK:
				// Inside a braced string, we set the line state
				styler.SetLineState(currentLine, stringLevel);
				break;
			default:
				// Reset the line state
				styler.SetLineState(currentLine, 0);
				break;
			}

			// continue with next char
			continue;
		}

		//--- What to do on white-space ?
		if (IsASpaceOrTab(sc.ch))
		{
			// Return to default if any of these states
			if (sc.state == SCE_REBOL_OPERATOR || sc.state == SCE_REBOL_CHARACTER ||
				sc.state == SCE_REBOL_NUMBER || sc.state == SCE_REBOL_PAIR ||
				sc.state == SCE_REBOL_TUPLE || sc.state == SCE_REBOL_FILE ||
				sc.state == SCE_REBOL_DATE || sc.state == SCE_REBOL_TIME ||
				sc.state == SCE_REBOL_MONEY || sc.state == SCE_REBOL_ISSUE ||
				sc.state == SCE_REBOL_URL || sc.state == SCE_REBOL_EMAIL) {
				sc.SetState(SCE_REBOL_DEFAULT);
			}
		}

		//--- Specialize state ?
		// URL, Email look like identifier
		if (sc.state == SCE_REBOL_IDENTIFIER)
		{
			if (sc.ch == ':' && !IsASpace(sc.chNext)) {
				sc.ChangeState(SCE_REBOL_URL);
			} else if (sc.ch == '@') {
				sc.ChangeState(SCE_REBOL_EMAIL);
			} else if (sc.ch == '$') {
				sc.ChangeState(SCE_REBOL_MONEY);
			}
		}
		// Words look like identifiers
		if (sc.state == SCE_REBOL_IDENTIFIER || (sc.state >= SCE_REBOL_WORD && sc.state <= SCE_REBOL_WORD8)) {
			// Keywords ?
			if (!IsAWordChar(sc.ch) || sc.Match('/')) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				blockComment = strcmp(s, "comment") == 0;
				if (keywords8.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD8);
				} else if (keywords7.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD7);
				} else if (keywords6.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD6);
				} else if (keywords5.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD5);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD4);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD3);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD2);
				} else if (keywords.InList(s)) {
					sc.ChangeState(SCE_REBOL_WORD);
				}
				// Keep same style if there are refinements
				if (!sc.Match('/')) {
					sc.SetState(SCE_REBOL_DEFAULT);
				}
			}
		// special numbers
		} else if (sc.state == SCE_REBOL_NUMBER) {
			switch (sc.ch) {
			case 'x':	sc.ChangeState(SCE_REBOL_PAIR);
						break;
			case ':':	sc.ChangeState(SCE_REBOL_TIME);
						break;
			case '-':
			case '/':	sc.ChangeState(SCE_REBOL_DATE);
						break;
			case '.':	if (++dotCount >= 2) sc.ChangeState(SCE_REBOL_TUPLE);
						break;
			}
		}

		//--- Determine if the current state should terminate
		if (sc.state == SCE_REBOL_QUOTEDSTRING || sc.state == SCE_REBOL_CHARACTER) {
			if (sc.ch == '^' && sc.chNext == '\"') {
				sc.Forward();
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_REBOL_DEFAULT);
			}
		} else if (sc.state == SCE_REBOL_BRACEDSTRING || sc.state == SCE_REBOL_COMMENTBLOCK) {
			if (sc.ch == '}') {
				if (--stringLevel == 0) {
					sc.ForwardSetState(SCE_REBOL_DEFAULT);
				}
			} else if (sc.ch == '{') {
				stringLevel++;
			}
		} else if (sc.state == SCE_REBOL_BINARY) {
			if (sc.ch == '}') {
				sc.ForwardSetState(SCE_REBOL_DEFAULT);
			}
		} else if (sc.state == SCE_REBOL_TAG) {
			if (sc.ch == '>') {
				sc.ForwardSetState(SCE_REBOL_DEFAULT);
			}
		} else if (sc.state == SCE_REBOL_PREFACE) {
			if (sc.MatchIgnoreCase("rebol"))
			{
				int i;
				for (i=5; IsASpaceOrTab(styler.SafeGetCharAt(sc.currentPos+i, 0)); i++);
				if (sc.GetRelative(i) == '[')
					sc.SetState(SCE_REBOL_DEFAULT);
			}
		}

		//--- Parens and bracket changes to default style when the current is a number
		if (sc.state == SCE_REBOL_NUMBER || sc.state == SCE_REBOL_PAIR || sc.state == SCE_REBOL_TUPLE ||
			sc.state == SCE_REBOL_MONEY || sc.state == SCE_REBOL_ISSUE || sc.state == SCE_REBOL_EMAIL ||
			sc.state == SCE_REBOL_URL || sc.state == SCE_REBOL_DATE || sc.state == SCE_REBOL_TIME) {
			if (sc.ch == '(' || sc.ch == '[' || sc.ch == ')' || sc.ch == ']') {
				sc.SetState(SCE_REBOL_DEFAULT);
			}
		}

		//--- Determine if a new state should be entered.
		if (sc.state == SCE_REBOL_DEFAULT) {
			if (IsAnOperator(sc.ch, sc.chNext, sc.GetRelative(2))) {
				sc.SetState(SCE_REBOL_OPERATOR);
			} else if (IsBinaryStart(sc.ch, sc.chNext, sc.GetRelative(2), sc.GetRelative(3))) {
				sc.SetState(SCE_REBOL_BINARY);
			} else if (IsAWordStart(sc.ch, sc.chNext)) {
				sc.SetState(SCE_REBOL_IDENTIFIER);
			} else if (IsADigit(sc.ch) || sc.ch == '+' || sc.ch == '-' || /*Decimal*/ sc.ch == '.' || sc.ch == ',') {
				dotCount = 0;
				sc.SetState(SCE_REBOL_NUMBER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_REBOL_QUOTEDSTRING);
			} else if (sc.ch == '{') {
				sc.SetState(blockComment ? SCE_REBOL_COMMENTBLOCK : SCE_REBOL_BRACEDSTRING);
				++stringLevel;
			} else if (sc.ch == ';') {
				sc.SetState(SCE_REBOL_COMMENTLINE);
			} else if (sc.ch == '$') {
				sc.SetState(SCE_REBOL_MONEY);
			} else if (sc.ch == '%') {
				sc.SetState(SCE_REBOL_FILE);
			} else if (sc.ch == '<') {
				sc.SetState(SCE_REBOL_TAG);
			} else if (sc.ch == '#' && sc.chNext == '"') {
				sc.SetState(SCE_REBOL_CHARACTER);
				sc.Forward();
			} else if (sc.ch == '#' && sc.chNext != '"' && sc.chNext != '{' ) {
				sc.SetState(SCE_REBOL_ISSUE);
			}
		}
	}
	sc.Complete();
}


static void FoldRebolDoc(unsigned int startPos, int length, int /* initStyle */, WordList *[],
                            Accessor &styler) {
	unsigned int lengthDoc = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_REBOL_DEFAULT) {
			if (ch == '[') {
				levelCurrent++;
			} else if (ch == ']') {
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

static const char * const rebolWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmREBOL(SCLEX_REBOL, ColouriseRebolDoc, "rebol", FoldRebolDoc, rebolWordListDesc);

