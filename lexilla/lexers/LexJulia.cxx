// Scintilla source code edit control
// Encoding: UTF-8
/** @file LexJulia.cxx
 ** Lexer for Julia.
 ** Reusing code from LexMatlab, LexPython and LexRust
 **
 ** Written by Bertrand Lacoste
 **
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "PropSetSimple.h"
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

static const int MAX_JULIA_IDENT_CHARS = 1023;

// Options used for LexerJulia
struct OptionsJulia {
    bool fold;
    bool foldComment;
    bool foldCompact;
    bool foldDocstring;
    bool foldSyntaxBased;
    bool highlightTypeannotation;
    bool highlightLexerror;
	OptionsJulia() {
        fold = true;
        foldComment = true;
        foldCompact = false;
        foldDocstring = true;
        foldSyntaxBased = true;
        highlightTypeannotation = false;
        highlightLexerror = false;
	}
};

const char * const juliaWordLists[] = {
    "Primary keywords and identifiers",
    "Built in types",
    "Other keywords",
    "Built in functions",
    0,
};

struct OptionSetJulia : public OptionSet<OptionsJulia> {
	OptionSetJulia() {
		DefineProperty("fold", &OptionsJulia::fold);

		DefineProperty("fold.compact", &OptionsJulia::foldCompact);

		DefineProperty("fold.comment", &OptionsJulia::foldComment);

		DefineProperty("fold.julia.docstring", &OptionsJulia::foldDocstring,
			"Fold multiline triple-doublequote strings, usually used to document a function or type above the definition.");

		DefineProperty("fold.julia.syntax.based", &OptionsJulia::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("lexer.julia.highlight.typeannotation", &OptionsJulia::highlightTypeannotation,
			"This option enables highlighting of the type identifier after `::`.");

		DefineProperty("lexer.julia.highlight.lexerror", &OptionsJulia::highlightLexerror,
			"This option enables highlighting of syntax error int character or number definition.");

		DefineWordListSets(juliaWordLists);
	}
};

LexicalClass juliaLexicalClasses[] = {
	// Lexer Julia SCLEX_JULIA SCE_JULIA_:
	0,  "SCE_JULIA_DEFAULT", "default", "White space",
	1,  "SCE_JULIA_COMMENT", "comment", "Comment",
	2,  "SCE_JULIA_NUMBER", "literal numeric", "Number",
	3,  "SCE_JULIA_KEYWORD1", "keyword", "Reserved keywords",
	4,  "SCE_JULIA_KEYWORD2", "identifier", "Builtin type names",
	5,  "SCE_JULIA_KEYWORD3", "identifier", "Constants",
	6,  "SCE_JULIA_CHAR", "literal string character", "Single quoted string",
	7,  "SCE_JULIA_OPERATOR", "operator", "Operator",
	8,  "SCE_JULIA_BRACKET", "bracket operator", "Bracket operator",
	9,  "SCE_JULIA_IDENTIFIER", "identifier", "Identifier",
	10, "SCE_JULIA_STRING", "literal string", "Double quoted String",
	11, "SCE_JULIA_SYMBOL", "literal string symbol", "Symbol",
	12, "SCE_JULIA_MACRO", "macro preprocessor", "Macro",
	13, "SCE_JULIA_STRINGINTERP", "literal string interpolated", "String interpolation",
	14, "SCE_JULIA_DOCSTRING", "literal string documentation", "Docstring",
	15, "SCE_JULIA_STRINGLITERAL", "literal string", "String literal prefix",
	16, "SCE_JULIA_COMMAND", "literal string command", "Command",
	17, "SCE_JULIA_COMMANDLITERAL", "literal string command", "Command literal prefix",
	18, "SCE_JULIA_TYPEANNOT", "identifier type", "Type annotation identifier",
	19, "SCE_JULIA_LEXERROR", "lexer error", "Lexing error",
	20, "SCE_JULIA_KEYWORD4", "identifier", "Builtin function names",
	21, "SCE_JULIA_TYPEOPERATOR", "operator type", "Type annotation operator",
};

class LexerJulia : public DefaultLexer {
	WordList keywords;
	WordList identifiers2;
	WordList identifiers3;
	WordList identifiers4;
	OptionsJulia options;
	OptionSetJulia osJulia;
public:
	explicit LexerJulia() :
		DefaultLexer("julia", SCLEX_JULIA, juliaLexicalClasses, ELEMENTS(juliaLexicalClasses)) {
	}
	virtual ~LexerJulia() {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osJulia.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osJulia.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osJulia.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osJulia.PropertyGet(key);
	}
	const char * SCI_METHOD DescribeWordListSets() override {
		return osJulia.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}

	static ILexer5 *LexerFactoryJulia() {
		return new LexerJulia();
	}
};

Sci_Position SCI_METHOD LexerJulia::PropertySet(const char *key, const char *val) {
	if (osJulia.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerJulia::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &identifiers2;
		break;
	case 2:
		wordListN = &identifiers3;
		break;
	case 3:
		wordListN = &identifiers4;
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

static inline bool IsJuliaOperator(int ch) {
    if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
        ch == '-' || ch == '+' || ch == '=' || ch == '|' ||
        ch == '<' || ch == '>' || ch == '/' || ch == '~' ||
        ch == '\\' ) {
        return true;
    }
    return false;
}

// The list contains non-ascii unary operators
static inline bool IsJuliaUnaryOperator (int ch) {
    if (ch == 0x00ac || ch == 0x221a || ch == 0x221b ||
        ch == 0x221c || ch == 0x22c6 || ch == 0x00b1 ||
        ch == 0x2213 ) {
        return true;
    }
    return false;
}

static inline bool IsJuliaParen (int ch) {
    if (ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
        ch == '[' || ch == ']' ) {
        return true;
    }
    return false;
}

// Unicode parsing from Julia source code:
// https://github.com/JuliaLang/julia/blob/master/src/flisp/julia_extensions.c
// keep the same function name to be easy to find again
static int is_wc_cat_id_start(uint32_t wc) {
    const CharacterCategory cat = CategoriseCharacter((int) wc);

    return (cat == ccLu || cat == ccLl ||
            cat == ccLt || cat == ccLm ||
            cat == ccLo || cat == ccNl ||
            cat == ccSc ||  // allow currency symbols
            // other symbols, but not arrows or replacement characters
            (cat == ccSo && !(wc >= 0x2190 && wc <= 0x21FF) &&
             wc != 0xfffc && wc != 0xfffd &&
             wc != 0x233f &&  // notslash
             wc != 0x00a6) || // broken bar

            // math symbol (category Sm) whitelist
            (wc >= 0x2140 && wc <= 0x2a1c &&
             ((wc >= 0x2140 && wc <= 0x2144) || // ⅀, ⅁, ⅂, ⅃, ⅄
              wc == 0x223f || wc == 0x22be || wc == 0x22bf || // ∿, ⊾, ⊿
              wc == 0x22a4 || wc == 0x22a5 ||   // ⊤ ⊥

              (wc >= 0x2200 && wc <= 0x2233 &&
               (wc == 0x2202 || wc == 0x2205 || wc == 0x2206 || // ∂, ∅, ∆
                wc == 0x2207 || wc == 0x220e || wc == 0x220f || // ∇, ∎, ∏
                wc == 0x2200 || wc == 0x2203 || wc == 0x2204 || // ∀, ∃, ∄
                wc == 0x2210 || wc == 0x2211 || // ∐, ∑
                wc == 0x221e || wc == 0x221f || // ∞, ∟
                wc >= 0x222b)) || // ∫, ∬, ∭, ∮, ∯, ∰, ∱, ∲, ∳

              (wc >= 0x22c0 && wc <= 0x22c3) ||  // N-ary big ops: ⋀, ⋁, ⋂, ⋃
              (wc >= 0x25F8 && wc <= 0x25ff) ||  // ◸, ◹, ◺, ◻, ◼, ◽, ◾, ◿

              (wc >= 0x266f &&
               (wc == 0x266f || wc == 0x27d8 || wc == 0x27d9 || // ♯, ⟘, ⟙
                (wc >= 0x27c0 && wc <= 0x27c1) ||  // ⟀, ⟁
                (wc >= 0x29b0 && wc <= 0x29b4) ||  // ⦰, ⦱, ⦲, ⦳, ⦴
                (wc >= 0x2a00 && wc <= 0x2a06) ||  // ⨀, ⨁, ⨂, ⨃, ⨄, ⨅, ⨆
                (wc >= 0x2a09 && wc <= 0x2a16) ||  // ⨉, ⨊, ⨋, ⨌, ⨍, ⨎, ⨏, ⨐, ⨑, ⨒, ⨓, ⨔, ⨕, ⨖
                wc == 0x2a1b || wc == 0x2a1c)))) || // ⨛, ⨜

            (wc >= 0x1d6c1 && // variants of \nabla and \partial
             (wc == 0x1d6c1 || wc == 0x1d6db ||
              wc == 0x1d6fb || wc == 0x1d715 ||
              wc == 0x1d735 || wc == 0x1d74f ||
              wc == 0x1d76f || wc == 0x1d789 ||
              wc == 0x1d7a9 || wc == 0x1d7c3)) ||

            // super- and subscript +-=()
            (wc >= 0x207a && wc <= 0x207e) ||
            (wc >= 0x208a && wc <= 0x208e) ||

            // angle symbols
            (wc >= 0x2220 && wc <= 0x2222) || // ∠, ∡, ∢
            (wc >= 0x299b && wc <= 0x29af) || // ⦛, ⦜, ⦝, ⦞, ⦟, ⦠, ⦡, ⦢, ⦣, ⦤, ⦥, ⦦, ⦧, ⦨, ⦩, ⦪, ⦫, ⦬, ⦭, ⦮, ⦯

            // Other_ID_Start
            wc == 0x2118 || wc == 0x212E || // ℘, ℮
            (wc >= 0x309B && wc <= 0x309C) || // katakana-hiragana sound marks

            // bold-digits and double-struck digits
            (wc >= 0x1D7CE && wc <= 0x1D7E1)); // 𝟎 through 𝟗 (inclusive), 𝟘 through 𝟡 (inclusive)
}

static inline bool IsIdentifierFirstCharacter (int ch) {
    if (IsASCII(ch)) {
        return (bool) (isalpha(ch) || ch == '_');
    }
    if (ch < 0xA1 || ch > 0x10ffff) {
        return false;
    }

    return is_wc_cat_id_start((uint32_t) ch);
}

static inline bool IsIdentifierCharacter (int ch) {
    if (IsASCII(ch)) {
        return (bool) (isalnum(ch) || ch == '_' || ch == '!');
    }
    if (ch < 0xA1 || ch > 0x10ffff) {
        return false;
    }

    if (is_wc_cat_id_start((uint32_t) ch)) {
        return true;
    }

    const CharacterCategory cat = CategoriseCharacter(ch);

    if (cat == ccMn || cat == ccMc ||
        cat == ccNd || cat == ccPc ||
        cat == ccSk || cat == ccMe ||
        cat == ccNo ||
        // primes (single, double, triple, their reverses, and quadruple)
        (ch >= 0x2032 && ch <= 0x2037) || (ch == 0x2057)) {
        return true;
    }
    return false;
}

// keep the same function name to be easy to find again
static const uint32_t opsuffs[] = {
   0x00b2, // ²
   0x00b3, // ³
   0x00b9, // ¹
   0x02b0, // ʰ
   0x02b2, // ʲ
   0x02b3, // ʳ
   0x02b7, // ʷ
   0x02b8, // ʸ
   0x02e1, // ˡ
   0x02e2, // ˢ
   0x02e3, // ˣ
   0x1d2c, // ᴬ
   0x1d2e, // ᴮ
   0x1d30, // ᴰ
   0x1d31, // ᴱ
   0x1d33, // ᴳ
   0x1d34, // ᴴ
   0x1d35, // ᴵ
   0x1d36, // ᴶ
   0x1d37, // ᴷ
   0x1d38, // ᴸ
   0x1d39, // ᴹ
   0x1d3a, // ᴺ
   0x1d3c, // ᴼ
   0x1d3e, // ᴾ
   0x1d3f, // ᴿ
   0x1d40, // ᵀ
   0x1d41, // ᵁ
   0x1d42, // ᵂ
   0x1d43, // ᵃ
   0x1d47, // ᵇ
   0x1d48, // ᵈ
   0x1d49, // ᵉ
   0x1d4d, // ᵍ
   0x1d4f, // ᵏ
   0x1d50, // ᵐ
   0x1d52, // ᵒ
   0x1d56, // ᵖ
   0x1d57, // ᵗ
   0x1d58, // ᵘ
   0x1d5b, // ᵛ
   0x1d5d, // ᵝ
   0x1d5e, // ᵞ
   0x1d5f, // ᵟ
   0x1d60, // ᵠ
   0x1d61, // ᵡ
   0x1d62, // ᵢ
   0x1d63, // ᵣ
   0x1d64, // ᵤ
   0x1d65, // ᵥ
   0x1d66, // ᵦ
   0x1d67, // ᵧ
   0x1d68, // ᵨ
   0x1d69, // ᵩ
   0x1d6a, // ᵪ
   0x1d9c, // ᶜ
   0x1da0, // ᶠ
   0x1da5, // ᶥ
   0x1da6, // ᶦ
   0x1dab, // ᶫ
   0x1db0, // ᶰ
   0x1db8, // ᶸ
   0x1dbb, // ᶻ
   0x1dbf, // ᶿ
   0x2032, // ′
   0x2033, // ″
   0x2034, // ‴
   0x2035, // ‵
   0x2036, // ‶
   0x2037, // ‷
   0x2057, // ⁗
   0x2070, // ⁰
   0x2071, // ⁱ
   0x2074, // ⁴
   0x2075, // ⁵
   0x2076, // ⁶
   0x2077, // ⁷
   0x2078, // ⁸
   0x2079, // ⁹
   0x207a, // ⁺
   0x207b, // ⁻
   0x207c, // ⁼
   0x207d, // ⁽
   0x207e, // ⁾
   0x207f, // ⁿ
   0x2080, // ₀
   0x2081, // ₁
   0x2082, // ₂
   0x2083, // ₃
   0x2084, // ₄
   0x2085, // ₅
   0x2086, // ₆
   0x2087, // ₇
   0x2088, // ₈
   0x2089, // ₉
   0x208a, // ₊
   0x208b, // ₋
   0x208c, // ₌
   0x208d, // ₍
   0x208e, // ₎
   0x2090, // ₐ
   0x2091, // ₑ
   0x2092, // ₒ
   0x2093, // ₓ
   0x2095, // ₕ
   0x2096, // ₖ
   0x2097, // ₗ
   0x2098, // ₘ
   0x2099, // ₙ
   0x209a, // ₚ
   0x209b, // ₛ
   0x209c, // ₜ
   0x2c7c, // ⱼ
   0x2c7d, // ⱽ
   0xa71b, // ꜛ
   0xa71c, // ꜜ
   0xa71d  // ꜝ
};
static const size_t opsuffs_len = sizeof(opsuffs) / (sizeof(uint32_t));

// keep the same function name to be easy to find again
static bool jl_op_suffix_char(uint32_t wc) {
    if (wc < 0xA1 || wc > 0x10ffff) {
        return false;
    }
    const CharacterCategory cat = CategoriseCharacter((int) wc);
    if (cat == ccMn || cat == ccMc ||
        cat == ccMe) {
        return true;
    }

    for (size_t i = 0; i < opsuffs_len; ++i) {
        if (wc == opsuffs[i]) {
            return true;
        }
    }
    return false;
}

// keep the same function name to be easy to find again
static bool never_id_char(uint32_t wc) {
     const CharacterCategory cat = CategoriseCharacter((int) wc);
     return (
          // spaces and control characters:
          (cat >= ccZs && cat <= ccCs) ||

          // ASCII and Latin1 non-connector punctuation
          (wc < 0xff &&
           cat >= ccPd && cat <= ccPo) ||

          wc == '`' ||

          // mathematical brackets
          (wc >= 0x27e6 && wc <= 0x27ef) ||
          // angle, corner, and lenticular brackets
          (wc >= 0x3008 && wc <= 0x3011) ||
          // tortoise shell, square, and more lenticular brackets
          (wc >= 0x3014 && wc <= 0x301b) ||
          // fullwidth parens
          (wc == 0xff08 || wc == 0xff09) ||
          // fullwidth square brackets
          (wc == 0xff3b || wc == 0xff3d));
}


static bool IsOperatorFirstCharacter (int ch) {
    if (IsASCII(ch)) {
        if (IsJuliaOperator(ch) ||
            ch == '!' || ch == '?' ||
            ch == ':' || ch == ';' ||
            ch == ',' || ch == '.' ) {
            return true;
        }else {
            return false;
        }
    } else if (is_wc_cat_id_start((uint32_t) ch)) {
        return false;
    } else if (IsJuliaUnaryOperator(ch) ||
               ! never_id_char((uint32_t) ch)) {
        return true;
    }
    return false;
}

static bool IsOperatorCharacter (int ch) {
    if (IsOperatorFirstCharacter(ch) ||
        (!IsASCII(ch) && jl_op_suffix_char((uint32_t) ch)) ) {
        return true;
    }
    return false;
}

static bool CheckBoundsIndexing(char *str) {
    if (strcmp("begin", str) == 0 || strcmp("end", str) == 0 ) {
        return true;
    }
    return false;
}

static int CheckKeywordFoldPoint(char *str) {
    if (strcmp ("if", str) == 0 ||
        strcmp ("for", str) == 0 ||
        strcmp ("while", str) == 0 ||
        strcmp ("try", str) == 0 ||
        strcmp ("do", str) == 0 ||
        strcmp ("begin", str) == 0 ||
        strcmp ("let", str) == 0 ||
        strcmp ("baremodule", str) == 0 ||
        strcmp ("quote", str) == 0 ||
        strcmp ("module", str) == 0 ||
        strcmp ("struct", str) == 0 ||
        strcmp ("type", str) == 0 ||
        strcmp ("macro", str) == 0 ||
        strcmp ("function", str) == 0) {
        return 1;
    }
    if (strcmp("end", str) == 0) {
        return -1;
    }
    return 0;
}

static bool IsNumberExpon(int ch, int base) {
    if ((base == 10 && (ch == 'e' || ch == 'E' || ch == 'f')) ||
        (base == 16 && (ch == 'p' || ch == 'P'))) {
        return true;
    }
    return false;
}

/* Scans a sequence of digits, returning true if it found any. */
static bool ScanDigits(StyleContext& sc, int base, bool allow_sep) {
	bool found = false;
    for (;;) {
		if (IsADigit(sc.chNext, base) || (allow_sep && sc.chNext == '_')) {
			found = true;
            sc.Forward();
		} else {
			break;
        }
	}
	return found;
}

