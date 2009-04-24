// Scintilla source code edit control
/** @file LexClw.cxx
 ** Lexer for Clarion.
 ** 2004/12/17 Updated Lexer
 **/
// Copyright 2003-2004 by Ron Schofield <ron@schofieldcomputer.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// Is an end of line character
inline bool IsEOL(const int ch) {

	return(ch == '\n');
}

// Convert character to uppercase
static char CharacterUpper(char chChar) {

	if (chChar < 'a' || chChar > 'z') {
		return(chChar);
	}
	else {
		return(static_cast<char>(chChar - 'a' + 'A'));
	}
}

// Convert string to uppercase
static void StringUpper(char *szString) {

	while (*szString) {
		*szString = CharacterUpper(*szString);
		szString++;
	}
}

// Is a label start character
inline bool IsALabelStart(const int iChar) {

	return(isalpha(iChar) || iChar == '_');
}

// Is a label character
inline bool IsALabelCharacter(const int iChar) {

	return(isalnum(iChar) || iChar == '_' || iChar == ':'); 
}

// Is the character is a ! and the the next character is not a ! 
inline bool IsACommentStart(const int iChar) {

	return(iChar == '!');
}

// Is the character a Clarion hex character (ABCDEF)
inline bool IsAHexCharacter(const int iChar, bool bCaseSensitive) {

	// Case insensitive.
	if (!bCaseSensitive) {
		if (strchr("ABCDEFabcdef", iChar) != NULL) {
			return(true);
		}
	}
	// Case sensitive
	else {
		if (strchr("ABCDEF", iChar) != NULL) {
			return(true);
		}
	}
	return(false);
}

// Is the character a Clarion base character (B=Binary, O=Octal, H=Hex)
inline bool IsANumericBaseCharacter(const int iChar, bool bCaseSensitive) {

	// Case insensitive.
	if (!bCaseSensitive) {
		// If character is a numeric base character
		if (strchr("BOHboh", iChar) != NULL) {
			return(true);
		}
	}
	// Case sensitive
	else {
		// If character is a numeric base character
		if (strchr("BOH", iChar) != NULL) {
			return(true);
		}
	}
	return(false);
}

// Set the correct numeric constant state
inline bool SetNumericConstantState(StyleContext &scDoc) {

	int iPoints = 0;			// Point counter
	char cNumericString[512];	// Numeric string buffer

	// Buffer the current numberic string
	scDoc.GetCurrent(cNumericString, sizeof(cNumericString));
	// Loop through the string until end of string (NULL termination)
	for (int iIndex = 0; cNumericString[iIndex] != '\0'; iIndex++) {
		// Depending on the character
		switch (cNumericString[iIndex]) {
			// Is a . (point)
			case '.' :
				// Increment point counter
				iPoints++;
				break;
			default :
				break;
		}	
	}
	// If points found (can be more than one for improper formatted number
	if (iPoints > 0) {
		return(true);
	}
	// Else no points found
	else {
		return(false);
	}
}

// Get the next word in uppercase from the current position (keyword lookahead)
inline bool GetNextWordUpper(Accessor &styler, unsigned int uiStartPos, int iLength, char *cWord) {

	unsigned int iIndex = 0;		// Buffer Index

	// Loop through the remaining string from the current position
	for (int iOffset = uiStartPos; iOffset < iLength; iOffset++) {
		// Get the character from the buffer using the offset
		char cCharacter = styler[iOffset];
		if (IsEOL(cCharacter)) {
			break;
		}
		// If the character is alphabet character
		if (isalpha(cCharacter)) {
			// Add UPPERCASE character to the word buffer
			cWord[iIndex++] = CharacterUpper(cCharacter);
		}
	}
	// Add null termination
	cWord[iIndex] = '\0';
	// If no word was found
	if (iIndex == 0) {
		// Return failure
		return(false);
	}
	// Else word was found
	else {
		// Return success
		return(true);
	}
}

