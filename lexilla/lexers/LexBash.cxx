// Scintilla source code edit control
/** @file LexBash.cxx
 ** Lexer for Bash.
 **/
// Copyright 2004-2012 by Neil Hodgson <neilh@scintilla.org>
// Adapted from LexPerl by Kein-Hong Man 2004
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <initializer_list>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "InList.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SubStyles.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

#define HERE_DELIM_MAX			256

// define this if you want 'invalid octals' to be marked as errors
// usually, this is not a good idea, permissive lexing is better
#undef PEDANTIC_OCTAL

#define BASH_BASE_ERROR			65
#define BASH_BASE_DECIMAL		66
#define BASH_BASE_HEX			67
#ifdef PEDANTIC_OCTAL
#define BASH_BASE_OCTAL			68
#define	BASH_BASE_OCTAL_ERROR	69
#endif

// state constants for parts of a bash command segment
enum class CmdState {
	Body,
	Start,
	Word,
	Test,			// test
	SingleBracket,	// []
	DoubleBracket,	// [[]]
	Arithmetic,
	Delimiter,
};

enum class CommandSubstitution : int {
	Backtick,
	Inside,
	InsideTrack,
};

// state constants for nested delimiter pairs, used by
// SCE_SH_STRING, SCE_SH_PARAM and SCE_SH_BACKTICKS processing
enum class QuoteStyle {
	Literal,		// ''
	CString,		// $''
	String,			// ""
	LString,		// $""
	HereDoc,		// here document
	Backtick,		// ``
	Parameter,		// ${}
	Command,		// $()
	CommandInside,	// $() with styling inside
	Arithmetic,		// $(()), $[]
};

#define BASH_QUOTE_STACK_MAX	7
#define BASH_SPECIAL_PARAMETER	"*@#?-$!"

constexpr int commandSubstitutionFlag = 0x40;
constexpr int MaskCommand(int state) noexcept {
	return state & ~commandSubstitutionFlag;
}

constexpr int translateBashDigit(int ch) noexcept {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'z') {
		return ch - 'a' + 10;
	} else if (ch >= 'A' && ch <= 'Z') {
		return ch - 'A' + 36;
	} else if (ch == '@') {
		return 62;
	} else if (ch == '_') {
		return 63;
	}
	return BASH_BASE_ERROR;
}

int getBashNumberBase(const char *s) noexcept {
	int i = 0;
	int base = 0;
	while (*s) {
		base = base * 10 + (*s++ - '0');
		i++;
	}
	if (base > 64 || i > 2) {
		return BASH_BASE_ERROR;
	}
	return base;
}

constexpr int opposite(int ch) noexcept {
	if (ch == '(') return ')';
	if (ch == '[') return ']';
	if (ch == '{') return '}';
	if (ch == '<') return '>';
	return ch;
}

int GlobScan(StyleContext &sc) {
	// forward scan for zsh globs, disambiguate versus bash arrays
	// complex expressions may still fail, e.g. unbalanced () '' "" etc
	int c = 0;
	int sLen = 0;
	int pCount = 0;
	int hash = 0;
	while ((c = sc.GetRelativeCharacter(++sLen)) != 0) {
		if (IsASpace(c)) {
			return 0;
		} else if (c == '\'' || c == '\"') {
			if (hash != 2) return 0;
		} else if (c == '#' && hash == 0) {
			hash = (sLen == 1) ? 2:1;
		} else if (c == '(') {
			pCount++;
		} else if (c == ')') {
			if (pCount == 0) {
				if (hash) return sLen;
				return 0;
			}
			pCount--;
		}
	}
	return 0;
}

