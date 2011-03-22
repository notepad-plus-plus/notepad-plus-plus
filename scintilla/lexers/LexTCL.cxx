// Scintilla source code edit control
/** @file LexTCL.cxx
 ** Lexer for TCL language.
 **/
// Copyright 1998-2001 by Andre Arpin <arpin@kingston.net>
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

// Extended to accept accented characters
static inline bool IsAWordChar(int ch) {
	return ch >= 0x80 ||
        (isalnum(ch) || ch == '_' || ch ==':' || ch=='.'); // : name space separator
}

static inline bool IsAWordStart(int ch) {
	return ch >= 0x80 || (ch ==':' || isalpha(ch) || ch == '_');
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	       (IsADigit(ch, 0x10) || toupper(ch) == 'E' ||
	        ch == '.' || ch == '-' || ch == '+');
}

static void ColouriseTCLDoc(unsigned int startPos, int length, int , WordList *keywordlists[], Accessor &styler) {
#define  isComment(s) (s==SCE_TCL_COMMENT || s==SCE_TCL_COMMENTLINE || s==SCE_TCL_COMMENT_BOX || s==SCE_TCL_BLOCK_COMMENT)
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool commentLevel = false;
    bool subBrace = false; // substitution begin with a brace ${.....}
	enum tLineState {LS_DEFAULT, LS_OPEN_COMMENT, LS_OPEN_DOUBLE_QUOTE, LS_COMMENT_BOX, LS_MASK_STATE = 0xf,
        LS_COMMAND_EXPECTED = 16, LS_BRACE_ONLY = 32 } lineState = LS_DEFAULT;
	bool prevSlash = false;
	int currentLevel = 0;
    bool expected = 0;
    bool subParen = 0;

	int currentLine = styler.GetLine(startPos);
    if (currentLine > 0)
        currentLine--;
	length += startPos - styler.LineStart(currentLine);
	// make sure lines overlap
	startPos = styler.LineStart(currentLine);

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
    WordList &keywords8 = *keywordlists[7];
    WordList &keywords9 = *keywordlists[8];

	if (currentLine > 0) {
        int ls = styler.GetLineState(currentLine - 1);
		lineState = tLineState(ls & LS_MASK_STATE);
		expected = LS_COMMAND_EXPECTED == tLineState(ls & LS_COMMAND_EXPECTED);
        subBrace = LS_BRACE_ONLY == tLineState(ls & LS_BRACE_ONLY);
		currentLevel = styler.LevelAt(currentLine - 1) >> 17;
		commentLevel = (styler.LevelAt(currentLine - 1) >> 16) & 1;
	} else
		styler.SetLevel(0, SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG);
	bool visibleChars = false;

	int previousLevel = currentLevel;
    StyleContext sc(startPos, length, SCE_TCL_DEFAULT, styler);
	for (; ; sc.Forward()) {
next:
        if (sc.ch=='\r' && sc.chNext == '\n') // only ignore \r on PC process on the mac
            continue;
        bool atEnd = !sc.More();  // make sure we coloured the last word
        if (lineState != LS_DEFAULT) {
            sc.SetState(SCE_TCL_DEFAULT);
            if (lineState == LS_OPEN_COMMENT)
                sc.SetState(SCE_TCL_COMMENTLINE);
            else if (lineState == LS_OPEN_DOUBLE_QUOTE)
                sc.SetState(SCE_TCL_IN_QUOTE);
            else if (lineState == LS_COMMENT_BOX && (sc.ch == '#' || (sc.ch == ' ' && sc.chNext=='#')))
                sc.SetState(SCE_TCL_COMMENT_BOX);
            lineState = LS_DEFAULT;
        }
        if (subBrace) { // ${ overrides every thing even \ except }
            if (sc.ch == '}') {
                subBrace = false;
                sc.SetState(SCE_TCL_OPERATOR);
                sc.ForwardSetState(SCE_TCL_DEFAULT);
                goto next;
            }
            else
                sc.SetState(SCE_TCL_SUB_BRACE);
            if (!sc.atLineEnd)
                continue;
        } else if (sc.state == SCE_TCL_DEFAULT || sc.state ==SCE_TCL_OPERATOR) {
            expected &= isspacechar(static_cast<unsigned char>(sc.ch)) || IsAWordStart(sc.ch) || sc.ch =='#';
        } else if (sc.state == SCE_TCL_SUBSTITUTION) {
            switch(sc.ch) {
            case '(':
                subParen=true;
                sc.SetState(SCE_TCL_OPERATOR);
                sc.ForwardSetState(SCE_TCL_SUBSTITUTION);
                continue;
            case ')':
                sc.SetState(SCE_TCL_OPERATOR);
                subParen=false;
                continue;
            case '$':
                continue;
            case ',':
                sc.SetState(SCE_TCL_OPERATOR);
                if (subParen)
                    sc.ForwardSetState(SCE_TCL_SUBSTITUTION);
                continue;
            default :
                // maybe spaces should be allowed ???
                if (!IsAWordChar(sc.ch)) { // probably the code is wrong
                    sc.SetState(SCE_TCL_DEFAULT);
                    subParen = 0;
                }
                break;
            }
        } else if (isComment(sc.state)) {
        } else if (!IsAWordChar(sc.ch)) {
            if ((sc.state == SCE_TCL_IDENTIFIER && expected) ||  sc.state == SCE_TCL_MODIFIER) {
                char w[100];
                char *s=w;
                sc.GetCurrent(w, sizeof(w));
                if (w[strlen(w)-1]=='\r')
                    w[strlen(w)-1]=0;
                while(*s == ':') // ignore leading : like in ::set a 10
                    ++s;
                bool quote = sc.state == SCE_TCL_IN_QUOTE;
                if (commentLevel  || expected) {
                    if (keywords.InList(s)) {
                        sc.ChangeState(quote ? SCE_TCL_WORD_IN_QUOTE : SCE_TCL_WORD);
                    } else if (keywords2.InList(s)) {
                        sc.ChangeState(quote ? SCE_TCL_WORD_IN_QUOTE : SCE_TCL_WORD2);
                    } else if (keywords3.InList(s)) {
                        sc.ChangeState(quote ? SCE_TCL_WORD_IN_QUOTE : SCE_TCL_WORD3);
                    } else if (keywords4.InList(s)) {
                        sc.ChangeState(quote ? SCE_TCL_WORD_IN_QUOTE : SCE_TCL_WORD4);
                    } else if (sc.GetRelative(-static_cast<int>(strlen(s))-1) == '{' &&
                        keywords5.InList(s) && sc.ch == '}') { // {keyword} exactly no spaces
                            sc.ChangeState(SCE_TCL_EXPAND);
                    }
                    if (keywords6.InList(s)) {
                        sc.ChangeState(SCE_TCL_WORD5);
                    } else if (keywords7.InList(s)) {
                        sc.ChangeState(SCE_TCL_WORD6);
                    } else if (keywords8.InList(s)) {
                        sc.ChangeState(SCE_TCL_WORD7);
                    } else if (keywords9.InList(s)) {
                        sc.ChangeState(SCE_TCL_WORD8);
                    }
                }
                expected = false;
                sc.SetState(quote ? SCE_TCL_IN_QUOTE : SCE_TCL_DEFAULT);
            } else if (sc.state == SCE_TCL_MODIFIER || sc.state == SCE_TCL_IDENTIFIER) {
                sc.SetState(SCE_TCL_DEFAULT);
            }
        }
		if (atEnd)
			break;
        if (sc.atLineEnd) {
            lineState = LS_DEFAULT;
			currentLine = styler.GetLine(sc.currentPos);
			if (foldComment && sc.state!=SCE_TCL_COMMENT && isComment(sc.state)) {
				if (currentLevel == 0) {
					++currentLevel;
					commentLevel = true;
				}
			} else {
				if (visibleChars && commentLevel) {
					--currentLevel;
					--previousLevel;
					commentLevel = false;
				}
			}
			int flag = 0;
			if (!visibleChars)
				flag = SC_FOLDLEVELWHITEFLAG;
			if (currentLevel > previousLevel)
				flag = SC_FOLDLEVELHEADERFLAG;
			styler.SetLevel(currentLine, flag + previousLevel + SC_FOLDLEVELBASE + (currentLevel << 17) + (commentLevel << 16));

			// Update the line state, so it can be seen by next line
			if (sc.state == SCE_TCL_IN_QUOTE)
				lineState = LS_OPEN_DOUBLE_QUOTE;
			else {
			     if (prevSlash) {
				    if (isComment(sc.state))
					    lineState = LS_OPEN_COMMENT;
                } else if (sc.state == SCE_TCL_COMMENT_BOX)
                    lineState = LS_COMMENT_BOX;
			}
            styler.SetLineState(currentLine,
                (subBrace ? LS_BRACE_ONLY : 0) |
                (expected ? LS_COMMAND_EXPECTED : 0)  | lineState);
            if (lineState == LS_COMMENT_BOX)
                sc.ForwardSetState(SCE_TCL_COMMENT_BOX);
            else if (lineState == LS_OPEN_DOUBLE_QUOTE)
                sc.ForwardSetState(SCE_TCL_IN_QUOTE);
            else
                sc.ForwardSetState(SCE_TCL_DEFAULT);
			prevSlash = false;
			previousLevel = currentLevel;
			goto next;
		}

		if (prevSlash) {
            prevSlash = false;
            if (sc.ch == '#' && IsANumberChar(sc.chNext))
                sc.ForwardSetState(SCE_TCL_NUMBER);
            continue;
		}
        prevSlash = sc.ch == '\\';
        if (isComment(sc.state))
            continue;
		if (sc.atLineStart) {
			visibleChars = false;
			if (sc.state!=SCE_TCL_IN_QUOTE && !isComment(sc.state))
            {
				sc.SetState(SCE_TCL_DEFAULT);
                expected = IsAWordStart(sc.ch)|| isspacechar(static_cast<unsigned char>(sc.ch));
            }
		}

		switch (sc.state) {
		case SCE_TCL_NUMBER:
			if (!IsANumberChar(sc.ch))
				sc.SetState(SCE_TCL_DEFAULT);
			break;
		case SCE_TCL_IN_QUOTE:
			if (sc.ch == '"') {
				sc.ForwardSetState(SCE_TCL_DEFAULT);
				visibleChars = true; // necessary if a " is the first and only character on a line
				goto next;
			} else if (sc.ch == '[' || sc.ch == ']' || sc.ch == '$') {
				sc.SetState(SCE_TCL_OPERATOR);
                expected = sc.ch == '[';
                sc.ForwardSetState(SCE_TCL_IN_QUOTE);
				goto next;
			}
            continue;
        case SCE_TCL_OPERATOR:
			sc.SetState(SCE_TCL_DEFAULT);
			break;
		}

		if (sc.ch == '#') {
			if (visibleChars) {
                if (sc.state != SCE_TCL_IN_QUOTE && expected)
					sc.SetState(SCE_TCL_COMMENT);
			} else {
                sc.SetState(SCE_TCL_COMMENTLINE);
                if (sc.chNext == '~')
                    sc.SetState(SCE_TCL_BLOCK_COMMENT);
                if (sc.atLineStart && (sc.chNext == '#' || sc.chNext == '-'))
                        sc.SetState(SCE_TCL_COMMENT_BOX);
            }
        }

		if (!isspacechar(static_cast<unsigned char>(sc.ch))) {
			visibleChars = true;
		}

		if (sc.ch == '\\') {
			prevSlash = true;
			continue;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_TCL_DEFAULT) {
            if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_TCL_IDENTIFIER);
			} else if (IsADigit(sc.ch) && !IsAWordChar(sc.chPrev)) {
				sc.SetState(SCE_TCL_NUMBER);
			} else {
				switch (sc.ch) {
				case '\"':
					sc.SetState(SCE_TCL_IN_QUOTE);
					break;
				case '{':
					sc.SetState(SCE_TCL_OPERATOR);
					expected = true;
					++currentLevel;
					break;
				case '}':
					sc.SetState(SCE_TCL_OPERATOR);
					--currentLevel;
					break;
				case '[':
                    expected = true;
				case ']':
				case '(':
				case ')':
					sc.SetState(SCE_TCL_OPERATOR);
					break;
				case ';':
                    expected = true;
					break;
                case '$':
                    subParen = 0;
                    if (sc.chNext != '{') {
                        sc.SetState(SCE_TCL_SUBSTITUTION);
                    }
                    else {
                        sc.SetState(SCE_TCL_OPERATOR);  // $
                        sc.Forward();  // {
                        sc.ForwardSetState(SCE_TCL_SUB_BRACE);
                        subBrace = true;
                    }
                    break;
                case '#':
                    if ((isspacechar(static_cast<unsigned char>(sc.chPrev))||
                            isoperator(static_cast<char>(sc.chPrev))) && IsADigit(sc.chNext,0x10))
                        sc.SetState(SCE_TCL_NUMBER);
                    break;
                case '-':
                    sc.SetState(IsADigit(sc.chNext)? SCE_TCL_NUMBER: SCE_TCL_MODIFIER);
                    break;
                default:
                    if (isoperator(static_cast<char>(sc.ch))) {
                        sc.SetState(SCE_TCL_OPERATOR);
                    }
				}
			}
		}
	}
	sc.Complete();
}

static const char * const tclWordListDesc[] = {
            "TCL Keywords",
            "TK Keywords",
            "iTCL Keywords",
            "tkCommands",
            "expand"
            "user1",
            "user2",
            "user3",
            "user4",
            0
        };

// this code supports folding in the colourizer
LexerModule lmTCL(SCLEX_TCL, ColouriseTCLDoc, "tcl", 0, tclWordListDesc);
