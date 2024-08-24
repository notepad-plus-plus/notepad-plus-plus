// Scintilla source code edit control
/** @file LexNim.cxx
** Lexer for Nim
** Written by Jad Altahan (github.com/xv)
** Nim manual: https://nim-lang.org/docs/manual.html
**/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <map>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {
    // Use an unnamed namespace to protect the functions and classes from name conflicts

enum NumType {
    Binary,
    Octal,
    Exponent,
    Hexadecimal,
    Decimal,
    FormatError
};

int GetNumStyle(const int numType) noexcept {
    if (numType == NumType::FormatError) {
        return SCE_NIM_NUMERROR;
    }

    return SCE_NIM_NUMBER;
}

constexpr bool IsLetter(const int ch) noexcept {
    // 97 to 122 || 65 to 90
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool IsAWordChar(const int ch) noexcept {
    return ch < 0x80 && (isalnum(ch) || ch == '_' || ch == '.');
}

int IsNumHex(const StyleContext &sc) noexcept {
    return sc.chNext == 'x' || sc.chNext == 'X';
}

int IsNumBinary(const StyleContext &sc) noexcept {
    return sc.chNext == 'b' || sc.chNext == 'B';
}

int IsNumOctal(const StyleContext &sc) {
    return IsADigit(sc.chNext) || sc.chNext == 'o';
}

constexpr bool IsNewline(const int ch) noexcept {
    return (ch == '\n' || ch == '\r');
}

bool IsFuncName(const char *str) noexcept {
    const char *identifiers[] = {
        "proc",
        "func",
        "macro",
        "method",
        "template",
        "iterator",
        "converter"
    };

    for (const char *id : identifiers) {
        if (strcmp(str, id) == 0) {
            return true;
        }
    }

    return false;
}

constexpr bool IsTripleLiteral(const int style) noexcept {
    return style == SCE_NIM_TRIPLE || style == SCE_NIM_TRIPLEDOUBLE;
}

constexpr bool IsLineComment(const int style) noexcept {
    return style == SCE_NIM_COMMENTLINE || style == SCE_NIM_COMMENTLINEDOC;
}

constexpr bool IsStreamComment(const int style) noexcept {
    return style == SCE_NIM_COMMENT || style == SCE_NIM_COMMENTDOC;
}

// Adopted from Accessor.cxx
int GetIndent(const Sci_Position line, Accessor &styler) {
    Sci_Position startPos = styler.LineStart(line);
    const Sci_Position eolPos = styler.LineStart(line + 1) - 1;

    char ch = styler[startPos];
    int style = styler.StyleAt(startPos);

    int indent = 0;
    bool inPrevPrefix = line > 0;
    Sci_Position posPrev = inPrevPrefix ? styler.LineStart(line - 1) : 0;

    // No fold points inside triple literals
    while ((IsASpaceOrTab(ch) || IsTripleLiteral(style)) && (startPos < eolPos)) {
        if (inPrevPrefix) {
            const char chPrev = styler[posPrev++];
            if (chPrev != ' ' && chPrev != '\t') {
                inPrevPrefix = false;
            }
        }

        if (ch == '\t') {
            indent = (indent / 8 + 1) * 8;
        } else {
            indent++;
        }

        startPos++;
        ch = styler[startPos];
        style = styler.StyleAt(startPos);
    }

    // Prevent creating fold lines for comments if indented
    if (!(IsStreamComment(style) || IsLineComment(style)))
        indent += SC_FOLDLEVELBASE;

    if (styler.LineStart(line) == styler.Length() 
        || IsASpaceOrTab(ch) 
        || IsNewline(ch) 
        || IsStreamComment(style)
        || IsLineComment(style)) {
        return indent | SC_FOLDLEVELWHITEFLAG;
    } else {
        return indent;
    }
}

int IndentAmount(const Sci_Position line, Accessor &styler) {
    const int indent = GetIndent(line, styler);
    const int indentLevel = indent & SC_FOLDLEVELNUMBERMASK;
    return indentLevel <= SC_FOLDLEVELBASE ? indent : indentLevel | (indent & ~SC_FOLDLEVELNUMBERMASK);
}

struct OptionsNim {
    bool fold;
    bool foldCompact;
    bool highlightRawStrIdent;

    OptionsNim() {
        fold = true;
        foldCompact = true;
        highlightRawStrIdent = false;
    }
};

static const char *const nimWordListDesc[] = {
    "Keywords",
    nullptr
};

struct OptionSetNim : public OptionSet<OptionsNim> {
    OptionSetNim() {
        DefineProperty("lexer.nim.raw.strings.highlight.ident", &OptionsNim::highlightRawStrIdent,
            "Set to 1 to enable highlighting generalized raw string identifiers. "
            "Generalized raw string identifiers are anything other than r (or R).");

        DefineProperty("fold", &OptionsNim::fold);
        DefineProperty("fold.compact", &OptionsNim::foldCompact);

        DefineWordListSets(nimWordListDesc);
    }
};

LexicalClass lexicalClasses[] = {
    // Lexer Nim SCLEX_NIM SCE_NIM_:
    0,  "SCE_NIM_DEFAULT",        "default",              "White space",
    1,  "SCE_NIM_COMMENT",        "comment block",        "Block comment",
    2,  "SCE_NIM_COMMENTDOC",     "comment block doc",    "Block doc comment",
    3,  "SCE_NIM_COMMENTLINE",    "comment line",         "Line comment",
    4,  "SCE_NIM_COMMENTLINEDOC", "comment doc",          "Line doc comment",
    5,  "SCE_NIM_NUMBER",         "literal numeric",      "Number",
    6,  "SCE_NIM_STRING",         "literal string",       "String",
    7,  "SCE_NIM_CHARACTER",      "literal string",       "Single quoted string",
    8,  "SCE_NIM_WORD",           "keyword",              "Keyword",
    9,  "SCE_NIM_TRIPLE",         "literal string",       "Triple quotes",
    10, "SCE_NIM_TRIPLEDOUBLE",   "literal string",       "Triple double quotes",
    11, "SCE_NIM_BACKTICKS",      "operator definition",  "Identifiers",
    12, "SCE_NIM_FUNCNAME",       "identifier",           "Function name definition",
    13, "SCE_NIM_STRINGEOL",      "error literal string", "String is not closed",
    14, "SCE_NIM_NUMERROR",       "numeric error",        "Numeric format error",
    15, "SCE_NIM_OPERATOR",       "operator",             "Operators",
    16, "SCE_NIM_IDENTIFIER",     "identifier",           "Identifiers",
};

}

class LexerNim : public DefaultLexer {
    CharacterSet setWord;
    WordList keywords;
    OptionsNim options;
    OptionSetNim osNim;

public:
    LexerNim() :
        DefaultLexer("nim", SCLEX_NIM, lexicalClasses, ELEMENTS(lexicalClasses)),
        setWord(CharacterSet::setAlphaNum, "_", 0x80, true) { }

    virtual ~LexerNim() { }

    void SCI_METHOD Release() noexcept override {
        delete this;
    }

    int SCI_METHOD Version() const noexcept override {
        return lvRelease5;
    }

    const char * SCI_METHOD PropertyNames() override {
        return osNim.PropertyNames();
    }

    int SCI_METHOD PropertyType(const char *name) override {
        return osNim.PropertyType(name);
    }

    const char * SCI_METHOD DescribeProperty(const char *name) override {
        return osNim.DescribeProperty(name);
    }

    Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;

	const char * SCI_METHOD PropertyGet(const char* key) override {
		return osNim.PropertyGet(key);
	}

    const char * SCI_METHOD DescribeWordListSets() override {
        return osNim.DescribeWordListSets();
    }

    Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

    void * SCI_METHOD PrivateCall(int, void *) noexcept override {
        return nullptr;
    }

    int SCI_METHOD LineEndTypesSupported() noexcept override {
        return SC_LINE_END_TYPE_UNICODE;
    }

    int SCI_METHOD PrimaryStyleFromStyle(int style) noexcept override {
        return style;
    }

    static ILexer5 *LexerFactoryNim() {
        return new LexerNim();
    }
};

Sci_Position SCI_METHOD LexerNim::PropertySet(const char *key, const char *val) {
    if (osNim.PropertySet(&options, key, val)) {
        return 0;
    }

    return -1;
}

Sci_Position SCI_METHOD LexerNim::WordListSet(int n, const char *wl) {
    WordList *wordListN = nullptr;

    switch (n) {
        case 0:
            wordListN = &keywords;
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

void SCI_METHOD LexerNim::Lex(Sci_PositionU startPos, Sci_Position length,
                              int initStyle, IDocument *pAccess) {
    // No one likes a leaky string
    if (initStyle == SCE_NIM_STRINGEOL) {
        initStyle = SCE_NIM_DEFAULT;
    }

    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    // Nim supports nested block comments!
    Sci_Position lineCurrent = styler.GetLine(startPos);
    int commentNestLevel = lineCurrent > 0 ? styler.GetLineState(lineCurrent - 1) : 0;

    int numType = NumType::Decimal;
    int decimalCount = 0;

    bool funcNameExists = false;
    bool isStylingRawString = false;
    bool isStylingRawStringIdent = false;

    for (; sc.More(); sc.Forward()) {
        if (sc.atLineStart) {
            if (sc.state == SCE_NIM_STRING) {
                sc.SetState(SCE_NIM_STRING);
            }

            lineCurrent = styler.GetLine(sc.currentPos);
            styler.SetLineState(lineCurrent, commentNestLevel);
        }

        // Handle string line continuation
        if (sc.ch == '\\' && (sc.chNext == '\n' || sc.chNext == '\r') &&
           (sc.state == SCE_NIM_STRING || sc.state == SCE_NIM_CHARACTER) && !isStylingRawString) {
            sc.Forward();

            if (sc.ch == '\r' && sc.chNext == '\n') {
                sc.Forward();
            }

            continue;
        }

        switch (sc.state) {
            case SCE_NIM_OPERATOR:
                funcNameExists = false;
                sc.SetState(SCE_NIM_DEFAULT);
                break;
            case SCE_NIM_NUMBER:
                // For a type suffix, such as 0x80'u8
                if (sc.ch == '\'') {
                    if (sc.chNext == 'i' || sc.chNext == 'I' || 
                        sc.chNext == 'u' || sc.chNext == 'U' ||
                        sc.chNext == 'f' || sc.chNext == 'F' || 
                        sc.chNext == 'd' || sc.chNext == 'D') {
                        sc.Forward(2);
                    }
                } else if (sc.ch == '.') {
                    if (IsADigit(sc.chNext)) {
                        sc.Forward();
                    } else if (numType <= NumType::Exponent) {
                        sc.SetState(SCE_NIM_OPERATOR);
                        break;
                    } else {
                        decimalCount++;

                        if (numType == NumType::Decimal) {
                            if (decimalCount <= 1 && !IsAWordChar(sc.chNext)) {
                                break;
                            }
                        } else if (numType == NumType::Hexadecimal) {
                            if (decimalCount <= 1 && IsADigit(sc.chNext, 16)) {
                                break;
                            }

                            sc.SetState(SCE_NIM_OPERATOR);
                            break;
                        }
                    }
                } else if (sc.ch == '_') {
                    // Accept only one underscore between digits
                    if (IsADigit(sc.chNext)) {
                        sc.Forward();
                    }
                } else if (numType == NumType::Decimal) {
                    if (sc.chPrev != '\'' && (sc.ch == 'e' || sc.ch == 'E')) {
                        numType = NumType::Exponent;

                        if (sc.chNext == '-' || sc.chNext == '+') {
                            sc.Forward();
                        }

                        break;
                    }

                    if (IsADigit(sc.ch)) {
                        break;
                    }
                } else if (numType == NumType::Hexadecimal) {
                    if (IsADigit(sc.ch, 16)) {
                        break;
                    }
                } else if (IsADigit(sc.ch)) {
                    if (numType == NumType::Exponent) {
                        break;
                    }

                    if (numType == NumType::Octal) {
                        // Accept only 0-7
                        if (sc.ch <= '7') {
                            break;
                        }
                    } else if (numType == NumType::Binary) {
                        // Accept only 0 and 1
                        if (sc.ch <= '1') {
                            break;
                        }
                    }

                    numType = NumType::FormatError;
                    break;
                }

                sc.ChangeState(GetNumStyle(numType));
                sc.SetState(SCE_NIM_DEFAULT);
                break;
            case SCE_NIM_IDENTIFIER:
                if (sc.ch == '.' || !IsAWordChar(sc.ch)) {
                    char s[100];
                    sc.GetCurrent(s, sizeof(s));
                    int style = SCE_NIM_IDENTIFIER;

                    if (keywords.InList(s) && !funcNameExists) {
                        // Prevent styling keywords if they are sub-identifiers
                        const Sci_Position segStart = styler.GetStartSegment() - 1;
                        if (segStart < 0 || styler.SafeGetCharAt(segStart, '\0') != '.') {
                            style = SCE_NIM_WORD;
                        }
                    } else if (funcNameExists) {
                        style = SCE_NIM_FUNCNAME;
                    }

                    sc.ChangeState(style);
                    sc.SetState(SCE_NIM_DEFAULT);

                    if (style == SCE_NIM_WORD) {
                        funcNameExists = IsFuncName(s);
                    } else {
                        funcNameExists = false;
                    }
                }

                if (IsAlphaNumeric(sc.ch) && sc.chNext == '\"') {
                    isStylingRawStringIdent = true;

                    if (options.highlightRawStrIdent) {
                        if (styler.SafeGetCharAt(sc.currentPos + 2) == '\"' &&
                            styler.SafeGetCharAt(sc.currentPos + 3) == '\"') {
                            sc.ChangeState(SCE_NIM_TRIPLEDOUBLE);
                        } else {
                            sc.ChangeState(SCE_NIM_STRING);
                        }
                    }

                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_FUNCNAME:
                if (sc.ch == '`') {
                    funcNameExists = false;
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                } else if (sc.atLineEnd) {
                    // Prevent leaking the style to the next line if not closed
                    funcNameExists = false;

                    sc.ChangeState(SCE_NIM_STRINGEOL);
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_COMMENT:
                if (sc.Match(']', '#')) {
                    if (commentNestLevel > 0) {
                        commentNestLevel--;
                    }

                    lineCurrent = styler.GetLine(sc.currentPos);
                    styler.SetLineState(lineCurrent, commentNestLevel);
                    sc.Forward();

                    if (commentNestLevel == 0) {
                        sc.ForwardSetState(SCE_NIM_DEFAULT);
                    }
                } else if (sc.Match('#', '[')) {
                    commentNestLevel++;
                    lineCurrent = styler.GetLine(sc.currentPos);
                    styler.SetLineState(lineCurrent, commentNestLevel);
                }
                break;
            case SCE_NIM_COMMENTDOC:
                if (sc.Match("]##")) {
                    if (commentNestLevel > 0) {
                        commentNestLevel--;
                    }

                    lineCurrent = styler.GetLine(sc.currentPos);
                    styler.SetLineState(lineCurrent, commentNestLevel);
                    sc.Forward(2);

                    if (commentNestLevel == 0) {
                        sc.ForwardSetState(SCE_NIM_DEFAULT);
                    }
                } else if (sc.Match("##[")) {
                    commentNestLevel++;
                    lineCurrent = styler.GetLine(sc.currentPos);
                    styler.SetLineState(lineCurrent, commentNestLevel);
                }
                break;
            case SCE_NIM_COMMENTLINE:
            case SCE_NIM_COMMENTLINEDOC:
                if (sc.atLineStart) {
                    sc.SetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_STRING:
                if (!isStylingRawStringIdent && !isStylingRawString && sc.ch == '\\') {
                    if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
                        sc.Forward();
                    }
                } else if (isStylingRawString && sc.ch == '\"' && sc.chNext == '\"') {
                    // Forward in situations such as r"a""bc\" so that "bc\" wouldn't be
                    // considered a string of its own
                    sc.Forward();
                } else if (sc.ch == '\"') {
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                } else if (sc.atLineEnd) {
                    sc.ChangeState(SCE_NIM_STRINGEOL);
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_CHARACTER:
                if (sc.ch == '\\') {
                    if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
                        sc.Forward();
                    }
                } else if (sc.ch == '\'') {
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                } else if (sc.atLineEnd) {
                    sc.ChangeState(SCE_NIM_STRINGEOL);
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_BACKTICKS:
                if (sc.ch == '`' ) {
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                } else if (sc.atLineEnd) {
                    sc.ChangeState(SCE_NIM_STRINGEOL);
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_TRIPLEDOUBLE:
                if (sc.Match(R"(""")")) {

                    // Outright forward all " after the closing """ as a triple double
                    //
                    // A valid example where this is needed is: """8 double quotes->""""""""
                    // You can have as many """ at the end as you wish, as long as the actual
                    // closing literal is there
                    while (sc.ch == '"') {
                        sc.Forward();
                    }

                    sc.SetState(SCE_NIM_DEFAULT);
                }
                break;
            case SCE_NIM_TRIPLE:
                if (sc.Match("'''")) {
                    sc.Forward(2);
                    sc.ForwardSetState(SCE_NIM_DEFAULT);
                }
                break;
        }

        if (sc.state == SCE_NIM_DEFAULT) {
            // Number
            if (IsADigit(sc.ch)) {
                sc.SetState(SCE_NIM_NUMBER);

                numType = NumType::Decimal;
                decimalCount = 0;

                if (sc.ch == '0') {
                    if (IsNumHex(sc)) {
                        numType = NumType::Hexadecimal;
                    } else if (IsNumBinary(sc)) {
                        numType = NumType::Binary;
                    } else if (IsNumOctal(sc)) {
                        numType = NumType::Octal;
                    }

                    if (numType != NumType::Decimal) {
                        sc.Forward();
                    }
                }
            }
            // Raw string
            else if (IsAlphaNumeric(sc.ch) && sc.chNext == '\"') {
                isStylingRawString = true;

                // Triple doubles can be raw strings too. How sweet
                if (styler.SafeGetCharAt(sc.currentPos + 2) == '\"' &&
                    styler.SafeGetCharAt(sc.currentPos + 3) == '\"') {
                    sc.SetState(SCE_NIM_TRIPLEDOUBLE);
                } else {
                    sc.SetState(SCE_NIM_STRING);
                }

                const int rawStrStyle = options.highlightRawStrIdent ? IsLetter(sc.ch) :
                                        (sc.ch == 'r' || sc.ch == 'R');

                if (rawStrStyle) {
                    sc.Forward();

                    if (sc.state == SCE_NIM_TRIPLEDOUBLE) {
                        sc.Forward(2);
                    }
                } else {
                    // Anything other than r/R is considered a general raw string identifier
                    isStylingRawStringIdent = true;
                    sc.SetState(SCE_NIM_IDENTIFIER);
                }
            }
            // String and triple double literal
            else if (sc.ch == '\"') {
                isStylingRawString = false;

                if (sc.Match(R"(""")")) {
                    sc.SetState(SCE_NIM_TRIPLEDOUBLE);
                    
                    // Keep forwarding until the total opening literal count is 5
                    // A valid example where this is needed is: """""<-5 double quotes"""
                    while (sc.ch == '"') {
                        sc.Forward();

                        if (sc.Match(R"(""")")) {
                            sc.Forward();
                            break;
                        }
                    }
                } else {
                    sc.SetState(SCE_NIM_STRING);
                }
            }
            // Charecter and triple literal
            else if (sc.ch == '\'') {
                if (sc.Match("'''")) {
                    sc.SetState(SCE_NIM_TRIPLE);
                } else {
                    sc.SetState(SCE_NIM_CHARACTER);
                }
            }
            // Operator definition
            else if (sc.ch == '`') {
                if (funcNameExists) {
                    sc.SetState(SCE_NIM_FUNCNAME);
                } else {
                    sc.SetState(SCE_NIM_BACKTICKS);
                }
            }
            // Keyword
            else if (iswordstart(sc.ch)) {
                sc.SetState(SCE_NIM_IDENTIFIER);
            }
            // Comments
            else if (sc.ch == '#') {
                if (sc.Match("##[") || sc.Match("#[")) {
                    commentNestLevel++;
                    lineCurrent = styler.GetLine(sc.currentPos);
                    styler.SetLineState(lineCurrent, commentNestLevel);
                }

                if (sc.Match("##[")) {
                    sc.SetState(SCE_NIM_COMMENTDOC);
                    sc.Forward();
                } else if (sc.Match("#[")) {
                    sc.SetState(SCE_NIM_COMMENT);
                    sc.Forward();
                } else if (sc.Match("##")) {
                    sc.SetState(SCE_NIM_COMMENTLINEDOC);
                } else {
                    sc.SetState(SCE_NIM_COMMENTLINE);
                }
            }
            // Operators
            else if (strchr("()[]{}:=;-\\/&%$!+<>|^?,.*~@", sc.ch)) {
                sc.SetState(SCE_NIM_OPERATOR);
            }
        }

        if (sc.atLineEnd) {
            funcNameExists = false;
            isStylingRawString = false;
            isStylingRawStringIdent = false;
        }
    }

    sc.Complete();
}

void SCI_METHOD LexerNim::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess) {
    if (!options.fold) {
        return;
    }

    Accessor styler(pAccess, nullptr);

    const Sci_Position docLines = styler.GetLine(styler.Length());
    const Sci_Position maxPos = startPos + length;
    const Sci_Position maxLines = styler.GetLine(maxPos == styler.Length() ? maxPos : maxPos - 1);

    Sci_Position lineCurrent = styler.GetLine(startPos);
    int indentCurrent = IndentAmount(lineCurrent, styler);

    while (lineCurrent > 0) {
        lineCurrent--;
        indentCurrent = IndentAmount(lineCurrent, styler);

        if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
            break;
        }
    }

    int indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;
    indentCurrent = indentCurrentLevel | (indentCurrent & ~SC_FOLDLEVELNUMBERMASK);

    while (lineCurrent <= docLines && lineCurrent <= maxLines) {
        Sci_Position lineNext = lineCurrent + 1;
        int indentNext = indentCurrent;
        int lev = indentCurrent;

        if (lineNext <= docLines) {
            indentNext = IndentAmount(lineNext, styler);
        }

        if (indentNext & SC_FOLDLEVELWHITEFLAG) {
            indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;
        }

        while (lineNext < docLines && (indentNext & SC_FOLDLEVELWHITEFLAG)) {
            lineNext++;
            indentNext = IndentAmount(lineNext, styler);
        }

        const int indentNextLevel = indentNext & SC_FOLDLEVELNUMBERMASK;
        indentNext = indentNextLevel | (indentNext & ~SC_FOLDLEVELNUMBERMASK);

        const int levelBeforeComments = std::max(indentCurrentLevel, indentNextLevel);

        Sci_Position skipLine = lineNext;
        int skipLevel = indentNextLevel;

        while (--skipLine > lineCurrent) {
            const int skipLineIndent = IndentAmount(skipLine, styler);

            if (options.foldCompact) {
                if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > indentNextLevel) {
                    skipLevel = levelBeforeComments;
                }

                const int whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;
                styler.SetLevel(skipLine, skipLevel | whiteFlag);
            } else {
                if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > indentNextLevel &&
                   !(skipLineIndent & SC_FOLDLEVELWHITEFLAG)) {
                    skipLevel = levelBeforeComments;
                }

                styler.SetLevel(skipLine, skipLevel);
            }
        }

        if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
            if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
                lev |= SC_FOLDLEVELHEADERFLAG;
            }
        }

        styler.SetLevel(lineCurrent, options.foldCompact ? lev : lev & ~SC_FOLDLEVELWHITEFLAG);

        indentCurrent = indentNext;
        indentCurrentLevel = indentNextLevel;
        lineCurrent = lineNext;
    }
}

extern const LexerModule lmNim(SCLEX_NIM, LexerNim::LexerFactoryNim, "nim", nimWordListDesc);