static inline bool ScanNHexas(StyleContext &sc, int max) {
    int n = 0;
    bool error = false;

    sc.Forward();
    if (!IsADigit(sc.ch, 16)) {
        error = true;
    } else {
        while (IsADigit(sc.ch, 16) && n < max) {
            sc.Forward();
            n++;
        }
    }
    return error;
}

static void resumeCharacter(StyleContext &sc, bool lexerror) {
    bool error = false;

    //  ''' case
    if (sc.chPrev == '\'' && sc.ch == '\'' && sc.chNext == '\'') {
        sc.Forward();
        sc.ForwardSetState(SCE_JULIA_DEFAULT);
        return;
    } else if (lexerror && sc.chPrev == '\'' && sc.ch == '\'') {
        sc.ChangeState(SCE_JULIA_LEXERROR);
        sc.ForwardSetState(SCE_JULIA_DEFAULT);

    // Escape characters
    } else if (sc.ch == '\\') {
        sc.Forward();
        if (sc.ch == '\'' || sc.ch == '\\' ) {
            sc.Forward();
        } else if (sc.ch == 'n' || sc.ch == 't' || sc.ch == 'a' ||
                   sc.ch == 'b' || sc.ch == 'e' || sc.ch == 'f' ||
                   sc.ch == 'r' || sc.ch == 'v' ) {
            sc.Forward();
        } else if (sc.ch == 'x') {
            error |= ScanNHexas(sc, 2);
        } else if (sc.ch == 'u') {
            error |= ScanNHexas(sc, 4);
        } else if (sc.ch == 'U') {
            error |= ScanNHexas(sc, 8);
        } else if (IsADigit(sc.ch, 8)) {
            int n = 1;
            int max = 3;
            sc.Forward();
            while (IsADigit(sc.ch, 8) && n < max) {
                sc.Forward();
                n++;
            }
        }

        if (lexerror) {
            if (sc.ch != '\'') {
                error = true;
                while (sc.ch != '\'' &&
                       sc.ch != '\r' &&
                       sc.ch != '\n') {
                    sc.Forward();
                }
            }

            if (error) {
                sc.ChangeState(SCE_JULIA_LEXERROR);
                sc.ForwardSetState(SCE_JULIA_DEFAULT);
            }
        }
    } else if (lexerror) {
        if (sc.ch < 0x20 || sc.ch > 0x10ffff) {
            error = true;
        } else {
            // single character
            sc.Forward();

            if (sc.ch != '\'') {
                error = true;
                while (sc.ch != '\'' &&
                       sc.ch != '\r' &&
                       sc.ch != '\n') {
                    sc.Forward();
                }
            }
        }

        if (error) {
            sc.ChangeState(SCE_JULIA_LEXERROR);
            sc.ForwardSetState(SCE_JULIA_DEFAULT);
        }
    }

    // closing quote
    if (sc.ch == '\'') {
        if (sc.chNext == '\'') {
            sc.Forward();
        } else {
            sc.ForwardSetState(SCE_JULIA_DEFAULT);
        }
    }
}

