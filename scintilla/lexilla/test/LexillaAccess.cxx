// Scintilla source code edit control
/** @file LexillaAccess.cxx
 ** Interface to Lexilla shared library.
 **/
 // Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.

#include <cstring>

#include <iterator>

#include <iostream>
#include <filesystem>

#if !_WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "ILexer.h"

#include "Lexilla.h"

#include "LexillaAccess.h"

#if _WIN32
#define EXT_LEXER_DECL __stdcall
typedef FARPROC Function;
typedef HMODULE Module;
#else
#define EXT_LEXER_DECL
typedef void *Function;
typedef void *Module;
#endif

typedef Scintilla::ILexer5 *(EXT_LEXER_DECL *CreateLexerFn)(const char *name);

Module lexillaDL {};

/// Generic function to convert from a Function(void* or FARPROC) to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
T FunctionPointer(Function function) noexcept {
	static_assert(sizeof(T) == sizeof(function));
	T fp;
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

CreateLexerFn fnCL;

Function FindSymbol(const char *symbol) noexcept {
#if _WIN32
	return ::GetProcAddress(lexillaDL, symbol);
#else
	return dlsym(lexillaDL, symbol);
#endif
}

Scintilla::ILexer5 *CreateLexer(std::string languageName) {
#ifdef LEXILLA_STATIC
	return CreateLexer(languageName.c_str());
#else
	return fnCL(languageName.c_str());
#endif
}

bool LoadLexilla([[maybe_unused]] std::filesystem::path path) {
#ifdef LEXILLA_STATIC
	return true;
#else
	std::filesystem::path sharedLibrary = path;
	sharedLibrary.append("bin");
#if _WIN32
	sharedLibrary.append("lexilla.dll");
	lexillaDL = ::LoadLibraryW(sharedLibrary.c_str());
#else
#if defined(__APPLE__)
	sharedLibrary.append("liblexilla.dylib");
#else
	sharedLibrary.append("liblexilla.so");
#endif
	lexillaDL = dlopen(sharedLibrary.c_str(), RTLD_LAZY);
#endif

	if (lexillaDL) {
		fnCL = FunctionPointer<CreateLexerFn>(FindSymbol("CreateLexer"));
	} else {
		std::cout << "Cannot load " << sharedLibrary.string() << "\n";
	}

	return lexillaDL && fnCL;
#endif
}
