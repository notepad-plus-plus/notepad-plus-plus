// Scintilla source code edit control
/** @file LexPerl.cxx
 ** Lexer for Perl.
 **/
// Copyright 1998-2008 by Neil Hodgson <neilh@scintilla.org>
// Lexical analysis fixes by Kein-Hong Man <mkh@pl.jaring.my>
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

// Info for HERE document handling from perldata.pod (reformatted):
// ----------------------------------------------------------------
// A line-oriented form of quoting is based on the shell ``here-doc'' syntax.
// Following a << you specify a string to terminate the quoted material, and
// all lines following the current line down to the terminating string are
// the value of the item.
// * The terminating string may be either an identifier (a word), or some
//   quoted text.
// * If quoted, the type of quotes you use determines the treatment of the
//   text, just as in regular quoting.
// * An unquoted identifier works like double quotes.
// * There must be no space between the << and the identifier.
//   (If you put a space it will be treated as a null identifier,
//    which is valid, and matches the first empty line.)
//   (This is deprecated, -w warns of this syntax)
// * The terminating string must appear by itself (unquoted and
//   with no surrounding whitespace) on the terminating line.

#define HERE_DELIM_MAX 256		// maximum length of HERE doc delimiter

#define PERLNUM_BINARY		1	// order is significant: 1-4 cannot have a dot
#define PERLNUM_HEX			2
#define PERLNUM_OCTAL		3
#define PERLNUM_FLOAT_EXP	4	// exponent part only
#define PERLNUM_DECIMAL		5	// 1-5 are numbers; 6-7 are strings
#define PERLNUM_VECTOR		6
#define PERLNUM_V_VECTOR	7
#define PERLNUM_BAD			8

#define BACK_NONE		0	// lookback state for bareword disambiguation:
#define BACK_OPERATOR	1	// whitespace/comments are insignificant
#define BACK_KEYWORD	2	// operators/keywords are needed for disambiguation

static bool isPerlKeyword(unsigned int start, unsigned int end, WordList &keywords, Accessor &styler)
{
	// old-style keyword matcher; needed because GetCurrent() needs
	// current segment to be committed, but we may abandon early...
	char s[100];
	unsigned int i, len = end - start;
	if (len > 30) { len = 30; }
	for (i = 0; i < len; i++, start++) s[i] = styler[start];
	s[i] = '\0';
	return keywords.InList(s);
}

static int disambiguateBareword(Accessor &styler, unsigned int bk, unsigned int fw,
                                int backFlag, unsigned int backPos, unsigned int endPos)
{
	// identifiers are recognized by Perl as barewords under some
	// conditions, the following attempts to do the disambiguation
	// by looking backward and forward; result in 2 LSB
	int result = 0;
	bool moreback = false;		// true if passed newline/comments
	bool brace = false;			// true if opening brace found
	// if BACK_NONE, neither operator nor keyword, so skip test
	if (backFlag == BACK_NONE)
		return result;
	// first look backwards past whitespace/comments to set EOL flag
	// (some disambiguation patterns must be on a single line)
	if (backPos <= static_cast<unsigned int>(styler.LineStart(styler.GetLine(bk))))
		moreback = true;
	// look backwards at last significant lexed item for disambiguation
	bk = backPos - 1;
	int ch = static_cast<unsigned char>(styler.SafeGetCharAt(bk));
	if (ch == '{' && !moreback) {
		// {bareword: possible variable spec
		brace = true;
	} else if ((ch == '&' && styler.SafeGetCharAt(bk - 1) != '&')
			// &bareword: subroutine call
			   || styler.Match(bk - 1, "->")
			// ->bareword: part of variable spec
			   || styler.Match(bk - 2, "sub")) {
			// sub bareword: subroutine declaration
			// (implied BACK_KEYWORD, no keywords end in 'sub'!)
		result |= 1;
	}
	// next, scan forward after word past tab/spaces only;
	// if ch isn't one of '[{(,' we can skip the test
	if ((ch == '{' || ch == '(' || ch == '['|| ch == ',')
		&& fw < endPos) {
		while (ch = static_cast<unsigned char>(styler.SafeGetCharAt(fw)),
			   IsASpaceOrTab(ch) && fw < endPos) {
			fw++;
		}
		if ((ch == '}' && brace)
			// {bareword}: variable spec
			|| styler.Match(fw, "=>")) {
			// [{(, bareword=>: hash literal
			result |= 2;
		}
	}
	return result;
}

static void skipWhitespaceComment(Accessor &styler, unsigned int &p)
{
	// when backtracking, we need to skip whitespace and comments
	int style;
	while ((p > 0) && (style = styler.StyleAt(p),
		   style == SCE_PL_DEFAULT || style == SCE_PL_COMMENTLINE))
		p--;
}

static int styleBeforeBracePair(Accessor &styler, unsigned int bk)
{
	// backtrack to find open '{' corresponding to a '}', balanced
	// return significant style to be tested for '/' disambiguation
	int braceCount = 1;
	if (bk == 0)
		return SCE_PL_DEFAULT;
	while (--bk > 0) {
		if (styler.StyleAt(bk) == SCE_PL_OPERATOR) {
			int bkch = static_cast<unsigned char>(styler.SafeGetCharAt(bk));
			if (bkch == ';') {	// early out
				break;
			} else if (bkch == '}') {
				braceCount++;
			} else if (bkch == '{') {
				if (--braceCount == 0) break;
			}
		}
	}
	if (bk > 0 && braceCount == 0) {
		// balanced { found, bk > 0, skip more whitespace/comments
		bk--;
		skipWhitespaceComment(styler, bk);
		return styler.StyleAt(bk);
	}
	return SCE_PL_DEFAULT;
}

static int styleCheckIdentifier(Accessor &styler, unsigned int bk)
{
	// backtrack to classify sub-styles of identifier under test
	// return sub-style to be tested for '/' disambiguation
	if (styler.SafeGetCharAt(bk) == '>')	// inputsymbol, like <foo>
		return 1;
	// backtrack to check for possible "->" or "::" before identifier
	while (bk > 0 && styler.StyleAt(bk) == SCE_PL_IDENTIFIER) {
		bk--;
	}
	while (bk > 0) {
		int bkstyle = styler.StyleAt(bk);
		if (bkstyle == SCE_PL_DEFAULT
			|| bkstyle == SCE_PL_COMMENTLINE) {
			// skip whitespace, comments
		} else if (bkstyle == SCE_PL_OPERATOR) {
			// test for "->" and "::"
			if (styler.Match(bk - 1, "->") || styler.Match(bk - 1, "::"))
				return 2;
		} else
			return 3;	// bare identifier
		bk--;
	}
	return 0;
}