static inline bool IsACharacter(StyleContext &sc) {
    return (sc.chPrev == '\'' && sc.chNext == '\'');
}

static void ScanParenInterpolation(StyleContext &sc) {
    // TODO: no syntax highlighting inside a string interpolation

    // Level of nested parenthesis
    int interp_level = 0;

    // If true, it is inside a string and parenthesis are not counted.
    bool allow_paren_string = false;


    // check for end of states
    for (; sc.More(); sc.Forward()) {
        // TODO: check corner cases for nested string interpolation
        // TODO: check corner cases with Command inside interpolation

        if ( sc.ch == '\"' && sc.chPrev != '\\') {
            // Toggle the string environment (parenthesis are not counted inside a string)
            allow_paren_string = !allow_paren_string;
        } else if ( !allow_paren_string ) {
            if ( sc.ch == '(' && !IsACharacter(sc) ) {
                interp_level ++;
            } else if ( sc.ch == ')' && !IsACharacter(sc) && interp_level > 0 ) {
                interp_level --;
                if (interp_level == 0) {
                    // Exit interpolation
                    return;
                }
            }
        }
    }
}
/*
 * Start parsing a number, parse the base.
 */
static void initNumber (StyleContext &sc, int &base, bool &with_dot) {
    base = 10;
    with_dot = false;
    sc.SetState(SCE_JULIA_NUMBER);
    if (sc.ch == '0') {
        if (sc.chNext == 'x') {
            sc.Forward();
            base = 16;
            if (sc.chNext == '.') {
                sc.Forward();
                with_dot = true;
            }
        } else if (sc.chNext == 'o') {
            sc.Forward();
            base = 8;
        } else if (sc.chNext == 'b') {
            sc.Forward();
            base = 2;
        }
    } else if (sc.ch == '.') {
        with_dot = true;
    }
}

