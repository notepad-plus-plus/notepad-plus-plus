// Scintilla source code edit control
// Encoding: UTF-8
/** @file LexMatlab.cxx
 ** Lexer for Matlab.
 ** Written by José Fonseca
 **
 ** Changes by Christoph Dalitz 2003/12/04:
 **   - added support for Octave
 **   - Strings can now be included both in single or double quotes
 **
 ** Changes by John Donoghue 2012/04/02
 **   - added block comment (and nested block comments)
 **   - added ... displayed as a comment
 **   - removed unused IsAWord functions
 **   - added some comments
 **
 ** Changes by John Donoghue 2014/08/01
 **   - fix allowed transpose ' after {} operator
 **
 ** Changes by John Donoghue 2016/11/15
 **   - update matlab code folding
 **
 ** Changes by John Donoghue 2017/01/18
 **   - update matlab block comment detection
 **
 ** Changes by Andrey Smolyakov 2022/04/15
 **   - add support for "arguments" block and class definition syntax
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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

static bool IsMatlabCommentChar(int c) {
	return (c == '%') ;
}

static bool IsOctaveCommentChar(int c) {
	return (c == '%' || c == '#') ;
}

static inline int LowerCase(int c) {
	if (c >= 'A' && c <= 'Z')
		return 'a' + c - 'A';
	return c;
}

static int CheckKeywordFoldPoint(char *str) {
	if (strcmp ("if", str) == 0 ||
		strcmp ("for", str) == 0 ||
		strcmp ("switch", str) == 0 ||
		strcmp ("while", str) == 0 ||
		strcmp ("try", str) == 0 ||
		strcmp ("do", str) == 0 ||
		strcmp ("parfor", str) == 0 ||
		strcmp ("classdef", str) == 0 ||
		strcmp ("spmd", str) == 0 ||
		strcmp ("arguments", str) == 0 ||
		strcmp ("methods", str) == 0 ||
		strcmp ("properties", str) == 0 ||
		strcmp ("events", str) == 0 ||
		strcmp ("function", str) == 0)
		return 1;
	if (strncmp("end", str, 3) == 0 ||
		strcmp("until", str) == 0)
		return -1;
	return 0;
}

static bool IsSpaceToEOL(Sci_Position startPos, Accessor &styler) {
	Sci_Position line = styler.GetLine(startPos);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = startPos; i < eol_pos; i++) {
		char ch = styler[i];
		if(!IsASpace(ch)) return false;
	}
	return true;
}

#define MATLAB_STATE_FOLD_LVL_OFFSET     8
#define MATLAB_STATE_FOLD_LVL_MASK       (0xFF00)
#define MATLAB_STATE_FLAGS_OFFSET        16
#define MATLAB_STATE_COMM_DEPTH_OFFSET   0
#define MATLAB_STATE_COMM_DEPTH_MASK     (0xFF)
#define MATLAB_STATE_EXPECTING_ARG_BLOCK (1 << MATLAB_STATE_FLAGS_OFFSET)
#define MATLAB_STATE_IN_CLASS_SCOPE      (1 <<(MATLAB_STATE_FLAGS_OFFSET+1))
#define MATLAB_STATE_IN_ARGUMENTS_SCOPE  (1 <<(MATLAB_STATE_FLAGS_OFFSET+2))

static int ComposeLineState(int commentDepth,
							int foldingLevel,
							int expectingArgumentsBlock,
							int inClassScope,
							int inArgumentsScope) {

	return  ((commentDepth << MATLAB_STATE_COMM_DEPTH_OFFSET)
				& MATLAB_STATE_COMM_DEPTH_MASK)					|
			((foldingLevel << MATLAB_STATE_FOLD_LVL_OFFSET)
				& MATLAB_STATE_FOLD_LVL_MASK)					|
			(expectingArgumentsBlock
				& MATLAB_STATE_EXPECTING_ARG_BLOCK)				|
			(inClassScope
				& MATLAB_STATE_IN_CLASS_SCOPE)					|
			(inArgumentsScope
				& MATLAB_STATE_IN_ARGUMENTS_SCOPE);
}

static void ColouriseMatlabOctaveDoc(
            Sci_PositionU startPos, Sci_Position length, int initStyle,
            WordList *keywordlists[], Accessor &styler,
            bool (*IsCommentChar)(int),
            bool ismatlab) {

	WordList &keywords = *keywordlists[0];

	styler.StartAt(startPos);

	// boolean for when the ' is allowed to be transpose vs the start/end
	// of a string
	bool transpose = false;

	// count of brackets as boolean for when end could be an operator not a keyword
	int allow_end_op = 0;

	// approximate position of first non space character in a line
	int nonSpaceColumn = -1;
	// approximate column position of the current character in a line
	int column = 0;

	// This line contains a function declaration
	bool funcDeclarationLine = false;
	// We've just seen "function" keyword, so now we may expect the "arguments"
	// keyword opening the corresponding code block
	int expectingArgumentsBlock = 0;
    // We saw "arguments" keyword, but not the closing "end"
    int inArgumentsScope = 0;
	// Current line's folding level
	int foldingLevel = 0;
	// Current line in in class scope
	int inClassScope = 0;

	// use the line state of each line to store the block comment depth
	Sci_Position curLine = styler.GetLine(startPos);
	int commentDepth = 0;
	// Restore the previous line's state, if there was such a line
	if (curLine > 0) {
		int prevState = styler.GetLineState(curLine-1);
		commentDepth = (prevState & MATLAB_STATE_COMM_DEPTH_MASK)
							>> MATLAB_STATE_COMM_DEPTH_OFFSET;
		foldingLevel = (prevState & MATLAB_STATE_FOLD_LVL_MASK)
							>> MATLAB_STATE_FOLD_LVL_OFFSET;
		expectingArgumentsBlock = prevState & MATLAB_STATE_EXPECTING_ARG_BLOCK;
		inClassScope = prevState & MATLAB_STATE_IN_CLASS_SCOPE;
		inArgumentsScope = prevState & MATLAB_STATE_IN_ARGUMENTS_SCOPE;
	}


	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward(), column++) {

		if(sc.atLineStart) {
			// set the line state to the current commentDepth
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, ComposeLineState(
				commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));

			// reset the column to 0, nonSpace to -1 (not set)
			column = 0;
			nonSpaceColumn = -1;

			// Reset the flag
			funcDeclarationLine = false;
		}

		// Semicolon ends function declaration
		// This condition is for one line functions support
		if (sc.chPrev == ';') {
			funcDeclarationLine = false;
		}

		// Only comments allowed between the function declaration and the
		// arguments code block
		if (expectingArgumentsBlock && !(funcDeclarationLine || inArgumentsScope)) {
			if ((sc.state != SCE_MATLAB_KEYWORD) &&
					(sc.state != SCE_MATLAB_COMMENT) &&
					(sc.state != SCE_MATLAB_DEFAULT) &&
					!(sc.state == SCE_MATLAB_OPERATOR && sc.chPrev == ';')) {
				expectingArgumentsBlock = 0;
				styler.SetLineState(curLine, ComposeLineState(
					commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));
			}
		}

		// We've just left the class scope
		if ((foldingLevel ==0) && inClassScope) {
			inClassScope = 0;
		}

		// save the column position of first non space character in a line
		if((nonSpaceColumn == -1) && (! IsASpace(sc.ch))) {
			nonSpaceColumn = column;
		}

		// check for end of states
		if (sc.state == SCE_MATLAB_OPERATOR) {
			if (sc.chPrev == '.') {
				if (sc.ch == '*' || sc.ch == '/' || sc.ch == '\\' || sc.ch == '^') {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
					transpose = true;
				} else if(sc.ch == '.' && sc.chNext == '.') {
					// we werent an operator, but a '...'
					sc.ChangeState(SCE_MATLAB_COMMENT);
					transpose = false;
				} else {
					sc.SetState(SCE_MATLAB_DEFAULT);
				}
			} else {
				sc.SetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_KEYWORD) {
			if (!isalnum(sc.ch) && sc.ch != '_') {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				bool notKeyword = false;
				transpose = false;

				if (keywords.InList(s)) {

					expectingArgumentsBlock = (funcDeclarationLine || inArgumentsScope) ? expectingArgumentsBlock : 0;

					if (strcmp ("end", s) == 0 && allow_end_op) {
						sc.ChangeState(SCE_MATLAB_NUMBER);
						notKeyword = true;
					} else if (strcmp("end", s) == 0 && !allow_end_op) {
						inArgumentsScope = 0;
					} else if (strcmp("function", s) == 0) {
						// Need this flag to handle "arguments" block correctly
						funcDeclarationLine = true;
						expectingArgumentsBlock = ismatlab ? MATLAB_STATE_EXPECTING_ARG_BLOCK : 0;
					} else if (strcmp("classdef", s) == 0) {
						// Need this flag to process "events", "methods" and "properties" blocks
						inClassScope = MATLAB_STATE_IN_CLASS_SCOPE;
					}
				} else {
					// "arguments" is a keyword here, despite not being in the keywords list
					if (expectingArgumentsBlock && !(funcDeclarationLine || inArgumentsScope) && (strcmp("arguments", s) == 0)) {
						// We've entered an "arguments" block
						inArgumentsScope = MATLAB_STATE_IN_ARGUMENTS_SCOPE;
					} else {
						// Found an identifier or a keyword after the function declaration
						// No need to wait for the arguments block anymore
						expectingArgumentsBlock = (funcDeclarationLine || inArgumentsScope) ? expectingArgumentsBlock : 0;

						// "properties", "methods" and "events" are not keywords if they're declared
						// inside some function in methods block
						// To avoid tracking possible nested functions scopes, lexer considers everything
						// beyond level 2 of folding to be in a scope of some function declared in the
						// methods block. It is ok for the valid syntax: classes can only be declared in
						// a separate file, function - only in methods block. However, in case of the invalid
						// syntax lexer may erroneously ignore a keyword.
						if (!((inClassScope) && (foldingLevel <= 2) && (
								strcmp("properties", s) == 0 ||
								strcmp("methods",    s) == 0 ||
								strcmp("events",     s) == 0 ))) {
							sc.ChangeState(SCE_MATLAB_IDENTIFIER);
							transpose = true;
							notKeyword = true;
						}
					}
				}

				sc.SetState(SCE_MATLAB_DEFAULT);
				if (!notKeyword) {
					foldingLevel += CheckKeywordFoldPoint(s);
				}
			}

			styler.SetLineState(curLine, ComposeLineState(
				commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));
		} else if (sc.state == SCE_MATLAB_NUMBER) {
			if (!isdigit(sc.ch) && sc.ch != '.'
			        && !(sc.ch == 'e' || sc.ch == 'E')
			        && !((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E'))
			        && !(((sc.ch == 'x' || sc.ch == 'X') && sc.chPrev == '0') || (sc.ch >= 'a' && sc.ch <= 'f') || (sc.ch >= 'A' && sc.ch <= 'F'))
			        && !(sc.ch == 's' || sc.ch == 'S' || sc.ch == 'u' || sc.ch == 'U')
			        && !(sc.ch == 'i' || sc.ch == 'I' || sc.ch == 'j' || sc.ch == 'J')
			        && !(sc.ch == '_')) {
				sc.SetState(SCE_MATLAB_DEFAULT);
				transpose = true;
			}
		} else if (sc.state == SCE_MATLAB_STRING) {
			if (sc.ch == '\'') {
				if (sc.chNext == '\'') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
				}
			} else if (sc.MatchLineEnd()) {
				sc.SetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_DOUBLEQUOTESTRING) {
			if (sc.ch == '\\' && !ismatlab) {
				sc.Forward(); // skip escape sequence, new line and others after backlash
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_MATLAB_DEFAULT);
			} else if (sc.MatchLineEnd()) {
				sc.SetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_COMMAND) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_MATLAB_DEFAULT);
				transpose = false;
			}
		} else if (sc.state == SCE_MATLAB_COMMENT) {
			// end or start of a nested a block comment?
			if( IsCommentChar(sc.ch) && sc.chNext == '}' && nonSpaceColumn == column && IsSpaceToEOL(sc.currentPos+2, styler)) {
				if(commentDepth > 0) commentDepth --;

				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, ComposeLineState(
					commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));
				sc.Forward();

				if (commentDepth == 0) {
					sc.ForwardSetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				}
			} else if( IsCommentChar(sc.ch) && sc.chNext == '{' && nonSpaceColumn == column && IsSpaceToEOL(sc.currentPos+2, styler)) {
				commentDepth ++;

				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, ComposeLineState(
					commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));
				sc.Forward();
				transpose = false;

			} else if(commentDepth == 0) {
				// single line comment
				if (sc.atLineEnd || sc.ch == '\r' || sc.ch == '\n') {
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				}
			}
		}

		// check start of a new state
		if (sc.state == SCE_MATLAB_DEFAULT) {
			if (IsCommentChar(sc.ch)) {
				// ncrement depth if we are a block comment
				if(sc.chNext == '{' && nonSpaceColumn == column) {
					if(IsSpaceToEOL(sc.currentPos+2, styler)) {
						commentDepth ++;
					}
				}
				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, ComposeLineState(
					commentDepth, foldingLevel, expectingArgumentsBlock, inClassScope, inArgumentsScope));
				sc.SetState(SCE_MATLAB_COMMENT);
			} else if (sc.ch == '!' && sc.chNext != '=' ) {
				if(ismatlab) {
					sc.SetState(SCE_MATLAB_COMMAND);
				} else {
					sc.SetState(SCE_MATLAB_OPERATOR);
				}
			} else if (sc.ch == '\'') {
				if (transpose) {
					sc.SetState(SCE_MATLAB_OPERATOR);
				} else {
					sc.SetState(SCE_MATLAB_STRING);
				}
			} else if (sc.ch == '"') {
				sc.SetState(SCE_MATLAB_DOUBLEQUOTESTRING);
			} else if (isdigit(sc.ch) || (sc.ch == '.' && isdigit(sc.chNext))) {
				sc.SetState(SCE_MATLAB_NUMBER);
			} else if (isalpha(sc.ch)) {
				sc.SetState(SCE_MATLAB_KEYWORD);
			} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '@' || sc.ch == '\\') {
				if (sc.ch == '(' || sc.ch == '[' || sc.ch == '{') {
					allow_end_op ++;
				} else if ((sc.ch == ')' || sc.ch == ']' || sc.ch == '}') && (allow_end_op > 0)) {
					allow_end_op --;
				}

				if (sc.ch == ')' || sc.ch == ']' || sc.ch == '}') {
					transpose = true;
				} else {
					transpose = false;
				}
				sc.SetState(SCE_MATLAB_OPERATOR);
			} else {
				transpose = false;
			}
		}
	}
	sc.Complete();
}

static void ColouriseMatlabDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabCommentChar, true);
}

static void ColouriseOctaveDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveCommentChar, false);
}

static void FoldMatlabOctaveDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                                WordList *[], Accessor &styler,
                                bool (*IsComment)(int ch)) {

	if (styler.GetPropertyInt("fold") == 0)
		return;

	const bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	const bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	char word[100];
	int wordlen = 0;
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		// a line that starts with a comment
		if (foldComment && style == SCE_MATLAB_COMMENT && IsComment(ch) && visibleChars == 0) {
			// start/end of block comment
			if (chNext == '{' && IsSpaceToEOL(i+2, styler))
				levelNext ++;
			if (chNext == '}' && IsSpaceToEOL(i+2, styler))
				levelNext --;
		}
		// keyword
		if(style == SCE_MATLAB_KEYWORD) {
			word[wordlen++] = static_cast<char>(LowerCase(ch));
			if (wordlen == 100) {  // prevent overflow
				word[0] = '\0';
				wordlen = 1;
			}
			if (styleNext !=  SCE_MATLAB_KEYWORD) {
				word[wordlen] = '\0';
				wordlen = 0;

				levelNext += CheckKeywordFoldPoint(word);
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length() - 1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
	}
}

static void FoldMatlabDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabCommentChar);
}

static void FoldOctaveDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveCommentChar);
}

static const char * const matlabWordListDesc[] = {
	"Keywords",
	0
};

static const char * const octaveWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmMatlab(SCLEX_MATLAB, ColouriseMatlabDoc, "matlab", FoldMatlabDoc, matlabWordListDesc);

LexerModule lmOctave(SCLEX_OCTAVE, ColouriseOctaveDoc, "octave", FoldOctaveDoc, octaveWordListDesc);
