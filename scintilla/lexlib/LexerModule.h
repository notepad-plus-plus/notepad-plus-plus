// Scintilla source code edit control
/** @file LexerModule.h
 ** Colourise for particular languages.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERMODULE_H
#define LEXERMODULE_H

namespace Scintilla {

class Accessor;
class WordList;
struct LexicalClass;

typedef void (*LexerFunction)(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler);
typedef ILexer4 *(*LexerFactoryFunction)();

/**
 * A LexerModule is responsible for lexing and folding a particular language.
 * The Catalogue class maintains a list of LexerModules which can be searched to find a
 * module appropriate to a particular language.
 * The ExternalLexerModule subclass holds lexers loaded from DLLs or shared libraries.
 */
class LexerModule {
protected:
	int language;
	LexerFunction fnLexer;
	LexerFunction fnFolder;
	LexerFactoryFunction fnFactory;
	const char * const * wordListDescriptions;
	const LexicalClass *lexClasses;
	size_t nClasses;

public:
	const char *languageName;
	LexerModule(
		int language_,
		LexerFunction fnLexer_,
		const char *languageName_=nullptr,
		LexerFunction fnFolder_= nullptr,
		const char * const wordListDescriptions_[]=nullptr,
		const LexicalClass *lexClasses_=nullptr,
		size_t nClasses_=0);
	LexerModule(
		int language_,
		LexerFactoryFunction fnFactory_,
		const char *languageName_,
		const char * const wordListDescriptions_[]=nullptr);
	virtual ~LexerModule();
	int GetLanguage() const;

	// -1 is returned if no WordList information is available
	int GetNumWordLists() const;
	const char *GetWordListDescription(int index) const;
	const LexicalClass *LexClasses() const;
	size_t NamedStyles() const;

	ILexer4 *Create() const;

	virtual void Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;
	virtual void Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;

	friend class Catalogue;
};

inline int Maximum(int a, int b) {
	return (a > b) ? a : b;
}

// Shut up annoying Visual C++ warnings:
#ifdef _MSC_VER
#pragma warning(disable: 4244 4456 4457)
#endif

// Turn off shadow warnings for lexers as may be maintained by others
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wshadow"
#endif

}

#endif
