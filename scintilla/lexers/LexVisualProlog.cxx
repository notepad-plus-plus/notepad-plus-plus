// Scintilla source code edit control
/** @file LexVisualProlog.cxx
** Lexer for Visual Prolog.
**/
// Author Thomas Linder Puls, Prolog Development Denter A/S, http://www.visual-prolog.com
// Based on Lexer for C++, C, Java, and JavaScript.
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

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

class LexerVisualProlog : public ILexer {
    WordList majorKeywords;
    WordList minorKeywords;
    WordList directiveKeywords;
    WordList docKeywords;
    OptionsVisualProlog options;
    OptionSetVisualProlog osVisualProlog;
public:
    LexerVisualProlog() {
    }
    virtual ~LexerVisualProlog() {
    }
    void SCI_METHOD Release() {
        delete this;
    }
    int SCI_METHOD Version() const {
        return lvOriginal;
    }
    const char * SCI_METHOD PropertyNames() {
        return osVisualProlog.PropertyNames();
    }
    int SCI_METHOD PropertyType(const char *name) {
        return osVisualProlog.PropertyType(name);
    }
    const char * SCI_METHOD DescribeProperty(const char *name) {
        return osVisualProlog.DescribeProperty(name);
    }
    int SCI_METHOD PropertySet(const char *key, const char *val);
    const char * SCI_METHOD DescribeWordListSets() {
        return osVisualProlog.DescribeWordListSets();
    }
    int SCI_METHOD WordListSet(int n, const char *wl);
    void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
    void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess);

    void * SCI_METHOD PrivateCall(int, void *) {
        return 0;
    }

    static ILexer *LexerFactoryVisualProlog() {
        return new LexerVisualProlog();
    }
};

int SCI_METHOD LexerVisualProlog::PropertySet(const char *key, const char *val) {
    if (osVisualProlog.PropertySet(&options, key, val)) {
        return 0;
    }
    return -1;
}

int SCI_METHOD LexerVisualProlog::WordListSet(int n, const char *wl) {
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
    int firstModification = -1;
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
    int line;
    After(int line_) : line(line_) {}
};

// Look ahead to see which colour "end" should have (takes colour after the following keyword)
static void endLookAhead(char s[], LexAccessor &styler, int start, CharacterSet &setIdentifier) {
    char ch = styler.SafeGetCharAt(start, '\n');
    while (' ' == ch) {
        start++;
        ch = styler.SafeGetCharAt(start, '\n');
    }
    int i = 0;
    while (i < 100 && setIdentifier.Contains(ch)){
        s[i] = ch;
        i++;
        ch = styler.SafeGetCharAt(start + i, '\n');
    }
    s[i] = '\0';
}

