// Scintilla source code edit control
/** @file LexBatch.cxx
 ** Lexer for batch files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <initializer_list>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "InList.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

namespace {

constexpr bool Is0To9(char ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

bool IsAlphabetic(int ch) noexcept {
	return IsASCII(ch) && isalpha(ch);
}

inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
constexpr bool IsBOperator(char ch) noexcept {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') ||
		(ch == '|') || (ch == '?') || (ch == '*')||
		(ch == '&') || (ch == '(') || (ch == ')');
}

// Tests for BATCH Separators
constexpr bool IsBSeparator(char ch) noexcept {
	return (ch == '\\') || (ch == '.') || (ch == ';') ||
		(ch == '\"') || (ch == '\'') || (ch == '/');
}

// Tests for escape character
bool IsEscaped(const char* wordStr, Sci_PositionU pos) noexcept {
	bool isQoted=false;
	while (pos>0){
		pos--;
		if (wordStr[pos]=='^')
			isQoted=!isQoted;
		else
			break;
	}
	return isQoted;
}

// Tests for quote character
bool textQuoted(const char *lineBuffer, Sci_PositionU endPos) {
	char strBuffer[1024];
	strncpy(strBuffer, lineBuffer, endPos);
	strBuffer[endPos] = '\0';
	bool CurrentStatus = false;
	const char strQuotes[] = "\"'";
	const size_t strLength = strlen(strQuotes);
	for (size_t i = 0; i < strLength; i++) {
		const char *pQuote = strchr(strBuffer, strQuotes[i]);
		while (pQuote)
		{
			if (!IsEscaped(strBuffer, pQuote - strBuffer)) {
				CurrentStatus = !CurrentStatus;
			}
			pQuote = strchr(pQuote + 1, strQuotes[i]);
		}
		if (CurrentStatus) {
			break;
		}
	}
	return CurrentStatus;
}

void ColouriseBatchDoc(
    Sci_PositionU startPos,
    Sci_Position length,
    int /*initStyle*/,
    WordList *keywordlists[],
    Accessor &styler) {
	// Always backtracks to the start of a line that is not a continuation
	// of the previous line
	if (startPos > 0) {
		Sci_Position ln = styler.GetLine(startPos); // Current line number
		while (startPos > 0) {
			ln--;
			if ((styler.SafeGetCharAt(startPos-3) == '^' && styler.SafeGetCharAt(startPos-2) == '\r' && styler.SafeGetCharAt(startPos-1) == '\n')
			|| styler.SafeGetCharAt(startPos-2) == '^') {	// handle '^' line continuation
				// When the line continuation is found,
				// set the Start Position to the Start of the previous line
				length+=startPos-styler.LineStart(ln);
				startPos=styler.LineStart(ln);
			}
			else
				break;
		}
	}

	char lineBuffer[1024] {};

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	Sci_PositionU startLine = startPos;
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	bool isNotAssigned=false; // Used to flag Assignment in Set operation

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1) || (i==startPos + length-1)) {
			// End of line (or of line buffer) (or End of Last Line) met, colourise it
			lineBuffer[linePos] = '\0';
			const Sci_PositionU lengthLine=linePos;
			const Sci_PositionU endPos=i;
			const WordList &keywords = *keywordlists[0];      // Internal Commands
			const WordList &keywords2 = *keywordlists[1];     // External Commands (optional)

			// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
			//   Toggling Regular Keyword Checking off improves readability
			// Other Regular Keywords and External Commands / Programs might also benefit from toggling
			//   Need a more robust algorithm to properly toggle Regular Keyword Checking
			bool stopLineProcessing=false;  // Used to stop line processing if Comment or Drive Change found

			Sci_PositionU offset = 0;	// Line Buffer Offset
			// Skip initial spaces
			while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
				offset++;
			}
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
			// Set External Command / Program Location
			Sci_PositionU cmdLoc = offset;

			// Check for Fake Label (Comment) or Real Label - return if found
			if (lineBuffer[offset] == ':') {
				if (lineBuffer[offset + 1] == ':') {
					// Colorize Fake Label (Comment) - :: is similar to REM, see http://content.techweb.com/winmag/columns/explorer/2000/21.htm
					styler.ColourTo(endPos, SCE_BAT_COMMENT);
				} else {
					// Colorize Real Label
					// :[\t ]*[^\t &+:<>|]+
					const char *startLabelName = lineBuffer + offset + 1;
					const size_t whitespaceLength = strspn(startLabelName, "\t ");
					// Set of label-terminating characters determined experimentally
					const char *endLabel = strpbrk(startLabelName + whitespaceLength, "\t &+:<>|");
					if (endLabel) {
						styler.ColourTo(startLine + offset + endLabel - startLabelName, SCE_BAT_LABEL);
						styler.ColourTo(endPos, SCE_BAT_AFTER_LABEL);	// New style
					} else {
						styler.ColourTo(endPos, SCE_BAT_LABEL);
					}
				}
				stopLineProcessing=true;
			// Check for Drive Change (Drive Change is internal command) - return if found
			} else if ((IsAlphabetic(lineBuffer[offset])) &&
				(lineBuffer[offset + 1] == ':') &&
				((isspacechar(lineBuffer[offset + 2])) ||
				(((lineBuffer[offset + 2] == '\\')) &&
				(isspacechar(lineBuffer[offset + 3]))))) {
				// Colorize Regular Keyword
				styler.ColourTo(endPos, SCE_BAT_WORD);
				stopLineProcessing=true;
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
			while (offset < lengthLine  && !stopLineProcessing) {
				if (offset > startLine) {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
				}
				char wordBuffer[81]{};		// Word Buffer - large to catch long paths
				// Copy word from Line Buffer into Word Buffer and convert to lower case
				Sci_PositionU wbl = 0;		// Word Buffer Length
				for (; offset < lengthLine && wbl < 80 &&
						!isspacechar(lineBuffer[offset]); wbl++, offset++) {
					wordBuffer[wbl] = MakeLowerCase(lineBuffer[offset]);
				}
				wordBuffer[wbl] = '\0';
				const std::string_view wordView(wordBuffer);
				Sci_PositionU wbo = 0;		// Word Buffer Offset - also Special Keyword Buffer Length

				// Check for Comment - return if found
				if (continueProcessing) {
					if ((wordView == "rem") || (wordBuffer[0] == ':' && wordBuffer[1] == ':')) {
						if ((offset == wbl) || !textQuoted(lineBuffer, offset - wbl)) {
							styler.ColourTo(startLine + offset - wbl - 1, SCE_BAT_DEFAULT);
							styler.ColourTo(endPos, SCE_BAT_COMMENT);
							break;
						}
					}
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
					if (InList(wordView, {"echo", "goto", "prompt"})) {
						continueProcessing = false;
					}
					// SET requires additional processing for the assignment operator
					if (wordView == "set") {
						continueProcessing = false;
						isNotAssigned=true;
					}
					// Identify External Command / Program Location for ERRORLEVEL, and EXIST
					if (InList(wordView, {"errorlevel", "exist"})) {
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
					} else if (InList(wordView, {"call", "do", "loadhigh", "lh"})) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Check if call is followed by a label
						if ((lineBuffer[cmdLoc] == ':') &&
							(wordView == "call")) {
							continueProcessing = false;
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
					bool sKeywordFound = false;		// Exit Special Keyword for-loop if found
					for (Sci_PositionU keywordLength = 2; keywordLength < wbl && keywordLength < 7 && !sKeywordFound; keywordLength++) {
						// Special Keywords are those that allow certain characters without whitespace after the command
						// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
						// Special Keyword Buffer used to determine if the first n characters is a Keyword
						char sKeywordBuffer[10]{};	// Special Keyword Buffer
						wbo = 0;
						// Copy Keyword Length from Word Buffer into Special Keyword Buffer
						for (; wbo < keywordLength; wbo++) {
							sKeywordBuffer[wbo] = wordBuffer[wbo];
						}
						sKeywordBuffer[wbo] = '\0';
						// Check for Special Keyword in list
						if ((keywords.InList(sKeywordBuffer)) &&
							((IsBOperator(wordBuffer[wbo])) ||
							(IsBSeparator(wordBuffer[wbo])) ||
							(wordBuffer[wbo] == ':' &&
							(InList(sKeywordBuffer, {"call", "echo", "goto"}) )))) {
							sKeywordFound = true;
							// ECHO requires no further Regular Keyword Checking
							if (std::string_view(sKeywordBuffer) == "echo") {
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
								(((wordBuffer[wbo] != '%') &&
								(wordBuffer[wbo] != '!') &&
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))))) {
								wbo++;
							}
							// Reset External Command / Program Location
							cmdLoc = offset - (wbl - wbo);
							// Reset Offset to re-process remainder of word
							offset -= (wbl - wbo);
							// CHOICE requires no further Regular Keyword Checking
							if (wordView == "choice") {
								continueProcessing = false;
							}
							// Check for START (and its switches) - What follows is External Command \ Program
							if (wordView == "start") {
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
								(((wordBuffer[wbo] != '%') &&
								(wordBuffer[wbo] != '!') &&
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))))) {
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
						(wordBuffer[wbo] != '%')) {
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
					// Expanded Argument: %~[<path-operators>]<single digit>
					// Expanded Variable: %%~[<path-operators>]<single identifier character>
					// Path operators are exclusively alphabetic.
					// Expanded arguments have a single digit at the end.
					// Expanded variables have a single identifier character as variable name.
					} else if (((wbl > 1) && (wordBuffer[1] == '~')) ||
						((wbl > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] == '~'))) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - wbo);
						}
						const bool isArgument = (wordBuffer[1] == '~');
						if (isArgument) {
							Sci_PositionU expansionStopOffset = 2;
							bool isValid = false;
							for (; expansionStopOffset < wbl; expansionStopOffset++) {
								if (Is0To9(wordBuffer[expansionStopOffset])) {
									expansionStopOffset++;
									isValid = true;
									wbo = expansionStopOffset;
									// Colorize Expanded Argument
									styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
									break;
								}
							}
							if (!isValid) {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
							}
						// Expanded Variable
						} else {
							// start after ~
							wbo = 3;
							// Search to end of word for another % (can be a long path)
							while ((wbo < wbl) &&
								(wordBuffer[wbo] != '%') &&
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))) {
								wbo++;
							}
							if (wbo > 3) {
								// Colorize Expanded Variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
							} else {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
							}
						}
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
					// escaped %
					} else if (
						(wbl > 1) &&
						(wordBuffer[1] == '%')) {

						// Reset Offset to re-process remainder of word
						styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_DEFAULT);
						offset -= (wbl - 2);
					}
				// Check for Environment Variable (!x...!)
				} else if (wordBuffer[0] == '!') {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					wbo++;
					// Search to end of word for second ! (can be a long path)
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '!')) {
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
						if (continueProcessing)
							styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 2);
					// Check for Pipe Operator
					} else if ((wordBuffer[0] == '|') &&
								!(IsEscaped(lineBuffer,offset - wbl + wbo) || textQuoted(lineBuffer, offset - wbl) )) {
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
						continueProcessing = true;
					// Check for Other Operator
					} else {
						// Check for Operators: >, |, &
						if (((wordBuffer[0] == '>')||
						   (wordBuffer[0] == ')')||
						   (wordBuffer[0] == '(')||
						   (wordBuffer[0] == '&' )) &&
						   !(!continueProcessing && (IsEscaped(lineBuffer,offset - wbl + wbo)
						   || textQuoted(lineBuffer, offset - wbl) ))){
							// Turn Keyword and External Command / Program checking back on
							continueProcessing = true;
							isNotAssigned=false;
						}
						// Colorize Other Operators
						// Do not Colorize Parenthesis, quoted text and escaped operators
						if (((wordBuffer[0] != ')') && (wordBuffer[0] != '(')
						&& !textQuoted(lineBuffer, offset - wbl)  && !IsEscaped(lineBuffer,offset - wbl + wbo))
						&& !((wordBuffer[0] == '=') && !isNotAssigned ))
							styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);

						if ((wordBuffer[0] == '=') && isNotAssigned ){
							isNotAssigned=false;
						}
					}
				// Check for Default Text
				} else {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						((wordBuffer[wbo] != '%') &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo])))) {
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

			// handle line continuation for SET and ECHO commands except the last line
			if (!continueProcessing && (i<startPos + length-1)) {
				if (linePos==1 || (linePos==2 && lineBuffer[1]=='\r')) // empty line on Unix and Mac or on Windows
					continueProcessing=true;
				else {
					Sci_PositionU lineContinuationPos;
					if ((linePos>2) && lineBuffer[linePos-2]=='\r') // Windows EOL
						lineContinuationPos=linePos-3;
					else
						lineContinuationPos=linePos-2; // Unix or Mac EOL
					// Reset continueProcessing	if line continuation was not found
					if ((lineBuffer[lineContinuationPos]!='^')
							|| IsEscaped(lineBuffer, lineContinuationPos)
							|| textQuoted(lineBuffer, lineContinuationPos))
						continueProcessing=true;
				}
			}

			linePos = 0;
			startLine = i + 1;
		}
	}
}

const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	nullptr
};

}

LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", nullptr, batchWordListDesc);
