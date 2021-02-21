/** @file LexRaku.cxx
 ** Lexer for Raku
 **
 ** Copyright (c) 2019 Mark Reay <mark@reay.net.au>
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/*
 * Raku (Perl6) Lexer for Scintilla
 * ---------------------------------
 * ---------------------------------
 * 06-Dec-2019: More Unicode support:
 *              - Added a full scope of allowed numbers and letters
 * 29-Nov-2019: More  highlighting / implemented basic folding:
 *              - Operators (blanket cover, no sequence checking)
 *              - Class / Grammar name highlighting
 *              - Folding:
 *                - Comments: line / multi-line
 *                - POD sections
 *                - Code blocks {}
 * 26-Nov-2019: Basic syntax highlighting covering the following:
 *              - Comments, both line and embedded (multi-line)
 *              - POD, no inline highlighting as yet...
 *              - Heredoc block string, with variable highlighting (with qq)
 *              - Strings, with variable highlighting (with ")
 *              - Q Language, including adverbs (also basic q and qq)
 *              - Regex, including adverbs
 *              - Numbers
 *              - Bareword / identifiers
 *              - Types
 *              - Variables: mu, positional, associative, callable
 * TODO:
 *       - POD inline
 *       - Better operator sequence coverage
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <vector>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;

namespace { // anonymous namespace to isolate any name clashes
/*----------------------------------------------------------------------------*
 * --- DEFINITIONS: OPTIONS / CONSTANTS ---
 *----------------------------------------------------------------------------*/

// Number types
#define RAKUNUM_BINARY		1	// order is significant: 1-3 cannot have a dot
#define RAKUNUM_OCTAL		2
#define RAKUNUM_FLOAT_EXP	3	// exponent part only
#define RAKUNUM_HEX			4	// may be a hex float
#define RAKUNUM_DECIMAL		5	// 1-5 are numbers; 6-7 are strings
#define RAKUNUM_VECTOR		6
#define RAKUNUM_V_VECTOR	7
#define RAKUNUM_VERSION		8	// can contain multiple '.'s
#define RAKUNUM_BAD			9

// Regex / Q string types
#define RAKUTYPE_REGEX_NORM		0	// 0 char ident
#define RAKUTYPE_REGEX_S		1	// order is significant:
#define RAKUTYPE_REGEX_M		2	// 1 char ident
#define RAKUTYPE_REGEX_Y		3	// 1 char ident
#define RAKUTYPE_REGEX			4	// > RAKUTYPE_REGEX == 2 char identifiers
#define RAKUTYPE_REGEX_RX		5	// 2 char ident
#define RAKUTYPE_REGEX_TR		6	// 2 char ident
#define RAKUTYPE_QLANG			7	// < RAKUTYPE_QLANG == RAKUTYPE_REGEX_?
#define RAKUTYPE_STR_WQ			8	// 0 char ident < word quote >
#define RAKUTYPE_STR_Q			9	// 1 char ident
#define RAKUTYPE_STR_QX			10	// 2 char ident
#define RAKUTYPE_STR_QW			11	// 2 char ident
#define RAKUTYPE_STR_QQ			12	// 2 char ident
#define RAKUTYPE_STR_QQX		13	// 3 char ident
#define RAKUTYPE_STR_QQW		14	// 3 char ident
#define RAKUTYPE_STR_QQWW		15	// 4 char ident

// Delimiter types
#define RAKUDELIM_BRACKET		0	// bracket: regex, Q language
#define RAKUDELIM_QUOTE			1	// quote: normal string

// rakuWordLists: keywords as defined in config
const char *const rakuWordLists[] = {
	"Keywords and identifiers",
	"Functions",
	"Types basic",
	"Types composite",
	"Types domain-specific",
	"Types exception",
	"Adverbs",
	nullptr,
};

// Options and defaults
struct OptionsRaku {
	bool fold;
	bool foldCompact;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentPOD;
	OptionsRaku() {
		fold					= true;
		foldCompact				= false;
		foldComment				= true;
		foldCommentMultiline	= true;
		foldCommentPOD			= true;
	}
};

// init options and words
struct OptionSetRaku : public OptionSet<OptionsRaku> {
	OptionSetRaku() {
		DefineProperty("fold",			&OptionsRaku::fold);
		DefineProperty("fold.comment",	&OptionsRaku::foldComment);
		DefineProperty("fold.compact",	&OptionsRaku::foldCompact);

		DefineProperty("fold.raku.comment.multiline",	&OptionsRaku::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");
		DefineProperty("fold.raku.comment.pod",			&OptionsRaku::foldCommentPOD,
			"Set this property to 0 to disable folding POD comments when fold.comment=1.");

		// init word lists
		DefineWordListSets(rakuWordLists);
	}
};

// Delimiter pair
struct DelimPair {
	int opener;		// opener char
	int closer[2];	// closer chars
	bool interpol;	// can variables be interpolated?
	short count;	// delimiter char count
	DelimPair() {
		opener = 0;
		closer[0] = 0;
		closer[1] = 0;
		interpol = false;
		count = 0;
	}
	bool isCloser(int ch) const {
		return ch == closer[0] || ch == closer[1];
	}
};

/*----------------------------------------------------------------------------*
 * --- FUNCTIONS ---
 *----------------------------------------------------------------------------*/

/*
 * IsANewLine
 * - returns true if this is a new line char
 */
constexpr bool IsANewLine(int ch) noexcept {
	return ch == '\r' || ch == '\n';
}

/*
 * IsAWhitespace
 * - returns true if this is a whitespace (or newline) char
 */
bool IsAWhitespace(int ch) noexcept {
	return IsASpaceOrTab(ch) || IsANewLine(ch);
}

/*
 * IsAlphabet
 * - returns true if this is an alphabetical char
 */
constexpr bool IsAlphabet(int ch) noexcept {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/*
 * IsCommentLine
 * - returns true if this is a comment line
 *   - tests: SCE_RAKU_COMMENTLINE or SCE_RAKU_COMMENTEMBED
 * modified from: LexPerl.cxx
 */
bool IsCommentLine(Sci_Position line, LexAccessor &styler, int type = SCE_RAKU_COMMENTLINE) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		int style = styler.StyleAt(i);
		if (type == SCE_RAKU_COMMENTEMBED) {
			if (i == (eol_pos - 1) && style == type)
				return true;
		} else { // make sure the line is NOT a SCE_RAKU_COMMENTEMBED
			if (ch == '#' && style == type && styler[i+1] != '`' )
				return true;
			else if (!IsASpaceOrTab(ch))
				return false;
		}
	}
	return false;
}

/*
 * GetBracketCloseChar
 * - returns the end bracket char: opposite of start
 *   - see: http://www.unicode.org/Public/5.1.0/ucd/BidiMirroring.txt (first section)
 * - Categories are general matches for valid BiDi types
 * - Most closer chars are opener + 1
 */
