// Scintilla source code edit control
/**
 * @file LexMagik.cxx
 * Lexer for GE(r) Smallworld(tm) MagikSF
 */
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

/**
 * Is it a core character (C isalpha(), exclamation and question mark)
 *
 * \param  ch The character
 * \return True if ch is a character, False otherwise
 */
static inline bool IsAlphaCore(int ch) {
    return (isalpha(ch) || ch == '!' || ch == '?');
}

/**
 * Is it a character (IsAlphaCore() and underscore)
 *
 * \param  ch The character
 * \return True if ch is a character, False otherwise
 */
static inline bool IsAlpha(int ch) {
    return (IsAlphaCore(ch) || ch == '_');
}

/**
 * Is it a symbolic character (IsAlpha() and colon)
 *
 * \param  ch The character
 * \return True if ch is a character, False otherwise
 */
static inline bool IsAlphaSym(int ch) {
    return (IsAlpha(ch) || ch == ':');
}

/**
 * Is it a numerical character (IsAlpha() and 0 - 9)
 *
 * \param  ch The character
 * \return True if ch is a character, False otherwise
 */
static inline bool IsAlNum(int ch) {
    return ((ch >= '0' && ch <= '9') || IsAlpha(ch));
}

/**
 * Is it a symbolic numerical character (IsAlNum() and colon)
 *
 * \param  ch The character
 * \return True if ch is a character, False otherwise
 */
static inline bool IsAlNumSym(int ch) {
    return (IsAlNum(ch) || ch == ':');
}

/**
 * The lexer function
 *
 * \param  startPos Where to start scanning
 * \param  length Where to scan to
 * \param  initStyle The style at the initial point, not used in this folder
 * \param  keywordlists The keywordslists, currently, number 5 is used
 * \param  styler The styler
 */