// Clarion Language Colouring Procedure
static void ColouriseClarionDoc(unsigned int uiStartPos, int iLength, int iInitStyle, WordList *wlKeywords[], Accessor &accStyler, bool bCaseSensitive) {

	int iParenthesesLevel = 0;		// Parenthese Level
	int iColumn1Label = false;		// Label starts in Column 1

	WordList &wlClarionKeywords = *wlKeywords[0];			// Clarion Keywords
	WordList &wlCompilerDirectives = *wlKeywords[1];		// Compiler Directives
	WordList &wlRuntimeExpressions = *wlKeywords[2];		// Runtime Expressions
	WordList &wlBuiltInProcsFuncs = *wlKeywords[3];			// Builtin Procedures and Functions
	WordList &wlStructsDataTypes = *wlKeywords[4];			// Structures and Data Types
	WordList &wlAttributes = *wlKeywords[5];				// Procedure Attributes
	WordList &wlStandardEquates = *wlKeywords[6];			// Standard Equates
	WordList &wlLabelReservedWords = *wlKeywords[7];		// Clarion Reserved Keywords (Labels)
	WordList &wlProcLabelReservedWords = *wlKeywords[8];	// Clarion Reserved Keywords (Procedure Labels)

	const char wlProcReservedKeywordList[] = 
	"PROCEDURE FUNCTION";
	WordList wlProcReservedKeywords;
	wlProcReservedKeywords.Set(wlProcReservedKeywordList);

	const char wlCompilerKeywordList[] = 
	"COMPILE OMIT";
	WordList wlCompilerKeywords;
	wlCompilerKeywords.Set(wlCompilerKeywordList);

	const char wlLegacyStatementsList[] =
	"BOF EOF FUNCTION POINTER SHARE";
	WordList wlLegacyStatements;
	wlLegacyStatements.Set(wlLegacyStatementsList);

	StyleContext scDoc(uiStartPos, iLength, iInitStyle, accStyler);

	// lex source code
    for (; scDoc.More(); scDoc.Forward())
	{
		//
		// Determine if the current state should terminate.
		//

		// Label State Handling
		if (scDoc.state == SCE_CLW_LABEL) {
			// If the character is not a valid label
			if (!IsALabelCharacter(scDoc.ch)) {
				// If the character is a . (dot syntax)
				if (scDoc.ch == '.') {
					// Turn off column 1 label flag as label now cannot be reserved work
					iColumn1Label = false;
					// Uncolour the . (dot) to default state, move forward one character,
					// and change back to the label state.
					scDoc.SetState(SCE_CLW_DEFAULT);
					scDoc.Forward();
					scDoc.SetState(SCE_CLW_LABEL);
				}
				// Else check label
				else {
					char cLabel[512];		// Label buffer
					// Buffer the current label string
					scDoc.GetCurrent(cLabel,sizeof(cLabel));
					// If case insensitive, convert string to UPPERCASE to match passed keywords.
					if (!bCaseSensitive) {
						StringUpper(cLabel);
					}
					// Else if UPPERCASE label string is in the Clarion compiler keyword list
					if (wlCompilerKeywords.InList(cLabel) && iColumn1Label){
						// change the label to error state
						scDoc.ChangeState(SCE_CLW_COMPILER_DIRECTIVE);
					}
					// Else if UPPERCASE label string is in the Clarion reserved keyword list
					else if (wlLabelReservedWords.InList(cLabel) && iColumn1Label){
						// change the label to error state
						scDoc.ChangeState(SCE_CLW_ERROR);
					}
					// Else if UPPERCASE label string is 
					else if (wlProcLabelReservedWords.InList(cLabel) && iColumn1Label) {
						char cWord[512];	// Word buffer
						// Get the next word from the current position
						if (GetNextWordUpper(accStyler,scDoc.currentPos,uiStartPos+iLength,cWord)) {
							// If the next word is a procedure reserved word
							if (wlProcReservedKeywords.InList(cWord)) {
								// Change the label to error state
								scDoc.ChangeState(SCE_CLW_ERROR);
							}
						}
					}
					// Else if label string is in the compiler directive keyword list
					else if (wlCompilerDirectives.InList(cLabel)) {
						// change the state to compiler directive state
						scDoc.ChangeState(SCE_CLW_COMPILER_DIRECTIVE);
					}
					// Terminate the label state and set to default state
					scDoc.SetState(SCE_CLW_DEFAULT);
				}
			}
		}
		// Keyword State Handling
		else if (scDoc.state == SCE_CLW_KEYWORD) {
			// If character is : (colon)
			if (scDoc.ch == ':') {
				char cEquate[512];		// Equate buffer
				// Move forward to include : (colon) in buffer
				scDoc.Forward();
				// Buffer the equate string
				scDoc.GetCurrent(cEquate,sizeof(cEquate));
				// If case insensitive, convert string to UPPERCASE to match passed keywords.
				if (!bCaseSensitive) {
					StringUpper(cEquate);
				}
				// If statement string is in the equate list
				if (wlStandardEquates.InList(cEquate)) {
					// Change to equate state
					scDoc.ChangeState(SCE_CLW_STANDARD_EQUATE);
				}
			}
			// If the character is not a valid label character
			else if (!IsALabelCharacter(scDoc.ch)) {
				char cStatement[512];		// Statement buffer
				// Buffer the statement string
				scDoc.GetCurrent(cStatement,sizeof(cStatement));
				// If case insensitive, convert string to UPPERCASE to match passed keywords.
				if (!bCaseSensitive) {
					StringUpper(cStatement);
				}
				// If statement string is in the Clarion keyword list
				if (wlClarionKeywords.InList(cStatement)) {
					// Change the statement string to the Clarion keyword state
					scDoc.ChangeState(SCE_CLW_KEYWORD);
				}
				// Else if statement string is in the compiler directive keyword list
				else if (wlCompilerDirectives.InList(cStatement)) {
					// Change the statement string to the compiler directive state
					scDoc.ChangeState(SCE_CLW_COMPILER_DIRECTIVE);
				}
				// Else if statement string is in the runtime expressions keyword list
				else if (wlRuntimeExpressions.InList(cStatement)) {
					// Change the statement string to the runtime expressions state
					scDoc.ChangeState(SCE_CLW_RUNTIME_EXPRESSIONS);
				}
				// Else if statement string is in the builtin procedures and functions keyword list
				else if (wlBuiltInProcsFuncs.InList(cStatement)) {
					// Change the statement string to the builtin procedures and functions state
					scDoc.ChangeState(SCE_CLW_BUILTIN_PROCEDURES_FUNCTION);
				}
				// Else if statement string is in the tructures and data types keyword list
				else if (wlStructsDataTypes.InList(cStatement)) {
					// Change the statement string to the structures and data types state
					scDoc.ChangeState(SCE_CLW_STRUCTURE_DATA_TYPE);
				}
				// Else if statement string is in the procedure attribute keyword list
				else if (wlAttributes.InList(cStatement)) {
					// Change the statement string to the procedure attribute state
					scDoc.ChangeState(SCE_CLW_ATTRIBUTE);
				}
				// Else if statement string is in the standard equate keyword list
				else if (wlStandardEquates.InList(cStatement)) {
					// Change the statement string to the standard equate state
					scDoc.ChangeState(SCE_CLW_STANDARD_EQUATE);
				}
				// Else if statement string is in the deprecated or legacy keyword list
				else if (wlLegacyStatements.InList(cStatement)) {
					// Change the statement string to the standard equate state
					scDoc.ChangeState(SCE_CLW_DEPRECATED);
				}
				// Else the statement string doesn't match any work list
				else {
					// Change the statement string to the default state
					scDoc.ChangeState(SCE_CLW_DEFAULT);
				}
				// Terminate the keyword state and set to default state
				scDoc.SetState(SCE_CLW_DEFAULT);
			}
		}
		// String State Handling
		else if (scDoc.state == SCE_CLW_STRING) {
			// If the character is an ' (single quote)
			if (scDoc.ch == '\'') {
				// Set the state to default and move forward colouring
				// the ' (single quote) as default state
				// terminating the string state
				scDoc.SetState(SCE_CLW_DEFAULT);
				scDoc.Forward();
			}
			// If the next character is an ' (single quote)
			if (scDoc.chNext == '\'') {
				// Move forward one character and set to default state
				// colouring the next ' (single quote) as default state
				// terminating the string state
				scDoc.ForwardSetState(SCE_CLW_DEFAULT);
				scDoc.Forward();
			}
		}
		// Picture String State Handling
		else if (scDoc.state == SCE_CLW_PICTURE_STRING) {
			// If the character is an ( (open parenthese)
			if (scDoc.ch == '(') {
				// Increment the parenthese level
				iParenthesesLevel++;
			}
			// Else if the character is a ) (close parenthese) 
			else if (scDoc.ch == ')') {
				// If the parenthese level is set to zero
				// parentheses matched
				if (!iParenthesesLevel) {
					scDoc.SetState(SCE_CLW_DEFAULT);
				} 
				// Else parenthese level is greater than zero
				// still looking for matching parentheses
				else {
					// Decrement the parenthese level
					iParenthesesLevel--;
				}
			}
		}
		// Standard Equate State Handling
		else if (scDoc.state == SCE_CLW_STANDARD_EQUATE) {
			if (!isalnum(scDoc.ch)) {
				scDoc.SetState(SCE_CLW_DEFAULT);
			}
		}
		// Integer Constant State Handling
		else if (scDoc.state == SCE_CLW_INTEGER_CONSTANT) {
			// If the character is not a digit (0-9)
			// or character is not a hexidecimal character (A-F)
			// or character is not a . (point)
			// or character is not a numberic base character (B,O,H)
			if (!(isdigit(scDoc.ch)
			|| IsAHexCharacter(scDoc.ch, bCaseSensitive)
			|| scDoc.ch == '.'
			|| IsANumericBaseCharacter(scDoc.ch, bCaseSensitive))) {
				// If the number was a real 
				if (SetNumericConstantState(scDoc)) {
					// Colour the matched string to the real constant state
					scDoc.ChangeState(SCE_CLW_REAL_CONSTANT);
				}
				// Else the number was an integer
				else {
					// Colour the matched string to an integer constant state
					scDoc.ChangeState(SCE_CLW_INTEGER_CONSTANT);
				}
				// Terminate the integer constant state and set to default state
				scDoc.SetState(SCE_CLW_DEFAULT);
			}
		}

		//
		// Determine if a new state should be entered.
		//

		// Beginning of Line Handling
		if (scDoc.atLineStart) {
			// Reset the column 1 label flag
			iColumn1Label = false;
			// If column 1 character is a label start character
			if (IsALabelStart(scDoc.ch)) {
				// Label character is found in column 1
				// so set column 1 label flag and clear last column 1 label
				iColumn1Label = true;
				// Set the state to label
				scDoc.SetState(SCE_CLW_LABEL);
			}
			// else if character is a space or tab
			else if (IsASpace(scDoc.ch)){
				// Set to default state
				scDoc.SetState(SCE_CLW_DEFAULT);
			}
			// else if comment start (!) or is an * (asterisk)
			else if (IsACommentStart(scDoc.ch) || scDoc.ch == '*' ) {
				// then set the state to comment.
				scDoc.SetState(SCE_CLW_COMMENT);
			}
			// else the character is a ? (question mark)
			else if (scDoc.ch == '?') {
				// Change to the compiler directive state, move forward,
				// colouring the ? (question mark), change back to default state.
				scDoc.ChangeState(SCE_CLW_COMPILER_DIRECTIVE);
				scDoc.Forward();
				scDoc.SetState(SCE_CLW_DEFAULT);
			}
			// else an invalid character in column 1
			else {
				// Set to error state
				scDoc.SetState(SCE_CLW_ERROR);
			}
		}
		// End of Line Handling
		else if (scDoc.atLineEnd) {
			// Reset to the default state at the end of each line.
			scDoc.SetState(SCE_CLW_DEFAULT);
		}
		// Default Handling
		else {
			// If in default state 
			if (scDoc.state == SCE_CLW_DEFAULT) {
				// If is a letter could be a possible statement
				if (isalpha(scDoc.ch)) {
					// Set the state to Clarion Keyword and verify later
					scDoc.SetState(SCE_CLW_KEYWORD);
				}
				// else is a number
				else if (isdigit(scDoc.ch)) {
					// Set the state to Integer Constant and verify later
					scDoc.SetState(SCE_CLW_INTEGER_CONSTANT);
				}
				// else if the start of a comment or a | (line continuation)
				else if (IsACommentStart(scDoc.ch) || scDoc.ch == '|') {
					// then set the state to comment.
					scDoc.SetState(SCE_CLW_COMMENT);
				}		
				// else if the character is a ' (single quote)
				else if (scDoc.ch == '\'') {
					// If the character is also a ' (single quote) 
					// Embedded Apostrophe
					if (scDoc.chNext == '\'') {
						// Move forward colouring it as default state
						scDoc.ForwardSetState(SCE_CLW_DEFAULT);
					}
					else {
						// move to the next character and then set the state to comment.
						scDoc.ForwardSetState(SCE_CLW_STRING);
					}
				}		
				// else the character is an @ (ampersand)
				else if (scDoc.ch == '@') {
					// Case insensitive.
					if (!bCaseSensitive) {
						// If character is a valid picture token character
						if (strchr("DEKNPSTdeknpst", scDoc.chNext) != NULL) {
							// Set to the picture string state
							scDoc.SetState(SCE_CLW_PICTURE_STRING);
						}
					}
					// Case sensitive
					else {
						// If character is a valid picture token character
						if (strchr("DEKNPST", scDoc.chNext) != NULL) {
							// Set the picture string state
							scDoc.SetState(SCE_CLW_PICTURE_STRING);
						}
					}
				}		
			}
		}
	}
	// lexing complete
	scDoc.Complete();
}

