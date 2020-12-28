// Scintilla source code edit control
/** @file CharClassify.h
 ** Character classifications used by Document and RESearch.
 **/
// Copyright 2006-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARCLASSIFY_H
#define CHARCLASSIFY_H

namespace Scintilla {

class CharClassify {
public:
	CharClassify();

	enum cc { ccSpace, ccNewLine, ccWord, ccPunctuation };
	void SetDefaultCharClasses(bool includeWordClass);
	void SetCharClasses(const unsigned char *chars, cc newCharClass);
	int GetCharsOfClass(cc characterClass, unsigned char *buffer) const;
	cc GetClass(unsigned char ch) const noexcept { return static_cast<cc>(charClass[ch]);}
	bool IsWord(unsigned char ch) const noexcept { return static_cast<cc>(charClass[ch]) == ccWord;}

private:
	enum { maxChar=256 };
	unsigned char charClass[maxChar];    // not type cc to save space
};

}

#endif
