// Scintilla\ source code edit control
/** @file LexTCMD.cxx
 ** Lexer for Take Command / TCC batch scripts (.bat, .btm, .cmd).
 **/
// Written by Rex Conn (rconn [at] jpsoft [dot] com)
// based on the CMD lexer
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


static bool IsAlphabetic(int ch) {
	return IsASCII(ch) && isalpha(ch);
}

static inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') || ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
static bool IsBOperator(char ch) {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') || (ch == '|') || (ch == '&') || (ch == '!') || (ch == '?') || (ch == '*') || (ch == '(') || (ch == ')');
}

// Tests for BATCH Separators
static bool IsBSeparator(char ch) {
	return (ch == '\\') || (ch == '.') || (ch == ';') || (ch == ' ') || (ch == '\t') || (ch == '[') || (ch == ']') || (ch == '\"') || (ch == '\'') || (ch == '/');
}

// Find length of CMD FOR variable with modifier (%~...) or return 0
static unsigned int GetBatchVarLen( char *wordBuffer )
{
	int nLength = 0;
	if ( wordBuffer[0] == '%' ) {

		if ( wordBuffer[1] == '~' )
			nLength = 2;
		else if (( wordBuffer[1] == '%' ) && ( wordBuffer[2] == '~' ))
			nLength++;
		else
			return 0;

		for ( ; ( wordBuffer[nLength] ); nLength++ ) {

			switch ( toupper(wordBuffer[nLength]) ) {
			case 'A':
				// file attributes
			case 'D':
				// drive letter only
			case 'F':
				// fully qualified path name
			case 'N':
				// filename only
			case 'P':
				// path only
			case 'S':
				// short name
			case 'T':
				// date / time of file
			case 'X':
				// file extension only
			case 'Z':
				// file size
				break;
			default:
				return nLength;
			}
		}
	}

	return nLength;
}


