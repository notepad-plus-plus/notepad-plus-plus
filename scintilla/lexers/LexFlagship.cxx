// Scintilla source code edit control
/** @file LexFlagShip.cxx
 ** Lexer for Harbour and FlagShip.
 ** (Syntactically compatible to other xBase dialects, like Clipper, dBase, Clip, FoxPro etc.)
 **/
// Copyright 2005 by Randy Butler
// Copyright 2010 by Xavi <jarabal/at/gmail.com> (Harbour)
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
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

// Extended to accept accented characters
static inline bool IsAWordChar(int ch)
{
	return ch >= 0x80 ||
				(isalnum(ch) || ch == '_');
}

static void ColouriseFlagShipDoc(unsigned int startPos, int length, int initStyle,
                                 WordList *keywordlists[], Accessor &styler)
{

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];

	// property lexer.flagship.styling.within.preprocessor
	//	For Harbour code, determines whether all preprocessor code is styled in the preprocessor style (0) or only from the
	//	initial # to the end of the command word(1, the default). It also determines how to present text, dump, and disabled code.
	bool stylingWithinPreprocessor = styler.GetPropertyInt("lexer.flagship.styling.within.preprocessor", 1) != 0;

	CharacterSet setDoxygen(CharacterSet::setAlpha, "$@\\&<>#{}[]");

	int visibleChars = 0;
	int closeStringChar = 0;
	int styleBeforeDCKeyword = SCE_FS_DEFAULT;
	bool bEnableCode = initStyle < SCE_FS_DISABLEDCODE;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_FS_OPERATOR:
			case SCE_FS_OPERATOR_C:
			case SCE_FS_WORDOPERATOR:
				sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				break;
			case SCE_FS_IDENTIFIER:
			case SCE_FS_IDENTIFIER_C:
				if (!IsAWordChar(sc.ch)) {
					char s[64];
					sc.GetCurrentLowered(s, sizeof(s));
					if (keywords.InList(s)) {
						sc.ChangeState(bEnableCode ? SCE_FS_KEYWORD : SCE_FS_KEYWORD_C);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(bEnableCode ? SCE_FS_KEYWORD2 : SCE_FS_KEYWORD2_C);
					} else if (bEnableCode && keywords3.InList(s)) {
						sc.ChangeState(SCE_FS_KEYWORD3);
					} else if (bEnableCode && keywords4.InList(s)) {
						sc.ChangeState(SCE_FS_KEYWORD4);
					}// Else, it is really an identifier...
					sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				}
				break;
			case SCE_FS_NUMBER:
				if (!IsAWordChar(sc.ch) && !(sc.ch == '.' && IsADigit(sc.chNext))) {
					sc.SetState(SCE_FS_DEFAULT);
				}
				break;
			case SCE_FS_NUMBER_C:
				if (!IsAWordChar(sc.ch) && sc.ch != '.') {
					sc.SetState(SCE_FS_DEFAULT_C);
				}
				break;
			case SCE_FS_CONSTANT:
				if (!IsAWordChar(sc.ch)) {
					sc.SetState(SCE_FS_DEFAULT);
				}
				break;
			case SCE_FS_STRING:
			case SCE_FS_STRING_C:
				if (sc.ch == closeStringChar) {
					sc.ForwardSetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				} else if (sc.atLineEnd) {
					sc.ChangeState(bEnableCode ? SCE_FS_STRINGEOL : SCE_FS_STRINGEOL_C);
				}
				break;
			case SCE_FS_STRINGEOL:
			case SCE_FS_STRINGEOL_C:
				if (sc.atLineStart) {
					sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				}
				break;
			case SCE_FS_COMMENTDOC:
			case SCE_FS_COMMENTDOC_C:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = bEnableCode ? SCE_FS_COMMENTDOC : SCE_FS_COMMENTDOC_C;
						sc.SetState(SCE_FS_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_FS_COMMENT:
			case SCE_FS_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_FS_DEFAULT);
				}
				break;
			case SCE_FS_COMMENTLINEDOC:
			case SCE_FS_COMMENTLINEDOC_C:
				if (sc.atLineStart) {
					sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = bEnableCode ? SCE_FS_COMMENTLINEDOC : SCE_FS_COMMENTLINEDOC_C;
						sc.SetState(SCE_FS_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_FS_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_FS_COMMENTDOC || styleBeforeDCKeyword == SCE_FS_COMMENTDOC_C) &&
						sc.Match('*', '/')) {
					sc.ChangeState(SCE_FS_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				} else if (!setDoxygen.Contains(sc.ch)) {
					char s[64];
					sc.GetCurrentLowered(s, sizeof(s));
					if (!IsASpace(sc.ch) || !keywords5.InList(s + 1)) {
						sc.ChangeState(SCE_FS_COMMENTDOCKEYWORDERROR);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_FS_PREPROCESSOR:
			case SCE_FS_PREPROCESSOR_C:
				if (sc.atLineEnd) {
					if (!(sc.chPrev == ';' || sc.GetRelative(-2) == ';')) {
						sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
					}
				} else if (stylingWithinPreprocessor) {
					if (IsASpaceOrTab(sc.ch)) {
						sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
					}
				} else if (sc.Match('/', '*') || sc.Match('/', '/') || sc.Match('&', '&')) {
					sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				}
				break;
			case SCE_FS_DISABLEDCODE:
				if (sc.ch == '#' && visibleChars == 0) {
					sc.SetState(bEnableCode ? SCE_FS_PREPROCESSOR : SCE_FS_PREPROCESSOR_C);
					do {	// Skip whitespace between # and preprocessor word
						sc.Forward();
					} while (IsASpaceOrTab(sc.ch) && sc.More());
					if (sc.MatchIgnoreCase("pragma")) {
						sc.Forward(6);
						do {	// Skip more whitespace until keyword
							sc.Forward();
						} while (IsASpaceOrTab(sc.ch) && sc.More());
						if (sc.MatchIgnoreCase("enddump") || sc.MatchIgnoreCase("__endtext")) {
							bEnableCode = true;
							sc.SetState(SCE_FS_DISABLEDCODE);
							sc.Forward(sc.ch == '_' ? 8 : 6);
							sc.ForwardSetState(SCE_FS_DEFAULT);
						} else {
							sc.ChangeState(SCE_FS_DISABLEDCODE);
						}
					} else {
						sc.ChangeState(SCE_FS_DISABLEDCODE);
					}
				}
				break;
			case SCE_FS_DATE:
				if (sc.ch == '}') {
					sc.ForwardSetState(SCE_FS_DEFAULT);
				} else if (sc.atLineEnd) {
					sc.ChangeState(SCE_FS_STRINGEOL);
				}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_FS_DEFAULT || sc.state == SCE_FS_DEFAULT_C) {
			if (bEnableCode &&
					(sc.MatchIgnoreCase(".and.") || sc.MatchIgnoreCase(".not."))) {
				sc.SetState(SCE_FS_WORDOPERATOR);
				sc.Forward(4);
			} else if (bEnableCode && sc.MatchIgnoreCase(".or.")) {
				sc.SetState(SCE_FS_WORDOPERATOR);
				sc.Forward(3);
			} else if (bEnableCode &&
					(sc.MatchIgnoreCase(".t.") || sc.MatchIgnoreCase(".f.") ||
					(!IsAWordChar(sc.GetRelative(3)) && sc.MatchIgnoreCase("nil")))) {
				sc.SetState(SCE_FS_CONSTANT);
				sc.Forward(2);
			} else if (sc.Match('/', '*')) {
				sc.SetState(bEnableCode ? SCE_FS_COMMENTDOC : SCE_FS_COMMENTDOC_C);
				sc.Forward();
			} else if (bEnableCode && sc.Match('&', '&')) {
				sc.SetState(SCE_FS_COMMENTLINE);
				sc.Forward();
			} else if (sc.Match('/', '/')) {
				sc.SetState(bEnableCode ? SCE_FS_COMMENTLINEDOC : SCE_FS_COMMENTLINEDOC_C);
				sc.Forward();
			} else if (bEnableCode && sc.ch == '*' && visibleChars == 0) {
				sc.SetState(SCE_FS_COMMENT);
			} else if (sc.ch == '\"' || sc.ch == '\'') {
				sc.SetState(bEnableCode ? SCE_FS_STRING : SCE_FS_STRING_C);
				closeStringChar = sc.ch;
			} else if (closeStringChar == '>' && sc.ch == '<') {
				sc.SetState(bEnableCode ? SCE_FS_STRING : SCE_FS_STRING_C);
			} else if (sc.ch == '#' && visibleChars == 0) {
				sc.SetState(bEnableCode ? SCE_FS_PREPROCESSOR : SCE_FS_PREPROCESSOR_C);
				do {	// Skip whitespace between # and preprocessor word
					sc.Forward();
				} while (IsASpaceOrTab(sc.ch) && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(bEnableCode ? SCE_FS_DEFAULT : SCE_FS_DEFAULT_C);
				} else if (sc.MatchIgnoreCase("include")) {
					if (stylingWithinPreprocessor) {
						closeStringChar = '>';
					}
				} else if (sc.MatchIgnoreCase("pragma")) {
					sc.Forward(6);
					do {	// Skip more whitespace until keyword
						sc.Forward();
					} while (IsASpaceOrTab(sc.ch) && sc.More());
					if (sc.MatchIgnoreCase("begindump") || sc.MatchIgnoreCase("__cstream")) {
						bEnableCode = false;
						if (stylingWithinPreprocessor) {
							sc.SetState(SCE_FS_DISABLEDCODE);
							sc.Forward(8);
							sc.ForwardSetState(SCE_FS_DEFAULT_C);
						} else {
							sc.SetState(SCE_FS_DISABLEDCODE);
						}
					} else if (sc.MatchIgnoreCase("enddump") || sc.MatchIgnoreCase("__endtext")) {
						bEnableCode = true;
						sc.SetState(SCE_FS_DISABLEDCODE);
						sc.Forward(sc.ch == '_' ? 8 : 6);
						sc.ForwardSetState(SCE_FS_DEFAULT);
					}
				}
			} else if (bEnableCode && sc.ch == '{') {
				int p = 0;
				int chSeek;
				unsigned int endPos(startPos + length);
				do {	// Skip whitespace
					chSeek = sc.GetRelative(++p);
				} while (IsASpaceOrTab(chSeek) && (sc.currentPos + p < endPos));
				if (chSeek == '^') {
					sc.SetState(SCE_FS_DATE);
				} else {
					sc.SetState(SCE_FS_OPERATOR);
				}
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(bEnableCode ? SCE_FS_NUMBER : SCE_FS_NUMBER_C);
			} else if (IsAWordChar(sc.ch)) {
				sc.SetState(bEnableCode ? SCE_FS_IDENTIFIER : SCE_FS_IDENTIFIER_C);
			} else if (isoperator(static_cast<char>(sc.ch)) || (bEnableCode && sc.ch == '@')) {
				sc.SetState(bEnableCode ? SCE_FS_OPERATOR : SCE_FS_OPERATOR_C);
			}
		}

		if (sc.atLineEnd) {
			visibleChars = 0;
			closeStringChar = 0;
		}
		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}
	sc.Complete();
}

static void FoldFlagShipDoc(unsigned int startPos, int length, int,
									WordList *[], Accessor &styler)
{

	int endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0 && lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags);
	char chNext = styler[startPos];
	for (int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos-1)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2);
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

static const char * const FSWordListDesc[] = {
	"Keywords Commands",
	"Std Library Functions",
	"Procedure, return, exit",
	"Class (oop)",
	"Doxygen keywords",
	0
};

LexerModule lmFlagShip(SCLEX_FLAGSHIP, ColouriseFlagShipDoc, "flagship", FoldFlagShipDoc, FSWordListDesc);
