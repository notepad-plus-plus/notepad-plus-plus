// SciTE - Scintilla based Text Editor
// LexBullant.cxx - lexer for Bullant

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

static int classifyWordBullant(Sci_PositionU start, Sci_PositionU end, WordList &keywords, Accessor &styler) {
	char s[100];
	s[0] = '\0';
	for (Sci_PositionU i = 0; i < end - start + 1 && i < 30; i++) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		s[i + 1] = '\0';
	}
	int lev= 0;
	char chAttr = SCE_C_IDENTIFIER;
	if (isdigit(s[0]) || (s[0] == '.')){
		chAttr = SCE_C_NUMBER;
	}
	else {
		if (keywords.InList(s)) {
			chAttr = SCE_C_WORD;
			if (strcmp(s, "end") == 0)
				lev = -1;
			else if (strcmp(s, "method") == 0 ||
				strcmp(s, "case") == 0 ||
				strcmp(s, "class") == 0 ||
				strcmp(s, "debug") == 0 ||
				strcmp(s, "test") == 0 ||
				strcmp(s, "if") == 0 ||
				strcmp(s, "lock") == 0 ||
				strcmp(s, "transaction") == 0 ||
				strcmp(s, "trap") == 0 ||
				strcmp(s, "until") == 0 ||
				strcmp(s, "while") == 0)
				lev = 1;
		}
	}
	styler.ColourTo(end, chAttr);
	return lev;
}

static void ColouriseBullantDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
	Accessor &styler) {
	WordList &keywords = *keywordlists[0];

	styler.StartAt(startPos);

	bool fold = styler.GetPropertyInt("fold") != 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;

	int state = initStyle;
	if (state == SCE_C_STRINGEOL)	// Does not leak onto next line
		state = SCE_C_DEFAULT;
	char chPrev = ' ';
	char chNext = styler[startPos];
	Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	styler.StartSegment(startPos);
	int endFoundThisLine = 0;
	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
			// Trigger on CR only (Mac style) or either on LF from CR+LF (Dos/Win) or on LF alone (Unix)
			// Avoid triggering two times on Dos/Win
			// End of line
			endFoundThisLine = 0;
			if (state == SCE_C_STRINGEOL) {
				styler.ColourTo(i, state);
				state = SCE_C_DEFAULT;
			}
			if (fold) {
				int lev = levelPrev;
				if (visibleChars == 0)
					lev |= SC_FOLDLEVELWHITEFLAG;
				if ((levelCurrent > levelPrev) && (visibleChars > 0))
					lev |= SC_FOLDLEVELHEADERFLAG;
				styler.SetLevel(lineCurrent, lev);
				lineCurrent++;
				levelPrev = levelCurrent;
			}
			visibleChars = 0;

/*			int indentBlock = GetLineIndentation(lineCurrent);
			if (blockChange==1){
				lineCurrent++;
				int pos=SetLineIndentation(lineCurrent, indentBlock + indentSize);
			} else if (blockChange==-1) {
				indentBlock -= indentSize;
				if (indentBlock < 0)
					indentBlock = 0;
				SetLineIndentation(lineCurrent, indentBlock);
				lineCurrent++;
			}
			blockChange=0;
*/		}
		if (!(IsASCII(ch) && isspace(ch)))
			visibleChars++;

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			chPrev = ' ';
			i += 1;
			continue;
		}

		if (state == SCE_C_DEFAULT) {
			if (iswordstart(ch)) {
				styler.ColourTo(i-1, state);
					state = SCE_C_IDENTIFIER;
			} else if (ch == '@' && chNext == 'o') {
				if ((styler.SafeGetCharAt(i+2) =='f') && (styler.SafeGetCharAt(i+3) == 'f')) {
					styler.ColourTo(i-1, state);
					state = SCE_C_COMMENT;
				}
			} else if (ch == '#') {
				styler.ColourTo(i-1, state);
				state = SCE_C_COMMENTLINE;
			} else if (ch == '\"') {
				styler.ColourTo(i-1, state);
				state = SCE_C_STRING;
			} else if (ch == '\'') {
				styler.ColourTo(i-1, state);
				state = SCE_C_CHARACTER;
			} else if (isoperator(ch)) {
				styler.ColourTo(i-1, state);
				styler.ColourTo(i, SCE_C_OPERATOR);
			}
		} else if (state == SCE_C_IDENTIFIER) {
			if (!iswordchar(ch)) {
				int levelChange = classifyWordBullant(styler.GetStartSegment(), i - 1, keywords, styler);
				state = SCE_C_DEFAULT;
				chNext = styler.SafeGetCharAt(i + 1);
				if (ch == '#') {
					state = SCE_C_COMMENTLINE;
				} else if (ch == '\"') {
					state = SCE_C_STRING;
				} else if (ch == '\'') {
					state = SCE_C_CHARACTER;
				} else if (isoperator(ch)) {
					styler.ColourTo(i, SCE_C_OPERATOR);
				}
				if (endFoundThisLine == 0)
					levelCurrent+=levelChange;
				if (levelChange == -1)
					endFoundThisLine=1;
			}
		} else if (state == SCE_C_COMMENT) {
			if (ch == '@' && chNext == 'o') {
				if (styler.SafeGetCharAt(i+2) == 'n') {
					styler.ColourTo(i+2, state);
					state = SCE_C_DEFAULT;
					i+=2;
				}
			}
		} else if (state == SCE_C_COMMENTLINE) {
			if (ch == '\r' || ch == '\n') {
				endFoundThisLine = 0;
				styler.ColourTo(i-1, state);
				state = SCE_C_DEFAULT;
			}
		} else if (state == SCE_C_STRING) {
			if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
				}
			} else if (ch == '\"') {
				styler.ColourTo(i, state);
				state = SCE_C_DEFAULT;
			} else if (chNext == '\r' || chNext == '\n') {
				endFoundThisLine = 0;
				styler.ColourTo(i-1, SCE_C_STRINGEOL);
				state = SCE_C_STRINGEOL;
			}
		} else if (state == SCE_C_CHARACTER) {
			if ((ch == '\r' || ch == '\n') && (chPrev != '\\')) {
				endFoundThisLine = 0;
				styler.ColourTo(i-1, SCE_C_STRINGEOL);
				state = SCE_C_STRINGEOL;
			} else if (ch == '\\') {
				if (chNext == '\"' || chNext == '\'' || chNext == '\\') {
					i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
				}
			} else if (ch == '\'') {
				styler.ColourTo(i, state);
				state = SCE_C_DEFAULT;
			}
		}
		chPrev = ch;
	}
	styler.ColourTo(lengthDoc - 1, state);

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	if (fold) {
		int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
		//styler.SetLevel(lineCurrent, levelCurrent | flagsNext);
		styler.SetLevel(lineCurrent, levelPrev | flagsNext);

	}
}

static const char * const bullantWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmBullant(SCLEX_BULLANT, ColouriseBullantDoc, "bullant", 0, bullantWordListDesc);
