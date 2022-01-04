// Scintilla source code edit control
/** @file LexHollywood.cxx
 ** Lexer for Hollywood
 ** Written by Andreas Falkenhahn, based on the BlitzBasic/PureBasic/Lua lexers
 ** Thanks to Nicholai Benalal
 ** For more information on Hollywood, see http://www.hollywood-mal.com/
 ** Mail me (andreas <at> airsoftsoftwair <dot> de) for any bugs.
 ** This code is subject to the same license terms as the rest of the Scintilla project:
 ** The License.txt file describes the conditions under which this software may be distributed. 
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

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

using namespace Scintilla;
using namespace Lexilla;

/* Bits:
 * 1  - whitespace
 * 2  - operator
 * 4  - identifier
 * 8  - decimal digit
 * 16 - hex digit
 * 32 - bin digit
 * 64 - letter
 */
static int character_classification[128] =
{
	0, // NUL ($0)
	0, // SOH ($1)
	0, // STX ($2)
	0, // ETX ($3)
	0, // EOT ($4)
	0, // ENQ ($5)
	0, // ACK ($6)
	0, // BEL ($7)
	0, // BS ($8)
	1, // HT ($9)
	1, // LF ($A)
	0, // VT ($B)
	0, // FF ($C)
	1, // CR ($D)
	0, // SO ($E)
	0, // SI ($F)
	0, // DLE ($10)
	0, // DC1 ($11)
	0, // DC2 ($12)
	0, // DC3 ($13)
	0, // DC4 ($14)
	0, // NAK ($15)
	0, // SYN ($16)
	0, // ETB ($17)
	0, // CAN ($18)
	0, // EM ($19)
	0, // SUB ($1A)
	0, // ESC ($1B)
	0, // FS ($1C)
	0, // GS ($1D)
	0, // RS ($1E)
	0, // US ($1F)
	1, // space ($20)
	4, // ! ($21)
	0, // " ($22)
	0, // # ($23)
	4, // $ ($24)
	2, // % ($25)
	2, // & ($26)
	2, // ' ($27)
	2, // ( ($28)
	2, // ) ($29)
	2, // * ($2A)
	2, // + ($2B)
	2, // , ($2C)
	2, // - ($2D)
	// NB: we treat "." as an identifier although it is also an operator and a decimal digit
	// the reason why we treat it as an identifier is to support syntax highlighting for
	// plugin commands which always use a "." in their names, e.g. pdf.OpenDocument();
	// we handle the decimal digit case manually below so that 3.1415 and .123 is styled correctly
	// the collateral damage of treating "." as an identifier is that "." is never styled
	// SCE_HOLLYWOOD_OPERATOR
	4, // . ($2E) 
	2, // / ($2F)
	28, // 0 ($30)
	28, // 1 ($31)
	28, // 2 ($32)
	28, // 3 ($33)
	28, // 4 ($34)
	28, // 5 ($35)
	28, // 6 ($36)
	28, // 7 ($37)
	28, // 8 ($38)
	28, // 9 ($39)
	2, // : ($3A)
	2, // ; ($3B)
	2, // < ($3C)
	2, // = ($3D)
	2, // > ($3E)
	2, // ? ($3F)
	0, // @ ($40)
	84, // A ($41)
	84, // B ($42)
	84, // C ($43)
	84, // D ($44)
	84, // E ($45)
	84, // F ($46)
	68, // G ($47)
	68, // H ($48)
	68, // I ($49)
	68, // J ($4A)
	68, // K ($4B)
	68, // L ($4C)
	68, // M ($4D)
	68, // N ($4E)
	68, // O ($4F)
	68, // P ($50)
	68, // Q ($51)
	68, // R ($52)
	68, // S ($53)
	68, // T ($54)
	68, // U ($55)
	68, // V ($56)
	68, // W ($57)
	68, // X ($58)
	68, // Y ($59)
	68, // Z ($5A)
	2, // [ ($5B)
	2, // \ ($5C)
	2, // ] ($5D)
	2, // ^ ($5E)
	68, // _ ($5F)
	2, // ` ($60)
	84, // a ($61)
	84, // b ($62)
	84, // c ($63)
	84, // d ($64)
	84, // e ($65)
	84, // f ($66)
	68, // g ($67)
	68, // h ($68)
	68, // i ($69)
	68, // j ($6A)
	68, // k ($6B)
	68, // l ($6C)
	68, // m ($6D)
	68, // n ($6E)
	68, // o ($6F)
	68, // p ($70)
	68, // q ($71)
	68, // r ($72)
	68, // s ($73)
	68, // t ($74)
	68, // u ($75)
	68, // v ($76)
	68, // w ($77)
	68, // x ($78)
	68, // y ($79)
	68, // z ($7A)
	2, // { ($7B)
	2, // | ($7C)
	2, // } ($7D)
	2, // ~ ($7E)
	0, // &#127; ($7F)
};

