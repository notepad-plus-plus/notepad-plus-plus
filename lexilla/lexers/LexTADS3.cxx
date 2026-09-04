// Scintilla source code edit control
/** @file LexTADS3.cxx
 ** Lexer for TADS3.
 **/
// Copyright 1998-2006 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/*
 * TADS3 is a language designed by Michael J. Roberts for the writing of text
 * based games.  TADS comes from Text Adventure Development System.  It has good
 * support for the processing and outputting of formatted text and much of a
 * TADS program listing consists of strings.
 *
 * TADS has two types of strings, those enclosed in single quotes (') and those
 * enclosed in double quotes (").  These strings have different symantics and
 * can be given different highlighting if desired.
 *
 * There can be embedded within both types of strings html tags
 * ( <tag key=value> ), library directives ( <.directive> ), and message
 * parameters ( {The doctor's/his} ).
 *
 * Double quoted strings can also contain interpolated expressions
 * ( << rug.moved ? ' and a hole in the floor. ' : nil >> ).  These expressions
 * may themselves contain single or double quoted strings, although the double
 * quoted strings may not contain interpolated expressions.
 *
 * These embedded constructs influence the output and formatting and are an
 * important part of a program and require highlighting.
 *
 * LINKS
 * http://www.tads.org/
 */

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

static const int T3_SINGLE_QUOTE = 1;
static const int T3_INT_EXPRESSION = 2;
static const int T3_INT_EXPRESSION_IN_TAG = 4;
static const int T3_HTML_SQUOTE = 8;

static inline bool IsEOL(const int ch, const int chNext) {
        return (ch == '\r' && chNext != '\n') || (ch == '\n');
}

/*
 *   Test the current character to see if it's the START of an EOL sequence;
 *   if so, skip ahead to the last character of the sequence and return true,
 *   and if not just return false.  There are a few places where we want to
 *   check to see if a newline sequence occurs at a particular point, but
 *   where a caller expects a subroutine to stop only upon reaching the END
 *   of a newline sequence (in particular, CR-LF on Windows).  That's why
 *   IsEOL() above only returns true on CR if the CR isn't followed by an LF
 *   - it doesn't want to admit that there's a newline until reaching the END
 *   of the sequence.  We meet both needs by saying that there's a newline
 *   when we see the CR in a CR-LF, but skipping the CR before returning so
 *   that the caller's caller will see that we've stopped at the LF.
 */
static inline bool IsEOLSkip(StyleContext &sc)
{
    /* test for CR-LF */
    if (sc.ch == '\r' && sc.chNext == '\n')
    {
        /* got CR-LF - skip the CR and indicate that we're at a newline */
        sc.Forward();
        return true;
    }

    /*
     *   in other cases, we have at most a 1-character newline, so do the
     *   normal IsEOL test
     */
    return IsEOL(sc.ch, sc.chNext);
}

static inline bool IsATADS3Operator(const int ch) {
        return ch == '=' || ch == '{' || ch == '}' || ch == '(' || ch == ')'
                || ch == '[' || ch == ']' || ch == ',' || ch == ':' || ch == ';'
                || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%'
                || ch == '?' || ch == '!' || ch == '<' || ch == '>' || ch == '|'
                || ch == '@' || ch == '&' || ch == '~';
}

static inline bool IsAWordChar(const int ch) {
        return isalnum(ch) || ch == '_';
}

static inline bool IsAWordStart(const int ch) {
        return isalpha(ch) || ch == '_';
}

static inline bool IsAHexDigit(const int ch) {
        int lch = tolower(ch);
        return isdigit(lch) || lch == 'a' || lch == 'b' || lch == 'c'
                || lch == 'd' || lch == 'e' || lch == 'f';
}

static inline bool IsAnHTMLChar(int ch) {
        return isalnum(ch) || ch == '-' || ch == '_' || ch == '.';
}

static inline bool IsADirectiveChar(int ch) {
        return isalnum(ch) || isspace(ch) || ch == '-' || ch == '/';
}

static inline bool IsANumberStart(StyleContext &sc) {
        return isdigit(sc.ch)
                || (!isdigit(sc.chPrev) && sc.ch == '.' && isdigit(sc.chNext));
}

inline static void ColouriseTADS3Operator(StyleContext &sc) {
        int initState = sc.state;
        int c = sc.ch;
        sc.SetState(c == '{' || c == '}' ? SCE_T3_BRACE : SCE_T3_OPERATOR);
        sc.ForwardSetState(initState);
}

static void ColouriseTADSHTMLString(StyleContext &sc, int &lineState) {
        int endState = sc.state;
        int chQuote = sc.ch;
        int chString = (lineState & T3_SINGLE_QUOTE) ? '\'' : '"';
        if (endState == SCE_T3_HTML_STRING) {
                if (lineState&T3_SINGLE_QUOTE) {
                        endState = SCE_T3_S_STRING;
                        chString = '\'';
                } else if (lineState&T3_INT_EXPRESSION) {
                        endState = SCE_T3_X_STRING;
                        chString = '"';
                } else {
                        endState = SCE_T3_HTML_DEFAULT;
                        chString = '"';
                }
                chQuote = (lineState & T3_HTML_SQUOTE) ? '\'' : '"';
        } else {
                sc.SetState(SCE_T3_HTML_STRING);
                sc.Forward();
        }
        if (chQuote == '"')
                lineState &= ~T3_HTML_SQUOTE;
        else
                lineState |= T3_HTML_SQUOTE;

        while (sc.More()) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                if (sc.ch == chQuote) {
                        sc.ForwardSetState(endState);
                        return;
                }
                if (sc.Match('\\', static_cast<char>(chQuote))) {
                        sc.Forward(2);
                        sc.SetState(endState);
                        return;
                }
                if (sc.ch == chString) {
                        sc.SetState(SCE_T3_DEFAULT);
                        return;
                }

                if (sc.Match('<', '<')) {
                        lineState |= T3_INT_EXPRESSION | T3_INT_EXPRESSION_IN_TAG;
                        sc.SetState(SCE_T3_X_DEFAULT);
                        sc.Forward(2);
                        return;
                }

                if (sc.Match('\\', static_cast<char>(chQuote))
                        || sc.Match('\\', static_cast<char>(chString))
                        || sc.Match('\\', '\\')) {
                        sc.Forward(2);
                } else {
                        sc.Forward();
                }
        }
}

static void ColouriseTADS3HTMLTagStart(StyleContext &sc) {
        sc.SetState(SCE_T3_HTML_TAG);
        sc.Forward();
        if (sc.ch == '/') {
                sc.Forward();
        }
        while (IsAnHTMLChar(sc.ch)) {
                sc.Forward();
        }
}

static void ColouriseTADS3HTMLTag(StyleContext &sc, int &lineState) {
        int endState = sc.state;
        int chQuote = '"';
        int chString = '\'';
        switch (endState) {
                case SCE_T3_S_STRING:
                        ColouriseTADS3HTMLTagStart(sc);
                        sc.SetState(SCE_T3_HTML_DEFAULT);
                        chQuote = '\'';
                        chString = '"';
                        break;
                case SCE_T3_D_STRING:
                case SCE_T3_X_STRING:
                        ColouriseTADS3HTMLTagStart(sc);
                        sc.SetState(SCE_T3_HTML_DEFAULT);
                        break;
                case SCE_T3_HTML_DEFAULT:
                        if (lineState&T3_SINGLE_QUOTE) {
                                endState = SCE_T3_S_STRING;
                                chQuote = '\'';
                                chString = '"';
                        } else if (lineState&T3_INT_EXPRESSION) {
                                endState = SCE_T3_X_STRING;
                        } else {
                                endState = SCE_T3_D_STRING;
                        }
                        break;
        }

        while (sc.More()) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                if (sc.Match('/', '>')) {
                        sc.SetState(SCE_T3_HTML_TAG);
                        sc.Forward(2);
                        sc.SetState(endState);
                        return;
                }
                if (sc.ch == '>') {
                        sc.SetState(SCE_T3_HTML_TAG);
                        sc.ForwardSetState(endState);
                        return;
                }
                if (sc.ch == chQuote) {
                        sc.SetState(endState);
                        return;
                }
                if (sc.Match('\\', static_cast<char>(chQuote))) {
                        sc.Forward();
                        ColouriseTADSHTMLString(sc, lineState);
                        if (sc.state == SCE_T3_X_DEFAULT)
                            break;
                } else if (sc.ch == chString) {
                        ColouriseTADSHTMLString(sc, lineState);
                } else if (sc.ch == '=') {
                        ColouriseTADS3Operator(sc);
                } else {
                        sc.Forward();
                }
        }
}

static void ColouriseTADS3Keyword(StyleContext &sc,
                                                        WordList *keywordlists[],       Sci_PositionU endPos) {
        char s[250];
        WordList &keywords = *keywordlists[0];
        WordList &userwords1 = *keywordlists[1];
        WordList &userwords2 = *keywordlists[2];
        WordList &userwords3 = *keywordlists[3];
        int initState = sc.state;
        sc.SetState(SCE_T3_IDENTIFIER);
        while (sc.More() && (IsAWordChar(sc.ch))) {
                sc.Forward();
        }
        sc.GetCurrent(s, sizeof(s));
        if ( strcmp(s, "is") == 0 || strcmp(s, "not") == 0) {
                // have to find if "in" is next
                Sci_Position n = 1;
                while (n + sc.currentPos < endPos && IsASpaceOrTab(sc.GetRelative(n)))
                        n++;
                if (sc.GetRelative(n) == 'i' && sc.GetRelative(n+1) == 'n') {
                        sc.Forward(n+2);
                        sc.ChangeState(SCE_T3_KEYWORD);
                }
        } else if (keywords.InList(s)) {
                sc.ChangeState(SCE_T3_KEYWORD);
        } else if (userwords3.InList(s)) {
                sc.ChangeState(SCE_T3_USER3);
        } else if (userwords2.InList(s)) {
                sc.ChangeState(SCE_T3_USER2);
        } else if (userwords1.InList(s)) {
                sc.ChangeState(SCE_T3_USER1);
        }
        sc.SetState(initState);
}

static void ColouriseTADS3MsgParam(StyleContext &sc, int &lineState) {
        int endState = sc.state;
        int chQuote = '"';
        switch (endState) {
                case SCE_T3_S_STRING:
                        sc.SetState(SCE_T3_MSG_PARAM);
                        sc.Forward();
                        chQuote = '\'';
                        break;
                case SCE_T3_D_STRING:
                case SCE_T3_X_STRING:
                        sc.SetState(SCE_T3_MSG_PARAM);
                        sc.Forward();
                        break;
                case SCE_T3_MSG_PARAM:
                        if (lineState&T3_SINGLE_QUOTE) {
                                endState = SCE_T3_S_STRING;
                                chQuote = '\'';
                        } else if (lineState&T3_INT_EXPRESSION) {
                                endState = SCE_T3_X_STRING;
                        } else {
                                endState = SCE_T3_D_STRING;
                        }
                        break;
        }
        while (sc.More() && sc.ch != '}' && sc.ch != chQuote) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                if (sc.ch == '\\') {
                        sc.Forward();
                }
                sc.Forward();
        }
        if (sc.ch == chQuote) {
                sc.SetState(endState);
        } else {
                sc.ForwardSetState(endState);
        }
}

static void ColouriseTADS3LibDirective(StyleContext &sc, int &lineState) {
        int initState = sc.state;
        int chQuote = '"';
        switch (initState) {
                case SCE_T3_S_STRING:
                        sc.SetState(SCE_T3_LIB_DIRECTIVE);
                        sc.Forward(2);
                        chQuote = '\'';
                        break;
                case SCE_T3_D_STRING:
                        sc.SetState(SCE_T3_LIB_DIRECTIVE);
                        sc.Forward(2);
                        break;
                case SCE_T3_LIB_DIRECTIVE:
                        if (lineState&T3_SINGLE_QUOTE) {
                                initState = SCE_T3_S_STRING;
                                chQuote = '\'';
                        } else {
                                initState = SCE_T3_D_STRING;
                        }
                        break;
        }
        while (sc.More() && IsADirectiveChar(sc.ch)) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                sc.Forward();
        };
        if (sc.ch == '>' || !sc.More()) {
                sc.ForwardSetState(initState);
        } else if (sc.ch == chQuote) {
                sc.SetState(initState);
        } else {
                sc.ChangeState(initState);
                sc.Forward();
        }
}