int GetBracketCloseChar(const int ch) noexcept {
	const CharacterCategory cc = CategoriseCharacter(ch);
	switch (cc) {
		case ccSm:
			switch (ch) {
				case 0x3C: return 0x3E; // LESS-THAN SIGN
				case 0x2208: return 0x220B; // ELEMENT OF
				case 0x2209: return 0x220C; // NOT AN ELEMENT OF
				case 0x220A: return 0x220D; // SMALL ELEMENT OF
				case 0x2215: return 0x29F5; // DIVISION SLASH
				case 0x2243: return 0x22CD; // ASYMPTOTICALLY EQUAL TO
				case 0x2298: return 0x29B8; // CIRCLED DIVISION SLASH
				case 0x22A6: return 0x2ADE; // ASSERTION
				case 0x22A8: return 0x2AE4; // TRUE
				case 0x22A9: return 0x2AE3; // FORCES
				case 0x22AB: return 0x2AE5; // DOUBLE VERTICAL BAR DOUBLE RIGHT TURNSTILE
				case 0x22F2: return 0x22FA; // ELEMENT OF WITH LONG HORIZONTAL STROKE
				case 0x22F3: return 0x22FB; // ELEMENT OF WITH VERTICAL BAR AT END OF HORIZONTAL STROKE
				case 0x22F4: return 0x22FC; // SMALL ELEMENT OF WITH VERTICAL BAR AT END OF HORIZONTAL STROKE
				case 0x22F6: return 0x22FD; // ELEMENT OF WITH OVERBAR
				case 0x22F7: return 0x22FE; // SMALL ELEMENT OF WITH OVERBAR
				case 0xFF1C: return 0xFF1E; // FULLWIDTH LESS-THAN SIGN
			}
			break;
		case ccPs:
			switch (ch) {
				case 0x5B: return 0x5D; // LEFT SQUARE BRACKET
				case 0x7B: return 0x7D; // LEFT CURLY BRACKET
				case 0x298D: return 0x2990; // LEFT SQUARE BRACKET WITH TICK IN TOP CORNER
				case 0x298F: return 0x298E; // LEFT SQUARE BRACKET WITH TICK IN BOTTOM CORNER
				case 0xFF3B: return 0xFF3D; // FULLWIDTH LEFT SQUARE BRACKET
				case 0xFF5B: return 0xFF5D; // FULLWIDTH LEFT CURLY BRACKET
			}
			break;
		case ccPi:
			break;
		default: return 0;
	}
	return ch + 1;
}

/*
 * IsValidQuoteOpener
 * -
 */
bool IsValidQuoteOpener(const int ch, DelimPair &dp, int type = RAKUDELIM_BRACKET) noexcept {
	dp.closer[0] = 0;
	dp.closer[1] = 0;
	dp.interpol = true;
	if (type == RAKUDELIM_QUOTE) {
		switch (ch) {
			//   Opener		Closer					Description
			case '\'':		dp.closer[0] = '\'';	// APOSTROPHE
				dp.interpol = false;
				break;
			case '"':		dp.closer[0] = '"';		// QUOTATION MARK
				break;
			case 0x2018:	dp.closer[0] = 0x2019;	// LEFT SINGLE QUOTATION MARK
				dp.interpol = false;
				break;
			case 0x201C:	dp.closer[0] = 0x201D;	// LEFT DOUBLE QUOTATION MARK
				break;
			case 0x201D:	dp.closer[0] = 0x201C;	// RIGHT DOUBLE QUOTATION MARK
				break;
			case 0x201E:	dp.closer[0] = 0x201C;	// DOUBLE LOW-9 QUOTATION MARK
							dp.closer[1] = 0x201D;
				break;
			case 0xFF62:	dp.closer[0] = 0xFF63;	// HALFWIDTH LEFT CORNER BRACKET
				dp.interpol = false;
				break;
			default:		return false;
		}
	} else if (type == RAKUDELIM_BRACKET) {
		dp.closer[0] = GetBracketCloseChar(ch);
	}
	dp.opener = ch;
	dp.count = 1;
	return dp.closer[0] > 0;
}

/*
 * IsBracketOpenChar
 * - true if this is a valid start bracket character
 */
bool IsBracketOpenChar(int ch) noexcept {
	return GetBracketCloseChar(ch) > 0;
}

/*
 * IsValidRegOrQAdjacent
 * - returns true if ch is a valid character to put directly after Q / q
 *   * ref: Q Language: https://docs.raku.org/language/quoting
 */
bool IsValidRegOrQAdjacent(int ch) noexcept {
	return !(IsAlphaNumeric(ch) || ch == '_' || ch == '(' || ch == ')' || ch == '\'' );
}

/*
 * IsValidRegOrQPrecede
 * - returns true if ch is a valid preceeding character to put directly before Q / q
 *   * ref: Q Language: https://docs.raku.org/language/quoting
 */
bool IsValidRegOrQPrecede(int ch) noexcept {
	return !(IsAlphaNumeric(ch) || ch == '_');
}

/*
 * MatchCharInRange
 * - returns true if the mach character is found in range (of length)
 * - ignoreDelim (default false)
 */
bool MatchCharInRange(StyleContext &sc, const Sci_Position length,
		const int match, bool ignoreDelim = false) {
	Sci_Position len = 0;
	int chPrev = sc.chPrev;
	while (++len < length) {
		const int ch = sc.GetRelativeCharacter(len);
		if (ch == match && (ignoreDelim || chPrev != '\\'))
			return true;
	}
	return false;
}

/*
 * PrevNonWhitespaceChar
 * - returns the last non-whitespace char
 */
int PrevNonWhitespaceChar(StyleContext &sc) {
	Sci_Position rel = 0;
	Sci_Position max_back = 0 - sc.currentPos;
	while (--rel > max_back) {
		const int ch = sc.GetRelativeCharacter(rel);
		if (!IsAWhitespace(ch))
			return ch;
	}
	return 0; // no matching char
}

/*
 * IsQLangStartAtScPos
 * - returns true if this is a valid Q Language sc position
 *   - ref: https://docs.raku.org/language/quoting
 *   - Q :adverb :adverb //;
 *   - q,qx,qw,qq,qqx,qqw,qqww :adverb /:adverb /;
 */
bool IsQLangStartAtScPos(StyleContext &sc, int &type, const Sci_Position length) {
	const bool valid_adj = IsValidRegOrQAdjacent(sc.chNext);
	const int chFw2 = sc.GetRelativeCharacter(2);
	const int chFw3 = sc.GetRelativeCharacter(3);
	type = -1;
	if (IsValidRegOrQPrecede(sc.chPrev)) {
		if (sc.ch == 'Q' && valid_adj) {
			type = RAKUTYPE_QLANG;
		} else if (sc.ch == 'q') {
			switch (sc.chNext) {
				case 'x':
					type = RAKUTYPE_STR_QX;
					break;
				case 'w':
					type = RAKUTYPE_STR_QW;
					break;
				case 'q':
					if (chFw2 == 'x') {
						type = RAKUTYPE_STR_QQX;
					} else if (chFw2 == 'w') {
						if (chFw3 == 'w') {
							type = RAKUTYPE_STR_QQWW;
						} else {
							type = RAKUTYPE_STR_QQW;
						}
					} else {
						type = RAKUTYPE_STR_QQ;
					}
					break;
				default:
					type = RAKUTYPE_STR_Q;
			}
		} else if (sc.ch == '<' && MatchCharInRange(sc, length, '>')) {
			type = RAKUTYPE_STR_WQ; // < word quote >
		}
	}
	return type >= 0;
}

