// Scintilla source code edit control
/** @file CaseFolder.h
 ** Classes for case folding.
 **/
// Copyright 1998-2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CASEFOLDER_H
#define CASEFOLDER_H

namespace Scintilla {

class CaseFolder {
public:
	virtual ~CaseFolder();
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) = 0;
};

class CaseFolderTable : public CaseFolder {
protected:
	char mapping[256];
public:
	CaseFolderTable();
	~CaseFolderTable() override;
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override;
	void SetTranslation(char ch, char chTranslation);
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
