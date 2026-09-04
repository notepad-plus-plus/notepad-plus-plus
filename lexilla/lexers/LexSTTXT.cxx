// Scintilla source code edit control
/** @file LexSTTXT.cxx
 ** Lexer for Structured Text language.
 ** Written by Pavel Bulochkin
 **/
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

using namespace Lexilla;

static void ClassifySTTXTWord(WordList *keywordlists[], StyleContext &sc)
{
	char s[256] = { 0 };
	sc.GetCurrentLowered(s, sizeof(s));

 	if ((*keywordlists[0]).InList(s)) {
 		sc.ChangeState(SCE_STTXT_KEYWORD);
 	}

	else if ((*keywordlists[1]).InList(s)) {
		sc.ChangeState(SCE_STTXT_TYPE);
	}

	else if ((*keywordlists[2]).InList(s)) {
		sc.ChangeState(SCE_STTXT_FUNCTION);
	}

	else if ((*keywordlists[3]).InList(s)) {
		sc.ChangeState(SCE_STTXT_FB);
	}

	else if ((*keywordlists[4]).InList(s)) {
		sc.ChangeState(SCE_STTXT_VARS);
	}

	else if ((*keywordlists[5]).InList(s)) {
		sc.ChangeState(SCE_STTXT_PRAGMAS);
	}

	sc.SetState(SCE_STTXT_DEFAULT);
}

