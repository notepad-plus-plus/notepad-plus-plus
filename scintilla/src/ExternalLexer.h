// Scintilla source code edit control
/** @file ExternalLexer.h
 ** Support external lexers in DLLs.
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

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

typedef void*(EXT_LEXER_DECL *GetLexerFunction)(unsigned int Index);
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
		const char *languageName_=0, LexerFunction fnFolder_=0) :
		LexerModule(language_, fnLexer_, 0, fnFolder_),
		fneFactory(0), name(languageName_){
		languageName = name.c_str();
	}
	virtual void SetExternal(GetLexerFactoryFunction fFactory, int index);
};

/// LexerMinder points to an ExternalLexerModule - so we don't leak them.
class LexerMinder {
public:
	ExternalLexerModule *self;
	LexerMinder *next;
};

/// LexerLibrary exists for every External Lexer DLL, contains LexerMinders.
class LexerLibrary {
	DynamicLibrary	*lib;
	LexerMinder		*first;
	LexerMinder		*last;

public:
	explicit LexerLibrary(const char *ModuleName);
	~LexerLibrary();
	void Release();

	LexerLibrary	*next;
	std::string			m_sModuleName;
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
	static LexerManager *theInstance;

	void LoadLexerLibrary(const char *module);
	LexerLibrary *first;
	LexerLibrary *last;
};

class LMMinder {
public:
	~LMMinder();
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
