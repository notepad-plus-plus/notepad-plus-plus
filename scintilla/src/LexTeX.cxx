// Scintilla source code edit control

// File: LexTeX.cxx - general context conformant tex coloring scheme
// Author: Hans Hagen - PRAGMA ADE - Hasselt NL - www.pragma-ade.com
// Version: September 28, 2003

// Copyright: 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// This lexer is derived from the one written for the texwork environment (1999++) which in
// turn is inspired on texedit (1991++) which finds its roots in wdt (1986).

// If you run into strange boundary cases, just tell me and I'll look into it.


// TeX Folding code added by instanton (soft_share@126.com) with borrowed code from
// VisualTeX source by Alex Romanenko.


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

typedef int (*SpellFnDirect)(char*);
SpellFnDirect iscorrectfunction;      // external spellcheck function

typedef int (*IsLetterFnDirect)(int);
IsLetterFnDirect isletterfunction;    // external isletter function

#ifndef STATIC_BUILD
  extern "C" __declspec(dllexport) void setcheck(SpellFnDirect);
  extern "C" __declspec(dllexport) void setisletter(IsLetterFnDirect);
#endif


#include "Platform.h"
#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "StyleContext.h"

#define TEX_TEXT        STYLE_DEFAULT
#define TEX_COMMAND     1
#define TEX_COMMAND1    2
#define TEX_COMMAND2    3 
#define TEX_COMMAND3    4 
#define TEX_COMMAND4    5
#define TEX_KEYWORD1    6
#define TEX_KEYWORD2    7
#define TEX_COMMENT     8
#define TEX_MATHGROUP   9
#define TEX_BRACEGROUP  0
#define TEX_SYMBOL      11
#define TEX_NUMBER      12
#define TEX_BRACKET     13
#define TEX_TEXT_INCORRECT 14

#define DEF_TEXT        STYLE_DEFAULT
#define DEF_SYMBOL      1
#define DEF_NUMBER      2
#define DEF_BRACKET     3
#define DEF_TEXT_INCORRECT 4

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

static inline bool endOfLine(Accessor &styler, unsigned int i) {
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
    unsigned int startPos,
    int length,
    Accessor &styler,
	int defaultInterface) {

    char lineBuffer[1024] ;
	unsigned int linePos = 0 ;

    // some day we can make something lexer.tex.mapping=(all,0)(nl,1)(en,2)...

    if (styler.SafeGetCharAt(0) == '%') {
        for (unsigned int i = 0; i < startPos + length; i++) {
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
    unsigned int startPos,
    int length,
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
						k = strlen(key) ;
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
					sc.SetState(SCE_TEX_SYMBOL) ;
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






static inline bool isnumber(int ch) {
	return
      (ch == '0') || (ch == '1') || (ch == '2') || 
      (ch == '3') || (ch == '4') || (ch == '5') || 
      (ch == '6') || (ch == '7') || (ch == '8') || (ch == '9');
}

static inline bool isbracket(int ch) {
	return
      (ch == '(') || (ch == ')') || (ch == '[') || 
      (ch == ']') || (ch == '{') || (ch == '}'); 
}

static inline bool ismath(int ch) {
	return (ch == '$');
}


static inline bool iswordchar(int ch) {
	return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'));
}

static inline bool issymbol(int ch){
	return (ch == '~') || (ch == '`') || (ch == '\'')|| (ch == '~') ||
           (ch == '#') || (ch == '^') || (ch == '&') || (ch == '+') || 
           (ch == '-') || (ch == '=') || (ch == '>') || (ch == '<');
}

/**************************************************
 ** Function: 
 **
 **
***************************************************/

// pos is "\"
// return length of command in text
int ParseCommand(unsigned int pos, Accessor &styler, char *command)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos+1);
  
  if(ch==',' || ch==':' || ch==';' || ch=='%'){
      command[0]=ch;
      command[1]=0;
	  return 1;
  }

  // find end
     while(iswordchar(ch) && !isnumber(ch) && ch!='_' && ch!='.'){
          command[length]=ch;
          length++;
          ch=styler.SafeGetCharAt(pos+length+1);
     }
     
  command[length]='\0';   
  if(!length) return 0;
  return length+1;
}

// pos is letter
// return length of word
int ParseWord(unsigned int pos, Accessor &styler, char *word)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos);
  *word=0;

 while(isletterfunction(ch)){
          word[length]=ch;
          length++;
          ch=styler.SafeGetCharAt(pos+length);
  }
  word[length]=0;   
  return length;
 }

// pos is letter
// return length of word
int ParseAlphaWord(unsigned int pos, Accessor &styler, char *word)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos);
  *word=0;

 while(isalpha(ch)){
          word[length]=ch;
          length++;
          ch=styler.SafeGetCharAt(pos+length);
  }
  word[length]=0;   
  return length;
 }


// pos is "%"
// return length of comment 
int ParseComment(int pos, int endPos, Accessor &styler)
{
  int length=1;
  char ch=styler.SafeGetCharAt(pos+length);
  
  while(ch!='\n' && ch!='\r' && ch!='\0' && pos+length<endPos){
        length++;
        ch=styler.SafeGetCharAt(pos+length);
  }     
  return (length-1) ? length : 1;
}


// Brace Group ----------------------------------------------------

// pos is "{"
// return length group
int ParseBraceGroup(int pos, char *word, Accessor &styler)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos+1);  
  if(ch=='}') return 0;     // {}
  
  while(ch!='}' && ch!='\0' && ch!='\n' && ch!='\r' && ch!=' '){
        word[length]=ch;
        length++;
        ch=styler.SafeGetCharAt(pos+length+1);
  }     
 
  if(ch!='}') return 0; // unmached
  word[length]='\0';
  
  return length+1;  
}

