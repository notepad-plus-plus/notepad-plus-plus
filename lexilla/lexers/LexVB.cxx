// Scintilla source code edit control
/** @file LexVB.cxx
 ** Lexer for Visual Basic and VBScript.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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

// Internal state, highlighted as number
#define SCE_B_FILENUMBER SCE_B_DEFAULT+100


static bool IsVBComment(Accessor &styler, Sci_Position pos, Sci_Position len) {
	return len > 0 && styler[pos] == '\'';
}

static inline bool IsTypeCharacter(int ch) {
	return ch == '%' || ch == '&' || ch == '@' || ch == '!' || ch == '#' || ch == '$';
}

// Extended to accept accented characters
static inline bool IsAWordChar(int ch) {
	return ch >= 0x80 ||
	       (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return ch >= 0x80 ||
	       (isalpha(ch) || ch == '_');
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
             ch == '.' || ch == '-' || ch == '+' || ch == '_');
}

static void ColouriseVBDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                           WordList *keywordlists[], Accessor &styler, bool vbScriptSyntax) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	styler.StartAt(startPos);

	int visibleChars = 0;
	int fileNbDigits = 0;

	// property lexer.vb.strings.multiline
	//  Set to 1 to allow strings to continue over line ends.
	bool allowMultilineStr = styler.GetPropertyInt("lexer.vb.strings.multiline", 0) != 0;

	// Do not leak onto next line
	if (initStyle == SCE_B_STRINGEOL || initStyle == SCE_B_COMMENT || initStyle == SCE_B_PREPROCESSOR) {
		initStyle = SCE_B_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_B_OPERATOR) {
			sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				// In Basic (except VBScript), a variable name or a function name
				// can end with a special character indicating the type of the value
				// held or returned.
				bool skipType = false;
				if (!vbScriptSyntax && IsTypeCharacter(sc.ch)) {
					sc.Forward();	// Skip it
					skipType = true;
				}
				if (sc.ch == ']') {
					sc.Forward();
				}
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (skipType) {
					s[strlen(s) - 1] = '\0';
				}
				if (strcmp(s, "rem") == 0) {
					sc.ChangeState(SCE_B_COMMENT);
				} else {
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_B_KEYWORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_B_KEYWORD2);
					} else if (keywords3.InList(s)) {
						sc.ChangeState(SCE_B_KEYWORD3);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_B_KEYWORD4);
					}	// Else, it is really an identifier...
					sc.SetState(SCE_B_DEFAULT);
				}
			}
		} else if (sc.state == SCE_B_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			// Also accepts A-F for hex. numbers
			if (!IsANumberChar(sc.ch) && !(tolower(sc.ch) >= 'a' && tolower(sc.ch) <= 'f')) {
				sc.SetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_STRING) {
			// VB doubles quotes to preserve them, so just end this string
			// state now as a following quote will start again
			if (sc.ch == '\"') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					if (tolower(sc.chNext) == 'c') {
						sc.Forward();
					}
					sc.ForwardSetState(SCE_B_DEFAULT);
				}
			} else if (sc.atLineEnd && !allowMultilineStr) {
				visibleChars = 0;
				sc.ChangeState(SCE_B_STRINGEOL);
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_COMMENT) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_PREPROCESSOR) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_FILENUMBER) {
			if (IsADigit(sc.ch)) {
				fileNbDigits++;
				if (fileNbDigits > 3) {
					sc.ChangeState(SCE_B_DATE);
				}
			} else if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ',') {
				// Regular uses: Close #1; Put #1, ...; Get #1, ... etc.
				// Too bad if date is format #27, Oct, 2003# or something like that...
				// Use regular number state
				sc.ChangeState(SCE_B_NUMBER);
				sc.SetState(SCE_B_DEFAULT);
			} else if (sc.ch == '#') {
				sc.ChangeState(SCE_B_DATE);
				sc.ForwardSetState(SCE_B_DEFAULT);
			} else {
				sc.ChangeState(SCE_B_DATE);
			}
			if (sc.state != SCE_B_FILENUMBER) {
				fileNbDigits = 0;
			}
		} else if (sc.state == SCE_B_DATE) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ChangeState(SCE_B_STRINGEOL);
				sc.ForwardSetState(SCE_B_DEFAULT);
			} else if (sc.ch == '#') {
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		}

		if (sc.state == SCE_B_DEFAULT) {
			if (sc.ch == '\'') {
				sc.SetState(SCE_B_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_B_STRING);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_B_PREPROCESSOR);
			} else if (sc.ch == '#') {
				// It can be a date literal, ending with #, or a file number, from 1 to 511
				// The date literal depends on the locale, so anything can go between #'s.
				// Can be #January 1, 1993# or #1 Jan 93# or #05/11/2003#, etc.
				// So we set the FILENUMBER state, and switch to DATE if it isn't a file number
				sc.SetState(SCE_B_FILENUMBER);
			} else if (sc.ch == '&' && tolower(sc.chNext) == 'h') {
				// Hexadecimal number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (sc.ch == '&' && tolower(sc.chNext) == 'o') {
				// Octal number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (sc.ch == '&' && tolower(sc.chNext) == 'b') {
				// Binary number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_B_NUMBER);
			} else if (IsAWordStart(sc.ch) || (sc.ch == '[')) {
				sc.SetState(SCE_B_IDENTIFIER);
			} else if (isoperator(static_cast<char>(sc.ch)) || (sc.ch == '\\')) {	// Integer division
				sc.SetState(SCE_B_OPERATOR);
			}
		}

		if (sc.atLineEnd) {
			visibleChars = 0;
		}
		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}

	if (sc.state == SCE_B_IDENTIFIER && !IsAWordChar(sc.ch)) {
		// In Basic (except VBScript), a variable name or a function name
		// can end with a special character indicating the type of the value
		// held or returned.
		bool skipType = false;
		if (!vbScriptSyntax && IsTypeCharacter(sc.ch)) {
			sc.Forward();	// Skip it
			skipType = true;
		}
		if (sc.ch == ']') {
			sc.Forward();
		}
		char s[100];
		sc.GetCurrentLowered(s, sizeof(s));
		if (skipType) {
			s[strlen(s) - 1] = '\0';
		}
		if (strcmp(s, "rem") == 0) {
			sc.ChangeState(SCE_B_COMMENT);
		} else {
			if (keywords.InList(s)) {
				sc.ChangeState(SCE_B_KEYWORD);
			} else if (keywords2.InList(s)) {
				sc.ChangeState(SCE_B_KEYWORD2);
			} else if (keywords3.InList(s)) {
				sc.ChangeState(SCE_B_KEYWORD3);
			} else if (keywords4.InList(s)) {
				sc.ChangeState(SCE_B_KEYWORD4);
			}	// Else, it is really an identifier...
			sc.SetState(SCE_B_DEFAULT);
		}
	}

	sc.Complete();
}

static void FoldVBDoc(Sci_PositionU startPos, Sci_Position length, int,
						   WordList *[], Accessor &styler) {
	Sci_Position endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsVBComment);
	char chNext = styler[startPos];
	for (Sci_Position i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsVBComment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsVBComment);
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

static void ColouriseVBNetDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {
	ColouriseVBDoc(startPos, length, initStyle, keywordlists, styler, false);
}

static void ColouriseVBScriptDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {
	ColouriseVBDoc(startPos, length, initStyle, keywordlists, styler, true);
}

static const char * const vbWordListDesc[] = {
	"Keywords",
	"user1",
	"user2",
	"user3",
	0
};

LexerModule lmVB(SCLEX_VB, ColouriseVBNetDoc, "vb", FoldVBDoc, vbWordListDesc);
LexerModule lmVBScript(SCLEX_VBSCRIPT, ColouriseVBScriptDoc, "vbscript", FoldVBDoc, vbWordListDesc);

