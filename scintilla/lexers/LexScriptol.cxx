// Scintilla source code edit control
/** @file LexScriptol.cxx
 ** Lexer for Scriptol.
 **/

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

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static void ClassifyWordSol(Sci_PositionU start, Sci_PositionU end, WordList &keywords, Accessor &styler, char *prevWord)
{
    char s[100] = "";
    bool wordIsNumber = isdigit(styler[start]) != 0;
    for (Sci_PositionU i = 0; i < end - start + 1 && i < 30; i++)
     {
           s[i] = styler[start + i];
           s[i + 1] = '\0';
     }
    char chAttr = SCE_SCRIPTOL_IDENTIFIER;
    if (0 == strcmp(prevWord, "class"))       chAttr = SCE_SCRIPTOL_CLASSNAME;
    else if (wordIsNumber)                    chAttr = SCE_SCRIPTOL_NUMBER;
    else if (keywords.InList(s))              chAttr = SCE_SCRIPTOL_KEYWORD;
    else for (Sci_PositionU i = 0; i < end - start + 1; i++)  // test dotted idents
    {
        if (styler[start + i] == '.')
        {
            styler.ColourTo(start + i - 1, chAttr);
            styler.ColourTo(start + i, SCE_SCRIPTOL_OPERATOR);
        }
    }
    styler.ColourTo(end, chAttr);
    strcpy(prevWord, s);
}

static bool IsSolComment(Accessor &styler, Sci_Position pos, Sci_Position len)
{
   if(len > 0)
   {
     char c = styler[pos];
     if(c == '`') return true;
     if(len > 1)
     {
        if(c == '/')
        {
          c = styler[pos + 1];
          if(c == '/') return true;
          if(c == '*') return true;
        }
     }
   }
   return false;
}

static bool IsSolStringStart(char ch)
{
    if (ch == '\'' || ch == '"')  return true;
    return false;
}

static bool IsSolWordStart(char ch)
{
    return (iswordchar(ch) && !IsSolStringStart(ch));
}


static int GetSolStringState(Accessor &styler, Sci_Position i, Sci_Position *nextIndex)
{
	char ch = styler.SafeGetCharAt(i);
	char chNext = styler.SafeGetCharAt(i + 1);

        if (ch != '\"' && ch != '\'')
        {
            *nextIndex = i + 1;
            return SCE_SCRIPTOL_DEFAULT;
	}
        // ch is either single or double quotes in string
        // code below seem non-sense but is here for future extensions
	if (ch == chNext && ch == styler.SafeGetCharAt(i + 2))
        {
          *nextIndex = i + 3;
          if(ch == '\"') return SCE_SCRIPTOL_TRIPLE;
          if(ch == '\'') return SCE_SCRIPTOL_TRIPLE;
          return SCE_SCRIPTOL_STRING;
	}
        else
        {
          *nextIndex = i + 1;
          if (ch == '"') return SCE_SCRIPTOL_STRING;
          else           return SCE_SCRIPTOL_STRING;
	}
}


