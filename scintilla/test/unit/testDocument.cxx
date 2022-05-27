/** @file testDocument.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "ScintillaTypes.h"

#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"

#include "CharacterCategoryMap.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test Document.

struct Folding {
	int from;
	int to;
	int length;
};

// Table of case folding for non-ASCII bytes in Windows Latin code page 1252
Folding foldings1252[] = {
	{0x8a, 0x9a, 0x01},
	{0x8c, 0x9c, 0x01},
	{0x8e, 0x9e, 0x01},
	{0x9f, 0xff, 0x01},
	{0xc0, 0xe0, 0x17},
	{0xd8, 0xf8, 0x07},
};

// Table of case folding for non-ASCII bytes in Windows Russian code page 1251
Folding foldings1251[] = {
	{0x80, 0x90, 0x01},
	{0x81, 0x83, 0x01},
	{0x8a, 0x9a, 0x01},
	{0x8c, 0x9c, 0x04},
	{0xa1, 0xa2, 0x01},
	{0xa3, 0xbc, 0x01},
	{0xa5, 0xb4, 0x01},
	{0xa8, 0xb8, 0x01},
	{0xaa, 0xba, 0x01},
	{0xaf, 0xbf, 0x01},
	{0xb2, 0xb3, 0x01},
	{0xbd, 0xbe, 0x01},
	{0xc0, 0xe0, 0x20},
};

std::string ReadFile(std::string path) {
	std::ifstream ifs(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}

struct DocPlus {
	Document document;

	DocPlus(std::string_view svInitial, int codePage) : document(DocumentOption::Default) {
		SetCodePage(codePage);
		document.InsertString(0, svInitial.data(), svInitial.length());
	}

	void SetCodePage(int codePage) {
		document.SetDBCSCodePage(codePage);
		if (codePage == CpUtf8) {
			document.SetCaseFolder(std::make_unique<CaseFolderUnicode>());
		} else {
			// This case folder will not handle many DBCS cases. Scintilla uses platform-specific code for DBCS
			// case folding which can not easily be inserted in platform-independent tests.
			std::unique_ptr<CaseFolderTable> pcft = std::make_unique<CaseFolderTable>();
			document.SetCaseFolder(std::move(pcft));
		}
	}

	void SetSBCSFoldings(const Folding *foldings, size_t length) {
		std::unique_ptr<CaseFolderTable> pcft = std::make_unique<CaseFolderTable>();
		for (size_t block = 0; block < length; block++) {
			for (int fold = 0; fold < foldings[block].length; fold++) {
				pcft->SetTranslation(foldings[block].from + fold, foldings[block].to + fold);
			}
		}
		document.SetCaseFolder(std::move(pcft));
	}

	Sci::Position FindNeedle(const std::string &needle, FindOption options, Sci::Position *length) {
		assert(*length == static_cast<Sci::Position>(needle.length()));
		return document.FindText(0, document.Length(), needle.c_str(), options, length);
	}
	Sci::Position FindNeedleReverse(const std::string &needle, FindOption options, Sci::Position *length) {
		assert(*length == static_cast<Sci::Position>(needle.length()));
		return document.FindText(document.Length(), 0, needle.c_str(), options, length);
	}
	void MoveGap(Sci::Position gapNew) {
		// Move gap to gapNew by inserting
		document.InsertString(gapNew, "!", 1);
		// Remove insertion
		document.DeleteChars(gapNew, 1);
	}
};

void TimeTrace(std::string_view sv, const Catch::Timer &tikka) {
	std::cout << sv << std::setw(5) << tikka.getElapsedMilliseconds() << " milliseconds" << std::endl;
}

TEST_CASE("Document") {

	const char sText[] = "Scintilla";
	const Sci::Position sLength = static_cast<Sci::Position>(strlen(sText));

	SECTION("InsertOneLine") {
		DocPlus doc("", 0);
		const Sci::Position length = doc.document.InsertString(0, sText, sLength);
		REQUIRE(sLength == doc.document.Length());
		REQUIRE(length == sLength);
		REQUIRE(1 == doc.document.LinesTotal());
		REQUIRE(0 == doc.document.LineStart(0));
		REQUIRE(0 == doc.document.LineFromPosition(0));
		REQUIRE(sLength == doc.document.LineStart(1));
		REQUIRE(0 == doc.document.LineFromPosition(static_cast<int>(sLength)));
		REQUIRE(doc.document.CanUndo());
		REQUIRE(!doc.document.CanRedo());
	}

	// Search ranges are from first argument to just before second argument
	// Arguments are expected to be at character boundaries and will be tweaked if
	// part way through a character.
	SECTION("SearchInLatin") {
		DocPlus doc("abcde", 0);	// a b c d e
		std::string finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.FindNeedleReverse(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 2, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("SearchInBothSegments") {
		DocPlus doc("ab-ab", 0);	// a b - a b
		std::string finding = "ab";
		for (int gapPos = 0; gapPos <= 5; gapPos++) {
			doc.MoveGap(gapPos);
			Sci::Position lengthFinding = finding.length();
			Sci::Position location = doc.document.FindText(0, doc.document.Length(), finding.c_str(), FindOption::MatchCase, &lengthFinding);
			REQUIRE(location == 0);
			location = doc.document.FindText(2, doc.document.Length(), finding.c_str(), FindOption::MatchCase, &lengthFinding);
			REQUIRE(location == 3);
		}
	}

	SECTION("InsensitiveSearchInLatin") {
		DocPlus doc("abcde", 0);	// a b c d e
		std::string finding = "B";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 2, finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("InsensitiveSearchIn1252") {
		// In Windows Latin, code page 1252, C6 is AE and E6 is ae
		DocPlus doc("tru\xc6s\xe6t", 0);	// t r u AE s ae t
		doc.SetSBCSFoldings(foldings1252, std::size(foldings1252));

		// Search for upper-case AE
		std::string finding = "\xc6";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(4, doc.document.Length(), finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 5);

		// Search for lower-case ae
		finding = "\xe6";
		location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(4, doc.document.Length(), finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
	}

	SECTION("Search2InLatin") {
		// Checks that the initial '_' and final 'f' are ignored since they are outside the search bounds
		DocPlus doc("_abcdef", 0);	// _ a b c d e f
		std::string finding = "cd";
		Sci::Position lengthFinding = finding.length();
		size_t docLength = doc.document.Length() - 1;
		Sci::Position location = doc.document.FindText(1, docLength, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(docLength, 1, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(docLength, 1, "bc", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(docLength, 1, "ab", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(docLength, 1, "de", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 4);
		location = doc.document.FindText(docLength, 1, "_a", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
		location = doc.document.FindText(docLength, 1, "ef", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
		lengthFinding = 3;
		location = doc.document.FindText(docLength, 1, "cde", FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 3);
	}

	SECTION("SearchInUTF8") {
		DocPlus doc("ab\xCE\x93" "d", CpUtf8);	// a b gamma d
		const std::string finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(doc.document.Length(), 0, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
		// Check doesn't try to follow a lead-byte past the search end
		const std::string findingUTF = "\xCE\x93";
		lengthFinding = findingUTF.length();
		location = doc.document.FindText(0, 4, findingUTF.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 2);
		// Only succeeds as 3 is partway through character so adjusted to 4
		location = doc.document.FindText(0, 3, findingUTF.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, findingUTF.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("InsensitiveSearchInUTF8") {
		DocPlus doc("ab\xCE\x93" "d", CpUtf8);	// a b gamma d
		const std::string finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(doc.document.Length(), 0, finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		const std::string findingUTF = "\xCE\x93";
		lengthFinding = findingUTF.length();
		location = doc.FindNeedle(findingUTF, FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(doc.document.Length(), 0, findingUTF.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 4, findingUTF.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		// Only succeeds as 3 is partway through character so adjusted to 4
		location = doc.document.FindText(0, 3, findingUTF.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, findingUTF.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("SearchInShiftJIS") {
		// {CJK UNIFIED IDEOGRAPH-9955} is two bytes: {0xE9, 'b'} in Shift-JIS
		// The 'b' can be incorrectly matched by the search string 'b' when the search
		// does not iterate the text correctly.
		DocPlus doc("ab\xe9" "b ", 932);	// a b {CJK UNIFIED IDEOGRAPH-9955} {space}
		std::string finding = "b";
		// Search forwards
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		// Search backwards
		lengthFinding = finding.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding.c_str(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
	}

	SECTION("InsensitiveSearchInShiftJIS") {
		// {CJK UNIFIED IDEOGRAPH-9955} is two bytes: {0xE9, 'b'} in Shift-JIS
		// The 'b' can be incorrectly matched by the search string 'b' when the search
		// does not iterate the text correctly.
		DocPlus doc("ab\xe9" "b ", 932);	// a b {CJK UNIFIED IDEOGRAPH-9955} {space}
		std::string finding = "b";
		// Search forwards
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		// Search backwards
		lengthFinding = finding.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		std::string finding932 = "\xe9" "b";
		// Search forwards
		lengthFinding = finding932.length();
		location = doc.FindNeedle(finding932, FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		// Search backwards
		lengthFinding = finding932.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding932.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 3, finding932.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, finding932.c_str(), FindOption::None, &lengthFinding);
		REQUIRE(location == -1);
		// Can not test case mapping of double byte text as folder available here does not implement this
	}

	SECTION("GetCharacterAndWidth DBCS") {
		Document doc(DocumentOption::Default);
		doc.SetDBCSCodePage(932);
		REQUIRE(doc.CodePage() == 932);
		const Sci::Position length = doc.InsertString(0, "H\x84\xff\x84H", 5);
		// This text is invalid in code page 932.
		// A reasonable interpretation is as 4 items: 2 characters and 2 character fragments
		// The last item is a 2-byte CYRILLIC CAPITAL LETTER ZE character
		// H [84] [FF] ZE
		REQUIRE(5 == length);
		REQUIRE(5 == doc.Length());
		Sci::Position width = 0;
		// test GetCharacterAndWidth()
		int ch = doc.GetCharacterAndWidth(0, &width);
		REQUIRE(width == 1);
		REQUIRE(ch == 'H');
		ch = doc.GetCharacterAndWidth(1, &width);
		REQUIRE(width == 1);
		REQUIRE(ch == 0x84);
		width = 0;
		ch = doc.GetCharacterAndWidth(2, &width);
		REQUIRE(width == 1);
		REQUIRE(ch == 0xff);
		width = 0;
		ch = doc.GetCharacterAndWidth(3, &width);
		REQUIRE(width == 2);
		REQUIRE(ch == 0x8448);
		// test LenChar()
		width = doc.LenChar(0);
		REQUIRE(width == 1);
		width = doc.LenChar(1);
		REQUIRE(width == 1);
		width = doc.LenChar(2);
		REQUIRE(width == 1);
		width = doc.LenChar(3);
		REQUIRE(width == 2);
		// test MovePositionOutsideChar()
		Sci::Position pos = doc.MovePositionOutsideChar(1, 1);
		REQUIRE(pos == 1);
		pos = doc.MovePositionOutsideChar(2, 1);
		REQUIRE(pos == 2);
		pos = doc.MovePositionOutsideChar(3, 1);
		REQUIRE(pos == 3);
		pos = doc.MovePositionOutsideChar(4, 1);
		REQUIRE(pos == 5);
		pos = doc.MovePositionOutsideChar(1, -1);
		REQUIRE(pos == 1);
		pos = doc.MovePositionOutsideChar(2, -1);
		REQUIRE(pos == 2);
		pos = doc.MovePositionOutsideChar(3, -1);
		REQUIRE(pos == 3);
		pos = doc.MovePositionOutsideChar(4, -1);
		REQUIRE(pos == 3);
		// test NextPosition()
		pos = doc.NextPosition(0, 1);
		REQUIRE(pos == 1);
		pos = doc.NextPosition(1, 1);
		REQUIRE(pos == 2);
		pos = doc.NextPosition(2, 1);
		REQUIRE(pos == 3);
		pos = doc.NextPosition(3, 1);
		REQUIRE(pos == 5);
		pos = doc.NextPosition(1, -1);
		REQUIRE(pos == 0);
		pos = doc.NextPosition(2, -1);
		REQUIRE(pos == 1);
		pos = doc.NextPosition(3, -1);
		REQUIRE(pos == 2);
		pos = doc.NextPosition(5, -1);
		REQUIRE(pos == 3);
	}

	SECTION("NextPosition Valid DBCS") {
		Document doc(DocumentOption::Default);
		doc.SetDBCSCodePage(932);
		REQUIRE(doc.CodePage() == 932);
		// This text is valid in code page 932.
		// O p e n = U+958B Ku ( O ) U+7DE8 -
		// U+958B open
		// U+7DE8 arrange
		const std::string japaneseText = "Open=\x8aJ\x82\xad(O)\x95\xd2-";
		const Sci::Position length = doc.InsertString(0, japaneseText.c_str(), japaneseText.length());
		REQUIRE(length == 15);
		// Forwards
		REQUIRE(doc.NextPosition( 0, 1) == 1);
		REQUIRE(doc.NextPosition( 1, 1) == 2);
		REQUIRE(doc.NextPosition( 2, 1) == 3);
		REQUIRE(doc.NextPosition( 3, 1) == 4);
		REQUIRE(doc.NextPosition( 4, 1) == 5);
		REQUIRE(doc.NextPosition( 5, 1) == 7);	// Double byte
		REQUIRE(doc.NextPosition( 6, 1) == 7);
		REQUIRE(doc.NextPosition( 7, 1) == 9);	// Double byte
		REQUIRE(doc.NextPosition( 8, 1) == 9);
		REQUIRE(doc.NextPosition( 9, 1) == 10);
		REQUIRE(doc.NextPosition(10, 1) == 11);
		REQUIRE(doc.NextPosition(11, 1) == 12);
		REQUIRE(doc.NextPosition(12, 1) == 14);	// Double byte
		REQUIRE(doc.NextPosition(13, 1) == 14);
		REQUIRE(doc.NextPosition(14, 1) == 15);
		REQUIRE(doc.NextPosition(15, 1) == 15);
		// Backwards
		REQUIRE(doc.NextPosition( 0, -1) == 0);
		REQUIRE(doc.NextPosition( 1, -1) == 0);
		REQUIRE(doc.NextPosition( 2, -1) == 1);
		REQUIRE(doc.NextPosition( 3, -1) == 2);
		REQUIRE(doc.NextPosition( 4, -1) == 3);
		REQUIRE(doc.NextPosition( 5, -1) == 4);
		REQUIRE(doc.NextPosition( 6, -1) == 5);	// Double byte
		REQUIRE(doc.NextPosition( 7, -1) == 5);
		REQUIRE(doc.NextPosition( 8, -1) == 7);	// Double byte
		REQUIRE(doc.NextPosition( 9, -1) == 7);
		REQUIRE(doc.NextPosition(10, -1) == 9);
		REQUIRE(doc.NextPosition(11, -1) == 10);
		REQUIRE(doc.NextPosition(12, -1) == 11);
		REQUIRE(doc.NextPosition(13, -1) == 12);	// Double byte
		REQUIRE(doc.NextPosition(14, -1) == 12);
		REQUIRE(doc.NextPosition(15, -1) == 14);
	}

}

TEST_CASE("Words") {

	SECTION("WordsInText") {
		const DocPlus doc(" abc ", 0);
		REQUIRE(doc.document.IsWordAt(1, 4));
		REQUIRE(!doc.document.IsWordAt(0, 1));
		REQUIRE(!doc.document.IsWordAt(1, 2));
		const DocPlus docPunct(" [!] ", 0);
		REQUIRE(docPunct.document.IsWordAt(1, 4));
		REQUIRE(!docPunct.document.IsWordAt(0, 1));
		REQUIRE(!docPunct.document.IsWordAt(1, 2));
		const DocPlus docMixed(" -ab ", 0);	// '-' is punctuation, 'ab' is word
		REQUIRE(docMixed.document.IsWordAt(2, 4));
		REQUIRE(docMixed.document.IsWordAt(1, 4));
		REQUIRE(docMixed.document.IsWordAt(1, 2));
		REQUIRE(!docMixed.document.IsWordAt(1, 3));	// 3 is between a and b so not word edge
		// Scintilla's word definition just examines the ends
		const DocPlus docOverSpace(" a b ", 0);
		REQUIRE(docOverSpace.document.IsWordAt(1, 4));
	}

	SECTION("WordsAtEnds") {
		const DocPlus doc("a c", 0);
		REQUIRE(doc.document.IsWordAt(0, 1));
		REQUIRE(doc.document.IsWordAt(2, 3));
		const DocPlus docEndSpace(" a c ", 0);
		REQUIRE(!docEndSpace.document.IsWordAt(0, 2));
		REQUIRE(!docEndSpace.document.IsWordAt(3, 5));
	}
}

TEST_CASE("SafeSegment") {
	SECTION("Short") {
		const DocPlus doc("", 0);
		// all encoding: break before or after last space
		const std::string_view text = "12 ";
		size_t length = doc.document.SafeSegment(text);
		REQUIRE(length <= text.length());
		REQUIRE(text[length - 1] == '2');
		REQUIRE(text[length] == ' ');
	}

	SECTION("ASCII") {
		const DocPlus doc("", 0);
		// all encoding: break before or after last space
		std::string_view text = "12 3 \t45";
		size_t length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == ' ');
		REQUIRE(text[length] == '\t');

		// UTF-8 and ASCII: word and punctuation boundary in middle of text
		text = "(IsBreakSpace(text[j]))";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'j');
		REQUIRE(text[length] == ']');

		// UTF-8 and ASCII: word and punctuation boundary near start of text
		text = "(IsBreakSpace";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '(');
		REQUIRE(text[length] == 'I');

		// UTF-8 and ASCII: word and punctuation boundary near end of text
		text = "IsBreakSpace)";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'e');
		REQUIRE(text[length] == ')');

		// break before last character
		text = "JapaneseJa";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'J');
		REQUIRE(text[length] == 'a');
	}

	SECTION("UTF-8") {
		const DocPlus doc("", CpUtf8);
		// break before last character: no trail byte
		std::string_view text = "JapaneseJa";
		size_t length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'J');
		REQUIRE(text[length] == 'a');

		// break before last character: 1 trail byte
		text = "Japanese\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xc2\xa9";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\x9e');
		REQUIRE(text[length] == '\xc2');

		// break before last character: 2 trail bytes
		text = "Japanese\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\xac');
		REQUIRE(text[length] == '\xe8');

		// break before last character: 3 trail bytes
		text = "Japanese\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xf0\x9f\x98\x8a";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\x9e');
		REQUIRE(text[length] == '\xf0');
	}

	SECTION("DBCS Shift-JIS") {
		const DocPlus doc("", 932);
		// word and punctuation boundary in middle of text: single byte
		std::string_view text = "(IsBreakSpace(text[j]))";
		size_t length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'j');
		REQUIRE(text[length] == ']');

		// word and punctuation boundary in middle of text: double byte
		text = "(IsBreakSpace(text[\x8c\xea]))";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\xea');
		REQUIRE(text[length] == ']');

		// word and punctuation boundary near start of text
		text = "(IsBreakSpace";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '(');
		REQUIRE(text[length] == 'I');

		// word and punctuation boundary near end of text: single byte
		text = "IsBreakSpace)";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'e');
		REQUIRE(text[length] == ')');

		// word and punctuation boundary near end of text: double byte
		text = "IsBreakSpace\x8c\xea)";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\xea');
		REQUIRE(text[length] == ')');

		// break before last character: single byte
		text = "JapaneseJa";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == 'J');
		REQUIRE(text[length] == 'a');

		// break before last character: double byte
		text = "Japanese\x93\xfa\x96\x7b\x8c\xea";
		length = doc.document.SafeSegment(text);
		REQUIRE(text[length - 1] == '\x7b');
		REQUIRE(text[length] == '\x8c');
	}
}
