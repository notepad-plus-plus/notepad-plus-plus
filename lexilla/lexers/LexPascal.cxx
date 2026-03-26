// Scintilla source code edit control
/** @file LexPascal.cxx
 ** Lexer for Pascal.
 ** Written by Laurent le Tynevez
 ** Updated by Simon Steele <s.steele@pnotepad.org> September 2002
 ** Updated by Mathias Rauen <scite@madshi.net> May 2003 (Delphi adjustments)
 ** Completely rewritten by Marko Njezic <sf@maxempire.com> October 2008
 **/

/*

A few words about features of the new completely rewritten LexPascal...

Generally speaking LexPascal tries to support all available Delphi features (up
to Delphi XE4 at this time).

~ HIGHLIGHTING:

If you enable "lexer.pascal.smart.highlighting" property, some keywords will
only be highlighted in appropriate context. As implemented those are keywords
related to property and DLL exports declarations (similar to how Delphi IDE
works).

For example, keywords "read" and "write" will only be highlighted if they are in
property declaration:

property MyProperty: boolean read FMyProperty write FMyProperty;

~ FOLDING:

Folding is supported in the following cases:

- Folding of stream-like comments
- Folding of groups of consecutive line comments
- Folding of preprocessor blocks (the following preprocessor blocks are
supported: IF / IFEND; IFDEF, IFNDEF, IFOPT / ENDIF and REGION / ENDREGION
blocks), including nesting of preprocessor blocks up to 255 levels
- Folding of code blocks on appropriate keywords (the following code blocks are
supported: "begin, asm, record, try, case / end" blocks, class & object
declarations and interface declarations)

Remarks:

- Folding of code blocks tries to handle all special cases in which folding
should not occur. As implemented those are:

1. Structure "record case / end" (there's only one "end" statement and "case" is
ignored as fold point)
2. Forward class declarations ("type TMyClass = class;") and object method
declarations ("TNotifyEvent = procedure(Sender: TObject) of object;") are
ignored as fold points
3. Simplified complete class declarations ("type TMyClass = class(TObject);")
are ignored as fold points
4. Every other situation when class keyword doesn't actually start class
declaration ("class procedure", "class function", "class of", "class var",
"class property" and "class operator")
5. Forward (disp)interface declarations ("type IMyInterface = interface;") are
ignored as fold points

- Folding of code blocks inside preprocessor blocks is disabled (any comments
inside them will be folded fine) because there is no guarantee that complete
code block will be contained inside folded preprocessor block in which case
folded code block could end prematurely at the end of preprocessor block if
there is no closing statement inside. This was done in order to properly process
document that may contain something like this:

type
{$IFDEF UNICODE}
  TMyClass = class(UnicodeAncestor)
{$ELSE}
  TMyClass = class(AnsiAncestor)
{$ENDIF}
  private
  ...
  public
  ...
  published
  ...
end;

If class declarations were folded, then the second class declaration would end
at "$ENDIF" statement, first class statement would end at "end;" statement and
preprocessor "$IFDEF" block would go all the way to the end of document.
However, having in mind all this, if you want to enable folding of code blocks
inside preprocessor blocks, you can disable folding of preprocessor blocks by
changing "fold.preprocessor" property, in which case everything inside them
would be folded.

~ KEYWORDS:

The list of keywords that can be used in pascal.properties file (up to Delphi
XE4):

- Keywords: absolute abstract and array as asm assembler automated begin case
cdecl class const constructor delayed deprecated destructor dispid dispinterface
div do downto dynamic else end except experimental export exports external far
file final finalization finally for forward function goto helper if
implementation in inherited initialization inline interface is label library
message mod near nil not object of on operator or out overload override packed
pascal platform private procedure program property protected public published
raise record reference register reintroduce repeat resourcestring safecall
sealed set shl shr static stdcall strict string then threadvar to try type unit
unsafe until uses var varargs virtual while winapi with xor

- Keywords related to the "smart highlithing" feature: add default implements
index name nodefault read readonly remove stored write writeonly

- Keywords related to Delphi packages (in addition to all above): package
contains requires

*/

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <string>
#include <string_view>
#include <map>

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

