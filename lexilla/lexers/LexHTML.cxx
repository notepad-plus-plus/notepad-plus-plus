// Scintilla source code edit control
/** @file LexHTML.cxx
 ** Lexer for HTML.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
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
#include <set>
#include <functional>

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

#define SCE_HA_JS (SCE_HJA_START - SCE_HJ_START)
#define SCE_HA_VBS (SCE_HBA_START - SCE_HB_START)
#define SCE_HA_PYTHON (SCE_HPA_START - SCE_HP_START)

enum script_type { eScriptNone = 0, eScriptJS, eScriptVBS, eScriptPython, eScriptPHP, eScriptXML, eScriptSGML, eScriptSGMLblock, eScriptComment };
enum script_mode { eHtml = 0, eNonHtmlScript, eNonHtmlPreProc, eNonHtmlScriptPreProc };

constexpr bool IsAWordChar(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '.' || ch == '_';
}

constexpr bool IsAWordStart(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '_';
}

bool IsOperator(int ch) noexcept {
	if (IsAlphaNumeric(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
	        ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
	        ch == '?' || ch == '!' || ch == '.' || ch == '~')
		return true;
	return false;
}

unsigned char SafeGetUnsignedCharAt(Accessor &styler, Sci_Position position, char chDefault = ' ') {
	return styler.SafeGetCharAt(position, chDefault);
}

// Put an upper limit to bound time taken for unexpected text.
constexpr Sci_PositionU maxLengthCheck = 200;

std::string GetNextWord(Accessor &styler, Sci_PositionU start) {
	std::string ret;
	for (Sci_PositionU i = 0; i < maxLengthCheck; i++) {
		const char ch = styler.SafeGetCharAt(start + i);
		if ((i == 0) && !IsAWordStart(ch))
			break;
		if ((i > 0) && !IsAWordChar(ch))
			break;
		ret.push_back(ch);
	}
	return ret;
}

bool Contains(const std::string &s, std::string_view search) noexcept {
	return s.find(search) != std::string::npos;
}

script_type segIsScriptingIndicator(const Accessor &styler, Sci_PositionU start, Sci_PositionU end, script_type prevValue) {
	const std::string s = styler.GetRangeLowered(start, end+1);
	if (Contains(s, "vbs"))
		return eScriptVBS;
	if (Contains(s, "pyth"))
		return eScriptPython;
	// https://html.spec.whatwg.org/multipage/scripting.html#attr-script-type
	// https://mimesniff.spec.whatwg.org/#javascript-mime-type
	if (Contains(s, "javas") || Contains(s, "ecmas") || Contains(s, "module") || Contains(s, "jscr"))
		return eScriptJS;
	if (Contains(s, "php"))
		return eScriptPHP;

	const size_t xml = s.find("xml");
	if (xml != std::string::npos) {
		for (size_t t = 0; t < xml; t++) {
			if (!IsASpace(s[t])) {
				return prevValue;
			}
		}
		return eScriptXML;
	}

	return prevValue;
}

constexpr bool IsPHPScriptState(int state) noexcept {
	return (state >= SCE_HPHP_DEFAULT && state <= SCE_HPHP_OPERATOR) || (state == SCE_HPHP_COMPLEX_VARIABLE);
}

script_type ScriptOfState(int state) noexcept {
	if ((state >= SCE_HP_START) && (state <= SCE_HP_IDENTIFIER)) {
		return eScriptPython;
	} else if ((state >= SCE_HB_START && state <= SCE_HB_STRINGEOL) || (state == SCE_H_ASPAT || state == SCE_H_XCCOMMENT)) {
		return eScriptVBS;
	} else if ((state >= SCE_HJ_START) && (state <= SCE_HJ_TEMPLATELITERAL)) {
		return eScriptJS;
	} else if (IsPHPScriptState(state)) {
		return eScriptPHP;
	} else if ((state >= SCE_H_SGML_DEFAULT) && (state < SCE_H_SGML_BLOCK_DEFAULT)) {
		return eScriptSGML;
	} else if (state == SCE_H_SGML_BLOCK_DEFAULT) {
		return eScriptSGMLblock;
	} else {
		return eScriptNone;
	}
}

constexpr int statePrintForState(int state, script_mode inScriptType) noexcept {
	int StateToPrint = state;

	if (state >= SCE_HJ_START) {
		if ((state >= SCE_HP_START) && (state <= SCE_HP_IDENTIFIER)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_PYTHON);
		} else if ((state >= SCE_HB_START) && (state <= SCE_HB_STRINGEOL)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_VBS);
		} else if ((state >= SCE_HJ_START) && (state <= SCE_HJ_TEMPLATELITERAL)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_JS);
		}
	}

	return StateToPrint;
}

constexpr int stateForPrintState(int StateToPrint) noexcept {
	int state = StateToPrint;

	if ((StateToPrint >= SCE_HPA_START) && (StateToPrint <= SCE_HPA_IDENTIFIER)) {
		state = StateToPrint - SCE_HA_PYTHON;
	} else if ((StateToPrint >= SCE_HBA_START) && (StateToPrint <= SCE_HBA_STRINGEOL)) {
		state = StateToPrint - SCE_HA_VBS;
	} else if ((StateToPrint >= SCE_HJA_START) && (StateToPrint <= SCE_HJA_TEMPLATELITERAL)) {
		state = StateToPrint - SCE_HA_JS;
	}

	return state;
}

constexpr bool IsNumberChar(char ch) noexcept {
	return IsADigit(ch) || ch == '.' || ch == '-' || ch == '#';
}

constexpr bool isStringState(int state) noexcept {
	bool bResult = false;

	switch (state) {
	case SCE_HJ_DOUBLESTRING:
	case SCE_HJ_SINGLESTRING:
	case SCE_HJ_REGEX:
	case SCE_HJ_TEMPLATELITERAL:
	case SCE_HJA_DOUBLESTRING:
	case SCE_HJA_SINGLESTRING:
	case SCE_HJA_REGEX:
	case SCE_HJA_TEMPLATELITERAL:
	case SCE_HB_STRING:
	case SCE_HBA_STRING:
	case SCE_HP_STRING:
	case SCE_HP_CHARACTER:
	case SCE_HP_TRIPLE:
	case SCE_HP_TRIPLEDOUBLE:
	case SCE_HPA_STRING:
	case SCE_HPA_CHARACTER:
	case SCE_HPA_TRIPLE:
	case SCE_HPA_TRIPLEDOUBLE:
	case SCE_HPHP_HSTRING:
	case SCE_HPHP_SIMPLESTRING:
	case SCE_HPHP_HSTRING_VARIABLE:
	case SCE_HPHP_COMPLEX_VARIABLE:
		bResult = true;
		break;
	default :
		break;
	}
	return bResult;
}

constexpr bool stateAllowsTermination(int state) noexcept {
	bool allowTermination = !isStringState(state);
	if (allowTermination) {
		switch (state) {
		case SCE_HPHP_COMMENT:
		case SCE_HP_COMMENTLINE:
		case SCE_HPA_COMMENTLINE:
			allowTermination = false;
			break;
		default:
			break;
		}
	}
	return allowTermination;
}

bool isPreProcessorEndTag(int state, int ch) noexcept {
	const script_type type = ScriptOfState(state);
	if (state == SCE_H_ASP || AnyOf(type, eScriptVBS, eScriptJS, eScriptPython)) {
		return ch == '%';
	}
	if (type == eScriptPHP) {
		return ch == '%' || ch == '?';
	}
	return ch == '?';
}

// not really well done, since it's only comments that should lex the %> and <%
constexpr bool isCommentASPState(int state) noexcept {
	bool bResult = false;

	switch (state) {
	case SCE_HJ_COMMENT:
	case SCE_HJ_COMMENTLINE:
	case SCE_HJ_COMMENTDOC:
	case SCE_HB_COMMENTLINE:
	case SCE_HP_COMMENTLINE:
	case SCE_HPHP_COMMENT:
	case SCE_HPHP_COMMENTLINE:
		bResult = true;
		break;
	default :
		break;
	}
	return bResult;
}

bool classifyAttribHTML(script_mode inScriptType, Sci_PositionU start, Sci_PositionU end, const WordList &keywords, const WordClassifier &classifier, Accessor &styler, const std::string &tag) {
	int chAttr = SCE_H_ATTRIBUTEUNKNOWN;
	bool isLanguageType = false;
	if (IsNumberChar(styler[start])) {
		chAttr = SCE_H_NUMBER;
	} else {
		const std::string s = styler.GetRangeLowered(start, end+1);
		if (keywords.InList(s)) {
			chAttr = SCE_H_ATTRIBUTE;
		} else {
			int subStyle = classifier.ValueFor(s);
			if (subStyle < 0) {
				// Didn't find attribute, check for tag.attribute
				const std::string tagAttribute = tag + "." + s;
				subStyle = classifier.ValueFor(tagAttribute);
			}
			if (subStyle >= 0) {
				chAttr = subStyle;
			}
		}

		if (inScriptType == eNonHtmlScript) {
			// see https://html.spec.whatwg.org/multipage/scripting.html#script-processing-model
			if (s == "type" || s == "language") {
				isLanguageType = true;
			}
		}
	}
	if ((chAttr == SCE_H_ATTRIBUTEUNKNOWN) && !keywords)
		// No keywords -> all are known
		chAttr = SCE_H_ATTRIBUTE;
	styler.ColourTo(end, chAttr);
	return isLanguageType;
}

// https://html.spec.whatwg.org/multipage/custom-elements.html#custom-elements-core-concepts
bool isHTMLCustomElement(const std::string &tag) noexcept {
	// check valid HTML custom element name: starts with an ASCII lower alpha and contains hyphen.
	// IsUpperOrLowerCase() is used for `html.tags.case.sensitive=1`.
	if (tag.length() < 2 || !IsUpperOrLowerCase(tag[0])) {
		return false;
	}
	if (tag.find('-') == std::string::npos) {
		return false;
	}
	return true;
}

int classifyTagHTML(Sci_PositionU start, Sci_PositionU end,
                    const WordList &keywords, const WordClassifier &classifier, Accessor &styler, bool &tagDontFold,
                    bool caseSensitive, bool isXml, bool allowScripts,
                    const std::set<std::string> &nonFoldingTags,
                    std::string &tag) {
	tag.clear();
	// Copy after the '<' and stop before ' '
	for (Sci_PositionU cPos = start; cPos <= end; cPos++) {
		const char ch = styler[cPos];
		if (IsASpace(ch)) {
			break;
		}
		if ((ch != '<') && (ch != '/')) {
			tag.push_back(caseSensitive ? ch : MakeLowerCase(ch));
		}
	}
	// if the current language is XML, I can fold any tag
	// if the current language is HTML, I don't want to fold certain tags (input, meta, etc.)
	//...to find it in the list of no-container-tags
	tagDontFold = (!isXml) && (nonFoldingTags.count(tag) > 0);
	// No keywords -> all are known
	int chAttr = SCE_H_TAGUNKNOWN;
	if (!tag.empty() && (tag[0] == '!')) {
		chAttr = SCE_H_SGML_DEFAULT;
	} else if (!keywords || keywords.InList(tag)) {
		chAttr = SCE_H_TAG;
	} else if (!isXml && isHTMLCustomElement(tag)) {
		chAttr = SCE_H_TAG;
	} else {
		const int subStyle = classifier.ValueFor(tag);
		if (subStyle >= 0) {
			chAttr = subStyle;
		}
	}
	if (chAttr != SCE_H_TAGUNKNOWN) {
		styler.ColourTo(end, chAttr);
	}
	if (chAttr == SCE_H_TAG) {
		if (allowScripts && (tag == "script")) {
			// check to see if this is a self-closing tag by sniffing ahead
			bool isSelfClose = false;
			for (Sci_PositionU cPos = end; cPos <= end + maxLengthCheck; cPos++) {
				const char ch = styler.SafeGetCharAt(cPos, '\0');
				if (ch == '\0' || ch == '>')
					break;
				if (ch == '/' && styler.SafeGetCharAt(cPos + 1, '\0') == '>') {
					isSelfClose = true;
					break;
				}
			}

			// do not enter a script state if the tag self-closed
			if (!isSelfClose)
				chAttr = SCE_H_SCRIPT;
		} else if (!isXml && (tag == "comment")) {
			chAttr = SCE_H_COMMENT;
		}
	}
	return chAttr;
}

void classifyWordHTJS(Sci_PositionU start, Sci_PositionU end,
                             const WordList &keywords, const WordClassifier &classifier, const WordClassifier &classifierServer, Accessor &styler, script_mode inScriptType) {
	const std::string s = styler.GetRange(start, end+1);
	int chAttr = SCE_HJ_WORD;
	const bool wordIsNumber = IsADigit(s[0]) || ((s[0] == '.') && IsADigit(s[1]));
	if (wordIsNumber) {
		chAttr = SCE_HJ_NUMBER;
	} else if (keywords.InList(s)) {
		chAttr = SCE_HJ_KEYWORD;
	} else {
		const int subStyle = (inScriptType == eNonHtmlScript) ? classifier.ValueFor(s) : classifierServer.ValueFor(s);
		if (subStyle >= 0) {
			chAttr = subStyle;
		}
	}

	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
}

int classifyWordHTVB(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, const WordClassifier &classifier, Accessor &styler, script_mode inScriptType) {
	int chAttr = SCE_HB_IDENTIFIER;
	const bool wordIsNumber = IsADigit(styler[start]) || (styler[start] == '.');
	if (wordIsNumber) {
		chAttr = SCE_HB_NUMBER;
	} else {
		const std::string s = styler.GetRangeLowered(start, end+1);
		if (keywords.InList(s)) {
			chAttr = SCE_HB_WORD;
			if (s == "rem")
				chAttr = SCE_HB_COMMENTLINE;
		} else {
			const int subStyle = classifier.ValueFor(s);
			if (subStyle >= 0) {
				chAttr = subStyle;
			}
		}
	}
	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
	if (chAttr == SCE_HB_COMMENTLINE)
		return SCE_HB_COMMENTLINE;
	else
		return SCE_HB_DEFAULT;
}

void classifyWordHTPy(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, const WordClassifier &classifier, Accessor &styler, std::string &prevWord, script_mode inScriptType, bool isMako) {
	const bool wordIsNumber = IsADigit(styler[start]);
	const std::string s = styler.GetRange(start, end + 1);
	int chAttr = SCE_HP_IDENTIFIER;
	if (prevWord == "class")
		chAttr = SCE_HP_CLASSNAME;
	else if (prevWord == "def")
		chAttr = SCE_HP_DEFNAME;
	else if (wordIsNumber)
		chAttr = SCE_HP_NUMBER;
	else if (keywords.InList(s))
		chAttr = SCE_HP_WORD;
	else if (isMako && (s == "block"))
		chAttr = SCE_HP_WORD;
	else {
		const int subStyle = classifier.ValueFor(s);
		if (subStyle >= 0) {
			chAttr = subStyle;
		}
	}
	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
	prevWord = s;
}

// Update the word colour to default or keyword
// Called when in a PHP word
void classifyWordHTPHP(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, const WordClassifier &classifier, Accessor &styler) {
	int chAttr = SCE_HPHP_DEFAULT;
	const bool wordIsNumber = IsADigit(styler[start]) || (styler[start] == '.' && start+1 <= end && IsADigit(styler[start+1]));
	if (wordIsNumber) {
		chAttr = SCE_HPHP_NUMBER;
	} else {
		const std::string s = styler.GetRangeLowered(start, end+1);;
		if (keywords.InList(s)) {
			chAttr = SCE_HPHP_WORD;
		} else {
			const int subStyle = classifier.ValueFor(s);
			if (subStyle >= 0) {
				chAttr = subStyle;
			}
		}
	}
	styler.ColourTo(end, chAttr);
}

bool isWordHSGML(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, const Accessor &styler) {
	const std::string s = styler.GetRange(start, end + 1);
	return keywords.InList(s);
}

bool isWordCdata(Sci_PositionU start, Sci_PositionU end, const Accessor &styler) {
	const std::string s = styler.GetRange(start, end + 1);
	return s == "[CDATA[";
}

// Return the first state to reach when entering a scripting language
constexpr int StateForScript(script_type scriptLanguage) noexcept {
	int Result = SCE_HJ_START;
	switch (scriptLanguage) {
	case eScriptVBS:
		Result = SCE_HB_START;
		break;
	case eScriptPython:
		Result = SCE_HP_START;
		break;
	case eScriptPHP:
		Result = SCE_HPHP_DEFAULT;
		break;
	case eScriptXML:
		Result = SCE_H_TAGUNKNOWN;
		break;
	case eScriptSGML:
		Result = SCE_H_SGML_DEFAULT;
		break;
	case eScriptComment:
		Result = SCE_H_COMMENT;
		break;
	default :
		break;
	}
	return Result;
}

constexpr int defaultStateForSGML(script_type scriptLanguage) noexcept {
	return (scriptLanguage == eScriptSGMLblock)? SCE_H_SGML_BLOCK_DEFAULT : SCE_H_SGML_DEFAULT;
}

constexpr bool issgmlwordchar(int ch) noexcept {
	return !IsASCII(ch) ||
		(IsAlphaNumeric(ch) || ch == '.' || ch == '_' || ch == ':' || ch == '!' || ch == '#');
}

constexpr bool IsPhpWordStart(int ch) noexcept {
	return (IsUpperOrLowerCase(ch) || (ch == '_')) || (ch >= 0x7f);
}

constexpr bool IsPhpWordChar(int ch) noexcept {
	return IsADigit(ch) || IsPhpWordStart(ch);
}

constexpr bool InTagState(int state) noexcept {
	return AnyOf(state, SCE_H_TAG, SCE_H_TAGUNKNOWN, SCE_H_SCRIPT,
	       SCE_H_ATTRIBUTE, SCE_H_ATTRIBUTEUNKNOWN,
	       SCE_H_NUMBER, SCE_H_OTHER,
	       SCE_H_DOUBLESTRING, SCE_H_SINGLESTRING);
}

constexpr bool isLineEnd(int ch) noexcept {
	return ch == '\r' || ch == '\n';
}

bool isMakoBlockEnd(const int ch, const int chNext, const std::string &blockType) noexcept {
	if (blockType.empty()) {
		return ((ch == '%') && (chNext == '>'));
	} else if (InList(blockType, { "inherit", "namespace", "include", "page" })) {
		return ((ch == '/') && (chNext == '>'));
	} else if (blockType == "%") {
		if (ch == '/' && isLineEnd(chNext))
			return true;
		else
			return isLineEnd(ch);
	} else if (blockType == "{") {
		return ch == '}';
	} else {
		return (ch == '>');
	}
}

bool isDjangoBlockEnd(const int ch, const int chNext, const std::string &blockType) noexcept {
	if (blockType.empty()) {
		return false;
	} else if (blockType == "%") {
		return ((ch == '%') && (chNext == '}'));
	} else if (blockType == "{") {
		return ((ch == '}') && (chNext == '}'));
	} else {
		return false;
	}
}

class PhpNumberState {
	enum NumberBase { BASE_10 = 0, BASE_2, BASE_8, BASE_16 };
	static constexpr const char *const digitList[] = { "_0123456789", "_01", "_01234567", "_0123456789abcdefABCDEF" };

	NumberBase base = BASE_10;
	bool decimalPart = false;
	bool exponentPart = false;
	bool invalid = false;
	bool finished = false;

	bool leadingZero = false;
	bool invalidBase8 = false;

	bool betweenDigits = false;
	bool decimalChar = false;
	bool exponentChar = false;

public:
	[[nodiscard]] bool isInvalid() const noexcept { return invalid; }
	[[nodiscard]] bool isFinished() const noexcept { return finished; }

	bool init(int ch, int chPlus1, int chPlus2) noexcept {
		base = BASE_10;
		decimalPart = false;
		exponentPart = false;
		invalid = false;
		finished = false;

		leadingZero = false;
		invalidBase8 = false;

		betweenDigits = false;
		decimalChar = false;
		exponentChar = false;

		if (ch == '.' && strchr(digitList[BASE_10] + !betweenDigits, chPlus1) != nullptr) {
			decimalPart = true;
			betweenDigits = true;
		} else if (ch == '0' && (chPlus1 == 'b' || chPlus1 == 'B')) {
			base = BASE_2;
		} else if (ch == '0' && (chPlus1 == 'o' || chPlus1 == 'O')) {
			base = BASE_8;
		} else if (ch == '0' && (chPlus1 == 'x' || chPlus1 == 'X')) {
			base = BASE_16;
		} else if (strchr(digitList[BASE_10] + !betweenDigits, ch) != nullptr) {
			leadingZero = ch == '0';
			betweenDigits = true;
			check(chPlus1, chPlus2);
			if (finished && leadingZero) {
				// single zero should be base 10
				base = BASE_10;
			}
		} else {
			return false;
		}
		return true;
	}

	bool check(int ch, int chPlus1) noexcept {
		if (strchr(digitList[base] + !betweenDigits, ch) != nullptr) {
			if (leadingZero) {
				invalidBase8 = invalidBase8 || strchr(digitList[BASE_8] + !betweenDigits, ch) == nullptr;
			}

			betweenDigits = ch != '_';
			decimalChar = false;
			exponentChar = false;
		} else if (ch == '_') {
			invalid = true;

			betweenDigits = false;
			decimalChar = false;
			// exponentChar is unchanged
		} else if (base == BASE_10 && ch == '.' && (
					!(decimalPart || exponentPart) || strchr(digitList[BASE_10] + !betweenDigits, chPlus1) != nullptr)
			  ) {
			invalid = invalid || !betweenDigits || decimalPart || exponentPart;
			decimalPart = true;

			betweenDigits = false;
			decimalChar = true;
			exponentChar = false;
		} else if (base == BASE_10 && (ch == 'e' || ch == 'E')) {
			invalid = invalid || !(betweenDigits || decimalChar) || exponentPart;
			exponentPart = true;

			betweenDigits = false;
			decimalChar = false;
			exponentChar = true;
		} else if (base == BASE_10 && (ch == '-' || ch == '+') && exponentChar) {
			invalid = invalid || strchr(digitList[BASE_10] + !betweenDigits, chPlus1) == nullptr;

			betweenDigits = false;
			decimalChar = false;
			// exponentChar is unchanged
		} else if (IsPhpWordChar(ch)) {
			invalid = true;

			betweenDigits = false;
			decimalChar = false;
			exponentChar = false;
		} else {
			invalid = invalid || !(betweenDigits || decimalChar);
			finished = true;
			if (base == BASE_10 && leadingZero && !decimalPart && !exponentPart) {
				base = BASE_8;
				invalid = invalid || invalidBase8;
			}
		}
		return finished;
	}
};

constexpr bool isPHPStringState(int state) noexcept {
	return
	    (state == SCE_HPHP_HSTRING) ||
	    (state == SCE_HPHP_SIMPLESTRING) ||
	    (state == SCE_HPHP_HSTRING_VARIABLE) ||
	    (state == SCE_HPHP_COMPLEX_VARIABLE);
}

constexpr bool StyleNeedsBacktrack(int state) noexcept {
	return InTagState(state) || isPHPStringState(state);
}

enum class AllowPHP : int {
	None, // No PHP
	PHP, // <?php and <?=
	Question, // <?
};

enum class InstructionTag {
	None,
	XML,
	Open,// <? ?> short open tag
	Echo,// <?= ?> short echo tag
	PHP, // <?php ?> standard tag
};

InstructionTag segIsScriptInstruction(AllowPHP allowPHP, int state, const Accessor &styler, Sci_PositionU start, bool isXml) {
	constexpr std::string_view phpTag = "php";
	constexpr std::string_view xmlTag = "xml";
	const std::string tag = styler.GetRangeLowered(start, start + phpTag.length());
	if (allowPHP != AllowPHP::None) {
		// Require <?php or <?=
		if (tag == phpTag) {
			return InstructionTag::PHP;
		}
		if (!tag.empty() && (tag.front() == '=')) {
			return InstructionTag::Echo;
		}
	}
	if (isXml || tag == xmlTag) {
		return AnyOf(state, SCE_H_DEFAULT, SCE_H_SGML_BLOCK_DEFAULT)? InstructionTag::XML : InstructionTag::None;
	}
	return (allowPHP == AllowPHP::Question) ? InstructionTag::Open : InstructionTag::None;
}

Sci_Position FindPhpStringDelimiter(std::string &phpStringDelimiter, Sci_Position i, const Sci_Position lengthDoc, Accessor &styler, bool &isSimpleString) {
	const Sci_Position beginning = i - 1;
	bool isQuoted = false;

	while (i < lengthDoc && (styler[i] == ' ' || styler[i] == '\t'))
		i++;
	char ch = styler.SafeGetCharAt(i);
	const char chNext = styler.SafeGetCharAt(i + 1);
	phpStringDelimiter.clear();
	if (!IsPhpWordStart(ch)) {
		if ((ch == '\'' || ch == '\"') && IsPhpWordStart(chNext)) {
			isSimpleString = ch == '\'';
			isQuoted = true;
			i++;
			ch = chNext;
		} else {
			return beginning;
		}
	}
	phpStringDelimiter.push_back(ch);
	i++;
	Sci_Position j = i;
	for (; j < lengthDoc && !isLineEnd(styler[j]); j++) {
		if (!IsPhpWordChar(styler[j]) && isQuoted) {
			if (((isSimpleString && styler[j] == '\'') || (!isSimpleString && styler[j] == '\"')) && isLineEnd(styler.SafeGetCharAt(j + 1))) {
				isQuoted = false;
				j++;
				break;
			} else {
				phpStringDelimiter.clear();
				return beginning;
			}
		}
		phpStringDelimiter.push_back(styler[j]);
	}
	if (isQuoted) {
		phpStringDelimiter.clear();
		return beginning;
	}
	return j - 1;
}

// Options used for LexerHTML
struct OptionsHTML {
	int aspDefaultLanguage = eScriptJS;
	bool caseSensitive = false;
	bool allowScripts = true;
	AllowPHP allowPHPinXML = AllowPHP::Question;
	AllowPHP allowPHPinHTML = AllowPHP::Question;
	bool isMako = false;
	bool isDjango = false;
	bool allowASPinXML = true;
	bool allowASPinHTML = true;
	bool fold = false;
	bool foldHTML = false;
	bool foldHTMLPreprocessor = true;
	bool foldCompact = true;
	bool foldComment = false;
	bool foldHeredoc = false;
	bool foldXmlAtTagOpen = false;
};

const char * const htmlWordListDesc[] = {
	"HTML elements and attributes",
	"JavaScript keywords",
	"VBScript keywords",
	"Python keywords",
	"PHP keywords",
	"SGML and DTD keywords",
	nullptr,
};

const char * const phpscriptWordListDesc[] = {
	"", //Unused
	"", //Unused
	"", //Unused
	"", //Unused
	"PHP keywords",
	"", //Unused
	nullptr,
};

struct OptionSetHTML : public OptionSet<OptionsHTML> {
	explicit OptionSetHTML(bool isPHPScript_) {

		DefineProperty("asp.default.language", &OptionsHTML::aspDefaultLanguage,
			"Script in ASP code is initially assumed to be in JavaScript. "
			"To change this to VBScript set asp.default.language to 2. Python is 3.");

		DefineProperty("html.tags.case.sensitive", &OptionsHTML::caseSensitive,
			"For XML and HTML, setting this property to 1 will make tags match in a case "
			"sensitive way which is the expected behaviour for XML and XHTML.");

		DefineProperty("lexer.xml.allow.scripts", &OptionsHTML::allowScripts,
			"Set to 0 to disable scripts in XML.");

		DefineProperty("lexer.xml.allow.php", &OptionsHTML::allowPHPinXML,
			"Set to 0 to disable PHP in XML, 1 to accept <?php and <?=, 2 to also accept <?."
			"The default is 2.");

		DefineProperty("lexer.html.allow.php", &OptionsHTML::allowPHPinHTML,
			"Set to 0 to disable PHP in HTML, 1 to accept <?php and <?=, 2 to also accept <?."
			"The default is 2.");

		DefineProperty("lexer.html.mako", &OptionsHTML::isMako,
			"Set to 1 to enable the mako template language.");

		DefineProperty("lexer.html.django", &OptionsHTML::isDjango,
			"Set to 1 to enable the django template language.");

		DefineProperty("lexer.xml.allow.asp", &OptionsHTML::allowASPinXML,
			"Set to 0 to disable ASP in XML.");

		DefineProperty("lexer.html.allow.asp", &OptionsHTML::allowASPinHTML,
			"Set to 0 to disable ASP in HTML.");

		DefineProperty("fold", &OptionsHTML::fold);

		DefineProperty("fold.html", &OptionsHTML::foldHTML,
			"Folding is turned on or off for HTML and XML files with this option. "
			"The fold option must also be on for folding to occur.");

		DefineProperty("fold.html.preprocessor", &OptionsHTML::foldHTMLPreprocessor,
			"Folding is turned on or off for scripts embedded in HTML files with this option. "
			"The default is on.");

		DefineProperty("fold.compact", &OptionsHTML::foldCompact);

		DefineProperty("fold.hypertext.comment", &OptionsHTML::foldComment,
			"Allow folding for comments in scripts embedded in HTML. "
			"The default is off.");

		DefineProperty("fold.hypertext.heredoc", &OptionsHTML::foldHeredoc,
			"Allow folding for heredocs in scripts embedded in HTML. "
			"The default is off.");

		DefineProperty("fold.xml.at.tag.open", &OptionsHTML::foldXmlAtTagOpen,
			"Enable folding for XML at the start of open tag. "
			"The default is off.");

		DefineWordListSets(isPHPScript_ ? phpscriptWordListDesc : htmlWordListDesc);
	}
};

constexpr char styleSubable[] = { SCE_H_TAG, SCE_H_ATTRIBUTE, SCE_HJ_WORD, SCE_HJA_WORD, SCE_HB_WORD, SCE_HP_WORD, SCE_HPHP_WORD, 0 };
// Allow normal styles to be contiguous using 0x80 to 0xBF by assigning sub-styles from 0xC0 to 0xFF
constexpr int SubStylesHTML = 0xC0;

const LexicalClass lexicalClassesHTML[] = {
	// Lexer HTML SCLEX_HTML SCE_H_ SCE_HJ_ SCE_HJA_ SCE_HB_ SCE_HBA_ SCE_HP_ SCE_HPHP_ SCE_HPA_:
	0, "SCE_H_DEFAULT", "default", "Text",
	1, "SCE_H_TAG", "tag", "Tags",
	2, "SCE_H_ERRORTAGUNKNOWN", "error tag", "Unknown Tags",
	3, "SCE_H_ATTRIBUTE", "attribute", "Attributes",
	4, "SCE_H_ATTRIBUTEUNKNOWN", "error attribute", "Unknown Attributes",
	5, "SCE_H_NUMBER", "literal numeric", "Numbers",
	6, "SCE_H_DOUBLESTRING", "literal string", "Double quoted strings",
	7, "SCE_H_SINGLESTRING", "literal string", "Single quoted strings",
	8, "SCE_H_OTHER", "tag operator", "Other inside tag, including space and '='",
	9, "SCE_H_COMMENT", "comment", "Comment",
	10, "SCE_H_ENTITY", "literal", "Entities",
	11, "SCE_H_TAGEND", "tag", "XML style tag ends '/>'",
	12, "SCE_H_XMLSTART", "identifier", "XML identifier start '<?'",
	13, "SCE_H_XMLEND", "identifier", "XML identifier end '?>'",
	14, "SCE_H_SCRIPT", "error", "Internal state which should never be visible",
	15, "SCE_H_ASP", "preprocessor", "ASP <% ... %>",
	16, "SCE_H_ASPAT", "preprocessor", "ASP <% ... %>",
	17, "SCE_H_CDATA", "literal", "CDATA",
	18, "SCE_H_QUESTION", "preprocessor", "PHP",
	19, "SCE_H_VALUE", "literal string", "Unquoted values",
	20, "SCE_H_XCCOMMENT", "comment", "ASP.NET, JSP Comment <%-- ... --%>",
	21, "SCE_H_SGML_DEFAULT", "default", "SGML tags <! ... >",
	22, "SCE_H_SGML_COMMAND", "preprocessor", "SGML command",
	23, "SCE_H_SGML_1ST_PARAM", "preprocessor", "SGML 1st param",
	24, "SCE_H_SGML_DOUBLESTRING", "literal string", "SGML double string",
	25, "SCE_H_SGML_SIMPLESTRING", "literal string", "SGML single string",
	26, "SCE_H_SGML_ERROR", "error", "SGML error",
	27, "SCE_H_SGML_SPECIAL", "literal", "SGML special (#XXXX type)",
	28, "SCE_H_SGML_ENTITY", "literal", "SGML entity",
	29, "SCE_H_SGML_COMMENT", "comment", "SGML comment",
	30, "SCE_H_SGML_1ST_PARAM_COMMENT", "error comment", "SGML first parameter - lexer internal. It is an error if any text is in this style.",
	31, "SCE_H_SGML_BLOCK_DEFAULT", "default", "SGML block",
	32, "", "predefined", "",
	33, "", "predefined", "",
	34, "", "predefined", "",
	35, "", "predefined", "",
	36, "", "predefined", "",
	37, "", "predefined", "",
	38, "", "predefined", "",
	39, "", "predefined", "",
	40, "SCE_HJ_START", "client javascript default", "JS Start - allows eol filled background to not start on same line as SCRIPT tag",
	41, "SCE_HJ_DEFAULT", "client javascript default", "JS Default",
	42, "SCE_HJ_COMMENT", "client javascript comment", "JS Comment",
	43, "SCE_HJ_COMMENTLINE", "client javascript comment line", "JS Line Comment",
	44, "SCE_HJ_COMMENTDOC", "client javascript comment documentation", "JS Doc comment",
	45, "SCE_HJ_NUMBER", "client javascript literal numeric", "JS Number",
	46, "SCE_HJ_WORD", "client javascript identifier", "JS Word",
	47, "SCE_HJ_KEYWORD", "client javascript keyword", "JS Keyword",
	48, "SCE_HJ_DOUBLESTRING", "client javascript literal string", "JS Double quoted string",
	49, "SCE_HJ_SINGLESTRING", "client javascript literal string", "JS Single quoted string",
	50, "SCE_HJ_SYMBOLS", "client javascript operator", "JS Symbols",
	51, "SCE_HJ_STRINGEOL", "client javascript error literal string", "JavaScript EOL",
	52, "SCE_HJ_REGEX", "client javascript literal regex", "JavaScript RegEx",
	53, "SCE_HJ_TEMPLATELITERAL", "client javascript literal template", "JS Template Literal",
	54, "", "unused", "",
	55, "SCE_HJA_START", "server javascript default", "JS Start - allows eol filled background to not start on same line as SCRIPT tag",
	56, "SCE_HJA_DEFAULT", "server javascript default", "JS Default",
	57, "SCE_HJA_COMMENT", "server javascript comment", "JS Comment",
	58, "SCE_HJA_COMMENTLINE", "server javascript comment line", "JS Line Comment",
	59, "SCE_HJA_COMMENTDOC", "server javascript comment documentation", "JS Doc comment",
	60, "SCE_HJA_NUMBER", "server javascript literal numeric", "JS Number",
	61, "SCE_HJA_WORD", "server javascript identifier", "JS Word",
	62, "SCE_HJA_KEYWORD", "server javascript keyword", "JS Keyword",
	63, "SCE_HJA_DOUBLESTRING", "server javascript literal string", "JS Double quoted string",
	64, "SCE_HJA_SINGLESTRING", "server javascript literal string", "JS Single quoted string",
	65, "SCE_HJA_SYMBOLS", "server javascript operator", "JS Symbols",
	66, "SCE_HJA_STRINGEOL", "server javascript error literal string", "JavaScript EOL",
	67, "SCE_HJA_REGEX", "server javascript literal regex", "JavaScript RegEx",
	68, "SCE_HJA_TEMPLATELITERAL", "server javascript literal template", "JS Template Literal",
	69, "", "unused", "",
	70, "SCE_HB_START", "client basic default", "Start",
	71, "SCE_HB_DEFAULT", "client basic default", "Default",
	72, "SCE_HB_COMMENTLINE", "client basic comment line", "Comment",
	73, "SCE_HB_NUMBER", "client basic literal numeric", "Number",
	74, "SCE_HB_WORD", "client basic keyword", "KeyWord",
	75, "SCE_HB_STRING", "client basic literal string", "String",
	76, "SCE_HB_IDENTIFIER", "client basic identifier", "Identifier",
	77, "SCE_HB_STRINGEOL", "client basic literal string", "Unterminated string",
	78, "", "unused", "",
	79, "", "unused", "",
	80, "SCE_HBA_START", "server basic default", "Start",
	81, "SCE_HBA_DEFAULT", "server basic default", "Default",
	82, "SCE_HBA_COMMENTLINE", "server basic comment line", "Comment",
	83, "SCE_HBA_NUMBER", "server basic literal numeric", "Number",
	84, "SCE_HBA_WORD", "server basic keyword", "KeyWord",
	85, "SCE_HBA_STRING", "server basic literal string", "String",
	86, "SCE_HBA_IDENTIFIER", "server basic identifier", "Identifier",
	87, "SCE_HBA_STRINGEOL", "server basic literal string", "Unterminated string",
	88, "", "unused", "",
	89, "", "unused", "",
	90, "SCE_HP_START", "client python default", "Embedded Python",
	91, "SCE_HP_DEFAULT", "client python default", "Embedded Python",
	92, "SCE_HP_COMMENTLINE", "client python comment line", "Comment",
	93, "SCE_HP_NUMBER", "client python literal numeric", "Number",
	94, "SCE_HP_STRING", "client python literal string", "String",
	95, "SCE_HP_CHARACTER", "client python literal string character", "Single quoted string",
	96, "SCE_HP_WORD", "client python keyword", "Keyword",
	97, "SCE_HP_TRIPLE", "client python literal string", "Triple quotes",
	98, "SCE_HP_TRIPLEDOUBLE", "client python literal string", "Triple double quotes",
	99, "SCE_HP_CLASSNAME", "client python identifier", "Class name definition",
	100, "SCE_HP_DEFNAME", "client python identifier", "Function or method name definition",
	101, "SCE_HP_OPERATOR", "client python operator", "Operators",
	102, "SCE_HP_IDENTIFIER", "client python identifier", "Identifiers",
	103, "", "unused", "",
	104, "SCE_HPHP_COMPLEX_VARIABLE", "server php identifier", "PHP complex variable",
	105, "SCE_HPA_START", "server python default", "ASP Python",
	106, "SCE_HPA_DEFAULT", "server python default", "ASP Python",
	107, "SCE_HPA_COMMENTLINE", "server python comment line", "Comment",
	108, "SCE_HPA_NUMBER", "server python literal numeric", "Number",
	109, "SCE_HPA_STRING", "server python literal string", "String",
	110, "SCE_HPA_CHARACTER", "server python literal string character", "Single quoted string",
	111, "SCE_HPA_WORD", "server python keyword", "Keyword",
	112, "SCE_HPA_TRIPLE", "server python literal string", "Triple quotes",
	113, "SCE_HPA_TRIPLEDOUBLE", "server python literal string", "Triple double quotes",
	114, "SCE_HPA_CLASSNAME", "server python identifier", "Class name definition",
	115, "SCE_HPA_DEFNAME", "server python identifier", "Function or method name definition",
	116, "SCE_HPA_OPERATOR", "server python operator", "Operators",
	117, "SCE_HPA_IDENTIFIER", "server python identifier", "Identifiers",
	118, "SCE_HPHP_DEFAULT", "server php default", "Default",
	119, "SCE_HPHP_HSTRING", "server php literal string", "Double quoted String",
	120, "SCE_HPHP_SIMPLESTRING", "server php literal string", "Single quoted string",
	121, "SCE_HPHP_WORD", "server php keyword", "Keyword",
	122, "SCE_HPHP_NUMBER", "server php literal numeric", "Number",
	123, "SCE_HPHP_VARIABLE", "server php identifier", "Variable",
	124, "SCE_HPHP_COMMENT", "server php comment", "Comment",
	125, "SCE_HPHP_COMMENTLINE", "server php comment line", "One line comment",
	126, "SCE_HPHP_HSTRING_VARIABLE", "server php literal string identifier", "PHP variable in double quoted string",
	127, "SCE_HPHP_OPERATOR", "server php operator", "PHP operator",
};

const LexicalClass lexicalClassesXML[] = {
	// Lexer.Secondary XML SCLEX_XML SCE_H_:
	0, "SCE_H_DEFAULT", "default", "Default",
	1, "SCE_H_TAG", "tag", "Tags",
	2, "SCE_H_TAGUNKNOWN", "error tag", "Unknown Tags",
	3, "SCE_H_ATTRIBUTE", "attribute", "Attributes",
	4, "SCE_H_ERRORATTRIBUTEUNKNOWN", "error attribute", "Unknown Attributes",
	5, "SCE_H_NUMBER", "literal numeric", "Numbers",
	6, "SCE_H_DOUBLESTRING", "literal string", "Double quoted strings",
	7, "SCE_H_SINGLESTRING", "literal string", "Single quoted strings",
	8, "SCE_H_OTHER", "tag operator", "Other inside tag, including space and '='",
	9, "SCE_H_COMMENT", "comment", "Comment",
	10, "SCE_H_ENTITY", "literal", "Entities",
	11, "SCE_H_TAGEND", "tag", "XML style tag ends '/>'",
	12, "SCE_H_XMLSTART", "identifier", "XML identifier start '<?'",
	13, "SCE_H_XMLEND", "identifier", "XML identifier end '?>'",
	14, "", "unused", "",
	15, "", "unused", "",
	16, "", "unused", "",
	17, "SCE_H_CDATA", "literal", "CDATA",
	18, "SCE_H_QUESTION", "preprocessor", "Question",
	19, "SCE_H_VALUE", "literal string", "Unquoted Value",
	20, "", "unused", "",
	21, "SCE_H_SGML_DEFAULT", "default", "SGML tags <! ... >",
	22, "SCE_H_SGML_COMMAND", "preprocessor", "SGML command",
	23, "SCE_H_SGML_1ST_PARAM", "preprocessor", "SGML 1st param",
	24, "SCE_H_SGML_DOUBLESTRING", "literal string", "SGML double string",
	25, "SCE_H_SGML_SIMPLESTRING", "literal string", "SGML single string",
	26, "SCE_H_SGML_ERROR", "error", "SGML error",
	27, "SCE_H_SGML_SPECIAL", "literal", "SGML special (#XXXX type)",
	28, "SCE_H_SGML_ENTITY", "literal", "SGML entity",
	29, "SCE_H_SGML_COMMENT", "comment", "SGML comment",
	30, "", "unused", "",
	31, "SCE_H_SGML_BLOCK_DEFAULT", "default", "SGML block",
};

const char * const tagsThatDoNotFold[] = {
	"area",
	"base",
	"basefont",
	"br",
	"col",
	"command",
	"embed",
	"frame",
	"hr",
	"img",
	"input",
	"isindex",
	"keygen",
	"link",
	"meta",
	"param",
	"source",
	"track",
	"wbr"
};

}

class LexerHTML : public DefaultLexer {
	bool isXml;
	bool isPHPScript;
	WordList keywordsHTML;
	WordList keywordsJS;
	WordList keywordsVB;
	WordList keywordsPy;
	WordList keywordsPHP;
	WordList keywordsSGML; // SGML (DTD) keywords
	OptionsHTML options;
	OptionSetHTML osHTML;
	std::set<std::string> nonFoldingTags;
	SubStyles subStyles{styleSubable,SubStylesHTML,SubStylesAvailable,0};
public:
	explicit LexerHTML(bool isXml_, bool isPHPScript_) :
		DefaultLexer(
			isXml_ ? "xml" : (isPHPScript_ ? "phpscript" : "hypertext"),
			isXml_ ? SCLEX_XML : (isPHPScript_ ? SCLEX_PHPSCRIPT : SCLEX_HTML),
			isXml_ ?  lexicalClassesXML : lexicalClassesHTML,
			isXml_ ?  std::size(lexicalClassesXML) : std::size(lexicalClassesHTML)),
		isXml(isXml_),
		isPHPScript(isPHPScript_),
		osHTML(isPHPScript_),
		nonFoldingTags(std::begin(tagsThatDoNotFold), std::end(tagsThatDoNotFold)) {
	}
	~LexerHTML() override {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osHTML.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osHTML.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osHTML.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osHTML.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osHTML.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	// No Fold as all folding performs in Lex.

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
		const int styleBase = subStyles.BaseStyle(style);
		const bool lowerCase = AnyOf(styleBase, SCE_H_TAG, SCE_H_ATTRIBUTE, SCE_HB_WORD);
		subStyles.SetIdentifiers(style, identifiers, lowerCase);
	}
	int SCI_METHOD DistanceToSecondaryStyles() override {
		return 0;
	}
	const char *SCI_METHOD GetSubStyleBases() override {
		return styleSubable;
	}

	static ILexer5 *LexerFactoryHTML() {
		return new LexerHTML(false, false);
	}
	static ILexer5 *LexerFactoryXML() {
		return new LexerHTML(true, false);
	}
	static ILexer5 *LexerFactoryPHPScript() {
		return new LexerHTML(false, true);
	}
};

Sci_Position SCI_METHOD LexerHTML::PropertySet(const char *key, const char *val) {
	if (osHTML.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerHTML::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	bool lowerCase = false;
	switch (n) {
	case 0:
		wordListN = &keywordsHTML;
		lowerCase = true;
		break;
	case 1:
		wordListN = &keywordsJS;
		break;
	case 2:
		wordListN = &keywordsVB;
		lowerCase = true;
		break;
	case 3:
		wordListN = &keywordsPy;
		break;
	case 4:
		wordListN = &keywordsPHP;
		break;
	case 5:
		wordListN = &keywordsSGML;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl, lowerCase)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerHTML::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);

	const WordClassifier &classifierTags = subStyles.Classifier(SCE_H_TAG);
	const WordClassifier &classifierAttributes = subStyles.Classifier(SCE_H_ATTRIBUTE);
	const WordClassifier &classifierJavaScript = subStyles.Classifier(SCE_HJ_WORD);
	const WordClassifier &classifierJavaScriptServer = subStyles.Classifier(SCE_HJA_WORD);
	const WordClassifier &classifierBasic = subStyles.Classifier(SCE_HB_WORD);
	const WordClassifier &classifierPython = subStyles.Classifier(SCE_HP_WORD);
	const WordClassifier &classifierPHP = subStyles.Classifier(SCE_HPHP_WORD);

	if (isPHPScript && (startPos == 0)) {
		initStyle = SCE_HPHP_DEFAULT;
	}
	std::string lastTag;
	std::string prevWord;
	PhpNumberState phpNumber;
	std::string phpStringDelimiter;
	int StateToPrint = initStyle;
	int state = stateForPrintState(StateToPrint);
	std::string makoBlockType;
	int makoComment = 0;
	std::string djangoBlockType;
	// If inside a tag, it may be a script tag, so reread from the start of line starting tag to ensure any language tags are seen
	// PHP string can be heredoc, must find a delimiter first. Reread from beginning of line containing the string, to get the correct lineState
	if (StyleNeedsBacktrack(state)) {
		while ((startPos > 0) && (StyleNeedsBacktrack(styler.StyleIndexAt(startPos - 1)))) {
			const Sci_Position backLineStart = styler.LineStart(styler.GetLine(startPos-1));
			length += startPos - backLineStart;
			startPos = backLineStart;
		}
		if (startPos > 0) {
			state = styler.StyleIndexAt(startPos - 1);
		} else {
			state = isPHPScript ? SCE_HPHP_DEFAULT : SCE_H_DEFAULT;
		}
	}
	styler.StartAt(startPos);

	/* Nothing handles getting out of these, so we need not start in any of them.
	 * As we're at line start and they can't span lines, we'll re-detect them anyway */
	switch (state) {
		case SCE_H_QUESTION:
		case SCE_H_XMLSTART:
		case SCE_H_XMLEND:
		case SCE_H_ASP:
			state = SCE_H_DEFAULT;
			break;
		default:
			break;
	}

	Sci_Position lineCurrent = styler.GetLine(startPos);
	int lineState;
	if (lineCurrent > 0) {
		lineState = styler.GetLineState(lineCurrent-1);
	} else {
		// Default client and ASP scripting language is JavaScript
		lineState = eScriptJS << 8;
		lineState |= options.aspDefaultLanguage << 4;
	}
	script_mode inScriptType = static_cast<script_mode>((lineState >> 0) & 0x03); // 2 bits of scripting mode

	bool tagOpened = (lineState >> 2) & 0x01; // 1 bit to know if we are in an opened tag
	bool tagClosing = (lineState >> 3) & 0x01; // 1 bit to know if we are in a closing tag
	bool tagDontFold = false; //some HTML tags should not be folded
	script_type aspScript = static_cast<script_type>((lineState >> 4) & 0x0F); // 4 bits of script name
	script_type clientScript = static_cast<script_type>((lineState >> 8) & 0x0F); // 4 bits of script name
	int beforePreProc = (lineState >> 12) & 0xFF; // 8 bits of state
	bool isLanguageType = (lineState >> 20) & 1; // type or language attribute for script tag
	int sgmlBlockLevel = (lineState >> 21);

	script_type scriptLanguage = ScriptOfState(state);
	// If eNonHtmlScript coincides with SCE_H_COMMENT, assume eScriptComment
	if (inScriptType == eNonHtmlScript && state == SCE_H_COMMENT) {
		scriptLanguage = eScriptComment;
	}
	script_type beforeLanguage = ScriptOfState(beforePreProc);
	const bool foldHTML = options.foldHTML;
	const bool fold = foldHTML && options.fold;
	const bool foldHTMLPreprocessor = foldHTML && options.foldHTMLPreprocessor;
	const bool foldCompact = options.foldCompact;
	const bool foldComment = fold && options.foldComment;
	const bool foldHeredoc = fold && options.foldHeredoc;
	const bool foldXmlAtTagOpen = isXml && fold && options.foldXmlAtTagOpen;
	const bool caseSensitive = options.caseSensitive;
	const bool allowScripts = options.allowScripts;
	const AllowPHP allowPHP = isXml ? options.allowPHPinXML : options.allowPHPinHTML;
	const bool isMako = options.isMako;
	const bool isDjango = options.isDjango;
	const bool allowASP = (isXml ? options.allowASPinXML : options.allowASPinHTML) && !isMako && !isDjango;
	const CharacterSet setHTMLWord(CharacterSet::setAlphaNum, ".-_:!#", true);
	const CharacterSet setTagContinue(CharacterSet::setAlphaNum, ".-_:!#[", true);
	const CharacterSet setAttributeContinue(CharacterSet::setAlphaNum, ".-_:!#/", true);
	// TODO: also handle + and - (except if they're part of ++ or --) and return keywords
	const CharacterSet setOKBeforeJSRE(CharacterSet::setNone, "([{=,:;!%^&*|?~> ");
	// Only allow [A-Za-z0-9.#-_:] in entities
	const CharacterSet setEntity(CharacterSet::setAlphaNum, ".#-_:");

	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	Sci_Position visibleChars = 0;
	int lineStartVisibleChars = 0;

	int chPrev = ' ';
	int ch = ' ';
	int chPrevNonWhite = ' ';
	// look back to set chPrevNonWhite properly for better regex colouring
	if (scriptLanguage == eScriptJS && startPos > 0) {
		Sci_Position back = startPos;
		int style = 0;
		while (--back) {
			style = styler.StyleIndexAt(back);
			if (style < SCE_HJ_DEFAULT || style > SCE_HJ_COMMENTDOC)
				// includes SCE_HJ_COMMENT & SCE_HJ_COMMENTLINE
				break;
		}
		if (style == SCE_HJ_SYMBOLS) {
			chPrevNonWhite = SafeGetUnsignedCharAt(styler, back);
		}
	}

	styler.StartSegment(startPos);
	const Sci_Position lengthDoc = startPos + length;
	for (Sci_Position i = startPos; i < lengthDoc; i++) {
		const int chPrev2 = chPrev;
		chPrev = ch;
		if (!IsASpace(ch) && state != SCE_HJ_COMMENT &&
			state != SCE_HJ_COMMENTLINE && state != SCE_HJ_COMMENTDOC)
			chPrevNonWhite = ch;
		ch = static_cast<unsigned char>(styler[i]);
		int chNext = SafeGetUnsignedCharAt(styler, i + 1);
		const int chNext2 = SafeGetUnsignedCharAt(styler, i + 2);

		// Handle DBCS codepages
		if (styler.IsLeadByte(static_cast<char>(ch))) {
			chPrev = ' ';
			i += 1;
			continue;
		}

		if ((!IsASpace(ch) || !foldCompact) && fold)
			visibleChars++;
		if (!IsASpace(ch))
			lineStartVisibleChars++;

		// decide what is the current state to print (depending of the script tag)
		StateToPrint = statePrintForState(state, inScriptType);

		// handle script folding
		if (fold) {
			switch (scriptLanguage) {
			case eScriptJS:
			case eScriptPHP:
				//not currently supported				case eScriptVBS:

				if (!AnyOf(state, SCE_HPHP_COMMENT, SCE_HPHP_COMMENTLINE, SCE_HJ_COMMENT, SCE_HJ_COMMENTLINE, SCE_HJ_COMMENTDOC) &&
				    !isStringState(state)) {
				//Platform::DebugPrintf("state=%d, StateToPrint=%d, initStyle=%d\n", state, StateToPrint, initStyle);
				//if ((state == SCE_HPHP_OPERATOR) || (state == SCE_HPHP_DEFAULT) || (state == SCE_HJ_SYMBOLS) || (state == SCE_HJ_START) || (state == SCE_HJ_DEFAULT)) {
					if (ch == '#') {
						Sci_Position j = i + 1;
						while ((j < lengthDoc) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
							j++;
						}
						if (styler.Match(j, "region") || styler.Match(j, "if")) {
							levelCurrent++;
						} else if (styler.Match(j, "end")) {
							levelCurrent--;
						}
					} else if ((ch == '{') || (ch == '}') || (foldComment && (ch == '/') && (chNext == '*'))) {
						levelCurrent += (((ch == '{') || (ch == '/')) ? 1 : -1);
					}
				} else if (((state == SCE_HPHP_COMMENT) || (state == SCE_HJ_COMMENT || state == SCE_HJ_COMMENTDOC)) && foldComment && (ch == '*') && (chNext == '/')) {
					levelCurrent--;
				}
				break;
			case eScriptPython:
				if (state != SCE_HP_COMMENTLINE && !isMako) {
					if ((ch == ':') && ((chNext == '\n') || (chNext == '\r' && chNext2 == '\n'))) {
						levelCurrent++;
					} else if ((ch == '\n') && !((chNext == '\r') && (chNext2 == '\n')) && (chNext != '\n')) {
						// check if the number of tabs is lower than the level
						constexpr int tabWidth = 8;
						int Findlevel = (levelCurrent & ~SC_FOLDLEVELBASE) * tabWidth;
						for (Sci_Position j = 0; Findlevel > 0; j++) {
							const char chTmp = styler.SafeGetCharAt(i + j + 1);
							if (chTmp == '\t') {
								Findlevel -= tabWidth;
							} else if (chTmp == ' ') {
								Findlevel--;
							} else {
								break;
							}
						}

						if (Findlevel > 0) {
							levelCurrent -= Findlevel / tabWidth;
							if (Findlevel % tabWidth)
								levelCurrent--;
						}
					}
				}
				break;
			default:
				break;
			}
		}

		if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
			// Trigger on CR only (Mac style) or either on LF from CR+LF (Dos/Win) or on LF alone (Unix)
			// Avoid triggering two times on Dos/Win
			// New line -> record any line state onto /next/ line
			if (fold) {
				int lev = levelPrev;
				if (visibleChars == 0)
					lev |= SC_FOLDLEVELWHITEFLAG;
				if ((levelCurrent > levelPrev) && (visibleChars > 0))
					lev |= SC_FOLDLEVELHEADERFLAG;

				styler.SetLevel(lineCurrent, lev);
				visibleChars = 0;
				levelPrev = levelCurrent;
			}
			styler.SetLineState(lineCurrent,
			                    ((inScriptType & 0x03) << 0) |
			                    ((tagOpened ? 1 : 0) << 2) |
			                    ((tagClosing ? 1 : 0) << 3) |
			                    ((aspScript & 0x0F) << 4) |
			                    ((clientScript & 0x0F) << 8) |
			                    ((beforePreProc & 0xFF) << 12) |
			                    ((isLanguageType ? 1 : 0) << 20) |
			                    (sgmlBlockLevel << 21));
			lineCurrent++;
			lineStartVisibleChars = 0;
		}

		// handle start of Mako comment line
		if (isMako && ch == '#' && chNext == '#') {
			makoComment = 1;
			state = SCE_HP_COMMENTLINE;
		}

		// handle end of Mako comment line
		else if (isMako && makoComment && (ch == '\r' || ch == '\n')) {
			makoComment = 0;
			styler.ColourTo(i - 1, StateToPrint);
			if (scriptLanguage == eScriptPython) {
				state = SCE_HP_DEFAULT;
			} else {
				state = SCE_H_DEFAULT;
			}
		}
		// Allow falling through to mako handling code if newline is going to end a block
		if (((ch == '\r' && chNext != '\n') || (ch == '\n')) &&
			(!isMako || (makoBlockType != "%"))) {
		}
		// Ignore everything in mako comment until the line ends
		else if (isMako && makoComment) {
		}

		// generic end of script processing
		else if ((inScriptType == eNonHtmlScript) && (ch == '<') && (chNext == '/')) {
			// Check if it's the end of the script tag (or any other HTML tag)
			switch (state) {
				// in these cases, you can embed HTML tags (to confirm !!!!!!!!!!!!!!!!!!!!!!)
			case SCE_H_DOUBLESTRING:
			case SCE_H_SINGLESTRING:
			case SCE_HJ_COMMENT:
			case SCE_HJ_COMMENTDOC:
			//case SCE_HJ_COMMENTLINE: // removed as this is a common thing done to hide
			// the end of script marker from some JS interpreters.
			//case SCE_HB_COMMENTLINE:
			case SCE_HBA_COMMENTLINE:
			case SCE_HJ_DOUBLESTRING:
			case SCE_HJ_SINGLESTRING:
			case SCE_HJ_REGEX:
			case SCE_HJ_TEMPLATELITERAL:
			case SCE_HB_STRING:
			case SCE_HBA_STRING:
			case SCE_HP_STRING:
			case SCE_HP_TRIPLE:
			case SCE_HP_TRIPLEDOUBLE:
			case SCE_HPHP_HSTRING:
			case SCE_HPHP_SIMPLESTRING:
			case SCE_HPHP_COMMENT:
			case SCE_HPHP_COMMENTLINE:
				break;
			default :
				// check if the closing tag is a script tag
				if (const char *tag =
						(state == SCE_HJ_COMMENTLINE || state == SCE_HB_COMMENTLINE || isXml) ? "script" :
						state == SCE_H_COMMENT ? "comment" : nullptr) {
					Sci_Position j = i + 2;
					int chr;
					do {
						chr = static_cast<int>(*tag++);
					} while (chr != 0 && chr == MakeLowerCase(styler.SafeGetCharAt(j++)));
					if (chr != 0) break;
				}
				// closing tag of the script (it's a closing HTML tag anyway)
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_TAGUNKNOWN;
				inScriptType = eHtml;
				scriptLanguage = eScriptNone;
				clientScript = eScriptJS;
				isLanguageType = false;
				i += 2;
				visibleChars += 2;
				tagClosing = true;
				if (foldXmlAtTagOpen) {
					levelCurrent--;
				}
				continue;
			}
		}

		/////////////////////////////////////
		// handle the start of PHP pre-processor = Non-HTML
		else if ((ch == '<') && (chNext == '?') && !IsPHPScriptState(state)) {
 			const InstructionTag tag = segIsScriptInstruction(allowPHP, state, styler, i + 2, isXml);
 			if (tag != InstructionTag::None) {
				beforeLanguage = scriptLanguage;
				scriptLanguage = (tag == InstructionTag::XML) ? eScriptXML : eScriptPHP;
				styler.ColourTo(i - 1, StateToPrint);
				beforePreProc = state;
				i++;
				visibleChars++;
				if (tag >= InstructionTag::Echo) {
					i += (tag == InstructionTag::Echo) ? 1 : 3;
				}
				if (scriptLanguage == eScriptXML)
					styler.ColourTo(i, SCE_H_XMLSTART);
				else
					styler.ColourTo(i, SCE_H_QUESTION);
				state = StateForScript(scriptLanguage);
				if (inScriptType == eNonHtmlScript)
					inScriptType = eNonHtmlScriptPreProc;
				else
					inScriptType = eNonHtmlPreProc;
				// Fold whole script, but not if the XML first tag (all XML-like tags in this case)
				if (foldHTMLPreprocessor && (scriptLanguage != eScriptXML)) {
					levelCurrent++;
				}
				// should be better
				ch = SafeGetUnsignedCharAt(styler, i);
				continue;
			}
		}

		// handle the start Mako template Python code
		else if (isMako && scriptLanguage == eScriptNone && ((ch == '<' && chNext == '%') ||
															 (lineStartVisibleChars == 1 && ch == '%') ||
															 (lineStartVisibleChars == 1 && ch == '/' && chNext == '%') ||
															 (ch == '$' && chNext == '{') ||
															 (ch == '<' && chNext == '/' && chNext2 == '%'))) {
			if (ch == '%' || ch == '/')
				makoBlockType = "%";
			else if (ch == '$')
				makoBlockType = "{";
			else if (chNext == '/')
				makoBlockType = GetNextWord(styler, i+3);	// Tag end: </%tag>
			else
				makoBlockType = GetNextWord(styler, i+2);	// Tag: <%tag...>
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			if (inScriptType == eNonHtmlScript)
				inScriptType = eNonHtmlScriptPreProc;
			else
				inScriptType = eNonHtmlPreProc;

			if (chNext == '/') {
				i += 2;
				visibleChars += 2;
			} else if (ch != '%') {
				i++;
				visibleChars++;
			}
			state = SCE_HP_START;
			scriptLanguage = eScriptPython;
			styler.ColourTo(i, SCE_H_ASP);
			if (ch != '%' && ch != '$' && ch != '/') {
				i += makoBlockType.length();
				visibleChars += makoBlockType.length();
				if (keywordsPy.InList(makoBlockType))
					styler.ColourTo(i, SCE_HP_WORD);
				else
					styler.ColourTo(i, SCE_H_TAGUNKNOWN);
			}

			ch = SafeGetUnsignedCharAt(styler, i);
			continue;
		}

		// handle the start/end of Django comment
		else if (isDjango && state != SCE_H_COMMENT && (ch == '{' && chNext == '#')) {
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			beforeLanguage = scriptLanguage;
			if (inScriptType == eNonHtmlScript)
				inScriptType = eNonHtmlScriptPreProc;
			else
				inScriptType = eNonHtmlPreProc;
			i += 1;
			visibleChars += 1;
			scriptLanguage = eScriptComment;
			state = SCE_H_COMMENT;
			styler.ColourTo(i, SCE_H_ASP);
			ch = SafeGetUnsignedCharAt(styler, i);
			continue;
		} else if (isDjango && state == SCE_H_COMMENT && (ch == '#' && chNext == '}')) {
			styler.ColourTo(i - 1, StateToPrint);
			i += 1;
			visibleChars += 1;
			styler.ColourTo(i, SCE_H_ASP);
			state = beforePreProc;
			if (inScriptType == eNonHtmlScriptPreProc)
				inScriptType = eNonHtmlScript;
			else
				inScriptType = eHtml;
			scriptLanguage = beforeLanguage;
			continue;
		}

		// handle the start Django template code
		else if (isDjango && scriptLanguage != eScriptPython && scriptLanguage != eScriptComment && (ch == '{' && (chNext == '%' ||  chNext == '{'))) {
			if (chNext == '%')
				djangoBlockType = "%";
			else
				djangoBlockType = "{";
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			if (inScriptType == eNonHtmlScript)
				inScriptType = eNonHtmlScriptPreProc;
			else
				inScriptType = eNonHtmlPreProc;

			i += 1;
			visibleChars += 1;
			state = SCE_HP_START;
			beforeLanguage = scriptLanguage;
			scriptLanguage = eScriptPython;
			styler.ColourTo(i, SCE_H_ASP);

			ch = SafeGetUnsignedCharAt(styler, i);
			continue;
		}

		// handle the start of ASP pre-processor = Non-HTML
		else if ((ch == '<') && (chNext == '%') && allowASP && !isCommentASPState(state) && !isPHPStringState(state)) {
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			if (inScriptType == eNonHtmlScript)
				inScriptType = eNonHtmlScriptPreProc;
			else
				inScriptType = eNonHtmlPreProc;
			// fold whole script
			if (foldHTMLPreprocessor)
				levelCurrent++;
			if (chNext2 == '@') {
				i += 2; // place as if it was the second next char treated
				visibleChars += 2;
				state = SCE_H_ASPAT;
				scriptLanguage = eScriptVBS;
			} else if ((chNext2 == '-') && (styler.SafeGetCharAt(i + 3) == '-')) {
				styler.ColourTo(i + 3, SCE_H_ASP);
				state = SCE_H_XCCOMMENT;
				scriptLanguage = eScriptVBS;
				continue;
			} else {
				if (chNext2 == '=') {
					i += 2; // place as if it was the second next char treated
					visibleChars += 2;
				} else {
					i++; // place as if it was the next char treated
					visibleChars++;
				}

				state = StateForScript(aspScript);
				scriptLanguage = aspScript;
			}
			styler.ColourTo(i, SCE_H_ASP);
			// should be better
			ch = SafeGetUnsignedCharAt(styler, i);
			continue;
		}

		/////////////////////////////////////
		// handle the start of SGML language (DTD)
		else if (AnyOf(scriptLanguage, eScriptNone, eScriptXML, eScriptSGMLblock) &&
				 (chPrev == '<') &&
				 (ch == '!') &&
				 AnyOf(state, SCE_H_DEFAULT, SCE_H_SGML_BLOCK_DEFAULT)) {
			beforePreProc = state;
			styler.ColourTo(i - 2, StateToPrint);
			if ((state != SCE_H_SGML_BLOCK_DEFAULT) && (chNext == '-') && (chNext2 == '-')) {
				state = SCE_H_COMMENT; // wait for a pending command
				styler.ColourTo(i + 2, SCE_H_COMMENT);
				i += 2; // follow styling after the --
				if (!isXml) {
					// handle empty comment: <!-->, <!--->
					// https://html.spec.whatwg.org/multipage/parsing.html#parse-error-abrupt-closing-of-empty-comment
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
					if ((chNext == '>') || (chNext == '-' && SafeGetUnsignedCharAt(styler, i + 2) == '>')) {
						if (chNext == '-') {
							i += 1;
						}
						chPrev = '-';
						ch = '-';
					}
				}
			} else if ((state != SCE_H_SGML_BLOCK_DEFAULT) && isWordCdata(i + 1, i + 7, styler)) {
				state = SCE_H_CDATA;
			} else {
				styler.ColourTo(i, SCE_H_SGML_DEFAULT); // <! is default
				beforeLanguage = scriptLanguage;
				scriptLanguage = eScriptSGML;
				if ((chNext == '-') && (chNext2 == '-')) {
					i += 2;
					state = SCE_H_SGML_COMMENT;
				} else {
					state = (chNext == '[') ? SCE_H_SGML_DEFAULT : SCE_H_SGML_COMMAND; // wait for a pending command
				}
			}
			// fold whole tag (-- when closing the tag)
			if (foldHTMLPreprocessor || state == SCE_H_COMMENT || state == SCE_H_CDATA)
				levelCurrent++;
			continue;
		}

		// handle the end of Mako Python code
		else if (isMako &&
			     ((inScriptType == eNonHtmlPreProc) || (inScriptType == eNonHtmlScriptPreProc)) &&
				 (scriptLanguage != eScriptNone) && stateAllowsTermination(state) &&
				 isMakoBlockEnd(ch, chNext, makoBlockType)) {
			if (state == SCE_H_ASPAT) {
				aspScript = segIsScriptingIndicator(styler,
				                                    styler.GetStartSegment(), i - 1, aspScript);
			}
			if (state == SCE_HP_WORD) {
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywordsPy, classifierPython, styler, prevWord, inScriptType, isMako);
			} else {
				styler.ColourTo(i - 1, StateToPrint);
			}
			if ((makoBlockType != "%") && (makoBlockType != "{") && ch != '>') {
				i++;
				visibleChars++;
			} else if ((makoBlockType == "%") && ch == '/') {
				i++;
				visibleChars++;
			}
			if ((makoBlockType != "%") || ch == '/') {
				styler.ColourTo(i, SCE_H_ASP);
			}
			state = beforePreProc;
			if (inScriptType == eNonHtmlScriptPreProc)
				inScriptType = eNonHtmlScript;
			else
				inScriptType = eHtml;
			scriptLanguage = eScriptNone;
			continue;
		}

		// handle the end of Django template code
		else if (isDjango &&
			     ((inScriptType == eNonHtmlPreProc) || (inScriptType == eNonHtmlScriptPreProc)) &&
				 (scriptLanguage != eScriptNone) && stateAllowsTermination(state) &&
				 isDjangoBlockEnd(ch, chNext, djangoBlockType)) {
			if (state == SCE_H_ASPAT) {
				aspScript = segIsScriptingIndicator(styler,
				                                    styler.GetStartSegment(), i - 1, aspScript);
			}
			if (state == SCE_HP_WORD) {
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywordsPy, classifierPython, styler, prevWord, inScriptType, isMako);
			} else {
				styler.ColourTo(i - 1, StateToPrint);
			}
			i += 1;
			visibleChars += 1;
			styler.ColourTo(i, SCE_H_ASP);
			state = beforePreProc;
			if (inScriptType == eNonHtmlScriptPreProc)
				inScriptType = eNonHtmlScript;
			else
				inScriptType = eHtml;
			scriptLanguage = beforeLanguage;
			continue;
		}

		// handle the end of a pre-processor = Non-HTML
		else if ((!isMako && !isDjango && ((inScriptType == eNonHtmlPreProc) || (inScriptType == eNonHtmlScriptPreProc)) &&
				  (((scriptLanguage != eScriptNone) && stateAllowsTermination(state))) &&
				  ((chNext == '>') && isPreProcessorEndTag(state, ch))) ||
		         ((scriptLanguage == eScriptSGML) && (ch == '>') && !AnyOf(state, SCE_H_SGML_COMMENT, SCE_H_SGML_DOUBLESTRING, SCE_H_SGML_SIMPLESTRING))) {
			if (state == SCE_H_ASPAT) {
				aspScript = segIsScriptingIndicator(styler,
				                                    styler.GetStartSegment(), i - 1, aspScript);
			}
			// Bounce out of any ASP mode
			switch (state) {
			case SCE_HJ_WORD:
				classifyWordHTJS(styler.GetStartSegment(), i - 1, keywordsJS, classifierJavaScript, classifierJavaScriptServer, styler, inScriptType);
				break;
			case SCE_HB_WORD:
				classifyWordHTVB(styler.GetStartSegment(), i - 1, keywordsVB, classifierBasic, styler, inScriptType);
				break;
			case SCE_HP_WORD:
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywordsPy, classifierPython, styler, prevWord, inScriptType, isMako);
				break;
			case SCE_HPHP_WORD:
				classifyWordHTPHP(styler.GetStartSegment(), i - 1, keywordsPHP, classifierPHP, styler);
				break;
			case SCE_H_XCCOMMENT:
				styler.ColourTo(i - 1, state);
				break;
			default :
				styler.ColourTo(i - 1, StateToPrint);
				break;
			}
			if (scriptLanguage != eScriptSGML) {
				i++;
				visibleChars++;
			}
			if (ch == '%')
				styler.ColourTo(i, SCE_H_ASP);
			else if (scriptLanguage == eScriptXML)
				styler.ColourTo(i, SCE_H_XMLEND);
			else if (scriptLanguage == eScriptSGML)
				styler.ColourTo(i, SCE_H_SGML_DEFAULT);
			else
				styler.ColourTo(i, SCE_H_QUESTION);
			state = beforePreProc;
			if (inScriptType == eNonHtmlScriptPreProc)
				inScriptType = eNonHtmlScript;
			else
				inScriptType = eHtml;
			// Unfold all scripting languages, except for XML tag
			if (foldHTMLPreprocessor && (scriptLanguage != eScriptXML)) {
				levelCurrent--;
			}
			scriptLanguage = beforeLanguage;
			continue;
		}
		/////////////////////////////////////

		switch (state) {
		case SCE_H_DEFAULT:
			if (ch == '<') {
				// in HTML, fold on tag open and unfold on tag close
				tagOpened = true;
				tagClosing = (chNext == '/');
				if (foldXmlAtTagOpen && !AnyOf(chNext, '/', '?', '!', '-', '%')) {
					levelCurrent++;
				}
				if (foldXmlAtTagOpen && chNext == '/') {
					levelCurrent--;
				}
				styler.ColourTo(i - 1, StateToPrint);
				if (chNext != '!')
					state = SCE_H_TAGUNKNOWN;
			} else if (ch == '&') {
				styler.ColourTo(i - 1, SCE_H_DEFAULT);
				state = SCE_H_ENTITY;
			}
			break;
		case SCE_H_SGML_DEFAULT:
		case SCE_H_SGML_BLOCK_DEFAULT:
//			if (scriptLanguage == eScriptSGMLblock)
//				StateToPrint = SCE_H_SGML_BLOCK_DEFAULT;

			if (ch == '\"') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SGML_DOUBLESTRING;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SGML_SIMPLESTRING;
			} else if ((ch == '-') && (chPrev == '-')) {
				if (static_cast<Sci_Position>(styler.GetStartSegment()) <= (i - 2)) {
					styler.ColourTo(i - 2, StateToPrint);
				}
				state = SCE_H_SGML_COMMENT;
			} else if (ch == '%' && IsUpperOrLowerCase(chNext)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SGML_ENTITY;
			} else if (ch == '#') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SGML_SPECIAL;
			} else if (ch == '[') {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i, SCE_H_SGML_DEFAULT);
				++sgmlBlockLevel;
				scriptLanguage = eScriptSGMLblock;
				state = SCE_H_SGML_BLOCK_DEFAULT;
			} else if (ch == ']') {
				if (sgmlBlockLevel > 0) {
					--sgmlBlockLevel;
					if (sgmlBlockLevel == 0) {
						beforePreProc = SCE_H_DEFAULT;
					}
					styler.ColourTo(i - 1, StateToPrint);
					styler.ColourTo(i, SCE_H_SGML_DEFAULT);
					scriptLanguage = eScriptSGML;
				} else {
					styler.ColourTo(i - 1, StateToPrint);
					styler.ColourTo(i, SCE_H_SGML_ERROR);
				}
				state = SCE_H_SGML_DEFAULT;
			}
			break;
		case SCE_H_SGML_COMMAND:
			if (!issgmlwordchar(ch)) {
				if (isWordHSGML(styler.GetStartSegment(), i - 1, keywordsSGML, styler)) {
					styler.ColourTo(i - 1, StateToPrint);
					state = SCE_H_SGML_1ST_PARAM;
				} else {
					state = SCE_H_SGML_ERROR;
				}
			}
			break;
		case SCE_H_SGML_1ST_PARAM:
			// wait for the beginning of the word
			if ((ch == '-') && (chPrev == '-')) {
				styler.ColourTo(i - 2, SCE_H_SGML_DEFAULT);
				state = SCE_H_SGML_1ST_PARAM_COMMENT;
			} else if (issgmlwordchar(ch)) {
				styler.ColourTo(i - 1, SCE_H_SGML_DEFAULT);
				// find the length of the word
				Sci_Position size = 1;
				while (setHTMLWord.Contains(SafeGetUnsignedCharAt(styler, i + size)))
					size++;
				styler.ColourTo(i + size - 1, StateToPrint);
				i += size - 1;
				visibleChars += size - 1;
				ch = SafeGetUnsignedCharAt(styler, i);
				state = SCE_H_SGML_DEFAULT;
				continue;
			}
			break;
		case SCE_H_SGML_ERROR:
			if ((ch == '-') && (chPrev == '-')) {
				styler.ColourTo(i - 2, StateToPrint);
				state = SCE_H_SGML_COMMENT;
			}
			break;
		case SCE_H_SGML_DOUBLESTRING:
			if (ch == '\"') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_SGML_DEFAULT;
			}
			break;
		case SCE_H_SGML_SIMPLESTRING:
			if (ch == '\'') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_SGML_DEFAULT;
			}
			break;
		case SCE_H_SGML_COMMENT:
			if ((ch == '-') && (chPrev == '-')) {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_SGML_DEFAULT;
			}
			break;
		case SCE_H_CDATA:
			if ((chPrev2 == ']') && (chPrev == ']') && (ch == '>')) {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_DEFAULT;
				levelCurrent--;
			}
			break;
		case SCE_H_COMMENT:
			if ((scriptLanguage != eScriptComment) && (chPrev2 == '-') && (chPrev == '-') && (ch == '>' || (!isXml && ch == '!' && chNext == '>'))) {
				// close HTML comment with --!>
				// https://html.spec.whatwg.org/multipage/parsing.html#parse-error-incorrectly-closed-comment
				if (ch == '!') {
					i += 1;
				}
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_DEFAULT;
				levelCurrent--;
			}
			break;
		case SCE_H_SGML_1ST_PARAM_COMMENT:
			if ((ch == '-') && (chPrev == '-')) {
				styler.ColourTo(i, SCE_H_SGML_COMMENT);
				state = SCE_H_SGML_1ST_PARAM;
			}
			break;
		case SCE_H_SGML_SPECIAL:
			if (!IsUpperCase(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				if (IsAlphaNumeric(ch)) {
					state = SCE_H_SGML_ERROR;
				} else {
					state = SCE_H_SGML_DEFAULT;
				}
			}
			break;
		case SCE_H_SGML_ENTITY:
			if (!(IsAlphaNumeric(ch)) && ch != '-' && ch != '.' && ch != '_') {
				styler.ColourTo(i, ((ch == ';') ? StateToPrint : SCE_H_SGML_ERROR));
				state = defaultStateForSGML(scriptLanguage);
			}
			break;
		case SCE_H_ENTITY:
			if (ch == ';') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_DEFAULT;
			} else if (!setEntity.Contains(ch)) {
				styler.ColourTo(i-1, SCE_H_TAGUNKNOWN);
				state = SCE_H_DEFAULT;
				if (!isLineEnd(ch)) {
					// Retreat one byte so the character that is invalid inside entity
					// may start something else like a tag.
					--i;
					continue;
				}
			}
			break;
		case SCE_H_TAGUNKNOWN:
			if (!setTagContinue.Contains(ch) && !((ch == '/') && (chPrev == '<'))) {
				int eClass = classifyTagHTML(styler.GetStartSegment(),
					i - 1, keywordsHTML, classifierTags, styler, tagDontFold, caseSensitive, isXml, allowScripts, nonFoldingTags, lastTag);
				if (eClass == SCE_H_SCRIPT || eClass == SCE_H_COMMENT) {
					if (!tagClosing) {
						inScriptType = eNonHtmlScript;
						scriptLanguage = eClass == SCE_H_SCRIPT ? clientScript : eScriptComment;
					} else {
						scriptLanguage = eScriptNone;
					}
					isLanguageType = false;
					eClass = SCE_H_TAG;
				}
				if (ch == '>') {
					styler.ColourTo(i, eClass);
					if (inScriptType == eNonHtmlScript) {
						state = StateForScript(scriptLanguage);
					} else {
						state = SCE_H_DEFAULT;
					}
					tagOpened = false;
					if (!(foldXmlAtTagOpen || tagDontFold)) {
						if (tagClosing) {
							levelCurrent--;
						} else {
							levelCurrent++;
						}
					}
					tagClosing = false;
				} else if (ch == '/' && chNext == '>') {
					if (eClass == SCE_H_TAGUNKNOWN) {
						styler.ColourTo(i + 1, SCE_H_TAGUNKNOWN);
					} else {
						styler.ColourTo(i - 1, StateToPrint);
						styler.ColourTo(i + 1, SCE_H_TAGEND);
					}
					i++;
					ch = chNext;
					state = SCE_H_DEFAULT;
					tagOpened = false;
					if (foldXmlAtTagOpen) {
						levelCurrent--;
					}
				} else {
					if (eClass != SCE_H_TAGUNKNOWN) {
						if (eClass == SCE_H_SGML_DEFAULT) {
							state = SCE_H_SGML_DEFAULT;
						} else {
							state = SCE_H_OTHER;
						}
					}
				}
			}
			break;
		case SCE_H_ATTRIBUTE:
			if (!setAttributeContinue.Contains(ch)) {
				isLanguageType = classifyAttribHTML(inScriptType, styler.GetStartSegment(), i - 1, keywordsHTML, classifierAttributes, styler, lastTag);
				if (ch == '>') {
					styler.ColourTo(i, SCE_H_TAG);
					if (inScriptType == eNonHtmlScript) {
						state = StateForScript(scriptLanguage);
					} else {
						state = SCE_H_DEFAULT;
					}
					tagOpened = false;
					if (!(foldXmlAtTagOpen || tagDontFold)) {
						if (tagClosing) {
							levelCurrent--;
						} else {
							levelCurrent++;
						}
					}
					tagClosing = false;
				} else if (ch == '=') {
					styler.ColourTo(i, SCE_H_OTHER);
					state = SCE_H_VALUE;
				} else {
					state = SCE_H_OTHER;
				}
			}
			break;
		case SCE_H_OTHER:
			if (ch == '>') {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i, SCE_H_TAG);
				if (inScriptType == eNonHtmlScript) {
					state = StateForScript(scriptLanguage);
				} else {
					state = SCE_H_DEFAULT;
				}
				tagOpened = false;
				if (!(foldXmlAtTagOpen || tagDontFold)) {
					if (tagClosing) {
						levelCurrent--;
					} else {
						levelCurrent++;
					}
				}
				tagClosing = false;
			} else if (ch == '\"') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_DOUBLESTRING;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SINGLESTRING;
			} else if (ch == '=') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_VALUE;
			} else if (ch == '/' && chNext == '>') {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i + 1, SCE_H_TAGEND);
				i++;
				ch = chNext;
				state = SCE_H_DEFAULT;
				tagOpened = false;
				if (foldXmlAtTagOpen) {
					levelCurrent--;
				}
			} else if (ch == '?' && chNext == '>') {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i + 1, SCE_H_XMLEND);
				i++;
				ch = chNext;
				state = SCE_H_DEFAULT;
			} else if (setHTMLWord.Contains(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_ATTRIBUTE;
			}
			break;
		case SCE_H_DOUBLESTRING:
			if (ch == '\"') {
				if (isLanguageType) {
					scriptLanguage = segIsScriptingIndicator(styler, styler.GetStartSegment(), i, scriptLanguage);
					clientScript = scriptLanguage;
					isLanguageType = false;
				}
				styler.ColourTo(i, SCE_H_DOUBLESTRING);
				state = SCE_H_OTHER;
			}
			break;
		case SCE_H_SINGLESTRING:
			if (ch == '\'') {
				if (isLanguageType) {
					scriptLanguage = segIsScriptingIndicator(styler, styler.GetStartSegment(), i, scriptLanguage);
					clientScript = scriptLanguage;
					isLanguageType = false;
				}
				styler.ColourTo(i, SCE_H_SINGLESTRING);
				state = SCE_H_OTHER;
			}
			break;
		case SCE_H_VALUE:
			if (!setHTMLWord.Contains(ch)) {
				if (ch == '\"' && chPrev == '=') {
					// Should really test for being first character
					state = SCE_H_DOUBLESTRING;
				} else if (ch == '\'' && chPrev == '=') {
					state = SCE_H_SINGLESTRING;
				} else {
					if (IsNumberChar(styler[styler.GetStartSegment()])) {
						styler.ColourTo(i - 1, SCE_H_NUMBER);
					} else {
						styler.ColourTo(i - 1, StateToPrint);
					}
					if (ch == '>') {
						styler.ColourTo(i, SCE_H_TAG);
						if (inScriptType == eNonHtmlScript) {
							state = StateForScript(scriptLanguage);
						} else {
							state = SCE_H_DEFAULT;
						}
						tagOpened = false;
						if (!tagDontFold) {
							if (tagClosing) {
								levelCurrent--;
							} else {
								levelCurrent++;
							}
						}
						tagClosing = false;
					} else {
						state = SCE_H_OTHER;
					}
				}
			}
			break;
		case SCE_HJ_DEFAULT:
		case SCE_HJ_START:
		case SCE_HJ_SYMBOLS:
			if (IsAWordStart(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_WORD;
			} else if (ch == '/' && chNext == '*') {
				styler.ColourTo(i - 1, StateToPrint);
				i++;
				if (chNext2 == '*')
					state = SCE_HJ_COMMENTDOC;
				else
					state = SCE_HJ_COMMENT;
			} else if (ch == '/' && chNext == '/') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_COMMENTLINE;
			} else if (ch == '/' && setOKBeforeJSRE.Contains(chPrevNonWhite)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_REGEX;
			} else if (ch == '\"') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_DOUBLESTRING;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_SINGLESTRING;
			} else if (ch == '`') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_TEMPLATELITERAL;
			} else if ((ch == '<') && (chNext == '!') && (chNext2 == '-') &&
			           styler.SafeGetCharAt(i + 3) == '-') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_COMMENTLINE;
			} else if ((ch == '-') && (chNext == '-') && (chNext2 == '>')) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_COMMENTLINE;
				i += 2;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i, statePrintForState(SCE_HJ_SYMBOLS, inScriptType));
				state = SCE_HJ_DEFAULT;
			} else if ((ch == ' ') || (ch == '\t')) {
				if (state == SCE_HJ_START) {
					styler.ColourTo(i - 1, StateToPrint);
					state = SCE_HJ_DEFAULT;
				}
			}
			break;
		case SCE_HJ_WORD:
			if (!IsAWordChar(ch)) {
				classifyWordHTJS(styler.GetStartSegment(), i - 1, keywordsJS,
					classifierJavaScript, classifierJavaScriptServer, styler, inScriptType);
				//styler.ColourTo(i - 1, eHTJSKeyword);
				state = SCE_HJ_DEFAULT;
				if (ch == '/' && chNext == '*') {
					i++;
					if (chNext2 == '*')
						state = SCE_HJ_COMMENTDOC;
					else
						state = SCE_HJ_COMMENT;
				} else if (ch == '/' && chNext == '/') {
					state = SCE_HJ_COMMENTLINE;
				} else if (ch == '\"') {
					state = SCE_HJ_DOUBLESTRING;
				} else if (ch == '\'') {
					state = SCE_HJ_SINGLESTRING;
				} else if (ch == '`') {
					state = SCE_HJ_TEMPLATELITERAL;
				} else if ((ch == '-') && (chNext == '-') && (chNext2 == '>')) {
					styler.ColourTo(i - 1, StateToPrint);
					state = SCE_HJ_COMMENTLINE;
					i += 2;
				} else if (IsOperator(ch)) {
					styler.ColourTo(i, statePrintForState(SCE_HJ_SYMBOLS, inScriptType));
					state = SCE_HJ_DEFAULT;
				}
			}
			break;
		case SCE_HJ_COMMENT:
		case SCE_HJ_COMMENTDOC:
			if (ch == '/' && chPrev == '*') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HJ_DEFAULT;
				ch = ' ';
			}
			break;
		case SCE_HJ_COMMENTLINE:
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, statePrintForState(SCE_HJ_COMMENTLINE, inScriptType));
				state = SCE_HJ_DEFAULT;
				ch = ' ';
			}
			break;
		case SCE_HJ_DOUBLESTRING:
			if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
				}
			} else if (ch == '\"') {
				styler.ColourTo(i, statePrintForState(SCE_HJ_DOUBLESTRING, inScriptType));
				state = SCE_HJ_DEFAULT;
			} else if (isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				if (chPrev != '\\' && (chPrev2 != '\\' || chPrev != '\r' || ch != '\n')) {
					state = SCE_HJ_STRINGEOL;
				}
			}
			break;
		case SCE_HJ_SINGLESTRING:
			if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
				}
			} else if (ch == '\'') {
				styler.ColourTo(i, statePrintForState(SCE_HJ_SINGLESTRING, inScriptType));
				state = SCE_HJ_DEFAULT;
			} else if (isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				if (chPrev != '\\' && (chPrev2 != '\\' || chPrev != '\r' || ch != '\n')) {
					state = SCE_HJ_STRINGEOL;
				}
			}
			break;
		case SCE_HJ_TEMPLATELITERAL:
			if (ch == '\\') {
				if (chNext == '$' || chNext == '`' || chNext == '\\') {
					i++;
				}
			} else if (ch == '`') {
				styler.ColourTo(i, statePrintForState(SCE_HJ_TEMPLATELITERAL, inScriptType));
				state = SCE_HJ_DEFAULT;
			}
			break;
		case SCE_HJ_STRINGEOL:
			if (!isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_DEFAULT;
			} else if (!isLineEnd(chNext)) {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HJ_DEFAULT;
			}
			break;
		case SCE_HJ_REGEX:
			if (ch == '\r' || ch == '\n' || ch == '/') {
				if (ch == '/') {
					while (IsLowerCase(chNext)) {   // gobble regex flags
						i++;
						ch = chNext;
						chNext = SafeGetUnsignedCharAt(styler, i + 1);
					}
				}
				styler.ColourTo(i, StateToPrint);
				state = SCE_HJ_DEFAULT;
				continue;
			} else if (ch == '\\') {
				// Gobble up the quoted character
				if (chNext == '\\' || chNext == '/') {
					i++;
					ch = chNext;
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
				}
			}
			break;
		case SCE_HB_DEFAULT:
		case SCE_HB_START:
			if (IsAWordStart(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_WORD;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_COMMENTLINE;
			} else if (ch == '\"') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_STRING;
			} else if ((ch == '<') && (chNext == '!') && (chNext2 == '-') &&
			           styler.SafeGetCharAt(i + 3) == '-') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_COMMENTLINE;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i, statePrintForState(SCE_HB_DEFAULT, inScriptType));
				state = SCE_HB_DEFAULT;
			} else if ((ch == ' ') || (ch == '\t')) {
				if (state == SCE_HB_START) {
					styler.ColourTo(i - 1, StateToPrint);
					state = SCE_HB_DEFAULT;
				}
			}
			break;
		case SCE_HB_WORD:
			if (!IsAWordChar(ch)) {
				state = classifyWordHTVB(styler.GetStartSegment(), i - 1, keywordsVB, classifierBasic, styler, inScriptType);
				if (state == SCE_HB_DEFAULT) {
					if (ch == '\"') {
						state = SCE_HB_STRING;
					} else if (ch == '\'') {
						state = SCE_HB_COMMENTLINE;
					} else if (IsOperator(ch)) {
						styler.ColourTo(i, statePrintForState(SCE_HB_DEFAULT, inScriptType));
						state = SCE_HB_DEFAULT;
					}
				}
			}
			break;
		case SCE_HB_STRING:
			if (ch == '\"') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HB_DEFAULT;
			} else if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_STRINGEOL;
			}
			break;
		case SCE_HB_COMMENTLINE:
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_DEFAULT;
			}
			break;
		case SCE_HB_STRINGEOL:
			if (!isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HB_DEFAULT;
			} else if (!isLineEnd(chNext)) {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HB_DEFAULT;
			}
			break;
		case SCE_HP_DEFAULT:
		case SCE_HP_START:
			if (IsAWordStart(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HP_WORD;
			} else if ((ch == '<') && (chNext == '!') && (chNext2 == '-') &&
			           styler.SafeGetCharAt(i + 3) == '-') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HP_COMMENTLINE;
			} else if (ch == '#') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HP_COMMENTLINE;
			} else if (ch == '\"') {
				styler.ColourTo(i - 1, StateToPrint);
				if (chNext == '\"' && chNext2 == '\"') {
					i += 2;
					state = SCE_HP_TRIPLEDOUBLE;
					ch = ' ';
					chPrev = ' ';
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
				} else {
					//					state = statePrintForState(SCE_HP_STRING,inScriptType);
					state = SCE_HP_STRING;
				}
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, StateToPrint);
				if (chNext == '\'' && chNext2 == '\'') {
					i += 2;
					state = SCE_HP_TRIPLE;
					ch = ' ';
					chPrev = ' ';
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
				} else {
					state = SCE_HP_CHARACTER;
				}
			} else if (IsOperator(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				styler.ColourTo(i, statePrintForState(SCE_HP_OPERATOR, inScriptType));
			} else if ((ch == ' ') || (ch == '\t')) {
				if (state == SCE_HP_START) {
					styler.ColourTo(i - 1, StateToPrint);
					state = SCE_HP_DEFAULT;
				}
			}
			break;
		case SCE_HP_WORD:
			if (!IsAWordChar(ch)) {
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywordsPy, classifierPython, styler, prevWord, inScriptType, isMako);
				state = SCE_HP_DEFAULT;
				if (ch == '#') {
					state = SCE_HP_COMMENTLINE;
				} else if (ch == '\"') {
					if (chNext == '\"' && chNext2 == '\"') {
						i += 2;
						state = SCE_HP_TRIPLEDOUBLE;
						ch = ' ';
						chPrev = ' ';
						chNext = SafeGetUnsignedCharAt(styler, i + 1);
					} else {
						state = SCE_HP_STRING;
					}
				} else if (ch == '\'') {
					if (chNext == '\'' && chNext2 == '\'') {
						i += 2;
						state = SCE_HP_TRIPLE;
						ch = ' ';
						chPrev = ' ';
						chNext = SafeGetUnsignedCharAt(styler, i + 1);
					} else {
						state = SCE_HP_CHARACTER;
					}
				} else if (IsOperator(ch)) {
					styler.ColourTo(i, statePrintForState(SCE_HP_OPERATOR, inScriptType));
				}
			}
			break;
		case SCE_HP_COMMENTLINE:
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HP_DEFAULT;
			}
			break;
		case SCE_HP_STRING:
			if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
					ch = chNext;
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
				}
			} else if (ch == '\"') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HP_DEFAULT;
			}
			break;
		case SCE_HP_CHARACTER:
			if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
					ch = chNext;
					chNext = SafeGetUnsignedCharAt(styler, i + 1);
				}
			} else if (ch == '\'') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HP_DEFAULT;
			}
			break;
		case SCE_HP_TRIPLE:
			if (ch == '\'' && chPrev == '\'' && chPrev2 == '\'') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HP_DEFAULT;
			}
			break;
		case SCE_HP_TRIPLEDOUBLE:
			if (ch == '\"' && chPrev == '\"' && chPrev2 == '\"') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HP_DEFAULT;
			}
			break;
			///////////// start - PHP state handling
		case SCE_HPHP_WORD:
			if (!IsPhpWordChar(ch)) {
				classifyWordHTPHP(styler.GetStartSegment(), i - 1, keywordsPHP, classifierPHP, styler);
				if (ch == '/' && chNext == '*') {
					i++;
					state = SCE_HPHP_COMMENT;
				} else if (ch == '/' && chNext == '/') {
					i++;
					state = SCE_HPHP_COMMENTLINE;
				} else if (ch == '#' && chNext != '[') {
					state = SCE_HPHP_COMMENTLINE;
				} else if (ch == '\"') {
					state = SCE_HPHP_HSTRING;
					phpStringDelimiter = "\"";
				} else if (styler.Match(i, "<<<")) {
					bool isSimpleString = false;
					i = FindPhpStringDelimiter(phpStringDelimiter, i + 3, lengthDoc, styler, isSimpleString);
					if (!phpStringDelimiter.empty()) {
						state = (isSimpleString ? SCE_HPHP_SIMPLESTRING : SCE_HPHP_HSTRING);
						if (foldHeredoc) levelCurrent++;
					}
				} else if (ch == '\'') {
					state = SCE_HPHP_SIMPLESTRING;
					phpStringDelimiter = "\'";
				} else if (ch == '$' && IsPhpWordStart(chNext)) {
					state = SCE_HPHP_VARIABLE;
				} else if (IsOperator(ch)) {
					state = SCE_HPHP_OPERATOR;
				} else {
					state = SCE_HPHP_DEFAULT;
				}
			}
			break;
		case SCE_HPHP_NUMBER:
			if (phpNumber.check(chNext, chNext2)) {
				styler.ColourTo(i, phpNumber.isInvalid() ? SCE_HPHP_DEFAULT : SCE_HPHP_NUMBER);
				state = SCE_HPHP_DEFAULT;
			}
			break;
		case SCE_HPHP_VARIABLE:
			if (!IsPhpWordChar(chNext)) {
				styler.ColourTo(i, SCE_HPHP_VARIABLE);
				state = SCE_HPHP_DEFAULT;
			}
			break;
		case SCE_HPHP_COMMENT:
			if (ch == '/' && chPrev == '*') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HPHP_DEFAULT;
			}
			break;
		case SCE_HPHP_COMMENTLINE:
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HPHP_DEFAULT;
			}
			break;
		case SCE_HPHP_HSTRING:
			if (ch == '\\' && ((phpStringDelimiter == "\"") || chNext == '$' || chNext == '{')) {
				// skip the next char
				i++;
			} else if (((ch == '{' && chNext == '$') || (ch == '$' && chNext == '{'))
				&& IsPhpWordStart(chNext2)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HPHP_COMPLEX_VARIABLE;
			} else if (ch == '$' && IsPhpWordStart(chNext)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HPHP_HSTRING_VARIABLE;
			} else if (styler.Match(i, phpStringDelimiter.c_str())) {
				if (phpStringDelimiter == "\"") {
					styler.ColourTo(i, StateToPrint);
					state = SCE_HPHP_DEFAULT;
				} else if (lineStartVisibleChars == 1) {
					const int psdLength = static_cast<int>(phpStringDelimiter.length());
					if (!IsPhpWordChar(styler.SafeGetCharAt(i + psdLength))) {
						i += (((i + psdLength) < lengthDoc) ? psdLength : lengthDoc) - 1;
						styler.ColourTo(i, StateToPrint);
						state = SCE_HPHP_DEFAULT;
						if (foldHeredoc) levelCurrent--;
					}
				}
			}
			break;
		case SCE_HPHP_SIMPLESTRING:
			if (phpStringDelimiter == "\'") {
				if (ch == '\\') {
					// skip the next char
					i++;
				} else if (ch == '\'') {
					styler.ColourTo(i, StateToPrint);
					state = SCE_HPHP_DEFAULT;
				}
			} else if (lineStartVisibleChars == 1 && styler.Match(i, phpStringDelimiter.c_str())) {
				const int psdLength = static_cast<int>(phpStringDelimiter.length());
				if (!IsPhpWordChar(styler.SafeGetCharAt(i + psdLength))) {
					i += (((i + psdLength) < lengthDoc) ? psdLength : lengthDoc) - 1;
					styler.ColourTo(i, StateToPrint);
					state = SCE_HPHP_DEFAULT;
					if (foldHeredoc) levelCurrent--;
				}
			}
			break;
		case SCE_HPHP_HSTRING_VARIABLE:
			if (!IsPhpWordChar(chNext)) {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HPHP_HSTRING;
			}
			break;
		case SCE_HPHP_COMPLEX_VARIABLE:
			if (ch == '}') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_HPHP_HSTRING;
			}
			break;
		case SCE_HPHP_OPERATOR:
		case SCE_HPHP_DEFAULT:
			styler.ColourTo(i - 1, StateToPrint);
			if (phpNumber.init(ch, chNext, chNext2)) {
				if (phpNumber.isFinished()) {
					styler.ColourTo(i, phpNumber.isInvalid() ? SCE_HPHP_DEFAULT : SCE_HPHP_NUMBER);
					state = SCE_HPHP_DEFAULT;
				} else {
					state = SCE_HPHP_NUMBER;
				}
			} else if (IsAWordStart(ch)) {
				state = SCE_HPHP_WORD;
			} else if (ch == '/' && chNext == '*') {
				i++;
				state = SCE_HPHP_COMMENT;
			} else if (ch == '/' && chNext == '/') {
				i++;
				state = SCE_HPHP_COMMENTLINE;
			} else if (ch == '#' && chNext != '[') {
				state = SCE_HPHP_COMMENTLINE;
			} else if (ch == '\"') {
				state = SCE_HPHP_HSTRING;
				phpStringDelimiter = "\"";
			} else if (styler.Match(i, "<<<")) {
				bool isSimpleString = false;
				i = FindPhpStringDelimiter(phpStringDelimiter, i + 3, lengthDoc, styler, isSimpleString);
				if (!phpStringDelimiter.empty()) {
					state = (isSimpleString ? SCE_HPHP_SIMPLESTRING : SCE_HPHP_HSTRING);
					if (foldHeredoc) levelCurrent++;
				}
			} else if (ch == '\'') {
				state = SCE_HPHP_SIMPLESTRING;
				phpStringDelimiter = "\'";
			} else if (ch == '$' && IsPhpWordStart(chNext)) {
				state = SCE_HPHP_VARIABLE;
			} else if (IsOperator(ch)) {
				state = SCE_HPHP_OPERATOR;
			} else if ((state == SCE_HPHP_OPERATOR) && (IsASpace(ch))) {
				state = SCE_HPHP_DEFAULT;
			}
			break;
			///////////// end - PHP state handling
		}

		// Some of the above terminated their lexeme but since the same character starts
		// the same class again, only reenter if non empty segment.

		const bool nonEmptySegment = i >= static_cast<Sci_Position>(styler.GetStartSegment());
		if (state == SCE_HB_DEFAULT) {    // One of the above succeeded
			if ((ch == '\"') && (nonEmptySegment)) {
				state = SCE_HB_STRING;
			} else if (ch == '\'') {
				state = SCE_HB_COMMENTLINE;
			} else if (IsAWordStart(ch)) {
				state = SCE_HB_WORD;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i, SCE_HB_DEFAULT);
			}
		} else if (state == SCE_HBA_DEFAULT) {    // One of the above succeeded
			if ((ch == '\"') && (nonEmptySegment)) {
				state = SCE_HBA_STRING;
			} else if (ch == '\'') {
				state = SCE_HBA_COMMENTLINE;
			} else if (IsAWordStart(ch)) {
				state = SCE_HBA_WORD;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i, SCE_HBA_DEFAULT);
			}
		} else if (state == SCE_HJ_DEFAULT) {    // One of the above succeeded
			if (ch == '/' && chNext == '*') {
				i++;
				if (styler.SafeGetCharAt(i + 1) == '*')
					state = SCE_HJ_COMMENTDOC;
				else
					state = SCE_HJ_COMMENT;
			} else if (ch == '/' && chNext == '/') {
				state = SCE_HJ_COMMENTLINE;
			} else if ((ch == '\"') && (nonEmptySegment)) {
				state = SCE_HJ_DOUBLESTRING;
			} else if ((ch == '\'') && (nonEmptySegment)) {
				state = SCE_HJ_SINGLESTRING;
			} else if ((ch == '`') && (nonEmptySegment)) {
				state = SCE_HJ_TEMPLATELITERAL;
			} else if (IsAWordStart(ch)) {
				state = SCE_HJ_WORD;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i, statePrintForState(SCE_HJ_SYMBOLS, inScriptType));
			}
		}
	}

	switch (state) {
	case SCE_HJ_WORD:
		classifyWordHTJS(styler.GetStartSegment(), lengthDoc - 1, keywordsJS,
			classifierJavaScript, classifierJavaScriptServer, styler, inScriptType);
		break;
	case SCE_HB_WORD:
		classifyWordHTVB(styler.GetStartSegment(), lengthDoc - 1, keywordsVB, classifierBasic, styler, inScriptType);
		break;
	case SCE_HP_WORD:
		classifyWordHTPy(styler.GetStartSegment(), lengthDoc - 1, keywordsPy, classifierPython, styler, prevWord, inScriptType, isMako);
		break;
	case SCE_HPHP_WORD:
		classifyWordHTPHP(styler.GetStartSegment(), lengthDoc - 1, keywordsPHP, classifierPHP, styler);
		break;
	default:
		StateToPrint = statePrintForState(state, inScriptType);
		if (static_cast<Sci_Position>(styler.GetStartSegment()) < lengthDoc)
			styler.ColourTo(lengthDoc - 1, StateToPrint);
		break;
	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	if (fold) {
		const int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
		styler.SetLevel(lineCurrent, levelPrev | flagsNext);
	}
	styler.Flush();
}

extern const LexerModule lmHTML(SCLEX_HTML, LexerHTML::LexerFactoryHTML, "hypertext", htmlWordListDesc);
extern const LexerModule lmXML(SCLEX_XML, LexerHTML::LexerFactoryXML, "xml", htmlWordListDesc);
extern const LexerModule lmPHPSCRIPT(SCLEX_PHPSCRIPT, LexerHTML::LexerFactoryPHPScript, "phpscript", phpscriptWordListDesc);
