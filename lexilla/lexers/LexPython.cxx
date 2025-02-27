// Scintilla source code edit control
/** @file LexPython.cxx
 ** Lexer for Python.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
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

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SubStyles.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {
// Use an unnamed namespace to protect the functions and classes from name conflicts

/* Notes on f-strings: f-strings are strings prefixed with f (e.g. f'') that may
   have arbitrary expressions in {}.  The tokens in the expressions are lexed as if
   they were outside of any string.  Expressions may contain { and } characters as
   long as there is a closing } for every {, may be 2+ lines in a triple quoted
   string, and may have a formatting specifier following a ! or :, but both !
   and : are valid inside of a bracketed expression and != is a valid
   expression token even outside of a bracketed expression.

   When in an f-string expression, the lexer keeps track of the state value of
   the f-string and the nesting count for the expression (# of [, (, { seen - # of
   }, ), ] seen).  f-strings may be nested (e.g. f'{ a + f"{1+2}"') so a stack of
   states and nesting counts is kept.  If a f-string expression continues beyond
   the end of a line, this stack is saved in a std::map that maps a line number to
   the stack at the end of that line.  std::vector is used for the stack.

   The PEP for f-strings is at https://www.python.org/dev/peps/pep-0498/
*/
struct SingleFStringExpState {
	int state;
	int nestingCount;
};

/* kwCDef, kwCTypeName only used for Cython */
enum kwType { kwOther, kwClass, kwDef, kwImport, kwCDef, kwCTypeName, kwCPDef };

enum literalsAllowed { litNone = 0, litU = 1, litB = 2, litF = 4 };

constexpr int indicatorWhitespace = 1;

bool IsPyComment(Accessor &styler, Sci_Position pos, Sci_Position len) {
	return len > 0 && styler[pos] == '#';
}

constexpr bool IsPyStringTypeChar(int ch, literalsAllowed allowed) noexcept {
	return
		((allowed & litB) && (ch == 'b' || ch == 'B')) ||
		((allowed & litU) && (ch == 'u' || ch == 'U')) ||
		((allowed & litF) && (ch == 'f' || ch == 'F'));
}

constexpr bool IsQuote(int ch) {
	return AnyOf(ch, '"', '\'');
}

constexpr bool IsRawPrefix(int ch) {
	return AnyOf(ch, 'r', 'R');
}

bool IsPyStringStart(int ch, int chNext, int chNext2, literalsAllowed allowed) noexcept {
	// To cover both python2 and python3 lex character prefixes as --
	// ur'' is a string, but ru'' is not
	// fr'', rf'', br'', rb'' are all strings
	if (IsQuote(ch))
		return true;
	if (IsPyStringTypeChar(ch, allowed)) {
		if (IsQuote(chNext))
			return true;
		if (IsRawPrefix(chNext) && IsQuote(chNext2))
			return true;
	}
	if (IsRawPrefix(ch)) {
		if (IsQuote(chNext))
			return true;
		if (IsPyStringTypeChar(chNext, allowed) && !AnyOf(chNext, 'u', 'U') && IsQuote(chNext2))
			return true;
	}

	return false;
}

constexpr bool IsPyFStringState(int st) noexcept {
	return AnyOf(st, SCE_P_FCHARACTER, SCE_P_FSTRING, SCE_P_FTRIPLE, SCE_P_FTRIPLEDOUBLE);
}

constexpr bool IsPySingleQuoteStringState(int st) noexcept {
	return AnyOf(st, SCE_P_CHARACTER, SCE_P_STRING, SCE_P_FCHARACTER, SCE_P_FSTRING);
}

constexpr bool IsPyTripleQuoteStringState(int st) noexcept {
	return AnyOf(st, SCE_P_TRIPLE, SCE_P_TRIPLEDOUBLE, SCE_P_FTRIPLE, SCE_P_FTRIPLEDOUBLE);
}

char GetPyStringQuoteChar(int st) noexcept {
	if (AnyOf(st, SCE_P_CHARACTER, SCE_P_FCHARACTER, SCE_P_TRIPLE, SCE_P_FTRIPLE))
		return '\'';
	if (AnyOf(st, SCE_P_STRING, SCE_P_FSTRING, SCE_P_TRIPLEDOUBLE, SCE_P_FTRIPLEDOUBLE))
		return '"';

	return '\0';
}

void PushStateToStack(int state, std::vector<SingleFStringExpState> &stack, SingleFStringExpState *&currentFStringExp) {
	const SingleFStringExpState single = {state, 0};
	stack.push_back(single);

	currentFStringExp = &stack.back();
}

int PopFromStateStack(std::vector<SingleFStringExpState> &stack, SingleFStringExpState *&currentFStringExp) noexcept {
	int state = 0;

	if (!stack.empty()) {
		state = stack.back().state;
		stack.pop_back();
	}

	if (stack.empty()) {
		currentFStringExp = nullptr;
	} else {
		currentFStringExp = &stack.back();
	}

	return state;
}

