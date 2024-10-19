// Scintilla source code edit control
/** @file LexTroff.cxx
 ** Lexer for the Troff typesetting language.
 ** This should work for Groff, Heirloom Troff and Neatroff and consequently
 ** for man pages.
 **
 ** There are a number of restrictions:
 ** Escapes are not interpreted everywhere e.g. as part of command names.
 ** For the same reasons, changing control characters via `.cc` or `.c2` will
 ** not affect lexing - subsequent requests will not be styled correctly.
 ** Luckily this feature is rarely used.
 ** Line feeds cannot be escaped everywhere - this would require a state machine
 ** for all parsing.
 ** However, the C lexer has the same restriction.
 ** It is impossible to predict which macro argument is a numeric expression or where
 ** the number is actually treated as text.
 ** Also, escapes with levels of indirection (eg. `\\$1`) cannot currently
 ** be highlighted, as it is impossible to predict the context in which an
 ** expansion will be used.
 ** Indirect blocks (.ami, .ami1, .dei and .dei1) cannot be folded.
 ** No effort is done to highlight any of the preprocessors (tbl, pic, grap...).
 **/
// Copyright 2022-2024 by Robin Haberkorn <robin.haberkorn@googlemail.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <algorithm>

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

// Can be used as the line state to detect continuations
// across calls to ColouriseTroffDoc().
enum TroffState {
	DEFAULT = 0,
	// REQUEST/COMMAND on a line means that it is a command line with continuation.
	REQUEST,
	COMMAND,
	// Flow control request with continuation
	FLOW_CONTROL,
	// Inside "ignore input" block (usually .ig)
	IGNORE
};

static Sci_PositionU TroffEscapeBracketSize(Sci_PositionU startLine, Sci_PositionU endPos,
                                            Accessor &styler);
static Sci_PositionU TroffEscapeQuoteSize(Sci_PositionU startLine, Sci_PositionU endPos,
                                            Accessor &styler);

static Sci_PositionU ColouriseTroffEscape(Sci_PositionU startLine, Sci_PositionU endPos,
                                          Accessor &styler, WordList *keywordlists[],
                                          TroffState state);
static void ColouriseTroffLine(Sci_PositionU startLine, Sci_PositionU endPos,
                               Accessor &styler, WordList *keywordlists[],
                               TroffState state);

// For all escapes that take a one character (\*x), two character (\*(xy) or
// arbitrary number of characters (\*[...]).
static Sci_PositionU TroffEscapeBracketSize(Sci_PositionU startLine, Sci_PositionU endPos,
                                            Accessor &styler) {
	if (startLine > endPos) {
		return 0;
	}

	if (styler[startLine] == '(') {
		// two character name
		return std::min((Sci_PositionU)3, endPos-startLine+1);
	}

	if (styler[startLine] != '[') {
		// one character name
		return 1;
	}
	// name with arbitrary size between [...]

	Sci_PositionU i = startLine+1;

	while (i <= endPos) {
		if (styler[i] == ']') {
			return i-startLine+1;
		}

		if (styler[i] != '\\') {
			// not an escape
			i++;
			continue;
		}

		if (++i > endPos) {
			break;
		}

		// NOTE: Other escapes are not interpreted within \x[...].
		switch (styler[i]) {
		case '*': // string register
		case '$': // macro argument
		case 'V': // environment variable
		case 'n': // numeric register
			i += 1;
			i += TroffEscapeBracketSize(i, endPos, styler);
			break;
		case 'A': // Is there a string with this name?
		case 'B': // Is there a register with this name?
		case 'R': // register increase
			i += 1;
			i += TroffEscapeQuoteSize(i, endPos, styler);
			break;
		default:
			i += TroffEscapeBracketSize(i, endPos, styler);
		}
	}

	return endPos-startLine+1;
}

