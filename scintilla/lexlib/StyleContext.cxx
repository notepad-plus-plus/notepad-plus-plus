// Scintilla source code edit control
/** @file StyleContext.cxx
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

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
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

void StyleContext::GetCurrentLowered(char *s, Sci_PositionU len) {
	getRangeLowered(styler.GetStartSegment(), currentPos - 1, styler, s, len);
}