static void ColouriseTADS3String(StyleContext &sc, int &lineState) {
        int chQuote = sc.ch;
        int endState = sc.state;
        switch (sc.state) {
                case SCE_T3_DEFAULT:
                case SCE_T3_X_DEFAULT:
                        if (chQuote == '"') {
                                if (sc.state == SCE_T3_DEFAULT) {
                                        sc.SetState(SCE_T3_D_STRING);
                                } else {
                                        sc.SetState(SCE_T3_X_STRING);
                                }
                                lineState &= ~T3_SINGLE_QUOTE;
                        } else {
                                sc.SetState(SCE_T3_S_STRING);
                                lineState |= T3_SINGLE_QUOTE;
                        }
                        sc.Forward();
                        break;
                case SCE_T3_S_STRING:
                        chQuote = '\'';
                        endState = lineState&T3_INT_EXPRESSION ?
                                SCE_T3_X_DEFAULT : SCE_T3_DEFAULT;
                        break;
                case SCE_T3_D_STRING:
                        chQuote = '"';
                        endState = SCE_T3_DEFAULT;
                        break;
                case SCE_T3_X_STRING:
                        chQuote = '"';
                        endState = SCE_T3_X_DEFAULT;
                        break;
        }
        while (sc.More()) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                if (sc.ch == chQuote) {
                        sc.ForwardSetState(endState);
                        return;
                }
                if (sc.state == SCE_T3_D_STRING && sc.Match('<', '<')) {
                        lineState |= T3_INT_EXPRESSION;
                        sc.SetState(SCE_T3_X_DEFAULT);
                        sc.Forward(2);
                        return;
                }
                if (sc.Match('\\', static_cast<char>(chQuote))
                    || sc.Match('\\', '\\')) {
                        sc.Forward(2);
                } else if (sc.ch == '{') {
                        ColouriseTADS3MsgParam(sc, lineState);
                } else if (sc.Match('<', '.')) {
                        ColouriseTADS3LibDirective(sc, lineState);
                } else if (sc.ch == '<') {
                        ColouriseTADS3HTMLTag(sc, lineState);
                        if (sc.state == SCE_T3_X_DEFAULT)
                                return;
                } else {
                        sc.Forward();
                }
        }
}