// For all escapes that take an argument in quotes, as in \v'...'.
// Actually, this works with any character as a delimiter.
static Sci_PositionU TroffEscapeQuoteSize(Sci_PositionU startLine, Sci_PositionU endPos,
                                          Accessor &styler) {
	if (startLine > endPos) {
		return 0;
	}

	char delim = styler[startLine];

	Sci_PositionU i = startLine+1;

	while (i <= endPos) {
		if (styler[i] == delim) {
			return i-startLine+1;
		}

		if (styler[i] != '\\') {
			// not an escape
			i++;
			continue;
		}

		if (++i > endPos) {
			break;
		}

		// NOTE: Other escapes are not interpreted within \x'...'.
		switch (styler[i]) {
		case '*': // string register
		case '$': // macro argument
		case 'V': // environment variable
		case 'n': // numeric register
			i += 1;
			i += TroffEscapeBracketSize(i, endPos, styler);
			break;
		case 'A': // Is there a string with this name?
		case 'B': // Is there a register with this name?
		case 'R': // register increase
			i += 1;
			i += TroffEscapeQuoteSize(i, endPos, styler);
			break;
		default:
			i += TroffEscapeBracketSize(i, endPos, styler);
		}
	}
	
	return endPos-startLine+1;
}

static Sci_PositionU ColouriseTroffEscape(Sci_PositionU startLine, Sci_PositionU endPos,
                                          Accessor &styler, WordList *keywordlists[],
                                          TroffState state) {
	Sci_PositionU i = startLine;

	Sci_Position curLine = styler.GetLine(startLine);

	if (styler[i] != '\\') {
		// not an escape - still consume one character
		return 1;
	}
	styler.ColourTo(i-1, SCE_TROFF_DEFAULT);
	i++;
	if (i > endPos) {
		return i-startLine;
	}

	switch (styler[i]) {
	case '"':
		// ordinary end of line comment
		styler.ColourTo(endPos, SCE_TROFF_COMMENT);
		return endPos-startLine+1;

	case '#':
		// \# comments will also "swallow" the EOL characters similar to
		// an escape at the end of line.
		// They are usually used only to comment entire lines, but we support
		// them after command lines anyway.
		styler.ColourTo(endPos, SCE_TROFF_COMMENT);
		// Next line will be a continuation
		styler.SetLineState(curLine, state);
		return endPos-startLine+1;

	case '*':
		// String register
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_STRING);
		break;
	case '$':
		// Macro parameter
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_MACRO);
		break;
	case 'V':
		// Environment variable
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_ENV);
		break;
	case 'f':
	case 'F':
		// Font change
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_FONT);
		break;
	case 'n':
		// Number register
		i += 1;
		if (styler[i] == '+' || styler[i] == '-') {
			i += 1;
		}
		if (i > endPos) {
			break;
		}
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_NUMBER);
		break;
	case 'g':
	case 'k':
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_NUMBER);
		break;
	case 'R':
		// Number register increase
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_NUMBER);
		break;
	case 'm':
	case 'M':
		// Colour change
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_COLOUR);
		break;
	case 'O':
		// Nested suppressions
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_SUPPRESSION);
		break;
	case 's':
		// Type size
		// This is special in supporting both a +/- prefix directly
		// after \s and both the [...] and '...' syntax.
		i += 1;
		if (i > endPos) {
			break;
		}
		if (styler[i] == '+' || styler[i] == '-') {
			i += 1;
		}
		if (styler[i] == '\'') {
			i += TroffEscapeQuoteSize(i, endPos, styler);
		} else if (styler[i] == '(') {
			// You can place a +/- even after the opening brace as in \s(+12.
			// Otherwise this could be handled by TroffEscapeBracketSize().
			i += 1;
			if (i > endPos) {
				break;
			}
			if (styler[i] == '+' || styler[i] == '-') {
				i += 1;
			}
			i += std::min((Sci_PositionU)2, endPos-i);
		} else {
			i += TroffEscapeBracketSize(i, endPos, styler);
		}
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_SIZE);
		break;
	case '{':
		// Opening code block
		i += 1;
		styler.ColourTo(i-1, SCE_TROFF_OPERATOR);
		// Groff actually supports commands immediately
		// following the opening brace as in `\{.tm`.
		// That's only why \{\ will actually treat the next line
		// as an ordinary command again.
		ColouriseTroffLine(i, endPos, styler, keywordlists, state);
		return endPos-startLine+1;
	case '}':
		// Closing code block
		// This exists only for the rare case of closing a
		// block in the middle of a line. 
		i += 1;
		styler.ColourTo(i-1, SCE_TROFF_OPERATOR);
		break;
	case '\n':
	case '\r':
		// Escape newline
		styler.ColourTo(endPos, SCE_TROFF_ESCAPE_GLYPH);
		i += 1;
		// Next line will be a continuation
		styler.SetLineState(curLine, state);
		break;
	case '!':
		// transparent line
		styler.ColourTo(endPos, SCE_TROFF_ESCAPE_TRANSPARENT);
		return endPos-startLine+1;
	case '?':
		// Transparent embed until \?
		i++;
		while (i <= endPos) {
			if (styler.Match(i, "\\?")) {
				i += 2;
				break;
			}
			i++;
		}
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_TRANSPARENT);
		break;
	case 'b':
		// pile of chars
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_GLYPH);
		break;
	case 'A':
	case 'B':
		// Valid identifier or register?
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_ISVALID);
		break;
	case 'C':
	case 'N':
		// additional glyphs
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_GLYPH);
		break;
	case 'D':
	case 'l':
	case 'L':
		// drawing commands
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_DRAW);
		break;
	case 'h':
	case 'v':
		// horizontal/vertical movement
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_MOVE);
		break;
	case 'H':
		// font height
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_HEIGHT);
		break;
	case 'o':
		// overstrike
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_OVERSTRIKE);
		break;
	case 'S':
		// slant
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_SLANT);
		break;
	case 'w':
		// width of string
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_WIDTH);
		break;
	case 'x':
		// increase vertical spacing
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_VSPACING);
		break;
	case 'X':
		// device control commands
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_DEVICE);
		break;
	case 'Y':
		// device control commands
		i += 1;
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_DEVICE);
		break;
	case 'Z':
		// format string without moving
		i += 1;
		i += TroffEscapeQuoteSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_NOMOVE);
		break;
	case 'z':
		// Zero-width format
		i += std::min((Sci_PositionU)2, endPos-i);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_NOMOVE);
		break;
	default:
		i += TroffEscapeBracketSize(i, endPos, styler);
		styler.ColourTo(i-1, SCE_TROFF_ESCAPE_GLYPH);
	}

	return i-startLine;
}

