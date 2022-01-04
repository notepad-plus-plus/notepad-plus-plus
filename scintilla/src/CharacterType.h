// Scintilla source code edit control
/** @file CharacterType.h
 ** Tests for character type and case-insensitive comparisons.
 **/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERTYPE_H
#define CHARACTERTYPE_H

namespace Scintilla::Internal {

// Functions for classifying characters

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
constexpr bool IsASpace(int ch) noexcept {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

constexpr bool IsSpaceOrTab(int ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

constexpr bool IsControl(int ch) noexcept {
	return ((ch >= 0) && (ch <= 0x1F)) || (ch == 0x7F);
}

constexpr bool IsEOLCharacter(int ch) noexcept {
	return ch == '\r' || ch == '\n';
}

constexpr bool IsBreakSpace(int ch) noexcept {
	// used for text breaking, treat C0 control character as space.
	// by default C0 control character is handled as special representation,
	// so not appears in normal text. 0x7F DEL is omitted to simplify the code.
	return ch >= 0 && ch <= ' ';
}

constexpr bool IsADigit(int ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

constexpr bool IsADigit(int ch, int base) noexcept {
	if (base <= 10) {
		return (ch >= '0') && (ch < '0' + base);
	} else {
		return ((ch >= '0') && (ch <= '9')) ||
		       ((ch >= 'A') && (ch < 'A' + base - 10)) ||
		       ((ch >= 'a') && (ch < 'a' + base - 10));
	}
}

constexpr bool IsASCII(int ch) noexcept {
	return (ch >= 0) && (ch < 0x80);
}

constexpr bool IsLowerCase(int ch) noexcept {
	return (ch >= 'a') && (ch <= 'z');
}

constexpr bool IsUpperCase(int ch) noexcept {
	return (ch >= 'A') && (ch <= 'Z');
}

constexpr bool IsUpperOrLowerCase(int ch) noexcept {
	return IsUpperCase(ch) || IsLowerCase(ch);
}

constexpr bool IsAlphaNumeric(int ch) noexcept {
	return
		((ch >= '0') && (ch <= '9')) ||
		((ch >= 'a') && (ch <= 'z')) ||
		((ch >= 'A') && (ch <= 'Z'));
}

constexpr bool IsPunctuation(int ch) noexcept {
	switch (ch) {
	case '!':
	case '"':
	case '#':
	case '$':
	case '%':
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
	case '?':
	case '@':
	case '[':
	case '\\':
	case ']':
	case '^':
	case '_':
	case '`':
	case '{':
	case '|':
	case '}':
	case '~':
		return true;
	default:
		return false;
	}
}

// Simple case functions for ASCII supersets.

template <typename T>
constexpr T MakeUpperCase(T ch) noexcept {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return ch - 'a' + 'A';
}

template <typename T>
constexpr T MakeLowerCase(T ch) noexcept {
	if (ch < 'A' || ch > 'Z')
		return ch;
	else
		return ch - 'A' + 'a';
}

int CompareCaseInsensitive(const char *a, const char *b) noexcept;
int CompareNCaseInsensitive(const char *a, const char *b, size_t len) noexcept;

}

#endif