static void ColouriseTADS3Comment(StyleContext &sc, int endState) {
        sc.SetState(SCE_T3_BLOCK_COMMENT);
        while (sc.More()) {
                if (IsEOL(sc.ch, sc.chNext)) {
                        return;
                }
                if (sc.Match('*', '/')) {
                        sc.Forward(2);
                        sc.SetState(endState);
                        return;
                }
                sc.Forward();
        }
}

static void ColouriseToEndOfLine(StyleContext &sc, int initState, int endState) {
        sc.SetState(initState);
        while (sc.More()) {
                if (sc.ch == '\\') {
                        sc.Forward();
                        if (IsEOLSkip(sc)) {
                                        return;
                        }
                }
                if (IsEOL(sc.ch, sc.chNext)) {
                        sc.SetState(endState);
                        return;
                }
                sc.Forward();
        }
}

static void ColouriseTADS3Number(StyleContext &sc) {
        int endState = sc.state;
        bool inHexNumber = false;
        bool seenE = false;
        bool seenDot = sc.ch == '.';
        sc.SetState(SCE_T3_NUMBER);
        if (sc.More()) {
                sc.Forward();
        }
        if (sc.chPrev == '0' && tolower(sc.ch) == 'x') {
                inHexNumber = true;
                sc.Forward();
        }
        while (sc.More()) {
                if (inHexNumber) {
                        if (!IsAHexDigit(sc.ch)) {
                                break;
                        }
                } else if (!isdigit(sc.ch)) {
                        if (!seenE && tolower(sc.ch) == 'e') {
                                seenE = true;
                                seenDot = true;
                                if (sc.chNext == '+' || sc.chNext == '-') {
                                        sc.Forward();
                                }
                        } else if (!seenDot && sc.ch == '.') {
                                seenDot = true;
                        } else {
                                break;
                        }
                }
                sc.Forward();
        }
        sc.SetState(endState);
}

