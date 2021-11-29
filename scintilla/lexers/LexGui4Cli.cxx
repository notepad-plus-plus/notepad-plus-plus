// Scintilla source code edit control
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// @file LexGui4Cli.cxx
/*
This is the Lexer for Gui4Cli, included in SciLexer.dll
- by d. Keletsekis, 2/10/2003

To add to SciLexer.dll:
1. Add the values below to INCLUDE\Scintilla.iface
2. Run the scripts/HFacer.py script
3. Run the scripts/LexGen.py script

val SCE_GC_DEFAULT=0
val SCE_GC_COMMENTLINE=1
val SCE_GC_COMMENTBLOCK=2
val SCE_GC_GLOBAL=3
val SCE_GC_EVENT=4
val SCE_GC_ATTRIBUTE=5
val SCE_GC_CONTROL=6
val SCE_GC_COMMAND=7
val SCE_GC_STRING=8
val SCE_GC_OPERATOR=9
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Scintilla;

#define debug Platform::DebugPrintf

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch =='\\');
}

inline bool isGCOperator(int ch)
{	if (isalnum(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		 ch == '(' || ch == ')' || ch == '=' || ch == '%' ||
		 ch == '[' || ch == ']' || ch == '<' || ch == '>' ||
		 ch == ',' || ch == ';' || ch == ':')
		return true;
	return false;
}

#define isSpace(x)		((x)==' ' || (x)=='\t')
#define isNL(x)			((x)=='\n' || (x)=='\r')
#define isSpaceOrNL(x)  (isSpace(x) || isNL(x))
#define BUFFSIZE 500
#define isFoldPoint(x)  ((styler.LevelAt(x) & SC_FOLDLEVELNUMBERMASK) == 1024)

static void colorFirstWord(WordList *keywordlists[], Accessor &styler,
									StyleContext *sc, char *buff, Sci_Position length, Sci_Position)
{
	Sci_Position c = 0;
	while (sc->More() && isSpaceOrNL(sc->ch))
	{	sc->Forward();
	}
	styler.ColourTo(sc->currentPos - 1, sc->state);

	if (!IsAWordChar(sc->ch)) // comment, marker, etc..
		return;

	while (sc->More() && !isSpaceOrNL(sc->ch) && (c < length-1) && !isGCOperator(sc->ch))
	{	buff[c] = static_cast<char>(sc->ch);
		++c; sc->Forward();
	}
	buff[c] = '\0';
	char *p = buff;
	while (*p)	// capitalize..
	{	if (islower(*p)) *p = static_cast<char>(toupper(*p));
		++p;
	}

	WordList &kGlobal		= *keywordlists[0];	// keyword lists set by the user
	WordList &kEvent		= *keywordlists[1];
	WordList &kAttribute	= *keywordlists[2];
	WordList &kControl	= *keywordlists[3];
	WordList &kCommand	= *keywordlists[4];

	int state = 0;
	// int level = styler.LevelAt(line) & SC_FOLDLEVELNUMBERMASK;
	// debug ("line = %d, level = %d", line, level);

	if	     (kGlobal.InList(buff))		state = SCE_GC_GLOBAL;
	else if (kAttribute.InList(buff))	state = SCE_GC_ATTRIBUTE;
	else if (kControl.InList(buff))		state = SCE_GC_CONTROL;
	else if (kCommand.InList(buff))		state = SCE_GC_COMMAND;
	else if (kEvent.InList(buff))			state = SCE_GC_EVENT;

	if (state)
	{	sc->ChangeState(state);
		styler.ColourTo(sc->currentPos - 1, sc->state);
		sc->ChangeState(SCE_GC_DEFAULT);
	}
	else
	{	sc->ChangeState(SCE_GC_DEFAULT);
		styler.ColourTo(sc->currentPos - 1, sc->state);
	}
}

// Main colorizing function called by Scintilla
static void
ColouriseGui4CliDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                    WordList *keywordlists[], Accessor &styler)
{
	styler.StartAt(startPos);

	Sci_Position currentline = styler.GetLine(startPos);
	int quotestart = 0, oldstate;
	styler.StartSegment(startPos);
	bool noforward;
	char buff[BUFFSIZE+1];	// buffer for command name

	StyleContext sc(startPos, length, initStyle, styler);
	buff[0] = '\0'; // cbuff = 0;

	if (sc.state != SCE_GC_COMMENTBLOCK) // colorize 1st word..
		colorFirstWord(keywordlists, styler, &sc, buff, BUFFSIZE, currentline);

	while (sc.More())
	{	noforward = 0;

		switch (sc.ch)
		{
			case '/':
				if (sc.state == SCE_GC_COMMENTBLOCK || sc.state == SCE_GC_STRING)
					break;
				if (sc.chNext == '/')	// line comment
				{	sc.SetState (SCE_GC_COMMENTLINE);
					sc.Forward();
					styler.ColourTo(sc.currentPos, sc.state);
				}
				else if (sc.chNext == '*')	// block comment
				{	sc.SetState(SCE_GC_COMMENTBLOCK);
					sc.Forward();
					styler.ColourTo(sc.currentPos, sc.state);
				}
				else
					styler.ColourTo(sc.currentPos, sc.state);
				break;

			case '*':	// end of comment block, or operator..
				if (sc.state == SCE_GC_STRING)
					break;
				if (sc.state == SCE_GC_COMMENTBLOCK && sc.chNext == '/')
				{	sc.Forward();
					styler.ColourTo(sc.currentPos, sc.state);
					sc.ChangeState (SCE_GC_DEFAULT);
				}
				else
					styler.ColourTo(sc.currentPos, sc.state);
				break;

			case '\'':	case '\"': // strings..
				if (sc.state == SCE_GC_COMMENTBLOCK || sc.state == SCE_GC_COMMENTLINE)
					break;
				if (sc.state == SCE_GC_STRING)
				{	if (sc.ch == quotestart)	// match same quote char..
					{	styler.ColourTo(sc.currentPos, sc.state);
						sc.ChangeState(SCE_GC_DEFAULT);
						quotestart = 0;
				}	}
				else
				{	styler.ColourTo(sc.currentPos - 1, sc.state);
					sc.ChangeState(SCE_GC_STRING);
					quotestart = sc.ch;
				}
				break;

			case ';':	// end of commandline character
				if (sc.state != SCE_GC_COMMENTBLOCK && sc.state != SCE_GC_COMMENTLINE &&
					 sc.state != SCE_GC_STRING)
				{
					styler.ColourTo(sc.currentPos - 1, sc.state);
					styler.ColourTo(sc.currentPos, SCE_GC_OPERATOR);
					sc.ChangeState(SCE_GC_DEFAULT);
					sc.Forward();
					colorFirstWord(keywordlists, styler, &sc, buff, BUFFSIZE, currentline);
					noforward = 1; // don't move forward - already positioned at next char..
				}
				break;

			case '+': case '-': case '=':	case '!':	// operators..
			case '<': case '>': case '&': case '|': case '$':
				if (sc.state != SCE_GC_COMMENTBLOCK && sc.state != SCE_GC_COMMENTLINE &&
					 sc.state != SCE_GC_STRING)
				{
					styler.ColourTo(sc.currentPos - 1, sc.state);
					styler.ColourTo(sc.currentPos, SCE_GC_OPERATOR);
					sc.ChangeState(SCE_GC_DEFAULT);
				}
				break;

			case '\\':	// escape - same as operator, but also mark in strings..
				if (sc.state != SCE_GC_COMMENTBLOCK && sc.state != SCE_GC_COMMENTLINE)
				{
					oldstate = sc.state;
					styler.ColourTo(sc.currentPos - 1, sc.state);
					sc.Forward(); // mark also the next char..
					styler.ColourTo(sc.currentPos, SCE_GC_OPERATOR);
					sc.ChangeState(oldstate);
				}
				break;

			case '\n': case '\r':
				++currentline;
				if (sc.state == SCE_GC_COMMENTLINE)
				{	styler.ColourTo(sc.currentPos, sc.state);
					sc.ChangeState (SCE_GC_DEFAULT);
				}
				else if (sc.state != SCE_GC_COMMENTBLOCK)
				{	colorFirstWord(keywordlists, styler, &sc, buff, BUFFSIZE, currentline);
					noforward = 1; // don't move forward - already positioned at next char..
				}
				break;

//			case ' ': case '\t':
//			default :
		}

		if (!noforward) sc.Forward();

	}
	sc.Complete();
}

// Main folding function called by Scintilla - (based on props (.ini) files function)
static void FoldGui4Cli(Sci_PositionU startPos, Sci_Position length, int,
								WordList *[], Accessor &styler)
{
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	bool headerPoint = false;

	for (Sci_PositionU i = startPos; i < endPos; i++)
	{
		char ch = chNext;
		chNext = styler[i+1];

		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_GC_EVENT || style == SCE_GC_GLOBAL)
		{	headerPoint = true; // fold at events and globals
		}

		if (atEOL)
		{	int lev = SC_FOLDLEVELBASE+1;

			if (headerPoint)
				lev = SC_FOLDLEVELBASE;

			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;

			if (headerPoint)
				lev |= SC_FOLDLEVELHEADERFLAG;

			if (lev != styler.LevelAt(lineCurrent)) // set level, if not already correct
			{	styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;		// re-initialize our flags
			visibleChars = 0;
			headerPoint = false;
		}

		if (!(isspacechar(ch))) // || (style == SCE_GC_COMMENTLINE) || (style != SCE_GC_COMMENTBLOCK)))
			visibleChars++;
	}

	int lev = headerPoint ? SC_FOLDLEVELBASE : SC_FOLDLEVELBASE+1;
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, lev | flagsNext);
}

// I have no idea what these are for.. probably accessible by some message.
static const char * const gui4cliWordListDesc[] = {
	"Globals", "Events", "Attributes", "Control", "Commands",
	0
};

// Declare language & pass our function pointers to Scintilla
LexerModule lmGui4Cli(SCLEX_GUI4CLI, ColouriseGui4CliDoc, "gui4cli", FoldGui4Cli, gui4cliWordListDesc);

#undef debug

