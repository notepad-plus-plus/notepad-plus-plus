// Scintilla source code edit control
/** @file LexForth.cxx
 ** Lexer for FORTH
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
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

static inline bool IsAWordStart(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.');
}

static inline bool IsANumChar(int ch) {
	return (ch < 0x80) && (isxdigit(ch) || ch == '.' || ch == 'e' || ch == 'E' );
}

static inline bool IsASpaceChar(int ch) {
	return (ch < 0x80) && isspace(ch);
}

static void ColouriseForthDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordLists[],
                            Accessor &styler) {

    WordList &control = *keywordLists[0];
    WordList &keyword = *keywordLists[1];
    WordList &defword = *keywordLists[2];
    WordList &preword1 = *keywordLists[3];
    WordList &preword2 = *keywordLists[4];
    WordList &strings = *keywordLists[5];

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
	{
		// Determine if the current state should terminate.
		if (sc.state == SCE_FORTH_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_FORTH_DEFAULT);
			}
		}else if (sc.state == SCE_FORTH_COMMENT_ML) {
			if (sc.ch == ')') {
				sc.ForwardSetState(SCE_FORTH_DEFAULT);
			}
		}else if (sc.state == SCE_FORTH_IDENTIFIER || sc.state == SCE_FORTH_NUMBER) {
			// handle numbers here too, because what we thought was a number might
			// turn out to be a keyword e.g. 2DUP
			if (IsASpaceChar(sc.ch) ) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				int newState = sc.state == SCE_FORTH_NUMBER ? SCE_FORTH_NUMBER : SCE_FORTH_DEFAULT;
				if (control.InList(s)) {
					sc.ChangeState(SCE_FORTH_CONTROL);
				} else if (keyword.InList(s)) {
					sc.ChangeState(SCE_FORTH_KEYWORD);
				} else if (defword.InList(s)) {
					sc.ChangeState(SCE_FORTH_DEFWORD);
				}  else if (preword1.InList(s)) {
					sc.ChangeState(SCE_FORTH_PREWORD1);
				} else if (preword2.InList(s)) {
					sc.ChangeState(SCE_FORTH_PREWORD2);
				} else if (strings.InList(s)) {
					sc.ChangeState(SCE_FORTH_STRING);
					newState = SCE_FORTH_STRING;
				}
				sc.SetState(newState);
			}
			if (sc.state == SCE_FORTH_NUMBER) {
				if (IsASpaceChar(sc.ch)) {
					sc.SetState(SCE_FORTH_DEFAULT);
				} else if (!IsANumChar(sc.ch)) {
					sc.ChangeState(SCE_FORTH_IDENTIFIER);
				}
			}
		}else if (sc.state == SCE_FORTH_STRING) {
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_FORTH_DEFAULT);
			}
		}else if (sc.state == SCE_FORTH_LOCALE) {
			if (sc.ch == '}') {
				sc.ForwardSetState(SCE_FORTH_DEFAULT);
			}
		}else if (sc.state == SCE_FORTH_DEFWORD) {
			if (IsASpaceChar(sc.ch)) {
				sc.SetState(SCE_FORTH_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_FORTH_DEFAULT) {
			if (sc.ch == '\\'){
				sc.SetState(SCE_FORTH_COMMENT);
			} else if (sc.ch == '(' &&
					(sc.atLineStart || IsASpaceChar(sc.chPrev)) &&
					(sc.atLineEnd   || IsASpaceChar(sc.chNext))) {
				sc.SetState(SCE_FORTH_COMMENT_ML);
			} else if (	(sc.ch == '$' && (IsASCII(sc.chNext) && isxdigit(sc.chNext))) ) {
				// number starting with $ is a hex number
				sc.SetState(SCE_FORTH_NUMBER);
				while(sc.More() && IsASCII(sc.chNext) && isxdigit(sc.chNext))
					sc.Forward();
			} else if ( (sc.ch == '%' && (IsASCII(sc.chNext) && (sc.chNext == '0' || sc.chNext == '1'))) ) {
				// number starting with % is binary
				sc.SetState(SCE_FORTH_NUMBER);
				while(sc.More() && IsASCII(sc.chNext) && (sc.chNext == '0' || sc.chNext == '1'))
					sc.Forward();
			} else if (	IsASCII(sc.ch) &&
						(isxdigit(sc.ch) || ((sc.ch == '.' || sc.ch == '-') && IsASCII(sc.chNext) && isxdigit(sc.chNext)) )
					){
				sc.SetState(SCE_FORTH_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_FORTH_IDENTIFIER);
			} else if (sc.ch == '{') {
				sc.SetState(SCE_FORTH_LOCALE);
			} else if (sc.ch == ':' && IsASCII(sc.chNext) && isspace(sc.chNext)) {
				// highlight word definitions e.g.  : GCD ( n n -- n ) ..... ;
				//                                  ^ ^^^
				sc.SetState(SCE_FORTH_DEFWORD);
				while(sc.More() && IsASCII(sc.chNext) && isspace(sc.chNext))
					sc.Forward();
			} else if (sc.ch == ';' &&
					(sc.atLineStart || IsASpaceChar(sc.chPrev)) &&
					(sc.atLineEnd   || IsASpaceChar(sc.chNext))	) {
				// mark the ';' that ends a word
				sc.SetState(SCE_FORTH_DEFWORD);
				sc.ForwardSetState(SCE_FORTH_DEFAULT);
			}
		}

	}
	sc.Complete();
}

static void FoldForthDoc(Sci_PositionU, Sci_Position, int, WordList *[],
						Accessor &) {
}

static const char * const forthWordLists[] = {
			"control keywords",
			"keywords",
			"definition words",
			"prewords with one argument",
			"prewords with two arguments",
			"string definition keywords",
			0,
		};

extern const LexerModule lmForth(SCLEX_FORTH, ColouriseForthDoc, "forth", FoldForthDoc, forthWordLists);


