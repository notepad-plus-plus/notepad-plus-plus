// Scintilla source code edit control
/** @file CharacterSet.h
 ** Encapsulates a set of characters. Used to test if a character is within a set.
 **/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERSET_H
#define CHARACTERSET_H

namespace Scintilla {

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
	CharacterSet(const CharacterSet &other) {
		size = other.size;
		valueAfter = other.valueAfter;
		bset = new bool[size];
		for (int i=0; i < size; i++) {
			bset[i] = other.bset[i];
		}
	}
	CharacterSet &operator=(CharacterSet &&other) {
		if (this != &other) {
			delete []bset;
			size = other.size;
			valueAfter = other.valueAfter;
			bset = other.bset;
			other.size = 0;
			other.bset = nullptr;
		}
		return *this;
	}
	~CharacterSet() {
		delete []bset;
		bset = nullptr;
		size = 0;
	}
	CharacterSet &operator=(const CharacterSet &other) {
		if (this != &other) {
			bool *bsetNew = new bool[other.size];
			for (int i=0; i < other.size; i++) {
				bsetNew[i] = other.bset[i];
			}
			delete []bset;
			size = other.size;
			valueAfter = other.valueAfter;
			bset = bsetNew;
		}
		return *this;
	}
	void Add(int val) {
		assert(val >= 0);
		assert(val < size);
		bset[val] = true;
	}
	void AddString(const char *setToAdd) {
		for (const char *cp=setToAdd; *cp; cp++) {
			const unsigned char uch = *cp;
			assert(uch < size);
			bset[uch] = true;
		}
	}
	bool Contains(int val) const {
		assert(val >= 0);
		if (val < 0) return false;
		return (val < size) ? bset[val] : valueAfter;
	}
	bool Contains(char ch) const {
		// Overload char as char may be signed
		const unsigned char uch = ch;
		return Contains(uch);
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
	return (ch >= 0) && (ch < 0x80);
}

inline bool IsLowerCase(int ch) {
	return (ch >= 'a') && (ch <= 'z');
}

inline bool IsUpperCase(int ch) {
	return (ch >= 'A') && (ch <= 'Z');
}

inline bool IsUpperOrLowerCase(int ch) {
	return IsUpperCase(ch) || IsLowerCase(ch);
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
	return IsAlphaNumeric(ch) || ch == '.' || ch == '_';
}

inline bool iswordstart(int ch) {
	return IsAlphaNumeric(ch) || ch == '_';
}

inline bool isoperator(int ch) {
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
inline T MakeUpperCase(T ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return ch - 'a' + 'A';
}

template <typename T>
inline T MakeLowerCase(T ch) {
	if (ch < 'A' || ch > 'Z')
		return ch;
	else
		return ch - 'A' + 'a';
}

int CompareCaseInsensitive(const char *a, const char *b);
int CompareNCaseInsensitive(const char *a, const char *b, size_t len);

}

#endif
