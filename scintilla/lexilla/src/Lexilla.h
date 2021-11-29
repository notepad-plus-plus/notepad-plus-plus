// Scintilla source code edit control
/** @file Lexilla.h
 ** Lexer infrastructure.
 ** Declare functions in Lexilla library.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#if _WIN32
#define LEXILLA_CALLING_CONVENTION __stdcall
#else
#define LEXILLA_CALLING_CONVENTION
#endif

extern "C" {

Scintilla::ILexer5 * LEXILLA_CALLING_CONVENTION CreateLexer(const char *name);

}
