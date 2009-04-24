// Scintilla source code edit control
/** @file LexPOV.cxx
 ** Lexer for POV-Ray SDL (Persistance of Vision Raytracer, Scene Description Language).
 ** Written by Philippe Lhoste but this is mostly a derivative of LexCPP...
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Some points that distinguish from a simple C lexer:
// Identifiers start only by a character.
// No line continuation character.
// Strings are limited to 256 characters.
// Directives are similar to preprocessor commands,
// but we match directive keywords and colorize incorrect ones.
// Block comments can be nested (code stolen from my code in LexLua).

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

static inline bool IsAWordChar(int ch) {
	return ch < 0x80 && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return ch < 0x80 && isalpha(ch);
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
             ch == '.' || ch == '-' || ch == '+');
}

static void ColourisePovDoc(
	unsigned int startPos,
	int length,
	int initStyle,
	WordList *keywordlists[],
    Accessor &styler) {

	WordList &keywords1 = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
	WordList &keywords8 = *keywordlists[7];

	int currentLine = styler.GetLine(startPos);
	// Initialize the block comment /* */ nesting level, if we are inside such a comment.
	int blockCommentLevel = 0;
	if (initStyle == SCE_POV_COMMENT) {
		blockCommentLevel = styler.GetLineState(currentLine - 1);
	}

	// Do not leak onto next line
	if (initStyle == SCE_POV_STRINGEOL || initStyle == SCE_POV_COMMENTLINE) {
		initStyle = SCE_POV_DEFAULT;
	}

	short stringLen = 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			if (sc.state == SCE_POV_COMMENT) {
				// Inside a block comment, we set the line state
				styler.SetLineState(currentLine, blockCommentLevel);
			} else {
				// Reset the line state
				styler.SetLineState(currentLine, 0);
			}
		}

		if (sc.atLineStart && (sc.state == SCE_POV_STRING)) {
			// Prevent SCE_POV_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_POV_STRING);
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_POV_OPERATOR) {
			sc.SetState(SCE_POV_DEFAULT);
		} else if (sc.state == SCE_POV_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_POV_DEFAULT);
			}
		} else if (sc.state == SCE_POV_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (keywords2.InList(s)) {
					sc.ChangeState(SCE_POV_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_POV_WORD3);
				} else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_POV_WORD4);
				} else if (keywords5.InList(s)) {
					sc.ChangeState(SCE_POV_WORD5);
				} else if (keywords6.InList(s)) {
					sc.ChangeState(SCE_POV_WORD6);
				} else if (keywords7.InList(s)) {
					sc.ChangeState(SCE_POV_WORD7);
				} else if (keywords8.InList(s)) {
					sc.ChangeState(SCE_POV_WORD8);
				}
				sc.SetState(SCE_POV_DEFAULT);
			}
		} else if (sc.state == SCE_POV_DIRECTIVE) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				char *p;
				sc.GetCurrent(s, sizeof(s));
				p = s;
				// Skip # and whitespace between # and directive word
				do {
					p++;
				} while ((*p == ' ' || *p == '\t') && *p != '\0');
				if (!keywords1.InList(p)) {
					sc.ChangeState(SCE_POV_BADDIRECTIVE);
				}
				sc.SetState(SCE_POV_DEFAULT);
			}
		} else if (sc.state == SCE_POV_COMMENT) {
			if (sc.Match('/', '*')) {
				blockCommentLevel++;
				sc.Forward();
			} else if (sc.Match('*', '/') && blockCommentLevel > 0) {
				blockCommentLevel--;
				sc.Forward();
				if (blockCommentLevel == 0) {
					sc.ForwardSetState(SCE_POV_DEFAULT);
				}
			}
		} else if (sc.state == SCE_POV_COMMENTLINE) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_POV_DEFAULT);
			}
		} else if (sc.state == SCE_POV_STRING) {
			if (sc.ch == '\\') {
				stringLen++;
				if (strchr("abfnrtuv0'\"", sc.chNext)) {
					// Compound characters are counted as one.
					// Note: for Unicode chars \u, we shouldn't count the next 4 digits...
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_POV_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_POV_STRINGEOL);
				sc.ForwardSetState(SCE_POV_DEFAULT);
			} else {
				stringLen++;
			}
			if (stringLen > 256) {
				// Strings are limited to 256 chars
				sc.SetState(SCE_POV_STRINGEOL);
			}
		} else if (sc.state == SCE_POV_STRINGEOL) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_C_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_POV_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_POV_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_POV_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_POV_IDENTIFIER);
			} else if (sc.Match('/', '*')) {
				blockCommentLevel = 1;
				sc.SetState(SCE_POV_COMMENT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_POV_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_POV_STRING);
				stringLen = 0;
			} else if (sc.ch == '#') {
				sc.SetState(SCE_POV_DIRECTIVE);
				// Skip whitespace between # and directive word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_POV_DEFAULT);
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POV_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static void FoldPovDoc(
	unsigned int startPos,
	int length,
	int initStyle,
	WordList *[],
	Accessor &styler) {

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldDirective = styler.GetPropertyInt("fold.directive") != 0;
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
		if (foldComment && (style == SCE_POV_COMMENT)) {
			if (stylePrev != SCE_POV_COMMENT) {
				levelCurrent++;
			} else if ((styleNext != SCE_POV_COMMENT) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (foldComment && (style == SCE_POV_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelCurrent++;
				} else if (chNext2 == '}') {
					levelCurrent--;
				}
			}
		}
		if (foldDirective && (style == SCE_POV_DIRECTIVE)) {
			if (ch == '#') {
				unsigned int j=i+1;
				while ((j<endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
			}
		}
		if (style == SCE_POV_OPERATOR) {
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

static const char * const povWordLists[] = {
	"Language directives",
	"Objects & CSG & Appearance",
	"Types & Modifiers & Items",
	"Predefined Identifiers",
	"Predefined Functions",
	"User defined 1",
	"User defined 2",
	"User defined 3",
	0,
};

LexerModule lmPOV(SCLEX_POV, ColourisePovDoc, "pov", FoldPovDoc, povWordLists);
