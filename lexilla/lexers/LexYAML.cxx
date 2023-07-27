// Scintilla source code edit control
/** @file LexYAML.cxx
 ** Lexer for YAML.
 **/
// Copyright 2003- by Sean O'Dell <sean@celsoft.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

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

namespace {

const char * const yamlWordListDesc[] = {
	"Keywords",
	nullptr
};

inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
		((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

/**
 * Check for space, tab, line feed, or carriage return.
 * See YAML 1.2 spec sections 5.4. Line Break Characters and 5.5. White Space Characters.
 */
constexpr bool IsWhiteSpaceOrEOL(char ch) noexcept {
	return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

unsigned int SpaceCount(char* lineBuffer) noexcept {
	if (lineBuffer == nullptr)
		return 0;

	char* headBuffer = lineBuffer;

	while (*headBuffer == ' ')
		headBuffer++;

	return static_cast<unsigned int>(headBuffer - lineBuffer);
}

bool KeywordAtChar(const char* lineBuffer, char* startComment, const WordList &keywords) noexcept {
	if (lineBuffer == nullptr || startComment <= lineBuffer)
		return false;
	char* endValue = startComment - 1;
	while (endValue >= lineBuffer && *endValue == ' ')
		endValue--;
	Sci_PositionU len = static_cast<Sci_PositionU>(endValue - lineBuffer) + 1;
	char s[100];
	if (len > (sizeof(s) / sizeof(s[0]) - 1))
		return false;
	strncpy(s, lineBuffer, len);
	s[len] = '\0';
	return (keywords.InList(s));
}

#define YAML_STATE_BITSIZE		16
#define YAML_STATE_MASK			(0xFFFF0000)
#define YAML_STATE_DOCUMENT		(1 << YAML_STATE_BITSIZE)
#define YAML_STATE_VALUE		(2 << YAML_STATE_BITSIZE)
#define YAML_STATE_COMMENT		(3 << YAML_STATE_BITSIZE)
#define YAML_STATE_TEXT_PARENT	(4 << YAML_STATE_BITSIZE)
#define YAML_STATE_TEXT			(5 << YAML_STATE_BITSIZE)

void ColouriseYAMLLine(
	char *lineBuffer,
	Sci_PositionU currentLine,
	Sci_PositionU lengthLine,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
	const WordList &keywords,
	Accessor &styler) {

	Sci_PositionU i = 0;
	bool bInQuotes = false;
	const unsigned int indentAmount = SpaceCount(lineBuffer);

	if (currentLine > 0) {
		const int parentLineState = styler.GetLineState(currentLine - 1);

		if ((parentLineState&YAML_STATE_MASK) == YAML_STATE_TEXT || (parentLineState&YAML_STATE_MASK) == YAML_STATE_TEXT_PARENT) {
			const unsigned int parentIndentAmount = parentLineState&(~YAML_STATE_MASK);
			if (indentAmount > parentIndentAmount) {
				styler.SetLineState(currentLine, YAML_STATE_TEXT | parentIndentAmount);
				styler.ColourTo(endPos, SCE_YAML_TEXT);
				return;
			}
		}
	}
	styler.SetLineState(currentLine, 0);
	if (strncmp(lineBuffer, "---", 3) == 0 || strncmp(lineBuffer, "...", 3) == 0) {	// Document marker
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
		} else if (lineBuffer[i] == '#' && isspacechar(lineBuffer[i - 1]) && !bInQuotes) {
			styler.ColourTo(startLine + i - 1, SCE_YAML_DEFAULT);
			styler.ColourTo(endPos, SCE_YAML_COMMENT);
			return;
		} else if (lineBuffer[i] == ':' && !bInQuotes && (IsWhiteSpaceOrEOL(lineBuffer[i + 1]) || i == lengthLine - 1)) {
			styler.ColourTo(startLine + i - 1, SCE_YAML_IDENTIFIER);
			styler.ColourTo(startLine + i, SCE_YAML_OPERATOR);
			// Non-folding scalar
			i++;
			while ((i < lengthLine) && isspacechar(lineBuffer[i]))
				i++;
			Sci_PositionU endValue = lengthLine - 1;
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
			} else if (lineBuffer[i] == '#') {
				styler.ColourTo(startLine + i - 1, SCE_YAML_DEFAULT);
				styler.ColourTo(endPos, SCE_YAML_COMMENT);
				return;
			}
			Sci_PositionU startComment = i;
			bInQuotes = false;
			while (startComment < lengthLine) { // Comment must be space padded
				if (lineBuffer[startComment] == '\'' || lineBuffer[startComment] == '\"')
					bInQuotes = !bInQuotes;
				if (lineBuffer[startComment] == '#' && isspacechar(lineBuffer[startComment - 1]) && !bInQuotes)
					break;
				startComment++;
			}
			styler.SetLineState(currentLine, YAML_STATE_VALUE);
			if (lineBuffer[i] == '&' || lineBuffer[i] == '*') {
				styler.ColourTo(startLine + startComment - 1, SCE_YAML_REFERENCE);
				if (startComment < lengthLine)
					styler.ColourTo(endPos, SCE_YAML_COMMENT);
				return;
			}
			if (KeywordAtChar(&lineBuffer[i], &lineBuffer[startComment], keywords)) { // Convertible value (true/false, etc.)
				styler.ColourTo(startLine + startComment - 1, SCE_YAML_KEYWORD);
				if (startComment < lengthLine)
					styler.ColourTo(endPos, SCE_YAML_COMMENT);
				return;
			}
			const Sci_PositionU i2 = i;
			while ((i < startComment) && lineBuffer[i]) {
				if (!(IsASCII(lineBuffer[i]) && isdigit(lineBuffer[i])) && lineBuffer[i] != '-'
				        && lineBuffer[i] != '.' && lineBuffer[i] != ',' && lineBuffer[i] != ' ') {
					styler.ColourTo(startLine + startComment - 1, SCE_YAML_DEFAULT);
					if (startComment < lengthLine)
						styler.ColourTo(endPos, SCE_YAML_COMMENT);
					return;
				}
				i++;
			}
			if (i > i2) {
				styler.ColourTo(startLine + startComment - 1, SCE_YAML_NUMBER);
				if (startComment < lengthLine)
					styler.ColourTo(endPos, SCE_YAML_COMMENT);
				return;
			}
			break; // shouldn't get here, but just in case, the rest of the line is coloured the default
		}
		i++;
	}
	styler.ColourTo(endPos, SCE_YAML_DEFAULT);
}

void ColouriseYAMLDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *keywordLists[], Accessor &styler) {
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;
	const Sci_PositionU endPos = startPos + length;
	const Sci_PositionU maxPos = styler.Length();
	Sci_PositionU lineCurrent = styler.GetLine(startPos);

	for (Sci_PositionU i = startPos; i < maxPos && i < endPos; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line (or of line buffer) met, colourise it
			ColouriseYAMLLine(lineBuffer.data(), lineCurrent, lineBuffer.length(), startLine, i, *keywordLists[0], styler);
			lineBuffer.clear();
			startLine = i + 1;
			lineCurrent++;
		}
	}
	if (!lineBuffer.empty()) {	// Last line does not have ending characters
		ColouriseYAMLLine(lineBuffer.data(), lineCurrent, lineBuffer.length(), startLine, startPos + length - 1, *keywordLists[0], styler);
	}
}

