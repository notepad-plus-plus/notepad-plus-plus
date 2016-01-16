// Scintilla source code edit control
// Nimrod lexer
// (c) 2009 Andreas Rumpf
/** @file LexNimrod.cxx
 ** Lexer for Nimrod.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsAWordChar(int ch) {
	return (ch >= 0x80) || isalnum(ch) || ch == '_';
}

static Sci_Position tillEndOfTripleQuote(Accessor &styler, Sci_Position pos, Sci_Position max) {
  /* search for """ */
  for (;;) {
    if (styler.SafeGetCharAt(pos, '\0') == '\0') return pos;
    if (pos >= max) return pos;
    if (styler.Match(pos, "\"\"\"")) {
      return pos + 2;
    }
    pos++;
  }
}

#define CR 13 /* use both because Scite allows changing the line ending */
#define LF 10

static bool inline isNewLine(int ch) {
  return ch == CR || ch == LF;
}

static Sci_Position scanString(Accessor &styler, Sci_Position pos, Sci_Position max, bool rawMode) {
  for (;;) {
    if (pos >= max) return pos;
    char ch = styler.SafeGetCharAt(pos, '\0');
    if (ch == CR || ch == LF || ch == '\0') return pos;
    if (ch == '"') return pos;
    if (ch == '\\' && !rawMode) {
      pos += 2;
    } else {
      pos++;
    }
  }
}

static Sci_Position scanChar(Accessor &styler, Sci_Position pos, Sci_Position max) {
  for (;;) {
    if (pos >= max) return pos;
    char ch = styler.SafeGetCharAt(pos, '\0');
    if (ch == CR || ch == LF || ch == '\0') return pos;
    if (ch == '\'' && !isalnum(styler.SafeGetCharAt(pos+1, '\0')) )
      return pos;
    if (ch == '\\') {
      pos += 2;
    } else {
      pos++;
    }
  }
}

static Sci_Position scanIdent(Accessor &styler, Sci_Position pos, WordList &keywords) {
  char buf[100]; /* copy to lowercase and ignore underscores */
  Sci_Position i = 0;

  for (;;) {
    char ch = styler.SafeGetCharAt(pos, '\0');
    if (!IsAWordChar(ch)) break;
    if (ch != '_' && i < ((int)sizeof(buf))-1) {
      buf[i] = static_cast<char>(tolower(ch));
      i++;
    }
    pos++;
  }
  buf[i] = '\0';
  /* look for keyword */
  if (keywords.InList(buf)) {
    styler.ColourTo(pos-1, SCE_P_WORD);
  } else {
    styler.ColourTo(pos-1, SCE_P_IDENTIFIER);
  }
  return pos;
}

static Sci_Position scanNumber(Accessor &styler, Sci_Position pos) {
  char ch, ch2;
  ch = styler.SafeGetCharAt(pos, '\0');
  ch2 = styler.SafeGetCharAt(pos+1, '\0');
  if (ch == '0' && (ch2 == 'b' || ch2 == 'B')) {
    /* binary number: */
    pos += 2;
    for (;;) {
      ch = styler.SafeGetCharAt(pos, '\0');
      if (ch == '_' || (ch >= '0' && ch <= '1')) ++pos;
      else break;
    }
  } else if (ch == '0' &&
            (ch2 == 'o' || ch2 == 'O' || ch2 == 'c' || ch2 == 'C')) {
    /* octal number: */
    pos += 2;
    for (;;) {
      ch = styler.SafeGetCharAt(pos, '\0');
      if (ch == '_' || (ch >= '0' && ch <= '7')) ++pos;
      else break;
    }
  } else if (ch == '0' && (ch2 == 'x' || ch2 == 'X')) {
    /* hexadecimal number: */
    pos += 2;
    for (;;) {
      ch = styler.SafeGetCharAt(pos, '\0');
      if (ch == '_' || (ch >= '0' && ch <= '9')
          || (ch >= 'a' && ch <= 'f')
          || (ch >= 'A' && ch <= 'F')) ++pos;
      else break;
    }
  } else {
    // skip decimal part:
    for (;;) {
      ch = styler.SafeGetCharAt(pos, '\0');
      if (ch == '_' || (ch >= '0' && ch <= '9')) ++pos;
      else break;
    }
    ch2 = styler.SafeGetCharAt(pos+1, '\0');
    if (ch == '.' && ch2 >= '0' && ch2 <= '9') {
      ++pos; // skip '.'
      for (;;) {
        ch = styler.SafeGetCharAt(pos, '\0');
        if (ch == '_' || (ch >= '0' && ch <= '9')) ++pos;
        else break;
      }
    }
    if (ch == 'e' || ch == 'E') {
      ++pos;
      ch = styler.SafeGetCharAt(pos, '\0');
      if (ch == '-' || ch == '+') ++pos;
      for (;;) {
        ch = styler.SafeGetCharAt(pos, '\0');
        if (ch == '_' || (ch >= '0' && ch <= '9')) ++pos;
        else break;
      }
    }
  }
  if (ch == '\'') {
    /* a type suffix: */
    pos++;
    for (;;) {
      ch = styler.SafeGetCharAt(pos);
      if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z')
         || (ch >= 'a' && ch <= 'z') || ch == '_') ++pos;
      else break;
    }
  }
  styler.ColourTo(pos-1, SCE_P_NUMBER);
  return pos;
}

