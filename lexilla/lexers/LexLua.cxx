// Scintilla source code edit control
/** @file LexLua.cxx
 ** Lexer for Lua language.
 **
 ** Written by Paul Winwood.
 ** Folder by Alexey Yutkin.
 ** Modified by Marcos E. Wurzius & Philippe Lhoste
 **/

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <string>
#include <string_view>
#include <map>

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
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

// Test for [=[ ... ]=] delimiters, returns 0 if it's only a [ or ],
// return 1 for [[ or ]], returns >=2 for [=[ or ]=] and so on.
// The maximum number of '=' characters allowed is 254.
int LongDelimCheck(StyleContext &sc) {
	int sep = 1;
	while (sc.GetRelative(sep) == '=' && sep < 0xFF)
		sep++;
	if (sc.GetRelative(sep) == sc.ch)
		return sep;
	return 0;
}

const char * const luaWordListDesc[] = {
	"Keywords",
	"Basic functions",
	"String, (table) & math functions",
	"(coroutines), I/O & system facilities",
	"user1",
	"user2",
	"user3",
	"user4",
    nullptr
};

const LexicalClass lexicalClasses[] = {
	// Lexer Lua SCLEX_LUA SCE_LUA_:
	0, "SCE_LUA_DEFAULT", "default", "White space: Visible only in View Whitespace mode (or if it has a back colour)",
	1, "SCE_LUA_COMMENT", "comment", "Block comment (Lua 5.0)",
	2, "SCE_LUA_COMMENTLINE", "comment line", "Line comment",
	3, "SCE_LUA_COMMENTDOC", "comment documentation", "Doc comment",
	4, "SCE_LUA_NUMBER", "literal numeric", "Number",
	5, "SCE_LUA_WORD", "keyword", "Keyword",
	6, "SCE_LUA_STRING", "literal string", "(Double quoted) String",
	7, "SCE_LUA_CHARACTER", "literal string character", "Character (Single quoted string)",
	8, "SCE_LUA_LITERALSTRING", "literal string", "Literal string",
	9, "SCE_LUA_PREPROCESSOR", "preprocessor", "Preprocessor (obsolete in Lua 4.0 and up)",
	10, "SCE_LUA_OPERATOR", "operator", "Operators",
	11, "SCE_LUA_IDENTIFIER", "identifier", "Identifier (everything else...)",
	12, "SCE_LUA_STRINGEOL", "error literal string", "End of line where string is not closed",
	13, "SCE_LUA_WORD2", "identifier", "Other keywords",
	14, "SCE_LUA_WORD3", "identifier", "Other keywords",
	15, "SCE_LUA_WORD4", "identifier", "Other keywords",
	16, "SCE_LUA_WORD5", "identifier", "Other keywords",
	17, "SCE_LUA_WORD6", "identifier", "Other keywords",
	18, "SCE_LUA_WORD7", "identifier", "Other keywords",
	19, "SCE_LUA_WORD8", "identifier", "Other keywords",
	20, "SCE_LUA_LABEL", "label", "Labels",
};

// Options used for LexerLua
struct OptionsLua {
	bool foldCompact = true;
};

struct OptionSetLua : public OptionSet<OptionsLua> {
	OptionSetLua() {
		DefineProperty("fold.compact", &OptionsLua::foldCompact);

		DefineWordListSets(luaWordListDesc);
	}
};

