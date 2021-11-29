/** @file LexD.cxx
 ** Lexer for D.
 **
 ** Copyright (c) 2006 by Waldemar Augustyn <waldemar@wdmsys.com>
 ** Converted to lexer object and added further folding features/properties by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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

/* Nested comments require keeping the value of the nesting level for every
   position in the document.  But since scintilla always styles line by line,
   we only need to store one value per line. The non-negative number indicates
   nesting level at the end of the line.
*/

// Underscore, letter, digit and universal alphas from C99 Appendix D.

static bool IsWordStart(int ch) {
	return (IsASCII(ch) && (isalpha(ch) || ch == '_')) || !IsASCII(ch);
}

static bool IsWord(int ch) {
	return (IsASCII(ch) && (isalnum(ch) || ch == '_')) || !IsASCII(ch);
}

static bool IsDoxygen(int ch) {
	if (IsASCII(ch) && islower(ch))
		return true;
	if (ch == '$' || ch == '@' || ch == '\\' ||
		ch == '&' || ch == '#' || ch == '<' || ch == '>' ||
		ch == '{' || ch == '}' || ch == '[' || ch == ']')
		return true;
	return false;
}

static bool IsStringSuffix(int ch) {
	return ch == 'c' || ch == 'w' || ch == 'd';
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_D_COMMENT ||
		style == SCE_D_COMMENTDOC ||
		style == SCE_D_COMMENTDOCKEYWORD ||
		style == SCE_D_COMMENTDOCKEYWORDERROR;
}

// An individual named option for use in an OptionSet

// Options used for LexerD
struct OptionsD {
	bool fold;
	bool foldSyntaxBased;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	int  foldAtElseInt;
	bool foldAtElse;
	OptionsD() {
		fold = false;
		foldSyntaxBased = true;
		foldComment = false;
		foldCommentMultiline = true;
		foldCommentExplicit = true;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = true;
		foldAtElseInt = -1;
		foldAtElse = false;
	}
};

static const char * const dWordLists[] = {
			"Primary keywords and identifiers",
			"Secondary keywords and identifiers",
			"Documentation comment keywords",
			"Type definitions and aliases",
			"Keywords 5",
			"Keywords 6",
			"Keywords 7",
			0,
		};

