// Scintilla source code edit control
/** @file LexHTML.cxx
 ** Lexer for HTML.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string>
#include <map>
#include <set>

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

namespace {

#define SCE_HA_JS (SCE_HJA_START - SCE_HJ_START)
#define SCE_HA_VBS (SCE_HBA_START - SCE_HB_START)
#define SCE_HA_PYTHON (SCE_HPA_START - SCE_HP_START)

enum script_type { eScriptNone = 0, eScriptJS, eScriptVBS, eScriptPython, eScriptPHP, eScriptXML, eScriptSGML, eScriptSGMLblock, eScriptComment };
enum script_mode { eHtml = 0, eNonHtmlScript, eNonHtmlPreProc, eNonHtmlScriptPreProc };

inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

inline bool IsOperator(int ch) {
	if (IsASCII(ch) && isalnum(ch))
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

void GetTextSegment(Accessor &styler, Sci_PositionU start, Sci_PositionU end, char *s, size_t len) {
	Sci_PositionU i = 0;
	for (; (i < end - start + 1) && (i < len-1); i++) {
		s[i] = MakeLowerCase(styler[start + i]);
	}
	s[i] = '\0';
}

std::string GetStringSegment(Accessor &styler, Sci_PositionU start, Sci_PositionU end) {
	std::string s;
	Sci_PositionU i = 0;
	for (; (i < end - start + 1); i++) {
		s.push_back(MakeLowerCase(styler[start + i]));
	}
	return s;
}

std::string GetNextWord(Accessor &styler, Sci_PositionU start) {
	std::string ret;
	Sci_PositionU i = 0;
	for (; i < 200; i++) {	// Put an upper limit to bound time taken for unexpected text.
		const char ch = styler.SafeGetCharAt(start + i);
		if ((i == 0) && !IsAWordStart(ch))
			break;
		if ((i > 0) && !IsAWordChar(ch))
			break;
		ret.push_back(ch);
	}
	return ret;
}

script_type segIsScriptingIndicator(Accessor &styler, Sci_PositionU start, Sci_PositionU end, script_type prevValue) {
	char s[100];
	GetTextSegment(styler, start, end, s, sizeof(s));
	//Platform::DebugPrintf("Scripting indicator [%s]\n", s);
	if (strstr(s, "src"))	// External script
		return eScriptNone;
	if (strstr(s, "vbs"))
		return eScriptVBS;
	if (strstr(s, "pyth"))
		return eScriptPython;
	if (strstr(s, "javas"))
		return eScriptJS;
	if (strstr(s, "jscr"))
		return eScriptJS;
	if (strstr(s, "php"))
		return eScriptPHP;
	if (strstr(s, "xml")) {
		const char *xml = strstr(s, "xml");
		for (const char *t=s; t<xml; t++) {
			if (!IsASpace(*t)) {
				return prevValue;
			}
		}
		return eScriptXML;
	}

	return prevValue;
}

int PrintScriptingIndicatorOffset(Accessor &styler, Sci_PositionU start, Sci_PositionU end) {
	int iResult = 0;
	std::string s = GetStringSegment(styler, start, end);
	if (0 == strncmp(s.c_str(), "php", 3)) {
		iResult = 3;
	}
	return iResult;
}

script_type ScriptOfState(int state) {
	if ((state >= SCE_HP_START) && (state <= SCE_HP_IDENTIFIER)) {
		return eScriptPython;
	} else if ((state >= SCE_HB_START) && (state <= SCE_HB_STRINGEOL)) {
		return eScriptVBS;
	} else if ((state >= SCE_HJ_START) && (state <= SCE_HJ_REGEX)) {
		return eScriptJS;
	} else if ((state >= SCE_HPHP_DEFAULT) && (state <= SCE_HPHP_COMMENTLINE)) {
		return eScriptPHP;
	} else if ((state >= SCE_H_SGML_DEFAULT) && (state < SCE_H_SGML_BLOCK_DEFAULT)) {
		return eScriptSGML;
	} else if (state == SCE_H_SGML_BLOCK_DEFAULT) {
		return eScriptSGMLblock;
	} else {
		return eScriptNone;
	}
}

int statePrintForState(int state, script_mode inScriptType) {
	int StateToPrint = state;

	if (state >= SCE_HJ_START) {
		if ((state >= SCE_HP_START) && (state <= SCE_HP_IDENTIFIER)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_PYTHON);
		} else if ((state >= SCE_HB_START) && (state <= SCE_HB_STRINGEOL)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_VBS);
		} else if ((state >= SCE_HJ_START) && (state <= SCE_HJ_REGEX)) {
			StateToPrint = state + ((inScriptType == eNonHtmlScript) ? 0 : SCE_HA_JS);
		}
	}

	return StateToPrint;
}

int stateForPrintState(int StateToPrint) {
	int state;

	if ((StateToPrint >= SCE_HPA_START) && (StateToPrint <= SCE_HPA_IDENTIFIER)) {
		state = StateToPrint - SCE_HA_PYTHON;
	} else if ((StateToPrint >= SCE_HBA_START) && (StateToPrint <= SCE_HBA_STRINGEOL)) {
		state = StateToPrint - SCE_HA_VBS;
	} else if ((StateToPrint >= SCE_HJA_START) && (StateToPrint <= SCE_HJA_REGEX)) {
		state = StateToPrint - SCE_HA_JS;
	} else {
		state = StateToPrint;
	}

	return state;
}

inline bool IsNumber(Sci_PositionU start, Accessor &styler) {
	return IsADigit(styler[start]) || (styler[start] == '.') ||
	       (styler[start] == '-') || (styler[start] == '#');
}

inline bool isStringState(int state) {
	bool bResult;

	switch (state) {
	case SCE_HJ_DOUBLESTRING:
	case SCE_HJ_SINGLESTRING:
	case SCE_HJA_DOUBLESTRING:
	case SCE_HJA_SINGLESTRING:
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
		bResult = false;
		break;
	}
	return bResult;
}

inline bool stateAllowsTermination(int state) {
	bool allowTermination = !isStringState(state);
	if (allowTermination) {
		switch (state) {
		case SCE_HB_COMMENTLINE:
		case SCE_HPHP_COMMENT:
		case SCE_HP_COMMENTLINE:
		case SCE_HPA_COMMENTLINE:
			allowTermination = false;
		}
	}
	return allowTermination;
}

// not really well done, since it's only comments that should lex the %> and <%
inline bool isCommentASPState(int state) {
	bool bResult;

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
		bResult = false;
		break;
	}
	return bResult;
}

void classifyAttribHTML(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, Accessor &styler) {
	const bool wordIsNumber = IsNumber(start, styler);
	char chAttr = SCE_H_ATTRIBUTEUNKNOWN;
	if (wordIsNumber) {
		chAttr = SCE_H_NUMBER;
	} else {
		std::string s = GetStringSegment(styler, start, end);
		if (keywords.InList(s.c_str()))
			chAttr = SCE_H_ATTRIBUTE;
	}
	if ((chAttr == SCE_H_ATTRIBUTEUNKNOWN) && !keywords)
		// No keywords -> all are known
		chAttr = SCE_H_ATTRIBUTE;
	styler.ColourTo(end, chAttr);
}

// https://html.spec.whatwg.org/multipage/custom-elements.html#custom-elements-core-concepts
bool isHTMLCustomElement(const std::string &tag) {
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
                           const WordList &keywords, Accessor &styler, bool &tagDontFold,
                    bool caseSensitive, bool isXml, bool allowScripts,
                    const std::set<std::string> &nonFoldingTags) {
	std::string tag;
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
	char chAttr = SCE_H_TAGUNKNOWN;
	if (!tag.empty() && (tag[0] == '!')) {
		chAttr = SCE_H_SGML_DEFAULT;
	} else if (!keywords || keywords.InList(tag.c_str())) {
		chAttr = SCE_H_TAG;
	} else if (!isXml && isHTMLCustomElement(tag)) {
		chAttr = SCE_H_TAG;
	}
	if (chAttr != SCE_H_TAGUNKNOWN) {
		styler.ColourTo(end, chAttr);
	}
	if (chAttr == SCE_H_TAG) {
		if (allowScripts && (tag == "script")) {
			// check to see if this is a self-closing tag by sniffing ahead
			bool isSelfClose = false;
			for (Sci_PositionU cPos = end; cPos <= end + 200; cPos++) {
				const char ch = styler.SafeGetCharAt(cPos, '\0');
				if (ch == '\0' || ch == '>')
					break;
				else if (ch == '/' && styler.SafeGetCharAt(cPos + 1, '\0') == '>') {
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
                             const WordList &keywords, Accessor &styler, script_mode inScriptType) {
	char s[30 + 1];
	Sci_PositionU i = 0;
	for (; i < end - start + 1 && i < 30; i++) {
		s[i] = styler[start + i];
	}
	s[i] = '\0';

	char chAttr = SCE_HJ_WORD;
	const bool wordIsNumber = IsADigit(s[0]) || ((s[0] == '.') && IsADigit(s[1]));
	if (wordIsNumber) {
		chAttr = SCE_HJ_NUMBER;
	} else if (keywords.InList(s)) {
		chAttr = SCE_HJ_KEYWORD;
	}
	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
}

int classifyWordHTVB(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, Accessor &styler, script_mode inScriptType) {
	char chAttr = SCE_HB_IDENTIFIER;
	const bool wordIsNumber = IsADigit(styler[start]) || (styler[start] == '.');
	if (wordIsNumber) {
		chAttr = SCE_HB_NUMBER;
	} else {
		std::string s = GetStringSegment(styler, start, end);
		if (keywords.InList(s.c_str())) {
			chAttr = SCE_HB_WORD;
			if (s == "rem")
				chAttr = SCE_HB_COMMENTLINE;
		}
	}
	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
	if (chAttr == SCE_HB_COMMENTLINE)
		return SCE_HB_COMMENTLINE;
	else
		return SCE_HB_DEFAULT;
}

void classifyWordHTPy(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, Accessor &styler, std::string &prevWord, script_mode inScriptType, bool isMako) {
	const bool wordIsNumber = IsADigit(styler[start]);
	std::string s;
	for (Sci_PositionU i = 0; i < end - start + 1 && i < 30; i++) {
		s.push_back(styler[start + i]);
	}
	char chAttr = SCE_HP_IDENTIFIER;
	if (prevWord == "class")
		chAttr = SCE_HP_CLASSNAME;
	else if (prevWord == "def")
		chAttr = SCE_HP_DEFNAME;
	else if (wordIsNumber)
		chAttr = SCE_HP_NUMBER;
	else if (keywords.InList(s.c_str()))
		chAttr = SCE_HP_WORD;
	else if (isMako && (s == "block"))
		chAttr = SCE_HP_WORD;
	styler.ColourTo(end, statePrintForState(chAttr, inScriptType));
	prevWord = s;
}

// Update the word colour to default or keyword
// Called when in a PHP word
void classifyWordHTPHP(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, Accessor &styler) {
	char chAttr = SCE_HPHP_DEFAULT;
	const bool wordIsNumber = IsADigit(styler[start]) || (styler[start] == '.' && start+1 <= end && IsADigit(styler[start+1]));
	if (wordIsNumber) {
		chAttr = SCE_HPHP_NUMBER;
	} else {
		std::string s = GetStringSegment(styler, start, end);
		if (keywords.InList(s.c_str()))
			chAttr = SCE_HPHP_WORD;
	}
	styler.ColourTo(end, chAttr);
}

bool isWordHSGML(Sci_PositionU start, Sci_PositionU end, const WordList &keywords, Accessor &styler) {
	std::string s;
	for (Sci_PositionU i = 0; i < end - start + 1 && i < 30; i++) {
		s.push_back(styler[start + i]);
	}
	return keywords.InList(s.c_str());
}

bool isWordCdata(Sci_PositionU start, Sci_PositionU end, Accessor &styler) {
	std::string s;
	for (Sci_PositionU i = 0; i < end - start + 1 && i < 30; i++) {
		s.push_back(styler[start + i]);
	}
	return s == "[CDATA[";
}

// Return the first state to reach when entering a scripting language
int StateForScript(script_type scriptLanguage) {
	int Result;
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
		Result = SCE_HJ_START;
		break;
	}
	return Result;
}

inline bool issgmlwordchar(int ch) {
	return !IsASCII(ch) ||
		(isalnum(ch) || ch == '.' || ch == '_' || ch == ':' || ch == '!' || ch == '#' || ch == '[');
}

inline bool IsPhpWordStart(int ch) {
	return (IsASCII(ch) && (isalpha(ch) || (ch == '_'))) || (ch >= 0x7f);
}

inline bool IsPhpWordChar(int ch) {
	return IsADigit(ch) || IsPhpWordStart(ch);
}

bool InTagState(int state) {
	return state == SCE_H_TAG || state == SCE_H_TAGUNKNOWN ||
	       state == SCE_H_SCRIPT ||
	       state == SCE_H_ATTRIBUTE || state == SCE_H_ATTRIBUTEUNKNOWN ||
	       state == SCE_H_NUMBER || state == SCE_H_OTHER ||
	       state == SCE_H_DOUBLESTRING || state == SCE_H_SINGLESTRING;
}

bool IsCommentState(const int state) {
	return state == SCE_H_COMMENT || state == SCE_H_SGML_COMMENT;
}

bool IsScriptCommentState(const int state) {
	return state == SCE_HJ_COMMENT || state == SCE_HJ_COMMENTLINE || state == SCE_HJA_COMMENT ||
		   state == SCE_HJA_COMMENTLINE || state == SCE_HB_COMMENTLINE || state == SCE_HBA_COMMENTLINE;
}

bool isLineEnd(int ch) {
	return ch == '\r' || ch == '\n';
}

bool isMakoBlockEnd(const int ch, const int chNext, const std::string &blockType) {
	if (blockType.empty()) {
		return ((ch == '%') && (chNext == '>'));
	} else if ((blockType == "inherit") ||
			   (blockType == "namespace") ||
			   (blockType == "include") ||
			   (blockType == "page")) {
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

bool isDjangoBlockEnd(const int ch, const int chNext, const std::string &blockType) {
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

bool isPHPStringState(int state) {
	return
	    (state == SCE_HPHP_HSTRING) ||
	    (state == SCE_HPHP_SIMPLESTRING) ||
	    (state == SCE_HPHP_HSTRING_VARIABLE) ||
	    (state == SCE_HPHP_COMPLEX_VARIABLE);
}

Sci_Position FindPhpStringDelimiter(std::string &phpStringDelimiter, Sci_Position i, const Sci_Position lengthDoc, Accessor &styler, bool &isSimpleString) {
	Sci_Position j;
	const Sci_Position beginning = i - 1;
	bool isValidSimpleString = false;

	while (i < lengthDoc && (styler[i] == ' ' || styler[i] == '\t'))
		i++;
	char ch = styler.SafeGetCharAt(i);
	const char chNext = styler.SafeGetCharAt(i + 1);
	phpStringDelimiter.clear();
	if (!IsPhpWordStart(ch)) {
		if (ch == '\'' && IsPhpWordStart(chNext)) {
			i++;
			ch = chNext;
			isSimpleString = true;
		} else {
			return beginning;
		}
	}
	phpStringDelimiter.push_back(ch);
	i++;
	for (j = i; j < lengthDoc && !isLineEnd(styler[j]); j++) {
		if (!IsPhpWordChar(styler[j])) {
			if (isSimpleString && (styler[j] == '\'') && isLineEnd(styler.SafeGetCharAt(j + 1))) {
				isValidSimpleString = true;
				j++;
				break;
			} else {
				phpStringDelimiter.clear();
				return beginning;
			}
		}
		phpStringDelimiter.push_back(styler[j]);
	}
	if (isSimpleString && !isValidSimpleString) {
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
	bool isMako = false;
	bool isDjango = false;
	bool fold = false;
	bool foldHTML = false;
	bool foldHTMLPreprocessor = true;
	bool foldCompact = true;
	bool foldComment = false;
	bool foldHeredoc = false;
	bool foldXmlAtTagOpen = false;
	OptionsHTML() noexcept {
	}
};

const char * const htmlWordListDesc[] = {
	"HTML elements and attributes",
	"JavaScript keywords",
	"VBScript keywords",
	"Python keywords",
	"PHP keywords",
	"SGML and DTD keywords",
	0,
};

const char * const phpscriptWordListDesc[] = {
	"", //Unused
	"", //Unused
	"", //Unused
	"", //Unused
	"PHP keywords",
	"", //Unused
	0,
};

struct OptionSetHTML : public OptionSet<OptionsHTML> {
	OptionSetHTML(bool isPHPScript_) {

		DefineProperty("asp.default.language", &OptionsHTML::aspDefaultLanguage,
			"Script in ASP code is initially assumed to be in JavaScript. "
			"To change this to VBScript set asp.default.language to 2. Python is 3.");

		DefineProperty("html.tags.case.sensitive", &OptionsHTML::caseSensitive,
			"For XML and HTML, setting this property to 1 will make tags match in a case "
			"sensitive way which is the expected behaviour for XML and XHTML.");

		DefineProperty("lexer.xml.allow.scripts", &OptionsHTML::allowScripts,
			"Set to 0 to disable scripts in XML.");

		DefineProperty("lexer.html.mako", &OptionsHTML::isMako,
			"Set to 1 to enable the mako template language.");

		DefineProperty("lexer.html.django", &OptionsHTML::isDjango,
			"Set to 1 to enable the django template language.");

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

LexicalClass lexicalClassesHTML[] = {
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
	20, "SCE_H_XCCOMMENT", "comment", "JSP Comment <%-- ... --%>",
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
	53, "", "unused", "",
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
	68, "", "unused", "",
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

LexicalClass lexicalClassesXML[] = {
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

const char *tagsThatDoNotFold[] = {
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
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6; // SGML (DTD) keywords
	OptionsHTML options;
	OptionSetHTML osHTML;
	std::set<std::string> nonFoldingTags;
public:
	explicit LexerHTML(bool isXml_, bool isPHPScript_) :
		DefaultLexer(
			isXml_ ? "xml" : (isPHPScript_ ? "phpscript" : "hypertext"),
			isXml_ ? SCLEX_XML : (isPHPScript_ ? SCLEX_PHPSCRIPT : SCLEX_HTML),
			isXml_ ? lexicalClassesHTML : lexicalClassesXML,
			isXml_ ? std::size(lexicalClassesHTML) : std::size(lexicalClassesXML)),
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
	WordList *wordListN = 0;
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

void SCI_METHOD LexerHTML::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);
	if (isPHPScript && (startPos == 0)) {
		initStyle = SCE_HPHP_DEFAULT;
	}
	styler.StartAt(startPos);
	std::string prevWord;
	std::string phpStringDelimiter;
	int StateToPrint = initStyle;
	int state = stateForPrintState(StateToPrint);
	std::string makoBlockType;
	int makoComment = 0;
	std::string djangoBlockType;
	// If inside a tag, it may be a script tag, so reread from the start of line starting tag to ensure any language tags are seen
	if (InTagState(state)) {
		while ((startPos > 0) && (InTagState(styler.StyleAt(startPos - 1)))) {
			const Sci_Position backLineStart = styler.LineStart(styler.GetLine(startPos-1));
			length += startPos - backLineStart;
			startPos = backLineStart;
		}
		state = SCE_H_DEFAULT;
	}
	// String can be heredoc, must find a delimiter first. Reread from beginning of line containing the string, to get the correct lineState
	if (isPHPStringState(state)) {
		while (startPos > 0 && (isPHPStringState(state) || !isLineEnd(styler[startPos - 1]))) {
			startPos--;
			length++;
			state = styler.StyleAt(startPos);
		}
		if (startPos == 0)
			state = SCE_H_DEFAULT;
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
	const bool isMako = options.isMako;
	const bool isDjango = options.isDjango;
	const CharacterSet setHTMLWord(CharacterSet::setAlphaNum, ".-_:!#", 0x80, true);
	const CharacterSet setTagContinue(CharacterSet::setAlphaNum, ".-_:!#[", 0x80, true);
	const CharacterSet setAttributeContinue(CharacterSet::setAlphaNum, ".-_:!#/", 0x80, true);
	// TODO: also handle + and - (except if they're part of ++ or --) and return keywords
	const CharacterSet setOKBeforeJSRE(CharacterSet::setNone, "([{=,:;!%^&*|?~");

	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int visibleChars = 0;
	int lineStartVisibleChars = 0;

	int chPrev = ' ';
	int ch = ' ';
	int chPrevNonWhite = ' ';
	// look back to set chPrevNonWhite properly for better regex colouring
	if (scriptLanguage == eScriptJS && startPos > 0) {
		Sci_Position back = startPos;
		int style = 0;
		while (--back) {
			style = styler.StyleAt(back);
			if (style < SCE_HJ_DEFAULT || style > SCE_HJ_COMMENTDOC)
				// includes SCE_HJ_COMMENT & SCE_HJ_COMMENTLINE
				break;
		}
		if (style == SCE_HJ_SYMBOLS) {
			chPrevNonWhite = static_cast<unsigned char>(styler.SafeGetCharAt(back));
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
		int chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
		const int chNext2 = static_cast<unsigned char>(styler.SafeGetCharAt(i + 2));

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

				if ((state != SCE_HPHP_COMMENT) && (state != SCE_HPHP_COMMENTLINE) && (state != SCE_HJ_COMMENT) && (state != SCE_HJ_COMMENTLINE) && (state != SCE_HJ_COMMENTDOC) && (!isStringState(state))) {
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
				} else if (((state == SCE_HPHP_COMMENT) || (state == SCE_HJ_COMMENT)) && foldComment && (ch == '*') && (chNext == '/')) {
					levelCurrent--;
				}
				break;
			case eScriptPython:
				if (state != SCE_HP_COMMENTLINE && !isMako) {
					if ((ch == ':') && ((chNext == '\n') || (chNext == '\r' && chNext2 == '\n'))) {
						levelCurrent++;
					} else if ((ch == '\n') && !((chNext == '\r') && (chNext2 == '\n')) && (chNext != '\n')) {
						// check if the number of tabs is lower than the level
						int Findlevel = (levelCurrent & ~SC_FOLDLEVELBASE) * 8;
						for (Sci_Position j = 0; Findlevel > 0; j++) {
							const char chTmp = styler.SafeGetCharAt(i + j + 1);
							if (chTmp == '\t') {
								Findlevel -= 8;
							} else if (chTmp == ' ') {
								Findlevel--;
							} else {
								break;
							}
						}

						if (Findlevel > 0) {
							levelCurrent -= Findlevel / 8;
							if (Findlevel % 8)
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
			                    ((beforePreProc & 0xFF) << 12));
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
			case SCE_HB_COMMENTLINE:
			case SCE_HBA_COMMENTLINE:
			case SCE_HJ_DOUBLESTRING:
			case SCE_HJ_SINGLESTRING:
			case SCE_HJ_REGEX:
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
						state == SCE_HJ_COMMENTLINE || isXml ? "script" :
						state == SCE_H_COMMENT ? "comment" : 0) {
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
		else if ((state != SCE_H_ASPAT) &&
		         !isStringState(state) &&
		         (state != SCE_HPHP_COMMENT) &&
		         (state != SCE_HPHP_COMMENTLINE) &&
		         (ch == '<') &&
		         (chNext == '?') &&
				 !IsScriptCommentState(state)) {
 			beforeLanguage = scriptLanguage;
			scriptLanguage = segIsScriptingIndicator(styler, i + 2, i + 6, isXml ? eScriptXML : eScriptPHP);
			if ((scriptLanguage != eScriptPHP) && (isStringState(state) || (state==SCE_H_COMMENT))) continue;
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			i++;
			visibleChars++;
			i += PrintScriptingIndicatorOffset(styler, styler.GetStartSegment() + 2, i + 6);
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
			ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
			continue;
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
				makoBlockType = GetNextWord(styler, i+3);
			else
				makoBlockType = GetNextWord(styler, i+2);
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
				visibleChars += static_cast<int>(makoBlockType.length());
				if (keywords4.InList(makoBlockType.c_str()))
					styler.ColourTo(i, SCE_HP_WORD);
				else
					styler.ColourTo(i, SCE_H_TAGUNKNOWN);
			}

			ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
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
			ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
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

			ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
			continue;
		}

		// handle the start of ASP pre-processor = Non-HTML
		else if (!isMako && !isDjango && !isCommentASPState(state) && (ch == '<') && (chNext == '%') && !isPHPStringState(state)) {
			styler.ColourTo(i - 1, StateToPrint);
			beforePreProc = state;
			if (inScriptType == eNonHtmlScript)
				inScriptType = eNonHtmlScriptPreProc;
			else
				inScriptType = eNonHtmlPreProc;

			if (chNext2 == '@') {
				i += 2; // place as if it was the second next char treated
				visibleChars += 2;
				state = SCE_H_ASPAT;
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
			}
			scriptLanguage = eScriptVBS;
			styler.ColourTo(i, SCE_H_ASP);
			// fold whole script
			if (foldHTMLPreprocessor)
				levelCurrent++;
			// should be better
			ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
			continue;
		}

		/////////////////////////////////////
		// handle the start of SGML language (DTD)
		else if (((scriptLanguage == eScriptNone) || (scriptLanguage == eScriptXML)) &&
				 (chPrev == '<') &&
				 (ch == '!') &&
				 (StateToPrint != SCE_H_CDATA) &&
				 (!IsCommentState(StateToPrint)) &&
				 (!IsScriptCommentState(StateToPrint))) {
			beforePreProc = state;
			styler.ColourTo(i - 2, StateToPrint);
			if ((chNext == '-') && (chNext2 == '-')) {
				state = SCE_H_COMMENT; // wait for a pending command
				styler.ColourTo(i + 2, SCE_H_COMMENT);
				i += 2; // follow styling after the --
			} else if (isWordCdata(i + 1, i + 7, styler)) {
				state = SCE_H_CDATA;
			} else {
				styler.ColourTo(i, SCE_H_SGML_DEFAULT); // <! is default
				scriptLanguage = eScriptSGML;
				state = SCE_H_SGML_COMMAND; // wait for a pending command
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
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywords4, styler, prevWord, inScriptType, isMako);
			} else {
				styler.ColourTo(i - 1, StateToPrint);
			}
			if ((makoBlockType != "%") && (makoBlockType != "{") && ch != '>') {
				i++;
				visibleChars++;
		    }
			else if ((makoBlockType == "%") && ch == '/') {
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
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywords4, styler, prevWord, inScriptType, isMako);
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
				  (((ch == '%') || (ch == '?')) && (chNext == '>'))) ||
		         ((scriptLanguage == eScriptSGML) && (ch == '>') && (state != SCE_H_SGML_COMMENT))) {
			if (state == SCE_H_ASPAT) {
				aspScript = segIsScriptingIndicator(styler,
				                                    styler.GetStartSegment(), i - 1, aspScript);
			}
			// Bounce out of any ASP mode
			switch (state) {
			case SCE_HJ_WORD:
				classifyWordHTJS(styler.GetStartSegment(), i - 1, keywords2, styler, inScriptType);
				break;
			case SCE_HB_WORD:
				classifyWordHTVB(styler.GetStartSegment(), i - 1, keywords3, styler, inScriptType);
				break;
			case SCE_HP_WORD:
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywords4, styler, prevWord, inScriptType, isMako);
				break;
			case SCE_HPHP_WORD:
				classifyWordHTPHP(styler.GetStartSegment(), i - 1, keywords5, styler);
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
				if (foldXmlAtTagOpen && !(chNext == '/' || chNext == '?' || chNext == '!' || chNext == '-' || chNext == '%')) {
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
			} else if (IsASCII(ch) && isalpha(ch) && (chPrev == '%')) {
				styler.ColourTo(i - 2, StateToPrint);
				state = SCE_H_SGML_ENTITY;
			} else if (ch == '#') {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_H_SGML_SPECIAL;
			} else if (ch == '[') {
				styler.ColourTo(i - 1, StateToPrint);
				scriptLanguage = eScriptSGMLblock;
				state = SCE_H_SGML_BLOCK_DEFAULT;
			} else if (ch == ']') {
				if (scriptLanguage == eScriptSGMLblock) {
					styler.ColourTo(i, StateToPrint);
					scriptLanguage = eScriptSGML;
				} else {
					styler.ColourTo(i - 1, StateToPrint);
					styler.ColourTo(i, SCE_H_SGML_ERROR);
				}
				state = SCE_H_SGML_DEFAULT;
			} else if (scriptLanguage == eScriptSGMLblock) {
				if ((ch == '!') && (chPrev == '<')) {
					styler.ColourTo(i - 2, StateToPrint);
					styler.ColourTo(i, SCE_H_SGML_DEFAULT);
					state = SCE_H_SGML_COMMAND;
				} else if (ch == '>') {
					styler.ColourTo(i - 1, StateToPrint);
					styler.ColourTo(i, SCE_H_SGML_DEFAULT);
				}
			}
			break;
		case SCE_H_SGML_COMMAND:
			if ((ch == '-') && (chPrev == '-')) {
				styler.ColourTo(i - 2, StateToPrint);
				state = SCE_H_SGML_COMMENT;
			} else if (!issgmlwordchar(ch)) {
				if (isWordHSGML(styler.GetStartSegment(), i - 1, keywords6, styler)) {
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
				if (scriptLanguage == eScriptSGMLblock) {
					styler.ColourTo(i - 2, SCE_H_SGML_BLOCK_DEFAULT);
				} else {
					styler.ColourTo(i - 2, SCE_H_SGML_DEFAULT);
				}
				state = SCE_H_SGML_1ST_PARAM_COMMENT;
			} else if (issgmlwordchar(ch)) {
				if (scriptLanguage == eScriptSGMLblock) {
					styler.ColourTo(i - 1, SCE_H_SGML_BLOCK_DEFAULT);
				} else {
					styler.ColourTo(i - 1, SCE_H_SGML_DEFAULT);
				}
				// find the length of the word
				int size = 1;
				while (setHTMLWord.Contains(static_cast<unsigned char>(styler.SafeGetCharAt(i + size))))
					size++;
				styler.ColourTo(i + size - 1, StateToPrint);
				i += size - 1;
				visibleChars += size - 1;
				ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
				if (scriptLanguage == eScriptSGMLblock) {
					state = SCE_H_SGML_BLOCK_DEFAULT;
				} else {
					state = SCE_H_SGML_DEFAULT;
				}
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
			if ((scriptLanguage != eScriptComment) && (chPrev2 == '-') && (chPrev == '-') && (ch == '>')) {
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
			if (!(IsASCII(ch) && isupper(ch))) {
				styler.ColourTo(i - 1, StateToPrint);
				if (isalnum(ch)) {
					state = SCE_H_SGML_ERROR;
				} else {
					state = SCE_H_SGML_DEFAULT;
				}
			}
			break;
		case SCE_H_SGML_ENTITY:
			if (ch == ';') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_SGML_DEFAULT;
			} else if (!(IsASCII(ch) && isalnum(ch)) && ch != '-' && ch != '.') {
				styler.ColourTo(i, SCE_H_SGML_ERROR);
				state = SCE_H_SGML_DEFAULT;
			}
			break;
		case SCE_H_ENTITY:
			if (ch == ';') {
				styler.ColourTo(i, StateToPrint);
				state = SCE_H_DEFAULT;
			}
			if (ch != '#' && !(IsASCII(ch) && isalnum(ch))	// Should check that '#' follows '&', but it is unlikely anyway...
				&& ch != '.' && ch != '-' && ch != '_' && ch != ':') { // valid in XML
				if (!IsASCII(ch))	// Possibly start of a multibyte character so don't allow this byte to be in entity style
					styler.ColourTo(i-1, SCE_H_TAGUNKNOWN);
				else
					styler.ColourTo(i, SCE_H_TAGUNKNOWN);
				state = SCE_H_DEFAULT;
			}
			break;
		case SCE_H_TAGUNKNOWN:
			if (!setTagContinue.Contains(ch) && !((ch == '/') && (chPrev == '<'))) {
				int eClass = classifyTagHTML(styler.GetStartSegment(),
					i - 1, keywords, styler, tagDontFold, caseSensitive, isXml, allowScripts, nonFoldingTags);
				if (eClass == SCE_H_SCRIPT || eClass == SCE_H_COMMENT) {
					if (!tagClosing) {
						inScriptType = eNonHtmlScript;
						scriptLanguage = eClass == SCE_H_SCRIPT ? clientScript : eScriptComment;
					} else {
						scriptLanguage = eScriptNone;
					}
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
				if (inScriptType == eNonHtmlScript) {
					const int scriptLanguagePrev = scriptLanguage;
					clientScript = segIsScriptingIndicator(styler, styler.GetStartSegment(), i - 1, scriptLanguage);
					scriptLanguage = clientScript;
					if ((scriptLanguagePrev != scriptLanguage) && (scriptLanguage == eScriptNone))
						inScriptType = eHtml;
				}
				classifyAttribHTML(styler.GetStartSegment(), i - 1, keywords, styler);
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
				if (inScriptType == eNonHtmlScript) {
					scriptLanguage = segIsScriptingIndicator(styler, styler.GetStartSegment(), i, scriptLanguage);
				}
				styler.ColourTo(i, SCE_H_DOUBLESTRING);
				state = SCE_H_OTHER;
			}
			break;
		case SCE_H_SINGLESTRING:
			if (ch == '\'') {
				if (inScriptType == eNonHtmlScript) {
					scriptLanguage = segIsScriptingIndicator(styler, styler.GetStartSegment(), i, scriptLanguage);
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
					if (IsNumber(styler.GetStartSegment(), styler)) {
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
				if (chNext2 == '*')
					state = SCE_HJ_COMMENTDOC;
				else
					state = SCE_HJ_COMMENT;
				if (chNext2 == '/') {
					// Eat the * so it isn't used for the end of the comment
					i++;
				}
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
				classifyWordHTJS(styler.GetStartSegment(), i - 1, keywords2, styler, inScriptType);
				//styler.ColourTo(i - 1, eHTJSKeyword);
				state = SCE_HJ_DEFAULT;
				if (ch == '/' && chNext == '*') {
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
			} else if ((inScriptType == eNonHtmlScript) && (ch == '-') && (chNext == '-') && (chNext2 == '>')) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_COMMENTLINE;
				i += 2;
			} else if (isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_STRINGEOL;
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
			} else if ((inScriptType == eNonHtmlScript) && (ch == '-') && (chNext == '-') && (chNext2 == '>')) {
				styler.ColourTo(i - 1, StateToPrint);
				state = SCE_HJ_COMMENTLINE;
				i += 2;
			} else if (isLineEnd(ch)) {
				styler.ColourTo(i - 1, StateToPrint);
				if (chPrev != '\\' && (chPrev2 != '\\' || chPrev != '\r' || ch != '\n')) {
					state = SCE_HJ_STRINGEOL;
				}
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
					while (IsASCII(chNext) && islower(chNext)) {   // gobble regex flags
						i++;
						ch = chNext;
						chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
					}
				}
				styler.ColourTo(i, StateToPrint);
				state = SCE_HJ_DEFAULT;
			} else if (ch == '\\') {
				// Gobble up the quoted character
				if (chNext == '\\' || chNext == '/') {
					i++;
					ch = chNext;
					chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
				state = classifyWordHTVB(styler.GetStartSegment(), i - 1, keywords3, styler, inScriptType);
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
					chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
					chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
				classifyWordHTPy(styler.GetStartSegment(), i - 1, keywords4, styler, prevWord, inScriptType, isMako);
				state = SCE_HP_DEFAULT;
				if (ch == '#') {
					state = SCE_HP_COMMENTLINE;
				} else if (ch == '\"') {
					if (chNext == '\"' && chNext2 == '\"') {
						i += 2;
						state = SCE_HP_TRIPLEDOUBLE;
						ch = ' ';
						chPrev = ' ';
						chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
					} else {
						state = SCE_HP_STRING;
					}
				} else if (ch == '\'') {
					if (chNext == '\'' && chNext2 == '\'') {
						i += 2;
						state = SCE_HP_TRIPLE;
						ch = ' ';
						chPrev = ' ';
						chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
					chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
					chNext = static_cast<unsigned char>(styler.SafeGetCharAt(i + 1));
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
			if (!IsAWordChar(ch)) {
				classifyWordHTPHP(styler.GetStartSegment(), i - 1, keywords5, styler);
				if (ch == '/' && chNext == '*') {
					i++;
					state = SCE_HPHP_COMMENT;
				} else if (ch == '/' && chNext == '/') {
					i++;
					state = SCE_HPHP_COMMENTLINE;
				} else if (ch == '#') {
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
			// recognize bases 8,10 or 16 integers OR floating-point numbers
			if (!IsADigit(ch)
				&& strchr(".xXabcdefABCDEF", ch) == NULL
				&& ((ch != '-' && ch != '+') || (chPrev != 'e' && chPrev != 'E'))) {
				styler.ColourTo(i - 1, SCE_HPHP_NUMBER);
				if (IsOperator(ch))
					state = SCE_HPHP_OPERATOR;
				else
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
				} else if (isLineEnd(chPrev)) {
					const int psdLength = static_cast<int>(phpStringDelimiter.length());
					const char chAfterPsd = styler.SafeGetCharAt(i + psdLength);
					const char chAfterPsd2 = styler.SafeGetCharAt(i + psdLength + 1);
					if (isLineEnd(chAfterPsd) ||
						(chAfterPsd == ';' && isLineEnd(chAfterPsd2))) {
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
			} else if (isLineEnd(chPrev) && styler.Match(i, phpStringDelimiter.c_str())) {
				const int psdLength = static_cast<int>(phpStringDelimiter.length());
				const char chAfterPsd = styler.SafeGetCharAt(i + psdLength);
				const char chAfterPsd2 = styler.SafeGetCharAt(i + psdLength + 1);
				if (isLineEnd(chAfterPsd) ||
				(chAfterPsd == ';' && isLineEnd(chAfterPsd2))) {
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
			if (IsADigit(ch) || (ch == '.' && IsADigit(chNext))) {
				state = SCE_HPHP_NUMBER;
			} else if (IsAWordStart(ch)) {
				state = SCE_HPHP_WORD;
			} else if (ch == '/' && chNext == '*') {
				i++;
				state = SCE_HPHP_COMMENT;
			} else if (ch == '/' && chNext == '/') {
				i++;
				state = SCE_HPHP_COMMENTLINE;
			} else if (ch == '#') {
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
				if (styler.SafeGetCharAt(i + 2) == '*')
					state = SCE_HJ_COMMENTDOC;
				else
					state = SCE_HJ_COMMENT;
			} else if (ch == '/' && chNext == '/') {
				state = SCE_HJ_COMMENTLINE;
			} else if ((ch == '\"') && (nonEmptySegment)) {
				state = SCE_HJ_DOUBLESTRING;
			} else if ((ch == '\'') && (nonEmptySegment)) {
				state = SCE_HJ_SINGLESTRING;
			} else if (IsAWordStart(ch)) {
				state = SCE_HJ_WORD;
			} else if (IsOperator(ch)) {
				styler.ColourTo(i, statePrintForState(SCE_HJ_SYMBOLS, inScriptType));
			}
		}
	}

	switch (state) {
	case SCE_HJ_WORD:
		classifyWordHTJS(styler.GetStartSegment(), lengthDoc - 1, keywords2, styler, inScriptType);
		break;
	case SCE_HB_WORD:
		classifyWordHTVB(styler.GetStartSegment(), lengthDoc - 1, keywords3, styler, inScriptType);
		break;
	case SCE_HP_WORD:
		classifyWordHTPy(styler.GetStartSegment(), lengthDoc - 1, keywords4, styler, prevWord, inScriptType, isMako);
		break;
	case SCE_HPHP_WORD:
		classifyWordHTPHP(styler.GetStartSegment(), lengthDoc - 1, keywords5, styler);
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

LexerModule lmHTML(SCLEX_HTML, LexerHTML::LexerFactoryHTML, "hypertext", htmlWordListDesc);
LexerModule lmXML(SCLEX_XML, LexerHTML::LexerFactoryXML, "xml", htmlWordListDesc);
LexerModule lmPHPSCRIPT(SCLEX_PHPSCRIPT, LexerHTML::LexerFactoryPHPScript, "phpscript", phpscriptWordListDesc);
