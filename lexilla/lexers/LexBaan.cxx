// Scintilla source code edit control
/** @file LexBaan.cxx
** Lexer for Baan.
** Based heavily on LexCPP.cxx
**/
// Copyright 2001- by Vamsi Potluru <vamsi@who.net> & Praveen Ambekar <ambekarpraveen@yahoo.com>
// Maintainer Email: oirfeodent@yahoo.co.in
// The License.txt file describes the conditions under which this software may be distributed.

// C standard library
#include <stdlib.h>
#include <string.h>

// C++ wrappers of C standard library
#include <cassert>

// C++ standard library
#include <string>
#include <string_view>
#include <map>
#include <functional>

// Scintilla headers

// Non-platform-specific headers

// include
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

// lexlib
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
// Use an unnamed namespace to protect the functions and classes from name conflicts

// Options used for LexerBaan
struct OptionsBaan {
	bool fold;
	bool foldComment;
	bool foldPreprocessor;
	bool foldCompact;
	bool baanFoldSyntaxBased;
	bool baanFoldKeywordsBased;
	bool baanFoldSections;
	bool baanFoldInnerLevel;
	bool baanStylingWithinPreprocessor;
	OptionsBaan() {
		fold = false;
		foldComment = false;
		foldPreprocessor = false;
		foldCompact = false;
		baanFoldSyntaxBased = false;
		baanFoldKeywordsBased = false;
		baanFoldSections = false;
		baanFoldInnerLevel = false;
		baanStylingWithinPreprocessor = false;
	}
};

const char *const baanWordLists[] = {
	"Baan & BaanSQL Reserved Keywords ",
	"Baan Standard functions",
	"Baan Functions Abridged",
	"Baan Main Sections ",
	"Baan Sub Sections",
	"PreDefined Variables",
	"PreDefined Attributes",
	"Enumerates",
	0,
};

struct OptionSetBaan : public OptionSet<OptionsBaan> {
	OptionSetBaan() {
		DefineProperty("fold", &OptionsBaan::fold);

		DefineProperty("fold.comment", &OptionsBaan::foldComment);

		DefineProperty("fold.preprocessor", &OptionsBaan::foldPreprocessor);

		DefineProperty("fold.compact", &OptionsBaan::foldCompact);

		DefineProperty("fold.baan.syntax.based", &OptionsBaan::baanFoldSyntaxBased,
			"Set this property to 0 to disable syntax based folding, which is folding based on '{' & '('.");

		DefineProperty("fold.baan.keywords.based", &OptionsBaan::baanFoldKeywordsBased,
			"Set this property to 0 to disable keywords based folding, which is folding based on "
			" for, if, on (case), repeat, select, while and fold ends based on endfor, endif, endcase, until, endselect, endwhile respectively."
			"Also folds declarations which are grouped together.");

		DefineProperty("fold.baan.sections", &OptionsBaan::baanFoldSections,
			"Set this property to 0 to disable folding of Main Sections as well as Sub Sections.");

		DefineProperty("fold.baan.inner.level", &OptionsBaan::baanFoldInnerLevel,
			"Set this property to 1 to enable folding of inner levels of select statements."
			"Disabled by default. case and if statements are also eligible" );

		DefineProperty("lexer.baan.styling.within.preprocessor", &OptionsBaan::baanStylingWithinPreprocessor,
			"For Baan code, determines whether all preprocessor code is styled in the "
			"preprocessor style (0, the default) or only from the initial # to the end "
			"of the command word(1).");

		DefineWordListSets(baanWordLists);
	}
};

static inline bool IsAWordChar(const int  ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch == '$');
}

static inline bool IsAnOperator(int ch) {
	if (IsAlphaNumeric(ch))
		return false;
	if (ch == '#' || ch == '^' || ch == '&' || ch == '*' ||
		ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
		ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
		ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
		ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
		ch == '?' || ch == '!' || ch == '"' || ch == '~' ||
		ch == '\\')
		return true;
	return false;
}

