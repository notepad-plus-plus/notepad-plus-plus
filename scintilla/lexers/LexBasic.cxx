// Scintilla source code edit control
/** @file LexBasic.cxx
 ** Lexer for BlitzBasic and PureBasic.
 ** Converted to lexer object and added further folding features/properties by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// This tries to be a unified Lexer/Folder for all the BlitzBasic/BlitzMax/PurBasic basics
// and derivatives. Once they diverge enough, might want to split it into multiple
// lexers for more code clearity.
//
// Mail me (elias <at> users <dot> sf <dot> net) for any bugs.

// Folding only works for simple things like functions or types.

// You may want to have a look at my ctags lexer as well, if you additionally to coloring
// and folding need to extract things like label tags in your editor.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <map>

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
		0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		1,  2,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  10, 2,
	 60, 60, 28, 28, 28, 28, 28, 28, 28, 28,  2,  2,  2,  2,  2,  2,
		2, 84, 84, 84, 84, 84, 84, 68, 68, 68, 68, 68, 68, 68, 68, 68,
	 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68,  2,  2,  2,  2, 68,
		2, 84, 84, 84, 84, 84, 84, 68, 68, 68, 68, 68, 68, 68, 68, 68,
	 68, 68, 68, 68, 68, 68, 68, 68, 68, 68, 68,  2,  2,  2,  2,  0
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

static bool IsBinDigit(int c) {
	return c < 128 && (character_classification[c] & 32);
}

static bool IsLetter(int c) {
	return c < 128 && (character_classification[c] & 64);
}

static int LowerCase(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 'a' + c - 'A';
	return c;
}

static int CheckBlitzFoldPoint(char const *token, int &level) {
	if (!strcmp(token, "function") ||
		!strcmp(token, "type")) {
		level |= SC_FOLDLEVELHEADERFLAG;
		return 1;
	}
	if (!strcmp(token, "end function") ||
		!strcmp(token, "end type")) {
		return -1;
	}
	return 0;
}

static int CheckPureFoldPoint(char const *token, int &level) {
	if (!strcmp(token, "procedure") ||
		!strcmp(token, "enumeration") ||
		!strcmp(token, "interface") ||
		!strcmp(token, "structure")) {
		level |= SC_FOLDLEVELHEADERFLAG;
		return 1;
	}
	if (!strcmp(token, "endprocedure") ||
		!strcmp(token, "endenumeration") ||
		!strcmp(token, "endinterface") ||
		!strcmp(token, "endstructure")) {
		return -1;
	}
	return 0;
}

static int CheckFreeFoldPoint(char const *token, int &level) {
	if (!strcmp(token, "function") ||
		!strcmp(token, "sub") ||
		!strcmp(token, "enum") ||
		!strcmp(token, "type") ||
		!strcmp(token, "union") ||
		!strcmp(token, "property") ||
		!strcmp(token, "destructor") ||
		!strcmp(token, "constructor")) {
		level |= SC_FOLDLEVELHEADERFLAG;
		return 1;
	}
	if (!strcmp(token, "end function") ||
		!strcmp(token, "end sub") ||
		!strcmp(token, "end enum") ||
		!strcmp(token, "end type") ||
		!strcmp(token, "end union") ||
		!strcmp(token, "end property") ||
		!strcmp(token, "end destructor") ||
		!strcmp(token, "end constructor")) {
		return -1;
	}
	return 0;
}

// An individual named option for use in an OptionSet

// Options used for LexerBasic
struct OptionsBasic {
	bool fold;
	bool foldSyntaxBased;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	OptionsBasic() {
		fold = false;
		foldSyntaxBased = true;
		foldCommentExplicit = false;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = true;
	}
};

static const char * const blitzbasicWordListDesc[] = {
	"BlitzBasic Keywords",
	"user1",
	"user2",
	"user3",
	0
};

static const char * const purebasicWordListDesc[] = {
	"PureBasic Keywords",
	"PureBasic PreProcessor Keywords",
	"user defined 1",
	"user defined 2",
	0
};

static const char * const freebasicWordListDesc[] = {
	"FreeBasic Keywords",
	"FreeBasic PreProcessor Keywords",
	"user defined 1",
	"user defined 2",
	0
};

struct OptionSetBasic : public OptionSet<OptionsBasic> {
	OptionSetBasic(const char * const wordListDescriptions[]) {
		DefineProperty("fold", &OptionsBasic::fold);

		DefineProperty("fold.basic.syntax.based", &OptionsBasic::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.basic.comment.explicit", &OptionsBasic::foldCommentExplicit,
			"This option enables folding explicit fold points when using the Basic lexer. "
			"Explicit fold points allows adding extra folding by placing a ;{ (BB/PB) or '{ (FB) comment at the start "
			"and a ;} (BB/PB) or '} (FB) at the end of a section that should be folded.");

		DefineProperty("fold.basic.explicit.start", &OptionsBasic::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard ;{ (BB/PB) or '{ (FB).");

		DefineProperty("fold.basic.explicit.end", &OptionsBasic::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard ;} (BB/PB) or '} (FB).");

		DefineProperty("fold.basic.explicit.anywhere", &OptionsBasic::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsBasic::foldCompact);

		DefineWordListSets(wordListDescriptions);
	}
};

class LexerBasic : public DefaultLexer {
	char comment_char;
	int (*CheckFoldPoint)(char const *, int &);
	WordList keywordlists[4];
	OptionsBasic options;
	OptionSetBasic osBasic;
public:
	LexerBasic(char comment_char_, int (*CheckFoldPoint_)(char const *, int &), const char * const wordListDescriptions[]) :
						 comment_char(comment_char_),
						 CheckFoldPoint(CheckFoldPoint_),
						 osBasic(wordListDescriptions) {
	}
	virtual ~LexerBasic() {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease4;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osBasic.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osBasic.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osBasic.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD DescribeWordListSets() override {
		return osBasic.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}
	static ILexer4 *LexerFactoryBlitzBasic() {
		return new LexerBasic(';', CheckBlitzFoldPoint, blitzbasicWordListDesc);
	}
	static ILexer4 *LexerFactoryPureBasic() {
		return new LexerBasic(';', CheckPureFoldPoint, purebasicWordListDesc);
	}
	static ILexer4 *LexerFactoryFreeBasic() {
		return new LexerBasic('\'', CheckFreeFoldPoint, freebasicWordListDesc );
	}
};

Sci_Position SCI_METHOD LexerBasic::PropertySet(const char *key, const char *val) {
	if (osBasic.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerBasic::WordListSet(int n, const char *wl) {
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

void SCI_METHOD LexerBasic::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	bool wasfirst = true, isfirst = true; // true if first token in a line
	styler.StartAt(startPos);
	int styleBeforeKeyword = SCE_B_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	// Can't use sc.More() here else we miss the last character
	for (; ; sc.Forward()) {
		if (sc.state == SCE_B_IDENTIFIER) {
			if (!IsIdentifier(sc.ch)) {
				// Labels
				if (wasfirst && sc.Match(':')) {
					sc.ChangeState(SCE_B_LABEL);
					sc.ForwardSetState(SCE_B_DEFAULT);
				} else {
					char s[100];
					int kstates[4] = {
						SCE_B_KEYWORD,
						SCE_B_KEYWORD2,
						SCE_B_KEYWORD3,
						SCE_B_KEYWORD4,
					};
					sc.GetCurrentLowered(s, sizeof(s));
					for (int i = 0; i < 4; i++) {
						if (keywordlists[i].InList(s)) {
							sc.ChangeState(kstates[i]);
						}
					}
					// Types, must set them as operator else they will be
					// matched as number/constant
					if (sc.Match('.') || sc.Match('$') || sc.Match('%') ||
						sc.Match('#')) {
						sc.SetState(SCE_B_OPERATOR);
					} else {
						sc.SetState(SCE_B_DEFAULT);
					}
				}
			}
		} else if (sc.state == SCE_B_OPERATOR) {
			if (!IsOperator(sc.ch) || sc.Match('#'))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_LABEL) {
			if (!IsIdentifier(sc.ch))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_CONSTANT) {
			if (!IsIdentifier(sc.ch))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_NUMBER) {
			if (!IsDigit(sc.ch))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_HEXNUMBER) {
			if (!IsHexDigit(sc.ch))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_BINNUMBER) {
			if (!IsBinDigit(sc.ch))
				sc.SetState(SCE_B_DEFAULT);
		} else if (sc.state == SCE_B_STRING) {
			if (sc.ch == '"') {
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
			if (sc.atLineEnd) {
				sc.ChangeState(SCE_B_ERROR);
				sc.SetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_COMMENT || sc.state == SCE_B_PREPROCESSOR) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_DOCLINE) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_B_DEFAULT);
			} else if (sc.ch == '\\' || sc.ch == '@') {
				if (IsLetter(sc.chNext) && sc.chPrev != '\\') {
					styleBeforeKeyword = sc.state;
					sc.SetState(SCE_B_DOCKEYWORD);
				};
			}
		} else if (sc.state == SCE_B_DOCKEYWORD) {
			if (IsSpace(sc.ch)) {
				sc.SetState(styleBeforeKeyword);
			}	else if (sc.atLineEnd && styleBeforeKeyword == SCE_B_DOCLINE) {
				sc.SetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_COMMENTBLOCK) {
			if (sc.Match("\'/")) {
				sc.Forward();
				sc.ForwardSetState(SCE_B_DEFAULT);
			}
		} else if (sc.state == SCE_B_DOCBLOCK) {
			if (sc.Match("\'/")) {
				sc.Forward();
				sc.ForwardSetState(SCE_B_DEFAULT);
			} else if (sc.ch == '\\' || sc.ch == '@') {
				if (IsLetter(sc.chNext) && sc.chPrev != '\\') {
					styleBeforeKeyword = sc.state;
					sc.SetState(SCE_B_DOCKEYWORD);
				};
			}
		}

		if (sc.atLineStart)
			isfirst = true;

		if (sc.state == SCE_B_DEFAULT || sc.state == SCE_B_ERROR) {
			if (isfirst && sc.Match('.') && comment_char != '\'') {
					sc.SetState(SCE_B_LABEL);
			} else if (isfirst && sc.Match('#')) {
				wasfirst = isfirst;
				sc.SetState(SCE_B_IDENTIFIER);
			} else if (sc.Match(comment_char)) {
				// Hack to make deprecated QBASIC '$Include show
				// up in freebasic with SCE_B_PREPROCESSOR.
				if (comment_char == '\'' && sc.Match(comment_char, '$'))
					sc.SetState(SCE_B_PREPROCESSOR);
				else if (sc.Match("\'*") || sc.Match("\'!")) {
					sc.SetState(SCE_B_DOCLINE);
				} else {
					sc.SetState(SCE_B_COMMENT);
				}
			} else if (sc.Match("/\'")) {
				if (sc.Match("/\'*") || sc.Match("/\'!")) {	// Support of gtk-doc/Doxygen doc. style
					sc.SetState(SCE_B_DOCBLOCK);
				} else {
					sc.SetState(SCE_B_COMMENTBLOCK);
				}
				sc.Forward();	// Eat the ' so it isn't used for the end of the comment
			} else if (sc.Match('"')) {
				sc.SetState(SCE_B_STRING);
			} else if (IsDigit(sc.ch)) {
				sc.SetState(SCE_B_NUMBER);
			} else if (sc.Match('$') || sc.Match("&h") || sc.Match("&H") || sc.Match("&o") || sc.Match("&O")) {
				sc.SetState(SCE_B_HEXNUMBER);
			} else if (sc.Match('%') || sc.Match("&b") || sc.Match("&B")) {
				sc.SetState(SCE_B_BINNUMBER);
			} else if (sc.Match('#')) {
				sc.SetState(SCE_B_CONSTANT);
			} else if (IsOperator(sc.ch)) {
				sc.SetState(SCE_B_OPERATOR);
			} else if (IsIdentifier(sc.ch)) {
				wasfirst = isfirst;
				sc.SetState(SCE_B_IDENTIFIER);
			} else if (!IsSpace(sc.ch)) {
				sc.SetState(SCE_B_ERROR);
			}
		}

		if (!IsSpace(sc.ch))
			isfirst = false;

		if (!sc.More())
			break;
	}
	sc.Complete();
}


void SCI_METHOD LexerBasic::Fold(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_Position line = styler.GetLine(startPos);
	int level = styler.LevelAt(line);
	int go = 0, done = 0;
	Sci_Position endPos = startPos + length;
	char word[256];
	int wordlen = 0;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	int cNext = styler[startPos];

	// Scan for tokens at the start of the line (they may include
	// whitespace, for tokens like "End Function"
	for (Sci_Position i = startPos; i < endPos; i++) {
		int c = cNext;
		cNext = styler.SafeGetCharAt(i + 1);
		bool atEOL = (c == '\r' && cNext != '\n') || (c == '\n');
		if (options.foldSyntaxBased && !done && !go) {
			if (wordlen) { // are we scanning a token already?
				word[wordlen] = static_cast<char>(LowerCase(c));
				if (!IsIdentifier(c)) { // done with token
					word[wordlen] = '\0';
					go = CheckFoldPoint(word, level);
					if (!go) {
						// Treat any whitespace as single blank, for
						// things like "End   Function".
						if (IsSpace(c) && IsIdentifier(word[wordlen - 1])) {
							word[wordlen] = ' ';
							if (wordlen < 255)
								wordlen++;
						}
						else // done with this line
							done = 1;
					}
				} else if (wordlen < 255) {
					wordlen++;
				}
			} else { // start scanning at first non-whitespace character
				if (!IsSpace(c)) {
					if (IsIdentifier(c)) {
						word[0] = static_cast<char>(LowerCase(c));
						wordlen = 1;
					} else // done with this line
						done = 1;
				}
			}
		}
		if (options.foldCommentExplicit && ((styler.StyleAt(i) == SCE_B_COMMENT) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
 					level |= SC_FOLDLEVELHEADERFLAG;
					go = 1;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
 					go = -1;
 				}
			} else {
				if (c == comment_char) {
					if (cNext == '{') {
						level |= SC_FOLDLEVELHEADERFLAG;
						go = 1;
					} else if (cNext == '}') {
						go = -1;
					}
				}
 			}
 		}
		if (atEOL) { // line end
			if (!done && wordlen == 0 && options.foldCompact) // line was only space
				level |= SC_FOLDLEVELWHITEFLAG;
			if (level != styler.LevelAt(line))
				styler.SetLevel(line, level);
			level += go;
			line++;
			// reset state
			wordlen = 0;
			level &= ~SC_FOLDLEVELHEADERFLAG;
			level &= ~SC_FOLDLEVELWHITEFLAG;
			go = 0;
			done = 0;
		}
	}
}

LexerModule lmBlitzBasic(SCLEX_BLITZBASIC, LexerBasic::LexerFactoryBlitzBasic, "blitzbasic", blitzbasicWordListDesc);

LexerModule lmPureBasic(SCLEX_PUREBASIC, LexerBasic::LexerFactoryPureBasic, "purebasic", purebasicWordListDesc);

LexerModule lmFreeBasic(SCLEX_FREEBASIC, LexerBasic::LexerFactoryFreeBasic, "freebasic", freebasicWordListDesc);
