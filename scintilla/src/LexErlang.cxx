// Scintilla source code edit control
/** @file LexErlang.cxx
 ** Lexer for Erlang.
 ** Written by Peter-Henry Mander, based on Matlab lexer by José Fonseca
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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

/*
   TODO:
   o  _Param should be a new lexical type
*/

static int is_radix(int radix, int ch) {
   int digit;
   if ( 16 < radix || 2 > radix ) {
      return 0;
   }
   if ( isdigit(ch) ) {
      digit = ch - '0';
   } else if ( isxdigit(ch) ) {
      digit = toupper(ch) - 'A' + 10;
   } else {
      return 0;
   }
   if ( digit < radix ) {
      return 1;
   } else {
      return 0;
   }
}

typedef enum {
   STATE_NULL,
   ATOM_UNQUOTED,
   ATOM_QUOTED,
   ATOM_FUN_NAME,
   NODE_NAME_UNQUOTED,
   NODE_NAME_QUOTED,
   MACRO_START,
   MACRO_UNQUOTED,
   MACRO_QUOTED,
   RECORD_START,
   RECORD_UNQUOTED,
   RECORD_QUOTED,
   NUMERAL_START,
   NUMERAL_SIGNED,
   NUMERAL_RADIX_LITERAL,
   NUMERAL_SPECULATIVE_MANTISSA,
   NUMERAL_FLOAT_MANTISSA,
   NUMERAL_FLOAT_EXPONENT,
   NUMERAL_FLOAT_SIGNED_EXPONENT,
   PARSE_ERROR
} atom_parse_state_t;

