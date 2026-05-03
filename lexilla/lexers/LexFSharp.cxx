/**
 * @file LexFSharp.cxx
 * Lexer for F# 5.0
 * Copyright (c) 2021 Robert Di Pardo <dipardo.r@gmail.com>
 * Parts of LexerFSharp::Lex were adapted from LexCaml.cxx by Robert Roessler ("RR").
 * Parts of LexerFSharp::Fold were adapted from LexBash.cxx by Neil Hodgson and Kein-Hong Man.
 * The License.txt file describes the conditions under which this software may be distributed.
 */
// clang-format off
#include <cstdlib>
#include <cassert>

#include <string>
#include <string_view>
#include <map>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"
// clang-format on

using namespace Scintilla;
using namespace Lexilla;

static const char *const lexerName = "fsharp";
static constexpr int WORDLIST_SIZE = 5;
static const char *const fsharpWordLists[] = {
	"standard language keywords",
	"core functions, including those in the FSharp.Collections namespace",
	"built-in types, core namespaces, modules",
	"optional",
	"optional",
	nullptr,
};
static constexpr int keywordClasses[] = {
	SCE_FSHARP_KEYWORD, SCE_FSHARP_KEYWORD2, SCE_FSHARP_KEYWORD3, SCE_FSHARP_KEYWORD4, SCE_FSHARP_KEYWORD5,
};

