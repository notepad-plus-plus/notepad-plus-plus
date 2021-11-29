// Scintilla source code edit control
/** @file ExternalLexer.h
 ** Support external lexers in DLLs or shared libraries.
 **/
// Copyright 2001 Simon Steele <ss@pnotepad.org>, portions copyright Neil Hodgson.
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EXTERNALLEXER_H
#define EXTERNALLEXER_H

namespace Scintilla {

void ExternalLexerLoad(const char *path);

}

#endif
