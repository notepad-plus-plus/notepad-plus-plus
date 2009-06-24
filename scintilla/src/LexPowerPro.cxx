// Scintilla source code edit control
// @file LexPowerPro.cxx
// PowerPro utility, written by Bruce Switzer, is available from http://powerpro.webeddie.com
// PowerPro lexer is written by Christopher Bean (cbean@cb-software.net)
//
// Lexer code heavily borrowed from: 
//	LexAU3.cxx by Jos van der Zande
//	LexCPP.cxx by Neil Hodgson
//	LexVB.cxx by Neil Hodgson
//
// Changes:
// 	2008-10-25 - Initial release
//	2008-10-26 - Changed how <name> is hilighted in  'function <name>' so that 
//				 local isFunction = "" and local functions = "" don't get falsely highlighted
//	2008-12-14 - Added bounds checking for szKeyword and szDo
//			   - Replaced SetOfCharacters with CharacterSet
//			   - Made sure that CharacterSet::Contains is passed only positive values
//			   - Made sure that the return value of Accessor::SafeGetCharAt is positive before 
//				 passsing to functions that require positive values like isspacechar()
//			   - Removed unused visibleChars processing from ColourisePowerProDoc()
//			   - Fixed bug with folding logic where line continuations didn't end where 
//				 they were supposed to
//			   - Moved all helper functions to the top of the file
//
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Platform.h"
#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "CharacterSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsStreamCommentStyle(int style) {
	return style == SCE_POWERPRO_COMMENTBLOCK;
}

static bool IsContinuationLine(unsigned int szLine, Accessor &styler)
{
	int nsPos = styler.LineStart(szLine);
	int nePos = styler.LineStart(szLine + 1) - 2;
	while (nsPos < nePos)
	{
		int stylech = styler.StyleAt(nsPos);
		if (!(stylech == SCE_POWERPRO_COMMENTBLOCK)) {
			char ch = styler.SafeGetCharAt(nePos);
			char chPrev = styler.SafeGetCharAt(nePos-1);
			char chPrevPrev = styler.SafeGetCharAt(nePos-2);
			if (ch > 0 && chPrev > 0 && chPrevPrev > 0 && !isspacechar(ch) && !isspacechar(chPrev) && !isspacechar(chPrevPrev) ) {
				if (chPrevPrev == ';' && chPrev == ';' && ch == '+')
					return true;
				else
					return false;
			}
		}
		nePos--; // skip to next char
	}
	return false;
}

// Routine to find first none space on the current line and return its Style
// needed for comment lines not starting on pos 1 
static int GetStyleFirstWord(unsigned int szLine, Accessor &styler)
{
	int nsPos = styler.LineStart(szLine);
	int nePos = styler.LineStart(szLine+1) - 1;
	char ch = styler.SafeGetCharAt(nsPos);
	
	while (ch > 0 && isspacechar(ch) && nsPos < nePos)
	{
		nsPos++; // skip to next char
		ch = styler.SafeGetCharAt(nsPos);

	}
	return styler.StyleAt(nsPos);
}

//returns true if there is a function to highlight
//used to highlight <name> in 'function <name>'
static bool HasFunction(Accessor &styler, unsigned int currentPos) {
	
	//check for presence of 'function '
	return 	(styler.SafeGetCharAt(currentPos) == ' ' 	
	&& tolower(styler.SafeGetCharAt(currentPos-1)) == 'n' 
	&& tolower(styler.SafeGetCharAt(currentPos-2)) == 'o'
	&& tolower(styler.SafeGetCharAt(currentPos-3)) == 'i'
	&& tolower(styler.SafeGetCharAt(currentPos-4)) == 't'
	&& tolower(styler.SafeGetCharAt(currentPos-5)) == 'c'
	&& tolower(styler.SafeGetCharAt(currentPos-6)) == 'n'
	&& tolower(styler.SafeGetCharAt(currentPos-7)) == 'u'
	&& tolower(styler.SafeGetCharAt(currentPos-8)) == 'f'
	//only allow 'function ' to appear at the beginning of a line
	&& (styler.SafeGetCharAt(currentPos-9) == '\n'   		
		|| styler.SafeGetCharAt(currentPos-9) == '\r'
		|| (styler.SafeGetCharAt(currentPos -9, '\0')) == '\0') //is the first line
	);
}

