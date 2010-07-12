// Scintilla source code edit control
/** @file LexSQL.cxx
 ** Lexer for SQL, including PL/SQL and SQL*Plus.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

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

static inline bool IsAWordChar(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return (ch < 0x80) && (isalpha(ch) || ch == '_');
}

static inline bool IsADoxygenChar(int ch) {
	return (islower(ch) || ch == '$' || ch == '@' ||
	        ch == '\\' || ch == '&' || ch == '<' ||
	        ch == '>' || ch == '#' || ch == '{' ||
	        ch == '}' || ch == '[' || ch == ']');
}

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
             ch == '.' || ch == '-' || ch == '+');
}

static void ColouriseSQLDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords1 = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &kw_pldoc = *keywordlists[2];
	WordList &kw_sqlplus = *keywordlists[3];
	WordList &kw_user1 = *keywordlists[4];
	WordList &kw_user2 = *keywordlists[5];
	WordList &kw_user3 = *keywordlists[6];
	WordList &kw_user4 = *keywordlists[7];

	StyleContext sc(startPos, length, initStyle, styler);

	// property sql.backslash.escapes 
	//	Enables backslash as an escape character in SQL. 
	bool sqlBackslashEscapes = styler.GetPropertyInt("sql.backslash.escapes", 0) != 0;

	bool sqlBackticksIdentifier = styler.GetPropertyInt("lexer.sql.backticks.identifier", 0) != 0;
	int styleBeforeDCKeyword = SCE_SQL_DEFAULT;
	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_SQL_OPERATOR:
			sc.SetState(SCE_SQL_DEFAULT);
			break;
		case SCE_SQL_NUMBER:
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_SQL_DEFAULT);
			}
			break;
		case SCE_SQL_IDENTIFIER:
			if (!IsAWordChar(sc.ch)) {
				int nextState = SCE_SQL_DEFAULT;
				char s[1000];
				sc.GetCurrentLowered(s, sizeof(s));
				if (keywords1.InList(s)) {
					sc.ChangeState(SCE_SQL_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_SQL_WORD2);
				} else if (kw_sqlplus.InListAbbreviated(s, '~')) {
					sc.ChangeState(SCE_SQL_SQLPLUS);
					if (strncmp(s, "rem", 3) == 0) {
						nextState = SCE_SQL_SQLPLUS_COMMENT;
					} else if (strncmp(s, "pro", 3) == 0) {
						nextState = SCE_SQL_SQLPLUS_PROMPT;
					}
				} else if (kw_user1.InList(s)) {
					sc.ChangeState(SCE_SQL_USER1);
				} else if (kw_user2.InList(s)) {
					sc.ChangeState(SCE_SQL_USER2);
				} else if (kw_user3.InList(s)) {
					sc.ChangeState(SCE_SQL_USER3);
				} else if (kw_user4.InList(s)) {
					sc.ChangeState(SCE_SQL_USER4);
				}
				sc.SetState(nextState);
			}
			break;
		case SCE_SQL_QUOTEDIDENTIFIER:
			if (sc.ch == 0x60) {
				if (sc.chNext == 0x60) {
					sc.Forward();	// Ignore it
				} else {
					sc.ForwardSetState(SCE_SQL_DEFAULT);
				}
			}
			break;
		case SCE_SQL_COMMENT:
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_SQL_DEFAULT);
			}
			break;
		case SCE_SQL_COMMENTDOC:
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_SQL_DEFAULT);
			} else if (sc.ch == '@' || sc.ch == '\\') { // Doxygen support
				// Verify that we have the conditions to mark a comment-doc-keyword
				if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
					styleBeforeDCKeyword = SCE_SQL_COMMENTDOC;
					sc.SetState(SCE_SQL_COMMENTDOCKEYWORD);
				}
			}
			break;
		case SCE_SQL_COMMENTLINE:
		case SCE_SQL_COMMENTLINEDOC:
		case SCE_SQL_SQLPLUS_COMMENT:
		case SCE_SQL_SQLPLUS_PROMPT:
			if (sc.atLineStart) {
				sc.SetState(SCE_SQL_DEFAULT);
			}
			break;
		case SCE_SQL_COMMENTDOCKEYWORD:
			if ((styleBeforeDCKeyword == SCE_SQL_COMMENTDOC) && sc.Match('*', '/')) {
				sc.ChangeState(SCE_SQL_COMMENTDOCKEYWORDERROR);
				sc.Forward();
				sc.ForwardSetState(SCE_SQL_DEFAULT);
			} else if (!IsADoxygenChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (!isspace(sc.ch) || !kw_pldoc.InList(s + 1)) {
					sc.ChangeState(SCE_SQL_COMMENTDOCKEYWORDERROR);
				}
				sc.SetState(styleBeforeDCKeyword);
			}
			break;
		case SCE_SQL_CHARACTER:
			if (sqlBackslashEscapes && sc.ch == '\\') {
				sc.Forward();
			} else if (sc.ch == '\'') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_SQL_DEFAULT);
				}
			}
			break;
		case SCE_SQL_STRING:
			if (sc.ch == '\\') {
				// Escape sequence
				sc.Forward();
			} else if (sc.ch == '\"') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_SQL_DEFAULT);
				}
			}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_SQL_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_SQL_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_SQL_IDENTIFIER);
			} else if (sc.ch == 0x60 && sqlBackticksIdentifier) {
				sc.SetState(SCE_SQL_QUOTEDIDENTIFIER);
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {	// Support of Doxygen doc. style
					sc.SetState(SCE_SQL_COMMENTDOC);
				} else {
					sc.SetState(SCE_SQL_COMMENT);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('-', '-')) {
				// MySQL requires a space or control char after --
				// http://dev.mysql.com/doc/mysql/en/ansi-diff-comments.html
				// Perhaps we should enforce that with proper property:
//~ 			} else if (sc.Match("-- ")) {
				sc.SetState(SCE_SQL_COMMENTLINE);
			} else if (sc.ch == '#') {
				sc.SetState(SCE_SQL_COMMENTLINEDOC);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_SQL_CHARACTER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_SQL_STRING);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_SQL_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_SQL_COMMENT ||
	       style == SCE_SQL_COMMENTDOC ||
	       style == SCE_SQL_COMMENTDOCKEYWORD ||
	       style == SCE_SQL_COMMENTDOCKEYWORDERROR;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment.
static void FoldSQLDoc(unsigned int startPos, int length, int initStyle,
                            WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldOnlyBegin = styler.GetPropertyInt("fold.sql.only.begin", 0) != 0;

	// property fold.sql.exists 
	//	Enables "EXISTS" to end a fold as is started by "IF" in "DROP TABLE IF EXISTS". 
	bool foldSqlExists = styler.GetPropertyInt("fold.sql.exists", 1) != 0;

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0) {
		levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
	}
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool endFound = false;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (foldComment && (style == SCE_SQL_COMMENTLINE)) {
			// MySQL needs -- comments to be followed by space or control char
			if ((ch == '-') && (chNext == '-')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				char chNext3 = styler.SafeGetCharAt(i + 3);
				if (chNext2 == '{' || chNext3 == '{') {
					levelNext++;
				} else if (chNext2 == '}' || chNext3 == '}') {
					levelNext--;
				}
			}
		}
		if (style == SCE_SQL_OPERATOR) {
			if (ch == '(') {
				levelNext++;
			} else if (ch == ')') {
				levelNext--;
			}
		}
		// If new keyword (cannot trigger on elseif or nullif, does less tests)
		if (style == SCE_SQL_WORD && stylePrev != SCE_SQL_WORD) {
			const int MAX_KW_LEN = 6;	// Maximum length of folding keywords
			char s[MAX_KW_LEN + 2];
			unsigned int j = 0;
			for (; j < MAX_KW_LEN + 1; j++) {
				if (!iswordchar(styler[i + j])) {
					break;
				}
				s[j] = static_cast<char>(tolower(styler[i + j]));
			}
			if (j == MAX_KW_LEN + 1) {
				// Keyword too long, don't test it
				s[0] = '\0';
			} else {
				s[j] = '\0';
			}
			if ((!foldOnlyBegin) && (strcmp(s, "if") == 0 || strcmp(s, "loop") == 0)) {
				if (endFound) {
					// ignore
					endFound = false;
				} else {
					levelNext++;
				}
			} else if (strcmp(s, "begin") == 0) {
				levelNext++;
			} else if ((strcmp(s, "end") == 0) ||
//						// DROP TABLE IF EXISTS or CREATE TABLE IF NOT EXISTS
						(foldSqlExists && (strcmp(s, "exists") == 0)) ||
//						//  SQL Anywhere permits IF ... ELSE ... ENDIF
//						//      will only be active if "endif" appears in the 
//						//		keyword list.
						(strcmp(s, "endif") == 0)) {
				endFound = true;
				levelNext--;
				if (levelNext < SC_FOLDLEVELBASE) {
					levelNext = SC_FOLDLEVELBASE;
				}
			}
		}
		if (atEOL) {
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
			visibleChars = 0;
			endFound = false;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
}

static const char * const sqlWordListDesc[] = {
	"Keywords",
	"Database Objects",
	"PLDoc",
	"SQL*Plus",
	"User Keywords 1",
	"User Keywords 2",
	"User Keywords 3",
	"User Keywords 4",
	0
};

LexerModule lmSQL(SCLEX_SQL, ColouriseSQLDoc, "sql", FoldSQLDoc, sqlWordListDesc);
