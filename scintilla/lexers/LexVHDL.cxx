// Scintilla source code edit control
/** @file LexVHDL.cxx
 ** Lexer for VHDL
 ** Written by Phil Reid,
 ** Based on:
 **  - The Verilog Lexer by Avi Yegudin
 **  - The Fortran Lexer by Chuan-jian Shen
 **  - The C++ lexer by Neil Hodgson
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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

static void ColouriseVHDLDoc(
  Sci_PositionU startPos,
  Sci_Position length,
  int initStyle,
  WordList *keywordlists[],
  Accessor &styler);


/***************************************/
static inline bool IsAWordChar(const int ch) {
  return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' );
}

/***************************************/
static inline bool IsAWordStart(const int ch) {
  return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

/***************************************/
static inline bool IsABlank(unsigned int ch) {
    return (ch == ' ') || (ch == 0x09) || (ch == 0x0b) ;
}

/***************************************/
static void ColouriseVHDLDoc(
  Sci_PositionU startPos,
  Sci_Position length,
  int initStyle,
  WordList *keywordlists[],
  Accessor &styler)
{
  WordList &Keywords   = *keywordlists[0];
  WordList &Operators  = *keywordlists[1];
  WordList &Attributes = *keywordlists[2];
  WordList &Functions  = *keywordlists[3];
  WordList &Packages   = *keywordlists[4];
  WordList &Types      = *keywordlists[5];
  WordList &User       = *keywordlists[6];

  StyleContext sc(startPos, length, initStyle, styler);
  bool isExtendedId = false;    // true when parsing an extended identifier

  for (; sc.More(); sc.Forward())
  {

    // Determine if the current state should terminate.
    if (sc.state == SCE_VHDL_OPERATOR) {
      sc.SetState(SCE_VHDL_DEFAULT);
    } else if (sc.state == SCE_VHDL_NUMBER) {
      if (!IsAWordChar(sc.ch) && (sc.ch != '#')) {
        sc.SetState(SCE_VHDL_DEFAULT);
      }
    } else if (sc.state == SCE_VHDL_IDENTIFIER) {
      if (!isExtendedId && (!IsAWordChar(sc.ch) || (sc.ch == '.'))) {
        char s[100];
        sc.GetCurrentLowered(s, sizeof(s));
        if (Keywords.InList(s)) {
          sc.ChangeState(SCE_VHDL_KEYWORD);
        } else if (Operators.InList(s)) {
          sc.ChangeState(SCE_VHDL_STDOPERATOR);
        } else if (Attributes.InList(s)) {
          sc.ChangeState(SCE_VHDL_ATTRIBUTE);
        } else if (Functions.InList(s)) {
          sc.ChangeState(SCE_VHDL_STDFUNCTION);
        } else if (Packages.InList(s)) {
          sc.ChangeState(SCE_VHDL_STDPACKAGE);
        } else if (Types.InList(s)) {
          sc.ChangeState(SCE_VHDL_STDTYPE);
        } else if (User.InList(s)) {
          sc.ChangeState(SCE_VHDL_USERWORD);
        }
        sc.SetState(SCE_VHDL_DEFAULT);
      } else if (isExtendedId && ((sc.ch == '\\') || sc.atLineEnd)) {
        // extended identifiers are terminated by backslash, check for end of line in case we have invalid syntax
        isExtendedId = false;
        sc.ForwardSetState(SCE_VHDL_DEFAULT);
      }
    } else if (sc.state == SCE_VHDL_COMMENT || sc.state == SCE_VHDL_COMMENTLINEBANG) {
      if (sc.atLineEnd) {
        sc.SetState(SCE_VHDL_DEFAULT);
      }
    } else if (sc.state == SCE_VHDL_STRING) {
      if (sc.ch == '\\') {
        if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
          sc.Forward();
        }
      } else if (sc.ch == '\"') {
        sc.ForwardSetState(SCE_VHDL_DEFAULT);
      } else if (sc.atLineEnd) {
        sc.ChangeState(SCE_VHDL_STRINGEOL);
        sc.ForwardSetState(SCE_VHDL_DEFAULT);
      }
    } else if (sc.state == SCE_VHDL_BLOCK_COMMENT){
      if(sc.ch == '*' && sc.chNext == '/'){
        sc.Forward();
        sc.ForwardSetState(SCE_VHDL_DEFAULT);
      }
    }

    // Determine if a new state should be entered.
    if (sc.state == SCE_VHDL_DEFAULT) {
      if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
        sc.SetState(SCE_VHDL_NUMBER);
      } else if (IsAWordStart(sc.ch)) {
        sc.SetState(SCE_VHDL_IDENTIFIER);
      } else if (sc.Match('-', '-')) {
        if (sc.Match("--!"))  // Nice to have a different comment style
          sc.SetState(SCE_VHDL_COMMENTLINEBANG);
        else
          sc.SetState(SCE_VHDL_COMMENT);
      } else if (sc.Match('/', '*')){
        sc.SetState(SCE_VHDL_BLOCK_COMMENT);
      } else if (sc.ch == '\"') {
        sc.SetState(SCE_VHDL_STRING);
      } else if (sc.ch == '\\') {
        isExtendedId = true;
        sc.SetState(SCE_VHDL_IDENTIFIER);
      } else if (isoperator(static_cast<char>(sc.ch))) {
        sc.SetState(SCE_VHDL_OPERATOR);
      }
    }
  }
  sc.Complete();
}
//=============================================================================
static bool IsCommentLine(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		char chNext = styler[i+1];
		if ((ch == '-') && (chNext == '-'))
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}
static bool IsCommentBlockStart(Sci_Position line, Accessor &styler)
{
    Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		char chNext = styler[i+1];
        char style = styler.StyleAt(i);
		if ((style == SCE_VHDL_BLOCK_COMMENT) && (ch == '/') && (chNext == '*'))
			return true;
	}
	return false;
}

