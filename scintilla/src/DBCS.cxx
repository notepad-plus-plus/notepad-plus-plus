// Scintilla source code edit control
/** @file DBCS.cxx
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "DBCS.h"

using namespace Scintilla::Internal;

namespace Scintilla::Internal {

bool DBCSIsLeadByte(int codePage, char ch) noexcept {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	const unsigned char uch = ch;
	switch (codePage) {
	case 932:
		// Shift_jis
		return ((uch >= 0x81) && (uch <= 0x9F)) ||
			((uch >= 0xE0) && (uch <= 0xFC));
		// Lead bytes F0 to FC may be a Microsoft addition.
	case 936:
		// GBK
		return (uch >= 0x81) && (uch <= 0xFE);
	case 949:
		// Korean Wansung KS C-5601-1987
		return (uch >= 0x81) && (uch <= 0xFE);
	case 950:
		// Big5
		return (uch >= 0x81) && (uch <= 0xFE);
	case 1361:
		// Korean Johab KS C-5601-1992
		return
			((uch >= 0x84) && (uch <= 0xD3)) ||
			((uch >= 0xD8) && (uch <= 0xDE)) ||
			((uch >= 0xE0) && (uch <= 0xF9));
	}
	return false;
}

bool IsDBCSValidSingleByte(int codePage, int ch) noexcept {
	switch (codePage) {
	case 932:
		return ch == 0x80
			|| (ch >= 0xA0 && ch <= 0xDF)
			|| (ch >= 0xFD);

	default:
		return false;
	}
}

}