/* Return the state to use for the string starting at i; *nextIndex will be set to the first index following the quote(s) */
int GetPyStringState(Accessor &styler, Sci_Position i, Sci_PositionU *nextIndex, literalsAllowed allowed) {
	char ch = styler.SafeGetCharAt(i);
	char chNext = styler.SafeGetCharAt(i + 1);
	bool isFString = false;

	// Advance beyond r, a type char, or both (in either order)
	// Note that this depends on IsPyStringStart to enforce ru'' not being a string
	if (IsRawPrefix(ch)) {
		i++;
		if (IsPyStringTypeChar(chNext, allowed)) {
			if (AnyOf(chNext, 'f', 'F'))
				isFString = true;
			i++;
		}
		ch = styler.SafeGetCharAt(i);
		chNext = styler.SafeGetCharAt(i + 1);
	} else if (IsPyStringTypeChar(ch, allowed)) {
		if (AnyOf(ch, 'f', 'F'))
			isFString = true;
		if (IsRawPrefix(chNext))
			i += 2;
		else
			i += 1;
		ch = styler.SafeGetCharAt(i);
		chNext = styler.SafeGetCharAt(i + 1);
	}

	// ch and i will be the first quote
	if (!IsQuote(ch)) {
		*nextIndex = i + 1;
		return SCE_P_DEFAULT;
	}

	if (ch == chNext && ch == styler.SafeGetCharAt(i + 2)) {
		*nextIndex = i + 3;

		if (ch == '"')
			return (isFString ? SCE_P_FTRIPLEDOUBLE : SCE_P_TRIPLEDOUBLE);
		else
			return (isFString ? SCE_P_FTRIPLE : SCE_P_TRIPLE);
	} else {
		*nextIndex = i + 1;

		if (ch == '"')
			return (isFString ? SCE_P_FSTRING : SCE_P_STRING);
		else
			return (isFString ? SCE_P_FCHARACTER : SCE_P_CHARACTER);
	}
}