/*
 * Resume parsing a String or Command, bounded by the `quote` character (\" or \`)
 * The `triple` argument specifies if it is a triple-quote String or Command.
 * Interpolation is detected (with `$`), and parsed if `allow_interp` is true.
 */
static void resumeStringLike(StyleContext &sc, int quote, bool triple, bool allow_interp, bool full_highlight) {
    int stylePrev = sc.state;
    bool checkcurrent = false;

    // Escape characters
    if (sc.ch == '\\') {
        if (sc.chNext == quote || sc.chNext == '\\' || sc.chNext == '$') {
            sc.Forward();
        }
    } else if (allow_interp && sc.ch == '$') {
        // If the interpolation is only of a variable, do not change state
        if (sc.chNext == '(') {
            if (full_highlight) {
                sc.SetState(SCE_JULIA_STRINGINTERP);
            } else {
                sc.ForwardSetState(SCE_JULIA_STRINGINTERP);
            }
            ScanParenInterpolation(sc);
            sc.ForwardSetState(stylePrev);

            checkcurrent = true;

        } else if (full_highlight && IsIdentifierFirstCharacter(sc.chNext)) {
            sc.SetState(SCE_JULIA_STRINGINTERP);
            sc.Forward();
            sc.Forward();
            for (; sc.More(); sc.Forward()) {
                if (! IsIdentifierCharacter(sc.ch)) {
                    break;
                }
            }
            sc.SetState(stylePrev);

            checkcurrent = true;
        }

        if (checkcurrent) {
            // Check that the current character is not a special char,
            // otherwise it will be skipped
            resumeStringLike(sc, quote, triple, allow_interp, full_highlight);
        }

    } else if (sc.ch == quote) {
        if (triple) {
            if (sc.chNext == quote && sc.GetRelativeCharacter(2) == quote) {
                // Move to the end of the triple quotes
                Sci_PositionU nextIndex = sc.currentPos + 2;
                while (nextIndex > sc.currentPos && sc.More()) {
                    sc.Forward();
                }
                sc.ForwardSetState(SCE_JULIA_DEFAULT);
            }
        } else {
            sc.ForwardSetState(SCE_JULIA_DEFAULT);
        }
    }
}

