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
#include <map>
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
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Lexilla;

namespace {

const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	nullptr
};

const LexicalClass lexicalClasses[] = {
	// Lexer batch SCLEX_BATCH SCE_BAT_
	0, "SCE_BAT_DEFAULT", "default", "White space",
	1, "SCE_BAT_COMMENT", "comment", "Line comment",
	2, "SCE_BAT_WORD", "keyword", "Keyword",
	3, "SCE_BAT_LABEL", "label", "Label",
	4, "SCE_BAT_HIDE", "preprocessor", "Hide line @",
	5, "SCE_BAT_COMMAND", "identifier", "Command",
	6, "SCE_BAT_IDENTIFIER", "identifier", "Identifier",
	7, "SCE_BAT_OPERATOR", "operator", "Operator",
	8, "SCE_BAT_AFTER_LABEL", "comment","After label",
};

class LexerBatch : public DefaultLexer {
	WordList keywords;
	WordList keywords2;
	std::string wordLists;
public:
	explicit LexerBatch() :
		DefaultLexer("batch", SCLEX_BATCH, lexicalClasses, std::size(lexicalClasses)) {
		wordLists = JoinWordListDescriptions(batchWordListDesc);
	}
	LexerBatch(const LexerBatch &) = delete;
	LexerBatch(LexerBatch &&) = delete;
	LexerBatch &operator=(const LexerBatch &) = delete;
	LexerBatch &operator=(LexerBatch &&) = delete;
	~LexerBatch() override = default;

	const char *SCI_METHOD DescribeWordListSets() override {
		return wordLists.c_str();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, Scintilla::IDocument *pAccess) override;

	static ILexer5 *LexerFactoryBatch() {
		return new LexerBatch();
	}
};

