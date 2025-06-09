// Scintilla source code edit control
/** @file LexMake.cxx
 ** Lexer for make files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

// Options used for LexerMakeFile
struct OptionsMake {
};

const char *const makeWordListDescription[] = {
	"Directives",
	nullptr
};

struct OptionSetMake : public OptionSet<OptionsMake> {
	OptionSetMake() {
		DefineWordListSets(makeWordListDescription);
	}
};

const LexicalClass lexicalClasses[] = {
	// Lexer makefile SCLEX_MAKEFILE SCE_MAKE_
	0, "SCE_MAKE_DEFAULT", "default", "White space",
	1, "SCE_MAKE_COMMENT", "comment", "Comment",
	2, "SCE_MAKE_PREPROCESSOR", "preprocessor", "Preprocessor",
	3, "SCE_MAKE_IDENTIFIER", "identifier", "Identifiers",
	4, "SCE_MAKE_OPERATOR", "operator", "Operator",
	5, "SCE_MAKE_TARGET", "identifier", "Identifiers",
	9, "SCE_MAKE_IDEOL", "error identifier", "Incomplete identifier reference",
};

bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
		((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

class LexerMakeFile : public DefaultLexer {
	WordList directives;
	OptionsMake options;
	OptionSetMake osMake;
public:
	LexerMakeFile() :
		DefaultLexer("makefile", SCLEX_MAKEFILE, lexicalClasses, std::size(lexicalClasses)) {
	}

	const char *SCI_METHOD PropertyNames() override {
		return osMake.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osMake.PropertyType(name);
	}
	const char *SCI_METHOD DescribeProperty(const char *name) override {
		return osMake.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char *SCI_METHOD PropertyGet(const char *key) override {
		return osMake.PropertyGet(key);
	}
	const char *SCI_METHOD DescribeWordListSets() override {
		return osMake.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;

	void ColouriseMakeLine(std::string_view lineBuffer,
		Sci_PositionU startLine, Sci_PositionU endPos, Accessor &styler);

	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	static ILexer5 *LexerFactoryMakeFile() {
		return new LexerMakeFile();
	}
};

Sci_Position SCI_METHOD LexerMakeFile::PropertySet(const char *key, const char *val) {
	if (osMake.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerMakeFile::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &directives;
		break;
	default:
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN && wordListN->Set(wl)) {
		firstModification = 0;
	}
	return firstModification;
}

void LexerMakeFile::ColouriseMakeLine(
	const std::string_view lineBuffer,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
	Accessor &styler) {

	const Sci_PositionU lengthLine = lineBuffer.length();
	Sci_PositionU i = 0;
	Sci_Position lastNonSpace = -1;
	unsigned int state = SCE_MAKE_DEFAULT;
	bool bSpecial = false;

	// check for a tab character in column 0 indicating a command
	const bool bCommand = StartsWith(lineBuffer, '\t');

	// Skip initial spaces
	while ((i < lengthLine) && isspacechar(lineBuffer[i])) {
		i++;
	}
	if (i < lengthLine) {
		if (lineBuffer[i] == '#') {	// Comment
			styler.ColourTo(endPos, SCE_MAKE_COMMENT);
			return;
		}
		if (lineBuffer[i] == '!') {	// Special directive
			styler.ColourTo(endPos, SCE_MAKE_PREPROCESSOR);
			return;
		}
		if (IsUpperOrLowerCase(lineBuffer[i]) &&
			(lineBuffer.find_first_of(":=") == std::string::npos)) {
			const std::string_view firstWord(lineBuffer.substr(i));
			size_t endWord = 0;
			while ((endWord < firstWord.length()) && IsUpperOrLowerCase(firstWord[endWord])) {
				endWord++;
			}
			if (directives.InList(firstWord.substr(0, endWord))) {
				styler.ColourTo(startLine + i + endWord - 1, SCE_MAKE_PREPROCESSOR);
				i += endWord;
			}
		}
	}
	int varCount = 0;
	while (i < lengthLine) {
		if (lineBuffer.substr(i, 2) == "$(") {
			styler.ColourTo(startLine + i - 1, state);
			state = SCE_MAKE_IDENTIFIER;
			varCount++;
		} else if (state == SCE_MAKE_IDENTIFIER && lineBuffer[i] == ')') {
			if (--varCount == 0) {
				styler.ColourTo(startLine + i, state);
				state = SCE_MAKE_DEFAULT;
			}
		}

		// skip identifier and target styling if this is a command line
		if (!bSpecial && !bCommand && AnyOf(lineBuffer[i], ':', '=')) {
			// Three cases:
			// : target
			// := immediate assignment
			// = lazy assignment
			const bool colon = lineBuffer[i] == ':';
			const bool immediate = colon && ((i + 1) < lengthLine) && (lineBuffer[i + 1] == '=');
			const Sci_PositionU lengthOperator = immediate ? 1 : 0;
			if (lastNonSpace >= 0) {
				const int attribute = (colon && !immediate) ? SCE_MAKE_TARGET : SCE_MAKE_IDENTIFIER;
				styler.ColourTo(startLine + lastNonSpace, attribute);
			}
			styler.ColourTo(startLine + i - 1, SCE_MAKE_DEFAULT);
			styler.ColourTo(startLine + i + lengthOperator, SCE_MAKE_OPERATOR);
			bSpecial = true;	// Only react to the first ':' or '=' of the line
			state = SCE_MAKE_DEFAULT;
		}
		if (!isspacechar(lineBuffer[i])) {
			lastNonSpace = i;
		}
		i++;
	}
	if (state == SCE_MAKE_IDENTIFIER) {
		styler.ColourTo(endPos, SCE_MAKE_IDEOL);	// Error, variable reference not ended
	} else {
		styler.ColourTo(endPos, SCE_MAKE_DEFAULT);
	}
}

void LexerMakeFile::Lex(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, IDocument *pAccess) {
	Accessor styler(pAccess, nullptr);
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;
	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line (or of line buffer) met, colourise it
			ColouriseMakeLine(lineBuffer, startLine, i, styler);
			lineBuffer.clear();
			startLine = i + 1;
		}
	}
	if (!lineBuffer.empty()) {	// Last line does not have ending characters
		ColouriseMakeLine(lineBuffer, startLine, startPos + length - 1, styler);
	}
	styler.Flush();
}

}

extern const LexerModule lmMake(SCLEX_MAKEFILE, LexerMakeFile::LexerFactoryMakeFile, "makefile", makeWordListDescription);
