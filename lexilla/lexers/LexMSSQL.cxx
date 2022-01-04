// Scintilla source code edit control
/** @file LexMSSQL.cxx
 ** Lexer for MSSQL.
 **/
// By Filip Yaghob <fyaghob@gmail.com>
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

#define KW_MSSQL_STATEMENTS         0
#define KW_MSSQL_DATA_TYPES         1
#define KW_MSSQL_SYSTEM_TABLES      2
#define KW_MSSQL_GLOBAL_VARIABLES   3
#define KW_MSSQL_FUNCTIONS          4
#define KW_MSSQL_STORED_PROCEDURES  5
#define KW_MSSQL_OPERATORS          6

static char classifyWordSQL(Sci_PositionU start,
                            Sci_PositionU end,
                            WordList *keywordlists[],
                            Accessor &styler,
                            unsigned int actualState,
							unsigned int prevState) {
	char s[256];
	bool wordIsNumber = isdigit(styler[start]) || (styler[start] == '.');

	WordList &kwStatements          = *keywordlists[KW_MSSQL_STATEMENTS];
    WordList &kwDataTypes           = *keywordlists[KW_MSSQL_DATA_TYPES];
    WordList &kwSystemTables        = *keywordlists[KW_MSSQL_SYSTEM_TABLES];
    WordList &kwGlobalVariables     = *keywordlists[KW_MSSQL_GLOBAL_VARIABLES];
    WordList &kwFunctions           = *keywordlists[KW_MSSQL_FUNCTIONS];
    WordList &kwStoredProcedures    = *keywordlists[KW_MSSQL_STORED_PROCEDURES];
    WordList &kwOperators           = *keywordlists[KW_MSSQL_OPERATORS];

	for (Sci_PositionU i = 0; i < end - start + 1 && i < 128; i++) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		s[i + 1] = '\0';
	}
	char chAttr = SCE_MSSQL_IDENTIFIER;

	if (actualState == SCE_MSSQL_GLOBAL_VARIABLE) {

        if (kwGlobalVariables.InList(&s[2]))
            chAttr = SCE_MSSQL_GLOBAL_VARIABLE;

	} else if (wordIsNumber) {
		chAttr = SCE_MSSQL_NUMBER;

	} else if (prevState == SCE_MSSQL_DEFAULT_PREF_DATATYPE) {
		// Look first in datatypes
        if (kwDataTypes.InList(s))
            chAttr = SCE_MSSQL_DATATYPE;
		else if (kwOperators.InList(s))
			chAttr = SCE_MSSQL_OPERATOR;
		else if (kwStatements.InList(s))
			chAttr = SCE_MSSQL_STATEMENT;
		else if (kwSystemTables.InList(s))
			chAttr = SCE_MSSQL_SYSTABLE;
		else if (kwFunctions.InList(s))
            chAttr = SCE_MSSQL_FUNCTION;
		else if (kwStoredProcedures.InList(s))
			chAttr = SCE_MSSQL_STORED_PROCEDURE;

	} else {
		if (kwOperators.InList(s))
			chAttr = SCE_MSSQL_OPERATOR;
		else if (kwStatements.InList(s))
			chAttr = SCE_MSSQL_STATEMENT;
		else if (kwSystemTables.InList(s))
			chAttr = SCE_MSSQL_SYSTABLE;
		else if (kwFunctions.InList(s))
			chAttr = SCE_MSSQL_FUNCTION;
		else if (kwStoredProcedures.InList(s))
			chAttr = SCE_MSSQL_STORED_PROCEDURE;
		else if (kwDataTypes.InList(s))
			chAttr = SCE_MSSQL_DATATYPE;
	}

	styler.ColourTo(end, chAttr);

	return chAttr;
}