static inline int IsAnyOtherIdentifier(char *s, Sci_Position sLength) {

	/*	IsAnyOtherIdentifier uses standard templates used in baan.
	The matching template is shown as comments just above the return condition.
	^ - refers to any character [a-z].
	# - refers to any number [0-9].
	Other characters shown are compared as is.
	Tried implementing with Regex... it was too complicated for me.
	Any other implementation suggestion welcome.
	*/
	switch (sLength) {
	case 8:
		if (isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			//^^^^^###
			return(SCE_BAAN_TABLEDEF);
		}
		break;
	case 9:
		if (s[0] == 't' && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && isalpha(s[5]) && IsADigit(s[6]) && IsADigit(s[7]) && IsADigit(s[8])) {
			//t^^^^^###
			return(SCE_BAAN_TABLEDEF);
		}
		else if (s[8] == '.' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			//^^^^^###.
			return(SCE_BAAN_TABLESQL);
		}
		break;
	case 13:
		if (s[8] == '.' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			//^^^^^###.****
			return(SCE_BAAN_TABLESQL);
		}
		else if (s[0] == 'r' && s[1] == 'c' && s[2] == 'd' && s[3] == '.' && s[4] == 't' && isalpha(s[5]) && isalpha(s[6]) && isalpha(s[7]) && isalpha(s[8]) && isalpha(s[9]) && IsADigit(s[10]) && IsADigit(s[11]) && IsADigit(s[12])) {
			//rcd.t^^^^^###
			return(SCE_BAAN_TABLEDEF);
		}
		break;
	case 14:
	case 15:
		if (s[8] == '.' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			if (s[13] != ':') {
				//^^^^^###.******
				return(SCE_BAAN_TABLESQL);
			}
		}
		break;
	case 16:
	case 17:
		if (s[8] == '.' && s[9] == '_' && s[10] == 'i' && s[11] == 'n' && s[12] == 'd' && s[13] == 'e' && s[14] == 'x' && IsADigit(s[15]) && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			//^^^^^###._index##
			return(SCE_BAAN_TABLEDEF);
		}
		else if (s[8] == '.' && s[9] == '_' && s[10] == 'c' && s[11] == 'o' && s[12] == 'm' && s[13] == 'p' && s[14] == 'n' && s[15] == 'r' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[5]) && IsADigit(s[6]) && IsADigit(s[7])) {
			//^^^^^###._compnr
			return(SCE_BAAN_TABLEDEF);
		}
		break;
	default:
		break;
	}
	if (sLength > 14 && s[5] == '.' && s[6] == 'd' && s[7] == 'l' && s[8] == 'l' && s[13] == '.' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && IsADigit(s[9]) && IsADigit(s[10]) && IsADigit(s[11]) && IsADigit(s[12])) {
		//^^^^^.dll####.
		return(SCE_BAAN_FUNCTION);
	}
	else if (sLength > 15 && s[2] == 'i' && s[3] == 'n' && s[4] == 't' && s[5] == '.' && s[6] == 'd' && s[7] == 'l' && s[8] == 'l' && isalpha(s[0]) && isalpha(s[1]) && isalpha(s[9]) && isalpha(s[10]) && isalpha(s[11]) && isalpha(s[12]) && isalpha(s[13])) {
		//^^int.dll^^^^^.
		return(SCE_BAAN_FUNCTION);
	}
	else if (sLength > 11 && s[0] == 'i' && s[10] == '.' && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3]) && isalpha(s[4]) && isalpha(s[5]) && IsADigit(s[6]) && IsADigit(s[7]) && IsADigit(s[8]) && IsADigit(s[9])) {
		//i^^^^^####.
		return(SCE_BAAN_FUNCTION);
	}

	return(SCE_BAAN_DEFAULT);
}

