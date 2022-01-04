// Scintilla source code edit control
/** @file DBCS.h
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DBCS_H
#define DBCS_H

namespace Scintilla::Internal {

constexpr bool IsDBCSCodePage(int codePage) noexcept {
	return codePage == 932
	       || codePage == 936
	       || codePage == 949
	       || codePage == 950
	       || codePage == 1361;
}

bool DBCSIsLeadByte(int codePage, char ch) noexcept;
bool IsDBCSValidSingleByte(int codePage, int ch) noexcept;

}

#endif