bool IsCommentLine(Sci_Position line, LexAccessor &styler) {
	const Sci_Position pos = styler.LineStart(line);
	const Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		const char ch = styler[i];
		if (ch == '#')
			return true;
		if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

constexpr bool StyleForceBacktrack(int state) noexcept {
	return AnyOf(state, SCE_SH_CHARACTER, SCE_SH_STRING, SCE_SH_BACKTICKS, SCE_SH_HERE_Q, SCE_SH_PARAM);
}

struct OptionsBash {
	bool fold = false;
	bool foldComment = false;
	bool foldCompact = true;
	bool stylingInsideString = false;
	bool stylingInsideBackticks = false;
	bool stylingInsideParameter = false;
	bool stylingInsideHeredoc = false;
	bool nestedBackticks = true;
	CommandSubstitution commandSubstitution = CommandSubstitution::Backtick;
	std::string specialParameter = BASH_SPECIAL_PARAMETER;

	[[nodiscard]] bool stylingInside(int state) const noexcept {
		switch (state) {
		case SCE_SH_STRING:
			return stylingInsideString;
		case SCE_SH_BACKTICKS:
			return stylingInsideBackticks;
		case SCE_SH_PARAM:
			return stylingInsideParameter;
		case SCE_SH_HERE_Q:
			return stylingInsideHeredoc;
		default:
			return false;
		}
	}
};

const char * const bashWordListDesc[] = {
	"Keywords",
	nullptr
};

struct OptionSetBash : public OptionSet<OptionsBash> {
	OptionSetBash() {
		DefineProperty("fold", &OptionsBash::fold);

		DefineProperty("fold.comment", &OptionsBash::foldComment);

		DefineProperty("fold.compact", &OptionsBash::foldCompact);

		DefineProperty("lexer.bash.styling.inside.string", &OptionsBash::stylingInsideString,
			"Set this property to 1 to highlight shell expansions inside string.");

		DefineProperty("lexer.bash.styling.inside.backticks", &OptionsBash::stylingInsideBackticks,
			"Set this property to 1 to highlight shell expansions inside backticks.");

		DefineProperty("lexer.bash.styling.inside.parameter", &OptionsBash::stylingInsideParameter,
			"Set this property to 1 to highlight shell expansions inside ${} parameter expansion.");

		DefineProperty("lexer.bash.styling.inside.heredoc", &OptionsBash::stylingInsideHeredoc,
			"Set this property to 1 to highlight shell expansions inside here document.");

		DefineProperty("lexer.bash.command.substitution", &OptionsBash::commandSubstitution,
			"Set how to highlight $() command substitution. "
			"0 (the default) highlighted as backticks. "
			"1 highlighted inside. "
			"2 highlighted inside with extra scope tracking.");

		DefineProperty("lexer.bash.nested.backticks", &OptionsBash::nestedBackticks,
			"Set this property to 0 to disable nested backquoted command substitution.");

		DefineProperty("lexer.bash.special.parameter", &OptionsBash::specialParameter,
			"Set shell (default is Bash) special parameters.");

		DefineWordListSets(bashWordListDesc);
	}
};

class QuoteCls {	// Class to manage quote pairs (simplified vs LexPerl)
public:
	int Count = 0;
	int Up = '\0';
	int Down = '\0';
	QuoteStyle Style = QuoteStyle::Literal;
	int Outer = SCE_SH_DEFAULT;
	CmdState State = CmdState::Body;
	void Clear() noexcept {
		Count = 0;
		Up	  = '\0';
		Down  = '\0';
		Style = QuoteStyle::Literal;
		Outer = SCE_SH_DEFAULT;
		State = CmdState::Body;
	}
	void Start(int u, QuoteStyle s, int outer, CmdState state) noexcept {
		Count = 1;
		Up    = u;
		Down  = opposite(Up);
		Style = s;
		Outer = outer;
		State = state;
	}
};

class QuoteStackCls {	// Class to manage quote pairs that nest
public:
	int Depth = 0;
	int State = SCE_SH_DEFAULT;
	bool lineContinuation = false;
	bool nestedBackticks = false;
	CommandSubstitution commandSubstitution = CommandSubstitution::Backtick;
	int insideCommand = 0;
	unsigned backtickLevel = 0;
	QuoteCls Current;
	QuoteCls Stack[BASH_QUOTE_STACK_MAX];
	const CharacterSet &setParamStart;
	QuoteStackCls(const CharacterSet &setParamStart_) noexcept : setParamStart{setParamStart_} {}
	[[nodiscard]] bool Empty() const noexcept {
		return Current.Up == '\0';
	}
	void Start(int u, QuoteStyle s, int outer, CmdState state) noexcept {
		if (Empty()) {
			Current.Start(u, s, outer, state);
			if (s == QuoteStyle::Backtick) {
				++backtickLevel;
			}
		} else {
			Push(u, s, outer, state);
		}
	}
	void Push(int u, QuoteStyle s, int outer, CmdState state) noexcept {
		if (Depth >= BASH_QUOTE_STACK_MAX) {
			return;
		}
		Stack[Depth] = Current;
		Depth++;
		Current.Start(u, s, outer, state);
		if (s == QuoteStyle::Backtick) {
			++backtickLevel;
		}
	}
	void Pop() noexcept {
		if (Depth == 0) {
			Clear();
			return;
		}
		if (backtickLevel != 0 && Current.Style == QuoteStyle::Backtick) {
			--backtickLevel;
		}
		if (insideCommand != 0 && Current.Style == QuoteStyle::CommandInside) {
			insideCommand = 0;
			for (int i = 0; i < Depth; i++) {
				if (Stack[i].Style == QuoteStyle::CommandInside) {
					insideCommand = commandSubstitutionFlag;
					break;
				}
			}
		}
		Depth--;
		Current = Stack[Depth];
	}
	void Clear() noexcept {
		Depth = 0;
		State = SCE_SH_DEFAULT;
		insideCommand = 0;
		backtickLevel = 0;
		Current.Clear();
	}
	bool CountDown(StyleContext &sc, CmdState &cmdState) {
		Current.Count--;
		while (Current.Count > 0 && sc.chNext == Current.Down) {
			Current.Count--;
			sc.Forward();
		}
		if (Current.Count == 0) {
			cmdState = Current.State;
			const int outer = Current.Outer;
			Pop();
			sc.ForwardSetState(outer | insideCommand);
			return true;
		}
		return false;
	}
	void Expand(StyleContext &sc, CmdState &cmdState, bool stylingInside) {
		const CmdState current = cmdState;
		const int state = sc.state;
		QuoteStyle style = QuoteStyle::Literal;
		State = state;
		sc.SetState(SCE_SH_SCALAR);
		sc.Forward();
		if (sc.ch == '{') {
			style = QuoteStyle::Parameter;
			sc.ChangeState(SCE_SH_PARAM);
		} else if (sc.ch == '\'') {
			style = QuoteStyle::CString;
			sc.ChangeState(SCE_SH_STRING);
		} else if (sc.ch == '"') {
			style = QuoteStyle::LString;
			sc.ChangeState(SCE_SH_STRING);
		} else if (sc.ch == '(' || sc.ch == '[') {
			if (sc.ch == '[' || sc.chNext == '(') {
				style = QuoteStyle::Arithmetic;
				cmdState = CmdState::Arithmetic;
				sc.ChangeState(SCE_SH_OPERATOR);
			} else {
				if (stylingInside && commandSubstitution >= CommandSubstitution::Inside) {
					style = QuoteStyle::CommandInside;
					cmdState = CmdState::Delimiter;
					sc.ChangeState(SCE_SH_OPERATOR);
					if (commandSubstitution == CommandSubstitution::InsideTrack) {
						insideCommand = commandSubstitutionFlag;
					}
				} else {
					style = QuoteStyle::Command;
					sc.ChangeState(SCE_SH_BACKTICKS);
				}
			}
		} else {
			// scalar has no delimiter pair
			if (!setParamStart.Contains(sc.ch)) {
				stylingInside = false; // not scalar
			}
		}
		if (!stylingInside) {
			sc.ChangeState(state);
		} else {
			sc.ChangeState(sc.state | insideCommand);
		}
		if (style != QuoteStyle::Literal) {
			Start(sc.ch, style, state, current);
			sc.Forward();
		}
	}
	void Escape(StyleContext &sc) {
		unsigned count = 1;
		while (sc.chNext == '\\') {
			++count;
			sc.Forward();
		}
		bool escaped = count & 1U; // odd backslash escape next character
		if (escaped && (sc.chNext == '\r' || sc.chNext == '\n')) {
			lineContinuation = true;
			if (sc.state == SCE_SH_IDENTIFIER) {
				sc.SetState(SCE_SH_OPERATOR | insideCommand);
			}
			return;
		}
		if (backtickLevel > 0 && nestedBackticks) {
			/*
			for $k$ level substitution with $N$ backslashes:
			* when $N/2^k$ is odd, following dollar is escaped.
			* when $(N - 1)/2^k$ is even, following quote is escaped.
			* when $N = n\times 2^{k + 1} - 1$, following backtick is escaped.
			* when $N = n\times 2^{k + 1} + 2^k - 1$, following backtick starts inner substitution.
			* when $N = m\times 2^k + 2^{k - 1} - 1$ and $k > 1$, following backtick ends current substitution.
			*/
			if (sc.chNext == '$') {
				escaped = (count >> backtickLevel) & 1U;
			} else if (sc.chNext == '\"' || sc.chNext == '\'') {
				escaped = (((count - 1) >> backtickLevel) & 1U) == 0;
			} else if (sc.chNext == '`' && escaped) {
				unsigned mask = 1U << (backtickLevel + 1);
				count += 1;
				escaped = (count & (mask - 1)) == 0;
				if (!escaped) {
					unsigned remain = count - (mask >> 1U);
					if (static_cast<int>(remain) >= 0 && (remain & (mask - 1)) == 0) {
						escaped = true;
						++backtickLevel;
					} else if (backtickLevel > 1) {
						mask >>= 1U;
						remain = count - (mask >> 1U);
						if (static_cast<int>(remain) >= 0 && (remain & (mask - 1)) == 0) {
							escaped = true;
							--backtickLevel;
						}
					}
				}
			}
		}
		if (escaped) {
			sc.Forward();
		}
	}
};

const char styleSubable[] = { SCE_SH_IDENTIFIER, SCE_SH_SCALAR, 0 };

const LexicalClass lexicalClasses[] = {
	// Lexer Bash SCLEX_BASH SCE_SH_:
	0, "SCE_SH_DEFAULT", "default", "White space",
	1, "SCE_SH_ERROR", "error", "Error",
	2, "SCE_SH_COMMENTLINE", "comment line", "Line comment: #",
	3, "SCE_SH_NUMBER", "literal numeric", "Number",
	4, "SCE_SH_WORD", "keyword", "Keyword",
	5, "SCE_SH_STRING", "literal string", "String",
	6, "SCE_SH_CHARACTER", "literal string", "Single quoted string",
	7, "SCE_SH_OPERATOR", "operator", "Operators",
	8, "SCE_SH_IDENTIFIER", "identifier", "Identifiers",
	9, "SCE_SH_SCALAR", "identifier", "Scalar variable",
	10, "SCE_SH_PARAM", "identifier", "Parameter",
	11, "SCE_SH_BACKTICKS", "literal string", "Backtick quoted command",
	12, "SCE_SH_HERE_DELIM", "operator", "Heredoc delimiter",
	13, "SCE_SH_HERE_Q", "here-doc literal string", "Heredoc quoted string",
};

}

