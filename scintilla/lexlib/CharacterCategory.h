// Scintilla source code edit control
/** @file CharacterCategory.h
 ** Returns the Unicode general category of a character.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERCATEGORY_H
#define CHARACTERCATEGORY_H

namespace Scintilla {

enum CharacterCategory {
	ccLu, ccLl, ccLt, ccLm, ccLo,
	ccMn, ccMc, ccMe,
	ccNd, ccNl, ccNo,
	ccPc, ccPd, ccPs, ccPe, ccPi, ccPf, ccPo,
	ccSm, ccSc, ccSk, ccSo,
	ccZs, ccZl, ccZp,
	ccCc, ccCf, ccCs, ccCo, ccCn
};

CharacterCategory CategoriseCharacter(int character);

// Common definitions of allowable characters in identifiers from UAX #31.
bool IsIdStart(int character);
bool IsIdContinue(int character);
bool IsXidStart(int character);
bool IsXidContinue(int character);

class CharacterCategoryMap {
private:
	std::vector<unsigned char> dense;
public:
	CharacterCategoryMap();
	CharacterCategory CategoryFor(int character) const {
		if (static_cast<size_t>(character) < dense.size()) {
			return static_cast<CharacterCategory>(dense[character]);
		} else {
			// binary search through ranges
			return CategoriseCharacter(character);
		}
	}
	int Size() const noexcept;
	void Optimize(int countCharacters);
};

}

#endif