/* rewritten from scratch, because I couldn't get rid of the bugs...
   (A character based approach sucks!)
*/
static void ColouriseNimrodDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                                WordList *keywordlists[], Accessor &styler) {
  Sci_Position pos = startPos;
  Sci_Position max = startPos + length;
  char ch;
  WordList &keywords = *keywordlists[0];

  styler.StartAt(startPos);
  styler.StartSegment(startPos);

  switch (initStyle) {
    /* check where we are: */
    case SCE_P_TRIPLEDOUBLE:
      pos = tillEndOfTripleQuote(styler, pos, max);
      styler.ColourTo(pos, SCE_P_TRIPLEDOUBLE);
      pos++;
    break;
    default: /* nothing to do: */
    break;
  }
  while (pos < max) {
    ch = styler.SafeGetCharAt(pos, '\0');
    switch (ch) {
      case '\0': return;
      case '#': {
        bool doccomment = (styler.SafeGetCharAt(pos+1) == '#');
        while (pos < max && !isNewLine(styler.SafeGetCharAt(pos, LF))) pos++;
        if (doccomment)
          styler.ColourTo(pos, SCE_C_COMMENTLINEDOC);
        else
          styler.ColourTo(pos, SCE_P_COMMENTLINE);
      } break;
      case 'r': case 'R': {
        if (styler.SafeGetCharAt(pos+1) == '"') {
          pos = scanString(styler, pos+2, max, true);
          styler.ColourTo(pos, SCE_P_STRING);
          pos++;
        } else {
          pos = scanIdent(styler, pos, keywords);
        }
      } break;
      case '"':
        if (styler.Match(pos+1, "\"\"")) {
          pos = tillEndOfTripleQuote(styler, pos+3, max);
          styler.ColourTo(pos, SCE_P_TRIPLEDOUBLE);
        } else {
          pos = scanString(styler, pos+1, max, false);
          styler.ColourTo(pos, SCE_P_STRING);
        }
        pos++;
      break;
      case '\'':
        pos = scanChar(styler, pos+1, max);
        styler.ColourTo(pos, SCE_P_CHARACTER);
        pos++;
      break;
      default: // identifers, numbers, operators, whitespace
        if (ch >= '0' && ch <= '9') {
          pos = scanNumber(styler, pos);
        } else if (IsAWordChar(ch)) {
          pos = scanIdent(styler, pos, keywords);
        } else if (ch == '`') {
          pos++;
          while (pos < max) {
            ch = styler.SafeGetCharAt(pos, LF);
            if (ch == '`') {
              ++pos;
              break;
            }
            if (ch == CR || ch == LF) break;
            ++pos;
          }
          styler.ColourTo(pos, SCE_P_IDENTIFIER);
        } else if (strchr("()[]{}:=;-\\/&%$!+<>|^?,.*~@", ch)) {
          styler.ColourTo(pos, SCE_P_OPERATOR);
          pos++;
        } else {
          styler.ColourTo(pos, SCE_P_DEFAULT);
          pos++;
        }
      break;
    }
  }
}