namespace {

struct OptionsFSharp {
	bool fold = true;
	bool foldCompact = true;
	bool foldComment = true;
	bool foldCommentStream = true;
	bool foldCommentMultiLine = true;
	bool foldPreprocessor = false;
	bool foldQuotes = false;
	bool foldImports = true;
};

struct OptionSetFSharp : public OptionSet<OptionsFSharp> {
	OptionSetFSharp() {
		DefineProperty("fold", &OptionsFSharp::fold);
		DefineProperty("fold.compact", &OptionsFSharp::foldCompact);
		DefineProperty("fold.comment", &OptionsFSharp::foldComment,
				   "Setting this option to 0 disables comment folding in F# files.");

		DefineProperty("fold.fsharp.comment.stream", &OptionsFSharp::foldCommentStream,
				   "Setting this option to 0 disables folding of ML-style comments in F# files when "
				   "fold.comment=1.");

		DefineProperty("fold.fsharp.comment.multiline", &OptionsFSharp::foldCommentMultiLine,
				   "Setting this option to 0 disables folding of grouped line comments in F# files when "
				   "fold.comment=1.");

		DefineProperty("fold.fsharp.preprocessor", &OptionsFSharp::foldPreprocessor,
				   "Setting this option to 1 enables folding of F# compiler directives.");

		DefineProperty("fold.fsharp.quotes", &OptionsFSharp::foldQuotes,
				   "Setting this option to 1 enables folding of multi-line strings in F# files.");

		DefineProperty("fold.fsharp.imports", &OptionsFSharp::foldImports,
				   "Setting this option to 0 disables folding of F# import declarations.");

		DefineWordListSets(fsharpWordLists);
	}
};

class UnicodeChar {
	enum class Notation { none, asciiDec, asciiHex, utf16, utf32 };
	Notation type = Notation::none;
	// single-byte Unicode char (000 - 255)
	int asciiDigits[3] = { 0 };
	int maxDigit = '9';
	int toEnd = 0;
	bool invalid = false;

public:
	UnicodeChar() noexcept = default;
	explicit UnicodeChar(const int prefix) {
		if (IsADigit(prefix)) {
			*asciiDigits = prefix;
			if (*asciiDigits >= '0' && *asciiDigits <= '2') {
				type = Notation::asciiDec;
				// count first digit as "prefix"
				toEnd = 2;
			}
		} else if (prefix == 'x' || prefix == 'u' || prefix == 'U') {
			switch (prefix) {
				case 'x':
					type = Notation::asciiHex;
					toEnd = 2;
					break;
				case 'u':
					type = Notation::utf16;
					toEnd = 4;
					break;
				case 'U':
					type = Notation::utf32;
					toEnd = 8;
					break;
			}
		}
	}
	void Parse(const int ch) {
		invalid = false;
		switch (type) {
			case Notation::asciiDec: {
				maxDigit = (*asciiDigits < '2') ? '9' : (asciiDigits[1] <= '4') ? '9' : '5';
				if (IsADigit(ch) && asciiDigits[1] <= maxDigit && ch <= maxDigit) {
					asciiDigits[1] = ch;
					toEnd--;
				} else {
					invalid = true;
				}
				break;
			}
			case Notation::asciiHex:
			case Notation::utf16:
				if (IsADigit(ch, 16)) {
					toEnd--;
				} else {
					invalid = true;
				}
				break;
			case Notation::utf32:
				if ((toEnd > 6 && ch == '0') || (toEnd <= 6 && IsADigit(ch, 16))) {
					toEnd--;
				} else {
					invalid = true;
				}
				break;
			case Notation::none:
				break;
		}
	}
	constexpr bool AtEnd() noexcept {
		return invalid || type == Notation::none || (type != Notation::none && toEnd < 0);
	}
};

inline bool MatchStreamCommentStart(StyleContext &cxt) {
	// match (* ... *), but allow point-free usage of the `*` operator,
	// e.g.  List.fold (*) 1 [ 1; 2; 3 ]
	return (cxt.Match('(', '*') && cxt.GetRelative(2) != ')');
}

inline bool MatchStreamCommentEnd(const StyleContext &cxt) {
	return (cxt.ch == ')' && cxt.chPrev == '*');
}

inline bool MatchLineComment(const StyleContext &cxt) {
	// style shebang lines as comments in F# scripts:
	// https://fsharp.org/specs/language-spec/4.1/FSharpSpec-4.1-latest.pdf#page=30&zoom=auto,-98,537
	return cxt.Match('/', '/') || cxt.Match('#', '!');
}

inline bool MatchLineNumberStart(StyleContext &cxt) {
	return cxt.atLineStart && (cxt.MatchIgnoreCase("#line") ||
		(cxt.ch == '#' && (IsADigit(cxt.chNext) || IsADigit(cxt.GetRelative(2)))));
}

inline bool MatchPPDirectiveStart(const StyleContext &cxt) {
	return (cxt.atLineStart && cxt.ch == '#' && iswordstart(cxt.chNext));
}

inline bool MatchTypeAttributeStart(const StyleContext &cxt) {
	return cxt.Match('[', '<');
}

inline bool MatchTypeAttributeEnd(const StyleContext &cxt) {
	return (cxt.ch == ']' && cxt.chPrev == '>');
}

inline bool MatchQuotedExpressionStart(const StyleContext &cxt) {
	return cxt.Match('<', '@');
}

inline bool MatchQuotedExpressionEnd(const StyleContext &cxt) {
	return (cxt.ch == '>' && cxt.chPrev == '@');
}

inline bool MatchStringStart(StyleContext &cxt) {
	return (cxt.ch == '"' || (cxt.chNext == '"' && AnyOf(cxt.ch, '@', '$')) ||
		(cxt.GetRelative(2, '"') && (cxt.Match('@', '$') || cxt.Match('$', '@'))));
}

inline bool FollowsEscapedBackslash(StyleContext &cxt) {
	int count = 0;
	for (Sci_Position offset = 1; cxt.GetRelative(-offset) == '\\'; offset++)
		count++;
	return count % 2 != 0;
}

inline bool MatchStringEnd(StyleContext &cxt, bool &rawString) {
	bool result = false;
	switch (cxt.state) {
		case SCE_FSHARP_STRING:
			result =
			    (rawString && cxt.Match("\"\"\"") && cxt.chPrev != '$') ||
			    (!rawString && cxt.ch == '"' &&
			     (cxt.chPrev != '\\' || (cxt.GetRelative(-2) == '\\' && !FollowsEscapedBackslash(cxt))));
			break;
		case SCE_FSHARP_VERBATIM:
			result =
			    // embedded quotes must be in pairs
			    cxt.ch == '"' && cxt.chNext != '"' &&
			    (cxt.chPrev != '"' ||
			     // empty verbatim string?
			     (AnyOf(cxt.GetRelative(-2), '@', '$') ||
			      // pair of quotes at end of verbatim string?
			      (cxt.GetRelative(-2) == '"' && !(AnyOf(cxt.GetRelative(-3), '@', '$')))));
			break;
		default:
			break;
	}
	if (rawString && result)
		rawString = false;
	return result;
}

inline bool MatchCharacterStart(StyleContext &cxt) {
	// don't style generic type parameters: 'a, 'b, 'T, etc.
	return (cxt.ch == '\'' && !(cxt.chPrev == ':' || cxt.GetRelative(-2) == ':'));
}

inline bool IsLineEnd(StyleContext &cxt, const Sci_Position offset) {
	const int ch = cxt.GetRelative(offset, '\n');
	return (ch == '\r' || ch == '\n');
}

class LexerFSharp : public DefaultLexer {
	WordList keywords[WORDLIST_SIZE];
	OptionsFSharp options;
	OptionSetFSharp optionSet;
	CharacterSet setOperators;
	CharacterSet setFormatSpecs;
	CharacterSet setDotNetFormatSpecs;
	CharacterSet setFormatFlags;
	CharacterSet numericMetaChars1;
	CharacterSet numericMetaChars2;
	std::map<int, int> numericPrefixes = { { 'b', 2 }, { 'o', 8 }, { 'x', 16 } };
	static constexpr int rawStringMask = 0x10000;
	static constexpr int interpolatedStringMask = 0x20000;

public:
	explicit LexerFSharp()
	    : DefaultLexer(lexerName, SCLEX_FSHARP),
	      setOperators(CharacterSet::setNone, "~^'-+*/%=@|&<>()[]{};,:!?"),
	      setFormatSpecs(CharacterSet::setNone, ".%aAbBcdeEfFgGiMoOstuxX0123456789"),
	      setDotNetFormatSpecs(CharacterSet::setNone, "cCdDeEfFgGnNpPxX"),
	      setFormatFlags(CharacterSet::setNone, ".-+0 "),
	      numericMetaChars1(CharacterSet::setNone, "_uU"),
	      numericMetaChars2(CharacterSet::setNone, "fFIlLmMnsy") {
		SetOptionSet(&optionSet);
	}
	LexerFSharp(const LexerFSharp &) = delete;
	LexerFSharp(LexerFSharp &&) = delete;
	LexerFSharp &operator=(const LexerFSharp &) = delete;
	LexerFSharp &operator=(LexerFSharp &&) = delete;
	~LexerFSharp() override = default;
	static ILexer5 *LexerFactoryFSharp() {
		return new LexerFSharp();
	}
	void SCI_METHOD Release() noexcept override {
		delete this;
	}
	int SCI_METHOD Version() const noexcept override {
		return lvRelease5;
	}
	const char *SCI_METHOD GetName() noexcept override {
		return lexerName;
	}
	int SCI_METHOD GetIdentifier() noexcept override {
		return SCLEX_FSHARP;
	}
	int SCI_METHOD LineEndTypesSupported() noexcept override {
		return SC_LINE_END_TYPE_DEFAULT;
	}
	void *SCI_METHOD PrivateCall(int, void *) noexcept override {
		return nullptr;
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override {
		if (optionSet.PropertySet(&options, key, val)) {
			return 0;
		}
		return INVALID_POSITION;
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU start, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU start, Sci_Position length, int initStyle,IDocument *pAccess) override;

private:
	inline bool IsNumber(StyleContext &cxt, const int base = 10) {
		return IsADigit(cxt.ch, base) || (IsADigit(cxt.chPrev, base) && numericMetaChars1.Contains(cxt.ch)) ||
		       (IsADigit(cxt.GetRelative(-2), base) && numericMetaChars2.Contains(cxt.ch));
	}

	inline bool IsFloat(StyleContext &cxt) {
		if (cxt.MatchIgnoreCase("e+") || cxt.MatchIgnoreCase("e-")) {
			cxt.Forward();
			return true;
		}
		return ((cxt.chPrev == '.' && IsADigit(cxt.ch)) ||
			(IsADigit(cxt.chPrev) && (cxt.ch == '.' || numericMetaChars2.Contains(cxt.ch))));
	}
};

Sci_Position SCI_METHOD LexerFSharp::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	Sci_Position firstModification = INVALID_POSITION;

