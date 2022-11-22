//-*- coding: utf-8 -*-
// Scintilla source code edit control
/** @file LexSQL.cxx
 ** Lexer for SQL, including PL/SQL and SQL*Plus.
 ** Improved by Jérôme LAFORGE <jerome.laforge_AT_gmail_DOT_com> from 2010 to 2012.
 **/
// Copyright 1998-2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SparseState.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

static inline bool IsAWordChar(int ch, bool sqlAllowDottedWord) {
	if (!sqlAllowDottedWord)
		return (ch < 0x80) && (isalnum(ch) || ch == '_');
	else
		return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.');
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

static inline bool IsANumberChar(int ch, int chPrev) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	       (isdigit(ch) || toupper(ch) == 'E' ||
	        ch == '.' || ((ch == '-' || ch == '+') && chPrev < 0x80 && toupper(chPrev) == 'E'));
}

typedef unsigned int sql_state_t;

class SQLStates {
public :
	void Set(Sci_Position lineNumber, unsigned short int sqlStatesLine) {
		sqlStatement.Set(lineNumber, sqlStatesLine);
	}

	sql_state_t IgnoreWhen (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_IGNORE_WHEN;
		else
			sqlStatesLine &= ~MASK_IGNORE_WHEN;

		return sqlStatesLine;
	}

	sql_state_t IntoCondition (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_CONDITION;
		else
			sqlStatesLine &= ~MASK_INTO_CONDITION;

		return sqlStatesLine;
	}

	sql_state_t IntoExceptionBlock (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_EXCEPTION;
		else
			sqlStatesLine &= ~MASK_INTO_EXCEPTION;

		return sqlStatesLine;
	}

	sql_state_t IntoDeclareBlock (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_DECLARE;
		else
			sqlStatesLine &= ~MASK_INTO_DECLARE;

		return sqlStatesLine;
	}

	sql_state_t IntoMergeStatement (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_MERGE_STATEMENT;
		else
			sqlStatesLine &= ~MASK_MERGE_STATEMENT;

		return sqlStatesLine;
	}

	sql_state_t CaseMergeWithoutWhenFound (sql_state_t sqlStatesLine, bool found) {
		if (found)
			sqlStatesLine |= MASK_CASE_MERGE_WITHOUT_WHEN_FOUND;
		else
			sqlStatesLine &= ~MASK_CASE_MERGE_WITHOUT_WHEN_FOUND;

		return sqlStatesLine;
	}
	sql_state_t IntoSelectStatementOrAssignment (sql_state_t sqlStatesLine, bool found) {
		if (found)
			sqlStatesLine |= MASK_INTO_SELECT_STATEMENT_OR_ASSIGNEMENT;
		else
			sqlStatesLine &= ~MASK_INTO_SELECT_STATEMENT_OR_ASSIGNEMENT;
		return sqlStatesLine;
	}

	sql_state_t BeginCaseBlock (sql_state_t sqlStatesLine) {
		if ((sqlStatesLine & MASK_NESTED_CASES) < MASK_NESTED_CASES) {
			sqlStatesLine++;
		}
		return sqlStatesLine;
	}

	sql_state_t EndCaseBlock (sql_state_t sqlStatesLine) {
		if ((sqlStatesLine & MASK_NESTED_CASES) > 0) {
			sqlStatesLine--;
		}
		return sqlStatesLine;
	}

	sql_state_t IntoCreateStatement (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_CREATE;
		else
			sqlStatesLine &= ~MASK_INTO_CREATE;

		return sqlStatesLine;
	}

	sql_state_t IntoCreateViewStatement (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_CREATE_VIEW;
		else
			sqlStatesLine &= ~MASK_INTO_CREATE_VIEW;

		return sqlStatesLine;
	}

	sql_state_t IntoCreateViewAsStatement (sql_state_t sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_CREATE_VIEW_AS_STATEMENT;
		else
			sqlStatesLine &= ~MASK_INTO_CREATE_VIEW_AS_STATEMENT;

		return sqlStatesLine;
	}