using namespace Lexilla;

namespace {

// Options used for LexerPascal
struct OptionsPascal {
	bool bSmartHighlighting = true;
	bool fold = false;
	bool foldComment = false;
	bool foldPreprocessor = false;
	bool foldCompact = true;
};

const char *const pascalWordListDesc[] = {
	"Keywords",
	nullptr
};

struct OptionSetPascal : public OptionSet<OptionsPascal> {
	OptionSetPascal() {
		DefineProperty("lexer.pascal.smart.highlighting", &OptionsPascal::bSmartHighlighting,
			"Set to 0 to not completely handle some complex features like 'property'.");

		DefineProperty("fold", &OptionsPascal::fold);

		DefineProperty("fold.comment", &OptionsPascal::foldComment);

		DefineProperty("fold.preprocessor", &OptionsPascal::foldPreprocessor);

		DefineProperty("fold.compact", &OptionsPascal::foldCompact);

		DefineWordListSets(pascalWordListDesc);
	}
};

const LexicalClass lexicalClasses[] = {
	// Lexer pascal SCLEX_PASCAL SCE_PAS_
	0, "SCE_PAS_DEFAULT", "default", "White space",
	1, "SCE_PAS_IDENTIFIER", "identifier", "Identifier",
	2, "SCE_PAS_COMMENT", "comment", "Comment: { ... }",
	3, "SCE_PAS_COMMENT2", "comment", "Comment: (* ... *)",
	4, "SCE_PAS_COMMENTLINE","comment line", "Line Comment: // ...",
	5, "SCE_PAS_PREPROCESSOR", "preprocessor", "Preprocessor: {$ ... }",
	6, "SCE_PAS_PREPROCESSOR2", "preprocessor", "Preprocessor: (*$ ... *)",
	7, "SCE_PAS_NUMBER", "literal numeric", "Number",
	8, "SCE_PAS_HEXNUMBER", "literal numeric", "Hex Number",
	9, "SCE_PAS_WORD", "keyword", "Keyword",
	10, "SCE_PAS_STRING", "literal string", "Single quoted string",
	11, "SCE_PAS_STRINGEOL", "error literal string", "End of line where string is not closed",
	12, "SCE_PAS_CHARACTER","literal string character", "Character",
	13, "SCE_PAS_OPERATOR","operator", "Operator",
	14, "SCE_PAS_ASM", "assembler", "Inline Assembler", 
};

class LexerPascal : public DefaultLexer {
	WordList keywords;
	OptionsPascal options;
	OptionSetPascal osPascal;
public:
	explicit LexerPascal() :
		DefaultLexer("pascal", SCLEX_PASCAL, lexicalClasses, std::size(lexicalClasses)) {
		SetOptionSet(&osPascal);
	}
	LexerPascal(const LexerPascal &) = delete;
	LexerPascal(LexerPascal &&) = delete;
	LexerPascal &operator=(const LexerPascal &) = delete;
	LexerPascal &operator=(LexerPascal &&) = delete;
	~LexerPascal() override = default;

	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument *pAccess) override;

	static ILexer5 *LexerFactoryPascal() {
		return new LexerPascal();
	}
};