static void ColouriseSTTXTDoc (Sci_PositionU startPos, Sci_Position length, int initStyle,
							  WordList *keywordlists[], Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);

	CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);
	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setNumber(CharacterSet::setDigits, "_.eE");
	CharacterSet setHexNumber(CharacterSet::setDigits, "_abcdefABCDEF");
	CharacterSet setOperator(CharacterSet::setNone,",.+-*/:;<=>[]()%&");
	CharacterSet setDataTime(CharacterSet::setDigits,"_.-:dmshDMSH");

 	for ( ; sc.More() ; sc.Forward())
 	{
		if(sc.atLineStart && sc.state != SCE_STTXT_COMMENT)
			sc.SetState(SCE_STTXT_DEFAULT);

		switch(sc.state)
		{
			case SCE_STTXT_NUMBER: {
				if(!setNumber.Contains(sc.ch))
					sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_HEXNUMBER: {
				if (setHexNumber.Contains(sc.ch))
					continue;
				else if(setDataTime.Contains(sc.ch))
					sc.ChangeState(SCE_STTXT_DATETIME);
				else if(setWord.Contains(sc.ch))
					sc.ChangeState(SCE_STTXT_DEFAULT);
				else
					sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_DATETIME: {
				if (setDataTime.Contains(sc.ch))
					continue;
				else if(setWord.Contains(sc.ch))
					sc.ChangeState(SCE_STTXT_DEFAULT);
				else
					sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_OPERATOR: {
				sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_PRAGMA: {
				if (sc.ch == '}')
					sc.ForwardSetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_COMMENTLINE: {
				if (sc.atLineStart)
					sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_COMMENT: {
				if(sc.Match('*',')'))
				{
					sc.Forward();
					sc.ForwardSetState(SCE_STTXT_DEFAULT);
				}
				break;
			}
			case SCE_STTXT_STRING1: {
				if(sc.atLineEnd)
					sc.SetState(SCE_STTXT_STRINGEOL);
				else if(sc.ch == '\'' && sc.chPrev != '$')
					sc.ForwardSetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_STRING2: {
				if (sc.atLineEnd)
					sc.SetState(SCE_STTXT_STRINGEOL);
				else if(sc.ch == '\"' && sc.chPrev != '$')
					sc.ForwardSetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_STRINGEOL: {
				if(sc.atLineStart)
					sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_CHARACTER: {
				if(setHexNumber.Contains(sc.ch))
					sc.SetState(SCE_STTXT_HEXNUMBER);
				else if(setDataTime.Contains(sc.ch))
					sc.SetState(SCE_STTXT_DATETIME);
				else sc.SetState(SCE_STTXT_DEFAULT);
				break;
			}
			case SCE_STTXT_IDENTIFIER: {
				if(!setWord.Contains(sc.ch))
					ClassifySTTXTWord(keywordlists, sc);
				break;
			}
		}

		if(sc.state == SCE_STTXT_DEFAULT)
		{
			if(IsADigit(sc.ch))
				sc.SetState(SCE_STTXT_NUMBER);
			else if (setWordStart.Contains(sc.ch))
				sc.SetState(SCE_STTXT_IDENTIFIER);
			else if (sc.Match('/', '/'))
				sc.SetState(SCE_STTXT_COMMENTLINE);
			else if(sc.Match('(', '*'))
				sc.SetState(SCE_STTXT_COMMENT);
			else if (sc.ch == '{')
				sc.SetState(SCE_STTXT_PRAGMA);
			else if (sc.ch == '\'')
				sc.SetState(SCE_STTXT_STRING1);
			else if (sc.ch == '\"')
				sc.SetState(SCE_STTXT_STRING2);
			else if(sc.ch == '#')
				sc.SetState(SCE_STTXT_CHARACTER);
			else if (setOperator.Contains(sc.ch))
				sc.SetState(SCE_STTXT_OPERATOR);
		}
 	}

	if (sc.state == SCE_STTXT_IDENTIFIER && setWord.Contains(sc.chPrev))
		ClassifySTTXTWord(keywordlists, sc);

	sc.Complete();
}

static const char * const STTXTWordListDesc[] = {
	"Keywords",
	"Types",
	"Functions",
	"FB",
	"Local_Var",
	"Local_Pragma",
	0
};

static bool IsCommentLine(Sci_Position line, Accessor &styler, bool type)
{
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eolPos = styler.LineStart(line + 1) - 1;

	for (Sci_Position i = pos; i < eolPos; i++)
	{
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);

		if(type) {
			 if (ch == '/' && chNext == '/' && style == SCE_STTXT_COMMENTLINE)
				return true;
		}
		else if (ch == '(' && chNext == '*' && style == SCE_STTXT_COMMENT)
			break;

		if (!IsASpaceOrTab(ch))
			return false;
	}

	for (Sci_Position i = eolPos-2; i>pos; i--)
	{
		char ch = styler[i];
		char chPrev = styler.SafeGetCharAt(i-1);
		int style = styler.StyleAt(i);

		if(ch == ')' && chPrev == '*' && style == SCE_STTXT_COMMENT)
			return true;
		if(!IsASpaceOrTab(ch))
			return false;
	}

	return false;
}

static bool IsPragmaLine(Sci_Position line, Accessor &styler)
{
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eolPos = styler.LineStart(line+1) - 1;

	for (Sci_Position i = pos ; i < eolPos ; i++)
	{
		char ch = styler[i];
		int style = styler.StyleAt(i);

		if(ch == '{' && style == SCE_STTXT_PRAGMA)
			return true;
		else if (!IsASpaceOrTab(ch))
			return false;
	}
	return false;
}

static void GetRangeUpper(Sci_PositionU start,Sci_PositionU end,Accessor &styler,char *s,Sci_PositionU len)
{
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(toupper(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static void ClassifySTTXTWordFoldPoint(int &levelCurrent,Sci_PositionU lastStart,
									 Sci_PositionU currentPos, Accessor &styler)
{
	char s[256];
	GetRangeUpper(lastStart, currentPos, styler, s, sizeof(s));

	// See Table C.2 - Keywords
	if (!strcmp(s, "ACTION") ||
		!strcmp(s, "CASE") ||
		!strcmp(s, "CONFIGURATION") ||
		!strcmp(s, "FOR") ||
		!strcmp(s, "FUNCTION") ||
		!strcmp(s, "FUNCTION_BLOCK") ||
		!strcmp(s, "IF") ||
		!strcmp(s, "INITIAL_STEP") ||
		!strcmp(s, "REPEAT") ||
		!strcmp(s, "RESOURCE") ||
		!strcmp(s, "STEP") ||
		!strcmp(s, "STRUCT") ||
		!strcmp(s, "TRANSITION") ||
		!strcmp(s, "TYPE") ||
		!strcmp(s, "VAR") ||
		!strcmp(s, "VAR_INPUT") ||
		!strcmp(s, "VAR_OUTPUT") ||
		!strcmp(s, "VAR_IN_OUT") ||
		!strcmp(s, "VAR_TEMP") ||
		!strcmp(s, "VAR_EXTERNAL") ||
		!strcmp(s, "VAR_ACCESS") ||
		!strcmp(s, "VAR_CONFIG") ||
		!strcmp(s, "VAR_GLOBAL") ||
		!strcmp(s, "WHILE"))
	{
		levelCurrent++;
	}
	else if (!strcmp(s, "END_ACTION") ||
		!strcmp(s, "END_CASE") ||
		!strcmp(s, "END_CONFIGURATION") ||
		!strcmp(s, "END_FOR") ||
		!strcmp(s, "END_FUNCTION") ||
		!strcmp(s, "END_FUNCTION_BLOCK") ||
		!strcmp(s, "END_IF") ||
		!strcmp(s, "END_REPEAT") ||
		!strcmp(s, "END_RESOURCE") ||
		!strcmp(s, "END_STEP") ||
		!strcmp(s, "END_STRUCT") ||
		!strcmp(s, "END_TRANSITION") ||
		!strcmp(s, "END_TYPE") ||
		!strcmp(s, "END_VAR") ||
		!strcmp(s, "END_WHILE"))
	{
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}

static void FoldSTTXTDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[],Accessor &styler)
{
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	Sci_Position lastStart = 0;

	CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);

	for (Sci_PositionU i = startPos; i < endPos; i++)
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && style == SCE_STTXT_COMMENT) {
			if(stylePrev != SCE_STTXT_COMMENT)
				levelCurrent++;
			else if(styleNext != SCE_STTXT_COMMENT && !atEOL)
				levelCurrent--;
		}
		if ( foldComment && atEOL && ( IsCommentLine(lineCurrent, styler,false)
			|| IsCommentLine(lineCurrent,styler,true))) {
 			if(!IsCommentLine(lineCurrent-1, styler,true) && IsCommentLine(lineCurrent+1, styler,true))
				levelCurrent++;
			if (IsCommentLine(lineCurrent-1, styler,true) && !IsCommentLine(lineCurrent+1, styler,true))
				levelCurrent--;
			if (!IsCommentLine(lineCurrent-1, styler,false) && IsCommentLine(lineCurrent+1, styler,false))
				levelCurrent++;
			if (IsCommentLine(lineCurrent-1, styler,false) && !IsCommentLine(lineCurrent+1, styler,false))
				levelCurrent--;
		}
		if(foldPreprocessor && atEOL && IsPragmaLine(lineCurrent, styler)) {
			if(!IsPragmaLine(lineCurrent-1, styler) && IsPragmaLine(lineCurrent+1, styler ))
				levelCurrent++;
			else if(IsPragmaLine(lineCurrent-1, styler) && !IsPragmaLine(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (stylePrev != SCE_STTXT_KEYWORD && style == SCE_STTXT_KEYWORD) {
				lastStart = i;
		}
		if(stylePrev == SCE_STTXT_KEYWORD) {
			if(setWord.Contains(ch) && !setWord.Contains(chNext))
				ClassifySTTXTWordFoldPoint(levelCurrent,lastStart, i, styler);
		}
		if (!IsASpace(ch)) {
			visibleChars++;
		}
		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
				styler.SetLevel(lineCurrent, lev);

			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}

		// If we didn't reach the EOL in previous loop, store line level and whitespace information.
		// The rest will be filled in later...
		int lev = levelPrev;
		if (visibleChars == 0 && foldCompact)
			lev |= SC_FOLDLEVELWHITEFLAG;
		styler.SetLevel(lineCurrent, lev);
	}
}

extern const LexerModule lmSTTXT(SCLEX_STTXT, ColouriseSTTXTDoc, "fcST", FoldSTTXTDoc, STTXTWordListDesc);
