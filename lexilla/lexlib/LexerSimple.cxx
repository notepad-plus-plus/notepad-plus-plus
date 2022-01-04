// Scintilla source code edit control
/** @file LexerSimple.cxx
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include <string>
#include <string_view>

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

using namespace Lexilla;

LexerSimple::LexerSimple(const LexerModule *module_) :
	LexerBase(module_->LexClasses(), module_->NamedStyles()),
	module(module_) {
	for (int wl = 0; wl < module->GetNumWordLists(); wl++) {
		if (!wordLists.empty())
			wordLists += "\n";
		wordLists += module->GetWordListDescription(wl);
	}
}

const char * SCI_METHOD LexerSimple::DescribeWordListSets() {
	return wordLists.c_str();
}

void SCI_METHOD LexerSimple::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess) {
	Accessor astyler(pAccess, &props);
	module->Lex(startPos, lengthDoc, initStyle, keyWordLists, astyler);
	astyler.Flush();
}

void SCI_METHOD LexerSimple::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess) {
	if (props.GetInt("fold")) {
		Accessor astyler(pAccess, &props);
		module->Fold(startPos, lengthDoc, initStyle, keyWordLists, astyler);
		astyler.Flush();
	}
}

const char * SCI_METHOD LexerSimple::GetName() {
	return module->languageName;
}

int SCI_METHOD LexerSimple::GetIdentifier() {
	return module->GetLanguage();
}
