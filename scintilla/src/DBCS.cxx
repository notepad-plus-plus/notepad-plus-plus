// Scintilla source code edit control
/** @file DBCS.cxx
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>

#include <array>
#include <map>

#include "DBCS.h"

using namespace Scintilla::Internal;

namespace Scintilla::Internal {

bool DBCSIsLeadByte(int codePage, char ch) noexcept {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	const unsigned char uch = ch;
	switch (codePage) {
	case cp932:
		// Shift_jis
		return ((uch >= 0x81) && (uch <= 0x9F)) ||
			((uch >= 0xE0) && (uch <= 0xFC));
		// Lead bytes F0 to FC may be a Microsoft addition.
	case cp936:
		// GBK
		return (uch >= 0x81) && (uch <= 0xFE);
	case cp949:
		// Korean Wansung KS C-5601-1987
		return (uch >= 0x81) && (uch <= 0xFE);
	case cp950:
		// Big5
		return (uch >= 0x81) && (uch <= 0xFE);
	case cp1361:
		// Korean Johab KS C-5601-1992
		return
			((uch >= 0x84) && (uch <= 0xD3)) ||
			((uch >= 0xD8) && (uch <= 0xDE)) ||
			((uch >= 0xE0) && (uch <= 0xF9));
	default:
		break;
	}
	return false;
}

bool DBCSIsTrailByte(int codePage, char ch) noexcept {
	const unsigned char trail = ch;
	switch (codePage) {
	case cp932:
		// Shift_jis
		return (trail != 0x7F) &&
			((trail >= 0x40) && (trail <= 0xFC));
	case cp936:
		// GBK
		return (trail != 0x7F) &&
			((trail >= 0x40) && (trail <= 0xFE));
	case cp949:
		// Korean Wansung KS C-5601-1987
		return
			((trail >= 0x41) && (trail <= 0x5A)) ||
			((trail >= 0x61) && (trail <= 0x7A)) ||
			((trail >= 0x81) && (trail <= 0xFE));
	case cp950:
		// Big5
		return
			((trail >= 0x40) && (trail <= 0x7E)) ||
			((trail >= 0xA1) && (trail <= 0xFE));
	case cp1361:
		// Korean Johab KS C-5601-1992
		return
			((trail >= 0x31) && (trail <= 0x7E)) ||
			((trail >= 0x81) && (trail <= 0xFE));
	default:
		break;
	}
	return false;
}

bool IsDBCSValidSingleByte(int codePage, int ch) noexcept {
	switch (codePage) {
	case cp932:
		return ch == 0x80
			|| (ch >= 0xA0 && ch <= 0xDF)
			|| (ch >= 0xFD);

	default:
		return false;
	}
}

using CodePageToFoldMap = std::map<int, FoldMap>;
CodePageToFoldMap cpToFoldMap;

bool DBCSHasFoldMap(int codePage) {
	const CodePageToFoldMap::const_iterator it = cpToFoldMap.find(codePage);
	return it != cpToFoldMap.end();
}

void DBCSSetFoldMap(int codePage, const FoldMap &foldMap) {
	cpToFoldMap[codePage] = foldMap;
}

FoldMap *DBCSGetMutableFoldMap(int codePage) {
	// Constructs if needed
	return &cpToFoldMap[codePage];
}

const FoldMap *DBCSGetFoldMap(int codePage) {
	return &cpToFoldMap[codePage];
}

}