static bool IsCommentLine(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (ch == '|' && style == SCE_BAAN_COMMENT)
			return true;
		else if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

static bool IsPreProcLine(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (ch == '#' && style == SCE_BAAN_PREPROCESSOR) {
			if (styler.Match(i, "#elif") || styler.Match(i, "#else") || styler.Match(i, "#endif")
				|| styler.Match(i, "#if") || styler.Match(i, "#ifdef") || styler.Match(i, "#ifndef"))
				// Above PreProcessors has a seperate fold mechanism.
				return false;
			else
				return true;
		}
		else if (ch == '^')
			return true;
		else if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

static int mainOrSubSectionLine(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (style == SCE_BAAN_WORD5 || style == SCE_BAAN_WORD4)
			return style;
		else if (IsASpaceOrTab(ch))
			continue;
		else
			break;
	}
	return 0;
}

static bool priorSectionIsSubSection(Sci_Position line, LexAccessor &styler){
	while (line > 0) {
		Sci_Position pos = styler.LineStart(line);
		Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
		for (Sci_Position i = pos; i < eol_pos; i++) {
			char ch = styler[i];
			int style = styler.StyleAt(i);
			if (style == SCE_BAAN_WORD4)
				return true;
			else if (style == SCE_BAAN_WORD5)
				return false;
			else if (IsASpaceOrTab(ch))
				continue;
			else
				break;
		}
		line--;
	}
	return false;
}

static bool nextSectionIsSubSection(Sci_Position line, LexAccessor &styler) {
	while (line > 0) {
		Sci_Position pos = styler.LineStart(line);
		Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
		for (Sci_Position i = pos; i < eol_pos; i++) {
			char ch = styler[i];
			int style = styler.StyleAt(i);
			if (style == SCE_BAAN_WORD4)
				return true;
			else if (style == SCE_BAAN_WORD5)
				return false;
			else if (IsASpaceOrTab(ch))
				continue;
			else
				break;
		}
		line++;
	}
	return false;
}

static bool IsDeclarationLine(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (style == SCE_BAAN_WORD) {
			if (styler.Match(i, "table") || styler.Match(i, "extern") || styler.Match(i, "long")
				|| styler.Match(i, "double") || styler.Match(i, "boolean") || styler.Match(i, "string")
				|| styler.Match(i, "domain")) {
				for (Sci_Position j = eol_pos; j > pos; j--) {
					int styleFromEnd = styler.StyleAt(j);
					if (styleFromEnd == SCE_BAAN_COMMENT)
						continue;
					else if (IsASpace(styler[j]))
						continue;
					else if (styler[j] != ',')
						//Above conditions ensures, Declaration is not part of any function parameters.
						return true;
					else
						return false;
				}
			}
			else
				return false;
		}
		else if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

static bool IsInnerLevelFold(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (style == SCE_BAAN_WORD && (styler.Match(i, "else" ) || styler.Match(i, "case")
			|| styler.Match(i, "default") || styler.Match(i, "selectdo") || styler.Match(i, "selecteos")
			|| styler.Match(i, "selectempty") || styler.Match(i, "selecterror")))
			return true;
		else if (IsASpaceOrTab(ch))
			continue;
		else
			return false;
	}
	return false;
}

static inline bool wordInArray(const std::string& value, std::string *array, int length)
{
	for (int i = 0; i < length; i++)
	{
		if (value == array[i])
		{
			return true;
		}
	}

	return false;
}

class WordListAbridged : public WordList {
public:
	WordListAbridged() {
		kwAbridged = false;
		kwHasSection = false;
	};
	~WordListAbridged() {
		Clear();
	};
	bool kwAbridged;
	bool kwHasSection;
	bool Contains(const char *s) {
		return kwAbridged ? InListAbridged(s, '~') : InList(s);
	};
};

}

