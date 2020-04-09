// Scintilla source code edit control

// File: LexTeX.cxx - general context conformant tex coloring scheme
// Author: Hans Hagen - PRAGMA ADE - Hasselt NL - www.pragma-ade.com
// Version: September 28, 2003

// Copyright: 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// This lexer is derived from the one written for the texwork environment (1999++) which in
// turn is inspired on texedit (1991++) which finds its roots in wdt (1986).

// If you run into strange boundary cases, just tell me and I'll look into it.


// TeX Folding code added by instanton (soft_share@126.com) with borrowed code from VisualTeX source by Alex Romanenko.
// Version: June 22, 2007

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

// val SCE_TEX_DEFAULT = 0
// val SCE_TEX_SPECIAL = 1
// val SCE_TEX_GROUP   = 2
// val SCE_TEX_SYMBOL  = 3
// val SCE_TEX_COMMAND = 4
// val SCE_TEX_TEXT    = 5

// Definitions in SciTEGlobal.properties:
//
// TeX Highlighting
//
// # Default
// style.tex.0=fore:#7F7F00
// # Special
// style.tex.1=fore:#007F7F
// # Group
// style.tex.2=fore:#880000
// # Symbol
// style.tex.3=fore:#7F7F00
// # Command
// style.tex.4=fore:#008800
// # Text
// style.tex.5=fore:#000000

// lexer.tex.interface.default=0
// lexer.tex.comment.process=0

// todo: lexer.tex.auto.if

// Auxiliary functions:

static inline bool endOfLine(Accessor &styler, Sci_PositionU i) {
	return
      (styler[i] == '\n') || ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n')) ;
}

static inline bool isTeXzero(int ch) {
	return
      (ch == '%') ;
}

static inline bool isTeXone(int ch) {
	return
      (ch == '[') || (ch == ']') || (ch == '=') || (ch == '#') ||
      (ch == '(') || (ch == ')') || (ch == '<') || (ch == '>') ||
      (ch == '"') ;
}

static inline bool isTeXtwo(int ch) {
	return
      (ch == '{') || (ch == '}') || (ch == '$') ;
}

static inline bool isTeXthree(int ch) {
	return
      (ch == '~') || (ch == '^') || (ch == '_') || (ch == '&') ||
      (ch == '-') || (ch == '+') || (ch == '\"') || (ch == '`') ||
      (ch == '/') || (ch == '|') || (ch == '%') ;
}

static inline bool isTeXfour(int ch) {
	return
      (ch == '\\') ;
}

static inline bool isTeXfive(int ch) {
	return
      ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
      (ch == '@') || (ch == '!') || (ch == '?') ;
}

static inline bool isTeXsix(int ch) {
	return
      (ch == ' ') ;
}

static inline bool isTeXseven(int ch) {
	return
      (ch == '^') ;
}

// Interface determination