static void ColouriseErlangDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];

	styler.StartAt(startPos);

	StyleContext sc(startPos, length, initStyle, styler);
   atom_parse_state_t parse_state = STATE_NULL;
   int radix_digits = 0;
   int exponent_digits = 0;
	for (; sc.More(); sc.Forward()) {
      if ( STATE_NULL != parse_state ) {
         switch (parse_state) {
         case STATE_NULL:
				sc.SetState(SCE_ERLANG_DEFAULT);
            break;
         case ATOM_UNQUOTED:
            if ( '@' == sc.ch ){
               parse_state = NODE_NAME_UNQUOTED;
            } else if ( !isalnum(sc.ch) && sc.ch != '_' ) {
               char s[100];
               sc.GetCurrent(s, sizeof(s));
               if (keywords.InList(s)) {
                  sc.ChangeState(SCE_ERLANG_KEYWORD);
                  sc.SetState(SCE_ERLANG_DEFAULT);
                  parse_state = STATE_NULL;
               } else {
                  if ( '/' == sc.ch ) {
                     parse_state = ATOM_FUN_NAME;
                  } else {
                     sc.ChangeState(SCE_ERLANG_ATOM);
                     sc.SetState(SCE_ERLANG_DEFAULT);
                     parse_state = STATE_NULL;
                  }
               }
            }
            break;
         case ATOM_QUOTED:
            if ( '@' == sc.ch ){
               parse_state = NODE_NAME_QUOTED;
            } else if ( '\'' == sc.ch && '\\' != sc.chPrev ) {
               sc.ChangeState(SCE_ERLANG_ATOM);
               sc.ForwardSetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case ATOM_FUN_NAME:
            if ( !isdigit(sc.ch) ) {
               sc.ChangeState(SCE_ERLANG_FUNCTION_NAME);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NODE_NAME_QUOTED:
            if ( '@' == sc.ch ) {
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else if ( '\'' == sc.ch && '\\' != sc.chPrev ) {
               sc.ChangeState(SCE_ERLANG_NODE_NAME);
               sc.ForwardSetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NODE_NAME_UNQUOTED:
            if ( '@' == sc.ch ) {
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else if ( !isalnum(sc.ch) && sc.ch != '_' ) {
               sc.ChangeState(SCE_ERLANG_NODE_NAME);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case RECORD_START:
            if ( '\'' == sc.ch ) {
               parse_state = RECORD_QUOTED;
            } else if (isalpha(sc.ch) && islower(sc.ch)) {
               parse_state = RECORD_UNQUOTED;
            } else { // error
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case RECORD_QUOTED:
            if ( '\'' == sc.ch && '\\' != sc.chPrev ) {
               sc.ChangeState(SCE_ERLANG_RECORD);
               sc.ForwardSetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case RECORD_UNQUOTED:
            if ( !isalpha(sc.ch) && '_' != sc.ch ) {
               sc.ChangeState(SCE_ERLANG_RECORD);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case MACRO_START:
            if ( '\'' == sc.ch ) {
               parse_state = MACRO_QUOTED;
            } else if (isalpha(sc.ch)) {
               parse_state = MACRO_UNQUOTED;
            } else { // error
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case MACRO_UNQUOTED:
            if ( !isalpha(sc.ch) && '_' != sc.ch ) {
               sc.ChangeState(SCE_ERLANG_MACRO);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case MACRO_QUOTED:
            if ( '\'' == sc.ch && '\\' != sc.chPrev ) {
               sc.ChangeState(SCE_ERLANG_MACRO);
               sc.ForwardSetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NUMERAL_START:
            if ( isdigit(sc.ch) ) {
               radix_digits *= 10;
               radix_digits += sc.ch - '0'; // Assuming ASCII here!
            } else if ( '#' == sc.ch ) {
               if ( 2 > radix_digits || 16 < radix_digits) {
                  sc.SetState(SCE_ERLANG_DEFAULT);
                  parse_state = STATE_NULL;
               } else {
                  parse_state = NUMERAL_RADIX_LITERAL;
               }
            } else if ( '.' == sc.ch && isdigit(sc.chNext)) {
               radix_digits = 0;
               parse_state = NUMERAL_FLOAT_MANTISSA;
            } else if ( 'e' == sc.ch || 'E' == sc.ch ) {
               exponent_digits = 0;
               parse_state = NUMERAL_FLOAT_EXPONENT;
            } else {
               radix_digits = 0;
               sc.ChangeState(SCE_ERLANG_NUMBER);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NUMERAL_RADIX_LITERAL:
            if ( !is_radix(radix_digits,sc.ch) ) {
               radix_digits = 0;
               if ( !isalnum(sc.ch) ) {
                  sc.ChangeState(SCE_ERLANG_NUMBER);
               }
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NUMERAL_FLOAT_MANTISSA:
            if ( 'e' == sc.ch || 'E' == sc.ch ) {
               exponent_digits = 0;
               parse_state = NUMERAL_FLOAT_EXPONENT;
            } else if ( !isdigit(sc.ch) ) {
               sc.ChangeState(SCE_ERLANG_NUMBER);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            }
            break;
         case NUMERAL_FLOAT_EXPONENT:
            if ( '-' == sc.ch || '+' == sc.ch ) {
               parse_state = NUMERAL_FLOAT_SIGNED_EXPONENT;
            } else if ( !isdigit(sc.ch) ) {
               if ( 0 < exponent_digits ) {
                  sc.ChangeState(SCE_ERLANG_NUMBER);
               }
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else {
               ++exponent_digits;
            }
            break;
         case NUMERAL_FLOAT_SIGNED_EXPONENT:
            if ( !isdigit(sc.ch) ) {
               if ( 0 < exponent_digits ) {
                  sc.ChangeState(SCE_ERLANG_NUMBER);
               }
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else {
               ++exponent_digits;
            }
            break;
         case NUMERAL_SIGNED:
            if ( !isdigit(sc.ch) ) {
               sc.ChangeState(SCE_ERLANG_NUMBER);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else if ( '.' == sc.ch ) {
               parse_state = NUMERAL_FLOAT_MANTISSA;
            }
            break;
         case NUMERAL_SPECULATIVE_MANTISSA:
            if ( !isdigit(sc.ch) ) {
               sc.ChangeState(SCE_ERLANG_OPERATOR);
               sc.SetState(SCE_ERLANG_DEFAULT);
               parse_state = STATE_NULL;
            } else {
               parse_state = NUMERAL_FLOAT_MANTISSA;
            }
            break;
         case PARSE_ERROR:
				sc.SetState(SCE_ERLANG_DEFAULT);
            parse_state = STATE_NULL;
            break;
         }
      } else if (sc.state == SCE_ERLANG_OPERATOR) {
			if (sc.chPrev == '.') {
				if (sc.ch == '*' || sc.ch == '/' || sc.ch == '\\' || sc.ch == '^') {
					sc.ForwardSetState(SCE_ERLANG_DEFAULT);
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_ERLANG_DEFAULT);
				} else {
					sc.SetState(SCE_ERLANG_DEFAULT);
				}
			} else {
				sc.SetState(SCE_ERLANG_DEFAULT);
			}
		} else if (sc.state == SCE_ERLANG_VARIABLE) {
			if (!isalnum(sc.ch) && sc.ch != '_') {
            sc.SetState(SCE_ERLANG_DEFAULT);
			}
		} else if (sc.state == SCE_ERLANG_STRING) {
			if (sc.ch == '\"' && sc.chPrev != '\\') {
				sc.ForwardSetState(SCE_ERLANG_DEFAULT);
			}
		} else if (sc.state == SCE_ERLANG_COMMENT ) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_ERLANG_DEFAULT);
			}
      } else if (sc.state == SCE_ERLANG_CHARACTER ) {
         if ( sc.chPrev == '\\' ) {
            sc.ForwardSetState(SCE_ERLANG_DEFAULT);
         } else if ( sc.ch != '\\' ) {
            sc.ForwardSetState(SCE_ERLANG_DEFAULT);
         }
      }

		if (sc.state == SCE_ERLANG_DEFAULT) {
			if (sc.ch == '%') {
				sc.SetState(SCE_ERLANG_COMMENT);
			} else if (sc.ch == '\"') {
            sc.SetState(SCE_ERLANG_STRING);
         } else if (sc.ch == '#') {
            parse_state = RECORD_START;
				sc.SetState(SCE_ERLANG_UNKNOWN);
         } else if (sc.ch == '?') {
            parse_state = MACRO_START;
				sc.SetState(SCE_ERLANG_UNKNOWN);
         } else if (sc.ch == '$') {
				sc.SetState(SCE_ERLANG_CHARACTER);
         } else if (sc.ch == '\'') {
            parse_state = ATOM_QUOTED;
				sc.SetState(SCE_ERLANG_UNKNOWN);
			} else if ( isdigit(sc.ch) ) {
            parse_state = NUMERAL_START;
            radix_digits = sc.ch - '0';
				sc.SetState(SCE_ERLANG_UNKNOWN);
         } else if ( '.' == sc.ch ) {
            parse_state = NUMERAL_SPECULATIVE_MANTISSA;
				sc.SetState(SCE_ERLANG_UNKNOWN);
			} else if (isalpha(sc.ch) && isupper(sc.ch)) {
				sc.SetState(SCE_ERLANG_VARIABLE);
			} else if (isalpha(sc.ch)) {
            parse_state = ATOM_UNQUOTED;
				sc.SetState(SCE_ERLANG_UNKNOWN);
			} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '\\') {
				sc.SetState(SCE_ERLANG_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static int ClassifyFoldPointErlang(
   Accessor &styler,
   int styleNext,
   int keyword_start
) {
	int lev = 0;
   if ( styler.Match(keyword_start,"case")
      || (
            styler.Match(keyword_start,"fun")
         && SCE_ERLANG_FUNCTION_NAME != styleNext)
      || styler.Match(keyword_start,"if")
      || styler.Match(keyword_start,"query")
      || styler.Match(keyword_start,"receive")
   ) {
      ++lev;
   } else if ( styler.Match(keyword_start,"end") ) {
      --lev;
   }
	return lev;
}


static void FoldErlangDoc(
   unsigned int startPos, int length, int initStyle,
   WordList** /*keywordlists*/, Accessor &styler
) {
	unsigned int endPos = startPos + length;
	//~ int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler.SafeGetCharAt(startPos);
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	int keyword_start = 0;

   bool fold_keywords = true;
   bool fold_comments = true;
   bool fold_braces = true;
   bool fold_function_clauses = false;
   bool fold_clauses = false;

   //int clause_level = 0;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

      if ( (stylePrev != SCE_ERLANG_KEYWORD) && (style == SCE_ERLANG_KEYWORD) ) {
         keyword_start = i;
      }
      if ( fold_keywords ) {
         if ( (stylePrev == SCE_ERLANG_KEYWORD)
            && (style != SCE_ERLANG_KEYWORD)
            && (style != SCE_ERLANG_ATOM)
         ) {
            levelCurrent += ClassifyFoldPointErlang(styler,styleNext,keyword_start);
         }
      }

      if ( fold_comments ) {
         if (style == SCE_ERLANG_COMMENT) {
            if ((ch == '%') && (chNext == '{')) {
               levelCurrent++;
            } else if ((ch == '%') && (chNext == '}')) {
               levelCurrent--;
            }
         }
      }

      if ( fold_function_clauses ) {
         if ( (SC_FOLDLEVELBASE == levelCurrent) /*&& (style == SCE_ERLANG_OPERATOR)*/ ) {
            if ( (ch == '-') && (chNext == '>')) {
               //~ fprintf(stderr,"levelCurrent=%d\n", levelCurrent);
               //++clause_level;
               //~ if ( 0 < clause_level )
                  ++levelCurrent;
            }
         }
         //~ if (  (stylePrev != SCE_ERLANG_RECORD)
            //~ && (style != SCE_ERLANG_NUMBER)
            //~ && (style != SCE_ERLANG_STRING)
            //~ && (style != SCE_ERLANG_COMMENT)
         //~ ) {
            if ( (SC_FOLDLEVELBASE+1 == levelCurrent) && (ch == '.') ) {
               //--clause_level;
               //~ if ( 0 == clause_level )
                  --levelCurrent;
            }
         //~ }
      }

      if ( fold_clauses ) {
         if ( (0 < levelCurrent) && (style == SCE_ERLANG_OPERATOR) ) {
            if ((ch == '-') && (chNext == '>')) {
               levelCurrent++;
            }
            if ( (ch == ';') ) {
               levelCurrent--;
            }
         }
         if ( (stylePrev != SCE_ERLANG_RECORD)
            && (style != SCE_ERLANG_NUMBER)
            && (style != SCE_ERLANG_STRING)
            && (style != SCE_ERLANG_COMMENT)
         ) {
            if ( (ch == '.') ) {
               levelCurrent--;
            }
         }
         if (  (stylePrev == SCE_ERLANG_KEYWORD)
            && (style != SCE_ERLANG_KEYWORD)
            && (style != SCE_ERLANG_ATOM)
            && (
               styler.Match(keyword_start,"end") // 'end' counted twice if fold_keywords too
               || styler.Match(keyword_start,"after") )
         ) {
            levelCurrent--;
         }
      }

      if ( fold_braces ) {
         if (style == SCE_ERLANG_OPERATOR) {
            if ( (ch == '{') || (ch == '(') || (ch == '[') ) {
               levelCurrent++;
            } else if ( (ch == '}') || (ch == ')') || (ch == ']') ) {
               levelCurrent--;
            }
         }
      }

		if (atEOL) {
			int lev = levelPrev;
			//~ if (visibleChars == 0 && foldCompact)
				//~ lev |= SC_FOLDLEVELWHITEFLAG;
			//~ if ((levelCurrent > levelPrev) && (visibleChars > 0))
			if ((levelCurrent > levelPrev)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
         }
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			//~ visibleChars = 0;
		}
		//~ if (!isspacechar(ch))
			//~ visibleChars++;

	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const erlangWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmErlang(
   SCLEX_ERLANG,
   ColouriseErlangDoc,
   "erlang",
   FoldErlangDoc,
   erlangWordListDesc);