class LexerBaan : public DefaultLexer {
	WordListAbridged keywords;
	WordListAbridged keywords2;
	WordListAbridged keywords3;
	WordListAbridged keywords4;
	WordListAbridged keywords5;
	WordListAbridged keywords6;
	WordListAbridged keywords7;
	WordListAbridged keywords8;
	WordListAbridged keywords9;
	OptionsBaan options;
	OptionSetBaan osBaan;
public:
	LexerBaan() : DefaultLexer("baan", SCLEX_BAAN) {
	}

	virtual ~LexerBaan() {
	}

	int SCI_METHOD Version() const override {
		return lvRelease5;
	}

	void SCI_METHOD Release() override {
		delete this;
	}

	const char * SCI_METHOD PropertyNames() override {
		return osBaan.PropertyNames();
	}

	int SCI_METHOD PropertyType(const char * name) override {
		return osBaan.PropertyType(name);
	}

	const char * SCI_METHOD DescribeProperty(const char * name) override {
		return osBaan.DescribeProperty(name);
	}

	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;

	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osBaan.PropertyGet(key);
	}

	const char * SCI_METHOD DescribeWordListSets() override {
		return osBaan.DescribeWordListSets();
	}

	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return NULL;
	}

	static ILexer5 * LexerFactoryBaan() {
		return new LexerBaan();
	}
};

