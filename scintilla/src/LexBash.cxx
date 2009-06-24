// Scintilla source code edit control
/** @file LexBash.cxx
 ** Lexer for Bash.
 **/
// Copyright 2004-2008 by Neil Hodgson <neilh@scintilla.org>
// Adapted from LexPerl by Kein-Hong Man 2004
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "CharacterSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

#define HERE_DELIM_MAX 256

// define this if you want 'invalid octals' to be marked as errors
// usually, this is not a good idea, permissive lexing is better
#undef PEDANTIC_OCTAL

#define BASH_BASE_ERROR		65
#define BASH_BASE_DECIMAL	66
#define BASH_BASE_HEX		67
#ifdef PEDANTIC_OCTAL
#define BASH_BASE_OCTAL		68
#define BASH_BASE_OCTAL_ERROR	69
#endif

static inline int translateBashDigit(int ch) {
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

static inline int getBashNumberBase(char *s) {
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

static int opposite(int ch) {
	if (ch == '(') return ')';
	if (ch == '[') return ']';
	if (ch == '{') return '}';
	if (ch == '<') return '>';
	return ch;
}

static void ColouriseBashDoc(unsigned int startPos, int length, int initStyle,
							 WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];

	CharacterSet setWordStart(CharacterSet::setAlpha, "_");
	// note that [+-] are often parts of identifiers in shell scripts
	CharacterSet setWord(CharacterSet::setAlphaNum, "._+-");
	CharacterSet setBashOperator(CharacterSet::setNone, "^&\\%()-+=|{}[]:;>,*/<?!.~@");
	CharacterSet setSingleCharOp(CharacterSet::setNone, "rwxoRWXOezsfdlpSbctugkTBMACahGLNn");
	CharacterSet setParam(CharacterSet::setAlphaNum, "$_");
	CharacterSet setHereDoc(CharacterSet::setAlpha, "_\\-+!");
	CharacterSet setHereDoc2(CharacterSet::setAlphaNum, "_-+!");
	CharacterSet setLeftShift(CharacterSet::setDigits, "=$");

	class HereDocCls {	// Class to manage HERE document elements
	public:
		int State;		// 0: '<<' encountered
		// 1: collect the delimiter
		// 2: here doc text (lines after the delimiter)
		int Quote;		// the char after '<<'
		bool Quoted;		// true if Quote in ('\'','"','`')
		bool Indent;		// indented delimiter (for <<-)
		int DelimiterLength;	// strlen(Delimiter)
		char *Delimiter;	// the Delimiter, 256: sizeof PL_tokenbuf
		HereDocCls() {
			State = 0;
			Quote = 0;
			Quoted = false;
			Indent = 0;
			DelimiterLength = 0;
			Delimiter = new char[HERE_DELIM_MAX];
			Delimiter[0] = '\0';
		}
		void Append(int ch) {
			Delimiter[DelimiterLength++] = static_cast<char>(ch);
			Delimiter[DelimiterLength] = '\0';
		}
		~HereDocCls() {
			delete []Delimiter;
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

	int numBase = 0;
	int digit;
	unsigned int endPos = startPos + length;

	// Backtrack to beginning of style if required...
	// If in a long distance lexical state, backtrack to find quote characters
	if (initStyle == SCE_SH_HERE_Q) {
		while ((startPos > 1) && (styler.StyleAt(startPos) != SCE_SH_HERE_DELIM)) {
			startPos--;
		}
		startPos = styler.LineStart(styler.GetLine(startPos));
		initStyle = styler.StyleAt(startPos - 1);
	}
	// Bash strings can be multi-line with embedded newlines, so backtrack.
	// Bash numbers have additional state during lexing, so backtrack too.
	if (initStyle == SCE_SH_STRING
	 || initStyle == SCE_SH_BACKTICKS
	 || initStyle == SCE_SH_CHARACTER
	 || initStyle == SCE_SH_NUMBER
	 || initStyle == SCE_SH_IDENTIFIER
	 || initStyle == SCE_SH_COMMENTLINE) {
		while ((startPos > 1) && (styler.StyleAt(startPos - 1) == initStyle)) {
			startPos--;
		}
		initStyle = SCE_SH_DEFAULT;
	}

	StyleContext sc(startPos, endPos - startPos, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_SH_OPERATOR:
				sc.SetState(SCE_SH_DEFAULT);
				break;
			case SCE_SH_WORD:
				// "." never used in Bash variable names but used in file names
				if (!setWord.Contains(sc.ch)) {
					char s[1000];
					sc.GetCurrent(s, sizeof(s));
					if (s[0] != '-' &&	// for file operators
						!keywords.InList(s)) {
						sc.ChangeState(SCE_SH_IDENTIFIER);
					}
					sc.SetState(SCE_SH_DEFAULT);
				}
				break;
			case SCE_SH_IDENTIFIER:
				if (sc.chPrev == '\\') {	// for escaped chars
					sc.ForwardSetState(SCE_SH_DEFAULT);
				} else if (!setWord.Contains(sc.ch)) {
					sc.SetState(SCE_SH_DEFAULT);
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
				if (sc.ch == '\\' && (sc.chNext == '\r' || sc.chNext == '\n')) {
					// comment continuation
					sc.Forward();
					if (sc.ch == '\r' && sc.chNext == '\n') {
						sc.Forward();
					}
				} else if (sc.atLineEnd) {
					sc.ForwardSetState(SCE_SH_DEFAULT);
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
					} else if (!HereDoc.Indent && sc.chNext == '-') {	// <<- indent case
						HereDoc.Indent = true;
					} else if (setHereDoc.Contains(sc.chNext)) {
						// an unquoted here-doc delimiter, no special handling
						// TODO check what exactly bash considers part of the delim
						HereDoc.State = 1;
					} else if (sc.chNext == '<') {	// HERE string <<<
						sc.Forward();
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else if (IsASpace(sc.chNext)) {
						// eat whitespace
					} else if (setLeftShift.Contains(sc.chNext)) {
						// left shift << or <<= operator cases
						sc.ChangeState(SCE_SH_OPERATOR);
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else {
						// symbols terminates; deprecated zero-length delimiter
						HereDoc.State = 1;
					}
				} else if (HereDoc.State == 1) { // collect the delimiter
					if (HereDoc.Quoted) { // a quoted here-doc delimiter
						if (sc.ch == HereDoc.Quote) { // closing quote => end of delimiter
							sc.ForwardSetState(SCE_SH_DEFAULT);
						} else {
							if (sc.ch == '\\' && sc.chNext == HereDoc.Quote) { // escaped quote
								sc.Forward();
							}
							HereDoc.Append(sc.ch);
						}
					} else { // an unquoted here-doc delimiter
						if (setHereDoc2.Contains(sc.ch)) {
							HereDoc.Append(sc.ch);
						} else if (sc.ch == '\\') {
							// skip escape prefix
						} else {
							sc.SetState(SCE_SH_DEFAULT);
						}
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
					while (IsASpace(sc.ch) && !sc.atLineEnd) {	// whitespace prefix
						sc.Forward();
						prefixws++;
					}
					if (prefixws > 0)
						sc.SetState(SCE_SH_HERE_Q);
					while (!sc.atLineEnd) {
						sc.Forward();
					}
					char s[HERE_DELIM_MAX];
					sc.GetCurrent(s, sizeof(s));
					if (sc.LengthCurrent() == 0)
						break;
					if (s[strlen(s) - 1] == '\r')
						s[strlen(s) - 1] = '\0';
					if (strcmp(HereDoc.Delimiter, s) == 0) {
						if ((prefixws > 0 && HereDoc.Indent) ||	// indentation rule
							(prefixws == 0 && !HereDoc.Indent)) {
							sc.SetState(SCE_SH_DEFAULT);
							break;
						}
					}
				}
				break;
			case SCE_SH_SCALAR:	// variable names
				if (!setParam.Contains(sc.ch)) {
					if (sc.LengthCurrent() == 1) {
						// Special variable: $(, $_ etc.
						sc.ForwardSetState(SCE_SH_DEFAULT);
					} else {
						sc.SetState(SCE_SH_DEFAULT);
					}
				}
				break;
			case SCE_SH_STRING:	// delimited styles
			case SCE_SH_CHARACTER:
			case SCE_SH_BACKTICKS:
			case SCE_SH_PARAM:
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
		}

		// Must check end of HereDoc state 1 before default state is handled
		if (HereDoc.State == 1 && sc.atLineEnd) {
			// Begin of here-doc (the line after the here-doc delimiter):
			// Lexically, the here-doc starts from the next line after the >>, but the
			// first line of here-doc seem to follow the style of the last EOL sequence
			HereDoc.State = 2;
			if (HereDoc.Quoted) {
				if (sc.state == SCE_SH_HERE_DELIM) {
					// Missing quote at end of string! We are stricter than bash.
					// Colour here-doc anyway while marking this bit as an error.
					sc.ChangeState(SCE_SH_ERROR);
				}
				// HereDoc.Quote always == '\''
			}
			sc.SetState(SCE_SH_HERE_Q);
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_SH_DEFAULT) {
			if (sc.ch == '\\') {	// escaped character
				sc.SetState(SCE_SH_IDENTIFIER);
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
				sc.SetState(SCE_SH_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_SH_STRING);
				Quote.Start(sc.ch);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_SH_CHARACTER);
				Quote.Start(sc.ch);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_SH_BACKTICKS);
				Quote.Start(sc.ch);
			} else if (sc.ch == '$') {
				sc.SetState(SCE_SH_SCALAR);
				sc.Forward();
				if (sc.ch == '{') {
					sc.ChangeState(SCE_SH_PARAM);
				} else if (sc.ch == '\'') {
					sc.ChangeState(SCE_SH_CHARACTER);
				} else if (sc.ch == '"') {
					sc.ChangeState(SCE_SH_STRING);
				} else if (sc.ch == '(' || sc.ch == '`') {
					sc.ChangeState(SCE_SH_BACKTICKS);
					if (sc.chNext == '(') {	// $(( is lexed as operator
						sc.ChangeState(SCE_SH_OPERATOR);
					}
				} else {
					continue;	// scalar has no delimiter pair
				}
				// fallthrough, open delim for $[{'"(`]
				Quote.Start(sc.ch);
			} else if (sc.Match('<', '<')) {
				sc.SetState(SCE_SH_HERE_DELIM);
				HereDoc.State = 0;
				HereDoc.Indent = false;
			} else if (sc.ch == '-'	&&	// one-char file test operators
					   setSingleCharOp.Contains(sc.chNext) &&
					   !setWord.Contains(sc.GetRelative(2)) &&
					   IsASpace(sc.chPrev)) {
				sc.SetState(SCE_SH_WORD);
				sc.Forward();
			} else if (setBashOperator.Contains(sc.ch)) {
				sc.SetState(SCE_SH_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static bool IsCommentLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	int eol_pos = styler.LineStart(line + 1) - 1;
	for (int i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		if (ch == '#')
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

static void FoldBashDoc(unsigned int startPos, int length, int, WordList *[],
						Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		// Comment folding
		if (foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
				&& IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
					 && !IsCommentLine(lineCurrent + 1, styler))
				levelCurrent--;
		}
		if (style == SCE_SH_OPERATOR) {
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}
		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
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

static const char * const bashWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmBash(SCLEX_BASH, ColouriseBashDoc, "bash", FoldBashDoc, bashWordListDesc);
