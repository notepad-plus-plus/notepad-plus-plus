// Scintilla source code edit control
/** @file LexAPDL.cxx
 ** Lexer for APDL. Based on the lexer for Assembler by The Black Horus.
 ** By Hadar Raz.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
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


static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80 && (isalnum(ch) || ch == '_'));
}

static inline bool IsAnOperator(char ch) {
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' || ch == '^' ||
		ch == '[' || ch == ']' || ch == '<' || ch == '&' ||
		ch == '>' || ch == ',' || ch == '|' || ch == '~' ||
		ch == '$' || ch == ':' || ch == '%')
		return true;
	return false;
}

static void ColouriseAPDLDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	int stringStart = ' ';

	WordList &processors = *keywordlists[0];
	WordList &commands = *keywordlists[1];
	WordList &slashcommands = *keywordlists[2];
	WordList &starcommands = *keywordlists[3];
	WordList &arguments = *keywordlists[4];
	WordList &functions = *keywordlists[5];

	// Do not leak onto next line
	initStyle = SCE_APDL_DEFAULT;
	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		if (sc.state == SCE_APDL_NUMBER) {
			if (!(IsADigit(sc.ch) || sc.ch == '.' || (sc.ch == 'e' || sc.ch == 'E') ||
				((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
				sc.SetState(SCE_APDL_DEFAULT);
			}
		} else if (sc.state == SCE_APDL_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_APDL_DEFAULT);
			}
		} else if (sc.state == SCE_APDL_COMMENTBLOCK) {
			if (sc.atLineEnd) {
				if (sc.ch == '\r') {
				sc.Forward();
				}
				sc.ForwardSetState(SCE_APDL_DEFAULT);
			}
		} else if (sc.state == SCE_APDL_STRING) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_APDL_DEFAULT);
			} else if ((sc.ch == '\'' && stringStart == '\'') || (sc.ch == '\"' && stringStart == '\"')) {
				sc.ForwardSetState(SCE_APDL_DEFAULT);
			}
		} else if (sc.state == SCE_APDL_WORD) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (processors.InList(s)) {
					sc.ChangeState(SCE_APDL_PROCESSOR);
				} else if (slashcommands.InList(s)) {
					sc.ChangeState(SCE_APDL_SLASHCOMMAND);
				} else if (starcommands.InList(s)) {
					sc.ChangeState(SCE_APDL_STARCOMMAND);
				} else if (commands.InList(s)) {
					sc.ChangeState(SCE_APDL_COMMAND);
				} else if (arguments.InList(s)) {
					sc.ChangeState(SCE_APDL_ARGUMENT);
				} else if (functions.InList(s)) {
					sc.ChangeState(SCE_APDL_FUNCTION);
				}
				sc.SetState(SCE_APDL_DEFAULT);
			}
		} else if (sc.state == SCE_APDL_OPERATOR) {
			if (!IsAnOperator(static_cast<char>(sc.ch))) {
			    sc.SetState(SCE_APDL_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_APDL_DEFAULT) {
			if (sc.ch == '!' && sc.chNext == '!') {
				sc.SetState(SCE_APDL_COMMENTBLOCK);
			} else if (sc.ch == '!') {
				sc.SetState(SCE_APDL_COMMENT);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_APDL_NUMBER);
			} else if (sc.ch == '\'' || sc.ch == '\"') {
				sc.SetState(SCE_APDL_STRING);
				stringStart = sc.ch;
			} else if (IsAWordChar(sc.ch) || ((sc.ch == '*' || sc.ch == '/') && !isgraph(sc.chPrev))) {
				sc.SetState(SCE_APDL_WORD);
			} else if (IsAnOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_APDL_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static const char * const apdlWordListDesc[] = {
    "processors",
    "commands",
    "slashommands",
    "starcommands",
    "arguments",
    "functions",
    0
};

LexerModule lmAPDL(SCLEX_APDL, ColouriseAPDLDoc, "apdl", 0, apdlWordListDesc);
