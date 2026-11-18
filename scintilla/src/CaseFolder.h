// Scintilla source code edit control
/** @file CaseFolder.h
 ** Classes for case folding.
 **/
// Copyright 1998-2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CASEFOLDER_H
#define CASEFOLDER_H

namespace Scintilla::Internal {

class CaseFolder {
public:
	CaseFolder() = default;
	// Deleted so CaseFolder objects can not be copied.
	CaseFolder(const CaseFolder &source) = delete;
	CaseFolder(CaseFolder &&) = delete;
	CaseFolder &operator=(const CaseFolder &) = delete;
	CaseFolder &operator=(CaseFolder &&) = delete;
	virtual ~CaseFolder() = default;
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) = 0;
};

class CaseFolderTable : public CaseFolder {
protected:
	char mapping[256];
public:
	CaseFolderTable() noexcept;
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override;
	void SetTranslation(char ch, char chTranslation) noexcept;
	void StandardASCII() noexcept;
};

class ICaseConverter;

class CaseFolderUnicode : public CaseFolderTable {
	ICaseConverter *converter;
public:
	CaseFolderUnicode();
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override;
};

}

#endif
