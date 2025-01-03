// Scintilla source code edit control
/** @file LexNix.cxx
 ** Lexer for Nix.
 **/
// Based on Zufu Liu's Notepad4 Dart lexer
// Modified for Nix and Scintilla by Jiri Techet, 2024
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>
#include <cstring>

#include <string>
#include <string_view>
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
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {
// Use an unnamed namespace to protect the functions and classes from name conflicts

constexpr bool IsAGraphic(int ch) noexcept {
	// excludes C0 control characters and whitespace
	return ch > 32 && ch < 127;
}

constexpr bool IsIdentifierChar(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '_' || ch == '\'' || ch == '-';
}

constexpr bool IsIdentifierStart(int ch) noexcept {
	return IsUpperOrLowerCase(ch) || ch == '_';
}

constexpr bool IsNumberContinue(int chPrev, int ch, int chNext) noexcept {
	return ((ch == '+' || ch == '-') && (chPrev == 'e' || chPrev == 'E'))
		|| (ch == '.' && chNext != '.');
}

constexpr bool IsNumberStart(int ch, int chNext) noexcept {
	return IsADigit(ch) || (ch == '.' && IsADigit(chNext));
}

constexpr bool IsDecimalNumber(int chPrev, int ch, int chNext) noexcept {
	return IsIdentifierChar(ch) || IsNumberContinue(chPrev, ch, chNext);
}

constexpr bool IsNixIdentifierStart(int ch) noexcept {
	return IsIdentifierStart(ch);
}

constexpr bool IsNixIdentifierChar(int ch) noexcept {
	return IsIdentifierChar(ch);
}

constexpr bool IsPathTerminator(int ch) noexcept {
	return isspacechar(ch) || AnyOf(ch, ';', ',', ']', '}', ')');
}

bool IsKey(LexAccessor &styler, Sci_Position start) {
	for (Sci_Position i = 0; i < 50; i++) {
		char curr = styler.SafeGetCharAt(start+i, '\0');
		char next = styler.SafeGetCharAt(start+i+1, '\0');
		bool atEOL = (curr == '\r' && next != '\n') || (curr == '\n') || (curr == '\0');
		if (curr == '=') {
			return true;
		} else if (!isspacechar(curr) || atEOL) {
			return false;
		}
	}
	return false;
}

bool IsPath(LexAccessor &styler, Sci_Position start) {
	for (Sci_Position i = 0; i < 50; i++) {
		char curr = styler.SafeGetCharAt(start+i, '\0');
		char next = styler.SafeGetCharAt(start+i+1, '\0');
		bool atEOL = (curr == '\r' && next != '\n') || (curr == '\n') || (curr == '\0');
		if (curr == '/') {
			return true;
		} else if (IsPathTerminator(curr) || curr == '$' || atEOL) {
			return false;
		}
	}
	return false;
}

enum {
	NixLineStateMaskInterpolation = 1,	// string interpolation
};

// string interpolating state
struct InterpolatingState {
	int state;
	int braceCount;
};

// Options used for LexerNix
struct OptionsNix {
	bool fold = false;
};

const char * const nixWordListDesc[] = {
	"Keywords 1",
	"Keywords 2",
	"Keywords 3",
	"Keywords 4",
	nullptr
};

struct OptionSetNix : public OptionSet<OptionsNix> {
	OptionSetNix() {
		DefineProperty("fold", &OptionsNix::fold);

		DefineWordListSets(nixWordListDesc);
	}
};


LexicalClass lexicalClasses[] = {
	// Lexer NIX SCLEX_NIX SCE_NIX_:
	0, "SCE_NIX_DEFAULT", "default", "White space",
	1, "SCE_NIX_COMMENTLINE", "comment line", "Comment: //",
	2, "SCE_NIX_COMMENTBLOCK", "comment", "Comment: /* */",
	3, "SCE_NIX_STRING", "literal string", "Double quoted string",
	4, "SCE_NIX_STRING_MULTILINE", "literal string multiline", "Single quoted multiline string",
	5, "SCE_NIX_ESCAPECHAR", "literal string escapesequence", "Escape sequence",
	6, "SCE_NIX_IDENTIFIER", "identifier", "Identifier",
	7, "SCE_NIX_OPERATOR", "operator", "Operator",
	8, "SCE_NIX_OPERATOR_STRING", "operator interpolated", "Braces following $ inside string",
	9, "SCE_NIX_NUMBER", "literal numeric", "Number",
	10, "SCE_NIX_KEY", "key", "Keys preceding '='",
	11, "SCE_NIX_PATH", "identifier", "Path literal",
	12, "SCE_NIX_KEYWORD1", "keyword", "Primary keywords",
	13, "SCE_NIX_KEYWORD2", "identifier", "Keywords 2",
	14, "SCE_NIX_KEYWORD3", "identifier", "Keywords 3",
	15, "SCE_NIX_KEYWORD4", "identifier", "Keywords 4",
};

class LexerNix : public DefaultLexer {
	WordList keywordsPrimary;
	WordList keywordsSecondary;
	WordList keywordsTertiary;
	WordList keywordsTypes;
	OptionsNix options;
	OptionSetNix osNix;
public:
	LexerNix(const char *languageName_, int language_) :
		DefaultLexer(languageName_, language_, lexicalClasses, std::size(lexicalClasses)) {
	}
	// Deleted so LexerNix objects can not be copied.
	LexerNix(const LexerNix &) = delete;
	LexerNix(LexerNix &&) = delete;
	void operator=(const LexerNix &) = delete;
	void operator=(LexerNix &&) = delete;
	~LexerNix() override = default;

	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osNix.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osNix.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osNix.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osNix.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osNix.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void *SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}

	void BacktrackToStart(const LexAccessor &styler, int stateMask, Sci_PositionU &startPos, Sci_Position &lengthDoc, int &initStyle);

	static ILexer5 *LexerFactoryNix() {
		return new LexerNix("nix", SCLEX_NIX);
	}
};