static int CheckTeXInterface(
    Sci_PositionU startPos,
    Sci_Position length,
    Accessor &styler,
	int defaultInterface) {

    char lineBuffer[1024] ;
	Sci_PositionU linePos = 0 ;

    // some day we can make something lexer.tex.mapping=(all,0)(nl,1)(en,2)...

    if (styler.SafeGetCharAt(0) == '%') {
        for (Sci_PositionU i = 0; i < startPos + length; i++) {
            lineBuffer[linePos++] = styler.SafeGetCharAt(i) ;
            if (endOfLine(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
                lineBuffer[linePos] = '\0';
                if (strstr(lineBuffer, "interface=all")) {
                    return 0 ;
				} else if (strstr(lineBuffer, "interface=tex")) {
                    return 1 ;
                } else if (strstr(lineBuffer, "interface=nl")) {
                    return 2 ;
                } else if (strstr(lineBuffer, "interface=en")) {
                    return 3 ;
                } else if (strstr(lineBuffer, "interface=de")) {
                    return 4 ;
                } else if (strstr(lineBuffer, "interface=cz")) {
                    return 5 ;
                } else if (strstr(lineBuffer, "interface=it")) {
                    return 6 ;
                } else if (strstr(lineBuffer, "interface=ro")) {
                    return 7 ;
                } else if (strstr(lineBuffer, "interface=latex")) {
					// we will move latex cum suis up to 91+ when more keyword lists are supported
                    return 8 ;
				} else if (styler.SafeGetCharAt(1) == 'D' && strstr(lineBuffer, "%D \\module")) {
					// better would be to limit the search to just one line
					return 3 ;
                } else {
                    return defaultInterface ;
                }
            }
		}
    }

    return defaultInterface ;
}

static void ColouriseTeXDoc(
    Sci_PositionU startPos,
    Sci_Position length,
    int,
    WordList *keywordlists[],
    Accessor &styler) {

	styler.StartAt(startPos) ;
	styler.StartSegment(startPos) ;

	bool processComment   = styler.GetPropertyInt("lexer.tex.comment.process",   0) == 1 ;
	bool useKeywords      = styler.GetPropertyInt("lexer.tex.use.keywords",      1) == 1 ;
	bool autoIf           = styler.GetPropertyInt("lexer.tex.auto.if",           1) == 1 ;
	int  defaultInterface = styler.GetPropertyInt("lexer.tex.interface.default", 1) ;

	char key[100] ;
	int  k ;
	bool newifDone = false ;
	bool inComment = false ;

	int currentInterface = CheckTeXInterface(startPos,length,styler,defaultInterface) ;

    if (currentInterface == 0) {
        useKeywords = false ;
        currentInterface = 1 ;
    }

    WordList &keywords = *keywordlists[currentInterface-1] ;

	StyleContext sc(startPos, length, SCE_TEX_TEXT, styler);

	bool going = sc.More() ; // needed because of a fuzzy end of file state

	for (; going; sc.Forward()) {

		if (! sc.More()) { going = false ; } // we need to go one behind the end of text

		if (inComment) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_TEX_TEXT) ;
				newifDone = false ;
				inComment = false ;
			}
		} else {
			if (! isTeXfive(sc.ch)) {
				if (sc.state == SCE_TEX_COMMAND) {
					if (sc.LengthCurrent() == 1) { // \<noncstoken>
						if (isTeXseven(sc.ch) && isTeXseven(sc.chNext)) {
							sc.Forward(2) ; // \^^ and \^^<token>
						}
						sc.ForwardSetState(SCE_TEX_TEXT) ;
					} else {
						sc.GetCurrent(key, sizeof(key)-1) ;
						k = static_cast<int>(strlen(key)) ;
						memmove(key,key+1,k) ; // shift left over escape token
						key[k] = '\0' ;
						k-- ;
						if (! keywords || ! useKeywords) {
							sc.SetState(SCE_TEX_COMMAND) ;
							newifDone = false ;
						} else if (k == 1) { //\<cstoken>
							sc.SetState(SCE_TEX_COMMAND) ;
							newifDone = false ;
						} else if (keywords.InList(key)) {
    						sc.SetState(SCE_TEX_COMMAND) ;
							newifDone = autoIf && (strcmp(key,"newif") == 0) ;
						} else if (autoIf && ! newifDone && (key[0] == 'i') && (key[1] == 'f') && keywords.InList("if")) {
	    					sc.SetState(SCE_TEX_COMMAND) ;
						} else {
							sc.ChangeState(SCE_TEX_TEXT) ;
							sc.SetState(SCE_TEX_TEXT) ;
							newifDone = false ;
						}
					}
				}
				if (isTeXzero(sc.ch)) {
					sc.SetState(SCE_TEX_SYMBOL);

					if (!endOfLine(styler,sc.currentPos + 1))
						sc.ForwardSetState(SCE_TEX_DEFAULT) ;

					inComment = ! processComment ;
					newifDone = false ;
				} else if (isTeXseven(sc.ch) && isTeXseven(sc.chNext)) {
					sc.SetState(SCE_TEX_TEXT) ;
					sc.ForwardSetState(SCE_TEX_TEXT) ;
				} else if (isTeXone(sc.ch)) {
					sc.SetState(SCE_TEX_SPECIAL) ;
					newifDone = false ;
				} else if (isTeXtwo(sc.ch)) {
					sc.SetState(SCE_TEX_GROUP) ;
					newifDone = false ;
				} else if (isTeXthree(sc.ch)) {
					sc.SetState(SCE_TEX_SYMBOL) ;
					newifDone = false ;
				} else if (isTeXfour(sc.ch)) {
					sc.SetState(SCE_TEX_COMMAND) ;
				} else if (isTeXsix(sc.ch)) {
					sc.SetState(SCE_TEX_TEXT) ;
				} else if (sc.atLineEnd) {
					sc.SetState(SCE_TEX_TEXT) ;
					newifDone = false ;
					inComment = false ;
				} else {
					sc.SetState(SCE_TEX_TEXT) ;
				}
			} else if (sc.state != SCE_TEX_COMMAND) {
				sc.SetState(SCE_TEX_TEXT) ;
			}
		}
	}
	sc.ChangeState(SCE_TEX_TEXT) ;
	sc.Complete();

}


static inline bool isNumber(int ch) {
	return
      (ch == '0') || (ch == '1') || (ch == '2') ||
      (ch == '3') || (ch == '4') || (ch == '5') ||
      (ch == '6') || (ch == '7') || (ch == '8') || (ch == '9');
}

static inline bool isWordChar(int ch) {
	return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'));
}

static Sci_Position ParseTeXCommand(Sci_PositionU pos, Accessor &styler, char *command)
{
  Sci_Position length=0;
  char ch=styler.SafeGetCharAt(pos+1);

  if(ch==',' || ch==':' || ch==';' || ch=='%'){
      command[0]=ch;
      command[1]=0;
	  return 1;
  }

  // find end
     while(isWordChar(ch) && !isNumber(ch) && ch!='_' && ch!='.' && length<100){
          command[length]=ch;
          length++;
          ch=styler.SafeGetCharAt(pos+length+1);
     }

  command[length]='\0';
  if(!length) return 0;
  return length+1;
}

