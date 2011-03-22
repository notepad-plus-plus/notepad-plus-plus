// Scintilla source code edit control
/** @file LexCsound.cxx
 ** Lexer for Csound (Orchestra & Score)
 ** Written by Georg Ritter - <ritterfuture A T gmail D O T com>
 **/
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

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' ||
		ch == '_' || ch == '?');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.' ||
		ch == '%' || ch == '@' || ch == '$' || ch == '?');
}

static inline bool IsCsoundOperator(char ch) {
	if (isascii(ch) && isalnum(ch))
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

static void ColouriseCsoundDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
				Accessor &styler) {

	WordList &opcode = *keywordlists[0];
	WordList &headerStmt = *keywordlists[1];
	WordList &otherKeyword = *keywordlists[2];

	// Do not leak onto next line
	if (initStyle == SCE_CSOUND_STRINGEOL)
		initStyle = SCE_CSOUND_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
	{
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
		if (sc.state == SCE_CSOUND_OPERATOR) {
			if (!IsCsoundOperator(static_cast<char>(sc.ch))) {
			    sc.SetState(SCE_CSOUND_DEFAULT);
			}
		}else if (sc.state == SCE_CSOUND_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_CSOUND_DEFAULT);
			}
		} else if (sc.state == SCE_CSOUND_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) ) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));

				if (opcode.InList(s)) {
					sc.ChangeState(SCE_CSOUND_OPCODE);
				} else if (headerStmt.InList(s)) {
					sc.ChangeState(SCE_CSOUND_HEADERSTMT);
				} else if (otherKeyword.InList(s)) {
					sc.ChangeState(SCE_CSOUND_USERKEYWORD);
				} else if (s[0] == 'p') {
					sc.ChangeState(SCE_CSOUND_PARAM);
				} else if (s[0] == 'a') {
					sc.ChangeState(SCE_CSOUND_ARATE_VAR);
				} else if (s[0] == 'k') {
					sc.ChangeState(SCE_CSOUND_KRATE_VAR);
				} else if (s[0] == 'i') { // covers both i-rate variables and i-statements
					sc.ChangeState(SCE_CSOUND_IRATE_VAR);
				} else if (s[0] == 'g') {
					sc.ChangeState(SCE_CSOUND_GLOBAL_VAR);
				}
				sc.SetState(SCE_CSOUND_DEFAULT);
			}
		}
		else if (sc.state == SCE_CSOUND_COMMENT ) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_CSOUND_DEFAULT);
			}
		}
		else if ((sc.state == SCE_CSOUND_ARATE_VAR) ||
			(sc.state == SCE_CSOUND_KRATE_VAR) ||
		(sc.state == SCE_CSOUND_IRATE_VAR)) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_CSOUND_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_CSOUND_DEFAULT) {
			if (sc.ch == ';'){
				sc.SetState(SCE_CSOUND_COMMENT);
			} else if (isdigit(sc.ch) || (sc.ch == '.' && isdigit(sc.chNext))) {
				sc.SetState(SCE_CSOUND_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_CSOUND_IDENTIFIER);
			} else if (IsCsoundOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_CSOUND_OPERATOR);
			} else if (sc.ch == 'p') {
				sc.SetState(SCE_CSOUND_PARAM);
			} else if (sc.ch == 'a') {
				sc.SetState(SCE_CSOUND_ARATE_VAR);
			} else if (sc.ch == 'k') {
				sc.SetState(SCE_CSOUND_KRATE_VAR);
			} else if (sc.ch == 'i') { // covers both i-rate variables and i-statements
				sc.SetState(SCE_CSOUND_IRATE_VAR);
			} else if (sc.ch == 'g') {
				sc.SetState(SCE_CSOUND_GLOBAL_VAR);
			}
		}
	}
	sc.Complete();
}

static void FoldCsoundInstruments(unsigned int startPos, int length, int /* initStyle */, WordList *[],
		Accessor &styler) {
	unsigned int lengthDoc = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int stylePrev = 0;
	int styleNext = styler.StyleAt(startPos);
	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if ((stylePrev != SCE_CSOUND_OPCODE) && (style == SCE_CSOUND_OPCODE)) {
			char s[20];
			unsigned int j = 0;
			while ((j < (sizeof(s) - 1)) && (iswordchar(styler[i + j]))) {
				s[j] = styler[i + j];
				j++;
			}
			s[j] = '\0';

			if (strcmp(s, "instr") == 0)
				levelCurrent++;
			if (strcmp(s, "endin") == 0)
				levelCurrent--;
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0)
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
		stylePrev = style;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}


static const char * const csoundWordListDesc[] = {
	"Opcodes",
	"Header Statements",
	"User keywords",
	0
};

LexerModule lmCsound(SCLEX_CSOUND, ColouriseCsoundDoc, "csound", FoldCsoundInstruments, csoundWordListDesc);