	bool IsIgnoreWhen (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_IGNORE_WHEN) != 0;
	}

	bool IsIntoCondition (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_CONDITION) != 0;
	}

	bool IsIntoCaseBlock (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_NESTED_CASES) != 0;
	}

	bool IsIntoExceptionBlock (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_EXCEPTION) != 0;
	}
	bool IsIntoSelectStatementOrAssignment (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_SELECT_STATEMENT_OR_ASSIGNEMENT) != 0;
	}
	bool IsCaseMergeWithoutWhenFound (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_CASE_MERGE_WITHOUT_WHEN_FOUND) != 0;
	}

	bool IsIntoDeclareBlock (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_DECLARE) != 0;
	}

	bool IsIntoMergeStatement (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_MERGE_STATEMENT) != 0;
	}

	bool IsIntoCreateStatement (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_CREATE) != 0;
	}

	bool IsIntoCreateViewStatement (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_CREATE_VIEW) != 0;
	}

	bool IsIntoCreateViewAsStatement (sql_state_t sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_CREATE_VIEW_AS_STATEMENT) != 0;
	}

	sql_state_t ForLine(Sci_Position lineNumber) {
		return sqlStatement.ValueAt(lineNumber);
	}

	SQLStates() {}

private :
	SparseState <sql_state_t> sqlStatement;
	enum {
		MASK_NESTED_CASES                         = 0x0001FF,
		MASK_INTO_SELECT_STATEMENT_OR_ASSIGNEMENT = 0x000200,
		MASK_CASE_MERGE_WITHOUT_WHEN_FOUND        = 0x000400,
		MASK_MERGE_STATEMENT                      = 0x000800,
		MASK_INTO_DECLARE                         = 0x001000,
		MASK_INTO_EXCEPTION                       = 0x002000,
		MASK_INTO_CONDITION                       = 0x004000,
		MASK_IGNORE_WHEN                          = 0x008000,
		MASK_INTO_CREATE                          = 0x010000,
		MASK_INTO_CREATE_VIEW                     = 0x020000,
		MASK_INTO_CREATE_VIEW_AS_STATEMENT        = 0x040000
	};
};

// Options used for LexerSQL
struct OptionsSQL {
	bool fold;
	bool foldAtElse;
	bool foldComment;
	bool foldCompact;
	bool foldOnlyBegin;
	bool sqlBackticksIdentifier;
	bool sqlNumbersignComment;
	bool sqlBackslashEscapes;
	bool sqlAllowDottedWord;
	OptionsSQL() {
		fold = false;
		foldAtElse = false;
		foldComment = false;
		foldCompact = false;
		foldOnlyBegin = false;
		sqlBackticksIdentifier = false;
		sqlNumbersignComment = false;
		sqlBackslashEscapes = false;
		sqlAllowDottedWord = false;
	}
};

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

struct OptionSetSQL : public OptionSet<OptionsSQL> {
	OptionSetSQL() {
		DefineProperty("fold", &OptionsSQL::fold);

		DefineProperty("fold.sql.at.else", &OptionsSQL::foldAtElse,
		               "This option enables SQL folding on a \"ELSE\" and \"ELSIF\" line of an IF statement.");

		DefineProperty("fold.comment", &OptionsSQL::foldComment);

		DefineProperty("fold.compact", &OptionsSQL::foldCompact);

		DefineProperty("fold.sql.only.begin", &OptionsSQL::foldOnlyBegin);

		DefineProperty("lexer.sql.backticks.identifier", &OptionsSQL::sqlBackticksIdentifier);

		DefineProperty("lexer.sql.numbersign.comment", &OptionsSQL::sqlNumbersignComment,
		               "If \"lexer.sql.numbersign.comment\" property is set to 0 a line beginning with '#' will not be a comment.");

		DefineProperty("sql.backslash.escapes", &OptionsSQL::sqlBackslashEscapes,
		               "Enables backslash as an escape character in SQL.");

		DefineProperty("lexer.sql.allow.dotted.word", &OptionsSQL::sqlAllowDottedWord,
		               "Set to 1 to colourise recognized words with dots "
		               "(recommended for Oracle PL/SQL objects).");

		DefineWordListSets(sqlWordListDesc);
	}
};

