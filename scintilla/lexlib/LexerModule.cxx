// Scintilla source code edit control
/** @file LexerModule.cxx
 ** Colourise for particular languages.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include <string>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "LexerSimple.h"

using namespace Scintilla;

LexerModule::LexerModule(int language_,
	LexerFunction fnLexer_,
	const char *languageName_,
	LexerFunction fnFolder_,
	const char *const wordListDescriptions_[],
	const LexicalClass *lexClasses_,
	size_t nClasses_) :
	language(language_),
	fnLexer(fnLexer_),
	fnFolder(fnFolder_),
	fnFactory(nullptr),
	wordListDescriptions(wordListDescriptions_),
	lexClasses(lexClasses_),
	nClasses(nClasses_),
	languageName(languageName_) {
}

LexerModule::LexerModule(int language_,
	LexerFactoryFunction fnFactory_,
	const char *languageName_,
	const char * const wordListDescriptions_[]) :
	language(language_),
	fnLexer(nullptr),
	fnFolder(nullptr),
	fnFactory(fnFactory_),
	wordListDescriptions(wordListDescriptions_),
	lexClasses(nullptr),
	nClasses(0),
	languageName(languageName_) {
}

LexerModule::~LexerModule() {
}

int LexerModule::GetLanguage() const { 
	return language;
}

int LexerModule::GetNumWordLists() const {
	if (!wordListDescriptions) {
		return -1;
	} else {
		int numWordLists = 0;

		while (wordListDescriptions[numWordLists]) {
			++numWordLists;
		}

		return numWordLists;
	}
}

const char *LexerModule::GetWordListDescription(int index) const {
	assert(index < GetNumWordLists());
	if (!wordListDescriptions || (index >= GetNumWordLists())) {
		return "";
	} else {
		return wordListDescriptions[index];
	}
}

const LexicalClass *LexerModule::LexClasses() const {
	return lexClasses;
}

size_t LexerModule::NamedStyles() const {
	return nClasses;
}

ILexer4 *LexerModule::Create() const {
	if (fnFactory)
		return fnFactory();
	else
		return new LexerSimple(this);
}

void LexerModule::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
	  WordList *keywordlists[], Accessor &styler) const {
	if (fnLexer)
		fnLexer(startPos, lengthDoc, initStyle, keywordlists, styler);
}

void LexerModule::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
	  WordList *keywordlists[], Accessor &styler) const {
	if (fnFolder) {
		Sci_Position lineCurrent = styler.GetLine(startPos);
		// Move back one line in case deletion wrecked current line fold state
		if (lineCurrent > 0) {
			lineCurrent--;
			const Sci_Position newStartPos = styler.LineStart(lineCurrent);
			lengthDoc += startPos - newStartPos;
			startPos = newStartPos;
			initStyle = 0;
			if (startPos > 0) {
				initStyle = styler.StyleAt(startPos - 1);
			}
		}
		fnFolder(startPos, lengthDoc, initStyle, keywordlists, styler);
	}
}
