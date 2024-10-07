// Scintilla source code edit control
/** @file LexVisualProlog.cxx
** Lexer for Visual Prolog.
**/
// Author Thomas Linder Puls, PDC A/S, http://www.visual-prolog.com
// Based on Lexer for C++, C, Java, and JavaScript.
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// The line state contains:
// In SCE_VISUALPROLOG_STRING: The closing quote and information about verbatim string.
// and a stack of nesting kinds: comment, embedded (syntax) and (syntax) place holder

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

namespace {
// Options used for LexerVisualProlog
struct OptionsVisualProlog {
    bool verbatimStrings;
    bool backQuotedStrings;
    OptionsVisualProlog() {
        verbatimStrings = true;
        backQuotedStrings = false;
    }
};

static const char* const visualPrologWordLists[] = {
    "Major keywords (class, predicates, ...)",
    "Minor keywords (if, then, try, ...)",
    "Directive keywords without the '#' (include, requires, ...)",
    "Documentation keywords without the '@' (short, detail, ...)",
    0,
};

struct OptionSetVisualProlog : public OptionSet<OptionsVisualProlog> {
    OptionSetVisualProlog() {
        DefineProperty("lexer.visualprolog.verbatim.strings", &OptionsVisualProlog::verbatimStrings,
            "Set to 0 to disable highlighting verbatim strings using '@'.");
        DefineProperty("lexer.visualprolog.backquoted.strings", &OptionsVisualProlog::backQuotedStrings,
            "Set to 1 to enable using back quotes (``) to delimit strings.");
        DefineWordListSets(visualPrologWordLists);
    }
};

LexicalClass lexicalClasses[] = {
    SCE_VISUALPROLOG_DEFAULT, "SCE_VISUALPROLOG_DEFAULT", "default", "Default style",
    SCE_VISUALPROLOG_KEY_MAJOR, "SCE_VISUALPROLOG_KEY_MAJOR", "keyword major", "Major keyword",
    SCE_VISUALPROLOG_KEY_MINOR, "SCE_VISUALPROLOG_KEY_MINOR", "keyword minor", "Minor keyword",
    SCE_VISUALPROLOG_KEY_DIRECTIVE, "SCE_VISUALPROLOG_KEY_DIRECTIVE", "keyword preprocessor", "Directove keyword",
    SCE_VISUALPROLOG_COMMENT_BLOCK, "SCE_VISUALPROLOG_COMMENT_BLOCK", "comment", "Multiline comment /* */",
    SCE_VISUALPROLOG_COMMENT_LINE, "SCE_VISUALPROLOG_COMMENT_LINE", "comment line", "Line comment % ...",
    SCE_VISUALPROLOG_COMMENT_KEY, "SCE_VISUALPROLOG_COMMENT_KEY", "comment documentation keyword", "Doc keyword in comment % @short ...",
    SCE_VISUALPROLOG_COMMENT_KEY_ERROR, "SCE_VISUALPROLOG_COMMENT_KEY_ERROR", "comment", "A non recognized doc keyword % @qqq ...",
    SCE_VISUALPROLOG_IDENTIFIER, "SCE_VISUALPROLOG_IDENTIFIER", "identifier", "Identifier (black)",
    SCE_VISUALPROLOG_VARIABLE, "SCE_VISUALPROLOG_VARIABLE", "variable identifier", "Variable (green)",
    SCE_VISUALPROLOG_ANONYMOUS, "SCE_VISUALPROLOG_ANONYMOUS", "variable anonymous identifier", "Anonymous Variable _XXX (dimmed green)",
    SCE_VISUALPROLOG_NUMBER, "SCE_VISUALPROLOG_NUMBER", "numeric", "Number",
    SCE_VISUALPROLOG_OPERATOR, "SCE_VISUALPROLOG_OPERATOR", "operator", "Operator",
    SCE_VISUALPROLOG_STRING, "SCE_VISUALPROLOG_STRING", "literal string", "String literal",
    SCE_VISUALPROLOG_STRING_QUOTE, "SCE_VISUALPROLOG_STRING_QUOTE", "literal string quote", "Quotes surrounding string literals",
    SCE_VISUALPROLOG_STRING_ESCAPE, "SCE_VISUALPROLOG_STRING_ESCAPE", "literal string escapesequence", "Escape sequence in string literal",
    SCE_VISUALPROLOG_STRING_ESCAPE_ERROR, "SCE_VISUALPROLOG_STRING_ESCAPE_ERROR", "error literal string escapesequence", "Error in escape sequence in string literal",
    SCE_VISUALPROLOG_STRING_EOL, "SCE_VISUALPROLOG_STRING_EOL", "literal string multiline raw escapesequence", "Verbatim/multiline string literal EOL",
    SCE_VISUALPROLOG_EMBEDDED, "SCE_VISUALPROLOG_EMBEDDED", "literal string embedded", "Embedded syntax [| ... |]",
    SCE_VISUALPROLOG_PLACEHOLDER, "SCE_VISUALPROLOG_PLACEHOLDER", "operator embedded", "Syntax place holder {| ... |}:ident in embedded syntax"
};

LexicalClass getLexicalClass(int style) {
    for (auto lc : lexicalClasses) {
        if (style == lc.value) {
            return lc;
        }
    }
    return {style, "", "unused", ""};
}


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
    const char* SCI_METHOD PropertyNames() override {
        return osVisualProlog.PropertyNames();
    }
    int SCI_METHOD PropertyType(const char* name) override {
        return osVisualProlog.PropertyType(name);
    }
    const char* SCI_METHOD DescribeProperty(const char* name) override {
        return osVisualProlog.DescribeProperty(name);
    }
    Sci_Position SCI_METHOD PropertySet(const char* key, const char* val) override;
    const char* SCI_METHOD PropertyGet(const char* key) override {
        return osVisualProlog.PropertyGet(key);
    }
    const char* SCI_METHOD DescribeWordListSets() override {
        return osVisualProlog.DescribeWordListSets();
    }
    Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;

