// Scintilla source code edit control
/** @file LexerBase.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERBASE_H
#define LEXERBASE_H

namespace Scintilla {

// A simple lexer with no state
class LexerBase : public ILexer5 {
protected:
	const LexicalClass *lexClasses;
	size_t nClasses;
	PropSetSimple props;
	enum {numWordLists=KEYWORDSET_MAX+1};
	WordList *keyWordLists[numWordLists+1];
public:
	LexerBase(const LexicalClass *lexClasses_=nullptr, size_t nClasses_=0);
	virtual ~LexerBase();
	void SCI_METHOD Release() override;
	int SCI_METHOD Version() const override;
	const char * SCI_METHOD PropertyNames() override;
	int SCI_METHOD PropertyType(const char *name) override;
	const char * SCI_METHOD DescribeProperty(const char *name) override;
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD DescribeWordListSets() override;
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) override = 0;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) override = 0;
	void * SCI_METHOD PrivateCall(int operation, void *pointer) override;
	int SCI_METHOD LineEndTypesSupported() override;
	int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override;
	int SCI_METHOD SubStylesStart(int styleBase) override;
	int SCI_METHOD SubStylesLength(int styleBase) override;
	int SCI_METHOD StyleFromSubStyle(int subStyle) override;
	int SCI_METHOD PrimaryStyleFromStyle(int style) override;
	void SCI_METHOD FreeSubStyles() override;
	void SCI_METHOD SetIdentifiers(int style, const char *identifiers) override;
	int SCI_METHOD DistanceToSecondaryStyles() override;
	const char * SCI_METHOD GetSubStyleBases() override;
	int SCI_METHOD NamedStyles() override;
	const char * SCI_METHOD NameOfStyle(int style) override;
	const char * SCI_METHOD TagsOfStyle(int style) override;
	const char * SCI_METHOD DescriptionOfStyle(int style) override;
	// ILexer5 methods
	const char * SCI_METHOD GetName() override;
	int SCI_METHOD GetIdentifier() override;
	const char *SCI_METHOD PropertyGet(const char *key) override;
};

}

#endif