static void ColouriseMagikDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {
    styler.StartAt(startPos);

    WordList &keywords = *keywordlists[0];
    WordList &pragmatics = *keywordlists[1];
    WordList &containers = *keywordlists[2];
    WordList &flow = *keywordlists[3];
    WordList &characters = *keywordlists[4];

	StyleContext sc(startPos, length, initStyle, styler);


	for (; sc.More(); sc.Forward()) {

    repeat:

        if(sc.ch == '#') {
            if (sc.chNext == '#') sc.SetState(SCE_MAGIK_HYPER_COMMENT);
            else sc.SetState(SCE_MAGIK_COMMENT);
            for(; sc.More() && !(sc.atLineEnd); sc.Forward());
            sc.SetState(SCE_MAGIK_DEFAULT);
            goto repeat;
        }

        if(sc.ch == '"') {
            sc.SetState(SCE_MAGIK_STRING);

            if(sc.More())
            {
                sc.Forward();
                for(; sc.More() && sc.ch != '"'; sc.Forward());
            }

            sc.ForwardSetState(SCE_MAGIK_DEFAULT);
            goto repeat;
        }

	    // The default state
	    if(sc.state == SCE_MAGIK_DEFAULT) {

	        // A certain keyword has been detected
	        if (sc.ch == '_' && (
                    sc.currentPos == 0 || !IsAlNum(sc.chPrev))) {
	            char keyword[50];
	            memset(keyword, '\0', 50);

	            for(
                    int scanPosition = 0;
                    scanPosition < 50;
                    scanPosition++) {
	                char keywordChar = static_cast<char>(
                        tolower(styler.SafeGetCharAt(
                            scanPosition +
                                static_cast<Sci_Position>(sc.currentPos+1), ' ')));
                    if(IsAlpha(keywordChar)) {
                        keyword[scanPosition] = keywordChar;
                    } else {
                        break;
                    }
	            }

                // It is a pragma
	            if(pragmatics.InList(keyword)) {
	                sc.SetState(SCE_MAGIK_PRAGMA);
	            }

	            // it is a normal keyword like _local, _self, etc.
	            else if(keywords.InList(keyword)) {
	                sc.SetState(SCE_MAGIK_KEYWORD);
	            }

                // It is a container keyword, such as _method, _proc, etc.
	            else if(containers.InList(keyword)) {
	                sc.SetState(SCE_MAGIK_CONTAINER);
	            }

	            // It is a flow keyword, such as _for, _if, _try, etc.
	            else if(flow.InList(keyword)) {
	                sc.SetState(SCE_MAGIK_FLOW);
	            }

	            // Interpret as unknown keyword
	            else {
	                sc.SetState(SCE_MAGIK_UNKNOWN_KEYWORD);
	            }
	        }

            // Symbolic expression
	        else if(sc.ch == ':' && !IsAlNum(sc.chPrev)) {
	            sc.SetState(SCE_MAGIK_SYMBOL);
	            bool firstTrip = true;
	            for(sc.Forward(); sc.More(); sc.Forward()) {
	                if(firstTrip && IsAlphaSym(sc.ch));
	                else if(!firstTrip && IsAlNumSym(sc.ch));
	                else if(sc.ch == '|') {
	                    for(sc.Forward();
                            sc.More() && sc.ch != '|';
                            sc.Forward());
	                }
	                else break;

	                firstTrip = false;
	            }
	            sc.SetState(SCE_MAGIK_DEFAULT);
	            goto repeat;
	        }

            // Identifier (label) expression
	        else if(sc.ch == '@') {
	            sc.SetState(SCE_MAGIK_IDENTIFIER);
	            bool firstTrip = true;
	            for(sc.Forward(); sc.More(); sc.Forward()) {
	                if(firstTrip && IsAlphaCore(sc.ch)) {
	                    firstTrip = false;
	                }
	                else if(!firstTrip && IsAlpha(sc.ch));
	                else break;
	            }
	            sc.SetState(SCE_MAGIK_DEFAULT);
	            goto repeat;
	        }

	        // Start of a character
            else if(sc.ch == '%') {
                sc.SetState(SCE_MAGIK_CHARACTER);
                sc.Forward();
                char keyword[50];
	            memset(keyword, '\0', 50);

	            for(
                    int scanPosition = 0;
                    scanPosition < 50;
                    scanPosition++) {
	                char keywordChar = static_cast<char>(
                        tolower(styler.SafeGetCharAt(
                            scanPosition +
                                static_cast<int>(sc.currentPos), ' ')));
                    if(IsAlpha(keywordChar)) {
                        keyword[scanPosition] = keywordChar;
                    } else {
                        break;
                    }
	            }

	            if(characters.InList(keyword)) {
	                sc.Forward(static_cast<int>(strlen(keyword)));
	            } else {
	                sc.Forward();
	            }

                sc.SetState(SCE_MAGIK_DEFAULT);
                goto repeat;
            }

            // Operators
	        else if(
                sc.ch == '>' ||
                sc.ch == '<' ||
                sc.ch == '.' ||
                sc.ch == ',' ||
                sc.ch == '+' ||
                sc.ch == '-' ||
                sc.ch == '/' ||
                sc.ch == '*' ||
                sc.ch == '~' ||
                sc.ch == '$' ||
                sc.ch == '=') {
                sc.SetState(SCE_MAGIK_OPERATOR);
            }

            // Braces
            else if(sc.ch == '(' || sc.ch == ')') {
                sc.SetState(SCE_MAGIK_BRACE_BLOCK);
            }

            // Brackets
            else if(sc.ch == '{' || sc.ch == '}') {
                sc.SetState(SCE_MAGIK_BRACKET_BLOCK);
            }

            // Square Brackets
            else if(sc.ch == '[' || sc.ch == ']') {
                sc.SetState(SCE_MAGIK_SQBRACKET_BLOCK);
            }


	    }

	    // It is an operator
	    else if(
            sc.state == SCE_MAGIK_OPERATOR ||
            sc.state == SCE_MAGIK_BRACE_BLOCK ||
            sc.state == SCE_MAGIK_BRACKET_BLOCK ||
            sc.state == SCE_MAGIK_SQBRACKET_BLOCK) {
	        sc.SetState(SCE_MAGIK_DEFAULT);
	        goto repeat;
	    }

	    // It is the pragma state
	    else if(sc.state == SCE_MAGIK_PRAGMA) {
	        if(!IsAlpha(sc.ch)) {
	            sc.SetState(SCE_MAGIK_DEFAULT);
                goto repeat;
	        }
	    }

	    // It is the keyword state
	    else if(
            sc.state == SCE_MAGIK_KEYWORD ||
            sc.state == SCE_MAGIK_CONTAINER ||
            sc.state == SCE_MAGIK_FLOW ||
            sc.state == SCE_MAGIK_UNKNOWN_KEYWORD) {
	        if(!IsAlpha(sc.ch)) {
	            sc.SetState(SCE_MAGIK_DEFAULT);
	            goto repeat;
	        }
	    }
	}

	sc.Complete();
}

/**
 * The word list description
 */
