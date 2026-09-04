// Scintilla source code edit control
/** @file LexGDScript.cxx
 ** Lexer for GDScript.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// Heavily modified later for GDScript
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

enum kwType { kwOther, kwClass, kwDef, kwExtends};

constexpr int indicatorWhitespace = 1;

bool IsGDStringStart(int ch) {
    return (ch == '\'' || ch == '"');
}

bool IsGDComment(Accessor &styler, Sci_Position pos, Sci_Position len) {
	return len > 0 && styler[pos] == '#';
}

constexpr bool IsGDSingleQuoteStringState(int st) noexcept {
	return ((st == SCE_GD_CHARACTER) || (st == SCE_GD_STRING));
}

constexpr bool IsGDTripleQuoteStringState(int st) noexcept {
	return ((st == SCE_GD_TRIPLE) || (st == SCE_GD_TRIPLEDOUBLE));
}

char GetGDStringQuoteChar(int st) noexcept {
	if ((st == SCE_GD_CHARACTER) || (st == SCE_GD_TRIPLE))
		return '\'';
	if ((st == SCE_GD_STRING) || (st == SCE_GD_TRIPLEDOUBLE))
		return '"';

	return '\0';
}

/* Return the state to use for the string starting at i; *nextIndex will be set to the first index following the quote(s) */
int GetGDStringState(Accessor &styler, Sci_Position i, Sci_PositionU *nextIndex) {
	char ch = styler.SafeGetCharAt(i);
	char chNext = styler.SafeGetCharAt(i + 1);

	if (ch != '"' && ch != '\'') {
		*nextIndex = i + 1;
		return SCE_GD_DEFAULT;
	}

	if (ch == chNext && ch == styler.SafeGetCharAt(i + 2)) {
		*nextIndex = i + 3;

		if (ch == '"')
			return SCE_GD_TRIPLEDOUBLE;
		else
			return SCE_GD_TRIPLE;
	} else {
		*nextIndex = i + 1;

		if (ch == '"')
			return SCE_GD_STRING;
		else
			return SCE_GD_CHARACTER;
	}
}

int GetGDStringState(int ch) {
	if (ch != '"' && ch != '\'')
		return SCE_GD_DEFAULT;

	if (ch == '"')
		return SCE_GD_STRING;
	else
		return SCE_GD_CHARACTER;
}