static bool IsCommentBlockEnd(Sci_Position line, Accessor &styler)
{
    Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;

	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		char chNext = styler[i+1];
        char style = styler.StyleAt(i);
		if ((style == SCE_VHDL_BLOCK_COMMENT) && (ch == '*') && (chNext == '/'))
			return true;
	}
	return false;
}

static bool IsCommentStyle(char style)
{
    return style == SCE_VHDL_BLOCK_COMMENT || style == SCE_VHDL_COMMENT || style == SCE_VHDL_COMMENTLINEBANG;
}

//=============================================================================
// Folding the code
static void FoldNoBoxVHDLDoc(
  Sci_PositionU startPos,
  Sci_Position length,
  int,
  Accessor &styler)
{
  // Decided it would be smarter to have the lexer have all keywords included. Therefore I
  // don't check if the style for the keywords that I use to adjust the levels.
  char words[] =
    "architecture begin block case component else elsif end entity generate loop package process record then "
    "procedure protected function when units";
  WordList keywords;
  keywords.Set(words);

  bool foldComment      = styler.GetPropertyInt("fold.comment", 1) != 0;
  bool foldCompact      = styler.GetPropertyInt("fold.compact", 1) != 0;
  bool foldAtElse       = styler.GetPropertyInt("fold.at.else", 1) != 0;
  bool foldAtBegin      = styler.GetPropertyInt("fold.at.Begin", 1) != 0;
  bool foldAtParenthese = styler.GetPropertyInt("fold.at.Parenthese", 1) != 0;
  //bool foldAtWhen       = styler.GetPropertyInt("fold.at.When", 1) != 0;  //< fold at when in case statements

  int  visibleChars     = 0;
  Sci_PositionU endPos   = startPos + length;

  Sci_Position lineCurrent       = styler.GetLine(startPos);
  int levelCurrent      = SC_FOLDLEVELBASE;
  if(lineCurrent > 0)
    levelCurrent        = styler.LevelAt(lineCurrent-1) >> 16;
  //int levelMinCurrent   = levelCurrent;
  int levelMinCurrentElse = levelCurrent;   //< Used for folding at 'else'
  int levelMinCurrentBegin = levelCurrent;  //< Used for folding at 'begin'
  int levelNext         = levelCurrent;

  /***************************************/
  Sci_Position lastStart         = 0;
  char prevWord[32]     = "";

  /***************************************/
  // Find prev word
  // The logic for going up or down a level depends on a the previous keyword
  // This code could be cleaned up.
  Sci_Position end = 0;
  Sci_PositionU j;
  for(j = startPos; j>0; j--)
  {
    char ch       = styler.SafeGetCharAt(j);
    char chPrev   = styler.SafeGetCharAt(j-1);
    int style     = styler.StyleAt(j);
    int stylePrev = styler.StyleAt(j-1);
    if ((!IsCommentStyle(style)) && (stylePrev != SCE_VHDL_STRING))
    {
      if(IsAWordChar(chPrev) && !IsAWordChar(ch))
      {
        end = j-1;
      }
    }
    if ((!IsCommentStyle(style)) && (style != SCE_VHDL_STRING))
    {
      if(!IsAWordChar(chPrev) && IsAWordStart(ch) && (end != 0))
      {
        char s[32];
        Sci_PositionU k;
        for(k=0; (k<31 ) && (k<end-j+1 ); k++) {
          s[k] = static_cast<char>(tolower(styler[j+k]));
        }
        s[k] = '\0';

        if(keywords.InList(s)) {
          strcpy(prevWord, s);
          break;
        }
      }
    }
  }
  for(j=j+static_cast<Sci_PositionU>(strlen(prevWord)); j<endPos; j++)
  {
    char ch       = styler.SafeGetCharAt(j);
    int style     = styler.StyleAt(j);
    if ((!IsCommentStyle(style)) && (style != SCE_VHDL_STRING))
    {
      if((ch == ';') && (strcmp(prevWord, "end") == 0))
      {
        strcpy(prevWord, ";");
      }
    }
  }

  char  chNext          = styler[startPos];
  char  chPrev          = '\0';
  char  chNextNonBlank;
  int   styleNext       = styler.StyleAt(startPos);
  //Platform::DebugPrintf("Line[%04d] Prev[%20s] ************************* Level[%x]\n", lineCurrent+1, prevWord, levelCurrent);

  /***************************************/
  for (Sci_PositionU i = startPos; i < endPos; i++)
  {
    char ch         = chNext;
    chNext          = styler.SafeGetCharAt(i + 1);
    chPrev          = styler.SafeGetCharAt(i - 1);
    chNextNonBlank  = chNext;
    Sci_PositionU j  = i+1;
    while(IsABlank(chNextNonBlank) && j<endPos)
    {
      j ++ ;
      chNextNonBlank = styler.SafeGetCharAt(j);
    }
    int style           = styleNext;
    styleNext       = styler.StyleAt(i + 1);
    bool atEOL      = (ch == '\r' && chNext != '\n') || (ch == '\n');

    if (foldComment && atEOL)
    {
      if(IsCommentLine(lineCurrent, styler))
      {
        if(!IsCommentLine(lineCurrent-1, styler) && IsCommentLine(lineCurrent+1, styler))
        {
          levelNext++;
        }
        else if(IsCommentLine(lineCurrent-1, styler) && !IsCommentLine(lineCurrent+1, styler))
        {
          levelNext--;
        }
      }
      else
      {
        if (IsCommentBlockStart(lineCurrent, styler) && !IsCommentBlockEnd(lineCurrent, styler))
        {
          levelNext++;
        }
        else if (IsCommentBlockEnd(lineCurrent, styler) && !IsCommentBlockStart(lineCurrent, styler))
        {
          levelNext--;
        }
      }
    }

    if ((style == SCE_VHDL_OPERATOR) && foldAtParenthese)
    {
      if(ch == '(') {
        levelNext++;
      } else if (ch == ')') {
        levelNext--;
      }
    }

    if ((!IsCommentStyle(style)) && (style != SCE_VHDL_STRING))
    {
      if((ch == ';') && (strcmp(prevWord, "end") == 0))
      {
        strcpy(prevWord, ";");
      }

      if(!IsAWordChar(chPrev) && IsAWordStart(ch))
      {
        lastStart = i;
      }

      if(IsAWordChar(ch) && !IsAWordChar(chNext)) {
        char s[32];
        Sci_PositionU k;
        for(k=0; (k<31 ) && (k<i-lastStart+1 ); k++) {
          s[k] = static_cast<char>(tolower(styler[lastStart+k]));
        }
        s[k] = '\0';

        if(keywords.InList(s))
        {
          if (
            strcmp(s, "architecture") == 0  ||
            strcmp(s, "case") == 0          ||
            strcmp(s, "generate") == 0      ||
            strcmp(s, "block") == 0         ||
            strcmp(s, "loop") == 0          ||
            strcmp(s, "package") ==0        ||
            strcmp(s, "process") == 0       ||
            strcmp(s, "protected") == 0     ||
            strcmp(s, "record") == 0        ||
            strcmp(s, "then") == 0          ||
            strcmp(s, "units") == 0)
          {
            if (strcmp(prevWord, "end") != 0)
            {
              if (levelMinCurrentElse > levelNext) {
                levelMinCurrentElse = levelNext;
              }
              levelNext++;
            }
          } else if (
            strcmp(s, "component") == 0      ||
            strcmp(s, "entity") == 0         ||
            strcmp(s, "configuration") == 0 )
          {
            if (strcmp(prevWord, "end") != 0 && lastStart)
            { // check for instantiated unit by backward searching for the colon.
              Sci_PositionU pos = lastStart;
              char chAtPos, styleAtPos;
              do{// skip white spaces
                pos--;
                styleAtPos = styler.StyleAt(pos);
                chAtPos = styler.SafeGetCharAt(pos);
              }while(pos>0 &&
                     (chAtPos == ' ' || chAtPos == '\t' ||
                      chAtPos == '\n' || chAtPos == '\r' ||
                      IsCommentStyle(styleAtPos)));

              // check for a colon (':') before the instantiated units "entity", "component" or "configuration". Don't fold thereafter.
              if (chAtPos != ':')
              {
                if (levelMinCurrentElse > levelNext) {
                  levelMinCurrentElse = levelNext;
                }
                levelNext++;
              }
            }
          } else if (
            strcmp(s, "procedure") == 0     ||
            strcmp(s, "function") == 0)
          {
            if (strcmp(prevWord, "end") != 0) // check for "end procedure" etc.
            { // This code checks to see if the procedure / function is a definition within a "package"
              // rather than the actual code in the body.
              int BracketLevel = 0;
              for(Sci_Position pos=i+1; pos<styler.Length(); pos++)
              {
                int styleAtPos = styler.StyleAt(pos);
                char chAtPos = styler.SafeGetCharAt(pos);
                if(chAtPos == '(') BracketLevel++;
                if(chAtPos == ')') BracketLevel--;
                if(
                  (BracketLevel == 0) &&
                  (!IsCommentStyle(styleAtPos)) &&
                  (styleAtPos != SCE_VHDL_STRING) &&
                  !iswordchar(styler.SafeGetCharAt(pos-1)) &&
                  (chAtPos|' ')=='i' && (styler.SafeGetCharAt(pos+1)|' ')=='s' &&
                  !iswordchar(styler.SafeGetCharAt(pos+2)))
                {
                  if (levelMinCurrentElse > levelNext) {
                    levelMinCurrentElse = levelNext;
                  }
                  levelNext++;
                  break;
                }
                if((BracketLevel == 0) && (chAtPos == ';'))
                {
                  break;
                }
              }
            }

          } else if (strcmp(s, "end") == 0) {
            levelNext--;
          }  else if(strcmp(s, "elsif") == 0) { // elsif is followed by then so folding occurs correctly
            levelNext--;
          } else if (strcmp(s, "else") == 0) {
            if(strcmp(prevWord, "when") != 0)  // ignore a <= x when y else z;
            {
              levelMinCurrentElse = levelNext - 1;  // VHDL else is all on its own so just dec. the min level
            }
          } else if(
            ((strcmp(s, "begin") == 0) && (strcmp(prevWord, "architecture") == 0)) ||
            ((strcmp(s, "begin") == 0) && (strcmp(prevWord, "function") == 0)) ||
            ((strcmp(s, "begin") == 0) && (strcmp(prevWord, "procedure") == 0)))
          {
            levelMinCurrentBegin = levelNext - 1;
          }
          //Platform::DebugPrintf("Line[%04d] Prev[%20s] Cur[%20s] Level[%x]\n", lineCurrent+1, prevWord, s, levelCurrent);
          strcpy(prevWord, s);
        }
      }
    }
    if (atEOL) {
      int levelUse = levelCurrent;

      if (foldAtElse && (levelMinCurrentElse < levelUse)) {
        levelUse = levelMinCurrentElse;
      }
      if (foldAtBegin && (levelMinCurrentBegin < levelUse)) {
        levelUse = levelMinCurrentBegin;
      }
      int lev = levelUse | levelNext << 16;
      if (visibleChars == 0 && foldCompact)
        lev |= SC_FOLDLEVELWHITEFLAG;

      if (levelUse < levelNext)
        lev |= SC_FOLDLEVELHEADERFLAG;
      if (lev != styler.LevelAt(lineCurrent)) {
        styler.SetLevel(lineCurrent, lev);
      }
      //Platform::DebugPrintf("Line[%04d] ---------------------------------------------------- Level[%x]\n", lineCurrent+1, levelCurrent);
      lineCurrent++;
      levelCurrent = levelNext;
      //levelMinCurrent = levelCurrent;
      levelMinCurrentElse = levelCurrent;
      levelMinCurrentBegin = levelCurrent;
      visibleChars = 0;
    }
    /***************************************/
    if (!isspacechar(ch)) visibleChars++;
  }

  /***************************************/
//  Platform::DebugPrintf("Line[%04d] ---------------------------------------------------- Level[%x]\n", lineCurrent+1, levelCurrent);
}

