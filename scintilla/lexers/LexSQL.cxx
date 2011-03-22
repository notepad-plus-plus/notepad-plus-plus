// Scintilla source code edit control
/** @file LexSQL.cxx
 ** Lexer for SQL, including PL/SQL and SQL*Plus.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <string>
#include <vector>
#include <map>
#include <algorithm>

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

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

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

static inline bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	       (isdigit(ch) || toupper(ch) == 'E' ||
	        ch == '.' || ch == '-' || ch == '+');
}


class SQLStates {
public :
	void Set(int lineNumber, unsigned short int sqlStatesLine) {
		if (!sqlStatement.size() == 0 || !sqlStatesLine == 0) {
			sqlStatement.resize(lineNumber + 1, 0);
			sqlStatement[lineNumber] = sqlStatesLine;
		}
	}

	unsigned short int IgnoreWhen (unsigned short int sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_IGNORE_WHEN;
		else
			sqlStatesLine &= ~MASK_IGNORE_WHEN;

		return sqlStatesLine;
	}

	unsigned short int IntoCondition (unsigned short int sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_CONDITION;
		else
			sqlStatesLine &= ~MASK_INTO_CONDITION;

		return sqlStatesLine;
	}

	unsigned short int IntoExceptionBlock (unsigned short int sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_EXCEPTION;
		else
			sqlStatesLine &= ~MASK_INTO_EXCEPTION;

		return sqlStatesLine;
	}

	unsigned short int IntoDeclareBlock (unsigned short int sqlStatesLine, bool enable) {
		if (enable)
			sqlStatesLine |= MASK_INTO_DECLARE;
		else
			sqlStatesLine &= ~MASK_INTO_DECLARE;

		return sqlStatesLine;
	}

	unsigned short int BeginCaseBlock (unsigned short int sqlStatesLine) {
		if ((sqlStatesLine & MASK_NESTED_CASES) < MASK_NESTED_CASES) {
			sqlStatesLine++;
		}
		return sqlStatesLine;
	}

	unsigned short int EndCaseBlock (unsigned short int sqlStatesLine) {
		if ((sqlStatesLine & MASK_NESTED_CASES) > 0) {
			sqlStatesLine--;
		}
		return sqlStatesLine;
	}

	bool IsIgnoreWhen (unsigned short int sqlStatesLine) {
		return (sqlStatesLine & MASK_IGNORE_WHEN) != 0;
	}

	bool IsIntoCondition (unsigned short int sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_CONDITION) != 0;
	}

	bool IsIntoCaseBlock (unsigned short int sqlStatesLine) {
		return (sqlStatesLine & MASK_NESTED_CASES) != 0;
	}

	bool IsIntoExceptionBlock (unsigned short int sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_EXCEPTION) != 0;
	}

	bool IsIntoDeclareBlock (unsigned short int sqlStatesLine) {
		return (sqlStatesLine & MASK_INTO_DECLARE) != 0;
	}

	unsigned short int ForLine(int lineNumber) {
		if ((lineNumber > 0) && (sqlStatement.size() > static_cast<size_t>(lineNumber))) {
			return sqlStatement[lineNumber];
		} else {
			return 0;
		}
	}

	SQLStates() {}

private :
	std::vector <unsigned short int> sqlStatement;
	enum {
		MASK_INTO_DECLARE = 0x1000,
		MASK_INTO_EXCEPTION = 0x2000,
		MASK_INTO_CONDITION = 0x4000,
		MASK_IGNORE_WHEN = 0x8000,
		MASK_NESTED_CASES = 0x0FFF
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

		DefineProperty("lexer.sql.fold.at.else", &OptionsSQL::foldAtElse,
		               "This option enables SQL folding on a \"ELSE\" and \"ELSIF\"line of an IF statement.");

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

class LexerSQL : public ILexer {
public :
	LexerSQL() {}

	int SCI_METHOD Version () const {
		return lvOriginal;
	}

	void SCI_METHOD Release() {
		delete this;
	}

	const char * SCI_METHOD PropertyNames() {
		return osSQL.PropertyNames();
	}

	int SCI_METHOD PropertyType(const char *name) {
		return osSQL.PropertyType(name);
	}

	const char * SCI_METHOD DescribeProperty(const char *name) {
		return osSQL.DescribeProperty(name);
	}

	int SCI_METHOD PropertySet(const char *key, const char *val) {
		if (osSQL.PropertySet(&options, key, val)) {
			return 0;
		}
		return -1;
	}

	const char * SCI_METHOD DescribeWordListSets() {
		return osSQL.DescribeWordListSets();
	}

	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex (unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);

	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}

	static ILexer *LexerFactorySQL() {
		return new LexerSQL();
	}
private:
	bool IsStreamCommentStyle(int style) {
		return style == SCE_SQL_COMMENT ||
		       style == SCE_SQL_COMMENTDOC ||
		       style == SCE_SQL_COMMENTDOCKEYWORD ||
		       style == SCE_SQL_COMMENTDOCKEYWORDERROR;
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

int SCI_METHOD LexerSQL::WordListSet(int n, const char *wl) {
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
	int firstModification = -1;
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

void SCI_METHOD LexerSQL::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);
	StyleContext sc(startPos, length, initStyle, styler);
	int styleBeforeDCKeyword = SCE_SQL_DEFAULT;
	int offset = 0;
	for (; sc.More(); sc.Forward(), offset++) {
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

void SCI_METHOD LexerSQL::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {
	if (!options.fold)
		return;
	LexAccessor styler(pAccess);
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
	bool isUnfoldingIgnored = false;
	// this statementFound flag avoids to fold when the statement is on only one line by ignoring ELSE or ELSIF
	// eg. "IF condition1 THEN ... ELSIF condition2 THEN ... ELSE ... END IF;"
	bool statementFound = false;
	unsigned short int sqlStatesCurrentLine = 0;
	if (!options.foldOnlyBegin) {
		sqlStatesCurrentLine = sqlStates.ForLine(lineCurrent);
	}
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (atEOL || (ch == ';')) {
			if (endFound) {
				//Maybe this is the end of "EXCEPTION" BLOCK (eg. "BEGIN ... EXCEPTION ... END;")
				sqlStatesCurrentLine = sqlStates.IntoExceptionBlock(sqlStatesCurrentLine, false);
			}
			// set endFound and isUnfoldingIgnored to false if EOL is reached or ';' is found
			endFound = false;
			isUnfoldingIgnored = false;
		}
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

			if (strcmp(s, "if") == 0) {
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
						levelNext--; //again for the "end case;" and block when
					}
				} else if (!options.foldOnlyBegin) {
					if (strcmp(s, "case") == 0) {
						sqlStatesCurrentLine = sqlStates.BeginCaseBlock(sqlStatesCurrentLine);

						//for case block increment 2 times
						if (!statementFound)
							levelNext++;
					}

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
				// we are in same case "} ELSE {" in C language
				levelCurrent--;

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
				if (levelNext < SC_FOLDLEVELBASE) {
					levelNext = SC_FOLDLEVELBASE;
					isUnfoldingIgnored = true;
				}
			} else if ((!options.foldOnlyBegin) &&
			           strcmp(s, "when") == 0 &&
			           !sqlStates.IsIgnoreWhen(sqlStatesCurrentLine) &&
			           !sqlStates.IsIntoExceptionBlock(sqlStatesCurrentLine) &&
			           sqlStates.IsIntoCaseBlock(sqlStatesCurrentLine)) {
				sqlStatesCurrentLine = sqlStates.IntoCondition(sqlStatesCurrentLine, true);

				// Don't foldind when CASE and WHEN are on the same line (with flag statementFound) (eg. "CASE selector WHEN expression1 THEN sequence_of_statements1;\n")
				if (!statementFound) {
					levelCurrent--;
					levelNext--;
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
