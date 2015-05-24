// Scintilla source code edit control
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
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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

static bool IsMatlabCommentChar(int c) {
	return (c == '%') ;
}

static bool IsOctaveCommentChar(int c) {
	return (c == '%' || c == '#') ;
}

static bool IsMatlabComment(Accessor &styler, int pos, int len) {
	return len > 0 && IsMatlabCommentChar(styler[pos]) ;
}

static bool IsOctaveComment(Accessor &styler, int pos, int len) {
	return len > 0 && IsOctaveCommentChar(styler[pos]) ;
}

static void ColouriseMatlabOctaveDoc(
            unsigned int startPos, int length, int initStyle,
            WordList *keywordlists[], Accessor &styler,
            bool (*IsCommentChar)(int)) {

	WordList &keywords = *keywordlists[0];

	styler.StartAt(startPos);

	// boolean for when the ' is allowed to be transpose vs the start/end 
	// of a string
	bool transpose = false;

	// approximate position of first non space character in a line
	int nonSpaceColumn = -1;
	// approximate column position of the current character in a line
	int column = 0;

        // use the line state of each line to store the block comment depth
	int curLine = styler.GetLine(startPos);
        int commentDepth = curLine > 0 ? styler.GetLineState(curLine-1) : 0;


	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward(), column++) {

               	if(sc.atLineStart) {
			// set the line state to the current commentDepth 
			curLine = styler.GetLine(sc.currentPos);
                        styler.SetLineState(curLine, commentDepth);

			// reset the column to 0, nonSpace to -1 (not set)
			column = 0;
			nonSpaceColumn = -1; 
		}

		// save the column position of first non space character in a line
		if((nonSpaceColumn == -1) && (! IsASpace(sc.ch)))
		{
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
				sc.GetCurrentLowered(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = false;
				} else {
					sc.ChangeState(SCE_MATLAB_IDENTIFIER);
					sc.SetState(SCE_MATLAB_DEFAULT);
					transpose = true;
				}
			}
		} else if (sc.state == SCE_MATLAB_NUMBER) {
			if (!isdigit(sc.ch) && sc.ch != '.'
			        && !(sc.ch == 'e' || sc.ch == 'E')
			        && !((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E'))) {
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
			}
		} else if (sc.state == SCE_MATLAB_DOUBLEQUOTESTRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_MATLAB_DEFAULT);
			}
		} else if (sc.state == SCE_MATLAB_COMMAND) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_MATLAB_DEFAULT);
				transpose = false;
			}
		} else if (sc.state == SCE_MATLAB_COMMENT) {
			// end or start of a nested a block comment?
			if( IsCommentChar(sc.ch) && sc.chNext == '}' && nonSpaceColumn == column) {
                           	if(commentDepth > 0) commentDepth --;
 
				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, commentDepth);
				sc.Forward();

				if (commentDepth == 0) {
					sc.ForwardSetState(SCE_D_DEFAULT);
					transpose = false;
				}
                        }
                        else if( IsCommentChar(sc.ch) && sc.chNext == '{' && nonSpaceColumn == column)
                        {
 				commentDepth ++;

				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, commentDepth);
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
				if(sc.chNext == '{' && nonSpaceColumn == column)
					commentDepth ++;
				curLine = styler.GetLine(sc.currentPos);
				styler.SetLineState(curLine, commentDepth);
				sc.SetState(SCE_MATLAB_COMMENT);
			} else if (sc.ch == '!' && sc.chNext != '=' ) {
				sc.SetState(SCE_MATLAB_COMMAND);
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
				if (sc.ch == ')' || sc.ch == ']') {
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

static void ColouriseMatlabDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabCommentChar);
}

static void ColouriseOctaveDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {
	ColouriseMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveCommentChar);
}

static void FoldMatlabOctaveDoc(unsigned int startPos, int length, int,
                                WordList *[], Accessor &styler,
                                bool (*IsComment)(Accessor&, int, int)) {

	int endPos = startPos + length;

	// Backtrack to previous line in case need to fix its fold status
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	int spaceFlags = 0;
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, IsComment);
	char chNext = styler[startPos];
	for (int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			int lev = indentCurrent;
			int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags, IsComment);
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				} else if (indentNext & SC_FOLDLEVELWHITEFLAG) {
					// Line after is blank so check the next - maybe should continue further?
					int spaceFlags2 = 0;
					int indentNext2 = styler.IndentAmount(lineCurrent + 2, &spaceFlags2, IsComment);
					if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext2 & SC_FOLDLEVELNUMBERMASK)) {
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

static void FoldMatlabDoc(unsigned int startPos, int length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsMatlabComment);
}

static void FoldOctaveDoc(unsigned int startPos, int length, int initStyle,
                          WordList *keywordlists[], Accessor &styler) {
	FoldMatlabOctaveDoc(startPos, length, initStyle, keywordlists, styler, IsOctaveComment);
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
