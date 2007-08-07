// Scintilla source code edit control
/** @file LexPython.cxx
 ** Lexer for Python.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
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

enum kwType { kwOther, kwClass, kwDef, kwImport };

static bool IsPyComment(Accessor &styler, int pos, int len) {
	return len > 0 && styler[pos] == '#';
}

static bool IsPyStringStart(int ch, int chNext, int chNext2) {
	if (ch == '\'' || ch == '"')
		return true;
	if (ch == 'u' || ch == 'U') {
		if (chNext == '"' || chNext == '\'')
			return true;
		if ((chNext == 'r' || chNext == 'R') && (chNext2 == '"' || chNext2 == '\''))
			return true;
	}
	if ((ch == 'r' || ch == 'R') && (chNext == '"' || chNext == '\''))
		return true;

	return false;
}

/* Return the state to use for the string starting at i; *nextIndex will be set to the first index following the quote(s) */
static int GetPyStringState(Accessor &styler, int i, unsigned int *nextIndex) {
	char ch = styler.SafeGetCharAt(i);
	char chNext = styler.SafeGetCharAt(i + 1);

	// Advance beyond r, u, or ur prefix, but bail if there are any unexpected chars
	if (ch == 'r' || ch == 'R') {
		i++;
		ch = styler.SafeGetCharAt(i);
		chNext = styler.SafeGetCharAt(i + 1);
	} else if (ch == 'u' || ch == 'U') {
		if (chNext == 'r' || chNext == 'R')
			i += 2;
		else
			i += 1;
		ch = styler.SafeGetCharAt(i);
		chNext = styler.SafeGetCharAt(i + 1);
	}

	if (ch != '"' && ch != '\'') {
		*nextIndex = i + 1;
		return SCE_P_DEFAULT;
	}

	if (ch == chNext && ch == styler.SafeGetCharAt(i + 2)) {
		*nextIndex = i + 3;

		if (ch == '"')
			return SCE_P_TRIPLEDOUBLE;
		else
			return SCE_P_TRIPLE;
	} else {
		*nextIndex = i + 1;

		if (ch == '"')
			return SCE_P_STRING;
		else
			return SCE_P_CHARACTER;
	}
}

