// Scintilla source code edit control
/** @file DefaultLexer.cxx
 ** A lexer base class that provides reasonable default behaviour.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "DefaultLexer.h"

using namespace Scintilla;

static const char styleSubable[] = { 0 };

DefaultLexer::DefaultLexer(const char *languageName_, int language_,
	const LexicalClass *lexClasses_, size_t nClasses_) :
	languageName(languageName_),
	language(language_),
	lexClasses(lexClasses_),
	nClasses(nClasses_) {
}

DefaultLexer::~DefaultLexer() {
}

void SCI_METHOD DefaultLexer::Release() {
	delete this;
}

int SCI_METHOD DefaultLexer::Version() const {
	return lvRelease5;
}

const char * SCI_METHOD DefaultLexer::PropertyNames() {
	return "";
}

int SCI_METHOD DefaultLexer::PropertyType(const char *) {
	return SC_TYPE_BOOLEAN;
}

const char * SCI_METHOD DefaultLexer::DescribeProperty(const char *) {
	return "";
}

Sci_Position SCI_METHOD DefaultLexer::PropertySet(const char *, const char *) {
	return -1;
}

const char * SCI_METHOD DefaultLexer::DescribeWordListSets() {
	return "";
}

Sci_Position SCI_METHOD DefaultLexer::WordListSet(int, const char *) {
	return -1;
}

void SCI_METHOD DefaultLexer::Fold(Sci_PositionU, Sci_Position, int, IDocument *) {
}

void * SCI_METHOD DefaultLexer::PrivateCall(int, void *) {
	return nullptr;
}

int SCI_METHOD DefaultLexer::LineEndTypesSupported() {
	return SC_LINE_END_TYPE_DEFAULT;
}

int SCI_METHOD DefaultLexer::AllocateSubStyles(int, int) {
	return -1;
}

int SCI_METHOD DefaultLexer::SubStylesStart(int) {
	return -1;
}

int SCI_METHOD DefaultLexer::SubStylesLength(int) {
	return 0;
}

int SCI_METHOD DefaultLexer::StyleFromSubStyle(int subStyle) {
	return subStyle;
}

int SCI_METHOD DefaultLexer::PrimaryStyleFromStyle(int style) {
	return style;
}

void SCI_METHOD DefaultLexer::FreeSubStyles() {
}

void SCI_METHOD DefaultLexer::SetIdentifiers(int, const char *) {
}

int SCI_METHOD DefaultLexer::DistanceToSecondaryStyles() {
	return 0;
}

const char * SCI_METHOD DefaultLexer::GetSubStyleBases() {
	return styleSubable;
}

int SCI_METHOD DefaultLexer::NamedStyles() {
	return static_cast<int>(nClasses);
}

const char * SCI_METHOD DefaultLexer::NameOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].name : "";
}

const char * SCI_METHOD DefaultLexer::TagsOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].tags : "";
}

const char * SCI_METHOD DefaultLexer::DescriptionOfStyle(int style) {
	return (style < NamedStyles()) ? lexClasses[style].description : "";
}

// ILexer5 methods
const char * SCI_METHOD DefaultLexer::GetName() {
	return languageName;
}

int SCI_METHOD DefaultLexer::GetIdentifier() {
	return language;
}

