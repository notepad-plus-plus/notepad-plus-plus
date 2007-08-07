// Scintilla source code edit control
/** @file LexCaml.cxx
 ** Lexer for Objective Caml.
 **/
// Copyright 2005 by Robert Roessler <robertr@rftp.com>
// The License.txt file describes the conditions under which this software may be distributed.
/*	Release History
	20050204 Initial release.
	20050205 Quick compiler standards/"cleanliness" adjustment.
	20050206 Added cast for IsLeadByte().
	20050209 Changes to "external" build support.
	20050306 Fix for 1st-char-in-doc "corner" case.
	20050502 Fix for [harmless] one-past-the-end coloring.
	20050515 Refined numeric token recognition logic.
	20051125 Added 2nd "optional" keywords class.
	20051129 Support "magic" (read-only) comments for RCaml.
	20051204 Swtich to using StyleContext infrastructure.
*/

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

//	Since the Microsoft __iscsym[f] funcs are not ANSI...
inline int  iscaml(int c) {return isalnum(c) || c == '_';}
inline int iscamlf(int c) {return isalpha(c) || c == '_';}
inline int iscamld(int c) {return isdigit(c) || c == '_';}

static const int baseT[24] = {
	0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* A - L */
	0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0,16	/* M - X */
};

#ifdef BUILD_AS_EXTERNAL_LEXER
/*
	(actually seems to work!)
*/
#include "WindowAccessor.h"
#include "ExternalLexer.h"

#if PLAT_WIN
#include <windows.h>
#endif

static void ColouriseCamlDoc(
	unsigned int startPos, int length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler);

static void FoldCamlDoc(
	unsigned int startPos, int length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler);

static void InternalLexOrFold(int lexOrFold, unsigned int startPos, int length,
	int initStyle, char *words[], WindowID window, char *props);

static const char* LexerName = "caml";

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer,format,pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) {
}
#endif

bool Platform::IsDBCSLeadByte(int codePage, char ch) {
	return ::IsDBCSLeadByteEx(codePage, ch) != 0;
}

long Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam,
		reinterpret_cast<LPARAM>(lParam));
}

void EXT_LEXER_DECL Fold(unsigned int lexer, unsigned int startPos, int length,
	int initStyle, char *words[], WindowID window, char *props)
{
	// below useless evaluation(s) to supress "not used" warnings
	lexer;
	// build expected data structures and do the Fold
	InternalLexOrFold(1, startPos, length, initStyle, words, window, props);

}

int EXT_LEXER_DECL GetLexerCount()
{
	return 1;	// just us [Objective] Caml lexers here!
}

void EXT_LEXER_DECL GetLexerName(unsigned int Index, char *name, int buflength)
{
	// below useless evaluation(s) to supress "not used" warnings
	Index;
	// return as much of our lexer name as will fit (what's up with Index?)
	if (buflength > 0) {
		buflength--;
		int n = strlen(LexerName);
		if (n > buflength)
			n = buflength;
		memcpy(name, LexerName, n), name[n] = '\0';
	}
}

void EXT_LEXER_DECL Lex(unsigned int lexer, unsigned int startPos, int length,
	int initStyle, char *words[], WindowID window, char *props)
{
	// below useless evaluation(s) to supress "not used" warnings
	lexer;
	// build expected data structures and do the Lex
	InternalLexOrFold(0, startPos, length, initStyle, words, window, props);
}

static void InternalLexOrFold(int foldOrLex, unsigned int startPos, int length,
	int initStyle, char *words[], WindowID window, char *props)
{
	// create and initialize a WindowAccessor (including contained PropSet)
	PropSet ps;
	ps.SetMultiple(props);
	WindowAccessor wa(window, ps);
	// create and initialize WordList(s)
	int nWL = 0;
	for (; words[nWL]; nWL++) ;	// count # of WordList PTRs needed
	WordList** wl = new WordList* [nWL + 1];// alloc WordList PTRs
	int i = 0;
	for (; i < nWL; i++) {
		wl[i] = new WordList();	// (works or THROWS bad_alloc EXCEPTION)
		wl[i]->Set(words[i]);
	}
	wl[i] = 0;
	// call our "internal" folder/lexer (... then do Flush!)
	if (foldOrLex)
		FoldCamlDoc(startPos, length, initStyle, wl, wa);
	else
		ColouriseCamlDoc(startPos, length, initStyle, wl, wa);
	wa.Flush();
	// clean up before leaving
	for (i = nWL - 1; i >= 0; i--)
		delete wl[i];
	delete [] wl;
}

static
#endif	/* BUILD_AS_EXTERNAL_LEXER */

