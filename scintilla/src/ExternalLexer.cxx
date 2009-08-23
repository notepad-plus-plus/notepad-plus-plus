// Scintilla source code edit control
/** @file ExternalLexer.cxx
 ** Support external lexers in DLLs.
 **/
// Copyright 2001 Simon Steele <ss@pnotepad.org>, portions copyright Neil Hodgson.
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <string>

#include "Platform.h"

#include "Scintilla.h"

#include "SciLexer.h"
#include "PropSet.h"
#include "Accessor.h"
#include "DocumentAccessor.h"
#include "KeyWords.h"
#include "ExternalLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

LexerManager *LexerManager::theInstance = NULL;

//------------------------------------------
//
// ExternalLexerModule
//
//------------------------------------------

char **WordListsToStrings(WordList *val[]) {
	int dim = 0;
	while (val[dim])
		dim++;
	char **wls = new char * [dim + 1];
	for (int i = 0;i < dim;i++) {
		std::string words;
		words = "";
		for (int n = 0; n < val[i]->len; n++) {
			words += val[i]->words[n];
			if (n != val[i]->len - 1)
				words += " ";
		}
		wls[i] = new char[words.length() + 1];
		strcpy(wls[i], words.c_str());
	}
	wls[dim] = 0;
	return wls;
}

void DeleteWLStrings(char *strs[]) {
	int dim = 0;
	while (strs[dim]) {
		delete strs[dim];
		dim++;
	}
	delete [] strs;
}

void ExternalLexerModule::Lex(unsigned int startPos, int lengthDoc, int initStyle,
                              WordList *keywordlists[], Accessor &styler) const {
	if (!fneLexer)
		return ;

	char **kwds = WordListsToStrings(keywordlists);
	char *ps = styler.GetProperties();

	// The accessor passed in is always a DocumentAccessor so this cast and the subsequent
	// access will work. Can not use the stricter dynamic_cast as that requires RTTI.
	DocumentAccessor &da = static_cast<DocumentAccessor &>(styler);
	WindowID wID = da.GetWindow();

	fneLexer(externalLanguage, startPos, lengthDoc, initStyle, kwds, wID, ps);

	delete ps;
	DeleteWLStrings(kwds);
}

void ExternalLexerModule::Fold(unsigned int startPos, int lengthDoc, int initStyle,
                               WordList *keywordlists[], Accessor &styler) const {
	if (!fneFolder)
		return ;

	char **kwds = WordListsToStrings(keywordlists);
	char *ps = styler.GetProperties();

	// The accessor passed in is always a DocumentAccessor so this cast and the subsequent
	// access will work. Can not use the stricter dynamic_cast as that requires RTTI.
	DocumentAccessor &da = static_cast<DocumentAccessor &>(styler);
	WindowID wID = da.GetWindow();

	fneFolder(externalLanguage, startPos, lengthDoc, initStyle, kwds, wID, ps);

	delete ps;
	DeleteWLStrings(kwds);
}

void ExternalLexerModule::SetExternal(ExtLexerFunction fLexer, ExtFoldFunction fFolder, int index) {
	fneLexer = fLexer;
	fneFolder = fFolder;
	externalLanguage = index;
}

//------------------------------------------
//
// LexerLibrary
//
//------------------------------------------

LexerLibrary::LexerLibrary(const char* ModuleName) {
	// Initialise some members...
	first = NULL;
	last = NULL;

	// Load the DLL
	lib = DynamicLibrary::Load(ModuleName);
	if (lib->IsValid()) {
		m_sModuleName = ModuleName;
		//Cannot use reinterpret_cast because: ANSI C++ forbids casting between pointers to functions and objects
		GetLexerCountFn GetLexerCount = (GetLexerCountFn)(sptr_t)lib->FindFunction("GetLexerCount");

		if (GetLexerCount) {
			ExternalLexerModule *lex;
			LexerMinder *lm;

			// Find functions in the DLL
			GetLexerNameFn GetLexerName = (GetLexerNameFn)(sptr_t)lib->FindFunction("GetLexerName");
			ExtLexerFunction Lexer = (ExtLexerFunction)(sptr_t)lib->FindFunction("Lex");
			ExtFoldFunction Folder = (ExtFoldFunction)(sptr_t)lib->FindFunction("Fold");

			// Assign a buffer for the lexer name.
			char lexname[100];
			strcpy(lexname, "");

			int nl = GetLexerCount();

			for (int i = 0; i < nl; i++) {
				GetLexerName(i, lexname, 100);
				lex = new ExternalLexerModule(SCLEX_AUTOMATIC, NULL, lexname, NULL);

				// Create a LexerMinder so we don't leak the ExternalLexerModule...
				lm = new LexerMinder;
				lm->self = lex;
				lm->next = NULL;
				if (first != NULL) {
					last->next = lm;
					last = lm;
				} else {
					first = lm;
					last = lm;
				}

				// The external lexer needs to know how to call into its DLL to
				// do its lexing and folding, we tell it here. Folder may be null.
				lex->SetExternal(Lexer, Folder, i);
			}
		}
	}
	next = NULL;
}

LexerLibrary::~LexerLibrary() {
	Release();
	delete lib;
}

void LexerLibrary::Release() {
	//TODO maintain a list of lexers created, and delete them!
	LexerMinder *lm;
	LexerMinder *lmNext;
	lm = first;
	while (NULL != lm) {
		lmNext = lm->next;
		delete lm->self;
		delete lm;
		lm = lmNext;
	}

	first = NULL;
	last = NULL;
}

//------------------------------------------
//
// LexerManager
//
//------------------------------------------

/// Return the single LexerManager instance...
LexerManager *LexerManager::GetInstance() {
	if(!theInstance)
		theInstance = new LexerManager;
	return theInstance;
}

/// Delete any LexerManager instance...
void LexerManager::DeleteInstance()
{
	if(theInstance) {
		delete theInstance;
		theInstance = NULL;
	}
}

/// protected constructor - this is a singleton...
LexerManager::LexerManager() {
	first = NULL;
	last = NULL;
}

LexerManager::~LexerManager() {
	Clear();
}

void LexerManager::Load(const char* path)
{
	LoadLexerLibrary(path);
}

void LexerManager::LoadLexerLibrary(const char* module)
{
	LexerLibrary *lib = new LexerLibrary(module);
	if (NULL != first) {
		last->next = lib;
		last = lib;
	} else {
		first = lib;
		last = lib;
	}
}

void LexerManager::Clear()
{
	if (NULL != first) {
		LexerLibrary *cur = first;
		LexerLibrary *next;
		while (cur) {
			next = cur->next;
			delete cur;
			cur = next;
		}
		first = NULL;
		last = NULL;
	}
}

//------------------------------------------
//
// LexerManager
//
//------------------------------------------

LMMinder::~LMMinder()
{
	LexerManager::DeleteInstance();
}

LMMinder minder;
