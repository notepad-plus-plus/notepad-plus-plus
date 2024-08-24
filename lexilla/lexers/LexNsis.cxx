// Scintilla source code edit control
/** @file LexNsis.cxx
 ** Lexer for NSIS
 **/
// Copyright 2003 - 2005 by Angelo Mandato <angelo [at] spaceblue [dot] com>
// Last Updated: 03/13/2005
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

/*
// located in SciLexer.h
#define SCLEX_NSIS 43

#define SCE_NSIS_DEFAULT 0
#define SCE_NSIS_COMMENT 1
#define SCE_NSIS_STRINGDQ 2
#define SCE_NSIS_STRINGLQ 3
#define SCE_NSIS_STRINGRQ 4
#define SCE_NSIS_FUNCTION 5
#define SCE_NSIS_VARIABLE 6
#define SCE_NSIS_LABEL 7
#define SCE_NSIS_USERDEFINED 8
#define SCE_NSIS_SECTIONDEF 9
#define SCE_NSIS_SUBSECTIONDEF 10
#define SCE_NSIS_IFDEFINEDEF 11
#define SCE_NSIS_MACRODEF 12
#define SCE_NSIS_STRINGVAR 13
#define SCE_NSIS_NUMBER 14
// ADDED for Scintilla v1.63
#define SCE_NSIS_SECTIONGROUP 15
#define SCE_NSIS_PAGEEX 16
#define SCE_NSIS_FUNCTIONDEF 17
#define SCE_NSIS_COMMENTBOX 18
*/

static bool isNsisNumber(char ch)
{
  return (ch >= '0' && ch <= '9');
}