/*
 * IsRegexStartAtScPos
 * - returns true if this is a valid Regex sc position
 *   - ref: https://docs.raku.org/language/regexes
 *   - Regex: (rx/s/m/tr/y) :adverb /:adverb /;
 *   -              regex R :adverb //;
 *   -                     /:adverb /;
 */
bool IsRegexStartAtScPos(StyleContext &sc, int &type, CharacterSet &set) {
	const bool valid_adj = IsValidRegOrQAdjacent(sc.chNext);
	type = -1;
	if (IsValidRegOrQPrecede(sc.chPrev)) {
		switch (sc.ch) {
			case 'r':
				if (sc.chNext == 'x')
					type = RAKUTYPE_REGEX_RX;
				break;
			case 't':
			case 'T':
				if (sc.chNext == 'r' || sc.chNext == 'R')
					type = RAKUTYPE_REGEX_TR;
				break;
			case 'm':
				if (valid_adj)
					type = RAKUTYPE_REGEX_M;
				break;
			case 's':
			case 'S':
				if (valid_adj)
					type = RAKUTYPE_REGEX_S;
				break;
			case 'y':
				if (valid_adj)
					type = RAKUTYPE_REGEX_Y;
				break;
			case '/':
				if (set.Contains(PrevNonWhitespaceChar(sc)))
					type = RAKUTYPE_REGEX_NORM;
		}
	}
	return type >= 0;
}

/*
 * IsValidIdentPrecede
 * - returns if ch is a valid preceeding char to put directly before an identifier
 */
bool IsValidIdentPrecede(int ch) noexcept {
	return !(IsAlphaNumeric(ch) || ch == '_' || ch == '@' || ch == '$' || ch == '%');
}

/*
 * IsValidDelimiter
 * - returns if ch is a valid delimiter (most chars are valid)
 *   * ref: Q Language: https://docs.raku.org/language/quoting
 */
bool IsValidDelimiter(int ch) noexcept {
	return !(IsAlphaNumeric(ch) || ch == ':');
}

/*
 * GetDelimiterCloseChar
 * - returns the corrisponding close char for a given delimiter (could be the same char)
 */
int GetDelimiterCloseChar(int ch) noexcept {
	int ch_end = GetBracketCloseChar(ch);
	if (ch_end == 0 && IsValidDelimiter(ch)) {
		ch_end = ch;
	}
	return ch_end;
}

/*
 * GetRepeatCharCount
 * - returns the occurence count of match
 */
Sci_Position GetRepeatCharCount(StyleContext &sc, int chMatch, Sci_Position length) {
	Sci_Position cnt = 0;
	while (cnt < length) {
		if (sc.GetRelativeCharacter(cnt) != chMatch) {
			break;
		}
		cnt++;
	}
	return cnt;
}

/*
 * LengthToDelimiter
 * - returns the length until the end of a delimited string section
 *   - Ignores nested delimiters (if opener != closer)
 *   - no trailing char after last closer (default false)
 */
Sci_Position LengthToDelimiter(StyleContext &sc, const DelimPair &dp,
		Sci_Position length, bool noTrailing = false) {
	short cnt_open = 0;			// count open bracket
	short cnt_close = 0;		// count close bracket
	Sci_Position len = 0;		// count characters
	int chOpener = dp.opener;	// look for nested opener / closer
	if (dp.opener == dp.closer[0])
		chOpener = 0;			// no opening delimiter (no nesting possible)

	while (len < length) {
		const int chPrev = sc.GetRelativeCharacter(len - 1);
		const int ch = sc.GetRelativeCharacter(len);
		const int chNext = sc.GetRelativeCharacter(len+1);

		if (cnt_open == 0 && cnt_close == dp.count) {
			return len;				// end condition has been met
		} else {
			if (chPrev != '\\' && ch == chOpener) {			// ignore escape sequence
				cnt_open++;			// open nested bracket
			} else if (chPrev != '\\' && dp.isCloser(ch)) {	// ignore escape sequence
				if ( cnt_open > 0 ) {
					cnt_open--;		// close nested bracket
				} else if (dp.count > 1 && cnt_close < (dp.count - 1)) {
					if (cnt_close > 1) {
						if (dp.isCloser(chPrev)) {
							cnt_close++;
						} else {	// reset if previous char was not close
							cnt_close = 0;
						}
					} else {
						cnt_close++;
					}
				} else if (!noTrailing || (IsAWhitespace(chNext))) {
					cnt_close++;		// found last close
					if (cnt_close > 1 && !dp.isCloser(chPrev)) {
						cnt_close = 0;	// reset if previous char was not close
					}
				} else {
					cnt_close = 0;		// non handled close: reset
				}
			} else if (IsANewLine(ch)) {
				cnt_open = 0;			// reset after each line
				cnt_close = 0;
			}
		}
		len++;
	}
	return -1; // end condition has NOT been met
}

/*
 * LengthToEndHeredoc
 * - returns the length until the end of a heredoc section
 *   - delimiter string MUST begin on a new line
 */
Sci_Position LengthToEndHeredoc(const StyleContext &sc, LexAccessor &styler,
		const Sci_Position length, const char *delim) {
	bool on_new_ln = false;
	int i = 0; // str index
	for (int n = 0; n < length; n++) {
		const char ch = styler.SafeGetCharAt(sc.currentPos + n, 0);
		if (on_new_ln) {
			if (delim[i] == '\0')
				return n;	// at end of str, match found!
			if (ch != delim[i++])
				i = 0;		// no char match, reset 'i'ndex
		}
		if (i == 0)			// detect new line
			on_new_ln = IsANewLine(ch);
	}
	return -1;				// no match found
}

/*
 * LengthToNextChar
 * - returns the length until the next character
 */
Sci_Position LengthToNextChar(StyleContext &sc, const Sci_Position length) {
	Sci_Position len = 0;
	while (++len < length) {
		const int ch = sc.GetRelativeCharacter(len);
		if (!IsASpaceOrTab(ch) && !IsANewLine(ch)) {
			break;
		}
	}
	return len;
}

/*
 * GetRelativeString
 * - gets a relitive string and sets it in &str
 *   - resets string before seting
 */
void GetRelativeString(StyleContext &sc, Sci_Position offset, Sci_Position length,
		std::string &str) {
	Sci_Position pos = offset;
	str.clear();
	while (pos < length) {
		str += sc.GetRelativeCharacter(pos++);
	}
}

} // end anonymous namespace

/*----------------------------------------------------------------------------*
 * --- class: LexerRaku ---
 *----------------------------------------------------------------------------*/
