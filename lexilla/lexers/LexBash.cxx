// Scintilla source code edit control
/** @file LexBash.cxx
 ** Lexer for Bash.
 **/
// Copyright 2004-2012 by Neil Hodgson <neilh@scintilla.org>
// Adapted from LexPerl by Kein-Hong Man 2004
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
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
#define	BASH_CMD_BODY			0
#define BASH_CMD_START			1
#define BASH_CMD_WORD			2
#define BASH_CMD_TEST			3
#define BASH_CMD_ARITH			4
#define BASH_CMD_DELIM			5

// state constants for nested delimiter pairs, used by
// SCE_SH_STRING and SCE_SH_BACKTICKS processing
#define BASH_DELIM_LITERAL		0
#define BASH_DELIM_STRING		1
#define BASH_DELIM_CSTRING		2
#define BASH_DELIM_LSTRING		3
#define BASH_DELIM_COMMAND		4
#define BASH_DELIM_BACKTICK		5

#define BASH_DELIM_STACK_MAX	7

namespace {

inline int translateBashDigit(int ch) {
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

inline int getBashNumberBase(char *s) {
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

int opposite(int ch) {
	if (ch == '(') return ')';
	if (ch == '[') return ']';
	if (ch == '{') return '}';
	if (ch == '<') return '>';
	return ch;
}

int GlobScan(StyleContext &sc) {
	// forward scan for zsh globs, disambiguate versus bash arrays
	// complex expressions may still fail, e.g. unbalanced () '' "" etc
	int c, sLen = 0;
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
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		if (ch == '#')
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

struct OptionsBash {
	bool fold;
	bool foldComment;
	bool foldCompact;

	OptionsBash() {
		fold = false;
		foldComment = false;
		foldCompact = true;
	}
};

const char * const bashWordListDesc[] = {
	"Keywords",
	0
};

struct OptionSetBash : public OptionSet<OptionsBash> {
	OptionSetBash() {
		DefineProperty("fold", &OptionsBash::fold);

		DefineProperty("fold.comment", &OptionsBash::foldComment);

		DefineProperty("fold.compact", &OptionsBash::foldCompact);

		DefineWordListSets(bashWordListDesc);
	}
};

const char styleSubable[] = { SCE_SH_IDENTIFIER, SCE_SH_SCALAR, 0 };

LexicalClass lexicalClasses[] = {
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
	13, "SCE_SH_HERE_Q", "literal string", "Heredoc quoted string",
};

}

class LexerBash : public DefaultLexer {
	WordList keywords;
	OptionsBash options;
	OptionSetBash osBash;
	enum { ssIdentifier, ssScalar };
	SubStyles subStyles;
public:
	LexerBash() :
		DefaultLexer("bash", SCLEX_BASH, lexicalClasses, ELEMENTS(lexicalClasses)),
		subStyles(styleSubable, 0x80, 0x40, 0) {
	}
	virtual ~LexerBash() {
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
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
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

	static ILexer5 *LexerFactoryBash() {
		return new LexerBash();
	}
};

Sci_Position SCI_METHOD LexerBash::PropertySet(const char *key, const char *val) {
	if (osBash.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerBash::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords;
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

void SCI_METHOD LexerBash::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	WordList cmdDelimiter, bashStruct, bashStruct_in;
	cmdDelimiter.Set("| || |& & && ; ;; ( ) { }");
	bashStruct.Set("if elif fi while until else then do done esac eval");
	bashStruct_in.Set("for case select");

	CharacterSet setWordStart(CharacterSet::setAlpha, "_");
	// note that [+-] are often parts of identifiers in shell scripts
	CharacterSet setWord(CharacterSet::setAlphaNum, "._+-");
	CharacterSet setMetaCharacter(CharacterSet::setNone, "|&;()<> \t\r\n");
	setMetaCharacter.Add(0);
	CharacterSet setBashOperator(CharacterSet::setNone, "^&%()-+=|{}[]:;>,*/<?!.~@");
	CharacterSet setSingleCharOp(CharacterSet::setNone, "rwxoRWXOezsfdlpSbctugkTBMACahGLNn");
	CharacterSet setParam(CharacterSet::setAlphaNum, "$_");
	CharacterSet setHereDoc(CharacterSet::setAlpha, "_\\-+!%*,./:?@[]^`{}~");
	CharacterSet setHereDoc2(CharacterSet::setAlphaNum, "_-+!%*,./:=?@[]^`{}~");
	CharacterSet setLeftShift(CharacterSet::setDigits, "$");

	class HereDocCls {	// Class to manage HERE document elements
	public:
		int State;		// 0: '<<' encountered
		// 1: collect the delimiter
		// 2: here doc text (lines after the delimiter)
		int Quote;		// the char after '<<'
		bool Quoted;		// true if Quote in ('\'','"','`')
		bool Indent;		// indented delimiter (for <<-)
		int DelimiterLength;	// strlen(Delimiter)
		char Delimiter[HERE_DELIM_MAX];	// the Delimiter
		HereDocCls() {
			State = 0;
			Quote = 0;
			Quoted = false;
			Indent = 0;
			DelimiterLength = 0;
			Delimiter[0] = '\0';
		}
		void Append(int ch) {
			Delimiter[DelimiterLength++] = static_cast<char>(ch);
			Delimiter[DelimiterLength] = '\0';
		}
		~HereDocCls() {
		}
	};
	HereDocCls HereDoc;

	class QuoteCls {	// Class to manage quote pairs (simplified vs LexPerl)
		public:
		int Count;
		int Up, Down;
		QuoteCls() {
			Count = 0;
			Up    = '\0';
			Down  = '\0';
		}
		void Open(int u) {
			Count++;
			Up    = u;
			Down  = opposite(Up);
		}
		void Start(int u) {
			Count = 0;
			Open(u);
		}
	};
	QuoteCls Quote;

	class QuoteStackCls {	// Class to manage quote pairs that nest
		public:
		int Count;
		int Up, Down;
		int Style;
		int Depth;			// levels pushed
		int CountStack[BASH_DELIM_STACK_MAX];
		int UpStack   [BASH_DELIM_STACK_MAX];
		int StyleStack[BASH_DELIM_STACK_MAX];
		QuoteStackCls() {
			Count = 0;
			Up    = '\0';
			Down  = '\0';
			Style = 0;
			Depth = 0;
		}
		void Start(int u, int s) {
			Count = 1;
			Up    = u;
			Down  = opposite(Up);
			Style = s;
		}
		void Push(int u, int s) {
			if (Depth >= BASH_DELIM_STACK_MAX)
				return;
			CountStack[Depth] = Count;
			UpStack   [Depth] = Up;
			StyleStack[Depth] = Style;
			Depth++;
			Count = 1;
			Up    = u;
			Down  = opposite(Up);
			Style = s;
		}
		void Pop(void) {
			if (Depth <= 0)
				return;
			Depth--;
			Count = CountStack[Depth];
			Up    = UpStack   [Depth];
			Style = StyleStack[Depth];
			Down  = opposite(Up);
		}
		~QuoteStackCls() {
		}
	};
	QuoteStackCls QuoteStack;

	const WordClassifier &classifierIdentifiers = subStyles.Classifier(SCE_SH_IDENTIFIER);
	const WordClassifier &classifierScalars = subStyles.Classifier(SCE_SH_SCALAR);

	int numBase = 0;
	int digit;
	Sci_PositionU endPos = startPos + length;
	int cmdState = BASH_CMD_START;
	int testExprType = 0;
	LexAccessor styler(pAccess);

	// Always backtracks to the start of a line that is not a continuation
	// of the previous line (i.e. start of a bash command segment)
	Sci_Position ln = styler.GetLine(startPos);
	if (ln > 0 && startPos == static_cast<Sci_PositionU>(styler.LineStart(ln)))
		ln--;
	for (;;) {
		startPos = styler.LineStart(ln);
		if (ln == 0 || styler.GetLineState(ln) == BASH_CMD_START)
			break;
		ln--;
	}
	initStyle = SCE_SH_DEFAULT;

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		// handle line continuation, updates per-line stored state
		if (sc.atLineStart) {
			ln = styler.GetLine(sc.currentPos);
			if (sc.state == SCE_SH_STRING
			 || sc.state == SCE_SH_BACKTICKS
			 || sc.state == SCE_SH_CHARACTER
			 || sc.state == SCE_SH_HERE_Q
			 || sc.state == SCE_SH_COMMENTLINE
			 || sc.state == SCE_SH_PARAM) {
				// force backtrack while retaining cmdState
				styler.SetLineState(ln, BASH_CMD_BODY);
			} else {
				if (ln > 0) {
					if ((sc.GetRelative(-3) == '\\' && sc.GetRelative(-2) == '\r' && sc.chPrev == '\n')
					 || sc.GetRelative(-2) == '\\') {	// handle '\' line continuation
						// retain last line's state
					} else
						cmdState = BASH_CMD_START;
				}
				styler.SetLineState(ln, cmdState);
			}
		}

		// controls change of cmdState at the end of a non-whitespace element
		// states BODY|TEST|ARITH persist until the end of a command segment
		// state WORD persist, but ends with 'in' or 'do' construct keywords
		int cmdStateNew = BASH_CMD_BODY;
		if (cmdState == BASH_CMD_TEST || cmdState == BASH_CMD_ARITH || cmdState == BASH_CMD_WORD)
			cmdStateNew = cmdState;
		int stylePrev = sc.state;

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_SH_OPERATOR:
				sc.SetState(SCE_SH_DEFAULT);
				if (cmdState == BASH_CMD_DELIM)		// if command delimiter, start new command
					cmdStateNew = BASH_CMD_START;
				else if (sc.chPrev == '\\')			// propagate command state if line continued
					cmdStateNew = cmdState;
				break;
			case SCE_SH_WORD:
				// "." never used in Bash variable names but used in file names
				if (!setWord.Contains(sc.ch)) {
					char s[500];
					char s2[10];
					sc.GetCurrent(s, sizeof(s));
					int identifierStyle = SCE_SH_IDENTIFIER;
					int subStyle = classifierIdentifiers.ValueFor(s);
					if (subStyle >= 0) {
						identifierStyle = subStyle;
					}
					// allow keywords ending in a whitespace or command delimiter
					s2[0] = static_cast<char>(sc.ch);
					s2[1] = '\0';
					bool keywordEnds = IsASpace(sc.ch) || cmdDelimiter.InList(s2);
					// 'in' or 'do' may be construct keywords
					if (cmdState == BASH_CMD_WORD) {
						if (strcmp(s, "in") == 0 && keywordEnds)
							cmdStateNew = BASH_CMD_BODY;
						else if (strcmp(s, "do") == 0 && keywordEnds)
							cmdStateNew = BASH_CMD_START;
						else
							sc.ChangeState(identifierStyle);
						sc.SetState(SCE_SH_DEFAULT);
						break;
					}
					// a 'test' keyword starts a test expression
					if (strcmp(s, "test") == 0) {
						if (cmdState == BASH_CMD_START && keywordEnds) {
							cmdStateNew = BASH_CMD_TEST;
							testExprType = 0;
						} else
							sc.ChangeState(identifierStyle);
					}
					// detect bash construct keywords
					else if (bashStruct.InList(s)) {
						if (cmdState == BASH_CMD_START && keywordEnds)
							cmdStateNew = BASH_CMD_START;
						else
							sc.ChangeState(identifierStyle);
					}
					// 'for'|'case'|'select' needs 'in'|'do' to be highlighted later
					else if (bashStruct_in.InList(s)) {
						if (cmdState == BASH_CMD_START && keywordEnds)
							cmdStateNew = BASH_CMD_WORD;
						else
							sc.ChangeState(identifierStyle);
					}
					// disambiguate option items and file test operators
					else if (s[0] == '-') {
						if (cmdState != BASH_CMD_TEST)
							sc.ChangeState(identifierStyle);
					}
					// disambiguate keywords and identifiers
					else if (cmdState != BASH_CMD_START
						  || !(keywords.InList(s) && keywordEnds)) {
						sc.ChangeState(identifierStyle);
					}
					sc.SetState(SCE_SH_DEFAULT);
				}
				break;
			case SCE_SH_IDENTIFIER:
				if (sc.chPrev == '\\' || !setWord.Contains(sc.ch) ||
					  (cmdState == BASH_CMD_ARITH && !setWordStart.Contains(sc.ch))) {
					char s[500];
					sc.GetCurrent(s, sizeof(s));
					int subStyle = classifierIdentifiers.ValueFor(s);
					if (subStyle >= 0) {
						sc.ChangeState(subStyle);
					}
					if (sc.chPrev == '\\') {	// for escaped chars
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else {
						sc.SetState(SCE_SH_DEFAULT);
					}
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
					sc.ChangeState(SCE_SH_ERROR);
				}
				sc.SetState(SCE_SH_DEFAULT);
				break;
			case SCE_SH_COMMENTLINE:
				if (sc.MatchLineEnd() && sc.chPrev != '\\') {
					sc.SetState(SCE_SH_DEFAULT);
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
					HereDoc.DelimiterLength = 0;
					HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
					if (sc.chNext == '\'' || sc.chNext == '\"') {	// a quoted here-doc delimiter (' or ")
						sc.Forward();
						HereDoc.Quoted = true;
						HereDoc.State = 1;
					} else if (setHereDoc.Contains(sc.chNext) ||
					           (sc.chNext == '=' && cmdState != BASH_CMD_ARITH)) {
						// an unquoted here-doc delimiter, no special handling
						HereDoc.State = 1;
					} else if (sc.chNext == '<') {	// HERE string <<<
						sc.Forward();
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else if (IsASpace(sc.chNext)) {
						// eat whitespace
					} else if (setLeftShift.Contains(sc.chNext) ||
					           (sc.chNext == '=' && cmdState == BASH_CMD_ARITH)) {
						// left shift <<$var or <<= cases
						sc.ChangeState(SCE_SH_OPERATOR);
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else {
						// symbols terminates; deprecated zero-length delimiter
						HereDoc.State = 1;
					}
				} else if (HereDoc.State == 1) { // collect the delimiter
					// * if single quoted, there's no escape
					// * if double quoted, there are \\ and \" escapes
					if ((HereDoc.Quote == '\'' && sc.ch != HereDoc.Quote) ||
					    (HereDoc.Quoted && sc.ch != HereDoc.Quote && sc.ch != '\\') ||
					    (HereDoc.Quote != '\'' && sc.chPrev == '\\') ||
					    (setHereDoc2.Contains(sc.ch))) {
						HereDoc.Append(sc.ch);
					} else if (HereDoc.Quoted && sc.ch == HereDoc.Quote) {	// closing quote => end of delimiter
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else if (sc.ch == '\\') {
						if (HereDoc.Quoted && sc.chNext != HereDoc.Quote && sc.chNext != '\\') {
							// in quoted prefixes only \ and the quote eat the escape
							HereDoc.Append(sc.ch);
						} else {
							// skip escape prefix
						}
					} else if (!HereDoc.Quoted) {
						sc.SetState(SCE_SH_DEFAULT);
					}
					if (HereDoc.DelimiterLength >= HERE_DELIM_MAX - 1) {	// force blowup
						sc.SetState(SCE_SH_ERROR);
						HereDoc.State = 0;
					}
				}
				break;
			case SCE_SH_HERE_Q:
				// HereDoc.State == 2
				if (sc.atLineStart) {
					sc.SetState(SCE_SH_HERE_Q);
					int prefixws = 0;
					while (sc.ch == '\t' && !sc.MatchLineEnd()) {	// tabulation prefix
						sc.Forward();
						prefixws++;
					}
					if (prefixws > 0)
						sc.SetState(SCE_SH_HERE_Q);
					while (!sc.MatchLineEnd()) {
						sc.Forward();
					}
					char s[HERE_DELIM_MAX];
					sc.GetCurrent(s, sizeof(s));
					if (sc.LengthCurrent() == 0) {  // '' or "" delimiters
						if ((prefixws == 0 || HereDoc.Indent) &&
							HereDoc.Quoted && HereDoc.DelimiterLength == 0)
							sc.SetState(SCE_SH_DEFAULT);
						break;
					}
					if (s[strlen(s) - 1] == '\r')
						s[strlen(s) - 1] = '\0';
					if (strcmp(HereDoc.Delimiter, s) == 0) {
						if ((prefixws == 0) ||	// indentation rule
							(prefixws > 0 && HereDoc.Indent)) {
							sc.SetState(SCE_SH_DEFAULT);
							break;
						}
					}
				}
				break;
			case SCE_SH_SCALAR:	// variable names
				if (!setParam.Contains(sc.ch)) {
					char s[500];
					sc.GetCurrent(s, sizeof(s));
					int subStyle = classifierScalars.ValueFor(&s[1]); // skip the $
					if (subStyle >= 0) {
						sc.ChangeState(subStyle);
					}
					if (sc.LengthCurrent() == 1) {
						// Special variable: $(, $_ etc.
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else {
						sc.SetState(SCE_SH_DEFAULT);
					}
				}
				break;
			case SCE_SH_STRING:	// delimited styles, can nest
			case SCE_SH_BACKTICKS:
				if (sc.ch == '\\' && QuoteStack.Up != '\\') {
					if (QuoteStack.Style != BASH_DELIM_LITERAL)
						sc.Forward();
				} else if (sc.ch == QuoteStack.Down) {
					QuoteStack.Count--;
					if (QuoteStack.Count == 0) {
						if (QuoteStack.Depth > 0) {
							QuoteStack.Pop();
						} else
							sc.ForwardSetState(SCE_SH_DEFAULT);
					}
				} else if (sc.ch == QuoteStack.Up) {
					QuoteStack.Count++;
				} else {
					if (QuoteStack.Style == BASH_DELIM_STRING ||
						QuoteStack.Style == BASH_DELIM_LSTRING
					) {	// do nesting for "string", $"locale-string"
						if (sc.ch == '`') {
							QuoteStack.Push(sc.ch, BASH_DELIM_BACKTICK);
						} else if (sc.ch == '$' && sc.chNext == '(') {
							sc.Forward();
							QuoteStack.Push(sc.ch, BASH_DELIM_COMMAND);
						}
					} else if (QuoteStack.Style == BASH_DELIM_COMMAND ||
							   QuoteStack.Style == BASH_DELIM_BACKTICK
					) {	// do nesting for $(command), `command`
						if (sc.ch == '\'') {
							QuoteStack.Push(sc.ch, BASH_DELIM_LITERAL);
						} else if (sc.ch == '\"') {
							QuoteStack.Push(sc.ch, BASH_DELIM_STRING);
						} else if (sc.ch == '`') {
							QuoteStack.Push(sc.ch, BASH_DELIM_BACKTICK);
						} else if (sc.ch == '$') {
							if (sc.chNext == '\'') {
								sc.Forward();
								QuoteStack.Push(sc.ch, BASH_DELIM_CSTRING);
							} else if (sc.chNext == '\"') {
								sc.Forward();
								QuoteStack.Push(sc.ch, BASH_DELIM_LSTRING);
							} else if (sc.chNext == '(') {
								sc.Forward();
								QuoteStack.Push(sc.ch, BASH_DELIM_COMMAND);
							}
						}
					}
				}
				break;
			case SCE_SH_PARAM: // ${parameter}
				if (sc.ch == '\\' && Quote.Up != '\\') {
					sc.Forward();
				} else if (sc.ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0) {
						sc.ForwardSetState(SCE_SH_DEFAULT);
					}
				} else if (sc.ch == Quote.Up) {
					Quote.Count++;
				}
				break;
			case SCE_SH_CHARACTER: // singly-quoted strings
				if (sc.ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0) {
						sc.ForwardSetState(SCE_SH_DEFAULT);
					}
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
				if (sc.state == SCE_SH_HERE_DELIM) {
					// Missing quote at end of string! Syntax error in bash 4.3
					// Mark this bit as an error, do not colour any here-doc
					sc.ChangeState(SCE_SH_ERROR);
					sc.SetState(SCE_SH_DEFAULT);
				} else {
					// HereDoc.Quote always == '\''
					sc.SetState(SCE_SH_HERE_Q);
				}
			} else if (HereDoc.DelimiterLength == 0) {
				// no delimiter, illegal (but '' and "" are legal)
				sc.ChangeState(SCE_SH_ERROR);
				sc.SetState(SCE_SH_DEFAULT);
			} else {
				sc.SetState(SCE_SH_HERE_Q);
			}
		}

		// update cmdState about the current command segment
		if (stylePrev != SCE_SH_DEFAULT && sc.state == SCE_SH_DEFAULT) {
			cmdState = cmdStateNew;
		}
		// Determine if a new state should be entered.
		if (sc.state == SCE_SH_DEFAULT) {
			if (sc.ch == '\\') {
				// Bash can escape any non-newline as a literal
				sc.SetState(SCE_SH_IDENTIFIER);
				if (sc.chNext == '\r' || sc.chNext == '\n')
					sc.SetState(SCE_SH_OPERATOR);
			} else if (IsADigit(sc.ch)) {
				sc.SetState(SCE_SH_NUMBER);
				numBase = BASH_BASE_DECIMAL;
				if (sc.ch == '0') {	// hex,octal
					if (sc.chNext == 'x' || sc.chNext == 'X') {
						numBase = BASH_BASE_HEX;
						sc.Forward();
					} else if (IsADigit(sc.chNext)) {
#ifdef PEDANTIC_OCTAL
						numBase = BASH_BASE_OCTAL;
#else
						numBase = BASH_BASE_HEX;
#endif
					}
				}
			} else if (setWordStart.Contains(sc.ch)) {
				sc.SetState(SCE_SH_WORD);
			} else if (sc.ch == '#') {
				if (stylePrev != SCE_SH_WORD && stylePrev != SCE_SH_IDENTIFIER &&
					(sc.currentPos == 0 || setMetaCharacter.Contains(sc.chPrev))) {
					sc.SetState(SCE_SH_COMMENTLINE);
				} else {
					sc.SetState(SCE_SH_WORD);
				}
				// handle some zsh features within arithmetic expressions only
				if (cmdState == BASH_CMD_ARITH) {
					if (sc.chPrev == '[') {	// [#8] [##8] output digit setting
						sc.SetState(SCE_SH_WORD);
						if (sc.chNext == '#') {
							sc.Forward();
						}
					} else if (sc.Match("##^") && IsUpperCase(sc.GetRelative(3))) {	// ##^A
						sc.SetState(SCE_SH_IDENTIFIER);
						sc.Forward(3);
					} else if (sc.chNext == '#' && !IsASpace(sc.GetRelative(2))) {	// ##a
						sc.SetState(SCE_SH_IDENTIFIER);
						sc.Forward(2);
					} else if (setWordStart.Contains(sc.chNext)) {	// #name
						sc.SetState(SCE_SH_IDENTIFIER);
					}
				}
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_SH_STRING);
				QuoteStack.Start(sc.ch, BASH_DELIM_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_SH_CHARACTER);
				Quote.Start(sc.ch);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_SH_BACKTICKS);
				QuoteStack.Start(sc.ch, BASH_DELIM_BACKTICK);
			} else if (sc.ch == '$') {
				if (sc.Match("$((")) {
					sc.SetState(SCE_SH_OPERATOR);	// handle '((' later
					continue;
				}
				sc.SetState(SCE_SH_SCALAR);
				sc.Forward();
				if (sc.ch == '{') {
					sc.ChangeState(SCE_SH_PARAM);
					Quote.Start(sc.ch);
				} else if (sc.ch == '\'') {
					sc.ChangeState(SCE_SH_STRING);
					QuoteStack.Start(sc.ch, BASH_DELIM_CSTRING);
				} else if (sc.ch == '"') {
					sc.ChangeState(SCE_SH_STRING);
					QuoteStack.Start(sc.ch, BASH_DELIM_LSTRING);
				} else if (sc.ch == '(') {
					sc.ChangeState(SCE_SH_BACKTICKS);
					QuoteStack.Start(sc.ch, BASH_DELIM_COMMAND);
				} else if (sc.ch == '`') {	// $` seen in a configure script, valid?
					sc.ChangeState(SCE_SH_BACKTICKS);
					QuoteStack.Start(sc.ch, BASH_DELIM_BACKTICK);
				} else {
					continue;	// scalar has no delimiter pair
				}
			} else if (sc.Match('<', '<')) {
				sc.SetState(SCE_SH_HERE_DELIM);
				HereDoc.State = 0;
				if (sc.GetRelative(2) == '-') {	// <<- indent case
					HereDoc.Indent = true;
					sc.Forward();
				} else {
					HereDoc.Indent = false;
				}
			} else if (sc.ch == '-'	&&	// one-char file test operators
					   setSingleCharOp.Contains(sc.chNext) &&
					   !setWord.Contains(sc.GetRelative(2)) &&
					   IsASpace(sc.chPrev)) {
				sc.SetState(SCE_SH_WORD);
				sc.Forward();
			} else if (setBashOperator.Contains(sc.ch)) {
				char s[10];
				bool isCmdDelim = false;
				sc.SetState(SCE_SH_OPERATOR);
				// globs have no whitespace, do not appear in arithmetic expressions
				if (cmdState != BASH_CMD_ARITH && sc.ch == '(' && sc.chNext != '(') {
					int i = GlobScan(sc);
					if (i > 1) {
						sc.SetState(SCE_SH_IDENTIFIER);
						sc.Forward(i);
						continue;
					}
				}
				// handle opening delimiters for test/arithmetic expressions - ((,[[,[
				if (cmdState == BASH_CMD_START
				 || cmdState == BASH_CMD_BODY) {
					if (sc.Match('(', '(')) {
						cmdState = BASH_CMD_ARITH;
						sc.Forward();
					} else if (sc.Match('[', '[') && IsASpace(sc.GetRelative(2))) {
						cmdState = BASH_CMD_TEST;
						testExprType = 1;
						sc.Forward();
					} else if (sc.ch == '[' && IsASpace(sc.chNext)) {
						cmdState = BASH_CMD_TEST;
						testExprType = 2;
					}
				}
				// special state -- for ((x;y;z)) in ... looping
				if (cmdState == BASH_CMD_WORD && sc.Match('(', '(')) {
					cmdState = BASH_CMD_ARITH;
					sc.Forward();
					continue;
				}
				// handle command delimiters in command START|BODY|WORD state, also TEST if 'test'
				if (cmdState == BASH_CMD_START
				 || cmdState == BASH_CMD_BODY
				 || cmdState == BASH_CMD_WORD
				 || (cmdState == BASH_CMD_TEST && testExprType == 0)) {
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
						cmdState = BASH_CMD_DELIM;
						continue;
					}
				}
				// handle closing delimiters for test/arithmetic expressions - )),]],]
				if (cmdState == BASH_CMD_ARITH && sc.Match(')', ')')) {
					cmdState = BASH_CMD_BODY;
					sc.Forward();
				} else if (cmdState == BASH_CMD_TEST && IsASpace(sc.chPrev)) {
					if (sc.Match(']', ']') && testExprType == 1) {
						sc.Forward();
						cmdState = BASH_CMD_BODY;
					} else if (sc.ch == ']' && testExprType == 2) {
						cmdState = BASH_CMD_BODY;
					}
				}
			}
		}// sc.state
	}
	sc.Complete();
	if (sc.state == SCE_SH_HERE_Q) {
		styler.ChangeLexerState(sc.currentPos, styler.Length());
	}
	sc.Complete();
}

void SCI_METHOD LexerBash::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess) {
	if(!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	int skipHereCh = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	char word[8] = { '\0' }; // we're not interested in long words anyway
	unsigned int wordlen = 0;
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
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
		if (style == SCE_SH_WORD) {
			if ((wordlen + 1) < sizeof(word))
				word[wordlen++] = ch;
			if (styleNext != style) {
				word[wordlen] = '\0';
				wordlen = 0;
				if (strcmp(word, "if") == 0 || strcmp(word, "case") == 0 || strcmp(word, "do") == 0) {
					levelCurrent++;
				} else if (strcmp(word, "fi") == 0 || strcmp(word, "esac") == 0 || strcmp(word, "done") == 0) {
					levelCurrent--;
				}
			}
		}
		if (style == SCE_SH_OPERATOR) {
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}
		// Here Document folding
		if (style == SCE_SH_HERE_DELIM) {
			if (ch == '<' && chNext == '<') {
				if (styler.SafeGetCharAt(i + 2) == '<') {
					skipHereCh = 1;
				} else {
					if (skipHereCh == 0) {
						levelCurrent++;
					} else {
						skipHereCh = 0;
					}
				}
			}
		} else if (style == SCE_SH_HERE_Q && styler.StyleAt(i+1) == SCE_SH_DEFAULT) {
			levelCurrent--;
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
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

LexerModule lmBash(SCLEX_BASH, LexerBash::LexerFactoryBash, "bash", bashWordListDesc);
