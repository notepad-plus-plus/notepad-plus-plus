// Scintilla source code edit control
/** @file LexVisualProlog.cxx
** Lexer for Visual Prolog.
**/
// Author Thomas Linder Puls, Prolog Development Denter A/S, http://www.visual-prolog.com
// Based on Lexer for C++, C, Java, and JavaScript.
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// The line state contains:
// In SCE_VISUALPROLOG_STRING_VERBATIM_EOL (i.e. multiline string literal): The closingQuote.
// else (for SCE_VISUALPROLOG_COMMENT_BLOCK): The comment nesting level

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

// Options used for LexerVisualProlog
struct OptionsVisualProlog {
    OptionsVisualProlog() {
    }
};

static const char *const visualPrologWordLists[] = {
    "Major keywords (class, predicates, ...)",
    "Minor keywords (if, then, try, ...)",
    "Directive keywords without the '#' (include, requires, ...)",
    "Documentation keywords without the '@' (short, detail, ...)",
    0,
};

struct OptionSetVisualProlog : public OptionSet<OptionsVisualProlog> {
    OptionSetVisualProlog() {
        DefineWordListSets(visualPrologWordLists);
    }
};

class LexerVisualProlog : public DefaultLexer {
    WordList majorKeywords;
    WordList minorKeywords;
    WordList directiveKeywords;
    WordList docKeywords;
    OptionsVisualProlog options;
    OptionSetVisualProlog osVisualProlog;
public:
    LexerVisualProlog() : DefaultLexer("visualprolog", SCLEX_VISUALPROLOG) {
    }
    virtual ~LexerVisualProlog() {
    }
    void SCI_METHOD Release() override {
        delete this;
    }
    int SCI_METHOD Version() const override {
        return lvRelease5;
    }
    const char * SCI_METHOD PropertyNames() override {
        return osVisualProlog.PropertyNames();
    }
    int SCI_METHOD PropertyType(const char *name) override {
        return osVisualProlog.PropertyType(name);
    }
    const char * SCI_METHOD DescribeProperty(const char *name) override {
        return osVisualProlog.DescribeProperty(name);
    }
    Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osVisualProlog.PropertyGet(key);
	}
    const char * SCI_METHOD DescribeWordListSets() override {
        return osVisualProlog.DescribeWordListSets();
    }
    Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

    void * SCI_METHOD PrivateCall(int, void *) override {
        return 0;
    }

    static ILexer5 *LexerFactoryVisualProlog() {
        return new LexerVisualProlog();
    }
};

Sci_Position SCI_METHOD LexerVisualProlog::PropertySet(const char *key, const char *val) {
    if (osVisualProlog.PropertySet(&options, key, val)) {
        return 0;
    }
    return -1;
}

Sci_Position SCI_METHOD LexerVisualProlog::WordListSet(int n, const char *wl) {
    WordList *wordListN = 0;
    switch (n) {
    case 0:
        wordListN = &majorKeywords;
        break;
    case 1:
        wordListN = &minorKeywords;
        break;
    case 2:
        wordListN = &directiveKeywords;
        break;
    case 3:
        wordListN = &docKeywords;
        break;
    }
    Sci_Position firstModification = -1;
    if (wordListN) {
        WordList wlNew;
        wlNew.Set(wl);
        if (*wordListN != wlNew) {
            wordListN->Set(wl);
            firstModification = 0;
        }
    }
    return firstModification;
}

// Functor used to truncate history
struct After {
    Sci_Position line;
    After(Sci_Position line_) : line(line_) {}
};

static bool isLowerLetter(int ch){
    return ccLl == CategoriseCharacter(ch);
}

static bool isUpperLetter(int ch){
    return ccLu == CategoriseCharacter(ch);
}

static bool isAlphaNum(int ch){
    CharacterCategory cc = CategoriseCharacter(ch);
    return (ccLu == cc || ccLl == cc || ccLt == cc || ccLm == cc || ccLo == cc || ccNd == cc || ccNl == cc || ccNo == cc);
}

