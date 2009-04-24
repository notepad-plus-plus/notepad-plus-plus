// Scintilla source code edit control
/** @file LexBash.cxx
 ** Lexer for Bash.
 **/
// Copyright 2004-2007 by Neil Hodgson <neilh@scintilla.org>
// Adapted from LexPerl by Kein-Hong Man <mkh@pl.jaring.my> 2004
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

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

#define HERE_DELIM_MAX 256

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline int translateBashDigit(char ch) {
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

static inline bool isEOLChar(char ch) {
	return (ch == '\r') || (ch == '\n');
}

static bool isSingleCharOp(char ch) {
	char strCharSet[2];
	strCharSet[0] = ch;
	strCharSet[1] = '\0';
	return (NULL != strstr("rwxoRWXOezsfdlpSbctugkTBMACahGLNn", strCharSet));
}

static inline bool isBashOperator(char ch) {
	if (ch == '^' || ch == '&' || ch == '\\' || ch == '%' ||
	        ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '>' || ch == ',' || ch == '/' || ch == '<' ||
	        ch == '?' || ch == '!' || ch == '.' || ch == '~' ||
		ch == '@')
		return true;
	return false;
}

static int classifyWordBash(unsigned int start, unsigned int end, WordList &keywords, Accessor &styler) {
	char s[100];
	for (unsigned int i = 0; i < end - start + 1 && i < 30; i++) {
		s[i] = styler[start + i];
		s[i + 1] = '\0';
	}
	char chAttr = SCE_SH_IDENTIFIER;
	if (keywords.InList(s))
		chAttr = SCE_SH_WORD;
	styler.ColourTo(end, chAttr);
	return chAttr;
}

static inline int getBashNumberBase(unsigned int start, unsigned int end, Accessor &styler) {
	int base = 0;
	for (unsigned int i = 0; i < end - start + 1 && i < 10; i++) {
		base = base * 10 + (styler[start + i] - '0');
	}
	if (base > 64 || (end - start) > 1) {
		return BASH_BASE_ERROR;
	}
	return base;
}

static inline bool isEndVar(char ch) {
	return !isalnum(ch) && ch != '$' && ch != '_';
}

static inline bool isNonQuote(char ch) {
	return isalnum(ch) || ch == '_';
}

static bool isMatch(Accessor &styler, int lengthDoc, int pos, const char *val) {
	if ((pos + static_cast<int>(strlen(val))) >= lengthDoc) {
		return false;
	}
	while (*val) {
		if (*val != styler[pos++]) {
			return false;
		}
		val++;
	}
	return true;
}

static char opposite(char ch) {
	if (ch == '(')
		return ')';
	if (ch == '[')
		return ']';
	if (ch == '{')
		return '}';
	if (ch == '<')
		return '>';
	return ch;
}

static void ColouriseBashDoc(unsigned int startPos, int length, int initStyle,
                             WordList *keywordlists[], Accessor &styler) {

	// Lexer for bash often has to backtrack to start of current style to determine
	// which characters are being used as quotes, how deeply nested is the
	// start position and what the termination string is for here documents

	WordList &keywords = *keywordlists[0];

	class HereDocCls {
	public:
		int State;		// 0: '<<' encountered
		// 1: collect the delimiter
		// 2: here doc text (lines after the delimiter)
		char Quote;		// the char after '<<'
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
		~HereDocCls() {
			delete []Delimiter;
		}
	};
	HereDocCls HereDoc;

	class QuoteCls {
		public:
		int  Rep;
		int  Count;
		char Up;
		char Down;
		QuoteCls() {
			this->New(1);
		}
		void New(int r) {
			Rep   = r;
			Count = 0;
			Up    = '\0';
			Down  = '\0';
		}
		void Open(char u) {
			Count++;
			Up    = u;
			Down  = opposite(Up);
		}
	};
	QuoteCls Quote;

	int state = initStyle;
	int numBase = 0;
	unsigned int lengthDoc = startPos + length;

	// If in a long distance lexical state, seek to the beginning to find quote characters
	// Bash strings can be multi-line with embedded newlines, so backtrack.
	// Bash numbers have additional state during lexing, so backtrack too.
	if (state == SCE_SH_HERE_Q) {
		while ((startPos > 1) && (styler.StyleAt(startPos) != SCE_SH_HERE_DELIM)) {
			startPos--;
		}
		startPos = styler.LineStart(styler.GetLine(startPos));
		state = styler.StyleAt(startPos - 1);
	}
	if (state == SCE_SH_STRING
	 || state == SCE_SH_BACKTICKS
	 || state == SCE_SH_CHARACTER
	 || state == SCE_SH_NUMBER
	 || state == SCE_SH_IDENTIFIER
	 || state == SCE_SH_COMMENTLINE
	) {
		while ((startPos > 1) && (styler.StyleAt(startPos - 1) == state)) {
			startPos--;
		}
		state = SCE_SH_DEFAULT;
	}

	styler.StartAt(startPos);
	char chPrev = styler.SafeGetCharAt(startPos - 1);
	if (startPos == 0)
		chPrev = '\n';
	char chNext = styler[startPos];
	styler.StartSegment(startPos);

	for (unsigned int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		// if the current character is not consumed due to the completion of an
		// earlier style, lexing can be restarted via a simple goto
	restartLexer:
		chNext = styler.SafeGetCharAt(i + 1);
		char chNext2 = styler.SafeGetCharAt(i + 2);

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			chPrev = ' ';
			i += 1;
			continue;
		}

		if ((chPrev == '\r' && ch == '\n')) {	// skip on DOS/Windows
			styler.ColourTo(i, state);
			chPrev = ch;
			continue;
		}

		if (HereDoc.State == 1 && isEOLChar(ch)) {
			// Begin of here-doc (the line after the here-doc delimiter):
			// Lexically, the here-doc starts from the next line after the >>, but the
			// first line of here-doc seem to follow the style of the last EOL sequence
			HereDoc.State = 2;
			if (HereDoc.Quoted) {
				if (state == SCE_SH_HERE_DELIM) {
					// Missing quote at end of string! We are stricter than bash.
					// Colour here-doc anyway while marking this bit as an error.
					state = SCE_SH_ERROR;
				}
				styler.ColourTo(i - 1, state);
				// HereDoc.Quote always == '\''
				state = SCE_SH_HERE_Q;
			} else {
				styler.ColourTo(i - 1, state);
				// always switch
				state = SCE_SH_HERE_Q;
			}
		}

		if (state == SCE_SH_DEFAULT) {
			if (ch == '\\') {	// escaped character
				if (i < lengthDoc - 1)
					i++;
				ch = chNext;
				chNext = chNext2;
				styler.ColourTo(i, SCE_SH_IDENTIFIER);
			} else if (isdigit(ch)) {
				state = SCE_SH_NUMBER;
				numBase = BASH_BASE_DECIMAL;
				if (ch == '0') {	// hex,octal
					if (chNext == 'x' || chNext == 'X') {
						numBase = BASH_BASE_HEX;
						i++;
						ch = chNext;
						chNext = chNext2;
					} else if (isdigit(chNext)) {
#ifdef PEDANTIC_OCTAL
						numBase = BASH_BASE_OCTAL;
#else
						numBase = BASH_BASE_HEX;
#endif
					}
				}
			} else if (iswordstart(ch)) {
				state = SCE_SH_WORD;
				if (!iswordchar(chNext) && chNext != '+' && chNext != '-') {
					// We need that if length of word == 1!
					// This test is copied from the SCE_SH_WORD handler.
					classifyWordBash(styler.GetStartSegment(), i, keywords, styler);
					state = SCE_SH_DEFAULT;
				}
			} else if (ch == '#') {
				state = SCE_SH_COMMENTLINE;
			} else if (ch == '\"') {
				state = SCE_SH_STRING;
				Quote.New(1);
				Quote.Open(ch);
			} else if (ch == '\'') {
				state = SCE_SH_CHARACTER;
				Quote.New(1);
				Quote.Open(ch);
			} else if (ch == '`') {
				state = SCE_SH_BACKTICKS;
				Quote.New(1);
				Quote.Open(ch);
			} else if (ch == '$') {
				if (chNext == '{') {
					state = SCE_SH_PARAM;
					goto startQuote;
				} else if (chNext == '\'') {
					state = SCE_SH_CHARACTER;
					goto startQuote;
				} else if (chNext == '"') {
					state = SCE_SH_STRING;
					goto startQuote;
				} else if (chNext == '(' && chNext2 == '(') {
					styler.ColourTo(i, SCE_SH_OPERATOR);
					state = SCE_SH_DEFAULT;
					goto skipChar;
				} else if (chNext == '(' || chNext == '`') {
					state = SCE_SH_BACKTICKS;
				startQuote:
					Quote.New(1);
					Quote.Open(chNext);
					goto skipChar;
				} else {
					state = SCE_SH_SCALAR;
				skipChar:
					i++;
					ch = chNext;
					chNext = chNext2;
				}
			} else if (ch == '*') {
				if (chNext == '*') {	// exponentiation
					i++;
					ch = chNext;
					chNext = chNext2;
				}
				styler.ColourTo(i, SCE_SH_OPERATOR);
			} else if (ch == '<' && chNext == '<') {
				state = SCE_SH_HERE_DELIM;
				HereDoc.State = 0;
				HereDoc.Indent = false;
			} else if (ch == '-'	// file test operators
			           && isSingleCharOp(chNext)
			           && !isalnum((chNext2 = styler.SafeGetCharAt(i+2)))
			           && isspace(chPrev)) {
				styler.ColourTo(i + 1, SCE_SH_WORD);
				state = SCE_SH_DEFAULT;
				i++;
				ch = chNext;
				chNext = chNext2;
			} else if (isBashOperator(ch)) {
				styler.ColourTo(i, SCE_SH_OPERATOR);
			} else {
				// keep colouring defaults to make restart easier
				styler.ColourTo(i, SCE_SH_DEFAULT);
			}
		} else if (state == SCE_SH_NUMBER) {
			int digit = translateBashDigit(ch);
			if (numBase == BASH_BASE_DECIMAL) {
				if (ch == '#') {
					numBase = getBashNumberBase(styler.GetStartSegment(), i - 1, styler);
					if (numBase == BASH_BASE_ERROR)	// take the rest as comment
						goto numAtEnd;
				} else if (!isdigit(ch))
					goto numAtEnd;
			} else if (numBase == BASH_BASE_HEX) {
				if ((digit < 16) || (digit >= 36 && digit <= 41)) {
					// hex digit 0-9a-fA-F
				} else
					goto numAtEnd;
#ifdef PEDANTIC_OCTAL
			} else if (numBase == BASH_BASE_OCTAL ||
				   numBase == BASH_BASE_OCTAL_ERROR) {
				if (digit > 7) {
					if (digit <= 9) {
                                                numBase = BASH_BASE_OCTAL_ERROR;
					} else
						goto numAtEnd;
				}
#endif
			} else if (numBase == BASH_BASE_ERROR) {
				if (digit > 9)
					goto numAtEnd;
			} else {	// DD#DDDD number style handling
				if (digit != BASH_BASE_ERROR) {
					if (numBase <= 36) {
						// case-insensitive if base<=36
						if (digit >= 36) digit -= 26;
					}
					if (digit >= numBase) {
						if (digit <= 9) {
							numBase = BASH_BASE_ERROR;
						} else
							goto numAtEnd;
					}
				} else {
			numAtEnd:
					if (numBase == BASH_BASE_ERROR
#ifdef PEDANTIC_OCTAL
					    || numBase == BASH_BASE_OCTAL_ERROR
#endif
                                           )
						state = SCE_SH_ERROR;
					styler.ColourTo(i - 1, state);
					state = SCE_SH_DEFAULT;
					goto restartLexer;
				}
			}
		} else if (state == SCE_SH_WORD) {
			if (!iswordchar(chNext) && chNext != '+' && chNext != '-') {
				// "." never used in Bash variable names
				// but used in file names
				classifyWordBash(styler.GetStartSegment(), i, keywords, styler);
				state = SCE_SH_DEFAULT;
				ch = ' ';
			}
		} else if (state == SCE_SH_IDENTIFIER) {
			if (!iswordchar(chNext) && chNext != '+' && chNext != '-') {
				styler.ColourTo(i, SCE_SH_IDENTIFIER);
				state = SCE_SH_DEFAULT;
				ch = ' ';
			}
		} else {
			if (state == SCE_SH_COMMENTLINE) {
				if (ch == '\\' && isEOLChar(chNext)) {
					// comment continuation
					if (chNext == '\r' && chNext2 == '\n') {
						i += 2;
						ch = styler.SafeGetCharAt(i);
						chNext = styler.SafeGetCharAt(i + 1);
					} else {
						i++;
						ch = chNext;
						chNext = chNext2;
					}
				} else if (isEOLChar(ch)) {
					styler.ColourTo(i - 1, state);
					state = SCE_SH_DEFAULT;
					goto restartLexer;
				} else if (isEOLChar(chNext)) {
					styler.ColourTo(i, state);
					state = SCE_SH_DEFAULT;
				}
			} else if (state == SCE_SH_HERE_DELIM) {
				//
				// From Bash info:
				// ---------------
				// Specifier format is: <<[-]WORD
				// Optional '-' is for removal of leading tabs from here-doc.
				// Whitespace acceptable after <<[-] operator
				//
				if (HereDoc.State == 0) { // '<<' encountered
					HereDoc.State = 1;
					HereDoc.Quote = chNext;
					HereDoc.Quoted = false;
					HereDoc.DelimiterLength = 0;
					HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
					if (chNext == '\'' || chNext == '\"') {	// a quoted here-doc delimiter (' or ")
						i++;
						ch = chNext;
						chNext = chNext2;
						HereDoc.Quoted = true;
					} else if (!HereDoc.Indent && chNext == '-') {	// <<- indent case
						HereDoc.Indent = true;
						HereDoc.State = 0;
					} else if (isalpha(chNext) || chNext == '_' || chNext == '\\'
						|| chNext == '-' || chNext == '+' || chNext == '!') {
						// an unquoted here-doc delimiter, no special handling
                        // TODO check what exactly bash considers part of the delim
					} else if (chNext == '<') {	// HERE string <<<
						i++;
						ch = chNext;
						chNext = chNext2;
						styler.ColourTo(i, SCE_SH_HERE_DELIM);
						state = SCE_SH_DEFAULT;
						HereDoc.State = 0;
					} else if (isspacechar(chNext)) {
						// eat whitespace
						HereDoc.State = 0;
					} else if (isdigit(chNext) || chNext == '=' || chNext == '$') {
						// left shift << or <<= operator cases
						styler.ColourTo(i, SCE_SH_OPERATOR);
						state = SCE_SH_DEFAULT;
						HereDoc.State = 0;
					} else {
						// symbols terminates; deprecated zero-length delimiter
					}
				} else if (HereDoc.State == 1) { // collect the delimiter
					if (HereDoc.Quoted) { // a quoted here-doc delimiter
						if (ch == HereDoc.Quote) { // closing quote => end of delimiter
							styler.ColourTo(i, state);
							state = SCE_SH_DEFAULT;
						} else {
							if (ch == '\\' && chNext == HereDoc.Quote) { // escaped quote
								i++;
								ch = chNext;
								chNext = chNext2;
							}
							HereDoc.Delimiter[HereDoc.DelimiterLength++] = ch;
							HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
						}
					} else { // an unquoted here-doc delimiter
						if (isalnum(ch) || ch == '_' || ch == '-' || ch == '+' || ch == '!') {
							HereDoc.Delimiter[HereDoc.DelimiterLength++] = ch;
							HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
						} else if (ch == '\\') {
							// skip escape prefix
						} else {
							styler.ColourTo(i - 1, state);
							state = SCE_SH_DEFAULT;
							goto restartLexer;
						}
					}
					if (HereDoc.DelimiterLength >= HERE_DELIM_MAX - 1) {
						styler.ColourTo(i - 1, state);
						state = SCE_SH_ERROR;
						goto restartLexer;
					}
				}
			} else if (HereDoc.State == 2) {
				// state == SCE_SH_HERE_Q
				if (isMatch(styler, lengthDoc, i, HereDoc.Delimiter)) {
					if (!HereDoc.Indent && isEOLChar(chPrev)) {
					endHereDoc:
						// standard HERE delimiter
						i += HereDoc.DelimiterLength;
						chPrev = styler.SafeGetCharAt(i - 1);
						ch = styler.SafeGetCharAt(i);
						if (isEOLChar(ch)) {
							styler.ColourTo(i - 1, state);
							state = SCE_SH_DEFAULT;
							HereDoc.State = 0;
							goto restartLexer;
						}
						chNext = styler.SafeGetCharAt(i + 1);
					} else if (HereDoc.Indent) {
						// indented HERE delimiter
						unsigned int bk = (i > 0)? i - 1: 0;
						while (i > 0) {
							ch = styler.SafeGetCharAt(bk--);
							if (isEOLChar(ch)) {
								goto endHereDoc;
							} else if (!isspacechar(ch)) {
								break;	// got leading non-whitespace
							}
						}
					}
				}
			} else if (state == SCE_SH_SCALAR) {	// variable names
				if (isEndVar(ch)) {
					if ((state == SCE_SH_SCALAR)
					    && i == (styler.GetStartSegment() + 1)) {
						// Special variable: $(, $_ etc.
						styler.ColourTo(i, state);
						state = SCE_SH_DEFAULT;
					} else {
						styler.ColourTo(i - 1, state);
						state = SCE_SH_DEFAULT;
						goto restartLexer;
					}
				}
			} else if (state == SCE_SH_STRING
				|| state == SCE_SH_CHARACTER
				|| state == SCE_SH_BACKTICKS
				|| state == SCE_SH_PARAM
				) {
				if (!Quote.Down && !isspacechar(ch)) {
					Quote.Open(ch);
				} else if (ch == '\\' && Quote.Up != '\\') {
					i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
				} else if (ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0) {
						Quote.Rep--;
						if (Quote.Rep <= 0) {
							styler.ColourTo(i, state);
							state = SCE_SH_DEFAULT;
							ch = ' ';
						}
						if (Quote.Up == Quote.Down) {
							Quote.Count++;
						}
					}
				} else if (ch == Quote.Up) {
					Quote.Count++;
				}
			}
		}
		if (state == SCE_SH_ERROR) {
			break;
		}
		chPrev = ch;
	}
	styler.ColourTo(lengthDoc - 1, state);
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
                     && !IsCommentLine(lineCurrent+1, styler))
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
