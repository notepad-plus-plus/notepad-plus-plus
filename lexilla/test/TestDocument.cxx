// Lexilla lexer library
/** @file TestDocument.cxx
 ** Lexer testing.
 **/
 // Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include <iostream>

#include "ILexer.h"

#include "TestDocument.h"

namespace {

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

	int UnicodeFromUTF8(const unsigned char *us) noexcept {
		assert(us);
		switch (UTF8BytesOfLead[us[0]]) {
		case 1:
			return us[0];
		case 2:
			return ((us[0] & 0x1F) << 6) + (us[1] & 0x3F);
		case 3:
			return ((us[0] & 0xF) << 12) + ((us[1] & 0x3F) << 6) + (us[2] & 0x3F);
		default:
			return ((us[0] & 0x7) << 18) + ((us[1] & 0x3F) << 12) + ((us[2] & 0x3F) << 6) + (us[3] & 0x3F);
		}
	}

	inline constexpr bool UTF8IsTrailByte(unsigned char ch) noexcept {
		return (ch >= 0x80) && (ch < 0xc0);
	}

	constexpr unsigned char TrailByteValue(unsigned char c) {
		// The top 2 bits are 0b10 to indicate a trail byte.
		// The lower 6 bits contain the value.
		return c & 0b0011'1111;
	}
}

std::u32string UTF32FromUTF8(std::string_view svu8) {
	std::u32string ret;
	for (size_t i = 0; i < svu8.length();) {
		unsigned char ch = svu8.at(i);
		const unsigned int byteCount = UTF8BytesOfLead[ch];
		unsigned int value = 0;

		if (i + byteCount > svu8.length()) {
			// Trying to read past end
			ret.push_back(ch);
			break;
		}

		i++;
		switch (byteCount) {
		case 1:
			value = ch;
			break;
		case 2:
			value = (ch & 0x1F) << 6;
			ch = svu8.at(i++);
			value += TrailByteValue(ch);
			break;
		case 3:
			value = (ch & 0xF) << 12;
			ch = svu8.at(i++);
			value += TrailByteValue(ch) << 6;
			ch = svu8.at(i++);
			value += TrailByteValue(ch);
			break;
		default:
			value = (ch & 0x7) << 18;
			ch = svu8.at(i++);
			value += TrailByteValue(ch) << 12;
			ch = svu8.at(i++);
			value += TrailByteValue(ch) << 6;
			ch = svu8.at(i++);
			value += TrailByteValue(ch);
			break;
		}
		ret.push_back(value);
	}
	return ret;
}

void TestDocument::Set(std::string_view sv) {
	text = sv;
	textStyles.resize(text.size() + 1);
	lineStarts.clear();
	endStyled = 0;
	lineStarts.push_back(0);
	for (size_t pos = 0; pos < text.length(); pos++) {
		if (text.at(pos) == '\n') {
			lineStarts.push_back(pos + 1);
		}
	}
	lineStarts.push_back(text.length());
	lineStates.resize(lineStarts.size());
	lineLevels.resize(lineStarts.size(), 0x400);
}

#if _MSC_VER
// IDocument interface does not specify noexcept so best to not add it to implementation
#pragma warning(disable: 26440)
#endif

int SCI_METHOD TestDocument::Version() const {
	return Scintilla::dvRelease4;
}

void SCI_METHOD TestDocument::SetErrorStatus(int) {
}

Sci_Position SCI_METHOD TestDocument::Length() const {
	return text.length();
}

void SCI_METHOD TestDocument::GetCharRange(char *buffer, Sci_Position position, Sci_Position lengthRetrieve) const {
	text.copy(buffer, lengthRetrieve, position);
}

char SCI_METHOD TestDocument::StyleAt(Sci_Position position) const {
	if (position < 0) {
		return 0;
	}
	return textStyles.at(position);
}

Sci_Position SCI_METHOD TestDocument::LineFromPosition(Sci_Position position) const {
	if (position >= static_cast<Sci_Position>(text.length())) {
		return lineStarts.size() - 1 - 1;
	}

	const std::vector<Sci_Position>::const_iterator it = std::lower_bound(lineStarts.begin(), lineStarts.end(), position);
	Sci_Position line = it - lineStarts.begin();
	if (*it > position)
		line--;
	return line;
}

