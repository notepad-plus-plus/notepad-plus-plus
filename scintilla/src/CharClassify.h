// Scintilla source code edit control
/** @file CharClassify.h
 ** Character classifications used by Document and RESearch.
 **/
// Copyright 2006-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARCLASSIFY_H
#define CHARCLASSIFY_H

class CharClassify {
public:
	CharClassify();

	enum cc { ccSpace, ccNewLine, ccWord, ccPunctuation };
	void SetDefaultCharClasses(bool includeWordClass);
	void SetCharClasses(const unsigned char *chars, cc newCharClass);
	cc GetClass(unsigned char ch) const { return static_cast<cc>(charClass[ch]);}
	bool IsWord(unsigned char ch) const { return static_cast<cc>(charClass[ch]) == ccWord;}

private:
	enum { maxChar=256 };
	unsigned char charClass[maxChar];    // not type cc to save space
};

// These functions are implemented because each platform calls them something different.
int CompareCaseInsensitive(const char *a, const char *b);
int CompareNCaseInsensitive(const char *a, const char *b, size_t len);

inline char MakeUpperCase(char ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return static_cast<char>(ch - 'a' + 'A');
}

#endif
