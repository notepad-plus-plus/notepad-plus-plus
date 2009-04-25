// Scintilla source code edit control
/** @file LexCSS.cxx
 ** Lexer for Cascading Style Sheets
 ** Written by Jakub Vrána
 ** Improved by Philippe Lhoste (CSS2)
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
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

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


static inline bool IsAWordChar(const unsigned int ch) {
	/* FIXME:
	 * The CSS spec allows "ISO 10646 characters U+00A1 and higher" to be treated as word chars.
	 * Unfortunately, we are only getting string bytes here, and not full unicode characters. We cannot guarantee
	 * that our byte is between U+0080 - U+00A0 (to return false), so we have to allow all characters U+0080 and higher
	 */
	return ch >= 0x80 || isalnum(ch) || ch == '-' || ch == '_';
}

inline bool IsCssOperator(const int ch) {
	if (!((ch < 0x80) && isalnum(ch)) &&
		(ch == '{' || ch == '}' || ch == ':' || ch == ',' || ch == ';' ||
		 ch == '.' || ch == '#' || ch == '!' || ch == '@' ||
		 /* CSS2 */
		 ch == '*' || ch == '>' || ch == '+' || ch == '=' || ch == '~' || ch == '|' ||
		 ch == '[' || ch == ']' || ch == '(' || ch == ')')) {
		return true;
	}
	return false;
}