static void ColouriseMSSQLDoc(Sci_PositionU startPos, Sci_Position length,
                              int initStyle, WordList *keywordlists[], Accessor &styler) {


	styler.StartAt(startPos);

	bool fold = styler.GetPropertyInt("fold") != 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int spaceFlags = 0;

	int state = initStyle;
	int prevState = initStyle;
	char chPrev = ' ';
	char chNext = styler[startPos];
	styler.StartSegment(startPos);
	Sci_PositionU lengthDoc = startPos + length;
	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
			int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags);
			int lev = indentCurrent;
			if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
				// Only non whitespace lines can be headers
				int indentNext = styler.IndentAmount(lineCurrent + 1, &spaceFlags);
				if (indentCurrent < (indentNext & ~SC_FOLDLEVELWHITEFLAG)) {
					lev |= SC_FOLDLEVELHEADERFLAG;
				}
			}
			if (fold) {
				styler.SetLevel(lineCurrent, lev);
			}
		}

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			chPrev = ' ';
			i += 1;
			continue;
		}

		// When the last char isn't part of the state (have to deal with it too)...
		if ( (state == SCE_MSSQL_IDENTIFIER) ||
                    (state == SCE_MSSQL_STORED_PROCEDURE) ||
                    (state == SCE_MSSQL_DATATYPE) ||
                    //~ (state == SCE_MSSQL_COLUMN_NAME) ||
                    (state == SCE_MSSQL_FUNCTION) ||
                    //~ (state == SCE_MSSQL_GLOBAL_VARIABLE) ||
                    (state == SCE_MSSQL_VARIABLE)) {
			if (!iswordchar(ch)) {
				int stateTmp;

                if ((state == SCE_MSSQL_VARIABLE) || (state == SCE_MSSQL_COLUMN_NAME)) {
                    styler.ColourTo(i - 1, state);
					stateTmp = state;
                } else
                    stateTmp = classifyWordSQL(styler.GetStartSegment(), i - 1, keywordlists, styler, state, prevState);

				prevState = state;

				if (stateTmp == SCE_MSSQL_IDENTIFIER || stateTmp == SCE_MSSQL_VARIABLE)
					state = SCE_MSSQL_DEFAULT_PREF_DATATYPE;
				else
					state = SCE_MSSQL_DEFAULT;
			}
		} else if (state == SCE_MSSQL_LINE_COMMENT) {
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, state);
				prevState = state;
				state = SCE_MSSQL_DEFAULT;
			}
		} else if (state == SCE_MSSQL_GLOBAL_VARIABLE) {
			if ((ch != '@') && !iswordchar(ch)) {
				classifyWordSQL(styler.GetStartSegment(), i - 1, keywordlists, styler, state, prevState);
				prevState = state;
				state = SCE_MSSQL_DEFAULT;
			}
		}

		// If is the default or one of the above succeeded
		if (state == SCE_MSSQL_DEFAULT || state == SCE_MSSQL_DEFAULT_PREF_DATATYPE) {
			if (iswordstart(ch)) {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_IDENTIFIER;
			} else if (ch == '/' && chNext == '*') {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_COMMENT;
			} else if (ch == '-' && chNext == '-') {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_LINE_COMMENT;
			} else if (ch == '\'') {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_STRING;
			} else if (ch == '"') {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_COLUMN_NAME;
			} else if (ch == '[') {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
				state = SCE_MSSQL_COLUMN_NAME_2;
			} else if (isoperator(ch)) {
				styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				styler.ColourTo(i, SCE_MSSQL_OPERATOR);
                //~ style = SCE_MSSQL_DEFAULT;
				prevState = state;
				state = SCE_MSSQL_DEFAULT;
			} else if (ch == '@') {
                styler.ColourTo(i - 1, SCE_MSSQL_DEFAULT);
				prevState = state;
                if (chNext == '@') {
                    state = SCE_MSSQL_GLOBAL_VARIABLE;
//                    i += 2;
                } else
                    state = SCE_MSSQL_VARIABLE;
            }


		// When the last char is part of the state...
		} else if (state == SCE_MSSQL_COMMENT) {
				if (ch == '/' && chPrev == '*') {
					if (((i > (styler.GetStartSegment() + 2)) || ((initStyle == SCE_MSSQL_COMMENT) &&
					    (styler.GetStartSegment() == startPos)))) {
						styler.ColourTo(i, state);
						//~ state = SCE_MSSQL_COMMENT;
					prevState = state;
                        state = SCE_MSSQL_DEFAULT;
					}
				}
			} else if (state == SCE_MSSQL_STRING) {
				if (ch == '\'') {
					if ( chNext == '\'' ) {
						i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
					} else {
						styler.ColourTo(i, state);
					prevState = state;
						state = SCE_MSSQL_DEFAULT;
					//i++;
					}
				//ch = chNext;
				//chNext = styler.SafeGetCharAt(i + 1);
				}
			} else if (state == SCE_MSSQL_COLUMN_NAME) {
				if (ch == '"') {
					if (chNext == '"') {
						i++;
					ch = chNext;
					chNext = styler.SafeGetCharAt(i + 1);
				} else {
                    styler.ColourTo(i, state);
					prevState = state;
					state = SCE_MSSQL_DEFAULT_PREF_DATATYPE;
					//i++;
                }
                }
		} else if (state == SCE_MSSQL_COLUMN_NAME_2) {
			if (ch == ']') {
                styler.ColourTo(i, state);
				prevState = state;
                state = SCE_MSSQL_DEFAULT_PREF_DATATYPE;
                //i++;
			}
		}

		chPrev = ch;
	}
	styler.ColourTo(lengthDoc - 1, state);
}

static void FoldMSSQLDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool inComment = (styler.StyleAt(startPos-1) == SCE_MSSQL_COMMENT);
    char s[10] = "";
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
        // Comment folding
		if (foldComment) {
			if (!inComment && (style == SCE_MSSQL_COMMENT))
				levelCurrent++;
			else if (inComment && (style != SCE_MSSQL_COMMENT))
				levelCurrent--;
			inComment = (style == SCE_MSSQL_COMMENT);
		}
        if (style == SCE_MSSQL_STATEMENT) {
            // Folding between begin or case and end
            if (ch == 'b' || ch == 'B' || ch == 'c' || ch == 'C' || ch == 'e' || ch == 'E') {
                for (Sci_PositionU j = 0; j < 5; j++) {
					if (!iswordchar(styler[i + j])) {
						break;
					}
					s[j] = static_cast<char>(tolower(styler[i + j]));
					s[j + 1] = '\0';
                }
				if ((strcmp(s, "begin") == 0) || (strcmp(s, "case") == 0)) {
					levelCurrent++;
				}
				if (strcmp(s, "end") == 0) {
					levelCurrent--;
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
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const sqlWordListDesc[] = {
	"Statements",
    "Data Types",
    "System tables",
    "Global variables",
    "Functions",
    "System Stored Procedures",
    "Operators",
	0,
};

LexerModule lmMSSQL(SCLEX_MSSQL, ColouriseMSSQLDoc, "mssql", FoldMSSQLDoc, sqlWordListDesc);