Sci_Position SCI_METHOD TestDocument::LineStart(Sci_Position line) const {
	if (line < 0) {
		return 0;
	}
	if (line >= static_cast<Sci_Position>(lineStarts.size())) {
		return text.length();
	}
	return lineStarts.at(line);
}

int SCI_METHOD TestDocument::GetLevel(Sci_Position line) const {
	return lineLevels.at(line);
}

int SCI_METHOD TestDocument::SetLevel(Sci_Position line, int level) {
	return lineLevels.at(line) = level;
}

int SCI_METHOD TestDocument::GetLineState(Sci_Position line) const {
	return lineStates.at(line);
}

int SCI_METHOD TestDocument::SetLineState(Sci_Position line, int state) {
	return lineStates.at(line) = state;
}

void SCI_METHOD TestDocument::StartStyling(Sci_Position position) {
	endStyled = position;
}

bool SCI_METHOD TestDocument::SetStyleFor(Sci_Position length, char style) {
	for (Sci_Position i = 0; i < length; i++) {
		textStyles.at(endStyled) = style;
		endStyled++;
	}
	return true;
}

bool SCI_METHOD TestDocument::SetStyles(Sci_Position length, const char *styles) {
	assert(styles);
	for (Sci_Position i = 0; i < length; i++) {
		textStyles.at(endStyled) = styles[i];
		endStyled++;
	}
	return true;
}

void SCI_METHOD TestDocument::DecorationSetCurrentIndicator(int) {
	// Not implemented as no way to read decorations
}

void SCI_METHOD TestDocument::DecorationFillRange(Sci_Position, int, Sci_Position) {
	// Not implemented as no way to read decorations
}

void SCI_METHOD TestDocument::ChangeLexerState(Sci_Position, Sci_Position) {
	// Not implemented as no watcher to trigger
}

int SCI_METHOD TestDocument::CodePage() const {
	// Always UTF-8 for now
	return 65001;
}

bool SCI_METHOD TestDocument::IsDBCSLeadByte(char) const {
	// Always UTF-8 for now
	return false;
}

const char *SCI_METHOD TestDocument::BufferPointer() {
	return text.c_str();
}

int SCI_METHOD TestDocument::GetLineIndentation(Sci_Position) {
	// Never actually called - lexers use Accessor::IndentAmount
	return 0;
}

Sci_Position SCI_METHOD TestDocument::LineEnd(Sci_Position line) const {
	Sci_Position position = LineStart(line + 1);
	position--; // Back over CR or LF
	// When line terminator is CR+LF, may need to go back one more
	if ((position > LineStart(line)) && (text.at(position - 1) == '\r')) {
		position--;
	}
	return position;
}

Sci_Position SCI_METHOD TestDocument::GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const {
	Sci_Position pos = positionStart;
	if (characterOffset < 0) {
		while (characterOffset < 0) {
			if (pos <= 0) {
				return 0;
			}
			unsigned char previousByte = text.at(pos - 1);
			if (previousByte < 0x80) {
				pos--;
				characterOffset++;
			} else {
				while ((pos > 1) && UTF8IsTrailByte(previousByte)) {
					pos--;
					previousByte = text.at(pos - 1);
				}
				pos--;
				// text[pos] is now a character start
				characterOffset++;
			}
		}
		return pos;
	}
	assert(characterOffset >= 0);
	// TODO: invalid UTF-8
	while (characterOffset > 0) {
		Sci_Position width = 0;
		GetCharacterAndWidth(pos, &width);
		pos += width;
		characterOffset--;
	}
	return pos;
}

int SCI_METHOD TestDocument::GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const {
	// TODO: invalid UTF-8
	if (position >= static_cast<Sci_Position>(text.length())) {
		// Return NULs after document end
		if (pWidth) {
			*pWidth = 1;
		}
		return '\0';
	}
	const unsigned char leadByte = text.at(position);
	if (leadByte < 0x80) {
		if (pWidth) {
			*pWidth = 1;
		}
		return leadByte;
	}
	const int widthCharBytes = UTF8BytesOfLead[leadByte];
	unsigned char charBytes[] = { leadByte,0,0,0 };
	for (int b = 1; b < widthCharBytes; b++) {
		charBytes[b] = text.at(position + b);
	}

	if (pWidth) {
		*pWidth = widthCharBytes;
	}
	return UnicodeFromUTF8(charBytes);
}
