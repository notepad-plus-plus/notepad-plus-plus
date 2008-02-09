// Scintilla source code edit control
/** @file LexPS.cxx
 ** Lexer for PostScript
 **
 ** Written by Nigel Hathaway <nigel@bprj.co.uk>.
 ** The License.txt file describes the conditions under which this software may be distributed.
 **/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

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

static inline bool IsASelfDelimitingChar(const int ch) {
    return (ch == '[' || ch == ']' || ch == '{' || ch == '}' ||
            ch == '/' || ch == '<' || ch == '>' ||
            ch == '(' || ch == ')' || ch == '%');
}

static inline bool IsAWhitespaceChar(const int ch) {
    return (ch == ' '  || ch == '\t' || ch == '\r' ||
            ch == '\n' || ch == '\f' || ch == '\0');
}

static bool IsABaseNDigit(const int ch, const int base) {
    int maxdig = '9';
    int letterext = -1;

    if (base <= 10)
        maxdig = '0' + base - 1;
    else
        letterext = base - 11;

    return ((ch >= '0' && ch <= maxdig) ||
            (ch >= 'A' && ch <= ('A' + letterext)) ||
            (ch >= 'a' && ch <= ('a' + letterext)));
}

static inline bool IsABase85Char(const int ch) {
    return ((ch >= '!' && ch <= 'u') || ch == 'z');
}

