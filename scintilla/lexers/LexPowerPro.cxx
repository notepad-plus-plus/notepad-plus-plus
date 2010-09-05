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
//	2008-12-14 - Added bounds checking for szFirstWord and szDo
//			   - Replaced SetOfCharacters with CharacterSet
//			   - Made sure that CharacterSet::Contains is passed only positive values
//			   - Made sure that the return value of Accessor::SafeGetCharAt is positive before
//				 passing to functions that require positive values like isspacechar()
//			   - Removed unused visibleChars processing from ColourisePowerProDoc()
//			   - Fixed bug with folding logic where line continuations didn't end where
//				 they were supposed to
//			   - Moved all helper functions to the top of the file
//	2010-06-03 - Added onlySpaces variable to allow the @function and ;comment styles to be indented
//			   - Modified HasFunction function to be a bit more robust
//			   - Renamed HasFunction function to IsFunction
//			   - Cleanup
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
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

static inline bool IsStreamCommentStyle(int style) {
	return style == SCE_POWERPRO_COMMENTBLOCK;
}

static inline bool IsLineEndChar(unsigned char ch) {
	return 	ch == 0x0a 		//LF
			|| ch == 0x0c	//FF
			|| ch == 0x0d;	//CR
}

static bool IsContinuationLine(unsigned int szLine, Accessor &styler)
{
	int startPos = styler.LineStart(szLine);
	int endPos = styler.LineStart(szLine + 1) - 2;
	while (startPos < endPos)
	{
		char stylech = styler.StyleAt(startPos);
		if (!(stylech == SCE_POWERPRO_COMMENTBLOCK)) {
			char ch = styler.SafeGetCharAt(endPos);
			char chPrev = styler.SafeGetCharAt(endPos - 1);
			char chPrevPrev = styler.SafeGetCharAt(endPos - 2);
			if (ch > 0 && chPrev > 0 && chPrevPrev > 0 && !isspacechar(ch) && !isspacechar(chPrev) && !isspacechar(chPrevPrev) )
				return (chPrevPrev == ';' && chPrev == ';' && ch == '+');
			}
		endPos--; // skip to next char
	}
	return false;
}

// Routine to find first none space on the current line and return its Style
// needed for comment lines not starting on pos 1
static int GetStyleFirstWord(int szLine, Accessor &styler)
{
	int startPos = styler.LineStart(szLine);
	int endPos = styler.LineStart(szLine + 1) - 1;
	char ch = styler.SafeGetCharAt(startPos);

	while (ch > 0 && isspacechar(ch) && startPos < endPos)
	{
		startPos++; // skip to next char
		ch = styler.SafeGetCharAt(startPos);
	}
	return styler.StyleAt(startPos);
}