//=============================================================================
static void FoldVHDLDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[],
                       Accessor &styler) {
  FoldNoBoxVHDLDoc(startPos, length, initStyle, styler);
}

//=============================================================================
static const char * const VHDLWordLists[] = {
            "Keywords",
            "Operators",
            "Attributes",
            "Standard Functions",
            "Standard Packages",
            "Standard Types",
            "User Words",
            0,
        };


LexerModule lmVHDL(SCLEX_VHDL, ColouriseVHDLDoc, "vhdl", FoldVHDLDoc, VHDLWordLists);


// Keyword:
//    access after alias all architecture array assert attribute begin block body buffer bus case component
//    configuration constant disconnect downto else elsif end entity exit file for function generate generic
//    group guarded if impure in inertial inout is label library linkage literal loop map new next null of
//    on open others out package port postponed procedure process pure range record register reject report
//    return select severity shared signal subtype then to transport type unaffected units until use variable
//    wait when while with
//
// Operators:
//    abs and mod nand nor not or rem rol ror sla sll sra srl xnor xor
//
// Attributes:
//    left right low high ascending image value pos val succ pred leftof rightof base range reverse_range
//    length delayed stable quiet transaction event active last_event last_active last_value driving
//    driving_value simple_name path_name instance_name
//
// Std Functions:
//    now readline read writeline write endfile resolved to_bit to_bitvector to_stdulogic to_stdlogicvector
//    to_stdulogicvector to_x01 to_x01z to_UX01 rising_edge falling_edge is_x shift_left shift_right rotate_left
//    rotate_right resize to_integer to_unsigned to_signed std_match to_01
//
// Std Packages:
//    std ieee work standard textio std_logic_1164 std_logic_arith std_logic_misc std_logic_signed
//    std_logic_textio std_logic_unsigned numeric_bit numeric_std math_complex math_real vital_primitives
//    vital_timing
//
// Std Types:
//    boolean bit character severity_level integer real time delay_length natural positive string bit_vector
//    file_open_kind file_open_status line text side width std_ulogic std_ulogic_vector std_logic
//    std_logic_vector X01 X01Z UX01 UX01Z unsigned signed
//

