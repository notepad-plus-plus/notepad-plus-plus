// Scintilla source code edit control
/** @file LexAda.cxx
 ** Lexer for Ada 95
 **/
// Copyright 2002 by Sergey Koshcheyev <sergey.k@seznam.cz>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>

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

/*
 * Interface
 */

static void ColouriseDocument(
    Sci_PositionU startPos,
    Sci_Position length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler);

static const char * const adaWordListDesc[] = {
	"Keywords",
	0
};

LexerModule lmAda(SCLEX_ADA, ColouriseDocument, "ada", NULL, adaWordListDesc);

/*
 * Implementation
 */

// Functions that have apostropheStartsAttribute as a parameter set it according to whether
// an apostrophe encountered after processing the current token will start an attribute or
// a character literal.
static void ColouriseCharacter(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseComment(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseContext(StyleContext& sc, char chEnd, int stateEOL);
static void ColouriseDelimiter(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseLabel(StyleContext& sc, WordList& keywords, bool& apostropheStartsAttribute);
static void ColouriseNumber(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseString(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseWhiteSpace(StyleContext& sc, bool& apostropheStartsAttribute);
static void ColouriseWord(StyleContext& sc, WordList& keywords, bool& apostropheStartsAttribute);

static inline bool IsDelimiterCharacter(int ch);
static inline bool IsSeparatorOrDelimiterCharacter(int ch);
static bool IsValidIdentifier(const std::string& identifier);
static bool IsValidNumber(const std::string& number);
static inline bool IsWordStartCharacter(int ch);
static inline bool IsWordCharacter(int ch);

static void ColouriseCharacter(StyleContext& sc, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = true;

	sc.SetState(SCE_ADA_CHARACTER);

	// Skip the apostrophe and one more character (so that '' is shown as non-terminated and '''
	// is handled correctly)
	sc.Forward();
	sc.Forward();

	ColouriseContext(sc, '\'', SCE_ADA_CHARACTEREOL);
}

static void ColouriseContext(StyleContext& sc, char chEnd, int stateEOL) {
	while (!sc.atLineEnd && !sc.Match(chEnd)) {
		sc.Forward();
	}

	if (!sc.atLineEnd) {
		sc.ForwardSetState(SCE_ADA_DEFAULT);
	} else {
		sc.ChangeState(stateEOL);
	}
}

static void ColouriseComment(StyleContext& sc, bool& /*apostropheStartsAttribute*/) {
	// Apostrophe meaning is not changed, but the parameter is present for uniformity

	sc.SetState(SCE_ADA_COMMENTLINE);

	while (!sc.atLineEnd) {
		sc.Forward();
	}
}

static void ColouriseDelimiter(StyleContext& sc, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = sc.Match (')');
	sc.SetState(SCE_ADA_DELIMITER);
	sc.ForwardSetState(SCE_ADA_DEFAULT);
}

static void ColouriseLabel(StyleContext& sc, WordList& keywords, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = false;

	sc.SetState(SCE_ADA_LABEL);

	// Skip "<<"
	sc.Forward();
	sc.Forward();

	std::string identifier;

	while (!sc.atLineEnd && !IsSeparatorOrDelimiterCharacter(sc.ch)) {
		identifier += static_cast<char>(tolower(sc.ch));
		sc.Forward();
	}

	// Skip ">>"
	if (sc.Match('>', '>')) {
		sc.Forward();
		sc.Forward();
	} else {
		sc.ChangeState(SCE_ADA_ILLEGAL);
	}

	// If the name is an invalid identifier or a keyword, then make it invalid label
	if (!IsValidIdentifier(identifier) || keywords.InList(identifier.c_str())) {
		sc.ChangeState(SCE_ADA_ILLEGAL);
	}

	sc.SetState(SCE_ADA_DEFAULT);

}

static void ColouriseNumber(StyleContext& sc, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = true;

	std::string number;
	sc.SetState(SCE_ADA_NUMBER);

	// Get all characters up to a delimiter or a separator, including points, but excluding
	// double points (ranges).
	while (!IsSeparatorOrDelimiterCharacter(sc.ch) || (sc.ch == '.' && sc.chNext != '.')) {
		number += static_cast<char>(sc.ch);
		sc.Forward();
	}

	// Special case: exponent with sign
	if ((sc.chPrev == 'e' || sc.chPrev == 'E') &&
	        (sc.ch == '+' || sc.ch == '-')) {
		number += static_cast<char>(sc.ch);
		sc.Forward ();

		while (!IsSeparatorOrDelimiterCharacter(sc.ch)) {
			number += static_cast<char>(sc.ch);
			sc.Forward();
		}
	}

	if (!IsValidNumber(number)) {
		sc.ChangeState(SCE_ADA_ILLEGAL);
	}

	sc.SetState(SCE_ADA_DEFAULT);
}

static void ColouriseString(StyleContext& sc, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = true;

	sc.SetState(SCE_ADA_STRING);
	sc.Forward();

	ColouriseContext(sc, '"', SCE_ADA_STRINGEOL);
}

static void ColouriseWhiteSpace(StyleContext& sc, bool& /*apostropheStartsAttribute*/) {
	// Apostrophe meaning is not changed, but the parameter is present for uniformity
	sc.SetState(SCE_ADA_DEFAULT);
	sc.ForwardSetState(SCE_ADA_DEFAULT);
}

static void ColouriseWord(StyleContext& sc, WordList& keywords, bool& apostropheStartsAttribute) {
	apostropheStartsAttribute = true;
	sc.SetState(SCE_ADA_IDENTIFIER);

	std::string word;

	while (!sc.atLineEnd && !IsSeparatorOrDelimiterCharacter(sc.ch)) {
		word += static_cast<char>(tolower(sc.ch));
		sc.Forward();
	}

	if (!IsValidIdentifier(word)) {
		sc.ChangeState(SCE_ADA_ILLEGAL);

	} else if (keywords.InList(word.c_str())) {
		sc.ChangeState(SCE_ADA_WORD);

		if (word != "all") {
			apostropheStartsAttribute = false;
		}
	}

	sc.SetState(SCE_ADA_DEFAULT);
}

//
// ColouriseDocument
//

static void ColouriseDocument(
    Sci_PositionU startPos,
    Sci_Position length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler) {
	WordList &keywords = *keywordlists[0];

	StyleContext sc(startPos, length, initStyle, styler);

	Sci_Position lineCurrent = styler.GetLine(startPos);
	bool apostropheStartsAttribute = (styler.GetLineState(lineCurrent) & 1) != 0;

	while (sc.More()) {
		if (sc.atLineEnd) {
			// Go to the next line
			sc.Forward();
			lineCurrent++;

			// Remember the line state for future incremental lexing
			styler.SetLineState(lineCurrent, apostropheStartsAttribute);

			// Don't continue any styles on the next line
			sc.SetState(SCE_ADA_DEFAULT);
		}

		// Comments
		if (sc.Match('-', '-')) {
			ColouriseComment(sc, apostropheStartsAttribute);

		// Strings
		} else if (sc.Match('"')) {
			ColouriseString(sc, apostropheStartsAttribute);

		// Characters
		} else if (sc.Match('\'') && !apostropheStartsAttribute) {
			ColouriseCharacter(sc, apostropheStartsAttribute);

		// Labels
		} else if (sc.Match('<', '<')) {
			ColouriseLabel(sc, keywords, apostropheStartsAttribute);

		// Whitespace
		} else if (IsASpace(sc.ch)) {
			ColouriseWhiteSpace(sc, apostropheStartsAttribute);

		// Delimiters
		} else if (IsDelimiterCharacter(sc.ch)) {
			ColouriseDelimiter(sc, apostropheStartsAttribute);

		// Numbers
		} else if (IsADigit(sc.ch) || sc.ch == '#') {
			ColouriseNumber(sc, apostropheStartsAttribute);

		// Keywords or identifiers
		} else {
			ColouriseWord(sc, keywords, apostropheStartsAttribute);
		}
	}

	sc.Complete();
}

static inline bool IsDelimiterCharacter(int ch) {
	switch (ch) {
	case '&':
	case '\'':
	case '(':
	case ')':
	case '*':
	case '+':
	case ',':
	case '-':
	case '.':
	case '/':
	case ':':
	case ';':
	case '<':
	case '=':
	case '>':
	case '|':
		return true;
	default:
		return false;
	}
}

static inline bool IsSeparatorOrDelimiterCharacter(int ch) {
	return IsASpace(ch) || IsDelimiterCharacter(ch);
}

static bool IsValidIdentifier(const std::string& identifier) {
	// First character can't be '_', so initialize the flag to true
	bool lastWasUnderscore = true;

	size_t length = identifier.length();

	// Zero-length identifiers are not valid (these can occur inside labels)
	if (length == 0) {
		return false;
	}

	// Check for valid character at the start
	if (!IsWordStartCharacter(identifier[0])) {
		return false;
	}

	// Check for only valid characters and no double underscores
	for (size_t i = 0; i < length; i++) {
		if (!IsWordCharacter(identifier[i]) ||
		        (identifier[i] == '_' && lastWasUnderscore)) {
			return false;
		}
		lastWasUnderscore = identifier[i] == '_';
	}

	// Check for underscore at the end
	if (lastWasUnderscore == true) {
		return false;
	}

	// All checks passed
	return true;
}

static bool IsValidNumber(const std::string& number) {
	size_t hashPos = number.find("#");
	bool seenDot = false;

	size_t i = 0;
	size_t length = number.length();

	if (length == 0)
		return false; // Just in case

	// Decimal number
	if (hashPos == std::string::npos) {
		bool canBeSpecial = false;

		for (; i < length; i++) {
			if (number[i] == '_') {
				if (!canBeSpecial) {
					return false;
				}
				canBeSpecial = false;
			} else if (number[i] == '.') {
				if (!canBeSpecial || seenDot) {
					return false;
				}
				canBeSpecial = false;
				seenDot = true;
			} else if (IsADigit(number[i])) {
				canBeSpecial = true;
			} else {
				break;
			}
		}

		if (!canBeSpecial)
			return false;
	} else {
		// Based number
		bool canBeSpecial = false;
		int base = 0;

		// Parse base
		for (; i < length; i++) {
			int ch = number[i];
			if (ch == '_') {
				if (!canBeSpecial)
					return false;
				canBeSpecial = false;
			} else if (IsADigit(ch)) {
				base = base * 10 + (ch - '0');
				if (base > 16)
					return false;
				canBeSpecial = true;
			} else if (ch == '#' && canBeSpecial) {
				break;
			} else {
				return false;
			}
		}

		if (base < 2)
			return false;
		if (i == length)
			return false;

		i++; // Skip over '#'

		// Parse number
		canBeSpecial = false;

		for (; i < length; i++) {
			int ch = tolower(number[i]);

			if (ch == '_') {
				if (!canBeSpecial) {
					return false;
				}
				canBeSpecial = false;

			} else if (ch == '.') {
				if (!canBeSpecial || seenDot) {
					return false;
				}
				canBeSpecial = false;
				seenDot = true;

			} else if (IsADigit(ch)) {
				if (ch - '0' >= base) {
					return false;
				}
				canBeSpecial = true;

			} else if (ch >= 'a' && ch <= 'f') {
				if (ch - 'a' + 10 >= base) {
					return false;
				}
				canBeSpecial = true;

			} else if (ch == '#' && canBeSpecial) {
				break;

			} else {
				return false;
			}
		}

		if (i == length) {
			return false;
		}

		i++;
	}

	// Exponent (optional)
	if (i < length) {
		if (number[i] != 'e' && number[i] != 'E')
			return false;

		i++; // Move past 'E'

		if (i == length) {
			return false;
		}

		if (number[i] == '+')
			i++;
		else if (number[i] == '-') {
			if (seenDot) {
				i++;
			} else {
				return false; // Integer literals should not have negative exponents
			}
		}

		if (i == length) {
			return false;
		}

		bool canBeSpecial = false;

		for (; i < length; i++) {
			if (number[i] == '_') {
				if (!canBeSpecial) {
					return false;
				}
				canBeSpecial = false;
			} else if (IsADigit(number[i])) {
				canBeSpecial = true;
			} else {
				return false;
			}
		}

		if (!canBeSpecial)
			return false;
	}

	// if i == length, number was parsed successfully.
	return i == length;
}

static inline bool IsWordCharacter(int ch) {
	return IsWordStartCharacter(ch) || IsADigit(ch);
}

static inline bool IsWordStartCharacter(int ch) {
	return (IsASCII(ch) && isalpha(ch)) || ch == '_';
}