// Clarion Language Case Sensitive Colouring Procedure
static void ColouriseClarionDocSensitive(unsigned int uiStartPos, int iLength, int iInitStyle, WordList *wlKeywords[], Accessor &accStyler) {

	ColouriseClarionDoc(uiStartPos, iLength, iInitStyle, wlKeywords, accStyler, true);
}

// Clarion Language Case Insensitive Colouring Procedure
static void ColouriseClarionDocInsensitive(unsigned int uiStartPos, int iLength, int iInitStyle, WordList *wlKeywords[], Accessor &accStyler) {

	ColouriseClarionDoc(uiStartPos, iLength, iInitStyle, wlKeywords, accStyler, false);
}

// Fill Buffer

static void FillBuffer(unsigned int uiStart, unsigned int uiEnd, Accessor &accStyler, char *szBuffer, unsigned int uiLength) {

	unsigned int uiPos = 0;

	while ((uiPos < uiEnd - uiStart + 1) && (uiPos < uiLength-1)) {
		szBuffer[uiPos] = static_cast<char>(toupper(accStyler[uiStart + uiPos]));
		uiPos++;
	}
	szBuffer[uiPos] = '\0';
}

// Classify Clarion Fold Point

static int ClassifyClarionFoldPoint(int iLevel, const char* szString) {

	if (!(isdigit(szString[0]) || (szString[0] == '.'))) {
		if (strcmp(szString, "PROCEDURE") == 0) {
	//		iLevel = SC_FOLDLEVELBASE + 1;
		}
		else if (strcmp(szString, "MAP") == 0 ||
			strcmp(szString,"ACCEPT") == 0 ||
			strcmp(szString,"BEGIN") == 0 ||
			strcmp(szString,"CASE") == 0 ||
			strcmp(szString,"EXECUTE") == 0 ||
			strcmp(szString,"IF") == 0 ||
			strcmp(szString,"ITEMIZE") == 0 ||
			strcmp(szString,"INTERFACE") == 0 ||
			strcmp(szString,"JOIN") == 0 ||
			strcmp(szString,"LOOP") == 0 ||
			strcmp(szString,"MODULE") == 0 ||
			strcmp(szString,"RECORD") == 0) {
			iLevel++;
		}
		else if (strcmp(szString, "APPLICATION") == 0 ||
			strcmp(szString, "CLASS") == 0 ||
			strcmp(szString, "DETAIL") == 0 ||
			strcmp(szString, "FILE") == 0 ||
			strcmp(szString, "FOOTER") == 0 ||
			strcmp(szString, "FORM") == 0 ||
			strcmp(szString, "GROUP") == 0 ||
			strcmp(szString, "HEADER") == 0 ||
			strcmp(szString, "INTERFACE") == 0 ||
			strcmp(szString, "MENU") == 0 ||
			strcmp(szString, "MENUBAR") == 0 ||
			strcmp(szString, "OLE") == 0 ||
			strcmp(szString, "OPTION") == 0 ||
			strcmp(szString, "QUEUE") == 0 ||
			strcmp(szString, "REPORT") == 0 ||
			strcmp(szString, "SHEET") == 0 ||
			strcmp(szString, "TAB") == 0 ||
			strcmp(szString, "TOOLBAR") == 0 ||
			strcmp(szString, "VIEW") == 0 ||
			strcmp(szString, "WINDOW") == 0) {
			iLevel++;
		}
		else if (strcmp(szString, "END") == 0 ||
			strcmp(szString, "UNTIL") == 0 ||
			strcmp(szString, "WHILE") == 0) {
			iLevel--;
		}
	}
	return(iLevel);
}

