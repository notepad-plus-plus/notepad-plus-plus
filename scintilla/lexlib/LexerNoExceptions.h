// Scintilla source code edit control
/** @file LexerNoExceptions.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERNOEXCEPTIONS_H
#define LEXERNOEXCEPTIONS_H

namespace Scintilla {

// A simple lexer with no state
class LexerNoExceptions : public LexerBase {
public:
	// TODO Also need to prevent exceptions in constructor and destructor
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *) override;

	virtual void Lexer(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess, Accessor &styler) = 0;
	virtual void Folder(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess, Accessor &styler) = 0;
};

}

#endif