class LexerSQL : public DefaultLexer {
public :
	LexerSQL() : DefaultLexer("sql", SCLEX_SQL) {}

	virtual ~LexerSQL() {}

	int SCI_METHOD Version () const override {
		return lvRelease5;
	}

	void SCI_METHOD Release() override {
		delete this;
	}

	const char * SCI_METHOD PropertyNames() override {
		return osSQL.PropertyNames();
	}

	int SCI_METHOD PropertyType(const char *name) override {
		return osSQL.PropertyType(name);
	}

	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osSQL.DescribeProperty(name);
	}

	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override {
		if (osSQL.PropertySet(&options, key, val)) {
			return 0;
		}
		return -1;
	}

	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osSQL.PropertyGet(key);
	}

	const char * SCI_METHOD DescribeWordListSets() override {
		return osSQL.DescribeWordListSets();
	}

	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return 0;
	}

	static ILexer5 *LexerFactorySQL() {
		return new LexerSQL();
	}
private:
	bool IsStreamCommentStyle(int style) {
		return style == SCE_SQL_COMMENT ||
		       style == SCE_SQL_COMMENTDOC ||
		       style == SCE_SQL_COMMENTDOCKEYWORD ||
		       style == SCE_SQL_COMMENTDOCKEYWORDERROR;
	}

	bool IsCommentStyle (int style) {
		switch (style) {
		case SCE_SQL_COMMENT :
		case SCE_SQL_COMMENTDOC :
		case SCE_SQL_COMMENTLINE :
		case SCE_SQL_COMMENTLINEDOC :
		case SCE_SQL_COMMENTDOCKEYWORD :
		case SCE_SQL_COMMENTDOCKEYWORDERROR :
			return true;
		default :
			return false;
		}
	}

	bool IsCommentLine (Sci_Position line, LexAccessor &styler) {
		Sci_Position pos = styler.LineStart(line);
		Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
		for (Sci_Position i = pos; i + 1 < eol_pos; i++) {
			int style = styler.StyleAt(i);
			// MySQL needs -- comments to be followed by space or control char
			if (style == SCE_SQL_COMMENTLINE && styler.Match(i, "--"))
				return true;
			else if (!IsASpaceOrTab(styler[i]))
				return false;
		}
		return false;
	}

	OptionsSQL options;
	OptionSetSQL osSQL;
	SQLStates sqlStates;

	WordList keywords1;
	WordList keywords2;
	WordList kw_pldoc;
	WordList kw_sqlplus;
	WordList kw_user1;
	WordList kw_user2;
	WordList kw_user3;
	WordList kw_user4;
};

