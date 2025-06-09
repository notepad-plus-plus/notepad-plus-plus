// Scintilla source code edit control
/** @file LexMaxima.cxx
 ** Lexer for Maxima (http://maxima.sourceforge.net).
 ** Written by Gunter Königsmann based on the lisp lexer by Alexey Yutkin and Neil Hodgson .
 **/
// Copyright 2018 by Gunter Königsmann <wxMaxima@physikbuch.de>
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

static inline bool isMaximaoperator(char ch) {
  return (ch == '\'' || ch == '`' || ch == '(' ||
	  ch == ')'  || ch == '[' || ch == ']' ||
	  ch == '{'  || ch == '}' || ch == '!' ||
	  ch == '*'  || ch == '/' || ch == '^' ||
	  ch == ','  || ch == ':' || ch == '+' ||
	  ch == '-');
}

static void ColouriseMaximaDoc(Sci_PositionU startPos, Sci_Position length, int lastStyle,
			       WordList *[],
			       Accessor &styler) {

  styler.StartAt(startPos);

  Sci_Position lengthDoc = startPos + length;
  styler.StartSegment(startPos);

  Sci_Position i = startPos;

  // If we are in the middle of a comment we go back to its start before highlighting
  if(lastStyle == SCE_MAXIMA_COMMENT)
    {
      while((i>0) &&
	    !((styler.SafeGetCharAt(i+1) == '*') && (styler.SafeGetCharAt(i) == '/')))
	i--;
    }

  for (; i < lengthDoc; i++) {
    char ch = styler.SafeGetCharAt(i);
    char chNext = styler.SafeGetCharAt(i + 1);

    if (styler.IsLeadByte(ch))
      continue;

    // Handle comments.
    // Comments start with /* and end with */
    if((ch == '/') && (chNext == '*'))
      {
	i++;i++;

	chNext = styler.SafeGetCharAt(i);
	for (; i < lengthDoc; i++)
	  {
	    ch = chNext;
	    chNext = styler.SafeGetCharAt(i + 1);
	    if((ch == '*') && (chNext == '/'))
	      {
		i++;
		i++;
		break;
	      }
	  }
	if(i > lengthDoc)
	  i = lengthDoc;
	i--;
	styler.ColourTo(i, SCE_MAXIMA_COMMENT);
	continue;
      }

    // Handle Operators
    if(isMaximaoperator(ch))
      {
	styler.ColourTo(i, SCE_MAXIMA_OPERATOR);
	continue;
      }

    // Handle command endings.
    if((ch == '$') || (ch == ';'))
      {
	styler.ColourTo(i, SCE_MAXIMA_COMMANDENDING);
	continue;
      }

    // Handle numbers. Numbers always begin with a digit.
    if(IsASCII(ch) && isdigit(ch))
      {
	i++;
	for (; i < lengthDoc; i++)
	  {
	    ch = chNext;
	    chNext = styler.SafeGetCharAt(i + 1);

	    if(ch == '.')
	      continue;

	    // A "e" or similar can be followed by a "+" or a "-"
	    if(((ch == 'e') || (ch == 'b') || (ch == 'g') || (ch == 'f')) &&
	       ((chNext == '+') || (chNext == '-')))
	      {
		i++;
		chNext = styler.SafeGetCharAt(i + 1);
		continue;
	      }

	    if(!IsASCII(ch) || !(isdigit(ch) || islower(ch) || isupper(ch)))
	      {
		i--;
		break;
	      }
	  }
	styler.ColourTo(i, SCE_MAXIMA_NUMBER);
	continue;
      }

    // Handle strings
    if(ch == '\"')
      {
	i++;
	for (; i < lengthDoc; i++)
	  {
	    ch = chNext;
	    chNext = styler.SafeGetCharAt(i + 1);
	    if(ch == '\\')
	      i++;
	    else
	      {
		if(ch == '\"')
		  break;
	      }
	  }
	styler.ColourTo(i, SCE_MAXIMA_STRING);
	continue;
      }

    // Handle keywords. Maxima treats Non-ASCII chars as ordinary letters.
    if(((!IsASCII(ch))) || isalpha(ch) || (ch == '_'))
      {
	char cmd[100];
	int cmdidx = 0;
	memset(cmd,0,100);
	cmd[cmdidx++] = ch;
	i++;
	for (; i < lengthDoc; i++)
	  {
	    ch = chNext;
	    chNext = styler.SafeGetCharAt(i + 1);
	    if(ch == '\\')
	      {
		if(cmdidx < 99)
		  cmd[cmdidx++] = ch;
		i++;
		if(cmdidx < 99)
		  cmd[cmdidx++] = ch;
		continue;
	      }
	    if(isMaximaoperator(ch) || ((IsASCII(ch) && !isalpha(ch) && !isdigit(ch) && (ch != '_'))))
	      {
		i--;
		break;
	      }
	    if(cmdidx < 99)
	      cmd[cmdidx++] = ch;
	  }

	// A few known keywords
	if(
	   (strncmp(cmd,"if",99) == 0) ||
	   (strncmp(cmd,"then",99) == 0) ||
	   (strncmp(cmd,"else",99) == 0) ||
	   (strncmp(cmd,"thru",99) == 0) ||
	   (strncmp(cmd,"for",99) == 0) ||
	   (strncmp(cmd,"while",99) == 0) ||
	   (strncmp(cmd,"do",99) == 0)
	   )
	  {
	    styler.ColourTo(i, SCE_MAXIMA_COMMAND);
	    continue;
	  }

	// All other keywords are functions if they are followed
	// by an opening parenthesis
	char nextNonwhitespace = ' ';
	for (Sci_Position o = i + 1; o < lengthDoc; o++)
	  {
	    nextNonwhitespace = styler.SafeGetCharAt(o);
	    if(!IsASCII(nextNonwhitespace) || !isspacechar(nextNonwhitespace))
	      break;
	  }
	if(nextNonwhitespace == '(')
	  {
	    styler.ColourTo(i, SCE_MAXIMA_COMMAND);
	  }
	else
	  {
	    styler.ColourTo(i, SCE_MAXIMA_VARIABLE);
	  }
	continue;
      }

    styler.ColourTo(i-1, SCE_MAXIMA_UNKNOWN);
  }
}

extern const LexerModule lmMaxima(SCLEX_MAXIMA, ColouriseMaximaDoc, "maxima", 0, 0);