static void ColouriseTADS3Doc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                                                           WordList *keywordlists[], Accessor &styler) {
        int visibleChars = 0;
        int bracketLevel = 0;
        int lineState = 0;
        Sci_PositionU endPos = startPos + length;
        Sci_Position lineCurrent = styler.GetLine(startPos);
        if (lineCurrent > 0) {
                lineState = styler.GetLineState(lineCurrent-1);
        }
        StyleContext sc(startPos, length, initStyle, styler);

        while (sc.More()) {

                if (IsEOL(sc.ch, sc.chNext)) {
                        styler.SetLineState(lineCurrent, lineState);
                        lineCurrent++;
                        visibleChars = 0;
                        sc.Forward();
                        if (sc.ch == '\n') {
                                sc.Forward();
                        }
                }

                switch(sc.state) {
                        case SCE_T3_PREPROCESSOR:
                        case SCE_T3_LINE_COMMENT:
                                ColouriseToEndOfLine(sc, sc.state, lineState&T3_INT_EXPRESSION ?
                                        SCE_T3_X_DEFAULT : SCE_T3_DEFAULT);
                                break;
                        case SCE_T3_S_STRING:
                        case SCE_T3_D_STRING:
                        case SCE_T3_X_STRING:
                                ColouriseTADS3String(sc, lineState);
                                visibleChars++;
                                break;
                        case SCE_T3_MSG_PARAM:
                                ColouriseTADS3MsgParam(sc, lineState);
                                break;
                        case SCE_T3_LIB_DIRECTIVE:
                                ColouriseTADS3LibDirective(sc, lineState);
                                break;
                        case SCE_T3_HTML_DEFAULT:
                                ColouriseTADS3HTMLTag(sc, lineState);
                                break;
                        case SCE_T3_HTML_STRING:
                                ColouriseTADSHTMLString(sc, lineState);
                                break;
                        case SCE_T3_BLOCK_COMMENT:
                                ColouriseTADS3Comment(sc, lineState&T3_INT_EXPRESSION ?
                                        SCE_T3_X_DEFAULT : SCE_T3_DEFAULT);
                                break;
                        case SCE_T3_DEFAULT:
                        case SCE_T3_X_DEFAULT:
                                if (IsASpaceOrTab(sc.ch)) {
                                        sc.Forward();
                                } else if (sc.ch == '#' && visibleChars == 0) {
                                        ColouriseToEndOfLine(sc, SCE_T3_PREPROCESSOR, sc.state);
                                } else if (sc.Match('/', '*')) {
                                        ColouriseTADS3Comment(sc, sc.state);
                                        visibleChars++;
                                } else if (sc.Match('/', '/')) {
                                        ColouriseToEndOfLine(sc, SCE_T3_LINE_COMMENT, sc.state);
                                } else if (sc.ch == '"') {
                                        bracketLevel = 0;
                                        ColouriseTADS3String(sc, lineState);
                                        visibleChars++;
                                } else if (sc.ch == '\'') {
                                        ColouriseTADS3String(sc, lineState);
                                        visibleChars++;
                                } else if (sc.state == SCE_T3_X_DEFAULT && bracketLevel == 0
                                                   && sc.Match('>', '>')) {
                                        sc.Forward(2);
                                        sc.SetState(SCE_T3_D_STRING);
                                        if (lineState & T3_INT_EXPRESSION_IN_TAG)
                                                sc.SetState(SCE_T3_HTML_STRING);
                                        lineState &= ~(T3_SINGLE_QUOTE|T3_INT_EXPRESSION
                                                       |T3_INT_EXPRESSION_IN_TAG);
                                } else if (IsATADS3Operator(sc.ch)) {
                                        if (sc.state == SCE_T3_X_DEFAULT) {
                                                if (sc.ch == '(') {
                                                        bracketLevel++;
                                                } else if (sc.ch == ')' && bracketLevel > 0) {
                                                        bracketLevel--;
                                                }
                                        }
                                        ColouriseTADS3Operator(sc);
                                        visibleChars++;
                                } else if (IsANumberStart(sc)) {
                                        ColouriseTADS3Number(sc);
                                        visibleChars++;
                                } else if (IsAWordStart(sc.ch)) {
                                        ColouriseTADS3Keyword(sc, keywordlists, endPos);
                                        visibleChars++;
                                } else if (sc.Match("...")) {
                                        sc.SetState(SCE_T3_IDENTIFIER);
                                        sc.Forward(3);
                                        sc.SetState(SCE_T3_DEFAULT);
                                } else {
                                        sc.Forward();
                                        visibleChars++;
                                }
                                break;
                        default:
                                sc.SetState(SCE_T3_DEFAULT);
                                sc.Forward();
                }
        }
        sc.Complete();
}

