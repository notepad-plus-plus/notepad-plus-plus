// Scintilla source code edit control
/** @file LexSAS.cxx
 ** Lexer for SAS
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

static void ColouriseSASDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
    Accessor &styler) {

    WordList &keywords = *keywordlists[0];
    WordList &blockKeywords = *keywordlists[1];
    WordList &functionKeywords = *keywordlists[2];
    WordList &statements = *keywordlists[3];

    CharacterSet setCouldBePostOp(CharacterSet::setNone, "+-");
    CharacterSet setMacroStart(CharacterSet::setNone, "%");
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
            case SCE_SAS_OPERATOR:
                sc.SetState(SCE_SAS_DEFAULT);
                break;
            case SCE_SAS_NUMBER:
                // We accept almost anything because of hex. and number suffixes
                if (!setWord.Contains(sc.ch)) {
                    sc.SetState(SCE_SAS_DEFAULT);
                }
                break;
            case SCE_SAS_MACRO:
              if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
                char s[1000];
                sc.GetCurrentLowered(s, sizeof(s));
                if (keywords.InList(s)) {
                  sc.ChangeState(SCE_SAS_MACRO_KEYWORD);
                }
                else if (blockKeywords.InList(s)) {
                  sc.ChangeState(SCE_SAS_BLOCK_KEYWORD);
                }
                else if (functionKeywords.InList(s)) {
                  sc.ChangeState(SCE_SAS_MACRO_FUNCTION);
                }
                sc.SetState(SCE_SAS_DEFAULT);
              }
              break;
            case SCE_SAS_IDENTIFIER:
                if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
                    char s[1000];
                    sc.GetCurrentLowered(s, sizeof(s));
                    if (statements.InList(s)) {
                      sc.ChangeState(SCE_SAS_STATEMENT);
                    }
                    else if(blockKeywords.InList(s)) {
                      sc.ChangeState(SCE_SAS_BLOCK_KEYWORD);
                    }
                    sc.SetState(SCE_SAS_DEFAULT);
                }
                break;
            case SCE_SAS_COMMENTBLOCK:
                if (sc.Match('*', '/')) {
                    sc.Forward();
                    sc.ForwardSetState(SCE_SAS_DEFAULT);
                }
                break;
            case SCE_SAS_COMMENT:
            case SCE_SAS_COMMENTLINE:
                if (sc.Match(';')) {
                    sc.Forward();
                    sc.SetState(SCE_SAS_DEFAULT);
                }
                break;
            case SCE_SAS_STRING:
                if (sc.ch == '\"') {
                    sc.ForwardSetState(SCE_SAS_DEFAULT);
                }
                break;
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_SAS_DEFAULT) {
            if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_SAS_NUMBER);
            }
            else if (setWordStart.Contains(sc.ch)) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_SAS_IDENTIFIER);
            }
            else if (sc.Match('*') && !lineHasNonCommentChar) {
                sc.SetState(SCE_SAS_COMMENT);
            }
            else if (sc.Match('/', '*')) {
                sc.SetState(SCE_SAS_COMMENTBLOCK);
                sc.Forward();	// Eat the * so it isn't used for the end of the comment
            }
            else if (sc.Match('/', '/')) {
                sc.SetState(SCE_SAS_COMMENTLINE);
            }
            else if (sc.ch == '\"') {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_SAS_STRING);
            }
            else if (setMacroStart.Contains(sc.ch)) {
              lineHasNonCommentChar = true;
              sc.SetState(SCE_SAS_MACRO);
            }
            else if (isoperator(static_cast<char>(sc.ch))) {
                lineHasNonCommentChar = true;
                sc.SetState(SCE_SAS_OPERATOR);
            }
        }
    }

    sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldSASDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[],
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


static const char * const SASWordLists[] = {
    "Language Keywords",
	  "Macro Keywords",
    "Types",
    0,
};

LexerModule lmSAS(SCLEX_SAS, ColouriseSASDoc, "sas", FoldSASDoc, SASWordLists);
