// Scintilla source code edit control
/** @file CharacterSet.h
 ** Encapsulates a set of characters. Used to test if a character is within a set.
 **/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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
		PLATFORM_ASSERT(val >= 0);
		PLATFORM_ASSERT(val < size);
		bset[val] = true;
	}
	void AddString(const char *CharacterSet) {
		for (const char *cp=CharacterSet; *cp; cp++) {
			int val = static_cast<unsigned char>(*cp);
			PLATFORM_ASSERT(val >= 0);
			PLATFORM_ASSERT(val < size);
			bset[val] = true;
		}
	}
	bool Contains(int val) const {
		PLATFORM_ASSERT(val >= 0);
		if (val < 0) return false;
		return (val < size) ? bset[val] : valueAfter;
	}
};
