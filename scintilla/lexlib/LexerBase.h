// Scintilla source code edit control
/** @file LexerBase.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERBASE_H
#define LEXERBASE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// A simple lexer with no state
class LexerBase : public ILexer {
protected:
	PropSetSimple props;
	enum {numWordLists=KEYWORDSET_MAX+1};
	WordList *keyWordLists[numWordLists+1];
public:
	LexerBase();
	virtual ~LexerBase();
	void SCI_METHOD Release();
	int SCI_METHOD Version() const;
	const char * SCI_METHOD PropertyNames();
	int SCI_METHOD PropertyType(const char *name);
	const char * SCI_METHOD DescribeProperty(const char *name);
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets();
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) = 0;
	void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) = 0;
	void * SCI_METHOD PrivateCall(int operation, void *pointer);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
