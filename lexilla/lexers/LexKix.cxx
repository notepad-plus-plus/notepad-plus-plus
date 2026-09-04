// Scintilla source code edit control
/** @file LexKix.cxx
 ** Lexer for KIX-Scripts.
 **/
// Copyright 2004 by Manfred Becker <manfred@becker-trdf.de>
// The License.txt file describes the conditions under which this software may be distributed.
// Edited by Lee Wilmott (24-Jun-2014) added support for block comments

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

// Extended to accept accented characters
static inline bool IsAWordChar(int ch) {
	return ch >= 0x80 || isalnum(ch) || ch == '_';
}

static inline bool IsOperator(const int ch) {
	return (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '&' || ch == '|' || ch == '<' || ch == '>' || ch == '=');
}

static void ColouriseKixDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
//	WordList &keywords4 = *keywordlists[3];

	styler.StartAt(startPos);

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_KIX_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_COMMENTSTREAM) {
			if (sc.ch == '/' && sc.chPrev == '*') {
				sc.ForwardSetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_STRING1) {
			// This is a doubles quotes string
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_STRING2) {
			// This is a single quote string
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_NUMBER) {
			if (!IsADigit(sc.ch)) {
				sc.SetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_VAR) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_MACRO) {
			if (!IsAWordChar(sc.ch) && !IsADigit(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (!keywords3.InList(&s[1])) {
					sc.ChangeState(SCE_KIX_DEFAULT);
				}
				sc.SetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_OPERATOR) {
			if (!IsOperator(sc.ch)) {
				sc.SetState(SCE_KIX_DEFAULT);
			}
		} else if (sc.state == SCE_KIX_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (keywords.InList(s)) {
					sc.ChangeState(SCE_KIX_KEYWORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_KIX_FUNCTIONS);
				}
				sc.SetState(SCE_KIX_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_KIX_DEFAULT) {
			if (sc.ch == ';') {
				sc.SetState(SCE_KIX_COMMENT);
			} else if (sc.ch == '/' && sc.chNext == '*') {
				sc.SetState(SCE_KIX_COMMENTSTREAM);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_KIX_STRING1);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_KIX_STRING2);
			} else if (sc.ch == '$') {
				sc.SetState(SCE_KIX_VAR);
			} else if (sc.ch == '@') {
				sc.SetState(SCE_KIX_MACRO);
			} else if (IsADigit(sc.ch) || ((sc.ch == '.' || sc.ch == '&') && IsADigit(sc.chNext))) {
				sc.SetState(SCE_KIX_NUMBER);
			} else if (IsOperator(sc.ch)) {
				sc.SetState(SCE_KIX_OPERATOR);
			} else if (IsAWordChar(sc.ch)) {
				sc.SetState(SCE_KIX_IDENTIFIER);
			}
		}
	}
	sc.Complete();
}


extern const LexerModule lmKix(SCLEX_KIX, ColouriseKixDoc, "kix");

