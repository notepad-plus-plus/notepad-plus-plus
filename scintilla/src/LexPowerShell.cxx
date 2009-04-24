// Scintilla source code edit control
/** @file LexPowerShell.cxx
 ** Lexer for PowerShell scripts.
 **/
// Copyright 2008 by Tim Gerundt <tim@gerundt.de>
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

// Extended to accept accented characters
static inline bool IsAWordChar(int ch) {
	return ch >= 0x80 || isalnum(ch) || ch == '-';
}

static void ColourisePowerShellDoc(unsigned int startPos, int length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];

	styler.StartAt(startPos);

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_POWERSHELL_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_STRING) {
			// This is a doubles quotes string
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_CHARACTER) {
			// This is a single quote string
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_NUMBER) {
			if (!IsADigit(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_VARIABLE) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_OPERATOR) {
			if (!isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (keywords.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_KEYWORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_CMDLET);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_ALIAS);
				}
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_POWERSHELL_DEFAULT) {
			if (sc.ch == '#') {
				sc.SetState(SCE_POWERSHELL_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_POWERSHELL_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_POWERSHELL_CHARACTER);
			} else if (sc.ch == '$') {
				sc.SetState(SCE_POWERSHELL_VARIABLE);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_POWERSHELL_NUMBER);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POWERSHELL_OPERATOR);
			} else if (IsAWordChar(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_IDENTIFIER);
			}
		}
	}
	sc.Complete();
}


LexerModule lmPowerShell(SCLEX_POWERSHELL, ColourisePowerShellDoc, "powershell");

