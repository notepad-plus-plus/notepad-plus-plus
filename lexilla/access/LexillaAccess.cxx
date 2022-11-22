// SciTE - Scintilla based Text Editor
/** @file LexillaAccess.cxx
 ** Interface to loadable lexers.
 ** Maintains a list of lexer library paths and CreateLexer functions.
 ** If list changes then load all the lexer libraries and find the functions.
 ** When asked to create a lexer, call each function until one succeeds.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstring>

#include <string>
#include <string_view>
#include <vector>
#include <set>

#if !_WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "ILexer.h"

#include "Lexilla.h"

#include "LexillaAccess.h"

namespace {

#if _WIN32
typedef FARPROC Function;
typedef HMODULE Module;
constexpr const char *pathSeparator = "\\";
#else
typedef void *Function;
typedef void *Module;
constexpr const char *pathSeparator = "/";
#endif

/// Generic function to convert from a Function(void* or FARPROC) to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
T FunctionPointer(Function function) noexcept {
	static_assert(sizeof(T) == sizeof(function));
	T fp {};
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

#if _WIN32

std::wstring WideStringFromUTF8(std::string_view sv) {
	const int sLength = static_cast<int>(sv.length());
	const int cchWide = ::MultiByteToWideChar(CP_UTF8, 0, sv.data(), sLength, nullptr, 0);
	std::wstring sWide(cchWide, 0);
	::MultiByteToWideChar(CP_UTF8, 0, sv.data(), sLength, sWide.data(), cchWide);
	return sWide;
}

#endif

// Turn off deprecation checks as LexillaAccess deprecates its wrapper over
// the deprecated LexerNameFromID. Thus use within LexillaAccess is intentional.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
#pragma warning(disable: 4996)
#endif

std::string directoryLoadDefault;
std::string lastLoaded;

struct LexLibrary {
	Lexilla::CreateLexerFn fnCL;
	Lexilla::LexerNameFromIDFn fnLNFI;
	Lexilla::GetLibraryPropertyNamesFn fnGLPN;
	Lexilla::SetLibraryPropertyFn fnSLP;
	std::string nameSpace;
};
std::vector<LexLibrary> libraries;

std::vector<std::string> lexers;
std::vector<std::string> libraryProperties;

Function FindSymbol(Module m, const char *symbol) noexcept {
#if _WIN32
	return ::GetProcAddress(m, symbol);
#else
	return dlsym(m, symbol);
#endif
}

Lexilla::CreateLexerFn pCreateLexerDefault = nullptr;

bool NameContainsDot(std::string_view path) noexcept {
	for (std::string_view::const_reverse_iterator it = path.crbegin();
	     it != path.crend(); ++it) {
		if (*it == '.')
			return true;
		if (*it == '/' || *it == '\\')
			return false;
	}
	return false;
}

constexpr bool HasPrefix(std::string_view s, std::string_view prefix) noexcept {
	return (s.size() >= prefix.size()) && (prefix == s.substr(0, prefix.size()));
}

}

void Lexilla::SetDefault(CreateLexerFn pCreate) noexcept {
	pCreateLexerDefault = pCreate;
}

void Lexilla::SetDefaultDirectory(std::string_view directory) {
	directoryLoadDefault = directory;
}

bool Lexilla::Load(std::string_view sharedLibraryPaths) {
	if (sharedLibraryPaths == lastLoaded) {
		return !libraries.empty();
	}

	std::string_view paths = sharedLibraryPaths;
	lexers.clear();

	libraries.clear();
	while (!paths.empty()) {
		const size_t separator = paths.find_first_of(';');
		std::string path(paths.substr(0, separator));
		if (separator == std::string::npos) {
			paths.remove_prefix(paths.size());
		} else {
			paths.remove_prefix(separator + 1);
		}
		if (path == ".") {
			if (directoryLoadDefault.empty()) {
				path = "";
			} else {
				path = directoryLoadDefault;
				path += pathSeparator;
			}
			path += LEXILLA_LIB;
		}
		if (!NameContainsDot(path)) {
			// No '.' in name so add extension
			path.append(LEXILLA_EXTENSION);
		}
#if _WIN32
		// Convert from UTF-8 to wide characters
		std::wstring wsPath = WideStringFromUTF8(path);
		Module lexillaDL = ::LoadLibraryW(wsPath.c_str());
#else
		Module lexillaDL = dlopen(path.c_str(), RTLD_LAZY);
#endif
		if (lexillaDL) {
			GetLexerCountFn fnLexerCount = FunctionPointer<GetLexerCountFn>(
				FindSymbol(lexillaDL, LEXILLA_GETLEXERCOUNT));
			GetLexerNameFn fnLexerName = FunctionPointer<GetLexerNameFn>(
				FindSymbol(lexillaDL, LEXILLA_GETLEXERNAME));
			if (fnLexerCount && fnLexerName) {
				const int nLexers = fnLexerCount();
				for (int i = 0; i < nLexers; i++) {
					char name[100] = "";
					fnLexerName(i, name, sizeof(name));
					lexers.push_back(name);
				}
			}
			CreateLexerFn fnCL = FunctionPointer<CreateLexerFn>(
				FindSymbol(lexillaDL, LEXILLA_CREATELEXER));
			LexerNameFromIDFn fnLNFI = FunctionPointer<LexerNameFromIDFn>(
				FindSymbol(lexillaDL, LEXILLA_LEXERNAMEFROMID));
			GetLibraryPropertyNamesFn fnGLPN = FunctionPointer<GetLibraryPropertyNamesFn>(
				FindSymbol(lexillaDL, LEXILLA_GETLIBRARYPROPERTYNAMES));
			SetLibraryPropertyFn fnSLP = FunctionPointer<SetLibraryPropertyFn>(
				FindSymbol(lexillaDL, LEXILLA_SETLIBRARYPROPERTY));
			GetNameSpaceFn fnGNS = FunctionPointer<GetNameSpaceFn>(
				FindSymbol(lexillaDL, LEXILLA_GETNAMESPACE));
			std::string nameSpace;
			if (fnGNS) {
				nameSpace = fnGNS();
				nameSpace += LEXILLA_NAMESPACE_SEPARATOR;
			}
			LexLibrary lexLib {
				fnCL,
				fnLNFI,
				fnGLPN,
				fnSLP,
				nameSpace
			};
			libraries.push_back(lexLib);
		}
	}
	lastLoaded = sharedLibraryPaths;

	std::set<std::string> nameSet;
	for (const LexLibrary &lexLib : libraries) {
		if (lexLib.fnGLPN) {
			const char *cpNames = lexLib.fnGLPN();
			if (cpNames) {
				std::string_view names = cpNames;
				while (!names.empty()) {
					const size_t separator = names.find_first_of('\n');
					std::string name(names.substr(0, separator));
					nameSet.insert(name);
					if (separator == std::string::npos) {
						names.remove_prefix(names.size());
					} else {
						names.remove_prefix(separator + 1);
					}
				}
			}
		}
	}
	// Standard Lexilla does not have any properties so can't be added to set.
	libraryProperties = std::vector<std::string>(nameSet.begin(), nameSet.end());

	return !libraries.empty();
}

Scintilla::ILexer5 *Lexilla::MakeLexer(std::string_view languageName) {
	std::string sLanguageName(languageName);	// Ensure NUL-termination
	// First, try to match namespace then name suffix
	for (const LexLibrary &lexLib : libraries) {
		if (lexLib.fnCL && !lexLib.nameSpace.empty()) {
			if (HasPrefix(languageName, lexLib.nameSpace)) {
				Scintilla::ILexer5 *pLexer = lexLib.fnCL(sLanguageName.substr(lexLib.nameSpace.size()).c_str());
				if (pLexer) {
					return pLexer;
				}
			}
		}
	}
	// If no match with namespace, try to just match name
	for (const LexLibrary &lexLib : libraries) {
		if (lexLib.fnCL) {
			Scintilla::ILexer5 *pLexer = lexLib.fnCL(sLanguageName.c_str());
			if (pLexer) {
				return pLexer;
			}
		}
	}
	if (pCreateLexerDefault) {
		return pCreateLexerDefault(sLanguageName.c_str());
	}
#ifdef LEXILLA_STATIC
	Scintilla::ILexer5 *pLexer = CreateLexer(sLanguageName.c_str());
	if (pLexer) {
		return pLexer;
	}
#endif
	return nullptr;
}

std::vector<std::string> Lexilla::Lexers() {
	return lexers;
}

std::string Lexilla::NameFromID(int identifier) {
	for (const LexLibrary &lexLib : libraries) {
		if (lexLib.fnLNFI) {
			const char *name = lexLib.fnLNFI(identifier);
			if (name) {
				return name;
			}
		}
	}
	return std::string();
}

std::vector<std::string> Lexilla::LibraryProperties() {
	return libraryProperties;
}

void Lexilla::SetProperty(const char *key, const char *value) {
	for (const LexLibrary &lexLib : libraries) {
		if (lexLib.fnSLP) {
			lexLib.fnSLP(key, value);
		}
	}
	// Standard Lexilla does not have any properties so don't set.
}
