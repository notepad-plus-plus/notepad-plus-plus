// Scintilla source code edit control
/** @file CharacterSet.h
 ** Encapsulates a set of characters. Used to test if a character is within a set.
 **/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERSET_H
#define CHARACTERSET_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class CharacterSet {
	int size;
	bool valueAfter;
	bool *bset;
public:
	enum setBase {
		setNone=0,
		setLower=1,
		setUpper=2,
		setDigits=4,
		setAlpha=setLower|setUpper,
		setAlphaNum=setAlpha|setDigits
	};
	CharacterSet(setBase base=setNone, const char *initialSet="", int size_=0x80, bool valueAfter_=false) {
		size = size_;
		valueAfter = valueAfter_;
		bset = new bool[size];
		for (int i=0; i < size; i++) {
			bset[i] = false;
		}
		AddString(initialSet);
		if (base & setLower)
			AddString("abcdefghijklmnopqrstuvwxyz");
		if (base & setUpper)
			AddString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		if (base & setDigits)
			AddString("0123456789");
	}
	~CharacterSet() {
		delete []bset;
		bset = 0;
		size = 0;
	}
	void Add(int val) {
		assert(val >= 0);
		assert(val < size);
		bset[val] = true;
	}
	void AddString(const char *setToAdd) {
		for (const char *cp=setToAdd; *cp; cp++) {
			int val = static_cast<unsigned char>(*cp);
			assert(val >= 0);
			assert(val < size);
			bset[val] = true;
		}
	}
	bool Contains(int val) const {
		assert(val >= 0);
		if (val < 0) return false;
		return (val < size) ? bset[val] : valueAfter;
	}
};

// Functions for classifying characters

inline bool IsASpace(int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

inline bool IsASpaceOrTab(int ch) {
	return (ch == ' ') || (ch == '\t');
}

inline bool IsADigit(int ch) {
	return (ch >= '0') && (ch <= '9');
}

inline bool IsADigit(int ch, int base) {
	if (base <= 10) {
		return (ch >= '0') && (ch < '0' + base);
	} else {
		return ((ch >= '0') && (ch <= '9')) ||
		       ((ch >= 'A') && (ch < 'A' + base - 10)) ||
		       ((ch >= 'a') && (ch < 'a' + base - 10));
	}
}

inline bool IsASCII(int ch) {
	return ch < 0x80;
}

inline bool IsAlphaNumeric(int ch) {
	return
		((ch >= '0') && (ch <= '9')) ||
		((ch >= 'a') && (ch <= 'z')) ||
		((ch >= 'A') && (ch <= 'Z'));
}

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
inline bool isspacechar(int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

inline bool iswordchar(int ch) {
	return IsASCII(ch) && (IsAlphaNumeric(ch) || ch == '.' || ch == '_');
}

inline bool iswordstart(int ch) {
	return IsASCII(ch) && (IsAlphaNumeric(ch) || ch == '_');
}

inline bool isoperator(int ch) {
	if (IsASCII(ch) && IsAlphaNumeric(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
	        ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
	        ch == '?' || ch == '!' || ch == '.' || ch == '~')
		return true;
	return false;
}

// Simple case functions for ASCII.

inline char MakeUpperCase(char ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return static_cast<char>(ch - 'a' + 'A');
}

int CompareCaseInsensitive(const char *a, const char *b);
int CompareNCaseInsensitive(const char *a, const char *b, size_t len);

#ifdef SCI_NAMESPACE
}
#endif

#endif