class LexerLua : public DefaultLexer {
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6;
	WordList keywords7;
	WordList keywords8;
	OptionsLua options;
	OptionSetLua osLua;
public:
	explicit LexerLua() :
		DefaultLexer("lua", SCLEX_LUA, lexicalClasses, std::size(lexicalClasses)) {
	}
	~LexerLua() override = default;
	void SCI_METHOD Release() noexcept override {
		delete this;
	}
	int SCI_METHOD Version() const noexcept override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() noexcept override {
		return osLua.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osLua.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osLua.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osLua.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() noexcept override {
		return osLua.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	static ILexer5 *LexerFactoryLua() {
		return new LexerLua();
	}
};

Sci_Position SCI_METHOD LexerLua::PropertySet(const char *key, const char *val) {
	if (osLua.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerLua::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
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
	case 3:
		wordListN = &keywords4;
		break;
	case 4:
		wordListN = &keywords5;
		break;
	case 5:
		wordListN = &keywords6;
		break;
	case 6:
		wordListN = &keywords7;
		break;
	case 7:
		wordListN = &keywords8;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

constexpr int maskSeparator = 0xFF;
constexpr int maskStringWs = 0x100;
constexpr int maskDocComment = 0x200;

void LexerLua::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	// Accepts accented characters
	const CharacterSet setWordStart(CharacterSet::setAlpha, "_", true);
	const CharacterSet setWord(CharacterSet::setAlphaNum, "_", true);
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases. [pP] is for hex floats.
	const CharacterSet setNumber(CharacterSet::setDigits, ".-+abcdefpABCDEFP");
	const CharacterSet setExponent("eEpP");
	const CharacterSet setLuaOperator("*/-+()={}~[];<>,.^%:#&|");
	const CharacterSet setEscapeSkip("\"'\\");

	Sci_Position currentLine = styler.GetLine(startPos);
	// Initialize long string [[ ... ]] or block comment --[[ ... ]],
	// if we are inside such a string. Block comment was introduced in Lua 5.0,
	// blocks with separators [=[ ... ]=] in Lua 5.1.
	// Continuation of a string (\z whitespace escaping) is controlled by stringWs.
	int sepCount = 0;
	int stringWs = 0;
	int lastLineDocComment = 0;
	if ((currentLine > 0) &&
		AnyOf(initStyle, SCE_LUA_DEFAULT, SCE_LUA_LITERALSTRING, SCE_LUA_COMMENT, SCE_LUA_COMMENTDOC, SCE_LUA_STRING, SCE_LUA_CHARACTER)) {
		const int lineState = styler.GetLineState(currentLine - 1);
		sepCount = lineState & maskSeparator;
		stringWs = lineState & maskStringWs;
		lastLineDocComment = lineState & maskDocComment;
	}

	// results of identifier/keyword matching
	Sci_Position idenPos = 0;
	Sci_Position idenWordPos = 0;
	int idenStyle = SCE_LUA_IDENTIFIER;
	bool foundGoto = false;

	// Do not leak onto next line
	if (AnyOf(initStyle, SCE_LUA_STRINGEOL, SCE_LUA_COMMENTLINE, SCE_LUA_COMMENTDOC, SCE_LUA_PREPROCESSOR)) {
		initStyle = SCE_LUA_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);
	if (startPos == 0 && sc.ch == '#' && sc.chNext == '!') {
		// shbang line: "#!" is a comment only if located at the start of the script
		sc.SetState(SCE_LUA_COMMENTLINE);
	}
	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			switch (sc.state) {
			case SCE_LUA_DEFAULT:
			case SCE_LUA_LITERALSTRING:
			case SCE_LUA_COMMENT:
			case SCE_LUA_COMMENTDOC:
			case SCE_LUA_STRING:
			case SCE_LUA_CHARACTER:
				// Inside a literal string, block comment or string, we set the line state
				styler.SetLineState(currentLine, lastLineDocComment | stringWs | sepCount);
				break;
			default:
				// Reset the line state
				styler.SetLineState(currentLine, 0);
				break;
			}
		}
		if (sc.atLineStart && (sc.state == SCE_LUA_STRING)) {
			// Prevent SCE_LUA_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_LUA_STRING);
		}

		// Handle string line continuation
		if ((sc.state == SCE_LUA_STRING || sc.state == SCE_LUA_CHARACTER) &&
				sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_LUA_OPERATOR) {
			if (sc.ch == ':' && sc.chPrev == ':') {	// :: <label> :: forward scan
				sc.Forward();
				Sci_Position ln = 0;
				while (IsASpaceOrTab(sc.GetRelative(ln)))	// skip over spaces/tabs
					ln++;
				const Sci_Position ws1 = ln;
				if (setWordStart.Contains(sc.GetRelative(ln))) {
					int c = 0;
					std::string s;
					while (setWord.Contains(c = sc.GetRelative(ln))) {	// get potential label
						s.push_back(static_cast<char>(c));
						ln++;
					}
					const Sci_Position lbl = ln;
					if (!keywords.InList(s)) {
						while (IsASpaceOrTab(sc.GetRelative(ln)))	// skip over spaces/tabs
							ln++;
						const Sci_Position ws2 = ln - lbl;
						if (sc.GetRelative(ln) == ':' && sc.GetRelative(ln + 1) == ':') {
							// final :: found, complete valid label construct
							sc.ChangeState(SCE_LUA_LABEL);
							if (ws1) {
								sc.SetState(SCE_LUA_DEFAULT);
								sc.ForwardBytes(ws1);
							}
							sc.SetState(SCE_LUA_LABEL);
							sc.ForwardBytes(lbl - ws1);
							if (ws2) {
								sc.SetState(SCE_LUA_DEFAULT);
								sc.ForwardBytes(ws2);
							}
							sc.SetState(SCE_LUA_LABEL);
							sc.ForwardBytes(2);
						}
					}
				}
			}
			sc.SetState(SCE_LUA_DEFAULT);
		} else if (sc.state == SCE_LUA_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eEpP non-sign non-hexdigit char
			if (!setNumber.Contains(sc.ch)) {
				sc.SetState(SCE_LUA_DEFAULT);
			} else if (sc.ch == '-' || sc.ch == '+') {
				if (!setExponent.Contains(sc.chPrev))
					sc.SetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_IDENTIFIER) {
			idenPos--;			// commit already-scanned identifier/word parts
			if (idenWordPos > 0) {
				idenWordPos--;
				sc.ChangeState(idenStyle);
				sc.ForwardBytes(idenWordPos);
				idenPos -= idenWordPos;
				if (idenPos > 0) {
					sc.SetState(SCE_LUA_IDENTIFIER);
					sc.ForwardBytes(idenPos);
				}
			} else {
				sc.ForwardBytes(idenPos);
			}
			sc.SetState(SCE_LUA_DEFAULT);
			if (foundGoto) {					// goto <label> forward scan
				while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd)
					sc.Forward();
				if (setWordStart.Contains(sc.ch)) {
					sc.SetState(SCE_LUA_LABEL);
					sc.Forward();
					while (setWord.Contains(sc.ch))
						sc.Forward();
					std::string s;
					sc.GetCurrentString(s, StyleContext::Transform::none);
					if (keywords.InList(s))		// labels cannot be keywords
						sc.ChangeState(SCE_LUA_WORD);
				}
				sc.SetState(SCE_LUA_DEFAULT);
			}
		} else if (AnyOf(sc.state, SCE_LUA_COMMENTLINE, SCE_LUA_COMMENTDOC, SCE_LUA_PREPROCESSOR)) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_STRING) {
			if (stringWs) {
				if (!IsASpace(sc.ch))
					stringWs = 0;
			}
			if (sc.ch == '\\') {
				if (setEscapeSkip.Contains(sc.chNext)) {
					sc.Forward();
				} else if (sc.chNext == 'z') {
					sc.Forward();
					stringWs = maskStringWs;
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			} else if (stringWs == 0 && sc.atLineEnd) {
				sc.ChangeState(SCE_LUA_STRINGEOL);
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_CHARACTER) {
			if (stringWs) {
				if (!IsASpace(sc.ch))
					stringWs = 0;
			}
			if (sc.ch == '\\') {
				if (setEscapeSkip.Contains(sc.chNext)) {
					sc.Forward();
				} else if (sc.chNext == 'z') {
					sc.Forward();
					stringWs = maskStringWs;
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			} else if (stringWs == 0 && sc.atLineEnd) {
				sc.ChangeState(SCE_LUA_STRINGEOL);
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.ch == ']' && (sc.state == SCE_LUA_LITERALSTRING || sc.state == SCE_LUA_COMMENT)) {
			const int sep = LongDelimCheck(sc);
			if (sep == sepCount) {   // ]=]-style delim
				sc.Forward(sep);
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_LUA_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_LUA_NUMBER);
				if (sc.ch == '0' && AnyOf(sc.chNext, 'x', 'X')) {
					sc.Forward();
				}
			} else if (setWordStart.Contains(sc.ch)) {
				// For matching various identifiers with dots and colons, multiple
				// matches are done as identifier segments are added. Longest match is
				// set to a word style. The non-matched part is in identifier style.
				std::string ident;
				idenPos = 0;
				idenWordPos = 0;
				idenStyle = SCE_LUA_IDENTIFIER;
				foundGoto = false;
				int cNext = 0;
				do {
					int c = 0;
					const Sci_Position idenPosOld = idenPos;
					std::string identSeg;
					identSeg += static_cast<char>(sc.GetRelative(idenPos++));
					while (setWord.Contains(c = sc.GetRelative(idenPos))) {
						identSeg += static_cast<char>(c);
						idenPos++;
					}
					if (keywords.InList(identSeg) && (idenPosOld > 0)) {
						idenPos = idenPosOld - 1;	// keywords cannot mix
						ident.pop_back();
						break;
					}
					ident += identSeg;
					int newStyle = SCE_LUA_IDENTIFIER;
					if (keywords.InList(ident)) {
						newStyle = SCE_LUA_WORD;
					} else if (keywords2.InList(ident)) {
						newStyle = SCE_LUA_WORD2;
					} else if (keywords3.InList(ident)) {
						newStyle = SCE_LUA_WORD3;
					} else if (keywords4.InList(ident)) {
						newStyle = SCE_LUA_WORD4;
					} else if (keywords5.InList(ident)) {
						newStyle = SCE_LUA_WORD5;
					} else if (keywords6.InList(ident)) {
						newStyle = SCE_LUA_WORD6;
					} else if (keywords7.InList(ident)) {
						newStyle = SCE_LUA_WORD7;
					} else if (keywords8.InList(ident)) {
						newStyle = SCE_LUA_WORD8;
					}
					if (newStyle != SCE_LUA_IDENTIFIER) {
						idenStyle = newStyle;
						idenWordPos = idenPos;
					}
					if (idenStyle == SCE_LUA_WORD)	// keywords cannot mix
						break;
					cNext = sc.GetRelative(idenPos + 1);
					if ((c == '.' || c == ':') && setWordStart.Contains(cNext)) {
						ident += static_cast<char>(c);
						idenPos++;
					} else {
						cNext = 0;
					}
				} while (cNext);
                if ((idenStyle == SCE_LUA_WORD) && (ident == "goto")) {
					foundGoto = true;
				}
				sc.SetState(SCE_LUA_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_LUA_STRING);
				stringWs = 0;
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_LUA_CHARACTER);
				stringWs = 0;
			} else if (sc.ch == '[') {
				sepCount = LongDelimCheck(sc);
				if (sepCount == 0) {
					sc.SetState(SCE_LUA_OPERATOR);
				} else {
					sc.SetState(SCE_LUA_LITERALSTRING);
					sc.Forward(sepCount);
				}
			} else if (sc.Match('-', '-')) {
				sc.SetState(lastLineDocComment ? SCE_LUA_COMMENTDOC : SCE_LUA_COMMENTLINE);
				if (sc.Match("--[")) {
					sc.Forward(2);
					sepCount = LongDelimCheck(sc);
					if (sepCount > 0) {
						sc.ChangeState(SCE_LUA_COMMENT);
						sc.Forward(sepCount);
					}
				} else if (sc.Match("---")) {
					sc.SetState(SCE_LUA_COMMENTDOC);
					lastLineDocComment = maskDocComment;
				} else {
					sc.Forward();
				}
			} else if (sc.atLineStart && sc.Match('$')) {
				sc.SetState(SCE_LUA_PREPROCESSOR);	// Obsolete since Lua 4.0, but still in old code
			} else if (setLuaOperator.Contains(sc.ch)) {
				sc.SetState(SCE_LUA_OPERATOR);
			}
			if (!AnyOf(sc.state, SCE_LUA_DEFAULT, SCE_LUA_COMMENTDOC)) {
				lastLineDocComment = 0;
			}
		}
	}

	sc.Complete();
}

void LexerLua::Fold(Sci_PositionU startPos_, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	const Sci_Position startPos = startPos_;
	const Sci_Position lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	const bool foldCompact = options.foldCompact;
	int style = initStyle;
	int styleNext = styler.StyleIndexAt(startPos);

	for (Sci_Position i = startPos; i < lengthDoc; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleIndexAt(i + 1);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_LUA_WORD) {
			// Fixed list of folding words: if, do, function, repeat, end, until
			// Must fix up next line with initial characters if any new words added.
			if ((style != stylePrev) && AnyOf(ch, 'i', 'd', 'f', 'e', 'r', 'u')) {
				std::string s;
				for (Sci_Position j = 0; j < 8; j++) {	// 8 is length of longest: function
					if (!iswordchar(styler[i + j])) {
						break;
					}
					s.push_back(styler[i + j]);
				}

				if (s == "if" || s == "do" || s == "function" || s == "repeat") {
					levelCurrent++;
				}
				if (s == "end" || s == "until") {
					levelCurrent--;
				}
			}
		} else if (style == SCE_LUA_OPERATOR) {
			if (ch == '{' || ch == '(') {
				levelCurrent++;
			} else if (ch == '}' || ch == ')') {
				levelCurrent--;
			}
		} else if (style == SCE_LUA_LITERALSTRING || style == SCE_LUA_COMMENT) {
			if (stylePrev != style) {
				levelCurrent++;
			} else if (styleNext != style) {
				levelCurrent--;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	const int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

}

LexerModule lmLua(SCLEX_LUA, LexerLua::LexerFactoryLua, "lua", luaWordListDesc);