static void resumeCommand(StyleContext &sc, bool triple, bool allow_interp) {
    return resumeStringLike(sc, '`', triple, allow_interp, true);
}

static void resumeString(StyleContext &sc, bool triple, bool allow_interp) {
    return resumeStringLike(sc, '"', triple, allow_interp, true);
}

static void resumeNumber (StyleContext &sc, int base, bool &with_dot, bool lexerror) {
    if (IsNumberExpon(sc.ch, base)) {
        if (IsADigit(sc.chNext) || sc.chNext == '+' || sc.chNext == '-') {
            sc.Forward();
            // Capture all digits
            ScanDigits(sc, 10, false);
            sc.Forward();
        }
        sc.SetState(SCE_JULIA_DEFAULT);
    } else if (sc.ch == '.' && sc.chNext == '.') {
        // Interval operator `..`
        sc.SetState(SCE_JULIA_OPERATOR);
        sc.Forward();
        sc.ForwardSetState(SCE_JULIA_DEFAULT);
    } else if (sc.ch == '.' && !with_dot) {
        with_dot = true;
        ScanDigits(sc, base, true);
    } else if (IsADigit(sc.ch, base) || sc.ch == '_') {
        ScanDigits(sc, base, true);
    } else if (IsADigit(sc.ch) && !IsADigit(sc.ch, base)) {
        if (lexerror) {
            sc.ChangeState(SCE_JULIA_LEXERROR);
        }
        ScanDigits(sc, 10, false);
        sc.ForwardSetState(SCE_JULIA_DEFAULT);
    } else {
        sc.SetState(SCE_JULIA_DEFAULT);
    }
}