Sci_Position SCI_METHOD LexerPascal::PropertySet(const char *key, const char *val) {
	if (osPascal.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerPascal::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

void GetRangeLowered(Sci_Position start,
		Sci_Position end,
		LexAccessor &styler,
		char *s,
		Sci_Position len) {
	Sci_Position i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = MakeLowerCase(styler[start + i]);
		i++;
	}
	s[i] = '\0';
}

void GetForwardRangeLowered(Sci_Position start,
		const CharacterSet &charSet,
		LexAccessor &styler,
		char *s,
		Sci_Position len) {
	Sci_Position i = 0;
	while ((i < len-1) && charSet.Contains(styler.SafeGetCharAt(start + i))) {
		s[i] = MakeLowerCase(styler.SafeGetCharAt(start + i));
		i++;
	}
	s[i] = '\0';

}

enum {
	stateInAsm = 0x1000,
	stateInProperty = 0x2000,
	stateInExport = 0x4000,
	stateFoldInPreprocessor = 0x0100,
	stateFoldInRecord = 0x0200,
	stateFoldInPreprocessorLevelMask = 0x00FF,
	stateFoldMaskAll = 0x0FFF
};

void ClassifyPascalWord(const WordList &keywords, StyleContext &sc, int &curLineState, bool bSmartHighlighting) {
	char s[100];
	sc.GetCurrentLowered(s, sizeof(s));
	if (keywords.InList(s)) {
		if (curLineState & stateInAsm) {
			if (strcmp(s, "end") == 0 && sc.GetRelative(-4) != '@') {
				curLineState &= ~stateInAsm;
				sc.ChangeState(SCE_PAS_WORD);
			} else {
				sc.ChangeState(SCE_PAS_ASM);
			}
		} else {
			bool ignoreKeyword = false;
			if (strcmp(s, "asm") == 0) {
				curLineState |= stateInAsm;
			} else if (bSmartHighlighting) {
				if (strcmp(s, "property") == 0) {
					curLineState |= stateInProperty;
				} else if (strcmp(s, "exports") == 0) {
					curLineState |= stateInExport;
				} else if (!(curLineState & (stateInProperty | stateInExport)) && strcmp(s, "index") == 0) {
					ignoreKeyword = true;
				} else if (!(curLineState & stateInExport) && strcmp(s, "name") == 0) {
					ignoreKeyword = true;
				} else if (!(curLineState & stateInProperty) &&
					(strcmp(s, "read") == 0 || strcmp(s, "write") == 0 ||
					 strcmp(s, "default") == 0 || strcmp(s, "nodefault") == 0 ||
					 strcmp(s, "stored") == 0 || strcmp(s, "implements") == 0 ||
					 strcmp(s, "readonly") == 0 || strcmp(s, "writeonly") == 0 ||
					 strcmp(s, "add") == 0 || strcmp(s, "remove") == 0)) {
					ignoreKeyword = true;
				}
			}
			if (!ignoreKeyword) {
				sc.ChangeState(SCE_PAS_WORD);
			}
		}
	} else if (curLineState & stateInAsm) {
		sc.ChangeState(SCE_PAS_ASM);
	}
	sc.SetState(SCE_PAS_DEFAULT);
}

void LexerPascal::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument *pAccess) {
	LexAccessor styler(pAccess);

	const CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	const CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);
	const CharacterSet setNumber(CharacterSet::setDigits, ".-+eE");
	const CharacterSet setHexNumber(CharacterSet::setDigits, "abcdefABCDEF");
	const CharacterSet setOperator(CharacterSet::setNone, "#$&'()*+,-./:;<=>@[]^{}");

	int curLineState = 0;

	StyleContext sc(startPos, length, initStyle, styler);
	if (sc.currentLine > 0) {
		curLineState = styler.GetLineState(sc.currentLine - 1);
	}

	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_PAS_NUMBER:
				if (!setNumber.Contains(sc.ch) || (sc.ch == '.' && sc.chNext == '.')) {
					sc.SetState(SCE_PAS_DEFAULT);
				} else if (sc.ch == '-' || sc.ch == '+') {
					if (sc.chPrev != 'E' && sc.chPrev != 'e') {
						sc.SetState(SCE_PAS_DEFAULT);
					}
				}
				break;
			case SCE_PAS_IDENTIFIER:
				if (!setWord.Contains(sc.ch)) {
					ClassifyPascalWord(keywords, sc, curLineState, options.bSmartHighlighting);
				}
				break;
			case SCE_PAS_HEXNUMBER:
				if (!setHexNumber.Contains(sc.ch)) {
					sc.SetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_COMMENT:
			case SCE_PAS_PREPROCESSOR:
				if (sc.ch == '}') {
					sc.ForwardSetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_COMMENT2:
			case SCE_PAS_PREPROCESSOR2:
				if (sc.Match('*', ')')) {
					sc.Forward();
					sc.ForwardSetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_PAS_STRINGEOL);
				} else if (sc.ch == '\'' && sc.chNext == '\'') {
					sc.Forward();
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_CHARACTER:
				if (!setHexNumber.Contains(sc.ch) && sc.ch != '$') {
					sc.SetState(SCE_PAS_DEFAULT);
				}
				break;
			case SCE_PAS_OPERATOR:
				if (options.bSmartHighlighting && sc.chPrev == ';') {
					curLineState &= ~(stateInProperty | stateInExport);
				}
				sc.SetState(SCE_PAS_DEFAULT);
				break;
			case SCE_PAS_ASM:
				sc.SetState(SCE_PAS_DEFAULT);
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_PAS_DEFAULT) {
			if (IsADigit(sc.ch) && !(curLineState & stateInAsm)) {
				sc.SetState(SCE_PAS_NUMBER);
			} else if (setWordStart.Contains(sc.ch)) {
				sc.SetState(SCE_PAS_IDENTIFIER);
			} else if (sc.ch == '$' && !(curLineState & stateInAsm)) {
				sc.SetState(SCE_PAS_HEXNUMBER);
			} else if (sc.Match('{', '$')) {
				sc.SetState(SCE_PAS_PREPROCESSOR);
			} else if (sc.ch == '{') {
				sc.SetState(SCE_PAS_COMMENT);
			} else if (sc.Match("(*$")) {
				sc.SetState(SCE_PAS_PREPROCESSOR2);
			} else if (sc.Match('(', '*')) {
				sc.SetState(SCE_PAS_COMMENT2);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_PAS_COMMENTLINE);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_PAS_STRING);
			} else if (sc.ch == '#') {
				sc.SetState(SCE_PAS_CHARACTER);
			} else if (setOperator.Contains(sc.ch) && !(curLineState & stateInAsm)) {
				sc.SetState(SCE_PAS_OPERATOR);
			} else if (curLineState & stateInAsm) {
				sc.SetState(SCE_PAS_ASM);
			}
		}

		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			styler.SetLineState(sc.currentLine, curLineState);
		}
	}

	if (sc.state == SCE_PAS_IDENTIFIER && setWord.Contains(sc.chPrev)) {
		ClassifyPascalWord(keywords, sc, curLineState, options.bSmartHighlighting);
	}

	sc.Complete();
}

