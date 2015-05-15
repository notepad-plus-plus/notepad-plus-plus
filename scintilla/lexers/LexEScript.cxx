// Scintilla source code edit control
/** @file LexESCRIPT.cxx
 ** Lexer for ESCRIPT
 **/
// Copyright 2003 by Patrizio Bekerle (patrizio@bekerle.com)

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

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
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}



static void ColouriseESCRIPTDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];

	// Do not leak onto next line
	/*if (initStyle == SCE_ESCRIPT_STRINGEOL)
		initStyle = SCE_ESCRIPT_DEFAULT;*/

	StyleContext sc(startPos, length, initStyle, styler);

	bool caseSensitive = styler.GetPropertyInt("escript.case.sensitive", 0) != 0;

	for (; sc.More(); sc.Forward()) {

		/*if (sc.atLineStart && (sc.state == SCE_ESCRIPT_STRING)) {
			// Prevent SCE_ESCRIPT_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_ESCRIPT_STRING);
		}*/

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
		if (sc.state == SCE_ESCRIPT_OPERATOR || sc.state == SCE_ESCRIPT_BRACE) {
			sc.SetState(SCE_ESCRIPT_DEFAULT);
		} else if (sc.state == SCE_ESCRIPT_NUMBER) {
			if (!IsADigit(sc.ch) || sc.ch != '.') {
				sc.SetState(SCE_ESCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_ESCRIPT_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) || (sc.ch == '.')) {
				char s[100];
				if (caseSensitive) {
					sc.GetCurrent(s, sizeof(s));
				} else {
					sc.GetCurrentLowered(s, sizeof(s));
				}

//				sc.GetCurrentLowered(s, sizeof(s));

                                if (keywords.InList(s)) {
					sc.ChangeState(SCE_ESCRIPT_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_ESCRIPT_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_ESCRIPT_WORD3);
                                        // sc.state = SCE_ESCRIPT_IDENTIFIER;
				}
				sc.SetState(SCE_ESCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_ESCRIPT_COMMENT) {
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_ESCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_ESCRIPT_COMMENTDOC) {
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_ESCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_ESCRIPT_COMMENTLINE) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_ESCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_ESCRIPT_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_ESCRIPT_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_ESCRIPT_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_ESCRIPT_NUMBER);
			} else if (IsAWordStart(sc.ch) || (sc.ch == '#')) {
				sc.SetState(SCE_ESCRIPT_IDENTIFIER);
			} else if (sc.Match('/', '*')) {
				sc.SetState(SCE_ESCRIPT_COMMENT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_ESCRIPT_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_ESCRIPT_STRING);
				//} else if (isoperator(static_cast<char>(sc.ch))) {
			} else if (sc.ch == '+' || sc.ch == '-' || sc.ch == '*' || sc.ch == '/' || sc.ch == '=' || sc.ch == '<' || sc.ch == '>' || sc.ch == '&' || sc.ch == '|' || sc.ch == '!' || sc.ch == '?' || sc.ch == ':') {
				sc.SetState(SCE_ESCRIPT_OPERATOR);
			} else if (sc.ch == '{' || sc.ch == '}') {
				sc.SetState(SCE_ESCRIPT_BRACE);
			}
		}

	}
	sc.Complete();
}


static int classifyFoldPointESCRIPT(const char* s, const char* prevWord) {
	int lev = 0;
	if (strcmp(prevWord, "end") == 0) return lev;
	if ((strcmp(prevWord, "else") == 0 && strcmp(s, "if") == 0) || strcmp(s, "elseif") == 0)
		return -1;

        if (strcmp(s, "for") == 0 || strcmp(s, "foreach") == 0
	    || strcmp(s, "program") == 0 || strcmp(s, "function") == 0
	    || strcmp(s, "while") == 0 || strcmp(s, "case") == 0
	    || strcmp(s, "if") == 0 ) {
		lev = 1;
	} else if ( strcmp(s, "endfor") == 0 || strcmp(s, "endforeach") == 0
	    || strcmp(s, "endprogram") == 0 || strcmp(s, "endfunction") == 0
	    || strcmp(s, "endwhile") == 0 || strcmp(s, "endcase") == 0
	    || strcmp(s, "endif") == 0 ) {
		lev = -1;
	}

	return lev;
}


static bool IsStreamCommentStyle(int style) {
	return style == SCE_ESCRIPT_COMMENT ||
	       style == SCE_ESCRIPT_COMMENTDOC ||
	       style == SCE_ESCRIPT_COMMENTLINE;
}

static void FoldESCRIPTDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler) {
	//~ bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	// Do not know how to fold the comment at the moment.
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
        bool foldComment = true;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	int lastStart = 0;
	char prevWord[32] = "";

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');


		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}

		if (foldComment && (style == SCE_ESCRIPT_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelCurrent++;
				} else if (chNext2 == '}') {
					levelCurrent--;
				}
			}
		}

		if (stylePrev == SCE_ESCRIPT_DEFAULT && style == SCE_ESCRIPT_WORD3)
		{
			// Store last word start point.
			lastStart = i;
		}

		if (style == SCE_ESCRIPT_WORD3) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[32];
				unsigned int j;
				for(j = 0; ( j < 31 ) && ( j < i-lastStart+1 ); j++) {
					s[j] = static_cast<char>(tolower(styler[lastStart + j]));
				}
				s[j] = '\0';
				levelCurrent += classifyFoldPointESCRIPT(s, prevWord);
				strcpy(prevWord, s);
			}
		}
		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
			strcpy(prevWord, "");
		}

		if (!isspacechar(ch))
			visibleChars++;
	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}



static const char * const ESCRIPTWordLists[] = {
	"Primary keywords and identifiers",
	"Intrinsic functions",
	"Extended and user defined functions",
	0,
};

LexerModule lmESCRIPT(SCLEX_ESCRIPT, ColouriseESCRIPTDoc, "escript", FoldESCRIPTDoc, ESCRIPTWordLists);
