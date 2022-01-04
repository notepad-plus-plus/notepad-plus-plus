// Scintilla source code edit control
/** @file LexDataflex.cxx
 ** Lexer for DataFlex.
 ** Based on LexPascal.cxx
 ** Written by Wil van Antwerpen, June 2019
 **/

/*
// The License.txt file describes the conditions under which this software may be distributed.

A few words about features of LexDataflex...

Generally speaking LexDataflex tries to support all available DataFlex features (up
to DataFlex 19.1 at this time).

~ FOLDING:

Folding is supported in the following cases:

- Folding of stream-like comments
- Folding of groups of consecutive line comments
- Folding of preprocessor blocks (the following preprocessor blocks are
supported: #IFDEF, #IFNDEF, #ENDIF and #HEADER / #ENDHEADER 
blocks), 
- Folding of code blocks on appropriate keywords (the following code blocks are
supported: "begin, struct, type, case / end" blocks, class & object
declarations and interface declarations)

Remarks:

- We pass 4 arrays to the lexer:
1. The DataFlex keyword list, these are normal DataFlex keywords
2. The Scope Open list, for example, begin / procedure / while
3. The Scope Close list, for example, end / end_procedure / loop
4. Operator list, for ex. + / - / * / Lt / iand
These lists are all mutually exclusive, scope open words should not be in the keyword list and vice versa

- Folding of code blocks tries to handle all special cases in which folding
should not occur. 

~ KEYWORDS:

The list of keywords that can be used in dataflex.properties file (up to DataFlex
19.1):

- Keywords: .. snipped .. see dataflex.properties file.

*/

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


static void GetRangeLowered(Sci_PositionU start,
		Sci_PositionU end,
		Accessor &styler,
		char *s,
		Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static void GetForwardRangeLowered(Sci_PositionU start,
		CharacterSet &charSet,
		Accessor &styler,
		char *s,
		Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < len-1) && charSet.Contains(styler.SafeGetCharAt(start + i))) {
		s[i] = static_cast<char>(tolower(styler.SafeGetCharAt(start + i)));
		i++;
	}
	s[i] = '\0';

}

enum {
	stateInICode = 0x1000,
	stateSingleQuoteOpen = 0x2000,
	stateDoubleQuoteOpen = 0x4000,
	stateFoldInPreprocessor = 0x0100,
	stateFoldInCaseStatement = 0x0200,
	stateFoldInPreprocessorLevelMask = 0x00FF,
	stateFoldMaskAll = 0x0FFF
};


static bool IsFirstDataFlexWord(Sci_Position pos, Accessor &styler) {
	Sci_Position line = styler.GetLine(pos);
	Sci_Position start_pos = styler.LineStart(line);
	for (Sci_Position i = start_pos; i < pos; i++) {
		char ch = styler.SafeGetCharAt(i);
		if (!(ch == ' ' || ch == '\t'))
			return false;
	}
	return true;
}


inline bool IsADataFlexField(int ch) {
	return (ch == '.');
}