static Sci_PositionU ColouriseTroffNumber(Sci_PositionU startLine, Sci_PositionU endPos,
                                          Accessor &styler) {
	Sci_PositionU i = startLine;

	if (!isdigit(styler[i])) {
		// not a digit
		return 0;
	}

	styler.ColourTo(i-1, SCE_TROFF_DEFAULT);

	i++;
	while (i <= endPos && isdigit(styler[i])) {
		i++;
	}
	if (i > endPos) {
		goto done;
	}
	if (styler[i] == '.') {
		i++;
		while (i <= endPos && isdigit(styler[i])) {
			i++;
		}
		if (i > endPos) {
			goto done;
		}
	}

	// Unit is part of number
	if (strchr("ciPpmMnuvsf", styler[i])) {
		i++;
	}

done:
	styler.ColourTo(i-1, SCE_TROFF_NUMBER);
	return i-startLine;
}

static void ColouriseTroffLine(Sci_PositionU startLine, Sci_PositionU endPos,
                               Accessor &styler, WordList *keywordlists[],
                               TroffState state) {
	WordList &requests = *keywordlists[0];
	WordList &flowControlCond = *keywordlists[1];
	WordList &flowControlNocond = *keywordlists[2];
	WordList &ignoreBlocks = *keywordlists[3];
	Sci_PositionU i = startLine;

	if (startLine > endPos) {
		return;
	}

	Sci_Position curLine = styler.GetLine(startLine);
	// If the line state was not DEFAULT (i.e. the line had a continuation)
	// and the corresponding escape is erased, we must make sure that
	// the line state becomes DEFAULT again.
	styler.SetLineState(curLine, TroffState::DEFAULT);

	if (state == TroffState::IGNORE) {
		// Inside "ignore input" blocks
		if (styler[i] == '.') {
			i++;
			// Groff does not appear to allow spaces after the dot on
			// lines ending ignore blocks.
#if 0
			while (i <= endPos && isspacechar(styler[i])) {
				i++;
			}
#endif

			Sci_PositionU startCommand = i;
			while (i <= endPos && !isspacechar(styler[i])) {
				i++;
			}

			// Check whether this is the end of the block by backtracking.
			// The alternative would be to maintain a per-lexer data structure
			// of ignore blocks along with their "end-mac" parameters.
			if (i > startCommand) {
				std::string endmac = styler.GetRange(startCommand, i);
				if (endmac == ".") {
					endmac.clear();
				}
				// We styled right up to the "end-mac" argument or EOL.
				Sci_PositionU startEndmac = startLine;
				int style;
				while ((style = styler.BufferStyleAt(startEndmac-1)) != SCE_TROFF_REQUEST &&
				       style != SCE_TROFF_COMMAND) {
					startEndmac--;
				}
				Sci_PositionU endEndmac = startEndmac;
				while (!isspacechar(styler.SafeGetCharAt(endEndmac))) {
					endEndmac++;
				}

				std::string buf = endEndmac > startEndmac ?
							styler.GetRange(startEndmac, endEndmac) : "";
				if (buf == endmac) {
					// Found the end of the ignore block.
					// This line can be an ordinary request or macro,
					// so we completely restyle it.
					state = TroffState::DEFAULT;
					ColouriseTroffLine(startLine, endPos, styler, keywordlists, state);
					return;
				}
			}
		}

		styler.ColourTo(endPos, SCE_TROFF_IGNORE);
		styler.SetLineState(curLine, TroffState::IGNORE);
		return;
	}

	if (state == TroffState::FLOW_CONTROL) {
		// Allow leading spaces only in continuations of flow control requests.
		while (i <= endPos && isspacechar(styler[i])) {
			i++;
		}
	}

	// NOTE: The control characters can theoretically be changed via `.cc` and `.c2`.
	// This is seldom done and in principle it makes Roff unparseable since there is
	// no way to determine whether one of those commands has been executed without
	// executing the entire code which may run forever (halting problem).
	// So not supporting alternate control characters is simply a necessary restriction of this lexer.
	if ((state == TroffState::DEFAULT || state == TroffState::FLOW_CONTROL) &&
	    (styler[i] == '.' || styler[i] == '\'')) {
		// Control line
		// TODO: It may make sense to highlight the non-breaking control character (') in
		// a separate style.
		i++;

		while (i <= endPos && isspacechar(styler[i])) {
			i++;
		}
		Sci_PositionU startCommand = i;
		while (i <= endPos && !isspacechar(styler[i])) {
			i++;
			// FIXME: Highlight escapes in this position?
		}

		if (startCommand == i) {
			// lone dot without anything following
			styler.ColourTo(endPos, SCE_TROFF_REQUEST);
			return;
		}

		std::string buf = styler.GetRange(startCommand, i);

		if (buf == "\\\"") {
			// Handling .\" separately makes sure that the entire line including
			// the leading dot is highlighted as a comment
			styler.ColourTo(endPos, SCE_TROFF_COMMENT);
			return;
		}

		if (buf == "\\}") {
			// Handling .\} separately makes sure it is styled like
			// the opening \{ braces.
			styler.ColourTo(endPos, SCE_TROFF_OPERATOR);
			return;
		}

		// Ignore blocks
		if (ignoreBlocks.InList(buf)) {
			// It's important to style right up to the optional end-mac argument,
			// since we use the SCE_TROFF_REQUEST/COMMAND style to check for block ends (see above).
			while (i <= endPos && isspacechar(styler[i]) &&
			       styler[i] != '\n' && styler[i] != '\r') {
				i++;
			}
			styler.ColourTo(i-1, requests.InList(buf) ? SCE_TROFF_REQUEST : SCE_TROFF_COMMAND);
			styler.ColourTo(endPos, SCE_TROFF_DEFAULT);
			// Next line will be an ignore block
			styler.SetLineState(curLine, TroffState::IGNORE);
			return;
		}

		// Flow control without conditionals
		if (flowControlNocond.InList(buf)) {
			styler.ColourTo(i-1, requests.InList(buf) ? SCE_TROFF_REQUEST : SCE_TROFF_COMMAND);

			state = TroffState::FLOW_CONTROL;
			styler.ColourTo(i-1, SCE_TROFF_DEFAULT);
			ColouriseTroffLine(i, endPos, styler, keywordlists, state);
			return;
		}

		// It is tempting to treat flow control requests like ordinary requests,
		// but you cannot really predict the beginning of the next command.
		// Eg. .if 'FOO'.tm' .tm BAR
		// We therefore have to parse at least the apostrophe expressions
		// and keep track of escapes.
		if (flowControlCond.InList(buf)) {
			styler.ColourTo(i-1, requests.InList(buf) ? SCE_TROFF_REQUEST : SCE_TROFF_COMMAND);

			while (i <= endPos && isspacechar(styler[i])) {
				i++;
			}
			if (i > endPos) {
				return;
			}

			if (styler[i] == '!') {
				i++;
				styler.ColourTo(i-1, SCE_TROFF_OPERATOR);
			}
			if (i > endPos) {
				return;
			}

			if (styler[i] == '\'') {
				// string comparison: 's1's2'
				// FIXME: Should be highlighted?
				// However, none of the conditionals are currently highlighted.
				i++;
				while (i <= endPos && styler[i] != '\'') {
					i += ColouriseTroffEscape(i, endPos, styler, keywordlists, state);
				}
				if (i > endPos) {
					return;
				}
				i++;
				while (i <= endPos && styler[i] != '\'') {
					i += ColouriseTroffEscape(i, endPos, styler, keywordlists, state);
				}
				if (i > endPos) {
					return;
				}
				i++;
			} else {
				// Everything else - including numeric expressions -
				// can contain spaces only in escapes and inside balanced round braces.
				int braceLevel = 0;
				while (i <= endPos && (braceLevel > 0 || !isspacechar(styler[i]))) {
					Sci_PositionU numLen;

					switch (styler[i]) {
					case '(':
						braceLevel++;
						i++;
						break;
					case ')':
						braceLevel--;
						i++;
						break;
					default:
						numLen = ColouriseTroffNumber(i, endPos, styler);
						i += numLen;
						if (numLen > 0) {
							break;
						}

						i += ColouriseTroffEscape(i, endPos, styler, keywordlists, state);
					}
				}
			}

			state = TroffState::FLOW_CONTROL;
			styler.ColourTo(i-1, SCE_TROFF_DEFAULT);
			ColouriseTroffLine(i, endPos, styler, keywordlists, state);
			return;
		}

		// remaining non-flow-control requests and commands
		state = requests.InList(buf) ? TroffState::REQUEST : TroffState::COMMAND;
		styler.ColourTo(i-1, state == TroffState::REQUEST
					? SCE_TROFF_REQUEST : SCE_TROFF_COMMAND);
	}

	// Text line, request/command parameters including continuations
	while (i <= endPos) {
		if (state == TroffState::COMMAND && styler[i] == '"') {
			// Macro or misc command line arguments - assume that double-quoted strings
			// actually denote arguments.
			// FIXME: Allow escape linebreaks on the end of strings?
			// This would require another TroffState.
			styler.ColourTo(i-1, SCE_TROFF_DEFAULT);

			i++;
			while (i <= endPos && styler[i] != '"') {
				styler.ColourTo(i-1, SCE_TROFF_STRING);
				i += ColouriseTroffEscape(i, endPos, styler, keywordlists, state);
			}
			if (styler[i] == '"') {
				i++;
			}
			styler.ColourTo(i-1, SCE_TROFF_STRING);
			continue;
		}

		if (state == TroffState::COMMAND || state == TroffState::REQUEST) {
			// Numbers are supposed to be highlighted on every command line.
			// FIXME: Not all requests actually treat numbers specially.
			// Theoretically, we have to parse on a per-request basis.
			Sci_PositionU numLen;
			numLen = ColouriseTroffNumber(i, endPos, styler);
			i += numLen;
			if (numLen > 0) {
				continue;
			}
		}

		i += ColouriseTroffEscape(i, endPos, styler, keywordlists, state);
	}

	styler.ColourTo(endPos, SCE_TROFF_DEFAULT);
}