	if (n < WORDLIST_SIZE) {
		wordListN = &keywords[n];
	}
	if (wordListN && wordListN->Set(wl)) {
		firstModification = 0;
	}
	return firstModification;
}

void SCI_METHOD LexerFSharp::Lex(Sci_PositionU start, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	StyleContext sc(start, static_cast<Sci_PositionU>(length), initStyle, styler);
	Sci_Position lineCurrent = styler.GetLine(static_cast<Sci_Position>(start));
	Sci_PositionU cursor = 0;
	UnicodeChar uniCh = UnicodeChar();
	std::string token;
	constexpr Sci_Position MAX_WORD_LEN = 64;
	constexpr int SPACE = ' ';
	int currentBase = 10;
	int stringResumeState = SCE_FSHARP_STRING;
	int lineState = (lineCurrent >= 1) ? styler.GetLineState(lineCurrent - 1) : 0;
	int levelNesting = lineState & 0xff;
	bool isRawString = (lineState & rawStringMask) != 0;
	bool canInterpolate = (lineState & interpolatedStringMask) != 0;
	bool isInterpolated = false;

	while (sc.More()) {
		Sci_PositionU colorSpan = sc.currentPos - 1;
		int state = -1;
		bool advance = true;

		switch (sc.state & 0xff) {
			case SCE_FSHARP_DEFAULT:
				cursor = sc.currentPos;

				if (MatchLineNumberStart(sc)) {
					state = SCE_FSHARP_LINENUM;
				} else if (MatchPPDirectiveStart(sc)) {
					state = SCE_FSHARP_PREPROCESSOR;
				} else if (MatchLineComment(sc)) {
					state = SCE_FSHARP_COMMENTLINE;
					sc.Forward();
					sc.ch = SPACE;
				} else if (MatchStreamCommentStart(sc)) {
					state = SCE_FSHARP_COMMENT;
					sc.Forward();
					sc.ch = SPACE;
				} else if (MatchTypeAttributeStart(sc)) {
					state = SCE_FSHARP_ATTRIBUTE;
					sc.Forward();
				} else if (MatchQuotedExpressionStart(sc)) {
					state = SCE_FSHARP_QUOTATION;
					sc.Forward();
				} else if (MatchCharacterStart(sc)) {
					state = SCE_FSHARP_CHARACTER;
				} else if (MatchStringStart(sc)) {
					bool isVerbatim = sc.ch == '@' || sc.Match('$', '@');
					canInterpolate = sc.ch == '$' || sc.Match('@', '$');
					// allow unescaped double quotes inside literal or interpolated raw strings
					isRawString = sc.Match("$\"\"\"") || sc.Match("\"\"\"") ||
						      sc.Match("@$\"\"\"") || sc.Match("$@\"\"\"");
					state = isVerbatim ? SCE_FSHARP_VERBATIM : SCE_FSHARP_STRING;
				} else if (IsADigit(sc.ch, currentBase) ||
					   ((sc.ch == '+' || sc.ch == '-') && IsADigit(sc.chNext))) {
					state = SCE_FSHARP_NUMBER;
				} else if (setOperators.Contains(sc.ch) &&
					   // don't use operator style in async keywords (e.g. `return!`)
					   !(sc.ch == '!' && iswordstart(sc.chPrev)) &&
					   // don't use operator style in member access, array/string indexing
					   !(sc.ch == '.' && (sc.chPrev == '\"' || iswordstart(sc.chPrev)) &&
					     (iswordstart(sc.chNext) || sc.chNext == '['))) {
					state = SCE_FSHARP_OPERATOR;
				} else if (sc.Match('`', '`')) {
					sc.Forward();
					state = SCE_FSHARP_QUOT_IDENTIFIER;
				} else if (iswordstart(sc.ch)) {
					state = SCE_FSHARP_IDENTIFIER;
				} else {
					state = SCE_FSHARP_DEFAULT;
				}
				break;
			case SCE_FSHARP_LINENUM:
			case SCE_FSHARP_PREPROCESSOR:
				if (sc.MatchLineEnd()) {
					state = SCE_FSHARP_DEFAULT;
				}
				break;
			case SCE_FSHARP_COMMENTLINE:
				if (sc.atLineStart) {
					state = SCE_FSHARP_DEFAULT;
					advance = false;
				}
				break;
			case SCE_FSHARP_COMMENT:
				if (MatchStreamCommentStart(sc)) {
					sc.Forward();
					sc.ch = SPACE;
					levelNesting++;
				} else if (MatchStreamCommentEnd(sc)) {
					if (levelNesting > 0)
						levelNesting--;
					else {
						state = SCE_FSHARP_DEFAULT;
						colorSpan++;
					}
				}
				break;
			case SCE_FSHARP_ATTRIBUTE:
			case SCE_FSHARP_QUOTATION:
				if (MatchTypeAttributeEnd(sc) || MatchQuotedExpressionEnd(sc)) {
					state = SCE_FSHARP_DEFAULT;
					colorSpan++;
				}
				break;
			case SCE_FSHARP_CHARACTER:
				if (sc.chPrev == '\\' && sc.GetRelative(-2) != '\\') {
					uniCh = UnicodeChar(sc.ch);
				} else if (sc.ch == '\'' &&
					   ((sc.chPrev == ' ' && sc.GetRelative(-2) == '\'') || sc.chPrev != '\\' ||
						(sc.chPrev == '\\' && sc.GetRelative(-2) == '\\'))) {
					// byte literal?
					if (sc.Match('\'', 'B')) {
						sc.Forward();
						colorSpan++;
					}
					if (!sc.atLineEnd) {
						colorSpan++;
					} else {
						sc.ChangeState(SCE_FSHARP_IDENTIFIER);
					}
					state = SCE_FSHARP_DEFAULT;
				} else {
					uniCh.Parse(sc.ch);
					if (uniCh.AtEnd() && (sc.currentPos - cursor) >= 2) {
						// terminate now, since we left the char behind
						sc.ChangeState(SCE_FSHARP_IDENTIFIER);
						advance = false;
					}
				}
				break;
			case SCE_FSHARP_STRING:
			case SCE_FSHARP_VERBATIM:
				if (MatchStringEnd(sc, isRawString)) {
					const Sci_Position strLen = static_cast<Sci_Position>(sc.currentPos - cursor);
					// backtrack to start of string
					for (Sci_Position i = -strLen; i < 0; i++) {
						const int startQuote = sc.GetRelative(i);
						if (startQuote == '\"') {
							// byte array?
							if (sc.Match('\"', 'B')) {
								sc.Forward();
								colorSpan++;
							}
							if (!sc.atLineEnd) {
								colorSpan++;
							} else {
								sc.ChangeState(SCE_FSHARP_IDENTIFIER);
							}
							state = SCE_FSHARP_DEFAULT;
							canInterpolate = false;
							break;
						}
					}
				} else if (sc.ch == '%' && !(sc.Match("%  ") || sc.Match("% \"")) &&
					   (setFormatSpecs.Contains(sc.chNext) || setFormatFlags.Contains(sc.chNext))) {
					if (canInterpolate && sc.chNext != '%') {
						for (Sci_Position i = 2; i < length && !IsLineEnd(sc, i); i++) {
							if (sc.GetRelative(i) == '{') {
								state = setFormatSpecs.Contains(sc.GetRelative(i - 1))
									    ? SCE_FSHARP_FORMAT_SPEC
									    : state;
								break;
							}
						}
					} else {
						state = SCE_FSHARP_FORMAT_SPEC;
					}
				} else if (isInterpolated) {
					if (sc.ch == ',') {
						// .NET alignment specifier?
						state = (sc.chNext == '+' || sc.chNext == '-' || IsADigit(sc.chNext))
							    ? SCE_FSHARP_FORMAT_SPEC
							    : state;
					} else if (sc.ch == ':') {
						// .NET format specifier?
						state = setDotNetFormatSpecs.Contains(sc.chNext)
							    ? SCE_FSHARP_FORMAT_SPEC
							    : state;
					} else if (sc.chNext == '}') {
						isInterpolated = false;
						sc.Forward();
					}
				} else if (canInterpolate && sc.ch == '{') {
					isInterpolated = true;
				}
				break;
			case SCE_FSHARP_QUOT_IDENTIFIER:
				if (sc.ch == '`' && sc.chPrev == '`') {
					colorSpan++;
					state = SCE_FSHARP_DEFAULT;
				}
				break;
			case SCE_FSHARP_IDENTIFIER:
				if (!(iswordstart(sc.ch) || AnyOf(sc.ch, '\'', '!'))) {
					const Sci_PositionU wordLen = sc.currentPos - cursor;
					if (wordLen < MAX_WORD_LEN) {
						// wordLength is believable as keyword, [re-]construct token - RR
						token = styler.GetRange(sc.currentPos - wordLen, sc.currentPos);
						// except for the standard source file macros,
						// a snake_case_identifier can never be a keyword
						if (token.find('_') == token.npos) {
							for (int i = 0; i < WORDLIST_SIZE; i++) {
								if (keywords[i].InList(token)) {
									sc.ChangeState(keywordClasses[i]);
									break;
								}
							}
						} else if (token == "__LINE__" || token == "__SOURCE_DIRECTORY__" ||
							   token == "__SOURCE_FILE__") {
							sc.ChangeState(SCE_FSHARP_KEYWORD);
						}
					}
					state = SCE_FSHARP_DEFAULT;
					advance = false;
				}
				break;
			case SCE_FSHARP_OPERATOR:
				// special-case "()" and "[]" tokens as KEYWORDS - RR
				if ((sc.ch == ')' && sc.chPrev == '(') || (sc.ch == ']' && sc.chPrev == '[')) {
					sc.ChangeState(SCE_FSHARP_KEYWORD);
					colorSpan++;
				} else {
					advance = false;
				}
				state = SCE_FSHARP_DEFAULT;
				break;
			case SCE_FSHARP_NUMBER:
				if ((setOperators.Contains(sc.chPrev) || IsASpaceOrTab(sc.chPrev)) && sc.ch == '0') {
					if (numericPrefixes.find(sc.chNext) != numericPrefixes.end()) {
						currentBase = numericPrefixes[sc.chNext];
						sc.Forward(2);
					}
				} else if ((setOperators.Contains(sc.GetRelative(-2)) || IsASpaceOrTab(sc.GetRelative(-2))) &&
					   sc.chPrev == '0') {
					if (numericPrefixes.find(sc.ch) != numericPrefixes.end()) {
						currentBase = numericPrefixes[sc.ch];
						sc.Forward();
					}
				}
				state = (IsNumber(sc, currentBase) || IsFloat(sc))
					? SCE_FSHARP_NUMBER
					// change style even when operators aren't spaced
					: setOperators.Contains(sc.ch) ? SCE_FSHARP_OPERATOR : SCE_FSHARP_DEFAULT;
				currentBase = (state == SCE_FSHARP_NUMBER) ? currentBase : 10;
				break;
			case SCE_FSHARP_FORMAT_SPEC:
				if (!(isInterpolated && IsADigit(sc.chNext)) &&
					(!setFormatSpecs.Contains(sc.chNext) ||
				    !(setFormatFlags.Contains(sc.ch) || IsADigit(sc.ch)) ||
				    (setFormatFlags.Contains(sc.ch) && sc.ch == sc.chNext))) {
					colorSpan++;
					state = stringResumeState;
				}
				break;
		}

		if (sc.MatchLineEnd()) {
			lineState = (sc.state == SCE_FSHARP_COMMENT) ? levelNesting : 0;
			if (sc.state == SCE_FSHARP_STRING || sc.state == SCE_FSHARP_VERBATIM)
				lineState |= (sc.state << 8);
			if (isRawString)
				lineState |= rawStringMask;
			if (canInterpolate)
				lineState |= interpolatedStringMask;
			styler.SetLineState(lineCurrent++, lineState);
			advance = true;
		}

		if (state >= SCE_FSHARP_DEFAULT) {
			if (state == SCE_FSHARP_FORMAT_SPEC)
				stringResumeState = sc.state;
			styler.ColourTo(colorSpan, sc.state);
			sc.ChangeState(state);
		}

		if (advance) {
			sc.Forward();
		}
	}

	sc.Complete();
}

bool LineContains(LexAccessor &styler, const char *word, const Sci_Position start,
		  const int chAttr = SCE_FSHARP_DEFAULT);

void FoldLexicalGroup(LexAccessor &styler, int &levelNext, const Sci_Position lineCurrent, const char *word,
		      const int chAttr);

void SCI_METHOD LexerFSharp::Fold(Sci_PositionU start, Sci_Position length, int initStyle, IDocument *pAccess) {
	if (!options.fold) {
		return;
	}

	LexAccessor styler(pAccess);
	Sci_Position startPos = static_cast<Sci_Position>(start);
	const Sci_PositionU endPos = start + length;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (lineCurrent > 0) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
		initStyle = (startPos > 0) ? styler.StyleAt(startPos - 1) : SCE_FSHARP_DEFAULT;
	}
	Sci_Position lineNext = lineCurrent + 1;
	Sci_Position lineStartNext = styler.LineStart(lineNext);
	int style = initStyle;
	int styleNext = styler.StyleAt(startPos);
	char chNext = styler[startPos];
	int levelCurrent = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelNext = levelCurrent;
	int visibleChars = 0;