void ColouriseCamlDoc(
	unsigned int startPos, int length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler)
{
	// initialize styler
	StyleContext sc(startPos, length, initStyle, styler);
	// set up [initial] state info (terminating states that shouldn't "bleed")
	int nesting = 0;
	if (sc.state < SCE_CAML_STRING)
		sc.state = SCE_CAML_DEFAULT;
	if (sc.state >= SCE_CAML_COMMENT)
		nesting = (sc.state & 0x0f) - SCE_CAML_COMMENT;

	int chBase = 0, chToken = 0, chLit = 0;
	WordList& keywords  = *keywordlists[0];
	WordList& keywords2 = *keywordlists[1];
	WordList& keywords3 = *keywordlists[2];
	const int useMagic = styler.GetPropertyInt("lexer.caml.magic", 0);

	// foreach char in range...
	while (sc.More()) {
		// set up [per-char] state info
		int state2 = -1;		// (ASSUME no state change)
		int chColor = sc.currentPos - 1;// (ASSUME standard coloring range)
		bool advance = true;	// (ASSUME scanner "eats" 1 char)

		// step state machine
		switch (sc.state & 0x0f) {
		case SCE_CAML_DEFAULT:
			chToken = sc.currentPos;	// save [possible] token start (JIC)
			// it's wide open; what do we have?
			if (iscamlf(sc.ch))
				state2 = SCE_CAML_IDENTIFIER;
			else if (sc.Match('`') && iscamlf(sc.chNext))
				state2 = SCE_CAML_TAGNAME;
			else if (sc.Match('#') && isdigit(sc.chNext))
				state2 = SCE_CAML_LINENUM;
			else if (isdigit(sc.ch)) {
				state2 = SCE_CAML_NUMBER, chBase = 10;
				if (sc.Match('0') && strchr("bBoOxX", sc.chNext))
					chBase = baseT[tolower(sc.chNext) - 'a'], sc.Forward();
			} else if (sc.Match('\''))	/* (char literal?) */
				state2 = SCE_CAML_CHAR, chLit = 0;
			else if (sc.Match('\"'))
				state2 = SCE_CAML_STRING;
			else if (sc.Match('(', '*'))
				state2 = SCE_CAML_COMMENT,
					sc.ch = ' ',	// (make SURE "(*)" isn't seen as a closed comment)
					sc.Forward();
			else if (strchr("!?~"		/* Caml "prefix-symbol" */
					"=<>@^|&+-*/$%"		/* Caml "infix-symbol" */
					"()[]{};,:.#", sc.ch))	/* Caml "bracket" or ;,:.# */
				state2 = SCE_CAML_OPERATOR;
			break;

		case SCE_CAML_IDENTIFIER:
			// [try to] interpret as [additional] identifier char
			if (!(iscaml(sc.ch) || sc.Match('\''))) {
				const int n = sc.currentPos - chToken;
				if (n < 24) {
					// length is believable as keyword, [re-]construct token
					char t[24];
					for (int i = -n; i < 0; i++)
						t[n + i] = static_cast<char>(sc.GetRelative(i));
					t[n] = '\0';
					// special-case "_" token as KEYWORD
					if ((n == 1 && sc.chPrev == '_') || keywords.InList(t))
						sc.ChangeState(SCE_CAML_KEYWORD);
					else if (keywords2.InList(t))
						sc.ChangeState(SCE_CAML_KEYWORD2);
					else if (keywords3.InList(t))
						sc.ChangeState(SCE_CAML_KEYWORD3);
				}
				state2 = SCE_CAML_DEFAULT, advance = false;
			}
			break;

		case SCE_CAML_TAGNAME:
			// [try to] interpret as [additional] tagname char
			if (!(iscaml(sc.ch) || sc.Match('\'')))
				state2 = SCE_CAML_DEFAULT, advance = false;
			break;

		/*case SCE_CAML_KEYWORD:
		case SCE_CAML_KEYWORD2:
		case SCE_CAML_KEYWORD3:
			// [try to] interpret as [additional] keyword char
			if (!iscaml(ch))
				state2 = SCE_CAML_DEFAULT, advance = false;
			break;*/

		case SCE_CAML_LINENUM:
			// [try to] interpret as [additional] linenum directive char
			if (!isdigit(sc.ch))
				state2 = SCE_CAML_DEFAULT, advance = false;
			break;

		case SCE_CAML_OPERATOR: {
			// [try to] interpret as [additional] operator char
			const char* o = 0;
			if (iscaml(sc.ch) || isspace(sc.ch)		   /* ident or whitespace */
				|| (o = strchr(")]};,\'\"`#", sc.ch),o)/* "termination" chars */
				|| !strchr("!$%&*+-./:<=>?@^|~", sc.ch)/* "operator" chars */) {
				// check for INCLUSIVE termination
				if (o && strchr(")]};,", sc.ch)) {
					if ((sc.Match(')') && sc.chPrev == '(')
						|| (sc.Match(']') && sc.chPrev == '['))
						// special-case "()" and "[]" tokens as KEYWORDS
						sc.ChangeState(SCE_CAML_KEYWORD);
					chColor++;
				} else
					advance = false;
				state2 = SCE_CAML_DEFAULT;
			}
			break;
		}

		case SCE_CAML_NUMBER:
			// [try to] interpret as [additional] numeric literal char
			// N.B. - improperly accepts "extra" digits in base 2 or 8 literals
			if (iscamld(sc.ch) || IsADigit(sc.ch, chBase))
				break;
			// how about an integer suffix?
			if ((sc.Match('l') || sc.Match('L') || sc.Match('n'))
				&& (iscamld(sc.chPrev) || IsADigit(sc.chPrev, chBase)))
				break;
			// or a floating-point literal?
			if (chBase == 10) {
				// with a decimal point?
				if (sc.Match('.') && iscamld(sc.chPrev))
					break;
				// with an exponent? (I)
				if ((sc.Match('e') || sc.Match('E'))
					&& (iscamld(sc.chPrev) || sc.chPrev == '.'))
					break;
				// with an exponent? (II)
				if ((sc.Match('+') || sc.Match('-'))
					&& (sc.chPrev == 'e' || sc.chPrev == 'E'))
					break;
			}
			// it looks like we have run out of number
			state2 = SCE_CAML_DEFAULT, advance = false;
			break;

		case SCE_CAML_CHAR:
			// [try to] interpret as [additional] char literal char
			if (sc.Match('\\')) {
				chLit = 1;	// (definitely IS a char literal)
				if (sc.chPrev == '\\')
					sc.ch = ' ';	// (so termination test isn't fooled)
			// should we be terminating - one way or another?
			} else if ((sc.Match('\'') && sc.chPrev != '\\') || sc.atLineEnd) {
				state2 = SCE_CAML_DEFAULT;
				if (sc.Match('\''))
					chColor++;
				else
					sc.ChangeState(SCE_CAML_IDENTIFIER);
			// ... maybe a char literal, maybe not
			} else if (chLit < 1 && sc.currentPos - chToken >= 2)
				sc.ChangeState(SCE_CAML_IDENTIFIER), advance = false;
			break;

		case SCE_CAML_STRING:
			// [try to] interpret as [additional] string literal char
			if (sc.Match('\\') && sc.chPrev == '\\')
				sc.ch = ' ';	// (so '\\' doesn't cause us trouble)
			else if (sc.Match('\"') && sc.chPrev != '\\')
				state2 = SCE_CAML_DEFAULT, chColor++;
			break;

		case SCE_CAML_COMMENT:
		case SCE_CAML_COMMENT1:
		case SCE_CAML_COMMENT2:
		case SCE_CAML_COMMENT3:
			// we're IN a comment - does this start a NESTED comment?
			if (sc.Match('(', '*'))
				state2 = sc.state + 1, chToken = sc.currentPos,
					sc.ch = ' ',	// (make SURE "(*)" isn't seen as a closed comment)
					sc.Forward(), nesting++;
			// [try to] interpret as [additional] comment char
			else if (sc.Match(')') && sc.chPrev == '*') {
				if (nesting)
					state2 = (sc.state & 0x0f) - 1, chToken = 0, nesting--;
				else
					state2 = SCE_CAML_DEFAULT;
				chColor++;
			// enable "magic" (read-only) comment AS REQUIRED
			} else if (useMagic && sc.currentPos - chToken == 4
				&& sc.Match('c') && sc.chPrev == 'r' && sc.GetRelative(-2) == '@')
				sc.state |= 0x10;	// (switch to read-only comment style)
			break;
		}

		// handle state change and char coloring as required
		if (state2 >= 0)
			styler.ColourTo(chColor, sc.state), sc.ChangeState(state2);
		// move to next char UNLESS re-scanning current char
		if (advance)
			sc.Forward();
	}

	// do any required terminal char coloring (JIC)
	sc.Complete();
}

#ifdef BUILD_AS_EXTERNAL_LEXER
static
#endif	/* BUILD_AS_EXTERNAL_LEXER */
void FoldCamlDoc(
	unsigned int startPos, int length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler)
{
	// below useless evaluation(s) to supress "not used" warnings
	startPos || length || initStyle || keywordlists[0] || styler.Length();
}

static const char * const camlWordListDesc[] = {
	"Keywords",		// primary Objective Caml keywords
	"Keywords2",	// "optional" keywords (typically from Pervasives)
	"Keywords3",	// "optional" keywords (typically typenames)
	0
};

#ifndef BUILD_AS_EXTERNAL_LEXER
LexerModule lmCaml(SCLEX_CAML, ColouriseCamlDoc, "caml", FoldCamlDoc, camlWordListDesc);
#endif	/* BUILD_AS_EXTERNAL_LEXER */