/*
 TADS3 has two styles of top level block (TLB). Eg

 // default style
 silverKey : Key 'small silver key' 'small silver key'
        "A small key glints in the sunlight. "
 ;

 and

 silverKey : Key {
        'small silver key'
        'small silver key'
        "A small key glints in the sunlight. "
 }

 Some constructs mandate one or the other, but usually the author has may choose
 either.

 T3_SEENSTART is used to indicate that a braceless TLB has been (potentially)
 seen and is also used to match the closing ';' of the default style.

 T3_EXPECTINGIDENTIFIER and T3_EXPECTINGPUNCTUATION are used to keep track of
 what characters may be seen without incrementing the block level.  The general
 pattern is identifier <punc> identifier, acceptable punctuation characters
 are ':', ',', '(' and ')'.  No attempt is made to ensure that punctuation
 characters are syntactically correct, eg parentheses match. A ')' always
 signifies the start of a block.  We just need to check if it is followed by a
 '{', in which case we let the brace handling code handle the folding level.

 expectingIdentifier == false && expectingIdentifier == false
 Before the start of a TLB.

 expectingIdentifier == true && expectingIdentifier == true
 Currently in an identifier.  Will accept identifier or punctuation.

 expectingIdentifier == true && expectingIdentifier == false
 Just seen a punctuation character & now waiting for an identifier to start.

 expectingIdentifier == false && expectingIdentifier == truee
 We were in an identifier and have seen space.  Now waiting to see a punctuation
 character

 Space, comments & preprocessor directives are always acceptable and are
 equivalent.
*/

