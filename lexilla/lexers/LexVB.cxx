// Scintilla source code edit control
/** @file LexVB.cxx
 ** Lexer for Visual Basic and VBScript.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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

// Internal state, highlighted as number
constexpr int SCE_B_FILENUMBER = SCE_B_DEFAULT + 100;

bool IsVBComment(Accessor &styler, Sci_Position pos, Sci_Position len) {
	return len > 0 && styler[pos] == '\'';
}

constexpr bool IsTypeCharacter(int ch) noexcept {
	return ch == '%' || ch == '&' || ch == '@' || ch == '!' || ch == '#' || ch == '$';
}

// Extended to accept accented characters
constexpr bool IsAWordChar(int ch) noexcept {
	return !IsASCII(ch) ||
	       (IsAlphaNumeric(ch) || ch == '.' || ch == '_');
}

constexpr bool IsAWordStart(int ch) noexcept {
	return !IsASCII(ch) ||
	       (IsUpperOrLowerCase(ch) || ch == '_');
}

constexpr bool IsANumberChar(int ch) noexcept {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return IsAHeXDigit(ch) || AnyOf(ch, '.', '-', '+', '_');
}

// Options used for LexerVB
struct OptionsVB {
	bool fold = false;
	bool allowMultilineStr = false;
};

const char * const vbWordListDesc[] = {
	"Keywords",
	"user1",
	"user2",
	"user3",
	nullptr
};

struct OptionSetVB : public OptionSet<OptionsVB> {
	OptionSetVB() {
		DefineProperty("fold", &OptionsVB::fold);

		DefineProperty("lexer.vb.strings.multiline", &OptionsVB::allowMultilineStr,
			"Set to 1 to allow strings to continue over line ends.");

		DefineWordListSets(vbWordListDesc);
	}
};

LexicalClass lexicalClasses[] = {
	// Lexer vb SCLEX_VB SCE_B_:
	0, "SCE_B_DEFAULT", "default", "White space",
	1, "SCE_B_COMMENT", "comment", "Comment: '",
	2, "SCE_B_NUMBER", "literal numeric", "Number",
	3, "SCE_B_KEYWORD", "keyword", "Keyword",
	4, "SCE_B_STRING", "literal string", "Double quoted string",
	5, "SCE_B_PREPROCESSOR", "preprocessor", "Preprocessor",
	6, "SCE_B_OPERATOR", "operator", "Operators",
	7, "SCE_B_IDENTIFIER", "identifier", "Identifiers",
	8, "SCE_B_DATE", "literal date", "Date",
	9, "SCE_B_STRINGEOL", "error literal string", "End of line where string is not closed",
	10, "SCE_B_KEYWORD2", "identifier", "Keywords2",
	11, "SCE_B_KEYWORD3", "identifier", "Keywords3",
	12, "SCE_B_KEYWORD4", "identifier", "Keywords4",
};

class LexerVB : public DefaultLexer {
	bool vbScriptSyntax;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	OptionsVB options;
	OptionSetVB osVB;
public:
	LexerVB(const char *languageName_, int language_, bool vbScriptSyntax_) :
		DefaultLexer(languageName_, language_, lexicalClasses, std::size(lexicalClasses)),
		vbScriptSyntax(vbScriptSyntax_) {
	}
	// Deleted so LexerVB objects can not be copied.
	LexerVB(const LexerVB &) = delete;
	LexerVB(LexerVB &&) = delete;
	void operator=(const LexerVB &) = delete;
	void operator=(LexerVB &&) = delete;
	~LexerVB() override = default;
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osVB.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osVB.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osVB.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osVB.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osVB.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void CheckIdentifier(Lexilla::StyleContext &sc);

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void *SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}
	static ILexer5 *LexerFactoryVB() {
		return new LexerVB("vb", SCLEX_VB, false);
	}
	static ILexer5 *LexerFactoryVBScript() {
		return new LexerVB("vbscript", SCLEX_VBSCRIPT, true);
	}
};

Sci_Position SCI_METHOD LexerVB::PropertySet(const char *key, const char *val) {
	if (osVB.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerVB::WordListSet(int n, const char *wl) {
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
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN && wordListN->Set(wl, true)) {
		firstModification = 0;
	}
	return firstModification;
}

void LexerVB::CheckIdentifier(StyleContext &sc) {
	// In Basic (except VBScript), a variable name or a function name
	// can end with a special character indicating the type of the value
	// held or returned.
	bool skipType = false;
	if (!vbScriptSyntax && IsTypeCharacter(sc.ch)) {
		sc.Forward();	// Skip it
		skipType = true;
	}
	if (sc.ch == ']') {
		sc.Forward();
	}
	char s[100];
	sc.GetCurrentLowered(s, sizeof(s));
	if (skipType) {
		s[strlen(s) - 1] = '\0';
	}
	if (strcmp(s, "rem") == 0) {
		sc.ChangeState(SCE_B_COMMENT);
	} else {
		if (keywords.InList(s)) {
			sc.ChangeState(SCE_B_KEYWORD);
		} else if (keywords2.InList(s)) {
			sc.ChangeState(SCE_B_KEYWORD2);
		} else if (keywords3.InList(s)) {
			sc.ChangeState(SCE_B_KEYWORD3);
		} else if (keywords4.InList(s)) {
			sc.ChangeState(SCE_B_KEYWORD4);
		}	// Else, it is really an identifier...
		sc.SetState(SCE_B_DEFAULT);
	}
}

void LexerVB::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);

	styler.StartAt(startPos);

	int visibleChars = 0;
	int fileNbDigits = 0;

	// Do not leak onto next line
	if (initStyle == SCE_B_STRINGEOL || initStyle == SCE_B_COMMENT || initStyle == SCE_B_PREPROCESSOR) {
		initStyle = SCE_B_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_B_OPERATOR) {
			sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				CheckIdentifier(sc);
			}
		} else if (sc.state == SCE_B_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			// Also accepts A-F for hex. numbers
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_STRING) {
			// VB doubles quotes to preserve them, so just end this string
			// state now as a following quote will start again
			if (sc.ch == '\"') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					if (MakeLowerCase(sc.chNext) == 'c') {
						sc.Forward();
					}
					sc.ForwardSetState(SCE_B_DEFAULT);
				}
			} else if (sc.atLineEnd && !options.allowMultilineStr) {
				visibleChars = 0;
				sc.ChangeState(SCE_B_STRINGEOL);
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_COMMENT) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_PREPROCESSOR) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_FILENUMBER) {
			if (IsADigit(sc.ch)) {
				fileNbDigits++;
				if (fileNbDigits > 3) {
					sc.ChangeState(SCE_B_DATE);
				}
			} else if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ',') {
				// Regular uses: Close #1; Put #1, ...; Get #1, ... etc.
				// Too bad if date is format #27, Oct, 2003# or something like that...
				// Use regular number state
				sc.ChangeState(SCE_B_NUMBER);
				sc.SetState(SCE_B_DEFAULT);
			} else if (sc.ch == '#') {
				sc.ChangeState(SCE_B_DATE);
				sc.ForwardSetState(SCE_B_DEFAULT);
			} else {
				sc.ChangeState(SCE_B_DATE);
			}
			if (sc.state != SCE_B_FILENUMBER) {
				fileNbDigits = 0;
			}
		} else if (sc.state == SCE_B_DATE) {
			if (sc.atLineEnd) {
				visibleChars = 0;
				sc.ChangeState(SCE_B_STRINGEOL);
				sc.ForwardSetState(SCE_B_DEFAULT);
			} else if (sc.ch == '#') {
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		}

		if (sc.state == SCE_B_DEFAULT) {
			if (sc.ch == '\'') {
				sc.SetState(SCE_B_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_B_STRING);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_B_PREPROCESSOR);
			} else if (sc.ch == '#') {
				// It can be a date literal, ending with #, or a file number, from 1 to 511
				// The date literal depends on the locale, so anything can go between #'s.
				// Can be #January 1, 1993# or #1 Jan 93# or #05/11/2003#, etc.
				// So we set the FILENUMBER state, and switch to DATE if it isn't a file number
				sc.SetState(SCE_B_FILENUMBER);
			} else if (sc.ch == '&' && MakeLowerCase(sc.chNext) == 'h') {
				// Hexadecimal number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (sc.ch == '&' && MakeLowerCase(sc.chNext) == 'o') {
				// Octal number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (sc.ch == '&' && MakeLowerCase(sc.chNext) == 'b') {
				// Binary number
				sc.SetState(SCE_B_NUMBER);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_B_NUMBER);
			} else if (IsAWordStart(sc.ch) || (sc.ch == '[')) {
				sc.SetState(SCE_B_IDENTIFIER);
			} else if (isoperator(sc.ch) || (sc.ch == '\\')) {	// Integer division
				sc.SetState(SCE_B_OPERATOR);
			}
		}

		if (sc.atLineEnd) {
			visibleChars = 0;
		}
		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}

	if (sc.state == SCE_B_IDENTIFIER && !IsAWordChar(sc.ch)) {
		CheckIdentifier(sc);
	}

	sc.Complete();
}

void LexerVB::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess) {
	if (!options.fold)
		return;

	Accessor styler(pAccess, nullptr);
	const Sci_Position endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsVBComment);
	char chNext = styler[startPos];
	for (Sci_Position i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			int lev = indentCurrent;
			const int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsVBComment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					const int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsVBComment);
					if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK)) {
						lev |= SC_FOLDLEVELHEADERFLAG;
					}
				}
			}
			indentCurrent = indentNext;
			styler.SetLevel(lineCurrent, lev);
			lineCurrent++;
		}
	}
}

}

extern const LexerModule lmVB(SCLEX_VB, LexerVB::LexerFactoryVB, "vb", vbWordListDesc);
extern const LexerModule lmVBScript(SCLEX_VBSCRIPT, LexerVB::LexerFactoryVBScript, "vbscript", vbWordListDesc);
