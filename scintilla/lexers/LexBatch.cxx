// Scintilla source code edit control
/** @file LexBatch.cxx
 ** Lexer for batch files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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

using namespace Scintilla;

static bool Is0To9(char ch) {
	return (ch >= '0') && (ch <= '9');
}

static bool IsAlphabetic(int ch) {
	return IsASCII(ch) && isalpha(ch);
}

static inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
static bool IsBOperator(char ch) {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') ||
		(ch == '|') || (ch == '?') || (ch == '*');
}

// Tests for BATCH Separators
static bool IsBSeparator(char ch) {
	return (ch == '\\') || (ch == '.') || (ch == ';') ||
		(ch == '\"') || (ch == '\'') || (ch == '/');
}

static void ColouriseBatchLine(
    char *lineBuffer,
    Sci_PositionU lengthLine,
    Sci_PositionU startLine,
    Sci_PositionU endPos,
    WordList *keywordlists[],
    Accessor &styler) {

	Sci_PositionU offset = 0;	// Line Buffer Offset
	Sci_PositionU cmdLoc;		// External Command / Program Location
	char wordBuffer[81];		// Word Buffer - large to catch long paths
	Sci_PositionU wbl;		// Word Buffer Length
	Sci_PositionU wbo;		// Word Buffer Offset - also Special Keyword Buffer Length
	WordList &keywords = *keywordlists[0];      // Internal Commands
	WordList &keywords2 = *keywordlists[1];     // External Commands (optional)

	// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
	//   Toggling Regular Keyword Checking off improves readability
	// Other Regular Keywords and External Commands / Programs might also benefit from toggling
	//   Need a more robust algorithm to properly toggle Regular Keyword Checking
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	// Special Keywords are those that allow certain characters without whitespace after the command
	// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
	// Special Keyword Buffer used to determine if the first n characters is a Keyword
	char sKeywordBuffer[10];	// Special Keyword Buffer
	bool sKeywordFound;		// Exit Special Keyword for-loop if found

	// Skip initial spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}
	// Colorize Default Text
	styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
	// Set External Command / Program Location
	cmdLoc = offset;

	// Check for Fake Label (Comment) or Real Label - return if found
	if (lineBuffer[offset] == ':') {
		if (lineBuffer[offset + 1] == ':') {
			// Colorize Fake Label (Comment) - :: is similar to REM, see http://content.techweb.com/winmag/columns/explorer/2000/21.htm
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
		} else {
			// Colorize Real Label
			styler.ColourTo(endPos, SCE_BAT_LABEL);
		}
		return;
	// Check for Drive Change (Drive Change is internal command) - return if found
	} else if ((IsAlphabetic(lineBuffer[offset])) &&
		(lineBuffer[offset + 1] == ':') &&
		((isspacechar(lineBuffer[offset + 2])) ||
		(((lineBuffer[offset + 2] == '\\')) &&
		(isspacechar(lineBuffer[offset + 3]))))) {
		// Colorize Regular Keyword
		styler.ColourTo(endPos, SCE_BAT_WORD);
		return;
	}

	// Check for Hide Command (@ECHO OFF/ON)
	if (lineBuffer[offset] == '@') {
		styler.ColourTo(startLine + offset, SCE_BAT_HIDE);
		offset++;
	}
	// Skip next spaces
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}

	// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
	while (offset < lengthLine) {
		if (offset > startLine) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
		}
		// Copy word from Line Buffer into Word Buffer
		wbl = 0;
		for (; offset < lengthLine && wbl < 80 &&
		        !isspacechar(lineBuffer[offset]); wbl++, offset++) {
			wordBuffer[wbl] = static_cast<char>(tolower(lineBuffer[offset]));
		}
		wordBuffer[wbl] = '\0';
		wbo = 0;

		// Check for Comment - return if found
		if (CompareCaseInsensitive(wordBuffer, "rem") == 0) {
			styler.ColourTo(endPos, SCE_BAT_COMMENT);
			return;
		}
		// Check for Separator
		if (IsBSeparator(wordBuffer[0])) {
			// Check for External Command / Program
			if ((cmdLoc == offset - wbl) &&
				((wordBuffer[0] == ':') ||
				(wordBuffer[0] == '\\') ||
				(wordBuffer[0] == '.'))) {
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
				// Colorize External Command / Program
				if (!keywords2) {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
				} else if (keywords2.InList(wordBuffer)) {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
				} else {
					styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
				}
				// Reset External Command / Program Location
				cmdLoc = offset;
			} else {
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
			}
		// Check for Regular Keyword in list
		} else if ((keywords.InList(wordBuffer)) &&
			(continueProcessing)) {
			// ECHO, GOTO, PROMPT and SET require no further Regular Keyword Checking
			if ((CompareCaseInsensitive(wordBuffer, "echo") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "goto") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "prompt") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "set") == 0)) {
				continueProcessing = false;
			}
			// Identify External Command / Program Location for ERRORLEVEL, and EXIST
			if ((CompareCaseInsensitive(wordBuffer, "errorlevel") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "exist") == 0)) {
				// Reset External Command / Program Location
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Skip comparison
				while ((cmdLoc < lengthLine) &&
					(!isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
			// Identify External Command / Program Location for CALL, DO, LOADHIGH and LH
			} else if ((CompareCaseInsensitive(wordBuffer, "call") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "do") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "loadhigh") == 0) ||
				(CompareCaseInsensitive(wordBuffer, "lh") == 0)) {
				// Reset External Command / Program Location
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
			}
			// Colorize Regular keyword
			styler.ColourTo(startLine + offset - 1, SCE_BAT_WORD);
			// No need to Reset Offset
		// Check for Special Keyword in list, External Command / Program, or Default Text
		} else if ((wordBuffer[0] != '%') &&
				   (wordBuffer[0] != '!') &&
			(!IsBOperator(wordBuffer[0])) &&
			(continueProcessing)) {
			// Check for Special Keyword
			//     Affected Commands are in Length range 2-6
			//     Good that ERRORLEVEL, EXIST, CALL, DO, LOADHIGH, and LH are unaffected
			sKeywordFound = false;
			for (Sci_PositionU keywordLength = 2; keywordLength < wbl && keywordLength < 7 && !sKeywordFound; keywordLength++) {
				wbo = 0;
				// Copy Keyword Length from Word Buffer into Special Keyword Buffer
				for (; wbo < keywordLength; wbo++) {
					sKeywordBuffer[wbo] = static_cast<char>(wordBuffer[wbo]);
				}
				sKeywordBuffer[wbo] = '\0';
				// Check for Special Keyword in list
				if ((keywords.InList(sKeywordBuffer)) &&
					((IsBOperator(wordBuffer[wbo])) ||
					(IsBSeparator(wordBuffer[wbo])))) {
					sKeywordFound = true;
					// ECHO requires no further Regular Keyword Checking
					if (CompareCaseInsensitive(sKeywordBuffer, "echo") == 0) {
						continueProcessing = false;
					}
					// Colorize Special Keyword as Regular Keyword
					styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_WORD);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
				}
			}
			// Check for External Command / Program or Default Text
			if (!sKeywordFound) {
				wbo = 0;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '%') &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo]))) {
						wbo++;
					}
					// Reset External Command / Program Location
					cmdLoc = offset - (wbl - wbo);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
					// CHOICE requires no further Regular Keyword Checking
					if (CompareCaseInsensitive(wordBuffer, "choice") == 0) {
						continueProcessing = false;
					}
					// Check for START (and its switches) - What follows is External Command \ Program
					if (CompareCaseInsensitive(wordBuffer, "start") == 0) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Reset External Command / Program Location if command switch detected
						if (lineBuffer[cmdLoc] == '/') {
							// Skip command switch
							while ((cmdLoc < lengthLine) &&
								(!isspacechar(lineBuffer[cmdLoc]))) {
								cmdLoc++;
							}
							// Skip next spaces
							while ((cmdLoc < lengthLine) &&
								(isspacechar(lineBuffer[cmdLoc]))) {
								cmdLoc++;
							}
						}
					}
					// Colorize External Command / Program
					if (!keywords2) {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
					} else if (keywords2.InList(wordBuffer)) {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
					} else {
						styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
					}
					// No need to Reset Offset
				// Check for Default Text
				} else {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '%') &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo]))) {
						wbo++;
					}
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
				}
			}
		// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
		} else if (wordBuffer[0] == '%') {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			wbo++;
			// Search to end of word for second % (can be a long path)
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '%') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			// Check for Argument (%n) or (%*)
			if (((Is0To9(wordBuffer[1])) || (wordBuffer[1] == '*')) &&
				(wordBuffer[wbo] != '%')) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - 2);
				}
				// Colorize Argument
				styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 2);
			// Check for Expanded Argument (%~...) / Variable (%%~...)
			} else if (((wbl > 1) && (wordBuffer[1] == '~')) ||
				((wbl > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] == '~'))) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Expanded Argument / Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			// Check for Environment Variable (%x...%)
			} else if ((wordBuffer[1] != '%') &&
				(wordBuffer[wbo] == '%')) {
				wbo++;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			// Check for Local Variable (%%a)
			} else if (
				(wbl > 2) &&
				(wordBuffer[1] == '%') &&
				(wordBuffer[2] != '%') &&
				(!IsBOperator(wordBuffer[2])) &&
				(!IsBSeparator(wordBuffer[2]))) {
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - 3);
				}
				// Colorize Local Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - 3), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 3);
			}
		// Check for Environment Variable (!x...!)
		} else if (wordBuffer[0] == '!') {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			wbo++;
			// Search to end of word for second ! (can be a long path)
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '!') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			if (wordBuffer[wbo] == '!') {
				wbo++;
				// Check for External Command / Program
				if (cmdLoc == offset - wbl) {
					cmdLoc = offset - (wbl - wbo);
				}
				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			}
		// Check for Operator
		} else if (IsBOperator(wordBuffer[0])) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
			// Check for Comparison Operator
			if ((wordBuffer[0] == '=') && (wordBuffer[1] == '=')) {
				// Identify External Command / Program Location for IF
				cmdLoc = offset;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Colorize Comparison Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 2);
			// Check for Pipe Operator
			} else if (wordBuffer[0] == '|') {
				// Reset External Command / Program Location
				cmdLoc = offset - wbl + 1;
				// Skip next spaces
				while ((cmdLoc < lengthLine) &&
					(isspacechar(lineBuffer[cmdLoc]))) {
					cmdLoc++;
				}
				// Colorize Pipe Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			// Check for Other Operator
			} else {
				// Check for > Operator
				if (wordBuffer[0] == '>') {
					// Turn Keyword and External Command / Program checking back on
					continueProcessing = true;
				}
				// Colorize Other Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			}
		// Check for Default Text
		} else {
			// Read up to %, Operator or Separator
			while ((wbo < wbl) &&
				(wordBuffer[wbo] != '%') &&
				(wordBuffer[wbo] != '!') &&
				(!IsBOperator(wordBuffer[wbo])) &&
				(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
			// Reset Offset to re-process remainder of word
			offset -= (wbl - wbo);
		}
		// Skip next spaces - nothing happens if Offset was Reset
		while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
			offset++;
		}
	}
	// Colorize Default Text for remainder of line - currently not lexed
	styler.ColourTo(endPos, SCE_BAT_DEFAULT);
}

static void ColouriseBatchDoc(
    Sci_PositionU startPos,
    Sci_Position length,
    int /*initStyle*/,
    WordList *keywordlists[],
    Accessor &styler) {

	char lineBuffer[1024];

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	Sci_PositionU startLine = startPos;
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseBatchLine(lineBuffer, linePos, startLine, i, keywordlists, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		lineBuffer[linePos] = '\0';
		ColouriseBatchLine(lineBuffer, linePos, startLine, startPos + length - 1,
		                   keywordlists, styler);
	}
}

static const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	0
};

LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", 0, batchWordListDesc);
