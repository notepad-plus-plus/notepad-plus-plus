// Scintilla source code edit control
/** @file ExternalLexer.cxx
 ** Support external lexers in DLLs or shared libraries.
 **/
// Copyright 2001 Simon Steele <ss@pnotepad.org>, portions copyright Neil Hodgson.
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "LexerModule.h"
#include "Catalogue.h"
#include "ExternalLexer.h"

using namespace Scintilla;

#if PLAT_WIN
#define EXT_LEXER_DECL __stdcall
#else
#define EXT_LEXER_DECL
#endif

namespace {

int nextLanguage = SCLEX_AUTOMATIC + 1;

typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef LexerFactoryFunction(EXT_LEXER_DECL *GetLexerFactoryFunction)(unsigned int Index);

/// Generic function to convert from a void* to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
T FunctionPointer(void *function) noexcept {
	static_assert(sizeof(T) == sizeof(function));
	T fp;
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

/// Sub-class of LexerModule to use an external lexer.
class ExternalLexerModule : public LexerModule {
protected:
	GetLexerFactoryFunction fneFactory;
	std::string name;
public:
	ExternalLexerModule(int language_, LexerFunction fnLexer_,
		const char *languageName_=nullptr, LexerFunction fnFolder_=nullptr) :
		LexerModule(language_, fnLexer_, nullptr, fnFolder_),
		fneFactory(nullptr), name(languageName_){
		languageName = name.c_str();
	}
	void SetExternal(GetLexerFactoryFunction fFactory, int index) noexcept;
};

/// LexerLibrary exists for every External Lexer DLL, contains ExternalLexerModules.
class LexerLibrary {
	std::unique_ptr<DynamicLibrary> lib;
	std::vector<std::unique_ptr<ExternalLexerModule>> modules;
public:
	explicit LexerLibrary(const char *moduleName_);
	~LexerLibrary();

	std::string moduleName;
};

/// LexerManager manages external lexers, contains LexerLibrarys.
class LexerManager {
public:
	~LexerManager();

	static LexerManager *GetInstance();
	static void DeleteInstance() noexcept;

	void Load(const char *path);
	void Clear() noexcept;

private:
	LexerManager();
	static std::unique_ptr<LexerManager> theInstance;
	std::vector<std::unique_ptr<LexerLibrary>> libraries;
};

class LMMinder {
public:
	~LMMinder();
};

std::unique_ptr<LexerManager> LexerManager::theInstance;

//------------------------------------------
//
// ExternalLexerModule
//
//------------------------------------------

void ExternalLexerModule::SetExternal(GetLexerFactoryFunction fFactory, int index) noexcept {
	fneFactory = fFactory;
	fnFactory = fFactory(index);
}

//------------------------------------------
//
// LexerLibrary
//
//------------------------------------------

LexerLibrary::LexerLibrary(const char *moduleName_) {
	// Load the DLL
	lib.reset(DynamicLibrary::Load(moduleName_));
	if (lib->IsValid()) {
		moduleName = moduleName_;
		GetLexerCountFn GetLexerCount = FunctionPointer<GetLexerCountFn>(lib->FindFunction("GetLexerCount"));

		if (GetLexerCount) {
			// Find functions in the DLL
			GetLexerNameFn GetLexerName = FunctionPointer<GetLexerNameFn>(lib->FindFunction("GetLexerName"));
			GetLexerFactoryFunction fnFactory = FunctionPointer<GetLexerFactoryFunction>(lib->FindFunction("GetLexerFactory"));

			if (!GetLexerName || !fnFactory) {
				return;
			}

			const int nl = GetLexerCount();

			for (int i = 0; i < nl; i++) {
				// Assign a buffer for the lexer name.
				char lexname[100] = "";
				GetLexerName(i, lexname, sizeof(lexname));
				ExternalLexerModule *lex = new ExternalLexerModule(nextLanguage, nullptr, lexname, nullptr);
				nextLanguage++;

				// This is storing a second reference to lex in the Catalogue as well as in modules.
				// TODO: Should use std::shared_ptr or similar to ensure allocation safety.
				Catalogue::AddLexerModule(lex);

				// Remember ExternalLexerModule so we don't leak it
				modules.push_back(std::unique_ptr<ExternalLexerModule>(lex));

				// The external lexer needs to know how to call into its DLL to
				// do its lexing and folding, we tell it here.
				lex->SetExternal(fnFactory, i);
			}
		}
	}
}

LexerLibrary::~LexerLibrary() {
}

//------------------------------------------
//
// LexerManager
//
//------------------------------------------

/// Return the single LexerManager instance...
LexerManager *LexerManager::GetInstance() {
	if (!theInstance)
		theInstance.reset(new LexerManager);
	return theInstance.get();
}

/// Delete any LexerManager instance...
void LexerManager::DeleteInstance() noexcept {
	theInstance.reset();
}

/// protected constructor - this is a singleton...
LexerManager::LexerManager() {
}

LexerManager::~LexerManager() {
	Clear();
}

void LexerManager::Load(const char *path) {
	for (const std::unique_ptr<LexerLibrary> &ll : libraries) {
		if (ll->moduleName == path)
			return;
	}
	libraries.push_back(std::make_unique<LexerLibrary>(path));
}

void LexerManager::Clear() noexcept {
	libraries.clear();
}

//------------------------------------------
//
// LMMinder	-- trigger to clean up at exit.
//
//------------------------------------------

LMMinder::~LMMinder() {
	LexerManager::DeleteInstance();
}

LMMinder minder;

}

namespace Scintilla {

void ExternalLexerLoad(const char *path) {
	LexerManager::GetInstance()->Load(path);
}

}