constexpr bool Is0To9(char ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

constexpr bool IsAlphabetic(int ch) noexcept {
	return IsUpperOrLowerCase(ch);
}

bool AtEOL(LexAccessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
constexpr bool IsBOperator(char ch) noexcept {
	return AnyOf(ch, '=', '+', '>', '<', '|', '?', '*', '&', '(', ')');
}

// Tests for BATCH Separators
constexpr bool IsBSeparator(char ch) noexcept {
	return AnyOf(ch, '\\', '.', ';', '\"', '\'', '/');
}

// Both operators and separators -- words often ended with these characters.
constexpr bool IsBPunctuation(char ch) noexcept {
	return IsBOperator(ch) || IsBSeparator(ch);
}

// These characters end words.
constexpr bool IsBEndWord(char ch) noexcept {
	return IsBPunctuation(ch) || AnyOf(ch, '%', '!');
}

constexpr void SkipSpace(Sci_PositionU &i, std::string_view sv) noexcept {
	while (i < sv.length() && isspacechar(sv[i])) {
		i++;
	}
}

constexpr void SkipNonSpace(Sci_PositionU &i, std::string_view sv) noexcept {
	while (i < sv.length() && !isspacechar(sv[i])) {
		i++;
	}
}

constexpr Sci_PositionU WordLength(std::string_view word) {
	Sci_PositionU i = 0;
	while ((i < word.length()) && (!IsBEndWord(word[i]))) {
		i++;
	}
	return i;
}

// Tests for escape character
constexpr bool IsEscaped(std::string_view wordStr, Sci_PositionU pos) noexcept {
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

constexpr bool IsQuotedBy(std::string_view svBuffer, char quote) noexcept {
	bool CurrentStatus = false;
	size_t pQuote = svBuffer.find(quote);
	while (pQuote != std::string_view::npos) {
		if (!IsEscaped(svBuffer, pQuote)) {
			CurrentStatus = !CurrentStatus;
		}
		pQuote = svBuffer.find(quote, pQuote + 1);
	}
	return CurrentStatus;
}

// Tests for quote character
constexpr bool textQuoted(std::string_view lineBuffer, Sci_PositionU endPos) noexcept {
	const std::string_view svBuffer = lineBuffer.substr(0, endPos);
	return IsQuotedBy(svBuffer, '\"') || IsQuotedBy(svBuffer, '\'');
}

bool IsContinuation(const std::string &lineBuffer) noexcept {
	if ((lineBuffer.length() <= 1) || (lineBuffer == "\r\n")) // empty line on Unix and Mac or on Windows
		return false;
	Sci_PositionU lineContinuationPos = lineBuffer.length() - 2; // Unix or Mac EOL
	if ((lineBuffer.length() > 2) && lineBuffer[lineBuffer.length() - 2] == '\r') // Windows EOL
		lineContinuationPos = lineBuffer.length() - 3;
	// Reset continueProcessing if line continuation was not found
	if ((lineBuffer[lineContinuationPos] != '^')
		|| IsEscaped(lineBuffer, lineContinuationPos)
		|| textQuoted(lineBuffer, lineContinuationPos))
		return false;
	return true;
}

Sci_Position SCI_METHOD LexerBatch::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &keywords;
		break;
	case 1:
		wordListN = &keywords2;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

void LexerBatch::Lex(Sci_PositionU startPos, Sci_Position length, int, Scintilla::IDocument *pAccess) {
	LexAccessor styler(pAccess);

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

	std::string lineBuffer;
	std::string word;

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	bool isNotAssigned=false; // Used to flag Assignment in Set operation

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		// All testing is performed on a lower case version of the line since batch is case-insensitive
		lineBuffer.push_back(MakeLowerCase(styler[i]));
		if (AtEOL(styler, i) || (i==startPos + length-1)) {
			// End of line (or of line buffer) (or End of Last Line) met, colourise it
			const Sci_PositionU lengthLine = lineBuffer.length();
			const Sci_PositionU endPos=i;

			// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
			//   Toggling Regular Keyword Checking off improves readability
			// Other Regular Keywords and External Commands / Programs might also benefit from toggling
			//   Need a more robust algorithm to properly toggle Regular Keyword Checking
			bool stopLineProcessing=false;  // Used to stop line processing if Comment or Drive Change found

			Sci_PositionU offset = 0;	// Line Buffer Offset
			// Skip initial spaces
			SkipSpace(offset, lineBuffer);
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
					const size_t whitespaceEnd = lineBuffer.find_first_not_of("\t ", offset + 1);
					const size_t whitespaceLength = (whitespaceEnd == std::string::npos) ? 0 : whitespaceEnd;
					// Set of label-terminating characters determined experimentally
					const size_t labelEnd = lineBuffer.find_first_of("\t &+:<>|", whitespaceLength);
					if (labelEnd != std::string::npos) {
						styler.ColourTo(startLine + labelEnd - 1, SCE_BAT_LABEL);
						styler.ColourTo(endPos, SCE_BAT_AFTER_LABEL);
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
			SkipSpace(offset, lineBuffer);

			// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
			while (offset < lengthLine  && !stopLineProcessing) {
				if (offset > startLine) {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
				}
				// Copy word from Line Buffer
				const Sci_PositionU wordStart = offset;
				SkipNonSpace(offset, lineBuffer);
				const Sci_PositionU wbl = offset - wordStart;		// Word Buffer Length
				// Using assign to prevent new allocations
				word.assign(lineBuffer, wordStart, wbl);
				const char first = word.front();
				Sci_PositionU wbo = 0;		// Word Buffer Offset - also Special Keyword Buffer Length

				// Check for Comment - return if found
				if (continueProcessing) {
					if ((word == "rem") || (word.substr(0,2) == "::")) {
						if ((offset == wbl) || !textQuoted(lineBuffer, wordStart)) {
							styler.ColourTo(startLine + wordStart - 1, SCE_BAT_DEFAULT);
							styler.ColourTo(endPos, SCE_BAT_COMMENT);
							break;
						}
					}
				}
				// Check for Separator
				if (IsBSeparator(first)) {
					// Check for External Command / Program
					int style = SCE_BAT_DEFAULT;
					if ((cmdLoc == wordStart) &&
						(AnyOf(first, ':', '\\', '.'))) {
						// Reset Offset to re-process remainder of word
						offset = wordStart + 1;
						// Colorize External Command / Program
						if ((!keywords2) || (keywords2.InList(word))) {
							style = SCE_BAT_COMMAND;
						}
						// Reset External Command / Program Location
						cmdLoc = offset;
					} else {
						// Reset Offset to re-process remainder of word
						offset = wordStart + 1;
					}
					// Colorize Text
					styler.ColourTo(startLine + offset - 1, style);
				// Check for Regular Keyword in list
				} else if ((keywords.InList(word)) && (continueProcessing)) {
					// ECHO, GOTO, PROMPT and SET require no further Regular Keyword Checking
					if (InList(word, {"echo", "goto", "prompt"})) {
						continueProcessing = false;
					}
					// SET requires additional processing for the assignment operator
					if (word == "set") {
						continueProcessing = false;
						isNotAssigned=true;
					}
					// Identify External Command / Program Location for ERRORLEVEL, and EXIST
					if (InList(word, {"errorlevel", "exist"})) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						SkipSpace(cmdLoc, lineBuffer);
						// Skip comparison
						SkipNonSpace(cmdLoc, lineBuffer);
						// Skip next spaces
						SkipSpace(cmdLoc, lineBuffer);
					// Identify External Command / Program Location for CALL, DO, LOADHIGH and LH
					} else if (InList(word, {"call", "do", "loadhigh", "lh"})) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						SkipSpace(cmdLoc, lineBuffer);
						// Check if call is followed by a label
						if ((lineBuffer[cmdLoc] == ':') &&
							(word == "call")) {
							continueProcessing = false;
						}
					}
					// Colorize Regular keyword
					styler.ColourTo(startLine + offset - 1, SCE_BAT_WORD);
					// No need to Reset Offset
				// Check for Special Keyword in list, External Command / Program, or Default Text
				} else if (
					(!(IsBOperator(first) || AnyOf(first, '%', '!'))) &&
					(continueProcessing)) {
					// Check for Special Keyword
					//     Affected Commands are in Length range 2-6
					//     Good that ERRORLEVEL, EXIST, CALL, DO, LOADHIGH, and LH are unaffected
					bool sKeywordFound = false;		// Exit Special Keyword for-loop if found
					for (Sci_PositionU keywordLength = 2; keywordLength < wbl && keywordLength < 7 && !sKeywordFound; keywordLength++) {
						// Special Keywords are those that allow certain characters without whitespace after the command
						// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
						// Special Keyword used to determine if the first n characters is a Keyword
						wbo = keywordLength;
						const std::string sKeyword = word.substr(0, keywordLength);
						// Check for Special Keyword in list
						if ((keywords.InList(sKeyword)) &&
							((IsBPunctuation(word[wbo])) ||
							(word[wbo] == ':' && (InList(sKeyword, {"call", "echo", "goto"}) )))) {
							sKeywordFound = true;
							// ECHO requires no further Regular Keyword Checking
							if (sKeyword== "echo") {
								continueProcessing = false;
							}
							// Colorize Special Keyword as Regular Keyword
							styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_WORD);
							// Reset Offset to re-process remainder of word
							offset = wordStart + wbo;
						}
					}
					// Check for External Command / Program or Default Text
					if (!sKeywordFound) {
						int style = SCE_BAT_DEFAULT;
						// Read up to %, !, Operator or Separator
						const Sci_PositionU lengthWord = WordLength(word);
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							// Reset External Command / Program Location
							cmdLoc = wordStart + lengthWord;
							// CHOICE requires no further Regular Keyword Checking
							if (word == "choice") {
								continueProcessing = false;
							}
							// Check for START (and its switches) - What follows is External Command \ Program
							if (word == "start") {
								// Skip next spaces
								SkipSpace(cmdLoc, lineBuffer);
								// Reset External Command / Program Location if command switch detected
								if (lineBuffer[cmdLoc] == '/') {
									// Skip command switch
									SkipNonSpace(cmdLoc, lineBuffer);
									// Skip next spaces
									SkipSpace(cmdLoc, lineBuffer);
								}
							}
							// Colorize External Command / Program
							if ((!keywords2) || (keywords2.InList(word))) {
								style = SCE_BAT_COMMAND;
							}
						}
						styler.ColourTo(startLine + wordStart + lengthWord - 1, style);
						// Reset Offset to re-process remainder of word
						offset = wordStart + lengthWord;
					}
				// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
				} else if (first == '%') {
					// Colorize Default Text
					styler.ColourTo(startLine + wordStart - 1, SCE_BAT_DEFAULT);
					wbo++;
					// Search to end of word for second % (can be a long path)
					while ((wbo < wbl) && (word[wbo] != '%')) {
						wbo++;
					}
					// Check for Argument (%n) or (%*)
					if (((Is0To9(word[1])) || (word[1] == '*')) &&
						(word[wbo] != '%')) {
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							cmdLoc = wordStart + 2;
						}
						// Colorize Argument
						styler.ColourTo(startLine + wordStart + 1, SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset = wordStart + 2;
					// Check for Expanded Argument (%~...) / Variable (%%~...)
					// Expanded Argument: %~[<path-operators>]<single digit>
					// Expanded Variable: %%~[<path-operators>]<single identifier character>
					// Path operators are exclusively alphabetic.
					// Expanded arguments have a single digit at the end.
					// Expanded variables have a single identifier character as variable name.
					} else if (((wbl > 1) && (word[1] == '~')) ||
						((wbl > 2) && (word[1] == '%') && (word[2] == '~'))) {
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							cmdLoc = wordStart + wbo;
						}
						const bool isArgument = (word[1] == '~');
						if (isArgument) {
							Sci_PositionU expansionStopOffset = 2;
							bool isValid = false;
							for (; expansionStopOffset < wbl; expansionStopOffset++) {
								if (Is0To9(word[expansionStopOffset])) {
									expansionStopOffset++;
									isValid = true;
									wbo = expansionStopOffset;
									// Colorize Expanded Argument
									styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_IDENTIFIER);
									break;
								}
							}
							if (!isValid) {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_DEFAULT);
							}
						// Expanded Variable
						} else {
							// start after ~
							wbo = 3;
							// Search to end of word for another % (can be a long path)
							while ((wbo < wbl) && (!(IsBPunctuation(word[wbo]) || (word[wbo] == '%')))) {
								wbo++;
							}
							if (wbo > 3) {
								// Colorize Expanded Variable
								styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_IDENTIFIER);
							} else {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_DEFAULT);
							}
						}
						// Reset Offset to re-process remainder of word
						offset = wordStart + wbo;
					// Check for Environment Variable (%x...%)
					} else if ((word[1] != '%') && (word[wbo] == '%')) {
						wbo++;
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							cmdLoc = wordStart + wbo;
						}
						// Colorize Environment Variable
						styler.ColourTo(startLine + wordStart + wbo - 1, SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset = wordStart + wbo;
					// Check for Local Variable (%%a)
					} else if (
						(wbl > 2) &&
						(word[1] == '%') &&
						(word[2] != '%') &&
						(!IsBPunctuation(word[2]))) {
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							cmdLoc = wordStart + 3;
						}
						// Colorize Local Variable
						styler.ColourTo(startLine + wordStart + 2, SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset = wordStart + 3;
					// escaped %
					} else if (
						(wbl > 1) &&
						(word[1] == '%')) {

						// Reset Offset to re-process remainder of word
						styler.ColourTo(startLine + wordStart + 1, SCE_BAT_DEFAULT);
						offset = wordStart + 2;
					}
				// Check for Environment Variable (!x...!)
				} else if (first == '!') {
					// Colorize Default Text
					styler.ColourTo(startLine + wordStart - 1, SCE_BAT_DEFAULT);
					const size_t nextExclamation = word.find('!', 1);
					if (nextExclamation != std::string::npos) {
						// Check for External Command / Program
						if (cmdLoc == wordStart) {
							cmdLoc = wordStart + nextExclamation + 1;
						}
						// Colorize Environment Variable
						styler.ColourTo(startLine + wordStart + nextExclamation, SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset = wordStart + nextExclamation + 1;
					}
				// Check for Operator
				} else if (IsBOperator(first)) {
					// Colorize Default Text
					styler.ColourTo(startLine + wordStart - 1, SCE_BAT_DEFAULT);
					// Check for Comparison Operator
					if ((first == '=') && (word[1] == '=')) {
						// Identify External Command / Program Location for IF
						cmdLoc = offset;
						// Skip next spaces
						SkipSpace(cmdLoc, lineBuffer);
						// Colorize Comparison Operator
						if (continueProcessing)
							styler.ColourTo(startLine + wordStart + 1, SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + wordStart + 1, SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset = wordStart + 2;
					// Check for Pipe Operator
					} else if ((first == '|') &&
								!(IsEscaped(lineBuffer, wordStart + wbo) || textQuoted(lineBuffer, wordStart))) {
						// Reset External Command / Program Location
						cmdLoc = wordStart + 1;
						// Skip next spaces
						SkipSpace(cmdLoc, lineBuffer);
						// Colorize Pipe Operator
						styler.ColourTo(startLine + wordStart, SCE_BAT_OPERATOR);
						// Reset Offset to re-process remainder of word
						offset = wordStart + 1;
						continueProcessing = true;
					// Check for Other Operator
					} else {
						// Check for Operators: >, |, &
						if ((AnyOf(first, '>', ')', '(', '&')) &&
						   !(!continueProcessing && (IsEscaped(lineBuffer, wordStart + wbo)
						   || textQuoted(lineBuffer, wordStart)))){
							// Turn Keyword and External Command / Program checking back on
							continueProcessing = true;
							isNotAssigned=false;
						}
						// Colorize Other Operators
						// Do not Colorize Parenthesis, quoted text and escaped operators
						if ((!AnyOf(first, ')', '(')
						&& !textQuoted(lineBuffer, wordStart) && !IsEscaped(lineBuffer, wordStart + wbo))
						&& !((first == '=') && !isNotAssigned))
							styler.ColourTo(startLine + wordStart, SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + wordStart, SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset = wordStart + 1;

						if ((first == '=') && isNotAssigned){
							isNotAssigned=false;
						}
					}
				// Check for Default Text
				} else {
					// Read up to %, !, Operator or Separator
					const Sci_PositionU lengthWord = WordLength(word);
					// Colorize Default Text
					styler.ColourTo(startLine + wordStart + lengthWord - 1, SCE_BAT_DEFAULT);
					// Reset Offset to re-process remainder of word
					offset = wordStart + lengthWord;
				}
				// Skip next spaces - nothing happens if Offset was Reset
				SkipSpace(offset, lineBuffer);
			}
			// Colorize Default Text for remainder of line - currently not lexed
			styler.ColourTo(endPos, SCE_BAT_DEFAULT);

			// handle line continuation for SET and ECHO commands except the last line
			if (!continueProcessing && (i<startPos + length-1)) {
				// Reset continueProcessing if line continuation was not found
				continueProcessing = !IsContinuation(lineBuffer);
			}

			lineBuffer.clear();
			startLine = i + 1;
		}
	}
	styler.Flush();
}

}

extern const LexerModule lmBatch(SCLEX_LUA, LexerBatch::LexerFactoryBatch, "batch", batchWordListDesc);
