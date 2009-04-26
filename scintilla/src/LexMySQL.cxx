// Scintilla source code edit control
/** @file LexMySQL.cxx
 ** Lexer for MySQL
 **/
// Adopted from LexSQL.cxx by Anders Karlsson <anders@mysql.com>
// Original work by Neil Hodgson <neilh@scintilla.org>
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

static void ColouriseMySQLDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &major_keywords = *keywordlists[0];
	WordList &keywords = *keywordlists[1];
	WordList &database_objects = *keywordlists[2];
	WordList &functions = *keywordlists[3];
	WordList &system_variables = *keywordlists[4];
	WordList &procedure_keywords = *keywordlists[5];
	WordList &kw_user1 = *keywordlists[6];
	WordList &kw_user2 = *keywordlists[7];
	WordList &kw_user3 = *keywordlists[8];

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_MYSQL_OPERATOR:
			sc.SetState(SCE_MYSQL_DEFAULT);
			break;
		case SCE_MYSQL_NUMBER:
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			if (!IsANumberChar(sc.ch)) {
				sc.SetState(SCE_MYSQL_DEFAULT);
			}
			break;
		case SCE_MYSQL_IDENTIFIER:
			if (!IsAWordChar(sc.ch)) {
				int nextState = SCE_MYSQL_DEFAULT;
				char s[1000];
				sc.GetCurrentLowered(s, sizeof(s));
				if (major_keywords.InList(s)) {
					sc.ChangeState(SCE_MYSQL_MAJORKEYWORD);
				} else if (keywords.InList(s)) {
					sc.ChangeState(SCE_MYSQL_KEYWORD);
				} else if (database_objects.InList(s)) {
					sc.ChangeState(SCE_MYSQL_DATABASEOBJECT);
				} else if (functions.InList(s)) {
					sc.ChangeState(SCE_MYSQL_FUNCTION);
				} else if (procedure_keywords.InList(s)) {
					sc.ChangeState(SCE_MYSQL_PROCEDUREKEYWORD);
				} else if (kw_user1.InList(s)) {
					sc.ChangeState(SCE_MYSQL_USER1);
				} else if (kw_user2.InList(s)) {
					sc.ChangeState(SCE_MYSQL_USER2);
				} else if (kw_user3.InList(s)) {
					sc.ChangeState(SCE_MYSQL_USER3);
				}
				sc.SetState(nextState);
			}
			break;
		case SCE_MYSQL_VARIABLE:
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_MYSQL_DEFAULT);
			}
			break;
		case SCE_MYSQL_SYSTEMVARIABLE:
			if (!IsAWordChar(sc.ch)) {
				char s[1000];
				sc.GetCurrentLowered(s, sizeof(s));
// Check for known system variables here.
				if (system_variables.InList(&s[2])) {
					sc.ChangeState(SCE_MYSQL_KNOWNSYSTEMVARIABLE);
				}
				sc.SetState(SCE_MYSQL_DEFAULT);
			}
			break;
		case SCE_MYSQL_QUOTEDIDENTIFIER:
			if (sc.ch == 0x60) {
				if (sc.chNext == 0x60) {
					sc.Forward();	// Ignore it
				} else {
					sc.ForwardSetState(SCE_MYSQL_DEFAULT);
				}
			}
			break;
		case SCE_MYSQL_COMMENT:
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_MYSQL_DEFAULT);
			}
			break;
		case SCE_MYSQL_COMMENTLINE:
			if (sc.atLineStart) {
				sc.SetState(SCE_MYSQL_DEFAULT);
			}
			break;
		case SCE_MYSQL_SQSTRING:
			if (sc.ch == '\\') {
				// Escape sequence
				sc.Forward();
			} else if (sc.ch == '\'') {
				if (sc.chNext == '\'') {
					sc.Forward();
				} else {
					sc.ChangeState(SCE_MYSQL_STRING);
					sc.ForwardSetState(SCE_MYSQL_DEFAULT);
				}
			}
			break;
		case SCE_MYSQL_DQSTRING:
			if (sc.ch == '\\') {
				// Escape sequence
				sc.Forward();
			} else if (sc.ch == '\"') {
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					sc.ChangeState(SCE_MYSQL_STRING);
					sc.ForwardSetState(SCE_MYSQL_DEFAULT);
				}
			}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_MYSQL_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_MYSQL_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_MYSQL_IDENTIFIER);
