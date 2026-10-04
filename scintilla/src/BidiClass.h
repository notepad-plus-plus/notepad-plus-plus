// Scintilla source code edit control
// First-strong paragraph direction, independent of visual alignment.
// The License.txt file describes the conditions under which this code may be distributed.
#ifndef BIDICLASS_H
#define BIDICLASS_H

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include "UniConversion.h"

namespace Scintilla::Internal {

#include "BidiClassData.h"

inline unsigned int StrongBidiDirection(unsigned int character) noexcept {
	if (character > 0x10FFFF) {
		return 0;
	}
	const auto range = std::upper_bound(std::begin(strongBidiRanges), std::end(strongBidiRanges),
		(character << 2) | 3U);
	return *std::prev(range) & 3U;
}

inline bool ParagraphIsRightToLeft(std::string_view paragraph, bool fallback) noexcept {
	size_t isolateDepth = 0;
	while (!paragraph.empty()) {
		const int classification = UTF8Classify(paragraph);
		if (classification & UTF8MaskInvalid) {
			paragraph.remove_prefix(1);
			continue;
		}
		const unsigned int character = static_cast<unsigned int>(UnicodeFromUTF8(paragraph));
		paragraph.remove_prefix(classification & UTF8MaskWidth);
		// UAX #9 P2: characters inside isolates do not determine the
		// containing paragraph's base direction, including nested isolates.
		if (character >= 0x2066 && character <= 0x2068) {
			++isolateDepth;
		} else if (character == 0x2069) {
			if (isolateDepth) {
				--isolateDepth;
			}
		} else if (!isolateDepth) {
			const unsigned int direction = StrongBidiDirection(character);
			if (direction) {
				return direction == 2;
			}
		}
	}
	return fallback;
}

}
#endif
