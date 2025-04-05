// Scintilla source code edit control
/** @file DBCS.h
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DBCS_H
#define DBCS_H

namespace Scintilla::Internal {

constexpr int cp932 = 932;
constexpr int cp936 = 936;
constexpr int cp949 = 949;
constexpr int cp950 = 950;
constexpr int cp1361 = 1361;

constexpr bool IsDBCSCodePage(int codePage) noexcept {
	return codePage == cp932
	       || codePage == cp936
	       || codePage == cp949
	       || codePage == cp950
	       || codePage == cp1361;
}

bool DBCSIsLeadByte(int codePage, char ch) noexcept;
bool DBCSIsTrailByte(int codePage, char ch) noexcept;
bool IsDBCSValidSingleByte(int codePage, int ch) noexcept;

// Calculate a number from a DBCS byte pair that can be used to index into an array or map. 
// Should only be called with genuine DBCS character pairs which means that ch1 has top bit set.
constexpr uint16_t DBCSIndex(char ch1, char ch2) noexcept {
	constexpr unsigned int highStart = 0x80U;
	constexpr unsigned int byteMultiply = 0x100U;
	const unsigned char uch1 = ch1;
	const unsigned char uch2 = ch2;
	return ((uch1 - highStart) * byteMultiply) + uch2;
}

struct DBCSPair {
	char chars[2];
};
using FoldMap = std::array<DBCSPair, 0x8000>;

bool DBCSHasFoldMap(int codePage);
void DBCSSetFoldMap(int codePage, const FoldMap &foldMap);
FoldMap *DBCSGetMutableFoldMap(int codePage);
const FoldMap *DBCSGetFoldMap(int codePage);

}

#endif