// Note that the order of SYSTEMVARIABLE and VARIABLE is important here.
			} else if (sc.ch == 0x40 && sc.chNext == 0x40) {
				sc.SetState(SCE_MYSQL_SYSTEMVARIABLE);
				sc.Forward(); // Skip past the second at-sign.
			} else if (sc.ch == 0x40) {
				sc.SetState(SCE_MYSQL_VARIABLE);
			} else if (sc.ch == 0x60) {
				sc.SetState(SCE_MYSQL_QUOTEDIDENTIFIER);
			} else if (sc.Match('/', '*')) {
				sc.SetState(SCE_MYSQL_COMMENT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('-', '-') || sc.Match('#')) {
				sc.SetState(SCE_MYSQL_COMMENTLINE);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_MYSQL_SQSTRING);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_MYSQL_DQSTRING);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_MYSQL_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_MYSQL_COMMENT;
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment.
static void FoldMySQLDoc(unsigned int startPos, int length, int initStyle,
                            WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldOnlyBegin = styler.GetPropertyInt("fold.sql.only.begin", 0) != 0;

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
	bool whenFound = false;
	bool elseFound = false;
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
		if (foldComment && (style == SCE_MYSQL_COMMENTLINE)) {
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
		if (style == SCE_MYSQL_OPERATOR) {
			if (ch == '(') {
				levelNext++;
			} else if (ch == ')') {
				levelNext--;
			}
		}

// Style new keywords here.
		if ((style == SCE_MYSQL_MAJORKEYWORD && stylePrev != SCE_MYSQL_MAJORKEYWORD)
		  || (style == SCE_MYSQL_KEYWORD && stylePrev != SCE_MYSQL_KEYWORD)
		  || (style == SCE_MYSQL_PROCEDUREKEYWORD && stylePrev != SCE_MYSQL_PROCEDUREKEYWORD)) {
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
			if (!foldOnlyBegin && endFound && (strcmp(s, "if") == 0 || strcmp(s, "while") == 0 || strcmp(s, "loop") == 0)) {
				endFound = false;
				levelNext--;
				if (levelNext < SC_FOLDLEVELBASE) {
					levelNext = SC_FOLDLEVELBASE;
				}
// Note that else is special here. It may or may be followed by an if then, but in aly case the level stays the
// same. When followed by a if .. then, the level will be increased later, if not, at eol.
			} else if (!foldOnlyBegin && strcmp(s, "else") == 0) {
				levelNext--;
				elseFound = true;
			} else if (!foldOnlyBegin && strcmp(s, "then") == 0) {
				if(whenFound) {
					whenFound = false;
				} else {
					levelNext++;
				}
			} else if (strcmp(s, "if") == 0) {
				elseFound = false;
			} else if (strcmp(s, "when") == 0) {
				whenFound = true;
			} else if (strcmp(s, "begin") == 0) {
				levelNext++;
			} else if (!foldOnlyBegin && (strcmp(s, "loop") == 0 || strcmp(s, "repeat") == 0
			  || strcmp(s, "while") == 0)) {
				if(endFound) {
					endFound = false;
				} else {
					levelNext++;
				}
			} else if (strcmp(s, "end") == 0) {
// Multiple END in a row are counted multiple times!
				if (endFound) {
					levelNext--;
					if (levelNext < SC_FOLDLEVELBASE) {
						levelNext = SC_FOLDLEVELBASE;
					}
				}
				endFound = true;
				whenFound = false;
			}
		}
// Handle this for a trailing end withiut an if / while etc, as in the case of a begin.
		if (endFound) {
			endFound = false;
			levelNext--;
			if (levelNext < SC_FOLDLEVELBASE) {
				levelNext = SC_FOLDLEVELBASE;
			}
		}
		if (atEOL) {
			if(elseFound)
				levelNext++;
			elseFound = false;

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
			whenFound = false;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
}

static const char * const mysqlWordListDesc[] = {
	"Major Keywords",
	"Keywords",
	"Database Objects",
	"Functions",
	"System Variables",
	"Procedure keywords",
	"User Keywords 1",
	"User Keywords 2",
	"User Keywords 3"
};

LexerModule lmMySQL(SCLEX_MYSQL, ColouriseMySQLDoc, "mysql", FoldMySQLDoc, mysqlWordListDesc);
