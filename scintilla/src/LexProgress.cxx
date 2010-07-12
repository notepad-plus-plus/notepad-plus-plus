// Scintilla source code edit control
/** @file LexProgress.cxx
 **  Lexer for Progress 4GL.
 ** Based on LexCPP.cxx of Neil Hodgson <neilh@scintilla.org>
  **/
// Copyright 2006-2007 by Yuval Papish <Yuval@YuvCom.com>
// The License.txt file describes the conditions under which this software may be distributed.

/** TODO:
WebSpeed support in html lexer
Support "end triggers" expression of the triggers phrase
Support more than 6 comments levels
**/
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

static inline bool IsAWordChar(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return (ch < 0x80) && (isalpha(ch) || ch == '_');
}

enum SentenceStart { SetSentenceStart = 0xf, ResetSentenceStart = 0x10}; // true -> bit = 0

static void Colourise4glDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

    WordList &keywords1 = *keywordlists[0];   // regular keywords
    WordList &keywords2 = *keywordlists[1];   // block opening keywords, only when SentenceStart
    WordList &keywords3 = *keywordlists[2];   // block opening keywords
    //WordList &keywords4 = *keywordlists[3]; // preprocessor keywords. Not implemented
    

	int visibleChars = 0;
	int mask;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
		}

		// Handle line continuation generically.
		if ((sc.state & 0xf) < SCE_4GL_COMMENT1) {
		if (sc.ch == '~') {
			if (sc.chNext > ' ') {
				// skip special char after ~
				sc.Forward();
				continue;
			}
			else {
				// Skip whitespace between ~ and EOL
				while (sc.More() && (sc.chNext == ' ' || sc.chNext == '\t') ) {
					sc.Forward();
				}
				if (sc.chNext == '\n' || sc.chNext == '\r') {
					sc.Forward();
					if (sc.ch == '\r' && sc.chNext == '\n') {
						sc.Forward();
					}
					sc.Forward();
					continue;
				}
			}
		}
		}
		// Determine if a new state should be terminated.
		mask = sc.state & 0x10;
		switch (sc.state & 0xf) {
			case SCE_4GL_OPERATOR:
				sc.SetState(SCE_4GL_DEFAULT | mask);
				break;
			case SCE_4GL_NUMBER:
				if (!(IsADigit(sc.ch))) {
					sc.SetState(SCE_4GL_DEFAULT | mask);
				}
				break;
			case SCE_4GL_IDENTIFIER:
				if (!IsAWordChar(sc.ch) && sc.ch != '-') {
					char s[1000];
					sc.GetCurrentLowered(s, sizeof(s));
					if ((((sc.state & 0x10) == 0) && keywords2.InList(s)) || keywords3.InList(s)) {
						sc.ChangeState(SCE_4GL_BLOCK | ResetSentenceStart);
					}
					else if (keywords1.InList(s)) {
						if ((s[0] == 'e' && s[1] =='n' && s[2] == 'd' && !isalnum(s[3]) && s[3] != '-') ||
							(s[0] == 'f' && s[1] =='o' && s[2] == 'r' && s[3] == 'w' && s[4] =='a' && s[5] == 'r' && s[6] == 'd'&& !isalnum(s[7]))) {
							sc.ChangeState(SCE_4GL_END | ResetSentenceStart);
						}
						else if	((s[0] == 'e' && s[1] =='l' && s[2] == 's' && s[3] == 'e') ||
								 (s[0] == 't' && s[1] =='h' && s[2] == 'e' && s[3] == 'n')) {
							sc.ChangeState(SCE_4GL_WORD & SetSentenceStart);
						}
						else {
							sc.ChangeState(SCE_4GL_WORD | ResetSentenceStart);
						}
					}
					sc.SetState(SCE_4GL_DEFAULT | (sc.state & 0x10));
				}
				break;
			case SCE_4GL_PREPROCESSOR:
				if (sc.atLineStart) {
					sc.SetState(SCE_4GL_DEFAULT & SetSentenceStart);
				}
				/* code removed to allow comments inside preprocessor
					else if (sc.ch == '*' && sc.chNext == '/') {
					sc.ForwardSetState(SCE_4GL_DEFAULT | sentenceStartState); } */
				break;
			case SCE_4GL_STRING:
				if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_4GL_DEFAULT | mask);
				}
				break;
			case SCE_4GL_CHARACTER:
				if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_4GL_DEFAULT | mask);
				}
				break;
			default:
				if ((sc.state & 0xf) >= SCE_4GL_COMMENT1) {
					if (sc.ch == '*' && sc.chNext == '/') {
						sc.Forward();
						if ((sc.state & 0xf) == SCE_4GL_COMMENT1) {
							sc.ForwardSetState(SCE_4GL_DEFAULT | mask);
						}
						else
							sc.SetState((sc.state & 0x1f) - 1);
					} else if (sc.ch == '/' && sc.chNext == '*') {
						sc.Forward();
						sc.SetState((sc.state & 0x1f) + 1);
					}
				}
		}

		// Determine if a new state should be entered.
		mask = sc.state & 0x10;
		if ((sc.state & 0xf) == SCE_4GL_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_4GL_NUMBER | ResetSentenceStart);
			} else if (IsAWordStart(sc.ch) || (sc.ch == '@')) {
				sc.SetState(SCE_4GL_IDENTIFIER | mask);
			} else if (sc.ch == '/' && sc.chNext == '*') {
				sc.SetState(SCE_4GL_COMMENT1 | mask);
				sc.Forward();
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_4GL_STRING | ResetSentenceStart);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_4GL_CHARACTER | ResetSentenceStart);
			} else if (sc.ch == '&' && visibleChars == 0 && ((sc.state & 0x10) == 0)) {
				sc.SetState(SCE_4GL_PREPROCESSOR | ResetSentenceStart);
				// Skip whitespace between & and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
			// Handle syntactical line termination
			} else if ((sc.ch == '.' || sc.ch == ':' || sc.ch == '}') && (sc.chNext == ' ' || sc.chNext == '\t' || sc.chNext == '\n' || sc.chNext == '\r')) {
				sc.SetState(sc.state & SetSentenceStart);
			} else if (isoperator(static_cast<char>(sc.ch))) {
		/* 	This code allows highlight of handles. Alas, it would cause the phrase "last-event:function"
			to be recognized as a BlockBegin */
			
				if (sc.ch == ':')
					sc.SetState(SCE_4GL_OPERATOR & SetSentenceStart);
				/* else */
					sc.SetState(SCE_4GL_OPERATOR | ResetSentenceStart);
			}
		}

		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return (style & 0xf) >= SCE_4GL_COMMENT1 ;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldNoBox4glDoc(unsigned int startPos, int length, int initStyle,
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = static_cast<char>(tolower(styler[startPos]));
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = static_cast<char>(tolower(styler.SafeGetCharAt(i + 1)));
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext)) { // && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		else if ((style & 0xf) == SCE_4GL_BLOCK && !isalnum(chNext)) {
			levelNext++;
		}
		else if ((style & 0xf) == SCE_4GL_END  && (ch == 'e' || ch == 'f')) {
			levelNext--;
		}
		if (atEOL) {
			int levelUse = levelCurrent;
			if (foldAtElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
}

static void Fold4glDoc(unsigned int startPos, int length, int initStyle, WordList *[],
                       Accessor &styler) {
	FoldNoBox4glDoc(startPos, length, initStyle, styler);
}

static const char * const FglWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Unused",
            "Global classes and typedefs",
            0,
        };

LexerModule lmProgress(SCLEX_PROGRESS, Colourise4glDoc, "progress", Fold4glDoc, FglWordLists);
