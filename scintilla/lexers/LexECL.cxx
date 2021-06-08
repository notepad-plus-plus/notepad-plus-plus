// Scintilla source code edit control
/** @file LexECL.cxx
 ** Lexer for ECL.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif
#ifdef __BORLANDC__
// Borland C++ displays warnings in vector header without this
#pragma option -w-ccc -w-rch
#endif

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"

#define SET_LOWER "abcdefghijklmnopqrstuvwxyz"
#define SET_UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define SET_DIGITS "0123456789"

using namespace Scintilla;

static bool IsSpaceEquiv(int state) {
	switch (state) {
	case SCE_ECL_DEFAULT:
	case SCE_ECL_COMMENT:
	case SCE_ECL_COMMENTLINE:
	case SCE_ECL_COMMENTLINEDOC:
	case SCE_ECL_COMMENTDOCKEYWORD:
	case SCE_ECL_COMMENTDOCKEYWORDERROR:
	case SCE_ECL_COMMENTDOC:
		return true;

	default:
		return false;
	}
}

static void ColouriseEclDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {
	WordList &keywords0 = *keywordlists[0];
	WordList &keywords1 = *keywordlists[1];
	WordList &keywords2 = *keywordlists[2];
	WordList &keywords3 = *keywordlists[3]; //Value Types
	WordList &keywords4 = *keywordlists[4];
	WordList &keywords5 = *keywordlists[5];
	WordList &keywords6 = *keywordlists[6];	//Javadoc Tags
	WordList cplusplus;
	cplusplus.Set("beginc endc");

	bool stylingWithinPreprocessor = false;

	CharacterSet setOKBeforeRE(CharacterSet::setNone, "(=,");
	CharacterSet setDoxygen(CharacterSet::setLower, "$@\\&<>#{}[]");
	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);
	CharacterSet setQualified(CharacterSet::setNone, "uUxX");

	int chPrevNonWhite = ' ';
	int visibleChars = 0;
	bool lastWordWasUUID = false;
	int styleBeforeDCKeyword = SCE_ECL_DEFAULT;
	bool continuationLine = false;

	if (initStyle == SCE_ECL_PREPROCESSOR) {
		// Set continuationLine if last character of previous line is '\'
		Sci_Position lineCurrent = styler.GetLine(startPos);
		if (lineCurrent > 0) {
			int chBack = styler.SafeGetCharAt(startPos-1, 0);
			int chBack2 = styler.SafeGetCharAt(startPos-2, 0);
			int lineEndChar = '!';
			if (chBack2 == '\r' && chBack == '\n') {
				lineEndChar = styler.SafeGetCharAt(startPos-3, 0);
			} else if (chBack == '\n' || chBack == '\r') {
				lineEndChar = chBack2;
			}
			continuationLine = lineEndChar == '\\';
		}
	}

	// look back to set chPrevNonWhite properly for better regex colouring
	if (startPos > 0) {
		Sci_Position back = startPos;
		while (--back && IsSpaceEquiv(styler.StyleAt(back)))
			;
		if (styler.StyleAt(back) == SCE_ECL_OPERATOR) {
			chPrevNonWhite = styler.SafeGetCharAt(back);
		}
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineStart) {
			if (sc.state == SCE_ECL_STRING) {
				// Prevent SCE_ECL_STRINGEOL from leaking back to previous line which
				// ends with a line continuation by locking in the state upto this position.
				sc.SetState(SCE_ECL_STRING);
			}
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
			lastWordWasUUID = false;
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continuationLine = true;
				continue;
			}
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_ECL_ADDED:
			case SCE_ECL_DELETED:
			case SCE_ECL_CHANGED:
			case SCE_ECL_MOVED:
			if (sc.atLineStart)
					sc.SetState(SCE_ECL_DEFAULT);
				break;
			case SCE_ECL_OPERATOR:
				sc.SetState(SCE_ECL_DEFAULT);
				break;
			case SCE_ECL_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (!setWord.Contains(sc.ch)) {
					sc.SetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_IDENTIFIER:
				if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
					char s[1000];
					sc.GetCurrentLowered(s, sizeof(s));
					if (keywords0.InList(s)) {
						lastWordWasUUID = strcmp(s, "uuid") == 0;
						sc.ChangeState(SCE_ECL_WORD0);
					} else if (keywords1.InList(s)) {
						sc.ChangeState(SCE_ECL_WORD1);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_ECL_WORD2);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_ECL_WORD4);
					} else if (keywords5.InList(s)) {
						sc.ChangeState(SCE_ECL_WORD5);
					}
					else	//Data types are of from KEYWORD##
					{
						int i = static_cast<int>(strlen(s)) - 1;
						while(i >= 0 && (isdigit(s[i]) || s[i] == '_'))
							--i;

						char s2[1000];
						strncpy(s2, s, i + 1);
						s2[i + 1] = 0;
						if (keywords3.InList(s2)) {
							sc.ChangeState(SCE_ECL_WORD3);
						}
					}
					sc.SetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_PREPROCESSOR:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_ECL_DEFAULT);
				} else if (stylingWithinPreprocessor) {
					if (IsASpace(sc.ch)) {
						sc.SetState(SCE_ECL_DEFAULT);
					}
				} else {
					if (sc.Match('/', '*') || sc.Match('/', '/')) {
						sc.SetState(SCE_ECL_DEFAULT);
					}
				}
				break;
			case SCE_ECL_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_ECL_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_ECL_COMMENTDOC;
						sc.SetState(SCE_ECL_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_ECL_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_COMMENTLINEDOC:
				if (sc.atLineStart) {
					sc.SetState(SCE_ECL_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_ECL_COMMENTLINEDOC;
						sc.SetState(SCE_ECL_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_ECL_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_ECL_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_ECL_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_ECL_DEFAULT);
				} else if (!setDoxygen.Contains(sc.ch)) {
					char s[1000];
					sc.GetCurrentLowered(s, sizeof(s));
					if (!IsASpace(sc.ch) || !keywords6.InList(s+1)) {
						sc.ChangeState(SCE_ECL_COMMENTDOCKEYWORDERROR);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_ECL_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_ECL_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_ECL_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_REGEX:
				if (sc.atLineStart) {
					sc.SetState(SCE_ECL_DEFAULT);
				} else if (sc.ch == '/') {
					sc.Forward();
					while ((sc.ch < 0x80) && islower(sc.ch))
						sc.Forward();    // gobble regex flags
					sc.SetState(SCE_ECL_DEFAULT);
				} else if (sc.ch == '\\') {
					// Gobble up the quoted character
					if (sc.chNext == '\\' || sc.chNext == '/') {
						sc.Forward();
					}
				}
				break;
			case SCE_ECL_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_ECL_DEFAULT);
				}
				break;
			case SCE_ECL_VERBATIM:
				if (sc.ch == '\"') {
					if (sc.chNext == '\"') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_ECL_DEFAULT);
					}
				}
				break;
			case SCE_ECL_UUID:
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ')') {
					sc.SetState(SCE_ECL_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		Sci_Position lineCurrent = styler.GetLine(sc.currentPos);
		int lineState = styler.GetLineState(lineCurrent);
		if (sc.state == SCE_ECL_DEFAULT) {
			if (lineState) {
				sc.SetState(lineState);
			}
			else if (sc.Match('@', '\"')) {
				sc.SetState(SCE_ECL_VERBATIM);
				sc.Forward();
			} else if (setQualified.Contains(sc.ch) && sc.chNext == '\'') {
				sc.SetState(SCE_ECL_CHARACTER);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_ECL_UUID);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_ECL_NUMBER);
				}
			} else if (setWordStart.Contains(sc.ch) || (sc.ch == '@')) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_ECL_UUID);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_ECL_IDENTIFIER);
				}
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {	// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_ECL_COMMENTDOC);
				} else {
					sc.SetState(SCE_ECL_COMMENT);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_ECL_COMMENTLINEDOC);
				else
					sc.SetState(SCE_ECL_COMMENTLINE);
			} else if (sc.ch == '/' && setOKBeforeRE.Contains(chPrevNonWhite)) {
				sc.SetState(SCE_ECL_REGEX);	// JavaScript's RegEx
//			} else if (sc.ch == '\"') {
//				sc.SetState(SCE_ECL_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_ECL_CHARACTER);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_ECL_PREPROCESSOR);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_ECL_DEFAULT);
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_ECL_OPERATOR);
			}
		}

		if (!IsASpace(sc.ch) && !IsSpaceEquiv(sc.state)) {
			chPrevNonWhite = sc.ch;
			visibleChars++;
		}
		continuationLine = false;
	}
	sc.Complete();

}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_ECL_COMMENT ||
		style == SCE_ECL_COMMENTDOC ||
		style == SCE_ECL_COMMENTDOCKEYWORD ||
		style == SCE_ECL_COMMENTDOCKEYWORDERROR;
}

static bool MatchNoCase(Accessor & styler, Sci_PositionU & pos, const char *s) {
	Sci_Position i=0;
	for (; *s; i++) {
		char compare_char = tolower(*s);
		char styler_char = tolower(styler.SafeGetCharAt(pos+i));
		if (compare_char != styler_char)
			return false;
		s++;
	}
	pos+=i-1;
	return true;
}


// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldEclDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
					   WordList *[], Accessor &styler) {
	bool foldComment = true;
	bool foldPreprocessor = true;
	bool foldCompact = true;
	bool foldAtElse = true;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev) && (stylePrev != SCE_ECL_COMMENTLINEDOC)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && (styleNext != SCE_ECL_COMMENTLINEDOC) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (foldComment && (style == SCE_ECL_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelNext++;
				} else if (chNext2 == '}') {
					levelNext--;
				}
			}
		}
		if (foldPreprocessor && (style == SCE_ECL_PREPROCESSOR)) {
			if (ch == '#') {
				Sci_PositionU j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (MatchNoCase(styler, j, "region") || MatchNoCase(styler, j, "if")) {
					levelNext++;
				} else if (MatchNoCase(styler, j, "endregion") || MatchNoCase(styler, j, "end")) {
					levelNext--;
				}
			}
		}
		if (style == SCE_ECL_OPERATOR) {
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
		if (style == SCE_ECL_WORD2) {
			if (MatchNoCase(styler, i, "record") || MatchNoCase(styler, i, "transform") || MatchNoCase(styler, i, "type") || MatchNoCase(styler, i, "function") ||
				MatchNoCase(styler, i, "module") || MatchNoCase(styler, i, "service") || MatchNoCase(styler, i, "interface") || MatchNoCase(styler, i, "ifblock") ||
				MatchNoCase(styler, i, "macro") || MatchNoCase(styler, i, "beginc++")) {
				levelNext++;
			} else if (MatchNoCase(styler, i, "endmacro") || MatchNoCase(styler, i, "endc++") || MatchNoCase(styler, i, "end")) {
				levelNext--;
			}
		}
		if (atEOL || (i == endPos-1)) {
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
			if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}
}

static const char * const EclWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmECL(
   SCLEX_ECL,
   ColouriseEclDoc,
   "ecl",
   FoldEclDoc,
   EclWordListDesc);
