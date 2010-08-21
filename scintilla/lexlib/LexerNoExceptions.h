// Scintilla source code edit control
/** @file LexerNoExceptions.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LexerNoExceptions_H
#define LexerNoExceptions_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// A simple lexer with no state
class LexerNoExceptions : public LexerBase {
public:
	// TODO Also need to prevent exceptions in constructor and destructor
	int SCI_METHOD PropertySet(const char *key, const char *val);
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *);

	virtual void Lexer(unsigned int startPos, int length, int initStyle, IDocument *pAccess, Accessor &styler) = 0;
	virtual void Folder(unsigned int startPos, int length, int initStyle, IDocument *pAccess, Accessor &styler) = 0;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