static bool isNsisChar(char ch)
{
  return (ch == '.' ) || (ch == '_' ) || isNsisNumber(ch) || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool isNsisLetter(char ch)
{
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool NsisNextLineHasElse(Sci_PositionU start, Sci_PositionU end, Accessor &styler)
{
  Sci_Position nNextLine = -1;
  for( Sci_PositionU i = start; i < end; i++ )
  {
    char cNext = styler.SafeGetCharAt( i );
    if( cNext == '\n' )
    {
      nNextLine = i+1;
      break;
    }
  }

  if( nNextLine == -1 ) // We never found the next line...
    return false;

  for( Sci_PositionU firstChar = nNextLine; firstChar < end; firstChar++ )
  {
    char cNext = styler.SafeGetCharAt( firstChar );
    if( cNext == ' ' )
      continue;
    if( cNext == '\t' )
      continue;
    if( cNext == '!' )
    {
      if( styler.Match(firstChar, "!else") )
        return true;
    }
    break;
  }

  return false;
}

static int NsisCmp( const char *s1, const char *s2, bool bIgnoreCase )
{
  if( bIgnoreCase )
     return CompareCaseInsensitive( s1, s2);

  return strcmp( s1, s2 );
}

static int calculateFoldNsis(Sci_PositionU start, Sci_PositionU end, int foldlevel, Accessor &styler, bool bElse, bool foldUtilityCmd )
{
  int style = styler.StyleAt(end);

  // If the word is too long, it is not what we are looking for
  if( end - start > 20 )
    return foldlevel;

  if( foldUtilityCmd )
  {
    // Check the style at this point, if it is not valid, then return zero
    if( style != SCE_NSIS_FUNCTIONDEF && style != SCE_NSIS_SECTIONDEF &&
        style != SCE_NSIS_SUBSECTIONDEF && style != SCE_NSIS_IFDEFINEDEF &&
        style != SCE_NSIS_MACRODEF && style != SCE_NSIS_SECTIONGROUP &&
        style != SCE_NSIS_PAGEEX )
          return foldlevel;
  }
  else
  {
    if( style != SCE_NSIS_FUNCTIONDEF && style != SCE_NSIS_SECTIONDEF &&
        style != SCE_NSIS_SUBSECTIONDEF && style != SCE_NSIS_SECTIONGROUP &&
        style != SCE_NSIS_PAGEEX )
          return foldlevel;
  }

  int newFoldlevel = foldlevel;
  bool bIgnoreCase = false;
  // property nsis.ignorecase
  // Set to 1 to ignore case for NSIS.
  if( styler.GetPropertyInt("nsis.ignorecase") == 1 )
    bIgnoreCase = true;

  char s[20]; // The key word we are looking for has atmost 13 characters
  s[0] = '\0';
  for (Sci_PositionU i = 0; i < end - start + 1 && i < 19; i++)
	{
		s[i] = static_cast<char>( styler[ start + i ] );
		s[i + 1] = '\0';
	}

  if( s[0] == '!' )
  {
    if( NsisCmp(s, "!ifndef", bIgnoreCase) == 0 || NsisCmp(s, "!ifdef", bIgnoreCase ) == 0 || NsisCmp(s, "!ifmacrodef", bIgnoreCase ) == 0 || NsisCmp(s, "!ifmacrondef", bIgnoreCase ) == 0 || NsisCmp(s, "!if", bIgnoreCase ) == 0 || NsisCmp(s, "!macro", bIgnoreCase ) == 0 )
      newFoldlevel++;
    else if( NsisCmp(s, "!endif", bIgnoreCase) == 0 || NsisCmp(s, "!macroend", bIgnoreCase ) == 0 )
      newFoldlevel--;
    else if( bElse && NsisCmp(s, "!else", bIgnoreCase) == 0 )
      newFoldlevel++;
  }
  else
  {
    if( NsisCmp(s, "Section", bIgnoreCase ) == 0 || NsisCmp(s, "SectionGroup", bIgnoreCase ) == 0 || NsisCmp(s, "Function", bIgnoreCase) == 0 || NsisCmp(s, "SubSection", bIgnoreCase ) == 0 || NsisCmp(s, "PageEx", bIgnoreCase ) == 0 )
      newFoldlevel++;
    else if( NsisCmp(s, "SectionGroupEnd", bIgnoreCase ) == 0 || NsisCmp(s, "SubSectionEnd", bIgnoreCase ) == 0 || NsisCmp(s, "FunctionEnd", bIgnoreCase) == 0 || NsisCmp(s, "SectionEnd", bIgnoreCase ) == 0 || NsisCmp(s, "PageExEnd", bIgnoreCase ) == 0 )
      newFoldlevel--;
  }

  return newFoldlevel;
}

static int classifyWordNsis(Sci_PositionU start, Sci_PositionU end, WordList *keywordLists[], Accessor &styler )
{
  bool bIgnoreCase = false;
  if( styler.GetPropertyInt("nsis.ignorecase") == 1 )
    bIgnoreCase = true;

  bool bUserVars = false;
  // property nsis.uservars
  // Set to 1 to recognise user defined variables in NSIS.
  if( styler.GetPropertyInt("nsis.uservars") == 1 )
    bUserVars = true;

	char s[100];
	s[0] = '\0';
	s[1] = '\0';

	WordList &Functions = *keywordLists[0];
	WordList &Variables = *keywordLists[1];
	WordList &Lables = *keywordLists[2];
	WordList &UserDefined = *keywordLists[3];

	for (Sci_PositionU i = 0; i < end - start + 1 && i < 99; i++)
	{
    if( bIgnoreCase )
      s[i] = static_cast<char>( tolower(styler[ start + i ] ) );
    else
		  s[i] = static_cast<char>( styler[ start + i ] );
		s[i + 1] = '\0';
	}

	// Check for special words...
	if( NsisCmp(s, "!macro", bIgnoreCase ) == 0 || NsisCmp(s, "!macroend", bIgnoreCase) == 0 ) // Covers !macro and !macroend
		return SCE_NSIS_MACRODEF;

	if( NsisCmp(s, "!ifdef", bIgnoreCase ) == 0 ||  NsisCmp(s, "!ifndef", bIgnoreCase) == 0 ||  NsisCmp(s, "!endif", bIgnoreCase) == 0 ) // Covers !ifdef, !ifndef and !endif
		return SCE_NSIS_IFDEFINEDEF;

	if( NsisCmp(s, "!if", bIgnoreCase ) == 0 || NsisCmp(s, "!else", bIgnoreCase )  == 0 ) // Covers !if and else
		return SCE_NSIS_IFDEFINEDEF;

	if (NsisCmp(s, "!ifmacrodef", bIgnoreCase ) == 0 || NsisCmp(s, "!ifmacrondef", bIgnoreCase )  == 0 ) // Covers !ifmacrodef and !ifnmacrodef
		return SCE_NSIS_IFDEFINEDEF;

  if( NsisCmp(s, "SectionGroup", bIgnoreCase) == 0 || NsisCmp(s, "SectionGroupEnd", bIgnoreCase) == 0 ) // Covers SectionGroup and SectionGroupEnd
    return SCE_NSIS_SECTIONGROUP;

	if( NsisCmp(s, "Section", bIgnoreCase ) == 0 || NsisCmp(s, "SectionEnd", bIgnoreCase) == 0 ) // Covers Section and SectionEnd
		return SCE_NSIS_SECTIONDEF;

	if( NsisCmp(s, "SubSection", bIgnoreCase) == 0 || NsisCmp(s, "SubSectionEnd", bIgnoreCase) == 0 ) // Covers SubSection and SubSectionEnd
		return SCE_NSIS_SUBSECTIONDEF;

  if( NsisCmp(s, "PageEx", bIgnoreCase) == 0 || NsisCmp(s, "PageExEnd", bIgnoreCase) == 0 ) // Covers PageEx and PageExEnd
    return SCE_NSIS_PAGEEX;

	if( NsisCmp(s, "Function", bIgnoreCase) == 0 || NsisCmp(s, "FunctionEnd", bIgnoreCase) == 0 ) // Covers Function and FunctionEnd
		return SCE_NSIS_FUNCTIONDEF;

	if ( Functions.InList(s) )
		return SCE_NSIS_FUNCTION;

	if ( Variables.InList(s) )
		return SCE_NSIS_VARIABLE;

	if ( Lables.InList(s) )
		return SCE_NSIS_LABEL;

	if( UserDefined.InList(s) )
		return SCE_NSIS_USERDEFINED;

	if( strlen(s) > 3 )
	{
		if( s[1] == '{' && s[strlen(s)-1] == '}' )
			return SCE_NSIS_VARIABLE;
	}

  // See if the variable is a user defined variable
  if( s[0] == '$' && bUserVars )
  {
    bool bHasSimpleNsisChars = true;
    for (Sci_PositionU j = 1; j < end - start + 1 && j < 99; j++)
	  {
      if( !isNsisChar( s[j] ) )
      {
        bHasSimpleNsisChars = false;
        break;
      }
	  }

    if( bHasSimpleNsisChars )
      return SCE_NSIS_VARIABLE;
  }

  // To check for numbers
  if( isNsisNumber( s[0] ) )
  {
    bool bHasSimpleNsisNumber = true;
    for (Sci_PositionU j = 1; j < end - start + 1 && j < 99; j++)
	  {
      if( !isNsisNumber( s[j] ) )
      {
        bHasSimpleNsisNumber = false;
        break;
      }
	  }

    if( bHasSimpleNsisNumber )
      return SCE_NSIS_NUMBER;
  }

	return SCE_NSIS_DEFAULT;
}

static void ColouriseNsisDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *keywordLists[], Accessor &styler)
{
	int state = SCE_NSIS_DEFAULT;
  if( startPos > 0 )
    state = styler.StyleAt(startPos-1); // Use the style from the previous line, usually default, but could be commentbox

	styler.StartAt( startPos );
	styler.GetLine( startPos );

	Sci_PositionU nLengthDoc = startPos + length;
	styler.StartSegment( startPos );

	char cCurrChar;
	bool bVarInString = false;
  bool bClassicVarInString = false;

	Sci_PositionU i;
	for( i = startPos; i < nLengthDoc; i++ )
	{
		cCurrChar = styler.SafeGetCharAt( i );
		char cNextChar = styler.SafeGetCharAt(i+1);

		switch(state)
		{
			case SCE_NSIS_DEFAULT:
				if( cCurrChar == ';' || cCurrChar == '#' ) // we have a comment line
				{
					styler.ColourTo(i-1, state );
					state = SCE_NSIS_COMMENT;
					break;
				}
				if( cCurrChar == '"' )
				{
					styler.ColourTo(i-1, state );
					state = SCE_NSIS_STRINGDQ;
					bVarInString = false;
          bClassicVarInString = false;
					break;
				}
				if( cCurrChar == '\'' )
				{
					styler.ColourTo(i-1, state );
					state = SCE_NSIS_STRINGRQ;
					bVarInString = false;
          bClassicVarInString = false;
					break;
				}
				if( cCurrChar == '`' )
				{
					styler.ColourTo(i-1, state );
					state = SCE_NSIS_STRINGLQ;
					bVarInString = false;
          bClassicVarInString = false;
					break;
				}

				// NSIS KeyWord,Function, Variable, UserDefined:
				if( cCurrChar == '$' || isNsisChar(cCurrChar) || cCurrChar == '!' )
				{
					styler.ColourTo(i-1,state);
				  state = SCE_NSIS_FUNCTION;

          // If it is a number, we must check and set style here first...
          if( isNsisNumber(cCurrChar) && (cNextChar == '\t' || cNextChar == ' ' || cNextChar == '\r' || cNextChar == '\n' ) )
              styler.ColourTo( i, SCE_NSIS_NUMBER);

					break;
				}

        if( cCurrChar == '/' && cNextChar == '*' )
        {
          styler.ColourTo(i-1,state);
          state = SCE_NSIS_COMMENTBOX;
          break;
        }

				break;
			case SCE_NSIS_COMMENT:
				if( cNextChar == '\n' || cNextChar == '\r' )
        {
          // Special case:
          if( cCurrChar == '\\' )
          {
            styler.ColourTo(i-2,state);
            styler.ColourTo(i,SCE_NSIS_DEFAULT);
          }
          else
          {
				    styler.ColourTo(i,state);
            state = SCE_NSIS_DEFAULT;
          }
        }
				break;
			case SCE_NSIS_STRINGDQ:
      case SCE_NSIS_STRINGLQ:
      case SCE_NSIS_STRINGRQ:

        if( styler.SafeGetCharAt(i-1) == '\\' && styler.SafeGetCharAt(i-2) == '$' )
          break; // Ignore the next character, even if it is a quote of some sort

        if( cCurrChar == '"' && state == SCE_NSIS_STRINGDQ )
				{
					styler.ColourTo(i,state);
				  state = SCE_NSIS_DEFAULT;
          break;
				}

        if( cCurrChar == '`' && state == SCE_NSIS_STRINGLQ )
        {
					styler.ColourTo(i,state);
				  state = SCE_NSIS_DEFAULT;
          break;
				}

        if( cCurrChar == '\'' && state == SCE_NSIS_STRINGRQ )
				{
					styler.ColourTo(i,state);
				  state = SCE_NSIS_DEFAULT;
          break;
				}

        if( cNextChar == '\r' || cNextChar == '\n' )
        {
          Sci_Position nCurLine = styler.GetLine(i+1);
          Sci_Position nBack = i;
          // We need to check if the previous line has a \ in it...
          bool bNextLine = false;

          while( nBack > 0 )
          {
            if( styler.GetLine(nBack) != nCurLine )
              break;

            char cTemp = styler.SafeGetCharAt(nBack, 'a'); // Letter 'a' is safe here

            if( cTemp == '\\' )
            {
              bNextLine = true;
              break;
            }
            if( cTemp != '\r' && cTemp != '\n' && cTemp != '\t' && cTemp != ' ' )
              break;

            nBack--;
          }

          if( bNextLine )
          {
            styler.ColourTo(i+1,state);
          }
          if( bNextLine == false )
          {
            styler.ColourTo(i,state);
				    state = SCE_NSIS_DEFAULT;
          }
        }
				break;

			case SCE_NSIS_FUNCTION:

				// NSIS KeyWord:
        if( cCurrChar == '$' )
          state = SCE_NSIS_DEFAULT;
        else if( cCurrChar == '\\' && (cNextChar == 'n' || cNextChar == 'r' || cNextChar == 't' ) )
          state = SCE_NSIS_DEFAULT;
				else if( (isNsisChar(cCurrChar) && !isNsisChar( cNextChar) && cNextChar != '}') || cCurrChar == '}' )
				{
					state = classifyWordNsis( styler.GetStartSegment(), i, keywordLists, styler );
					styler.ColourTo( i, state);
					state = SCE_NSIS_DEFAULT;
				}
				else if( !isNsisChar( cCurrChar ) && cCurrChar != '{' && cCurrChar != '}' )
				{
          if( classifyWordNsis( styler.GetStartSegment(), i-1, keywordLists, styler) == SCE_NSIS_NUMBER )
             styler.ColourTo( i-1, SCE_NSIS_NUMBER );

					state = SCE_NSIS_DEFAULT;

					if( cCurrChar == '"' )
					{
						state = SCE_NSIS_STRINGDQ;
						bVarInString = false;
            bClassicVarInString = false;
					}
					else if( cCurrChar == '`' )
					{
						state = SCE_NSIS_STRINGLQ;
						bVarInString = false;
            bClassicVarInString = false;
					}
					else if( cCurrChar == '\'' )
					{
						state = SCE_NSIS_STRINGRQ;
						bVarInString = false;
            bClassicVarInString = false;
					}
					else if( cCurrChar == '#' || cCurrChar == ';' )
          {
						state = SCE_NSIS_COMMENT;
          }
				}
				break;
      case SCE_NSIS_COMMENTBOX:

        if( styler.SafeGetCharAt(i-1) == '*' && cCurrChar == '/' )
        {
          styler.ColourTo(i,state);
          state = SCE_NSIS_DEFAULT;
        }
        break;
		}

		if( state == SCE_NSIS_COMMENT || state == SCE_NSIS_COMMENTBOX )
		{
			styler.ColourTo(i,state);
		}
		else if( state == SCE_NSIS_STRINGDQ || state == SCE_NSIS_STRINGLQ || state == SCE_NSIS_STRINGRQ )
		{
      bool bIngoreNextDollarSign = false;
      bool bUserVars = false;
      if( styler.GetPropertyInt("nsis.uservars") == 1 )
        bUserVars = true;

      if( bVarInString && cCurrChar == '$' )
      {
        bVarInString = false;
        bIngoreNextDollarSign = true;
      }
      else if( bVarInString && cCurrChar == '\\' && (cNextChar == 'n' || cNextChar == 'r' || cNextChar == 't' || cNextChar == '"' || cNextChar == '`' || cNextChar == '\'' ) )
      {
        styler.ColourTo( i+1, SCE_NSIS_STRINGVAR);
        bVarInString = false;
        bIngoreNextDollarSign = false;
      }

      // Covers "$INSTDIR and user vars like $MYVAR"
      else if( bVarInString && !isNsisChar(cNextChar) )
      {
        int nWordState = classifyWordNsis( styler.GetStartSegment(), i, keywordLists, styler);
				if( nWordState == SCE_NSIS_VARIABLE )
					styler.ColourTo( i, SCE_NSIS_STRINGVAR);
        else if( bUserVars )
          styler.ColourTo( i, SCE_NSIS_STRINGVAR);
        bVarInString = false;
      }
      // Covers "${TEST}..."
      else if( bClassicVarInString && cNextChar == '}' )
      {
        styler.ColourTo( i+1, SCE_NSIS_STRINGVAR);
				bClassicVarInString = false;
      }

      // Start of var in string
			if( !bIngoreNextDollarSign && cCurrChar == '$' && cNextChar == '{' )
			{
				styler.ColourTo( i-1, state);
				bClassicVarInString = true;
        bVarInString = false;
			}
      else if( !bIngoreNextDollarSign && cCurrChar == '$' )
      {
        styler.ColourTo( i-1, state);
        bVarInString = true;
        bClassicVarInString = false;
      }
		}
	}

  // Colourise remaining document
	styler.ColourTo(nLengthDoc-1,state);
}

static void FoldNsisDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler)
{
	// No folding enabled, no reason to continue...
	if( styler.GetPropertyInt("fold") == 0 )
		return;

  bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) == 1;
  bool foldUtilityCmd = styler.GetPropertyInt("nsis.foldutilcmd", 1) == 1;
  bool blockComment = false;

  Sci_Position lineCurrent = styler.GetLine(startPos);
  Sci_PositionU safeStartPos = styler.LineStart( lineCurrent );

  bool bArg1 = true;
  Sci_Position nWordStart = -1;

  int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelNext = levelCurrent;
  int style = styler.StyleAt(safeStartPos);
  if( style == SCE_NSIS_COMMENTBOX )
  {
    if( styler.SafeGetCharAt(safeStartPos) == '/' && styler.SafeGetCharAt(safeStartPos+1) == '*' )
      levelNext++;
    blockComment = true;
  }

  for (Sci_PositionU i = safeStartPos; i < startPos + length; i++)
	{
    char chCurr = styler.SafeGetCharAt(i);
    style = styler.StyleAt(i);
    if( blockComment && style != SCE_NSIS_COMMENTBOX )
    {
      levelNext--;
      blockComment = false;
    }
    else if( !blockComment && style == SCE_NSIS_COMMENTBOX )
    {
      levelNext++;
      blockComment = true;
    }

    if( bArg1 && !blockComment)
    {
      if( nWordStart == -1 && (isNsisLetter(chCurr) || chCurr == '!') )
      {
        nWordStart = i;
      }
      else if( isNsisLetter(chCurr) == false && nWordStart > -1 )
      {
        int newLevel = calculateFoldNsis( nWordStart, i-1, levelNext, styler, foldAtElse, foldUtilityCmd );

        if( newLevel == levelNext )
        {
          if( foldAtElse && foldUtilityCmd )
          {
            if( NsisNextLineHasElse(i, startPos + length, styler) )
              levelNext--;
          }
        }
        else
          levelNext = newLevel;
        bArg1 = false;
      }
    }

    if( chCurr == '\n' )
    {
      if( bArg1 && foldAtElse && foldUtilityCmd && !blockComment )
      {
        if( NsisNextLineHasElse(i, startPos + length, styler) )
          levelNext--;
      }

      // If we are on a new line...
      int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
      if (levelUse < levelNext )
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
				styler.SetLevel(lineCurrent, lev);

			lineCurrent++;
			levelCurrent = levelNext;
      bArg1 = true; // New line, lets look at first argument again
      nWordStart = -1;
    }
  }

	int levelUse = levelCurrent;
	int lev = levelUse | levelNext << 16;
	if (levelUse < levelNext)
		lev |= SC_FOLDLEVELHEADERFLAG;
	if (lev != styler.LevelAt(lineCurrent))
		styler.SetLevel(lineCurrent, lev);
}

static const char * const nsisWordLists[] = {
	"Functions",
	"Variables",
	"Lables",
	"UserDefined",
	0, };


extern const LexerModule lmNsis(SCLEX_NSIS, ColouriseNsisDoc, "nsis", FoldNsisDoc, nsisWordLists);