Sci_Position SCI_METHOD LexerBaan::PropertySet(const char *key, const char *val) {
	if (osBaan.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerBaan::WordListSet(int n, const char *wl) {
	WordListAbridged *WordListAbridgedN = 0;
	switch (n) {
	case 0:
		WordListAbridgedN = &keywords;
		break;
	case 1:
		WordListAbridgedN = &keywords2;
		break;
	case 2:
		WordListAbridgedN = &keywords3;
		break;
	case 3:
		WordListAbridgedN = &keywords4;
		break;
	case 4:
		WordListAbridgedN = &keywords5;
		break;
	case 5:
		WordListAbridgedN = &keywords6;
		break;
	case 6:
		WordListAbridgedN = &keywords7;
		break;
	case 7:
		WordListAbridgedN = &keywords8;
		break;
	case 8:
		WordListAbridgedN = &keywords9;
		break;
	}
	Sci_Position firstModification = -1;
	if (WordListAbridgedN) {
		WordListAbridged wlNew;
		wlNew.Set(wl);
		if (*WordListAbridgedN != wlNew) {
			WordListAbridgedN->Set(wl);
			WordListAbridgedN->kwAbridged = strchr(wl, '~') != NULL;
			WordListAbridgedN->kwHasSection = strchr(wl, ':') != NULL;

			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerBaan::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (initStyle == SCE_BAAN_STRINGEOL)	// Does not leak onto next line
		initStyle = SCE_BAAN_DEFAULT;

	int visibleChars = 0;
	bool lineHasDomain = false;
	bool lineHasFunction = false;
	bool lineHasPreProc = false;
	bool lineIgnoreString = false;
	bool lineHasDefines = false;
	bool numberIsHex = false;
	char word[1000];
	int wordlen = 0;

	std::string preProcessorTags[13] = { "#context_off", "#context_on",
		"#define", "#elif", "#else", "#endif",
		"#ident", "#if", "#ifdef", "#ifndef",
		"#include", "#pragma", "#undef" };
	LexAccessor styler(pAccess);
	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_BAAN_OPERATOR:
			sc.SetState(SCE_BAAN_DEFAULT);
			break;
		case SCE_BAAN_NUMBER:
			if (IsASpaceOrTab(sc.ch) || sc.ch == '\r' || sc.ch == '\n' || IsAnOperator(sc.ch)) {
				sc.SetState(SCE_BAAN_DEFAULT);
			}
			else if ((numberIsHex && !(MakeLowerCase(sc.ch) == 'x' || MakeLowerCase(sc.ch) == 'e' ||
				IsADigit(sc.ch, 16) || sc.ch == '.' || sc.ch == '-' || sc.ch == '+')) ||
				(!numberIsHex && !(MakeLowerCase(sc.ch) == 'e' || IsADigit(sc.ch)
				|| sc.ch == '.' || sc.ch == '-' || sc.ch == '+'))) {
					// check '-' for possible -10e-5. Add '+' as well.
					numberIsHex = false;
					sc.ChangeState(SCE_BAAN_IDENTIFIER);
					sc.SetState(SCE_BAAN_DEFAULT);
			}
			break;
		case SCE_BAAN_IDENTIFIER:
			if (!IsAWordChar(sc.ch)) {
				char s[1000];
				char s1[1000];
				sc.GetCurrentLowered(s, sizeof(s));
				if (sc.ch == ':') {
					memcpy(s1, s, sizeof(s));
					s1[sc.LengthCurrent()] = sc.ch;
					s1[sc.LengthCurrent() + 1] = '\0';
				}
				if ((keywords.kwHasSection && (sc.ch == ':')) ? keywords.Contains(s1) : keywords.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD);
					if (0 == strcmp(s, "domain")) {
						lineHasDomain = true;
					}
					else if (0 == strcmp(s, "function")) {
						lineHasFunction = true;
					}
				}
				else if (lineHasDomain) {
					sc.ChangeState(SCE_BAAN_DOMDEF);
					lineHasDomain = false;
				}
				else if (lineHasFunction) {
					sc.ChangeState(SCE_BAAN_FUNCDEF);
					lineHasFunction = false;
				}
				else if ((keywords2.kwHasSection && (sc.ch == ':')) ? keywords2.Contains(s1) : keywords2.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD2);
				}
				else if ((keywords3.kwHasSection && (sc.ch == ':')) ? keywords3.Contains(s1) : keywords3.Contains(s)) {
					if (sc.ch == '(')
						sc.ChangeState(SCE_BAAN_WORD3);
					else
						sc.ChangeState(SCE_BAAN_IDENTIFIER);
				}
				else if ((keywords4.kwHasSection && (sc.ch == ':')) ? keywords4.Contains(s1) : keywords4.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD4);
				}
				else if ((keywords5.kwHasSection && (sc.ch == ':')) ? keywords5.Contains(s1) : keywords5.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD5);
				}
				else if ((keywords6.kwHasSection && (sc.ch == ':')) ? keywords6.Contains(s1) : keywords6.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD6);
				}
				else if ((keywords7.kwHasSection && (sc.ch == ':')) ? keywords7.Contains(s1) : keywords7.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD7);
				}
				else if ((keywords8.kwHasSection && (sc.ch == ':')) ? keywords8.Contains(s1) : keywords8.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD8);
				}
				else if ((keywords9.kwHasSection && (sc.ch == ':')) ? keywords9.Contains(s1) : keywords9.Contains(s)) {
					sc.ChangeState(SCE_BAAN_WORD9);
				}
				else if (lineHasPreProc) {
					sc.ChangeState(SCE_BAAN_OBJECTDEF);
					lineHasPreProc = false;
				}
				else if (lineHasDefines) {
					sc.ChangeState(SCE_BAAN_DEFINEDEF);
					lineHasDefines = false;
				}
				else {
					int state = IsAnyOtherIdentifier(s, sc.LengthCurrent());
					if (state > 0) {
						sc.ChangeState(state);
					}
				}
				sc.SetState(SCE_BAAN_DEFAULT);
			}
			break;
		case SCE_BAAN_PREPROCESSOR:
			if (options.baanStylingWithinPreprocessor) {
				if (IsASpace(sc.ch) || IsAnOperator(sc.ch)) {
					sc.SetState(SCE_BAAN_DEFAULT);
				}
			}
			else {
				if (sc.atLineEnd && (sc.chNext != '^')) {
					sc.SetState(SCE_BAAN_DEFAULT);
				}
			}
			break;
		case SCE_BAAN_COMMENT:
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_BAAN_DEFAULT);
			}
			break;
		case SCE_BAAN_COMMENTDOC:
			if (sc.MatchIgnoreCase("enddllusage")) {
				for (unsigned int i = 0; i < 10; i++) {
					sc.Forward();
				}
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
			}
			else if (sc.MatchIgnoreCase("endfunctionusage")) {
				for (unsigned int i = 0; i < 15; i++) {
					sc.Forward();
				}
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
			}
			break;
		case SCE_BAAN_STRING:
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
			}
			else if ((sc.atLineEnd) && (sc.chNext != '^')) {
				sc.ChangeState(SCE_BAAN_STRINGEOL);
				sc.ForwardSetState(SCE_BAAN_DEFAULT);
				visibleChars = 0;
			}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_BAAN_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))
				|| ((sc.ch == '-' || sc.ch == '+') && (IsADigit(sc.chNext) || sc.chNext == '.'))
				|| (MakeLowerCase(sc.ch) == 'e' && (IsADigit(sc.chNext) || sc.chNext == '+' || sc.chNext == '-'))) {
				if ((sc.ch == '0' && MakeLowerCase(sc.chNext) == 'x') ||
					((sc.ch == '-' || sc.ch == '+') && sc.chNext == '0' && MakeLowerCase(sc.GetRelativeCharacter(2)) == 'x')){
					numberIsHex = true;
				}
				sc.SetState(SCE_BAAN_NUMBER);
			}
			else if (sc.MatchIgnoreCase("dllusage") || sc.MatchIgnoreCase("functionusage")) {
				sc.SetState(SCE_BAAN_COMMENTDOC);
				do {
					sc.Forward();
				} while ((!sc.atLineEnd) && sc.More());
			}
			else if (iswordstart(sc.ch)) {
				sc.SetState(SCE_BAAN_IDENTIFIER);
			}
			else if (sc.Match('|')) {
				sc.SetState(SCE_BAAN_COMMENT);
			}
			else if (sc.ch == '\"' && !(lineIgnoreString)) {
				sc.SetState(SCE_BAAN_STRING);
			}
			else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_BAAN_PREPROCESSOR);
				word[0] = '\0';
				wordlen = 0;
				while (sc.More() && !(IsASpace(sc.chNext) || IsAnOperator(sc.chNext))) {
					sc.Forward();
					wordlen++;
				}
				sc.GetCurrentLowered(word, sizeof(word));
				if (!sc.atLineEnd) {
					word[wordlen++] = sc.ch;
					word[wordlen++] = '\0';
				}
				if (!wordInArray(word, preProcessorTags, 13))
					// Colorise only preprocessor built in Baan.
					sc.ChangeState(SCE_BAAN_IDENTIFIER);
				if (strcmp(word, "#pragma") == 0 || strcmp(word, "#include") == 0) {
					lineHasPreProc = true;
					lineIgnoreString = true;
				}
				else if (strcmp(word, "#define") == 0 || strcmp(word, "#undef") == 0 ||
					strcmp(word, "#ifdef") == 0 || strcmp(word, "#if") == 0 || strcmp(word, "#ifndef") == 0) {
					lineHasDefines = true;
					lineIgnoreString = false;
				}
			}
			else if (IsAnOperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_BAAN_OPERATOR);
			}
		}

		if (sc.atLineEnd) {
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
			lineHasDomain = false;
			lineHasFunction = false;
			lineHasPreProc = false;
			lineIgnoreString = false;
			lineHasDefines = false;
			numberIsHex = false;
		}
		if (!IsASpace(sc.ch)) {
			visibleChars++;
		}
	}
	sc.Complete();
}

