/** @file LexD.cxx
 ** Lexer for D.
 **
 ** Copyright (c) 2006 by Waldemar Augustyn <waldemar@wdmsys.com>
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
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

/* Nested comments require keeping the value of the nesting level for every
   position in the document.  But since scintilla always styles line by line,
   we only need to store one value per line. The non-negative number indicates
   nesting level at the end of the line.
*/

// Underscore, letter, digit and universal alphas from C99 Appendix D.

static bool IsWordStart(int ch) {
	return (isascii(ch) && (isalpha(ch) || ch == '_')) || !isascii(ch);
}

static bool IsWord(int ch) {
	return (isascii(ch) && (isalnum(ch) || ch == '_')) || !isascii(ch);
}

static bool IsDoxygen(int ch) {
	if (isascii(ch) && islower(ch))
		return true;
	if (ch == '$' || ch == '@' || ch == '\\' ||
		ch == '&' || ch == '#' || ch == '<' || ch == '>' ||
		ch == '{' || ch == '}' || ch == '[' || ch == ']')
		return true;
	return false;
}

static bool IsStringSuffix(int ch) {
	return ch == 'c' || ch == 'w' || ch == 'd';
}


static void ColouriseDoc(unsigned int startPos, int length, int initStyle,
	WordList *keywordlists[], Accessor &styler, bool caseSensitive) {

	WordList &keywords  = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2]; //doxygen
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];

	int styleBeforeDCKeyword = SCE_D_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	int curLine = styler.GetLine(startPos);
	int curNcLevel = curLine > 0? styler.GetLineState(curLine-1): 0;
	bool numFloat = false; // Float literals have '+' and '-' signs
	bool numHex = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curNcLevel);
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_D_OPERATOR:
				sc.SetState(SCE_D_DEFAULT);
				break;
			case SCE_D_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (isascii(sc.ch) && (isalnum(sc.ch) || sc.ch == '_')) {
					continue;
				} else if (sc.ch == '.' && sc.chNext != '.' && !numFloat) {
					// Don't parse 0..2 as number.
					numFloat=true;
					continue;
				} else if ( ( sc.ch == '-' || sc.ch == '+' ) && (		/*sign and*/
					( !numHex && ( sc.chPrev == 'e' || sc.chPrev == 'E' ) ) || /*decimal or*/
					( sc.chPrev == 'p' || sc.chPrev == 'P' ) ) ) {		/*hex*/
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				} else {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_IDENTIFIER:
				if (!IsWord(sc.ch)) {
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_D_WORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_D_WORD2);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_D_TYPEDEF);
					} else if (keywords5.InList(s)) {
						sc.ChangeState(SCE_D_WORD5);
					} else if (keywords6.InList(s)) {
						sc.ChangeState(SCE_D_WORD6);
					} else if (keywords7.InList(s)) {
						sc.ChangeState(SCE_D_WORD7);
					}
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_D_COMMENTDOC;
						sc.SetState(SCE_D_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_D_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENTLINEDOC:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_D_COMMENTLINEDOC;
						sc.SetState(SCE_D_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_D_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_D_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_D_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				} else if (!IsDoxygen(sc.ch)) {
					char s[100];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (!IsASpace(sc.ch) || !keywords3.InList(s + 1)) {
						sc.ChangeState(SCE_D_COMMENTDOCKEYWORDERROR);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_D_COMMENTNESTED:
				if (sc.Match('+', '/')) {
					if (curNcLevel > 0)
						curNcLevel -= 1;
					curLine = styler.GetLine(sc.currentPos);
					styler.SetLineState(curLine, curNcLevel);
					sc.Forward();
					if (curNcLevel == 0) {
						sc.ForwardSetState(SCE_D_DEFAULT);
					}
				} else if (sc.Match('/','+')) {
					curNcLevel += 1;
					curLine = styler.GetLine(sc.currentPos);
					styler.SetLineState(curLine, curNcLevel);
					sc.Forward();
				}
				break;
			case SCE_D_STRING:
				if (sc.ch == '\\') {
					if (sc.chNext == '"' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '"') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_D_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					// Char has no suffixes
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGB:
				if (sc.ch == '`') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGR:
				if (sc.ch == '"') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_D_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_D_NUMBER);
				numFloat = sc.ch == '.';
				// Remember hex literal
				numHex = sc.ch == '0' && ( sc.chNext == 'x' || sc.chNext == 'X' );
			} else if ( (sc.ch == 'r' || sc.ch == 'x' || sc.ch == 'q')
				&& sc.chNext == '"' ) {
				// Limited support for hex and delimited strings: parse as r""
				sc.SetState(SCE_D_STRINGR);
				sc.Forward();
			} else if (IsWordStart(sc.ch) || sc.ch == '$') {
				sc.SetState(SCE_D_IDENTIFIER);
			} else if (sc.Match('/','+')) {
				curNcLevel += 1;
				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, curNcLevel);
				sc.SetState(SCE_D_COMMENTNESTED);
				sc.Forward();
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {   // Support of Qt/Doxygen doc. style
					sc.SetState(SCE_D_COMMENTDOC);
				} else {
					sc.SetState(SCE_D_COMMENT);
				}
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_D_COMMENTLINEDOC);
				else
					sc.SetState(SCE_D_COMMENTLINE);
			} else if (sc.ch == '"') {
				sc.SetState(SCE_D_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_D_CHARACTER);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_D_STRINGB);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_D_OPERATOR);
				if (sc.ch == '.' && sc.chNext == '.') sc.Forward(); // Range operator
			}
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_D_COMMENT ||
		style == SCE_D_COMMENTDOC ||
		style == SCE_D_COMMENTDOCKEYWORD ||
		style == SCE_D_COMMENTDOCKEYWORDERROR;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldDoc(unsigned int startPos, int length, int initStyle, Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	// property lexer.d.fold.at.else
	//  This option enables D folding on a "} else {" line of an if statement.
	bool foldAtElse = styler.GetPropertyInt("lexer.d.fold.at.else",
		styler.GetPropertyInt("fold.at.else", 0)) != 0;
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
	int style = initStyle;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (style == SCE_D_OPERATOR) {
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
			if (foldComment) {  // Handle nested comments
				int nc;
				nc =  styler.GetLineState(lineCurrent);
				nc -= lineCurrent>0? styler.GetLineState(lineCurrent-1): 0;
				levelNext += nc;
			}
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
		if (!IsASpace(ch))
			visibleChars++;
	}
}

static void FoldDDoc(unsigned int startPos, int length, int initStyle,
	WordList *[], Accessor &styler) {
		FoldDoc(startPos, length, initStyle, styler);
}

static const char * const dWordLists[] = {
			"Primary keywords and identifiers",
			"Secondary keywords and identifiers",
			"Documentation comment keywords",
			"Type definitions and aliases",
			"Keywords 5",
			"Keywords 6",
			"Keywords 7",
			0,
		};

static void ColouriseDDoc(unsigned int startPos, int length,
	int initStyle, WordList *keywordlists[], Accessor &styler) {
		ColouriseDoc(startPos, length, initStyle, keywordlists, styler, true);
}

LexerModule lmD(SCLEX_D, ColouriseDDoc, "d", FoldDDoc, dWordLists);