static void ColourisePowerProDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler, bool caseSensitive) {

	WordList &keywords  = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	
	//define the character sets 
	CharacterSet setWordStart(CharacterSet::setAlpha, "_@", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);

	StyleContext sc(startPos, length, initStyle, styler);
	char s_save[100]; //for last line highlighting
	
	for (; sc.More(); sc.Forward()) {
			
		// **********************************************
		// save the total current word for eof processing
		char s[100];
		sc.GetCurrentLowered(s, sizeof(s));
		 
		if ((sc.ch > 0) && setWord.Contains(sc.ch)) 
		{
			strcpy(s_save,s);
			int tp = strlen(s_save);
			if (tp < 99) {
				s_save[tp] = static_cast<char>(tolower(sc.ch));
				s_save[tp+1] = '\0';
			}
		}
		// **********************************************
		//

		if (sc.atLineStart) {
			if (sc.state == SCE_POWERPRO_DOUBLEQUOTEDSTRING) {
				// Prevent SCE_POWERPRO_STRINGEOL from leaking back to previous line which
				// ends with a line continuation by locking in the state upto this position.
				sc.SetState(SCE_POWERPRO_DOUBLEQUOTEDSTRING);
			}
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_POWERPRO_OPERATOR:
				sc.SetState(SCE_POWERPRO_DEFAULT);
				break;
			
			case SCE_POWERPRO_NUMBER:

				if (!IsADigit(sc.ch))
					sc.SetState(SCE_POWERPRO_DEFAULT);
				
				break;

			case SCE_POWERPRO_IDENTIFIER:
				//if ((sc.ch > 0) && !setWord.Contains(sc.ch) || (sc.ch == '.')) { // use this line if don't want to match keywords with . in them. ie: win.debug will match both win and debug so win debug will also be colorized
				if ((sc.ch > 0) && !setWord.Contains(sc.ch)){  // || (sc.ch == '.')) { // use this line if you want to match keywords with a . ie: win.debug will only match win.debug neither win nor debug will be colorized separately
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_POWERPRO_WORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_POWERPRO_WORD2);
					} else if (keywords3.InList(s)) {
						sc.ChangeState(SCE_POWERPRO_WORD3);
					} else if (keywords4.InList(s)) {
						sc.ChangeState(SCE_POWERPRO_WORD4);
					}
					sc.SetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_LINECONTINUE:
				if (sc.atLineStart) {
					sc.SetState(SCE_POWERPRO_DEFAULT);
				} else if (sc.Match('/', '*') || sc.Match('/', '/')) {
					sc.SetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_COMMENTBLOCK:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_DOUBLEQUOTEDSTRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_POWERPRO_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_SINGLEQUOTEDSTRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_POWERPRO_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_POWERPRO_DEFAULT);
				}
				break;

			case SCE_POWERPRO_VERBATIM:
				if (sc.ch == '\"') {
					if (sc.chNext == '\"') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_POWERPRO_DEFAULT);
					}
				}
				break;

			case SCE_POWERPRO_ALTQUOTE:
				if (sc.ch == '#') {
					if (sc.chNext == '#') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_POWERPRO_DEFAULT);
					}
				}
				break;
				
			case SCE_POWERPRO_FUNCTION:
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ' ' || sc.ch == '(') {
					sc.SetState(SCE_POWERPRO_DEFAULT);
				}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_POWERPRO_DEFAULT) {
			if (sc.Match('?', '\"')) {
				sc.SetState(SCE_POWERPRO_VERBATIM);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_POWERPRO_NUMBER);
			}else if (sc.Match('?','#')) {
				if (sc.ch == '?' && sc.chNext == '#') {
					sc.SetState(SCE_POWERPRO_ALTQUOTE);
					sc.Forward();
				}
			} else if (HasFunction(styler, sc.currentPos)) {	//highlight <name> in 'function <name>'
				sc.SetState(SCE_POWERPRO_FUNCTION); 
			} else if (sc.ch == '@' && sc.atLineStart) { 		//alternate function definition [label]
				sc.SetState(SCE_POWERPRO_FUNCTION);
			} else if ((sc.ch > 0) && (setWordStart.Contains(sc.ch) || (sc.ch == '?'))) {
				sc.SetState(SCE_POWERPRO_IDENTIFIER);
			} else if (sc.Match(";;+")) { 
				sc.SetState(SCE_POWERPRO_LINECONTINUE);
			} else if (sc.Match('/', '*')) {
				sc.SetState(SCE_POWERPRO_COMMENTBLOCK);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_POWERPRO_COMMENTLINE);
			} else if (sc.atLineStart && sc.ch == ';') {		//legacy comment that can only appear at the beginning of a line
				sc.SetState(SCE_POWERPRO_COMMENTLINE);
			} else if (sc.Match(";;")) {
				sc.SetState(SCE_POWERPRO_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_POWERPRO_DOUBLEQUOTEDSTRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_POWERPRO_SINGLEQUOTEDSTRING);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POWERPRO_OPERATOR);
			}
		}
	}

	//*************************************
	// Colourize the last word correctly 
	//*************************************
	if (sc.state == SCE_POWERPRO_IDENTIFIER)
	{
		if (keywords.InList(s_save)) {
			sc.ChangeState(SCE_POWERPRO_WORD);
			sc.SetState(SCE_POWERPRO_DEFAULT);
		}
		else if (keywords2.InList(s_save)) {
			sc.ChangeState(SCE_POWERPRO_WORD2);
			sc.SetState(SCE_POWERPRO_DEFAULT);
		}
		else if (keywords3.InList(s_save)) {
			sc.ChangeState(SCE_POWERPRO_WORD3);
			sc.SetState(SCE_POWERPRO_DEFAULT);
		}
		else if (keywords4.InList(s_save)) {
			sc.ChangeState(SCE_POWERPRO_WORD4);
			sc.SetState(SCE_POWERPRO_DEFAULT);
		}
		else {
			sc.SetState(SCE_POWERPRO_DEFAULT);
		}
	}
	sc.Complete();
}

