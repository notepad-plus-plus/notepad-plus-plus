// Scintilla source code edit control
/** @file LexerNoExceptions.cxx
 ** A simple lexer with no state which does not throw exceptions so can be used in an external lexer.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "LexerNoExceptions.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

Sci_Position SCI_METHOD LexerNoExceptions::PropertySet(const char *key, const char *val) {
	try {
		return LexerBase::PropertySet(key, val);
	} catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
	}
	return -1;
}

Sci_Position SCI_METHOD LexerNoExceptions::WordListSet(int n, const char *wl) {
	try {
		return LexerBase::WordListSet(n, wl);
	} catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
	}
	return -1;
}

void SCI_METHOD LexerNoExceptions::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	try {
		Accessor astyler(pAccess, &props);
		Lexer(startPos, length, initStyle, pAccess, astyler);
		astyler.Flush();
	} catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}
void SCI_METHOD LexerNoExceptions::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	try {
		Accessor astyler(pAccess, &props);
		Folder(startPos, length, initStyle, pAccess, astyler);
		astyler.Flush();
	} catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}
