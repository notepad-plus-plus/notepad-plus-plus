// Scintilla source code edit control
/** @file LexCPP.cxx
 ** Lexer for C++, C, Java, and JavaScript.
 ** Further folding features and configuration properties added by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <string>
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
#include "SparseState.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool IsSpaceEquiv(int state) {
	return (state <= SCE_C_COMMENTDOC) ||
		// including SCE_C_DEFAULT, SCE_C_COMMENT, SCE_C_COMMENTLINE
		(state == SCE_C_COMMENTLINEDOC) || (state == SCE_C_COMMENTDOCKEYWORD) ||
		(state == SCE_C_COMMENTDOCKEYWORDERROR);
}

// Preconditions: sc.currentPos points to a character after '+' or '-'.
// The test for pos reaching 0 should be redundant,
// and is in only for safety measures.
// Limitation: this code will give the incorrect answer for code like
// a = b+++/ptn/...
// Putting a space between the '++' post-inc operator and the '+' binary op
// fixes this, and is highly recommended for readability anyway.
static bool FollowsPostfixOperator(StyleContext &sc, LexAccessor &styler) {
	int pos = (int) sc.currentPos;
	while (--pos > 0) {
		char ch = styler[pos];
		if (ch == '+' || ch == '-') {
			return styler[pos - 1] == ch;
		}
	}
	return false;
}

static bool followsReturnKeyword(StyleContext &sc, LexAccessor &styler) {
	// Don't look at styles, so no need to flush.
	int pos = (int) sc.currentPos;
	int currentLine = styler.GetLine(pos);
	int lineStartPos = styler.LineStart(currentLine);
	char ch;
	while (--pos > lineStartPos) {
		ch = styler.SafeGetCharAt(pos);
		if (ch != ' ' && ch != '\t') {
			break;
		}
	}
	const char *retBack = "nruter";
	const char *s = retBack;
	while (*s
		&& pos >= lineStartPos
		&& styler.SafeGetCharAt(pos) == *s) {
		s++;
		pos--;
	}
	return !*s;
}

static std::string GetRestOfLine(LexAccessor &styler, int start, bool allowSpace) {
	std::string restOfLine;
	int i =0;
	char ch = styler.SafeGetCharAt(start + i, '\n');
	while ((ch != '\r') && (ch != '\n')) {
		if (allowSpace || (ch != ' '))
			restOfLine += ch;
		i++;
		ch = styler.SafeGetCharAt(start + i, '\n');
	}
	return restOfLine;
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_C_COMMENT ||
		style == SCE_C_COMMENTDOC ||
		style == SCE_C_COMMENTDOCKEYWORD ||
		style == SCE_C_COMMENTDOCKEYWORDERROR;
}

static std::vector<std::string> Tokenize(const std::string &s) {
	// Break into space separated tokens
	std::string word;
	std::vector<std::string> tokens;
	for (const char *cp = s.c_str(); *cp; cp++) {
		if ((*cp == ' ') || (*cp == '\t')) {
			if (!word.empty()) {
				tokens.push_back(word);
				word = "";
			}
		} else {
			word += *cp;
		}
	}
	if (!word.empty()) {
		tokens.push_back(word);
	}
	return tokens;
}

struct PPDefinition {
	int line;
	std::string key;
	std::string value;
	PPDefinition(int line_, const std::string &key_, const std::string &value_) :
		line(line_), key(key_), value(value_) {
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
		return 1 << level;
	}
public:
	LinePPState() : state(0), ifTaken(0), level(-1) {
	}
	bool IsInactive() const {
		return state != 0;
	}
	bool CurrentIfTaken() {
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
	LinePPState ForLine(int line) {
		if ((line > 0) && (vlls.size() > static_cast<size_t>(line))) {
			return vlls[line];
		} else {
			return LinePPState();
		}
	}
	void Add(int line, LinePPState lls) {
		vlls.resize(line+1);
		vlls[line] = lls;
	}
};

// An individual named option for use in an OptionSet

// Options used for LexerCPP
struct OptionsCPP {
	bool stylingWithinPreprocessor;
	bool identifiersAllowDollars;
	bool trackPreprocessor;
	bool updatePreprocessor;
	bool fold;
	bool foldSyntaxBased;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldPreprocessor;
	bool foldCompact;
	bool foldAtElse;
	OptionsCPP() {
		stylingWithinPreprocessor = false;
		identifiersAllowDollars = true;
		trackPreprocessor = true;
		updatePreprocessor = true;
		fold = false;
		foldSyntaxBased = true;
		foldComment = false;
		foldCommentMultiline = true;
		foldCommentExplicit = true;
		foldExplicitStart = "";
		foldExplicitEnd = "";
		foldExplicitAnywhere = false;
		foldPreprocessor = false;
		foldCompact = false;
		foldAtElse = false;
	}
};

static const char *const cppWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Global classes and typedefs",
            "Preprocessor definitions",
            0,
};

struct OptionSetCPP : public OptionSet<OptionsCPP> {
	OptionSetCPP() {
		DefineProperty("styling.within.preprocessor", &OptionsCPP::stylingWithinPreprocessor,
			"For C++ code, determines whether all preprocessor code is styled in the "
			"preprocessor style (0, the default) or only from the initial # to the end "
			"of the command word(1).");

		DefineProperty("lexer.cpp.allow.dollars", &OptionsCPP::identifiersAllowDollars,
			"Set to 0 to disallow the '$' character in identifiers with the cpp lexer.");

		DefineProperty("lexer.cpp.track.preprocessor", &OptionsCPP::trackPreprocessor,
			"Set to 1 to interpret #if/#else/#endif to grey out code that is not active.");

		DefineProperty("lexer.cpp.update.preprocessor", &OptionsCPP::updatePreprocessor,
			"Set to 1 to update preprocessor definitions when #define found.");

		DefineProperty("fold", &OptionsCPP::fold);

		DefineProperty("fold.cpp.syntax.based", &OptionsCPP::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.comment", &OptionsCPP::foldComment,
			"This option enables folding multi-line comments and explicit fold points when using the C++ lexer. "
			"Explicit fold points allows adding extra folding by placing a //{ comment at the start and a //} "
			"at the end of a section that should fold.");

		DefineProperty("fold.cpp.comment.multiline", &OptionsCPP::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

		DefineProperty("fold.cpp.comment.explicit", &OptionsCPP::foldCommentExplicit,
			"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

		DefineProperty("fold.cpp.explicit.start", &OptionsCPP::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard //{.");

		DefineProperty("fold.cpp.explicit.end", &OptionsCPP::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard //}.");

		DefineProperty("fold.cpp.explicit.anywhere", &OptionsCPP::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.preprocessor", &OptionsCPP::foldPreprocessor,
			"This option enables folding preprocessor directives when using the C++ lexer. "
			"Includes C#'s explicit #region and #endregion folding directives.");

		DefineProperty("fold.compact", &OptionsCPP::foldCompact);

		DefineProperty("fold.at.else", &OptionsCPP::foldAtElse,
			"This option enables C++ folding on a \"} else {\" line of an if statement.");

		DefineWordListSets(cppWordLists);
	}
};

class LexerCPP : public ILexer {
	bool caseSensitive;
	CharacterSet setWord;
	CharacterSet setNegationOp;
	CharacterSet setArithmethicOp;
	CharacterSet setRelOp;
	CharacterSet setLogicalOp;
	PPStates vlls;
	std::vector<PPDefinition> ppDefineHistory;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList ppDefinitions;
	std::map<std::string, std::string> preprocessorDefinitionsStart;
	OptionsCPP options;
	OptionSetCPP osCPP;
	SparseState<std::string> rawStringTerminators;
public:
	LexerCPP(bool caseSensitive_) :
		caseSensitive(caseSensitive_),
		setWord(CharacterSet::setAlphaNum, "._", 0x80, true),
		setNegationOp(CharacterSet::setNone, "!"),
		setArithmethicOp(CharacterSet::setNone, "+-/*%"),
		setRelOp(CharacterSet::setNone, "=!<>"),
		setLogicalOp(CharacterSet::setNone, "|&") {
	}
	~LexerCPP() {
	}
	void SCI_METHOD Release() {
		delete this;
	}
	int SCI_METHOD Version() const {
		return lvOriginal;
	}
	const char * SCI_METHOD PropertyNames() {
		return osCPP.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) {
		return osCPP.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) {
		return osCPP.DescribeProperty(name);
	}
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets() {
		return osCPP.DescribeWordListSets();
	}
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess);

	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}

	static ILexer *LexerFactoryCPP() {
		return new LexerCPP(true);
	}
	static ILexer *LexerFactoryCPPInsensitive() {
		return new LexerCPP(false);
	}

	void EvaluateTokens(std::vector<std::string> &tokens);
	bool EvaluateExpression(const std::string &expr, const std::map<std::string, std::string> &preprocessorDefinitions);
};

int SCI_METHOD LexerCPP::PropertySet(const char *key, const char *val) {
	if (osCPP.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

int SCI_METHOD LexerCPP::WordListSet(int n, const char *wl) {
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
		wordListN = &ppDefinitions;
		break;
	}
	int firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
			if (n == 4) {
				// Rebuild preprocessorDefinitions
				preprocessorDefinitionsStart.clear();
				for (int nDefinition = 0; nDefinition < ppDefinitions.len; nDefinition++) {
					char *cpDefinition = ppDefinitions.words[nDefinition];
					char *cpEquals = strchr(cpDefinition, '=');
					if (cpEquals) {
						std::string name(cpDefinition, cpEquals - cpDefinition);
						std::string val(cpEquals+1);
						preprocessorDefinitionsStart[name] = val;
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

// Functor used to truncate history
struct After {
	int line;
	After(int line_) : line(line_) {}
	bool operator() (PPDefinition &p) const {
		return p.line > line;
	}
};

void SCI_METHOD LexerCPP::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	CharacterSet setOKBeforeRE(CharacterSet::setNone, "([{=,:;!%^&*|?~+-");
	CharacterSet setCouldBePostOp(CharacterSet::setNone, "+-");

	CharacterSet setDoxygen(CharacterSet::setAlpha, "$@\\&<>#{}[]");

	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);

	if (options.identifiersAllowDollars) {
		setWordStart.Add('$');
		setWord.Add('$');
	}

	int chPrevNonWhite = ' ';
	int visibleChars = 0;
	bool lastWordWasUUID = false;
	int styleBeforeDCKeyword = SCE_C_DEFAULT;
	bool continuationLine = false;
	bool isIncludePreprocessor = false;

	int lineCurrent = styler.GetLine(startPos);
	if ((initStyle == SCE_C_PREPROCESSOR) ||
      (initStyle == SCE_C_COMMENTLINE) ||
      (initStyle == SCE_C_COMMENTLINEDOC)) {
		// Set continuationLine if last character of previous line is '\'
		if (lineCurrent > 0) {
			int chBack = styler.SafeGetCharAt(startPos-1, 0);
			int chBack2 = styler.SafeGetCharAt(startPos-2, 0);
			int lineEndChar = '!';
			if (chBack2 == '\r' && chBack == '\n') {
				lineEndChar = styler.SafeGetCharAt(startPos-3, 0);
			} else if (chBack == '\n' || chBack == '\r') {
				lineEndChar = chBack2;
			}
			continuationLine = lineEndChar == '\\';
		}
	}

	// look back to set chPrevNonWhite properly for better regex colouring
	if (startPos > 0) {
		int back = startPos;
		while (--back && IsSpaceEquiv(styler.StyleAt(back)))
			;
		if (styler.StyleAt(back) == SCE_C_OPERATOR) {
			chPrevNonWhite = styler.SafeGetCharAt(back);
		}
	}

	StyleContext sc(startPos, length, initStyle, styler, 0x7f);
	LinePPState preproc = vlls.ForLine(lineCurrent);

	bool definitionsChanged = false;

	// Truncate ppDefineHistory before current line

	if (!options.updatePreprocessor)
		ppDefineHistory.clear();

	std::vector<PPDefinition>::iterator itInvalid = std::find_if(ppDefineHistory.begin(), ppDefineHistory.end(), After(lineCurrent-1));
	if (itInvalid != ppDefineHistory.end()) {
		ppDefineHistory.erase(itInvalid, ppDefineHistory.end());
		definitionsChanged = true;
	}

	std::map<std::string, std::string> preprocessorDefinitions = preprocessorDefinitionsStart;
	for (std::vector<PPDefinition>::iterator itDef = ppDefineHistory.begin(); itDef != ppDefineHistory.end(); ++itDef) {
		preprocessorDefinitions[itDef->key] = itDef->value;
	}

	const int maskActivity = 0x3F;
	std::string rawStringTerminator = rawStringTerminators.ValueAt(lineCurrent-1);
	SparseState<std::string> rawSTNew(lineCurrent);

	int activitySet = preproc.IsInactive() ? 0x40 : 0;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			if ((sc.state == SCE_C_STRING) || (sc.state == SCE_C_CHARACTER)) {
				// Prevent SCE_C_STRINGEOL from leaking back to previous line which
				// ends with a line continuation by locking in the state upto this position.
				sc.SetState(sc.state);
			}
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
			lastWordWasUUID = false;
			isIncludePreprocessor = false;
			if (preproc.IsInactive()) {
				activitySet = 0x40;
				sc.SetState(sc.state | activitySet);
			}
			if (activitySet) {
				if (sc.ch == '#') {
					if (sc.Match("#else") || sc.Match("#end") || sc.Match("#if")) {
						//activitySet = 0;
					}
				}
			}
		}

		if (sc.atLineEnd) {
			lineCurrent++;
			vlls.Add(lineCurrent, preproc);
			if (rawStringTerminator != "") {
				rawSTNew.Set(lineCurrent-1, rawStringTerminator);
			}
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continuationLine = true;
				continue;
			}
		}

		const bool atLineEndBeforeSwitch = sc.atLineEnd;

		// Determine if the current state should terminate.
		switch (sc.state & maskActivity) {
			case SCE_C_OPERATOR:
				sc.SetState(SCE_C_DEFAULT|activitySet);
				break;
			case SCE_C_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (!(setWord.Contains(sc.ch) || ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_IDENTIFIER:
				if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywords.InList(s)) {
						lastWordWasUUID = strcmp(s, "uuid") == 0;
						sc.ChangeState(SCE_C_WORD|activitySet);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_C_WORD2|activitySet);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_C_GLOBALCLASS|activitySet);
					}
					const bool literalString = sc.ch == '\"';
					if (literalString || sc.ch == '\'') {
						size_t lenS = strlen(s);
						const bool raw = literalString && sc.chPrev == 'R';
						if (raw)
							s[lenS--] = '\0';
						bool valid =
							(lenS == 0) ||
							((lenS == 1) && ((s[0] == 'L') || (s[0] == 'u') || (s[0] == 'U'))) ||
							((lenS == 2) && literalString && (s[0] == 'u') && (s[1] == '8'));
						if (valid) {
							if (literalString)
								sc.ChangeState((raw ? SCE_C_STRINGRAW : SCE_C_STRING)|activitySet);
							else
								sc.ChangeState(SCE_C_CHARACTER|activitySet);
						}
					}
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_PREPROCESSOR:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (options.stylingWithinPreprocessor) {
					if (IsASpace(sc.ch)) {
						sc.SetState(SCE_C_DEFAULT|activitySet);
					}
				} else {
					if (sc.Match('/', '*') || sc.Match('/', '/')) {
						sc.SetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD|activitySet);
					}
				}
				break;
			case SCE_C_COMMENTLINE:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_COMMENTLINEDOC:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTLINEDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD|activitySet);
					}
				}
				break;
			case SCE_C_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_C_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				} else if (!setDoxygen.Contains(sc.ch)) {
					char s[100];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (!IsASpace(sc.ch) || !keywords3.InList(s + 1)) {
						sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR|activitySet);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_C_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL|activitySet);
				} else if (isIncludePreprocessor) {
					if (sc.ch == '>') {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
						isIncludePreprocessor = false;
					}
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_STRINGRAW:
				if (sc.Match(rawStringTerminator.c_str())) {
					for (size_t termPos=rawStringTerminator.size(); termPos; termPos--)
						sc.Forward();
					sc.SetState(SCE_C_DEFAULT|activitySet);
					rawStringTerminator = "";
				}
				break;
			case SCE_C_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL|activitySet);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_REGEX:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '/') {
					sc.Forward();
					while ((sc.ch < 0x80) && islower(sc.ch))
						sc.Forward();    // gobble regex flags
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '\\') {
					// Gobble up the quoted character
					if (sc.chNext == '\\' || sc.chNext == '/') {
						sc.Forward();
					}
				}
				break;
			case SCE_C_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_VERBATIM:
				if (sc.ch == '\"') {
					if (sc.chNext == '\"') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_TRIPLEVERBATIM:
				if (sc.Match ("\"\"\"")) {
					while (sc.Match('"')) {
						sc.Forward();
					}
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_UUID:
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ')') {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
		}

		if (sc.atLineEnd && !atLineEndBeforeSwitch) {
			// State exit processing consumed characters up to end of line.
			lineCurrent++;
			vlls.Add(lineCurrent, preproc);
		}

		// Determine if a new state should be entered.
		if ((sc.state & maskActivity) == SCE_C_DEFAULT) {
			if (sc.Match('@', '\"')) {
				sc.SetState(SCE_C_VERBATIM|activitySet);
				sc.Forward();
			} else if (sc.Match("\"\"\"")) {
				sc.SetState(SCE_C_TRIPLEVERBATIM|activitySet);
				sc.Forward(2);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID|activitySet);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_NUMBER|activitySet);
				}
			} else if (setWordStart.Contains(sc.ch) || (sc.ch == '@')) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID|activitySet);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_IDENTIFIER|activitySet);
				}
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {	// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTDOC|activitySet);
				} else {
					sc.SetState(SCE_C_COMMENT|activitySet);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTLINEDOC|activitySet);
				else
					sc.SetState(SCE_C_COMMENTLINE|activitySet);
			} else if (sc.ch == '/'
				   && (setOKBeforeRE.Contains(chPrevNonWhite)
				       || followsReturnKeyword(sc, styler))
				   && (!setCouldBePostOp.Contains(chPrevNonWhite)
				       || !FollowsPostfixOperator(sc, styler))) {
				sc.SetState(SCE_C_REGEX|activitySet);	// JavaScript's RegEx
			} else if (sc.ch == '\"') {
				if (sc.chPrev == 'R') {
					sc.SetState(SCE_C_STRINGRAW|activitySet);
					rawStringTerminator = ")";
					for (int termPos = sc.currentPos + 1;;termPos++) {
						char chTerminator = styler.SafeGetCharAt(termPos, '(');
						if (chTerminator == '(')
							break;
						rawStringTerminator += chTerminator;
					}
					rawStringTerminator += '\"';
				} else {
					sc.SetState(SCE_C_STRING|activitySet);
				}
				isIncludePreprocessor = false;	// ensure that '>' won't end the string
			} else if (isIncludePreprocessor && sc.ch == '<') {
				sc.SetState(SCE_C_STRING|activitySet);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_C_CHARACTER|activitySet);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_C_PREPROCESSOR|activitySet);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.Match("include")) {
					isIncludePreprocessor = true;
				} else {
					if (options.trackPreprocessor) {
						if (sc.Match("ifdef") || sc.Match("ifndef")) {
							bool isIfDef = sc.Match("ifdef");
							int i = isIfDef ? 5 : 6;
							std::string restOfLine = GetRestOfLine(styler, sc.currentPos + i + 1, false);
							bool foundDef = preprocessorDefinitions.find(restOfLine) != preprocessorDefinitions.end();
							preproc.StartSection(isIfDef == foundDef);
						} else if (sc.Match("if")) {
							std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 2, true);
							bool ifGood = EvaluateExpression(restOfLine, preprocessorDefinitions);
							preproc.StartSection(ifGood);
						} else if (sc.Match("else")) {
							if (!preproc.CurrentIfTaken()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? 0x40 : 0;
								if (!activitySet)
									sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
							} else if (!preproc.IsInactive()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? 0x40 : 0;
								if (!activitySet)
									sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
							}
						} else if (sc.Match("elif")) {
							// Ensure only one chosen out of #if .. #elif .. #elif .. #else .. #endif
							if (!preproc.CurrentIfTaken()) {
								// Similar to #if
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 2, true);
								bool ifGood = EvaluateExpression(restOfLine, preprocessorDefinitions);
								if (ifGood) {
									preproc.InvertCurrentLevel();
									activitySet = preproc.IsInactive() ? 0x40 : 0;
									if (!activitySet)
										sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
								}
							} else if (!preproc.IsInactive()) {
								preproc.InvertCurrentLevel();
								activitySet = preproc.IsInactive() ? 0x40 : 0;
								if (!activitySet)
									sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
							}
						} else if (sc.Match("endif")) {
							preproc.EndSection();
							activitySet = preproc.IsInactive() ? 0x40 : 0;
							sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
						} else if (sc.Match("define")) {
							if (options.updatePreprocessor && !preproc.IsInactive()) {
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 6, true);
								if (restOfLine.find(")") == std::string::npos) {	// Don't handle macros with arguments
									std::vector<std::string> tokens = Tokenize(restOfLine);
									std::string key;
									std::string value("1");
									if (tokens.size() >= 1) {
										key = tokens[0];
										if (tokens.size() >= 2) {
											value = tokens[1];
										}
										preprocessorDefinitions[key] = value;
										ppDefineHistory.push_back(PPDefinition(lineCurrent, key, value));
										definitionsChanged = true;
									}
								}
							}
						}
					}
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_C_OPERATOR|activitySet);
			}
		}

		if (!IsASpace(sc.ch) && !IsSpaceEquiv(sc.state)) {
			chPrevNonWhite = sc.ch;
			visibleChars++;
		}
		continuationLine = false;
	}
	const bool rawStringsChanged = rawStringTerminators.Merge(rawSTNew, lineCurrent);
	if (definitionsChanged || rawStringsChanged)
		styler.ChangeLexerState(startPos, startPos + length);
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerCPP::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldComment && options.foldCommentMultiline && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev) && (stylePrev != SCE_C_COMMENTLINEDOC)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && (styleNext != SCE_C_COMMENTLINEDOC) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_C_COMMENTLINE) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
					levelNext++;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
					levelNext--;
				}
			} else {
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
		if (options.foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
			if (ch == '#') {
				unsigned int j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (styler.Match(j, "region") || styler.Match(j, "if")) {
					levelNext++;
				} else if (styler.Match(j, "end")) {
					levelNext--;
				}
			}
		}
		if (options.foldSyntaxBased && (style == SCE_C_OPERATOR)) {
			if (ch == '{') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && options.foldAtElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			if (atEOL && (i == static_cast<unsigned int>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
	}
}

void LexerCPP::EvaluateTokens(std::vector<std::string> &tokens) {

	// Evaluate defined() statements to either 0 or 1
	for (size_t i=0; (i+2)<tokens.size();) {
		if ((tokens[i] == "defined") && (tokens[i+1] == "(")) {
			const char *val = "0";
			if (tokens[i+2] == ")") {
				// defined()
				tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 3);
			} else if (((i+2)<tokens.size()) && (tokens[i+3] == ")")) {
				// defined(<int>)
				tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 4);
				val = "1";
			}
			tokens[i] = val;
		} else {
			i++;
		}
	}

	// Find bracketed subexpressions and recurse on them
	std::vector<std::string>::iterator itBracket = std::find(tokens.begin(), tokens.end(), "(");
	std::vector<std::string>::iterator itEndBracket = std::find(tokens.begin(), tokens.end(), ")");
	while ((itBracket != tokens.end()) && (itEndBracket != tokens.end()) && (itEndBracket > itBracket)) {
		std::vector<std::string> inBracket(itBracket + 1, itEndBracket);
		EvaluateTokens(inBracket);

		// The insertion is done before the removal because there were failures with the opposite approach
		tokens.insert(itBracket, inBracket.begin(), inBracket.end());
		itBracket = std::find(tokens.begin(), tokens.end(), "(");
		itEndBracket = std::find(tokens.begin(), tokens.end(), ")");
		tokens.erase(itBracket, itEndBracket + 1);

		itBracket = std::find(tokens.begin(), tokens.end(), "(");
		itEndBracket = std::find(tokens.begin(), tokens.end(), ")");
	}

	// Evaluate logical negations
	for (size_t j=0; (j+1)<tokens.size();) {
		if (setNegationOp.Contains(tokens[j][0])) {
			int isTrue = atoi(tokens[j+1].c_str());
			if (tokens[j] == "!")
				isTrue = !isTrue;
			std::vector<std::string>::iterator itInsert =
				tokens.erase(tokens.begin() + j, tokens.begin() + j + 2);
			tokens.insert(itInsert, isTrue ? "1" : "0");
		} else {
			j++;
		}
	}

	// Evaluate expressions in precedence order
	enum precedence { precArithmetic, precRelative, precLogical };
	for (int prec=precArithmetic; prec <= precLogical; prec++) {
		// Looking at 3 tokens at a time so end at 2 before end
		for (size_t k=0; (k+2)<tokens.size();) {
			char chOp = tokens[k+1][0];
			if (
				((prec==precArithmetic) && setArithmethicOp.Contains(chOp)) ||
				((prec==precRelative) && setRelOp.Contains(chOp)) ||
				((prec==precLogical) && setLogicalOp.Contains(chOp))
				) {
				int valA = atoi(tokens[k].c_str());
				int valB = atoi(tokens[k+2].c_str());
				int result = 0;
				if (tokens[k+1] == "+")
					result = valA + valB;
				else if (tokens[k+1] == "-")
					result = valA - valB;
				else if (tokens[k+1] == "*")
					result = valA * valB;
				else if (tokens[k+1] == "/")
					result = valA / (valB ? valB : 1);
				else if (tokens[k+1] == "%")
					result = valA % (valB ? valB : 1);
				else if (tokens[k+1] == "<")
					result = valA < valB;
				else if (tokens[k+1] == "<=")
					result = valA <= valB;
				else if (tokens[k+1] == ">")
					result = valA > valB;
				else if (tokens[k+1] == ">=")
					result = valA >= valB;
				else if (tokens[k+1] == "==")
					result = valA == valB;
				else if (tokens[k+1] == "!=")
					result = valA != valB;
				else if (tokens[k+1] == "||")
					result = valA || valB;
				else if (tokens[k+1] == "&&")
					result = valA && valB;
				char sResult[30];
				sprintf(sResult, "%d", result);
				std::vector<std::string>::iterator itInsert =
					tokens.erase(tokens.begin() + k, tokens.begin() + k + 3);
				tokens.insert(itInsert, sResult);
			} else {
				k++;
			}
		}
	}
}

bool LexerCPP::EvaluateExpression(const std::string &expr, const std::map<std::string, std::string> &preprocessorDefinitions) {
	// Break into tokens, replacing with definitions
	std::string word;
	std::vector<std::string> tokens;
	const char *cp = expr.c_str();
	for (;;) {
		if (setWord.Contains(*cp)) {
			word += *cp;
		} else {
			std::map<std::string, std::string>::const_iterator it = preprocessorDefinitions.find(word);
			if (it != preprocessorDefinitions.end()) {
				tokens.push_back(it->second);
			} else if (!word.empty() && ((word[0] >= '0' && word[0] <= '9') || (word == "defined"))) {
				tokens.push_back(word);
			}
			word = "";
			if (!*cp) {
				break;
			}
			if ((*cp != ' ') && (*cp != '\t')) {
				std::string op(cp, 1);
				if (setRelOp.Contains(*cp)) {
					if (setRelOp.Contains(cp[1])) {
						op += cp[1];
						cp++;
					}
				} else if (setLogicalOp.Contains(*cp)) {
					if (setLogicalOp.Contains(cp[1])) {
						op += cp[1];
						cp++;
					}
				}
				tokens.push_back(op);
			}
		}
		cp++;
	}

	EvaluateTokens(tokens);

	// "0" or "" -> false else true
	bool isFalse = tokens.empty() ||
		((tokens.size() == 1) && ((tokens[0] == "") || tokens[0] == "0"));
	return !isFalse;
}

LexerModule lmCPP(SCLEX_CPP, LexerCPP::LexerFactoryCPP, "cpp", cppWordLists);
LexerModule lmCPPNoCase(SCLEX_CPPNOCASE, LexerCPP::LexerFactoryCPPInsensitive, "cppnocase", cppWordLists);