class LexerBash final : public DefaultLexer {
	WordList keywords;
	WordList cmdDelimiter;
	WordList bashStruct;
	WordList bashStruct_in;
	WordList testOperator;
	OptionsBash options;
	OptionSetBash osBash;
	CharacterSet setParamStart;
	enum { ssIdentifier, ssScalar };
	SubStyles subStyles{styleSubable};
public:
	LexerBash() :
		DefaultLexer("bash", SCLEX_BASH, lexicalClasses, std::size(lexicalClasses)),
		setParamStart(CharacterSet::setAlphaNum, "_" BASH_SPECIAL_PARAMETER) {
		cmdDelimiter.Set("| || |& & && ; ;; ( ) { }");
		bashStruct.Set("if elif fi while until else then do done esac eval");
		bashStruct_in.Set("for case select");
		testOperator.Set("eq ge gt le lt ne ef nt ot");
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osBash.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char* name) override {
		return osBash.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osBash.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char* key) override {
		return osBash.PropertyGet(key);
	}
	const char * SCI_METHOD DescribeWordListSets() override {
		return osBash.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos_, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}

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

	bool IsTestOperator(const char *s, const CharacterSet &setSingleCharOp) const noexcept {
		return (s[2] == '\0' && setSingleCharOp.Contains(s[1]))
			|| testOperator.InList(s + 1);
	}

	static ILexer5 *LexerFactoryBash() {
		return new LexerBash();
	}
};