	for (Sci_PositionU i = static_cast<Sci_PositionU>(startPos); i < endPos; i++) {
		const Sci_Position currentPos = static_cast<Sci_Position>(i);
		const bool atEOL = (currentPos == (lineStartNext - 1));
		const bool atLineOrDocEnd = (atEOL || (i == (endPos - 1)));
		const int stylePrev = style;
		const char ch = chNext;
		const bool inLineComment = (stylePrev == SCE_FSHARP_COMMENTLINE);
		style = styleNext;
		styleNext = styler.StyleAt(currentPos + 1);
		chNext = styler.SafeGetCharAt(currentPos + 1);

		if (options.foldComment) {
			if (options.foldCommentMultiLine && inLineComment && atEOL) {
				FoldLexicalGroup(styler, levelNext, lineCurrent, "//", SCE_FSHARP_COMMENTLINE);
			}

			if (options.foldCommentStream && style == SCE_FSHARP_COMMENT && !inLineComment) {
				if (stylePrev != SCE_FSHARP_COMMENT ||
				    (styler.Match(currentPos, "(*") &&
				     !LineContains(styler, "*)", currentPos + 2, SCE_FSHARP_COMMENT))) {
					levelNext++;
				} else if ((styleNext != SCE_FSHARP_COMMENT ||
					    ((styler.Match(currentPos, "*)") &&
					      !LineContains(styler, "(*", styler.LineStart(lineCurrent), SCE_FSHARP_COMMENT)) &&
					     styler.GetLineState(lineCurrent - 1) > 0)) &&
					   !atEOL) {
					levelNext--;
				}
			}
		}

		if (options.foldQuotes && atEOL) {
			const int lineState = (styler.GetLineState(lineCurrent > 0 ? lineCurrent - 1 : lineCurrent) >> 8) & 0xff;
			const bool isQuoted = (lineState == SCE_FSHARP_STRING || lineState == SCE_FSHARP_VERBATIM);
			const bool isStringlike = (style == SCE_FSHARP_STRING || style == SCE_FSHARP_VERBATIM);
			if (isStringlike && (!isQuoted || lineCurrent == 0)) {
				levelNext++;
			} else if (!isStringlike && isQuoted) {
				levelNext--;
			}
		}

		if (options.foldPreprocessor && style == SCE_FSHARP_PREPROCESSOR) {
			if (styler.Match(currentPos, "#if")) {
				levelNext++;
			} else if (styler.Match(currentPos, "#endif")) {
				levelNext--;
			}
		}

		if (options.foldImports && styler.Match(currentPos, "open ") && styleNext == SCE_FSHARP_KEYWORD) {
			FoldLexicalGroup(styler, levelNext, lineCurrent, "open ", SCE_FSHARP_KEYWORD);
		}

		if (atLineOrDocEnd) {
			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;

			if (visibleChars == 0 && options.foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelUse < levelNext) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			visibleChars = 0;
			lineCurrent++;
			lineNext = lineCurrent + 1;
			lineStartNext = styler.LineStart(lineNext);
			levelCurrent = levelNext;
		}

		if (!IsASpace(ch)) {
			visibleChars++;
		}
	}
	const int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelCurrent | flagsNext);
}

