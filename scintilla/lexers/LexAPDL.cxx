// Scintilla source code edit control
/** @file LexAPDL.cxx
 ** Lexer for APDL. Based on the lexer for Assembler by The Black Horus.
 ** By Hadar Raz.
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

using namespace Scintilla;

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

static void ColouriseAPDLDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
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

//------------------------------------------------------------------------------
// 06-27-07 Sergio Lucato
// - Included code folding for Ansys APDL lexer
// - Copyied from LexBasic.cxx and modified for APDL
//------------------------------------------------------------------------------

/* Bits:
 * 1  - whitespace
 * 2  - operator
 * 4  - identifier
 * 8  - decimal digit
 * 16 - hex digit
 * 32 - bin digit
 */
static int character_classification[128] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  0,  2,  2,  2,  2,  2,  2,  2,  6,  2,  2,  2,  10, 6,
    60, 60, 28, 28, 28, 28, 28, 28, 28, 28, 2,  2,  2,  2,  2,  2,
    2,  20, 20, 20, 20, 20, 20, 4,  4,  4,  4,  4,  4,  4,  4,  4,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  4,
    2,  20, 20, 20, 20, 20, 20, 4,  4,  4,  4,  4,  4,  4,  4,  4,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  0
};

static bool IsSpace(int c) {
	return c < 128 && (character_classification[c] & 1);
}

static bool IsIdentifier(int c) {
	return c < 128 && (character_classification[c] & 4);
}

static int LowerCase(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 'a' + c - 'A';
	return c;
}

static int CheckAPDLFoldPoint(char const *token, int &level) {
	if (!strcmp(token, "*if") ||
		!strcmp(token, "*do") ||
		!strcmp(token, "*dowhile") ) {
		level |= SC_FOLDLEVELHEADERFLAG;
		return 1;
	}
	if (!strcmp(token, "*endif") ||
		!strcmp(token, "*enddo") ) {
		return -1;
	}
	return 0;
}

static void FoldAPDLDoc(Sci_PositionU startPos, Sci_Position length, int,
	WordList *[], Accessor &styler) {

	Sci_Position line = styler.GetLine(startPos);
	int level = styler.LevelAt(line);
	int go = 0, done = 0;
	Sci_Position endPos = startPos + length;
	char word[256];
	int wordlen = 0;
	Sci_Position i;
    bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	// Scan for tokens at the start of the line (they may include
	// whitespace, for tokens like "End Function"
	for (i = startPos; i < endPos; i++) {
		int c = styler.SafeGetCharAt(i);
		if (!done && !go) {
			if (wordlen) { // are we scanning a token already?
				word[wordlen] = static_cast<char>(LowerCase(c));
				if (!IsIdentifier(c)) { // done with token
					word[wordlen] = '\0';
					go = CheckAPDLFoldPoint(word, level);
					if (!go) {
						// Treat any whitespace as single blank, for
						// things like "End   Function".
						if (IsSpace(c) && IsIdentifier(word[wordlen - 1])) {
							word[wordlen] = ' ';
							if (wordlen < 255)
								wordlen++;
						}
						else // done with this line
							done = 1;
					}
				} else if (wordlen < 255) {
					wordlen++;
				}
			} else { // start scanning at first non-whitespace character
				if (!IsSpace(c)) {
					if (IsIdentifier(c)) {
						word[0] = static_cast<char>(LowerCase(c));
						wordlen = 1;
					} else // done with this line
						done = 1;
				}
			}
		}
		if (c == '\n') { // line end
			if (!done && wordlen == 0 && foldCompact) // line was only space
				level |= SC_FOLDLEVELWHITEFLAG;
			if (level != styler.LevelAt(line))
				styler.SetLevel(line, level);
			level += go;
			line++;
			// reset state
			wordlen = 0;
			level &= ~SC_FOLDLEVELHEADERFLAG;
			level &= ~SC_FOLDLEVELWHITEFLAG;
			go = 0;
			done = 0;
		}
	}
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

LexerModule lmAPDL(SCLEX_APDL, ColouriseAPDLDoc, "apdl", FoldAPDLDoc, apdlWordListDesc);
