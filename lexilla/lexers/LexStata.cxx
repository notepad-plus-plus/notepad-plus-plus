// Scintilla source code edit control
/** @file LexStata.cxx
 ** Lexer for Stata
 **/
// Author: Luke Rasmussen (luke.rasmussen@gmail.com)
//
// The License.txt file describes the conditions under which this software may
// be distributed.
//
// Developed as part of the StatTag project at Northwestern University Feinberg
// School of Medicine with funding from Northwestern University Clinical and
// Translational Sciences Institute through CTSA grant UL1TR001422.  This work
// has not been reviewed or endorsed by NCATS or the NIH.

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

static void ColouriseStataDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
    Accessor &styler) {

    WordList &keywords = *keywordlists[0];
    WordList &types = *keywordlists[1];
    
    CharacterSet setCouldBePostOp(CharacterSet::setNone, "+-");
    CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
    CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);

    StyleContext sc(startPos, length, initStyle, styler);
    bool lineHasNonCommentChar = false;
    for (; sc.More(); sc.Forward()) {
        if (sc.atLineStart) {
          lineHasNonCommentChar = false;
        }

        // Determine if the current state should terminate.
        switch (sc.state) {
            case SCE_STATA_OPERATOR:
                sc.SetState(SCE_STATA_DEFAULT);
                break;
            case SCE_STATA_NUMBER:
                // We accept almost anything because of hex. and number suffixes
                if (!setWord.Contains(sc.ch)) {
                    sc.SetState(SCE_STATA_DEFAULT);
                }
                break;
            case SCE_STATA_IDENTIFIER:
                if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
                    char s[1000];
                    sc.GetCurrent(s, sizeof(s));
                    if (keywords.InList(s)) {
                        sc.ChangeState(SCE_STATA_WORD);
                    }
                    else if (types.InList(s)) {
                        sc.ChangeState(SCE_STATA_TYPE);
                    }
                    sc.SetState(SCE_STATA_DEFAULT);
                }
                break;
            case SCE_STATA_COMMENTBLOCK:
                if (sc.Match('*', '/')) {
                    sc.Forward();
                    sc.ForwardSetState(SCE_STATA_DEFAULT);
                }
                break;
            case SCE_STATA_COMMENT:
            case SCE_STATA_COMMENTLINE:
                if (sc.atLineStart) {
                    sc.SetState(SCE_STATA_DEFAULT);
                }
                break;
            case SCE_STATA_STRING:
                if (sc.ch == '\\') {
                    // Per Stata documentation, the following characters are the only ones that can
                    // be escaped (not our typical set of quotes, etc.):
                    // https://www.stata.com/support/faqs/programming/backslashes-and-macros/
                    if (sc.chNext == '$' || sc.chNext == '`' || sc.chNext == '\\') {
                        sc.Forward();
                    }
                }
                else if (sc.ch == '\"') {
                    sc.ForwardSetState(SCE_STATA_DEFAULT);
                }
                break;
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_STATA_DEFAULT) {
            if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_STATA_NUMBER);
            }
            else if (setWordStart.Contains(sc.ch)) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_STATA_IDENTIFIER);
            }
            else if (sc.Match('*') && !lineHasNonCommentChar) {
                sc.SetState(SCE_STATA_COMMENT);
            }
            else if (sc.Match('/', '*')) {
                sc.SetState(SCE_STATA_COMMENTBLOCK);
                sc.Forward();	// Eat the * so it isn't used for the end of the comment
            }
            else if (sc.Match('/', '/')) {
                sc.SetState(SCE_STATA_COMMENTLINE);
            }
            else if (sc.ch == '\"') {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_STATA_STRING);
            }
            else if (isoperator(sc.ch)) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_STATA_OPERATOR);
            }
        }
    }

    sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldStataDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[],
    Accessor &styler) {
    bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
    bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
    Sci_PositionU endPos = startPos + length;
    int visibleChars = 0;
    Sci_Position lineCurrent = styler.GetLine(startPos);
    int levelCurrent = SC_FOLDLEVELBASE;
    if (lineCurrent > 0)
        levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
    int levelMinCurrent = levelCurrent;
    int levelNext = levelCurrent;
    char chNext = styler[startPos];
    int styleNext = styler.StyleAt(startPos);
    for (Sci_PositionU i = startPos; i < endPos; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        int style = styleNext;
        styleNext = styler.StyleAt(i + 1);
        bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
        if (style == SCE_R_OPERATOR) {
            if (ch == '{') {
                // Measure the minimum before a '{' to allow
                // folding on "} else {"
                if (levelMinCurrent > levelNext) {
                    levelMinCurrent = levelNext;
                }
                levelNext++;
            }
            else if (ch == '}') {
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


static const char * const StataWordLists[] = {
    "Language Keywords",
    "Types",
    0,
};

extern const LexerModule lmStata(SCLEX_STATA, ColouriseStataDoc, "stata", FoldStataDoc, StataWordLists);