static const int T3_SEENSTART = 1 << 12;
static const int T3_EXPECTINGIDENTIFIER = 1 << 13;
static const int T3_EXPECTINGPUNCTUATION = 1 << 14;

static inline bool IsStringTransition(int s1, int s2) {
        return s1 != s2
                && (s1 == SCE_T3_S_STRING || s1 == SCE_T3_X_STRING
                        || (s1 == SCE_T3_D_STRING && s2 != SCE_T3_X_DEFAULT))
                && s2 != SCE_T3_LIB_DIRECTIVE
                && s2 != SCE_T3_MSG_PARAM
                && s2 != SCE_T3_HTML_TAG
                && s2 != SCE_T3_HTML_STRING;
}

static inline bool IsATADS3Punctuation(const int ch) {
        return ch == ':' || ch == ',' || ch == '(' || ch == ')';
}

static inline bool IsAnIdentifier(const int style) {
        return style == SCE_T3_IDENTIFIER
                || style == SCE_T3_USER1
                || style == SCE_T3_USER2
                || style == SCE_T3_USER3;
}

static inline bool IsAnOperator(const int style) {
    return style == SCE_T3_OPERATOR || style == SCE_T3_BRACE;
}

static inline bool IsSpaceEquivalent(const int ch, const int style) {
        return isspace(ch)
                || style == SCE_T3_BLOCK_COMMENT
                || style == SCE_T3_LINE_COMMENT
                || style == SCE_T3_PREPROCESSOR;
}

