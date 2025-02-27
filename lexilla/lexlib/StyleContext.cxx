// Scintilla source code edit control
/** @file StyleContext.cxx
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#include <cstdlib>
#include <cstdint>
#include <cassert>

#include <string>
#include <string_view>

#include "ILexer.h"

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"

using namespace Lexilla;

StyleContext::StyleContext(Sci_PositionU startPos, Sci_PositionU length,
	int initStyle, LexAccessor &styler_, char chMask) :
	styler(styler_),
	multiByteAccess((styler.Encoding() == EncodingType::eightBit) ? nullptr : styler.MultiByteAccess()),
	lengthDocument(static_cast<Sci_PositionU>(styler.Length())),
	endPos(((startPos + length) < lengthDocument) ? (startPos + length) : (lengthDocument+1)),
	lineDocEnd(styler.GetLine(lengthDocument)),
	currentPosLastRelative(SIZE_MAX),
	currentPos(startPos),
	currentLine(styler.GetLine(startPos)),
	lineEnd(styler.LineEnd(currentLine)),
	lineStartNext(styler.LineStart(currentLine + 1)),
	atLineStart(static_cast<Sci_PositionU>(styler.LineStart(currentLine)) == startPos),
	// Mask off all bits which aren't in the chMask.
	state(initStyle &chMask) {

	styler.StartAt(startPos /*, chMask*/);
	styler.StartSegment(startPos);

	chPrev = GetRelativeCharacter(-1);

	// Variable width is now 0 so GetNextChar gets the char at currentPos into chNext/widthNext
	GetNextChar();
	ch = chNext;
	width = widthNext;

	GetNextChar();
}

bool StyleContext::MatchIgnoreCase(const char *s) {
	if (MakeLowerCase(ch) != static_cast<unsigned char>(*s))
		return false;
	s++;
	if (!*s)
		return true;
	if (MakeLowerCase(chNext) != static_cast<unsigned char>(*s))
		return false;
	s++;
	for (int n = 2; *s; n++) {
		if (*s !=
			MakeLowerCase(styler.SafeGetCharAt(currentPos + n, 0)))
			return false;
		s++;
	}
	return true;
}

bool StyleContext::MatchIgnoreCase2(const char *s) {
	if (MakeLowerCase(ch) != MakeLowerCase(static_cast<unsigned char>(*s)))
		return false;
	s++;
	if (!*s)
		return true;
	if (MakeLowerCase(chNext) != MakeLowerCase(static_cast<unsigned char>(*s)))
		return false;
	s++;
	for (int n = 2; *s; n++) {
		if (MakeLowerCase(static_cast<unsigned char>(*s)) !=
			MakeLowerCase(static_cast<unsigned char>(styler.SafeGetCharAt(currentPos + n))))
			return false;
		s++;
	}
	return true;
}

void StyleContext::GetCurrent(char *s, Sci_PositionU len) const {
	styler.GetRange(styler.GetStartSegment(), currentPos, s, len);
}

void StyleContext::GetCurrentLowered(char *s, Sci_PositionU len) const {
	styler.GetRangeLowered(styler.GetStartSegment(), currentPos, s, len);
}

void StyleContext::GetCurrentString(std::string &string, Transform transform) const {
	const Sci_PositionU startPos = styler.GetStartSegment();
	const Sci_PositionU len = currentPos - styler.GetStartSegment();
	string.resize(len);
	if (transform == Transform::lower) {
		styler.GetRangeLowered(startPos, currentPos, string.data(), len + 1);
	} else {
		styler.GetRange(startPos, currentPos, string.data(), len + 1);
	}
}