// String ----------------------------------------------------

// pos is "\""
// return length group
int ParseString(int pos, int endPos, Accessor &styler, int *flag)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos+1);  
  if(ch=='\"'){
      *flag=1;
	  return 2;     // ""
  }
  if(ch=='\0' || ch=='\n' || ch=='\r' || pos+length+1==endPos){
      *flag=0;
	  return 1; 
  }

  while(ch!='\"' && ch!='\0' && ch!='\n' && ch!='\r' && pos+length<endPos){
	  if(ch=='\\'){
		  ch=styler.SafeGetCharAt(pos+length+1);
          length++;
	  }
 	  length++;
      ch=styler.SafeGetCharAt(pos+length+1);
  }     
 
  if(ch!='\"') *flag=0; // unmached
  else *flag=1;

  return length+2;  
}


int ParseChar(int pos, int endPos, Accessor &styler, int *flag)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos+1);  
  if(ch=='\''){
      *flag=1;
	  return 2;     // ""
  }

  if(ch=='\0' || ch=='\n' || ch=='\r' || pos+length+1==endPos){
      *flag=0;
	  return 1; 
  }


  while(ch!='\'' && ch!='\0' && ch!='\n' && ch!='\r' && pos+length<endPos){
	  if(ch=='\\'){
		  ch=styler.SafeGetCharAt(pos+length+1);
          length++;
	  }
 	  length++;
      ch=styler.SafeGetCharAt(pos+length+1);
  }     
 
  if(ch!='\'') *flag=0; // unmached
  else *flag=1;

  return length+2;  
}


// classifyFoldPointTeX: borrowed from VisualTeX with modifications

static int classifyFoldPointTeX(const char* s) {
	int lev=0; 
	if (!(isdigit(s[0]) || (s[0] == '.'))){
		if (strcmp(s, "begin")==0||strcmp(s,"StartFolding")==0||
			strcmp(s,"section")==0||strcmp(s,"chapter")==0||
			strcmp(s,"part")==0||strcmp(s,"subsection")==0||
			strcmp(s,"subsubsection")==0||strcmp(s,"CJKfamily")==0||
			strcmp(s,"title")==0||strcmp(s,"documentclass")==0)
			lev=1;
		if (strcmp(s,"EndFolding")==0||strcmp(s, "end")==0||
			strcmp(s,"maketitle")==0) 
		lev=-1;
	}
	return lev;
}

bool iscommandstyle(int style)
{
  return (style==TEX_COMMAND) || (style==TEX_COMMAND1) || (style==TEX_COMMAND2) || (style==TEX_COMMAND3) || (style==TEX_COMMAND4);
}


// FoldTeXDoc: borrowed from VisualTeX with modifications

static void FoldTexDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler) 
{
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos+length;
	int visibleChars=0;
	int lineCurrent=styler.GetLine(startPos);
	int levelPrev=styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent=levelPrev;
	char chNext=styler[startPos];
	int style=initStyle;
	
	char buffer[100]="";

	for (unsigned int i=startPos; i < endPos; i++) {
		char ch=chNext;
		chNext=styler.SafeGetCharAt(i+1);
		style = styler.StyleAt(i);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

        if(ch=='\\' && iscommandstyle(style)) {
                ParseCommand(i, styler, buffer);
			levelCurrent += classifyFoldPointTeX(buffer);
		}


//--------------------------------- Added by instanton ---------------------------------------
	char chNext2;
	char chNext3;
	char chNext4;
	char chNext5;
	char chNext6;
	chNext2=styler.SafeGetCharAt(i+2);
	chNext3=styler.SafeGetCharAt(i+3);
	chNext4=styler.SafeGetCharAt(i+4);
	chNext5=styler.SafeGetCharAt(i+5);
	chNext6=styler.SafeGetCharAt(i+6);
		
	bool atEOSec = (ch == '\r' || ch=='\n') && 
			(chNext == '\\') && (chNext2=='s') && 
				((chNext3=='e'&& chNext4=='c' && chNext5=='t')||
				(chNext3=='u' && chNext4=='b' && chNext5=='s'));
	bool atEOChap = (ch == '\r' || ch=='\n') && 
			(chNext == '\\') && (chNext2=='c') && 
				(chNext3=='h')&& (chNext4=='a') && (chNext5=='p');
	bool atBeginMark = (ch== '%') && (chNext == '\\') && (chNext2 == 'b') &&
			(chNext3 =='e') && (chNext4 =='g') && (chNext5 =='i') &&
			(chNext6 =='n');
	bool atEndMark = (ch=='%') && (chNext == '\\') && (chNext2 =='e') &&
			(chNext3 =='n') && (chNext4 =='d');
	bool atEOCJK = (ch=='\r'|| ch=='\n') && (chNext == '\\') && 
			(chNext2=='C') && (chNext3=='J')&& (chNext4=='K') && 
			(chNext5=='f') && (chNext6 =='a');	


	if(atBeginMark){
		levelCurrent+=1;
	}
	
	if(atEOSec || atEOChap || atEndMark || atEOCJK){
//	if(atEOSec || atEOChap || atEndMark){
		levelCurrent-=1;
	}

	if(ch=='\\' && chNext=='['){
		levelCurrent+=1;
	}
	
	if(ch=='\\' && chNext==']'){
		levelCurrent-=1;
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