//class LexerRaku : public ILexerWithMetaData {
class LexerRaku : public DefaultLexer {
	CharacterSet setWord;
	CharacterSet setSigil;
	CharacterSet setTwigil;
	CharacterSet setOperator;
	CharacterSet setSpecialVar;
	WordList regexIdent;			// identifiers that specify a regex
	OptionsRaku options;			// Options from config
	OptionSetRaku osRaku;
	WordList keywords;				// Word Lists from config
	WordList functions;
	WordList typesBasic;
	WordList typesComposite;
	WordList typesDomainSpecific;
	WordList typesExceptions;
	WordList adverbs;

public:
	// Defined as explicit, so that constructor can not be copied
	explicit LexerRaku() :
		DefaultLexer("raku", SCLEX_RAKU),
		setWord(CharacterSet::setAlphaNum, "-_", 0x80),
		setSigil(CharacterSet::setNone, "$&%@"),
		setTwigil(CharacterSet::setNone, "!*.:<=?^~"),
		setOperator(CharacterSet::setNone, "^&\\()-+=|{}[]:;<>,?!.~"),
		setSpecialVar(CharacterSet::setNone, "_/!") {
		regexIdent.Set("regex rule token");
	}
	// Deleted so LexerRaku objects can not be copied.
	LexerRaku(const LexerRaku &) = delete;
	LexerRaku(LexerRaku &&) = delete;
	void operator=(const LexerRaku &) = delete;
	void operator=(LexerRaku &&) = delete;
	virtual ~LexerRaku() {
	}
	void SCI_METHOD Release() noexcept override {
		delete this;
	}
	int SCI_METHOD Version() const noexcept override {
		return lvRelease5;
	}
	const char *SCI_METHOD PropertyNames() override {
		return osRaku.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osRaku.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osRaku.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osRaku.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osRaku.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	static ILexer5 *LexerFactoryRaku() {
		return new LexerRaku();
	}

protected:
	bool IsOperatorChar(const int ch);
	bool IsWordChar(const int ch, bool allowNumber = true);
	bool IsWordStartChar(const int ch);
	bool IsNumberChar(const int ch, int base = 10);
	bool ProcessRegexTwinCapture(StyleContext &sc, const Sci_Position length,
		int &type, const DelimPair &dp);
	void ProcessStringVars(StyleContext &sc, const Sci_Position length, const int varState);
	bool ProcessValidRegQlangStart(StyleContext &sc, Sci_Position length, const int type,
		WordList &wordsAdverbs, DelimPair &dp);
	Sci_Position LengthToNonWordChar(StyleContext &sc, Sci_Position length,
		char *s, const int size, Sci_Position offset = 0);
};

/*----------------------------------------------------------------------------*
 * --- METHODS: LexerRaku ---
 *----------------------------------------------------------------------------*/

/*
 * LexerRaku::IsOperatorChar
 * - Test for both ASCII and Unicode operators
 *   see: https://docs.raku.org/language/unicode_entry
 */
bool LexerRaku::IsOperatorChar(const int ch) {
	if (ch > 0x7F) {
		switch (ch) {
			//   Unicode	ASCII Equiv.
			case 0x2208:	// (elem)
			case 0x2209:	// !(elem)
			case 0x220B:	// (cont)
			case 0x220C:	// !(cont)
			case 0x2216:	// (-)
			case 0x2229:	// (&)
			case 0x222A:	// (|)
			case 0x2282:	// (<)
			case 0x2283:	// (>)
			case 0x2284:	// !(<)
			case 0x2285:	// !(>)
			case 0x2286:	// (<=)
			case 0x2287:	// (>=)
			case 0x2288:	// !(<=)
			case 0x2289:	// !(>=)
			case 0x228D:	// (.)
			case 0x228E:	// (+)
			case 0x2296:	// (^)
				return true;
		}
	}
	return setOperator.Contains(ch);
}

/*
 * LexerRaku::IsWordChar
 * - Test for both ASCII and Unicode identifier characters
 *   see: https://docs.raku.org/language/unicode_ascii
 *   also: ftp://ftp.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
 *   FIXME: *still* may not contain all valid characters
 */
bool LexerRaku::IsWordChar(const int ch, bool allowNumber) {
	// Unicode numbers should not apear in word identifiers
	if (ch > 0x7F) {
		const CharacterCategory cc = CategoriseCharacter(ch);
		switch (cc) {
			// Letters
			case ccLu:
			case ccLl:
			case ccLt:
			case ccLm:
			case ccLo:
				return true;
			default:
				return false;
		}
	} else if (allowNumber && IsADigit(ch)) {
		return true; // an ASCII number type
	}
	return setWord.Contains(ch);
}

/*
 * LexerRaku::IsWordStartChar
 * - Test for both ASCII and Unicode identifier "start / first" characters
 */
bool LexerRaku::IsWordStartChar(const int ch) {
	return ch != '-' && IsWordChar(ch, false); // no numbers allowed
}

/*
 * LexerRaku::IsNumberChar
 * - Test for both ASCII and Unicode identifier number characters
 *   see: https://docs.raku.org/language/unicode_ascii
 *   also: ftp://ftp.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
 *   FILTERED by Unicode letters that are NUMBER
 *     and NOT PARENTHESIZED or CIRCLED
 *   FIXME: *still* may not contain all valid number characters
 */
bool LexerRaku::IsNumberChar(const int ch, int base) {
	if (ch > 0x7F) {
		const CharacterCategory cc = CategoriseCharacter(ch);
		switch (cc) {
			// Numbers
			case ccNd:
			case ccNl:
			case ccNo:
				return true;
			default:
				return false;
		}
	}
	return IsADigit(ch, base);
}

/*
 * LexerRaku::PropertySet
 * -
 */
Sci_Position SCI_METHOD LexerRaku::PropertySet(const char *key, const char *val) {
	if (osRaku.PropertySet(&options, key, val))
		return 0;
	return -1;
}

/*
 * LexerRaku::WordListSet
 * -
 */
Sci_Position SCI_METHOD LexerRaku::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
		case 0:
			wordListN = &keywords;
			break;
		case 1:
			wordListN = &functions;
			break;
		case 2:
			wordListN = &typesBasic;
			break;
		case 3:
			wordListN = &typesComposite;
			break;
		case 4:
			wordListN = &typesDomainSpecific;
			break;
		case 5:
			wordListN = &typesExceptions;
			break;
		case 6:
			wordListN = &adverbs;
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

/*
 * LexerRaku::ProcessRegexTwinCapture
 * - processes the transition between a regex pair (two sets of delimiters)
 * - moves to first new delimiter, if a bracket
 * - returns true when valid delimiter start found (if bracket)
 */
bool LexerRaku::ProcessRegexTwinCapture(StyleContext &sc, const Sci_Position length,
		int &type, const DelimPair &dp) {

	if (type == RAKUTYPE_REGEX_S || type == RAKUTYPE_REGEX_TR || type == RAKUTYPE_REGEX_Y) {
		type = -1; // clear type

		// move past chRegQClose if it was the previous char
		if (dp.isCloser(sc.chPrev))
			sc.Forward();

		// no processing needed for non-bracket
		if (dp.isCloser(dp.opener))
			return true;

		// move to next opening bracket
		const Sci_Position len = LengthToNextChar(sc, length);
		if (sc.GetRelativeCharacter(len) == dp.opener) {
			sc.Forward(len);
			return true;
		}
	}
	return false;
}

/*
 * LexerRaku::ProcessStringVars
 * - processes a string and highlights any valid variables
 */
void LexerRaku::ProcessStringVars(StyleContext &sc, const Sci_Position length, const int varState) {
	const int state = sc.state;
	for (Sci_Position pos = 0; pos < length; pos++) {
		if (sc.state == varState && !IsWordChar(sc.ch)) {
			sc.SetState(state);
		} else if (sc.chPrev != '\\'
				&& (sc.ch == '$' || sc.ch == '@')
				&& IsWordStartChar(sc.chNext)) {
			sc.SetState(varState);
		}
		sc.Forward(); // Next character
	}
}
/*
 * LexerRaku::ProcessValidRegQlangStart
 * - processes a section of the document range from after a Regex / Q delimiter
 * - returns true on success
 *   - sets: adverbs, chOpen, chClose, chCount
 *  ref: https://docs.raku.org/language/regexes
 */
bool LexerRaku::ProcessValidRegQlangStart(StyleContext &sc, Sci_Position length, const int type,
		WordList &wordsAdverbs, DelimPair &dp) {
	Sci_Position startPos = sc.currentPos;
	Sci_Position startLen = length;
	const int target_state = sc.state;
	int state = SCE_RAKU_DEFAULT;
	std::string str;

	// find our opening delimiter (and occurrences) / save any adverbs
	dp.opener = 0;					// adverbs can be after the first delimiter
	bool got_all_adverbs = false;	// in Regex statements
	bool got_ident = false;			// regex can have an identifier: 'regex R'
	sc.SetState(state);				// set state default to avoid pre-highlights
	while ((dp.opener == 0 || !got_all_adverbs) && sc.More()) {

		// move to the next non-space character
		const bool was_space = IsAWhitespace(sc.ch);
		if (!got_all_adverbs && was_space) {
			sc.Forward(LengthToNextChar(sc, length));
		}
		length = startLen - (sc.currentPos - startPos); // update length remaining

		// parse / eat an identifier (if type == RAKUTYPE_REGEX)
		if (dp.opener == 0 && !got_ident && type == RAKUTYPE_REGEX && IsAlphabet(sc.ch)) {

			// eat identifier / account for special adverb :sym<name>
			bool got_sym = false;
			while (sc.More()) {
				sc.SetState(SCE_RAKU_IDENTIFIER);
				while (sc.More() && (IsAlphaNumeric(sc.chNext)
						|| sc.chNext == '_' || sc.chNext == '-')) {
					sc.Forward();
				}
				sc.Forward();
				if (got_sym && sc.ch == '>') {
					sc.SetState(SCE_RAKU_OPERATOR);	// '>'
					sc.Forward();
					break;
				} else if (type == RAKUTYPE_REGEX && sc.Match(":sym<")) {
					sc.SetState(SCE_RAKU_ADVERB);	// ':sym'
					sc.Forward(4);
					sc.SetState(SCE_RAKU_OPERATOR);	// '<'
					sc.Forward();
					got_sym = true;
				} else {
					break;
				}
			}
			sc.SetState(state);
			got_ident = true;
		}

		// parse / save an adverb: RAKUTYPE_REGEX only has adverbs after delim
		//                      >= RAKUTYPE_QLANG only has adverbs before delim
		else if (!got_all_adverbs && sc.ch == ':' && (!(dp.opener == 0 && got_ident)
				&& !(dp.opener > 0 && type >= RAKUTYPE_QLANG))) {
			sc.SetState(SCE_RAKU_ADVERB);
			while (IsAlphaNumeric(sc.chNext) && sc.More()) {
				sc.Forward();
				str += sc.ch;
			}
			str += ' ';
			sc.Forward();
			sc.SetState(state);
		}

		// find starting delimiter
		else if (dp.opener == 0 && (was_space || IsValidRegOrQAdjacent(sc.ch))
				&& IsValidDelimiter(sc.ch)) {	// make sure the delimiter is legal (most are)
			sc.SetState((state = target_state));// start state here...
			dp.opener = sc.ch;					// this is our delimiter, get count
			if (type < RAKUTYPE_QLANG)			// type is Regex
				dp.count = 1;					// has only one delimiter
			else
				dp.count = GetRepeatCharCount(sc, dp.opener, length);
			sc.Forward(dp.count);
		}

		// we must have all the adverbs by now...
		else {
			if (got_all_adverbs)
				break; // prevent infinite loop: occurs on missing open char
			got_all_adverbs = true;
		}
	}

	// set word list / find a valid closing delimiter (or bomb!)
	wordsAdverbs.Set(str.c_str());
	dp.closer[0] = GetDelimiterCloseChar(dp.opener);
	dp.closer[1] = 0; // no other closer char
	return dp.closer[0] > 0;
}

/*
 * LexerRaku::LengthToNonWordChar
 * - returns the length until the next non "word" character: AlphaNum + '_'
 *   - also sets all the parsed chars in 's'
 */
Sci_Position LexerRaku::LengthToNonWordChar(StyleContext &sc, Sci_Position length,
		char *s, const int size, Sci_Position offset) {
	Sci_Position len = 0;
	Sci_Position max_length = size < length ? size : length;
	while (len <= max_length) {
		const int ch = sc.GetRelativeCharacter(len + offset);
		if (!IsWordChar(ch)) {
			s[len] = '\0';
			break;
		}
		s[len] = ch;
		len++;
	}
	s[len + 1] = '\0';
	return len;
}

/*
 * LexerRaku::Lex
 * - Main lexer method
 */
void SCI_METHOD LexerRaku::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	DelimPair dpEmbeded;			// delimiter pair: embeded comments
	DelimPair dpString;				// delimiter pair: string
	DelimPair dpRegQ;				// delimiter pair: Regex / Q Lang
	std::string hereDelim;			// heredoc delimiter (if in heredoc)
	int hereState = 0;				// heredoc state to use (Q / QQ)
	int numState = 0;				// number state / type
	short cntDecimal = 0;			// number decinal count
	std::string wordLast;			// last word seen
	std::string identLast;			// last identifier seen
	std::string adverbLast;			// last (single) adverb seen
	WordList lastAdverbs;			// last adverbs seen
	Sci_Position len;				// temp length value
	char s[100];					// temp char string
	int typeDetect;					// temp type detected (for regex and Q lang)
	Sci_Position lengthToEnd;		// length until the end of range