Sci_Position SCI_METHOD LexerNix::PropertySet(const char *key, const char *val) {
	if (osNix.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerNix::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywordsPrimary;
		break;
	case 1:
		wordListN = &keywordsSecondary;
		break;
	case 2:
		wordListN = &keywordsTertiary;
		break;
	case 3:
		wordListN = &keywordsTypes;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN && wordListN->Set(wl/*, false*/)) {
		firstModification = 0;
	}
	return firstModification;
}

void LexerNix::BacktrackToStart(const LexAccessor &styler, int stateMask, Sci_PositionU &startPos, Sci_Position &lengthDoc, int &initStyle) {
	const Sci_Position currentLine = styler.GetLine(startPos);
	if (currentLine != 0) {
		Sci_Position line = currentLine - 1;
		int lineState = styler.GetLineState(line);
		while ((lineState & stateMask) != 0 && line != 0) {
			--line;
			lineState = styler.GetLineState(line);
		}
		if ((lineState & stateMask) == 0) {
			++line;
		}
		if (line != currentLine) {
			const Sci_PositionU endPos = startPos + lengthDoc;
			startPos = (line == 0) ? 0 : styler.LineStart(line);
			lengthDoc = endPos - startPos;
			initStyle = (startPos == 0) ? 0 : styler.StyleAt(startPos - 1);
		}
	}
}

