// Scintilla source code edit control
/** @file LexAsm.cxx
 ** Lexer for Assembler, just for the MASM syntax
 ** Written by The Black Horus
 ** Enhancements and NASM stuff by Kein-Hong Man, 2003-10
 ** SCE_ASM_COMMENTBLOCK and SCE_ASM_CHARACTER are for future GNU as colouring
 ** Converted to lexer object and added further folding features/properties by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <map>
#include <set>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

bool IsAWordChar(const int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '.' || ch == '_' || ch == '?';
}

bool IsAWordStart(const int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '_' || ch == '.' ||
		ch == '%' || ch == '@' || ch == '$' || ch == '?';
}

bool IsAsmOperator(const int ch) noexcept {
	if (IsAlphaNumeric(ch))
		return false;
	// '.' left out as it is used to make up numbers
	return AnyOf(ch, '*', '/', '-', '+', '(', ')', '=', '^', '[', ']', '<', '&', '>', ',', '|', '~', '%', ':');
}

constexpr bool IsStreamCommentStyle(int style) noexcept {
	return style == SCE_ASM_COMMENTDIRECTIVE || style == SCE_ASM_COMMENTBLOCK;
}

// An individual named option for use in an OptionSet

// Options used for LexerAsm
struct OptionsAsm {
	std::string delimiter;
	bool fold = false;
	bool foldSyntaxBased = true;
	bool foldCommentMultiline = false;
	bool foldCommentExplicit = false;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere = false;
	bool foldCompact = true;
	std::string commentChar;
	[[nodiscard]] char Delimiter() const noexcept {
		return delimiter.empty() ? '~' : delimiter[0];
	}
};

const char *const asmWordListDesc[] = {
	"CPU instructions",
	"FPU instructions",
	"Registers",
	"Directives",
	"Directive operands",
	"Extended instructions",
	"Directives4Foldstart",
	"Directives4Foldend",
	nullptr
};

struct OptionSetAsm : public OptionSet<OptionsAsm> {
	OptionSetAsm() {
		DefineProperty("lexer.asm.comment.delimiter", &OptionsAsm::delimiter,
			"Character used for COMMENT directive's delimiter, replacing the standard \"~\".");

		DefineProperty("fold", &OptionsAsm::fold);

		DefineProperty("fold.asm.syntax.based", &OptionsAsm::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.asm.comment.multiline", &OptionsAsm::foldCommentMultiline,
			"Set this property to 1 to enable folding multi-line comments.");

		DefineProperty("fold.asm.comment.explicit", &OptionsAsm::foldCommentExplicit,
			"This option enables folding explicit fold points when using the Asm lexer. "
			"Explicit fold points allows adding extra folding by placing a ;{ comment at the start and a ;} "
			"at the end of a section that should fold.");

		DefineProperty("fold.asm.explicit.start", &OptionsAsm::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard ;{.");

		DefineProperty("fold.asm.explicit.end", &OptionsAsm::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard ;}.");

		DefineProperty("fold.asm.explicit.anywhere", &OptionsAsm::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsAsm::foldCompact);

		DefineProperty("lexer.as.comment.character", &OptionsAsm::commentChar,
			"Overrides the default comment character (which is ';' for asm and '#' for as).");

		DefineWordListSets(asmWordListDesc);
	}
};

class LexerAsm : public DefaultLexer {
	WordList cpuInstruction;
	WordList mathInstruction;
	WordList registers;
	WordList directive;
	WordList directiveOperand;
	WordList extInstruction;
	WordList directives4foldstart;
	WordList directives4foldend;
	OptionsAsm options;
	OptionSetAsm osAsm;
	char commentChar;
public:
	LexerAsm(const char *languageName_, int language_, char commentChar_) :
		DefaultLexer(languageName_, language_),
		commentChar(commentChar_) {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osAsm.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osAsm.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osAsm.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osAsm.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osAsm.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void *SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}

	static ILexer5 *LexerFactoryAsm() {
		return new LexerAsm("asm", SCLEX_ASM, ';');
	}

	static ILexer5 *LexerFactoryAs() {
		return new LexerAsm("as", SCLEX_AS, '#');
	}
};

Sci_Position SCI_METHOD LexerAsm::PropertySet(const char *key, const char *val) {
	if (osAsm.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerAsm::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &cpuInstruction;
		break;
	case 1:
		wordListN = &mathInstruction;
		break;
	case 2:
		wordListN = &registers;
		break;
	case 3:
		wordListN = &directive;
		break;
	case 4:
		wordListN = &directiveOperand;
		break;
	case 5:
		wordListN = &extInstruction;
		break;
	case 6:
		wordListN = &directives4foldstart;
		break;
	case 7:
		wordListN = &directives4foldend;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl, true)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerAsm::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	const char commentCharacter = options.commentChar.empty() ?
		commentChar : options.commentChar.front();

	// Do not leak onto next line
	if (initStyle == SCE_ASM_STRINGEOL)
		initStyle = SCE_ASM_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			switch (sc.state) {
			case SCE_ASM_STRING:
			case SCE_ASM_CHARACTER:
				// Prevent SCE_ASM_STRINGEOL from leaking back to previous line
				sc.SetState(sc.state);
				break;
			case SCE_ASM_COMMENT:
				sc.SetState(SCE_ASM_DEFAULT);
				break;
			default:
				break;
			}
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_ASM_OPERATOR:
			if (!IsAsmOperator(sc.ch)) {
				sc.SetState(SCE_ASM_DEFAULT);
			}
			break;

		case SCE_ASM_NUMBER:
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_ASM_DEFAULT);
			}
			break;

		case SCE_ASM_IDENTIFIER:
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				bool IsDirective = false;

				if (cpuInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_CPUINSTRUCTION);
				} else if (mathInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_MATHINSTRUCTION);
				} else if (registers.InList(s)) {
					sc.ChangeState(SCE_ASM_REGISTER);
				}  else if (directive.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVE);
					IsDirective = true;
				} else if (directiveOperand.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVEOPERAND);
				} else if (extInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_EXTINSTRUCTION);
				}
				sc.SetState(SCE_ASM_DEFAULT);
				if (IsDirective && !strcmp(s, "comment")) {
					while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd) {
						sc.ForwardSetState(SCE_ASM_DEFAULT);
					}
					if (sc.ch == options.Delimiter()) {
						sc.SetState(SCE_ASM_COMMENTDIRECTIVE);
					}
				}
			}
			break;

		case SCE_ASM_COMMENTDIRECTIVE:
			if (sc.ch == options.Delimiter()) {
				while (!sc.MatchLineEnd()) {
					sc.Forward();
				}
				sc.SetState(SCE_ASM_DEFAULT);
			}
			break;

		case SCE_ASM_STRING:
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
			break;

		case SCE_ASM_CHARACTER:
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
			break;

		default:
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_ASM_DEFAULT) {
			if (sc.ch == commentCharacter) {
				sc.SetState(SCE_ASM_COMMENT);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_ASM_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_ASM_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_ASM_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_ASM_CHARACTER);
			} else if (IsAsmOperator(sc.ch)) {
				sc.SetState(SCE_ASM_OPERATOR);
			}
		}

	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "else".

void SCI_METHOD LexerAsm::Fold(Sci_PositionU startPos_, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	const Sci_Position startPos = static_cast<Sci_Position>(startPos_);
	const Sci_Position endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = FoldLevelStart(styler.LevelAt(lineCurrent-1));
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleIndexAt(startPos);
	int style = initStyle;
	std::string word;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (Sci_Position i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleIndexAt(i + 1);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldCommentMultiline && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldCommentExplicit && ((style == SCE_ASM_COMMENT) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
					levelNext++;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
					levelNext--;
				}
			} else {
				if (ch == ';') {
					if (chNext == '{') {
						levelNext++;
					} else if (chNext == '}') {
						levelNext--;
					}
				}
			}
		}
		if (options.foldSyntaxBased && (style == SCE_ASM_DIRECTIVE)) {
			word.push_back(MakeLowerCase(ch));
			if (styleNext != SCE_ASM_DIRECTIVE) {   // reading directive ready
				if (directives4foldstart.InList(word)) {
					levelNext++;
				} else if (directives4foldend.InList(word)) {
					levelNext--;
				}
				word.clear();
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			const int lev = FoldLevelForCurrentNext(levelCurrent, levelNext) |
				FoldLevelFlags(levelCurrent, levelNext, visibleChars == 0 && options.foldCompact);
			styler.SetLevelIfDifferent(lineCurrent, lev);
			lineCurrent++;
			levelCurrent = levelNext;
			if (atEOL && (i == (styler.Length() - 1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, FoldLevelForCurrent(levelCurrent) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
	}
}

}

extern const LexerModule lmAsm(SCLEX_ASM, LexerAsm::LexerFactoryAsm, "asm", asmWordListDesc);
extern const LexerModule lmAs(SCLEX_AS, LexerAsm::LexerFactoryAs, "as", asmWordListDesc);

