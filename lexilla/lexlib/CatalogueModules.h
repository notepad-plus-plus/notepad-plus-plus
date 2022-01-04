// Scintilla source code edit control
/** @file CatalogueModules.h
 ** Lexer infrastructure.
 ** Contains a list of LexerModules which can be searched to find a module appropriate for a
 ** particular language.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CATALOGUEMODULES_H
#define CATALOGUEMODULES_H

namespace Lexilla {

class CatalogueModules {
	std::vector<LexerModule *> lexerCatalogue;
public:
	const LexerModule *Find(int language) const {
		for (const LexerModule *lm : lexerCatalogue) {
			if (lm->GetLanguage() == language) {
				return lm;
			}
		}
		return nullptr;
	}

	const LexerModule *Find(const char *languageName) const noexcept {
		if (languageName) {
			for (const LexerModule *lm : lexerCatalogue) {
				if (lm->languageName && (0 == strcmp(lm->languageName, languageName))) {
					return lm;
				}
			}
		}
		return nullptr;
	}

	void AddLexerModule(LexerModule *plm) {
		lexerCatalogue.push_back(plm);
	}

	void AddLexerModules(std::initializer_list<LexerModule *> modules) {
		lexerCatalogue.insert(lexerCatalogue.end(), modules);
	}

	unsigned int Count() const noexcept {
		return static_cast<unsigned int>(lexerCatalogue.size());
	}

	const char *Name(unsigned int index) const noexcept {
		if (index < static_cast<unsigned int>(lexerCatalogue.size())) {
			return lexerCatalogue[index]->languageName;
		} else {
			return "";
		}
	}

	LexerFactoryFunction Factory(unsigned int index) const noexcept {
		// Works for object lexers but not for function lexers
		return lexerCatalogue[index]->fnFactory;
	}

	Scintilla::ILexer5 *Create(unsigned int index) const {
		const LexerModule *plm = lexerCatalogue[index];
		if (!plm) {
			return nullptr;
		}
		return plm->Create();
	}
};

}

#endif