static void ColouriseTroffDoc(Sci_PositionU startPos, Sci_Position length, int,
                              WordList *keywordlists[], Accessor &styler) {
	styler.StartAt(startPos);
	Sci_PositionU startLine = startPos;
	Sci_Position curLine = styler.GetLine(startLine);

	styler.StartSegment(startLine);

	// NOTE: startPos will always be at the beginning of a line
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		// NOTE: The CR in CRLF counts into the current line.
		bool atEOL = (styler[i] == '\r' && styler.SafeGetCharAt(i+1) != '\n') || styler[i] == '\n';

		if (atEOL || i == startPos + length - 1) {
			// If the previous line had a continuation, the current line
			// is parsed like an argument to the command that had that continuation.
			TroffState state = curLine > 0 ? (TroffState)styler.GetLineState(curLine-1)
			                               : TroffState::DEFAULT;

			// End of line (or of line buffer) met, colourise it
			ColouriseTroffLine(startLine, i, styler, keywordlists, state);
			startLine = i + 1;
			curLine++;
		}
	}
}

static void FoldTroffDoc(Sci_PositionU startPos, Sci_Position length, int,
                         WordList *keywordlists[], Accessor &styler) {
	WordList &endmacBlocks = *keywordlists[4];

	Sci_PositionU endPos = startPos + length;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);
	std::string requestName;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || ch == '\n';

		if (style == SCE_TROFF_OPERATOR && ch == '\\') {
			if (chNext == '{') {
				levelCurrent++;
			} else if (chNext == '}') {
				levelCurrent--;
			}
		} else if (style != SCE_TROFF_IGNORE && styleNext == SCE_TROFF_IGNORE) {
			// Beginning of ignore block
			levelCurrent++;
		} else if (style == SCE_TROFF_IGNORE && styleNext != SCE_TROFF_IGNORE) {
			// End of ignore block
			// The end-mac line will not be folded, which is probably okay
			// since it could start the next ignore block or could be any command that is
			// executed.
			// This is not handled by the endmac block handling below
			// since the `.ig` syntax is different.
			// FIXME: Fold at least `..` lines.
			levelCurrent--;
		} else if ((style == SCE_TROFF_REQUEST || style == SCE_TROFF_COMMAND) &&
		           !isspacechar(ch)) {
			requestName.push_back(ch);
		}

		if (!atEOL) {
			continue;
		}

		int lev = levelPrev;

		if (requestName.length() > 1) {
			if (endmacBlocks.InList(requestName.substr(1))) {
				// beginning of block
				levelCurrent++;
			} else {
				// potential end of block
				// This parsing could be avoided if we kept a list
				// line numbers and end-mac strings.
				// The parsing here could also be simplified if
				// we highlighted the end-mac strings in ColouriseTroffLine().
				std::string endmac = requestName.substr(1);
				if (endmac == ".") {
					endmac.clear();
				}

				// find start of block
				Sci_Position startLine = lineCurrent;
				while (startLine > 0 && styler.LevelAt(startLine-1) >= levelCurrent) {
					startLine--;
				}
				if (startLine) {
					Sci_Position startEndmac = styler.LineStart(startLine);
					int reqStyle;
					// skip the request/command name
					while ((reqStyle = styler.StyleAt(startEndmac)) == SCE_TROFF_REQUEST ||
					       reqStyle == SCE_TROFF_COMMAND) {
						startEndmac++;
					}
					while (isspacechar(styler.SafeGetCharAt(startEndmac))) {
						startEndmac++;
					}
					// skip the macro name
					while (!isspacechar(styler.SafeGetCharAt(startEndmac))) {
						startEndmac++;
					}
					while (isspacechar(styler.SafeGetCharAt(startEndmac)) &&
					       styler[startEndmac] != '\n' && styler[startEndmac] != '\r') {
						startEndmac++;
					}
					Sci_Position endEndmac = startEndmac;
					while (!isspacechar(styler.SafeGetCharAt(endEndmac))) {
						endEndmac++;
					}
					std::string buf = endEndmac > startEndmac ?
								styler.GetRange(startEndmac, endEndmac) : "";
					if (buf == endmac) {
						// FIXME: Better fold the previous line unless it is `..`.
						levelCurrent--;
					}
				}
			}
		} else if (foldCompact && requestName == ".") {
			// lone dot ona line has no effect
			lev |= SC_FOLDLEVELWHITEFLAG;
		}
		if (levelCurrent > levelPrev) {
			lev |= SC_FOLDLEVELHEADERFLAG;
		}
		if (lev != styler.LevelAt(lineCurrent)) {
			styler.SetLevel(lineCurrent, lev);
		}
		lineCurrent++;
		levelPrev = levelCurrent;
		requestName.clear();
	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const troffWordLists[] = {
            "Predefined requests",
            "Flow control requests/commands with conditionals",
            "Flow control requests/commands without conditionals",
            "Requests and commands, initiating ignore blocks",
            "Requests and commands with end-macros",
            NULL,
        };

extern const LexerModule lmTroff(SCLEX_TROFF, ColouriseTroffDoc, "troff", FoldTroffDoc, troffWordLists);
