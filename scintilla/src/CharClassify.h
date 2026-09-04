// Scintilla source code edit control
/** @file CharClassify.h
 ** Character classifications used by Document and RESearch.
 **/
// Copyright 2006-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARCLASSIFY_H
#define CHARCLASSIFY_H

namespace Scintilla::Internal {

enum class CharacterClass : unsigned char { space, newLine, word, punctuation };

class CharClassify {
public:
	CharClassify();

	void SetDefaultCharClasses(bool includeWordClass);
	void SetCharClasses(const unsigned char *chars, CharacterClass newCharClass);
	int GetCharsOfClass(CharacterClass characterClass, unsigned char *buffer) const noexcept;
	CharacterClass GetClass(unsigned char ch) const noexcept { return charClass[ch];}
	bool IsWord(unsigned char ch) const noexcept { return charClass[ch] == CharacterClass::word;}

private:
	static constexpr int maxChar=256;
	CharacterClass charClass[maxChar];
};

}

#endif