static void resumeOperator (StyleContext &sc) {
    if (sc.chNext == ':' && (sc.ch == ':' || sc.ch == '<' ||
                    (sc.ch == '>' && (sc.chPrev != '-' && sc.chPrev != '=')))) {
        // Case `:a=>:b`
        sc.Forward();
        sc.ForwardSetState(SCE_JULIA_DEFAULT);
    } else if (sc.ch == ':') {
        // Case `foo(:baz,:baz)` or `:one+:two`
        // Let the default case switch decide if it is a symbol
        sc.SetState(SCE_JULIA_DEFAULT);
    } else if (sc.ch == '\'') {
        sc.SetState(SCE_JULIA_DEFAULT);
    } else if ((sc.ch == '.' && sc.chPrev != '.') || IsIdentifierFirstCharacter(sc.ch) ||
               (! (sc.chPrev == '.' && IsOperatorFirstCharacter(sc.ch)) &&
                ! IsOperatorCharacter(sc.ch)) ) {
        sc.SetState(SCE_JULIA_DEFAULT);
    }
}

void SCI_METHOD LexerJulia::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	PropSetSimple props;
	Accessor styler(pAccess, &props);

	Sci_Position pos = startPos;
	styler.StartAt(pos);
	styler.StartSegment(pos);

    // use the line state of each line to store block/multiline states
    Sci_Position curLine = styler.GetLine(startPos);
    // Default is false for everything and 0 counters.
	int lineState = (curLine > 0) ? styler.GetLineState(curLine-1) : 0;

    bool transpose = (lineState >> 0) & 0x01;                // 1 bit to know if ' is allowed to mean transpose
    bool istripledocstring = (lineState >> 1) & 0x01;        // 1 bit to know if we are in a triple doublequotes string
	bool triple_backtick = (lineState >> 2) & 0x01;          // 1 bit to know if we are in a triple backtick command
	bool israwstring = (lineState >> 3) & 0x01;              // 1 bit to know if we are in a raw string
    int indexing_level = (int)((lineState >> 4) & 0x0F);     // 4 bits of bracket nesting counter
    int list_comprehension = (int)((lineState >> 8) & 0x0F); // 4 bits of parenthesis nesting counter
    int commentDepth = (int)((lineState >> 12) & 0x0F);      // 4 bits of nested comment counter

    // base for parsing number
    int base = 10;
    // number has a float dot ?
    bool with_dot = false;

    StyleContext sc(startPos, length, initStyle, styler);

    for (; sc.More(); sc.Forward()) {

        //// check for end of states
        switch (sc.state) {
            case SCE_JULIA_BRACKET:
                sc.SetState(SCE_JULIA_DEFAULT);
                break;
            case SCE_JULIA_OPERATOR:
                resumeOperator(sc);
                break;
            case SCE_JULIA_TYPEOPERATOR:
                sc.SetState(SCE_JULIA_DEFAULT);
                break;
            case SCE_JULIA_TYPEANNOT:
                if (! IsIdentifierCharacter(sc.ch)) {
                    sc.SetState(SCE_JULIA_DEFAULT);
                }
                break;
            case SCE_JULIA_IDENTIFIER:
                // String literal
                if (sc.ch == '\"') {
                    // If the string literal has a prefix, interpolation is disabled
                    israwstring = true;
                    sc.ChangeState(SCE_JULIA_STRINGLITERAL);
                    sc.SetState(SCE_JULIA_DEFAULT);

                } else if (sc.ch == '`') {
                    // If the string literal has a prefix, interpolation is disabled
                    israwstring = true;
                    sc.ChangeState(SCE_JULIA_COMMANDLITERAL);
                    sc.SetState(SCE_JULIA_DEFAULT);

                // Continue if the character is an identifier character
                } else if (! IsIdentifierCharacter(sc.ch)) {
                    char s[MAX_JULIA_IDENT_CHARS + 1];
                    sc.GetCurrent(s, sizeof(s));

                    // Treat the keywords differently if we are indexing or not
                    if ( indexing_level > 0 && CheckBoundsIndexing(s)) {
                        // Inside [], (), `begin` and `end` are numbers not block keywords
                        sc.ChangeState(SCE_JULIA_NUMBER);
                        transpose = false;

                    } else {
                        if (keywords.InList(s)) {
                            sc.ChangeState(SCE_JULIA_KEYWORD1);
                            transpose = false;
                        } else if (identifiers2.InList(s)) {
                            sc.ChangeState(SCE_JULIA_KEYWORD2);
                            transpose = false;
                        } else if (identifiers3.InList(s)) {
                            sc.ChangeState(SCE_JULIA_KEYWORD3);
                            transpose = false;
                        } else if (identifiers4.InList(s)) {
                            sc.ChangeState(SCE_JULIA_KEYWORD4);
                            // These identifiers can be used for variable names also,
                            // so transpose is not forbidden.
                            //transpose = false;
                        }
                    }
                    sc.SetState(SCE_JULIA_DEFAULT);

                    // TODO: recognize begin-end blocks inside list comprehension
                    // b = [(begin n%2; n*2 end) for n in 1:10]
                    // TODO: recognize better comprehension for-if to avoid problem with code-folding
                    // c = [(if isempty(a); missing else first(b) end) for (a, b) in zip(l1, l2)]
                }
                break;
            case SCE_JULIA_NUMBER:
                resumeNumber(sc, base, with_dot, options.highlightLexerror);
                break;
            case SCE_JULIA_CHAR:
                resumeCharacter(sc, options.highlightLexerror);
                break;
            case SCE_JULIA_DOCSTRING:
                resumeString(sc, true, !israwstring);
                if (sc.state == SCE_JULIA_DEFAULT && israwstring) {
                    israwstring = false;
                }
                break;
            case SCE_JULIA_STRING:
                resumeString(sc, false, !israwstring);
                if (sc.state == SCE_JULIA_DEFAULT && israwstring) {
                    israwstring = false;
                }
                break;
            case SCE_JULIA_COMMAND:
                resumeCommand(sc, triple_backtick, !israwstring);
                break;
            case SCE_JULIA_MACRO:
                if (IsASpace(sc.ch) || ! IsIdentifierCharacter(sc.ch)) {
                    sc.SetState(SCE_JULIA_DEFAULT);
                }
                break;
            case SCE_JULIA_SYMBOL:
                if (! IsIdentifierCharacter(sc.ch)) {
                    sc.SetState(SCE_JULIA_DEFAULT);
                }
                break;
            case SCE_JULIA_COMMENT:
                if( commentDepth > 0 ) {
                    // end or start of a nested a block comment
                    if ( sc.ch == '=' && sc.chNext == '#') {
                        commentDepth --;
                        sc.Forward();

                        if (commentDepth == 0) {
                            sc.ForwardSetState(SCE_JULIA_DEFAULT);
                        }
                    } else if( sc.ch == '#' && sc.chNext == '=') {
                        commentDepth ++;
                        sc.Forward();
                    }
                } else {
                    // single line comment
                    if (sc.atLineEnd || sc.ch == '\r' || sc.ch == '\n') {
                        sc.SetState(SCE_JULIA_DEFAULT);
                        transpose = false;
                    }
                }
                break;
        }

        // check start of a new state
        if (sc.state == SCE_JULIA_DEFAULT) {
            if (sc.ch == '#') {
                sc.SetState(SCE_JULIA_COMMENT);
                // increment depth if we are a block comment
                if(sc.chNext == '=') {
                    commentDepth ++;
                    sc.Forward();
                }
            } else if (sc.ch == '!') {
                sc.SetState(SCE_JULIA_OPERATOR);
                transpose = false;
            } else if (sc.ch == '\'') {
                if (transpose) {
                    sc.SetState(SCE_JULIA_OPERATOR);
                } else {
                    sc.SetState(SCE_JULIA_CHAR);
                }
            } else if (sc.ch == '\"') {
                istripledocstring = (sc.chNext == '\"' && sc.GetRelativeCharacter(2) == '\"');
                if (istripledocstring) {
                    sc.SetState(SCE_JULIA_DOCSTRING);
                    // Move to the end of the triple quotes
                    Sci_PositionU nextIndex = sc.currentPos + 2;
                    while (nextIndex > sc.currentPos && sc.More()) {
                        sc.Forward();
                    }
                } else {
                    sc.SetState(SCE_JULIA_STRING);
                }
            } else if (sc.ch == '`') {
                triple_backtick = (sc.chNext == '`' && sc.GetRelativeCharacter(2) == '`');
                sc.SetState(SCE_JULIA_COMMAND);
                if (triple_backtick) {
                    // Move to the end of the triple backticks
                    Sci_PositionU nextIndex = sc.currentPos + 2;
                    while (nextIndex > sc.currentPos && sc.More()) {
                        sc.Forward();
                    }
                }
            } else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
                initNumber(sc, base, with_dot);
            } else if (IsIdentifierFirstCharacter(sc.ch)) {
                sc.SetState(SCE_JULIA_IDENTIFIER);
                transpose = true;
            } else if (sc.ch == '@') {
                sc.SetState(SCE_JULIA_MACRO);
                transpose = false;

            // Several parsing of operators, should keep the order of `if` blocks
            } else if ((sc.ch == ':' || sc.ch == '<' || sc.ch == '>') && sc.chNext == ':') {
                sc.SetState(SCE_JULIA_TYPEOPERATOR);
                sc.Forward();
                // Highlight the next identifier, if option is set
                if (options.highlightTypeannotation &&
                    IsIdentifierFirstCharacter(sc.chNext)) {
                    sc.ForwardSetState(SCE_JULIA_TYPEANNOT);
                }
            } else if (sc.ch == ':') {
                // TODO: improve detection of range
                // should be solved with begin-end parsing
                // `push!(arr, s1 :s2)` and `a[begin :end]
                if (IsIdentifierFirstCharacter(sc.chNext) &&
                    ! IsIdentifierCharacter(sc.chPrev) &&
                    sc.chPrev != ')' && sc.chPrev != ']' ) {
                    sc.SetState(SCE_JULIA_SYMBOL);
                } else {
                    sc.SetState(SCE_JULIA_OPERATOR);
                }
            } else if (IsJuliaParen(sc.ch)) {
                if (sc.ch == '[') {
                    list_comprehension ++;
                    indexing_level ++;
                } else if (sc.ch == ']' && (indexing_level > 0)) {
                    list_comprehension --;
                    indexing_level --;
                } else if (sc.ch == '(') {
                    list_comprehension ++;
                } else if (sc.ch == ')' && (list_comprehension > 0)) {
                    list_comprehension --;
                }

                if (sc.ch == ')' || sc.ch == ']' || sc.ch == '}') {
                    transpose = true;
                } else {
                    transpose = false;
                }
                sc.SetState(SCE_JULIA_BRACKET);
            } else if (IsOperatorFirstCharacter(sc.ch)) {
                transpose = false;
                sc.SetState(SCE_JULIA_OPERATOR);
            } else {
                transpose = false;
            }
        }

        // update the line information (used for line-by-line lexing and folding)
        if (sc.atLineEnd) {
            // set the line state to the current state
            curLine = styler.GetLine(sc.currentPos);

            lineState = ((transpose ? 1 : 0) << 0) |
                        ((istripledocstring ? 1 : 0) << 1) |
                        ((triple_backtick ? 1 : 0) << 2) |
                        ((israwstring ? 1 : 0) << 3) |
                        ((indexing_level & 0x0F) << 4) |
                        ((list_comprehension & 0x0F) << 8) |
                        ((commentDepth & 0x0F) << 12);
            styler.SetLineState(curLine, lineState);
        }
    }
    sc.Complete();
}

