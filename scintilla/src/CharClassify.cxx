// Scintilla source code edit control
/** @file CharClassify.cxx
 ** Character classifications used by Document and RESearch.
 **/
// Copyright 2006 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include <stdexcept>

#include "CharacterType.h"
#include "CharClassify.h"

using namespace Scintilla::Internal;

CharClassify::CharClassify() : charClass{} {
	SetDefaultCharClasses(true);
}

void CharClassify::SetDefaultCharClasses(bool includeWordClass) {
	// Initialize all char classes to default values
	for (int ch = 0; ch < maxChar; ch++) {
		if (ch == '\r' || ch == '\n')
			charClass[ch] = CharacterClass::newLine;
		else if (IsControl(ch) || ch == ' ')
			charClass[ch] = CharacterClass::space;
		else if (includeWordClass && (ch >= 0x80 || IsAlphaNumeric(ch) || ch == '_'))
			charClass[ch] = CharacterClass::word;
		else
			charClass[ch] = CharacterClass::punctuation;
	}
}

void CharClassify::SetCharClasses(const unsigned char *chars, CharacterClass newCharClass) {
	// Apply the newCharClass to the specified chars
	if (chars) {
		while (*chars) {
			charClass[*chars] = newCharClass;
			chars++;
		}
	}
}

int CharClassify::GetCharsOfClass(CharacterClass characterClass, unsigned char *buffer) const noexcept {
	// Get characters belonging to the given char class; return the number
	// of characters (if the buffer is NULL, don't write to it).
	int count = 0;
	for (int ch = maxChar - 1; ch >= 0; --ch) {
		if (charClass[ch] == characterClass) {
			++count;
			if (buffer) {
				*buffer = static_cast<unsigned char>(ch);
				buffer++;
			}
		}
	}
	return count;
}
