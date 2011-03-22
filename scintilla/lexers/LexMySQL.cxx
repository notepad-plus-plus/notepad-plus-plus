/**
 * Scintilla source code edit control
 * @file LexMySQL.cxx
 * Lexer for MySQL
 *
 * Improved by Mike Lischke <mike.lischke@sun.com>
 * Adopted from LexSQL.cxx by Anders Karlsson <anders@mysql.com>
 * Original work by Neil Hodgson <neilh@scintilla.org>
 * Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed.
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

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsAWordChar(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return (ch < 0x80) && (isalpha(ch) || ch == '_');
}

static inline bool IsADoxygenChar(int ch) {
	return (islower(ch) || ch == '$' || ch == '@' ||
	        ch == '\\' || ch == '&' || ch == '<' ||
	        ch == '>' || ch == '#' || ch == '{' ||
	        ch == '}' || ch == '[' || ch == ']');
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
             ch == '.' || ch == '-' || ch == '+');
}

//--------------------------------------------------------------------------------------------------

/**
 * Check if the current content context represent a keyword and set the context state if so.
 */
static void CheckForKeyword(StyleContext& sc, WordList* keywordlists[])
{
  int length = sc.LengthCurrent() + 1; // +1 for the next char
  char* s = new char[length];
  sc.GetCurrentLowered(s, length);
  if (keywordlists[0]->InList(s))
    sc.ChangeState(SCE_MYSQL_MAJORKEYWORD);
  else
    if (keywordlists[1]->InList(s))
      sc.ChangeState(SCE_MYSQL_KEYWORD);
    else
      if (keywordlists[2]->InList(s))
        sc.ChangeState(SCE_MYSQL_DATABASEOBJECT);
      else
        if (keywordlists[3]->InList(s))
          sc.ChangeState(SCE_MYSQL_FUNCTION);
        else
          if (keywordlists[5]->InList(s))
            sc.ChangeState(SCE_MYSQL_PROCEDUREKEYWORD);
          else
            if (keywordlists[6]->InList(s))
              sc.ChangeState(SCE_MYSQL_USER1);
            else
              if (keywordlists[7]->InList(s))
                sc.ChangeState(SCE_MYSQL_USER2);
              else
                if (keywordlists[8]->InList(s))
                  sc.ChangeState(SCE_MYSQL_USER3);
  delete [] s;
}

//--------------------------------------------------------------------------------------------------

