// SciTE - Scintilla based Text Editor
/** @file LexillaAccess.h
 ** Interface to loadable lexers.
 ** This does not depend on SciTE code so can be copied out into other projects.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXILLAACCESS_H
#define LEXILLAACCESS_H

namespace Lexilla {

// Directory to load default Lexilla from, commonly the directory of the application.
void SetDefaultDirectory(std::string_view directory);

// Specify CreateLexer when statically linked so no hard dependency in LexillaAccess
// so it doesn't have to be built in two forms - static and dynamic.
void SetDefault(CreateLexerFn pCreate) noexcept;

// sharedLibraryPaths is a ';' separated list of shared libraries to load.
// On Win32 it is treated as UTF-8 and on Unix it is passed to dlopen directly.
// Return true if any shared libraries are loaded.
bool Load(std::string_view sharedLibraryPaths);

Scintilla::ILexer5 *MakeLexer(std::string_view languageName);

std::vector<std::string> Lexers();
[[deprecated]] std::string NameFromID(int identifier);
std::vector<std::string> LibraryProperties();
void SetProperty(const char *key, const char *value);

}

#endif
