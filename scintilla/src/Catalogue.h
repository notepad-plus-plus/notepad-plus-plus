// Scintilla source code edit control
/** @file Catalogue.h
 ** Lexer infrastructure.
 ** Contains a list of LexerModules which can be searched to find a module appropriate for a
 ** particular language.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CATALOGUE_H
#define CATALOGUE_H

namespace Scintilla {

class Catalogue {
public:
	static const LexerModule *Find(int language);
	static const LexerModule *Find(const char *languageName);
	static void AddLexerModule(LexerModule *plm);
};

}

#endif