static void ColouriseMySQLDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
  {
		// Determine if the current state should terminate.
		switch (sc.state)
    {
      case SCE_MYSQL_OPERATOR:
        sc.SetState(SCE_MYSQL_DEFAULT);
        break;
      case SCE_MYSQL_NUMBER:
        // We stop the number definition on non-numerical non-dot non-eE non-sign char.
        if (!IsANumberChar(sc.ch))
          sc.SetState(SCE_MYSQL_DEFAULT);
        break;
      case SCE_MYSQL_IDENTIFIER:
        // Switch from identifier to keyword state and open a new state for the new char.
        if (!IsAWordChar(sc.ch))
        {
          CheckForKeyword(sc, keywordlists);

          // Additional check for function keywords needed.
          // A function name must be followed by an opening parenthesis.
          if (sc.state == SCE_MYSQL_FUNCTION && sc.ch != '(')
            sc.ChangeState(SCE_MYSQL_DEFAULT);

          sc.SetState(SCE_MYSQL_DEFAULT);
        }
        break;
      case SCE_MYSQL_VARIABLE:
        if (!IsAWordChar(sc.ch))
          sc.SetState(SCE_MYSQL_DEFAULT);
        break;
      case SCE_MYSQL_SYSTEMVARIABLE:
        if (!IsAWordChar(sc.ch))
        {
          int length = sc.LengthCurrent() + 1;
          char* s = new char[length];
          sc.GetCurrentLowered(s, length);

          // Check for known system variables here.
          if (keywordlists[4]->InList(&s[2]))
            sc.ChangeState(SCE_MYSQL_KNOWNSYSTEMVARIABLE);
          delete [] s;

          sc.SetState(SCE_MYSQL_DEFAULT);
        }
        break;
      case SCE_MYSQL_QUOTEDIDENTIFIER:
        if (sc.ch == '`')
        {
          if (sc.chNext == '`')
            sc.Forward();	// Ignore it
          else
            sc.ForwardSetState(SCE_MYSQL_DEFAULT);
				}
  			break;
      case SCE_MYSQL_COMMENT:
      case SCE_MYSQL_HIDDENCOMMAND:
        if (sc.Match('*', '/'))
        {
          sc.Forward();
          sc.ForwardSetState(SCE_MYSQL_DEFAULT);
        }
        break;
      case SCE_MYSQL_COMMENTLINE:
        if (sc.atLineStart)
          sc.SetState(SCE_MYSQL_DEFAULT);
        break;
      case SCE_MYSQL_SQSTRING:
        if (sc.ch == '\\')
          sc.Forward(); // Escape sequence
        else
          if (sc.ch == '\'')
          {
            // End of single quoted string reached?
            if (sc.chNext == '\'')
              sc.Forward();
            else
              sc.ForwardSetState(SCE_MYSQL_DEFAULT);
          }
        break;
      case SCE_MYSQL_DQSTRING:
        if (sc.ch == '\\')
          sc.Forward(); // Escape sequence
        else
          if (sc.ch == '\"')
          {
            // End of single quoted string reached?
            if (sc.chNext == '\"')
              sc.Forward();
            else
              sc.ForwardSetState(SCE_MYSQL_DEFAULT);
          }
        break;
    }

    // Determine if a new state should be entered.
    if (sc.state == SCE_MYSQL_DEFAULT)
    {
      switch (sc.ch)
      {
        case '@':
          if (sc.chNext == '@')
          {
            sc.SetState(SCE_MYSQL_SYSTEMVARIABLE);
            sc.Forward(2); // Skip past @@.
          }
          else
            if (IsAWordStart(sc.ch))
            {
              sc.SetState(SCE_MYSQL_VARIABLE);
              sc.Forward(); // Skip past @.
            }
            else
              sc.SetState(SCE_MYSQL_OPERATOR);
          break;
        case '`':
          sc.SetState(SCE_MYSQL_QUOTEDIDENTIFIER);
          break;
        case '#':
          sc.SetState(SCE_MYSQL_COMMENTLINE);
          break;
        case '\'':
          sc.SetState(SCE_MYSQL_SQSTRING);
          break;
        case '\"':
          sc.SetState(SCE_MYSQL_DQSTRING);
          break;
        default:
          if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext)))
            sc.SetState(SCE_MYSQL_NUMBER);
          else
            if (IsAWordStart(sc.ch))
              sc.SetState(SCE_MYSQL_IDENTIFIER);
            else
              if (sc.Match('/', '*'))
              {
                sc.SetState(SCE_MYSQL_COMMENT);

                // Skip comment introducer and check for hidden command.
                sc.Forward(2);
                if (sc.ch == '!')
                {
                  sc.ChangeState(SCE_MYSQL_HIDDENCOMMAND);
                  sc.Forward();
                }
              }
              else
                if (sc.Match("--"))
                {
                  // Special MySQL single line comment.
                  sc.SetState(SCE_MYSQL_COMMENTLINE);
                  sc.Forward(2);

                  // Check the third character too. It must be a space or EOL.
                  if (sc.ch != ' ' && sc.ch != '\n' && sc.ch != '\r')
                    sc.ChangeState(SCE_MYSQL_OPERATOR);
                }
                else
                  if (isoperator(static_cast<char>(sc.ch)))
                    sc.SetState(SCE_MYSQL_OPERATOR);
      }
    }
  }

  // Do a final check for keywords if we currently have an identifier, to highlight them
  // also at the end of a line.
  if (sc.state == SCE_MYSQL_IDENTIFIER)
  {
    CheckForKeyword(sc, keywordlists);

    // Additional check for function keywords needed.
    // A function name must be followed by an opening parenthesis.
    if (sc.state == SCE_MYSQL_FUNCTION && sc.ch != '(')
      sc.ChangeState(SCE_MYSQL_DEFAULT);
  }

  sc.Complete();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to determine if we have a foldable comment currently.
 */
static bool IsStreamCommentStyle(int style)
{
	return style == SCE_MYSQL_COMMENT;
}

//--------------------------------------------------------------------------------------------------

/**
 * Code copied from StyleContext and modified to work here. Should go into Accessor as a
 * companion to Match()...
 */
