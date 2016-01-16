// Scintilla source code edit control
/** @file LexerModule.h
 ** Colourise for particular languages.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERMODULE_H
#define LEXERMODULE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class Accessor;
class WordList;

typedef void (*LexerFunction)(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler);
typedef ILexer *(*LexerFactoryFunction)();

/**
 * A LexerModule is responsible for lexing and folding a particular language.
 * The class maintains a list of LexerModules which can be searched to find a
 * module appropriate to a particular language.
 */
class LexerModule {
protected:
	int language;
	LexerFunction fnLexer;
	LexerFunction fnFolder;
	LexerFactoryFunction fnFactory;
	const char * const * wordListDescriptions;

public:
	const char *languageName;
	LexerModule(int language_,
		LexerFunction fnLexer_,
		const char *languageName_=0,
		LexerFunction fnFolder_=0,
		const char * const wordListDescriptions_[] = NULL);
	LexerModule(int language_,
		LexerFactoryFunction fnFactory_,
		const char *languageName_,
		const char * const wordListDescriptions_[] = NULL);
	virtual ~LexerModule() {
	}
	int GetLanguage() const { return language; }

	// -1 is returned if no WordList information is available
	int GetNumWordLists() const;
	const char *GetWordListDescription(int index) const;

	ILexer *Create() const;

	virtual void Lex(Sci_PositionU startPos, Sci_Position length, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;
	virtual void Fold(Sci_PositionU startPos, Sci_Position length, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;

	friend class Catalogue;
};

inline int Maximum(int a, int b) {
	return (a > b) ? a : b;
}

// Shut up annoying Visual C++ warnings:
#ifdef _MSC_VER
#pragma warning(disable: 4244 4309 4456 4457)
#endif

// Turn off shadow warnings for lexers as may be maintained by others
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#ifdef SCI_NAMESPACE
}
#endif

#endif