static void ClassifyDataFlexWord(WordList *keywordlists[], StyleContext &sc, Accessor &styler) {
	WordList& keywords = *keywordlists[0];
	WordList& scopeOpen   = *keywordlists[1];
	WordList& scopeClosed = *keywordlists[2];
	WordList& operators   = *keywordlists[3];

	char s[100];
	int oldState;
	int newState;
	size_t tokenlen;

    oldState = sc.state;
	newState = oldState;
	sc.GetCurrentLowered(s, sizeof(s));
	tokenlen = strnlen(s,sizeof(s));
	if (keywords.InList(s)) {
		// keywords in DataFlex can be used as table column names (file.field) and as such they
		// should not be characterized as a keyword. So test for that.
		// for ex. somebody using date as field name.
        if (!IsADataFlexField(sc.GetRelative(-static_cast<int>(tokenlen+1)))) {
	      newState = SCE_DF_WORD;
	    }
	}
	if (oldState == newState) {
		if ((scopeOpen.InList(s) || scopeClosed.InList(s)) && (strcmp(s, "for") != 0) && (strcmp(s, "repeat") != 0)) {
			// scope words in DataFlex can be used as table column names (file.field) and as such they
			// should not be characterized as a scope word. So test for that.
			// for ex. somebody using procedure for field name.
			if (!IsADataFlexField(sc.GetRelative(-static_cast<int>(tokenlen+1)))) {
				newState = SCE_DF_SCOPEWORD;
			}
		} 
		// no code folding on the next words, but just want to paint them like keywords (as they are) (??? doesn't the code to the opposite?)
		if (strcmp(s, "if") == 0 ||  
			strcmp(s, "ifnot") == 0 ||
			strcmp(s, "case") == 0 ||
			strcmp(s, "else") == 0 ) {
				newState = SCE_DF_SCOPEWORD;
		} 
	}
	if (oldState != newState && newState == SCE_DF_WORD) {
		// a for loop must have for at the start of the line, for is also used in "define abc for 123"
		if ( (strcmp(s, "for") == 0) && (IsFirstDataFlexWord(sc.currentPos-3, styler)) ) {   
				newState = SCE_DF_SCOPEWORD;
		} 
	}
	if (oldState != newState && newState == SCE_DF_WORD) {
		// a repeat loop must have repeat at the start of the line, repeat is also used in 'move (repeat("d",5)) to sFoo'
		if ( (strcmp(s, "repeat") == 0) && (IsFirstDataFlexWord(sc.currentPos-6, styler)) ) {   
				newState = SCE_DF_SCOPEWORD;
		} 
	}
	if (oldState == newState)  {
	  if (operators.InList(s)) {
		  newState = SCE_DF_OPERATOR;
		} 
	}

	if (oldState != newState) {
		sc.ChangeState(newState);
	}
	sc.SetState(SCE_DF_DEFAULT);
}

static void ColouriseDataFlexDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
		Accessor &styler) {
//	bool bSmartHighlighting = styler.GetPropertyInt("lexer.dataflex.smart.highlighting", 1) != 0;

			CharacterSet setWordStart(CharacterSet::setAlpha, "_$#@", 0x80, true);
			CharacterSet setWord(CharacterSet::setAlphaNum, "_$#@", 0x80, true);
	CharacterSet setNumber(CharacterSet::setDigits, ".-+eE");
	CharacterSet setHexNumber(CharacterSet::setDigits, "abcdefABCDEF");
	CharacterSet setOperator(CharacterSet::setNone, "*+-/<=>^");

	Sci_Position curLine = styler.GetLine(startPos);
	int curLineState = curLine > 0 ? styler.GetLineState(curLine - 1) : 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curLineState);
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_DF_NUMBER:
				if (!setNumber.Contains(sc.ch) || (sc.ch == '.' && sc.chNext == '.')) {
					sc.SetState(SCE_DF_DEFAULT);
				} else if (sc.ch == '-' || sc.ch == '+') {
					if (sc.chPrev != 'E' && sc.chPrev != 'e') {
						sc.SetState(SCE_DF_DEFAULT);
					}
				}
				break;
			case SCE_DF_IDENTIFIER:
				if (!setWord.Contains(sc.ch)) {
					ClassifyDataFlexWord(keywordlists, sc, styler);
				}
				break;
			case SCE_DF_HEXNUMBER:
				if (!(setHexNumber.Contains(sc.ch) || sc.ch == 'I') ) { // in |CI$22a we also want to color the "I"
					sc.SetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_METATAG:
				if (sc.atLineStart || sc.chPrev == '}') {
					sc.SetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_PREPROCESSOR:
				if (sc.atLineStart || IsASpaceOrTab(sc.ch)) {
					sc.SetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_IMAGE:
				if (sc.atLineStart && sc.Match("/*")) {
					sc.Forward();  // these characters are still part of the DF Image
					sc.ForwardSetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_PREPROCESSOR2:
				// we don't have inline comments or preprocessor2 commands
				//if (sc.Match('*', ')')) {
				//	sc.Forward();
				//	sc.ForwardSetState(SCE_DF_DEFAULT);
				//}
				break;
			case SCE_DF_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_DF_STRINGEOL);
				} else if (sc.ch == '\'' && sc.chNext == '\'') {
					sc.Forward();
				} else if (sc.ch == '\"' && sc.chNext == '\"') {
					sc.Forward();
				} else if (sc.ch == '\'' || sc.ch == '\"') {
					if (sc.ch == '\'' && (curLineState & stateSingleQuoteOpen) ) {
 				    curLineState &= ~(stateSingleQuoteOpen);
					sc.ForwardSetState(SCE_DF_DEFAULT);
					}
					else if (sc.ch == '\"' && (curLineState & stateDoubleQuoteOpen) ) {
 				    curLineState &= ~(stateDoubleQuoteOpen);
					sc.ForwardSetState(SCE_DF_DEFAULT);
					}
				}
				break;
			case SCE_DF_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_DF_DEFAULT);
				}
				break;
			case SCE_DF_SCOPEWORD:
				//if (!setHexNumber.Contains(sc.ch) && sc.ch != '$') {
				//	sc.SetState(SCE_DF_DEFAULT);
				//}
				break;
			case SCE_DF_OPERATOR:
//				if (bSmartHighlighting && sc.chPrev == ';') {
//					curLineState &= ~(stateInProperty | stateInExport);
//				}
				sc.SetState(SCE_DF_DEFAULT);
				break;
			case SCE_DF_ICODE:
				if (sc.atLineStart || IsASpace(sc.ch) || isoperator(sc.ch)) {
				sc.SetState(SCE_DF_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_DF_DEFAULT) {
			if (IsADigit(sc.ch)) {
				sc.SetState(SCE_DF_NUMBER);
			} else if (sc.Match('/', '/') || sc.Match("#REM")) {
				sc.SetState(SCE_DF_COMMENTLINE);
			} else if ((sc.ch == '#' && !sc.Match("#REM")) && IsFirstDataFlexWord(sc.currentPos, styler)) {
				sc.SetState(SCE_DF_PREPROCESSOR);
			// || (sc.ch == '|' && sc.chNext == 'C' && sc.GetRelativeCharacter(2) == 'I' && sc.GetRelativeCharacter(3) == '$') ) {
			} else if ((sc.ch == '$' && ((!setWord.Contains(sc.chPrev)) || sc.chPrev == 'I' ) ) || (sc.Match("|CI$")) ) {
				sc.SetState(SCE_DF_HEXNUMBER); // start with $ and previous character not in a..zA..Z0..9 excluding "I" OR start with |CI$
			} else if (setWordStart.Contains(sc.ch)) {
				sc.SetState(SCE_DF_IDENTIFIER);
			} else if (sc.ch == '{') {
				sc.SetState(SCE_DF_METATAG);
			//} else if (sc.Match("(*$")) {
			//	sc.SetState(SCE_DF_PREPROCESSOR2);
			} else if (sc.ch == '/' && setWord.Contains(sc.chNext) &&  sc.atLineStart) {
				sc.SetState(SCE_DF_IMAGE);
			//	sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.ch == '\'' || sc.ch == '\"') {
				if (sc.ch == '\'' && !(curLineState & stateDoubleQuoteOpen)) {
				  curLineState |= stateSingleQuoteOpen;
				} else if (sc.ch == '\"' && !(curLineState & stateSingleQuoteOpen)) {
				  curLineState |= stateDoubleQuoteOpen;
				}
			    sc.SetState(SCE_DF_STRING);
			} else if (setOperator.Contains(sc.ch)) {
				sc.SetState(SCE_DF_OPERATOR);
//			} else if (curLineState & stateInICode) {
				// ICode start ! in a string followed by close string mark is not icode
			} else if ((sc.ch == '!') && !(sc.ch == '!' && ((sc.chNext == '\"') || (sc.ch == '\'')) )) {
				sc.SetState(SCE_DF_ICODE);
			}
		}
	}

	if (sc.state == SCE_DF_IDENTIFIER && setWord.Contains(sc.chPrev)) {
		ClassifyDataFlexWord(keywordlists, sc, styler);
	}

	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_DF_IMAGE;
}

