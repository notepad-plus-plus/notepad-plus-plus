// Lexilla lexer library use example
/** @file CheckLexilla.c
 ** Check that Lexilla.h works.
 **/
// Copyright 2021 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.
// If the public domain is not possible in your location then it can also be used under the same
// license as Scintilla. https://www.scintilla.org/License.txt

/* Build and run

    Win32
gcc CheckLexilla.c -I ../../include -o CheckLexilla
CheckLexilla
CheckLexilla ../SimpleLexer/SimpleLexer.dll

   Win32 Visual C++
cl CheckLexilla.c -I ../../include -Fe: CheckLexilla
CheckLexilla
CheckLexilla ../SimpleLexer/SimpleLexer.dll

    macOS
clang CheckLexilla.c -I ../../include -o CheckLexilla
./CheckLexilla
./CheckLexilla ../SimpleLexer/SimpleLexer.dylib

    Linux
gcc CheckLexilla.c -I ../../include -ldl -o CheckLexilla
./CheckLexilla
./CheckLexilla ../SimpleLexer/SimpleLexer.so

While principally meant for compilation as C to act as an example of using Lexilla
from C it can also be built as C++.

Warnings are intentionally shown for the deprecated typedef LexerNameFromIDFn when compiled with
GCC or Clang or as C++.

*/

#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if defined(__cplusplus)
#include "ILexer.h"
#endif

#include "Lexilla.h"

#if defined(__cplusplus)
using namespace Lexilla;
#endif

#if defined(_WIN32)
typedef FARPROC Function;
typedef HMODULE Module;
#else
typedef void *Function;
typedef void *Module;
#endif

static Function FindSymbol(Module m, const char *symbol) {
#if defined(_WIN32)
	return GetProcAddress(m, symbol);
#else
	return dlsym(m, symbol);
#endif
}

int main(int argc, char *argv[]) {
	char szLexillaPath[] = "../../bin/" LEXILLA_LIB LEXILLA_EXTENSION;
	const char *libPath = szLexillaPath;
	if (argc > 1) {
		libPath = argv[1];
	}
#if defined(_WIN32)
	Module lexillaLibrary = LoadLibraryA(libPath);
#else
	Module lexillaLibrary = dlopen(libPath, RTLD_LAZY);
#endif

	printf("Opened %s -> %p.\n", libPath, lexillaLibrary);
	if (lexillaLibrary) {
		GetLexerCountFn lexerCount = (GetLexerCountFn)FindSymbol(lexillaLibrary, LEXILLA_GETLEXERCOUNT);
		if (lexerCount) {
			int nLexers = lexerCount();
			printf("There are %d lexers.\n", nLexers);
			GetLexerNameFn lexerName = (GetLexerNameFn)FindSymbol(lexillaLibrary, LEXILLA_GETLEXERNAME);
			for (int i = 0; i < nLexers; i++) {
				char name[100] = "";
				lexerName(i, name, sizeof(name));
				printf("%s ", name);
			}
			printf("\n");

			GetLexerFactoryFn lexerFactory = (GetLexerFactoryFn)FindSymbol(lexillaLibrary, LEXILLA_GETLEXERFACTORY);
			LexerFactoryFunction lexerFactory4 = lexerFactory(4);	// 4th entry is "as" which is an object lexer so works
			printf("Lexer factory 4 -> %p.\n", lexerFactory4);

			CreateLexerFn lexerCreate = (CreateLexerFn)FindSymbol(lexillaLibrary, LEXILLA_CREATELEXER);
			ILexer5 *lexerCpp = lexerCreate("cpp");
			printf("Created cpp lexer -> %p.\n", lexerCpp);

			LexerNameFromIDFn lexerNameFromID = (LexerNameFromIDFn)FindSymbol(lexillaLibrary, LEXILLA_LEXERNAMEFROMID);
			if (lexerNameFromID) {
				const char *lexerNameCpp = lexerNameFromID(3);	// SCLEX_CPP=3
				if (lexerNameCpp) {
					printf("Lexer name 3 -> %s.\n", lexerNameCpp);
				} else {
					printf("Lexer name 3 not available.\n");
				}
			} else {
				printf("Lexer name from ID not supported.\n");
			}

			GetLibraryPropertyNamesFn libraryProperties = (GetLibraryPropertyNamesFn)FindSymbol(lexillaLibrary, LEXILLA_GETLIBRARYPROPERTYNAMES);
			if (libraryProperties) {
				const char *names = libraryProperties();
				printf("Property names '%s'.\n", names);
			} else {
				printf("Property names not supported.\n");
			}

			SetLibraryPropertyFn librarySetProperty = (SetLibraryPropertyFn)FindSymbol(lexillaLibrary, LEXILLA_SETLIBRARYPROPERTY);
			if (librarySetProperty) {
				librarySetProperty("key", "value");
			} else {
				printf("Set property not supported.\n");
			}

			GetNameSpaceFn libraryNameSpace = (GetLibraryPropertyNamesFn)FindSymbol(lexillaLibrary, LEXILLA_GETNAMESPACE);
			if (libraryNameSpace) {
				const char *nameSpace = libraryNameSpace();
				printf("Name space '%s'.\n", nameSpace);
			} else {
				printf("Name space not supported.\n");
			}
		}
	}
}