static int classifyFoldPointTeXPaired(const char* s) {
	int lev=0;
	if (!(isdigit(s[0]) || (s[0] == '.'))){
		if (strcmp(s, "begin")==0||strcmp(s,"FoldStart")==0||
			strcmp(s,"abstract")==0||strcmp(s,"unprotect")==0||
			strcmp(s,"title")==0||strncmp(s,"start",5)==0||strncmp(s,"Start",5)==0||
			strcmp(s,"documentclass")==0||strncmp(s,"if",2)==0
			)
			lev=1;
		if (strcmp(s, "end")==0||strcmp(s,"FoldStop")==0||
			strcmp(s,"maketitle")==0||strcmp(s,"protect")==0||
			strncmp(s,"stop",4)==0||strncmp(s,"Stop",4)==0||
			strcmp(s,"fi")==0
			)
		lev=-1;
	}
	return lev;
}

static int classifyFoldPointTeXUnpaired(const char* s) {
	int lev=0;
	if (!(isdigit(s[0]) || (s[0] == '.'))){
		if (strcmp(s,"part")==0||
			strcmp(s,"chapter")==0||
			strcmp(s,"section")==0||
			strcmp(s,"subsection")==0||
			strcmp(s,"subsubsection")==0||
			strcmp(s,"CJKfamily")==0||
			strcmp(s,"appendix")==0||
			strcmp(s,"Topic")==0||strcmp(s,"topic")==0||
			strcmp(s,"subject")==0||strcmp(s,"subsubject")==0||
			strcmp(s,"def")==0||strcmp(s,"gdef")==0||strcmp(s,"edef")==0||
			strcmp(s,"xdef")==0||strcmp(s,"framed")==0||
			strcmp(s,"frame")==0||
			strcmp(s,"foilhead")==0||strcmp(s,"overlays")==0||strcmp(s,"slide")==0
			){
			    lev=1;
			}
	}
	return lev;
}

static bool IsTeXCommentLine(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;

	Sci_Position startpos = pos;

	while (startpos<eol_pos){
		char ch = styler[startpos];
		if (ch!='%' && ch!=' ') return false;
		else if (ch=='%') return true;
		startpos++;
	}

	return false;
}

// FoldTeXDoc: borrowed from VisualTeX with modifications

static void FoldTexDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler)
{
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos+length;
	int visibleChars=0;
	Sci_Position lineCurrent=styler.GetLine(startPos);
	int levelPrev=styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent=levelPrev;
	char chNext=styler[startPos];
	char buffer[100]="";

	for (Sci_PositionU i=startPos; i < endPos; i++) {
		char ch=chNext;
		chNext=styler.SafeGetCharAt(i+1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

        if(ch=='\\') {
            ParseTeXCommand(i, styler, buffer);
			levelCurrent += classifyFoldPointTeXPaired(buffer)+classifyFoldPointTeXUnpaired(buffer);
		}

		if (levelCurrent > SC_FOLDLEVELBASE && ((ch == '\r' || ch=='\n') && (chNext == '\\'))) {
            ParseTeXCommand(i+1, styler, buffer);
			levelCurrent -= classifyFoldPointTeXUnpaired(buffer);
		}

	char chNext2;
	char chNext3;
	char chNext4;
	char chNext5;
	chNext2=styler.SafeGetCharAt(i+2);
	chNext3=styler.SafeGetCharAt(i+3);
	chNext4=styler.SafeGetCharAt(i+4);
	chNext5=styler.SafeGetCharAt(i+5);

	bool atEOfold = (ch == '%') &&
			(chNext == '%') && (chNext2=='}') &&
				(chNext3=='}')&& (chNext4=='-')&& (chNext5=='-');

	bool atBOfold = (ch == '%') &&
			(chNext == '%') && (chNext2=='-') &&
				(chNext3=='-')&& (chNext4=='{')&& (chNext5=='{');

	if(atBOfold){
		levelCurrent+=1;
	}

	if(atEOfold){
		levelCurrent-=1;
	}

	if(ch=='\\' && chNext=='['){
		levelCurrent+=1;
	}

	if(ch=='\\' && chNext==']'){
		levelCurrent-=1;
	}

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;

	if (foldComment && atEOL && IsTeXCommentLine(lineCurrent, styler))
        {
            if (lineCurrent==0 && IsTeXCommentLine(lineCurrent + 1, styler)
				)
                levelCurrent++;
            else if (lineCurrent!=0 && !IsTeXCommentLine(lineCurrent - 1, styler)
               && IsTeXCommentLine(lineCurrent + 1, styler)
				)
                levelCurrent++;
            else if (lineCurrent!=0 && IsTeXCommentLine(lineCurrent - 1, styler) &&
                     !IsTeXCommentLine(lineCurrent+1, styler))
                levelCurrent--;
        }

//---------------------------------------------------------------------------------------------

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




static const char * const texWordListDesc[] = {
    "TeX, eTeX, pdfTeX, Omega",
    "ConTeXt Dutch",
    "ConTeXt English",
    "ConTeXt German",
    "ConTeXt Czech",
    "ConTeXt Italian",
    "ConTeXt Romanian",
	0,
} ;

LexerModule lmTeX(SCLEX_TEX,   ColouriseTeXDoc, "tex", FoldTexDoc, texWordListDesc);