static void ColouriseTCMDLine( char *lineBuffer, Sci_PositionU lengthLine, Sci_PositionU startLine, Sci_PositionU endPos, WordList *keywordlists[], Accessor &styler)
{
	Sci_PositionU offset = 0;	// Line Buffer Offset
	char wordBuffer[260];		// Word Buffer - large to catch long paths
	Sci_PositionU wbl;			// Word Buffer Length
	Sci_PositionU wbo;			// Word Buffer Offset - also Special Keyword Buffer Length
	WordList &keywords = *keywordlists[0];      // Internal Commands
//	WordList &keywords2 = *keywordlists[1];     // Aliases (optional)
	bool isDelayedExpansion = 1;				// !var!

	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	// Special Keywords are those that allow certain characters without whitespace after the command
	// Examples are: cd. cd\ echo: echo. path=
	bool inString = false; // Used for processing while ""
	// Special Keyword Buffer used to determine if the first n characters is a Keyword
	char sKeywordBuffer[260] = "";	// Special Keyword Buffer
	bool sKeywordFound;		// Exit Special Keyword for-loop if found

	// Skip leading whitespace
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}
	// Colorize Default Text
	styler.ColourTo(startLine + offset - 1, SCE_TCMD_DEFAULT);

	if ( offset >= lengthLine )
		return;

	// Check for Fake Label (Comment) or Real Label - return if found
	if (lineBuffer[offset] == ':') {
		if (lineBuffer[offset + 1] == ':') {
			// Colorize Fake Label (Comment) - :: is the same as REM
			styler.ColourTo(endPos, SCE_TCMD_COMMENT);
		} else {
			// Colorize Real Label
			styler.ColourTo(endPos, SCE_TCMD_LABEL);
		}
		return;

	// Check for Comment - return if found
	} else if (( CompareNCaseInsensitive(lineBuffer+offset, "rem", 3) == 0 ) && (( lineBuffer[offset+3] == 0 ) || ( isspace(lineBuffer[offset+3] )))) {
			styler.ColourTo(endPos, SCE_TCMD_COMMENT);
			return;

	// Check for Drive Change (Drive Change is internal command) - return if found
	} else if ((IsAlphabetic(lineBuffer[offset])) &&
		(lineBuffer[offset + 1] == ':') &&
		((isspacechar(lineBuffer[offset + 2])) ||
		(((lineBuffer[offset + 2] == '\\')) &&
		(isspacechar(lineBuffer[offset + 3]))))) {
		// Colorize Regular Keyword
		styler.ColourTo(endPos, SCE_TCMD_WORD);
		return;
	}

	// Check for Hide Command (@ECHO OFF/ON)
	if (lineBuffer[offset] == '@') {
		styler.ColourTo(startLine + offset, SCE_TCMD_HIDE);
		offset++;
	}
	// Skip whitespace
	while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
		offset++;
	}

	// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
	while (offset < lengthLine) {
		if (offset > startLine) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_TCMD_DEFAULT);
		}
		// Copy word from Line Buffer into Word Buffer
		wbl = 0;
		for (; offset < lengthLine && ( wbl < 260 ) && !isspacechar(lineBuffer[offset]); wbl++, offset++) {
			wordBuffer[wbl] = static_cast<char>(tolower(lineBuffer[offset]));
		}
		wordBuffer[wbl] = '\0';
		wbo = 0;

		// Check for Separator
		if (IsBSeparator(wordBuffer[0])) {

			// Reset Offset to re-process remainder of word
			offset -= (wbl - 1);
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);

			if (wordBuffer[0] == '"')
				inString = !inString;

		// Check for Regular expression
		} else if (( wordBuffer[0] == ':' ) && ( wordBuffer[1] == ':' ) && (continueProcessing)) {

			// Colorize Regular exoressuin
			styler.ColourTo(startLine + offset - 1, SCE_TCMD_DEFAULT);
			// No need to Reset Offset

		// Check for Labels in text (... :label)
		} else if (wordBuffer[0] == ':' && isspacechar(lineBuffer[offset - wbl - 1])) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_TCMD_DEFAULT);
			// Colorize Label
			styler.ColourTo(startLine + offset - 1, SCE_TCMD_CLABEL);
			// No need to Reset Offset
		// Check for delayed expansion Variable (!x...!)
		} else if (isDelayedExpansion && wordBuffer[0] == '!') {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_TCMD_DEFAULT);
			wbo++;
			// Search to end of word for second !
			while ((wbo < wbl) && (wordBuffer[wbo] != '!') && (!IsBOperator(wordBuffer[wbo])) && (!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			if (wordBuffer[wbo] == '!') {
				wbo++;
				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_TCMD_EXPANSION);
			} else {
				wbo = 1;
				// Colorize Symbol
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_TCMD_DEFAULT);
			}

			// Reset Offset to re-process remainder of word
			offset -= (wbl - wbo);

		// Check for Regular Keyword in list
		} else if ((keywords.InList(wordBuffer)) &&	(!inString) && (continueProcessing)) {

			// ECHO, PATH, and PROMPT require no further Regular Keyword Checking
			if ((CompareCaseInsensitive(wordBuffer, "echo") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echos") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echoerr") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echoserr") == 0) ||
			  (CompareCaseInsensitive(wordBuffer, "path") == 0) ||
			  (CompareCaseInsensitive(wordBuffer, "prompt") == 0)) {
				continueProcessing = false;
			}

			// Colorize Regular keyword
			styler.ColourTo(startLine + offset - 1, SCE_TCMD_WORD);
			// No need to Reset Offset

		} else if ((wordBuffer[0] != '%') && (wordBuffer[0] != '!') && (!IsBOperator(wordBuffer[0])) &&	(!inString) && (continueProcessing)) {

			// a few commands accept "illegal" syntax -- cd\, echo., etc.
			sscanf( wordBuffer, "%[^.<>|&=\\/]", sKeywordBuffer );
			sKeywordFound = false;

			if ((CompareCaseInsensitive(sKeywordBuffer, "echo") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echos") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echoerr") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "echoserr") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "cd") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "path") == 0) ||
			  (CompareCaseInsensitive(sKeywordBuffer, "prompt") == 0)) {

				// no further Regular Keyword Checking
				continueProcessing = false;
				sKeywordFound = true;
				wbo = (Sci_PositionU)strlen( sKeywordBuffer );

				// Colorize Special Keyword as Regular Keyword
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_TCMD_WORD);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			}

			// Check for Default Text
			if (!sKeywordFound) {
				wbo = 0;
				// Read up to %, Operator or Separator
				while ((wbo < wbl) && (wordBuffer[wbo] != '%') && (!isDelayedExpansion || wordBuffer[wbo] != '!') && (!IsBOperator(wordBuffer[wbo])) &&	(!IsBSeparator(wordBuffer[wbo]))) {
					wbo++;
				}
				// Colorize Default Text
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_TCMD_DEFAULT);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);
			}

		// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
		} else if (wordBuffer[0] == '%') {
			unsigned int varlen;
			unsigned int n = 1;
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_TCMD_DEFAULT);
			wbo++;

			// check for %[nn] syntax
			if ( wordBuffer[1] == '[' ) {
				n++;
				while ((n < wbl) && (wordBuffer[n] != ']')) {
					n++;
				}
				if ( wordBuffer[n] == ']' )
					n++;
				goto ColorizeArg;
			}

			// Search to end of word for second % or to the first terminator (can be a long path)
			while ((wbo < wbl) && (wordBuffer[wbo] != '%') && (!IsBOperator(wordBuffer[wbo])) && (!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}

			// Check for Argument (%n) or (%*)
			if (((isdigit(wordBuffer[1])) || (wordBuffer[1] == '*')) && (wordBuffer[wbo] != '%')) {
				while (( wordBuffer[n] ) && ( strchr( "%0123456789*#$", wordBuffer[n] ) != NULL ))
					n++;
ColorizeArg:
				// Colorize Argument
				styler.ColourTo(startLine + offset - 1 - (wbl - n), SCE_TCMD_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - n);

			// Check for Variable with modifiers (%~...)
			} else if ((varlen = GetBatchVarLen(wordBuffer)) != 0) {

				// Colorize Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - varlen), SCE_TCMD_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - varlen);

			// Check for Environment Variable (%x...%)
			} else if (( wordBuffer[1] ) && ( wordBuffer[1] != '%')) {
				if ( wordBuffer[wbo] == '%' )
					wbo++;

				// Colorize Environment Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_TCMD_ENVIRONMENT);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - wbo);

			// Check for Local Variable (%%a)
			} else if (	(wbl > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] != '%') && (!IsBOperator(wordBuffer[2])) && (!IsBSeparator(wordBuffer[2]))) {

				n = 2;
				while (( wordBuffer[n] ) && (!IsBOperator(wordBuffer[n])) && (!IsBSeparator(wordBuffer[n])))
					n++;

				// Colorize Local Variable
				styler.ColourTo(startLine + offset - 1 - (wbl - n), SCE_TCMD_IDENTIFIER);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - n);

			// Check for %%
			} else if ((wbl > 1) && (wordBuffer[1] == '%')) {

				// Colorize Symbols
				styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_TCMD_DEFAULT);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 2);
			} else {

				// Colorize Symbol
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_TCMD_DEFAULT);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			}

		// Check for Operator
		} else if (IsBOperator(wordBuffer[0])) {
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - wbl, SCE_TCMD_DEFAULT);

			// Check for Pipe, compound, or conditional Operator
			if ((wordBuffer[0] == '|') || (wordBuffer[0] == '&')) {

				// Colorize Pipe Operator
				styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_TCMD_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
				continueProcessing = true;

			// Check for Other Operator
			} else {
				// Check for > Operator
				if ((wordBuffer[0] == '>') || (wordBuffer[0] == '<')) {
					// Turn Keyword and External Command / Program checking back on
					continueProcessing = true;
				}
				// Colorize Other Operator
				if (!inString || !(wordBuffer[0] == '(' || wordBuffer[0] == ')'))
					styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_TCMD_OPERATOR);
				// Reset Offset to re-process remainder of word
				offset -= (wbl - 1);
			}

		// Check for Default Text
		} else {
			// Read up to %, Operator or Separator
			while ((wbo < wbl) && (wordBuffer[wbo] != '%') && (!isDelayedExpansion || wordBuffer[wbo] != '!') && (!IsBOperator(wordBuffer[wbo])) &&	(!IsBSeparator(wordBuffer[wbo]))) {
				wbo++;
			}
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_TCMD_DEFAULT);
			// Reset Offset to re-process remainder of word
			offset -= (wbl - wbo);
		}

		// Skip whitespace - nothing happens if Offset was Reset
		while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
			offset++;
		}
	}
	// Colorize Default Text for remainder of line - currently not lexed
	styler.ColourTo(endPos, SCE_TCMD_DEFAULT);
}