static bool IsCommentLine(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eolPos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' && style == SCE_DF_COMMENTLINE) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}



static unsigned int GetFoldInPreprocessorLevelFlag(int lineFoldStateCurrent) {
	return lineFoldStateCurrent & stateFoldInPreprocessorLevelMask;
}

static void SetFoldInPreprocessorLevelFlag(int &lineFoldStateCurrent, unsigned int nestLevel) {
	lineFoldStateCurrent &= ~stateFoldInPreprocessorLevelMask;
	lineFoldStateCurrent |= nestLevel & stateFoldInPreprocessorLevelMask;
}

static int ClassifyDataFlexPreprocessorFoldPoint(int &levelCurrent, int &lineFoldStateCurrent,
		Sci_PositionU startPos, Accessor &styler) {
	CharacterSet setWord(CharacterSet::setAlpha);

	char s[100];	// Size of the longest possible keyword + one additional character + null
	GetForwardRangeLowered(startPos, setWord, styler, s, sizeof(s));
	size_t iLen = strnlen(s,sizeof(s));
	size_t iWordSize = 0;

	unsigned int nestLevel = GetFoldInPreprocessorLevelFlag(lineFoldStateCurrent);

	if (strcmp(s, "command") == 0 ||
		// The #if/#ifdef etcetera commands are not currently foldable as it is easy to write code that
		// breaks the collaps logic, so we keep things simple and not include that for now.
		strcmp(s, "header") == 0) {
		nestLevel++;
		SetFoldInPreprocessorLevelFlag(lineFoldStateCurrent, nestLevel);
		lineFoldStateCurrent |= stateFoldInPreprocessor;
		levelCurrent++;
		iWordSize = iLen;
	} else if (strcmp(s, "endcommand") == 0 ||
		strcmp(s, "endheader") == 0) {
		nestLevel--;
		SetFoldInPreprocessorLevelFlag(lineFoldStateCurrent, nestLevel);
		if (nestLevel == 0) {
			lineFoldStateCurrent &= ~stateFoldInPreprocessor;
		}
		levelCurrent--;
		iWordSize = iLen;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
	return static_cast<int>(iWordSize);
}


static void ClassifyDataFlexWordFoldPoint(int &levelCurrent, int &lineFoldStateCurrent,
		Sci_PositionU lastStart, Sci_PositionU currentPos, WordList *[], Accessor &styler) {
	char s[100];

	// property fold.dataflex.compilerlist
	//	Set to 1 for enabling the code folding feature in *.prn files
	bool foldPRN = styler.GetPropertyInt("fold.dataflex.compilerlist",0) != 0;

	GetRangeLowered(lastStart, currentPos, styler, s, sizeof(s));

	if (strcmp(s, "case") == 0) {
		lineFoldStateCurrent |= stateFoldInCaseStatement;
	} else if (strcmp(s, "begin") == 0) {
		levelCurrent++;
	} else if (strcmp(s, "for") == 0 ||
		strcmp(s, "while") == 0 ||
		strcmp(s, "repeat") == 0 ||
		strcmp(s, "for_all") == 0 ||
		strcmp(s, "struct") == 0 ||
		strcmp(s, "type") == 0 ||
		strcmp(s, "begin_row") == 0 ||
		strcmp(s, "item_list") == 0 ||
		strcmp(s, "begin_constraints") == 0 ||
		strcmp(s, "begin_transaction") == 0 ||
		strcmp(s, "enum_list") == 0 ||
		strcmp(s, "class") == 0 || 
		strcmp(s, "object") == 0 ||
		strcmp(s, "cd_popup_object") == 0 ||
		strcmp(s, "procedure") == 0 ||
		strcmp(s, "procedure_section") == 0 ||
		strcmp(s, "function") == 0 ) {
			if ((IsFirstDataFlexWord(lastStart, styler )) || foldPRN) {
			levelCurrent++;
			}
	} else if (strcmp(s, "end") == 0) {  // end is not always the first keyword, for example "case end"
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	} else if (strcmp(s, "loop") == 0 ||
				   strcmp(s, "until") == 0 ||
				   strcmp(s, "end_class") == 0 ||
				   strcmp(s, "end_object") == 0 ||
				   strcmp(s, "cd_end_object") == 0 ||
				   strcmp(s, "end_procedure") == 0 ||
				   strcmp(s, "end_function") == 0 ||
				   strcmp(s, "end_for_all") == 0 ||
				   strcmp(s, "end_struct") == 0 ||
				   strcmp(s, "end_type") == 0 ||
				   strcmp(s, "end_row") == 0 ||
				   strcmp(s, "end_item_list") == 0 ||
				   strcmp(s, "end_constraints") == 0 ||
				   strcmp(s, "end_transaction") == 0 ||
				   strcmp(s, "end_enum_list") == 0 ) {
	//		lineFoldStateCurrent &= ~stateFoldInRecord;
			if ((IsFirstDataFlexWord(lastStart, styler )) || foldPRN) {
				levelCurrent--;
				if (levelCurrent < SC_FOLDLEVELBASE) {
					levelCurrent = SC_FOLDLEVELBASE;
				}
			}
	}

}


static void ClassifyDataFlexMetaDataFoldPoint(int &levelCurrent, 
		Sci_PositionU lastStart, Sci_PositionU currentPos, WordList *[], Accessor &styler) {
	char s[100];

	GetRangeLowered(lastStart, currentPos, styler, s, sizeof(s));

    if (strcmp(s, "#beginsection") == 0) {
		levelCurrent++;
	} else if (strcmp(s, "#endsection") == 0) {
			levelCurrent--;
			if (levelCurrent < SC_FOLDLEVELBASE) {
				levelCurrent = SC_FOLDLEVELBASE;
			}
	}

}

static void FoldDataFlexDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
		Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int lineFoldStateCurrent = lineCurrent > 0 ? styler.GetLineState(lineCurrent - 1) & stateFoldMaskAll : 0;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	int iWordSize;

	Sci_Position lastStart = 0;
	CharacterSet setWord(CharacterSet::setAlphaNum, "_$#@", 0x80, true);

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext)) {
				levelCurrent--;
			}
		}
		if (foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
			    && IsCommentLine(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsCommentLine(lineCurrent - 1, styler)
			         && !IsCommentLine(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (foldPreprocessor) {
			if (style == SCE_DF_PREPROCESSOR) {
				iWordSize = ClassifyDataFlexPreprocessorFoldPoint(levelCurrent, lineFoldStateCurrent, i + 1, styler);
			//} else if (style == SCE_DF_PREPROCESSOR2 && ch == '(' && chNext == '*'
			//           && styler.SafeGetCharAt(i + 2) == '$') {
			//	ClassifyDataFlexPreprocessorFoldPoint(levelCurrent, lineFoldStateCurrent, i + 3, styler);
				i = i + iWordSize;
			}
		}

		if (stylePrev != SCE_DF_SCOPEWORD && style == SCE_DF_SCOPEWORD)
		{
			// Store last word start point.
			lastStart = i;
		}
		if (stylePrev == SCE_DF_SCOPEWORD) {
			if(setWord.Contains(ch) && !setWord.Contains(chNext)) {
				ClassifyDataFlexWordFoldPoint(levelCurrent, lineFoldStateCurrent, lastStart, i, keywordlists, styler);
			}
		}

		if (stylePrev == SCE_DF_METATAG && ch == '#')
		{
			// Store last word start point.
			lastStart = i;
		}
		if (stylePrev == SCE_DF_METATAG) {
			if(setWord.Contains(ch) && !setWord.Contains(chNext)) {
				ClassifyDataFlexMetaDataFoldPoint(levelCurrent, lastStart, i, keywordlists, styler);
			}
		}

		if (!IsASpace(ch))
			visibleChars++;

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			int newLineState = (styler.GetLineState(lineCurrent) & ~stateFoldMaskAll) | lineFoldStateCurrent;
			styler.SetLineState(lineCurrent, newLineState);
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
	}

	// If we didn't reach the EOL in previous loop, store line level and whitespace information.
	// The rest will be filled in later...
	int lev = levelPrev;
	if (visibleChars == 0 && foldCompact)
		lev |= SC_FOLDLEVELWHITEFLAG;
	styler.SetLevel(lineCurrent, lev);
}

static const char * const dataflexWordListDesc[] = {
	"Keywords",
	"Scope open",
	"Scope close",
	"Operators",
	0
};

LexerModule lmDataflex(SCLEX_DATAFLEX, ColouriseDataFlexDoc, "dataflex", FoldDataFlexDoc, dataflexWordListDesc);