//returns true if there is a function to highlight
//used to highlight <name> in 'function <name>'
//note:
//		sample line (without quotes): "\tfunction asdf()
//		currentPos will be the position of 'a'
static bool IsFunction(Accessor &styler, unsigned int currentPos) {

	const char function[10] = "function "; //10 includes \0
	unsigned int numberOfCharacters = sizeof(function) - 1;
	unsigned int position = currentPos - numberOfCharacters;

	//compare each character with the letters in the function array
	//return false if ALL don't match
	for (unsigned int i = 0; i < numberOfCharacters; i++) {
		char c = styler.SafeGetCharAt(position++);
		if (c != function[i])
			return false;
	}

	//make sure that there are only spaces (or tabs) between the beginning
	//of the line and the function declaration
	position = currentPos - numberOfCharacters - 1; 		//-1 to move to char before 'function'
	for (unsigned int j = 0; j < 16; j++) {					//check up to 16 preceeding characters
		char c = styler.SafeGetCharAt(position--, '\0');	//if can't read char, return NUL (past beginning of document)
		if (c <= 0)	//reached beginning of document
			return true;
		if (c > 0 && IsLineEndChar(c))
			return true;
		else if (c > 0 && !IsASpaceOrTab(c))
			return false;
	}

	//fall-through
	return false;
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

	//are there only spaces between the first letter of the line and the beginning of the line
	bool onlySpaces = true;

	for (; sc.More(); sc.Forward()) {

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
				if (isspacechar(sc.ch) || sc.ch == '(') {
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
			} else if (IsFunction(styler, sc.currentPos)) {	//highlight <name> in 'function <name>'
				sc.SetState(SCE_POWERPRO_FUNCTION);
			} else if (onlySpaces && sc.ch == '@') { 		//alternate function definition [label]
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
			} else if (onlySpaces && sc.ch == ';') {		//legacy comment that can only have blank space in front of it
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

		//maintain a record of whether or not all the preceding characters on
		//a line are space characters
		if (onlySpaces && !IsASpaceOrTab(sc.ch))
			onlySpaces = false;

		//reset when starting a new line
		if (sc.atLineEnd)
			onlySpaces = true;
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

	//used to tell if we're recursively folding the whole document, or just a small piece (ie: if statement or 1 function)
	bool isFoldingAll = true;

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
	while ((lineCurrent > 0 && IsContinuationLine(lineCurrent, styler))
	       || (lineCurrent > 1 && IsContinuationLine(lineCurrent - 1, styler))) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
	}

	if (lineCurrent > 0) {
		stylePrev = GetStyleFirstWord(lineCurrent-1,styler);
	}

	// vars for getting first word to check for keywords
	bool isFirstWordStarted = false;
	bool isFirstWordEnded = false;

	const unsigned int FIRST_WORD_MAX_LEN = 10;
	char szFirstWord[FIRST_WORD_MAX_LEN] = "";
	unsigned int firstWordLen = 0;

	char szDo[3]="";
	int	 szDolen = 0;
	bool isDoLastWord = false;

	// var for indentlevel
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
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

		if ((ch > 0) && setWord.Contains(ch))
			visibleChars++;

		// get the syle for the current character neede to check in comment
		int stylech = styler.StyleAt(i);

		// start the capture of the first word
		if (!isFirstWordStarted && (ch > 0)) {
			if (setWord.Contains(ch) || setWordStart.Contains(ch) || ch == ';' || ch == '/') {
				isFirstWordStarted = true;
				if (firstWordLen < FIRST_WORD_MAX_LEN - 1) {
					szFirstWord[firstWordLen++] = static_cast<char>(tolower(ch));
					szFirstWord[firstWordLen] = '\0';
				}
			}
		} // continue capture of the first word on the line
		else if (isFirstWordStarted && !isFirstWordEnded && (ch > 0)) {
			if (!setWord.Contains(ch)) {
				isFirstWordEnded = true;
			}
			else if (firstWordLen < (FIRST_WORD_MAX_LEN - 1)) {
				szFirstWord[firstWordLen++] = static_cast<char>(tolower(ch));
				szFirstWord[firstWordLen] = '\0';
			}
		}

		if (stylech != SCE_POWERPRO_COMMENTLINE) {

			//reset isDoLastWord if we find a character(ignoring spaces) after 'do'
			if (isDoLastWord && (ch > 0) && setWord.Contains(ch))
				isDoLastWord = false;

			// --find out if the word "do" is the last on a "if" line--
			// collect each letter and put it into a buffer 2 chars long
			// if we end up with "do" in the buffer when we reach the end of
			// the line, "do" was the last word on the line
			if ((ch > 0) && isFirstWordEnded && strcmp(szFirstWord, "if") == 0) {
				if (szDolen == 2) {
					szDo[0] = szDo[1];
					szDo[1] = static_cast<char>(tolower(ch));
					szDo[2] = '\0';

					if (strcmp(szDo, "do") == 0)
						isDoLastWord = true;

				} else if (szDolen < 2) {
					szDo[szDolen++] = static_cast<char>(tolower(ch));
					szDo[szDolen] = '\0';
				}
			}
		}

		// End of Line found so process the information
		 if ((ch == '\r' && chNext != '\n') // \r\n
			|| ch == '\n' 					// \n
			|| i == endPos) {				// end of selection

			// **************************
			// Folding logic for Keywords
			// **************************

			// if a keyword is found on the current line and the line doesn't end with ;;+ (continuation)
			//    and we are not inside a commentblock.
			if (firstWordLen > 0
				&& chPrev != '+' && chPrevPrev != ';' && chPrevPrevPrev !=';'
				&& (!IsStreamCommentStyle(style) || foldInComment) ) {

				// only fold "if" last keyword is "then"  (else its a one line if)
				if (strcmp(szFirstWord, "if") == 0  && isDoLastWord)
						levelNext++;

				// create new fold for these words
				if (strcmp(szFirstWord, "for") == 0)
					levelNext++;

				//handle folding for functions/labels
				//Note: Functions and labels don't have an explicit end like [end function]
				//	1. functions/labels end at the start of another function
				//	2. functions/labels end at the end of the file
				if ((strcmp(szFirstWord, "function") == 0) || (firstWordLen > 0 && szFirstWord[0] == '@')) {
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
				if (strcmp(szFirstWord, "endif") == 0 || strcmp(szFirstWord, "endfor") == 0) {
						levelNext--;
						levelCurrent--;
				}

				// end the fold for these words before the current line and Start new fold
				if (strcmp(szFirstWord, "else") == 0 || strcmp(szFirstWord, "elseif") == 0 )
						levelCurrent--;

			}
			// Preprocessor and Comment folding
			int styleNext = GetStyleFirstWord(lineCurrent + 1,styler);

			// *********************************
			// Folding logic for Comment blocks
			// *********************************
			if (foldComment && IsStreamCommentStyle(style)) {

				// Start of a comment block
				if (stylePrev != style && IsStreamCommentStyle(styleNext) && styleNext == style) {
				    levelNext++;
				} // fold till the last line for normal comment lines
				else if (IsStreamCommentStyle(stylePrev)
						&& styleNext != SCE_POWERPRO_COMMENTLINE
						&& stylePrev == SCE_POWERPRO_COMMENTLINE
						&& style == SCE_POWERPRO_COMMENTLINE) {
					levelNext--;
				} // fold till the one but last line for Blockcomment lines
				else if (IsStreamCommentStyle(stylePrev)
						&& styleNext != SCE_POWERPRO_COMMENTBLOCK
						&& style == SCE_POWERPRO_COMMENTBLOCK) {
					levelNext--;
					levelCurrent--;
				}
			}

			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
				styler.SetLevel(lineCurrent, lev);

			// reset values for the next line
			lineCurrent++;
			stylePrev = style;
			style = styleNext;
			levelCurrent = levelNext;
			visibleChars = 0;

			// if the last characters are ;;+ then don't reset since the line continues on the next line.
			if (chPrev != '+' && chPrevPrev != ';' && chPrevPrevPrev != ';') {
				firstWordLen = 0;
				szDolen = 0;
				isFirstWordStarted = false;
				isFirstWordEnded = false;
				isDoLastWord = false;

				//blank out first word
				for (unsigned int i = 0; i < FIRST_WORD_MAX_LEN; i++)
					szFirstWord[i] = '\0';
			}
		}

		// save the last processed characters
		if ((ch > 0) && !isspacechar(ch)) {
			chPrevPrevPrev = chPrevPrev;
			chPrevPrev = chPrev;
			chPrev = ch;
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


