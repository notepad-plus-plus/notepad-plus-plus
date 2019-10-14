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

std::unique_ptr<LexerManager> LexerManager::theInstance;

//------------------------------------------
//
// ExternalLexerModule
//
//------------------------------------------

void ExternalLexerModule::SetExternal(GetLexerFactoryFunction fFactory, int index) {
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
		//Cannot use reinterpret_cast because: ANSI C++ forbids casting between pointers to functions and objects
		GetLexerCountFn GetLexerCount = (GetLexerCountFn)(sptr_t)lib->FindFunction("GetLexerCount");

		if (GetLexerCount) {
			// Find functions in the DLL
			GetLexerNameFn GetLexerName = (GetLexerNameFn)(sptr_t)lib->FindFunction("GetLexerName");
			GetLexerFactoryFunction fnFactory = (GetLexerFactoryFunction)(sptr_t)lib->FindFunction("GetLexerFactory");

			const int nl = GetLexerCount();

			for (int i = 0; i < nl; i++) {
				// Assign a buffer for the lexer name.
				char lexname[100] = "";
				GetLexerName(i, lexname, sizeof(lexname));
				ExternalLexerModule *lex = new ExternalLexerModule(SCLEX_AUTOMATIC, nullptr, lexname, nullptr);
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
void LexerManager::DeleteInstance() {
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

void LexerManager::Clear() {
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