struct OptionSetD : public OptionSet<OptionsD> {
	OptionSetD() {
		DefineProperty("fold", &OptionsD::fold);

		DefineProperty("fold.d.syntax.based", &OptionsD::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.comment", &OptionsD::foldComment);

		DefineProperty("fold.d.comment.multiline", &OptionsD::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

		DefineProperty("fold.d.comment.explicit", &OptionsD::foldCommentExplicit,
			"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

		DefineProperty("fold.d.explicit.start", &OptionsD::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard //{.");

		DefineProperty("fold.d.explicit.end", &OptionsD::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard //}.");

		DefineProperty("fold.d.explicit.anywhere", &OptionsD::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsD::foldCompact);

		DefineProperty("lexer.d.fold.at.else", &OptionsD::foldAtElseInt,
			"This option enables D folding on a \"} else {\" line of an if statement.");

		DefineProperty("fold.at.else", &OptionsD::foldAtElse);

		DefineWordListSets(dWordLists);
	}
};

class LexerD : public DefaultLexer {
	bool caseSensitive;
	WordList keywords;
	WordList keywords2;
	WordList keywords3;
	WordList keywords4;
	WordList keywords5;
	WordList keywords6;
	WordList keywords7;
	OptionsD options;
	OptionSetD osD;
public:
	LexerD(bool caseSensitive_) :
		DefaultLexer("D", SCLEX_D),
		caseSensitive(caseSensitive_) {
	}
	virtual ~LexerD() {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osD.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osD.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osD.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osD.PropertyGet(key);
	}
	const char * SCI_METHOD DescribeWordListSets() override {
		return osD.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}

	static ILexer5 *LexerFactoryD() {
		return new LexerD(true);
	}
	static ILexer5 *LexerFactoryDInsensitive() {
		return new LexerD(false);
	}
};

Sci_Position SCI_METHOD LexerD::PropertySet(const char *key, const char *val) {
	if (osD.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerD::WordListSet(int n, const char *wl) {
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
		wordListN = &keywords5;
		break;
	case 5:
		wordListN = &keywords6;
		break;
	case 6:
		wordListN = &keywords7;
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

void SCI_METHOD LexerD::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	int styleBeforeDCKeyword = SCE_D_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	Sci_Position curLine = styler.GetLine(startPos);
	int curNcLevel = curLine > 0? styler.GetLineState(curLine-1): 0;
	bool numFloat = false; // Float literals have '+' and '-' signs
	bool numHex = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curNcLevel);
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_D_OPERATOR:
				sc.SetState(SCE_D_DEFAULT);
				break;
			case SCE_D_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (IsASCII(sc.ch) && (isalnum(sc.ch) || sc.ch == '_')) {
					continue;
				} else if (sc.ch == '.' && sc.chNext != '.' && !numFloat) {
					// Don't parse 0..2 as number.
					numFloat=true;
					continue;
				} else if ( ( sc.ch == '-' || sc.ch == '+' ) && (		/*sign and*/
					( !numHex && ( sc.chPrev == 'e' || sc.chPrev == 'E' ) ) || /*decimal or*/
					( sc.chPrev == 'p' || sc.chPrev == 'P' ) ) ) {		/*hex*/
					// Parse exponent sign in float literals: 2e+10 0x2e+10
					continue;
				} else {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_IDENTIFIER:
				if (!IsWord(sc.ch)) {
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_D_WORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_D_WORD2);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_D_TYPEDEF);
					} else if (keywords5.InList(s)) {
						sc.ChangeState(SCE_D_WORD5);
					} else if (keywords6.InList(s)) {
						sc.ChangeState(SCE_D_WORD6);
					} else if (keywords7.InList(s)) {
						sc.ChangeState(SCE_D_WORD7);
					}
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_D_COMMENTDOC;
						sc.SetState(SCE_D_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_D_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_COMMENTLINEDOC:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_D_COMMENTLINEDOC;
						sc.SetState(SCE_D_COMMENTDOCKEYWORD);
					}
				}
				break;
			case SCE_D_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_D_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_D_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				} else if (!IsDoxygen(sc.ch)) {
					char s[100];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (!IsASpace(sc.ch) || !keywords3.InList(s + 1)) {
						sc.ChangeState(SCE_D_COMMENTDOCKEYWORDERROR);
					}
					sc.SetState(styleBeforeDCKeyword);
				}
				break;
			case SCE_D_COMMENTNESTED:
				if (sc.Match('+', '/')) {
					if (curNcLevel > 0)
						curNcLevel -= 1;
					curLine = styler.GetLine(sc.currentPos);
					styler.SetLineState(curLine, curNcLevel);
					sc.Forward();
					if (curNcLevel == 0) {
						sc.ForwardSetState(SCE_D_DEFAULT);
					}
				} else if (sc.Match('/','+')) {
					curNcLevel += 1;
					curLine = styler.GetLine(sc.currentPos);
					styler.SetLineState(curLine, curNcLevel);
					sc.Forward();
				}
				break;
			case SCE_D_STRING:
				if (sc.ch == '\\') {
					if (sc.chNext == '"' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '"') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_D_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					// Char has no suffixes
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGB:
				if (sc.ch == '`') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
			case SCE_D_STRINGR:
				if (sc.ch == '"') {
					if(IsStringSuffix(sc.chNext))
						sc.Forward();
					sc.ForwardSetState(SCE_D_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_D_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_D_NUMBER);
				numFloat = sc.ch == '.';
				// Remember hex literal
				numHex = sc.ch == '0' && ( sc.chNext == 'x' || sc.chNext == 'X' );
			} else if ( (sc.ch == 'r' || sc.ch == 'x' || sc.ch == 'q')
				&& sc.chNext == '"' ) {
				// Limited support for hex and delimited strings: parse as r""
				sc.SetState(SCE_D_STRINGR);
				sc.Forward();
			} else if (IsWordStart(sc.ch) || sc.ch == '$') {
				sc.SetState(SCE_D_IDENTIFIER);
			} else if (sc.Match('/','+')) {
				curNcLevel += 1;
				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, curNcLevel);
				sc.SetState(SCE_D_COMMENTNESTED);
				sc.Forward();
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {   // Support of Qt/Doxygen doc. style
					sc.SetState(SCE_D_COMMENTDOC);
				} else {
					sc.SetState(SCE_D_COMMENT);
				}
				sc.Forward();   // Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_D_COMMENTLINEDOC);
				else
					sc.SetState(SCE_D_COMMENTLINE);
			} else if (sc.ch == '"') {
				sc.SetState(SCE_D_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_D_CHARACTER);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_D_STRINGB);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_D_OPERATOR);
				if (sc.ch == '.' && sc.chNext == '.') sc.Forward(); // Range operator
			}
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerD::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool foldAtElse = options.foldAtElseInt >= 0 ? options.foldAtElseInt != 0 : options.foldAtElse;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldComment && options.foldCommentMultiline && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_D_COMMENTLINE) || options.foldExplicitAnywhere)) {
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
		if (options.foldSyntaxBased && (style == SCE_D_OPERATOR)) {
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
		if (atEOL || (i == endPos-1)) {
			if (options.foldComment && options.foldCommentMultiline) {  // Handle nested comments
				int nc;
				nc =  styler.GetLineState(lineCurrent);
				nc -= lineCurrent>0? styler.GetLineState(lineCurrent-1): 0;
				levelNext += nc;
			}
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && foldAtElse) {
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
			visibleChars = 0;
		}
		if (!IsASpace(ch))
			visibleChars++;
	}
}

LexerModule lmD(SCLEX_D, LexerD::LexerFactoryD, "d", dWordLists);