static void ColouriseTCMDDoc( Sci_PositionU startPos, Sci_Position length, int /*initStyle*/, WordList *keywordlists[], Accessor &styler )
{
	char lineBuffer[16384];

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	Sci_PositionU startLine = startPos;
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0';
			ColouriseTCMDLine(lineBuffer, linePos, startLine, i, keywordlists, styler);
			linePos = 0;
			startLine = i + 1;
		}
	}
	if (linePos > 0) {	// Last line does not have ending characters
		lineBuffer[linePos] = '\0';
		ColouriseTCMDLine(lineBuffer, linePos, startLine, startPos + length - 1, keywordlists, styler);
	}
}

// Convert string to upper case
static void StrUpr(char *s) {
	while (*s) {
		*s = MakeUpperCase(*s);
		s++;
	}
}

// Folding support (for DO, IFF, SWITCH, TEXT, and command groups)
static void FoldTCMDDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler)
{
	Sci_Position line = styler.GetLine(startPos);
	int level = styler.LevelAt(line);
	int levelIndent = 0;
	Sci_PositionU endPos = startPos + length;
	char s[16] = "";

    char chPrev = styler.SafeGetCharAt(startPos - 1);

	// Scan for ( and )
	for (Sci_PositionU i = startPos; i < endPos; i++) {

		int c = styler.SafeGetCharAt(i, '\n');
		int style = styler.StyleAt(i);
        bool bLineStart = ((chPrev == '\r') || (chPrev == '\n')) || i == 0;

		if (style == SCE_TCMD_OPERATOR) {
			// CheckFoldPoint
			if (c == '(') {
				levelIndent += 1;
			} else if (c == ')') {
				levelIndent -= 1;
			}
		}

        if (( bLineStart ) && ( style == SCE_TCMD_WORD )) {
            for (Sci_PositionU j = 0; j < 10; j++) {
                if (!iswordchar(styler[i + j])) {
                    break;
                }
                s[j] = styler[i + j];
                s[j + 1] = '\0';
            }

			StrUpr( s );
            if ((strcmp(s, "DO") == 0) || (strcmp(s, "IFF") == 0) || (strcmp(s, "SWITCH") == 0) || (strcmp(s, "TEXT") == 0)) {
                levelIndent++;
            } else if ((strcmp(s, "ENDDO") == 0) || (strcmp(s, "ENDIFF") == 0) || (strcmp(s, "ENDSWITCH") == 0) || (strcmp(s, "ENDTEXT") == 0)) {
                levelIndent--;
            }
        }

		if (c == '\n') { // line end
				if (levelIndent > 0) {
						level |= SC_FOLDLEVELHEADERFLAG;
				}
				if (level != styler.LevelAt(line))
						styler.SetLevel(line, level);
				level += levelIndent;
				if ((level & SC_FOLDLEVELNUMBERMASK) < SC_FOLDLEVELBASE)
						level = SC_FOLDLEVELBASE;
				line++;
				// reset state
				levelIndent = 0;
				level &= ~SC_FOLDLEVELHEADERFLAG;
				level &= ~SC_FOLDLEVELWHITEFLAG;
		}

		chPrev = c;
	}
}

static const char *const tcmdWordListDesc[] = {
	"Internal Commands",
	"Aliases",
	0
};

LexerModule lmTCMD(SCLEX_TCMD, ColouriseTCMDDoc, "tcmd", FoldTCMDDoc, tcmdWordListDesc);