static void ColourisePSDoc(
    unsigned int startPos,
    int length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler) {

    WordList &keywords1 = *keywordlists[0];
    WordList &keywords2 = *keywordlists[1];
    WordList &keywords3 = *keywordlists[2];
    WordList &keywords4 = *keywordlists[3];
    WordList &keywords5 = *keywordlists[4];

    StyleContext sc(startPos, length, initStyle, styler);

    bool tokenizing = styler.GetPropertyInt("ps.tokenize") != 0;
    int pslevel = styler.GetPropertyInt("ps.level", 3);
    int lineCurrent = styler.GetLine(startPos);
    int nestTextCurrent = 0;
    if (lineCurrent > 0 && initStyle == SCE_PS_TEXT)
        nestTextCurrent = styler.GetLineState(lineCurrent - 1);
    int numRadix = 0;
    bool numHasPoint = false;
    bool numHasExponent = false;
    bool numHasSign = false;

    // Clear out existing tokenization
    if (tokenizing && length > 0) {
        styler.StartAt(startPos, static_cast<char>(INDIC2_MASK));
        styler.ColourTo(startPos + length-1, 0);
        styler.Flush();
        styler.StartAt(startPos);
        styler.StartSegment(startPos);
    }

    for (; sc.More(); sc.Forward()) {
        if (sc.atLineStart)
            lineCurrent = styler.GetLine(sc.currentPos);

        // Determine if the current state should terminate.
        if (sc.state == SCE_PS_COMMENT || sc.state == SCE_PS_DSC_VALUE) {
            if (sc.atLineEnd) {
                sc.SetState(SCE_C_DEFAULT);
            }
        } else if (sc.state == SCE_PS_DSC_COMMENT) {
            if (sc.ch == ':') {
                sc.Forward();
                if (!sc.atLineEnd)
                    sc.SetState(SCE_PS_DSC_VALUE);
                else
                    sc.SetState(SCE_C_DEFAULT);
            } else if (sc.atLineEnd) {
                sc.SetState(SCE_C_DEFAULT);
            } else if (IsAWhitespaceChar(sc.ch) && sc.ch != '\r') {
                sc.ChangeState(SCE_PS_COMMENT);
            }
        } else if (sc.state == SCE_PS_NUMBER) {
            if (IsASelfDelimitingChar(sc.ch) || IsAWhitespaceChar(sc.ch)) {
                if ((sc.chPrev == '+' || sc.chPrev == '-' ||
                     sc.chPrev == 'E' || sc.chPrev == 'e') && numRadix == 0)
                    sc.ChangeState(SCE_PS_NAME);
                sc.SetState(SCE_C_DEFAULT);
            } else if (sc.ch == '#') {
                if (numHasPoint || numHasExponent || numHasSign || numRadix != 0) {
                    sc.ChangeState(SCE_PS_NAME);
                } else {
                    char szradix[5];
                    sc.GetCurrent(szradix, 4);
                    numRadix = atoi(szradix);
                    if (numRadix < 2 || numRadix > 36)
                        sc.ChangeState(SCE_PS_NAME);
                }
            } else if ((sc.ch == 'E' || sc.ch == 'e') && numRadix == 0) {
                if (numHasExponent) {
                    sc.ChangeState(SCE_PS_NAME);
                } else {
                    numHasExponent = true;
                    if (sc.chNext == '+' || sc.chNext == '-')
                        sc.Forward();
                }
            } else if (sc.ch == '.') {
                if (numHasPoint || numHasExponent || numRadix != 0) {
                    sc.ChangeState(SCE_PS_NAME);
                } else {
                    numHasPoint = true;
                }
            } else if (numRadix == 0) {
                if (!IsABaseNDigit(sc.ch, 10))
                    sc.ChangeState(SCE_PS_NAME);
            } else {
                if (!IsABaseNDigit(sc.ch, numRadix))
                    sc.ChangeState(SCE_PS_NAME);
            }
        } else if (sc.state == SCE_PS_NAME || sc.state == SCE_PS_KEYWORD) {
            if (IsASelfDelimitingChar(sc.ch) || IsAWhitespaceChar(sc.ch)) {
                char s[100];
                sc.GetCurrent(s, sizeof(s));
                if ((pslevel >= 1 && keywords1.InList(s)) ||
                    (pslevel >= 2 && keywords2.InList(s)) ||
                    (pslevel >= 3 && keywords3.InList(s)) ||
                    keywords4.InList(s) || keywords5.InList(s)) {
                    sc.ChangeState(SCE_PS_KEYWORD);
                }
                sc.SetState(SCE_C_DEFAULT);
            }
        } else if (sc.state == SCE_PS_LITERAL || sc.state == SCE_PS_IMMEVAL) {
            if (IsASelfDelimitingChar(sc.ch) || IsAWhitespaceChar(sc.ch))
                sc.SetState(SCE_C_DEFAULT);
        } else if (sc.state == SCE_PS_PAREN_ARRAY || sc.state == SCE_PS_PAREN_DICT ||
                   sc.state == SCE_PS_PAREN_PROC) {
            sc.SetState(SCE_C_DEFAULT);
        } else if (sc.state == SCE_PS_TEXT) {
            if (sc.ch == '(') {
                nestTextCurrent++;
            } else if (sc.ch == ')') {
                if (--nestTextCurrent == 0)
                   sc.ForwardSetState(SCE_PS_DEFAULT);
            } else if (sc.ch == '\\') {
                sc.Forward();
            }
        } else if (sc.state == SCE_PS_HEXSTRING) {
            if (sc.ch == '>') {
                sc.ForwardSetState(SCE_PS_DEFAULT);
            } else if (!IsABaseNDigit(sc.ch, 16) && !IsAWhitespaceChar(sc.ch)) {
                sc.SetState(SCE_PS_HEXSTRING);
                styler.ColourTo(sc.currentPos, SCE_PS_BADSTRINGCHAR);
            }
        } else if (sc.state == SCE_PS_BASE85STRING) {
            if (sc.Match('~', '>')) {
                sc.Forward();
                sc.ForwardSetState(SCE_PS_DEFAULT);
            } else if (!IsABase85Char(sc.ch) && !IsAWhitespaceChar(sc.ch)) {
                sc.SetState(SCE_PS_BASE85STRING);
                styler.ColourTo(sc.currentPos, SCE_PS_BADSTRINGCHAR);
            }
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_C_DEFAULT) {
            unsigned int tokenpos = sc.currentPos;

            if (sc.ch == '[' || sc.ch == ']') {
                sc.SetState(SCE_PS_PAREN_ARRAY);
            } else if (sc.ch == '{' || sc.ch == '}') {
                sc.SetState(SCE_PS_PAREN_PROC);
            } else if (sc.ch == '/') {
                if (sc.chNext == '/') {
                    sc.SetState(SCE_PS_IMMEVAL);
                    sc.Forward();
                } else {
                    sc.SetState(SCE_PS_LITERAL);
                }
            } else if (sc.ch == '<') {
                if (sc.chNext == '<') {
                    sc.SetState(SCE_PS_PAREN_DICT);
                    sc.Forward();
                } else if (sc.chNext == '~') {
                    sc.SetState(SCE_PS_BASE85STRING);
                    sc.Forward();
                } else {
                    sc.SetState(SCE_PS_HEXSTRING);
                }
            } else if (sc.ch == '>' && sc.chNext == '>') {
                    sc.SetState(SCE_PS_PAREN_DICT);
                    sc.Forward();
            } else if (sc.ch == '>' || sc.ch == ')') {
                sc.SetState(SCE_C_DEFAULT);
                styler.ColourTo(sc.currentPos, SCE_PS_BADSTRINGCHAR);
            } else if (sc.ch == '(') {
                sc.SetState(SCE_PS_TEXT);
                nestTextCurrent = 1;
            } else if (sc.ch == '%') {
                if (sc.chNext == '%' && sc.atLineStart) {
                    sc.SetState(SCE_PS_DSC_COMMENT);
                    sc.Forward();
                    if (sc.chNext == '+') {
                        sc.Forward();
                        sc.ForwardSetState(SCE_PS_DSC_VALUE);
                    }
                } else {
                    sc.SetState(SCE_PS_COMMENT);
                }
            } else if ((sc.ch == '+' || sc.ch == '-' || sc.ch == '.') &&
                       IsABaseNDigit(sc.chNext, 10)) {
                sc.SetState(SCE_PS_NUMBER);
                numRadix = 0;
                numHasPoint = (sc.ch == '.');
                numHasExponent = false;
                numHasSign = (sc.ch == '+' || sc.ch == '-');
            } else if ((sc.ch == '+' || sc.ch == '-') && sc.chNext == '.' &&
                       IsABaseNDigit(sc.GetRelative(2), 10)) {
                sc.SetState(SCE_PS_NUMBER);
                numRadix = 0;
                numHasPoint = false;
                numHasExponent = false;
                numHasSign = true;
            } else if (IsABaseNDigit(sc.ch, 10)) {
                sc.SetState(SCE_PS_NUMBER);
                numRadix = 0;
                numHasPoint = false;
                numHasExponent = false;
                numHasSign = false;
            } else if (!IsAWhitespaceChar(sc.ch)) {
                sc.SetState(SCE_PS_NAME);
            }

            // Mark the start of tokens
            if (tokenizing && sc.state != SCE_C_DEFAULT && sc.state != SCE_PS_COMMENT &&
                sc.state != SCE_PS_DSC_COMMENT && sc.state != SCE_PS_DSC_VALUE) {
                styler.Flush();
                styler.StartAt(tokenpos, static_cast<char>(INDIC2_MASK));
                styler.ColourTo(tokenpos, INDIC2_MASK);
                styler.Flush();
                styler.StartAt(tokenpos);
                styler.StartSegment(tokenpos);
            }
        }

        if (sc.atLineEnd)
            styler.SetLineState(lineCurrent, nestTextCurrent);
    }

    sc.Complete();
}

static void FoldPSDoc(unsigned int startPos, int length, int, WordList *[],
                       Accessor &styler) {
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
    char chNext = styler[startPos];
    int styleNext = styler.StyleAt(startPos);
    int style;
    for (unsigned int i = startPos; i < endPos; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        style = styleNext;
        styleNext = styler.StyleAt(i + 1);
        bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');  //mac??
        if ((style & 31) == SCE_PS_PAREN_PROC) {
            if (ch == '{') {
                // Measure the minimum before a '{' to allow
                // folding on "} {"
                if (levelMinCurrent > levelNext) {
                    levelMinCurrent = levelNext;
                }
                levelNext++;
            } else if (ch == '}') {
                levelNext--;
            }
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

static const char * const psWordListDesc[] = {
    "PS Level 1 operators",
    "PS Level 2 operators",
    "PS Level 3 operators",
    "RIP-specific operators",
    "User-defined operators",
    0
};

LexerModule lmPS(SCLEX_PS, ColourisePSDoc, "ps", FoldPSDoc, psWordListDesc);