static void ColouriseSolDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                            WordList *keywordlists[], Accessor &styler)
 {

	Sci_Position lengthDoc = startPos + length;
        char stringType = '\"';

	if (startPos > 0)
        {
            Sci_Position lineCurrent = styler.GetLine(startPos);
            if (lineCurrent > 0)
            {
              startPos = styler.LineStart(lineCurrent-1);
              if (startPos == 0) initStyle = SCE_SCRIPTOL_DEFAULT;
              else               initStyle = styler.StyleAt(startPos-1);
            }
	}

	styler.StartAt(startPos);

	WordList &keywords = *keywordlists[0];

	char prevWord[200];
	prevWord[0] = '\0';
        if (length == 0)  return;

	int state = initStyle & 31;

	Sci_Position nextIndex = 0;
        char chPrev  = ' ';
        char chPrev2 = ' ';
        char chNext  = styler[startPos];
	styler.StartSegment(startPos);
	for (Sci_Position i = startPos; i < lengthDoc; i++)
        {

       char ch = chNext;
       chNext = styler.SafeGetCharAt(i + 1);

       if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == lengthDoc))
       {
          if ((state == SCE_SCRIPTOL_DEFAULT) ||
              (state == SCE_SCRIPTOL_TRIPLE) ||
              (state == SCE_SCRIPTOL_COMMENTBLOCK))
          {
              styler.ColourTo(i, state);
          }
        }

        if (styler.IsLeadByte(ch))
         {
             chNext = styler.SafeGetCharAt(i + 2);
             chPrev  = ' ';
             chPrev2 = ' ';
             i += 1;
             continue;
         }

        if (state == SCE_SCRIPTOL_STRINGEOL)
         {
             if (ch != '\r' && ch != '\n')
             {
                    styler.ColourTo(i - 1, state);
                    state = SCE_SCRIPTOL_DEFAULT;
             }
         }

        if (state == SCE_SCRIPTOL_DEFAULT)
         {
            if (IsSolWordStart(ch))
            {
                 styler.ColourTo(i - 1, state);
                 state = SCE_SCRIPTOL_KEYWORD;
            }
            else if (ch == '`')
            {
                styler.ColourTo(i - 1, state);
                state = SCE_SCRIPTOL_COMMENTLINE;
            }
            else if (ch == '/')
            {
                styler.ColourTo(i - 1, state);
                if(chNext == '/') state = SCE_SCRIPTOL_CSTYLE;
                if(chNext == '*') state = SCE_SCRIPTOL_COMMENTBLOCK;
            }

            else if (IsSolStringStart(ch))
            {
               styler.ColourTo(i - 1, state);
               state = GetSolStringState(styler, i, &nextIndex);
               if(state == SCE_SCRIPTOL_STRING)
               {
                 stringType = ch;
               }
               if (nextIndex != i + 1)
               {
                   i = nextIndex - 1;
                   ch = ' ';
                   chPrev = ' ';
                   chNext = styler.SafeGetCharAt(i + 1);
               }
           }
            else if (isoperator(ch))
            {
                 styler.ColourTo(i - 1, state);
                 styler.ColourTo(i, SCE_SCRIPTOL_OPERATOR);
            }
          }
          else if (state == SCE_SCRIPTOL_KEYWORD)
          {
              if (!iswordchar(ch))
              {
                 ClassifyWordSol(styler.GetStartSegment(), i - 1, keywords, styler, prevWord);
                 state = SCE_SCRIPTOL_DEFAULT;
                 if (ch == '`')
                 {
                     state = chNext == '`' ? SCE_SCRIPTOL_PERSISTENT : SCE_SCRIPTOL_COMMENTLINE;
                 }
                 else if (IsSolStringStart(ch))
                 {
                    styler.ColourTo(i - 1, state);
                    state = GetSolStringState(styler, i, &nextIndex);
                    if (nextIndex != i + 1)
                    {
                       i = nextIndex - 1;
                       ch = ' ';
                       chPrev = ' ';
                       chNext = styler.SafeGetCharAt(i + 1);
                     }
                 }
                 else if (isoperator(ch))
                 {
                     styler.ColourTo(i, SCE_SCRIPTOL_OPERATOR);
                 }
             }
          }
          else
          {
            if (state == SCE_SCRIPTOL_COMMENTLINE ||
                state == SCE_SCRIPTOL_PERSISTENT ||
                state == SCE_SCRIPTOL_CSTYLE)
            {
                 if (ch == '\r' || ch == '\n')
                 {
                     styler.ColourTo(i - 1, state);
                     state = SCE_SCRIPTOL_DEFAULT;
                 }
            }
            else if(state == SCE_SCRIPTOL_COMMENTBLOCK)
            {
              if(chPrev == '*' && ch == '/')
              {
                styler.ColourTo(i, state);
                state = SCE_SCRIPTOL_DEFAULT;
              }
            }
            else if ((state == SCE_SCRIPTOL_STRING) ||
                     (state == SCE_SCRIPTOL_CHARACTER))
            {
               if ((ch == '\r' || ch == '\n') && (chPrev != '\\'))
                {
                    styler.ColourTo(i - 1, state);
                    state = SCE_SCRIPTOL_STRINGEOL;
                }
                else if (ch == '\\')
                {
                   if (chNext == '\"' || chNext == '\'' || chNext == '\\')
                   {
                        i++;
                        ch = chNext;
                        chNext = styler.SafeGetCharAt(i + 1);
                   }
                 }
                else if ((ch == '\"') || (ch == '\''))
                {
                    // must match the entered quote type
                    if(ch == stringType)
                    {
                      styler.ColourTo(i, state);
                      state = SCE_SCRIPTOL_DEFAULT;
                    }
                 }
             }
             else if (state == SCE_SCRIPTOL_TRIPLE)
             {
                if ((ch == '\'' && chPrev == '\'' && chPrev2 == '\'') ||
                    (ch == '\"' && chPrev == '\"' && chPrev2 == '\"'))
                 {
                    styler.ColourTo(i, state);
                    state = SCE_SCRIPTOL_DEFAULT;
                 }
             }

           }
          chPrev2 = chPrev;
          chPrev = ch;
	}
        if (state == SCE_SCRIPTOL_KEYWORD)
        {
            ClassifyWordSol(styler.GetStartSegment(),
                 lengthDoc-1, keywords, styler, prevWord);
	}
        else
        {
            styler.ColourTo(lengthDoc-1, state);
	}
}

static void FoldSolDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
						   WordList *[], Accessor &styler)
 {
	Sci_Position lengthDoc = startPos + length;

	Sci_Position lineCurrent = styler.GetLine(startPos);
	if (startPos > 0)
        {
          if (lineCurrent > 0)
          {
               lineCurrent--;
               startPos = styler.LineStart(lineCurrent);
               if (startPos == 0)
                    initStyle = SCE_SCRIPTOL_DEFAULT;
               else
                    initStyle = styler.StyleAt(startPos-1);
           }
	}
	int state = initStyle & 31;
	int spaceFlags = 0;
        int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsSolComment);
        if (state == SCE_SCRIPTOL_TRIPLE)
             indentCurrent |= SC_FOLDLEVELWHITEFLAG;
	char chNext = styler[startPos];
	for (Sci_Position i = startPos; i < lengthDoc; i++)
         {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i) & 31;

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == lengthDoc))
                {
                   int lev = indentCurrent;
                   int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsSolComment);
                   if (style == SCE_SCRIPTOL_TRIPLE)
                        indentNext |= SC_FOLDLEVELWHITEFLAG;
                   if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG))
                    {
                        // Only non whitespace lines can be headers
                        if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
                        {
                              lev |= SC_FOLDLEVELHEADERFLAG;
                        }
                        else if (indentNext & SC_FOLDLEVELWHITEFLAG)
                        {
                             // Line after is blank so check the next - maybe should continue further?
                             int spaceFlags2 = 0;
                             int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsSolComment);
                             if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK))
                             {
                                   lev |= SC_FOLDLEVELHEADERFLAG;
                              }
                        }
                    }
                   indentCurrent = indentNext;
                   styler.SetLevel(lineCurrent, lev);
                   lineCurrent++;
		}
	}
}

LexerModule lmScriptol(SCLEX_SCRIPTOL, ColouriseSolDoc, "scriptol", FoldSolDoc);