	// Backtrack to last SCE_RAKU_DEFAULT or 0
	Sci_PositionU newStartPos = startPos;
	if (initStyle != SCE_RAKU_DEFAULT) {
		while (newStartPos > 0) {
			newStartPos--;
			if (styler.StyleAt(newStartPos) == SCE_RAKU_DEFAULT)
				break;
		}
	}

	// Backtrack to start of line before SCE_RAKU_HEREDOC_Q?
	if (initStyle == SCE_RAKU_HEREDOC_Q || initStyle == SCE_RAKU_HEREDOC_QQ) {
		while (newStartPos > 0) {
			if (IsANewLine(styler.SafeGetCharAt(newStartPos - 1)))
				break; // Stop if previous char is a new line
			newStartPos--;
		}
	}

	// Re-calculate (any) changed startPos, length and initStyle state
	if (newStartPos < startPos) {
		initStyle = SCE_RAKU_DEFAULT;
		length += startPos - newStartPos;
		startPos = newStartPos;
	}

	// init StyleContext
	StyleContext sc(startPos, length, initStyle, styler);

	// StyleContext Loop
	for (; sc.More(); sc.Forward()) {
		lengthToEnd = (length - (sc.currentPos - startPos)); // end of range

		/* *** Determine if the current state should terminate ************** *
		 * Everything within the 'switch' statement processes characters up
		 * until the end of a syntax highlight section / state.
		 * ****************************************************************** */
		switch (sc.state) {
			case SCE_RAKU_OPERATOR:
				sc.SetState(SCE_RAKU_DEFAULT);
				break; // FIXME: better valid operator sequences needed?
			case SCE_RAKU_COMMENTLINE:
				if (IsANewLine(sc.ch)) {
					sc.SetState(SCE_RAKU_DEFAULT);
				}
				break;
			case SCE_RAKU_COMMENTEMBED:
				if ((len = LengthToDelimiter(sc, dpEmbeded, lengthToEnd)) >= 0) {
					sc.Forward(len);			// Move to end delimiter
					sc.SetState(SCE_RAKU_DEFAULT);
				} else {
					sc.Forward(lengthToEnd);	// no end delimiter found
				}
				break;
			case SCE_RAKU_POD:
				if (sc.atLineStart && sc.Match("=end pod")) {
					sc.Forward(8);
					sc.SetState(SCE_RAKU_DEFAULT);
				}
				break;
			case SCE_RAKU_STRING:

				// Process the string for variables: move to end delimiter
				if ((len = LengthToDelimiter(sc, dpString, lengthToEnd)) >= 0) {
					if (dpString.interpol) {
						ProcessStringVars(sc, len, SCE_RAKU_STRING_VAR);
					} else {
						sc.Forward(len);
					}
					sc.SetState(SCE_RAKU_DEFAULT);
				} else {
					sc.Forward(lengthToEnd);	// no end delimiter found
				}
				break;
			case SCE_RAKU_STRING_Q:
			case SCE_RAKU_STRING_QQ:
			case SCE_RAKU_STRING_Q_LANG:

				// No string: previous char was the delimiter
				if (dpRegQ.count == 1 && dpRegQ.isCloser(sc.chPrev)) {
					sc.SetState(SCE_RAKU_DEFAULT);
				}

				// Process the string for variables: move to end delimiter
				else if ((len = LengthToDelimiter(sc, dpRegQ, lengthToEnd)) >= 0) {

					// set (any) heredoc delimiter string
					if (lastAdverbs.InList("to")) {
						GetRelativeString(sc, -1, len - dpRegQ.count, hereDelim);
						hereState = SCE_RAKU_HEREDOC_Q; // default heredoc state
					}

					// select variable identifiers
					if (sc.state == SCE_RAKU_STRING_QQ || lastAdverbs.InList("qq")) {
						ProcessStringVars(sc, len, SCE_RAKU_STRING_VAR);
						hereState = SCE_RAKU_HEREDOC_QQ; // potential heredoc state
					} else {
						sc.Forward(len);
					}
					sc.SetState(SCE_RAKU_DEFAULT);
				} else {
					sc.Forward(lengthToEnd);	// no end delimiter found
				}
				break;
			case SCE_RAKU_HEREDOC_Q:
			case SCE_RAKU_HEREDOC_QQ:
				if ((len = LengthToEndHeredoc(sc, styler, lengthToEnd, hereDelim.c_str())) >= 0) {
					// select variable identifiers
					if (sc.state == SCE_RAKU_HEREDOC_QQ) {
						ProcessStringVars(sc, len, SCE_RAKU_STRING_VAR);
					} else {
						sc.Forward(len);
					}
					sc.SetState(SCE_RAKU_DEFAULT);
				} else {
					sc.Forward(lengthToEnd);	// no end delimiter found
				}
				hereDelim.clear();				// clear heredoc delimiter
				break;
			case SCE_RAKU_REGEX:
				// account for typeDetect = RAKUTYPE_REGEX_S/TR/Y
				while (sc.state == SCE_RAKU_REGEX) {

					// No string: previous char was the delimiter
					if (dpRegQ.count == 1 && dpRegQ.isCloser(sc.chPrev)) {
						if (ProcessRegexTwinCapture(sc, lengthToEnd, typeDetect, dpRegQ))
							continue;
						sc.SetState(SCE_RAKU_DEFAULT);
						break;
					}

					// Process the string for variables: move to end delimiter
					else if ((len = LengthToDelimiter(sc, dpRegQ, lengthToEnd)) >= 0) {
						ProcessStringVars(sc, len, SCE_RAKU_REGEX_VAR);
						if (ProcessRegexTwinCapture(sc, lengthToEnd, typeDetect, dpRegQ))
							continue;
						sc.SetState(SCE_RAKU_DEFAULT);
						break;
					} else {
						sc.Forward(lengthToEnd); // no end delimiter found
						break;
					}
				}
				break;
			case SCE_RAKU_NUMBER:
				if (sc.ch == '.') {
					if (sc.chNext == '.') {		// '..' is an operator
						sc.SetState(SCE_RAKU_OPERATOR);
						sc.Forward();
						if (sc.chNext == '.')	// '...' is also an operator
							sc.Forward();
						break;
					} else if (numState > RAKUNUM_FLOAT_EXP
							&& (cntDecimal < 1 || numState == RAKUNUM_VERSION)) {
						cntDecimal++;
						sc.Forward();
					} else {
						sc.SetState(SCE_RAKU_DEFAULT);
						break; // too many decinal places
					}
				}
				switch (numState) {
					case RAKUNUM_BINARY:
						if (!IsNumberChar(sc.ch, 2))
							sc.SetState(SCE_RAKU_DEFAULT);
						break;
					case RAKUNUM_OCTAL:
						if (!IsNumberChar(sc.ch, 8))
							sc.SetState(SCE_RAKU_DEFAULT);
						break;
					case RAKUNUM_HEX:
						if (!IsNumberChar(sc.ch, 16))
							sc.SetState(SCE_RAKU_DEFAULT);
						break;
					case RAKUNUM_DECIMAL:
					case RAKUNUM_VERSION:
						if (!IsNumberChar(sc.ch))
							sc.SetState(SCE_RAKU_DEFAULT);
				}
				break;
			case SCE_RAKU_WORD:
			case SCE_RAKU_FUNCTION:
			case SCE_RAKU_TYPEDEF:
			case SCE_RAKU_ADVERB:
				sc.SetState(SCE_RAKU_DEFAULT);
				break;
			case SCE_RAKU_MU:
			case SCE_RAKU_POSITIONAL:
			case SCE_RAKU_ASSOCIATIVE:
			case SCE_RAKU_CALLABLE:
			case SCE_RAKU_IDENTIFIER:
			case SCE_RAKU_GRAMMAR:
			case SCE_RAKU_CLASS:
				sc.SetState(SCE_RAKU_DEFAULT);
				break;
		}

		/* *** Determine if a new state should be entered ******************* *
		 * Everything below here identifies the beginning of a state, all or part
		 * of the characters within this state are processed here, the rest are
		 * completed above in the terminate state section.
		 * ****************************************************************** */
		if (sc.state == SCE_RAKU_DEFAULT) {

			// --- Single line comment
			if (sc.ch == '#') {
				sc.SetState(SCE_RAKU_COMMENTLINE);
			}

			// --- POD block
			else if (sc.atLineStart && sc.Match("=begin pod")) {
				sc.SetState(SCE_RAKU_POD);
				sc.Forward(10);
			}

			// --- String (normal)
			else if (sc.chPrev != '\\' && (IsValidQuoteOpener(sc.ch, dpString, RAKUDELIM_QUOTE))) {
				sc.SetState(SCE_RAKU_STRING);
			}

			// --- String (Q Language) ----------------------------------------
			//   - https://docs.raku.org/language/quoting
			//   - Q :adverb :adverb //;
			//   - q,qx,qw,qq,qqx,qqw,qqww :adverb :adverb //;
			else if (IsQLangStartAtScPos(sc, typeDetect, lengthToEnd)) {
				int state = SCE_RAKU_STRING_Q_LANG;
				Sci_Position forward = 1;	// single char ident (default)
				if (typeDetect > RAKUTYPE_QLANG) {
					state = SCE_RAKU_STRING_Q;
					if (typeDetect == RAKUTYPE_STR_WQ)
						forward = 0;		// no char ident
				}
				if (typeDetect > RAKUTYPE_STR_Q) {
					if (typeDetect == RAKUTYPE_STR_QQ)
						state = SCE_RAKU_STRING_QQ;
					forward++;				// two char ident
				}
				if (typeDetect > RAKUTYPE_STR_QQ)
					forward++;				// three char ident
				if (typeDetect == RAKUTYPE_STR_QQWW)
					forward++;				// four char ident

				// Proceed: check for a valid character after statement
				if (IsValidRegOrQAdjacent(sc.GetRelative(forward)) || typeDetect == RAKUTYPE_QLANG) {
					sc.SetState(state);
					sc.Forward(forward);
					lastAdverbs.Clear();

					// Process: adverbs / opening delimiter / adverbs after delim
					if (ProcessValidRegQlangStart(sc, lengthToEnd, typeDetect,
							lastAdverbs, dpRegQ))
						sc.SetState(state);
				}
			}

			// --- Regex (rx/s/m/tr/y) ----------------------------------------
			//   - https://docs.raku.org/language/regexes
			else if ((IsRegexStartAtScPos(sc, typeDetect, setOperator) || regexIdent.InList(wordLast.c_str()))) {
				if (typeDetect == -1) { // must be a regex identifier word
					wordLast.clear();
					typeDetect = RAKUTYPE_REGEX;
				}
				Sci_Position forward = 0;	// no ident (RAKUTYPE_REGEX, RAKUTYPE_REGEX_NORM)
				if (typeDetect > 0 && typeDetect != RAKUTYPE_REGEX)
					forward++;				// single char ident
				if (typeDetect > RAKUTYPE_REGEX)
					forward++;				// two char ident

				// Proceed: check for a valid character after statement
				if (IsValidRegOrQAdjacent(sc.GetRelative(forward)) || typeDetect == RAKUTYPE_REGEX_NORM) {
					sc.SetState(SCE_RAKU_REGEX);
					sc.Forward(forward);
					lastAdverbs.Clear();

					// Process: adverbs / opening delimiter / adverbs after delim
					if (ProcessValidRegQlangStart(sc, lengthToEnd, typeDetect,
							lastAdverbs, dpRegQ))
						sc.SetState(SCE_RAKU_REGEX);
				}
			}

			// --- Numbers ----------------------------------------------------
			else if (IsValidIdentPrecede(sc.chPrev) && (IsNumberChar(sc.ch)
					|| (sc.ch == 'v' && IsNumberChar(sc.chNext) && wordLast == "use"))) {
				numState = RAKUNUM_DECIMAL;	// default: decimal (base 10)
				cntDecimal = 0;
				sc.SetState(SCE_RAKU_NUMBER);
				if (sc.ch == 'v')			// forward past 'v'
					sc.Forward();
				if (wordLast == "use") {	// package version number
					numState = RAKUNUM_VERSION;
				} else if (sc.ch == '0') {	// other type of number
					switch (sc.chNext) {
						case 'b':	// binary (base 2)
							numState = RAKUNUM_BINARY;
							break;
						case 'o':	// octal (base 8)
							numState = RAKUNUM_OCTAL;
							break;
						case 'x':	// hexadecimal (base 16)
							numState = RAKUNUM_HEX;
					}
					if (numState != RAKUNUM_DECIMAL)
						sc.Forward();		// forward to number type char
				}
			}

			// --- Keywords / functions / types / barewords -------------------
			else if ((sc.currentPos == 0 || sc.atLineStart || IsValidIdentPrecede(sc.chPrev))
					&& IsWordStartChar(sc.ch)) {
				len = LengthToNonWordChar(sc, lengthToEnd, s, sizeof(s));
				if (keywords.InList(s)) {
					sc.SetState(SCE_RAKU_WORD);		// Keywords
				} else if(functions.InList(s)) {
					sc.SetState(SCE_RAKU_FUNCTION);	// Functions
				} else if(typesBasic.InList(s)) {
					sc.SetState(SCE_RAKU_TYPEDEF);	// Types (basic)
				} else if(typesComposite.InList(s)) {
					sc.SetState(SCE_RAKU_TYPEDEF);	// Types (composite)
				} else if(typesDomainSpecific.InList(s)) {
					sc.SetState(SCE_RAKU_TYPEDEF);	// Types (domain-specific)
				} else if(typesExceptions.InList(s)) {
					sc.SetState(SCE_RAKU_TYPEDEF);	// Types (exceptions)
				} else {
					if (wordLast == "class")
						sc.SetState(SCE_RAKU_CLASS);	// a Class ident
					else if (wordLast == "grammar")
						sc.SetState(SCE_RAKU_GRAMMAR);	// a Grammar ident
					else
						sc.SetState(SCE_RAKU_IDENTIFIER);	// Bareword
					identLast = s;						// save identifier
				}
				if (adverbLast == "sym") {				// special adverb ":sym"
					sc.SetState(SCE_RAKU_IDENTIFIER);	// treat as identifier
					identLast = s;						// save identifier
				}
				if (sc.state != SCE_RAKU_IDENTIFIER)
					wordLast = s;					// save word
				sc.Forward(len - 1);				// ...forward past word
			}

			// --- Adverbs ----------------------------------------------------
			else if (sc.ch == ':' && IsWordStartChar(sc.chNext)) {
				len = LengthToNonWordChar(sc, lengthToEnd, s, sizeof(s), 1);
				if (adverbs.InList(s)) {
					sc.SetState(SCE_RAKU_ADVERB);	// Adverbs (begin with ':')
					adverbLast = s;					// save word
					sc.Forward(len); // ...forward past word (less offset: 1)
				}
			}

			// --- Identifiers: $mu / @positional / %associative / &callable --
			//     see: https://docs.raku.org/language/variables
			else if (setSigil.Contains(sc.ch) && (setTwigil.Contains(sc.chNext)
					|| setSpecialVar.Contains(sc.chNext)
					|| IsWordStartChar(sc.chNext))) {

				// State based on sigil
				switch (sc.ch) {
					case '$': sc.SetState(SCE_RAKU_MU);
						break;
					case '@': sc.SetState(SCE_RAKU_POSITIONAL);
						break;
					case '%': sc.SetState(SCE_RAKU_ASSOCIATIVE);
						break;
					case '&': sc.SetState(SCE_RAKU_CALLABLE);
				}
				const int state = sc.state;
				sc.Forward();
				char ch_delim = 0;
				if (setSpecialVar.Contains(sc.ch)
						&& !setWord.Contains(sc.chNext)) {	// Process Special Var
					ch_delim = -1;
				} else if (setTwigil.Contains(sc.ch)) {		// Process Twigil
					sc.SetState(SCE_RAKU_OPERATOR);
					if (sc.ch == '<' && setWord.Contains(sc.chNext))
						ch_delim = '>';
					sc.Forward();
					sc.SetState(state);
				}

				// Process (any) identifier
				if (ch_delim >= 0) {
					sc.Forward(LengthToNonWordChar(sc, lengthToEnd, s, sizeof(s)) - 1);
					if (ch_delim > 0 && sc.chNext == ch_delim) {
						sc.Forward();
						sc.SetState(SCE_RAKU_OPERATOR);
					}
					identLast = s;	// save identifier
				}
			}

			// --- Operators --------------------------------------------------
			else if (IsOperatorChar(sc.ch)) {
				// FIXME: better valid operator sequences needed?
				sc.SetState(SCE_RAKU_OPERATOR);
			}

			// --- Heredoc: begin ---------------------------------------------
			else if (!hereDelim.empty() && sc.atLineEnd) {
				if (IsANewLine(sc.ch))
					sc.Forward(); // skip a possible CRLF situation
				sc.SetState(hereState);
			}

			// Reset words: on operator simi-colon OR '}' (end of statement)
			if (sc.state == SCE_RAKU_OPERATOR && (sc.ch == ';' || sc.ch == '}')) {
				wordLast.clear();
				identLast.clear();
				adverbLast.clear();
			}
		}

		/* *** Determine if an "embedded comment" is to be entered ********** *
		 * This type of embedded comment section, or multi-line comment comes
		 * after a normal comment has begun... e.g: #`[ ... ]
		 * ****************************************************************** */
		else if (sc.state == SCE_RAKU_COMMENTLINE && sc.chPrev == '#' && sc.ch == '`') {
			if (IsBracketOpenChar(sc.chNext)) {
				sc.Forward(); // Condition met for "embedded comment"
				dpEmbeded.opener = sc.ch;

				// Find the opposite (termination) closeing bracket (if any)
				dpEmbeded.closer[0] = GetBracketCloseChar(dpEmbeded.opener);
				if (dpEmbeded.closer[0] > 0) { // Enter "embedded comment"

					// Find multiple opening character occurence
					dpEmbeded.count = GetRepeatCharCount(sc, dpEmbeded.opener, lengthToEnd);
					sc.SetState(SCE_RAKU_COMMENTEMBED);
					sc.Forward(dpEmbeded.count - 1); // incremented in the next loop
				}
			}
		}
	}

