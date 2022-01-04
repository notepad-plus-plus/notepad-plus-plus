// Scintilla source code edit control
/** @file LexSML.cxx
 ** Lexer for SML.
 **/
// Copyright 2009 by James Moffatt and Yuzhou Xin
// Modified from LexCaml.cxx by Robert Roessler <robertr@rftp.com> Copyright 2005
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wcomma"
#endif

inline int  issml(int c) {return isalnum(c) || c == '_';}
inline int issmlf(int c) {return isalpha(c) || c == '_';}
inline int issmld(int c) {return isdigit(c) || c == '_';}


using namespace Lexilla;

static void ColouriseSMLDoc(
	Sci_PositionU startPos, Sci_Position length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);
	int nesting = 0;
	if (sc.state < SCE_SML_STRING)
		sc.state = SCE_SML_DEFAULT;
	if (sc.state >= SCE_SML_COMMENT)
		nesting = (sc.state & 0x0f) - SCE_SML_COMMENT;

	Sci_PositionU chToken = 0;
	int chBase = 0, chLit = 0;
	WordList& keywords  = *keywordlists[0];
	WordList& keywords2 = *keywordlists[1];
	WordList& keywords3 = *keywordlists[2];
	const int useMagic = styler.GetPropertyInt("lexer.caml.magic", 0);

	while (sc.More()) {
		int state2 = -1;
		Sci_Position chColor = sc.currentPos - 1;
		bool advance = true;

		switch (sc.state & 0x0f) {
		case SCE_SML_DEFAULT:
			chToken = sc.currentPos;
			if (issmlf(sc.ch))
				state2 = SCE_SML_IDENTIFIER;
			else if (sc.Match('`') && issmlf(sc.chNext))
				state2 = SCE_SML_TAGNAME;
			else if (sc.Match('#')&&isdigit(sc.chNext))
					state2 = SCE_SML_LINENUM;
			else if (sc.Match('#','\"')){
					state2 = SCE_SML_CHAR,chLit = 0;
					sc.Forward();

				}
			else if (isdigit(sc.ch)) {
				state2 = SCE_SML_NUMBER, chBase = 10;
				if (sc.Match('0') && strchr("xX", sc.chNext))
					chBase = 16, sc.Forward();}
			else if (sc.Match('\"')&&sc.chPrev!='#')
				state2 = SCE_SML_STRING;
			else if (sc.Match('(', '*')){
				state2 = SCE_SML_COMMENT,
					sc.ch = ' ',
					sc.Forward();}
			else if (strchr("!~"
					"=<>@^+-*/"
					"()[];,:.#", sc.ch))
				state2 = SCE_SML_OPERATOR;
			break;

		case SCE_SML_IDENTIFIER:
			if (!(issml(sc.ch) || sc.Match('\''))) {
				const Sci_Position n = sc.currentPos - chToken;
				if (n < 24) {
					char t[24];
					for (Sci_Position i = -n; i < 0; i++)
						t[n + i] = static_cast<char>(sc.GetRelative(i));
					t[n] = '\0';
					if ((n == 1 && sc.chPrev == '_') || keywords.InList(t))
						sc.ChangeState(SCE_SML_KEYWORD);
					else if (keywords2.InList(t))
						sc.ChangeState(SCE_SML_KEYWORD2);
					else if (keywords3.InList(t))
						sc.ChangeState(SCE_SML_KEYWORD3);
				}
				state2 = SCE_SML_DEFAULT, advance = false;
			}
			break;

		case SCE_SML_TAGNAME:
			if (!(issml(sc.ch) || sc.Match('\'')))
				state2 = SCE_SML_DEFAULT, advance = false;
			break;

		case SCE_SML_LINENUM:
			if (!isdigit(sc.ch))
				state2 = SCE_SML_DEFAULT, advance = false;
			break;

		case SCE_SML_OPERATOR: {
			const char* o = 0;
			if (issml(sc.ch) || isspace(sc.ch)
				|| (o = strchr(")]};,\'\"`#", sc.ch),o)
				|| !strchr("!$%&*+-./:<=>?@^|~", sc.ch)) {
				if (o && strchr(")]};,", sc.ch)) {
					if ((sc.Match(')') && sc.chPrev == '(')
						|| (sc.Match(']') && sc.chPrev == '['))
						sc.ChangeState(SCE_SML_KEYWORD);
					chColor++;
				} else
					advance = false;
				state2 = SCE_SML_DEFAULT;
			}
			break;
		}

		case SCE_SML_NUMBER:
			if (issmld(sc.ch) || IsADigit(sc.ch, chBase))
				break;
			if ((sc.Match('l') || sc.Match('L') || sc.Match('n'))
				&& (issmld(sc.chPrev) || IsADigit(sc.chPrev, chBase)))
				break;
			if (chBase == 10) {
				if (sc.Match('.') && issmld(sc.chPrev))
					break;
				if ((sc.Match('e') || sc.Match('E'))
					&& (issmld(sc.chPrev) || sc.chPrev == '.'))
					break;
				if ((sc.Match('+') || sc.Match('-'))
					&& (sc.chPrev == 'e' || sc.chPrev == 'E'))
					break;
			}
			state2 = SCE_SML_DEFAULT, advance = false;
			break;

		case SCE_SML_CHAR:
			if (sc.Match('\\')) {
				chLit = 1;
				if (sc.chPrev == '\\')
					sc.ch = ' ';
			} else if ((sc.Match('\"') && sc.chPrev != '\\') || sc.atLineEnd) {
				state2 = SCE_SML_DEFAULT;
				chLit = 1;
				if (sc.Match('\"'))
					chColor++;
				else
					sc.ChangeState(SCE_SML_IDENTIFIER);
			} else if (chLit < 1 && sc.currentPos - chToken >= 3)
				sc.ChangeState(SCE_SML_IDENTIFIER), advance = false;
			break;

		case SCE_SML_STRING:
			if (sc.Match('\\') && sc.chPrev == '\\')
				sc.ch = ' ';
			else if (sc.Match('\"') && sc.chPrev != '\\')
				state2 = SCE_SML_DEFAULT, chColor++;
			break;

		case SCE_SML_COMMENT:
		case SCE_SML_COMMENT1:
		case SCE_SML_COMMENT2:
		case SCE_SML_COMMENT3:
			if (sc.Match('(', '*'))
				state2 = sc.state + 1, chToken = sc.currentPos,
					sc.ch = ' ',
					sc.Forward(), nesting++;
			else if (sc.Match(')') && sc.chPrev == '*') {
				if (nesting)
					state2 = (sc.state & 0x0f) - 1, chToken = 0, nesting--;
				else
					state2 = SCE_SML_DEFAULT;
				chColor++;
			} else if (useMagic && sc.currentPos - chToken == 4
				&& sc.Match('c') && sc.chPrev == 'r' && sc.GetRelative(-2) == '@')
				sc.state |= 0x10;
			break;
		}

		if (state2 >= 0)
			styler.ColourTo(chColor, sc.state), sc.ChangeState(state2);
		if (advance)
			sc.Forward();
	}

	sc.Complete();
}

static void FoldSMLDoc(
	Sci_PositionU, Sci_Position,
	int,
	WordList *[],
	Accessor &)
{
}

static const char * const SMLWordListDesc[] = {
	"Keywords",
	"Keywords2",
	"Keywords3",
	0
};

LexerModule lmSML(SCLEX_SML, ColouriseSMLDoc, "SML", FoldSMLDoc, SMLWordListDesc);

