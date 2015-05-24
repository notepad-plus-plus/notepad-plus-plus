// Scintilla source code edit control
/** @file CharacterCategory.h
 ** Returns the Unicode general category of a character.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERCATEGORY_H
#define CHARACTERCATEGORY_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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

#ifdef SCI_NAMESPACE
}
#endif

#endif