void SCI_METHOD LexerBaan::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	if (!options.fold)
		return;

	char word[100];
	int wordlen = 0;
	bool foldStart = true;
	bool foldNextSelect = true;
	bool afterFunctionSection = false;
	bool beforeDeclarationSection = false;
	int currLineStyle = 0;
	int nextLineStyle = 0;

	std::string startTags[6] = { "for", "if", "on", "repeat", "select", "while" };
	std::string endTags[6] = { "endcase", "endfor", "endif", "endselect", "endwhile", "until" };
	std::string selectCloseTags[5] = { "selectdo", "selecteos", "selectempty", "selecterror", "endselect" };

	LexAccessor styler(pAccess);
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	// Backtrack to previous line in case need to fix its fold status
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}

	int levelPrev = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelPrev = styler.LevelAt(lineCurrent - 1) >> 16;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int style = initStyle;
	int styleNext = styler.StyleAt(startPos);

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		int stylePrev = (i) ? styler.StyleAt(i - 1) : SCE_BAAN_DEFAULT;
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		// Comment folding
		if (options.foldComment && style == SCE_BAAN_COMMENTDOC) {
			if (style != stylePrev) {
				levelCurrent++;
			}
			else if (style != styleNext) {
				levelCurrent--;
			}
		}
		if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler)) {
			if (!IsCommentLine(lineCurrent - 1, styler)
				&& IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
				&& !IsCommentLine(lineCurrent + 1, styler))
				levelCurrent--;
		}
		// PreProcessor Folding
		if (options.foldPreprocessor) {
			if (atEOL && IsPreProcLine(lineCurrent, styler)) {
				if (!IsPreProcLine(lineCurrent - 1, styler)
					&& IsPreProcLine(lineCurrent + 1, styler))
					levelCurrent++;
				else if (IsPreProcLine(lineCurrent - 1, styler)
					&& !IsPreProcLine(lineCurrent + 1, styler))
					levelCurrent--;
			}
			else if (style == SCE_BAAN_PREPROCESSOR) {
				// folds #ifdef/#if/#ifndef - they are not part of the IsPreProcLine folding.
				if (ch == '#') {
					if (styler.Match(i, "#ifdef") || styler.Match(i, "#if") || styler.Match(i, "#ifndef")
						|| styler.Match(i, "#context_on"))
						levelCurrent++;
					else if (styler.Match(i, "#endif") || styler.Match(i, "#context_off"))
						levelCurrent--;
				}
			}
		}
		//Syntax Folding
		if (options.baanFoldSyntaxBased && (style == SCE_BAAN_OPERATOR)) {
			if (ch == '{' || ch == '(') {
				levelCurrent++;
			}
			else if (ch == '}' || ch == ')') {
				levelCurrent--;
			}
		}
		//Keywords Folding
		if (options.baanFoldKeywordsBased) {
			if (atEOL && IsDeclarationLine(lineCurrent, styler)) {
				if (!IsDeclarationLine(lineCurrent - 1, styler)
					&& IsDeclarationLine(lineCurrent + 1, styler))
					levelCurrent++;
				else if (IsDeclarationLine(lineCurrent - 1, styler)
					&& !IsDeclarationLine(lineCurrent + 1, styler))
					levelCurrent--;
			}
			else if (style == SCE_BAAN_WORD) {
				word[wordlen++] = static_cast<char>(MakeLowerCase(ch));
				if (wordlen == 100) {                   // prevent overflow
					word[0] = '\0';
					wordlen = 1;
				}
				if (styleNext != SCE_BAAN_WORD) {
					word[wordlen] = '\0';
					wordlen = 0;
					if (strcmp(word, "for") == 0) {
						Sci_PositionU j = i + 1;
						while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
							j++;
						}
						if (styler.Match(j, "update")) {
							// Means this is a "for update" used by Select which is already folded.
							foldStart = false;
						}
					}
					else if (strcmp(word, "on") == 0) {
						Sci_PositionU j = i + 1;
						while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
							j++;
						}
						if (!styler.Match(j, "case")) {
							// Means this is not a "on Case" statement... could be "on" used by index.
							foldStart = false;
						}
					}
					else if (strcmp(word, "select") == 0) {
						if (foldNextSelect) {
							// Next Selects are sub-clause till reach of selectCloseTags[] array.
							foldNextSelect = false;
							foldStart = true;
						}
						else {
							foldNextSelect = false;
							foldStart = false;
						}
					}
					else if (wordInArray(word, selectCloseTags, 5)) {
						// select clause ends, next select clause can be folded.
						foldNextSelect = true;
						foldStart = true;
					}
					else {
						foldStart = true;
					}
					if (foldStart) {
						if (wordInArray(word, startTags, 6)) {
							levelCurrent++;
						}
						else if (wordInArray(word, endTags, 6)) {
							levelCurrent--;
						}
					}
				}
			}
		}
		// Fold inner level of if/select/case statements
		if (options.baanFoldInnerLevel && atEOL) {
			bool currLineInnerLevel = IsInnerLevelFold(lineCurrent, styler);
			bool nextLineInnerLevel = IsInnerLevelFold(lineCurrent + 1, styler);
			if (currLineInnerLevel && currLineInnerLevel != nextLineInnerLevel) {
				levelCurrent++;
			}
			else if (nextLineInnerLevel && nextLineInnerLevel != currLineInnerLevel) {
				levelCurrent--;
			}
		}
		// Section Foldings.
		// One way of implementing Section Foldings, as there is no END markings of sections.
		// first section ends on the previous line of next section.
		// Re-written whole folding to accomodate this.
		if (options.baanFoldSections && atEOL) {
			currLineStyle = mainOrSubSectionLine(lineCurrent, styler);
			nextLineStyle = mainOrSubSectionLine(lineCurrent + 1, styler);
			if (currLineStyle != 0 && currLineStyle != nextLineStyle) {
				if (levelCurrent < levelPrev)
					--levelPrev;
				for (Sci_Position j = styler.LineStart(lineCurrent); j < styler.LineStart(lineCurrent + 1) - 1; j++) {
					if (IsASpaceOrTab(styler[j]))
						continue;
					else if (styler.StyleAt(j) == SCE_BAAN_WORD5) {
						if (styler.Match(j, "functions:")) {
							// Means functions: is the end of MainSections.
							// Nothing to fold after this.
							afterFunctionSection = true;
							break;
						}
						else {
							afterFunctionSection = false;
							break;
						}
					}
					else {
						afterFunctionSection = false;
						break;
					}
				}
				if (!afterFunctionSection)
					levelCurrent++;
			}
			else if (nextLineStyle != 0 && currLineStyle != nextLineStyle
				&& (priorSectionIsSubSection(lineCurrent -1 ,styler)
					|| !nextSectionIsSubSection(lineCurrent + 1, styler))) {
				for (Sci_Position j = styler.LineStart(lineCurrent + 1); j < styler.LineStart(lineCurrent + 1 + 1) - 1; j++) {
					if (IsASpaceOrTab(styler[j]))
						continue;
					else if (styler.StyleAt(j) == SCE_BAAN_WORD5) {
						if (styler.Match(j, "declaration:")) {
							// Means declaration: is the start of MainSections.
							// Nothing to fold before this.
							beforeDeclarationSection = true;
							break;
						}
						else {
							beforeDeclarationSection = false;
							break;
						}
					}
					else {
						beforeDeclarationSection = false;
						break;
					}
				}
				if (!beforeDeclarationSection) {
					levelCurrent--;
					if (nextLineStyle == SCE_BAAN_WORD5 && priorSectionIsSubSection(lineCurrent-1, styler))
						// next levelCurrent--; is to unfold previous subsection fold.
						// On reaching the next main section, the previous main as well sub section ends.
						levelCurrent--;
				}
			}
		}
		if (atEOL) {
			int lev = levelPrev;
			lev |= levelCurrent << 16;
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
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

extern const LexerModule lmBaan(SCLEX_BAAN, LexerBaan::LexerFactoryBaan, "baan", baanWordLists);