static bool IsCommentLine(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eol_pos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		if (ch == '#')
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

static bool IsQuoteLine(Sci_Position line, Accessor &styler) {
	int style = styler.StyleAt(styler.LineStart(line)) & 31;
	return ((style == SCE_P_TRIPLE) || (style == SCE_P_TRIPLEDOUBLE));
}


static void FoldNimrodDoc(Sci_PositionU startPos, Sci_Position length,
                          int /*initStyle - unused*/,
                          WordList *[], Accessor &styler) {
	const Sci_Position maxPos = startPos + length;
	const Sci_Position maxLines = styler.GetLine(maxPos - 1); // Requested last line
	const Sci_Position docLines = styler.GetLine(styler.Length() - 1); // Available last line
	const bool foldComment = styler.GetPropertyInt("fold.comment.nimrod") != 0;
	const bool foldQuotes = styler.GetPropertyInt("fold.quotes.nimrod") != 0;

	// Backtrack to previous non-blank line so we can determine indent level
	// for any white space lines (needed esp. within triple quoted strings)
	// and so we can fix any preceding fold level (which is why we go back
	// at least one line in all cases)
	int spaceFlags = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
	while (lineCurrent > 0) {
		lineCurrent--;
		indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
		if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG) &&
		        (!IsCommentLine(lineCurrent, styler)) &&
		        (!IsQuoteLine(lineCurrent, styler)))
			break;
	}
	int indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;

	// Set up initial loop state
	startPos = styler.LineStart(lineCurrent);
	int prev_state = SCE_P_DEFAULT & 31;
	if (lineCurrent >= 1)
		prev_state = styler.StyleAt(startPos - 1) & 31;
	int prevQuote = foldQuotes && ((prev_state == SCE_P_TRIPLE) ||
	                               (prev_state == SCE_P_TRIPLEDOUBLE));
	int prevComment = 0;
	if (lineCurrent >= 1)
		prevComment = foldComment && IsCommentLine(lineCurrent - 1, styler);

	// Process all characters to end of requested range or end of any triple quote
	// or comment that hangs over the end of the range.  Cap processing in all cases
	// to end of document (in case of unclosed quote or comment at end).
	while ((lineCurrent <= docLines) && ((lineCurrent <= maxLines) ||
	                                      prevQuote || prevComment)) {

		// Gather info
		int lev = indentCurrent;
		Sci_Position lineNext = lineCurrent + 1;
		int indentNext = indentCurrent;
		int quote = false;
		if (lineNext <= docLines) {
			// Information about next line is only available if not at end of document
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, NULL);
			int style = styler.StyleAt(styler.LineStart(lineNext)) & 31;
			quote = foldQuotes && ((style == SCE_P_TRIPLE) || (style == SCE_P_TRIPLEDOUBLE));
		}
		const int quote_start = (quote && !prevQuote);
		const int quote_continue = (quote && prevQuote);
		const int comment = foldComment && IsCommentLine(lineCurrent, styler);
		const int comment_start = (comment && !prevComment && (lineNext <= docLines) &&
		                           IsCommentLine(lineNext, styler) &&
		                           (lev > SC_FOLDLEVELBASE));
		const int comment_continue = (comment && prevComment);
		if ((!quote || !prevQuote) && !comment)
			indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;
		if (quote)
			indentNext = indentCurrentLevel;
		if (indentNext & SC_FOLDLEVELWHITEFLAG)
			indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;

		if (quote_start) {
			// Place fold point at start of triple quoted string
			lev |= SC_FOLDLEVELHEADERFLAG;
		} else if (quote_continue || prevQuote) {
			// Add level to rest of lines in the string
			lev = lev + 1;
		} else if (comment_start) {
			// Place fold point at start of a block of comments
			lev |= SC_FOLDLEVELHEADERFLAG;
		} else if (comment_continue) {
			// Add level to rest of lines in the block
			lev = lev + 1;
		}

		// Skip past any blank lines for next indent level info; we skip also
		// comments (all comments, not just those starting in column 0)
		// which effectively folds them into surrounding code rather
		// than screwing up folding.

		while (!quote &&
		        (lineNext < docLines) &&
		        ((indentNext & SC_FOLDLEVELWHITEFLAG) ||
		         (lineNext <= docLines && IsCommentLine(lineNext, styler)))) {

			lineNext++;
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, NULL);
		}

		const int levelAfterComments = indentNext & SC_FOLDLEVELNUMBERMASK;
		const int levelBeforeComments =
		    Maximum(indentCurrentLevel,levelAfterComments);

		// Now set all the indent levels on the lines we skipped
		// Do this from end to start.  Once we encounter one line
		// which is indented more than the line after the end of
		// the comment-block, use the level of the block before

		Sci_Position skipLine = lineNext;
		int skipLevel = levelAfterComments;

		while (--skipLine > lineCurrent) {
			int skipLineIndent = styler.IndentAmount(skipLine, &spaceFlags, NULL);

			if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments)
				skipLevel = levelBeforeComments;

			int whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;

			styler.SetLevel(skipLine, skipLevel | whiteFlag);
		}

		// Set fold header on non-quote/non-comment line
		if (!quote && !comment && !(indentCurrent & SC_FOLDLEVELWHITEFLAG) ) {
			if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) <
			     (indentNext & SC_FOLDLEVELNUMBERMASK))
				lev |= SC_FOLDLEVELHEADERFLAG;
		}

		// Keep track of triple quote and block comment state of previous line
		prevQuote = quote;
		prevComment = comment_start || comment_continue;

		// Set fold level for this line and move to next line
		styler.SetLevel(lineCurrent, lev);
		indentCurrent = indentNext;
		lineCurrent = lineNext;
	}

	// NOTE: Cannot set level of last line here because indentCurrent doesn't have
	// header flag set; the loop above is crafted to take care of this case!
	//styler.SetLevel(lineCurrent, indentCurrent);
}

static const char * const nimrodWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmNimrod(SCLEX_NIMROD, ColouriseNimrodDoc, "nimrod", FoldNimrodDoc,
				     nimrodWordListDesc);
