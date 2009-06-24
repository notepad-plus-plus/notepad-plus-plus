// Scintilla source code edit control
/** @file LexFortran.cxx
 ** Lexer for Fortran.
 ** Writen by Chuan-jian Shen, Last changed Sep. 2003
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
/***************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
/***************************************/
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

/***********************************************/
static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '%');
}
/**********************************************/
static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch));
}
/***************************************/
inline bool IsABlank(unsigned int ch) {
    return (ch == ' ') || (ch == 0x09) || (ch == 0x0b) ;
}
/***************************************/
inline bool IsALineEnd(char ch) {
    return ((ch == '\n') || (ch == '\r')) ;
}
/***************************************/
unsigned int GetContinuedPos(unsigned int pos, Accessor &styler) {
	while (!IsALineEnd(styler.SafeGetCharAt(pos++))) continue;
	if (styler.SafeGetCharAt(pos) == '\n') pos++;
	while (IsABlank(styler.SafeGetCharAt(pos++))) continue;
	char chCur = styler.SafeGetCharAt(pos);
	if (chCur == '&') {
		while (IsABlank(styler.SafeGetCharAt(++pos))) continue;
		return pos;
	} else {
		return pos;
	}
}
/***************************************/
static void ColouriseFortranDoc(unsigned int startPos, int length, int initStyle,
			WordList *keywordlists[], Accessor &styler, bool isFixFormat) {
	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	/***************************************/
	int posLineStart = 0, numNonBlank = 0, prevState = 0;
	int endPos = startPos + length;
	/***************************************/
	// backtrack to the nearest keyword
	while ((startPos > 1) && (styler.StyleAt(startPos) != SCE_F_WORD)) {
		startPos--;
	}
	startPos = styler.LineStart(styler.GetLine(startPos));
	initStyle = styler.StyleAt(startPos - 1);
	StyleContext sc(startPos, endPos-startPos, initStyle, styler);
	/***************************************/
	for (; sc.More(); sc.Forward()) {
		// remember the start position of the line
		if (sc.atLineStart) {
			posLineStart = sc.currentPos;
			numNonBlank = 0;
			sc.SetState(SCE_F_DEFAULT);
		}
		if (!IsASpaceOrTab(sc.ch)) numNonBlank ++;
		/***********************************************/
		// Handle the fix format generically
		int toLineStart = sc.currentPos - posLineStart;
		if (isFixFormat && (toLineStart < 6 || toLineStart > 72)) {
			if ((toLineStart == 0 && (tolower(sc.ch) == 'c' || sc.ch == '*')) || sc.ch == '!') {
                if (sc.MatchIgnoreCase("cdec$") || sc.MatchIgnoreCase("*dec$") || sc.MatchIgnoreCase("!dec$") ||
                    sc.MatchIgnoreCase("cdir$") || sc.MatchIgnoreCase("*dir$") || sc.MatchIgnoreCase("!dir$") ||
                    sc.MatchIgnoreCase("cms$")  || sc.MatchIgnoreCase("*ms$")  || sc.MatchIgnoreCase("!ms$")  ||
                    sc.chNext == '$') {
                    sc.SetState(SCE_F_PREPROCESSOR);
				} else {
					sc.SetState(SCE_F_COMMENT);
				}

				while (!sc.atLineEnd && sc.More()) sc.Forward(); // Until line end
			} else if (toLineStart > 72) {
				sc.SetState(SCE_F_COMMENT);
				while (!sc.atLineEnd && sc.More()) sc.Forward(); // Until line end
			} else if (toLineStart < 5) {
				if (IsADigit(sc.ch))
					sc.SetState(SCE_F_LABEL);
				else
					sc.SetState(SCE_F_DEFAULT);
			} else if (toLineStart == 5) {
				if (!IsASpace(sc.ch) && sc.ch != '0') {
					sc.SetState(SCE_F_CONTINUATION);
					sc.ForwardSetState(prevState);
				} else
					sc.SetState(SCE_F_DEFAULT);
			}
			continue;
		}
		/***************************************/
		// Handle line continuation generically.
		if (!isFixFormat && sc.ch == '&') {
			char chTemp = ' ';
			int j = 1;
			while (IsABlank(chTemp) && j<132) {
				chTemp = static_cast<char>(sc.GetRelative(j));
				j++;
			}
			if (chTemp == '!') {
				sc.SetState(SCE_F_CONTINUATION);
				if (sc.chNext == '!') sc.ForwardSetState(SCE_F_COMMENT);
			} else if (chTemp == '\r' || chTemp == '\n') {
				int currentState = sc.state;
				sc.SetState(SCE_F_CONTINUATION);
				sc.ForwardSetState(SCE_F_DEFAULT);
				while (IsASpace(sc.ch) && sc.More()) sc.Forward();
				if (sc.ch == '&') {
					sc.SetState(SCE_F_CONTINUATION);
					sc.Forward();
				}
				sc.SetState(currentState);
			}
		}
		/***************************************/
		// Determine if the current state should terminate.
		if (sc.state == SCE_F_OPERATOR) {
			sc.SetState(SCE_F_DEFAULT);
		} else if (sc.state == SCE_F_NUMBER) {
			if (!(IsAWordChar(sc.ch) || sc.ch=='\'' || sc.ch=='\"' || sc.ch=='.')) {
				sc.SetState(SCE_F_DEFAULT);
			}
		} else if (sc.state == SCE_F_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) || (sc.ch == '%')) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_F_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_F_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_F_WORD3);
				}
				sc.SetState(SCE_F_DEFAULT);
			}
		} else if (sc.state == SCE_F_COMMENT || sc.state == SCE_F_PREPROCESSOR) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_F_DEFAULT);
			}
		} else if (sc.state == SCE_F_STRING1) {
			prevState = sc.state;
			if (sc.ch == '\'') {
				if (sc.chNext == '\'') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_F_DEFAULT);
					prevState = SCE_F_DEFAULT;
				}
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_F_STRINGEOL);
				sc.ForwardSetState(SCE_F_DEFAULT);
			}
		} else if (sc.state == SCE_F_STRING2) {
			prevState = sc.state;
			if (sc.atLineEnd) {
				sc.ChangeState(SCE_F_STRINGEOL);
				sc.ForwardSetState(SCE_F_DEFAULT);
			} else if (sc.ch == '\"') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_F_DEFAULT);
					prevState = SCE_F_DEFAULT;
				}
			}
		} else if (sc.state == SCE_F_OPERATOR2) {
			if (sc.ch == '.') {
				sc.ForwardSetState(SCE_F_DEFAULT);
			}
		} else if (sc.state == SCE_F_CONTINUATION) {
			sc.SetState(SCE_F_DEFAULT);
		} else if (sc.state == SCE_F_LABEL) {
			if (!IsADigit(sc.ch)) {
				sc.SetState(SCE_F_DEFAULT);
			} else {
				if (isFixFormat && sc.currentPos-posLineStart > 4)
					sc.SetState(SCE_F_DEFAULT);
				else if (numNonBlank > 5)
					sc.SetState(SCE_F_DEFAULT);
			}
		}
		/***************************************/
		// Determine if a new state should be entered.
		if (sc.state == SCE_F_DEFAULT) {
			if (sc.ch == '!') {
                if (sc.MatchIgnoreCase("!dec$") || sc.MatchIgnoreCase("!dir$") ||
                    sc.MatchIgnoreCase("!ms$") || sc.chNext == '$') {
					sc.SetState(SCE_F_PREPROCESSOR);
				} else {
					sc.SetState(SCE_F_COMMENT);
				}
			} else if ((!isFixFormat) && IsADigit(sc.ch) && numNonBlank == 1) {
				sc.SetState(SCE_F_LABEL);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_F_NUMBER);
			} else if ((tolower(sc.ch) == 'b' || tolower(sc.ch) == 'o' ||
					    tolower(sc.ch) == 'z') && (sc.chNext == '\"' || sc.chNext == '\'')) {
				sc.SetState(SCE_F_NUMBER);
				sc.Forward();
			} else if (sc.ch == '.' && isalpha(sc.chNext)) {
				sc.SetState(SCE_F_OPERATOR2);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_F_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_F_STRING2);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_F_STRING1);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_F_OPERATOR);
			}
		}
	}
	sc.Complete();
}
/***************************************/
// To determine the folding level depending on keywords
static int classifyFoldPointFortran(const char* s, const char* prevWord, const char chNextNonBlank) {
	int lev = 0;
	if ((strcmp(prevWord, "else") == 0 && strcmp(s, "if") == 0) || strcmp(s, "elseif") == 0)
		return -1;
	if (strcmp(s, "associate") == 0 || strcmp(s, "block") == 0
	    || strcmp(s, "blockdata") == 0 || strcmp(s, "select") == 0
	    || strcmp(s, "do") == 0 || strcmp(s, "enum") ==0
	    || strcmp(s, "function") == 0 || strcmp(s, "interface") == 0
		|| strcmp(s, "module") == 0 || strcmp(s, "program") == 0
		|| strcmp(s, "subroutine") == 0 || strcmp(s, "then") == 0
		|| (strcmp(s, "type") == 0 && chNextNonBlank != '(') ){
			if (strcmp(prevWord, "end") == 0)
				lev = 0;
			else
				lev = 1;
	} else if ((strcmp(s, "end") == 0 && chNextNonBlank != '=')
		|| strcmp(s, "endassociate") == 0 || strcmp(s, "endblock") == 0
		|| strcmp(s, "endblockdata") == 0 || strcmp(s, "endselect") == 0
		|| strcmp(s, "enddo") == 0 || strcmp(s, "endenum") ==0
		|| strcmp(s, "endif") == 0 || strcmp(s, "endforall") == 0
		|| strcmp(s, "endfunction") == 0 || strcmp(s, "endinterface") == 0
		|| strcmp(s, "endmodule") == 0 || strcmp(s, "endprogram") == 0
		|| strcmp(s, "endsubroutine") == 0 || strcmp(s, "endtype") == 0
		|| strcmp(s, "endwhere") == 0
		|| strcmp(s, "procedure") == 0 ) { // Take care of the module procedure statement
			lev = -1;
	} else if (strcmp(prevWord, "end") == 0 && strcmp(s, "if") == 0){ // end if
			lev = 0;
	}
	return lev;
}
// Folding the code
static void FoldFortranDoc(unsigned int startPos, int length, int initStyle,
						   Accessor &styler, bool isFixFormat) {
	//
	// bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	// Do not know how to fold the comment at the moment.
	//
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	char chNextNonBlank;
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	/***************************************/
	int lastStart = 0;
	char prevWord[32] = "";
	char Label[6] = "";
	// Variables for do label folding.
	static int doLabels[100];
	static int posLabel=-1;
	/***************************************/
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		chNextNonBlank = chNext;
		unsigned int j=i+1;
		while(IsABlank(chNextNonBlank) && j<endPos) {
			j ++ ;
			chNextNonBlank = styler.SafeGetCharAt(j);
		}
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		//
		if (stylePrev == SCE_F_DEFAULT && (style == SCE_F_WORD || style == SCE_F_LABEL)) {
			// Store last word and label start point.
			lastStart = i;
		}
		/***************************************/
		if (style == SCE_F_WORD) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[32];
				unsigned int k;
				for(k=0; (k<31 ) && (k<i-lastStart+1 ); k++) {
					s[k] = static_cast<char>(tolower(styler[lastStart+k]));
				}
				s[k] = '\0';
				// Handle the forall and where statement and structure.
				if (strcmp(s, "forall") == 0 || strcmp(s, "where") == 0) {
					if (strcmp(prevWord, "end") != 0) {
						j = i + 1;
						char chBrace = '(', chSeek = ')', ch1 = styler.SafeGetCharAt(j);
						// Find the position of the first (
						while (ch1 != chBrace && j<endPos) {
							j++;
							ch1 = styler.SafeGetCharAt(j);
						}
						char styBrace = styler.StyleAt(j);
						int depth = 1;
						char chAtPos;
						char styAtPos;
						while (j<endPos) {
							j++;
							chAtPos = styler.SafeGetCharAt(j);
							styAtPos = styler.StyleAt(j);
							if (styAtPos == styBrace) {
								if (chAtPos == chBrace) depth++;
								if (chAtPos == chSeek) depth--;
								if (depth == 0) break;
							}
						}
						while (j<endPos) {
							j++;
							chAtPos = styler.SafeGetCharAt(j);
							styAtPos = styler.StyleAt(j);
							if (styAtPos == SCE_F_COMMENT || IsABlank(chAtPos)) continue;
							if (isFixFormat) {
								if (!IsALineEnd(chAtPos)) {
									break;
								} else {
									if (lineCurrent < styler.GetLine(styler.Length()-1)) {
										j = styler.LineStart(lineCurrent+1);
										if (styler.StyleAt(j+5) == SCE_F_CONTINUATION) {
											j += 5;
											continue;
										} else {
											levelCurrent++;
											break;
										}
									}
								}
							} else {
								if (chAtPos == '&' && styler.StyleAt(j) == SCE_F_CONTINUATION) {
									j = GetContinuedPos(j+1, styler);
									continue;
								} else if (IsALineEnd(chAtPos)) {
									levelCurrent ++;
									break;
								} else {
									break;
								}
							}
						}
					}
				} else {
					levelCurrent += classifyFoldPointFortran(s, prevWord, chNextNonBlank);
					// Store the do Labels into array
					if (strcmp(s, "do") == 0 && IsADigit(chNextNonBlank)) {
						unsigned int k = 0;
						for (i=j; (i<j+5 && i<endPos); i++) {
							ch = styler.SafeGetCharAt(i);
							if (IsADigit(ch))
								Label[k++] = ch;
							else
								break;
						}
						Label[k] = '\0';
						posLabel ++;
						doLabels[posLabel] = atoi(Label);
					}
				}
				strcpy(prevWord, s);
			}
		} else if (style == SCE_F_LABEL) {
			if(IsADigit(ch) && !IsADigit(chNext)) {
				for(j = 0; ( j < 5 ) && ( j < i-lastStart+1 ); j++) {
					ch = styler.SafeGetCharAt(lastStart + j);
					if (IsADigit(ch) && styler.StyleAt(lastStart+j) == SCE_F_LABEL)
						Label[j] = ch;
					else
						break;
				}
				Label[j] = '\0';
				while (doLabels[posLabel] == atoi(Label) && posLabel > -1) {
					levelCurrent--;
					posLabel--;
				}
			}
		}
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
			strcpy(prevWord, "");
		}
		/***************************************/
		if (!isspacechar(ch)) visibleChars++;
	}
	/***************************************/
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}
/***************************************/
static const char * const FortranWordLists[] = {
	"Primary keywords and identifiers",
	"Intrinsic functions",
	"Extended and user defined functions",
	0,
};
/***************************************/
static void ColouriseFortranDocFreeFormat(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {
	ColouriseFortranDoc(startPos, length, initStyle, keywordlists, styler, false);
}
/***************************************/
static void ColouriseFortranDocFixFormat(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {
	ColouriseFortranDoc(startPos, length, initStyle, keywordlists, styler, true);
}
/***************************************/
static void FoldFortranDocFreeFormat(unsigned int startPos, int length, int initStyle,
		WordList *[], Accessor &styler) {
	FoldFortranDoc(startPos, length, initStyle,styler, false);
}
/***************************************/
static void FoldFortranDocFixFormat(unsigned int startPos, int length, int initStyle,
		WordList *[], Accessor &styler) {
	FoldFortranDoc(startPos, length, initStyle,styler, true);
}
/***************************************/
LexerModule lmFortran(SCLEX_FORTRAN, ColouriseFortranDocFreeFormat, "fortran", FoldFortranDocFreeFormat, FortranWordLists);
LexerModule lmF77(SCLEX_F77, ColouriseFortranDocFixFormat, "f77", FoldFortranDocFixFormat, FortranWordLists);
