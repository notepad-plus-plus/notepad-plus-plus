// Scintilla source code edit control
/** @file CharacterSet.h
 ** Encapsulates a set of characters. Used to test if a character is within a set.
 **/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERSET_H
#define CHARACTERSET_H

namespace Lexilla {

template<int N>
class CharacterSetArray {
	static constexpr int bitsPerChar = 8;
	static constexpr int mask3Bits = 7;
	unsigned char bset[((N-1)/bitsPerChar) + 1] = {};
	bool valueAfter = false;
public:
	enum setBase {
		setNone=0,
		setLower=1,
		setUpper=2,
		setDigits=4,
		setAlpha=setLower|setUpper,
		setAlphaNum=setAlpha|setDigits
	};
	explicit CharacterSetArray(setBase base=setNone, const char *initialSet="", bool valueAfter_=false) noexcept {
		valueAfter = valueAfter_;
		AddString(initialSet);
		if (base & setLower)
			AddString("abcdefghijklmnopqrstuvwxyz");
		if (base & setUpper)
			AddString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		if (base & setDigits)
			AddString("0123456789");
	}
	explicit CharacterSetArray(const char *initialSet, bool valueAfter_=false) noexcept :
		CharacterSetArray(setNone, initialSet, valueAfter_) {
	}
	// For compatibility with previous version but should not be used in new code.
	CharacterSetArray(setBase base, const char *initialSet, [[maybe_unused]]int size_, bool valueAfter_=false) noexcept :
		CharacterSetArray(base, initialSet, valueAfter_) {
		assert(size_ == N);
	}
	void Add(int val) noexcept {
		assert(val >= 0);
		assert(val < N);
		bset[val >> 3] |= 1 << (val & mask3Bits);
	}
	void AddString(const char *setToAdd) noexcept {
		for (const char *cp=setToAdd; *cp; cp++) {
			const unsigned char uch = *cp;
			assert(uch < N);
			Add(uch);
		}
	}
	[[nodiscard]] bool Contains(int val) const noexcept {
		assert(val >= 0);
		if (val < 0) return false;
		if (val >= N) return valueAfter;
		return bset[val >> 3] & (1 << (val & mask3Bits));
	}
	[[nodiscard]] bool Contains(char ch) const noexcept {
		// Overload char as char may be signed
		const unsigned char uch = ch;
		return Contains(uch);
	}
};

constexpr int countASCII = 0x80;
using CharacterSet = CharacterSetArray<countASCII>;

// Functions for classifying characters

template <typename T, typename... Args>
constexpr bool AnyOf(T t, Args... args) noexcept {
#if defined(__clang__)
	static_assert(__is_integral(T) || __is_enum(T));
#endif
	return ((t == args) || ...);
}

// prevent pointer without <type_traits>
template <typename T, typename... Args>
constexpr void AnyOf([[maybe_unused]] T *t, [[maybe_unused]] Args... args) noexcept {}
template <typename T, typename... Args>
constexpr void AnyOf([[maybe_unused]] const T *t, [[maybe_unused]] Args... args) noexcept {}

constexpr int charTab = 0x09;
constexpr int charCarriageReturn = 0x0D;

constexpr bool IsASpace(int ch) noexcept {
    return (ch == ' ') || ((ch >= charTab) && (ch <= charCarriageReturn));
}

constexpr bool IsASpaceOrTab(int ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

constexpr bool IsADigit(int ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

constexpr bool IsAHeXDigit(int ch) noexcept {
	return (ch >= '0' && ch <= '9')
		|| (ch >= 'A' && ch <= 'F')
		|| (ch >= 'a' && ch <= 'f');
}

constexpr bool IsAnOctalDigit(int ch) noexcept {
	return ch >= '0' && ch <= '7';
}

constexpr bool IsADigit(int ch, int base) noexcept {
	constexpr int digits = 10;
	if (base <= digits) {
		return (ch >= '0') && (ch < '0' + base);
	}
	return ((ch >= '0') && (ch <= '9')) ||
		    ((ch >= 'A') && (ch < 'A' + base - digits)) ||
		    ((ch >= 'a') && (ch < 'a' + base - digits));
}

constexpr bool IsASCII(int ch) noexcept {
	constexpr int lastASCII = 0x7F;
	return (ch >= 0) && (ch <= lastASCII);
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

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
constexpr bool isspacechar(int ch) noexcept {
    return (ch == ' ') || ((ch >= charTab) && (ch <= charCarriageReturn));
}

constexpr bool iswordchar(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '.' || ch == '_';
}

constexpr bool iswordstart(int ch) noexcept {
	return IsAlphaNumeric(ch) || ch == '_';
}

constexpr bool isoperator(int ch) noexcept {
	if (IsAlphaNumeric(ch))
		return false;
	if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
	        ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
	        ch == '?' || ch == '!' || ch == '.' || ch == '~')
		return true;
	return false;
}

// Simple case functions for ASCII supersets.

template <typename T>
constexpr T MakeUpperCase(T ch) noexcept {
	if (ch < 'a' || ch > 'z')
		return ch;
	return ch - 'a' + 'A';
}

template <typename T>
constexpr T MakeLowerCase(T ch) noexcept {
	if (ch < 'A' || ch > 'Z')
		return ch;
	return ch - 'A' + 'a';
}

int CompareCaseInsensitive(const char *a, const char *b) noexcept;
bool EqualCaseInsensitive(std::string_view a, std::string_view b) noexcept;
int CompareNCaseInsensitive(const char *a, const char *b, size_t len) noexcept;

constexpr bool StartsWith(std::string_view s, char start) noexcept {
	return !s.empty() && (s.front() == start);
}

}

#endif
