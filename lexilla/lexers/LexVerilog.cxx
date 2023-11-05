// Scintilla source code edit control
/** @file LexVerilog.cxx
 ** Lexer for Verilog.
 ** Written by Avi Yegudin, based on C++ lexer by Neil Hodgson
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

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
#include "LexerModule.h"

#include "OptionSet.h"
#include "SubStyles.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {
	// Use an unnamed namespace to protect the functions and classes from name conflicts

struct PPDefinition {
	Sci_Position line;
	std::string key;
	std::string value;
	bool isUndef;
	std::string arguments;
	PPDefinition(Sci_Position line_, const std::string &key_, const std::string &value_, bool isUndef_ = false, const std::string &arguments_ = "") :
		line(line_), key(key_), value(value_), isUndef(isUndef_), arguments(arguments_) {
	}
};

class LinePPState {
	int state;
	int ifTaken;
	int level;
	bool ValidLevel() const {
		return level >= 0 && level < 32;
	}
	int maskLevel() const {
		if (level >= 0) {
			return 1 << level;
		} else {
			return 1;
		}
	}
public:
	LinePPState() : state(0), ifTaken(0), level(-1) {
	}
	bool IsInactive() const {
		return state != 0;
	}
	bool CurrentIfTaken() const {
		return (ifTaken & maskLevel()) != 0;
	}
	void StartSection(bool on) {
		level++;
		if (ValidLevel()) {
			if (on) {
				state &= ~maskLevel();
				ifTaken |= maskLevel();
			} else {
				state |= maskLevel();
				ifTaken &= ~maskLevel();
			}
		}
	}
	void EndSection() {
		if (ValidLevel()) {
			state &= ~maskLevel();
			ifTaken &= ~maskLevel();
		}
		level--;
	}
	void InvertCurrentLevel() {
		if (ValidLevel()) {
			state ^= maskLevel();
			ifTaken |= maskLevel();
		}
	}
};

// Hold the preprocessor state for each line seen.
// Currently one entry per line but could become sparse with just one entry per preprocessor line.
class PPStates {
	std::vector<LinePPState> vlls;
public:
	LinePPState ForLine(Sci_Position line) const {
		if ((line > 0) && (vlls.size() > static_cast<size_t>(line))) {
			return vlls[line];
		} else {
			return LinePPState();
		}
	}
	void Add(Sci_Position line, LinePPState lls) {
		vlls.resize(line+1);
		vlls[line] = lls;
	}
};

// Options used for LexerVerilog
struct OptionsVerilog {
	bool foldComment;
	bool foldPreprocessor;
	bool foldPreprocessorElse;
	bool foldCompact;
	bool foldAtElse;
	bool foldAtModule;
	bool trackPreprocessor;
	bool updatePreprocessor;
	bool portStyling;
	bool allUppercaseDocKeyword;
	OptionsVerilog() {
		foldComment = false;
		foldPreprocessor = false;
		foldPreprocessorElse = false;
		foldCompact = false;
		foldAtElse = false;
		foldAtModule = false;
		// for backwards compatibility, preprocessor functionality is disabled by default
		trackPreprocessor = false;
		updatePreprocessor = false;
		// for backwards compatibility, treat input/output/inout as regular keywords
		portStyling = false;
		// for backwards compatibility, don't treat all uppercase identifiers as documentation keywords
		allUppercaseDocKeyword = false;
	}
};

struct OptionSetVerilog : public OptionSet<OptionsVerilog> {
	OptionSetVerilog() {
		DefineProperty("fold.comment", &OptionsVerilog::foldComment,
			"This option enables folding multi-line comments when using the Verilog lexer.");
		DefineProperty("fold.preprocessor", &OptionsVerilog::foldPreprocessor,
			"This option enables folding preprocessor directives when using the Verilog lexer.");
		DefineProperty("fold.compact", &OptionsVerilog::foldCompact);
		DefineProperty("fold.at.else", &OptionsVerilog::foldAtElse,
			"This option enables folding on the else line of an if statement.");
		DefineProperty("fold.verilog.flags", &OptionsVerilog::foldAtModule,
			"This option enables folding module definitions. Typically source files "
			"contain only one module definition so this option is somewhat useless.");
		DefineProperty("lexer.verilog.track.preprocessor", &OptionsVerilog::trackPreprocessor,
			"Set to 1 to interpret `if/`else/`endif to grey out code that is not active.");
		DefineProperty("lexer.verilog.update.preprocessor", &OptionsVerilog::updatePreprocessor,
			"Set to 1 to update preprocessor definitions when `define, `undef, or `undefineall found.");
		DefineProperty("lexer.verilog.portstyling", &OptionsVerilog::portStyling,
			"Set to 1 to style input, output, and inout ports differently from regular keywords.");
		DefineProperty("lexer.verilog.allupperkeywords", &OptionsVerilog::allUppercaseDocKeyword,
			"Set to 1 to style identifiers that are all uppercase as documentation keyword.");
		DefineProperty("lexer.verilog.fold.preprocessor.else", &OptionsVerilog::foldPreprocessorElse,
			"This option enables folding on `else and `elsif preprocessor directives.");
	}
};

const char styleSubable[] = {0};

}

class LexerVerilog : public DefaultLexer {
	CharacterSet setWord;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList ppDefinitions;
	PPStates vlls;
	std::vector<PPDefinition> ppDefineHistory;
	struct SymbolValue {
		std::string value;
		std::string arguments;
		SymbolValue(const std::string &value_="", const std::string &arguments_="") : value(value_), arguments(arguments_) {
		}
		SymbolValue &operator = (const std::string &value_) {
			value = value_;
			arguments.clear();
			return *this;
		}
		bool IsMacro() const {
			return !arguments.empty();
		}
	};
	typedef std::map<std::string, SymbolValue> SymbolTable;
	SymbolTable preprocessorDefinitionsStart;
	OptionsVerilog options;
	OptionSetVerilog osVerilog;
	enum { activeFlag = 0x40 };
	SubStyles subStyles;

	// states at end of line (EOL) during fold operations:
	//		foldExternFlag: EOL while parsing an extern function/task declaration terminated by ';'
	//		foldWaitDisableFlag: EOL while parsing wait or disable statement, terminated by "fork" or '('
	//		typdefFlag: EOL while parsing typedef statement, terminated by ';'
	enum {foldExternFlag = 0x01, foldWaitDisableFlag = 0x02, typedefFlag = 0x04, protectedFlag = 0x08};
	// map using line number as key to store fold state information
	std::map<Sci_Position, int> foldState;

public:
	LexerVerilog() :
		DefaultLexer("verilog", SCLEX_VERILOG),
		setWord(CharacterSet::setAlphaNum, "._", 0x80, true),
		subStyles(styleSubable, 0x80, 0x40, activeFlag) {
		}
	virtual ~LexerVerilog() {}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	const char* SCI_METHOD PropertyNames() override {
		return osVerilog.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char* name) override {
		return osVerilog.PropertyType(name);
	}
	const char* SCI_METHOD DescribeProperty(const char* name) override {
		return osVerilog.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char* key, const char* val) override {
	    return osVerilog.PropertySet(&options, key, val);
	}
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osVerilog.PropertyGet(key);
	}
	const char* SCI_METHOD DescribeWordListSets() override {
		return osVerilog.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void* SCI_METHOD PrivateCall(int, void*) override {
		return 0;
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
		int styleBase = subStyles.BaseStyle(MaskActive(subStyle));
		int active = subStyle & activeFlag;
		return styleBase | active;
	}
	int SCI_METHOD PrimaryStyleFromStyle(int style) override {
		return MaskActive(style);
 	}
	void SCI_METHOD FreeSubStyles() override {
		subStyles.Free();
	}
	void SCI_METHOD SetIdentifiers(int style, const char *identifiers) override {
		subStyles.SetIdentifiers(style, identifiers);
	}
	int SCI_METHOD DistanceToSecondaryStyles() override {
		return activeFlag;
	}
	const char * SCI_METHOD GetSubStyleBases() override {
		return styleSubable;
	}
	static ILexer5* LexerFactoryVerilog() {
		return new LexerVerilog();
	}
	static int MaskActive(int style) {
		return style & ~activeFlag;
	}
	std::vector<std::string> Tokenize(const std::string &expr) const;
};

Sci_Position SCI_METHOD LexerVerilog::WordListSet(int n, const char *wl) {
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
		wordListN = &ppDefinitions;
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
			if (n == 5) {
				// Rebuild preprocessorDefinitions
				preprocessorDefinitionsStart.clear();
				for (int nDefinition = 0; nDefinition < ppDefinitions.Length(); nDefinition++) {
					const char *cpDefinition = ppDefinitions.WordAt(nDefinition);
					const char *cpEquals = strchr(cpDefinition, '=');
					if (cpEquals) {
						std::string name(cpDefinition, cpEquals - cpDefinition);
						std::string val(cpEquals+1);
						size_t bracket = name.find('(');
						size_t bracketEnd = name.find(')');
						if ((bracket != std::string::npos) && (bracketEnd != std::string::npos)) {
							// Macro
							std::string args = name.substr(bracket + 1, bracketEnd - bracket - 1);
							name = name.substr(0, bracket);
							preprocessorDefinitionsStart[name] = SymbolValue(val, args);
						} else {
							preprocessorDefinitionsStart[name] = val;
						}
					} else {
						std::string name(cpDefinition);
						std::string val("1");
						preprocessorDefinitionsStart[name] = val;
					}
				}
			}
		}
	}
	return firstModification;
}

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '\''|| ch == '$');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '$');
}

static inline bool AllUpperCase(const char *a) {
	while (*a) {
		if (*a >= 'a' && *a <= 'z') return false;
		a++;
	}
	return true;
}

// Functor used to truncate history
struct After {
	Sci_Position line;
	explicit After(Sci_Position line_) : line(line_) {}
	bool operator()(PPDefinition &p) const {
		return p.line > line;
	}
};

static std::string GetRestOfLine(LexAccessor &styler, Sci_Position start, bool allowSpace) {
	std::string restOfLine;
	Sci_Position i =0;
	char ch = styler.SafeGetCharAt(start, '\n');
	Sci_Position endLine = styler.LineEnd(styler.GetLine(start));
	while (((start+i) < endLine) && (ch != '\r')) {
		char chNext = styler.SafeGetCharAt(start + i + 1, '\n');
		if (ch == '/' && (chNext == '/' || chNext == '*'))
			break;
		if (allowSpace || (ch != ' '))
			restOfLine += ch;
		i++;
		ch = chNext;
	}
	return restOfLine;
}

static bool IsSpaceOrTab(int ch) {
	return ch == ' ' || ch == '\t';
}

void SCI_METHOD LexerVerilog::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess)
{
	LexAccessor styler(pAccess);

	const int kwOther=0, kwDot=0x100, kwInput=0x200, kwOutput=0x300, kwInout=0x400, kwProtected=0x800;
	int lineState = kwOther;
	bool continuationLine = false;

	Sci_Position curLine = styler.GetLine(startPos);
	if (curLine > 0) lineState = styler.GetLineState(curLine - 1);

	// Do not leak onto next line
	if (initStyle == SCE_V_STRINGEOL)
		initStyle = SCE_V_DEFAULT;

	if ((MaskActive(initStyle) == SCE_V_PREPROCESSOR) ||
			(MaskActive(initStyle) == SCE_V_COMMENTLINE) ||
			(MaskActive(initStyle) == SCE_V_COMMENTLINEBANG)) {
		// Set continuationLine if last character of previous line is '\'
		if (curLine > 0) {
			Sci_Position endLinePrevious = styler.LineEnd(curLine - 1);
			if (endLinePrevious > 0) {
				continuationLine = styler.SafeGetCharAt(endLinePrevious-1) == '\\';
			}
		}
	}

	StyleContext sc(startPos, length, initStyle, styler);
	LinePPState preproc = vlls.ForLine(curLine);

	bool definitionsChanged = false;

	// Truncate ppDefineHistory before current line

	if (!options.updatePreprocessor)
		ppDefineHistory.clear();

	std::vector<PPDefinition>::iterator itInvalid = std::find_if(ppDefineHistory.begin(), ppDefineHistory.end(), After(curLine-1));
	if (itInvalid != ppDefineHistory.end()) {
		ppDefineHistory.erase(itInvalid, ppDefineHistory.end());
		definitionsChanged = true;
	}

	SymbolTable preprocessorDefinitions = preprocessorDefinitionsStart;
	for (std::vector<PPDefinition>::iterator itDef = ppDefineHistory.begin(); itDef != ppDefineHistory.end(); ++itDef) {
		if (itDef->isUndef)
			preprocessorDefinitions.erase(itDef->key);
		else
			preprocessorDefinitions[itDef->key] = SymbolValue(itDef->value, itDef->arguments);
	}

	int activitySet = preproc.IsInactive() ? activeFlag : 0;
	Sci_Position lineEndNext = styler.LineEnd(curLine);
	bool isEscapedId = false;    // true when parsing an escaped Identifier
	bool isProtected = (lineState&kwProtected) != 0;	// true when parsing a protected region

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineStart) {
			if (sc.state == SCE_V_STRING) {
				// Prevent SCE_V_STRINGEOL from leaking back to previous line
				sc.SetState(SCE_V_STRING);
			}
			if ((MaskActive(sc.state) == SCE_V_PREPROCESSOR) && (!continuationLine)) {
				sc.SetState(SCE_V_DEFAULT|activitySet);
			}
			if (preproc.IsInactive()) {
				activitySet = activeFlag;
				sc.SetState(sc.state | activitySet);
			}
		}

		if (sc.MatchLineEnd()) {
			curLine++;
			lineEndNext = styler.LineEnd(curLine);
			vlls.Add(curLine, preproc);
			// Update the line state, so it can be seen by next line
			styler.SetLineState(curLine, lineState);
			isEscapedId = false;    // EOL terminates an escaped Identifier
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (static_cast<Sci_Position>((sc.currentPos+1)) >= lineEndNext) {
				curLine++;
				lineEndNext = styler.LineEnd(curLine);
				vlls.Add(curLine, preproc);
				// Update the line state, so it can be seen by next line
				styler.SetLineState(curLine, lineState);
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					// Even in UTF-8, \r and \n are separate
					sc.Forward();
				}
				continuationLine = true;
				sc.Forward();
				continue;
			}
		}

		// for comment keyword
		if (MaskActive(sc.state) == SCE_V_COMMENT_WORD && !IsAWordChar(sc.ch)) {
			char s[100];
			int state = lineState & 0xff;
			sc.GetCurrent(s, sizeof(s));
			if (keywords5.InList(s)) {
				sc.ChangeState(SCE_V_COMMENT_WORD|activitySet);
			} else {
				sc.ChangeState(state|activitySet);
			}
			sc.SetState(state|activitySet);
		}

		const bool atLineEndBeforeSwitch = sc.MatchLineEnd();

		// Determine if the current state should terminate.
		switch (MaskActive(sc.state)) {
			case SCE_V_OPERATOR:
				sc.SetState(SCE_V_DEFAULT|activitySet);
				break;
			case SCE_V_NUMBER:
				if (!(IsAWordChar(sc.ch) || (sc.ch == '?'))) {
					sc.SetState(SCE_V_DEFAULT|activitySet);
				}
				break;
			case SCE_V_IDENTIFIER:
				if (!isEscapedId &&(!IsAWordChar(sc.ch) || (sc.ch == '.'))) {
					char s[100];
					lineState &= 0xff00;
					sc.GetCurrent(s, sizeof(s));
					if (options.portStyling && (strcmp(s, "input") == 0)) {
						lineState = kwInput;
						sc.ChangeState(SCE_V_INPUT|activitySet);
					} else if (options.portStyling && (strcmp(s, "output") == 0)) {
						lineState = kwOutput;
						sc.ChangeState(SCE_V_OUTPUT|activitySet);
					} else if (options.portStyling && (strcmp(s, "inout") == 0)) {
						lineState = kwInout;
						sc.ChangeState(SCE_V_INOUT|activitySet);
					} else if (lineState == kwInput) {
						sc.ChangeState(SCE_V_INPUT|activitySet);
					} else if (lineState == kwOutput) {
						sc.ChangeState(SCE_V_OUTPUT|activitySet);
					} else if (lineState == kwInout) {
						sc.ChangeState(SCE_V_INOUT|activitySet);
					} else if (lineState == kwDot) {
						lineState = kwOther;
						if (options.portStyling)
							sc.ChangeState(SCE_V_PORT_CONNECT|activitySet);
					} else if (keywords.InList(s)) {
						sc.ChangeState(SCE_V_WORD|activitySet);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_V_WORD2|activitySet);
					} else if (keywords3.InList(s)) {
						sc.ChangeState(SCE_V_WORD3|activitySet);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_V_USER|activitySet);
					} else if (options.allUppercaseDocKeyword && AllUpperCase(s)) {
						sc.ChangeState(SCE_V_USER|activitySet);
					}
					sc.SetState(SCE_V_DEFAULT|activitySet);
				}
				break;
			case SCE_V_PREPROCESSOR:
				if (!IsAWordChar(sc.ch) || sc.MatchLineEnd()) {
					sc.SetState(SCE_V_DEFAULT|activitySet);
				}
				break;
			case SCE_V_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_V_DEFAULT|activitySet);
				} else if (IsAWordStart(sc.ch)) {
					lineState = sc.state | (lineState & 0xff00);
					sc.SetState(SCE_V_COMMENT_WORD|activitySet);
				}
				break;
			case SCE_V_COMMENTLINE:
			case SCE_V_COMMENTLINEBANG:
				if (sc.atLineStart) {
					sc.SetState(SCE_V_DEFAULT|activitySet);
				} else if (IsAWordStart(sc.ch)) {
					lineState = sc.state | (lineState & 0xff00);
					sc.SetState(SCE_V_COMMENT_WORD|activitySet);
				}
				break;
			case SCE_V_STRING:
				if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_V_DEFAULT|activitySet);
				} else if (sc.MatchLineEnd()) {
					sc.ChangeState(SCE_V_STRINGEOL|activitySet);
					if (sc.Match('\r', '\n'))
						sc.Forward();
					sc.ForwardSetState(SCE_V_DEFAULT|activitySet);
				}
				break;
		}

		if (sc.MatchLineEnd() && !atLineEndBeforeSwitch) {
			// State exit processing consumed characters up to end of line.
			curLine++;
			lineEndNext = styler.LineEnd(curLine);
			vlls.Add(curLine, preproc);
			// Update the line state, so it can be seen by next line
			styler.SetLineState(curLine, lineState);
			isEscapedId = false;    // EOL terminates an escaped Identifier
		}

		// Determine if a new state should be entered.
		if (MaskActive(sc.state) == SCE_V_DEFAULT) {
			if (sc.ch == '`') {
				sc.SetState(SCE_V_PREPROCESSOR|activitySet);
				// Skip whitespace between ` and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.MatchLineEnd()) {
					sc.SetState(SCE_V_DEFAULT|activitySet);
					styler.SetLineState(curLine, lineState);
				} else {
					if (sc.Match("protected")) {
						isProtected = true;
						lineState |= kwProtected;
						styler.SetLineState(curLine, lineState);
					} else if (sc.Match("endprotected")) {
						isProtected = false;
						lineState &= ~kwProtected;
						styler.SetLineState(curLine, lineState);
					} else if (!isProtected && options.trackPreprocessor) {
						if (sc.Match("ifdef") || sc.Match("ifndef")) {
							bool isIfDef = sc.Match("ifdef");
							int i = isIfDef ? 5 : 6;
							std::string restOfLine = GetRestOfLine(styler, sc.currentPos + i + 1, false);
							bool foundDef = preprocessorDefinitions.find(restOfLine) != preprocessorDefinitions.end();
							preproc.StartSection(isIfDef == foundDef);
						} else if (sc.Match("else")) {
							if (!preproc.CurrentIfTaken()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? activeFlag : 0;
								if (!activitySet) {
									sc.ChangeState(SCE_V_PREPROCESSOR|activitySet);
								}
							} else if (!preproc.IsInactive()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? activeFlag : 0;
								if (!activitySet) {
									sc.ChangeState(SCE_V_PREPROCESSOR|activitySet);
								}
							}
						} else if (sc.Match("elsif")) {
							// Ensure only one chosen out of `if .. `elsif .. `elsif .. `else .. `endif
							if (!preproc.CurrentIfTaken()) {
								// Similar to `ifdef
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 6, true);
								bool ifGood = preprocessorDefinitions.find(restOfLine) != preprocessorDefinitions.end();
								if (ifGood) {
									preproc.InvertCurrentLevel();
									activitySet = preproc.IsInactive() ? activeFlag : 0;
									if (!activitySet)
										sc.ChangeState(SCE_V_PREPROCESSOR|activitySet);
								}
							} else if (!preproc.IsInactive()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? activeFlag : 0;
								if (!activitySet)
									sc.ChangeState(SCE_V_PREPROCESSOR|activitySet);
							}
						} else if (sc.Match("endif")) {
							preproc.EndSection();
							activitySet = preproc.IsInactive() ? activeFlag : 0;
							sc.ChangeState(SCE_V_PREPROCESSOR|activitySet);
						} else if (sc.Match("define")) {
							if (options.updatePreprocessor && !preproc.IsInactive()) {
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 6, true);
								size_t startName = 0;
								while ((startName < restOfLine.length()) && IsSpaceOrTab(restOfLine[startName]))
									startName++;
								size_t endName = startName;
								while ((endName < restOfLine.length()) && setWord.Contains(static_cast<unsigned char>(restOfLine[endName])))
									endName++;
								std::string key = restOfLine.substr(startName, endName-startName);
								if ((endName < restOfLine.length()) && (restOfLine.at(endName) == '(')) {
									// Macro
									size_t endArgs = endName;
									while ((endArgs < restOfLine.length()) && (restOfLine[endArgs] != ')'))
										endArgs++;
									std::string args = restOfLine.substr(endName + 1, endArgs - endName - 1);
									size_t startValue = endArgs+1;
									while ((startValue < restOfLine.length()) && IsSpaceOrTab(restOfLine[startValue]))
										startValue++;
									std::string value;
									if (startValue < restOfLine.length())
										value = restOfLine.substr(startValue);
									preprocessorDefinitions[key] = SymbolValue(value, args);
									ppDefineHistory.push_back(PPDefinition(curLine, key, value, false, args));
									definitionsChanged = true;
								} else {
									// Value
									size_t startValue = endName;
									while ((startValue < restOfLine.length()) && IsSpaceOrTab(restOfLine[startValue]))
										startValue++;
									std::string value = restOfLine.substr(startValue);
									preprocessorDefinitions[key] = value;
									ppDefineHistory.push_back(PPDefinition(curLine, key, value));
									definitionsChanged = true;
								}
							}
						} else if (sc.Match("undefineall")) {
							if (options.updatePreprocessor && !preproc.IsInactive()) {
								// remove all preprocessor definitions
								std::map<std::string, SymbolValue>::iterator itDef;
								for(itDef = preprocessorDefinitions.begin(); itDef != preprocessorDefinitions.end(); ++itDef) {
									ppDefineHistory.push_back(PPDefinition(curLine, itDef->first, "", true));
								}
								preprocessorDefinitions.clear();
								definitionsChanged = true;
							}
						} else if (sc.Match("undef")) {
							if (options.updatePreprocessor && !preproc.IsInactive()) {
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 5, true);
								std::vector<std::string> tokens = Tokenize(restOfLine);
								std::string key;
								if (tokens.size() >= 1) {
									key = tokens[0];
									preprocessorDefinitions.erase(key);
									ppDefineHistory.push_back(PPDefinition(curLine, key, "", true));
									definitionsChanged = true;
								}
							}
						}
					}
				}
			} else if (!isProtected) {
				if (IsADigit(sc.ch) || (sc.ch == '\'') || (sc.ch == '.' && IsADigit(sc.chNext))) {
					sc.SetState(SCE_V_NUMBER|activitySet);
				} else if (IsAWordStart(sc.ch)) {
					sc.SetState(SCE_V_IDENTIFIER|activitySet);
				} else if (sc.Match('/', '*')) {
					sc.SetState(SCE_V_COMMENT|activitySet);
					sc.Forward();	// Eat the * so it isn't used for the end of the comment
				} else if (sc.Match('/', '/')) {
					if (sc.Match("//!"))	// Nice to have a different comment style
						sc.SetState(SCE_V_COMMENTLINEBANG|activitySet);
					else
						sc.SetState(SCE_V_COMMENTLINE|activitySet);
				} else if (sc.ch == '\"') {
					sc.SetState(SCE_V_STRING|activitySet);
				} else if (sc.ch == '\\') {
					// escaped identifier, everything is ok up to whitespace
					isEscapedId = true;
					sc.SetState(SCE_V_IDENTIFIER|activitySet);
				} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '@' || sc.ch == '#') {
					sc.SetState(SCE_V_OPERATOR|activitySet);
					if (sc.ch == '.') lineState = kwDot;
					if (sc.ch == ';') lineState = kwOther;
				}
			}
		}
		if (isEscapedId && isspacechar(sc.ch)) {
			isEscapedId = false;
		}
	}
	if (definitionsChanged) {
		styler.ChangeLexerState(startPos, startPos + length);
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_V_COMMENT;
}

static bool IsCommentLine(Sci_Position line, LexAccessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eolPos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' &&
		   (style == SCE_V_COMMENTLINE || style == SCE_V_COMMENTLINEBANG)) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
void SCI_METHOD LexerVerilog::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess)
{
	LexAccessor styler(pAccess);
	bool foldAtBrace  = 1;
	bool foldAtParenthese  = 1;

	Sci_Position lineCurrent = styler.GetLine(startPos);
	// Move back one line to be compatible with LexerModule::Fold behavior, fixes problem with foldComment behavior
	if (lineCurrent > 0) {
		lineCurrent--;
		Sci_Position newStartPos = styler.LineStart(lineCurrent);
		length += startPos - newStartPos;
		startPos = newStartPos;
		initStyle = 0;
		if (startPos > 0) {
			initStyle = styler.StyleAt(startPos - 1);
		}
	}
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = MaskActive(styler.StyleAt(startPos));
	int style = MaskActive(initStyle);

	// restore fold state (if it exists) for prior line
	int stateCurrent = 0;
	std::map<Sci_Position,int>::iterator foldStateIterator = foldState.find(lineCurrent-1);
	if (foldStateIterator != foldState.end()) {
		stateCurrent = foldStateIterator->second;
	}

	// remove all foldState entries after lineCurrent-1
	foldStateIterator = foldState.upper_bound(lineCurrent-1);
	if (foldStateIterator != foldState.end()) {
		foldState.erase(foldStateIterator, foldState.end());
	}

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = MaskActive(styler.StyleAt(i + 1));
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (!(stateCurrent & protectedFlag)) {
			if (options.foldComment && IsStreamCommentStyle(style)) {
				if (!IsStreamCommentStyle(stylePrev)) {
					levelNext++;
				} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
					// Comments don't end at end of line and the next character may be unstyled.
					levelNext--;
				}
			}
			if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler))
			{
				if (!IsCommentLine(lineCurrent - 1, styler)
					&& IsCommentLine(lineCurrent + 1, styler))
					levelNext++;
				else if (IsCommentLine(lineCurrent - 1, styler)
						 && !IsCommentLine(lineCurrent+1, styler))
					levelNext--;
			}
			if (options.foldComment && (style == SCE_V_COMMENTLINE)) {
				if ((ch == '/') && (chNext == '/')) {
					char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{') {
						levelNext++;
					} else if (chNext2 == '}') {
						levelNext--;
					}
				}
			}
		}
		if (ch == '`') {
			Sci_PositionU j = i + 1;
			while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
				j++;
			}
			if (styler.Match(j, "protected")) {
				stateCurrent |= protectedFlag;
				levelNext++;
			} else if (styler.Match(j, "endprotected")) {
				stateCurrent &= ~protectedFlag;
				levelNext--;
			} else if (!(stateCurrent & protectedFlag) && options.foldPreprocessor && (style == SCE_V_PREPROCESSOR)) {
				if (styler.Match(j, "if")) {
					if (options.foldPreprocessorElse) {
						// Measure the minimum before a begin to allow
						// folding on "end else begin"
						if (levelMinCurrent > levelNext) {
							levelMinCurrent = levelNext;
						}
					}
					levelNext++;
				} else if (options.foldPreprocessorElse && styler.Match(j, "else")) {
					levelNext--;
					if (levelMinCurrent > levelNext) {
						levelMinCurrent = levelNext;
					}
					levelNext++;
				} else if (options.foldPreprocessorElse && styler.Match(j, "elsif")) {
					levelNext--;
					// Measure the minimum before a begin to allow
					// folding on "end else begin"
					if (levelMinCurrent > levelNext) {
						levelMinCurrent = levelNext;
					}
					levelNext++;
				} else if (styler.Match(j, "endif")) {
					levelNext--;
				}
			}
		}
		if (style == SCE_V_OPERATOR) {
			if (foldAtParenthese) {
				if (ch == '(') {
					levelNext++;
				} else if (ch == ')') {
					levelNext--;
				}
			}
			// semicolons terminate external declarations
			if (ch == ';') {
				// extern and pure virtual declarations terminated by semicolon
				if (stateCurrent & foldExternFlag) {
					levelNext--;
					stateCurrent &= ~foldExternFlag;
				}
				// wait and disable statements terminated by semicolon
				if (stateCurrent & foldWaitDisableFlag) {
					stateCurrent &= ~foldWaitDisableFlag;
				}
				// typedef statements terminated by semicolon
				if (stateCurrent & typedefFlag) {
					stateCurrent &= ~typedefFlag;
				}
			}
			// wait and disable statements containing '(' will not contain "fork" keyword, special processing is not needed
			if (ch == '(') {
				if (stateCurrent & foldWaitDisableFlag) {
					stateCurrent &= ~foldWaitDisableFlag;
				}
			}
		}
		if (style == SCE_V_OPERATOR) {
			if (foldAtBrace) {
				if (ch == '{') {
					levelNext++;
				} else if (ch == '}') {
					levelNext--;
				}
			}
		}
		if (style == SCE_V_WORD && stylePrev != SCE_V_WORD) {
			Sci_PositionU j = i;
			if (styler.Match(j, "case") ||
				styler.Match(j, "casex") ||
				styler.Match(j, "casez") ||
				styler.Match(j, "covergroup") ||
				styler.Match(j, "function") ||
				styler.Match(j, "generate") ||
				styler.Match(j, "interface") ||
				styler.Match(j, "package") ||
				styler.Match(j, "primitive") ||
				styler.Match(j, "program") ||
				styler.Match(j, "sequence") ||
				styler.Match(j, "specify") ||
				styler.Match(j, "table") ||
				styler.Match(j, "task") ||
				(styler.Match(j, "module") && options.foldAtModule)) {
				levelNext++;
			} else if (styler.Match(j, "begin")) {
				// Measure the minimum before a begin to allow
				// folding on "end else begin"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (styler.Match(j, "class")) {
				// class does not introduce a block when used in a typedef statement
				if (!(stateCurrent & typedefFlag))
					levelNext++;
			} else if (styler.Match(j, "fork")) {
				// fork does not introduce a block when used in a wait or disable statement
				if (stateCurrent & foldWaitDisableFlag) {
					stateCurrent &= ~foldWaitDisableFlag;
				} else
					levelNext++;
			} else if (styler.Match(j, "endcase") ||
				styler.Match(j, "endclass") ||
				styler.Match(j, "endfunction") ||
				styler.Match(j, "endgenerate") ||
				styler.Match(j, "endgroup") ||
				styler.Match(j, "endinterface") ||
				styler.Match(j, "endpackage") ||
				styler.Match(j, "endprimitive") ||
				styler.Match(j, "endprogram") ||
				styler.Match(j, "endsequence") ||
				styler.Match(j, "endspecify") ||
				styler.Match(j, "endtable") ||
				styler.Match(j, "endtask") ||
				styler.Match(j, "join") ||
				styler.Match(j, "join_any") ||
				styler.Match(j, "join_none") ||
				(styler.Match(j, "endmodule") && options.foldAtModule) ||
				(styler.Match(j, "end") && !IsAWordChar(styler.SafeGetCharAt(j + 3)))) {
				levelNext--;
			} else if (styler.Match(j, "extern") ||
				styler.Match(j, "pure")) {
				// extern and pure virtual functions/tasks are terminated by ';' not endfunction/endtask
				stateCurrent |= foldExternFlag;
			} else if (styler.Match(j, "disable") ||
				styler.Match(j, "wait")) {
				// fork does not introduce a block when used in a wait or disable statement
				stateCurrent |= foldWaitDisableFlag;
			} else if (styler.Match(j, "typedef")) {
				stateCurrent |= typedefFlag;
			}
		}
		if (atEOL) {
			int levelUse = levelCurrent;
			if (options.foldAtElse||options.foldPreprocessorElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (stateCurrent) {
				foldState[lineCurrent] = stateCurrent;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
}

std::vector<std::string> LexerVerilog::Tokenize(const std::string &expr) const {
	// Break into tokens
	std::vector<std::string> tokens;
	const char *cp = expr.c_str();
	while (*cp) {
		std::string word;
		if (setWord.Contains(static_cast<unsigned char>(*cp))) {
			// Identifiers and numbers
			while (setWord.Contains(static_cast<unsigned char>(*cp))) {
				word += *cp;
				cp++;
			}
		} else if (IsSpaceOrTab(*cp)) {
			while (IsSpaceOrTab(*cp)) {
				cp++;
			}
			continue;
		} else {
			// Should handle strings, characters, and comments here
			word += *cp;
			cp++;
		}
		tokens.push_back(word);
	}
	return tokens;
}

static const char * const verilogWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "System Tasks",
            "User defined tasks and identifiers",
            "Documentation comment keywords",
            "Preprocessor definitions",
            0,
        };

LexerModule lmVerilog(SCLEX_VERILOG, LexerVerilog::LexerFactoryVerilog, "verilog", verilogWordLists);
