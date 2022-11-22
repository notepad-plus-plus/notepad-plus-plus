// Scintilla source code edit control
// Encoding: UTF-8
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
/** @file LexErlang.cxx
 ** Lexer for Erlang.
 ** Enhanced by Etienne 'Lenain' Girondel (lenaing@gmail.com)
 ** Originally wrote by Peter-Henry Mander,
 ** based on Matlab lexer by José Fonseca.
 **/

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

static int is_radix(int radix, int ch) {
	int digit;

	if (36 < radix || 2 > radix)
		return 0;

	if (isdigit(ch)) {
		digit = ch - '0';
	} else if (isalnum(ch)) {
		digit = toupper(ch) - 'A' + 10;
	} else {
		return 0;
	}

	return (digit < radix);
}

typedef enum {
	STATE_NULL,
	COMMENT,
	COMMENT_FUNCTION,
	COMMENT_MODULE,
	COMMENT_DOC,
	COMMENT_DOC_MACRO,
	ATOM_UNQUOTED,
	ATOM_QUOTED,
	NODE_NAME_UNQUOTED,
	NODE_NAME_QUOTED,
	MACRO_START,
	MACRO_UNQUOTED,
	MACRO_QUOTED,
	RECORD_START,
	RECORD_UNQUOTED,
	RECORD_QUOTED,
	NUMERAL_START,
	NUMERAL_BASE_VALUE,
	NUMERAL_FLOAT,
	NUMERAL_EXPONENT,
	PREPROCESSOR
} atom_parse_state_t;

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (ch != ' ') && (isalnum(ch) || ch == '_');
}