bool IsAWordChar(int ch, bool unicodeIdentifiers) noexcept {
	if (IsASCII(ch))
		return (IsAlphaNumeric(ch) || ch == '.' || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	// Python uses the XID_Continue set from Unicode data
	return IsXidContinue(ch);
}

bool IsAWordStart(int ch, bool unicodeIdentifiers) noexcept {
	if (IsASCII(ch))
		return (IsUpperOrLowerCase(ch) || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	// Python uses the XID_Start set from Unicode data
	return IsXidStart(ch);
}

bool IsFirstNonWhitespace(Sci_Position pos, Accessor &styler) {
	const Sci_Position line = styler.GetLine(pos);
	const Sci_Position start_pos = styler.LineStart(line);
	for (Sci_Position i = start_pos; i < pos; i++) {
		const char ch = styler[i];
		if (!IsASpaceOrTab(ch))
			return false;
	}
	return true;
}

unsigned char GetNextNonWhitespaceChar(Accessor &styler, Sci_PositionU pos, Sci_PositionU maxPos, Sci_PositionU *charPosPtr = nullptr) {
	while (pos < maxPos) {
		const unsigned char ch = styler.SafeGetCharAt(pos, '\0');
		if (!AnyOf(ch, ' ', '\t', '\n', '\r')) {
			if (charPosPtr != nullptr) {
				*charPosPtr = pos;
			}
			return ch;
		}
		pos++;
	}

	return '\0';
}

// Returns whether symbol is "match" or "case" and it is an identifier &
// not a keyword. Does not require the line to end with : so "match\n"
// and "match (x)\n" return false because match could be a keyword once
// more text is added
bool IsMatchOrCaseIdentifier(const StyleContext &sc, Accessor &styler, const char *symbol) {
	if (strcmp(symbol, "match") != 0 && strcmp(symbol, "case") != 0) {
		return false;
	}

	if (!IsFirstNonWhitespace(sc.currentPos - strlen(symbol), styler)) {
		return true;
	}

	// The match keyword can be followed by an expression; the case keyword
	// is a bit more restrictive but not much. Here, we look for the next
	// char and return false if the char cannot start an expression; for '.'
	// we look at the following char to see if it's a digit because .1 is a
	// number
	Sci_PositionU nextCharPos = 0;
	const unsigned char nextChar = GetNextNonWhitespaceChar(styler, sc.currentPos, sc.lineEnd, &nextCharPos);
	if (nextChar == '=' || nextChar == '#') {
		return true;
	}
	if (nextChar == '.' && nextCharPos >= sc.currentPos) {
		const unsigned char followingChar = GetNextNonWhitespaceChar(styler, nextCharPos+1, sc.lineEnd);
		if (!IsADigit(followingChar)) {
			return true;
		}
	}

	return false;
}

// Options used for LexerPython
struct OptionsPython {
	int whingeLevel = 0;
	bool base2or8Literals = true;
	bool stringsU = true;
	bool stringsB = true;
	bool stringsF = true;
	bool stringsOverNewline = false;
	bool keywords2NoSubIdentifiers = false;
	bool fold = false;
	bool foldQuotes = false;
	bool foldCompact = false;
	bool unicodeIdentifiers = true;
	int identifierAttributes = 0;
	int decoratorAttributes = 0;
	bool pep701StringsF = true;

	[[nodiscard]] literalsAllowed AllowedLiterals() const noexcept {
		literalsAllowed allowedLiterals = stringsU ? litU : litNone;
		if (stringsB)
			allowedLiterals = static_cast<literalsAllowed>(allowedLiterals | litB);
		if (stringsF)
			allowedLiterals = static_cast<literalsAllowed>(allowedLiterals | litF);
		return allowedLiterals;
	}
};

const char *const pythonWordListDesc[] = {
	"Keywords",
	"Highlighted identifiers",
	nullptr
};

struct OptionSetPython : public OptionSet<OptionsPython> {
	OptionSetPython() {
		DefineProperty("tab.timmy.whinge.level", &OptionsPython::whingeLevel,
			       "For Python code, checks whether indenting is consistent. "
			       "The default, 0 turns off indentation checking, "
			       "1 checks whether each line is potentially inconsistent with the previous line, "
			       "2 checks whether any space characters occur before a tab character in the indentation, "
			       "3 checks whether any spaces are in the indentation, and "
			       "4 checks for any tab characters in the indentation. "
			       "1 is a good level to use.");

		DefineProperty("lexer.python.literals.binary", &OptionsPython::base2or8Literals,
			       "Set to 0 to not recognise Python 3 binary and octal literals: 0b1011 0o712.");

		DefineProperty("lexer.python.strings.u", &OptionsPython::stringsU,
			       "Set to 0 to not recognise Python Unicode literals u\"x\" as used before Python 3.");

		DefineProperty("lexer.python.strings.b", &OptionsPython::stringsB,
			       "Set to 0 to not recognise Python 3 bytes literals b\"x\".");

		DefineProperty("lexer.python.strings.f", &OptionsPython::stringsF,
			       "Set to 0 to not recognise Python 3.6 f-string literals f\"var={var}\".");

		DefineProperty("lexer.python.strings.f.pep.701", &OptionsPython::pep701StringsF,
			       "Set to 0 to use pre-PEP 701 / Python 3.12 f-string lexing.");

		DefineProperty("lexer.python.strings.over.newline", &OptionsPython::stringsOverNewline,
			       "Set to 1 to allow strings to span newline characters.");

		DefineProperty("lexer.python.keywords2.no.sub.identifiers", &OptionsPython::keywords2NoSubIdentifiers,
			       "When enabled, it will not style keywords2 items that are used as a sub-identifier. "
			       "Example: when set, will not highlight \"foo.open\" when \"open\" is a keywords2 item.");

		DefineProperty("fold", &OptionsPython::fold);

		DefineProperty("fold.quotes.python", &OptionsPython::foldQuotes,
			       "This option enables folding multi-line quoted strings when using the Python lexer.");

		DefineProperty("fold.compact", &OptionsPython::foldCompact);

		DefineProperty("lexer.python.unicode.identifiers", &OptionsPython::unicodeIdentifiers,
			       "Set to 0 to not recognise Python 3 Unicode identifiers.");

		DefineProperty("lexer.python.identifier.attributes", &OptionsPython::identifierAttributes,
			       "Set to 1 to recognise Python identifier attributes.");

		DefineProperty("lexer.python.decorator.attributes", &OptionsPython::decoratorAttributes,
			       "Set to 1 to recognise Python decorator attributes.");

		DefineWordListSets(pythonWordListDesc);
	}
};

const char styleSubable[] = { SCE_P_IDENTIFIER, 0 };

const LexicalClass lexicalClasses[] = {
	// Lexer Python SCLEX_PYTHON SCE_P_:
	0, "SCE_P_DEFAULT", "default", "White space",
	1, "SCE_P_COMMENTLINE", "comment line", "Comment",
	2, "SCE_P_NUMBER", "literal numeric", "Number",
	3, "SCE_P_STRING", "literal string", "String",
	4, "SCE_P_CHARACTER", "literal string", "Single quoted string",
	5, "SCE_P_WORD", "keyword", "Keyword",
	6, "SCE_P_TRIPLE", "literal string", "Triple quotes",
	7, "SCE_P_TRIPLEDOUBLE", "literal string", "Triple double quotes",
	8, "SCE_P_CLASSNAME", "identifier", "Class name definition",
	9, "SCE_P_DEFNAME", "identifier", "Function or method name definition",
	10, "SCE_P_OPERATOR", "operator", "Operators",
	11, "SCE_P_IDENTIFIER", "identifier", "Identifiers",
	12, "SCE_P_COMMENTBLOCK", "comment", "Comment-blocks",
	13, "SCE_P_STRINGEOL", "error literal string", "End of line where string is not closed",
	14, "SCE_P_WORD2", "identifier", "Highlighted identifiers",
	15, "SCE_P_DECORATOR", "preprocessor", "Decorators",
	16, "SCE_P_FSTRING", "literal string interpolated", "F-String",
	17, "SCE_P_FCHARACTER", "literal string interpolated", "Single quoted f-string",
	18, "SCE_P_FTRIPLE", "literal string interpolated", "Triple quoted f-string",
	19, "SCE_P_FTRIPLEDOUBLE", "literal string interpolated", "Triple double quoted f-string",
	20, "SCE_P_ATTRIBUTE", "identifier", "Attribute of identifier",
};

class LexerPython : public DefaultLexer {
	WordList keywords;
	WordList keywords2;
	OptionsPython options;
	OptionSetPython osPython;
	enum { ssIdentifier };
	SubStyles subStyles{styleSubable};
	std::map<Sci_Position, std::vector<SingleFStringExpState> > ftripleStateAtEol;
public:
	explicit LexerPython() :
		DefaultLexer("python", SCLEX_PYTHON, lexicalClasses, std::size(lexicalClasses)) {
	}
	~LexerPython() override = default;
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osPython.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osPython.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osPython.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osPython.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osPython.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void *SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}

	int SCI_METHOD LineEndTypesSupported() override {
		return SC_LINE_END_TYPE_UNICODE;
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

	static ILexer5 *LexerFactoryPython() {
		return new LexerPython();
	}

private:
	void ProcessLineEnd(StyleContext &sc, std::vector<SingleFStringExpState> &fstringStateStack, SingleFStringExpState *&currentFStringExp, bool &inContinuedString);
};

Sci_Position SCI_METHOD LexerPython::PropertySet(const char *key, const char *val) {
	if (osPython.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerPython::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &keywords2;
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

void LexerPython::ProcessLineEnd(StyleContext &sc, std::vector<SingleFStringExpState> &fstringStateStack, SingleFStringExpState *&currentFStringExp, bool &inContinuedString) {
	// Before pep 701 single quote f-string's could not continue to a 2nd+ line
	// Post pep 701, they can continue both with a trailing \ and if a { field is
	// not ended with a }
	if (!options.pep701StringsF) {
		long deepestSingleStateIndex = -1;
		unsigned long i;

		// Find the deepest single quote state because that string will end; no \ continuation in f-string
		for (i = 0; i < fstringStateStack.size(); i++) {
			if (IsPySingleQuoteStringState(fstringStateStack[i].state)) {
				deepestSingleStateIndex = i;
				break;
			}
		}

		if (deepestSingleStateIndex != -1) {
			sc.SetState(fstringStateStack[deepestSingleStateIndex].state);
			while (fstringStateStack.size() > static_cast<unsigned long>(deepestSingleStateIndex)) {
				PopFromStateStack(fstringStateStack, currentFStringExp);
			}
		}
	}

	if (!fstringStateStack.empty()) {
		std::pair<Sci_Position, std::vector<SingleFStringExpState> > val;
		val.first = sc.currentLine;
		val.second = fstringStateStack;

		ftripleStateAtEol.insert(val);
	}

	if ((sc.state == SCE_P_DEFAULT)
			|| IsPyTripleQuoteStringState(sc.state)) {
		// Perform colourisation of white space and triple quoted strings at end of each line to allow
		// tab marking to work inside white space and triple quoted strings
		sc.SetState(sc.state);
	}
	if (IsPySingleQuoteStringState(sc.state)) {
		if (inContinuedString || options.stringsOverNewline) {
			inContinuedString = false;
		} else {
			sc.ChangeState(SCE_P_STRINGEOL);
			sc.ForwardSetState(SCE_P_DEFAULT);
		}
	}
}

void SCI_METHOD LexerPython::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);

	// Track whether in f-string expression; vector is used for a stack to
	// handle nested f-strings such as f"""{f'''{f"{f'{1}'}"}'''}"""
	std::vector<SingleFStringExpState> fstringStateStack;
	SingleFStringExpState *currentFStringExp = nullptr;

	const Sci_Position endPos = startPos + length;

	// Backtrack to previous line in case need to fix its tab whinging
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			// Look for backslash-continued lines
			while (lineCurrent > 0) {
				const Sci_Position eolPos = styler.LineStart(lineCurrent) - 1;
				const int eolStyle = styler.StyleIndexAt(eolPos);
				if (AnyOf(eolStyle, SCE_P_STRING, SCE_P_CHARACTER, SCE_P_STRINGEOL)) {
					lineCurrent -= 1;
				} else {
					break;
				}
			}
			startPos = styler.LineStart(lineCurrent);
		}
		initStyle = startPos == 0 ? SCE_P_DEFAULT : styler.StyleIndexAt(startPos - 1);
	}

	const literalsAllowed allowedLiterals = options.AllowedLiterals();

	if (initStyle == SCE_P_STRINGEOL) {
		initStyle = SCE_P_DEFAULT;
	}

	// Set up fstate stack from last line and remove any subsequent ftriple at eol states
	std::map<Sci_Position, std::vector<SingleFStringExpState> >::iterator it;
	it = ftripleStateAtEol.find(lineCurrent - 1);
	if (it != ftripleStateAtEol.end() && !it->second.empty()) {
		fstringStateStack = it->second;
		currentFStringExp = &fstringStateStack.back();
	}
	it = ftripleStateAtEol.lower_bound(lineCurrent);
	if (it != ftripleStateAtEol.end()) {
		ftripleStateAtEol.erase(it, ftripleStateAtEol.end());
	}

	kwType kwLast = kwOther;
	int spaceFlags = 0;
	styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
	bool base_n_number = false;

	const WordClassifier &classifierIdentifiers = subStyles.Classifier(SCE_P_IDENTIFIER);

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	bool indentGood = true;
	Sci_Position startIndicator = sc.currentPos;
	bool inContinuedString = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
			indentGood = true;
			if (options.whingeLevel == 1) {
				indentGood = (spaceFlags & wsInconsistent) == 0;
			} else if (options.whingeLevel == 2) {
				indentGood = (spaceFlags & wsSpaceTab) == 0;
			} else if (options.whingeLevel == 3) {
				indentGood = (spaceFlags & wsSpace) == 0;
			} else if (options.whingeLevel == 4) {
				indentGood = (spaceFlags & wsTab) == 0;
			}
			if (!indentGood) {
				styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 0);
				startIndicator = sc.currentPos;
			}
		}

		if (sc.atLineEnd) {
			ProcessLineEnd(sc, fstringStateStack, currentFStringExp, inContinuedString);
			lineCurrent++;
			if (!sc.More())
				break;
		}

		bool needEOLCheck = false;


		if (sc.state == SCE_P_OPERATOR) {
			kwLast = kwOther;
			sc.SetState(SCE_P_DEFAULT);
		} else if (sc.state == SCE_P_NUMBER) {
			if (!IsAWordChar(sc.ch, false) &&
					!(!base_n_number && ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
				sc.SetState(SCE_P_DEFAULT);
			}
		} else if (sc.state == SCE_P_IDENTIFIER) {
			if ((sc.ch == '.') || (!IsAWordChar(sc.ch, options.unicodeIdentifiers))) {
				char identifier[100];
				sc.GetCurrent(identifier, sizeof(identifier));
				int style = SCE_P_IDENTIFIER;
				if ((kwLast == kwImport) && (strcmp(identifier, "as") == 0)) {
					style = SCE_P_WORD;
				} else if (keywords.InList(identifier) && !IsMatchOrCaseIdentifier(sc, styler, identifier)) {
					style = SCE_P_WORD;
				} else if (kwLast == kwClass) {
					style = SCE_P_CLASSNAME;
				} else if (kwLast == kwDef) {
					style = SCE_P_DEFNAME;
				} else if (kwLast == kwCDef || kwLast == kwCPDef) {
					Sci_Position pos = sc.currentPos;
					unsigned char ch = styler.SafeGetCharAt(pos, '\0');
					while (ch != '\0') {
						if (ch == '(') {
							style = SCE_P_DEFNAME;
							break;
						} else if (ch == ':') {
							style = SCE_P_CLASSNAME;
							break;
						} else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
							pos++;
							ch = styler.SafeGetCharAt(pos, '\0');
						} else {
							break;
						}
					}
				} else if (keywords2.InList(identifier)) {
					if (options.keywords2NoSubIdentifiers) {
						// We don't want to highlight keywords2
						// that are used as a sub-identifier,
						// i.e. not open in "foo.open".
						const Sci_Position pos = styler.GetStartSegment() - 1;
						if (pos < 0 || (styler.SafeGetCharAt(pos, '\0') != '.'))
							style = SCE_P_WORD2;
					} else {
						style = SCE_P_WORD2;
					}
				} else {
					const int subStyle = classifierIdentifiers.ValueFor(identifier);
					if (subStyle >= 0) {
						style = subStyle;
					}
					if (options.identifierAttributes > 0 || options.decoratorAttributes > 0) {
						// Does the user even want attributes styled?
						Sci_Position pos = styler.GetStartSegment() - 1;
						unsigned char ch = styler.SafeGetCharAt(pos, '\0');
						while (ch != '\0' && (ch == '.' || ch == ' ' || ch == '\\' || ch == '\t' || ch == '\n' || ch == '\r')) {
							// Backwards search for a . while only allowing certain valid characters
							if (IsAWordChar(ch, options.unicodeIdentifiers)) {
								break;
							}
							pos--;
							ch = styler.SafeGetCharAt(pos, '\0');
						}
						if (ch == '.') {
							// Is this an attribute we could style? if it is, do as asked
							bool isComment = AnyOf(styler.BufferStyleAt(pos), SCE_P_COMMENTLINE, SCE_P_COMMENTBLOCK);
							bool isDecoratorAttribute = false;
							const Sci_Position attrLine = styler.GetLine(pos);
							for (Sci_Position i = styler.LineStart(attrLine); i < pos; i++) {
								const char attrCh = styler[i];
								if (attrCh == '@')
									isDecoratorAttribute = true;
								if (attrCh == '#')
									isComment = true;
								// Detect if this attribute belongs to a decorator
								if (!IsASpaceOrTab(attrCh))
									break;
							}
							if (((isDecoratorAttribute) && (!isComment)) && (((options.decoratorAttributes == 1)  && (style == SCE_P_IDENTIFIER)) || (options.decoratorAttributes == 2))) {
								// Style decorator attributes as decorators but respect already styled identifiers (unless requested to ignore already styled identifiers)
								style = SCE_P_DECORATOR;
							}
							if (((!isDecoratorAttribute) && (!isComment)) && (((options.identifierAttributes == 1) && (style == SCE_P_IDENTIFIER)) || (options.identifierAttributes == 2))) {
								// Style attributes and ignore decorator attributes but respect already styled identifiers (unless requested to ignore already styled identifiers)
								style = SCE_P_ATTRIBUTE;
							}
						}
					}
				}
				sc.ChangeState(style);
				sc.SetState(SCE_P_DEFAULT);
				if (style == SCE_P_WORD) {
					if (0 == strcmp(identifier, "class"))
						kwLast = kwClass;
					else if (0 == strcmp(identifier, "def"))
						kwLast = kwDef;
					else if (0 == strcmp(identifier, "import"))
						kwLast = kwImport;
					else if (0 == strcmp(identifier, "cdef"))
						kwLast = kwCDef;
					else if (0 == strcmp(identifier, "cpdef"))
						kwLast = kwCPDef;
					else if (0 == strcmp(identifier, "cimport"))
						kwLast = kwImport;
					else if (kwLast != kwCDef && kwLast != kwCPDef)
						kwLast = kwOther;
				} else if (kwLast != kwCDef && kwLast != kwCPDef) {
					kwLast = kwOther;
				}
			}
		} else if ((sc.state == SCE_P_COMMENTLINE) || (sc.state == SCE_P_COMMENTBLOCK)) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_P_DEFAULT);
			}
		} else if (sc.state == SCE_P_DECORATOR) {
			if (!IsAWordStart(sc.ch, options.unicodeIdentifiers)) {
				sc.SetState(SCE_P_DEFAULT);
			}
		} else if (IsPySingleQuoteStringState(sc.state)) {
			if (sc.ch == '\\') {
				if ((sc.chNext == '\r') && (sc.GetRelative(2) == '\n')) {
					sc.Forward();
				}
				if (sc.chNext == '\n' || sc.chNext == '\r') {
					inContinuedString = true;
				} else {
					// Don't roll over the newline.
					sc.Forward();
				}
			} else if (sc.ch == GetPyStringQuoteChar(sc.state)) {
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		} else if ((sc.state == SCE_P_TRIPLE) || (sc.state == SCE_P_FTRIPLE)) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match(R"(''')")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		} else if ((sc.state == SCE_P_TRIPLEDOUBLE) || (sc.state == SCE_P_FTRIPLEDOUBLE)) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match(R"(""")")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		}

		// Note if used and not if else because string states also match
		// some of the above clauses
		if (IsPyFStringState(sc.state) && sc.ch == '{') {
			if (sc.chNext == '{') {
				sc.Forward();
			} else {
				PushStateToStack(sc.state, fstringStateStack, currentFStringExp);
				sc.ForwardSetState(SCE_P_DEFAULT);
			}
			needEOLCheck = true;
		}

		// If in an f-string expression and pre pep 701 lexing is used,
		// check for the ending quote(s) and end f-string to handle
		// syntactically incorrect cases like f'{' and f"""{""". Post
		// pep 701, a quote may appear in a { } field so cases like
		// f"n = {":".join(seq)}" is valid
		if (!options.pep701StringsF && !fstringStateStack.empty() && IsQuote(sc.ch)) {
			long matching_stack_i = -1;
			for (unsigned long stack_i = 0; stack_i < fstringStateStack.size() && matching_stack_i == -1; stack_i++) {
				const int stack_state = fstringStateStack[stack_i].state;
				const char quote = GetPyStringQuoteChar(stack_state);
				if (sc.ch == quote) {
					if (IsPySingleQuoteStringState(stack_state)) {
						matching_stack_i = stack_i;
					} else if (quote == '"' ? sc.Match(R"(""")") : sc.Match("'''")) {
						matching_stack_i = stack_i;
					}
				}
			}

			if (matching_stack_i != -1) {
				sc.SetState(fstringStateStack[matching_stack_i].state);
				if (IsPyTripleQuoteStringState(fstringStateStack[matching_stack_i].state)) {
					sc.Forward();
					sc.Forward();
				}
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;

				while (fstringStateStack.size() > static_cast<unsigned long>(matching_stack_i)) {
					PopFromStateStack(fstringStateStack, currentFStringExp);
				}
			}
		}
		// End of code to find the end of a state

		if (!indentGood && !IsASpaceOrTab(sc.ch)) {
			styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 1);
			startIndicator = sc.currentPos;
			indentGood = true;
		}

		// One cdef or cpdef line, clear kwLast only at end of line
		if ((kwLast == kwCDef || kwLast == kwCPDef) && sc.atLineEnd) {
			kwLast = kwOther;
		}

		// State exit code may have moved on to end of line
		if (needEOLCheck && sc.atLineEnd) {
			ProcessLineEnd(sc, fstringStateStack, currentFStringExp, inContinuedString);
			lineCurrent++;
			styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
			if (!sc.More())
				break;
		}

		// If in f-string expression, check for }, :, ! to resume f-string state or update nesting count
		if (currentFStringExp && !IsPySingleQuoteStringState(sc.state) && !IsPyTripleQuoteStringState(sc.state)) {
			if (currentFStringExp->nestingCount == 0 && (sc.ch == '}' || sc.ch == ':' || (sc.ch == '!' && sc.chNext != '='))) {
				sc.SetState(PopFromStateStack(fstringStateStack, currentFStringExp));
			} else {
				if (sc.ch == '{' || sc.ch == '[' || sc.ch == '(') {
					currentFStringExp->nestingCount++;
				} else if (sc.ch == '}' || sc.ch == ']' || sc.ch == ')') {
					currentFStringExp->nestingCount--;
				}
			}
		}

		// Check for a new state starting character
		if (sc.state == SCE_P_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (sc.ch == '0' && (sc.chNext == 'x' || sc.chNext == 'X')) {
					base_n_number = true;
					sc.SetState(SCE_P_NUMBER);
				} else if (sc.ch == '0' &&
						(sc.chNext == 'o' || sc.chNext == 'O' || sc.chNext == 'b' || sc.chNext == 'B')) {
					if (options.base2or8Literals) {
						base_n_number = true;
						sc.SetState(SCE_P_NUMBER);
					} else {
						sc.SetState(SCE_P_NUMBER);
						sc.ForwardSetState(SCE_P_IDENTIFIER);
					}
				} else {
					base_n_number = false;
					sc.SetState(SCE_P_NUMBER);
				}
			} else if (isoperator(sc.ch) || sc.ch == '`') {
				sc.SetState(SCE_P_OPERATOR);
			} else if (sc.ch == '#') {
				sc.SetState(sc.chNext == '#' ? SCE_P_COMMENTBLOCK : SCE_P_COMMENTLINE);
			} else if (sc.ch == '@') {
				if (IsFirstNonWhitespace(sc.currentPos, styler))
					sc.SetState(SCE_P_DECORATOR);
				else
					sc.SetState(SCE_P_OPERATOR);
			} else if (IsPyStringStart(sc.ch, sc.chNext, sc.GetRelative(2), allowedLiterals)) {
				Sci_PositionU nextIndex = 0;
				sc.SetState(GetPyStringState(styler, sc.currentPos, &nextIndex, allowedLiterals));
				while (nextIndex > (sc.currentPos + 1) && sc.More()) {
					sc.Forward();
				}
			} else if (IsAWordStart(sc.ch, options.unicodeIdentifiers)) {
				sc.SetState(SCE_P_IDENTIFIER);
			}
		}
	}
	styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 0);
	sc.Complete();
}

bool IsCommentLine(Sci_Position line, Accessor &styler) {
	const Sci_Position pos = styler.LineStart(line);
	const Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		const char ch = styler[i];
		if (ch == '#')
			return true;
		if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

bool IsQuoteLine(Sci_Position line, const Accessor &styler) {
	const int style = styler.StyleIndexAt(styler.LineStart(line));
	return IsPyTripleQuoteStringState(style);
}


void SCI_METHOD LexerPython::Fold(Sci_PositionU startPos, Sci_Position length, int /*initStyle - unused*/, IDocument *pAccess) {
	if (!options.fold)
		return;

	Accessor styler(pAccess, nullptr);

	const Sci_Position maxPos = startPos + length;
	const Sci_Position maxLines = (maxPos == styler.Length()) ? styler.GetLine(maxPos) : styler.GetLine(maxPos - 1);	// Requested last line
	const Sci_Position docLines = styler.GetLine(styler.Length());	// Available last line

	// Backtrack to previous non-blank line so we can determine indent level
	// for any white space lines (needed esp. within triple quoted strings)
	// and so we can fix any preceding fold level (which is why we go back
	// at least one line in all cases)
	int spaceFlags = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, nullptr);
	while (lineCurrent > 0) {
		lineCurrent--;
		indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, nullptr);
		if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG) &&
				(!IsCommentLine(lineCurrent, styler)) &&
				(!IsQuoteLine(lineCurrent, styler)))
			break;
	}
	int indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;

	// Set up initial loop state
	startPos = styler.LineStart(lineCurrent);
	int prev_state = SCE_P_DEFAULT;
	if (lineCurrent >= 1)
		prev_state = styler.StyleIndexAt(startPos - 1);
	int prevQuote = options.foldQuotes && IsPyTripleQuoteStringState(prev_state);

	// Process all characters to end of requested range or end of any triple quote
	//that hangs over the end of the range.  Cap processing in all cases
	// to end of document (in case of unclosed quote at end).
	while ((lineCurrent <= docLines) && ((lineCurrent <= maxLines) || prevQuote)) {

		// Gather info
		int lev = indentCurrent;
		Sci_Position lineNext = lineCurrent + 1;
		int indentNext = indentCurrent;
		int quote = false;
		if (lineNext <= docLines) {
			// Information about next line is only available if not at end of document
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, nullptr);
			const Sci_Position lookAtPos = (styler.LineStart(lineNext) == styler.Length()) ? styler.Length() - 1 : styler.LineStart(lineNext);
			const int style = styler.StyleIndexAt(lookAtPos);
			quote = options.foldQuotes && IsPyTripleQuoteStringState(style);
		}
		const bool quote_start = (quote && !prevQuote);
		const bool quote_continue = (quote && prevQuote);
		if (!quote || !prevQuote)
			indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;
		if (quote)
			indentNext = indentCurrentLevel;
		if (indentNext & SC_FOLDLEVELWHITEFLAG)
			indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;

		if (quote_start) {
			// Place fold point at start of triple quoted string
			lev |= SC_FOLDLEVELHEADERFLAG;
		} else if (quote_continue || prevQuote) {
			// Add level to rest of lines in the string
			lev = lev + 1;
		}

		// Skip past any blank lines for next indent level info; we skip also
		// comments (all comments, not just those starting in column 0)
		// which effectively folds them into surrounding code rather
		// than screwing up folding.  If comments end file, use the min
		// comment indent as the level after

		int minCommentLevel = indentCurrentLevel;
		while (!quote &&
				(lineNext < docLines) &&
				((indentNext & SC_FOLDLEVELWHITEFLAG) || (IsCommentLine(lineNext, styler)))) {

			if (IsCommentLine(lineNext, styler) && indentNext < minCommentLevel) {
				minCommentLevel = indentNext;
			}

			lineNext++;
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, nullptr);
		}

		const int levelAfterComments = ((lineNext < docLines) ? indentNext & SC_FOLDLEVELNUMBERMASK : minCommentLevel);
		const int levelBeforeComments = std::max(indentCurrentLevel, levelAfterComments);

		// Now set all the indent levels on the lines we skipped
		// Do this from end to start.  Once we encounter one line
		// which is indented more than the line after the end of
		// the comment-block, use the level of the block before

		Sci_Position skipLine = lineNext;
		int skipLevel = levelAfterComments;

		while (--skipLine > lineCurrent) {
			const int skipLineIndent = styler.IndentAmount(skipLine, &spaceFlags, nullptr);

			if (options.foldCompact) {
				if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments)
					skipLevel = levelBeforeComments;

				const int whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;

				styler.SetLevel(skipLine, skipLevel | whiteFlag);
			} else {
				if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments &&
						!(skipLineIndent & SC_FOLDLEVELWHITEFLAG) &&
						!IsCommentLine(skipLine, styler))
					skipLevel = levelBeforeComments;

				styler.SetLevel(skipLine, skipLevel);
			}
		}

		// Set fold header on non-quote line
		if (!quote && !(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
			if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
				lev |= SC_FOLDLEVELHEADERFLAG;
		}

		// Keep track of triple quote state of previous line
		prevQuote = quote;

		// Set fold level for this line and move to next line
		styler.SetLevel(lineCurrent, options.foldCompact ? lev : lev & ~SC_FOLDLEVELWHITEFLAG);
		indentCurrent = indentNext;
		lineCurrent = lineNext;
	}

	// NOTE: Cannot set level of last line here because indentCurrent doesn't have
	// header flag set; the loop above is crafted to take care of this case!
	//styler.SetLevel(lineCurrent, indentCurrent);
}

}

extern const LexerModule lmPython(SCLEX_PYTHON, LexerPython::LexerFactoryPython, "python",
		     pythonWordListDesc);
