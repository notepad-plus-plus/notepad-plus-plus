// Scintilla source code edit control
/** @file LexerBase.cxx
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

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

using namespace Lexilla;

static const char styleSubable[] = { 0 };

LexerBase::LexerBase(const LexicalClass *lexClasses_, size_t nClasses_) :
	lexClasses(lexClasses_), nClasses(nClasses_) {
	for (int wl = 0; wl < numWordLists; wl++)
		keyWordLists[wl] = new WordList;
	keyWordLists[numWordLists] = nullptr;
}

LexerBase::~LexerBase() {
	for (int wl = 0; wl < numWordLists; wl++) {
		delete keyWordLists[wl];
		keyWordLists[wl] = nullptr;
	}
	keyWordLists[numWordLists] = nullptr;
}

void SCI_METHOD LexerBase::Release() {
	delete this;
}

int SCI_METHOD LexerBase::Version() const {
	return Scintilla::lvRelease5;
}

const char * SCI_METHOD LexerBase::PropertyNames() {
	return "";
}

int SCI_METHOD LexerBase::PropertyType(const char *) {
	return SC_TYPE_BOOLEAN;
}

const char * SCI_METHOD LexerBase::DescribeProperty(const char *) {
	return "";
}

Sci_Position SCI_METHOD LexerBase::PropertySet(const char *key, const char *val) {
	if (props.Set(key, val)) {
		return 0;
	} else {
		return -1;
	}
}

const char *SCI_METHOD LexerBase::PropertyGet(const char *key) {
	return props.Get(key);
}

const char * SCI_METHOD LexerBase::DescribeWordListSets() {
	return "";
}

Sci_Position SCI_METHOD LexerBase::WordListSet(int n, const char *wl) {
	if (n < numWordLists) {
		if (keyWordLists[n]->Set(wl)) {
			return 0;
		}
	}
	return -1;
}

void * SCI_METHOD LexerBase::PrivateCall(int, void *) {
	return nullptr;
}

int SCI_METHOD LexerBase::LineEndTypesSupported() {
	return SC_LINE_END_TYPE_DEFAULT;
}

int SCI_METHOD LexerBase::AllocateSubStyles(int, int) {
	return -1;
}

int SCI_METHOD LexerBase::SubStylesStart(int) {
	return -1;
}

int SCI_METHOD LexerBase::SubStylesLength(int) {
	return 0;
}

int SCI_METHOD LexerBase::StyleFromSubStyle(int subStyle) {
	return subStyle;
}

int SCI_METHOD LexerBase::PrimaryStyleFromStyle(int style) {
	return style;
}

void SCI_METHOD LexerBase::FreeSubStyles() {
}

void SCI_METHOD LexerBase::SetIdentifiers(int, const char *) {
}

int SCI_METHOD LexerBase::DistanceToSecondaryStyles() {
	return 0;
}

const char * SCI_METHOD LexerBase::GetSubStyleBases() {
	return styleSubable;
}

int SCI_METHOD LexerBase::NamedStyles() {
	return static_cast<int>(nClasses);
}

const char * SCI_METHOD LexerBase::NameOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].name : "";
}

const char * SCI_METHOD LexerBase::TagsOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].tags : "";
}

const char * SCI_METHOD LexerBase::DescriptionOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].description : "";
}

// ILexer5 methods

const char *SCI_METHOD LexerBase::GetName() {
	return "";
}

int SCI_METHOD LexerBase::GetIdentifier() {
	return SCLEX_AUTOMATIC;
}
