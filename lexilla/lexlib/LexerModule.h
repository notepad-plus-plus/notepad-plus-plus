// Scintilla source code edit control
/** @file LexerModule.h
 ** Colourise for particular languages.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERMODULE_H
#define LEXERMODULE_H

namespace Lexilla {

class Accessor;
class WordList;
struct LexicalClass;

using LexerFunction = void (*)(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler);
using LexerFactoryFunction = Scintilla::ILexer5 *(*)();

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
		size_t nClasses_=0) noexcept;
	LexerModule(
		int language_,
		LexerFactoryFunction fnFactory_,
		const char *languageName_,
		const char * const wordListDescriptions_[]=nullptr) noexcept;
	[[nodiscard]] int GetLanguage() const noexcept;

	// -1 is returned if no WordList information is available
	[[nodiscard]] int GetNumWordLists() const noexcept;
	[[nodiscard]] const char *GetWordListDescription(int index) const noexcept;
	[[nodiscard]] const LexicalClass *LexClasses() const noexcept;
	[[nodiscard]] size_t NamedStyles() const noexcept;

	[[nodiscard]] Scintilla::ILexer5 *Create() const;

	void Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;
	void Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;

	friend class CatalogueModules;
};

constexpr int Maximum(int a, int b) noexcept {
	return (a > b) ? a : b;
}

// Shut up annoying Visual C++ warnings:
#if defined(_MSC_VER)
#pragma warning(disable: 4244 4456 4457)
#endif

// Turn off shadow warnings for lexers as may be maintained by others
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wshadow"
#endif

// Clang doesn't like omitting braces in array initialization but they just add
// noise to LexicalClass arrays in lexers
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

}

#endif
