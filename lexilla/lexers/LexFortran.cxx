// Scintilla source code edit control
/** @file LexFortran.cxx
 ** Lexer for Fortran.
 ** Written by Chuan-jian Shen, Last changed Sep. 2003
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
/***************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
/***************************************/
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
/***************************************/

using namespace Lexilla;

/***********************************************/
static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '%');
}
/**********************************************/
static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch));
}
/***************************************/
static inline bool IsABlank(unsigned int ch) {
	return (ch == ' ') || (ch == 0x09) || (ch == 0x0b) ;
}
/***************************************/
static inline bool IsALineEnd(char ch) {
	return ((ch == '\n') || (ch == '\r')) ;
}
/***************************************/
static Sci_PositionU GetContinuedPos(Sci_PositionU pos, Accessor &styler) {
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
static void ColouriseFortranDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
        WordList *keywordlists[], Accessor &styler, bool isFixFormat) {
	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	/***************************************/
	Sci_Position posLineStart = 0;
	int numNonBlank = 0, prevState = 0;
	Sci_Position endPos = startPos + length;
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
		Sci_Position toLineStart = sc.currentPos - posLineStart;
		if (isFixFormat && (toLineStart < 6 || toLineStart >= 72)) {
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
			} else if (toLineStart >= 72) {
				sc.SetState(SCE_F_COMMENT);
				while (!sc.atLineEnd && sc.More()) sc.Forward(); // Until line end
			} else if (toLineStart < 5) {
				if (IsADigit(sc.ch))
					sc.SetState(SCE_F_LABEL);
				else
					sc.SetState(SCE_F_DEFAULT);
			} else if (toLineStart == 5) {
				//if (!IsASpace(sc.ch) && sc.ch != '0') {
				if (sc.ch != '\r' && sc.ch != '\n') {
					sc.SetState(SCE_F_CONTINUATION);
					if (!IsASpace(sc.ch) && sc.ch != '0')
						sc.ForwardSetState(prevState);
				} else
					sc.SetState(SCE_F_DEFAULT);
			}
			continue;
		}
		/***************************************/
		// Handle line continuation generically.
		if (!isFixFormat && sc.ch == '&' && sc.state != SCE_F_COMMENT) {
			char chTemp = ' ';
			Sci_Position j = 1;
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
				while (IsASpace(sc.ch) && sc.More()) {
					sc.Forward();
					if (sc.atLineStart) numNonBlank = 0;
					if (!IsASpaceOrTab(sc.ch)) numNonBlank ++;
				}
				if (sc.ch == '&') {
					sc.SetState(SCE_F_CONTINUATION);
					sc.Forward();
				}
				sc.SetState(currentState);
			}
		}
		/***************************************/
		// Hanndle preprocessor directives
		if (sc.ch == '#' && numNonBlank == 1)
		{
			sc.SetState(SCE_F_PREPROCESSOR);
			while (!sc.atLineEnd && sc.More())
				sc.Forward(); // Until line end
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
static void CheckLevelCommentLine(const unsigned int nComL,
				  Sci_Position nComColB[], Sci_Position nComColF[], Sci_Position &nComCur,
				  bool comLineB[], bool comLineF[], bool &comLineCur,
				  int &levelDeltaNext) {
	levelDeltaNext = 0;
	if (!comLineCur) {
		return;
	}

	if (!comLineF[0] || nComColF[0] != nComCur) {
		unsigned int i=0;
		for (; i<nComL; i++) {
			if (!comLineB[i] || nComColB[i] != nComCur) {
				break;
			}
		}
		if (i == nComL) {
			levelDeltaNext = -1;
		}
	}
	else if (!comLineB[0] || nComColB[0] != nComCur) {
		unsigned int i=0;
		for (; i<nComL; i++) {
			if (!comLineF[i] || nComColF[i] != nComCur) {
				break;
			}
		}
		if (i == nComL) {
			levelDeltaNext = 1;
		}
	}
}
/***************************************/
static void GetIfLineComment(Accessor &styler, bool isFixFormat, const Sci_Position line, bool &isComLine, Sci_Position &comCol) {
	Sci_Position col = 0;
	isComLine = false;
	Sci_Position pos = styler.LineStart(line);
	Sci_Position len = styler.Length();
	while(pos<len) {
		char ch = styler.SafeGetCharAt(pos);
		if (ch == '!' || (isFixFormat && col == 0 && (tolower(ch) == 'c' || ch == '*'))) {
			isComLine = true;
			comCol = col;
			break;
		}
		else if (!IsABlank(ch) || IsALineEnd(ch)) {
			break;
		}
		pos++;
		col++;
	}
}
/***************************************/
static void StepCommentLine(Accessor &styler, bool isFixFormat, Sci_Position lineCurrent, const unsigned int nComL,
				  Sci_Position nComColB[], Sci_Position nComColF[], Sci_Position &nComCur,
				  bool comLineB[], bool comLineF[], bool &comLineCur) {
	Sci_Position nLineTotal = styler.GetLine(styler.Length()-1) + 1;
	if (lineCurrent >= nLineTotal) {
		return;
	}

	for (int i=nComL-2; i>=0; i--) {
		nComColB[i+1] = nComColB[i];
		comLineB[i+1] = comLineB[i];
	}
	nComColB[0] = nComCur;
	comLineB[0] = comLineCur;
	nComCur = nComColF[0];
	comLineCur = comLineF[0];
	for (unsigned int i=0; i+1<nComL; i++) {
		nComColF[i] = nComColF[i+1];
		comLineF[i] = comLineF[i+1];
	}
	Sci_Position chL = lineCurrent + nComL;
	if (chL < nLineTotal) {
		GetIfLineComment(styler, isFixFormat, chL, comLineF[nComL-1], nComColF[nComL-1]);
	}
	else {
		comLineF[nComL-1] = false;
	}
}
/***************************************/
static void CheckBackComLines(Accessor &styler, bool isFixFormat, Sci_Position lineCurrent, const unsigned int nComL,
				  Sci_Position nComColB[], Sci_Position nComColF[], Sci_Position nComCur,
				  bool comLineB[], bool comLineF[], bool &comLineCur) {
	unsigned int nLines = nComL + nComL + 1;
	bool* comL = new bool[nLines];
	Sci_Position* nComCol = new Sci_Position[nLines];
	bool comL0;
	Sci_Position nComCol0;
	GetIfLineComment(styler, isFixFormat, lineCurrent-nComL-1, comL0, nComCol0);
	for (unsigned int i=0; i<nComL; i++) {
		unsigned copyTo = nComL - i - 1;
		comL[copyTo]    = comLineB[i];
		nComCol[copyTo] = nComColB[i];
	}
	assert(nComL < nLines);
	comL[nComL] = comLineCur;
	nComCol[nComL] = nComCur;
	for (unsigned int i=0; i<nComL; i++) {
		unsigned copyTo = i + nComL + 1;
		comL[copyTo]    = comLineF[i];
		nComCol[copyTo] = nComColF[i];
	}
	
	Sci_Position lineC = lineCurrent - nComL + 1;
	Sci_PositionU iStart;
	if (lineC <= 0) {
		lineC = 0;
		iStart = nComL - lineCurrent;
	}
	else {
		iStart = 1;
	}
	bool levChanged = false;
	int lev = styler.LevelAt(lineC) & SC_FOLDLEVELNUMBERMASK;
	
	for (Sci_PositionU i=iStart; i<=nComL; i++) {
		if (comL[i] && (!comL[i-1] || nComCol[i] != nComCol[i-1])) {
			bool increase = true;
			Sci_PositionU until = i + nComL;
			for (Sci_PositionU j=i+1; j<=until; j++) {
				if (!comL[j] || nComCol[j] != nComCol[i]) {
					increase = false;
					break;
				}
			}
			lev = styler.LevelAt(lineC) & SC_FOLDLEVELNUMBERMASK;
			if (increase) {
				int levH = lev | SC_FOLDLEVELHEADERFLAG;
				lev += 1;
				if (levH != styler.LevelAt(lineC)) {
					styler.SetLevel(lineC, levH);
				}
				for (Sci_Position j=lineC+1; j<=lineCurrent; j++) {
					if (lev != styler.LevelAt(j)) {
						styler.SetLevel(j, lev);
					}
				}
				break;
			}
			else {
				if (lev != styler.LevelAt(lineC)) {
					styler.SetLevel(lineC, lev);
				}
			}
			levChanged = true;
		}
		else if (levChanged && comL[i]) {
			if (lev != styler.LevelAt(lineC)) {
				styler.SetLevel(lineC, lev);
			}
		}
		lineC++;
	}
	delete[] comL;
	delete[] nComCol;
}
/***************************************/
// To determine the folding level depending on keywords
static int classifyFoldPointFortran(const char* s, const char* prevWord, const char chNextNonBlank) {
	int lev = 0;

	if ((strcmp(prevWord, "module") == 0 && strcmp(s, "subroutine") == 0)
		|| (strcmp(prevWord, "module") == 0 && strcmp(s, "function") == 0)) {
		lev = 0;
	} else if (strcmp(s, "associate") == 0 || strcmp(s, "block") == 0
	        || strcmp(s, "blockdata") == 0 || strcmp(s, "select") == 0
	        || strcmp(s, "selecttype") == 0 || strcmp(s, "selectcase") == 0
	        || strcmp(s, "do") == 0 || strcmp(s, "enum") ==0
	        || strcmp(s, "function") == 0 || strcmp(s, "interface") == 0
	        || strcmp(s, "module") == 0 || strcmp(s, "program") == 0
	        || strcmp(s, "subroutine") == 0 || strcmp(s, "then") == 0
	        || (strcmp(s, "type") == 0 && chNextNonBlank != '(')
		|| strcmp(s, "critical") == 0 || strcmp(s, "submodule") == 0){
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
	        || strcmp(s, "endwhere") == 0 || strcmp(s, "endcritical") == 0
		|| (strcmp(prevWord, "module") == 0 && strcmp(s, "procedure") == 0)  // Take care of the "module procedure" statement
		|| strcmp(s, "endsubmodule") == 0 || strcmp(s, "endteam") == 0) {
		lev = -1;
	} else if (strcmp(prevWord, "end") == 0 && strcmp(s, "if") == 0){ // end if
		lev = 0;
	} else if (strcmp(prevWord, "type") == 0 && strcmp(s, "is") == 0){ // type is
		lev = -1;
	} else if ((strcmp(prevWord, "end") == 0 && strcmp(s, "procedure") == 0)
			   || strcmp(s, "endprocedure") == 0) {
			lev = 1; // level back to 0, because no folding support for "module procedure" in submodule
	} else if (strcmp(prevWord, "change") == 0 && strcmp(s, "team") == 0){ // change team
		lev = 1;
	}
	return lev;
}
/***************************************/
// Folding the code
static void FoldFortranDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
        Accessor &styler, bool isFixFormat) {

	bool foldComment = styler.GetPropertyInt("fold.comment", 1) != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	bool isPrevLine;
	if (lineCurrent > 0) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
		isPrevLine = true;
	} else {
		isPrevLine = false;
	}
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	int levelDeltaNext = 0;

	const unsigned int nComL = 3; // defines how many comment lines should be before they are folded
	Sci_Position nComColB[nComL] = {};
	Sci_Position nComColF[nComL] = {};
	Sci_Position nComCur = 0;
	bool comLineB[nComL] = {};
	bool comLineF[nComL] = {};
	bool comLineCur;
	Sci_Position nLineTotal = styler.GetLine(styler.Length()-1) + 1;
	if (foldComment) {
		for (unsigned int i=0; i<nComL; i++) {
			Sci_Position chL = lineCurrent-(i+1);
			if (chL < 0) {
				comLineB[i] = false;
				break;
			}
			GetIfLineComment(styler, isFixFormat, chL, comLineB[i], nComColB[i]);
			if (!comLineB[i]) {
				for (unsigned int j=i+1; j<nComL; j++) {
					comLineB[j] = false;
				}
				break;
			}
		}
		for (unsigned int i=0; i<nComL; i++) {
			Sci_Position chL = lineCurrent+i+1;
			if (chL >= nLineTotal) {
				comLineF[i] = false;
				break;
			}
			GetIfLineComment(styler, isFixFormat, chL, comLineF[i], nComColF[i]);
		}
		GetIfLineComment(styler, isFixFormat, lineCurrent, comLineCur, nComCur);
		CheckBackComLines(styler, isFixFormat, lineCurrent, nComL, nComColB, nComColF, nComCur, 
				comLineB, comLineF, comLineCur);
	}
	int levelCurrent = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;

	/***************************************/
	Sci_Position lastStart = 0;
	char prevWord[32] = "";
	/***************************************/
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		char chNextNonBlank = chNext;
		bool nextEOL = false;
		if (IsALineEnd(chNextNonBlank)) {
			nextEOL = true;
		}
		Sci_PositionU j=i+1;
		while(IsABlank(chNextNonBlank) && j<endPos) {
			j ++ ;
			chNextNonBlank = styler.SafeGetCharAt(j);
			if (IsALineEnd(chNextNonBlank)) {
				nextEOL = true;
			}
		}
		if (!nextEOL && j == endPos) {
			nextEOL = true;
		}
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		//
		if (((isFixFormat && stylePrev == SCE_F_CONTINUATION) || stylePrev == SCE_F_DEFAULT
			|| stylePrev == SCE_F_OPERATOR) && (style == SCE_F_WORD || style == SCE_F_LABEL)) {
			// Store last word and label start point.
			lastStart = i;
		}
		/***************************************/
		if (style == SCE_F_WORD) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[32];
				Sci_PositionU k;
				for(k=0; (k<31 ) && (k<i-lastStart+1 ); k++) {
					s[k] = static_cast<char>(tolower(styler[lastStart+k]));
				}
				s[k] = '\0';
				// Handle the forall and where statement and structure.
				if (strcmp(s, "forall") == 0 || (strcmp(s, "where") == 0 && strcmp(prevWord, "else") != 0)) {
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
						Sci_Position tmpLineCurrent = lineCurrent;
						while (j<endPos) {
							j++;
							chAtPos = styler.SafeGetCharAt(j);
							styAtPos = styler.StyleAt(j);
							if (!IsALineEnd(chAtPos) && (styAtPos == SCE_F_COMMENT || IsABlank(chAtPos))) continue;
							if (isFixFormat) {
								if (!IsALineEnd(chAtPos)) {
									break;
								} else {
									if (tmpLineCurrent < styler.GetLine(styler.Length()-1)) {
										tmpLineCurrent++;
										j = styler.LineStart(tmpLineCurrent);
										if (styler.StyleAt(j+5) == SCE_F_CONTINUATION
											&& !IsABlank(styler.SafeGetCharAt(j+5)) && styler.SafeGetCharAt(j+5) != '0') {
											j += 5;
											continue;
										} else {
											levelDeltaNext++;
											break;
										}
									}
								}
							} else {
								if (chAtPos == '&' && styler.StyleAt(j) == SCE_F_CONTINUATION) {
									j = GetContinuedPos(j+1, styler);
									continue;
								} else if (IsALineEnd(chAtPos)) {
									levelDeltaNext++;
									break;
								} else {
									break;
								}
							}
						}
					}
				} else {
					int wordLevelDelta = classifyFoldPointFortran(s, prevWord, chNextNonBlank);
					levelDeltaNext += wordLevelDelta;
					if (((strcmp(s, "else") == 0) && (nextEOL || chNextNonBlank == '!')) ||
						(strcmp(prevWord, "else") == 0 && strcmp(s, "where") == 0) || strcmp(s, "elsewhere") == 0) {
						if (!isPrevLine) {
							levelCurrent--;
						}
						levelDeltaNext++;
					} else if ((strcmp(prevWord, "else") == 0 && strcmp(s, "if") == 0) || strcmp(s, "elseif") == 0) {
						if (!isPrevLine) {
							levelCurrent--;
						}
					} else if ((strcmp(prevWord, "select") == 0 && strcmp(s, "case") == 0) || strcmp(s, "selectcase") == 0 ||
							   (strcmp(prevWord, "select") == 0 && strcmp(s, "type") == 0) || strcmp(s, "selecttype") == 0) {
						levelDeltaNext += 2;
					} else if ((strcmp(s, "case") == 0 && chNextNonBlank == '(') || (strcmp(prevWord, "case") == 0 && strcmp(s, "default") == 0) ||
							   (strcmp(prevWord, "type") == 0 && strcmp(s, "is") == 0) ||
							   (strcmp(prevWord, "class") == 0 && strcmp(s, "is") == 0) ||
							   (strcmp(prevWord, "class") == 0 && strcmp(s, "default") == 0) ) {
						if (!isPrevLine) {
							levelCurrent--;
						}
						levelDeltaNext++;
					} else if ((strcmp(prevWord, "end") == 0 && strcmp(s, "select") == 0) || strcmp(s, "endselect") == 0) {
						levelDeltaNext -= 2;
					}

					// There are multiple forms of "do" loop. The older form with a label "do 100 i=1,10" would require matching
					// labels to ensure the folding level does not decrease too far when labels are used for other purposes.
					// Since this is difficult, do-label constructs are not folded.
					if (strcmp(s, "do") == 0 && IsADigit(chNextNonBlank)) {
						// Remove delta for do-label
						levelDeltaNext -= wordLevelDelta;
					}
				}
				strcpy(prevWord, s);
			}
		}
		if (atEOL) {
			if (foldComment) {
				int ldNext;
				CheckLevelCommentLine(nComL, nComColB, nComColF, nComCur, comLineB, comLineF, comLineCur, ldNext);
				levelDeltaNext += ldNext;
			}
			int lev = levelCurrent;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelDeltaNext > 0) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
				styler.SetLevel(lineCurrent, lev);

			lineCurrent++;
			levelCurrent += levelDeltaNext;
			levelDeltaNext = 0;
			visibleChars = 0;
			strcpy(prevWord, "");
			isPrevLine = false;

			if (foldComment) {
				StepCommentLine(styler, isFixFormat, lineCurrent, nComL, nComColB, nComColF, nComCur,
						comLineB, comLineF, comLineCur);
			}
		}
		/***************************************/
		if (!isspacechar(ch)) visibleChars++;
	}
	/***************************************/
}
/***************************************/
static const char * const FortranWordLists[] = {
	"Primary keywords and identifiers",
	"Intrinsic functions",
	"Extended and user defined functions",
	0,
};
/***************************************/
static void ColouriseFortranDocFreeFormat(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
        Accessor &styler) {
	ColouriseFortranDoc(startPos, length, initStyle, keywordlists, styler, false);
}
/***************************************/
static void ColouriseFortranDocFixFormat(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
        Accessor &styler) {
	ColouriseFortranDoc(startPos, length, initStyle, keywordlists, styler, true);
}
/***************************************/
static void FoldFortranDocFreeFormat(Sci_PositionU startPos, Sci_Position length, int initStyle,
        WordList *[], Accessor &styler) {
	FoldFortranDoc(startPos, length, initStyle,styler, false);
}
/***************************************/
static void FoldFortranDocFixFormat(Sci_PositionU startPos, Sci_Position length, int initStyle,
        WordList *[], Accessor &styler) {
	FoldFortranDoc(startPos, length, initStyle,styler, true);
}
/***************************************/
extern const LexerModule lmFortran(SCLEX_FORTRAN, ColouriseFortranDocFreeFormat, "fortran", FoldFortranDocFreeFormat, FortranWordLists);
extern const LexerModule lmF77(SCLEX_F77, ColouriseFortranDocFixFormat, "f77", FoldFortranDocFixFormat, FortranWordLists);