bool IsCommentLine(Sci_Position line, Accessor &styler) {
	const Sci_Position pos = styler.LineStart(line);
	if (styler[pos] == '#')
		return true;
	return false;
}

void FoldYAMLDoc(Sci_PositionU startPos, Sci_Position length, int /*initStyle - unused*/,
                      WordList *[], Accessor &styler) {
	const Sci_Position maxPos = startPos + length;
	const Sci_Position maxLines = styler.GetLine(maxPos - 1);             // Requested last line
	const Sci_Position docLines = styler.GetLine(styler.Length() - 1);  // Available last line

	// property fold.comment.yaml
	// Set to 1 to allow folding of comment blocks in YAML.
	const bool foldComment = styler.GetPropertyInt("fold.comment.yaml") != 0;

	// Backtrack to previous non-blank line so we can determine indent level
	// for any white space lines
	// and so we can fix any preceding fold level (which is why we go back
	// at least one line in all cases)
	int spaceFlags = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, nullptr);
	while (lineCurrent > 0) {
		lineCurrent--;
		indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, nullptr);
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
		Sci_Position lineNext = lineCurrent + 1;
		int indentNext = indentCurrent;
		if (lineNext <= docLines) {
			// Information about next line is only available if not at end of document
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, nullptr);
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
			indentNext = styler.IndentAmount(lineNext, &spaceFlags, nullptr);
		}

		const int levelAfterComments = indentNext & SC_FOLDLEVELNUMBERMASK;
		const int levelBeforeComments = Maximum(indentCurrentLevel,levelAfterComments);

		// Now set all the indent levels on the lines we skipped
		// Do this from end to start.  Once we encounter one line
		// which is indented more than the line after the end of
		// the comment-block, use the level of the block before

		Sci_Position skipLine = lineNext;
		int skipLevel = levelAfterComments;

		while (--skipLine > lineCurrent) {
			const int skipLineIndent = styler.IndentAmount(skipLine, &spaceFlags, nullptr);

			if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments)
				skipLevel = levelBeforeComments;

			const int whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;

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

}

LexerModule lmYAML(SCLEX_YAML, ColouriseYAMLDoc, "yaml", FoldYAMLDoc, yamlWordListDesc);