void LexerNix::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);

	std::vector<InterpolatingState> interpolatingStack;

	if (startPos != 0) {
		// backtrack to the line where interpolation starts
		BacktrackToStart(styler, NixLineStateMaskInterpolation, startPos, lengthDoc, initStyle);
	}

	StyleContext sc(startPos, lengthDoc, initStyle, styler);

	while (sc.More()) {
		switch (sc.state) {
		case SCE_NIX_OPERATOR:
		case SCE_NIX_OPERATOR_STRING:
			sc.SetState(SCE_NIX_DEFAULT);
			break;

		case SCE_NIX_NUMBER:
			if (!IsDecimalNumber(sc.chPrev, sc.ch, sc.chNext)) {
				sc.SetState(SCE_NIX_DEFAULT);
			}
			break;

		case SCE_NIX_IDENTIFIER:
			if (sc.state == SCE_NIX_IDENTIFIER && !IsNixIdentifierChar(sc.ch)) {
				char s[64];
				sc.GetCurrent(s, sizeof(s));
				if (IsKey(styler, sc.currentPos)) {
					sc.ChangeState(SCE_NIX_KEY);
				} else if (keywordsPrimary.InList(s)) {
					sc.ChangeState(SCE_NIX_KEYWORD1);
				} else if (keywordsSecondary.InList(s)) {
					sc.ChangeState(SCE_NIX_KEYWORD2);
				} else if (keywordsTertiary.InList(s)) {
					sc.ChangeState(SCE_NIX_KEYWORD3);
				} else if (keywordsTypes.InList(s)) {
					sc.ChangeState(SCE_NIX_KEYWORD4);
				}

				sc.SetState(SCE_NIX_DEFAULT);
			}
			break;

		case SCE_NIX_COMMENTLINE:
			if (sc.atLineStart) {
				sc.SetState(SCE_NIX_DEFAULT);
			}
			break;

		case SCE_NIX_COMMENTBLOCK:
			if (sc.Match('*', '/')) {
				sc.Forward(2);
				sc.SetState(SCE_NIX_DEFAULT);
				continue;
			}
			break;

		case SCE_NIX_STRING:
			if (sc.atLineStart) {
				sc.SetState(SCE_NIX_DEFAULT);
			} else if (sc.ch == '\\' && AnyOf(sc.chNext, '"', '\\', 'n', 'r', 't', '$')) {
				sc.SetState(SCE_NIX_STRING);
				sc.ChangeState(SCE_NIX_ESCAPECHAR);
				sc.Forward(2);
				sc.SetState(SCE_NIX_STRING);
				continue;
			} else if (sc.ch == '\"') {
				sc.Forward();
				sc.SetState(SCE_NIX_DEFAULT);
				continue;
			} else if (sc.Match('$','$')) {
				sc.Forward();
				continue;
			}
			break;

		case SCE_NIX_PATH:
			if (sc.atLineStart) {
				sc.SetState(SCE_NIX_DEFAULT);
			} else if (sc.ch == '>') {
				sc.Forward();
				sc.SetState(SCE_NIX_DEFAULT);
				continue;
			} else if (IsPathTerminator(sc.ch)) {
				sc.SetState(SCE_NIX_DEFAULT);
			}
			break;

		case SCE_NIX_STRING_MULTILINE:
			if (sc.ch == '\'' && sc.chNext == '\'') {
				if (AnyOf(styler.SafeGetCharAt(sc.currentPos+2, '\0'), '$', '\'', '\\')) {
					sc.SetState(SCE_NIX_ESCAPECHAR);
					sc.Forward(2);
					if (sc.ch == '$' || sc.ch == '\'') {
						sc.Forward();
					} else if (sc.ch == '\\') {
						sc.Forward(2);
					}
					sc.SetState(SCE_NIX_STRING_MULTILINE);
					continue;
				} else {
					sc.Forward(2);
					sc.SetState(SCE_NIX_DEFAULT);
					continue;
				}
			} else if (sc.Match('$','$')) {
				sc.Forward();
				continue;
			}
			break;
		}

		if (sc.state == SCE_NIX_DEFAULT)
		{
			if (sc.ch == '/' && sc.chNext == '*') {
				sc.SetState(SCE_NIX_COMMENTBLOCK);
				sc.Forward();
				continue;
			} else if (sc.ch == '#') {
				sc.SetState(SCE_NIX_COMMENTLINE);
			} else if (sc.ch == '"') {
				sc.SetState(SCE_NIX_STRING);
			} else if (sc.ch == '\'' && sc.chNext == '\'') {
				sc.SetState(SCE_NIX_STRING_MULTILINE);
				sc.Forward();
				continue;
			} else if (IsNumberStart(sc.ch, sc.chNext)) {
				sc.SetState(SCE_NIX_NUMBER);
			} else if (sc.ch == '<') {
				sc.SetState(SCE_NIX_PATH);
			} else if ((IsNixIdentifierStart(sc.ch) || AnyOf(sc.ch, '~', '.', '/')) &&
					IsPath(styler, sc.currentPos)) {
				sc.SetState(SCE_NIX_PATH);
			} else if (IsNixIdentifierStart(sc.ch)) {
				sc.SetState(SCE_NIX_IDENTIFIER);
			} else if (IsAGraphic(sc.ch) && sc.ch != '$') {
				sc.SetState(SCE_NIX_OPERATOR);
				if (!interpolatingStack.empty() && AnyOf(sc.ch, '{', '}')) {
					InterpolatingState &current = interpolatingStack.back();
					if (sc.ch == '{') {
						current.braceCount += 1;
					} else {
						current.braceCount -= 1;
						if (current.braceCount == 0) {
							sc.ChangeState(SCE_NIX_OPERATOR_STRING);
							sc.ForwardSetState(current.state);
							interpolatingStack.pop_back();
							continue;
						}
					}
				}
			}
		}

		// string interpolations
		if (AnyOf(sc.state, SCE_NIX_DEFAULT, SCE_NIX_STRING, SCE_NIX_STRING_MULTILINE, SCE_NIX_PATH)) {
			if (sc.Match('$','{')) {
				interpolatingStack.push_back({sc.state, 1});
				sc.SetState(SCE_NIX_OPERATOR_STRING);
				sc.Forward();
			}
		}

		if (sc.atLineEnd) {
			int lineState = 0;
			if (!interpolatingStack.empty()) {
				lineState |= NixLineStateMaskInterpolation;
			}
			styler.SetLineState(sc.currentLine, lineState);
		}

		sc.Forward();
	}

	sc.Complete();
}

