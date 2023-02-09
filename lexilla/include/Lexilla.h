// Lexilla lexer library
/** @file Lexilla.h
 ** Lexilla definitions for dynamic and static linking.
 ** For C++, more features and type safety are available with the LexillaAccess module.
 **/
// Copyright 2020 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXILLA_H
#define LEXILLA_H

// Define the default Lexilla shared library name for each platform
#if defined(_WIN32)
#define LEXILLA_LIB "lexilla"
#define LEXILLA_EXTENSION ".dll"
#else
#define LEXILLA_LIB "liblexilla"
#if defined(__APPLE__)
#define LEXILLA_EXTENSION ".dylib"
#else
#define LEXILLA_EXTENSION ".so"
#endif
#endif

// On Win32 use the stdcall calling convention otherwise use the standard calling convention
#if defined(_WIN32)
#define LEXILLA_CALL __stdcall
#else
#define LEXILLA_CALL
#endif

#if defined(__OBJC2__)
// Objective C(++) treats '[' as a message expression.
#define DEPRECATE_DEFINITION
#elif defined(__cplusplus)
#define DEPRECATE_DEFINITION [[deprecated]]
#elif defined(__GNUC__) || defined(__clang__)
#define DEPRECATE_DEFINITION __attribute__((deprecated))
#else
// MSVC __declspec(deprecated) has different positioning rules to GCC so define to nothing
#define DEPRECATE_DEFINITION
#endif

#if defined(__cplusplus)
// Must have already included ILexer.h to have Scintilla::ILexer5 defined.
using Scintilla::ILexer5;
#else
typedef void ILexer5;
#endif

typedef ILexer5 *(*LexerFactoryFunction)();

#if defined(__cplusplus)
namespace Lexilla {
#endif

typedef int (LEXILLA_CALL *GetLexerCountFn)();
typedef void (LEXILLA_CALL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef LexerFactoryFunction(LEXILLA_CALL *GetLexerFactoryFn)(unsigned int Index);
typedef ILexer5*(LEXILLA_CALL *CreateLexerFn)(const char *name);
DEPRECATE_DEFINITION typedef const char *(LEXILLA_CALL *LexerNameFromIDFn)(int identifier);
typedef const char *(LEXILLA_CALL *GetLibraryPropertyNamesFn)();
typedef void(LEXILLA_CALL *SetLibraryPropertyFn)(const char *key, const char *value);
typedef const char *(LEXILLA_CALL *GetNameSpaceFn)();

#if defined(__cplusplus)
}
#endif

#define LEXILLA_NAMESPACE_SEPARATOR '.'

#define LEXILLA_GETLEXERCOUNT "GetLexerCount"
#define LEXILLA_GETLEXERNAME "GetLexerName"
#define LEXILLA_GETLEXERFACTORY "GetLexerFactory"
#define LEXILLA_CREATELEXER "CreateLexer"
#define LEXILLA_LEXERNAMEFROMID "LexerNameFromID"
#define LEXILLA_GETLIBRARYPROPERTYNAMES "GetLibraryPropertyNames"
#define LEXILLA_SETLIBRARYPROPERTY "SetLibraryProperty"
#define LEXILLA_GETNAMESPACE "GetNameSpace"

// Static linking prototypes

#if defined(__cplusplus)
extern "C" {
#endif

ILexer5 * LEXILLA_CALL CreateLexer(const char *name);
int LEXILLA_CALL GetLexerCount();
void LEXILLA_CALL GetLexerName(unsigned int index, char *name, int buflength);
LexerFactoryFunction LEXILLA_CALL GetLexerFactory(unsigned int index);
DEPRECATE_DEFINITION const char *LEXILLA_CALL LexerNameFromID(int identifier);
const char * LEXILLA_CALL GetLibraryPropertyNames();
void LEXILLA_CALL SetLibraryProperty(const char *key, const char *value);
const char *LEXILLA_CALL GetNameSpace();

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
namespace Lexilla {
	class LexerModule;
}
// Add a static lexer (in the same binary) to Lexilla's list
void AddStaticLexerModule(Lexilla::LexerModule *plm);
#endif

#endif