    void* SCI_METHOD PrivateCall(int, void*) override {
        return 0;
    }

    int SCI_METHOD NamedStyles() override {
        int namedStyles = 0;
        for (auto lc : lexicalClasses) {
            if (namedStyles < lc.value) {
                namedStyles = lc.value;
            }
        }
        return namedStyles;
    }
    const char* SCI_METHOD NameOfStyle(int style) override {
        return getLexicalClass(style).name;
    }
    const char* SCI_METHOD TagsOfStyle(int style) override {
        return getLexicalClass(style).tags;
    }
    const char* SCI_METHOD DescriptionOfStyle(int style) override {
        return getLexicalClass(style).description;
    }

    static ILexer5* LexerFactoryVisualProlog() {
        return new LexerVisualProlog();
    }
};

Sci_Position SCI_METHOD LexerVisualProlog::PropertySet(const char* key, const char* val) {
    if (osVisualProlog.PropertySet(&options, key, val)) {
        return 0;
    }
    return -1;
}

Sci_Position SCI_METHOD LexerVisualProlog::WordListSet(int n, const char* wl) {
    WordList* wordListN = 0;
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

static bool isLowerLetter(int ch) {
    return ccLl == CategoriseCharacter(ch);
}

static bool isUpperLetter(int ch) {
    return ccLu == CategoriseCharacter(ch);
}

static bool isAlphaNum(int ch) {
    CharacterCategory cc = CategoriseCharacter(ch);
    return (ccLu == cc || ccLl == cc || ccLt == cc || ccLm == cc || ccLo == cc || ccNd == cc || ccNl == cc || ccNo == cc);
}

static bool isStringVerbatimOpenClose(int ch) {
    CharacterCategory cc = CategoriseCharacter(ch);
    return (ccPc <= cc && cc <= ccSo);
}

static bool isIdChar(int ch) {
    return ('_') == ch || isAlphaNum(ch);
}

// Look ahead to see which colour "end" should have (takes colour after the following keyword)
static void endLookAhead(char s[], LexAccessor& styler, Sci_Position start) {
    char ch = styler.SafeGetCharAt(start, '\n');
    while (' ' == ch) {
        start++;
        ch = styler.SafeGetCharAt(start, '\n');
    }
    Sci_Position i = 0;
    while (i < 100 && isLowerLetter(ch)) {
        s[i] = ch;
        i++;
        ch = styler.SafeGetCharAt(start + i, '\n');
    }
    s[i] = '\0';
}


class lineState {
public:
    bool verbatim = false;
    int closingQuote = 0;
    int kindStack = 0;

    bool isOpenStringVerbatim(int next) {
        if (next > 0x7FFF) {
            return false;
        }
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

    enum kind {
        none = 0,
        comment = 1,
        embedded = 2,
        placeholder = 3
    };

    void setState(int state) {
        verbatim = state >> 31;
        closingQuote = state >> 16 & 0x7FFF;
        kindStack = state & 0xFFFF;
    }

    int getState() {
        return verbatim << 31 | closingQuote << 16 | (kindStack & 0xFFFF);
    }

    void enter(kind k) {
        kindStack = kindStack << 2 | k;
    }

    void leave(kind k) {
        if (k == currentKind()) {
            kindStack = kindStack >> 2;
        }
    }
    kind currentKind() {
        return static_cast<kind>(kindStack & 0x3);
    }
    kind stateKind2(int ks) {
        if (0 == ks) {
            return none;
        } else {
            kind k1 = stateKind2(ks >> 2);
            kind k2 = static_cast<kind>(ks & 0x3);
            if (embedded == k1 && k2 == comment) {
                return embedded;
            } else {
                return k2;
            }
        }
    }
    kind stateKind() {
        return stateKind2(kindStack);
    }
};

void SCI_METHOD LexerVisualProlog::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) {
    LexAccessor styler(pAccess);
    CharacterSet setDoxygen(CharacterSet::setAlpha, "");
    CharacterSet setNumber(CharacterSet::setNone, "0123456789abcdefABCDEFxoXO_");

    StyleContext sc(startPos, length, initStyle, styler, 0x7f);

    int styleBeforeDocKeyword = SCE_VISUALPROLOG_DEFAULT;

    lineState ls;
    if (sc.currentLine >= 1) {
        ls.setState(styler.GetLineState(sc.currentLine - 1));
    }

    bool newState = false;

    for (; sc.More(); sc.Forward()) {

        Sci_Position currentLineEntry = sc.currentLine;

        if (newState) {
            newState = false;
            int state;
            switch (ls.stateKind()) {
                case lineState::comment:
                    state = SCE_VISUALPROLOG_COMMENT_BLOCK;
                    break;
                case lineState::embedded:
                    state = SCE_VISUALPROLOG_EMBEDDED;
                    break;
                case lineState::placeholder:
                    state = SCE_VISUALPROLOG_PLACEHOLDER;
                    break;
                default:
                    state = SCE_VISUALPROLOG_DEFAULT;
                    break;
            }
            sc.SetState(state);
        }

        // Determine if the current state should terminate.
        switch (sc.state) {
            case SCE_VISUALPROLOG_OPERATOR:
                sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                break;
            case SCE_VISUALPROLOG_NUMBER:
                // We accept almost anything because of hex, '.' and number suffixes
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
                    if (!directiveKeywords.InList(s + 1)) {
                        sc.ChangeState(SCE_VISUALPROLOG_IDENTIFIER);
                    }
                    sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                }
                break;
            case SCE_VISUALPROLOG_COMMENT_LINE:
                if (sc.MatchLineEnd()) {
                    int nextState = (lineState::comment == ls.currentKind()) ? SCE_VISUALPROLOG_COMMENT_BLOCK : SCE_VISUALPROLOG_DEFAULT;
                    sc.SetState(nextState);
                } else if (sc.Match('@')) {
                    styleBeforeDocKeyword = SCE_VISUALPROLOG_COMMENT_LINE;
                    sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
                }
                break;
            case SCE_VISUALPROLOG_COMMENT_BLOCK:
                if (sc.Match('*', '/')) {
                    sc.Forward();
                    ls.leave(lineState::comment);
                    newState = true;
                } else if (sc.Match('/', '*')) {
                    sc.Forward();
                    ls.enter(lineState::comment);
                } else if (sc.Match('@')) {
                    styleBeforeDocKeyword = SCE_VISUALPROLOG_COMMENT_BLOCK;
                    sc.SetState(SCE_VISUALPROLOG_COMMENT_KEY_ERROR);
                }
                break;
            case SCE_VISUALPROLOG_COMMENT_KEY_ERROR:
                if (!setDoxygen.Contains(sc.ch) || sc.MatchLineEnd()) {
                    char s[1000];
                    sc.GetCurrent(s, sizeof(s));
                    if (docKeywords.InList(s + 1)) {
                        sc.ChangeState(SCE_VISUALPROLOG_COMMENT_KEY);
                    }
                    if (SCE_VISUALPROLOG_COMMENT_LINE == styleBeforeDocKeyword && sc.MatchLineEnd()) {
                        // end line comment
                        int nextState = (lineState::comment == ls.currentKind()) ? SCE_VISUALPROLOG_COMMENT_BLOCK : SCE_VISUALPROLOG_DEFAULT;
                        sc.SetState(nextState);
                    } else {
                        sc.SetState(styleBeforeDocKeyword);
                        if (SCE_VISUALPROLOG_COMMENT_BLOCK == styleBeforeDocKeyword && sc.Match('*', '/')) {
                            // we have consumed the '*' if it comes immediately after the docKeyword
                            sc.Forward();
                            ls.leave(lineState::comment);
                            newState = true;
                        }
                    }
                }
                break;
            case SCE_VISUALPROLOG_STRING_ESCAPE_ERROR:
                if (sc.atLineStart) {
                    sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                    break;
                }
                [[fallthrough]];
            case SCE_VISUALPROLOG_STRING_ESCAPE:
            case SCE_VISUALPROLOG_STRING_QUOTE:
            case SCE_VISUALPROLOG_STRING_EOL:
                // return to SCE_VISUALPROLOG_STRING and treat as such (fallthrough)
                sc.SetState(SCE_VISUALPROLOG_STRING);
                [[fallthrough]];
            case SCE_VISUALPROLOG_STRING:
                if (sc.MatchLineEnd() | sc.atLineEnd) {
                    if (ls.verbatim) {
                        sc.SetState(SCE_VISUALPROLOG_STRING_EOL);
                    } else {
                        ls.closingQuote = 0;
                        sc.SetState(SCE_VISUALPROLOG_STRING_ESCAPE_ERROR);
                    }
                } else if (sc.Match(ls.closingQuote)) {
                    if (ls.verbatim && ls.closingQuote == sc.chNext) {
                        sc.SetState(SCE_VISUALPROLOG_STRING_ESCAPE);
                        sc.Forward();
                    } else {
                        ls.closingQuote = 0;
                        sc.SetState(SCE_VISUALPROLOG_STRING_QUOTE);
                        sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
                    }
                } else if (!ls.verbatim && sc.Match('\\')) {
                    sc.SetState(SCE_VISUALPROLOG_STRING_ESCAPE_ERROR);
                    sc.Forward();
                    if (sc.MatchLineEnd()) {
                        sc.ForwardSetState(SCE_VISUALPROLOG_DEFAULT);
                    } else {
                        if (sc.Match('"') || sc.Match('\'') || sc.Match('\\') || sc.Match('n') || sc.Match('l') || sc.Match('r') || sc.Match('t')) {
                            sc.ChangeState(SCE_VISUALPROLOG_STRING_ESCAPE);
                        } else if (sc.Match('u')) {
                            if (IsADigit(sc.chNext, 16)) {
                                sc.Forward();
                                if (IsADigit(sc.chNext, 16)) {
                                    sc.Forward();
                                    if (IsADigit(sc.chNext, 16)) {
                                        sc.Forward();
                                        if (IsADigit(sc.chNext, 16)) {
                                            sc.Forward();
                                            sc.ChangeState(SCE_VISUALPROLOG_STRING_ESCAPE);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            case SCE_VISUALPROLOG_EMBEDDED:
                if (sc.Match('|', ']')) {
                    sc.Forward();
                    ls.leave(lineState::embedded);
                    newState = true;
                } else if (sc.Match('[', '|')) {
                    sc.Forward();
                    ls.enter(lineState::embedded);
                } else if (sc.Match('{', '|') && lineState::comment != ls.currentKind()) {
                    sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                } else if (sc.Match('/', '*')) {
                    sc.Forward();
                    ls.enter(lineState::comment);
                } else if (sc.Match('*', '/')) {
                    sc.Forward();
                    ls.leave(lineState::comment);
                    newState = true;
                }
                break;
            case SCE_VISUALPROLOG_PLACEHOLDER:
                if (lineState::embedded == ls.currentKind()) {
                    sc.SetState(SCE_VISUALPROLOG_EMBEDDED);
                } else {
                    sc.SetState(SCE_VISUALPROLOG_DEFAULT);
                }
                break;
        }

        if (currentLineEntry != sc.currentLine) {
            styler.SetLineState(currentLineEntry, ls.getState());
        }
        if (sc.MatchLineEnd() | sc.atLineEnd) {
            if (sc.More()) { // currentLine can be outside the document 
                styler.SetLineState(sc.currentLine, ls.getState());
            }
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_VISUALPROLOG_DEFAULT) {
            if (options.verbatimStrings && sc.Match('@') && ls.isOpenStringVerbatim(sc.chNext)) {
                ls.verbatim = true;
                sc.SetState(SCE_VISUALPROLOG_STRING_QUOTE);
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
                ls.enter(lineState::comment);
                sc.Forward();
            } else if (sc.Match('%')) {
                sc.SetState(SCE_VISUALPROLOG_COMMENT_LINE);
            } else if (sc.Match('[', '|')) {
                sc.SetState(SCE_VISUALPROLOG_EMBEDDED);
                ls.enter(lineState::embedded);
                sc.Forward();
            } else if (sc.Match('{', '|')) {
                sc.SetState(SCE_VISUALPROLOG_PLACEHOLDER);
                ls.enter(lineState::placeholder);
                sc.Forward();
            } else if (sc.Match('|', '}')) {
                sc.SetState(SCE_VISUALPROLOG_PLACEHOLDER);
                sc.Forward();
                if (':' == sc.chNext) {
                    sc.Forward();
                    for (; isIdChar(sc.chNext); sc.Forward()) {
                    }
                }
                ls.leave(lineState::placeholder);
                newState = true;
            } else if (sc.Match('\'')) {
                ls.verbatim = false;
                ls.closingQuote = '\'';
                sc.SetState(SCE_VISUALPROLOG_STRING_QUOTE);
            } else if (sc.Match('"')) {
                ls.verbatim = false;
                ls.closingQuote = '"';
                sc.SetState(SCE_VISUALPROLOG_STRING_QUOTE);
            } else if (options.backQuotedStrings && sc.Match('`')) {
                ls.verbatim = false;
                ls.closingQuote = '`';
                sc.SetState(SCE_VISUALPROLOG_STRING_QUOTE);
            } else if (sc.Match('#')) {
                sc.SetState(SCE_VISUALPROLOG_KEY_DIRECTIVE);
            } else if (isoperator(static_cast<char>(sc.ch)) || sc.Match('\\') ||
                (!options.verbatimStrings && sc.Match('@'))) {
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

#if defined(__clang__)
#if __has_warning("-Wunused-but-set-variable")
// Disable warning for visibleChars
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif
#endif

void SCI_METHOD LexerVisualProlog::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) {

    LexAccessor styler(pAccess);

    Sci_PositionU endPos = startPos + length;
    int visibleChars = 0;
    Sci_Position currentLine = styler.GetLine(startPos);
    int levelCurrent = SC_FOLDLEVELBASE;
    if (currentLine > 0)
        levelCurrent = styler.LevelAt(currentLine - 1) >> 16;
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
        if (atEOL || (i == endPos - 1)) {
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
            if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length() - 1))) {
                // There is an empty line at end of file so give it same level and empty
                styler.SetLevel(currentLine, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
            }
            visibleChars = 0;
        }
    }
}
}

extern const LexerModule lmVisualProlog(SCLEX_VISUALPROLOG, LexerVisualProlog::LexerFactoryVisualProlog, "visualprolog", visualPrologWordLists);