static void ColouriseErlangDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
								WordList *keywordlists[], Accessor &styler) {

	StyleContext sc(startPos, length, initStyle, styler);
	WordList &reservedWords = *keywordlists[0];
	WordList &erlangBIFs = *keywordlists[1];
	WordList &erlangPreproc = *keywordlists[2];
	WordList &erlangModulesAtt = *keywordlists[3];
	WordList &erlangDoc = *keywordlists[4];
	WordList &erlangDocMacro = *keywordlists[5];
	int radix_digits = 0;
	int exponent_digits = 0;
	atom_parse_state_t parse_state = STATE_NULL;
	atom_parse_state_t old_parse_state = STATE_NULL;
	bool to_late_to_comment = false;
	char cur[100];
	int old_style = SCE_ERLANG_DEFAULT;

	styler.StartAt(startPos);

	for (; sc.More(); sc.Forward()) {
		int style = SCE_ERLANG_DEFAULT;
		if (STATE_NULL != parse_state) {

			switch (parse_state) {

				case STATE_NULL : sc.SetState(SCE_ERLANG_DEFAULT); break;

			/* COMMENTS ------------------------------------------------------*/
				case COMMENT : {
					if (sc.ch != '%') {
						to_late_to_comment = true;
					} else if (!to_late_to_comment && sc.ch == '%') {
						// Switch to comment level 2 (Function)
						sc.ChangeState(SCE_ERLANG_COMMENT_FUNCTION);
						old_style = SCE_ERLANG_COMMENT_FUNCTION;
						parse_state = COMMENT_FUNCTION;
						sc.Forward();
					}
				}
				// V--- Falling through!
				// Falls through.
				case COMMENT_FUNCTION : {
					if (sc.ch != '%') {
						to_late_to_comment = true;
					} else if (!to_late_to_comment && sc.ch == '%') {
						// Switch to comment level 3 (Module)
						sc.ChangeState(SCE_ERLANG_COMMENT_MODULE);
						old_style = SCE_ERLANG_COMMENT_MODULE;
						parse_state = COMMENT_MODULE;
						sc.Forward();
					}
				}
				// V--- Falling through!
				// Falls through.
				case COMMENT_MODULE : {
					if (parse_state != COMMENT) {
						// Search for comment documentation
						if (sc.chNext == '@') {
							old_parse_state = parse_state;
							parse_state = ('{' == sc.ch)
											? COMMENT_DOC_MACRO
											: COMMENT_DOC;
							sc.ForwardSetState(sc.state);
						}
					}

					// All comments types fall here.
					if (sc.MatchLineEnd()) {
						to_late_to_comment = false;
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case COMMENT_DOC :
				// V--- Falling through!
				case COMMENT_DOC_MACRO : {

					if (!isalnum(sc.ch)) {
						// Try to match documentation comment
						sc.GetCurrent(cur, sizeof(cur));

						if (parse_state == COMMENT_DOC_MACRO
							&& erlangDocMacro.InList(cur)) {
								sc.ChangeState(SCE_ERLANG_COMMENT_DOC_MACRO);
								while (sc.ch != '}' && !sc.atLineEnd)
									sc.Forward();
						} else if (erlangDoc.InList(cur)) {
							sc.ChangeState(SCE_ERLANG_COMMENT_DOC);
						} else {
							sc.ChangeState(old_style);
						}

						// Switch back to old state
						sc.SetState(old_style);
						parse_state = old_parse_state;
					}

					if (sc.MatchLineEnd()) {
						to_late_to_comment = false;
						sc.ChangeState(old_style);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Atoms ---------------------------------------------------------*/
				case ATOM_UNQUOTED : {
					if ('@' == sc.ch){
						parse_state = NODE_NAME_UNQUOTED;
					} else if (sc.ch == ':') {
						// Searching for module name
						if (sc.chNext == ' ') {
							// error
							sc.ChangeState(SCE_ERLANG_UNKNOWN);
							parse_state = STATE_NULL;
						} else {
							sc.Forward();
							if (isalnum(sc.ch) || (sc.ch == '\''))  {
								sc.GetCurrent(cur, sizeof(cur));
								sc.ChangeState(SCE_ERLANG_MODULES);
								sc.SetState(SCE_ERLANG_MODULES);
							}
							if (sc.ch == '\'') {
								parse_state = ATOM_QUOTED;
							}

						}
					} else if (!IsAWordChar(sc.ch)) {

						sc.GetCurrent(cur, sizeof(cur));
						if (reservedWords.InList(cur)) {
							style = SCE_ERLANG_KEYWORD;
						} else if (erlangBIFs.InList(cur)
									&& strcmp(cur,"erlang:")){
							style = SCE_ERLANG_BIFS;
						} else if (sc.ch == '(' || '/' == sc.ch){
							style = SCE_ERLANG_FUNCTION_NAME;
						} else {
							style = SCE_ERLANG_ATOM;
						}

						sc.ChangeState(style);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}

				} break;

				case ATOM_QUOTED : {
					if ( '@' == sc.ch ){
						parse_state = NODE_NAME_QUOTED;
					} else if ('\'' == sc.ch && '\\' != sc.chPrev) {
						sc.ChangeState(SCE_ERLANG_ATOM_QUOTED);
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Node names ----------------------------------------------------*/
				case NODE_NAME_UNQUOTED : {
					if ('@' == sc.ch) {
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					} else if (!IsAWordChar(sc.ch)) {
						sc.ChangeState(SCE_ERLANG_NODE_NAME);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case NODE_NAME_QUOTED : {
					if ('@' == sc.ch) {
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					} else if ('\'' == sc.ch && '\\' != sc.chPrev) {
						sc.ChangeState(SCE_ERLANG_NODE_NAME_QUOTED);
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Records -------------------------------------------------------*/
				case RECORD_START : {
					if ('\'' == sc.ch) {
						parse_state = RECORD_QUOTED;
					} else if (isalpha(sc.ch) && islower(sc.ch)) {
						parse_state = RECORD_UNQUOTED;
					} else { // error
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case RECORD_UNQUOTED : {
					if (!IsAWordChar(sc.ch)) {
						sc.ChangeState(SCE_ERLANG_RECORD);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case RECORD_QUOTED : {
					if ('\'' == sc.ch && '\\' != sc.chPrev) {
						sc.ChangeState(SCE_ERLANG_RECORD_QUOTED);
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Macros --------------------------------------------------------*/
				case MACRO_START : {
					if ('\'' == sc.ch) {
						parse_state = MACRO_QUOTED;
					} else if (isalpha(sc.ch)) {
						parse_state = MACRO_UNQUOTED;
					} else { // error
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case MACRO_UNQUOTED : {
					if (!IsAWordChar(sc.ch)) {
						sc.ChangeState(SCE_ERLANG_MACRO);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

				case MACRO_QUOTED : {
					if ('\'' == sc.ch && '\\' != sc.chPrev) {
						sc.ChangeState(SCE_ERLANG_MACRO_QUOTED);
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Numerics ------------------------------------------------------*/
			/* Simple integer */
				case NUMERAL_START : {
					if (isdigit(sc.ch)) {
						radix_digits *= 10;
						radix_digits += sc.ch - '0'; // Assuming ASCII here!
					} else if ('#' == sc.ch) {
						if (2 > radix_digits || 36 < radix_digits) {
							sc.SetState(SCE_ERLANG_DEFAULT);
							parse_state = STATE_NULL;
						} else {
							parse_state = NUMERAL_BASE_VALUE;
						}
					} else if ('.' == sc.ch && isdigit(sc.chNext)) {
						radix_digits = 0;
						parse_state = NUMERAL_FLOAT;
					} else if ('e' == sc.ch || 'E' == sc.ch) {
						exponent_digits = 0;
						parse_state = NUMERAL_EXPONENT;
					} else {
						radix_digits = 0;
						sc.ChangeState(SCE_ERLANG_NUMBER);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* Integer in other base than 10 (x#yyy) */
				case NUMERAL_BASE_VALUE : {
					if (!is_radix(radix_digits,sc.ch)) {
						radix_digits = 0;

						if (!isalnum(sc.ch))
							sc.ChangeState(SCE_ERLANG_NUMBER);

						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* Float (x.yyy) */
				case NUMERAL_FLOAT : {
					if ('e' == sc.ch || 'E' == sc.ch) {
						exponent_digits = 0;
						parse_state = NUMERAL_EXPONENT;
					} else if (!isdigit(sc.ch)) {
						sc.ChangeState(SCE_ERLANG_NUMBER);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			/* Exponent, either integer or float (xEyy, x.yyEzzz) */
				case NUMERAL_EXPONENT : {
					if (('-' == sc.ch || '+' == sc.ch)
							&& (isdigit(sc.chNext))) {
						sc.Forward();
					} else if (!isdigit(sc.ch)) {
						if (0 < exponent_digits)
							sc.ChangeState(SCE_ERLANG_NUMBER);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					} else {
						++exponent_digits;
					}
				} break;

			/* -------------------------------------------------------------- */
			/* Preprocessor --------------------------------------------------*/
				case PREPROCESSOR : {
					if (!IsAWordChar(sc.ch)) {

						sc.GetCurrent(cur, sizeof(cur));
						if (erlangPreproc.InList(cur)) {
							style = SCE_ERLANG_PREPROC;
						} else if (erlangModulesAtt.InList(cur)) {
							style = SCE_ERLANG_MODULES_ATT;
						}

						sc.ChangeState(style);
						sc.SetState(SCE_ERLANG_DEFAULT);
						parse_state = STATE_NULL;
					}
				} break;

			}

		} /* End of : STATE_NULL != parse_state */
		else
		{
			switch (sc.state) {
				case SCE_ERLANG_VARIABLE : {
					if (!IsAWordChar(sc.ch))
						sc.SetState(SCE_ERLANG_DEFAULT);
				} break;
				case SCE_ERLANG_STRING : {
					 if (sc.ch == '\"' && sc.chPrev != '\\')
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
				} break;
				case SCE_ERLANG_COMMENT : {
					 if (sc.atLineEnd)
						sc.SetState(SCE_ERLANG_DEFAULT);
				} break;
				case SCE_ERLANG_CHARACTER : {
					if (sc.chPrev == '\\') {
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
					} else if (sc.ch != '\\') {
						sc.ForwardSetState(SCE_ERLANG_DEFAULT);
					}
				} break;
				case SCE_ERLANG_OPERATOR : {
					if (sc.chPrev == '.') {
						if (sc.ch == '*' || sc.ch == '/' || sc.ch == '\\'
							|| sc.ch == '^') {
							sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						} else if (sc.ch == '\'') {
							sc.ForwardSetState(SCE_ERLANG_DEFAULT);
						} else {
							sc.SetState(SCE_ERLANG_DEFAULT);
						}
					} else {
						sc.SetState(SCE_ERLANG_DEFAULT);
					}
				} break;
			}
		}

		if (sc.state == SCE_ERLANG_DEFAULT) {
			bool no_new_state = false;

			switch (sc.ch) {
				case '\"' : sc.SetState(SCE_ERLANG_STRING); break;
				case '$' : sc.SetState(SCE_ERLANG_CHARACTER); break;
				case '%' : {
					parse_state = COMMENT;
					sc.SetState(SCE_ERLANG_COMMENT);
				} break;
				case '#' : {
					parse_state = RECORD_START;
					sc.SetState(SCE_ERLANG_UNKNOWN);
				} break;
				case '?' : {
					parse_state = MACRO_START;
					sc.SetState(SCE_ERLANG_UNKNOWN);
				} break;
				case '\'' : {
					parse_state = ATOM_QUOTED;
					sc.SetState(SCE_ERLANG_UNKNOWN);
				} break;
				case '+' :
				case '-' : {
					if (IsADigit(sc.chNext)) {
						parse_state = NUMERAL_START;
						radix_digits = 0;
						sc.SetState(SCE_ERLANG_UNKNOWN);
					} else if (sc.ch != '+') {
						parse_state = PREPROCESSOR;
						sc.SetState(SCE_ERLANG_UNKNOWN);
					}
				} break;
				default : no_new_state = true;
			}

			if (no_new_state) {
				if (isdigit(sc.ch)) {
					parse_state = NUMERAL_START;
					radix_digits = sc.ch - '0';
					sc.SetState(SCE_ERLANG_UNKNOWN);
				} else if (isupper(sc.ch) || '_' == sc.ch) {
					sc.SetState(SCE_ERLANG_VARIABLE);
				} else if (isalpha(sc.ch)) {
					parse_state = ATOM_UNQUOTED;
					sc.SetState(SCE_ERLANG_UNKNOWN);
				} else if (isoperator(static_cast<char>(sc.ch))
							|| sc.ch == '\\') {
					sc.SetState(SCE_ERLANG_OPERATOR);
				}
			}
		}

	}
	sc.Complete();
}

static int ClassifyErlangFoldPoint(
	Accessor &styler,
	int styleNext,
	Sci_Position keyword_start
) {
	int lev = 0;
	if (styler.Match(keyword_start,"case")
		|| (
			styler.Match(keyword_start,"fun")
			&& (SCE_ERLANG_FUNCTION_NAME != styleNext)
			)
		|| styler.Match(keyword_start,"if")
		|| styler.Match(keyword_start,"query")
		|| styler.Match(keyword_start,"receive")
	) {
		++lev;
	} else if (styler.Match(keyword_start,"end")) {
		--lev;
	}

	return lev;
}

static void FoldErlangDoc(
	Sci_PositionU startPos, Sci_Position length, int initStyle,
	WordList** /*keywordlists*/, Accessor &styler
) {
	Sci_PositionU endPos = startPos + length;
	Sci_Position currentLine = styler.GetLine(startPos);
	int lev;
	int previousLevel = styler.LevelAt(currentLine) & SC_FOLDLEVELNUMBERMASK;
	int currentLevel = previousLevel;
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	int stylePrev;
	Sci_Position keyword_start = 0;
	char ch;
	char chNext = styler.SafeGetCharAt(startPos);
	bool atEOL;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		// Get styles
		stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		atEOL = ((ch == '\r') && (chNext != '\n')) || (ch == '\n');

		if (stylePrev != SCE_ERLANG_KEYWORD
			&& style == SCE_ERLANG_KEYWORD) {
			keyword_start = i;
		}

		// Fold on keywords
		if (stylePrev == SCE_ERLANG_KEYWORD
			&& style != SCE_ERLANG_KEYWORD
			&& style != SCE_ERLANG_ATOM
		) {
			currentLevel += ClassifyErlangFoldPoint(styler,
													styleNext,
													keyword_start);
		}

		// Fold on comments
		if (style == SCE_ERLANG_COMMENT
			|| style == SCE_ERLANG_COMMENT_MODULE
			|| style == SCE_ERLANG_COMMENT_FUNCTION) {

			if (ch == '%' && chNext == '{') {
				currentLevel++;
			} else if (ch == '%' && chNext == '}') {
				currentLevel--;
			}
		}

		// Fold on braces
		if (style == SCE_ERLANG_OPERATOR) {
			if (ch == '{' || ch == '(' || ch == '[') {
				currentLevel++;
			} else if (ch == '}' || ch == ')' || ch == ']') {
				currentLevel--;
			}
		}


		if (atEOL) {
			lev = previousLevel;

			if (currentLevel > previousLevel)
				lev |= SC_FOLDLEVELHEADERFLAG;

			if (lev != styler.LevelAt(currentLine))
				styler.SetLevel(currentLine, lev);

			currentLine++;
			previousLevel = currentLevel;
		}

	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	styler.SetLevel(currentLine,
					previousLevel
					| (styler.LevelAt(currentLine) & ~SC_FOLDLEVELNUMBERMASK));
}

static const char * const erlangWordListDesc[] = {
	"Erlang Reserved words",
	"Erlang BIFs",
	"Erlang Preprocessor",
	"Erlang Module Attributes",
	"Erlang Documentation",
	"Erlang Documentation Macro",
	0
};

LexerModule lmErlang(
	SCLEX_ERLANG,
	ColouriseErlangDoc,
	"erlang",
	FoldErlangDoc,
	erlangWordListDesc);
