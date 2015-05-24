// Scintilla source code edit control
/** @file CaseFolder.cxx
 ** Classes for case folding.
 **/
// Copyright 1998-2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <vector>
#include <algorithm>

#include "CaseConvert.h"
#include "UniConversion.h"
#include "CaseFolder.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

CaseFolder::~CaseFolder() {
}

CaseFolderTable::CaseFolderTable() {
	for (size_t iChar=0; iChar<sizeof(mapping); iChar++) {
		mapping[iChar] = static_cast<char>(iChar);
	}
}

CaseFolderTable::~CaseFolderTable() {
}

size_t CaseFolderTable::Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
	if (lenMixed > sizeFolded) {
		return 0;
	} else {
		for (size_t i=0; i<lenMixed; i++) {
			folded[i] = mapping[static_cast<unsigned char>(mixed[i])];
		}
		return lenMixed;
	}
}

void CaseFolderTable::SetTranslation(char ch, char chTranslation) {
	mapping[static_cast<unsigned char>(ch)] = chTranslation;
}

void CaseFolderTable::StandardASCII() {
	for (size_t iChar=0; iChar<sizeof(mapping); iChar++) {
		if (iChar >= 'A' && iChar <= 'Z') {
			mapping[iChar] = static_cast<char>(iChar - 'A' + 'a');
		} else {
			mapping[iChar] = static_cast<char>(iChar);
		}
	}
}

CaseFolderUnicode::CaseFolderUnicode() {
	StandardASCII();
	converter = ConverterFor(CaseConversionFold);
}

size_t CaseFolderUnicode::Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
	if ((lenMixed == 1) && (sizeFolded > 0)) {
		folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
		return 1;
	} else {
		return converter->CaseConvertString(folded, sizeFolded, mixed, lenMixed);
	}
}