void LexerNix::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) {
	if (!options.fold)
		return;

	Accessor styler(pAccess, nullptr);
	const Sci_PositionU endPos = startPos + lengthDoc;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	while (lineCurrent > 0) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
		initStyle = (startPos > 0) ? styler.StyleIndexAt(startPos) : 0;
		if (!AnyOf(initStyle, SCE_NIX_COMMENTBLOCK, SCE_NIX_STRING_MULTILINE)) {
			break;
		}
	}
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0) {
		levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
	}

	int levelNext = levelCurrent;
	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent + 1);
	lineStartNext = std::min(lineStartNext, endPos);

	char chNext = styler[startPos];
	int styleNext = styler.StyleIndexAt(startPos);
	int style = initStyle;

	while (startPos < endPos) {
		const char ch = chNext;
		const int stylePrev = style;
		style = styleNext;
		chNext = styler[++startPos];
		styleNext = styler.StyleIndexAt(startPos);

		switch (style) {
		case SCE_NIX_COMMENTBLOCK:
			if (style != stylePrev) {
				levelNext++;
			}
			if (style != styleNext) {
				levelNext--;
			}
			break;

		case SCE_NIX_STRING_MULTILINE:
			if (style != stylePrev && !AnyOf(stylePrev, SCE_NIX_ESCAPECHAR, SCE_NIX_OPERATOR_STRING)) {
				levelNext++;
			}
			if (style != styleNext && !AnyOf(styleNext, SCE_NIX_ESCAPECHAR, SCE_NIX_OPERATOR_STRING)) {
				levelNext--;
			}
			break;

		case SCE_NIX_OPERATOR:
		case SCE_NIX_OPERATOR_STRING:
			if (ch == '{' || ch == '[' || ch == '(') {
				levelNext++;
			} else if (ch == '}' || ch == ']' || ch == ')') {
				levelNext--;
			}
			break;
		}

		if (startPos == lineStartNext) {
			levelNext = std::max(levelNext, SC_FOLDLEVELBASE);

			const int levelUse = levelCurrent;
			int lev = levelUse | (levelNext << 16);
			if (levelUse < levelNext) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			styler.SetLevel(lineCurrent, lev);

			lineCurrent++;
			lineStartNext = styler.LineStart(lineCurrent + 1);
			lineStartNext = std::min(lineStartNext, endPos);
			levelCurrent = levelNext;
		}
	}
}

}  // unnamed namespace end

extern const LexerModule lmNix(SCLEX_NIX, LexerNix::LexerFactoryNix, "nix", nixWordListDesc);
