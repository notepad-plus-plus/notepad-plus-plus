// Scintilla source code edit control
/** @file StyleContext.cxx
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#include <cstdlib>
#include <cassert>

#include "ILexer.h"

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"

using namespace Scintilla;

bool StyleContext::MatchIgnoreCase(const char *s) {
	if (MakeLowerCase(ch) != static_cast<unsigned char>(*s))
		return false;
	s++;
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

static void getRange(Sci_PositionU start,
		Sci_PositionU end,
		LexAccessor &styler,
		char *s,
		Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = styler[start + i];
		i++;
	}
	s[i] = '\0';
}

void StyleContext::GetCurrent(char *s, Sci_PositionU len) {
	getRange(styler.GetStartSegment(), currentPos - 1, styler, s, len);
}

static void getRangeLowered(Sci_PositionU start,
		Sci_PositionU end,
		LexAccessor &styler,
		char *s,
		Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = MakeLowerCase(styler[start + i]);
		i++;
	}
	s[i] = '\0';
}

void StyleContext::GetCurrentLowered(char *s, Sci_PositionU len) {
	getRangeLowered(styler.GetStartSegment(), currentPos - 1, styler, s, len);
}
