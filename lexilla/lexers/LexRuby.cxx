// Scintilla source code edit control
/** @file LexRuby.cxx
 ** Lexer for Ruby.
 **/
// Copyright 2001- by Clemens Wyss <wys@helbling.ch>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <vector>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "InList.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SubStyles.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

//XXX Identical to Perl, put in common area
constexpr bool isEOLChar(char ch) noexcept {
    return (ch == '\r') || (ch == '\n');
}

constexpr bool isSafeASCII(char ch) noexcept {
    return static_cast<unsigned char>(ch) <= 127;
}

// This one's redundant, but makes for more readable code
constexpr bool isHighBitChar(char ch) noexcept {
    return static_cast<unsigned char>(ch) > 127;
}

bool isSafeAlpha(char ch) noexcept {
    return (isSafeASCII(ch) && isalpha(ch)) || ch == '_';
}

bool isSafeAlphaOrHigh(char ch) noexcept {
	return isHighBitChar(ch) || isalpha(ch) || ch == '_';
}

bool isSafeAlnum(char ch) noexcept {
    return (isSafeASCII(ch) && isalnum(ch)) || ch == '_';
}

bool isSafeAlnumOrHigh(char ch) noexcept {
    return isHighBitChar(ch) || isalnum(ch) || ch == '_';
}

bool isSafeDigit(char ch) noexcept {
    return isSafeASCII(ch) && isdigit(ch);
}

bool isSafeWordcharOrHigh(char ch) noexcept {
    // Error: scintilla's KeyWords.h includes '.' as a word-char
    // we want to separate things that can take methods from the
    // methods.
    return isHighBitChar(ch) || isalnum(ch) || ch == '_';
}

