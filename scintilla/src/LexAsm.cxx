// Scintilla source code edit control
/** @file LexAsm.cxx
 ** Lexer for Assembler, just for the MASM syntax
 ** Written by The Black Horus
 ** Enhancements and NASM stuff by Kein-Hong Man, 2003-10
 ** SCE_ASM_COMMENTBLOCK and SCE_ASM_CHARACTER are for future GNU as colouring
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
	return (ch < 0x80) && (isalnum(ch) || ch == '.' ||
		ch == '_' || ch == '?');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.' ||
		ch == '%' || ch == '@' || ch == '$' || ch == '?');
}

static inline bool IsAsmOperator(char ch) {
	if (isalnum(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' || ch == '^' ||
		ch == '[' || ch == ']' || ch == '<' || ch == '&' ||
		ch == '>' || ch == ',' || ch == '|' || ch == '~' ||
		ch == '%' || ch == ':')
		return true;
	return false;
}

static void ColouriseAsmDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &cpuInstruction = *keywordlists[0];
	WordList &mathInstruction = *keywordlists[1];
	WordList &registers = *keywordlists[2];
	WordList &directive = *keywordlists[3];
	WordList &directiveOperand = *keywordlists[4];
	WordList &extInstruction = *keywordlists[5];

	// Do not leak onto next line
	if (initStyle == SCE_ASM_STRINGEOL)
		initStyle = SCE_ASM_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
	{

		// Prevent SCE_ASM_STRINGEOL from leaking back to previous line
		if (sc.atLineStart && (sc.state == SCE_ASM_STRING)) {
			sc.SetState(SCE_ASM_STRING);
		} else if (sc.atLineStart && (sc.state == SCE_ASM_CHARACTER)) {
			sc.SetState(SCE_ASM_CHARACTER);
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_ASM_OPERATOR) {
			if (!IsAsmOperator(static_cast<char>(sc.ch))) {
			    sc.SetState(SCE_ASM_DEFAULT);
			}
		}else if (sc.state == SCE_ASM_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) ) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (cpuInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_CPUINSTRUCTION);
				} else if (mathInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_MATHINSTRUCTION);
				} else if (registers.InList(s)) {
					sc.ChangeState(SCE_ASM_REGISTER);
				}  else if (directive.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVE);
				} else if (directiveOperand.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVEOPERAND);
				} else if (extInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_EXTINSTRUCTION);
				}
				sc.SetState(SCE_ASM_DEFAULT);
			}
		}
		else if (sc.state == SCE_ASM_COMMENT ) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_CHARACTER) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_ASM_DEFAULT) {
			if (sc.ch == ';'){
				sc.SetState(SCE_ASM_COMMENT);
			} else if (isdigit(sc.ch) || (sc.ch == '.' && isdigit(sc.chNext))) {
				sc.SetState(SCE_ASM_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_ASM_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_ASM_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_ASM_CHARACTER);
			} else if (IsAsmOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_ASM_OPERATOR);
			}
		}

	}
	sc.Complete();
}

static const char * const asmWordListDesc[] = {
	"CPU instructions",
	"FPU instructions",
	"Registers",
	"Directives",
	"Directive operands",
	"Extended instructions",
	0
};

LexerModule lmAsm(SCLEX_ASM, ColouriseAsmDoc, "asm", 0, asmWordListDesc);