static bool IsSpace(int c) {
	return c < 128 && (character_classification[c] & 1);
}

static bool IsOperator(int c) {
	return c < 128 && (character_classification[c] & 2);
}

static bool IsIdentifier(int c) {
	return c < 128 && (character_classification[c] & 4);
}

static bool IsDigit(int c) {
	return c < 128 && (character_classification[c] & 8);
}

static bool IsHexDigit(int c) {
	return c < 128 && (character_classification[c] & 16);
}

static int LowerCase(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 'a' + c - 'A';
	return c;
}

static int CheckHollywoodFoldPoint(char const *token) {
	if (!strcmp(token, "function")) {
		return 1;
	}
	if (!strcmp(token, "endfunction")) {
		return -1;
	}
	return 0;
}

// An individual named option for use in an OptionSet

// Options used for LexerHollywood
struct OptionsHollywood {
	bool fold;
	bool foldCompact;
	OptionsHollywood() {
		fold = false;
		foldCompact = false;
	}
};

static const char * const hollywoodWordListDesc[] = {
	"Hollywood keywords",
	"Hollywood standard API functions",
	"Hollywood plugin API functions",
	"Hollywood plugin methods",
	0
};

struct OptionSetHollywood : public OptionSet<OptionsHollywood> {
	OptionSetHollywood(const char * const wordListDescriptions[]) {
		DefineProperty("fold", &OptionsHollywood::fold);
		DefineProperty("fold.compact", &OptionsHollywood::foldCompact);
		DefineWordListSets(wordListDescriptions);
	}
};

class LexerHollywood : public DefaultLexer {
	int (*CheckFoldPoint)(char const *);
	WordList keywordlists[4];	
	OptionsHollywood options;
	OptionSetHollywood osHollywood;
public:
	LexerHollywood(int (*CheckFoldPoint_)(char const *), const char * const wordListDescriptions[]) :
						 DefaultLexer("hollywood", SCLEX_HOLLYWOOD),
						 CheckFoldPoint(CheckFoldPoint_),
						 osHollywood(wordListDescriptions) {
	}
	virtual ~LexerHollywood() {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osHollywood.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osHollywood.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osHollywood.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char* key) override {
		return osHollywood.PropertyGet(key);
	}
	const char * SCI_METHOD DescribeWordListSets() override {
		return osHollywood.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}
	static ILexer5 *LexerFactoryHollywood() {
		return new LexerHollywood(CheckHollywoodFoldPoint, hollywoodWordListDesc);
	}
};

