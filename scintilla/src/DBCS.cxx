// Scintilla source code edit control
/** @file DBCS.cxx
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>

#include <vector>
#include <array>
#include <map>
#include <algorithm>

#include "DBCS.h"

using namespace Scintilla::Internal;

namespace Scintilla::Internal {

// Silence 'magic' number use since the set of DBCS lead and trail bytes differ
// between encodings and would require many constant declarations that would just
// obscure the behaviour.

// NOLINTBEGIN(*-magic-numbers)

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
		// Shift_jis
		return ch == 0x80
			|| (ch >= 0xA0 && ch <= 0xDF)
			|| (ch >= 0xFD);
	case cp936:
		// GBK
		return ch == 0x80;
	default:
		return false;
	}
}

// NOLINTEND(*-magic-numbers)

namespace {

struct CodePageFoldMap {
	int codePage = 0;
	FoldMap foldMap;
	explicit CodePageFoldMap(int codePage_) noexcept : codePage {codePage_} {}
};

using CodePageToFoldMap = std::vector<CodePageFoldMap>;
CodePageToFoldMap cpToFoldMap;

}

FoldMap *DBCSCreateFoldMap(int codePage) {
	cpToFoldMap.emplace_back(codePage);
	return &(cpToFoldMap.back().foldMap);
}

const FoldMap *DBCSGetFoldMap(int codePage) {
	const CodePageToFoldMap::iterator it = std::find_if(cpToFoldMap.begin(), cpToFoldMap.end(),
		[codePage](const CodePageFoldMap &cpfm) -> bool {return cpfm.codePage == codePage; });
	if (it != cpToFoldMap.end()) {
		return &(it->foldMap);
	}
	return nullptr;
}

}