static int inputsymbolScan(Accessor &styler, unsigned int pos, unsigned int endPos)
{
	// looks forward for matching > on same line; a bit ugly
	unsigned int fw = pos;
	while (++fw < endPos) {
		int fwch = static_cast<unsigned char>(styler.SafeGetCharAt(fw));
		if (fwch == '\r' || fwch == '\n') {
			return 0;
		} else if (fwch == '>') {
			if (styler.Match(fw - 2, "<=>"))	// '<=>' case
				return 0;
			return fw - pos;
		}
	}
	return 0;
}

static int podLineScan(Accessor &styler, unsigned int &pos, unsigned int endPos)
{
	// forward scan the current line to classify line for POD style
	int state = -1;
	while (pos <= endPos) {
		int ch = static_cast<unsigned char>(styler.SafeGetCharAt(pos));
		if (ch == '\n' || ch == '\r' || pos >= endPos) {
			if (ch == '\r' && styler.SafeGetCharAt(pos + 1) == '\n') pos++;
			break;
		}
		if (IsASpaceOrTab(ch)) {	// whitespace, take note
			if (state == -1)
				state = SCE_PL_DEFAULT;
		} else if (state == SCE_PL_DEFAULT) {	// verbatim POD line
			state = SCE_PL_POD_VERB;
		} else if (state != SCE_PL_POD_VERB) {	// regular POD line
			state = SCE_PL_POD;
		}
		pos++;
	}
	if (state == -1)
		state = SCE_PL_DEFAULT;
	return state;
}

static bool styleCheckSubPrototype(Accessor &styler, unsigned int bk)
{
	// backtrack to identify if we're starting a subroutine prototype
	// we also need to ignore whitespace/comments:
	// 'sub' [whitespace|comment] <identifier> [whitespace|comment]
	styler.Flush();
	skipWhitespaceComment(styler, bk);
	if (bk == 0 || styler.StyleAt(bk) != SCE_PL_IDENTIFIER)	// check identifier
		return false;
	while (bk > 0 && (styler.StyleAt(bk) == SCE_PL_IDENTIFIER)) {
		bk--;
	}
	skipWhitespaceComment(styler, bk);
	if (bk < 2 || styler.StyleAt(bk) != SCE_PL_WORD	// check "sub" keyword
		|| !styler.Match(bk - 2, "sub"))	// assume suffix is unique!
		return false;
	return true;
}

static bool isMatch(const char *sref, char *s)
{
	// match per-line delimiter - must kill trailing CR if CRLF
	int i = strlen(s);
	if (i != 0 && s[i - 1] == '\r')
		s[i - 1] = '\0';
	return (strcmp(sref, s) == 0);
}

static int actualNumStyle(int numberStyle) {
	if (numberStyle == PERLNUM_VECTOR || numberStyle == PERLNUM_V_VECTOR) {
		return SCE_PL_STRING;
	} else if (numberStyle == PERLNUM_BAD) {
		return SCE_PL_ERROR;
	}
	return SCE_PL_NUMBER;
}

static int opposite(int ch) {
	if (ch == '(') return ')';
	if (ch == '[') return ']';
	if (ch == '{') return '}';
	if (ch == '<') return '>';
	return ch;
}