bool LineContains(LexAccessor &styler, const char *word, const Sci_Position start, const int chAttr) {
	bool found = false;
	bool requireStyle = (chAttr > SCE_FSHARP_DEFAULT);
	for (Sci_Position i = start; i < styler.LineStart(styler.GetLine(start) + 1) - 1; i++) {
		if (styler.Match(i, word)) {
			found = requireStyle ? styler.StyleAt(i) == chAttr : true;
			break;
		}
	}
	return found;
}

void FoldLexicalGroup(LexAccessor &styler, int &levelNext, const Sci_Position lineCurrent, const char *word,
		      const int chAttr) {
	const Sci_Position linePrev = styler.LineStart(lineCurrent - 1);
	const Sci_Position lineNext = styler.LineStart(lineCurrent + 1);
	const bool follows = (lineCurrent > 0) && LineContains(styler, word, linePrev, chAttr);
	const bool isFollowed = LineContains(styler, word, lineNext, chAttr);

	if (isFollowed && !follows) {
		levelNext++;
	} else if (!isFollowed && follows && levelNext > SC_FOLDLEVELBASE) {
		levelNext--;
	}
}
} // namespace

extern const LexerModule lmFSharp(SCLEX_FSHARP, LexerFSharp::LexerFactoryFSharp, "fsharp", fsharpWordLists);