inline bool IsAWordChar(int ch, bool unicodeIdentifiers) {
	if (IsASCII(ch))
		return (IsAlphaNumeric(ch) || ch == '.' || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	return IsXidContinue(ch);
}

inline bool IsANodePathChar(int ch, bool unicodeIdentifiers) {
	if (IsASCII(ch))
		return (IsAlphaNumeric(ch) || ch == '_' || ch == '/' || ch =='%');

	if (!unicodeIdentifiers)
		return false;

	return IsXidContinue(ch);
}

inline bool IsAWordStart(int ch, bool unicodeIdentifiers) {
	if (IsASCII(ch))
		return (IsUpperOrLowerCase(ch) || ch == '_');

	if (!unicodeIdentifiers)
		return false;

	return IsXidStart(ch);
}

bool IsFirstNonWhitespace(Sci_Position pos, Accessor &styler) {
	const Sci_Position line = styler.GetLine(pos);
	const Sci_Position start_pos = styler.LineStart(line);
	for (Sci_Position i = start_pos; i < pos; i++) {
		const char ch = styler[i];
		if (!(ch == ' ' || ch == '\t'))
			return false;
	}
	return true;
}

// Options used for LexerGDScript
struct OptionsGDScript {
	int whingeLevel;
	bool base2or8Literals;
	bool stringsOverNewline;
	bool keywords2NoSubIdentifiers;
	bool fold;
	bool foldQuotes;
	bool foldCompact;
	bool unicodeIdentifiers;

	OptionsGDScript() noexcept {
		whingeLevel = 0;
		base2or8Literals = true;
		stringsOverNewline = false;
		keywords2NoSubIdentifiers = false;
		fold = false;
		foldQuotes = false;
		foldCompact = false;
		unicodeIdentifiers = true;
	}
};

const char *const gdscriptWordListDesc[] = {
	"Keywords",
	"Highlighted identifiers",
	nullptr
};

struct OptionSetGDScript : public OptionSet<OptionsGDScript> {
	OptionSetGDScript() {
		DefineProperty("lexer.gdscript.whinge.level", &OptionsGDScript::whingeLevel,
			       "For GDScript code, checks whether indenting is consistent. "
			       "The default, 0 turns off indentation checking, "
			       "1 checks whether each line is potentially inconsistent with the previous line, "
			       "2 checks whether any space characters occur before a tab character in the indentation, "
			       "3 checks whether any spaces are in the indentation, and "
			       "4 checks for any tab characters in the indentation. "
			       "1 is a good level to use.");

		DefineProperty("lexer.gdscript.literals.binary", &OptionsGDScript::base2or8Literals,
			       "Set to 0 to not recognise binary and octal literals: 0b1011 0o712.");

		DefineProperty("lexer.gdscript.strings.over.newline", &OptionsGDScript::stringsOverNewline,
			       "Set to 1 to allow strings to span newline characters.");

		DefineProperty("lexer.gdscript.keywords2.no.sub.identifiers", &OptionsGDScript::keywords2NoSubIdentifiers,
			       "When enabled, it will not style keywords2 items that are used as a sub-identifier. "
			       "Example: when set, will not highlight \"foo.open\" when \"open\" is a keywords2 item.");

		DefineProperty("fold", &OptionsGDScript::fold);

		DefineProperty("fold.gdscript.quotes", &OptionsGDScript::foldQuotes,
			       "This option enables folding multi-line quoted strings when using the GDScript lexer.");

		DefineProperty("fold.compact", &OptionsGDScript::foldCompact);

		DefineProperty("lexer.gdscript.unicode.identifiers", &OptionsGDScript::unicodeIdentifiers,
			       "Set to 0 to not recognise Unicode identifiers.");

		DefineWordListSets(gdscriptWordListDesc);
	}
};

const char styleSubable[] = { SCE_GD_IDENTIFIER, 0 };

LexicalClass lexicalClasses[] = {
	// Lexer GDScript SCLEX_GDSCRIPT SCE_GD_:
	0, "SCE_GD_DEFAULT", "default", "White space",
	1, "SCE_GD_COMMENTLINE", "comment line", "Comment",
	2, "SCE_GD_NUMBER", "literal numeric", "Number",
	3, "SCE_GD_STRING", "literal string", "String",
	4, "SCE_GD_CHARACTER", "literal string", "Single quoted string",
	5, "SCE_GD_WORD", "keyword", "Keyword",
	6, "SCE_GD_TRIPLE", "literal string", "Triple quotes",
	7, "SCE_GD_TRIPLEDOUBLE", "literal string", "Triple double quotes",
	8, "SCE_GD_CLASSNAME", "identifier", "Class name definition",
	9, "SCE_GD_FUNCNAME", "identifier", "Function or method name definition",
	10, "SCE_GD_OPERATOR", "operator", "Operators",
	11, "SCE_GD_IDENTIFIER", "identifier", "Identifiers",
	12, "SCE_GD_COMMENTBLOCK", "comment", "Comment-blocks",
	13, "SCE_GD_STRINGEOL", "error literal string", "End of line where string is not closed",
	14, "SCE_GD_WORD2", "identifier", "Highlighted identifiers",
	15, "SCE_GD_ANNOTATION", "annotation", "Annotations",
	16, "SCE_GD_NODEPATH", "path", "Node path",
};

}

class LexerGDScript : public DefaultLexer {
	WordList keywords;
	WordList keywords2;
	OptionsGDScript options;
	OptionSetGDScript osGDScript;
	enum { ssIdentifier };
	SubStyles subStyles{styleSubable};
public:
	explicit LexerGDScript() :
		DefaultLexer("gdscript", SCLEX_GDSCRIPT, lexicalClasses, ELEMENTS(lexicalClasses)) {
	}
	~LexerGDScript() override {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osGDScript.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osGDScript.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osGDScript.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osGDScript.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osGDScript.DescribeWordListSets();
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

	static ILexer5 *LexerFactoryGDScript() {
		return new LexerGDScript();
	}

private:
	void ProcessLineEnd(StyleContext &sc, bool &inContinuedString);
};

Sci_Position SCI_METHOD LexerGDScript::PropertySet(const char *key, const char *val) {
	if (osGDScript.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerGDScript::WordListSet(int n, const char *wl) {
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
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

void LexerGDScript::ProcessLineEnd(StyleContext &sc, bool &inContinuedString) {
	if ((sc.state == SCE_GD_DEFAULT)
			|| IsGDTripleQuoteStringState(sc.state)) {
		// Perform colourisation of white space and triple quoted strings at end of each line to allow
		// tab marking to work inside white space and triple quoted strings
		sc.SetState(sc.state);
	}

	if (IsGDSingleQuoteStringState(sc.state)) {
		if (inContinuedString || options.stringsOverNewline) {
			inContinuedString = false;
		} else {
			sc.ChangeState(SCE_GD_STRINGEOL);
			sc.ForwardSetState(SCE_GD_DEFAULT);
		}
	}
}

void SCI_METHOD LexerGDScript::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);

	const Sci_Position endPos = startPos + length;

	// Backtrack to previous line in case need to fix its tab whinging
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			// Look for backslash-continued lines
			while (lineCurrent > 0) {
				const Sci_Position eolPos = styler.LineStart(lineCurrent) - 1;
				const int eolStyle = styler.StyleAt(eolPos);
				if (eolStyle == SCE_GD_STRING || eolStyle == SCE_GD_CHARACTER
						|| eolStyle == SCE_GD_STRINGEOL) {
					lineCurrent -= 1;
				} else {
					break;
				}
			}
			startPos = styler.LineStart(lineCurrent);
		}
		initStyle = startPos == 0 ? SCE_GD_DEFAULT : styler.StyleAt(startPos - 1);
	}

