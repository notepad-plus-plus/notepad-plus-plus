// Scintilla source code edit control
/** @file LexAccessor.cxx
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
#include <cassert>
#include <cstring>

#include <string>
#include <algorithm>

#include "ILexer.h"

#include "LexAccessor.h"
#include "CharacterSet.h"

using namespace Lexilla;

namespace Lexilla {

bool LexAccessor::MatchIgnoreCase(Sci_Position pos, const char *s) {
	assert(s);
	for (; *s; s++, pos++) {
		if (*s != MakeLowerCase(SafeGetCharAt(pos))) {
			return false;
		}
	}
	return true;
}

void LexAccessor::GetRange(Sci_PositionU startPos_, Sci_PositionU endPos_, char *s, Sci_PositionU len) {
	assert(s);
	assert(startPos_ <= endPos_ && len != 0);
	endPos_ = std::min(endPos_, startPos_ + len - 1);
	len = endPos_ - startPos_;
	if (startPos_ >= static_cast<Sci_PositionU>(startPos) && endPos_ <= static_cast<Sci_PositionU>(endPos)) {
		const char * const p = buf + (startPos_ - startPos);
		memcpy(s, p, len);
	} else {
		pAccess->GetCharRange(s, startPos_, len);
	}
	s[len] = '\0';
}

void LexAccessor::GetRangeLowered(Sci_PositionU startPos_, Sci_PositionU endPos_, char *s, Sci_PositionU len) {
	assert(s);
	GetRange(startPos_, endPos_, s, len);
	while (*s) {
		if (*s >= 'A' && *s <= 'Z') {
			*s += 'a' - 'A';
		}
		++s;
	}
}

std::string LexAccessor::GetRange(Sci_PositionU startPos_, Sci_PositionU endPos_) {
	assert(startPos_ < endPos_);
	const Sci_PositionU len = endPos_ - startPos_;
	std::string s(len, '\0');
	GetRange(startPos_, endPos_, s.data(), len + 1);
	return s;
}

std::string LexAccessor::GetRangeLowered(Sci_PositionU startPos_, Sci_PositionU endPos_) {
	assert(startPos_ < endPos_);
	const Sci_PositionU len = endPos_ - startPos_;
	std::string s(len, '\0');
	GetRangeLowered(startPos_, endPos_, s.data(), len + 1);
	return s;
}

}