Sci_Position SCI_METHOD LexerHollywood::PropertySet(const char *key, const char *val) {
	if (osHollywood.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerHollywood::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywordlists[0];
		break;
	case 1:
		wordListN = &keywordlists[1];
		break;
	case 2:
		wordListN = &keywordlists[2];
		break;
	case 3:
		wordListN = &keywordlists[3];
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

void SCI_METHOD LexerHollywood::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	styler.StartAt(startPos);
	bool inString = false;
	
	StyleContext sc(startPos, length, initStyle, styler);

	// Can't use sc.More() here else we miss the last character
	for (; ; sc.Forward())
	 {
	 	if (sc.atLineStart) inString = false;
	 		
	 	if (sc.ch == '\"' && sc.chPrev != '\\') inString = !inString;
	 		
		if (sc.state == SCE_HOLLYWOOD_IDENTIFIER) {
			if (!IsIdentifier(sc.ch)) {				
				char s[100];
				int kstates[4] = {
					SCE_HOLLYWOOD_KEYWORD,
					SCE_HOLLYWOOD_STDAPI,
					SCE_HOLLYWOOD_PLUGINAPI,
					SCE_HOLLYWOOD_PLUGINMETHOD,
				};
				sc.GetCurrentLowered(s, sizeof(s));
				for (int i = 0; i < 4; i++) {
					if (keywordlists[i].InList(s)) {
						sc.ChangeState(kstates[i]);
					}
				}
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);				
			}
		} else if (sc.state == SCE_HOLLYWOOD_OPERATOR) {
			
			// always reset to default on operators because otherwise
			// comments won't be recognized in sequences like "+/* Hello*/"
			// --> "+/*" would be recognized as a sequence of operators
			
			// if (!IsOperator(sc.ch)) sc.SetState(SCE_HOLLYWOOD_DEFAULT);
			sc.SetState(SCE_HOLLYWOOD_DEFAULT);
			
		} else if (sc.state == SCE_HOLLYWOOD_PREPROCESSOR) {
			if (!IsIdentifier(sc.ch))
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
		} else if (sc.state == SCE_HOLLYWOOD_CONSTANT) {
			if (!IsIdentifier(sc.ch))
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
		} else if (sc.state == SCE_HOLLYWOOD_NUMBER) {
			if (!IsDigit(sc.ch) && sc.ch != '.')
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
		} else if (sc.state == SCE_HOLLYWOOD_HEXNUMBER) {
			if (!IsHexDigit(sc.ch))
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
		} else if (sc.state == SCE_HOLLYWOOD_STRING) {
			if (sc.ch == '"') {
				sc.ForwardSetState(SCE_HOLLYWOOD_DEFAULT);
			}
			if (sc.atLineEnd) {
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
			}
		} else if (sc.state == SCE_HOLLYWOOD_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_HOLLYWOOD_DEFAULT);
			}
		} else if (sc.state == SCE_HOLLYWOOD_COMMENTBLOCK) {
			if (sc.Match("*/") && !inString) {
				sc.Forward();
				sc.ForwardSetState(SCE_HOLLYWOOD_DEFAULT);
			}
		} else if (sc.state == SCE_HOLLYWOOD_STRINGBLOCK) {
			if (sc.Match("]]") && !inString) {
				sc.Forward();
				sc.ForwardSetState(SCE_HOLLYWOOD_DEFAULT);
			}			
		}

		if (sc.state == SCE_HOLLYWOOD_DEFAULT) {
			if (sc.Match(';')) {
				sc.SetState(SCE_HOLLYWOOD_COMMENT);
			} else if (sc.Match("/*")) {
				sc.SetState(SCE_HOLLYWOOD_COMMENTBLOCK);
				sc.Forward();
			} else if (sc.Match("[[")) {
				sc.SetState(SCE_HOLLYWOOD_STRINGBLOCK);
				sc.Forward();				
			} else if (sc.Match('"')) {
				sc.SetState(SCE_HOLLYWOOD_STRING);
			} else if (sc.Match('$')) { 
				sc.SetState(SCE_HOLLYWOOD_HEXNUMBER);
			} else if (sc.Match("0x") || sc.Match("0X")) {  // must be before IsDigit() because of 0x
				sc.SetState(SCE_HOLLYWOOD_HEXNUMBER);
				sc.Forward();
			} else if (sc.ch == '.' && (sc.chNext >= '0' && sc.chNext <= '9')) {  // ".1234" style numbers
				sc.SetState(SCE_HOLLYWOOD_NUMBER);
				sc.Forward();	
			} else if (IsDigit(sc.ch)) {
				sc.SetState(SCE_HOLLYWOOD_NUMBER);
			} else if (sc.Match('#')) {
				sc.SetState(SCE_HOLLYWOOD_CONSTANT);
			} else if (sc.Match('@')) {
				sc.SetState(SCE_HOLLYWOOD_PREPROCESSOR);	
			} else if (IsOperator(sc.ch)) {
				sc.SetState(SCE_HOLLYWOOD_OPERATOR);
			} else if (IsIdentifier(sc.ch)) {
				sc.SetState(SCE_HOLLYWOOD_IDENTIFIER);
			}
		}

		if (!sc.More())
			break;
	}
	sc.Complete();
}

void SCI_METHOD LexerHollywood::Fold(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);
	
	Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int done = 0;
	char word[256];
	int wordlen = 0;
		
	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (!done) {
			if (wordlen) { // are we scanning a token already?
				word[wordlen] = static_cast<char>(LowerCase(ch));
				if (!IsIdentifier(ch)) { // done with token
					word[wordlen] = '\0';
					levelCurrent += CheckFoldPoint(word);
					done = 1;
				} else if (wordlen < 255) {
					wordlen++;
				}
			} else { // start scanning at first non-whitespace character
				if (!IsSpace(ch)) {
					if (style != SCE_HOLLYWOOD_COMMENTBLOCK && IsIdentifier(ch)) {
						word[0] = static_cast<char>(LowerCase(ch));
						wordlen = 1;
					} else // done with this line
						done = 1;
				}
			}
		}		

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && options.foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
			done = 0;
			wordlen = 0;			
		}
		if (!IsSpace(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);	
}

LexerModule lmHollywood(SCLEX_HOLLYWOOD, LexerHollywood::LexerFactoryHollywood, "hollywood", hollywoodWordListDesc);
