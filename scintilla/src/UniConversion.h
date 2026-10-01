// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UTF-8 and UTF-16 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef UNICONVERSION_H
#define UNICONVERSION_H

namespace Scintilla::Internal {

constexpr int UTF8MaxBytes = 4;

constexpr int unicodeReplacementChar = 0xFFFD;

size_t UTF8Length(std::wstring_view wsv) noexcept;
size_t UTF8PositionFromUTF16Position(std::string_view u8Text, size_t positionUTF16) noexcept;
void UTF8FromUTF16(std::wstring_view wsv, char *putf, size_t len) noexcept;
void UTF8FromUTF32Character(int uch, char *putf) noexcept;
size_t UTF16Length(std::string_view svu8) noexcept;
size_t UTF16FromUTF8(std::string_view svu8, wchar_t *tbuf, size_t tlen);
size_t UTF32Length(std::string_view svu8) noexcept;
size_t UTF32FromUTF8(std::string_view svu8, unsigned int *tbuf, size_t tlen);
// WStringFromUTF8 does the right thing when wchar_t is 2 or 4 bytes so
// works on both Windows and Unix.
std::wstring WStringFromUTF8(std::string_view svu8);
unsigned int UTF16FromUTF32Character(unsigned int val, wchar_t *tbuf) noexcept;
bool UTF8IsValid(std::string_view svu8) noexcept;
std::string FixInvalidUTF8(const std::string &text);

extern const unsigned char UTF8BytesOfLead[256];

constexpr unsigned char firstNonASCII = 0x80;
constexpr unsigned char lastTrail = 0xBF;
constexpr unsigned char startFirst = 0xC2;
constexpr unsigned char startLast = 0xF4;
constexpr unsigned char bitsInTrail = 6;
constexpr unsigned char maskTrail = 0x3F;
constexpr unsigned char mask2 = 0x1F;
constexpr unsigned char mask3 = 0x0F;
constexpr unsigned char mask4 = 0x07;

inline int UnicodeFromUTF8(const unsigned char *us) noexcept {
	switch (UTF8BytesOfLead[us[0]]) {
	case 1:
		return us[0];
	case 2:
		return ((us[0] & mask2) << bitsInTrail) + (us[1] & maskTrail);
	case 3:
		return ((us[0] & mask3) << (2 * bitsInTrail)) + ((us[1] & maskTrail) << bitsInTrail) + (us[2] & maskTrail);
	default:
		return ((us[0] & mask4) << (3 * bitsInTrail)) + ((us[1] & maskTrail) << (2 * bitsInTrail)) + ((us[2] & maskTrail) << bitsInTrail) + (us[3] & maskTrail);
	}
}
int UnicodeFromUTF8(std::string_view sv) noexcept;

constexpr bool UTF8IsTrailByte(unsigned char ch) noexcept {
	return (ch >= firstNonASCII) && (ch <= lastTrail);
}

constexpr bool UTF8IsFirstByte(unsigned char ch) noexcept {
	return (ch >= startFirst) && (ch <= startLast);
}

constexpr bool UTF8IsAscii(unsigned char ch) noexcept {
	return ch < firstNonASCII;
}

constexpr bool UTF8IsAscii(char ch) noexcept {
	const unsigned char uch = ch;
	return uch < firstNonASCII;
}

enum { UTF8MaskWidth=0x7, UTF8MaskInvalid=0x8 };
int UTF8Classify(const unsigned char *us, size_t len) noexcept;
int UTF8Classify(const char *s, size_t len) noexcept;
inline int UTF8Classify(std::string_view sv) noexcept {
	return UTF8Classify(sv.data(), sv.length());
}

// Similar to UTF8Classify but returns a length of 1 for invalid bytes
// instead of setting the invalid flag
int UTF8DrawBytes(const char *s, size_t len) noexcept;

// NEL=\xC2\x85 LS=\xE2\x80\xA8 PS=\xE2\x80\xA9
constexpr unsigned char firstNEL = 0xC2;
constexpr unsigned char lastNEL = 0x85;
constexpr unsigned char firstLSPS = 0xE2;
constexpr unsigned char midLSPS = 0x80;
constexpr unsigned char lastLS = 0xA8;
constexpr unsigned char lastPS = 0xA9;

// Line separator is U+2028 \xe2\x80\xa8
// Paragraph separator is U+2029 \xe2\x80\xa9
constexpr int UTF8SeparatorLength = 3;
constexpr bool UTF8IsSeparator(const unsigned char *us) noexcept {
	return (us[0] == firstLSPS) && (us[1] == midLSPS) && ((us[2] == lastLS) || (us[2] == lastPS));
}

// NEL is U+0085 \xc2\x85
constexpr int UTF8NELLength = 2;
constexpr bool UTF8IsNEL(const unsigned char *us) noexcept {
	return (us[0] == firstNEL) && (us[1] == lastNEL);
}

// Is the sequence of 3 char a UTF-8 line end? Only the last two char are tested for a NEL.
constexpr bool UTF8IsMultibyteLineEnd(unsigned char ch0, unsigned char ch1, unsigned char ch2) noexcept {
	return
		((ch0 == firstLSPS) && (ch1 == midLSPS) && ((ch2 == lastLS) || (ch2 == lastPS))) ||
		((ch1 == firstNEL) && (ch2 == lastNEL));
}

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_LEAD_LAST = 0xDBFF };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };
enum { SUPPLEMENTAL_PLANE_FIRST = 0x10000 };

constexpr bool IsSurrogate(wchar_t uch) noexcept {
	return (uch >= SURROGATE_LEAD_FIRST) && (uch <= SURROGATE_TRAIL_LAST);
}

constexpr unsigned int UTF16CharLength(wchar_t uch) noexcept {
	return IsSurrogate(uch) ? 2 : 1;
}

constexpr unsigned int UTF16LengthFromUTF8ByteCount(unsigned int byteCount) noexcept {
	return (byteCount < 4) ? 1 : 2;
}

}

#endif
