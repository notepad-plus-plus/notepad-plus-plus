// Scintilla source code edit control
/** @file LexCPP.cxx
 ** Lexer for C++, C, Java, and JavaScript.
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
#include "CharacterSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool IsSpaceEquiv(int state) {
	return (state <= SCE_C_COMMENTDOC) ||
		// including SCE_C_DEFAULT, SCE_C_COMMENT, SCE_C_COMMENTLINE
		(state == SCE_C_COMMENTLINEDOC) || (state == SCE_C_COMMENTDOCKEYWORD) ||
		(state == SCE_C_COMMENTDOCKEYWORDERROR);
}

// Preconditions: sc.currentPos points to a character after '+' or '-'.
// The test for pos reaching 0 should be redundant,
// and is in only for safety measures.
// Limitation: this code will give the incorrect answer for code like
// a = b+++/ptn/...
// Putting a space between the '++' post-inc operator and the '+' binary op
// fixes this, and is highly recommended for readability anyway.
static bool FollowsPostfixOperator(StyleContext &sc, Accessor &styler) {
	int pos = (int) sc.currentPos;
	while (--pos > 0) {
		char ch = styler[pos];
		if (ch == '+' || ch == '-') {
			return styler[pos - 1] == ch;
		}
	}
	return false;
}

static void ColouriseCppDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler, bool caseSensitive) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	// property styling.within.preprocessor
	//	For C++ code, determines whether all preprocessor code is styled in the preprocessor style (0, the default)
	//	or only from the initial # to the end of the command word(1).
	bool stylingWithinPreprocessor = styler.GetPropertyInt("styling.within.preprocessor") != 0;

	CharacterSet setOKBeforeRE(CharacterSet::setNone, "([{=,:;!%^&*|?~+-");
	CharacterSet setCouldBePostOp(CharacterSet::setNone, "+-");

	CharacterSet setDoxygen(CharacterSet::setAlpha, "$@\\&<>#{}[]");

	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);

	// property lexer.cpp.allow.dollars
	//	Set to 0 to disallow the '$' character in identifiers with the cpp lexer.
	if (styler.GetPropertyInt("lexer.cpp.allow.dollars", 1) != 0) {
		setWordStart.Add('$');
		setWord.Add('$');
	}

	int chPrevNonWhite = ' ';
	int visibleChars = 0;
	bool lastWordWasUUID = false;
	int styleBeforeDCKeyword = SCE_C_DEFAULT;
	bool continuationLine = false;
	bool isIncludePreprocessor = false;

	if (initStyle == SCE_C_PREPROCESSOR) {
		// Set continuationLine if last character of previous line is '\'
		int lineCurrent = styler.GetLine(startPos);
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
		int back = startPos;
		while (--back && IsSpaceEquiv(styler.StyleAt(back)))
			;
		if (styler.StyleAt(back) == SCE_C_OPERATOR) {
			chPrevNonWhite = styler.SafeGetCharAt(back);
		}
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			if (sc.state == SCE_C_STRING) {
				// Prevent SCE_C_STRINGEOL from leaking back to previous line which
				// ends with a line continuation by locking in the state upto this position.
				sc.SetState(SCE_C_STRING);
			}
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
			lastWordWasUUID = false;
			isIncludePreprocessor = false;
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
			case SCE_C_OPERATOR:
				sc.SetState(SCE_C_DEFAULT);
				break;
			case SCE_C_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (!setWord.Contains(sc.ch)) {
					sc.SetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_IDENTIFIER:
				if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywords.InList(s)) {
						lastWordWasUUID = strcmp(s, "uuid") == 0;
						sc.ChangeState(SCE_C_WORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_C_WORD2);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_C_GLOBALCLASS);
					}
					sc.SetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_PREPROCESSOR:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT);
				} else if (stylingWithinPreprocessor) {
					if (IsASpace(sc.ch)) {
						sc.SetState(SCE_C_DEFAULT);
					}
				} else {
					if (sc.Match('/', '*') || sc.Match('/', '/')) {
						sc.SetState(SCE_C_DEFAULT);
					}
				}
				break;
			case SCE_C_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_C_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_COMMENTLINEDOC:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTLINEDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_C_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_C_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				} else if (!setDoxygen.Contains(sc.ch)) {
					char s[100];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (!IsASpace(sc.ch) || !keywords3.InList(s + 1)) {
						sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_C_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL);
				} else if (isIncludePreprocessor) {
					if (sc.ch == '>') {
						sc.ForwardSetState(SCE_C_DEFAULT);
						isIncludePreprocessor = false;
					}
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_REGEX:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT);
				} else if (sc.ch == '/') {
					sc.Forward();
					while ((sc.ch < 0x80) && islower(sc.ch))
						sc.Forward();    // gobble regex flags
					sc.SetState(SCE_C_DEFAULT);
				} else if (sc.ch == '\\') {
					// Gobble up the quoted character
					if (sc.chNext == '\\' || sc.chNext == '/') {
						sc.Forward();
					}
				}
				break;
			case SCE_C_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT);
				}
				break;
			case SCE_C_VERBATIM:
				if (sc.ch == '\"') {
					if (sc.chNext == '\"') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_C_DEFAULT);
					}
				}
				break;
			case SCE_C_UUID:
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ')') {
					sc.SetState(SCE_C_DEFAULT);
				}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_C_DEFAULT) {
			if (sc.Match('@', '\"')) {
				sc.SetState(SCE_C_VERBATIM);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_NUMBER);
				}
			} else if (setWordStart.Contains(sc.ch) || (sc.ch == '@')) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_IDENTIFIER);
				}
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {	// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTDOC);
				} else {
					sc.SetState(SCE_C_COMMENT);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTLINEDOC);
				else
					sc.SetState(SCE_C_COMMENTLINE);
			} else if (sc.ch == '/' && setOKBeforeRE.Contains(chPrevNonWhite) &&
				(!setCouldBePostOp.Contains(chPrevNonWhite) || !FollowsPostfixOperator(sc, styler))) {
				sc.SetState(SCE_C_REGEX);	// JavaScript's RegEx
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_C_STRING);
				isIncludePreprocessor = false;	// ensure that '>' won't end the string
			} else if (isIncludePreprocessor && sc.ch == '<') {
				sc.SetState(SCE_C_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_C_CHARACTER);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_C_PREPROCESSOR);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_C_DEFAULT);
				} else if (sc.Match("include")) {
					isIncludePreprocessor = true;
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_C_OPERATOR);
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
	return style == SCE_C_COMMENT ||
		style == SCE_C_COMMENTDOC ||
		style == SCE_C_COMMENTDOCKEYWORD ||
		style == SCE_C_COMMENTDOCKEYWORDERROR;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldCppDoc(unsigned int startPos, int length, int initStyle,
					   WordList *[], Accessor &styler) {

	// property fold.comment
	//	This option enables folding multi-line comments and explicit fold points when using the C++ lexer.
	//	Explicit fold points allows adding extra folding by placing a //{ comment at the start and a //}
	//	at the end of a section that should fold.
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;

	// property fold.preprocessor
	//	This option enables folding preprocessor directives when using the C++ lexer.
	//	Includes C#'s explicit #region and #endregion folding directives.
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;

	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	// property fold.at.else
	//	This option enables C++ folding on a "} else {" line of an if statement.
	bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;

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
			if (!IsStreamCommentStyle(stylePrev) && (stylePrev != SCE_C_COMMENTLINEDOC)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && (styleNext != SCE_C_COMMENTLINEDOC) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (foldComment && (style == SCE_C_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelNext++;
				} else if (chNext2 == '}') {
					levelNext--;
				}
			}
		}
		if (foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
			if (ch == '#') {
				unsigned int j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (styler.Match(j, "region") || styler.Match(j, "if")) {
					levelNext++;
				} else if (styler.Match(j, "end")) {
					levelNext--;
				}
			}
		}
		if (style == SCE_C_OPERATOR) {
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
		if (!IsASpace(ch))
			visibleChars++;
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
			if (atEOL && (i == static_cast<unsigned int>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
	}
}

static const char *const cppWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Unused",
            "Global classes and typedefs",
            0,
};

static void ColouriseCppDocSensitive(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                                     Accessor &styler) {
	ColouriseCppDoc(startPos, length, initStyle, keywordlists, styler, true);
}

static void ColouriseCppDocInsensitive(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                                       Accessor &styler) {
	ColouriseCppDoc(startPos, length, initStyle, keywordlists, styler, false);
}

LexerModule lmCPP(SCLEX_CPP, ColouriseCppDocSensitive, "cpp", FoldCppDoc, cppWordLists);
LexerModule lmCPPNoCase(SCLEX_CPPNOCASE, ColouriseCppDocInsensitive, "cppnocase", FoldCppDoc, cppWordLists);
