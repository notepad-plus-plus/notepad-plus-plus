// Scintilla source code edit control
/** @file LexKVIrc.cxx
 ** Lexer for KVIrc script.
 **/
// Copyright 2013 by OmegaPhil <OmegaPhil+scintilla@gmail.com>, based in
// part from LexPython Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// and LexCmake Copyright 2007 by Cristian Adam <cristian [dot] adam [at] gmx [dot] net>

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


/* KVIrc Script syntactic rules: http://www.kvirc.net/doc/doc_syntactic_rules.html */

/* Utility functions */
static inline bool IsAWordChar(int ch) {

    /* Keyword list includes modules, i.e. words including '.', and
     * alias namespaces include ':' */
    return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.'
            || ch == ':');
}
static inline bool IsAWordStart(int ch) {

    /* Functions (start with '$') are treated separately to keywords */
    return (ch < 0x80) && (isalnum(ch) || ch == '_' );
}

/* Interface function called by Scintilla to request some text to be
 syntax highlighted */
static void ColouriseKVIrcDoc(Sci_PositionU startPos, Sci_Position length,
                              int initStyle, WordList *keywordlists[],
                              Accessor &styler)
{
    /* Fetching style context */
    StyleContext sc(startPos, length, initStyle, styler);

    /* Accessing keywords and function-marking keywords */
    WordList &keywords = *keywordlists[0];
    WordList &functionKeywords = *keywordlists[1];

    /* Looping for all characters - only automatically moving forward
     * when asked for (transitions leaving strings and keywords do this
     * already) */
    bool next = true;
    for( ; sc.More(); next ? sc.Forward() : (void)0 )
    {
        /* Resetting next */
        next = true;

        /* Dealing with different states */
        switch (sc.state)
        {
            case SCE_KVIRC_DEFAULT:

                /* Detecting single-line comments
                 * Unfortunately KVIrc script allows raw '#<channel
                 * name>' to be used, and appending # to an array returns
                 * its length...
                 * Going for a compromise where single line comments not
                 * starting on a newline are allowed in all cases except
                 * when they are preceeded with an opening bracket or comma
                 * (this will probably be the most common style a valid
                 * string-less channel name will be used with), with the
                 * array length case included
                 */
                if (
                    (sc.ch == '#' && sc.atLineStart) ||
                    (sc.ch == '#' && (
                        sc.chPrev != '(' && sc.chPrev != ',' &&
                        sc.chPrev != ']')
                    )
                )
                {
                    sc.SetState(SCE_KVIRC_COMMENT);
                    break;
                }

                /* Detecting multi-line comments */
                if (sc.Match('/', '*'))
                {
                    sc.SetState(SCE_KVIRC_COMMENTBLOCK);
                    break;
                }

                /* Detecting strings */
                if (sc.ch == '"')
                {
                    sc.SetState(SCE_KVIRC_STRING);
                    break;
                }

                /* Detecting functions */
                if (sc.ch == '$')
                {
                    sc.SetState(SCE_KVIRC_FUNCTION);
                    break;
                }

                /* Detecting variables */
                if (sc.ch == '%')
                {
                    sc.SetState(SCE_KVIRC_VARIABLE);
                    break;
                }

                /* Detecting numbers - isdigit is unsafe as it does not
                 * validate, use CharacterSet.h functions */
                if (IsADigit(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_NUMBER);
                    break;
                }

                /* Detecting words */
                if (IsAWordStart(sc.ch) && IsAWordChar(sc.chNext))
                {
                    sc.SetState(SCE_KVIRC_WORD);
                    sc.Forward();
                    break;
                }

                /* Detecting operators */
                if (isoperator(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_OPERATOR);
                    break;
                }

                break;

            case SCE_KVIRC_COMMENT:

                /* Breaking out of single line comment when a newline
                 * is introduced */
                if (sc.ch == '\r' || sc.ch == '\n')
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);
                    break;
                }

                break;

            case SCE_KVIRC_COMMENTBLOCK:

                /* Detecting end of multi-line comment */
                if (sc.Match('*', '/'))
                {
                    // Moving the current position forward two characters
                    // so that '*/' is included in the comment
                    sc.Forward(2);
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Comment has been exited and the current position
                     * moved forward, yet the new current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = false;
                    break;
                }

                break;

            case SCE_KVIRC_STRING:

                /* Detecting end of string - closing speechmarks */
                if (sc.ch == '"')
                {
                    /* Allowing escaped speechmarks to pass */
                    if (sc.chPrev == '\\')
                        break;

                    /* Moving the current position forward to capture the
                     * terminating speechmarks, and ending string */
                    sc.ForwardSetState(SCE_KVIRC_DEFAULT);

                    /* String has been exited and the current position
                     * moved forward, yet the new current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = false;
                    break;
                }

                /* Functions and variables are now highlighted in strings
                 * Detecting functions */
                if (sc.ch == '$')
                {
                    /* Allowing escaped functions to pass */
                    if (sc.chPrev == '\\')
                        break;

                    sc.SetState(SCE_KVIRC_STRING_FUNCTION);
                    break;
                }

                /* Detecting variables */
                if (sc.ch == '%')
                {
                    /* Allowing escaped variables to pass */
                    if (sc.chPrev == '\\')
                        break;

                    sc.SetState(SCE_KVIRC_STRING_VARIABLE);
                    break;
                }

                /* Breaking out of a string when a newline is introduced */
                if (sc.ch == '\r' || sc.ch == '\n')
                {
                    /* Allowing escaped newlines */
                    if (sc.chPrev == '\\')
                        break;

                    sc.SetState(SCE_KVIRC_DEFAULT);
                    break;
                }

                break;

            case SCE_KVIRC_FUNCTION:
            case SCE_KVIRC_VARIABLE:

                /* Detecting the end of a function/variable (word) */
                if (!IsAWordChar(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Word has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = false;
                    break;
                }

                break;

            case SCE_KVIRC_STRING_FUNCTION:
            case SCE_KVIRC_STRING_VARIABLE:

                /* A function or variable in a string
                 * Detecting the end of a function/variable (word) */
                if (!IsAWordChar(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_STRING);

                    /* Word has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = false;
                    break;
                }

                break;

            case SCE_KVIRC_NUMBER:

                /* Detecting the end of a number */
                if (!IsADigit(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Number has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward */
                    next = false;
                    break;
                }

                break;

            case SCE_KVIRC_OPERATOR:

                /* Because '%' is an operator but is also the marker for
                 * a variable, I need to always treat operators as single
                 * character strings and therefore redo their detection
                 * after every character */
                sc.SetState(SCE_KVIRC_DEFAULT);

                /* Operator has been exited yet the current character
                 * has yet to be defined - loop without moving
                 * forward */
                next = false;
                break;

            case SCE_KVIRC_WORD:

                /* Detecting the end of a word */
                if (!IsAWordChar(sc.ch))
                {
                    /* Checking if the word was actually a keyword -
                     * fetching the current word, NULL-terminated like
                     * the keyword list */
                    char s[100];
                    Sci_Position wordLen = sc.currentPos - styler.GetStartSegment();
                    if (wordLen > 99)
                        wordLen = 99;  /* Include '\0' in buffer */
                    Sci_Position i;
                    for( i = 0; i < wordLen; ++i )
                    {
                        s[i] = styler.SafeGetCharAt( styler.GetStartSegment() + i );
                    }
                    s[wordLen] = '\0';

                    /* Actually detecting keywords and fixing the state */
                    if (keywords.InList(s))
                    {
                        /* The SetState call actually commits the
                         * previous keyword state */
                        sc.ChangeState(SCE_KVIRC_KEYWORD);
                    }
                    else if (functionKeywords.InList(s))
                    {
                        // Detecting function keywords and fixing the state
                        sc.ChangeState(SCE_KVIRC_FUNCTION_KEYWORD);
                    }

                    /* Transitioning to default and committing the previous
                     * word state */
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Word has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = false;
                    break;
                }

                break;
        }
    }

    /* Indicating processing is complete */
    sc.Complete();
}

static void FoldKVIrcDoc(Sci_PositionU startPos, Sci_Position length, int /*initStyle - unused*/,
                      WordList *[], Accessor &styler)
{
    /* Based on CMake's folder */

    /* Exiting if folding isnt enabled */
    if ( styler.GetPropertyInt("fold") == 0 )
        return;

    /* Obtaining current line number*/
    Sci_Position currentLine = styler.GetLine(startPos);

    /* Obtaining starting character - indentation is done on a line basis,
     * not character */
    Sci_PositionU safeStartPos = styler.LineStart( currentLine );

    /* Initialising current level - this is defined as indentation level
     * in the low 12 bits, with flag bits in the upper four bits.
     * It looks like two indentation states are maintained in the returned
     * 32bit value - 'nextLevel' in the most-significant bits, 'currentLevel'
     * in the least-significant bits. Since the next level is the most
     * up to date, this must refer to the current state of indentation.
     * So the code bitshifts the old current level out of existence to
     * get at the actual current state of indentation
     * Based on the LexerCPP.cxx line 958 comment */
    int currentLevel = SC_FOLDLEVELBASE;
    if (currentLine > 0)
        currentLevel = styler.LevelAt(currentLine - 1) >> 16;
    int nextLevel = currentLevel;

    // Looping for characters in range
    for (Sci_PositionU i = safeStartPos; i < startPos + length; ++i)
    {
        /* Folding occurs after syntax highlighting, meaning Scintilla
         * already knows where the comments are
         * Fetching the current state */
        int state = styler.StyleAt(i) & 31;

        switch( styler.SafeGetCharAt(i) )
        {
            case '{':

                /* Indenting only when the braces are not contained in
                 * a comment */
                if (state != SCE_KVIRC_COMMENT &&
                    state != SCE_KVIRC_COMMENTBLOCK)
                    ++nextLevel;
                break;

            case '}':

                /* Outdenting only when the braces are not contained in
                 * a comment */
                if (state != SCE_KVIRC_COMMENT &&
                    state != SCE_KVIRC_COMMENTBLOCK)
                    --nextLevel;
                break;

            case '\n':
            case '\r':

                /* Preparing indentation information to return - combining
                 * current and next level data */
                int lev = currentLevel | nextLevel << 16;

                /* If the next level increases the indent level, mark the
                 * current line as a fold point - current level data is
                 * in the least significant bits */
                if (nextLevel > currentLevel )
                    lev |= SC_FOLDLEVELHEADERFLAG;

                /* Updating indentation level if needed */
                if (lev != styler.LevelAt(currentLine))
                    styler.SetLevel(currentLine, lev);

                /* Updating variables */
                ++currentLine;
                currentLevel = nextLevel;

                /* Dealing with problematic Windows newlines -
                 * incrementing to avoid the extra newline breaking the
                 * fold point */
                if (styler.SafeGetCharAt(i) == '\r' &&
                    styler.SafeGetCharAt(i + 1) == '\n')
                    ++i;
                break;
        }
    }

    /* At this point the data has ended, so presumably the end of the line?
     * Preparing indentation information to return - combining current
     * and next level data */
    int lev = currentLevel | nextLevel << 16;

    /* If the next level increases the indent level, mark the current
     * line as a fold point - current level data is in the least
     * significant bits */
    if (nextLevel > currentLevel )
        lev |= SC_FOLDLEVELHEADERFLAG;

    /* Updating indentation level if needed */
    if (lev != styler.LevelAt(currentLine))
        styler.SetLevel(currentLine, lev);
}

/* Registering wordlists */
static const char *const kvircWordListDesc[] = {
	"primary",
	"function_keywords",
	0
};


/* Registering functions and wordlists */
extern const LexerModule lmKVIrc(SCLEX_KVIRC, ColouriseKVIrcDoc, "kvirc", FoldKVIrcDoc,
                    kvircWordListDesc);