static char peekAhead(Sci_PositionU startPos, Sci_PositionU endPos,
                                          Accessor &styler) {
        for (Sci_PositionU i = startPos; i < endPos; i++) {
                int style = styler.StyleAt(i);
                char ch = styler[i];
                if (!IsSpaceEquivalent(ch, style)) {
                        if (IsAnIdentifier(style)) {
                                return 'a';
                        }
                        if (IsATADS3Punctuation(ch)) {
                                return ':';
                        }
                        if (ch == '{') {
                                return '{';
                        }
                        return '*';
                }
        }
        return ' ';
}

static void FoldTADS3Doc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                            WordList *[], Accessor &styler) {
        Sci_PositionU endPos = startPos + length;
        Sci_Position lineCurrent = styler.GetLine(startPos);
        int levelCurrent = SC_FOLDLEVELBASE;
        if (lineCurrent > 0)
                levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
        int seenStart = levelCurrent & T3_SEENSTART;
        int expectingIdentifier = levelCurrent & T3_EXPECTINGIDENTIFIER;
        int expectingPunctuation = levelCurrent & T3_EXPECTINGPUNCTUATION;
        levelCurrent &= SC_FOLDLEVELNUMBERMASK;
        int levelMinCurrent = levelCurrent;
        int levelNext = levelCurrent;
        char chNext = styler[startPos];
        int styleNext = styler.StyleAt(startPos);
        int style = initStyle;
        char ch = chNext;
        int stylePrev = style;
        bool redo = false;
        for (Sci_PositionU i = startPos; i < endPos; i++) {
                if (redo) {
                        redo = false;
                        i--;
                } else {
                        ch = chNext;
                        chNext = styler.SafeGetCharAt(i + 1);
                        stylePrev = style;
                        style = styleNext;
                        styleNext = styler.StyleAt(i + 1);
                }
                bool atEOL = IsEOL(ch, chNext);

                if (levelNext == SC_FOLDLEVELBASE) {
                        if (IsSpaceEquivalent(ch, style)) {
                                if (expectingPunctuation) {
                                        expectingIdentifier = 0;
                                }
                                if (style == SCE_T3_BLOCK_COMMENT) {
                                        levelNext++;
                                }
                        } else if (ch == '{') {
                                levelNext++;
                                seenStart = 0;
                        } else if (ch == '\'' || ch == '"' || ch == '[') {
                                levelNext++;
                                if (seenStart) {
                                        redo = true;
                                }
                        } else if (ch == ';') {
                                seenStart = 0;
                                expectingIdentifier = 0;
                                expectingPunctuation = 0;
                        } else if (expectingIdentifier && expectingPunctuation) {
                                if (IsATADS3Punctuation(ch)) {
                                        if (ch == ')' && peekAhead(i+1, endPos, styler) != '{') {
                                                levelNext++;
                                        } else {
                                                expectingPunctuation = 0;
                                        }
                                } else if (!IsAnIdentifier(style)) {
                                        levelNext++;
                                }
                        } else if (expectingIdentifier && !expectingPunctuation) {
                                if (!IsAnIdentifier(style)) {
                                        levelNext++;
                                } else {
                                        expectingPunctuation = T3_EXPECTINGPUNCTUATION;
                                }
                        } else if (!expectingIdentifier && expectingPunctuation) {
                                if (!IsATADS3Punctuation(ch)) {
                                        levelNext++;
                                } else {
                                        if (ch == ')' && peekAhead(i+1, endPos, styler) != '{') {
                                                levelNext++;
                                        } else {
                                                expectingIdentifier = T3_EXPECTINGIDENTIFIER;
                                                expectingPunctuation = 0;
                                        }
                                }
                        } else if (!expectingIdentifier && !expectingPunctuation) {
                                if (IsAnIdentifier(style)) {
                                        seenStart = T3_SEENSTART;
                                        expectingIdentifier = T3_EXPECTINGIDENTIFIER;
                                        expectingPunctuation = T3_EXPECTINGPUNCTUATION;
                                }
                        }

                        if (levelNext != SC_FOLDLEVELBASE && style != SCE_T3_BLOCK_COMMENT) {
                                expectingIdentifier = 0;
                                expectingPunctuation = 0;
                        }

                } else if (levelNext == SC_FOLDLEVELBASE+1 && seenStart
                                   && ch == ';' && IsAnOperator(style)) {
                        levelNext--;
                        seenStart = 0;
                } else if (style == SCE_T3_BLOCK_COMMENT) {
                        if (stylePrev != SCE_T3_BLOCK_COMMENT) {
                                levelNext++;
                        } else if (styleNext != SCE_T3_BLOCK_COMMENT && !atEOL) {
                                // Comments don't end at end of line and the next character may be unstyled.
                                levelNext--;
                        }
                } else if (ch == '\'' || ch == '"') {
                        if (IsStringTransition(style, stylePrev)) {
                                if (levelMinCurrent > levelNext) {
                                        levelMinCurrent = levelNext;
                                }
                                levelNext++;
                        } else if (IsStringTransition(style, styleNext)) {
                                levelNext--;
                        }
                } else if (IsAnOperator(style)) {
                        if (ch == '{' || ch == '[') {
                                // Measure the minimum before a '{' to allow
                                // folding on "} else {"
                                if (levelMinCurrent > levelNext) {
                                        levelMinCurrent = levelNext;
                                }
                                levelNext++;
                        } else if (ch == '}' || ch == ']') {
                                levelNext--;
                        }
                }

                if (atEOL) {
                        if (seenStart && levelNext == SC_FOLDLEVELBASE) {
                                switch (peekAhead(i+1, endPos, styler)) {
                                        case ' ':
                                        case '{':
                                                break;
                                        case '*':
                                                levelNext++;
                                                break;
                                        case 'a':
                                                if (expectingPunctuation) {
                                                        levelNext++;
                                                }
                                                break;
                                        case ':':
                                                if (expectingIdentifier) {
                                                        levelNext++;
                                                }
                                                break;
                                }
                                if (levelNext != SC_FOLDLEVELBASE) {
                                        expectingIdentifier = 0;
                                        expectingPunctuation = 0;
                                }
                        }
                        int lev = levelMinCurrent | (levelNext | expectingIdentifier
                                | expectingPunctuation | seenStart) << 16;
                        if (levelMinCurrent < levelNext)
                                lev |= SC_FOLDLEVELHEADERFLAG;
                        if (lev != styler.LevelAt(lineCurrent)) {
                                styler.SetLevel(lineCurrent, lev);
                        }
                        lineCurrent++;
                        levelCurrent = levelNext;
                        levelMinCurrent = levelCurrent;
                }
        }
}

static const char * const tads3WordList[] = {
        "TADS3 Keywords",
        "User defined 1",
        "User defined 2",
        "User defined 3",
        0
};

extern const LexerModule lmTADS3(SCLEX_TADS3, ColouriseTADS3Doc, "tads3", FoldTADS3Doc, tads3WordList);
