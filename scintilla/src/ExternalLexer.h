// Scintilla source code edit control
/** @file ExternalLexer.h
 ** Support external lexers in DLLs or shared libraries.
 **/
// Copyright 2001 Simon Steele <ss@pnotepad.org>, portions copyright Neil Hodgson.
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EXTERNALLEXER_H
#define EXTERNALLEXER_H

#if PLAT_WIN
#define EXT_LEXER_DECL __stdcall
#else
#define EXT_LEXER_DECL
#endif

namespace Scintilla {

typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(unsigned int Index, char *name, int buflength);
typedef LexerFactoryFunction(EXT_LEXER_DECL *GetLexerFactoryFunction)(unsigned int Index);

/// Sub-class of LexerModule to use an external lexer.
class ExternalLexerModule : public LexerModule {
protected:
	GetLexerFactoryFunction fneFactory;
	std::string name;
public:
	ExternalLexerModule(int language_, LexerFunction fnLexer_,
		const char *languageName_=nullptr, LexerFunction fnFolder_=nullptr) :
		LexerModule(language_, fnLexer_, nullptr, fnFolder_),
		fneFactory(nullptr), name(languageName_){
		languageName = name.c_str();
	}
	virtual void SetExternal(GetLexerFactoryFunction fFactory, int index);
};

/// LexerLibrary exists for every External Lexer DLL, contains ExternalLexerModules.
class LexerLibrary {
	std::unique_ptr<DynamicLibrary> lib;
	std::vector<std::unique_ptr<ExternalLexerModule>> modules;
public:
	explicit LexerLibrary(const char *moduleName_);
	~LexerLibrary();

	std::string moduleName;
};

/// LexerManager manages external lexers, contains LexerLibrarys.
class LexerManager {
public:
	~LexerManager();

	static LexerManager *GetInstance();
	static void DeleteInstance();

	void Load(const char *path);
	void Clear();

private:
	LexerManager();
	static std::unique_ptr<LexerManager> theInstance;
	std::vector<std::unique_ptr<LexerLibrary>> libraries;
};

class LMMinder {
public:
	~LMMinder();
};

}

#endif
