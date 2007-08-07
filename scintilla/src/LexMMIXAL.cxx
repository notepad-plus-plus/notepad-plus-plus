// Scintilla source code edit control
/** @file LexMMIXAL.cxx
 ** Lexer for MMIX Assembler Language.
 ** Written by Christoph Hösler <christoph.hoesler@student.uni-tuebingen.de>
 ** For information about MMIX visit http://www-cs-faculty.stanford.edu/~knuth/mmix.html
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
	return (ch < 0x80) && (isalnum(ch) || ch == ':' || ch == '_');
}

inline bool isMMIXALOperator(char ch) {
	if (isalnum(ch))
		return false;
	if (ch == '+' || ch == '-' || ch == '|' || ch == '^' ||
		ch == '*' || ch == '/' || ch == '/' ||
		ch == '%' || ch == '<' || ch == '>' || ch == '&' ||
		ch == '~' || ch == '$' ||
		ch == ',' || ch == '(' || ch == ')' ||
		ch == '[' || ch == ']')
		return true;
	return false;
}

static void ColouriseMMIXALDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &opcodes = *keywordlists[0];
	WordList &special_register = *keywordlists[1];
	WordList &predef_symbols = *keywordlists[2];

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
	{
		// No EOL continuation
		if (sc.atLineStart) {
			if (sc.ch ==  '@' && sc.chNext == 'i') {
				sc.SetState(SCE_MMIXAL_INCLUDE);
			} else {
				sc.SetState(SCE_MMIXAL_LEADWS);
			}
		}

		// Check if first non whitespace character in line is alphanumeric
		if (sc.state == SCE_MMIXAL_LEADWS && !isspace(sc.ch)) {	// LEADWS
			if(!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_MMIXAL_COMMENT);
			} else {
				if(sc.atLineStart) {
					sc.SetState(SCE_MMIXAL_LABEL);
				} else {
					sc.SetState(SCE_MMIXAL_OPCODE_PRE);
				}
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_MMIXAL_OPERATOR) {			// OPERATOR
			sc.SetState(SCE_MMIXAL_OPERANDS);
		} else if (sc.state == SCE_MMIXAL_NUMBER) {		// NUMBER
			if (!isdigit(sc.ch)) {
				if (IsAWordChar(sc.ch)) {
					char s[100];
					sc.GetCurrent(s, sizeof(s));
					sc.ChangeState(SCE_MMIXAL_REF);
					sc.SetState(SCE_MMIXAL_REF);
				} else {
					sc.SetState(SCE_MMIXAL_OPERANDS);
				}
			}
		} else if (sc.state == SCE_MMIXAL_LABEL) {			// LABEL
			if (!IsAWordChar(sc.ch) ) {
				sc.SetState(SCE_MMIXAL_OPCODE_PRE);
			}
		} else if (sc.state == SCE_MMIXAL_REF) {			// REF
			if (!IsAWordChar(sc.ch) ) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (*s == ':') {	// ignore base prefix for match
					for (size_t i = 0; i != sizeof(s); ++i) {
						*(s+i) = *(s+i+1);
					}
				}
				if (special_register.InList(s)) {
					sc.ChangeState(SCE_MMIXAL_REGISTER);
				} else if (predef_symbols.InList(s)) {
					sc.ChangeState(SCE_MMIXAL_SYMBOL);
				}
				sc.SetState(SCE_MMIXAL_OPERANDS);
			}
		} else if (sc.state == SCE_MMIXAL_OPCODE_PRE) {	// OPCODE_PRE
				if (!isspace(sc.ch)) {
					sc.SetState(SCE_MMIXAL_OPCODE);
				}
		} else if (sc.state == SCE_MMIXAL_OPCODE) {		// OPCODE
			if (!IsAWordChar(sc.ch) ) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (opcodes.InList(s)) {
					sc.ChangeState(SCE_MMIXAL_OPCODE_VALID);
				} else {
					sc.ChangeState(SCE_MMIXAL_OPCODE_UNKNOWN);
				}
				sc.SetState(SCE_MMIXAL_OPCODE_POST);
			}
		} else if (sc.state == SCE_MMIXAL_STRING) {		// STRING
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_MMIXAL_OPERANDS);
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_MMIXAL_OPERANDS);
			}
		} else if (sc.state == SCE_MMIXAL_CHAR) {			// CHAR
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_MMIXAL_OPERANDS);
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_MMIXAL_OPERANDS);
			}
		} else if (sc.state == SCE_MMIXAL_REGISTER) {		// REGISTER
			if (!isdigit(sc.ch)) {
				sc.SetState(SCE_MMIXAL_OPERANDS);
			}
		} else if (sc.state == SCE_MMIXAL_HEX) {			// HEX
			if (!isxdigit(sc.ch)) {
				sc.SetState(SCE_MMIXAL_OPERANDS);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_MMIXAL_OPCODE_POST ||		// OPCODE_POST
			sc.state == SCE_MMIXAL_OPERANDS) {			// OPERANDS
			if (sc.state == SCE_MMIXAL_OPERANDS && isspace(sc.ch)) {
				if (!sc.atLineEnd) {
					sc.SetState(SCE_MMIXAL_COMMENT);
				}
			} else if (isdigit(sc.ch)) {
				sc.SetState(SCE_MMIXAL_NUMBER);
			} else if (IsAWordChar(sc.ch) || sc.Match('@')) {
				sc.SetState(SCE_MMIXAL_REF);
			} else if (sc.Match('\"')) {
				sc.SetState(SCE_MMIXAL_STRING);
			} else if (sc.Match('\'')) {
				sc.SetState(SCE_MMIXAL_CHAR);
			} else if (sc.Match('$')) {
				sc.SetState(SCE_MMIXAL_REGISTER);
			} else if (sc.Match('#')) {
				sc.SetState(SCE_MMIXAL_HEX);
			} else if (isMMIXALOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_MMIXAL_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static const char * const MMIXALWordListDesc[] = {
	"Operation Codes",
	"Special Register",
	"Predefined Symbols",
	0
};

LexerModule lmMMIXAL(SCLEX_MMIXAL, ColouriseMMIXALDoc, "mmixal", 0, MMIXALWordListDesc);

