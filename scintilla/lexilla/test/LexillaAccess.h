// Scintilla source code edit control
/** @file LexillaAccess.h
 ** Interface to Lexilla shared library.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

Scintilla::ILexer5 *CreateLexer(std::string languageName);
bool LoadLexilla(std::filesystem::path path);