constexpr bool IsStreamCommentStyle(int style) noexcept {
	return style == SCE_PAS_COMMENT || style == SCE_PAS_COMMENT2;
}

bool IsCommentLine(Sci_Position line, LexAccessor &styler) {
	const Sci_Position pos = styler.LineStart(line);
	const Sci_Position eolPos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eolPos; i++) {
		const char ch = styler[i];
		const char chNext = styler.SafeGetCharAt(i + 1);
		const int style = styler.StyleIndexAt(i);
		if (ch == '/' && chNext == '/' && style == SCE_PAS_COMMENTLINE) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}

constexpr unsigned int GetFoldInPreprocessorLevelFlag(int lineFoldStateCurrent) noexcept {
	return lineFoldStateCurrent & stateFoldInPreprocessorLevelMask;
}

void SetFoldInPreprocessorLevelFlag(int &lineFoldStateCurrent, unsigned int nestLevel) noexcept {
	lineFoldStateCurrent &= ~stateFoldInPreprocessorLevelMask;
	lineFoldStateCurrent |= nestLevel & stateFoldInPreprocessorLevelMask;
}

void ClassifyPascalPreprocessorFoldPoint(int &levelCurrent, int &lineFoldStateCurrent,
		Sci_Position startPos, LexAccessor &styler) {
	const CharacterSet setWord(CharacterSet::setAlpha);

	char s[11];	// Size of the longest possible keyword + one additional character + null
	GetForwardRangeLowered(startPos, setWord, styler, s, sizeof(s));

	unsigned int nestLevel = GetFoldInPreprocessorLevelFlag(lineFoldStateCurrent);

	if (strcmp(s, "if") == 0 ||
		strcmp(s, "ifdef") == 0 ||
		strcmp(s, "ifndef") == 0 ||
		strcmp(s, "ifopt") == 0 ||
		strcmp(s, "region") == 0) {
		nestLevel++;
		SetFoldInPreprocessorLevelFlag(lineFoldStateCurrent, nestLevel);
		lineFoldStateCurrent |= stateFoldInPreprocessor;
		levelCurrent++;
	} else if (strcmp(s, "endif") == 0 ||
		strcmp(s, "ifend") == 0 ||
		strcmp(s, "endregion") == 0) {
		nestLevel--;
		SetFoldInPreprocessorLevelFlag(lineFoldStateCurrent, nestLevel);
		if (nestLevel == 0) {
			lineFoldStateCurrent &= ~stateFoldInPreprocessor;
		}
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}

Sci_Position SkipWhiteSpace(Sci_Position currentPos, Sci_Position endPos,
		LexAccessor &styler, bool includeChars = false) {
	const CharacterSet setWord(CharacterSet::setAlphaNum, "_");
	Sci_Position j = currentPos + 1;
	char ch = styler.SafeGetCharAt(j);
	while ((j < endPos) && (IsASpaceOrTab(ch) || ch == '\r' || ch == '\n' ||
		IsStreamCommentStyle(styler.StyleAt(j)) || (includeChars && setWord.Contains(ch)))) {
		j++;
		ch = styler.SafeGetCharAt(j);
	}
	return j;
}

void ClassifyPascalWordFoldPoint(int &levelCurrent, int &lineFoldStateCurrent,
		Sci_Position startPos, Sci_Position endPos,
		Sci_Position lastStart, Sci_Position currentPos, LexAccessor &styler) {
	char s[100];
	GetRangeLowered(lastStart, currentPos, styler, s, sizeof(s));

	if (strcmp(s, "record") == 0) {
		lineFoldStateCurrent |= stateFoldInRecord;
		levelCurrent++;
	} else if (strcmp(s, "begin") == 0 ||
		strcmp(s, "asm") == 0 ||
		strcmp(s, "try") == 0 ||
		(strcmp(s, "case") == 0 && !(lineFoldStateCurrent & stateFoldInRecord))) {
		levelCurrent++;
	} else if (strcmp(s, "class") == 0 || strcmp(s, "object") == 0) {
		// "class" & "object" keywords require special handling...
		bool ignoreKeyword = false;
		Sci_Position j = SkipWhiteSpace(currentPos, endPos, styler);
		if (j < endPos) {
			const CharacterSet setWordStart(CharacterSet::setAlpha, "_");
			const CharacterSet setWord(CharacterSet::setAlphaNum, "_");

			if (styler.SafeGetCharAt(j) == ';') {
				// Handle forward class declarations ("type TMyClass = class;")
				// and object method declarations ("TNotifyEvent = procedure(Sender: TObject) of object;")
				ignoreKeyword = true;
			} else if (strcmp(s, "class") == 0) {
				// "class" keyword has a few more special cases...
				if (styler.SafeGetCharAt(j) == '(') {
					// Handle simplified complete class declarations ("type TMyClass = class(TObject);")
					j = SkipWhiteSpace(j, endPos, styler, true);
					if (j < endPos && styler.SafeGetCharAt(j) == ')') {
						j = SkipWhiteSpace(j, endPos, styler);
						if (j < endPos && styler.SafeGetCharAt(j) == ';') {
							ignoreKeyword = true;
						}
					}
				} else if (setWordStart.Contains(styler.SafeGetCharAt(j))) {
					char s2[11];	// Size of the longest possible keyword + one additional character + null
					GetForwardRangeLowered(j, setWord, styler, s2, sizeof(s2));

					if (strcmp(s2, "procedure") == 0 ||
						strcmp(s2, "function") == 0 ||
						strcmp(s2, "of") == 0 ||
						strcmp(s2, "var") == 0 ||
						strcmp(s2, "property") == 0 ||
						strcmp(s2, "operator") == 0) {
						ignoreKeyword = true;
					}
				}
			}
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "interface") == 0) {
		// "interface" keyword requires special handling...
		bool ignoreKeyword = true;
		Sci_Position j = lastStart - 1;
		char ch = styler.SafeGetCharAt(j);
		while ((j >= startPos) && (IsASpaceOrTab(ch) || ch == '\r' || ch == '\n' ||
			IsStreamCommentStyle(styler.StyleAt(j)))) {
			j--;
			ch = styler.SafeGetCharAt(j);
		}
		if (j >= startPos && styler.SafeGetCharAt(j) == '=') {
			ignoreKeyword = false;
		}
		if (!ignoreKeyword) {
			const Sci_Position k = SkipWhiteSpace(currentPos, endPos, styler);
			if (k < endPos && styler.SafeGetCharAt(k) == ';') {
				// Handle forward interface declarations ("type IMyInterface = interface;")
				ignoreKeyword = true;
			}
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "dispinterface") == 0) {
		// "dispinterface" keyword requires special handling...
		bool ignoreKeyword = false;
		const Sci_Position j = SkipWhiteSpace(currentPos, endPos, styler);
		if (j < endPos && styler.SafeGetCharAt(j) == ';') {
			// Handle forward dispinterface declarations ("type IMyInterface = dispinterface;")
			ignoreKeyword = true;
		}
		if (!ignoreKeyword) {
			levelCurrent++;
		}
	} else if (strcmp(s, "end") == 0) {
		lineFoldStateCurrent &= ~stateFoldInRecord;
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}

void LexerPascal::Fold(Sci_PositionU startPos_, Sci_Position length, int initStyle, Scintilla::IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_Position startPos = startPos_;
	const Sci_Position endPos = startPos + length;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	// Backtrack to previous line in case need to fix its fold status
	if (lineCurrent > 0) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
		initStyle = (startPos > 0) ? styler.StyleIndexAt(startPos - 1) : 0;
	}

	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int lineFoldStateCurrent = lineCurrent > 0 ? styler.GetLineState(lineCurrent - 1) & stateFoldMaskAll : 0;
	char chNext = styler[startPos];
	int styleNext = styler.StyleIndexAt(startPos);
	int style = initStyle;

	int visibleChars = 0;

	Sci_Position lastStart = 0;
	const CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);

	for (Sci_Position i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleIndexAt(i + 1);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (options.foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
			    && IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
			         && !IsCommentLine(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (options.foldPreprocessor) {
			if (style == SCE_PAS_PREPROCESSOR && ch == '{' && chNext == '$') {
				ClassifyPascalPreprocessorFoldPoint(levelCurrent, lineFoldStateCurrent, i + 2, styler);
			} else if (style == SCE_PAS_PREPROCESSOR2 && ch == '(' && chNext == '*'
			           && styler.SafeGetCharAt(i + 2) == '$') {
				ClassifyPascalPreprocessorFoldPoint(levelCurrent, lineFoldStateCurrent, i + 3, styler);
			}
		}

		if (stylePrev != SCE_PAS_WORD && style == SCE_PAS_WORD)
		{
			// Store last word start point.
			lastStart = i;
		}
		if (stylePrev == SCE_PAS_WORD && !(lineFoldStateCurrent & stateFoldInPreprocessor)) {
			if(setWord.Contains(ch) && !setWord.Contains(chNext)) {
				ClassifyPascalWordFoldPoint(levelCurrent, lineFoldStateCurrent, startPos, endPos, lastStart, i, styler);
			}
		}

		if (!IsASpace(ch))
			visibleChars++;

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			const int newLineState = (styler.GetLineState(lineCurrent) & ~stateFoldMaskAll) | lineFoldStateCurrent;
			styler.SetLineState(lineCurrent, newLineState);
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
	}

	// If we didn't reach the EOL in previous loop, store line level and whitespace information.
	// The rest will be filled in later...
	int lev = levelPrev;
	if (visibleChars == 0 && options.foldCompact)
		lev |= SC_FOLDLEVELWHITEFLAG;
	styler.SetLevel(lineCurrent, lev);
}

}

extern const LexerModule lmPascal(SCLEX_PASCAL, LexerPascal::LexerFactoryPascal, "pascal", pascalWordListDesc);