static inline bool IsAWordChar(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static void ColourisePyDoc(unsigned int startPos, int length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {

	int endPos = startPos + length;

	// Backtrack to previous line in case need to fix its tab whinging
	int lineCurrent = styler.GetLine(startPos);
	if (startPos > 0) {
		if (lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
			if (startPos == 0)
				initStyle = SCE_P_DEFAULT;
			else
				initStyle = styler.StyleAt(startPos - 1);
		}
	}

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];

	const int whingeLevel = styler.GetPropertyInt("tab.timmy.whinge.level");

	initStyle = initStyle & 31;
	if (initStyle == SCE_P_STRINGEOL) {
		initStyle = SCE_P_DEFAULT;
	}

	kwType kwLast = kwOther;
	int spaceFlags = 0;
	styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
	bool hexadecimal = false;

	// Python uses a different mask because bad indentation is marked by oring with 32
	StyleContext sc(startPos, endPos - startPos, initStyle, styler, 0x7f);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			const char chBad = static_cast<char>(64);
			const char chGood = static_cast<char>(0);
			char chFlags = chGood;
			if (whingeLevel == 1) {
				chFlags = (spaceFlags & wsInconsistent) ? chBad : chGood;
			} else if (whingeLevel == 2) {
				chFlags = (spaceFlags & wsSpaceTab) ? chBad : chGood;
			} else if (whingeLevel == 3) {
				chFlags = (spaceFlags & wsSpace) ? chBad : chGood;
			} else if (whingeLevel == 4) {
				chFlags = (spaceFlags & wsTab) ? chBad : chGood;
			}
			sc.SetState(sc.state);
			styler.SetFlags(chFlags, static_cast<char>(sc.state));
		}

		if (sc.atLineEnd) {
			if ((sc.state == SCE_P_DEFAULT) ||
			        (sc.state == SCE_P_TRIPLE) ||
			        (sc.state == SCE_P_TRIPLEDOUBLE)) {
				// Perform colourisation of white space and triple quoted strings at end of each line to allow
				// tab marking to work inside white space and triple quoted strings
				sc.SetState(sc.state);
			}
			lineCurrent++;
			styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
			if ((sc.state == SCE_P_STRING) || (sc.state == SCE_P_CHARACTER)) {
				sc.ChangeState(SCE_P_STRINGEOL);
				sc.ForwardSetState(SCE_P_DEFAULT);
			}
			if (!sc.More())
				break;
		}

		bool needEOLCheck = false;

		// Check for a state end
		if (sc.state == SCE_P_OPERATOR) {
			kwLast = kwOther;
			sc.SetState(SCE_P_DEFAULT);
		} else if (sc.state == SCE_P_NUMBER) {
			if (!IsAWordChar(sc.ch) &&
			        !(!hexadecimal && ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) {
				sc.SetState(SCE_P_DEFAULT);
			}
		} else if (sc.state == SCE_P_IDENTIFIER) {
			if ((sc.ch == '.') || (!IsAWordChar(sc.ch))) {
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				int style = SCE_P_IDENTIFIER;
				if ((kwLast == kwImport) && (strcmp(s, "as") == 0)) {
					style = SCE_P_WORD;
				} else if (keywords.InList(s)) {
					style = SCE_P_WORD;
				} else if (kwLast == kwClass) {
					style = SCE_P_CLASSNAME;
				} else if (kwLast == kwDef) {
					style = SCE_P_DEFNAME;
				} else if (keywords2.InList(s)) {
					style = SCE_P_WORD2;
				}
				sc.ChangeState(style);
				sc.SetState(SCE_P_DEFAULT);
				if (style == SCE_P_WORD) {
					if (0 == strcmp(s, "class"))
						kwLast = kwClass;
					else if (0 == strcmp(s, "def"))
						kwLast = kwDef;
					else if (0 == strcmp(s, "import"))
						kwLast = kwImport;
					else
						kwLast = kwOther;
				} else {
					kwLast = kwOther;
				}
			}
		} else if ((sc.state == SCE_P_COMMENTLINE) || (sc.state == SCE_P_COMMENTBLOCK)) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_P_DEFAULT);
			}
		} else if (sc.state == SCE_P_DECORATOR) {
			if (sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_P_DEFAULT);
			} else if (sc.ch == '#') {
				sc.SetState((sc.chNext == '#') ? SCE_P_COMMENTBLOCK  :  SCE_P_COMMENTLINE);
			}
		} else if ((sc.state == SCE_P_STRING) || (sc.state == SCE_P_CHARACTER)) {
			if (sc.ch == '\\') {
				if ((sc.chNext == '\r') && (sc.GetRelative(2) == '\n')) {
					sc.Forward();
				}
				sc.Forward();
			} else if ((sc.state == SCE_P_STRING) && (sc.ch == '\"')) {
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			} else if ((sc.state == SCE_P_CHARACTER) && (sc.ch == '\'')) {
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		} else if (sc.state == SCE_P_TRIPLE) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match("\'\'\'")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		} else if (sc.state == SCE_P_TRIPLEDOUBLE) {
			if (sc.ch == '\\') {
				sc.Forward();
			} else if (sc.Match("\"\"\"")) {
				sc.Forward();
				sc.Forward();
				sc.ForwardSetState(SCE_P_DEFAULT);
				needEOLCheck = true;
			}
		}

		// State exit code may have moved on to end of line
		if (needEOLCheck && sc.atLineEnd) {
			lineCurrent++;
			styler.IndentAmount(lineCurrent, &spaceFlags, IsPyComment);
			if (!sc.More())
				break;
		}

		// Check for a new state starting character
		if (sc.state == SCE_P_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (sc.ch == '0' && (sc.chNext == 'x' || sc.chNext == 'X')) {
					hexadecimal = true;
				} else {
					hexadecimal = false;
				}
				sc.SetState(SCE_P_NUMBER);
			} else if (isascii(sc.ch) && isoperator(static_cast<char>(sc.ch)) || sc.ch == '`') {
				sc.SetState(SCE_P_OPERATOR);
			} else if (sc.ch == '#') {
				sc.SetState(sc.chNext == '#' ? SCE_P_COMMENTBLOCK : SCE_P_COMMENTLINE);
			} else if (sc.ch == '@') {
				sc.SetState(SCE_P_DECORATOR);
			} else if (IsPyStringStart(sc.ch, sc.chNext, sc.GetRelative(2))) {
				unsigned int nextIndex = 0;
				sc.SetState(GetPyStringState(styler, sc.currentPos, &nextIndex));
				while (nextIndex > (sc.currentPos + 1) && sc.More()) {
					sc.Forward();
				}
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_P_IDENTIFIER);
			}
		}
	}
	sc.Complete();
}