static void FoldPowerProDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler)
{
	//define the character sets
	CharacterSet setWordStart(CharacterSet::setAlpha, "_@", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);

	bool isFoldingAll = true; //used to tell if we're recursively folding the whole document, or just a small piece (ie: if statement or 1 function)
	int endPos = startPos + length;
	int lastLine = styler.GetLine(styler.Length()); //used to help fold the last line correctly

	// get settings from the config files for folding comments and preprocessor lines
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldInComment = styler.GetPropertyInt("fold.comment") == 2;
	bool foldCompact = true;
	
	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		isFoldingAll = false;
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	// vars for style of previous/current/next lines 
	int style = GetStyleFirstWord(lineCurrent,styler);
	int stylePrev = 0;
	
	// find the first previous line without continuation character at the end
	while ((lineCurrent > 0 && IsContinuationLine(lineCurrent,styler)) ||
	       (lineCurrent > 1 && IsContinuationLine(lineCurrent-1,styler))) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
	}
	if (lineCurrent > 0) {
		stylePrev = GetStyleFirstWord(lineCurrent-1,styler);
	}
	// vars for getting first word to check for keywords
	bool FirstWordStart = false;
	bool FirstWordEnd = false;
		
	const unsigned int KEYWORD_MAX = 10;
	char szKeyword[KEYWORD_MAX]="";
	unsigned int	 szKeywordlen = 0;
	
	char szDo[3]="";
	int	 szDolen = 0;
	bool DoFoundLast = false;
	
	// var for indentlevel
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0) {
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	}
	int levelNext = levelCurrent;
	
	int	visibleChars = 0;
	int functionCount = 0;
	
	char chNext = styler.SafeGetCharAt(startPos);
	char chPrev = '\0';
	char chPrevPrev = '\0';
	char chPrevPrevPrev = '\0';
	
	for (int i = startPos; i < endPos; i++) {

		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		
		if ((ch > 0) && setWord.Contains(ch)) {
			visibleChars++;
		}
		
		// get the syle for the current character neede to check in comment
		int stylech = styler.StyleAt(i);
		
		// get first word for the line for indent check max 9 characters
		if (FirstWordStart && (!(FirstWordEnd))) {
			if ((ch > 0) && !setWord.Contains(ch)) {
				FirstWordEnd = true;
			}
			else if (szKeywordlen < KEYWORD_MAX - 1) {
				szKeyword[szKeywordlen++] = static_cast<char>(tolower(ch));
				szKeyword[szKeywordlen] = '\0';
			}
		}
		
		// start the capture of the first word 
		if (!(FirstWordStart)) {
			if ((ch > 0) && (setWord.Contains(ch) || setWordStart.Contains(ch) || ch == ';' || ch == '/')) {
				FirstWordStart = true;
				if (szKeywordlen < KEYWORD_MAX - 1) {
					szKeyword[szKeywordlen++] = static_cast<char>(tolower(ch));
					szKeyword[szKeywordlen] = '\0';
				}
			}
		}
		// only process this logic when not in comment section
		if (stylech != SCE_POWERPRO_COMMENTLINE) {
			if (DoFoundLast) {
				if (DoFoundLast && (ch > 0) && setWord.Contains(ch)) {
					DoFoundLast = false;
				}		
			}
			// find out if the word "do" is the last on a "if" line
			if (FirstWordEnd && strcmp(szKeyword,"if") == 0) {
				if (szDolen == 2) {
					szDo[0] = szDo[1];
					szDo[1] = static_cast<char>(tolower(ch));
					szDo[2] = '\0';
					if (strcmp(szDo,"do") == 0 ) {
						DoFoundLast = true;
					}
				}
				else if (szDolen < 2) {
					szDo[szDolen++] = static_cast<char>(tolower(ch));
					szDo[szDolen] = '\0';
				}
			}
		}

		// End of Line found so process the information 
		 if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
		 
			// **************************
			// Folding logic for Keywords
			// **************************
			
			// if a keyword is found on the current line and the line doesn't end with ;;+ (continuation)
			//    and we are not inside a commentblock.
			if (szKeywordlen > 0 && 
				(!(chPrev == '+' && chPrevPrev == ';' && chPrevPrevPrev ==';')) && 
				((!(IsStreamCommentStyle(style)) || foldInComment)) ) {
			
				// only fold "if" last keyword is "then"  (else its a one line if)
				if (strcmp(szKeyword,"if") == 0  && DoFoundLast) {
						levelNext++;
				}
				// create new fold for these words 
				if (strcmp(szKeyword,"for") == 0) {
					levelNext++;
				}
				
				//handle folding for functions/labels
				//Note: Functions and labels don't have an explicit end like [end function]
				//	1. functions/labels end at the start of another function
				//	2. functions/labels end at the end of the file
				if ((strcmp(szKeyword,"function") == 0) || (szKeywordlen > 0 && szKeyword[0] == '@')) {
					if (isFoldingAll) { //if we're folding the whole document (recursivly by lua script)
						
						if (functionCount > 0) {
							levelCurrent--;
						} else {
							levelNext++;
						}
						functionCount++;	
						
					} else { //if just folding a small piece (by clicking on the minus sign next to the word)
						levelCurrent--;
					}
				}
												
				// end the fold for these words before the current line
				if (strcmp(szKeyword,"endif") == 0 || strcmp(szKeyword,"endfor") == 0) {
						levelNext--;
						levelCurrent--;
				}
				// end the fold for these words before the current line and Start new fold 
				if (strcmp(szKeyword,"else") == 0 || strcmp(szKeyword,"elseif") == 0 ) {
						levelCurrent--;
				}
			}
			// Preprocessor and Comment folding
			int styleNext = GetStyleFirstWord(lineCurrent + 1,styler);

			// *********************************
			// Folding logic for Comment blocks
			// *********************************
			if (foldComment && IsStreamCommentStyle(style)) {
				// Start of a comment block
				if (!(stylePrev==style) && IsStreamCommentStyle(styleNext) && styleNext==style) {
				    levelNext++;
				} 
				// fold till the last line for normal comment lines
				else if (IsStreamCommentStyle(stylePrev) 
						&& !(styleNext == SCE_POWERPRO_COMMENTLINE)
						&& stylePrev == SCE_POWERPRO_COMMENTLINE 
						&& style == SCE_POWERPRO_COMMENTLINE) {
					levelNext--;
				}
				// fold till the one but last line for Blockcomment lines
				else if (IsStreamCommentStyle(stylePrev) 
						&& !(styleNext == SCE_POWERPRO_COMMENTBLOCK)
						&& style == SCE_POWERPRO_COMMENTBLOCK) {
					levelNext--;
					levelCurrent--;
				}
			}

			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			// reset values for the next line
			lineCurrent++;
			stylePrev = style;
			style = styleNext;
			levelCurrent = levelNext;
			visibleChars = 0;
			
			// if the last characters are ;;+ then don't reset since the line continues on the next line.
			if (chPrev == '+' && chPrevPrev == ';' && chPrevPrevPrev == ';') {
				//do nothing
			} else {
				szKeywordlen = 0;
				szDolen = 0;
				FirstWordStart = false;
				FirstWordEnd = false;
				DoFoundLast = false;
				//blank out keyword
				for (unsigned int i = 0; i < KEYWORD_MAX; i++) {
					szKeyword[i] = '\0';
				}
			}
		}

		// save the last processed characters
		if ((ch > 0) && !isspacechar(ch)) {
			chPrevPrevPrev = chPrevPrev;
			chPrevPrev = chPrev;
			chPrev = ch;
			visibleChars++;
		}
	}

	//close folds on the last line - without this a 'phantom' 
	//fold can appear when an open fold is on the last line
	//this can occur because functions and labels don't have an explicit end
	if (lineCurrent >= lastLine) {
		int lev = 0;
		lev |= SC_FOLDLEVELWHITEFLAG;
		styler.SetLevel(lineCurrent, lev);
	}

}

static const char * const powerProWordLists[] = {
            "Keyword list 1",
            "Keyword list 2",
            "Keyword list 3",
            "Keyword list 4",
            0,
        };

static void ColourisePowerProDocWrapper(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                                       Accessor &styler) {
	ColourisePowerProDoc(startPos, length, initStyle, keywordlists, styler, false);
}

LexerModule lmPowerPro(SCLEX_POWERPRO, ColourisePowerProDocWrapper, "powerpro", FoldPowerProDoc, powerProWordLists);