constexpr bool isWhiteSpace(char ch) noexcept {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool isQuestionMarkChar(char chNext, char chNext2) noexcept {
    // followed by a single character or escape sequence that corresponds to a single codepoint
    if (isSafeAlnum(chNext)) {
        return !isSafeWordcharOrHigh(chNext2);
    }
    // multibyte character, escape sequence, punctuation
    return !IsASpace(chNext);
}

// Options used for LexerRuby
struct OptionsRuby {
	bool foldCompact = true;
	bool foldComment = false;
};

const char *const rubyWordListDesc[] = {
    "Keywords",
    nullptr
};

struct OptionSetRuby : public OptionSet<OptionsRuby> {
	OptionSetRuby() {
		DefineProperty("fold.compact", &OptionsRuby::foldCompact);
		DefineProperty("fold.comment", &OptionsRuby::foldComment);

		DefineWordListSets(rubyWordListDesc);
	}
};

const char styleSubable[] = { SCE_RB_IDENTIFIER, 0 };

const LexicalClass lexicalClasses[] = {
	// Lexer ruby SCLEX_RUBY SCE_RB_
	0, "SCE_RB_DEFAULT", "default", "White space",
	1, "SCE_RB_ERROR", "error", "Error",
	2, "SCE_RB_COMMENTLINE", "comment", "Comment",
	3, "SCE_RB_POD", "data", "POD",
	4, "SCE_RB_NUMBER", "literal numeric", "Number",
	5, "SCE_RB_WORD", "keyword", "Keyword",
	6, "SCE_RB_STRING", "literal string", "Quoted string",
	7, "SCE_RB_CHARACTER", "literal string character", "Quoted string",
	8, "SCE_RB_CLASSNAME", "identifier", "Class name definition",
	9, "SCE_RB_DEFNAME", "identifier", "Function or method name definition",
	10, "SCE_RB_OPERATOR", "operator", "Operator",
	11, "SCE_RB_IDENTIFIER", "identifier", "Identifiers",
	12, "SCE_RB_REGEX", "literal regex", "RegEx",
	13, "SCE_RB_GLOBAL", "identifier", "Global",
	14, "SCE_RB_SYMBOL", "identifier symbol", "",
	15, "SCE_RB_MODULE_NAME", "identifier", "Module name",
	16, "SCE_RB_INSTANCE_VAR", "identifier", "Instance variable",
	17, "SCE_RB_CLASS_VAR", "identifier", "Class variable",
	18, "SCE_RB_BACKTICKS", "literal string interpolated", "Back ticks",
	19, "SCE_RB_DATASECTION", "data", "Data section",
	20, "SCE_RB_HERE_DELIM", "here-doc literal string", "Here-doc (delimiter)",
	21, "SCE_RB_HERE_Q", "here-doc literal string", "Here-doc (single quoted, q)",
	22, "SCE_RB_HERE_QQ", "here-doc literal string", "Here-doc (double quoted, qq)",
	23, "SCE_RB_HERE_QX", "here-doc literal string", "Here-doc (back ticks, qx)",
	24, "SCE_RB_STRING_Q", "literal string", "Single quoted string, generic",
	25, "SCE_RB_STRING_QQ", "literal string interpolated", "qq = double quoted string",
	26, "SCE_RB_STRING_QX", "literal string interpolated", "qx = back ticks",
	27, "SCE_RB_STRING_QR", "literal regex", "qr = regex",
	28, "SCE_RB_STRING_QW", "literal string interpolated", "qw = array",
	29, "SCE_RB_WORD_DEMOTED", "keyword", "Keyword demoted",
	30, "SCE_RB_STDIN", "file", "Standard input stream",
	31, "SCE_RB_STDOUT", "file", "Standard output stream",
	40, "SCE_RB_STDERR", "file", "Standard error stream",
	41, "SCE_RB_STRING_W", "literal string", "String array",
	42, "SCE_RB_STRING_I", "literal string", "Symbol array",
	43, "SCE_RB_STRING_QI", "literal string interpolated", "Interpolable symbol array",
	44, "SCE_RB_STRING_QS", "identifier symbol", "Symbol",
};

class LexerRuby : public DefaultLexer {
	WordList keywords;
	OptionsRuby options;
	OptionSetRuby osRuby;
	SubStyles subStyles{styleSubable};
public:
	LexerRuby() :
		DefaultLexer("ruby", SCLEX_RUBY, lexicalClasses, std::size(lexicalClasses)) {
	}
	// Deleted so LexerRuby objects can not be copied.
	LexerRuby(const LexerRuby &) = delete;
	LexerRuby(LexerRuby &&) = delete;
	void operator=(const LexerRuby &) = delete;
	void operator=(LexerRuby &&) = delete;
	~LexerRuby() override = default;

	const char *SCI_METHOD PropertyNames() override {
		return osRuby.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osRuby.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osRuby.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osRuby.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osRuby.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override {
		return subStyles.Allocate(styleBase, numberStyles);
	}
	int SCI_METHOD SubStylesStart(int styleBase) override {
		return subStyles.Start(styleBase);
	}
	int SCI_METHOD SubStylesLength(int styleBase) override {
		return subStyles.Length(styleBase);
	}
	int SCI_METHOD StyleFromSubStyle(int subStyle) override {
		const int styleBase = subStyles.BaseStyle(subStyle);
		return styleBase;
	}
	int SCI_METHOD PrimaryStyleFromStyle(int style) override {
		return style;
	}
	void SCI_METHOD FreeSubStyles() override {
		subStyles.Free();
	}
	void SCI_METHOD SetIdentifiers(int style, const char *identifiers) override {
		subStyles.SetIdentifiers(style, identifiers);
	}
	int SCI_METHOD DistanceToSecondaryStyles() override {
		return 0;
	}
	const char *SCI_METHOD GetSubStyleBases() override {
		return styleSubable;
	}

	static ILexer5 *LexerFactoryRuby() {
		return new LexerRuby();
	}
};

Sci_Position SCI_METHOD LexerRuby::PropertySet(const char *key, const char *val) {
	if (osRuby.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerRuby::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN && wordListN->Set(wl)) {
		firstModification = 0;
	}
	return firstModification;
}

bool followsDot(Sci_PositionU pos, Accessor &styler) {
    for (; pos >= 1; --pos) {
        const int style = styler.BufferStyleAt(pos);
        switch (style) {
        case SCE_RB_DEFAULT:
            if (IsASpaceOrTab(styler[pos])) {
                //continue
            } else {
                return false;
            }
            break;

        case SCE_RB_OPERATOR:
            return styler[pos] == '.';

        default:
            return false;
        }
    }
    return false;
}

constexpr bool IsIdentifierStyle(int style) noexcept {
    return style == SCE_RB_IDENTIFIER || style >= SubStylesFirst;
}

// Forward declarations
bool keywordIsAmbiguous(const std::string &prevWord) noexcept;
bool keywordDoStartsLoop(Sci_Position pos, Accessor &styler);
bool keywordIsModifier(const std::string &word, Sci_Position pos, Accessor &styler);

// pseudo style: prefer regex after identifier
#define SCE_RB_IDENTIFIER_PREFERRE  SCE_RB_UPPER_BOUND

int ClassifyWordRb(Sci_PositionU end, char ch, char chNext, const WordList &keywords, Accessor &styler, std::string &prevWord, const WordClassifier &idClasser) {
    const Sci_PositionU start = styler.GetStartSegment();
    const std::string s = styler.GetRange(start, end);
    int chAttr = SCE_RB_IDENTIFIER;
    int style = SCE_RB_DEFAULT;
    if (prevWord == "class")
        chAttr = SCE_RB_CLASSNAME;
    else if (prevWord == "module")
        chAttr = SCE_RB_MODULE_NAME;
    else if (prevWord == "def") {
        chAttr = SCE_RB_DEFNAME;
        if (ch == '.' || (ch == ':' && chNext == ':')) {
            if (s == "self") {
                style = SCE_RB_WORD_DEMOTED;
            } else {
                style = SCE_RB_IDENTIFIER;
                const int subStyle = idClasser.ValueFor(s);
                if (subStyle >= 0) {
                    style = subStyle;
                }
            }
        }
    } else if ((start == 0) || !followsDot(start - 1, styler)) {
        if (keywords.InList(s)) {
            if (keywordIsAmbiguous(s)
                && keywordIsModifier(s, start, styler)) {

                // Demoted keywords are colored as keywords,
                // but do not affect changes in indentation.
                //
                // Consider the word 'if':
                // 1. <<if test ...>> : normal
                // 2. <<stmt if test>> : demoted
                // 3. <<lhs = if ...>> : normal: start a new indent level
                // 4. <<obj.if = 10>> : color as identifier, since it follows '.'

                chAttr = SCE_RB_WORD_DEMOTED;
            } else {
                chAttr = SCE_RB_WORD;
                style = SCE_RB_WORD;
                prevWord = s;
            }
        } else {
            const int subStyle = idClasser.ValueFor(s);
            if (subStyle >= 0) {
                style = subStyle;
            }
        }
    }
    if (style == SCE_RB_DEFAULT) {
        style = chAttr;
        prevWord.clear();
    }
    styler.ColourTo(end - 1, style);

    if (chAttr == SCE_RB_IDENTIFIER) {
        // find heredoc in lib/ruby folder: rg "\w+\s+<<[\w\-~'\"`]"
        // Kernel methods
        if (InList(s, { "puts", "print", "warn", "eval" } )) {
            chAttr = SCE_RB_IDENTIFIER_PREFERRE;
        }
    }
    return chAttr;
}


//XXX Identical to Perl, put in common area
bool isMatch(Accessor &styler, Sci_Position lengthDoc, Sci_Position pos, const char *val) {
    if ((pos + static_cast<int>(strlen(val))) >= lengthDoc) {
        return false;
    }
    while (*val) {
        if (*val != styler[pos++]) {
            return false;
        }
        val++;
    }
    return true;
}

// Do Ruby better -- find the end of the line, work back,
// and then check for leading white space

// Precondition: the here-doc target can be indented
bool lookingAtHereDocDelim(Accessor &styler, Sci_Position pos, Sci_Position lengthDoc, const char *HereDocDelim) {
    if (!isMatch(styler, lengthDoc, pos, HereDocDelim)) {
        return false;
    }
    while (--pos > 0) {
        const char ch = styler[pos];
        if (isEOLChar(ch)) {
            return true;
        } else if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return false;
}

//XXX Identical to Perl, put in common area
constexpr char opposite(char ch) noexcept {
    if (ch == '(')
        return ')';
    if (ch == '[')
        return ']';
    if (ch == '{')
        return '}';
    if (ch == '<')
        return '>';
    return ch;
}

// Null transitions when we see we've reached the end
// and need to re-lex the curr char.

void redo_char(Sci_Position &i, char &ch, char &chNext, char &chNext2, int &state) noexcept {
    i--;
    chNext2 = chNext;
    chNext = ch;
    state = SCE_RB_DEFAULT;
}

void advance_char(Sci_Position &i, char &ch, char &chNext, char &chNext2) noexcept {
    i++;
    ch = chNext;
    chNext = chNext2;
}

// precondition: startPos points to one after the EOL char
bool currLineContainsHereDelims(Sci_Position &startPos, Accessor &styler) {
    if (startPos <= 1)
        return false;

    Sci_Position pos = startPos - 1;
    for (; pos > 0; pos--) {
        const char ch = styler.SafeGetCharAt(pos);
        if (isEOLChar(ch)) {
            // Leave the pointers where they are -- there are no
            // here doc delimiters on the current line, even if
            // the EOL isn't default style

            return false;
        }
        if (styler.StyleIndexAt(pos) == SCE_RB_HERE_DELIM) {
            break;
        }
    }
    if (pos == 0) {
        return false;
    }
    // Update the pointers so we don't have to re-analyze the string
    startPos = pos;
    return true;
}

// This class is used by the enter and exit methods, so it needs
// to be hoisted out of the function.

class QuoteCls {
public:
    int  Count = 0;
    char Up = '\0';
    char Down = '\0';
    QuoteCls() noexcept = default;
    void New() noexcept {
        Count = 0;
        Up    = '\0';
        Down  = '\0';
    }
    void Open(char u) noexcept {
        Count++;
        Up    = u;
        Down  = opposite(Up);
    }
};

constexpr bool isPercentLiteral(int state) noexcept {
    return state == SCE_RB_STRING_Q
           || state == SCE_RB_STRING_QQ
           // excluded SCE_RB_STRING_QR
           || state == SCE_RB_STRING_W
           || state == SCE_RB_STRING_QW
           || state == SCE_RB_STRING_I
           || state == SCE_RB_STRING_QI
           || state == SCE_RB_STRING_QS
           || state == SCE_RB_STRING_QX;
}

constexpr bool isInterpolableLiteral(int state) noexcept {
    return state != SCE_RB_STRING_Q
           && state != SCE_RB_STRING_W
           && state != SCE_RB_STRING_I
           && state != SCE_RB_STRING_QS
           && state != SCE_RB_CHARACTER;
}

bool isSingleSpecialVariable(char ch) noexcept {
    // https://docs.ruby-lang.org/en/master/globals_rdoc.html
    return strchr("~*$?!@/\\;,.=:<>\"&`'+", ch) != nullptr;
}

void InterpolateVariable(LexAccessor &styler, int state, Sci_Position &i, char &ch, char &chNext, char chNext2) {
    Sci_Position pos = i;
    styler.ColourTo(pos - 1, state);
    styler.ColourTo(pos, SCE_RB_OPERATOR);
    state = SCE_RB_GLOBAL;
    pos += 2;
    int len = 0;
    if (chNext == '$') {
        if (chNext2 == '-') {
            ++pos;
            len = 2;
        } else if (isSingleSpecialVariable(chNext2)) {
            ++pos;
            len = 1;
        }
    } else {
        state = SCE_RB_INSTANCE_VAR;
        if (chNext2 == '@') {
            state = SCE_RB_CLASS_VAR;
            ++pos;
        }
    }
    while (true) {
        chNext2 = styler.SafeGetCharAt(pos);
        --len;
        if (len == 0 || !isSafeWordcharOrHigh(chNext2)) {
            break;
        }
        ++pos;
    }
    --pos;
    styler.ColourTo(pos, state);
    i = pos;
    ch = chNext;
    chNext = chNext2;
}

bool isEmptyLine(Sci_Position pos, Accessor &styler) {
    int spaceFlags = 0;
    const Sci_Position lineCurrent = styler.GetLine(pos);
    const int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, nullptr);
    return (indentCurrent & SC_FOLDLEVELWHITEFLAG) != 0;
}

bool RE_CanFollowKeyword(const std::string &keyword) noexcept {
	return InList(keyword, {
		"and",
		"begin",
		"break",
		"case",
		"do",
		"else",
		"elsif",
		"if",
		"next",
		"return",
		"when",
		"unless",
		"until",
		"not",
		"or"
		});
}

// Look at chars up to but not including endPos
// Don't look at styles in case we're looking forward

Sci_Position skipWhitespace(Sci_Position startPos, Sci_Position endPos, Accessor &styler) {
    for (Sci_Position i = startPos; i < endPos; i++) {
        if (!IsASpaceOrTab(styler[i])) {
            return i;
        }
    }
    return endPos;
}

// This routine looks for false positives like
// undef foo, <<
// There aren't too many.
//
// iPrev points to the start of <<

bool sureThisIsHeredoc(Sci_Position iPrev, Accessor &styler, std::string &prevWord) {

    // Not so fast, since Ruby's so dynamic.  Check the context
    // to make sure we're OK.
    const Sci_Position lineStart = styler.GetLine(iPrev);
    const Sci_Position lineStartPosn = styler.LineStart(lineStart);
    styler.Flush();

    // Find the first word after some whitespace
    const Sci_Position firstWordPosn = skipWhitespace(lineStartPosn, iPrev, styler);
    if (firstWordPosn >= iPrev) {
        // Have something like {^     <<}
        //XXX Look at the first previous non-comment non-white line
        // to establish the context.  Not too likely though.
        return true;
    }
    const int prevStyle = styler.StyleIndexAt(firstWordPosn);
    switch (prevStyle) {
    case SCE_RB_WORD:
    case SCE_RB_WORD_DEMOTED:
    case SCE_RB_IDENTIFIER:
        break;
    default:
        return true;
    }
    Sci_Position firstWordEndPosn = firstWordPosn;
    prevWord.clear();
    for (;;) {
        if (firstWordEndPosn >= iPrev ||
                styler.StyleIndexAt(firstWordEndPosn) != prevStyle) {
            break;
        }
        prevWord.push_back(styler[firstWordEndPosn]);
        firstWordEndPosn += 1;
    }
    //XXX Write a style-aware thing to regex scintilla buffer objects
    // These keywords are what we were looking for
    return !InList(prevWord, { "undef", "def", "alias" });
}

// Routine that saves us from allocating a buffer for the here-doc target
// targetEndPos points one past the end of the current target
bool haveTargetMatch(Sci_Position currPos, Sci_Position lengthDoc, Sci_Position targetStartPos, Sci_Position targetEndPos, Accessor &styler) {
    if (lengthDoc - currPos < targetEndPos - targetStartPos) {
        return false;
    }
    for (Sci_Position i = targetStartPos, j = currPos;
            i < targetEndPos && j < lengthDoc;
            i++, j++) {
        if (styler[i] != styler[j]) {
            return false;
        }
    }
    return true;
}

// Finds the start position of the expression containing @p pos
// @p min_pos should be a known expression start, e.g. the start of the line
Sci_Position findExpressionStart(Sci_Position pos, Sci_Position min_pos, Accessor &styler) {
    int depth = 0;
    for (; pos > min_pos; pos -= 1) {
        const int style = styler.StyleIndexAt(pos - 1);
        if (style == SCE_RB_OPERATOR) {
            const char ch = styler[pos - 1];
            if (ch == '}' || ch == ')' || ch == ']') {
                depth += 1;
            } else if (ch == '{' || ch == '(' || ch == '[') {
                if (depth == 0) {
                    break;
                }
                depth -= 1;
            } else if (ch == ';' && depth == 0) {
                break;
            }
        }
    }
    return pos;
}

// We need a check because the form
// [identifier] <<[target]
// is ambiguous.  The Ruby lexer/parser resolves it by
// looking to see if [identifier] names a variable or a
// function.  If it's the first, it's the start of a here-doc.
// If it's a var, it's an operator.  This lexer doesn't
// maintain a symbol table, so it looks ahead to see what's
// going on, in cases where we have
// ^[white-space]*[identifier([.|::]identifier)*][white-space]*<<[target]
//
// If there's no occurrence of [target] on a line, assume we don't.

// return true == yes, we have no heredocs

bool sureThisIsNotHeredoc(Sci_Position lt2StartPos, Accessor &styler) {
    // Use full document, not just part we're styling
    const Sci_Position lengthDoc = styler.Length();
    const Sci_Position lineStart = styler.GetLine(lt2StartPos);
    const Sci_Position lineStartPosn = styler.LineStart(lineStart);
    styler.Flush();
    constexpr bool definitely_not_a_here_doc = true;
    constexpr bool looks_like_a_here_doc = false;

    // find the expression start rather than the line start
    const Sci_Position exprStartPosn = findExpressionStart(lt2StartPos, lineStartPosn, styler);

    // Find the first word after some whitespace
    Sci_Position firstWordPosn = skipWhitespace(exprStartPosn, lt2StartPos, styler);
    if (firstWordPosn >= lt2StartPos) {
        return definitely_not_a_here_doc;
    }
    int prevStyle = styler.StyleIndexAt(firstWordPosn);
    // If we have '<<' following a keyword, it's not a heredoc
    if (!IsIdentifierStyle(prevStyle)
            && prevStyle != SCE_RB_GLOBAL       // $stdout and $stderr
            && prevStyle != SCE_RB_SYMBOL
            && prevStyle != SCE_RB_INSTANCE_VAR
            && prevStyle != SCE_RB_CLASS_VAR) {
        return definitely_not_a_here_doc;
    }
    int newStyle = prevStyle;
    // Some compilers incorrectly warn about uninit newStyle
    for (firstWordPosn += 1; firstWordPosn <= lt2StartPos; firstWordPosn += 1) {
        // Inner loop looks at the name
        for (; firstWordPosn <= lt2StartPos; firstWordPosn += 1) {
            newStyle = styler.StyleIndexAt(firstWordPosn);
            if (newStyle != prevStyle) {
                break;
            }
        }
        // Do we have '::' or '.'?
        if (firstWordPosn < lt2StartPos && newStyle == SCE_RB_OPERATOR) {
            const char ch = styler[firstWordPosn];
            if (ch == '.') {
                // yes
            } else if (ch == ':') {
                if (styler.StyleIndexAt(++firstWordPosn) != SCE_RB_OPERATOR) {
                    return definitely_not_a_here_doc;
                } else if (styler[firstWordPosn] != ':') {
                    return definitely_not_a_here_doc;
                }
            } else {
                break;
            }
        } else {
            break;
        }
        // on second and next passes, only identifiers may appear since
        // class and instance variable are private
        prevStyle = SCE_RB_IDENTIFIER;
    }
    // Skip next batch of white-space
    firstWordPosn = skipWhitespace(firstWordPosn, lt2StartPos, styler);
    // possible symbol for an implicit hash argument
    if (firstWordPosn < lt2StartPos && styler.StyleIndexAt(firstWordPosn) == SCE_RB_SYMBOL) {
        for (; firstWordPosn <= lt2StartPos; firstWordPosn += 1) {
            if (styler.StyleIndexAt(firstWordPosn) != SCE_RB_SYMBOL) {
                break;
            }
        }
        // Skip next batch of white-space
        firstWordPosn = skipWhitespace(firstWordPosn, lt2StartPos, styler);
    }
    if (firstWordPosn != lt2StartPos) {
        // Have [[^ws[identifier]ws[*something_else*]ws<<
        return definitely_not_a_here_doc;
    }
    // OK, now 'j' will point to the current spot moving ahead
    Sci_Position j = firstWordPosn + 1;
    if (styler.StyleIndexAt(j) != SCE_RB_OPERATOR || styler[j] != '<') {
        // This shouldn't happen
        return definitely_not_a_here_doc;
    }
    const Sci_Position nextLineStartPosn = styler.LineStart(lineStart + 1);
    if (nextLineStartPosn >= lengthDoc) {
        return definitely_not_a_here_doc;
    }
    j = skipWhitespace(j + 1, nextLineStartPosn, styler);
    if (j >= lengthDoc) {
        return definitely_not_a_here_doc;
    }
    bool allow_indent = false;
    Sci_Position target_start = 0;
    Sci_Position target_end = 0;
    // From this point on no more styling, since we're looking ahead
    if (styler[j] == '-' || styler[j] == '~') {
        allow_indent = true;
        j++;
    } else {
        allow_indent = false;
    }

    // Allow for quoted targets.
    char target_quote = 0;
    switch (styler[j]) {
    case '\'':
    case '"':
    case '`':
        target_quote = styler[j];
        j += 1;
    }

    if (isSafeAlnumOrHigh(styler[j])) {
        // Initialize target_end because some compilers think it won't
        // be initialized by the time it's used
        target_start = target_end = j;
        j++;
    } else {
        return definitely_not_a_here_doc;
    }
    for (; j < lengthDoc; j++) {
        if (!isSafeAlnumOrHigh(styler[j])) {
            if (target_quote && styler[j] != target_quote) {
                // unquoted end
                return definitely_not_a_here_doc;
            }

            // And for now make sure that it's a newline
            // don't handle arbitrary expressions yet

            target_end = j;
            if (target_quote) {
                // Now we can move to the character after the string delimiter.
                j += 1;
            }
            j = skipWhitespace(j, lengthDoc, styler);
            if (j >= lengthDoc) {
                return definitely_not_a_here_doc;
            }
            const char ch = styler[j];
            if (ch == '#' || isEOLChar(ch) || ch == '.' || ch == ',' || IsLowerCase(ch)) {
                // This is OK, so break and continue;
                break;
            }
            return definitely_not_a_here_doc;
        }
    }

    // Just look at the start of each line
    Sci_Position last_line = styler.GetLine(lengthDoc - 1);
    // But don't go too far
    if (last_line > lineStart + 50) {
        last_line = lineStart + 50;
    }
    for (Sci_Position line_num = lineStart + 1; line_num <= last_line; line_num++) {
        j = styler.LineStart(line_num);
        if (allow_indent) {
            j = skipWhitespace(j, lengthDoc, styler);
        }
        // target_end is one past the end
        if (haveTargetMatch(j, lengthDoc, target_start, target_end, styler)) {
            // We got it
            return looks_like_a_here_doc;
        }
    }
    return definitely_not_a_here_doc;
}

//todo: if we aren't looking at a stdio character,
// move to the start of the first line that is not in a
// multiline construct

void synchronizeDocStart(Sci_PositionU &startPos, Sci_Position &length, int &initStyle, Accessor &styler, bool skipWhiteSpace=false) {
    // Retreat one line to match function lexer
    if (const Sci_Position lineCurrent = styler.GetLine(startPos); lineCurrent > 0) {
        const Sci_Position endPos = startPos + length;
        startPos = styler.LineStart(lineCurrent - 1);
        length = endPos - startPos;
        initStyle = (startPos > 0) ? styler.StyleIndexAt(startPos - 1) : 0;
    }

    const int style = styler.StyleIndexAt(startPos);
    switch (style) {
    case SCE_RB_STDIN:
    case SCE_RB_STDOUT:
    case SCE_RB_STDERR:
        // Don't do anything else with these.
        return;
    }

    Sci_Position pos = startPos;
    // Quick way to characterize each line
    Sci_Position lineStart = styler.GetLine(pos);
    for (; lineStart > 0; lineStart--) {
        // Now look at the style before the previous line's EOL
        pos = styler.LineStart(lineStart) - 1;
        if (pos <= 10) {
            lineStart = 0;
            break;
        }
        const char ch = styler.SafeGetCharAt(pos);
        const char chPrev = styler.SafeGetCharAt(pos - 1);
        if (ch == '\n' && chPrev == '\r') {
            pos--;
        }
        if (styler.SafeGetCharAt(pos - 1) == '\\') {
            // Continuation line -- keep going
        } else if (styler.StyleIndexAt(pos) != SCE_RB_DEFAULT) {
            // Part of multiline construct -- keep going
        } else if (currLineContainsHereDelims(pos, styler)) {
            // Keep going, with pos and length now pointing
            // at the end of the here-doc delimiter
        } else if (skipWhiteSpace && isEmptyLine(pos, styler)) {
            // Keep going
        } else {
            break;
        }
    }
    pos = styler.LineStart(lineStart);
    length += (startPos - pos);
    startPos = pos;
    initStyle = SCE_RB_DEFAULT;
}

void LexerRuby::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
    Accessor styler(pAccess, nullptr);
    styler.StartAt(startPos);

    // Lexer for Ruby often has to backtrack to start of current style to determine
    // which characters are being used as quotes, how deeply nested is the
    // start position and what the termination string is for here documents

    class HereDocCls {
    public:
        int State = 0;
        // States
        // 0: '<<' encountered
        // 1: collect the delimiter
        // 1b: text between the end of the delimiter and the EOL
        // 2: here doc text (lines after the delimiter)
        char Quote = 0;		// the char after '<<'
        bool Quoted = false;		// true if Quote in ('\'','"','`')
        int DelimiterLength = 0;	// strlen(Delimiter)
        char Delimiter[256] {};	// the Delimiter, limit of 256: from Perl
        bool CanBeIndented = false;
    };
    HereDocCls HereDoc;

    QuoteCls Quote;

    synchronizeDocStart(startPos, length, initStyle, styler, false);

    const WordClassifier &idClasser = subStyles.Classifier(SCE_RB_IDENTIFIER);

    bool preferRE = true;
    bool afterDef = false;
    int state = initStyle;
    const Sci_Position lengthDoc = startPos + length;

    std::string prevWord;
    if (length == 0)
        return;

    char chPrev = styler.SafeGetCharAt(startPos - 1);
    char chNext = styler.SafeGetCharAt(startPos);
    bool is_real_number = true;   // Differentiate between constants and ?-sequences.
    styler.StartAt(startPos);
    styler.StartSegment(startPos);

    static constexpr int q_states[] = {
        SCE_RB_STRING_Q,
        SCE_RB_STRING_QQ,
        SCE_RB_STRING_QR,
        SCE_RB_STRING_W,
        SCE_RB_STRING_QW,
        SCE_RB_STRING_QX,
        SCE_RB_STRING_I,
        SCE_RB_STRING_QI,
        SCE_RB_STRING_QS,
    };
    constexpr const char *q_chars = "qQrwWxiIs";

    // In most cases a value of 2 should be ample for the code in the
    // Ruby library, and the code the user is likely to enter.
    // For example,
    // fu_output_message "mkdir #{options[:mode] ? ('-m %03o ' % options[:mode]) : ''}#{list.join ' '}"
    //     if options[:verbose]
    // from fileutils.rb nests to a level of 2
    // If the user actually hits a 6th occurrence of '#{' in a double-quoted
    // string (including regex'es, %Q, %<sym>, %w, and other strings
    // that interpolate), it will stay as a string.  The problem with this
    // is that quotes might flip, a 7th '#{' will look like a comment,
    // and code-folding might be wrong.

    // If anyone runs into this problem, I recommend raising this
    // value slightly higher to replacing the fixed array with a linked
    // list.  Keep in mind this code will be called every time the lexer
    // is invoked.

#define INNER_STRINGS_MAX_COUNT 5
    class InnerExpression {
        // These vars track our instances of "...#{,,,%Q<..#{,,,}...>,,,}..."
        int inner_string_types[INNER_STRINGS_MAX_COUNT] {};
        // Track # braces when we push a new #{ thing
        int inner_expn_brace_counts[INNER_STRINGS_MAX_COUNT] {};
        QuoteCls inner_quotes[INNER_STRINGS_MAX_COUNT];
        int inner_string_count = 0;

    public:
        int brace_counts = 0;   // Number of #{ ... } things within an expression

        [[nodiscard]] bool canEnter() const noexcept {
            return inner_string_count < INNER_STRINGS_MAX_COUNT;
        }
        [[nodiscard]] bool canExit() const noexcept {
            return inner_string_count > 0;
        }
        void enter(int &state, const QuoteCls &curr_quote) noexcept {
            inner_string_types[inner_string_count] = state;
            state = SCE_RB_DEFAULT;
            inner_expn_brace_counts[inner_string_count] = brace_counts;
            brace_counts = 0;
            inner_quotes[inner_string_count] = curr_quote;
            ++inner_string_count;
        }
        void exit(int &state, QuoteCls &curr_quote) noexcept {
            --inner_string_count;
            state = inner_string_types[inner_string_count];
            brace_counts = inner_expn_brace_counts[inner_string_count];
            curr_quote = inner_quotes[inner_string_count];
        }
    };
    InnerExpression innerExpr;

    for (Sci_Position i = startPos; i < lengthDoc; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        char chNext2 = styler.SafeGetCharAt(i + 2);

        if (styler.IsLeadByte(ch)) {
            chNext = chNext2;
            chPrev = ' ';
            i += 1;
            continue;
        }

        // skip on DOS/Windows
        //No, don't, because some things will get tagged on,
        // so we won't recognize keywords, for example
#if 0
        if (ch == '\r' && chNext == '\n') {
            continue;
        }
#endif

        if (HereDoc.State == 1 && isEOLChar(ch)) {
            // Begin of here-doc (the line after the here-doc delimiter):
            HereDoc.State = 2;
            if (state == SCE_RB_WORD) {
                ClassifyWordRb(i, ch, chNext, keywords, styler, prevWord, idClasser);
            } else {
                styler.ColourTo(i - 1, state);
            }
            // Don't check for a missing quote, just jump into
            // the here-doc state
            state = SCE_RB_HERE_QQ;
            if (HereDoc.Quoted) {
                if (HereDoc.Quote == '\'') {
                    state = SCE_RB_HERE_Q;
                } else if (HereDoc.Quote == '`') {
                    state = SCE_RB_HERE_QX;
                }
            }
        }

        // Regular transitions
        if (state == SCE_RB_DEFAULT) {
            if (isSafeDigit(ch)) {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_NUMBER;
                is_real_number = true;
            } else if (isSafeAlphaOrHigh(ch)) {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_WORD;
            } else if (ch == '#') {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_COMMENTLINE;
            } else if (ch == '=') {
                // =begin indicates the start of a comment (doc) block
                if ((i == 0 || isEOLChar(chPrev))
                        && chNext == 'b'
                        && styler.SafeGetCharAt(i + 2) == 'e'
                        && styler.SafeGetCharAt(i + 3) == 'g'
                        && styler.SafeGetCharAt(i + 4) == 'i'
                        && styler.SafeGetCharAt(i + 5) == 'n'
                        && !isSafeWordcharOrHigh(styler.SafeGetCharAt(i + 6))) {
                    styler.ColourTo(i - 1, state);
                    state = SCE_RB_POD;
                } else {
                    styler.ColourTo(i - 1, state);
                    styler.ColourTo(i, SCE_RB_OPERATOR);
                    preferRE = true;
                }
            } else if (ch == '"') {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_STRING;
                Quote.New();
                Quote.Open(ch);
            } else if (ch == '\'') {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_CHARACTER;
                Quote.New();
                Quote.Open(ch);
            } else if (ch == '`') {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_BACKTICKS;
                Quote.New();
                Quote.Open(ch);
            } else if (ch == '@') {
                // Instance or class var
                styler.ColourTo(i - 1, state);
                if (chNext == '@') {
                    state = SCE_RB_CLASS_VAR;
                    advance_char(i, ch, chNext, chNext2); // pass by ref
                } else {
                    state = SCE_RB_INSTANCE_VAR;
                }
            } else if (ch == '$') {
                // Check for a builtin global
                styler.ColourTo(i - 1, state);
                // Recognize it bit by bit
                state = SCE_RB_GLOBAL;
            } else if (ch == '/' && preferRE) {
                // Ambiguous operator
                styler.ColourTo(i - 1, state);
                state = SCE_RB_REGEX;
                Quote.New();
                Quote.Open(ch);
            } else if (ch == '<' && chNext == '<' && chNext2 != '=') {
                if (afterDef) {
                    afterDef = false;
                    prevWord[0] = 0;
                }
                // Recognise the '<<' symbol - either a here document or a binary op
                styler.ColourTo(i - 1, state);
                i++;
                chNext = chNext2;
                styler.ColourTo(i, SCE_RB_OPERATOR);

                if (!(strchr("\"\'`_-~", chNext2) || isSafeAlphaOrHigh(chNext2))) {
                    // It's definitely not a here-doc,
                    // based on Ruby's lexer/parser in the
                    // heredoc_identifier routine.
                    // Nothing else to do.
                } else if (preferRE) {
                    if (sureThisIsHeredoc(i - 1, styler, prevWord)) {
                        state = SCE_RB_HERE_DELIM;
                        HereDoc.State = 0;
                    }
                    // else leave it in default state
                } else {
                    if (sureThisIsNotHeredoc(i - 1, styler)) {
                        // leave state as default
                        // We don't have all the heuristics Perl has for indications
                        // of a here-doc, because '<<' is overloadable and used
                        // for so many other classes.
                    } else {
                        state = SCE_RB_HERE_DELIM;
                        HereDoc.State = 0;
                    }
                }
                preferRE = (state != SCE_RB_HERE_DELIM);
            } else if (ch == ':') {
                afterDef = false;
                styler.ColourTo(i - 1, state);
                if (chNext == ':') {
                    // Mark "::" as an operator, not symbol start
                    styler.ColourTo(i + 1, SCE_RB_OPERATOR);
                    advance_char(i, ch, chNext, chNext2); // pass by ref
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                } else if (isSafeWordcharOrHigh(chNext)) {
                    state = SCE_RB_SYMBOL;
                } else if ((chNext == '@' || chNext == '$') &&
                           isSafeWordcharOrHigh(chNext2)) {
                    // instance and global variable followed by an identifier
                    advance_char(i, ch, chNext, chNext2);
                    state = SCE_RB_SYMBOL;
                } else if (((chNext == '@' && chNext2 == '@')  ||
                            (chNext == '$' && chNext2 == '-')) &&
                           isSafeWordcharOrHigh(styler.SafeGetCharAt(i+3))) {
                    // class variables and special global variable "$-IDENTCHAR"
                    state = SCE_RB_SYMBOL;
                    // $-IDENTCHAR doesn't continue past the IDENTCHAR
                    if (chNext == '$') {
                        styler.ColourTo(i+3, SCE_RB_SYMBOL);
                        state = SCE_RB_DEFAULT;
                    }
                    i += 3;
                    ch = styler.SafeGetCharAt(i);
                    chNext = styler.SafeGetCharAt(i+1);
                } else if (chNext == '$' && isSingleSpecialVariable(chNext2)) {
                    // single-character special global variables
                    i += 2;
                    ch = chNext2;
                    chNext = styler.SafeGetCharAt(i+1);
                    styler.ColourTo(i, SCE_RB_SYMBOL);
                    state = SCE_RB_DEFAULT;
                } else if (strchr("[*!~+-*/%=<>&^|", chNext)) {
                    // Do the operator analysis in-line, looking ahead
                    // Based on the table in pickaxe 2nd ed., page 339
                    bool doColoring = true;
                    switch (chNext) {
                    case '[':
                        if (chNext2 == ']') {
                            const char ch_tmp = styler.SafeGetCharAt(i + 3);
                            if (ch_tmp == '=') {
                                i += 3;
                                ch = ch_tmp;
                                chNext = styler.SafeGetCharAt(i + 1);
                            } else {
                                i += 2;
                                ch = chNext2;
                                chNext = ch_tmp;
                            }
                        } else {
                            doColoring = false;
                        }
                        break;

                    case '*':
                        if (chNext2 == '*') {
                            i += 2;
                            ch = chNext2;
                            chNext = styler.SafeGetCharAt(i + 1);
                        } else {
                            advance_char(i, ch, chNext, chNext2);
                        }
                        break;

                    case '!':
                        if (chNext2 == '=' || chNext2 == '~') {
                            i += 2;
                            ch = chNext2;
                            chNext = styler.SafeGetCharAt(i + 1);
                        } else {
                            advance_char(i, ch, chNext, chNext2);
                        }
                        break;

                    case '<':
                        if (chNext2 == '<') {
                            i += 2;
                            ch = chNext2;
                            chNext = styler.SafeGetCharAt(i + 1);
                        } else if (chNext2 == '=') {
                            const char ch_tmp = styler.SafeGetCharAt(i + 3);
                            if (ch_tmp == '>') {  // <=> operator
                                i += 3;
                                ch = ch_tmp;
                                chNext = styler.SafeGetCharAt(i + 1);
                            } else {
                                i += 2;
                                ch = chNext2;
                                chNext = ch_tmp;
                            }
                        } else {
                            advance_char(i, ch, chNext, chNext2);
                        }
                        break;

                    default:
                        // Simple one-character operators
                        advance_char(i, ch, chNext, chNext2);
                        break;
                    }
                    if (doColoring) {
                        styler.ColourTo(i, SCE_RB_SYMBOL);
                        state = SCE_RB_DEFAULT;
                    }
                } else if (!preferRE && !IsASpace(chNext)) {
                    // Don't color symbol strings (yet)
                    // Just color the ":" and color rest as string
                    styler.ColourTo(i, SCE_RB_SYMBOL);
                    state = SCE_RB_DEFAULT;
                } else {
                    styler.ColourTo(i, SCE_RB_OPERATOR);
                    state = SCE_RB_DEFAULT;
                    preferRE = true;
                }
            } else if (ch == '%' && !afterDef) {
                styler.ColourTo(i - 1, state);
                bool have_string = false;
                const char *hit = strchr(q_chars, chNext);
                if (hit != nullptr && !isSafeWordcharOrHigh(chNext2)) {
                    Quote.New();
                    state = q_states[hit - q_chars];
                    Quote.Open(chNext2);
                    i += 2;
                    ch = chNext2;
                    chNext = styler.SafeGetCharAt(i + 1);
                    have_string = true;
                } else if ((preferRE || (!isWhiteSpace(chNext) && chNext != '=')) && !isSafeWordcharOrHigh(chNext)) {
                    // Ruby doesn't allow high bit chars here,
                    // but the editor host might
                    Quote.New();
                    state = SCE_RB_STRING_QQ;
                    Quote.Open(chNext);
                    advance_char(i, ch, chNext, chNext2); // pass by ref
                    have_string = true;
                }
                if (!have_string) {
                    styler.ColourTo(i, SCE_RB_OPERATOR);
                    // stay in default
                    preferRE = true;
                }
            } else if (ch == '?') {
                afterDef = false;
                styler.ColourTo(i - 1, state);
                if (isHighBitChar(chNext)) {
                    preferRE = false;
                    Sci_Position width = 1;
                    styler.MultiByteAccess()->GetCharacterAndWidth(i + 1, &width);
                    chNext = styler.SafeGetCharAt(i + 1 + width);
                    if (isSafeWordcharOrHigh(chNext)) {
                        styler.ColourTo(i, SCE_RB_OPERATOR);
                        i += width;
                        state = SCE_RB_WORD;
                    } else {
                        i += width;
                        styler.ColourTo(i, SCE_RB_NUMBER);
                    }
                } else if (!isQuestionMarkChar(chNext, chNext2)) {
                    styler.ColourTo(i, SCE_RB_OPERATOR);
                    preferRE = chNext <= ' ';
                } else {
                    // It's the start of a character code escape sequence
                    // Color it as a number.
                    state = SCE_RB_NUMBER;
                    is_real_number = false;
                }
            } else if (isoperator(ch) || ch == '.') {
                styler.ColourTo(i - 1, state);
                if (afterDef && ch != '.') {
                    afterDef = false;
                    prevWord[0] = 0;
                    if (chNext == '@' && (ch == '+' || ch == '-' || ch == '!')) {
                        // unary operator method
                        ch = chNext;
                        chNext = chNext2;
                        i += 1;
                    }
                }
                styler.ColourTo(i, SCE_RB_OPERATOR);
                // If we're ending an expression or block,
                // assume it ends an object, and the ambivalent
                // constructs are binary operators
                //
                // So if we don't have one of these chars,
                // we aren't ending an object expression, and ops
                // like : << / are unary operators.

                if (ch == '{') {
                    ++innerExpr.brace_counts;
                    preferRE = true;
                } else if (ch == '}' && --innerExpr.brace_counts < 0
                           && innerExpr.canExit()) {
                    styler.ColourTo(i, SCE_RB_OPERATOR);
                    innerExpr.exit(state, Quote);
                } else {
                    preferRE = !AnyOf(ch, ')', '}', ']', '.');
                }
                // Stay in default state
            } else if (isEOLChar(ch)) {
                afterDef = false;
                // Make sure it's a true line-end, with no backslash
                if ((ch == '\r' || (ch == '\n' && chPrev != '\r'))
                        && chPrev != '\\') {
                    // Assume we've hit the end of the statement.
                    preferRE = true;
                }
            }
            if (afterDef && state != SCE_RB_DEFAULT) {
                afterDef = false;
            }
        } else if (state == SCE_RB_WORD) {
            if (ch == '.' || !isSafeWordcharOrHigh(ch)) {
                // Words include x? in all contexts,
                // and <letters>= after either 'def' or a dot
                // Move along until a complete word is on our left

                // Default accessor treats '.' as word-chars,
                // but we don't for now.

                if (ch == '='
                        && isSafeWordcharOrHigh(chPrev)
                        && (chNext == '('
                            || isWhiteSpace(chNext))
                        && ((prevWord == "def")
                            || followsDot(styler.GetStartSegment(), styler))) {
                    // <name>= is a name only when being defined -- Get it the next time
                    // This means that <name>=<name> is always lexed as
                    // <name>, (op, =), <name>
                } else if (ch == ':'
                           && isSafeWordcharOrHigh(chPrev)
                           && isWhiteSpace(chNext)) {
                    // keyword argument, symbol Hash key
                    styler.ColourTo(i, SCE_RB_SYMBOL);
                    state = SCE_RB_DEFAULT;
                    preferRE = true;
                } else if ((ch == '?' || ch == '!')
                           && isSafeWordcharOrHigh(chPrev)
                           && !isSafeWordcharOrHigh(chNext)) {
                    // <name>? is a name -- Get it the next time
                    // But <name>?<name> is always lexed as
                    // <name>, (op, ?), <name>
                    // Same with <name>! to indicate a method that
                    // modifies its target
                } else if (isEOLChar(ch)
                           && isMatch(styler, lengthDoc, i - 7, "__END__")) {
                    styler.ColourTo(i, SCE_RB_DATASECTION);
                    state = SCE_RB_DATASECTION;
                    // No need to handle this state -- we'll just move to the end
                    preferRE = false;
                } else {
                    const int word_style = ClassifyWordRb(i, ch, chNext, keywords, styler, prevWord, idClasser);
                    switch (word_style) {
                    case SCE_RB_WORD:
                        afterDef = prevWord == "def";
                        preferRE = RE_CanFollowKeyword(prevWord);
                        break;

                    case SCE_RB_WORD_DEMOTED:
                    case SCE_RB_DEFNAME:
                    case SCE_RB_IDENTIFIER_PREFERRE:
                        preferRE = true;
                        break;

                    case SCE_RB_IDENTIFIER:
                        preferRE = isEOLChar(ch);
                        break;

                    default:
                        preferRE = false;
                    }
                    if (ch == '.') {
                        // We might be redefining an operator-method
                        afterDef = word_style == SCE_RB_DEFNAME;
                    }
                    // And if it's the first
                    redo_char(i, ch, chNext, chNext2, state); // pass by ref
                }
            }
        } else if (state == SCE_RB_NUMBER) {
            if (!is_real_number) {
                if (ch != '\\' || chPrev == '\\') {
                    styler.ColourTo(i, state);
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                } else if (strchr("\\ntrfvaebs", chNext)) {
                    // Terminal escape sequence -- handle it next time
                    // Nothing more to do this time through the loop
                } else if (chNext == 'C' || chNext == 'M') {
                    if (chNext2 != '-') {
                        // \C or \M ends the sequence -- handle it next time
                    } else {
                        // Move from abc?\C-x
                        //               ^
                        // to
                        //                 ^
                        i += 2;
                        ch = chNext2;
                        chNext = styler.SafeGetCharAt(i + 1);
                    }
                } else if (chNext == 'c') {
                    // Stay here, \c is a combining sequence
                    advance_char(i, ch, chNext, chNext2); // pass by ref
                } else {
                    // ?\x, including ?\\ is final.
                    styler.ColourTo(i + 1, state);
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                    advance_char(i, ch, chNext, chNext2);
                }
            } else if (isSafeAlnumOrHigh(ch) || ch == '_' || (ch == '.' && isSafeDigit(chNext))) {
                // Keep going
            } else if (ch == '.' && chNext == '.') {
                styler.ColourTo(i - 1, state);
                redo_char(i, ch, chNext, chNext2, state); // pass by ref
            } else {
                styler.ColourTo(i - 1, state);
                redo_char(i, ch, chNext, chNext2, state); // pass by ref
                preferRE = false;
            }
        } else if (state == SCE_RB_COMMENTLINE) {
            if (isEOLChar(ch)) {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_DEFAULT;
                // Use whatever setting we had going into the comment
            }
        } else if (state == SCE_RB_HERE_DELIM) {
            // See the comment for SCE_RB_HERE_DELIM in LexPerl.cxx
            // Slightly different: if we find an immediate '-',
            // the target can appear indented.

            if (HereDoc.State == 0) { // '<<' encountered
                HereDoc.State = 1;
                HereDoc.DelimiterLength = 0;
                if (ch == '-' || ch == '~') {
                    HereDoc.CanBeIndented = true;
                    advance_char(i, ch, chNext, chNext2); // pass by ref
                } else {
                    HereDoc.CanBeIndented = false;
                }
                if (isEOLChar(ch)) {
                    // Bail out of doing a here doc if there's no target
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                } else {
                    HereDoc.Quote = ch;

                    if (ch == '\'' || ch == '"' || ch == '`') {
                        HereDoc.Quoted = true;
                        HereDoc.Delimiter[0] = '\0';
                    } else {
                        HereDoc.Quoted = false;
                        HereDoc.Delimiter[0] = ch;
                        HereDoc.Delimiter[1] = '\0';
                        HereDoc.DelimiterLength = 1;
                    }
                }
            } else if (HereDoc.State == 1) { // collect the delimiter
                if (isEOLChar(ch)) {
                    // End the quote now, and go back for more
                    styler.ColourTo(i - 1, state);
                    state = SCE_RB_DEFAULT;
                    i--;
                    chNext = ch;
                    preferRE = false;
                } else if (HereDoc.Quoted) {
                    if (ch == HereDoc.Quote) { // closing quote => end of delimiter
                        styler.ColourTo(i, state);
                        state = SCE_RB_DEFAULT;
                        preferRE = false;
                    } else {
                        if (ch == '\\' && !isEOLChar(chNext)) {
                            advance_char(i, ch, chNext, chNext2);
                        }
                        HereDoc.Delimiter[HereDoc.DelimiterLength++] = ch;
                        HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
                    }
                } else { // an unquoted here-doc delimiter
                    if (isSafeAlnumOrHigh(ch) || ch == '_') {
                        HereDoc.Delimiter[HereDoc.DelimiterLength++] = ch;
                        HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
                    } else {
                        styler.ColourTo(i - 1, state);
                        redo_char(i, ch, chNext, chNext2, state);
                        preferRE = false;
                    }
                }
                if (HereDoc.DelimiterLength >= static_cast<int>(sizeof(HereDoc.Delimiter)) - 1) {
                    styler.ColourTo(i - 1, state);
                    state = SCE_RB_ERROR;
                    preferRE = false;
                }
            }
        } else if (state == SCE_RB_HERE_Q || state == SCE_RB_HERE_QQ || state == SCE_RB_HERE_QX) {
            if (ch == '\\' && !isEOLChar(chNext)) {
                advance_char(i, ch, chNext, chNext2);
            } else if (ch == '#' && state != SCE_RB_HERE_Q
                       && (chNext == '{' || chNext == '@' || chNext == '$')) {
                if (chNext == '{') {
                    if (innerExpr.canEnter()) {
                        // process #{ ... }
                        styler.ColourTo(i - 1, state);
                        styler.ColourTo(i + 1, SCE_RB_OPERATOR);
                        innerExpr.enter(state, Quote);
                        preferRE = true;
                        // Skip one
                        advance_char(i, ch, chNext, chNext2);
                    }
                } else {
                    InterpolateVariable(styler, state, i, ch, chNext, chNext2);
                }
            }

            // Not needed: HereDoc.State == 2
            // Indentable here docs: look backwards
            // Non-indentable: look forwards, like in Perl
            //
            // Why: so we can quickly resolve things like <<-" abc"

            else if (!HereDoc.CanBeIndented) {
                if (isEOLChar(chPrev)
                        && isMatch(styler, lengthDoc, i, HereDoc.Delimiter)) {
                    styler.ColourTo(i - 1, state);
                    i += static_cast<Sci_Position>(HereDoc.DelimiterLength) - 1;
                    chNext = styler.SafeGetCharAt(i + 1);
                    if (isEOLChar(chNext)) {
                        styler.ColourTo(i, SCE_RB_HERE_DELIM);
                        state = SCE_RB_DEFAULT;
                        HereDoc.State = 0;
                        preferRE = false;
                    }
                    // Otherwise we skipped through the here doc faster.
                }
            } else if (isEOLChar(chNext)
                       && lookingAtHereDocDelim(styler,
                                                i - HereDoc.DelimiterLength + 1,
                                                lengthDoc,
                                                HereDoc.Delimiter)) {
                styler.ColourTo(i - HereDoc.DelimiterLength, state);
                styler.ColourTo(i, SCE_RB_HERE_DELIM);
                state = SCE_RB_DEFAULT;
                preferRE = false;
                HereDoc.State = 0;
            }
        } else if (state == SCE_RB_CLASS_VAR
                   || state == SCE_RB_INSTANCE_VAR
                   || state == SCE_RB_SYMBOL) {
            if (state == SCE_RB_SYMBOL &&
                    // FIDs suffices '?' and '!'
                    (((ch == '!' || ch == '?') && chNext != '=') ||
                     // identifier suffix '='
                     (ch == '=' && (chNext != '~' && chNext != '>' &&
                                    (chNext != '=' || chNext2 == '>'))))) {
                styler.ColourTo(i, state);
                state = SCE_RB_DEFAULT;
                preferRE = false;
            } else if (!isSafeWordcharOrHigh(ch)) {
                styler.ColourTo(i - 1, state);
                redo_char(i, ch, chNext, chNext2, state); // pass by ref
                preferRE = false;
            }
        } else if (state == SCE_RB_GLOBAL) {
            if (!isSafeWordcharOrHigh(ch)) {
                // handle special globals here as well
                if (chPrev == '$') {
                    if (ch == '-') {
                        // Include the next char, like $-a
                        advance_char(i, ch, chNext, chNext2);
                    }
                    styler.ColourTo(i, state);
                    state = SCE_RB_DEFAULT;
                } else {
                    styler.ColourTo(i - 1, state);
                    redo_char(i, ch, chNext, chNext2, state); // pass by ref
                }
                preferRE = false;
            }
        } else if (state == SCE_RB_POD) {
            // PODs end with ^=end\s, -- any whitespace can follow =end
            if (isWhiteSpace(ch)
                    && i > 5
                    && isEOLChar(styler[i - 5])
                    && isMatch(styler, lengthDoc, i - 4, "=end")) {
                styler.ColourTo(i - 1, state);
                state = SCE_RB_DEFAULT;
                preferRE = false;
            }
        } else if (state == SCE_RB_REGEX || state == SCE_RB_STRING_QR) {
            if (ch == '\\' && Quote.Up != '\\') {
                // Skip one
                advance_char(i, ch, chNext, chNext2);
            } else if (ch == Quote.Down) {
                Quote.Count--;
                if (Quote.Count == 0) {
                    // Include the options
                    while (isSafeAlpha(chNext)) {
                        i++;
                        ch = chNext;
                        chNext = styler.SafeGetCharAt(i + 1);
                    }
                    styler.ColourTo(i, state);
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                }
            } else if (ch == Quote.Up) {
                // Only if close quoter != open quoter
                Quote.Count++;

            } else if (ch == '#') {
                if (chNext == '{') {
                    if (innerExpr.canEnter()) {
                        // process #{ ... }
                        styler.ColourTo(i - 1, state);
                        styler.ColourTo(i + 1, SCE_RB_OPERATOR);
                        innerExpr.enter(state, Quote);
                        preferRE = true;
                        // Skip one
                        advance_char(i, ch, chNext, chNext2);
                    }
                } else if (chNext == '@' || chNext == '$') {
                    InterpolateVariable(styler, state, i, ch, chNext, chNext2);
                } else {
                    //todo: distinguish comments from pound chars
                    // for now, handle as comment
                    styler.ColourTo(i - 1, state);
                    bool inEscape = false;
                    while (++i < lengthDoc) {
                        ch = styler.SafeGetCharAt(i);
                        if (ch == '\\') {
                            inEscape = true;
                        } else if (isEOLChar(ch)) {
                            // Comment inside a regex
                            styler.ColourTo(i - 1, SCE_RB_COMMENTLINE);
                            break;
                        } else if (inEscape) {
                            inEscape = false;  // don't look at char
                        } else if (ch == Quote.Down) {
                            // Have the regular handler deal with this
                            // to get trailing modifiers.
                            i--;
                            ch = styler[i];
                            break;
                        }
                    }
                    chNext = styler.SafeGetCharAt(i + 1);
                }
            }
            // Quotes of all kinds...
        } else if (isPercentLiteral(state) ||
                   state == SCE_RB_STRING || state == SCE_RB_CHARACTER ||
                   state == SCE_RB_BACKTICKS) {
            if (!Quote.Down && !isspacechar(ch)) {
                Quote.Open(ch);
            } else if (ch == '\\' && Quote.Up != '\\') {
                //Riddle me this: Is it safe to skip *every* escaped char?
                advance_char(i, ch, chNext, chNext2);
            } else if (ch == Quote.Down) {
                Quote.Count--;
                if (Quote.Count == 0) {
                    styler.ColourTo(i, state);
                    state = SCE_RB_DEFAULT;
                    preferRE = false;
                }
            } else if (ch == Quote.Up) {
                Quote.Count++;
            } else if (ch == '#' && isInterpolableLiteral(state)) {
                if (chNext == '{') {
                    if (innerExpr.canEnter()) {
                        // process #{ ... }
                        styler.ColourTo(i - 1, state);
                        styler.ColourTo(i + 1, SCE_RB_OPERATOR);
                        innerExpr.enter(state, Quote);
                        preferRE = true;
                        // Skip one
                        advance_char(i, ch, chNext, chNext2);
                    }
                } else if (chNext == '@' || chNext == '$') {
                    InterpolateVariable(styler, state, i, ch, chNext, chNext2);
                }
            }
        }

        if (state == SCE_RB_ERROR) {
            break;
        }
        chPrev = ch;
    }
    if (state == SCE_RB_WORD) {
        // We've ended on a word, possibly at EOF, and need to
        // classify it.
        ClassifyWordRb(lengthDoc, '\0', '\0', keywords, styler, prevWord, idClasser);
    } else {
        styler.ColourTo(lengthDoc - 1, state);
    }
    styler.Flush();
}

// Helper functions for folding, disambiguation keywords
// Assert that there are no high-bit chars

std::string getPrevWord(Sci_Position pos, Accessor &styler, int word_state) {
    Sci_Position i = pos - 1;
    for (; i > 0; i--) {
        if (styler.StyleIndexAt(i) != word_state) {
            i++;
            break;
        }
    }
    std::string prevWord;
    for (; i <= pos; i++) {
        prevWord.push_back(styler[i]);
    }
    return prevWord;
}

bool keywordIsAmbiguous(const std::string &prevWord) noexcept {
    // Order from most likely used to least likely
    // Lots of ways to do a loop in Ruby besides 'while/until'
    return InList(prevWord, { "if", "do", "while", "unless", "until", "for" });
}

// Demote keywords in the following conditions:
// if, while, unless, until modify a statement
// do after a while or until, as a noise word (like then after if)

bool keywordIsModifier(const std::string &word, Sci_Position pos, Accessor &styler) {
    if (word[0] == 'd' && word[1] == 'o' && !word[2]) {
        return keywordDoStartsLoop(pos, styler);
    }
    int style = SCE_RB_DEFAULT;
    Sci_Position lineStart = styler.GetLine(pos);
    Sci_Position lineStartPosn = styler.LineStart(lineStart);
    // We want to step backwards until we don't care about the current
    // position. But first move lineStartPosn back behind any
    // continuations immediately above word.
    while (lineStartPosn > 0) {
        const char ch = styler[lineStartPosn-1];
        if (ch == '\n' || ch == '\r') {
            const char chPrev  = styler.SafeGetCharAt(lineStartPosn-2);
            const char chPrev2 = styler.SafeGetCharAt(lineStartPosn-3);
            lineStart = styler.GetLine(lineStartPosn-1);
            // If we find a continuation line, include it in our analysis.
            if (chPrev == '\\') {
                lineStartPosn = styler.LineStart(lineStart);
            } else if (ch == '\n' && chPrev == '\r' && chPrev2 == '\\') {
                lineStartPosn = styler.LineStart(lineStart);
            } else {
                break;
            }
        } else {
            break;
        }
    }

    styler.Flush();
    while (--pos >= lineStartPosn) {
        style = styler.StyleIndexAt(pos);
        if (style == SCE_RB_DEFAULT) {
            const char ch = styler[pos];
            if (IsASpaceOrTab(ch)) {
                //continue
            } else if (ch == '\r' || ch == '\n') {
                // Scintilla's LineStart() and GetLine() routines aren't
                // platform-independent, so if we have text prepared with
                // a different system we can't rely on it.

                // Also, lineStartPosn may have been moved to more than one
                // line above word's line while pushing past continuations.
                const char chPrev = styler.SafeGetCharAt(pos - 1);
                const char chPrev2 = styler.SafeGetCharAt(pos - 2);
                if (chPrev == '\\') {
                    pos-=1;  // gloss over the "\\"
                    //continue
                } else if (ch == '\n' && chPrev == '\r' && chPrev2 == '\\') {
                    pos-=2;  // gloss over the "\\\r"
                    //continue
                } else {
                    return false;
                }
            }
        } else {
            break;
        }
    }
    if (pos < lineStartPosn) {
        return false;
    }
    // First things where the action is unambiguous
    switch (style) {
    case SCE_RB_DEFAULT:
    case SCE_RB_COMMENTLINE:
    case SCE_RB_POD:
    case SCE_RB_CLASSNAME:
    case SCE_RB_DEFNAME:
    case SCE_RB_MODULE_NAME:
        return false;
    case SCE_RB_OPERATOR:
        break;
    case SCE_RB_WORD:
        // Watch out for uses of 'else if'
        //XXX: Make a list of other keywords where 'if' isn't a modifier
        //     and can appear legitimately
        // Formulate this to avoid warnings from most compilers
        if (word == "if") {
            const std::string prevWord = getPrevWord(pos, styler, SCE_RB_WORD);
            return prevWord != "else";
        }
        return true;
    default:
        return true;
    }
    // Assume that if the keyword follows an operator,
    // usually it's a block assignment, like
    // a << if x then y else z

    switch (styler[pos]) {
    case ')':
    case ']':
    case '}':
        return true;
    default:
        return false;
    }
}

#define WHILE_BACKWARDS "elihw"
#define UNTIL_BACKWARDS "litnu"
#define FOR_BACKWARDS "rof"

// Nothing fancy -- look to see if we follow a while/until somewhere
// on the current line

bool keywordDoStartsLoop(Sci_Position pos, Accessor &styler) {
    const Sci_Position lineStart = styler.GetLine(pos);
    const Sci_Position lineStartPosn = styler.LineStart(lineStart);
    styler.Flush();
    while (--pos >= lineStartPosn) {
        const int style = styler.StyleIndexAt(pos);
        if (style == SCE_RB_DEFAULT) {
            const char ch = styler[pos];
            if (ch == '\r' || ch == '\n') {
                // Scintilla's LineStart() and GetLine() routines aren't
                // platform-independent, so if we have text prepared with
                // a different system we can't rely on it.
                return false;
            }
        } else if (style == SCE_RB_WORD) {
            // Check for while or until, but write the word in backwards
            std::string prevWord;
            Sci_Position start_word = pos;
            for (;
                    start_word >= lineStartPosn && styler.StyleIndexAt(start_word) == SCE_RB_WORD;
                    start_word--) {
                prevWord.push_back(styler[start_word]);
            }
            // Did we see our keyword?
            if (InList(prevWord, { WHILE_BACKWARDS, UNTIL_BACKWARDS, FOR_BACKWARDS } )) {
                return true;
            }
            // We can move pos to the beginning of the keyword, and then
            // accept another decrement, as we can never have two contiguous
            // keywords:
            // word1 word2
            //           ^
            //        <-  move to start_word
            //      ^
            //      <- loop decrement
            //     ^  # pointing to end of word1 is fine
            pos = start_word;
        }
    }
    return false;
}

bool IsCommentLine(Sci_Position line, Accessor &styler) {
    const Sci_Position pos = styler.LineStart(line);
    const Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
    for (Sci_Position i = pos; i < eol_pos; i++) {
        const char ch = styler[i];
        if (ch == '#')
            return true;
        else if (ch != ' ' && ch != '\t')
            return false;
    }
    return false;
}

/*
 *  Folding Ruby
 *
 *  The language is quite complex to analyze without a full parse.
 *  For example, this line shouldn't affect fold level:
 *
 *   print "hello" if feeling_friendly?
 *
 *  Neither should this:
 *
 *   print "hello" \
 *      if feeling_friendly?
 *
 *
 *  But this should:
 *
 *   if feeling_friendly?  #++
 *     print "hello" \
 *     print "goodbye"
 *   end                   #--
 *
 *  So we cheat, by actually looking at the existing indentation
 *  levels for each line, and just echoing it back.  Like Python.
 *  Then if we get better at it, we'll take braces into consideration,
 *  which always affect folding levels.

 *  How the keywords should work:
 *  No effect:
 *  __FILE__ __LINE__ BEGIN END alias and
 *  defined? false in nil not or self super then
 *  true undef

 *  Always increment:
 *  begin  class def do for module when {
 *
 *  Always decrement:
 *  end }
 *
 *  Increment if these start a statement
 *  if unless until while -- do nothing if they're modifiers

 *  These end a block if there's no modifier, but don't bother
 *  break next redo retry return yield
 *
 *  These temporarily de-indent, but re-indent
 *  case else elsif ensure rescue
 *
 *  This means that the folder reflects indentation rather
 *  than setting it.  The language-service updates indentation
 *  when users type return and finishes entering de-denters.
 *
 *  Later offer to fold POD, here-docs, strings, and blocks of comments
 */

void LexerRuby::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
    Accessor styler(pAccess, nullptr);

    synchronizeDocStart(startPos, length, initStyle, styler, false);
    const Sci_PositionU endPos = startPos + length;
    int visibleChars = 0;
    Sci_Position lineCurrent = styler.GetLine(startPos);
    int levelPrev = startPos == 0 ? 0 : (styler.LevelAt(lineCurrent)
                                         & SC_FOLDLEVELNUMBERMASK
                                         & ~SC_FOLDLEVELBASE);
    int levelCurrent = levelPrev;
    char chPrev = '\0';
    char chNext = styler[startPos];
    int styleNext = styler.StyleIndexAt(startPos);
    int stylePrev = startPos <= 1 ? SCE_RB_DEFAULT : styler.StyleIndexAt(startPos - 1);
    // detect endless method definition to fix up code folding
    enum class MethodDefinition {
        None,
        Define,
        Operator,
        Name,
        Argument,
    };
    MethodDefinition method_definition = MethodDefinition::None;
    int argument_paren_count = 0;
    bool heredocOpen = false;

    for (Sci_PositionU i = startPos; i < endPos; i++) {
        const char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        const int style = styleNext;
        styleNext = styler.StyleIndexAt(i + 1);
        const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

        /* Multiline comment patch */
        if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler)) {
            if (!IsCommentLine(lineCurrent - 1, styler)
                    && IsCommentLine(lineCurrent + 1, styler))
                levelCurrent++;
            else if (IsCommentLine(lineCurrent - 1, styler)
                     && !IsCommentLine(lineCurrent + 1, styler))
                levelCurrent--;
        }

        if (style == SCE_RB_COMMENTLINE) {
            if (options.foldComment && stylePrev != SCE_RB_COMMENTLINE) {
                if (chNext == '{') {
                    levelCurrent++;
                } else if (chNext == '}' && levelCurrent > 0) {
                    levelCurrent--;
                }
            }
        } else if (style == SCE_RB_OPERATOR) {
            if (AnyOf(ch, '[', '{', '(')) {
                levelCurrent++;
            } else if (AnyOf(ch, ']', '}', ')')) {
                // Don't decrement below 0
                if (levelCurrent > 0)
                    levelCurrent--;
            }
        } else if (style == SCE_RB_WORD && styleNext != SCE_RB_WORD) {
            // Look at the keyword on the left and decide what to do
            const std::string prevWord = getPrevWord(i, styler, SCE_RB_WORD);
            if (prevWord == "end") {
                // Don't decrement below 0
                if (levelCurrent > 0)
                    levelCurrent--;
            } else if (prevWord == "def") {
                levelCurrent++;
                method_definition = MethodDefinition::Define;
			} else if (InList(prevWord, {
                    "if",
                    "class",
                    "module",
                    "begin",
                    "case",
                    "do",
                    "while",
                    "unless",
                    "until",
                    "for"
				})) {
				levelCurrent++;
			}
        } else if (style == SCE_RB_HERE_DELIM && !heredocOpen) {
            if (stylePrev == SCE_RB_OPERATOR && chPrev == '<' && styler.SafeGetCharAt(i - 2) == '<') {
                levelCurrent++;
                heredocOpen = true;
            } else if (styleNext != SCE_RB_HERE_DELIM) {
                levelCurrent--;
            }
        } else if (style == SCE_RB_STRING_QW || style == SCE_RB_STRING_W) {
            if (stylePrev != style) {
                levelCurrent++;
            }
            if (styleNext != style) {
                levelCurrent--;
            }
        }
        if (method_definition != MethodDefinition::None) {
            switch (method_definition) {
            case MethodDefinition::Define:
                if (style == SCE_RB_OPERATOR) {
                    method_definition = MethodDefinition::Operator;
                } else if (style == SCE_RB_DEFNAME || style == SCE_RB_WORD_DEMOTED || style == SCE_RB_CLASSNAME || IsIdentifierStyle(style)) {
                    method_definition = MethodDefinition::Name;
                } else if (!(style == SCE_RB_WORD || IsASpaceOrTab(ch))) {
                    method_definition = MethodDefinition::None;
                }
                if (method_definition <= MethodDefinition::Define) {
                    break;
                }
                // fall through for unary operator or single letter name
                [[fallthrough]];
            case MethodDefinition::Operator:
            case MethodDefinition::Name:
                if (isEOLChar(chNext) || chNext == '#') {
                    method_definition = MethodDefinition::None;
                } else if (chNext == '(' || chNext <= ' ') {
                    // setter method cannot be defined in an endless method definition.
                    if (ch == '=' && (method_definition == MethodDefinition::Name || chPrev == ']')) {
                        method_definition = MethodDefinition::None;
                    } else {
                        method_definition = MethodDefinition::Argument;
                        argument_paren_count = 0;
                    }
                }
                break;
            case MethodDefinition::Argument:
                if (style == SCE_RB_OPERATOR) {
                    if (ch == '(') {
                        ++argument_paren_count;
                    } else if (ch == ')') {
                        --argument_paren_count;
                    } else if (argument_paren_count == 0) {
                        method_definition = MethodDefinition::None;
                        if (ch == '=' && levelCurrent > 0) {
                            levelCurrent--;
                        }
                    }
                } else if (argument_paren_count == 0 && !IsASpaceOrTab(ch)) {
                    // '=' must be first character after method name or right parenthesis
                    method_definition = MethodDefinition::None;
                }
                break;
            default:
                break;
            }
        }
        if (atEOL || (i == endPos - 1)) {
            int lev = levelPrev;
            if (visibleChars == 0 && options.foldCompact)
                lev |= SC_FOLDLEVELWHITEFLAG;
            if ((levelCurrent > levelPrev) && (visibleChars > 0))
                lev |= SC_FOLDLEVELHEADERFLAG;
            styler.SetLevel(lineCurrent, lev|SC_FOLDLEVELBASE);
            lineCurrent++;
            levelPrev = levelCurrent;
            visibleChars = 0;
            method_definition = MethodDefinition::None;
            argument_paren_count = 0;
            heredocOpen = false;
        } else if (!isspacechar(ch)) {
            visibleChars++;
        }
        chPrev = ch;
        stylePrev = style;
    }
}

}

extern const LexerModule lmRuby(SCLEX_RUBY, LexerRuby::LexerFactoryRuby, "ruby", rubyWordListDesc);