static void forwardEscapeLiteral(StyleContext &sc, int OwnChar, int EscapeState) {
    sc.Forward();
    if (sc.ch == OwnChar || sc.ch == '\\' || sc.ch == 'n' || sc.ch == 'l' || sc.ch == 'r' || sc.ch == 't') {
        sc.ChangeState(EscapeState);
    } else if (sc.ch == 'u') {
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

void SCI_METHOD LexerVisualProlog::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {
    LexAccessor styler(pAccess);

    CharacterSet setDoxygen(CharacterSet::setAlpha, "$@\\&<>#{}[]");

    CharacterSet setLowerStart(CharacterSet::setLower);
    CharacterSet setVariableStart(CharacterSet::setUpper);
    CharacterSet setIdentifier(CharacterSet::setAlphaNum, "_", 0x80, true);

    int styleBeforeDocKeyword = SCE_VISUALPROLOG_DEFAULT;

    int currentLine = styler.GetLine(startPos);

    int nestLevel = 0;
    if (currentLine >= 1)
    {
        nestLevel = styler.GetLineState(currentLine - 1);
    }

    StyleContext sc(startPos, length, initStyle, styler, 0x7f);

    // Truncate ppDefineHistory before current line

    for (; sc.More(); sc.Forward()) {

        if (sc.atLineEnd) {
            // Update the line state, so it can be seen by next line
            styler.SetLineState(currentLine, nestLevel);
            currentLine++;
        }

        if (sc.atLineStart) {
            if ((sc.state == SCE_VISUALPROLOG_STRING) || (sc.state == SCE_VISUALPROLOG_CHARACTER)) {
                // Prevent SCE_VISUALPROLOG_STRING_EOL from leaking back to previous line which
                // ends with a line continuation by locking in the state upto this position.
                sc.SetState(sc.state);
            }
        }

        const bool atLineEndBeforeSwitch = sc.atLineEnd;

        // Determine if the current state should terminate.
        switch (sc.state) {
        case SCE_VISUALPROLOG_OPERATOR:
            sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            break;
        case SCE_VISUALPROLOG_NUMBER:
            // We accept almost anything because of hex. and number suffixes
            if (!(setIdentifier.Contains(sc.ch) || (sc.ch == '.') || ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_IDENTIFIER:
            if (!setIdentifier.Contains(sc.ch)) {
                char s[1000];
                sc.GetCurrent(s, sizeof(s));
                if (0 == strcmp(s, "end")) {
                    endLookAhead(s, styler, sc.currentPos, setIdentifier);
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
            if (!setIdentifier.Contains(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_KEY_DIRECTIVE:
            if (!setLowerStart.Contains(sc.ch)) {
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
            } else if (sc.ch == '%') {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_LINE);
            } else if (sc.ch == '@') {
                styleBeforeDocKeyword = sc.state;
                sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
            }
            break;
        case SCE_VISUALPROLOG_COMMENT_LINE:
            if (sc.atLineEnd) {
                int nextState = (nestLevel == 0) ? SCE_VISUALPROLOG_DEFAULT : SCE_VISUALPROLOG_COMMENT_BLOCK;
                sc.SetState(nextState);
            } else if (sc.ch == '@') {
                styleBeforeDocKeyword = sc.state;
                sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
            }
            break;
        case SCE_VISUALPROLOG_COMMENT_KEY_ERROR:
            if (!setDoxygen.Contains(sc.ch)) {
                char s[1000];
                sc.GetCurrent(s, sizeof(s));
                if (docKeywords.InList(s+1)) {
                    sc.ChangeState(SCE_VISUALPROLOG_COMMENT_KEY);
                }
                sc.SetState(styleBeforeDocKeyword);
            }
            if (SCE_VISUALPROLOG_COMMENT_LINE == styleBeforeDocKeyword && sc.atLineStart) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            } else if (SCE_VISUALPROLOG_COMMENT_BLOCK == styleBeforeDocKeyword && sc.atLineStart) {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_BLOCK);
            }
            break;
        case SCE_VISUALPROLOG_STRING_ESCAPE:
        case SCE_VISUALPROLOG_STRING_ESCAPE_ERROR:
            // return to SCE_VISUALPROLOG_STRING and treat as such (fall-through)
            sc.SetState(SCE_VISUALPROLOG_STRING);
        case SCE_VISUALPROLOG_STRING:
            if (sc.atLineEnd) {
                sc.SetState(SCE_VISUALPROLOG_STRING_EOL_OPEN);
            } else if (sc.ch == '"') {
                sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
            } else if (sc.ch == '\\') {
                sc.SetState(SCE_VISUALPROLOG_STRING_ESCAPE_ERROR);
                forwardEscapeLiteral(sc, '"', SCE_VISUALPROLOG_STRING_ESCAPE);
            }
            break;
        case SCE_VISUALPROLOG_CHARACTER_TOO_MANY:
            if (sc.atLineStart) {
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
            } else if (sc.ch == '\'') {
                sc.SetState(SCE_VISUALPROLOG_CHARACTER);
                sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
            }
            break;
        case SCE_VISUALPROLOG_CHARACTER:
            if (sc.atLineEnd) {
                sc.SetState(SCE_VISUALPROLOG_STRING_EOL_OPEN);  // reuse STRING_EOL_OPEN for this
            } else if (sc.ch == '\'') {
                sc.SetState(SCE_VISUALPROLOG_CHARACTER_ESCAPE_ERROR);
                sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
            } else {
                if (sc.ch == '\\') {
                    sc.SetState(SCE_VISUALPROLOG_CHARACTER_ESCAPE_ERROR);
                    forwardEscapeLiteral(sc, '\'', SCE_VISUALPROLOG_CHARACTER);
                } 
                sc.ForwardSetState(SCE_VISUALPROLOG_CHARACTER);
                if (sc.ch == '\'') {
                    sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
                } else {
                    sc.SetState(SCE_VISUALPROLOG_CHARACTER_TOO_MANY);
                }
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
        case SCE_VISUALPROLOG_STRING_VERBATIM:
            if (sc.atLineEnd) {
                sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM_EOL);
            } else if (sc.ch == '\"') {
                if (sc.chNext == '\"') {
                    sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM_SPECIAL);
                    sc.Forward();
                } else {
                    sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
                }
            }
            break;
        }

        if (sc.atLineEnd && !atLineEndBeforeSwitch) {
            // State exit processing consumed characters up to end of line.
            currentLine++;
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_VISUALPROLOG_DEFAULT) {
            if (sc.Match('@', '\"')) {
                sc.SetState(SCE_VISUALPROLOG_STRING_VERBATIM);
                sc.Forward();
            } else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
                sc.SetState(SCE_VISUALPROLOG_NUMBER);
            } else if (setLowerStart.Contains(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_IDENTIFIER);
            } else if (setVariableStart.Contains(sc.ch)) {
                sc.SetState(SCE_VISUALPROLOG_VARIABLE);
            } else if (sc.ch == '_') {
                sc.SetState(SCE_VISUALPROLOG_ANONYMOUS);
            } else if (sc.Match('/', '*')) {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_BLOCK);
                nestLevel = 1;
                sc.Forward();	// Eat the * so it isn't used for the end of the comment
            } else if (sc.ch == '%') {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_LINE);
            } else if (sc.ch == '\"') {
                sc.SetState(SCE_VISUALPROLOG_STRING);
            } else if (sc.ch == '\'') {
                sc.SetState(SCE_VISUALPROLOG_CHARACTER);
            } else if (sc.ch == '#') {
                sc.SetState(SCE_VISUALPROLOG_KEY_DIRECTIVE);
            } else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '\\') {
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

void SCI_METHOD LexerVisualProlog::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {

    LexAccessor styler(pAccess);

    unsigned int endPos = startPos + length;
    int visibleChars = 0;
    int currentLine = styler.GetLine(startPos);
    int levelCurrent = SC_FOLDLEVELBASE;
    if (currentLine > 0)
        levelCurrent = styler.LevelAt(currentLine-1) >> 16;
    int levelMinCurrent = levelCurrent;
    int levelNext = levelCurrent;
    char chNext = styler[startPos];
    int styleNext = styler.StyleAt(startPos);
    int style = initStyle;
    for (unsigned int i = startPos; i < endPos; i++) {
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
            if (atEOL && (i == static_cast<unsigned int>(styler.Length()-1))) {
                // There is an empty line at end of file so give it same level and empty
                styler.SetLevel(currentLine, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
            }
            visibleChars = 0;
        }
    }
}

LexerModule lmVisualProlog(SCLEX_VISUALPROLOG, LexerVisualProlog::LexerFactoryVisualProlog, "visualprolog", visualPrologWordLists);