static const char * const magikWordListDesc[] = {
    "Accessors (local, global, self, super, thisthread)",
    "Pragmatic (pragma, private)",
    "Containers (method, block, proc)",
    "Flow (if, then, elif, else)",
    "Characters (space, tab, newline, return)",
    "Fold Containers (method, proc, block, if, loop)",
    0};

/**
 * This function detects keywords which are able to have a body. Note that it
 * uses the Fold Containers word description, not the containers description. It
 * only works when the style at that particular position is set on Containers
 * or Flow (number 3 or 4).
 *
 * \param  keywordslist The list of keywords that are scanned, they should only
 *         contain the start keywords, not the end keywords
 * \param  keyword The actual keyword
 * \return 1 if it is a folding start-keyword, -1 if it is a folding end-keyword
 *         0 otherwise
 */
static inline int IsFoldingContainer(WordList &keywordslist, char * keyword) {
    if(
        strlen(keyword) > 3 &&
        keyword[0] == 'e' && keyword[1] == 'n' && keyword[2] == 'd') {
        if (keywordslist.InList(keyword + 3)) {
            return -1;
        }

    } else {
        if(keywordslist.InList(keyword)) {
            return 1;
        }
    }

    return 0;
}

/**
 * The folding function
 *
 * \param  startPos Where to start scanning
 * \param  length Where to scan to
 * \param  keywordslists The keywordslists, currently, number 5 is used
 * \param  styler The styler
 */
static void FoldMagikDoc(Sci_PositionU startPos, Sci_Position length, int,
    WordList *keywordslists[], Accessor &styler) {

    bool compact = styler.GetPropertyInt("fold.compact") != 0;

    WordList &foldingElements = *keywordslists[5];
    Sci_Position endPos = startPos + length;
    Sci_Position line = styler.GetLine(startPos);
    int level = styler.LevelAt(line) & SC_FOLDLEVELNUMBERMASK;
    int flags = styler.LevelAt(line) & ~SC_FOLDLEVELNUMBERMASK;

    for(
        Sci_Position currentPos = startPos;
        currentPos < endPos;
        currentPos++) {
            char currentState = styler.StyleAt(currentPos);
            char c = styler.SafeGetCharAt(currentPos, ' ');
            Sci_Position prevLine = styler.GetLine(currentPos - 1);
            line = styler.GetLine(currentPos);

            // Default situation
            if(prevLine < line) {
                styler.SetLevel(line, (level|flags) & ~SC_FOLDLEVELHEADERFLAG);
                flags = styler.LevelAt(line) & ~SC_FOLDLEVELNUMBERMASK;
            }

            if(
                (
                    currentState == SCE_MAGIK_CONTAINER ||
                    currentState == SCE_MAGIK_FLOW
                ) &&
                c == '_') {

                char keyword[50];
                memset(keyword, '\0', 50);

                for(
                    int scanPosition = 0;
                    scanPosition < 50;
                    scanPosition++) {
                    char keywordChar = static_cast<char>(
                        tolower(styler.SafeGetCharAt(
                            scanPosition +
                                currentPos + 1, ' ')));
                    if(IsAlpha(keywordChar)) {
                        keyword[scanPosition] = keywordChar;
                    } else {
                        break;
                    }
                }

                if(IsFoldingContainer(foldingElements, keyword) > 0) {
                    styler.SetLevel(
                        line,
                        styler.LevelAt(line) | SC_FOLDLEVELHEADERFLAG);
                    level++;
                } else if(IsFoldingContainer(foldingElements, keyword) < 0) {
                    styler.SetLevel(line, styler.LevelAt(line));
                    level--;
                }
            }

            if(
                compact && (
                    currentState == SCE_MAGIK_BRACE_BLOCK ||
                    currentState == SCE_MAGIK_BRACKET_BLOCK ||
                    currentState == SCE_MAGIK_SQBRACKET_BLOCK)) {
                if(c == '{' || c == '[' || c == '(') {
                    styler.SetLevel(
                        line,
                        styler.LevelAt(line) | SC_FOLDLEVELHEADERFLAG);
                    level++;
                } else if(c == '}' || c == ']' || c == ')') {
                    styler.SetLevel(line, styler.LevelAt(line));
                    level--;
                }
            }
        }

}

/**
 * Injecting the module
 */
LexerModule lmMagikSF(
    SCLEX_MAGIK, ColouriseMagikDoc, "magiksf", FoldMagikDoc, magikWordListDesc);