static bool isStringVerbatimOpenClose(int ch){
    CharacterCategory cc = CategoriseCharacter(ch);
    return (ccPc <= cc && cc <= ccSo);
}

static bool isIdChar(int ch){
    return ('_') == ch || isAlphaNum(ch);
}

static bool isOpenStringVerbatim(int next, int &closingQuote){
    switch (next) {
    case L'<':
        closingQuote = L'>';
        return true;
    case L'>':
        closingQuote = L'<';
        return true;
    case L'(':
        closingQuote = L')';
        return true;
    case L')':
        closingQuote = L'(';
        return true;
    case L'[':
        closingQuote = L']';
        return true;
    case L']':
        closingQuote = L'[';
        return true;
    case L'{':
        closingQuote = L'}';
        return true;
    case L'}':
        closingQuote = L'{';
        return true;
    case L'_':
    case L'.':
    case L',':
    case L';':
        return false;
    default:
        if (isStringVerbatimOpenClose(next)) {
            closingQuote = next;
            return true;
        } else {
			return false;
        }
    }
}

// Look ahead to see which colour "end" should have (takes colour after the following keyword)
static void endLookAhead(char s[], LexAccessor &styler, Sci_Position start) {
    char ch = styler.SafeGetCharAt(start, '\n');
    while (' ' == ch) {
        start++;
        ch = styler.SafeGetCharAt(start, '\n');
    }
    Sci_Position i = 0;
    while (i < 100 && isLowerLetter(ch)){
        s[i] = ch;
        i++;
        ch = styler.SafeGetCharAt(start + i, '\n');
    }
    s[i] = '\0';
}

static void forwardEscapeLiteral(StyleContext &sc, int EscapeState) {
    sc.Forward();
    if (sc.Match('"') || sc.Match('\'') || sc.Match('\\') || sc.Match('n') || sc.Match('l') || sc.Match('r') || sc.Match('t')) {
        sc.ChangeState(EscapeState);
    } else if (sc.Match('u')) {
        if (IsADigit(sc.chNext, 16)) {
            sc.Forward();
            if (IsADigit(sc.chNext, 16)) {
                sc.Forward();
                if (IsADigit(sc.chNext, 16)) {
                    sc.Forward();
                    if (IsADigit(sc.chNext, 16)) {
                        sc.Forward();
                        sc.ChangeState(EscapeState);
                    }
                }
            }
        }
    }
}