	// And we're done...
	sc.Complete();
}

/*
 * LexerRaku::Lex
 * - Main fold method
 *   NOTE: although Raku uses and supports UNICODE characters, we're only looking
 *         at normal chars here, using 'SafeGetCharAt' - for folding purposes
 *         that is all we need.
 */
#define RAKU_HEADFOLD_SHIFT	4
#define RAKU_HEADFOLD_MASK	0xF0
void SCI_METHOD LexerRaku::Fold(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {

	// init LexAccessor / return if fold option is off
	if (!options.fold) return;
	LexAccessor styler(pAccess);

	// init char and line positions
	const Sci_PositionU endPos = startPos + length;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	// Backtrack to last SCE_RAKU_DEFAULT line
	if (startPos > 0 && lineCurrent > 0) {
		while (lineCurrent > 0 && styler.StyleAt(startPos) != SCE_RAKU_DEFAULT) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
		lineCurrent = styler.GetLine(startPos);
	}
	Sci_PositionU lineStart = startPos;
	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent + 1);

	// init line folding level
	int levelPrev = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelPrev = styler.LevelAt(lineCurrent - 1) >> 16;
	int levelCurrent = levelPrev;

	// init char and style variables
	char chNext = styler[startPos];
	int stylePrev = styler.StyleAt(startPos - 1);
	int styleNext = styler.StyleAt(startPos);
	int styleNextStartLine = styler.StyleAt(lineStartNext);
	int visibleChars = 0;
	bool wasCommentMulti = false;

	// main loop
	for (Sci_PositionU i = startPos; i < endPos; i++) {

		// next char, style and flags
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		const bool atEOL = i == (lineStartNext - 1);
		const bool atLineStart = i == lineStart;

		// --- Comments / Multi-line / POD ------------------------------------
		if (options.foldComment) {

			// Multi-line
			if (options.foldCommentMultiline) {
				if (style == SCE_RAKU_COMMENTLINE && atLineStart && ch == '#' && chNext == '`'
						&& styleNextStartLine == SCE_RAKU_COMMENTEMBED) {
					levelCurrent++;
					wasCommentMulti = true; // don't confuse line comments
				} else if (style == SCE_RAKU_COMMENTEMBED && atLineStart
						&& styleNextStartLine != SCE_RAKU_COMMENTEMBED) {
					levelCurrent--;
				}
			}

			// Line comments
			if (!wasCommentMulti && atEOL && stylePrev == SCE_RAKU_COMMENTLINE
					&& IsCommentLine(lineCurrent, styler)) {
				if (!IsCommentLine(lineCurrent - 1, styler)
						&& IsCommentLine(lineCurrent + 1, styler))
					levelCurrent++;
				else if (IsCommentLine(lineCurrent - 1, styler)
						&& !IsCommentLine(lineCurrent + 1, styler))
					levelCurrent--;
			}

			// POD
			if (options.foldCommentPOD && atLineStart && style == SCE_RAKU_POD) {
				if (styler.Match(i, "=begin"))
					levelCurrent++;
				else if (styler.Match(i, "=end"))
					levelCurrent--;
			}
		}

		// --- Code block -----------------------------------------------------
		if (style == SCE_RAKU_OPERATOR) {
			if (ch == '{') {
				if (levelCurrent < levelPrev) levelPrev--;
				levelCurrent++;
			} else if (ch == '}') {
				levelCurrent--;
			}
		}

		// --- at end of line / range / apply fold ----------------------------
		if (atEOL) {
			int level = levelPrev;

			// set level flags
			level |= levelCurrent << 16;
			if (visibleChars == 0 && options.foldCompact)
				level |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				level |= SC_FOLDLEVELHEADERFLAG;
			if (level != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, level);
			}
			lineCurrent++;
			lineStart = lineStartNext;
			lineStartNext = styler.LineStart(lineCurrent + 1);
			styleNextStartLine = styler.StyleAt(lineStartNext);
			levelPrev = levelCurrent;
			visibleChars = 0;
			wasCommentMulti = false;
		}

		// increment visibleChars / set previous char
		if (!isspacechar(ch))
			visibleChars++;
		stylePrev = style;
	}

	// Done: set real level of the next line
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

/*----------------------------------------------------------------------------*
 * --- Scintilla: LexerModule ---
 *----------------------------------------------------------------------------*/

LexerModule lmRaku(SCLEX_RAKU, LexerRaku::LexerFactoryRaku, "raku", rakuWordLists);