// Clarion Language Folding Procedure
static void FoldClarionDoc(unsigned int uiStartPos, int iLength, int iInitStyle, WordList *[], Accessor &accStyler) {

	unsigned int uiEndPos = uiStartPos + iLength;
	int iLineCurrent = accStyler.GetLine(uiStartPos);
	int iLevelPrev = accStyler.LevelAt(iLineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int iLevelCurrent = iLevelPrev;
	char chNext = accStyler[uiStartPos];
	int iStyle = iInitStyle;
	int iStyleNext = accStyler.StyleAt(uiStartPos);
	int iVisibleChars = 0;
	int iLastStart = 0;

	for (unsigned int uiPos = uiStartPos; uiPos < uiEndPos; uiPos++) {

		char chChar = chNext;
		chNext = accStyler.SafeGetCharAt(uiPos + 1);
		int iStylePrev = iStyle;
		iStyle = iStyleNext;
		iStyleNext = accStyler.StyleAt(uiPos + 1);
		bool bEOL = (chChar == '\r' && chNext != '\n') || (chChar == '\n');
	
		if (iStylePrev == SCE_CLW_DEFAULT) {
			if (iStyle == SCE_CLW_KEYWORD || iStyle == SCE_CLW_STRUCTURE_DATA_TYPE) {
				// Store last word start point.
				iLastStart = uiPos;
			}
		}

		if (iStylePrev == SCE_CLW_KEYWORD || iStylePrev == SCE_CLW_STRUCTURE_DATA_TYPE) {
			if(iswordchar(chChar) && !iswordchar(chNext)) {
				char chBuffer[100];
				FillBuffer(iLastStart, uiPos, accStyler, chBuffer, sizeof(chBuffer));
				iLevelCurrent = ClassifyClarionFoldPoint(iLevelCurrent,chBuffer);
			//	if ((iLevelCurrent == SC_FOLDLEVELBASE + 1) && iLineCurrent > 1) {
			//		accStyler.SetLevel(iLineCurrent-1,SC_FOLDLEVELBASE);
			//		iLevelPrev = SC_FOLDLEVELBASE;
			//	}
			}
		}

		if (bEOL) {
			int iLevel = iLevelPrev;
			if ((iLevelCurrent > iLevelPrev) && (iVisibleChars > 0))
				iLevel |= SC_FOLDLEVELHEADERFLAG;
			if (iLevel != accStyler.LevelAt(iLineCurrent)) {
				accStyler.SetLevel(iLineCurrent,iLevel);
			}
			iLineCurrent++;
			iLevelPrev = iLevelCurrent;
			iVisibleChars = 0;
		}
		
		if (!isspacechar(chChar))
			iVisibleChars++;
	}

	// Fill in the real level of the next line, keeping the current flags
	// as they will be filled in later.
	int iFlagsNext = accStyler.LevelAt(iLineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	accStyler.SetLevel(iLineCurrent, iLevelPrev | iFlagsNext);
}

// Word List Descriptions
static const char * const rgWordListDescriptions[] = {
	"Clarion Keywords",
	"Compiler Directives",
	"Built-in Procedures and Functions",
	"Runtime Expressions",
	"Structure and Data Types",
	"Attributes",
	"Standard Equates",
	"Reserved Words (Labels)",
	"Reserved Words (Procedure Labels)",
	0,
};

// Case Sensitive Clarion Language Lexer
LexerModule lmClw(SCLEX_CLW, ColouriseClarionDocSensitive, "clarion", FoldClarionDoc, rgWordListDescriptions);

// Case Insensitive Clarion Language Lexer
LexerModule lmClwNoCase(SCLEX_CLWNOCASE, ColouriseClarionDocInsensitive, "clarionnocase", FoldClarionDoc, rgWordListDescriptions);