static void ColourisePerlDoc(unsigned int startPos, int length, int initStyle,
                             WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];

	// keywords that forces /PATTERN/ at all times; should track vim's behaviour
	WordList reWords;
	reWords.Set("elsif if split while");

	// charset classes
	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);
	CharacterSet setSingleCharOp(CharacterSet::setNone, "rwxoRWXOezsfdlpSbctugkTBMAC");
	// lexing of "%*</" operators is non-trivial; these are missing in the set below
	CharacterSet setPerlOperator(CharacterSet::setNone, "^&\\()-+=|{}[]:;>,?!.~");
	CharacterSet setQDelim(CharacterSet::setNone, "qrwx");
	CharacterSet setModifiers(CharacterSet::setAlpha);
	CharacterSet setPreferRE(CharacterSet::setNone, "*/<%");
	// setArray and setHash also accepts chars for special vars like $_,
	// which are then truncated when the next char does not match setVar
	CharacterSet setVar(CharacterSet::setAlphaNum, "#$_'", 0x80, true);
	CharacterSet setArray(CharacterSet::setAlpha, "#$_+-", 0x80, true);
	CharacterSet setHash(CharacterSet::setAlpha, "#$_!^+-", 0x80, true);
	CharacterSet &setPOD = setModifiers;
	CharacterSet setNonHereDoc(CharacterSet::setDigits, "=$@");
	CharacterSet setHereDocDelim(CharacterSet::setAlphaNum, "_");
	CharacterSet setSubPrototype(CharacterSet::setNone, "\\[$@%&*];");
	// for format identifiers
	CharacterSet setFormatStart(CharacterSet::setAlpha, "_=");
	CharacterSet &setFormat = setHereDocDelim;

	// Lexer for perl often has to backtrack to start of current style to determine
	// which characters are being used as quotes, how deeply nested is the
	// start position and what the termination string is for HERE documents.

	class HereDocCls {	// Class to manage HERE doc sequence
	public:
		int State;		// 0: '<<' encountered
						// 1: collect the delimiter
						// 2: here doc text (lines after the delimiter)
		int Quote;		// the char after '<<'
		bool Quoted;		// true if Quote in ('\'','"','`')
		int DelimiterLength;	// strlen(Delimiter)
		char *Delimiter;	// the Delimiter, 256: sizeof PL_tokenbuf
		HereDocCls() {
			State = 0;
			Quote = 0;
			Quoted = false;
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
	HereDocCls HereDoc;		// TODO: FIFO for stacked here-docs

	class QuoteCls {	// Class to manage quote pairs
		public:
		int Rep;
		int Count;
		int Up, Down;
		QuoteCls() {
			this->New(1);
		}
		void New(int r = 1) {
			Rep   = r;
			Count = 0;
			Up    = '\0';
			Down  = '\0';
		}
		void Open(int u) {
			Count++;
			Up    = u;
			Down  = opposite(Up);
		}
	};
	QuoteCls Quote;

	// additional state for number lexing
	int numState = PERLNUM_DECIMAL;
	int dotCount = 0;

	unsigned int endPos = startPos + length;

	// Backtrack to beginning of style if required...
	// If in a long distance lexical state, backtrack to find quote characters.
	// Includes strings (may be multi-line), numbers (additional state), format
	// bodies, as well as POD sections.
	if (initStyle == SCE_PL_HERE_Q
		|| initStyle == SCE_PL_HERE_QQ
		|| initStyle == SCE_PL_HERE_QX
		|| initStyle == SCE_PL_FORMAT
	) {
		int delim = (initStyle == SCE_PL_FORMAT) ? SCE_PL_FORMAT_IDENT:SCE_PL_HERE_DELIM;
		while ((startPos > 1) && (styler.StyleAt(startPos) != delim)) {
			startPos--;
		}
		startPos = styler.LineStart(styler.GetLine(startPos));
		initStyle = styler.StyleAt(startPos - 1);
	}
	if (initStyle == SCE_PL_STRING_Q
		|| initStyle == SCE_PL_STRING_QQ
		|| initStyle == SCE_PL_STRING_QX
		|| initStyle == SCE_PL_STRING_QR
		|| initStyle == SCE_PL_STRING_QW
		|| initStyle == SCE_PL_REGEX
		|| initStyle == SCE_PL_REGSUBST
		|| initStyle == SCE_PL_STRING
		|| initStyle == SCE_PL_BACKTICKS
		|| initStyle == SCE_PL_CHARACTER
		|| initStyle == SCE_PL_NUMBER
		|| initStyle == SCE_PL_IDENTIFIER
		|| initStyle == SCE_PL_ERROR
		|| initStyle == SCE_PL_SUB_PROTOTYPE
	) {
		while ((startPos > 1) && (styler.StyleAt(startPos - 1) == initStyle)) {
			startPos--;
		}
		initStyle = SCE_PL_DEFAULT;
	} else if (initStyle == SCE_PL_POD
			   || initStyle == SCE_PL_POD_VERB
	) {
		// POD backtracking finds preceeding blank lines and goes back past them
		int ln = styler.GetLine(startPos);
		if (ln > 0) {
			initStyle = styler.StyleAt(styler.LineStart(--ln));
			if (initStyle == SCE_PL_POD || initStyle == SCE_PL_POD_VERB) {
				while (ln > 0 && styler.GetLineState(ln) == SCE_PL_DEFAULT)
					ln--;
			}
			startPos = styler.LineStart(++ln);
			initStyle = styler.StyleAt(startPos - 1);
		} else {
			startPos = 0;
			initStyle = SCE_PL_DEFAULT;
		}
	}

	// backFlag, backPos are additional state to aid identifier corner cases.
	// Look backwards past whitespace and comments in order to detect either
	// operator or keyword. Later updated as we go along.
	int backFlag = BACK_NONE;
	unsigned int backPos = startPos;
	if (backPos > 0) {
		backPos--;
		skipWhitespaceComment(styler, backPos);
		if (styler.StyleAt(backPos) == SCE_PL_OPERATOR)
			backFlag = BACK_OPERATOR;
		else if (styler.StyleAt(backPos) == SCE_PL_WORD)
			backFlag = BACK_KEYWORD;
		backPos++;
	}

	StyleContext sc(startPos, endPos - startPos, initStyle, styler, static_cast<char>(STYLE_MAX));

	for (; sc.More(); sc.Forward()) {

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_PL_OPERATOR:
				sc.SetState(SCE_PL_DEFAULT);
				backFlag = BACK_OPERATOR;
				backPos = sc.currentPos;
				break;
			case SCE_PL_IDENTIFIER:		// identifier, bareword, inputsymbol
				if ((!setWord.Contains(sc.ch) && sc.ch != '\'')
					|| sc.Match('.', '.')
					|| sc.chPrev == '>') {	// end of inputsymbol
					sc.SetState(SCE_PL_DEFAULT);
				}
				break;
			case SCE_PL_WORD:		// keyword, plus special cases
				if (!setWord.Contains(sc.ch)) {
					char s[100];
					sc.GetCurrent(s, sizeof(s));
					if ((strcmp(s, "__DATA__") == 0) || (strcmp(s, "__END__") == 0)) {
						sc.ChangeState(SCE_PL_DATASECTION);
					} else {
						if ((strcmp(s, "format") == 0)) {
							sc.SetState(SCE_PL_FORMAT_IDENT);
							HereDoc.State = 0;
						} else {
							sc.SetState(SCE_PL_DEFAULT);
						}
						backFlag = BACK_KEYWORD;
						backPos = sc.currentPos;
					}
				}
				break;
			case SCE_PL_SCALAR:
			case SCE_PL_ARRAY:
			case SCE_PL_HASH:
			case SCE_PL_SYMBOLTABLE:
				if (sc.Match(':', ':')) {	// skip ::
					sc.Forward();
				} else if (!setVar.Contains(sc.ch)) {
					if (sc.LengthCurrent() == 1) {
						// Special variable: $(, $_ etc.
						sc.Forward();
					}
					sc.SetState(SCE_PL_DEFAULT);
				}
				break;
			case SCE_PL_NUMBER:
				// if no early break, number style is terminated at "(go through)"
				if (sc.ch == '.') {
					if (sc.chNext == '.') {
						// double dot is always an operator (go through)
					} else if (numState <= PERLNUM_FLOAT_EXP) {
						// non-decimal number or float exponent, consume next dot
						sc.SetState(SCE_PL_OPERATOR);
						break;
					} else {	// decimal or vectors allows dots
						dotCount++;
						if (numState == PERLNUM_DECIMAL) {
							if (dotCount <= 1)	// number with one dot in it
								break;
							if (IsADigit(sc.chNext)) {	// really a vector
								numState = PERLNUM_VECTOR;
								break;
							}
							// number then dot (go through)
						} else if (IsADigit(sc.chNext))	// vectors
							break;
						// vector then dot (go through)
					}
				} else if (sc.ch == '_') {
					// permissive underscoring for number and vector literals
					break;
				} else if (numState == PERLNUM_DECIMAL) {
					if (sc.ch == 'E' || sc.ch == 'e') {	// exponent, sign
						numState = PERLNUM_FLOAT_EXP;
						if (sc.chNext == '+' || sc.chNext == '-') {
							sc.Forward();
						}
						break;
					} else if (IsADigit(sc.ch))
						break;
					// number then word (go through)
				} else if (numState == PERLNUM_HEX) {
					if (IsADigit(sc.ch, 16))
						break;
				} else if (numState == PERLNUM_VECTOR || numState == PERLNUM_V_VECTOR) {
					if (IsADigit(sc.ch))	// vector
						break;
					if (setWord.Contains(sc.ch) && dotCount == 0) {	// change to word
						sc.ChangeState(SCE_PL_IDENTIFIER);
						break;
					}
					// vector then word (go through)
				} else if (IsADigit(sc.ch)) {
					if (numState == PERLNUM_FLOAT_EXP) {
						break;
					} else if (numState == PERLNUM_OCTAL) {
						if (sc.ch <= '7') break;
					} else if (numState == PERLNUM_BINARY) {
						if (sc.ch <= '1') break;
					}
					// mark invalid octal, binary numbers (go through)
					numState = PERLNUM_BAD;
					break;
				}
				// complete current number or vector
				sc.ChangeState(actualNumStyle(numState));
				sc.SetState(SCE_PL_DEFAULT);
				break;
			case SCE_PL_COMMENTLINE:
				if (sc.atLineEnd) {
					sc.SetState(SCE_PL_DEFAULT);
				}
				break;
			case SCE_PL_HERE_DELIM:
				if (HereDoc.State == 0) { // '<<' encountered
					int delim_ch = sc.chNext;
					int ws_skip = 0;
					HereDoc.State = 1;	// pre-init HERE doc class
					HereDoc.Quote = sc.chNext;
					HereDoc.Quoted = false;
					HereDoc.DelimiterLength = 0;
					HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
					if (IsASpaceOrTab(delim_ch)) {
						// skip whitespace; legal only for quoted delimiters
						unsigned int i = sc.currentPos + 1;
						while ((i < endPos) && IsASpaceOrTab(delim_ch)) {
							i++;
							delim_ch = static_cast<unsigned char>(styler.SafeGetCharAt(i));
						}
						ws_skip = i - sc.currentPos - 1;
					}
					if (delim_ch == '\'' || delim_ch == '"' || delim_ch == '`') {
						// a quoted here-doc delimiter; skip any whitespace
						sc.Forward(ws_skip + 1);
						HereDoc.Quote = delim_ch;
						HereDoc.Quoted = true;
					} else if ((ws_skip == 0 && setNonHereDoc.Contains(sc.chNext))
							   || ws_skip > 0) {
						// left shift << or <<= operator cases
						// restore position if operator
						sc.ChangeState(SCE_PL_OPERATOR);
						sc.ForwardSetState(SCE_PL_DEFAULT);
						backFlag = BACK_OPERATOR;
						backPos = sc.currentPos;
						HereDoc.State = 0;
					} else {
						// specially handle initial '\' for identifier
						if (ws_skip == 0 && HereDoc.Quote == '\\')
							sc.Forward();
						// an unquoted here-doc delimiter, no special handling
						// (cannot be prefixed by spaces/tabs), or
						// symbols terminates; deprecated zero-length delimiter
					}
				} else if (HereDoc.State == 1) { // collect the delimiter
					backFlag = BACK_NONE;
					if (HereDoc.Quoted) { // a quoted here-doc delimiter
						if (sc.ch == HereDoc.Quote) { // closing quote => end of delimiter
							sc.ForwardSetState(SCE_PL_DEFAULT);
						} else if (!sc.atLineEnd) {
							if (sc.Match('\\', static_cast<char>(HereDoc.Quote))) { // escaped quote
								sc.Forward();
							}
							if (sc.ch != '\r') {	// skip CR if CRLF
								HereDoc.Append(sc.ch);
							}
						}
					} else { // an unquoted here-doc delimiter
						if (setHereDocDelim.Contains(sc.ch)) {
							HereDoc.Append(sc.ch);
						} else {
							sc.SetState(SCE_PL_DEFAULT);
						}
					}
					if (HereDoc.DelimiterLength >= HERE_DELIM_MAX - 1) {
						sc.SetState(SCE_PL_ERROR);
						HereDoc.State = 0;
					}
				}
				break;
			case SCE_PL_HERE_Q:
			case SCE_PL_HERE_QQ:
			case SCE_PL_HERE_QX: {
				// also implies HereDoc.State == 2
				sc.Complete();
				while (!sc.atLineEnd)
					sc.Forward();
				char s[HERE_DELIM_MAX];
				sc.GetCurrent(s, sizeof(s));
				if (isMatch(HereDoc.Delimiter, s)) {
					sc.SetState(SCE_PL_DEFAULT);
					backFlag = BACK_NONE;
					HereDoc.State = 0;
				}
				} break;
			case SCE_PL_POD:
			case SCE_PL_POD_VERB: {
				unsigned int fw = sc.currentPos;
				int ln = styler.GetLine(fw);
				if (sc.atLineStart && sc.Match("=cut")) {	// end of POD
					sc.SetState(SCE_PL_POD);
					sc.Forward(4);
					sc.SetState(SCE_PL_DEFAULT);
					styler.SetLineState(ln, SCE_PL_POD);
					break;
				}
				int pod = podLineScan(styler, fw, endPos);	// classify POD line
				styler.SetLineState(ln, pod);
				if (pod == SCE_PL_DEFAULT) {
					if (sc.state == SCE_PL_POD_VERB) {
						unsigned int fw2 = fw;
						while (fw2 <= endPos && pod == SCE_PL_DEFAULT) {
							fw = fw2++;	// penultimate line (last blank line)
							pod = podLineScan(styler, fw2, endPos);
							styler.SetLineState(styler.GetLine(fw2), pod);
						}
						if (pod == SCE_PL_POD) {	// truncate verbatim POD early
							sc.SetState(SCE_PL_POD);
						} else
							fw = fw2;
					} else
						pod = SCE_PL_POD;
				} else {
					if (pod == SCE_PL_POD_VERB	// still part of current paragraph
					    && (styler.GetLineState(ln - 1) == SCE_PL_POD)) {
						pod = SCE_PL_POD;
						styler.SetLineState(ln, pod);
					} else if (pod == SCE_PL_POD
							   && (styler.GetLineState(ln - 1) == SCE_PL_POD_VERB)) {
						pod = SCE_PL_POD_VERB;
						styler.SetLineState(ln, pod);
					}
					sc.SetState(pod);
				}
				sc.Forward(fw - sc.currentPos);	// commit style
				} break;
			case SCE_PL_REGEX:
			case SCE_PL_STRING_QR:
				if (Quote.Rep <= 0) {
					if (!setModifiers.Contains(sc.ch))
						sc.SetState(SCE_PL_DEFAULT);
				} else if (!Quote.Up && !IsASpace(sc.ch)) {
					Quote.Open(sc.ch);
				} else if (sc.ch == '\\' && Quote.Up != '\\') {
					sc.Forward();
				} else if (sc.ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0)
						Quote.Rep--;
				} else if (sc.ch == Quote.Up) {
					Quote.Count++;
				}
				break;
			case SCE_PL_REGSUBST:
				if (Quote.Rep <= 0) {
					if (!setModifiers.Contains(sc.ch))
						sc.SetState(SCE_PL_DEFAULT);
				} else if (!Quote.Up && !IsASpace(sc.ch)) {
					Quote.Open(sc.ch);
				} else if (sc.ch == '\\' && Quote.Up != '\\') {
					sc.Forward();
				} else if (Quote.Count == 0 && Quote.Rep == 1) {
					// We matched something like s(...) or tr{...}, Perl 5.10
					// appears to allow almost any character for use as the
					// next delimiters. Whitespace and comments are accepted in
					// between, but we'll limit to whitespace here.
					// For '#', if no whitespace in between, it's a delimiter.
					if (IsASpace(sc.ch)) {
						// Keep going
					} else if (sc.ch == '#' && IsASpaceOrTab(sc.chPrev)) {
						sc.SetState(SCE_PL_DEFAULT);
					} else {
						Quote.Open(sc.ch);
					}
				} else if (sc.ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0)
						Quote.Rep--;
					if (Quote.Up == Quote.Down)
						Quote.Count++;
				} else if (sc.ch == Quote.Up) {
					Quote.Count++;
				}
				break;
			case SCE_PL_STRING_Q:
			case SCE_PL_STRING_QQ:
			case SCE_PL_STRING_QX:
			case SCE_PL_STRING_QW:
			case SCE_PL_STRING:
			case SCE_PL_CHARACTER:
			case SCE_PL_BACKTICKS:
				if (!Quote.Down && !IsASpace(sc.ch)) {
					Quote.Open(sc.ch);
				} else if (sc.ch == '\\' && Quote.Up != '\\') {
					sc.Forward();
				} else if (sc.ch == Quote.Down) {
					Quote.Count--;
					if (Quote.Count == 0)
						sc.ForwardSetState(SCE_PL_DEFAULT);
				} else if (sc.ch == Quote.Up) {
					Quote.Count++;
				}
				break;
			case SCE_PL_SUB_PROTOTYPE: {
				int i = 0;
				// forward scan; must all be valid proto characters
				while (setSubPrototype.Contains(sc.GetRelative(i)))
					i++;
				if (sc.GetRelative(i) == ')') {	// valid sub prototype
					sc.Forward(i);
					sc.ForwardSetState(SCE_PL_DEFAULT);
				} else {
					// abandon prototype, restart from '('
					sc.ChangeState(SCE_PL_OPERATOR);
					sc.SetState(SCE_PL_DEFAULT);
				}
				} break;
			case SCE_PL_FORMAT: {
				sc.Complete();
				while (!sc.atLineEnd)
					sc.Forward();
				char s[10];
				sc.GetCurrent(s, sizeof(s));
				if (isMatch(".", s))
					sc.SetState(SCE_PL_DEFAULT);
				} break;
			case SCE_PL_ERROR:
				break;
		}
		// Needed for specific continuation styles (one follows the other)
		switch (sc.state) {
			// continued from SCE_PL_WORD
			case SCE_PL_FORMAT_IDENT:
				// occupies HereDoc state 3 to avoid clashing with HERE docs
				if (IsASpaceOrTab(sc.ch)) {		// skip whitespace
					sc.ChangeState(SCE_PL_DEFAULT);
					while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd)
						sc.Forward();
					sc.SetState(SCE_PL_FORMAT_IDENT);
				}
				if (setFormatStart.Contains(sc.ch)) {	// identifier or '='
					if (sc.ch != '=') {
						do {
							sc.Forward();
						} while (setFormat.Contains(sc.ch));
					}
					while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd)
						sc.Forward();
					if (sc.ch == '=') {
						sc.ForwardSetState(SCE_PL_DEFAULT);
						HereDoc.State = 3;
					} else {
						// invalid indentifier; inexact fallback, but hey
						sc.ChangeState(SCE_PL_IDENTIFIER);
						sc.SetState(SCE_PL_DEFAULT);
					}
				} else {
					sc.ChangeState(SCE_PL_DEFAULT);	// invalid indentifier
				}
				backFlag = BACK_NONE;
				break;
		}

		// Must check end of HereDoc states here before default state is handled
		if (HereDoc.State == 1 && sc.atLineEnd) {
			// Begin of here-doc (the line after the here-doc delimiter):
			// Lexically, the here-doc starts from the next line after the >>, but the
			// first line of here-doc seem to follow the style of the last EOL sequence
			int st_new = SCE_PL_HERE_QQ;
			HereDoc.State = 2;
			if (HereDoc.Quoted) {
				if (sc.state == SCE_PL_HERE_DELIM) {
					// Missing quote at end of string! We are stricter than perl.
					// Colour here-doc anyway while marking this bit as an error.
					sc.ChangeState(SCE_PL_ERROR);
				}
				switch (HereDoc.Quote) {
					case '\'': st_new = SCE_PL_HERE_Q ; break;
					case '"' : st_new = SCE_PL_HERE_QQ; break;
					case '`' : st_new = SCE_PL_HERE_QX; break;
				}
			} else {
				if (HereDoc.Quote == '\\')
					st_new = SCE_PL_HERE_Q;
			}
			sc.SetState(st_new);
		}
		if (HereDoc.State == 3 && sc.atLineEnd) {
			// Start of format body.
			HereDoc.State = 0;
			sc.SetState(SCE_PL_FORMAT);
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_PL_DEFAULT) {
			if (IsADigit(sc.ch) ||
				(IsADigit(sc.chNext) && (sc.ch == '.' || sc.ch == 'v'))) {
				sc.SetState(SCE_PL_NUMBER);
				backFlag = BACK_NONE;
				numState = PERLNUM_DECIMAL;
				dotCount = 0;
				if (sc.ch == '0') {		// hex,bin,octal
					if (sc.chNext == 'x') {
						numState = PERLNUM_HEX;
					} else if (sc.chNext == 'b') {
						numState = PERLNUM_BINARY;
					} else if (IsADigit(sc.chNext)) {
						numState = PERLNUM_OCTAL;
					}
					if (numState != PERLNUM_DECIMAL) {
						sc.Forward();
					}
				} else if (sc.ch == 'v') {		// vector
					numState = PERLNUM_V_VECTOR;
				}
			} else if (setWord.Contains(sc.ch)) {
				// if immediately prefixed by '::', always a bareword
				sc.SetState(SCE_PL_WORD);
				if (sc.chPrev == ':' && sc.GetRelative(-2) == ':') {
					sc.ChangeState(SCE_PL_IDENTIFIER);
				}
				unsigned int bk = sc.currentPos;
				unsigned int fw = sc.currentPos + 1;
				// first check for possible quote-like delimiter
				if (sc.ch == 's' && !setWord.Contains(sc.chNext)) {
					sc.ChangeState(SCE_PL_REGSUBST);
					Quote.New(2);
				} else if (sc.ch == 'm' && !setWord.Contains(sc.chNext)) {
					sc.ChangeState(SCE_PL_REGEX);
					Quote.New();
				} else if (sc.ch == 'q' && !setWord.Contains(sc.chNext)) {
					sc.ChangeState(SCE_PL_STRING_Q);
					Quote.New();
				} else if (sc.ch == 'y' && !setWord.Contains(sc.chNext)) {
					sc.ChangeState(SCE_PL_REGSUBST);
					Quote.New(2);
				} else if (sc.Match('t', 'r') && !setWord.Contains(sc.GetRelative(2))) {
					sc.ChangeState(SCE_PL_REGSUBST);
					Quote.New(2);
					sc.Forward();
					fw++;
				} else if (sc.ch == 'q' && setQDelim.Contains(sc.chNext)
						   && !setWord.Contains(sc.GetRelative(2))) {
					if      (sc.chNext == 'q') sc.ChangeState(SCE_PL_STRING_QQ);
					else if (sc.chNext == 'x') sc.ChangeState(SCE_PL_STRING_QX);
					else if (sc.chNext == 'r') sc.ChangeState(SCE_PL_STRING_QR);
					else sc.ChangeState(SCE_PL_STRING_QW);	// sc.chNext == 'w'
					Quote.New();
					sc.Forward();
					fw++;
				} else if (sc.ch == 'x' && (sc.chNext == '=' ||	// repetition
						   !setWord.Contains(sc.chNext) ||
						   (IsADigit(sc.chPrev) && IsADigit(sc.chNext)))) {
					sc.ChangeState(SCE_PL_OPERATOR);
				}
				// if potentially a keyword, scan forward and grab word, then check
				// if it's really one; if yes, disambiguation test is performed
				// otherwise it is always a bareword and we skip a lot of scanning
				if (sc.state == SCE_PL_WORD) {
					while (setWord.Contains(static_cast<unsigned char>(styler.SafeGetCharAt(fw))))
						fw++;
					if (!isPerlKeyword(styler.GetStartSegment(), fw, keywords, styler)) {
						sc.ChangeState(SCE_PL_IDENTIFIER);
					}
				}
				// if already SCE_PL_IDENTIFIER, then no ambiguity, skip this
				// for quote-like delimiters/keywords, attempt to disambiguate
				// to select for bareword, change state -> SCE_PL_IDENTIFIER
				if (sc.state != SCE_PL_IDENTIFIER && bk > 0) {
					if (disambiguateBareword(styler, bk, fw, backFlag, backPos, endPos))
						sc.ChangeState(SCE_PL_IDENTIFIER);
				}
				backFlag = BACK_NONE;
			} else if (sc.ch == '#') {
				sc.SetState(SCE_PL_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_PL_STRING);
				Quote.New();
				Quote.Open(sc.ch);
				backFlag = BACK_NONE;
			} else if (sc.ch == '\'') {
				if (sc.chPrev == '&' && setWordStart.Contains(sc.chNext)) {
					// Archaic call
					sc.SetState(SCE_PL_IDENTIFIER);
				} else {
					sc.SetState(SCE_PL_CHARACTER);
					Quote.New();
					Quote.Open(sc.ch);
				}
				backFlag = BACK_NONE;
			} else if (sc.ch == '`') {
				sc.SetState(SCE_PL_BACKTICKS);
				Quote.New();
				Quote.Open(sc.ch);
				backFlag = BACK_NONE;
			} else if (sc.ch == '$') {
				sc.SetState(SCE_PL_SCALAR);
				if (sc.chNext == '{') {
					sc.ForwardSetState(SCE_PL_OPERATOR);
				} else if (IsASpace(sc.chNext)) {
					sc.ForwardSetState(SCE_PL_DEFAULT);
				} else {
					sc.Forward();
					if (sc.Match('`', '`') || sc.Match(':', ':')) {
						sc.Forward();
					}
				}
				backFlag = BACK_NONE;
			} else if (sc.ch == '@') {
				sc.SetState(SCE_PL_ARRAY);
				if (setArray.Contains(sc.chNext)) {
					// no special treatment
				} else if (sc.chNext == ':' && sc.GetRelative(2) == ':') {
					sc.Forward(2);
				} else if (sc.chNext == '{' || sc.chNext == '[') {
					sc.ForwardSetState(SCE_PL_OPERATOR);
				} else {
					sc.ChangeState(SCE_PL_OPERATOR);
				}
				backFlag = BACK_NONE;
			} else if (setPreferRE.Contains(sc.ch)) {
				// Explicit backward peeking to set a consistent preferRE for
				// any slash found, so no longer need to track preferRE state.
				// Find first previous significant lexed element and interpret.
				// A few symbols shares this code for disambiguation.
				bool preferRE = false;
				bool isHereDoc = sc.Match('<', '<');
				bool hereDocSpace = false;		// for: SCALAR [whitespace] '<<'
				unsigned int bk = (sc.currentPos > 0) ? sc.currentPos - 1: 0;
				unsigned int bkend;
				sc.Complete();
				styler.Flush();
				if (styler.StyleAt(bk) == SCE_PL_DEFAULT)
					hereDocSpace = true;
				skipWhitespaceComment(styler, bk);
				if (bk == 0) {
					// avoid backward scanning breakage
					preferRE = true;
				} else {
					int bkstyle = styler.StyleAt(bk);
					int bkch = static_cast<unsigned char>(styler.SafeGetCharAt(bk));
					switch(bkstyle) {
					case SCE_PL_OPERATOR:
						preferRE = true;
						if (bkch == ')' || bkch == ']') {
							preferRE = false;
						} else if (bkch == '}') {
							// backtrack by counting balanced brace pairs
							// needed to test for variables like ${}, @{} etc.
							bkstyle = styleBeforeBracePair(styler, bk);
							if (bkstyle == SCE_PL_SCALAR
								|| bkstyle == SCE_PL_ARRAY
								|| bkstyle == SCE_PL_HASH
								|| bkstyle == SCE_PL_SYMBOLTABLE
								|| bkstyle == SCE_PL_OPERATOR) {
								preferRE = false;
							}
						} else if (bkch == '+' || bkch == '-') {
							if (bkch == static_cast<unsigned char>(styler.SafeGetCharAt(bk - 1))
								&& bkch != static_cast<unsigned char>(styler.SafeGetCharAt(bk - 2)))
							// exceptions for operators: unary suffixes ++, --
							preferRE = false;
						}
						break;
					case SCE_PL_IDENTIFIER:
						preferRE = true;
						bkstyle = styleCheckIdentifier(styler, bk);
						if ((bkstyle == 1) || (bkstyle == 2)) {
							// inputsymbol or var with "->" or "::" before identifier
							preferRE = false;
						} else if (bkstyle == 3) {
							// bare identifier, test cases follows:
							if (sc.ch == '/') {
								// if '/', /PATTERN/ unless digit/space immediately after '/'
								// if '//', always expect defined-or operator to follow identifier
								if (IsASpace(sc.chNext) || IsADigit(sc.chNext) || sc.chNext == '/')
									preferRE = false;
							} else if (sc.ch == '*' || sc.ch == '%') {
								if (IsASpace(sc.chNext) || IsADigit(sc.chNext) || sc.Match('*', '*'))
									preferRE = false;
							} else if (sc.ch == '<') {
								if (IsASpace(sc.chNext) || sc.chNext == '=')
									preferRE = false;
							}
						}
						break;
					case SCE_PL_SCALAR:		// for $var<< case:
						if (isHereDoc && hereDocSpace)	// if SCALAR whitespace '<<', *always* a HERE doc
							preferRE = true;
						break;
					case SCE_PL_WORD:
						preferRE = true;
						// for HERE docs, always true
						if (sc.ch == '/') {
							// adopt heuristics similar to vim-style rules:
							// keywords always forced as /PATTERN/: split, if, elsif, while
							// everything else /PATTERN/ unless digit/space immediately after '/'
							// for '//', defined-or favoured unless special keywords
							bkend = bk + 1;
							while (bk > 0 && styler.StyleAt(bk - 1) == SCE_PL_WORD) {
								bk--;
							}
							if (isPerlKeyword(bk, bkend, reWords, styler))
								break;
							if (IsASpace(sc.chNext) || IsADigit(sc.chNext) || sc.chNext == '/')
								preferRE = false;
						} else if (sc.ch == '*' || sc.ch == '%') {
							if (IsASpace(sc.chNext) || IsADigit(sc.chNext) || sc.Match('*', '*'))
								preferRE = false;
						} else if (sc.ch == '<') {
							if (IsASpace(sc.chNext) || sc.chNext == '=')
								preferRE = false;
						}
						break;
					// other styles uses the default, preferRE=false
					case SCE_PL_POD:
					case SCE_PL_HERE_Q:
					case SCE_PL_HERE_QQ:
					case SCE_PL_HERE_QX:
						preferRE = true;
						break;
					}
				}
				backFlag = BACK_NONE;
				if (isHereDoc) {	// handle '<<', HERE doc
					if (preferRE) {
						sc.SetState(SCE_PL_HERE_DELIM);
						HereDoc.State = 0;
					} else {		// << operator
						sc.SetState(SCE_PL_OPERATOR);
						sc.Forward();
					}
				} else if (sc.ch == '*') {	// handle '*', typeglob
					if (preferRE) {
						sc.SetState(SCE_PL_SYMBOLTABLE);
						if (sc.chNext == ':' && sc.GetRelative(2) == ':') {
							sc.Forward(2);
						} else if (sc.chNext == '{') {
							sc.ForwardSetState(SCE_PL_OPERATOR);
						} else {
							sc.Forward();
						}
					} else {
						sc.SetState(SCE_PL_OPERATOR);
						if (sc.chNext == '*') 	// exponentiation
							sc.Forward();
					}
				} else if (sc.ch == '%') {	// handle '%', hash
					if (preferRE) {
						sc.SetState(SCE_PL_HASH);
						if (setHash.Contains(sc.chNext)) {
							sc.Forward();
						} else if (sc.chNext == ':' && sc.GetRelative(2) == ':') {
							sc.Forward(2);
						} else if (sc.chNext == '{') {
							sc.ForwardSetState(SCE_PL_OPERATOR);
						} else {
							sc.ChangeState(SCE_PL_OPERATOR);
						}
					} else {
						sc.SetState(SCE_PL_OPERATOR);
					}
				} else if (sc.ch == '<') {	// handle '<', inputsymbol
					if (preferRE) {
						// forward scan
						int i = inputsymbolScan(styler, sc.currentPos, endPos);
						if (i > 0) {
							sc.SetState(SCE_PL_IDENTIFIER);
							sc.Forward(i);
						} else {
							sc.SetState(SCE_PL_OPERATOR);
						}
					} else {
						sc.SetState(SCE_PL_OPERATOR);
					}
				} else {			// handle '/', regexp
					if (preferRE) {
						sc.SetState(SCE_PL_REGEX);
						Quote.New();
						Quote.Open(sc.ch);
					} else {		// / and // operators
						sc.SetState(SCE_PL_OPERATOR);
						if (sc.chNext == '/') {
							sc.Forward();
						}
					}
				}
			} else if (sc.ch == '='		// POD
					   && setPOD.Contains(sc.chNext)
					   && sc.atLineStart) {
				sc.SetState(SCE_PL_POD);
				backFlag = BACK_NONE;
			} else if (sc.ch == '-' && setWordStart.Contains(sc.chNext)) {	// extended '-' cases
				unsigned int bk = sc.currentPos;
				unsigned int fw = 2;
				if (setSingleCharOp.Contains(sc.chNext) &&	// file test operators
					!setWord.Contains(sc.GetRelative(2))) {
					sc.SetState(SCE_PL_WORD);
				} else {
					// nominally a minus and bareword; find extent of bareword
					while (setWord.Contains(sc.GetRelative(fw)))
						fw++;
					sc.SetState(SCE_PL_OPERATOR);
				}
				// force to bareword for hash key => or {variable literal} cases
				if (disambiguateBareword(styler, bk, bk + fw, backFlag, backPos, endPos) & 2) {
					sc.ChangeState(SCE_PL_IDENTIFIER);
				}
				backFlag = BACK_NONE;
			} else if (sc.ch == '(' && sc.currentPos > 0) {	// '(' or subroutine prototype
				sc.Complete();
				if (styleCheckSubPrototype(styler, sc.currentPos - 1)) {
					sc.SetState(SCE_PL_SUB_PROTOTYPE);
					backFlag = BACK_NONE;
				} else {
					sc.SetState(SCE_PL_OPERATOR);
				}
			} else if (setPerlOperator.Contains(sc.ch)) {	// operators
				sc.SetState(SCE_PL_OPERATOR);
				if (sc.Match('.', '.')) {	// .. and ...
					sc.Forward();
					if (sc.chNext == '.') sc.Forward();
				}
			} else if (sc.ch == 4 || sc.ch == 26) {		// ^D and ^Z ends valid perl source
				sc.SetState(SCE_PL_DATASECTION);
			} else {
				// keep colouring defaults
				sc.Complete();
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
		int style = styler.StyleAt(i);
		if (ch == '#' && style == SCE_PL_COMMENTLINE)
			return true;
		else if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

static void FoldPerlDoc(unsigned int startPos, int length, int, WordList *[],
                        Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	// Custom folding of POD and packages

	// property fold.perl.pod 
	//	Enable folding Pod blocks when using the Perl lexer. 
	bool foldPOD = styler.GetPropertyInt("fold.perl.pod", 1) != 0;

	// property fold.perl.package 
	//	Enable folding packages when using the Perl lexer. 
	bool foldPackage = styler.GetPropertyInt("fold.perl.package", 1) != 0;

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelPrev = styler.LevelAt(lineCurrent - 1) >> 16;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	char chPrev = styler.SafeGetCharAt(startPos - 1);
	int styleNext = styler.StyleAt(startPos);
	// Used at end of line to determine if the line was a package definition
	bool isPackageLine = false;
	bool isPodHeading = false;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		bool atLineStart = ((chPrev == '\r') || (chPrev == '\n')) || i == 0;
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
		if (style == SCE_PL_OPERATOR) {
			if (ch == '{') {
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}
		// Custom POD folding
		if (foldPOD && atLineStart) {
			int stylePrevCh = (i) ? styler.StyleAt(i - 1):SCE_PL_DEFAULT;
			if (style == SCE_PL_POD) {
				if (stylePrevCh != SCE_PL_POD && stylePrevCh != SCE_PL_POD_VERB)
					levelCurrent++;
				else if (styler.Match(i, "=cut"))
					levelCurrent--;
				else if (styler.Match(i, "=head"))
					isPodHeading = true;
			} else if (style == SCE_PL_DATASECTION) {
				if (ch == '=' && isascii(chNext) && isalpha(chNext) && levelCurrent == SC_FOLDLEVELBASE)
					levelCurrent++;
				else if (styler.Match(i, "=cut") && levelCurrent > SC_FOLDLEVELBASE)
					levelCurrent--;
				else if (styler.Match(i, "=head"))
					isPodHeading = true;
				// if package used or unclosed brace, level > SC_FOLDLEVELBASE!
				// reset needed as level test is vs. SC_FOLDLEVELBASE
				else if (styler.Match(i, "__END__"))
					levelCurrent = SC_FOLDLEVELBASE;
			}
		}
		// Custom package folding
		if (foldPackage && atLineStart) {
			if (style == SCE_PL_WORD && styler.Match(i, "package")) {
				isPackageLine = true;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (isPodHeading) {
				lev = levelPrev - 1;
				lev |= SC_FOLDLEVELHEADERFLAG;
				isPodHeading = false;
			}
			// Check if line was a package declaration
			// because packages need "special" treatment
			if (isPackageLine) {
				lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				levelCurrent = SC_FOLDLEVELBASE + 1;
				isPackageLine = false;
			}
			lev |= levelCurrent << 16;
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
		chPrev = ch;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const perlWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmPerl(SCLEX_PERL, ColourisePerlDoc, "perl", FoldPerlDoc, perlWordListDesc, 8);