Sci_Position SCI_METHOD LexerBash::PropertySet(const char *key, const char *val) {
	if (osBash.PropertySet(&options, key, val)) {
		if (strcmp(key, "lexer.bash.special.parameter") == 0) {
			setParamStart = CharacterSet(CharacterSet::setAlphaNum, "_");
			setParamStart.AddString(options.specialParameter.empty() ? BASH_SPECIAL_PARAMETER : options.specialParameter.c_str());
		}
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerBash::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
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

void SCI_METHOD LexerBash::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	const CharacterSet setWordStart(CharacterSet::setAlpha, "_");
	// note that [+-] are often parts of identifiers in shell scripts
	const CharacterSet setWord(CharacterSet::setAlphaNum, "._+-");
	CharacterSet setMetaCharacter(CharacterSet::setNone, "|&;()<> \t\r\n");
	setMetaCharacter.Add(0);
	const CharacterSet setBashOperator(CharacterSet::setNone, "^&%()-+=|{}[]:;>,*/<?!.~@");
	const CharacterSet setSingleCharOp(CharacterSet::setNone, "rwxoRWXOezsfdlpSbctugkTBMACahGLNn");
	const CharacterSet setParam(CharacterSet::setAlphaNum, "_");
	const CharacterSet setHereDoc(CharacterSet::setAlpha, "_\\-+!%*,./:?@[]^`{}~");
	const CharacterSet setHereDoc2(CharacterSet::setAlphaNum, "_-+!%*,./:=?@[]^`{}~");
	const CharacterSet setLeftShift(CharacterSet::setDigits, "$");

	class HereDocCls {	// Class to manage HERE document elements
	public:
		int State = 0;			// 0: '<<' encountered
		// 1: collect the delimiter
		// 2: here doc text (lines after the delimiter)
		int Quote = '\0';		// the char after '<<'
		bool Quoted = false;		// true if Quote in ('\'','"','`')
		bool Escaped = false;		// backslash in delimiter, common in configure script
		bool Indent = false;		// indented delimiter (for <<-)
		int BackslashCount = 0;
		int DelimiterLength = 0;	// strlen(Delimiter)
		char Delimiter[HERE_DELIM_MAX]{};	// the Delimiter
		HereDocCls() noexcept = default;
		void Append(int ch) {
			Delimiter[DelimiterLength++] = static_cast<char>(ch);
			Delimiter[DelimiterLength] = '\0';
		}
	};
	HereDocCls HereDoc;

	QuoteStackCls QuoteStack(setParamStart);
	QuoteStack.nestedBackticks = options.nestedBackticks;
	QuoteStack.commandSubstitution = options.commandSubstitution;

	const WordClassifier &classifierIdentifiers = subStyles.Classifier(SCE_SH_IDENTIFIER);
	const WordClassifier &classifierScalars = subStyles.Classifier(SCE_SH_SCALAR);

	int numBase = 0;
	int digit = 0;
	const Sci_PositionU endPos = startPos + length;
	CmdState cmdState = CmdState::Start;
	LexAccessor styler(pAccess);

	// Always backtracks to the start of a line that is not a continuation
	// of the previous line (i.e. start of a bash command segment)
	Sci_Position ln = styler.GetLine(startPos);
	if (ln > 0 && startPos == static_cast<Sci_PositionU>(styler.LineStart(ln)))
		ln--;
	for (;;) {
		startPos = styler.LineStart(ln);
		if (ln == 0 || styler.GetLineState(ln) == static_cast<int>(CmdState::Start))
			break;
		ln--;
	}
	initStyle = SCE_SH_DEFAULT;

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	while (sc.More()) {

		// handle line continuation, updates per-line stored state
		if (sc.atLineStart) {
			CmdState state = CmdState::Body;	// force backtrack while retaining cmdState
			if (!StyleForceBacktrack(MaskCommand(sc.state))) {
				// retain last line's state
				// arithmetic expression and double bracket test can span multiline without line continuation
				if (!QuoteStack.lineContinuation && !AnyOf(cmdState, CmdState::DoubleBracket, CmdState::Arithmetic)) {
					cmdState = CmdState::Start;
				}
				if (QuoteStack.Empty()) {	// force backtrack when nesting
					state = cmdState;
				}
			}
			QuoteStack.lineContinuation = false;
			styler.SetLineState(sc.currentLine, static_cast<int>(state));
		}

		// controls change of cmdState at the end of a non-whitespace element
		// states Body|Test|Arithmetic persist until the end of a command segment
		// state Word persist, but ends with 'in' or 'do' construct keywords
		CmdState cmdStateNew = CmdState::Body;
		if (cmdState >= CmdState::Word && cmdState <= CmdState::Arithmetic)
			cmdStateNew = cmdState;
		const int stylePrev = MaskCommand(sc.state);
		const int insideCommand = QuoteStack.insideCommand;

		// Determine if the current state should terminate.
		switch (MaskCommand(sc.state)) {
			case SCE_SH_OPERATOR:
				sc.SetState(SCE_SH_DEFAULT | insideCommand);
				if (cmdState == CmdState::Delimiter)		// if command delimiter, start new command
					cmdStateNew = CmdState::Start;
				else if (sc.chPrev == '\\')			// propagate command state if line continued
					cmdStateNew = cmdState;
				break;
			case SCE_SH_WORD:
				// "." never used in Bash variable names but used in file names
				if (!setWord.Contains(sc.ch) || sc.Match('+', '=') || sc.Match('.', '.')) {
					char s[500];
					sc.GetCurrent(s, sizeof(s));
					int identifierStyle = SCE_SH_IDENTIFIER | insideCommand;
					const int subStyle = classifierIdentifiers.ValueFor(s);
					if (subStyle >= 0) {
						identifierStyle = subStyle | insideCommand;
					}
					// allow keywords ending in a whitespace, meta character or command delimiter
					char s2[10]{};
					s2[0] = static_cast<char>(sc.ch);
					s2[1] = '\0';
					const bool keywordEnds = IsASpace(sc.ch) || setMetaCharacter.Contains(sc.ch) || cmdDelimiter.InList(s2);
					// 'in' or 'do' may be construct keywords
					if (cmdState == CmdState::Word) {
						if (strcmp(s, "in") == 0 && keywordEnds)
							cmdStateNew = CmdState::Body;
						else if (strcmp(s, "do") == 0 && keywordEnds)
							cmdStateNew = CmdState::Start;
						else
							sc.ChangeState(identifierStyle);
						sc.SetState(SCE_SH_DEFAULT | insideCommand);
						break;
					}
					// a 'test' keyword starts a test expression
					if (strcmp(s, "test") == 0) {
						if (cmdState == CmdState::Start && keywordEnds) {
							cmdStateNew = CmdState::Test;
						} else
							sc.ChangeState(identifierStyle);
					}
					// detect bash construct keywords
					else if (bashStruct.InList(s)) {
						if (cmdState == CmdState::Start && keywordEnds)
							cmdStateNew = CmdState::Start;
						else
							sc.ChangeState(identifierStyle);
					}
					// 'for'|'case'|'select' needs 'in'|'do' to be highlighted later
					else if (bashStruct_in.InList(s)) {
						if (cmdState == CmdState::Start && keywordEnds)
							cmdStateNew = CmdState::Word;
						else
							sc.ChangeState(identifierStyle);
					}
					// disambiguate option items and file test operators
					else if (s[0] == '-') {
						if (!AnyOf(cmdState, CmdState::Test, CmdState::SingleBracket, CmdState::DoubleBracket)
							  || !keywordEnds || !IsTestOperator(s, setSingleCharOp))
							sc.ChangeState(identifierStyle);
					}
					// disambiguate keywords and identifiers
					else if (cmdState != CmdState::Start
						  || !(keywords.InList(s) && keywordEnds)) {
						sc.ChangeState(identifierStyle);
					}
					sc.SetState(SCE_SH_DEFAULT | insideCommand);
				}
				break;
			case SCE_SH_IDENTIFIER:
				if (!setWord.Contains(sc.ch) ||
					  (cmdState == CmdState::Arithmetic && !setWordStart.Contains(sc.ch))) {
					char s[500];
					sc.GetCurrent(s, sizeof(s));
					const int subStyle = classifierIdentifiers.ValueFor(s);
					if (subStyle >= 0) {
						sc.ChangeState(subStyle | insideCommand);
					}
					sc.SetState(SCE_SH_DEFAULT | insideCommand);
				}
				break;
			case SCE_SH_NUMBER:
				digit = translateBashDigit(sc.ch);
				if (numBase == BASH_BASE_DECIMAL) {
					if (sc.ch == '#') {
						char s[10];
						sc.GetCurrent(s, sizeof(s));
						numBase = getBashNumberBase(s);
						if (numBase != BASH_BASE_ERROR)
							break;
					} else if (IsADigit(sc.ch))
						break;
				} else if (numBase == BASH_BASE_HEX) {
					if (IsADigit(sc.ch, 16))
						break;
#ifdef PEDANTIC_OCTAL
				} else if (numBase == BASH_BASE_OCTAL ||
						   numBase == BASH_BASE_OCTAL_ERROR) {
					if (digit <= 7)
						break;
					if (digit <= 9) {
						numBase = BASH_BASE_OCTAL_ERROR;
						break;
					}
#endif
				} else if (numBase == BASH_BASE_ERROR) {
					if (digit <= 9)
						break;
				} else {	// DD#DDDD number style handling
					if (digit != BASH_BASE_ERROR) {
						if (numBase <= 36) {
							// case-insensitive if base<=36
							if (digit >= 36) digit -= 26;
						}
						if (digit < numBase)
							break;
						if (digit <= 9) {
							numBase = BASH_BASE_ERROR;
							break;
						}
					}
				}
				// fallthrough when number is at an end or error
				if (numBase == BASH_BASE_ERROR
#ifdef PEDANTIC_OCTAL
					|| numBase == BASH_BASE_OCTAL_ERROR
#endif
				) {
					sc.ChangeState(SCE_SH_ERROR | insideCommand);
				} else if (digit < 62 || digit == 63 || (cmdState != CmdState::Arithmetic &&
					(sc.ch == '-' || (sc.ch == '.' && sc.chNext != '.')))) {
					// current character is alpha numeric, underscore, hyphen or dot
					sc.ChangeState(SCE_SH_IDENTIFIER | insideCommand);
					break;
				}
				sc.SetState(SCE_SH_DEFAULT | insideCommand);
				break;
			case SCE_SH_COMMENTLINE:
				if (sc.MatchLineEnd()) {
					sc.SetState(SCE_SH_DEFAULT | insideCommand);
				}
				break;
			case SCE_SH_HERE_DELIM:
				// From Bash info:
				// ---------------
				// Specifier format is: <<[-]WORD
				// Optional '-' is for removal of leading tabs from here-doc.
				// Whitespace acceptable after <<[-] operator
				//
				if (HereDoc.State == 0) { // '<<' encountered
					HereDoc.Quote = sc.chNext;
					HereDoc.Quoted = false;
					HereDoc.Escaped = false;
					HereDoc.BackslashCount = 0;
					HereDoc.DelimiterLength = 0;
					HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
					if (sc.chNext == '\'' || sc.chNext == '\"') {	// a quoted here-doc delimiter (' or ")
						sc.Forward();
						HereDoc.Quoted = true;
						HereDoc.State = 1;
					} else if (setHereDoc.Contains(sc.chNext) ||
					           (sc.chNext == '=' && cmdState != CmdState::Arithmetic)) {
						// an unquoted here-doc delimiter, no special handling
						HereDoc.State = 1;
					} else if (sc.chNext == '<') {	// HERE string <<<
						sc.Forward();
						sc.ForwardSetState(SCE_SH_DEFAULT | insideCommand);
					} else if (IsASpace(sc.chNext)) {
						// eat whitespace
					} else if (setLeftShift.Contains(sc.chNext) ||
					           (sc.chNext == '=' && cmdState == CmdState::Arithmetic)) {
						// left shift <<$var or <<= cases
						sc.ChangeState(SCE_SH_OPERATOR | insideCommand);
						sc.ForwardSetState(SCE_SH_DEFAULT | insideCommand);
					} else {
						// symbols terminates; deprecated zero-length delimiter
						HereDoc.State = 1;
					}
				} else if (HereDoc.State == 1) { // collect the delimiter
					// * if single quoted, there's no escape
					// * if double quoted, there are \\ and \" escapes
					if (HereDoc.Quoted && sc.ch == HereDoc.Quote && (HereDoc.BackslashCount & 1) == 0) { // closing quote => end of delimiter
						sc.ForwardSetState(SCE_SH_DEFAULT | insideCommand);
					} else if (sc.ch == '\\' && HereDoc.Quote != '\'') {
						HereDoc.Escaped = true;
						HereDoc.BackslashCount += 1;
						if ((HereDoc.BackslashCount & 1) == 0 || (HereDoc.Quoted && !AnyOf(sc.chNext, '\"', '\\'))) {
							// in quoted prefixes only \ and the quote eat the escape
							HereDoc.Append(sc.ch);
						} else {
							// skip escape prefix
						}
					} else if (HereDoc.Quoted || setHereDoc2.Contains(sc.ch) || (sc.ch > 32 && sc.ch < 127 && (HereDoc.BackslashCount & 1) != 0)) {
						HereDoc.BackslashCount = 0;
						HereDoc.Append(sc.ch);
					} else {
						sc.SetState(SCE_SH_DEFAULT | insideCommand);
					}
					if (HereDoc.DelimiterLength >= HERE_DELIM_MAX - 1) {	// force blowup
						sc.SetState(SCE_SH_ERROR | insideCommand);
						HereDoc.State = 0;
					}
				}
				break;
			case SCE_SH_SCALAR:	// variable names
				if (!setParam.Contains(sc.ch)) {
					char s[500];
					sc.GetCurrent(s, sizeof(s));
					const int subStyle = classifierScalars.ValueFor(&s[1]); // skip the $
					if (subStyle >= 0) {
						sc.ChangeState(subStyle | insideCommand);
					}
					if (sc.LengthCurrent() == 1) {
						// Special variable
						sc.Forward();
					}
					sc.SetState(QuoteStack.State | insideCommand);
					continue;
				}
				break;
			case SCE_SH_HERE_Q:
				// HereDoc.State == 2
				if (sc.atLineStart && QuoteStack.Current.Style == QuoteStyle::HereDoc) {
					sc.SetState(SCE_SH_HERE_Q | insideCommand);
					if (HereDoc.Indent) { // tabulation prefix
						while (sc.ch == '\t') {
							sc.Forward();
						}
					}
					if ((static_cast<Sci_Position>(sc.currentPos + HereDoc.DelimiterLength) == sc.lineEnd) &&
						(HereDoc.DelimiterLength == 0 || sc.Match(HereDoc.Delimiter))) {
						if (HereDoc.DelimiterLength != 0) {
							sc.SetState(SCE_SH_HERE_DELIM | insideCommand);
							while (!sc.MatchLineEnd()) {
								sc.Forward();
							}
						}
						QuoteStack.Pop();
						sc.SetState(SCE_SH_DEFAULT | QuoteStack.insideCommand);
						break;
					}
				}
				if (HereDoc.Quoted || HereDoc.Escaped) {
					break;
				}
				// fall through to handle nested shell expansions
				[[fallthrough]];
			case SCE_SH_STRING:	// delimited styles, can nest
			case SCE_SH_PARAM: // ${parameter}
			case SCE_SH_BACKTICKS:
				if (sc.ch == '\\') {
					if (QuoteStack.Current.Style != QuoteStyle::Literal)
						QuoteStack.Escape(sc);
				} else if (sc.ch == QuoteStack.Current.Down) {
					if (QuoteStack.CountDown(sc, cmdState)) {
						continue;
					}
				} else if (sc.ch == QuoteStack.Current.Up) {
					if (QuoteStack.Current.Style != QuoteStyle::Parameter) {
						QuoteStack.Current.Count++;
					}
				} else {
					if (QuoteStack.Current.Style == QuoteStyle::String ||
						QuoteStack.Current.Style == QuoteStyle::HereDoc ||
						QuoteStack.Current.Style == QuoteStyle::LString
					) {	// do nesting for "string", $"locale-string", heredoc
						const bool stylingInside = options.stylingInside(MaskCommand(sc.state));
						if (sc.ch == '`') {
							QuoteStack.Push(sc.ch, QuoteStyle::Backtick, sc.state, cmdState);
							if (stylingInside) {
								sc.SetState(SCE_SH_BACKTICKS | insideCommand);
							}
						} else if (sc.ch == '$' && !AnyOf(sc.chNext, '\"', '\'')) {
							QuoteStack.Expand(sc, cmdState, stylingInside);
							continue;
						}
					} else if (QuoteStack.Current.Style == QuoteStyle::Command ||
							   QuoteStack.Current.Style == QuoteStyle::Parameter ||
							   QuoteStack.Current.Style == QuoteStyle::Backtick
					) {	// do nesting for $(command), `command`, ${parameter}
						const bool stylingInside = options.stylingInside(MaskCommand(sc.state));
						if (sc.ch == '\'') {
							if (stylingInside) {
								QuoteStack.State = sc.state;
								sc.SetState(SCE_SH_CHARACTER | insideCommand);
							} else {
								QuoteStack.Push(sc.ch, QuoteStyle::Literal, sc.state, cmdState);
							}
						} else if (sc.ch == '\"') {
							QuoteStack.Push(sc.ch, QuoteStyle::String, sc.state, cmdState);
							if (stylingInside) {
								sc.SetState(SCE_SH_STRING | insideCommand);
							}
						} else if (sc.ch == '`') {
							QuoteStack.Push(sc.ch, QuoteStyle::Backtick, sc.state, cmdState);
							if (stylingInside) {
								sc.SetState(SCE_SH_BACKTICKS | insideCommand);
							}
						} else if (sc.ch == '$') {
							QuoteStack.Expand(sc, cmdState, stylingInside);
							continue;
						}
					}
				}
				break;
			case SCE_SH_CHARACTER: // singly-quoted strings
				if (sc.ch == '\'') {
					sc.ForwardSetState(QuoteStack.State | insideCommand);
					continue;
				}
				break;
		}

		// Must check end of HereDoc state 1 before default state is handled
		if (HereDoc.State == 1 && sc.MatchLineEnd()) {
			// Begin of here-doc (the line after the here-doc delimiter):
			// Lexically, the here-doc starts from the next line after the >>, but the
			// first line of here-doc seem to follow the style of the last EOL sequence
			HereDoc.State = 2;
			if (HereDoc.Quoted) {
				if (MaskCommand(sc.state) == SCE_SH_HERE_DELIM) {
					// Missing quote at end of string! Syntax error in bash 4.3
					// Mark this bit as an error, do not colour any here-doc
					sc.ChangeState(SCE_SH_ERROR | insideCommand);
					sc.SetState(SCE_SH_DEFAULT | insideCommand);
				} else {
					// HereDoc.Quote always == '\''
					sc.SetState(SCE_SH_HERE_Q | insideCommand);
					QuoteStack.Start(-1, QuoteStyle::HereDoc, SCE_SH_DEFAULT, cmdState);
				}
			} else if (HereDoc.DelimiterLength == 0) {
				// no delimiter, illegal (but '' and "" are legal)
				sc.ChangeState(SCE_SH_ERROR | insideCommand);
				sc.SetState(SCE_SH_DEFAULT | insideCommand);
			} else {
				sc.SetState(SCE_SH_HERE_Q | insideCommand);
				QuoteStack.Start(-1, QuoteStyle::HereDoc, SCE_SH_DEFAULT, cmdState);
			}
		}

		// update cmdState about the current command segment
		if (stylePrev != SCE_SH_DEFAULT && MaskCommand(sc.state) == SCE_SH_DEFAULT) {
			cmdState = cmdStateNew;
		}
		// Determine if a new state should be entered.
		if (MaskCommand(sc.state) == SCE_SH_DEFAULT) {
			if (sc.ch == '\\') {
				// Bash can escape any non-newline as a literal
				sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
				QuoteStack.Escape(sc);
			} else if (IsADigit(sc.ch)) {
				sc.SetState(SCE_SH_NUMBER | insideCommand);
				numBase = BASH_BASE_DECIMAL;
				if (sc.ch == '0') {	// hex,octal
					if (sc.chNext == 'x' || sc.chNext == 'X') {
						numBase = BASH_BASE_HEX;
						sc.Forward();
					} else if (IsADigit(sc.chNext)) {
#ifdef PEDANTIC_OCTAL
						numBase = BASH_BASE_OCTAL;
#endif
					}
				}
			} else if (setWordStart.Contains(sc.ch)) {
				sc.SetState(((cmdState == CmdState::Arithmetic)? SCE_SH_IDENTIFIER : SCE_SH_WORD) | insideCommand);
			} else if (sc.ch == '#') {
				if (stylePrev != SCE_SH_WORD && stylePrev != SCE_SH_IDENTIFIER &&
					(sc.currentPos == 0 || setMetaCharacter.Contains(sc.chPrev))) {
					sc.SetState(SCE_SH_COMMENTLINE | insideCommand);
				} else {
					sc.SetState(SCE_SH_WORD | insideCommand);
				}
				// handle some zsh features within arithmetic expressions only
				if (cmdState == CmdState::Arithmetic) {
					if (sc.chPrev == '[') {	// [#8] [##8] output digit setting
						sc.SetState(SCE_SH_WORD | insideCommand);
						if (sc.chNext == '#') {
							sc.Forward();
						}
					} else if (sc.Match("##^") && IsUpperCase(sc.GetRelative(3))) {	// ##^A
						sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
						sc.Forward(3);
					} else if (sc.chNext == '#' && !IsASpace(sc.GetRelative(2))) {	// ##a
						sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
						sc.Forward(2);
					} else if (setWordStart.Contains(sc.chNext)) {	// #name
						sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
					}
				}
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_SH_STRING | insideCommand);
				QuoteStack.Start(sc.ch, QuoteStyle::String, SCE_SH_DEFAULT, cmdState);
			} else if (sc.ch == '\'') {
				QuoteStack.State = SCE_SH_DEFAULT;
				sc.SetState(SCE_SH_CHARACTER | insideCommand);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_SH_BACKTICKS | insideCommand);
				QuoteStack.Start(sc.ch, QuoteStyle::Backtick, SCE_SH_DEFAULT, cmdState);
			} else if (sc.ch == '$') {
				QuoteStack.Expand(sc, cmdState, true);
				continue;
			} else if (cmdState != CmdState::Arithmetic && sc.Match('<', '<')) {
				sc.SetState(SCE_SH_HERE_DELIM | insideCommand);
				HereDoc.State = 0;
				if (sc.GetRelative(2) == '-') {	// <<- indent case
					HereDoc.Indent = true;
					sc.Forward();
				} else {
					HereDoc.Indent = false;
				}
			} else if (sc.ch == '-' && // test operator or short and long option
					   cmdState != CmdState::Arithmetic &&
					   sc.chPrev != '~' && !IsADigit(sc.chNext)) {
				if (IsASpace(sc.chPrev) || setMetaCharacter.Contains(sc.chPrev)) {
					sc.SetState(SCE_SH_WORD | insideCommand);
				} else {
					sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
				}
			} else if (setBashOperator.Contains(sc.ch)) {
				bool isCmdDelim = false;
				sc.SetState(SCE_SH_OPERATOR | insideCommand);
				// arithmetic expansion and command substitution
				if (QuoteStack.Current.Style == QuoteStyle::Arithmetic || QuoteStack.Current.Style == QuoteStyle::CommandInside) {
					if (sc.ch == QuoteStack.Current.Down) {
						if (QuoteStack.CountDown(sc, cmdState)) {
							continue;
						}
					} else if (sc.ch == QuoteStack.Current.Up) {
						QuoteStack.Current.Count++;
					}
				}
				// globs have no whitespace, do not appear in arithmetic expressions
				if (cmdState != CmdState::Arithmetic && sc.ch == '(' && sc.chNext != '(') {
					const int i = GlobScan(sc);
					if (i > 1) {
						sc.SetState(SCE_SH_IDENTIFIER | insideCommand);
						sc.Forward(i + 1);
						continue;
					}
				}
				// handle opening delimiters for test/arithmetic expressions - ((,[[,[
				if (cmdState == CmdState::Start
				 || cmdState == CmdState::Body) {
					if (sc.Match('(', '(')) {
						cmdState = CmdState::Arithmetic;
						sc.Forward();
					} else if (sc.Match('[', '[') && IsASpace(sc.GetRelative(2))) {
						cmdState = CmdState::DoubleBracket;
						sc.Forward();
					} else if (sc.ch == '[' && IsASpace(sc.chNext)) {
						cmdState = CmdState::SingleBracket;
					}
				}
				// special state -- for ((x;y;z)) in ... looping
				if (cmdState == CmdState::Word && sc.Match('(', '(')) {
					cmdState = CmdState::Arithmetic;
					sc.Forward(2);
					continue;
				}
				// handle command delimiters in command Start|Body|Word state, also Test if 'test' or '[]'
				if (cmdState < CmdState::DoubleBracket) {
					char s[10]{};
					s[0] = static_cast<char>(sc.ch);
					if (setBashOperator.Contains(sc.chNext)) {
						s[1] = static_cast<char>(sc.chNext);
						s[2] = '\0';
						isCmdDelim = cmdDelimiter.InList(s);
						if (isCmdDelim)
							sc.Forward();
					}
					if (!isCmdDelim) {
						s[1] = '\0';
						isCmdDelim = cmdDelimiter.InList(s);
					}
					if (isCmdDelim) {
						cmdState = CmdState::Delimiter;
						sc.Forward();
						continue;
					}
				}
				// handle closing delimiters for test/arithmetic expressions - )),]],]
				if (cmdState == CmdState::Arithmetic && sc.Match(')', ')')) {
					cmdState = CmdState::Body;
					sc.Forward();
				} else if (sc.ch == ']' && IsASpace(sc.chPrev)) {
					if (cmdState == CmdState::SingleBracket) {
						cmdState = CmdState::Body;
					} else if (cmdState == CmdState::DoubleBracket && sc.chNext == ']') {
						cmdState = CmdState::Body;
						sc.Forward();
					}
				}
			}
		}// sc.state

		sc.Forward();
	}
	sc.Complete();
	if (MaskCommand(sc.state) == SCE_SH_HERE_Q) {
		styler.ChangeLexerState(sc.currentPos, styler.Length());
	}
	sc.Complete();
}

