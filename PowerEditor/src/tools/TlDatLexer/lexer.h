#ifndef TLDATLEXER_LEXER_H
#define TLDATLEXER_LEXER_H

#include <ILexer.h>

typedef ILexer* (*LexerFactoryFunction)();

class TlDatLexer: public ILexer {
public:
	static const char* name;
	static const wchar_t* wname;
	static const std::size_t nameLen = 5;
	static const wchar_t* statusText;

	static ILexer* factory() { return new(std::nothrow) TlDatLexer; }

	TlDatLexer(): doneOnce_(false) {}

	int SCI_METHOD Version() const override { return lvOriginal; }
	void SCI_METHOD Release() override { delete this; }
	const char* SCI_METHOD PropertyNames() override { return ""; }
	int SCI_METHOD PropertyType(const char*) override { return 0; }
	const char* SCI_METHOD DescribeProperty(const char*) override { return ""; } 
	int SCI_METHOD PropertySet(const char*, const char*) override { return 0; }
	const char* SCI_METHOD DescribeWordListSets() override { return ""; }
	int SCI_METHOD WordListSet(int, const char*) override { return 0; }
	void* SCI_METHOD PrivateCall(int, void*) override { return 0; }

	void SCI_METHOD Lex(unsigned int, int, int, IDocument*) override;
	void SCI_METHOD Fold(unsigned int, int, int, IDocument*) override;

private:
	// The whole document needs to be lexed for tag matching to work.
	bool doneOnce_;
};

void matchTags(unsigned int pos, sptr_t scintilla, SciFnDirect message);

namespace TextStyle {
	enum: int {
		default_,
		openTag,
		closeTag,
		fieldType,
		fieldName,
		fieldValue,
		fieldValueBool,
		fieldValueNumber,
		fieldValueNote
	};
}

namespace IndicatorStyle {
	enum {
		error = 0
	};
}

#endif