static void ColouriseCssDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[], Accessor &styler) {
	WordList &css1Props = *keywordlists[0];
	WordList &pseudoClasses = *keywordlists[1];
	WordList &css2Props = *keywordlists[2];
	WordList &css3Props = *keywordlists[3];
	WordList &pseudoElements = *keywordlists[4];
	WordList &exProps = *keywordlists[5];
	WordList &exPseudoClasses = *keywordlists[6];
	WordList &exPseudoElements = *keywordlists[7];

	StyleContext sc(startPos, length, initStyle, styler);

	int lastState = -1; // before operator
	int lastStateC = -1; // before comment
	int op = ' '; // last operator
	int opPrev = ' '; // last operator

	for (; sc.More(); sc.Forward()) {
		if (sc.state == SCE_CSS_COMMENT && sc.Match('*', '/')) {
			if (lastStateC == -1) {
				// backtrack to get last state:
				// comments are like whitespace, so we must return to the previous state
				unsigned int i = startPos;
				for (; i > 0; i--) {
					if ((lastStateC = styler.StyleAt(i-1)) != SCE_CSS_COMMENT) {
						if (lastStateC == SCE_CSS_OPERATOR) {
							op = styler.SafeGetCharAt(i-1);
							opPrev = styler.SafeGetCharAt(i-2);
							while (--i) {
								lastState = styler.StyleAt(i-1);
								if (lastState != SCE_CSS_OPERATOR && lastState != SCE_CSS_COMMENT)
									break;
							}
							if (i == 0)
								lastState = SCE_CSS_DEFAULT;
						}
						break;
					}
				}
				if (i == 0)
					lastStateC = SCE_CSS_DEFAULT;
			}
			sc.Forward();
			sc.ForwardSetState(lastStateC);
		}

		if (sc.state == SCE_CSS_COMMENT)
			continue;

		if (sc.state == SCE_CSS_DOUBLESTRING || sc.state == SCE_CSS_SINGLESTRING) {
			if (sc.ch != (sc.state == SCE_CSS_DOUBLESTRING ? '\"' : '\''))
				continue;
			unsigned int i = sc.currentPos;
			while (i && styler[i-1] == '\\')
				i--;
			if ((sc.currentPos - i) % 2 == 1)
				continue;
			sc.ForwardSetState(SCE_CSS_VALUE);
		}

		if (sc.state == SCE_CSS_OPERATOR) {
			if (op == ' ') {
				unsigned int i = startPos;
				op = styler.SafeGetCharAt(i-1);
				opPrev = styler.SafeGetCharAt(i-2);
				while (--i) {
					lastState = styler.StyleAt(i-1);
					if (lastState != SCE_CSS_OPERATOR && lastState != SCE_CSS_COMMENT)
						break;
				}
			}
			switch (op) {
			case '@':
				if (lastState == SCE_CSS_DEFAULT)
					sc.SetState(SCE_CSS_DIRECTIVE);
				break;
			case '>':
			case '+':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS)
					sc.SetState(SCE_CSS_DEFAULT);
				break;
			case '[':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS)
					sc.SetState(SCE_CSS_ATTRIBUTE);
				break;
			case ']':
				if (lastState == SCE_CSS_ATTRIBUTE)
					sc.SetState(SCE_CSS_TAG);
				break;
			case '{':
				if (lastState == SCE_CSS_DIRECTIVE)
					sc.SetState(SCE_CSS_DEFAULT);
				else if (lastState == SCE_CSS_TAG)
					sc.SetState(SCE_CSS_IDENTIFIER);
				break;
			case '}':
				if (lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_VALUE || lastState == SCE_CSS_IMPORTANT ||
					lastState == SCE_CSS_IDENTIFIER || lastState == SCE_CSS_IDENTIFIER2 || lastState == SCE_CSS_IDENTIFIER3)
					sc.SetState(SCE_CSS_DEFAULT);
				break;
			case '(':
				if (lastState == SCE_CSS_PSEUDOCLASS)
					sc.SetState(SCE_CSS_TAG);
				else if (lastState == SCE_CSS_EXTENDED_PSEUDOCLASS)
					sc.SetState(SCE_CSS_EXTENDED_PSEUDOCLASS);
				break;
			case ')':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS ||
					lastState == SCE_CSS_PSEUDOELEMENT || lastState == SCE_CSS_EXTENDED_PSEUDOELEMENT)
					sc.SetState(SCE_CSS_TAG);
				break;
			case ':':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS ||
					lastState == SCE_CSS_PSEUDOELEMENT || lastState == SCE_CSS_EXTENDED_PSEUDOELEMENT)
					sc.SetState(SCE_CSS_PSEUDOCLASS);
				else if (lastState == SCE_CSS_IDENTIFIER || lastState == SCE_CSS_IDENTIFIER2 ||
					lastState == SCE_CSS_IDENTIFIER3 || lastState == SCE_CSS_EXTENDED_IDENTIFIER ||
					lastState == SCE_CSS_UNKNOWN_IDENTIFIER)
					sc.SetState(SCE_CSS_VALUE);
				break;
			case '.':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS)
					sc.SetState(SCE_CSS_CLASS);
				break;
			case '#':
				if (lastState == SCE_CSS_TAG || lastState == SCE_CSS_DEFAULT || lastState == SCE_CSS_CLASS || lastState == SCE_CSS_ID ||
					lastState == SCE_CSS_PSEUDOCLASS || lastState == SCE_CSS_EXTENDED_PSEUDOCLASS || lastState == SCE_CSS_UNKNOWN_PSEUDOCLASS)
					sc.SetState(SCE_CSS_ID);
				break;
			case ',':
			case '|':
			case '~':
				if (lastState == SCE_CSS_TAG)
					sc.SetState(SCE_CSS_DEFAULT);
				break;
			case ';':
				if (lastState == SCE_CSS_DIRECTIVE)
					sc.SetState(SCE_CSS_DEFAULT);
				else if (lastState == SCE_CSS_VALUE || lastState == SCE_CSS_IMPORTANT)
					sc.SetState(SCE_CSS_IDENTIFIER);
				break;
			case '!':
				if (lastState == SCE_CSS_VALUE)
					sc.SetState(SCE_CSS_IMPORTANT);
				break;
			}
		}

		if (IsAWordChar(sc.ch)) {
			if (sc.state == SCE_CSS_DEFAULT)
				sc.SetState(SCE_CSS_TAG);
			continue;
		}

		if (sc.ch == '*' && sc.state == SCE_CSS_DEFAULT) {
			sc.SetState(SCE_CSS_TAG);
			continue;
		}

		if (IsAWordChar(sc.chPrev) && (
			sc.state == SCE_CSS_IDENTIFIER || sc.state == SCE_CSS_IDENTIFIER2 ||
			sc.state == SCE_CSS_IDENTIFIER3 || sc.state == SCE_CSS_EXTENDED_IDENTIFIER ||
			sc.state == SCE_CSS_UNKNOWN_IDENTIFIER ||
			sc.state == SCE_CSS_PSEUDOCLASS || sc.state == SCE_CSS_PSEUDOELEMENT ||
			sc.state == SCE_CSS_EXTENDED_PSEUDOCLASS || sc.state == SCE_CSS_EXTENDED_PSEUDOELEMENT ||
			sc.state == SCE_CSS_UNKNOWN_PSEUDOCLASS ||
			sc.state == SCE_CSS_IMPORTANT
		)) {
			char s[100];
			sc.GetCurrentLowered(s, sizeof(s));
			char *s2 = s;
			while (*s2 && !IsAWordChar(*s2))
				s2++;
			switch (sc.state) {
			case SCE_CSS_IDENTIFIER:
			case SCE_CSS_IDENTIFIER2:
			case SCE_CSS_IDENTIFIER3:
			case SCE_CSS_EXTENDED_IDENTIFIER:
			case SCE_CSS_UNKNOWN_IDENTIFIER:
				if (css1Props.InList(s2))
					sc.ChangeState(SCE_CSS_IDENTIFIER);
				else if (css2Props.InList(s2))
					sc.ChangeState(SCE_CSS_IDENTIFIER2);
				else if (css3Props.InList(s2))
					sc.ChangeState(SCE_CSS_IDENTIFIER3);
				else if (exProps.InList(s2))
					sc.ChangeState(SCE_CSS_EXTENDED_IDENTIFIER);
				else
					sc.ChangeState(SCE_CSS_UNKNOWN_IDENTIFIER);
				break;
			case SCE_CSS_PSEUDOCLASS:
			case SCE_CSS_PSEUDOELEMENT:
			case SCE_CSS_EXTENDED_PSEUDOCLASS:
			case SCE_CSS_EXTENDED_PSEUDOELEMENT:
			case SCE_CSS_UNKNOWN_PSEUDOCLASS:
				if (op == ':' && opPrev != ':' && pseudoClasses.InList(s2))
					sc.ChangeState(SCE_CSS_PSEUDOCLASS);
				else if (opPrev == ':' && pseudoElements.InList(s2))
					sc.ChangeState(SCE_CSS_PSEUDOELEMENT);
				else if ((op == ':' || (op == '(' && lastState == SCE_CSS_EXTENDED_PSEUDOCLASS)) && opPrev != ':' && exPseudoClasses.InList(s2))
					sc.ChangeState(SCE_CSS_EXTENDED_PSEUDOCLASS);
				else if (opPrev == ':' && exPseudoElements.InList(s2))
					sc.ChangeState(SCE_CSS_EXTENDED_PSEUDOELEMENT);
				else
					sc.ChangeState(SCE_CSS_UNKNOWN_PSEUDOCLASS);
				break;
			case SCE_CSS_IMPORTANT:
				if (strcmp(s2, "important") != 0)
					sc.ChangeState(SCE_CSS_VALUE);
				break;
			}
		}

		if (sc.ch != '.' && sc.ch != ':' && sc.ch != '#' && (
			sc.state == SCE_CSS_CLASS || sc.state == SCE_CSS_ID ||
			(sc.ch != '(' && sc.ch != ')' && ( /* This line of the condition makes it possible to extend pseudo-classes with parentheses */
				sc.state == SCE_CSS_PSEUDOCLASS || sc.state == SCE_CSS_PSEUDOELEMENT ||
				sc.state == SCE_CSS_EXTENDED_PSEUDOCLASS || sc.state == SCE_CSS_EXTENDED_PSEUDOELEMENT ||
				sc.state == SCE_CSS_UNKNOWN_PSEUDOCLASS
			))
		))
			sc.SetState(SCE_CSS_TAG);

		if (sc.Match('/', '*')) {
			lastStateC = sc.state;
			sc.SetState(SCE_CSS_COMMENT);
			sc.Forward();
		} else if (sc.state == SCE_CSS_VALUE && (sc.ch == '\"' || sc.ch == '\'')) {
			sc.SetState((sc.ch == '\"' ? SCE_CSS_DOUBLESTRING : SCE_CSS_SINGLESTRING));
		} else if (IsCssOperator(sc.ch)
			&& (sc.state != SCE_CSS_ATTRIBUTE || sc.ch == ']')
			&& (sc.state != SCE_CSS_VALUE || sc.ch == ';' || sc.ch == '}' || sc.ch == '!')
			&& (sc.state != SCE_CSS_DIRECTIVE || sc.ch == ';' || sc.ch == '{')
		) {
			if (sc.state != SCE_CSS_OPERATOR)
				lastState = sc.state;
			sc.SetState(SCE_CSS_OPERATOR);
			op = sc.ch;
			opPrev = sc.chPrev;
		}
	}

	sc.Complete();
}

static void FoldCSSDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool inComment = (styler.StyleAt(startPos-1) == SCE_CSS_COMMENT);
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment) {
			if (!inComment && (style == SCE_CSS_COMMENT))
				levelCurrent++;
			else if (inComment && (style != SCE_CSS_COMMENT))
				levelCurrent--;
			inComment = (style == SCE_CSS_COMMENT);
		}
		if (style == SCE_CSS_OPERATOR) {
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

static const char * const cssWordListDesc[] = {
	"CSS1 Properties",
	"Pseudo-classes",
	"CSS2 Properties",
	"CSS3 Properties",
	"Pseudo-elements",
	"Browser-Specific CSS Properties",
	"Browser-Specific Pseudo-classes",
	"Browser-Specific Pseudo-elements",
	0
};

LexerModule lmCss(SCLEX_CSS, ColouriseCssDoc, "css", FoldCSSDoc, cssWordListDesc);