void SCI_METHOD LexerBash::Fold(Sci_PositionU startPos_, Sci_Position length, int initStyle, IDocument *pAccess) {
	if(!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_Position startPos = startPos_;
	const Sci_Position endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	// Backtrack to previous line in case need to fix its fold status
	if (lineCurrent > 0) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
		initStyle = (startPos > 0) ? styler.StyleIndexAt(startPos - 1) : 0;
	}

	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = MaskCommand(styler.StyleIndexAt(startPos));
	int style = MaskCommand(initStyle);
	char word[8] = { '\0' }; // we're not interested in long words anyway
	size_t wordlen = 0;
	for (Sci_Position i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = MaskCommand(styler.StyleIndexAt(i + 1));
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		// Comment folding
		if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
				&& IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
					 && !IsCommentLine(lineCurrent + 1, styler))
				levelCurrent--;
		}

		switch (style) {
		case SCE_SH_WORD:
			if ((wordlen + 1) < sizeof(word))
				word[wordlen++] = ch;
			if (styleNext != style) {
				word[wordlen] = '\0';
				wordlen = 0;
				if (InList(word, {"if", "case", "do"})) {
					levelCurrent++;
				} else if (InList(word, {"fi", "esac", "done"})) {
					levelCurrent--;
				}
			}
			break;

		case SCE_SH_OPERATOR:
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
			break;

		// Here Document folding
		case SCE_SH_HERE_DELIM:
			if (stylePrev == SCE_SH_HERE_Q) {
				levelCurrent--;
			} else if (stylePrev != SCE_SH_HERE_DELIM) {
				if (ch == '<' && chNext == '<') {
					if (styler.SafeGetCharAt(i + 2) != '<') {
						levelCurrent++;
					}
				}
			}
			break;
		case SCE_SH_HERE_Q:
			if (styleNext == SCE_SH_DEFAULT) {
				levelCurrent--;
			}
			break;
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	const int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

extern const LexerModule lmBash(SCLEX_BASH, LexerBash::LexerFactoryBash, "bash", bashWordListDesc);
