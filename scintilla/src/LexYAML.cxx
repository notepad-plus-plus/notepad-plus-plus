// Scintilla source code edit control
/** @file LexYAML.cxx
 ** Lexer for YAML.
 **/
// Copyright 2003- by Sean O'Dell <sean@celsoft.com>
// Release under the same license as Scintilla/SciTE.

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

static const char * const yamlWordListDesc[] = {
	"Keywords",
	0
};

static inline bool AtEOL(Accessor &styler, unsigned int i) {
	return (styler[i] == '\n') ||
		((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

static unsigned int SpaceCount(char* lineBuffer) {
	if (lineBuffer == NULL)
		return 0;
	
	char* headBuffer = lineBuffer;
	
	while (*headBuffer == ' ')
		headBuffer++;
	
	return headBuffer - lineBuffer;
}

#define YAML_STATE_BITSIZE 16
#define YAML_STATE_MASK			(0xFFFF0000)
#define YAML_STATE_DOCUMENT		(1 << YAML_STATE_BITSIZE)
#define YAML_STATE_VALUE			(2 << YAML_STATE_BITSIZE)
#define YAML_STATE_COMMENT		(3 << YAML_STATE_BITSIZE)
#define YAML_STATE_TEXT_PARENT	(4 << YAML_STATE_BITSIZE)
#define YAML_STATE_TEXT			(5 << YAML_STATE_BITSIZE)

static void ColouriseYAMLLine(
	char *lineBuffer,
	unsigned int currentLine,
	unsigned int lengthLine,
	unsigned int startLine,
	unsigned int endPos,
	WordList &keywords,
	Accessor &styler) {
	    
	unsigned int i = 0;
	bool bInQuotes = false;
	unsigned int indentAmount = SpaceCount(lineBuffer);
		
	if (currentLine > 0) {
		int parentLineState = styler.GetLineState(currentLine - 1);
	
		if ((parentLineState&YAML_STATE_MASK) == YAML_STATE_TEXT || (parentLineState&YAML_STATE_MASK) == YAML_STATE_TEXT_PARENT) {
			unsigned int parentIndentAmount = parentLineState&(~YAML_STATE_MASK);
			if (indentAmount > parentIndentAmount) {
				styler.SetLineState(currentLine, YAML_STATE_TEXT | parentIndentAmount);
				styler.ColourTo(endPos, SCE_YAML_TEXT);
				return;
			}
		}
	}
	styler.SetLineState(currentLine, 0);
	if (strncmp(lineBuffer, "---", 3) == 0) {	// Document marker
		styler.SetLineState(currentLine, YAML_STATE_DOCUMENT);
		styler.ColourTo(endPos, SCE_YAML_DOCUMENT);
		return;
	}
	// Skip initial spaces
	while ((i < lengthLine) && lineBuffer[i] == ' ') { // YAML always uses space, never TABS or anything else
		i++;
	}
	if (lineBuffer[i] == '\t') { // if we skipped all spaces, and we are NOT inside a text block, this is wrong
		styler.ColourTo(endPos, SCE_YAML_ERROR);
		return;
	}
	if (lineBuffer[i] == '#') {	// Comment
		styler.SetLineState(currentLine, YAML_STATE_COMMENT);
		styler.ColourTo(endPos, SCE_YAML_COMMENT);
		return;
	}
	while (i < lengthLine) {
		if (lineBuffer[i] == '\'' || lineBuffer[i] == '\"') {
			bInQuotes = !bInQuotes;
		} else if (lineBuffer[i] == ':' && !bInQuotes) {
			styler.ColourTo(startLine + i, SCE_YAML_IDENTIFIER);
			// Non-folding scalar
			i++;
			while ((i < lengthLine) && isspacechar(lineBuffer[i]))
				i++;
			unsigned int endValue = lengthLine - 1;
			while ((endValue >= i) && isspacechar(lineBuffer[endValue]))
				endValue--;
			lineBuffer[endValue + 1] = '\0';
			if (lineBuffer[i] == '|' || lineBuffer[i] == '>') {
				i++;
				if (lineBuffer[i] == '+' || lineBuffer[i] == '-')
					i++;
				while ((i < lengthLine) && isspacechar(lineBuffer[i]))
					i++;
				if (lineBuffer[i] == '\0') {
					styler.SetLineState(currentLine, YAML_STATE_TEXT_PARENT | indentAmount);
					styler.ColourTo(endPos, SCE_YAML_DEFAULT);
					return;
				} else if (lineBuffer[i] == '#') {
					styler.SetLineState(currentLine, YAML_STATE_TEXT_PARENT | indentAmount);
					styler.ColourTo(startLine + i - 1, SCE_YAML_DEFAULT);
					styler.ColourTo(endPos, SCE_YAML_COMMENT);
					return;
				} else {
					styler.ColourTo(endPos, SCE_YAML_ERROR);
					return;
				}
			}
			styler.SetLineState(currentLine, YAML_STATE_VALUE);
			if (lineBuffer[i] == '&' || lineBuffer[i] == '*') {
				styler.ColourTo(endPos, SCE_YAML_REFERENCE);
				return;
			}
			if (keywords.InList(&lineBuffer[i])) { // Convertible value (true/false, etc.)
				styler.ColourTo(endPos, SCE_YAML_KEYWORD);
				return;
			} else {
				unsigned int i2 = i;
				while ((i < lengthLine) && lineBuffer[i]) {
					if (!isdigit(lineBuffer[i]) && lineBuffer[i] != '-' && lineBuffer[i] != '.' && lineBuffer[i] != ',') {
						styler.ColourTo(endPos, SCE_YAML_DEFAULT);
						return;
					}
					i++;
				}
				if (i > i2) {
					styler.ColourTo(endPos, SCE_YAML_NUMBER);
					return;
				}
			}
			break; // shouldn't get here, but just in case, the rest of the line is coloured the default
		}
		i++;
	}
	styler.ColourTo(endPos, SCE_YAML_DEFAULT);
}

static void ColouriseYAMLDoc(unsigned int startPos, int length, int, WordList *keywordLists[], Accessor &styler) {
	char lineBuffer[1024];
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	unsigned int linePos = 0;
	unsigned int startLine = startPos;
	unsigned int endPos = startPos + length;
	unsigned int maxPos = styler.Length();
	unsigned int lineCurrent = styler.GetLine(startPos);
	
	for (unsigned int i = startPos; i < maxPos && i < endPos; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseYAMLLine(lineBuffer, lineCurrent, linePos, startLine, i, *keywordLists[0], styler);
			linePos = 0;
			startLine = i + 1;
			lineCurrent++;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		ColouriseYAMLLine(lineBuffer, lineCurrent, linePos, startLine, startPos + length - 1, *keywordLists[0], styler);
	}
}

static bool IsCommentLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	if (styler[pos] == '#')
		return true;
	return false;
}

static void FoldYAMLDoc(unsigned int startPos, int length, int /*initStyle - unused*/,
                      WordList *[], Accessor &styler) {
	const int maxPos = startPos + length;
	const int maxLines = styler.GetLine(maxPos - 1);             // Requested last line
	const int docLines = styler.GetLine(styler.Length() - 1);  // Available last line
	const bool foldComment = styler.GetPropertyInt("fold.comment.yaml") != 0;

	// Backtrack to previous non-blank line so we can determine indent level
	// for any white space lines
	// and so we can fix any preceding fold level (which is why we go back
	// at least one line in all cases)
	int spaceFlags = 0;
	int lineCurrent = styler.GetLine(startPos);
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
	while (lineCurrent > 0) {
		lineCurrent--;
		indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
		if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG) &&
		        (!IsCommentLine(lineCurrent, styler)))
			break;
	}
	int indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;

	// Set up initial loop state
	int prevComment = 0;
	if (lineCurrent >= 1)
		prevComment = foldComment && IsCommentLine(lineCurrent - 1, styler);

	// Process all characters to end of requested range
	// or comment that hangs over the end of the range.  Cap processing in all cases
	// to end of document (in case of unclosed comment at end).
	while ((lineCurrent <= docLines) && ((lineCurrent <= maxLines) || prevComment)) {

		// Gather info
		int lev = indentCurrent;
		int lineNext = lineCurrent + 1;
		int indentNext = indentCurrent;
		if (lineNext <= docLines) {
			// Information about next line is only available if not at end of document
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, NULL);
	}
		const int comment = foldComment && IsCommentLine(lineCurrent, styler);
		const int comment_start = (comment && !prevComment && (lineNext <= docLines) &&
		                           IsCommentLine(lineNext, styler) && (lev > SC_FOLDLEVELBASE));
		const int comment_continue = (comment && prevComment);
		if (!comment)
			indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;
		if (indentNext & SC_FOLDLEVELWHITEFLAG)
			indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;

		if (comment_start) {
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

		while ((lineNext < docLines) &&
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

		// Set fold header on non-comment line
		if (!comment && !(indentCurrent & SC_FOLDLEVELWHITEFLAG) ) {
			if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
				lev |= SC_FOLDLEVELHEADERFLAG;
		}

		// Keep track of block comment state of previous line
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

LexerModule lmYAML(SCLEX_YAML, ColouriseYAMLDoc, "yaml", FoldYAMLDoc, yamlWordListDesc);