static bool IsCommentLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	int eol_pos = styler.LineStart(line + 1) - 1;
	for (int i = pos; i < eol_pos; i++) {
		char ch = styler[i];
		if (ch == '#')
			return true;
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

static bool IsQuoteLine(int line, Accessor &styler) {
	int style = styler.StyleAt(styler.LineStart(line)) & 31;
	return ((style == SCE_P_TRIPLE) || (style == SCE_P_TRIPLEDOUBLE));
}


static void FoldPyDoc(unsigned int startPos, int length, int /*initStyle - unused*/,
                      WordList *[], Accessor &styler) {
	const int maxPos = startPos + length;
	const int maxLines = styler.GetLine(maxPos - 1);             // Requested last line
	const int docLines = styler.GetLine(styler.Length() - 1);  // Available last line
	const bool foldComment = styler.GetPropertyInt("fold.comment.python") != 0;
	const bool foldQuotes = styler.GetPropertyInt("fold.quotes.python") != 0;

	// Backtrack to previous non-blank line so we can determine indent level
	// for any white space lines (needed esp. within triple quoted strings)
	// and so we can fix any preceding fold level (which is why we go back
	// at least one line in all cases)
	int spaceFlags = 0;
	int lineCurrent = styler.GetLine(startPos);
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
	int prevQuote = foldQuotes && ((prev_state == SCE_P_TRIPLE) || (prev_state == SCE_P_TRIPLEDOUBLE));
	int prevComment = 0;
	if (lineCurrent >= 1)
		prevComment = foldComment && IsCommentLine(lineCurrent - 1, styler);

	// Process all characters to end of requested range or end of any triple quote
	// or comment that hangs over the end of the range.  Cap processing in all cases
	// to end of document (in case of unclosed quote or comment at end).
	while ((lineCurrent <= docLines) && ((lineCurrent <= maxLines) || prevQuote || prevComment)) {

		// Gather info
		int lev = indentCurrent;
		int lineNext = lineCurrent + 1;
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
		                           IsCommentLine(lineNext, styler) && (lev > SC_FOLDLEVELBASE));
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
		const int levelBeforeComments = Platform::Maximum(indentCurrentLevel,levelAfterComments);

		// Now set all the indent levels on the lines we skipped
		// Do this from end to start.  Once we encounter one line
		// which is indented more than the line after the end of
		// the comment-block, use the level of the block before

		int skipLine = lineNext;
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
			if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
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

static const char * const pythonWordListDesc[] = {
	"Keywords",
	"Highlighted identifiers",
	0
};

LexerModule lmPython(SCLEX_PYTHON, ColourisePyDoc, "python", FoldPyDoc,
					 pythonWordListDesc);
