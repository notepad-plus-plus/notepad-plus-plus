// Scintilla source code edit control
/** @file LexCIL.cxx
 ** Lexer for Common Intermediate Language
 ** Written by Jad Altahan (github.com/xv)
 ** CIL manual: https://www.ecma-international.org/publications/standards/Ecma-335.htm
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
#include <map>
#include <algorithm>

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

namespace {
    // Use an unnamed namespace to protect the functions and classes from name conflicts

bool IsAWordChar(const int ch) {
    return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.');
}

bool IsOperator(const int ch) {
    if ((ch < 0x80) && (isalnum(ch)))
        return false;

    if (strchr("!%&*+-/<=>@^|~()[]{}", ch)) {
        return true;
    }

    return false;
}

constexpr bool IsStreamCommentStyle(const int style) noexcept {
    return style == SCE_CIL_COMMENT;
}

struct OptionsCIL {
    bool fold;
    bool foldComment;
    bool foldCommentMultiline;
    bool foldCompact;

    OptionsCIL() {
        fold = true;
        foldComment = false;
        foldCommentMultiline = true;
        foldCompact = true;
    }
};

static const char *const cilWordListDesc[] = {
    "Primary CIL keywords",
    "Metadata",
    "Opcode instructions",
    0
};

struct OptionSetCIL : public OptionSet<OptionsCIL> {
    OptionSetCIL() {
        DefineProperty("fold", &OptionsCIL::fold);
        DefineProperty("fold.comment", &OptionsCIL::foldComment);

        DefineProperty("fold.cil.comment.multiline", &OptionsCIL::foldCommentMultiline,
            "Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

        DefineProperty("fold.compact", &OptionsCIL::foldCompact);

        DefineWordListSets(cilWordListDesc);
    }
};

LexicalClass lexicalClasses[] = {
    // Lexer CIL SCLEX_CIL SCE_CIL_:
    0,  "SCE_CIL_DEFAULT",     "default",              "White space",
    1,  "SCE_CIL_COMMENT",     "comment",              "Multi-line comment",
    2,  "SCE_CIL_COMMENTLINE", "comment line",         "Line comment",
    3,  "SCE_CIL_WORD",        "keyword",              "Keyword 1",
    4,  "SCE_CIL_WORD2",       "keyword",              "Keyword 2",
    5,  "SCE_CIL_WORD3",       "keyword",              "Keyword 3",
    6,  "SCE_CIL_STRING",      "literal string",       "Double quoted string",
    7,  "SCE_CIL_LABEL",       "label",                "Code label",
    8,  "SCE_CIL_OPERATOR",    "operator",             "Operators",
    9,  "SCE_CIL_STRINGEOL",   "error literal string", "String is not closed",
    10, "SCE_CIL_IDENTIFIER",  "identifier",           "Identifiers",
};

}

class LexerCIL : public DefaultLexer {
    WordList keywords, keywords2, keywords3;
    OptionsCIL options;
    OptionSetCIL osCIL;

public:
    LexerCIL() : DefaultLexer("cil", SCLEX_CIL, lexicalClasses, ELEMENTS(lexicalClasses)) { }

    virtual ~LexerCIL() { }

    void SCI_METHOD Release() override {
        delete this;
    }

    int SCI_METHOD Version() const override {
        return lvRelease5;
    }

    const char * SCI_METHOD PropertyNames() override {
        return osCIL.PropertyNames();
    }

    int SCI_METHOD PropertyType(const char *name) override {
        return osCIL.PropertyType(name);
    }

    const char * SCI_METHOD DescribeProperty(const char *name) override {
        return osCIL.DescribeProperty(name);
    }

    Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;

	const char * SCI_METHOD PropertyGet(const char* key) override {
		return osCIL.PropertyGet(key);
	}

    const char * SCI_METHOD DescribeWordListSets() override {
        return osCIL.DescribeWordListSets();
    }

    Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

    void * SCI_METHOD PrivateCall(int, void *) override {
        return 0;
    }

    int SCI_METHOD LineEndTypesSupported() override {
        return SC_LINE_END_TYPE_UNICODE;
    }

    int SCI_METHOD PrimaryStyleFromStyle(int style) override {
        return style;
    }

    static ILexer5 *LexerFactoryCIL() {
        return new LexerCIL();
    }
};

Sci_Position SCI_METHOD LexerCIL::PropertySet(const char *key, const char *val) {
    if (osCIL.PropertySet(&options, key, val)) {
        return 0;
    }

    return -1;
}

Sci_Position SCI_METHOD LexerCIL::WordListSet(int n, const char *wl) {
    WordList *wordListN = 0;

    switch (n) {
        case 0:
            wordListN = &keywords;
            break;
        case 1:
            wordListN = &keywords2;
            break;
        case 2:
            wordListN = &keywords3;
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

void SCI_METHOD LexerCIL::Lex(Sci_PositionU startPos, Sci_Position length,
                              int initStyle, IDocument *pAccess) {
    if (initStyle == SCE_CIL_STRINGEOL) {
        initStyle = SCE_CIL_DEFAULT;
    }

    Accessor styler(pAccess, NULL);
    StyleContext sc(startPos, length, initStyle, styler);

    bool identAtLineStart = false, // Checks if an identifier is at line start (ignoring spaces)
         canStyleLabels = false;   // Checks if conditions are met to style SCE_CIL_LABEL

    for (; sc.More(); sc.Forward()) {
        if (sc.atLineStart) {
            if (sc.state == SCE_CIL_STRING) {
                sc.SetState(SCE_CIL_STRING);
            }

            identAtLineStart = true;
        }

        // Handle string line continuation
        if (sc.ch == '\\' && (sc.chNext == '\n' || sc.chNext == '\r') &&
           (sc.state == SCE_CIL_STRING)) {
            sc.Forward();

            if (sc.ch == '\r' && sc.chNext == '\n') {
                sc.Forward();
            }

            continue;
        }

        switch (sc.state) {
            case SCE_CIL_OPERATOR:
                sc.SetState(SCE_CIL_DEFAULT);
                break;
            case SCE_CIL_IDENTIFIER:
                if (!IsAWordChar(sc.ch)) {
                    if (canStyleLabels && (sc.ch == ':' && sc.chNext != ':')) {
                        sc.ChangeState(SCE_CIL_LABEL);
                        sc.ForwardSetState(SCE_CIL_DEFAULT);
                    } else {
                        char kwSize[100];
                        sc.GetCurrent(kwSize, sizeof(kwSize));
                        int style = SCE_CIL_IDENTIFIER;

                        if (keywords.InList(kwSize)) {
                            style = SCE_CIL_WORD;
                        } else if (keywords2.InList(kwSize)) {
                            style = SCE_CIL_WORD2;
                        } else if (keywords3.InList(kwSize)) {
                            style = SCE_CIL_WORD3;
                        }

                        sc.ChangeState(style);
                        sc.SetState(SCE_CIL_DEFAULT);
                    }
                }
                break;
            case SCE_CIL_COMMENT:
                if (sc.Match('*', '/')) {
                    sc.Forward();
                    sc.ForwardSetState(SCE_CIL_DEFAULT);
                }
                break;
            case SCE_CIL_COMMENTLINE:
                if (sc.atLineStart) {
                    sc.SetState(SCE_CIL_DEFAULT);
                }
                break;
            case SCE_CIL_STRING:
                if (sc.ch == '\\') {
                    if (sc.chNext == '"' || sc.chNext == '\\') {
                        sc.Forward();
                    }
                } else if (sc.ch == '"') {
                    sc.ForwardSetState(SCE_CIL_DEFAULT);
                } else if (sc.atLineEnd) {
                    sc.ChangeState(SCE_CIL_STRINGEOL);
                    sc.ForwardSetState(SCE_CIL_DEFAULT);
                }
                break;
        }

        if (sc.state == SCE_CIL_DEFAULT) {
            // String
            if (sc.ch == '"') {
                sc.SetState(SCE_CIL_STRING);
            }
            // Keyword
            else if (IsAWordChar(sc.ch)) {
                // Allow setting SCE_CIL_LABEL style only if the label is the
                // first token in the line and does not start with a dot or a digit
                canStyleLabels = identAtLineStart && !(sc.ch == '.' || IsADigit(sc.ch));
                sc.SetState(SCE_CIL_IDENTIFIER);
            }
            // Multi-line comment
            else if (sc.Match('/', '*')) {
                sc.SetState(SCE_CIL_COMMENT);
                sc.Forward();
            }
            // Line comment
            else if (sc.Match('/', '/')) {
                sc.SetState(SCE_CIL_COMMENTLINE);
            }
            // Operators
            else if (IsOperator(sc.ch)) {
                sc.SetState(SCE_CIL_OPERATOR);
            }
        }

        if (!IsASpace(sc.ch)) {
            identAtLineStart = false;
        }
    }

    sc.Complete();
}

void SCI_METHOD LexerCIL::Fold(Sci_PositionU startPos, Sci_Position length, 
                               int initStyle, IDocument *pAccess) {
    if (!options.fold) {
        return;
    }

    LexAccessor styler(pAccess);

    const Sci_PositionU endPos = startPos + length;
    Sci_Position lineCurrent = styler.GetLine(startPos);

    int levelCurrent = SC_FOLDLEVELBASE;
    if (lineCurrent > 0)
        levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
    
    int style = initStyle;
    int styleNext = styler.StyleAt(startPos);
    int levelNext = levelCurrent;
    int visibleChars = 0;

    char chNext = styler[startPos];

    for (Sci_PositionU i = startPos; i < endPos; i++) {
        const char ch = chNext;
        int stylePrev = style;

        chNext = styler.SafeGetCharAt(i + 1);
        style = styleNext;
        styleNext = styler.StyleAt(i + 1);

        const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

        if (options.foldComment && 
            options.foldCommentMultiline && IsStreamCommentStyle(style)) {
            if (!IsStreamCommentStyle(stylePrev)) {
                levelNext++;
            } else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
                levelNext--;
            }
        }

        if (style == SCE_CIL_OPERATOR) {
            if (ch == '{') {
                levelNext++;
            } else if (ch == '}') {
                levelNext--;
            }
        }

        if (!IsASpace(ch)) {
            visibleChars++;
        }

        if (atEOL || (i == endPos - 1)) {
            int lev = levelCurrent | levelNext << 16;
            if (visibleChars == 0 && options.foldCompact)
                lev |= SC_FOLDLEVELWHITEFLAG;
            if (levelCurrent < levelNext)
                lev |= SC_FOLDLEVELHEADERFLAG;
            if (lev != styler.LevelAt(lineCurrent)) {
                styler.SetLevel(lineCurrent, lev);
            }
            
            lineCurrent++;
            levelCurrent = levelNext;
            
            if (options.foldCompact &&
                i == static_cast<Sci_PositionU>(styler.Length() - 1)) {
                styler.SetLevel(lineCurrent, lev | SC_FOLDLEVELWHITEFLAG);
            }

            visibleChars = 0;
        }
    }
}

LexerModule lmCIL(SCLEX_CIL, LexerCIL::LexerFactoryCIL, "cil", cilWordListDesc);