bool MatchIgnoreCase(Accessor &styler, int currentPos, const char *s)
{
  for (int n = 0; *s; n++)
  {
    if (*s != tolower(styler.SafeGetCharAt(currentPos + n)))
      return false;
    s++;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment.
static void FoldMySQLDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler)
{
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldOnlyBegin = styler.GetPropertyInt("fold.sql.only.begin", 0) != 0;

	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
	int levelNext = levelCurrent;

	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	
  bool endPending = false;
	bool whenPending = false;
	bool elseIfPending = false;

  char nextChar = styler.SafeGetCharAt(startPos);
  for (unsigned int i = startPos; length > 0; i++, length--)
  {
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
    
    char currentChar = nextChar;
    nextChar = styler.SafeGetCharAt(i + 1);
		bool atEOL = (currentChar == '\r' && nextChar != '\n') || (currentChar == '\n');
	
    switch (style)
    {
      case SCE_MYSQL_COMMENT:
        if (foldComment)
        {
          // Multiline comment style /* .. */.
          if (IsStreamCommentStyle(style))
          {
            // Increase level if we just start a foldable comment.
            if (!IsStreamCommentStyle(stylePrev))
              levelNext++;
            else
              // If we are in the middle of a foldable comment check if it ends now.
              // Don't end at the line end, though.
              if (!IsStreamCommentStyle(styleNext) && !atEOL)
                levelNext--;
          }
        }
        break;
      case SCE_MYSQL_COMMENTLINE:
        if (foldComment)
        { 
          // Not really a standard, but we add support for single line comments
          // with special curly braces syntax as foldable comments too.
          // MySQL needs -- comments to be followed by space or control char
          if (styler.Match(i, "--"))
          {
            char chNext2 = styler.SafeGetCharAt(i + 2);
            char chNext3 = styler.SafeGetCharAt(i + 3);
            if (chNext2 == '{' || chNext3 == '{')
              levelNext++;
            else
              if (chNext2 == '}' || chNext3 == '}')
                levelNext--;
          }
        }
        break;
      case SCE_MYSQL_HIDDENCOMMAND:
        if (endPending)
        {
          // A conditional command is not a white space so it should end the current block
          // before opening a new one.
          endPending = false;
          levelNext--;
          if (levelNext < SC_FOLDLEVELBASE)
            levelNext = SC_FOLDLEVELBASE;
        }
        if (style != stylePrev)
          levelNext++;
        else
          if (style != styleNext)
          {
            levelNext--;
            if (levelNext < SC_FOLDLEVELBASE)
              levelNext = SC_FOLDLEVELBASE;
          }
        break;
      case SCE_MYSQL_OPERATOR:
        if (endPending)
        {
          endPending = false;
          levelNext--;
          if (levelNext < SC_FOLDLEVELBASE)
            levelNext = SC_FOLDLEVELBASE;
        }
        if (currentChar == '(')
          levelNext++;
        else
          if (currentChar == ')')
          {
            levelNext--;
            if (levelNext < SC_FOLDLEVELBASE)
              levelNext = SC_FOLDLEVELBASE;
          }
        break;
      case SCE_MYSQL_MAJORKEYWORD:
      case SCE_MYSQL_KEYWORD:
      case SCE_MYSQL_FUNCTION:
      case SCE_MYSQL_PROCEDUREKEYWORD:
        // Reserved and other keywords.
        if (style != stylePrev)
        {
          // END decreases the folding level, regardless which keyword follows.
          bool endFound = MatchIgnoreCase(styler, i, "end");
          if (endPending)
          {
            levelNext--;
            if (levelNext < SC_FOLDLEVELBASE)
              levelNext = SC_FOLDLEVELBASE;
          }
          else
            if (!endFound)
            {
              if (MatchIgnoreCase(styler, i, "begin"))
                levelNext++;
              else
              {
                if (!foldOnlyBegin)
                {
                  bool whileFound = MatchIgnoreCase(styler, i, "while");
                  bool loopFound = MatchIgnoreCase(styler, i, "loop");
                  bool repeatFound = MatchIgnoreCase(styler, i, "repeat");
                  bool caseFound = MatchIgnoreCase(styler, i, "case");

                  if (whileFound || loopFound || repeatFound || caseFound)
                    levelNext++;
                  else
                  {
                    // IF alone does not increase the fold level as it is also used in non-block'ed
                    // code like DROP PROCEDURE blah IF EXISTS.
                    // Instead THEN opens the new level (if not part of an ELSEIF or WHEN (case) branch).
                    if (MatchIgnoreCase(styler, i, "then"))
                    {
                      if (!elseIfPending && !whenPending)
                        levelNext++;
                      else
                      {
                        elseIfPending = false;
                        whenPending = false;
                      }
                    }
                    else
                    {
                      // Neither of if/then/while/loop/repeat/case, so check for
                      // sub parts of IF and CASE.
                      if (MatchIgnoreCase(styler, i, "elseif"))
                        elseIfPending = true;
                      if (MatchIgnoreCase(styler, i, "when"))
                        whenPending = true;
                    }
                  }
                }
              }
            }
          
          // Keep the current end state for the next round.
          endPending = endFound;
        }
        break;
        
      default:
        if (!isspace(currentChar) && endPending)
        {
          // END followed by a non-whitespace character (not covered by other cases like identifiers)
          // also should end a folding block. Typical case: END followed by self defined delimiter.
          levelNext--;
          if (levelNext < SC_FOLDLEVELBASE)
            levelNext = SC_FOLDLEVELBASE;
        }
        break;
    }
    
    if (atEOL)
    {
      // Apply the new folding level to this line.
      // Leave pending states as they are otherwise a line break will de-sync
      // code folding and valid syntax.
      int levelUse = levelCurrent;
      int lev = levelUse | levelNext << 16;
      if (visibleChars == 0 && foldCompact)
        lev |= SC_FOLDLEVELWHITEFLAG;
      if (levelUse < levelNext)
        lev |= SC_FOLDLEVELHEADERFLAG;
      if (lev != styler.LevelAt(lineCurrent))
        styler.SetLevel(lineCurrent, lev);
      
      lineCurrent++;
      levelCurrent = levelNext;
      visibleChars = 0;
    }

		if (!isspacechar(currentChar))
      visibleChars++;
  }
}

//--------------------------------------------------------------------------------------------------

static const char * const mysqlWordListDesc[] = {
	"Major Keywords",
	"Keywords",
	"Database Objects",
	"Functions",
	"System Variables",
	"Procedure keywords",
	"User Keywords 1",
	"User Keywords 2",
	"User Keywords 3",
	0
};

LexerModule lmMySQL(SCLEX_MYSQL, ColouriseMySQLDoc, "mysql", FoldMySQLDoc, mysqlWordListDesc);