Sci_Position SCI_METHOD LexerSQL::WordListSet(int n, const char *wl) {
	WordList *wordListN = 0;
	switch (n) {
	case 0:
		wordListN = &keywords1;
		break;
	case 1:
		wordListN = &keywords2;
		break;
	case 2:
		wordListN = &kw_pldoc;
		break;
	case 3:
		wordListN = &kw_sqlplus;
		break;
	case 4:
		wordListN = &kw_user1;
		break;
	case 5:
		wordListN = &kw_user2;
		break;
	case 6:
		wordListN = &kw_user3;
		break;
	case 7:
		wordListN = &kw_user4;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerSQL::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	StyleContext sc(startPos, length, initStyle, styler);
	int styleBeforeDCKeyword = SCE_SQL_DEFAULT;

	for (; sc.More(); sc.Forward()) {
		// Determine if the current state should terminate.
		switch (sc.state) {
		case SCE_SQL_OPERATOR:
			sc.SetState(SCE_SQL_DEFAULT);
			break;
		case SCE_SQL_NUMBER:
			// We stop the number definition on non-numerical non-dot non-eE non-sign char
			if (!IsANumberChar(sc.ch, sc.chPrev)) {
				sc.SetState(SCE_SQL_DEFAULT);
			}
			break;
		case SCE_SQL_IDENTIFIER:
			if (!IsAWordChar(sc.ch, options.sqlAllowDottedWord)) {
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
			if (options.sqlBackslashEscapes && sc.ch == '\\') {
				sc.Forward();
			} else if (sc.ch == '\'') {
				if (sc.chNext == '\'') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_SQL_DEFAULT);
				}
			}
			break;
		case SCE_SQL_STRING:
			if (options.sqlBackslashEscapes && sc.ch == '\\') {
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
		case SCE_SQL_QOPERATOR:
			// Locate the unique Q operator character
			sc.Complete();
			char qOperator = 0x00;
			for (Sci_Position styleStartPos = sc.currentPos; styleStartPos > 0; --styleStartPos) {
				if (styler.StyleAt(styleStartPos - 1) != SCE_SQL_QOPERATOR) {
					qOperator = styler.SafeGetCharAt(styleStartPos + 2);
					break;
				}
			}

			char qComplement = 0x00;

			if (qOperator == '<') {
				qComplement = '>';
			} else if (qOperator == '(') {
				qComplement = ')';
			} else if (qOperator == '{') {
				qComplement = '}';
			} else if (qOperator == '[') {
				qComplement = ']';
			} else {
				qComplement = qOperator;
			}

			if (sc.Match(qComplement, '\'')) {
				sc.Forward();
				sc.ForwardSetState(SCE_SQL_DEFAULT);
			}
			break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_SQL_DEFAULT) {
			if (sc.Match('q', '\'') || sc.Match('Q', '\'')) {
				sc.SetState(SCE_SQL_QOPERATOR);
				sc.Forward();
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext)) ||
			          ((sc.ch == '-' || sc.ch == '+') && IsADigit(sc.chNext) && !IsADigit(sc.chPrev))) {
				sc.SetState(SCE_SQL_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_SQL_IDENTIFIER);
			} else if (sc.ch == 0x60 && options.sqlBackticksIdentifier) {
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
			} else if (sc.ch == '#' && options.sqlNumbersignComment) {
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

void SCI_METHOD LexerSQL::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	if (!options.fold)
		return;
	LexAccessor styler(pAccess);
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;

	if (lineCurrent > 0) {
		// Backtrack to previous line in case need to fix its fold status for folding block of single-line comments (i.e. '--').
		Sci_Position lastNLPos = -1;
		// And keep going back until we find an operator ';' followed
		// by white-space and/or comments. This will improve folding.
		while (--startPos > 0) {
			char ch = styler[startPos];
			if (ch == '\n' || (ch == '\r' && styler[startPos + 1] != '\n')) {
				lastNLPos = startPos;
			} else if (ch == ';' &&
				   styler.StyleAt(startPos) == SCE_SQL_OPERATOR) {
				bool isAllClear = true;
				for (Sci_Position tempPos = startPos + 1;
				     tempPos < lastNLPos;
				     ++tempPos) {
					int tempStyle = styler.StyleAt(tempPos);
					if (!IsCommentStyle(tempStyle)
					    && tempStyle != SCE_SQL_DEFAULT) {
						isAllClear = false;
						break;
					}
				}
				if (isAllClear) {
					startPos = lastNLPos + 1;
					break;
				}
			}
		}
		lineCurrent = styler.GetLine(startPos);
		if (lineCurrent > 0)
			levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
	}
	// And because folding ends at ';', keep going until we find one
	// Otherwise if create ... view ... as is split over multiple
	// lines the folding won't always update immediately.
	Sci_PositionU docLength = styler.Length();
	for (; endPos < docLength; ++endPos) {
		if (styler.SafeGetCharAt(endPos) == ';') {
			break;
		}
	}

	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool endFound = false;
	bool isUnfoldingIgnored = false;
	// this statementFound flag avoids to fold when the statement is on only one line by ignoring ELSE or ELSIF
	// eg. "IF condition1 THEN ... ELSIF condition2 THEN ... ELSE ... END IF;"
	bool statementFound = false;
	sql_state_t sqlStatesCurrentLine = 0;
	if (!options.foldOnlyBegin) {
		sqlStatesCurrentLine = sqlStates.ForLine(lineCurrent);
	}
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (atEOL || (!IsCommentStyle(style) && ch == ';')) {
			if (endFound) {
				//Maybe this is the end of "EXCEPTION" BLOCK (eg. "BEGIN ... EXCEPTION ... END;")
				sqlStatesCurrentLine = sqlStates.IntoExceptionBlock(sqlStatesCurrentLine, false);
			}
			// set endFound and isUnfoldingIgnored to false if EOL is reached or ';' is found
			endFound = false;
			isUnfoldingIgnored = false;
		}
		if ((!IsCommentStyle(style) && ch == ';')) {
			if (sqlStates.IsIntoMergeStatement(sqlStatesCurrentLine)) {
				// This is the end of "MERGE" statement.
				if (!sqlStates.IsCaseMergeWithoutWhenFound(sqlStatesCurrentLine))
					levelNext--;
				sqlStatesCurrentLine = sqlStates.IntoMergeStatement(sqlStatesCurrentLine, false);
				levelNext--;
			}
			if (sqlStates.IsIntoSelectStatementOrAssignment(sqlStatesCurrentLine))
				sqlStatesCurrentLine = sqlStates.IntoSelectStatementOrAssignment(sqlStatesCurrentLine, false);
			if (sqlStates.IsIntoCreateStatement(sqlStatesCurrentLine)) {
				if (sqlStates.IsIntoCreateViewStatement(sqlStatesCurrentLine)) {
					if (sqlStates.IsIntoCreateViewAsStatement(sqlStatesCurrentLine)) {
						levelNext--;
						sqlStatesCurrentLine = sqlStates.IntoCreateViewAsStatement(sqlStatesCurrentLine, false);
					}
					sqlStatesCurrentLine = sqlStates.IntoCreateViewStatement(sqlStatesCurrentLine, false);
				}
				sqlStatesCurrentLine = sqlStates.IntoCreateStatement(sqlStatesCurrentLine, false);
			}
		}
		if (ch == ':' && chNext == '=' && !IsCommentStyle(style))
			sqlStatesCurrentLine = sqlStates.IntoSelectStatementOrAssignment(sqlStatesCurrentLine, true);

		if (options.foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && (style == SCE_SQL_COMMENTLINE)) {
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
		// Fold block of single-line comments (i.e. '--').
		if (options.foldComment && atEOL && IsCommentLine(lineCurrent, styler)) {
			if (!IsCommentLine(lineCurrent - 1, styler) && IsCommentLine(lineCurrent + 1, styler))
				levelNext++;
			else if (IsCommentLine(lineCurrent - 1, styler) && !IsCommentLine(lineCurrent + 1, styler))
				levelNext--;
		}
		if (style == SCE_SQL_OPERATOR) {
			if (ch == '(') {
				if (levelCurrent > levelNext)
					levelCurrent--;
				levelNext++;
			} else if (ch == ')') {
				levelNext--;
			} else if ((!options.foldOnlyBegin) && ch == ';') {
				sqlStatesCurrentLine = sqlStates.IgnoreWhen(sqlStatesCurrentLine, false);
			}
		}
		// If new keyword (cannot trigger on elseif or nullif, does less tests)
		if (style == SCE_SQL_WORD && stylePrev != SCE_SQL_WORD) {
			const int MAX_KW_LEN = 9;	// Maximum length of folding keywords
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
			if (!options.foldOnlyBegin &&
			        strcmp(s, "select") == 0) {
				sqlStatesCurrentLine = sqlStates.IntoSelectStatementOrAssignment(sqlStatesCurrentLine, true);
			} else if (strcmp(s, "if") == 0) {
				if (endFound) {
					endFound = false;
					if (options.foldOnlyBegin && !isUnfoldingIgnored) {
						// this end isn't for begin block, but for if block ("end if;")
						// so ignore previous "end" by increment levelNext.
						levelNext++;
					}
				} else {
					if (!options.foldOnlyBegin)
						sqlStatesCurrentLine = sqlStates.IntoCondition(sqlStatesCurrentLine, true);
					if (levelCurrent > levelNext) {
						// doesn't include this line into the folding block
						// because doesn't hide IF (eg "END; IF")
						levelCurrent = levelNext;
					}
				}
			} else if (!options.foldOnlyBegin &&
			           strcmp(s, "then") == 0 &&
			           sqlStates.IsIntoCondition(sqlStatesCurrentLine)) {
				sqlStatesCurrentLine = sqlStates.IntoCondition(sqlStatesCurrentLine, false);
				if (!options.foldOnlyBegin) {
					if (levelCurrent > levelNext) {
						levelCurrent = levelNext;
					}
					if (!statementFound)
						levelNext++;

					statementFound = true;
				} else if (levelCurrent > levelNext) {
					// doesn't include this line into the folding block
					// because doesn't hide LOOP or CASE (eg "END; LOOP" or "END; CASE")
					levelCurrent = levelNext;
				}
			} else if (strcmp(s, "loop") == 0 ||
			           strcmp(s, "case") == 0) {
				if (endFound) {
					endFound = false;
					if (options.foldOnlyBegin && !isUnfoldingIgnored) {
						// this end isn't for begin block, but for loop block ("end loop;") or case block ("end case;")
						// so ignore previous "end" by increment levelNext.
						levelNext++;
					}
					if ((!options.foldOnlyBegin) && strcmp(s, "case") == 0) {
						sqlStatesCurrentLine = sqlStates.EndCaseBlock(sqlStatesCurrentLine);
						if (!sqlStates.IsCaseMergeWithoutWhenFound(sqlStatesCurrentLine))
							levelNext--; //again for the "end case;" and block when
					}
				} else if (!options.foldOnlyBegin) {
					if (strcmp(s, "case") == 0) {
						sqlStatesCurrentLine = sqlStates.BeginCaseBlock(sqlStatesCurrentLine);
						sqlStatesCurrentLine = sqlStates.CaseMergeWithoutWhenFound(sqlStatesCurrentLine, true);
					}

					if (levelCurrent > levelNext)
						levelCurrent = levelNext;

					if (!statementFound)
						levelNext++;

					statementFound = true;
				} else if (levelCurrent > levelNext) {
					// doesn't include this line into the folding block
					// because doesn't hide LOOP or CASE (eg "END; LOOP" or "END; CASE")
					levelCurrent = levelNext;
				}
			} else if ((!options.foldOnlyBegin) && (
			               // folding for ELSE and ELSIF block only if foldAtElse is set
			               // and IF or CASE aren't on only one line with ELSE or ELSIF (with flag statementFound)
			               options.foldAtElse && !statementFound) && strcmp(s, "elsif") == 0) {
				sqlStatesCurrentLine = sqlStates.IntoCondition(sqlStatesCurrentLine, true);
				levelCurrent--;
				levelNext--;
			} else if ((!options.foldOnlyBegin) && (
			               // folding for ELSE and ELSIF block only if foldAtElse is set
			               // and IF or CASE aren't on only one line with ELSE or ELSIF (with flag statementFound)
			               options.foldAtElse && !statementFound) && strcmp(s, "else") == 0) {
				// prevent also ELSE is on the same line (eg. "ELSE ... END IF;")
				statementFound = true;
				if (sqlStates.IsIntoCaseBlock(sqlStatesCurrentLine) && sqlStates.IsCaseMergeWithoutWhenFound(sqlStatesCurrentLine)) {
					sqlStatesCurrentLine = sqlStates.CaseMergeWithoutWhenFound(sqlStatesCurrentLine, false);
					levelNext++;
				} else {
					// we are in same case "} ELSE {" in C language
					levelCurrent--;
				}
			} else if (strcmp(s, "begin") == 0) {
				levelNext++;
				sqlStatesCurrentLine = sqlStates.IntoDeclareBlock(sqlStatesCurrentLine, false);
			} else if ((strcmp(s, "end") == 0) ||
			           // SQL Anywhere permits IF ... ELSE ... ENDIF
			           // will only be active if "endif" appears in the
			           // keyword list.
			           (strcmp(s, "endif") == 0)) {
				endFound = true;
				levelNext--;
				if (sqlStates.IsIntoSelectStatementOrAssignment(sqlStatesCurrentLine) && !sqlStates.IsCaseMergeWithoutWhenFound(sqlStatesCurrentLine))
					levelNext--;
				if (levelNext < SC_FOLDLEVELBASE) {
					levelNext = SC_FOLDLEVELBASE;
					isUnfoldingIgnored = true;
				}
			} else if ((!options.foldOnlyBegin) &&
			           strcmp(s, "when") == 0 &&
			           !sqlStates.IsIgnoreWhen(sqlStatesCurrentLine) &&
			           !sqlStates.IsIntoExceptionBlock(sqlStatesCurrentLine) && (
			               sqlStates.IsIntoCaseBlock(sqlStatesCurrentLine) ||
			               sqlStates.IsIntoMergeStatement(sqlStatesCurrentLine)
			               )
			           ) {
				sqlStatesCurrentLine = sqlStates.IntoCondition(sqlStatesCurrentLine, true);

				// Don't foldind when CASE and WHEN are on the same line (with flag statementFound) (eg. "CASE selector WHEN expression1 THEN sequence_of_statements1;\n")
				// and same way for MERGE statement.
				if (!statementFound) {
					if (!sqlStates.IsCaseMergeWithoutWhenFound(sqlStatesCurrentLine)) {
						levelCurrent--;
						levelNext--;
					}
					sqlStatesCurrentLine = sqlStates.CaseMergeWithoutWhenFound(sqlStatesCurrentLine, false);
				}
			} else if ((!options.foldOnlyBegin) && strcmp(s, "exit") == 0) {
				sqlStatesCurrentLine = sqlStates.IgnoreWhen(sqlStatesCurrentLine, true);
			} else if ((!options.foldOnlyBegin) && !sqlStates.IsIntoDeclareBlock(sqlStatesCurrentLine) && strcmp(s, "exception") == 0) {
				sqlStatesCurrentLine = sqlStates.IntoExceptionBlock(sqlStatesCurrentLine, true);
			} else if ((!options.foldOnlyBegin) &&
			           (strcmp(s, "declare") == 0 ||
			            strcmp(s, "function") == 0 ||
			            strcmp(s, "procedure") == 0 ||
			            strcmp(s, "package") == 0)) {
				sqlStatesCurrentLine = sqlStates.IntoDeclareBlock(sqlStatesCurrentLine, true);
			} else if ((!options.foldOnlyBegin) &&
			           strcmp(s, "merge") == 0) {
				sqlStatesCurrentLine = sqlStates.IntoMergeStatement(sqlStatesCurrentLine, true);
				sqlStatesCurrentLine = sqlStates.CaseMergeWithoutWhenFound(sqlStatesCurrentLine, true);
				levelNext++;
				statementFound = true;
			} else if ((!options.foldOnlyBegin) &&
				   strcmp(s, "create") == 0) {
				sqlStatesCurrentLine = sqlStates.IntoCreateStatement(sqlStatesCurrentLine, true);
			} else if ((!options.foldOnlyBegin) &&
				   strcmp(s, "view") == 0 &&
				   sqlStates.IsIntoCreateStatement(sqlStatesCurrentLine)) {
				sqlStatesCurrentLine = sqlStates.IntoCreateViewStatement(sqlStatesCurrentLine, true);
			} else if ((!options.foldOnlyBegin) &&
				   strcmp(s, "as") == 0 &&
				   sqlStates.IsIntoCreateViewStatement(sqlStatesCurrentLine) &&
				   ! sqlStates.IsIntoCreateViewAsStatement(sqlStatesCurrentLine)) {
				sqlStatesCurrentLine = sqlStates.IntoCreateViewAsStatement(sqlStatesCurrentLine, true);
				levelNext++;
			}
		}
		if (atEOL) {
			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			visibleChars = 0;
			statementFound = false;
			if (!options.foldOnlyBegin)
				sqlStates.Set(lineCurrent, sqlStatesCurrentLine);
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
}

LexerModule lmSQL(SCLEX_SQL, LexerSQL::LexerFactorySQL, "sql", sqlWordListDesc);