void SCI_METHOD LexerVisualProlog::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
    LexAccessor styler(pAccess);
    CharacterSet setDoxygen(CharacterSet::setAlpha, "");
    CharacterSet setNumber(CharacterSet::setNone, "0123456789abcdefABCDEFxoXO");

    StyleContext sc(startPos, length, initStyle, styler, 0x7f);

    int styleBeforeDocKeyword = SCE_VISUALPROLOG_DEFAULT;
    Sci_Position currentLine = styler.GetLine(startPos);

    int closingQuote = '"';
    int nestLevel = 0;
    if (currentLine >= 1)
    {
        nestLevel = styler.GetLineState(currentLine - 1);
        closingQuote = nestLevel;
    }

    // Truncate ppDefineHistory before current line

    for (; sc.More(); sc.Forward()) {

        // Determine if the current state should terminate.
        switch (sc.state) {
        case SCE_VISUALPROLOG_OPERATOR:
            sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            break;
        case SCE_VISUALPROLOG_NUMBER:
            // We accept almost anything because of hex. and number suffixes
            if (!(setNumber.Contains(sc.ch)) || (sc.Match('.') && IsADigit(sc.chNext))) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_IDENTIFIER:
            if (!isIdChar(sc.ch)) {
                char s[1000];
                sc.GetCurrent(s, sizeof(s));
                if (0 == strcmp(s, "end")) {
                    endLookAhead(s, styler, sc.currentPos);
                }
                if (majorKeywords.InList(s)) {
                    sc.ChangeState(SCE_VISUALPROLOG_KEY_MAJOR);
                } else if (minorKeywords.InList(s)) {
                    sc.ChangeState(SCE_VISUALPROLOG_KEY_MINOR);
                }
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_VARIABLE:
        case SCE_VISUALPROLOG_ANONYMOUS:
            if (!isIdChar(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_KEY_DIRECTIVE:
            if (!isLowerLetter(sc.ch)) {
                char s[1000];
                sc.GetCurrent(s, sizeof(s));
                if (!directiveKeywords.InList(s+1)) {
                    sc.ChangeState(SCE_VISUALPROLOG_IDENTIFIER);
                }
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_COMMENT_BLOCK:
            if (sc.Match('*', '/')) {
                sc.Forward();
                nestLevel--;
                int nextState = (nestLevel == 0) ? SCE_VISUALPROLOG_DEFAULT : SCE_VISUALPROLOG_COMMENT_BLOCK;
                sc.ForwardSetState(nextState);
            } else if (sc.Match('/', '*')) {
                sc.Forward();
                nestLevel++;
            } else if (sc.Match('@')) {
                styleBeforeDocKeyword = sc.state;
                sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
            }
            break;
        case SCE_VISUALPROLOG_COMMENT_LINE:
            if (sc.atLineEnd) {
                int nextState = (nestLevel == 0) ? SCE_VISUALPROLOG_DEFAULT : SCE_VISUALPROLOG_COMMENT_BLOCK;
                sc.SetState(nextState);
            } else if (sc.Match('@')) {
                styleBeforeDocKeyword = sc.state;
                sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
            }
            break;
        case SCE_VISUALPROLOG_COMMENT_KEY_ERROR:
            if (!setDoxygen.Contains(sc.ch) || sc.atLineEnd) {
                char s[1000];
                sc.GetCurrent(s, sizeof(s));
                if (docKeywords.InList(s+1)) {
                    sc.ChangeState(SCE_VISUALPROLOG_COMMENT_KEY);
                }
                if (SCE_VISUALPROLOG_COMMENT_LINE == styleBeforeDocKeyword && sc.atLineEnd) {
                    // end line comment
                    int nextState = (nestLevel == 0) ? SCE_VISUALPROLOG_DEFAULT : SCE_VISUALPROLOG_COMMENT_BLOCK;
                    sc.SetState(nextState);
                } else {
                    sc.SetState(styleBeforeDocKeyword);
                    if (SCE_VISUALPROLOG_COMMENT_BLOCK == styleBeforeDocKeyword && sc.Match('*', '/')) {
                        // we have consumed the '*' if it comes immediately after the docKeyword
                        sc.Forward();
                        sc.Forward();
                        nestLevel--;
                        if (0 == nestLevel) {
                            sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                        }
                    }
                }
            }
            break;
        case SCE_VISUALPROLOG_STRING_ESCAPE:
        case SCE_VISUALPROLOG_STRING_ESCAPE_ERROR:
            // return to SCE_VISUALPROLOG_STRING and treat as such (fall-through)
            sc.SetState(SCE_VISUALPROLOG_STRING);
            // Falls through.
        case SCE_VISUALPROLOG_STRING:
            if (sc.atLineEnd) {
                sc.SetState(SCE_VISUALPROLOG_STRING_EOL_OPEN);
            } else if (sc.Match(closingQuote)) {
                sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
            } else if (sc.Match('\\')) {
                sc.SetState(SCE_VISUALPROLOG_STRING_ESCAPE_ERROR);
                forwardEscapeLiteral(sc, SCE_VISUALPROLOG_STRING_ESCAPE);
            }
            break;
        case SCE_VISUALPROLOG_STRING_EOL_OPEN:
            if (sc.atLineStart) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_STRING_VERBATIM_SPECIAL:
        case SCE_VISUALPROLOG_STRING_VERBATIM_EOL:
            // return to SCE_VISUALPROLOG_STRING_VERBATIM and treat as such (fall-through)
            sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM);
            // Falls through.
        case SCE_VISUALPROLOG_STRING_VERBATIM:
            if (sc.atLineEnd) {
                sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM_EOL);
            } else if (sc.Match(closingQuote)) {
                if (closingQuote == sc.chNext) {
                    sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM_SPECIAL);
                    sc.Forward();
                } else {
                    sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
                }
            }
            break;
        }

        if (sc.atLineEnd) {
            // Update the line state, so it can be seen by next line
            int lineState = 0;
            if (SCE_VISUALPROLOG_STRING_VERBATIM_EOL == sc.state) {
                lineState = closingQuote;
            } else if (SCE_VISUALPROLOG_COMMENT_BLOCK == sc.state) {
                lineState = nestLevel;
            }
            styler.SetLineState(currentLine, lineState);
            currentLine++;
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_VISUALPROLOG_DEFAULT) {
            if (sc.Match('@') && isOpenStringVerbatim(sc.chNext, closingQuote)) {
                sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM);
                sc.Forward();
            } else if (IsADigit(sc.ch) || (sc.Match('.') && IsADigit(sc.chNext))) {
                sc.SetState(SCE_VISUALPROLOG_NUMBER);
            } else if (isLowerLetter(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_IDENTIFIER);
            } else if (isUpperLetter(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_VARIABLE);
            } else if (sc.Match('_')) {
                sc.SetState(SCE_VISUALPROLOG_ANONYMOUS);
            } else if (sc.Match('/', '*')) {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_BLOCK);
                nestLevel = 1;
                sc.Forward();	// Eat the * so it isn't used for the end of the comment
            } else if (sc.Match('%')) {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_LINE);
            } else if (sc.Match('\'')) {
                closingQuote = '\'';
                sc.SetState(SCE_VISUALPROLOG_STRING);
            } else if (sc.Match('"')) {
                closingQuote = '"';
                sc.SetState(SCE_VISUALPROLOG_STRING);
            } else if (sc.Match('#')) {
                sc.SetState(SCE_VISUALPROLOG_KEY_DIRECTIVE);
            } else if (isoperator(static_cast<char>(sc.ch)) || sc.Match('\\')) {
                sc.SetState(SCE_VISUALPROLOG_OPERATOR);
            }
        }

    }
    sc.Complete();
    styler.Flush();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerVisualProlog::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

    LexAccessor styler(pAccess);

    Sci_PositionU endPos = startPos + length;
    int visibleChars = 0;
    Sci_Position currentLine = styler.GetLine(startPos);
    int levelCurrent = SC_FOLDLEVELBASE;
    if (currentLine > 0)
        levelCurrent = styler.LevelAt(currentLine-1) >> 16;
    int levelMinCurrent = levelCurrent;
    int levelNext = levelCurrent;
    char chNext = styler[startPos];
    int styleNext = styler.StyleAt(startPos);
    int style = initStyle;
    for (Sci_PositionU i = startPos; i < endPos; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        style = styleNext;
        styleNext = styler.StyleAt(i + 1);
        bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
        if (style == SCE_VISUALPROLOG_OPERATOR) {
            if (ch == '{') {
                // Measure the minimum before a '{' to allow
                // folding on "} else {"
                if (levelMinCurrent > levelNext) {
                    levelMinCurrent = levelNext;
                }
                levelNext++;
            } else if (ch == '}') {
                levelNext--;
            }
        }
        if (!IsASpace(ch))
            visibleChars++;
        if (atEOL || (i == endPos-1)) {
            int levelUse = levelCurrent;
            int lev = levelUse | levelNext << 16;
            if (levelUse < levelNext)
                lev |= SC_FOLDLEVELHEADERFLAG;
            if (lev != styler.LevelAt(currentLine)) {
                styler.SetLevel(currentLine, lev);
            }
            currentLine++;
            levelCurrent = levelNext;
            levelMinCurrent = levelCurrent;
            if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length()-1))) {
                // There is an empty line at end of file so give it same level and empty
                styler.SetLevel(currentLine, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
            }
            visibleChars = 0;
        }
    }
}

LexerModule lmVisualProlog(SCLEX_VISUALPROLOG, LexerVisualProlog::LexerFactoryVisualProlog, "visualprolog", visualPrologWordLists);
