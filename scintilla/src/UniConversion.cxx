// Scintilla source code edit control
/** @file UniConversion.cxx
 ** Functions to handle UTF-8 and UTF-16 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>

#include <stdexcept>
#include <string>
#include <string_view>

#include "UniConversion.h"

using namespace Scintilla;

namespace Scintilla {

size_t UTF8Length(std::wstring_view wsv) noexcept {
	size_t len = 0;
	for (size_t i = 0; i < wsv.length() && wsv[i];) {
		const unsigned int uch = wsv[i];
		if (uch < 0x80) {
			len++;
		} else if (uch < 0x800) {
			len += 2;
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		} else {
			len += 3;
		}
		i++;
	}
	return len;
}

size_t UTF8PositionFromUTF16Position(std::string_view u8Text, size_t positionUTF16) noexcept {
	size_t positionUTF8 = 0;
	for (size_t lengthUTF16 = 0; (positionUTF8 < u8Text.length()) && (lengthUTF16 < positionUTF16);) {
		const unsigned char uch = u8Text[positionUTF8];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		lengthUTF16 += UTF16LengthFromUTF8ByteCount(byteCount);
		positionUTF8 += byteCount;
	}

	return positionUTF8;
}

void UTF8FromUTF16(std::wstring_view wsv, char *putf, size_t len) {
	size_t k = 0;
	for (size_t i = 0; i < wsv.length() && wsv[i];) {
		const unsigned int uch = wsv[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			const unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (wsv[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		} else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
	if (k < len)
		putf[k] = '\0';
}

void UTF8FromUTF32Character(int uch, char *putf) noexcept {
	size_t k = 0;
	if (uch < 0x80) {
		putf[k++] = static_cast<char>(uch);
	} else if (uch < 0x800) {
		putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
		putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
	} else if (uch < 0x10000) {
		putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
		putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
		putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
	} else {
		putf[k++] = static_cast<char>(0xF0 | (uch >> 18));
		putf[k++] = static_cast<char>(0x80 | ((uch >> 12) & 0x3f));
		putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
		putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
	}
	putf[k] = '\0';
}

size_t UTF16Length(std::string_view svu8) noexcept {
	size_t ulen = 0;
	for (size_t i = 0; i< svu8.length();) {
		const unsigned char ch = svu8[i];
		const unsigned int byteCount = UTF8BytesOfLead[ch];
		const unsigned int utf16Len = UTF16LengthFromUTF8ByteCount(byteCount);
		i += byteCount;
		ulen += (i > svu8.length()) ? 1 : utf16Len;
	}
	return ulen;
}

constexpr unsigned char TrailByteValue(unsigned char c) {
	// The top 2 bits are 0b10 to indicate a trail byte.
	// The lower 6 bits contain the value.
	return c & 0b0011'1111;
}

size_t UTF16FromUTF8(std::string_view svu8, wchar_t *tbuf, size_t tlen) {
	size_t ui = 0;
	for (size_t i = 0; i < svu8.length();) {
		unsigned char ch = svu8[i];
		const unsigned int byteCount = UTF8BytesOfLead[ch];
		unsigned int value;

		if (i + byteCount > svu8.length()) {
			// Trying to read past end but still have space to write
			if (ui < tlen) {
				tbuf[ui] = ch;
				ui++;
			}
			break;
		}

		const size_t outLen = UTF16LengthFromUTF8ByteCount(byteCount);
		if (ui + outLen > tlen) {
			throw std::runtime_error("UTF16FromUTF8: attempted write beyond end");
		}

		i++;
		switch (byteCount) {
		case 1:
			tbuf[ui] = ch;
			break;
		case 2:
			value = (ch & 0x1F) << 6;
			ch = svu8[i++];
			value += TrailByteValue(ch);
			tbuf[ui] = static_cast<wchar_t>(value);
			break;
		case 3:
			value = (ch & 0xF) << 12;
			ch = svu8[i++];
			value += (TrailByteValue(ch) << 6);
			ch = svu8[i++];
			value += TrailByteValue(ch);
			tbuf[ui] = static_cast<wchar_t>(value);
			break;
		default:
			// Outside the BMP so need two surrogates
			value = (ch & 0x7) << 18;
			ch = svu8[i++];
			value += TrailByteValue(ch) << 12;
			ch = svu8[i++];
			value += TrailByteValue(ch) << 6;
			ch = svu8[i++];
			value += TrailByteValue(ch);
			tbuf[ui] = static_cast<wchar_t>(((value - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
			ui++;
			tbuf[ui] = static_cast<wchar_t>((value & 0x3ff) + SURROGATE_TRAIL_FIRST);
			break;
		}
		ui++;
	}
	return ui;
}

size_t UTF32Length(std::string_view svu8) noexcept {
	size_t ulen = 0;
	for (size_t i = 0; i < svu8.length();) {
		const unsigned char ch = svu8[i];
		const unsigned int byteCount = UTF8BytesOfLead[ch];
		i += byteCount;
		ulen++;
	}
	return ulen;
}

size_t UTF32FromUTF8(std::string_view svu8, unsigned int *tbuf, size_t tlen) {
	size_t ui = 0;
	for (size_t i = 0; i < svu8.length();) {
		unsigned char ch = svu8[i];
		const unsigned int byteCount = UTF8BytesOfLead[ch];
		unsigned int value;

		if (i + byteCount > svu8.length()) {
			// Trying to read past end but still have space to write
			if (ui < tlen) {
				tbuf[ui] = ch;
				ui++;
			}
			break;
		}

		if (ui == tlen) {
			throw std::runtime_error("UTF32FromUTF8: attempted write beyond end");
		}

		i++;
		switch (byteCount) {
		case 1:
			value = ch;
			break;
		case 2:
			value = (ch & 0x1F) << 6;
			ch = svu8[i++];
			value += TrailByteValue(ch);
			break;
		case 3:
			value = (ch & 0xF) << 12;
			ch = svu8[i++];
			value += TrailByteValue(ch) << 6;
			ch = svu8[i++];
			value += TrailByteValue(ch);
			break;
		default:
			value = (ch & 0x7) << 18;
			ch = svu8[i++];
			value += TrailByteValue(ch) << 12;
			ch = svu8[i++];
			value += TrailByteValue(ch) << 6;
			ch = svu8[i++];
			value += TrailByteValue(ch);
			break;
		}
		tbuf[ui] = value;
		ui++;
	}
	return ui;
}

std::wstring WStringFromUTF8(std::string_view svu8) {
	if constexpr (sizeof(wchar_t) == 2) {
		const size_t len16 = UTF16Length(svu8);
		std::wstring ws(len16, 0);
		UTF16FromUTF8(svu8, &ws[0], len16);
		return ws;
	} else {
		const size_t len32 = UTF32Length(svu8);
		std::wstring ws(len32, 0);
		UTF32FromUTF8(svu8, reinterpret_cast<unsigned int *>(&ws[0]), len32);
		return ws;
	}
}

unsigned int UTF16FromUTF32Character(unsigned int val, wchar_t *tbuf) noexcept {
	if (val < SUPPLEMENTAL_PLANE_FIRST) {
		tbuf[0] = static_cast<wchar_t>(val);
		return 1;
	} else {
		tbuf[0] = static_cast<wchar_t>(((val - SUPPLEMENTAL_PLANE_FIRST) >> 10) + SURROGATE_LEAD_FIRST);
		tbuf[1] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		return 2;
	}
}

const unsigned char UTF8BytesOfLead[256] = {
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00 - 0F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 10 - 1F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 20 - 2F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 30 - 3F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 40 - 4F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 50 - 5F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 60 - 6F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70 - 7F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 80 - 8F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 90 - 9F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // A0 - AF
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // B0 - BF
1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0 - CF
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0 - DF
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0 - EF
4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // F0 - FF
};

// Return both the width of the first character in the string and a status
// saying whether it is valid or invalid.
// Most invalid sequences return a width of 1 so are treated as isolated bytes but
// the non-characters *FFFE, *FFFF and FDD0 .. FDEF return 3 or 4 as they can be
// reasonably treated as code points in some circumstances. They will, however,
// not have associated glyphs.
int UTF8Classify(const unsigned char *us, size_t len) noexcept {
	// For the rules: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
	if (us[0] < 0x80) {
		// ASCII
		return 1;
	}

	const size_t byteCount = UTF8BytesOfLead[us[0]];
	if (byteCount == 1 || byteCount > len) {
		// Invalid lead byte
		return UTF8MaskInvalid | 1;
	}

	if (!UTF8IsTrailByte(us[1])) {
		// Invalid trail byte
		return UTF8MaskInvalid | 1;
	}

	switch (byteCount) {
	case 2:
		return 2;

	case 3:
		if (UTF8IsTrailByte(us[2])) {
			if ((*us == 0xe0) && ((us[1] & 0xe0) == 0x80)) {
				// Overlong
				return UTF8MaskInvalid | 1;
			}
			if ((*us == 0xed) && ((us[1] & 0xe0) == 0xa0)) {
				// Surrogate
				return UTF8MaskInvalid | 1;
			}
			if ((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbe)) {
				// U+FFFE non-character - 3 bytes long
				return UTF8MaskInvalid | 3;
			}
			if ((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbf)) {
				// U+FFFF non-character - 3 bytes long
				return UTF8MaskInvalid | 3;
			}
			if ((*us == 0xef) && (us[1] == 0xb7) && (((us[2] & 0xf0) == 0x90) || ((us[2] & 0xf0) == 0xa0))) {
				// U+FDD0 .. U+FDEF
				return UTF8MaskInvalid | 3;
			}
			return 3;
		}
		break;

	default:
		if (UTF8IsTrailByte(us[2]) && UTF8IsTrailByte(us[3])) {
			if (((us[1] & 0xf) == 0xf) && (us[2] == 0xbf) && ((us[3] == 0xbe) || (us[3] == 0xbf))) {
				// *FFFE or *FFFF non-character
				return UTF8MaskInvalid | 4;
			}
			if (*us == 0xf4) {
				// Check if encoding a value beyond the last Unicode character 10FFFF
				if (us[1] > 0x8f) {
					return UTF8MaskInvalid | 1;
				}
			} else if ((*us == 0xf0) && ((us[1] & 0xf0) == 0x80)) {
				// Overlong
				return UTF8MaskInvalid | 1;
			}
			return 4;
		}
		break;
	}

	return UTF8MaskInvalid | 1;
}

int UTF8DrawBytes(const unsigned char *us, int len) noexcept {
	const int utf8StatusNext = UTF8Classify(us, len);
	return (utf8StatusNext & UTF8MaskInvalid) ? 1 : (utf8StatusNext & UTF8MaskWidth);
}

bool UTF8IsValid(std::string_view svu8) noexcept {
	const unsigned char *us = reinterpret_cast<const unsigned char *>(svu8.data());
	size_t remaining = svu8.length();
	while (remaining > 0) {
		const int utf8Status = UTF8Classify(us, remaining);
		if (utf8Status & UTF8MaskInvalid) {
			return false;
		} else {
			const int lenChar = utf8Status & UTF8MaskWidth;
			us += lenChar;
			remaining -= lenChar;
		}
	}
	return remaining == 0;
}

// Replace invalid bytes in UTF-8 with the replacement character
std::string FixInvalidUTF8(const std::string &text) {
	std::string result;
	const char *s = text.c_str();
	size_t remaining = text.size();
	while (remaining > 0) {
		const int utf8Status = UTF8Classify(reinterpret_cast<const unsigned char *>(s), remaining);
		if (utf8Status & UTF8MaskInvalid) {
			// Replacement character 0xFFFD = UTF8:"efbfbd".
			result.append("\xef\xbf\xbd");
			s++;
			remaining--;
		} else {
			const size_t len = utf8Status & UTF8MaskWidth;
			result.append(s, len);
			s += len;
			remaining -= len;
		}
	}
	return result;
}

}
