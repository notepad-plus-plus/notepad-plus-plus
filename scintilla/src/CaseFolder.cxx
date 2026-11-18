// Scintilla source code edit control
/** @file CaseFolder.cxx
 ** Classes for case folding.
 **/
// Copyright 1998-2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdexcept>
#include <vector>
#include <algorithm>

#include "CharacterType.h"
#include "CaseFolder.h"
#include "CaseConvert.h"

using namespace Scintilla::Internal;

namespace {

constexpr unsigned char IndexFromChar(char ch) {
	return static_cast<unsigned char>(ch);
}

}

CaseFolderTable::CaseFolderTable() noexcept : mapping{}  {
	StandardASCII();
}

size_t CaseFolderTable::Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
	if (lenMixed > sizeFolded) {
		return 0;
	}
	for (size_t i=0; i<lenMixed; i++) {
		folded[i] = mapping[IndexFromChar(mixed[i])];
	}
	return lenMixed;
}

void CaseFolderTable::SetTranslation(char ch, char chTranslation) noexcept {
	mapping[IndexFromChar(ch)] = chTranslation;
}

void CaseFolderTable::StandardASCII() noexcept {
	for (size_t iChar=0; iChar<std::size(mapping); iChar++) {
		mapping[iChar] = static_cast<char>(MakeLowerCase(iChar));
	}
}

CaseFolderUnicode::CaseFolderUnicode() {
	converter = ConverterFor(CaseConversion::fold);
}

size_t CaseFolderUnicode::Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
	if ((lenMixed == 1) && (sizeFolded > 0)) {
		folded[0] = mapping[IndexFromChar(mixed[0])];
		return 1;
	} else {
		return converter->CaseConvertString(folded, sizeFolded, mixed, lenMixed);
	}
}