	initStyle = initStyle & 31;
	if (initStyle == SCE_GD_STRINGEOL) {
		initStyle = SCE_GD_DEFAULT;
	}

	kwType kwLast = kwOther;
	int spaceFlags = 0;
	styler.IndentAmount(lineCurrent, &spaceFlags, IsGDComment);
	bool base_n_number = false;

	const WordClassifier &classifierIdentifiers = subStyles.Classifier(SCE_GD_IDENTIFIER);

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	bool indentGood = true;
	Sci_Position startIndicator = sc.currentPos;
	bool inContinuedString = false;
	bool percentIsNodePath = false;
	int nodePathStringState = SCE_GD_DEFAULT;
	
	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			styler.IndentAmount(lineCurrent, &spaceFlags, IsGDComment);
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
			percentIsNodePath = false;
			ProcessLineEnd(sc, inContinuedString);
			lineCurrent++;
			if (!sc.More())
				break;
		}

		bool needEOLCheck = false;

		if (sc.state == SCE_GD_OPERATOR) {
			kwLast = kwOther;
			sc.SetState(SCE_GD_DEFAULT);
		} else if (sc.state == SCE_GD_NUMBER) {
			if (!IsAWordChar(sc.ch, false) &&
					!(!base_n_number && ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
				sc.SetState(SCE_GD_DEFAULT);
			}
		} else if (sc.state == SCE_GD_IDENTIFIER) {
			if ((sc.ch == '.') || (!IsAWordChar(sc.ch, options.unicodeIdentifiers))) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				int style = SCE_GD_IDENTIFIER;
				if (keywords.InList(s)) {
					style = SCE_GD_WORD;
				} else if (kwLast == kwClass) {
					style = SCE_GD_CLASSNAME;
				} else if (kwLast == kwDef) {
					style = SCE_GD_FUNCNAME;
				} else if (keywords2.InList(s)) {
					if (options.keywords2NoSubIdentifiers) {
						// We don't want to highlight keywords2
						// that are used as a sub-identifier,
						// i.e. not open in "foo.open".
						const Sci_Position pos = styler.GetStartSegment() - 1;
						if (pos < 0 || (styler.SafeGetCharAt(pos, '\0') != '.'))
							style = SCE_GD_WORD2;
					} else {
						style = SCE_GD_WORD2;
					}
				} else {
					const int subStyle = classifierIdentifiers.ValueFor(s);
					if (subStyle >= 0) {
						style = subStyle;
					}
				}
				sc.ChangeState(style);
				sc.SetState(SCE_GD_DEFAULT);
				if (style == SCE_GD_WORD) {
					if (0 == strcmp(s, "class"))
						kwLast = kwClass;
					else if (0 == strcmp(s, "func"))
						kwLast = kwDef;
					else if (0 == strcmp(s, "extends"))
						kwLast = kwExtends;
					else
						kwLast = kwOther;
				} else {
					kwLast = kwOther;
				}
			}
		} else if ((sc.state == SCE_GD_COMMENTLINE) || (sc.state == SCE_GD_COMMENTBLOCK)) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_GD_DEFAULT);
			}
		} else if (sc.state == SCE_GD_ANNOTATION) {
			if (!IsAWordStart(sc.ch, options.unicodeIdentifiers)) {
				sc.SetState(SCE_GD_DEFAULT);
			}
		} else if (sc.state == SCE_GD_NODEPATH) {
			if (nodePathStringState != SCE_GD_DEFAULT) {
				if (sc.ch == GetGDStringQuoteChar(nodePathStringState) ) {
					nodePathStringState = SCE_GD_DEFAULT;
				}
			} else {
				if (IsGDStringStart(sc.ch)) {
					nodePathStringState = GetGDStringState(sc.ch);
				} else if (!IsANodePathChar(sc.ch, options.unicodeIdentifiers)) {
					sc.SetState(SCE_GD_DEFAULT);
				}
			}
		} else if (IsGDSingleQuoteStringState(sc.state)) {
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
			} else if (sc.ch == GetGDStringQuoteChar(sc.state)) {
				sc.ForwardSetState(SCE_GD_DEFAULT);
				needEOLCheck = true;
			}
		} else if (sc.state == SCE_GD_TRIPLE) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match(R"(''')")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_GD_DEFAULT);
				needEOLCheck = true;
			}
		} else if (sc.state == SCE_GD_TRIPLEDOUBLE) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match(R"(""")")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_GD_DEFAULT);
				needEOLCheck = true;
			}
		}

		if (!indentGood && !IsASpaceOrTab(sc.ch)) {
			styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 1);
			startIndicator = sc.currentPos;
			indentGood = true;
		}

		// State exit code may have moved on to end of line
		if (needEOLCheck && sc.atLineEnd) {
			ProcessLineEnd(sc, inContinuedString);
			lineCurrent++;
			styler.IndentAmount(lineCurrent, &spaceFlags, IsGDComment);
			if (!sc.More())
				break;
		}

		// Check for a new state starting character
		if (sc.state == SCE_GD_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (sc.ch == '0' && (sc.chNext == 'x' || sc.chNext == 'X')) {
					base_n_number = true;
					sc.SetState(SCE_GD_NUMBER);
				} else if (sc.ch == '0' &&
						(sc.chNext == 'o' || sc.chNext == 'O' || sc.chNext == 'b' || sc.chNext == 'B')) {
					if (options.base2or8Literals) {
						base_n_number = true;
						sc.SetState(SCE_GD_NUMBER);
					} else {
						sc.SetState(SCE_GD_NUMBER);
						sc.ForwardSetState(SCE_GD_IDENTIFIER);
					}
				} else {
					base_n_number = false;
					sc.SetState(SCE_GD_NUMBER);
				}
			} else if ((sc.ch == '$') || (sc.ch == '%' && (percentIsNodePath || IsFirstNonWhitespace(sc.currentPos, styler)))) {
				percentIsNodePath = false;
				sc.SetState(SCE_GD_NODEPATH);
			} else if (isoperator(sc.ch) || sc.ch == '`') {
				percentIsNodePath = !((sc.ch == ')') || (sc.ch == ']') || (sc.ch == '}'));
				sc.SetState(SCE_GD_OPERATOR);
			} else if (sc.ch == '#') {
				sc.SetState(sc.chNext == '#' ? SCE_GD_COMMENTBLOCK : SCE_GD_COMMENTLINE);
			} else if (sc.ch == '@') {
				if (IsFirstNonWhitespace(sc.currentPos, styler))
					sc.SetState(SCE_GD_ANNOTATION);
				else
					sc.SetState(SCE_GD_OPERATOR);
			} else if (IsGDStringStart(sc.ch)) {
				Sci_PositionU nextIndex = 0;
				sc.SetState(GetGDStringState(styler, sc.currentPos, &nextIndex));
				while (nextIndex > (sc.currentPos + 1) && sc.More()) {
					sc.Forward();
				}
            } else if (IsAWordStart(sc.ch, options.unicodeIdentifiers)) {
				sc.SetState(SCE_GD_IDENTIFIER);
			}
		}
	}
	styler.IndicatorFill(startIndicator, sc.currentPos, indicatorWhitespace, 0);
	sc.Complete();
}

static bool IsCommentLine(Sci_Position line, Accessor &styler) {
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

static bool IsQuoteLine(Sci_Position line, const Accessor &styler) {
	const int style = styler.StyleAt(styler.LineStart(line)) & 31;
	return IsGDTripleQuoteStringState(style);
}


void SCI_METHOD LexerGDScript::Fold(Sci_PositionU startPos, Sci_Position length, int /*initStyle - unused*/, IDocument *pAccess) {
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
	int prev_state = SCE_GD_DEFAULT & 31;
	if (lineCurrent >= 1)
		prev_state = styler.StyleAt(startPos - 1) & 31;
	int prevQuote = options.foldQuotes && IsGDTripleQuoteStringState(prev_state);

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
			const int style = styler.StyleAt(lookAtPos) & 31;
			quote = options.foldQuotes && IsGDTripleQuoteStringState(style);
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

extern const LexerModule lmGDScript(SCLEX_GDSCRIPT, LexerGDScript::LexerFactoryGDScript, "gdscript",
		     gdscriptWordListDesc);