void SCI_METHOD LexerJulia::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
    int lineState = 0;
	if (lineCurrent > 0) {
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
        lineState = styler.GetLineState(lineCurrent-1);
    }

    // level of nested brackets
    int indexing_level = (int)((lineState >> 4) & 0x0F);     // 4 bits of bracket nesting counter
    // level of nested parenthesis or brackets
    int list_comprehension = (int)((lineState >> 8) & 0x0F); // 4 bits of parenthesis nesting counter
    //int commentDepth = (int)((lineState >> 12) & 0x0F);      // 4 bits of nested comment counter
    
	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent+1);
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int stylePrev = styler.StyleAt(startPos - 1);
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
    char word[100];
    int wordlen = 0;
    for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = i == (lineStartNext-1);

        // a start/end of comment block
        if (options.foldComment && style == SCE_JULIA_COMMENT) {
            // start of block comment
            if (ch == '#' && chNext == '=') {
                levelNext ++;
            }
            // end of block comment
            if (ch == '=' && chNext == '#' && levelNext > 0) {
                levelNext --;
            }
        }

        // Syntax based folding, accounts for list comprehension
        if (options.foldSyntaxBased) {
            // list comprehension allow `for`, `if` and `begin` without `end`
            if (style == SCE_JULIA_BRACKET) {
                if (ch == '[') {
                    list_comprehension ++;
                    indexing_level ++;
                    levelNext ++;
                } else if (ch == ']') {
                    list_comprehension --;
                    indexing_level --;
                    levelNext --;
                } else if (ch == '(') {
                    list_comprehension ++;
                    levelNext ++;
                } else if (ch == ')') {
                    list_comprehension --;
                    levelNext --;
                }
                // check non-negative
                if (indexing_level < 0) {
                    indexing_level = 0;
                }
                if (list_comprehension < 0) {
                    list_comprehension = 0;
                }
            }

            // keyword
            if (style == SCE_JULIA_KEYWORD1) {
                word[wordlen++] = static_cast<char>(ch);
                if (wordlen == 100) {  // prevent overflow
                    word[0] = '\0';
                    wordlen = 1;
                }
                if (styleNext != SCE_JULIA_KEYWORD1) {
                    word[wordlen] = '\0';
                    wordlen = 0;
                    if (list_comprehension <= 0 && indexing_level <= 0) {
                        levelNext += CheckKeywordFoldPoint(word);
                    }
                }
            }
        }

        // Docstring
        if (options.foldDocstring) {
            if (stylePrev != SCE_JULIA_DOCSTRING && style == SCE_JULIA_DOCSTRING) {
                levelNext ++;
            } else if (style == SCE_JULIA_DOCSTRING && styleNext != SCE_JULIA_DOCSTRING) {
                levelNext --;
            }
        }

        // check non-negative level
        if (levelNext < 0) {
            levelNext = 0;
        }

        if (!IsASpace(ch)) {
            visibleChars++;
        }
        stylePrev = style;

        if (atEOL || (i == endPos-1)) {
            int levelUse = levelCurrent;
            int lev = levelUse | levelNext << 16;
            if (visibleChars == 0 && options.foldCompact) {
                lev |= SC_FOLDLEVELWHITEFLAG;
            }
            if (levelUse < levelNext) {
                lev |= SC_FOLDLEVELHEADERFLAG;
            }
            if (lev != styler.LevelAt(lineCurrent)) {
                styler.SetLevel(lineCurrent, lev);
            }
            lineCurrent++;
            lineStartNext = styler.LineStart(lineCurrent+1);
            levelCurrent = levelNext;
            if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length() - 1))) {
                // There is an empty line at end of file so give it same level and empty
                styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
            }
            visibleChars = 0;
        }
    }
}

LexerModule lmJulia(SCLEX_JULIA, LexerJulia::LexerFactoryJulia, "julia", juliaWordLists);
