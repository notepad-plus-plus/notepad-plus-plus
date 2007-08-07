// Scintilla source code edit control
/** @file KeyWords.h
 ** Colourise for particular languages.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

typedef void (*LexerFunction)(unsigned int startPos, int lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler);
                  
/**
 * A LexerModule is responsible for lexing and folding a particular language.
 * The class maintains a list of LexerModules which can be searched to find a
 * module appropriate to a particular language.
 */
class LexerModule {
protected:
	const LexerModule *next;
	int language;
	LexerFunction fnLexer;
	LexerFunction fnFolder;
	const char * const * wordListDescriptions;
	int styleBits;

	static const LexerModule *base;
	static int nextLanguage;

public:
	const char *languageName;
	LexerModule(int language_, 
		LexerFunction fnLexer_, 
		const char *languageName_=0, 
		LexerFunction fnFolder_=0,
		const char * const wordListDescriptions_[] = NULL,
		int styleBits_=5);
	virtual ~LexerModule() {
	}
	int GetLanguage() const { return language; }

	// -1 is returned if no WordList information is available
	int GetNumWordLists() const;
	const char *GetWordListDescription(int index) const;

	int GetStyleBitsNeeded() const;

	virtual void Lex(unsigned int startPos, int lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;
	virtual void Fold(unsigned int startPos, int lengthDoc, int initStyle,
                  WordList *keywordlists[], Accessor &styler) const;
	static const LexerModule *Find(int language);
	static const LexerModule *Find(const char *languageName);
};

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
inline bool isspacechar(unsigned char ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

inline bool iswordchar(char ch) {
	return isascii(ch) && (isalnum(ch) || ch == '.' || ch == '_');
}

inline bool iswordstart(char ch) {
	return isascii(ch) && (isalnum(ch) || ch == '_');
}

inline bool isoperator(char ch) {
	if (isascii(ch) && isalnum(ch))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
	        ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
	        ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
	        ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
	        ch == '?' || ch == '!' || ch == '.' || ch == '~')
		return true;
	return false;
}
