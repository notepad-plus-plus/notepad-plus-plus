/** @file testDocument.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <set>
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

// set global locale to pass std::regex related tests
// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63776
struct GlobalLocaleInitializer {
	GlobalLocaleInitializer() {
		try {
			std::locale::global(std::locale("en_US.UTF-8"));
		} catch (...) {}
	}
} globalLocaleInitializer;

// Test Document.

struct Folding {
	int from;
	int to;
	int length;
};

// Table of case folding for non-ASCII bytes in Windows Latin code page 1252
const Folding foldings1252[] = {
	{0x8a, 0x9a, 0x01},
	{0x8c, 0x9c, 0x01},
	{0x8e, 0x9e, 0x01},
	{0x9f, 0xff, 0x01},
	{0xc0, 0xe0, 0x17},
	{0xd8, 0xf8, 0x07},
};

// Table of case folding for non-ASCII bytes in Windows Russian code page 1251
const Folding foldings1251[] = {
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

std::string ReadFile(const std::string &path) {
	std::ifstream ifs(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}

struct Match {
	Sci::Position location = 0;
	Sci::Position length = 0;
	constexpr Match() = default;
	constexpr Match(Sci::Position location_, Sci::Position length_=0) : location(location_), length(length_) {
	}
	constexpr bool operator==(const Match &other) const {
		return location == other.location && length == other.length;
	}
};

std::ostream &operator << (std::ostream &os, Match const &value) {
	os << value.location << "," << value.length;
	return os;
}

struct DocPlus {
	Document document;

	DocPlus(std::string_view svInitial, int codePage) : document(DocumentOption::Default) {
		SetCodePage(codePage);
		document.InsertString(0, svInitial);
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

	Sci::Position FindNeedle(std::string_view needle, FindOption options, Sci::Position *length) {
		assert(*length == static_cast<Sci::Position>(needle.length()));
		return document.FindText(0, document.Length(), needle.data(), options, length);
	}
	Sci::Position FindNeedleReverse(std::string_view needle, FindOption options, Sci::Position *length) {
		assert(*length == static_cast<Sci::Position>(needle.length()));
		return document.FindText(document.Length(), 0, needle.data(), options, length);
	}

	Match FindString(Sci::Position minPos, Sci::Position maxPos, std::string_view needle, FindOption flags) {
		Sci::Position lengthFinding = needle.length();
		const Sci::Position location = document.FindText(minPos, maxPos, needle.data(), flags, &lengthFinding);
		return { location, lengthFinding };
	}

	std::string Substitute(std::string_view substituteText) {
		Sci::Position lengthsubstitute = substituteText.length();
		std::string substituted = document.SubstituteByPosition(substituteText.data(), &lengthsubstitute);
		assert(lengthsubstitute == static_cast<Sci::Position>(substituted.length()));
		return substituted;
	}

	void MoveGap(Sci::Position gapNew) {
		// Move gap to gapNew by inserting
		document.InsertString(gapNew, "!", 1);
		// Remove insertion
		document.DeleteChars(gapNew, 1);
	}

	[[nodiscard]] std::string Contents() const {
		const Sci::Position length = document.Length();
		std::string contents(length, 0);
		document.GetCharRange(contents.data(), 0, length);
		return contents;
	}
};

void TimeTrace(std::string_view sv, const Catch::Timer &tikka) {
	std::cout << sv << std::setw(5) << tikka.getElapsedMilliseconds() << " milliseconds" << std::endl;
}

TEST_CASE("Document") {

	constexpr std::string_view sText = "Scintilla";
	constexpr Sci::Position sLength = sText.length();
	constexpr FindOption rePosix = FindOption::RegExp | FindOption::Posix;
	constexpr FindOption reCxx11 = FindOption::RegExp | FindOption::Cxx11RegEx;

	SECTION("InsertOneLine") {
		DocPlus doc("", 0);
		const Sci::Position length = doc.document.InsertString(0, sText);
		REQUIRE(sLength == doc.document.Length());
		REQUIRE(length == sLength);
		REQUIRE(1 == doc.document.LinesTotal());
		REQUIRE(0 == doc.document.LineStart(0));
		REQUIRE(0 == doc.document.LineFromPosition(0));
		REQUIRE(0 == doc.document.LineStartPosition(0));
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
		constexpr std::string_view finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.FindNeedleReverse(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 2, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("SearchInBothSegments") {
		DocPlus doc("ab-ab", 0);	// a b - a b
		constexpr std::string_view finding = "ab";
		for (int gapPos = 0; gapPos <= 5; gapPos++) {
			doc.MoveGap(gapPos);
			Sci::Position lengthFinding = finding.length();
			Sci::Position location = doc.document.FindText(0, doc.document.Length(), finding.data(), FindOption::MatchCase, &lengthFinding);
			REQUIRE(location == 0);
			location = doc.document.FindText(2, doc.document.Length(), finding.data(), FindOption::MatchCase, &lengthFinding);
			REQUIRE(location == 3);
		}
	}

	SECTION("InsensitiveSearchInLatin") {
		DocPlus doc("abcde", 0);	// a b c d e
		constexpr std::string_view finding = "B";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 2, finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("InsensitiveSearchIn1252") {
		// In Windows Latin, code page 1252, C6 is AE and E6 is ae
		DocPlus doc("tru\xc6s\xe6t", 0);	// t r u AE s ae t
		doc.SetSBCSFoldings(foldings1252, std::size(foldings1252));

		// Search for upper-case AE
		std::string_view finding = "\xc6";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(4, doc.document.Length(), finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 5);

		// Search for lower-case ae
		finding = "\xe6";
		location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(4, doc.document.Length(), finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
		location = doc.FindNeedleReverse(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 5);
	}

	SECTION("Search2InLatin") {
		// Checks that the initial '_' and final 'f' are ignored since they are outside the search bounds
		DocPlus doc("_abcdef", 0);	// _ a b c d e f
		constexpr std::string_view finding = "cd";
		Sci::Position lengthFinding = finding.length();
		const size_t docLength = doc.document.Length() - 1;
		Sci::Position location = doc.document.FindText(1, docLength, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 3);
		location = doc.document.FindText(docLength, 1, finding.data(), FindOption::MatchCase, &lengthFinding);
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
		constexpr std::string_view finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(doc.document.Length(), 0, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(0, 1, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
		// Check doesn't try to follow a lead-byte past the search end
		constexpr std::string_view findingUTF = "\xCE\x93";
		lengthFinding = findingUTF.length();
		location = doc.document.FindText(0, 4, findingUTF.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 2);
		// Only succeeds as 3 is partway through character so adjusted to 4
		location = doc.document.FindText(0, 3, findingUTF.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, findingUTF.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("InsensitiveSearchInUTF8") {
		DocPlus doc("ab\xCE\x93" "d", CpUtf8);	// a b gamma d
		constexpr std::string_view finding = "b";
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		location = doc.document.FindText(doc.document.Length(), 0, finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		constexpr std::string_view findingUTF = "\xCE\x93";
		lengthFinding = findingUTF.length();
		location = doc.FindNeedle(findingUTF, FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(doc.document.Length(), 0, findingUTF.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 4, findingUTF.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		// Only succeeds as 3 is partway through character so adjusted to 4
		location = doc.document.FindText(0, 3, findingUTF.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, findingUTF.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == -1);
	}

	SECTION("SearchInShiftJIS") {
		// {CJK UNIFIED IDEOGRAPH-9955} is two bytes: {0xE9, 'b'} in Shift-JIS
		// The 'b' can be incorrectly matched by the search string 'b' when the search
		// does not iterate the text correctly.
		DocPlus doc("ab\xe9" "b ", 932);	// a b {CJK UNIFIED IDEOGRAPH-9955} {space}
		constexpr std::string_view finding = "b";
		// Search forwards
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
		// Search backwards
		lengthFinding = finding.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding.data(), FindOption::MatchCase, &lengthFinding);
		REQUIRE(location == 1);
	}

	SECTION("InsensitiveSearchInShiftJIS") {
		// {CJK UNIFIED IDEOGRAPH-9955} is two bytes: {0xE9, 'b'} in Shift-JIS
		// The 'b' can be incorrectly matched by the search string 'b' when the search
		// does not iterate the text correctly.
		DocPlus doc("ab\xe9" "b ", 932);	// a b {CJK UNIFIED IDEOGRAPH-9955} {space}
		constexpr std::string_view finding = "b";
		// Search forwards
		Sci::Position lengthFinding = finding.length();
		Sci::Position location = doc.FindNeedle(finding, FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		// Search backwards
		lengthFinding = finding.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 1);
		constexpr std::string_view finding932 = "\xe9" "b";
		// Search forwards
		lengthFinding = finding932.length();
		location = doc.FindNeedle(finding932, FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		// Search backwards
		lengthFinding = finding932.length();
		location = doc.document.FindText(doc.document.Length(), 0, finding932.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 3, finding932.data(), FindOption::None, &lengthFinding);
		REQUIRE(location == 2);
		location = doc.document.FindText(0, 2, finding932.data(), FindOption::None, &lengthFinding);
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
		constexpr std::string_view japaneseText = "Open=\x8aJ\x82\xad(O)\x95\xd2-";
		const Sci::Position length = doc.InsertString(0, japaneseText);
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

	SECTION("RegexSearchAndSubstitution") {
		DocPlus doc("\n\r\r\n 1a\xCE\x93z \n\r\r\n 2b\xCE\x93y \n\r\r\n", CpUtf8);// 1a gamma z 2b gamma y
		const Sci::Position docLength = doc.document.Length();
		Match match;

		constexpr std::string_view finding = R"(\d+(\w+))";
		constexpr std::string_view substituteText = R"(\t\1\n)";
		constexpr std::string_view longest = "\\w+";
		std::string substituted;

		match = doc.FindString(0, docLength, finding, rePosix);
		REQUIRE(match == Match(5, 5));
		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\ta\xCE\x93z\n");

		match = doc.FindString(docLength, 0, finding, rePosix);
		REQUIRE(match == Match(16, 5));
		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\tb\xCE\x93y\n");

		match = doc.FindString(docLength, 0, longest, rePosix);
		REQUIRE(match == Match(16, 5));

		#ifndef NO_CXX11_REGEX
		match = doc.FindString(0, docLength, finding, reCxx11);
		REQUIRE(match == Match(5, 5));
		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\ta\xCE\x93z\n");

		match = doc.FindString(docLength, 0, finding, reCxx11);
		REQUIRE(match == Match(16, 5));
		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\tb\xCE\x93y\n");

		match = doc.FindString(docLength, 0, longest, reCxx11);
		REQUIRE(match == Match(16, 5));
		#endif
	}

	SECTION("RegexAssertion") {
		DocPlus doc("ab cd ef\r\ngh ij kl", CpUtf8);
		const Sci::Position docLength = doc.document.Length();
		Match match;

		constexpr std::string_view findingBOL = "^";
		match = doc.FindString(0, docLength, findingBOL, rePosix);
		REQUIRE(match == Match(0));
		match = doc.FindString(1, docLength, findingBOL, rePosix);
		REQUIRE(match == Match(10));
		match = doc.FindString(docLength, 0, findingBOL, rePosix);
		REQUIRE(match == Match(10));
		match = doc.FindString(docLength - 1, 0, findingBOL, rePosix);
		REQUIRE(match == Match(10));

		#ifndef NO_CXX11_REGEX
		match = doc.FindString(0, docLength, findingBOL, reCxx11);
		REQUIRE(match == Match(0));
		match = doc.FindString(1, docLength, findingBOL, reCxx11);
		REQUIRE(match == Match(10));
		match = doc.FindString(docLength, 0, findingBOL, reCxx11);
		REQUIRE(match == Match(10));
		match = doc.FindString(docLength - 1, 0, findingBOL, reCxx11);
		REQUIRE(match == Match(10));
		#endif

		constexpr std::string_view findingEOL = "$";
		match = doc.FindString(0, docLength, findingEOL, rePosix);
		REQUIRE(match == Match(8));
		match = doc.FindString(1, docLength, findingEOL, rePosix);
		REQUIRE(match == Match(8));
		match = doc.FindString(docLength, 0, findingEOL, rePosix);
		REQUIRE(match == Match(18));
		match = doc.FindString(docLength - 1, 0, findingEOL, rePosix);
		REQUIRE(match == Match(8));

		#if !defined(NO_CXX11_REGEX) && !defined(_LIBCPP_VERSION)
		match = doc.FindString(0, docLength, findingEOL, reCxx11);
		REQUIRE(match == Match(8));
		match = doc.FindString(1, docLength, findingEOL, reCxx11);
		REQUIRE(match == Match(8));
		match = doc.FindString(docLength, 0, findingEOL, reCxx11);
		REQUIRE(match == Match(18));
		match = doc.FindString(docLength - 1, 0, findingEOL, reCxx11);
		REQUIRE(match == Match(8));
		#endif

		constexpr std::string_view findingBOW = "\\<";
		match = doc.FindString(0, docLength, findingBOW, rePosix);
		REQUIRE(match == Match(0));
		match = doc.FindString(1, docLength, findingBOW, rePosix);
		REQUIRE(match == Match(3));
		match = doc.FindString(docLength, 0, findingBOW, rePosix);
		REQUIRE(match == Match(16));
		match = doc.FindString(docLength - 1, 0, findingBOW, rePosix);
		REQUIRE(match == Match(16));

		constexpr std::string_view findingEOW = "\\>";
		match = doc.FindString(0, docLength, findingEOW, rePosix);
		REQUIRE(match == Match(2));
		match = doc.FindString(1, docLength, findingEOW, rePosix);
		REQUIRE(match == Match(2));
		match = doc.FindString(docLength, 0, findingEOW, rePosix);
		REQUIRE(match == Match(18));
		match = doc.FindString(docLength - 1, 0, findingEOW, rePosix);
		REQUIRE(match == Match(15));

		constexpr std::string_view findingEOWEOL = "\\>$";
		match = doc.FindString(0, docLength, findingEOWEOL, rePosix);
		REQUIRE(match == Match(8));
		match = doc.FindString(10, docLength, findingEOWEOL, rePosix);
		REQUIRE(match == Match(18));

		#ifndef NO_CXX11_REGEX
		constexpr std::string_view findingWB = "\\b";
		match = doc.FindString(0, docLength, findingWB, reCxx11);
		REQUIRE(match == Match(0));
		match = doc.FindString(1, docLength, findingWB, reCxx11);
		REQUIRE(match == Match(2));
		match = doc.FindString(docLength, 0, findingWB, reCxx11);
		#ifdef _LIBCPP_VERSION
		REQUIRE(match == Match(16));
		#else
		REQUIRE(match == Match(18));
		#endif
		match = doc.FindString(docLength - 1, 0, findingWB, reCxx11);
		REQUIRE(match == Match(16));

		constexpr std::string_view findingNWB = "\\B";
		match = doc.FindString(0, docLength, findingNWB, reCxx11);
		REQUIRE(match == Match(1));
		match = doc.FindString(1, docLength, findingNWB, reCxx11);
		REQUIRE(match == Match(1));
		#ifdef _LIBCPP_VERSION
		match = doc.FindString(docLength, 0, findingNWB, reCxx11);
		REQUIRE(match == Match(18));
		match = doc.FindString(docLength - 1, 0, findingNWB, reCxx11);
		REQUIRE(match == Match(14));
		#else
		match = doc.FindString(docLength, 0, findingNWB, reCxx11);
		REQUIRE(match == Match(17));
		match = doc.FindString(docLength - 1, 0, findingNWB, reCxx11);
		REQUIRE(match == Match(17));
		#endif
		#endif
	}

	SECTION("RegexContextualAssertion") {
		// For std::regex, check the use of assertions next to text in forward direction
		// These are more common than empty assertions
		DocPlus doc("ab cd ef\r\ngh ij kl", CpUtf8);
		const Sci::Position docLength = doc.document.Length();
		Match match;

		#ifndef NO_CXX11_REGEX

		match = doc.FindString(0, docLength, "^[a-z]", reCxx11);
		REQUIRE(match == Match(0, 1));
		match = doc.FindString(1, docLength, "^[a-z]", reCxx11);
		REQUIRE(match == Match(10, 1));

		match = doc.FindString(0, docLength, "[a-z]$", reCxx11);
		REQUIRE(match == Match(7, 1));
		match = doc.FindString(10, docLength, "[a-z]$", reCxx11);
		REQUIRE(match == Match(17, 1));

		match = doc.FindString(0, docLength, "\\b[a-z]", reCxx11);
		REQUIRE(match == Match(0, 1));
		match = doc.FindString(1, docLength, "\\b[a-z]", reCxx11);
		REQUIRE(match == Match(3, 1));
		match = doc.FindString(0, docLength, "[a-z]\\b", reCxx11);
		REQUIRE(match == Match(1, 1));
		match = doc.FindString(2, docLength, "[a-z]\\b", reCxx11);
		REQUIRE(match == Match(4, 1));

		match = doc.FindString(0, docLength, "\\B[a-z]", reCxx11);
		REQUIRE(match == Match(1, 1));
		match = doc.FindString(1, docLength, "\\B[a-z]", reCxx11);
		REQUIRE(match == Match(1, 1));
		match = doc.FindString(0, docLength, "[a-z]\\B", reCxx11);
		REQUIRE(match == Match(0, 1));
		match = doc.FindString(2, docLength, "[a-z]\\B", reCxx11);
		REQUIRE(match == Match(3, 1));

		#endif
	}

	SECTION("RESearchMovePositionOutsideCharUTF8") {
		DocPlus doc(" a\xCE\x93\xCE\x93z ", CpUtf8);// a gamma gamma z
		const Sci::Position docLength = doc.document.Length();
		constexpr std::string_view finding = R"([a-z](\w)\1)";

		Match match = doc.FindString(0, docLength, finding, rePosix);
		REQUIRE(match == Match(1, 5));

		constexpr std::string_view substituteText = R"(\t\1\n)";
		std::string substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\t\xCE\x93\n");

		#ifndef NO_CXX11_REGEX
		match = doc.FindString(0, docLength, finding, reCxx11);
		REQUIRE(match == Match(1, 5));

		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\t\xCE\x93\n");
		#endif
	}

	SECTION("RESearchMovePositionOutsideCharDBCS") {
		DocPlus doc(" \x98\x61xx 1aa\x83\xA1\x83\xA1z ", 932);// U+548C xx 1aa gamma gamma z
		const Sci::Position docLength = doc.document.Length();

		Match match = doc.FindString(0, docLength, R"([a-z](\w)\1)", rePosix);
		REQUIRE(match == Match(8, 5));

		constexpr std::string_view substituteText = R"(\t\1\n)";
		std::string substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\t\x83\xA1\n");

		match = doc.FindString(0, docLength, R"(\w([a-z])\1)", rePosix);
		REQUIRE(match == Match(6, 3));

		substituted = doc.Substitute(substituteText);
		REQUIRE(substituted == "\ta\n");
	}

	SECTION("BraceMatch") {
		DocPlus doc("{}(()())[]", CpUtf8);
		constexpr Sci::Position maxReStyle = 0; // unused parameter
		Sci::Position pos = doc.document.BraceMatch(0, maxReStyle, 0, false);
		REQUIRE(pos == 1);
		pos = doc.document.BraceMatch(1, maxReStyle, 0, false);
		REQUIRE(pos == 0);
		pos = doc.document.BraceMatch(8, maxReStyle, 0, false);
		REQUIRE(pos == 9);
		pos = doc.document.BraceMatch(9, maxReStyle, 0, false);
		REQUIRE(pos == 8);
		pos = doc.document.BraceMatch(2, maxReStyle, 0, false);
		REQUIRE(pos == 7);
		pos = doc.document.BraceMatch(7, maxReStyle, 0, false);
		REQUIRE(pos == 2);

		// BraceMatchNext()
		pos = doc.document.BraceMatch(2, maxReStyle, 3, true);
		REQUIRE(pos == 7);
		pos = doc.document.BraceMatch(2, maxReStyle, 4, true);
		REQUIRE(pos == 4);
		pos = doc.document.BraceMatch(2, maxReStyle, 5, true);
		REQUIRE(pos == 7);
		pos = doc.document.BraceMatch(2, maxReStyle, 6, true);
		REQUIRE(pos == 6);
		pos = doc.document.BraceMatch(2, maxReStyle, 7, true);
		REQUIRE(pos == 7);

		pos = doc.document.BraceMatch(7, maxReStyle, 6, true);
		REQUIRE(pos == 2);
		pos = doc.document.BraceMatch(7, maxReStyle, 5, true);
		REQUIRE(pos == 5);
		pos = doc.document.BraceMatch(7, maxReStyle, 4, true);
		REQUIRE(pos == 2);
		pos = doc.document.BraceMatch(7, maxReStyle, 3, true);
		REQUIRE(pos == 3);
		pos = doc.document.BraceMatch(7, maxReStyle, 2, true);
		REQUIRE(pos == 2);
	}

	SECTION("BraceMatch DBCS") {
		DocPlus doc("{\x81}\x81{}", 932); // { U+00B1 U+FF0B }
		constexpr Sci::Position maxReStyle = 0; // unused parameter
		Sci::Position pos = doc.document.BraceMatch(0, maxReStyle, 0, false);
		REQUIRE(pos == 5);
		pos = doc.document.BraceMatch(5, maxReStyle, 0, false);
		REQUIRE(pos == 0);
	}

}

TEST_CASE("DocumentUndo") {

	// These tests check that Undo reports the end of coalesced deletes

	constexpr std::string_view sText = "Scintilla";
	DocPlus doc(sText, 0);

	SECTION("CheckDeleteForwards") {
		// Delete forwards like the Del key
		doc.document.DeleteUndoHistory();
		doc.document.DeleteChars(1, 1);
		doc.document.DeleteChars(1, 1);
		doc.document.DeleteChars(1, 1);
		const Sci::Position position = doc.document.Undo();
		REQUIRE(position == 4);	// End of reinsertion
		REQUIRE(!doc.document.CanUndo());	// Exhausted undo stack
		REQUIRE(doc.document.CanRedo());
	}

	SECTION("CheckDeleteBackwards") {
		// Delete backwards like the backspace key
		doc.document.DeleteUndoHistory();
		doc.document.DeleteChars(5, 1);
		doc.document.DeleteChars(4, 1);
		doc.document.DeleteChars(3, 1);
		const Sci::Position position = doc.document.Undo();
		REQUIRE(position == 6);	// End of reinsertion
		REQUIRE(!doc.document.CanUndo());	// Exhausted undo stack
	}

	SECTION("CheckBothWays") {
		// Delete backwards like the backspace key
		doc.document.DeleteUndoHistory();
		// Like having the caret at position 5 then
		doc.document.DeleteChars(5, 1);	// Del
		doc.document.DeleteChars(4, 1); // Backspace
		doc.document.DeleteChars(4, 1); // Del
		doc.document.DeleteChars(3, 1); // Backspace
		const Sci::Position position = doc.document.Undo();
		REQUIRE(position == 7);	// End of reinsertion, Start at 5, 2*Del
		REQUIRE(!doc.document.CanUndo());	// Exhausted undo stack
	}

	SECTION("CheckInsert") {
		// Insertions are only coalesced when following previous
		doc.document.DeleteUndoHistory();
		doc.document.InsertString(1, "1");
		doc.document.InsertString(2, "2");
		doc.document.InsertString(3, "3");
		REQUIRE(doc.Contents() == "S123cintilla");
		const Sci::Position position = doc.document.Undo();
		REQUIRE(position == 1);	// Start of insertions
		REQUIRE(!doc.document.CanUndo());	// Exhausted undo stack
	}

	SECTION("CheckGrouped") {
		// Check that position returned for group is that at end of first deletion set
		// Also include a container undo action.
		doc.document.DeleteUndoHistory();
		doc.document.BeginUndoAction();
		// At 1, 2*Del so end of initial deletion sequence is 3
		doc.document.DeleteChars(1, 1); // 'c'
		doc.document.DeleteChars(1, 1); // 'i'
		doc.document.AddUndoAction(99, true);
		doc.document.InsertString(1, "1");
		doc.document.DeleteChars(4, 2); // 'il'
		doc.document.BeginUndoAction();
		REQUIRE(doc.Contents() == "S1ntla");
		const Sci::Position position = doc.document.Undo();
		REQUIRE(position == 3);	// Start of insertions
		REQUIRE(!doc.document.CanUndo());	// Exhausted undo stack
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
		constexpr std::string_view text = "12 ";
		const size_t length = doc.document.SafeSegment(text);
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

TEST_CASE("PerLine") {
	SECTION("LineMarkers") {
		DocPlus doc("1\n2\n", CpUtf8);
		REQUIRE(doc.document.LinesTotal() == 3);
		const int mh1 = doc.document.AddMark(0, 0);
		const int mh2 = doc.document.AddMark(1, 1);
		const int mh3 = doc.document.AddMark(2, 2);
		REQUIRE(mh1 != -1);
		REQUIRE(mh2 != -1);
		REQUIRE(mh3 != -1);
		REQUIRE(doc.document.AddMark(3, 3) == -1);

		// delete first character, no change
		REQUIRE(doc.document.CharAt(0) == '1');
		doc.document.DeleteChars(0, 1);
		REQUIRE(doc.document.LinesTotal() == 3);
		REQUIRE(doc.document.MarkerHandleFromLine(0, 0) == mh1);
		REQUIRE(doc.document.MarkerHandleFromLine(0, 1) == -1);
		REQUIRE(doc.document.MarkerHandleFromLine(1, 0) == mh2);
		REQUIRE(doc.document.MarkerHandleFromLine(1, 1) == -1);

		// delete first line, so merged
		REQUIRE(doc.document.CharAt(0) == '\n');
		doc.document.DeleteChars(0, 1);
		REQUIRE(doc.document.CharAt(0) == '2');
		const std::set handleSet {mh1, mh2};
		const int handle1 = doc.document.MarkerHandleFromLine(0, 0);
		const int handle2 = doc.document.MarkerHandleFromLine(0, 1);
		REQUIRE(handle1 != handle2);
		REQUIRE(handleSet.count(handle1) == 1);
		REQUIRE(handleSet.count(handle2) == 1);
		REQUIRE(doc.document.MarkerHandleFromLine(0, 2) == -1);
		REQUIRE(doc.document.MarkerHandleFromLine(1, 0) == mh3);
		REQUIRE(doc.document.MarkerHandleFromLine(1, 1) == -1);
	}

	SECTION("LineAnnotation") {
		DocPlus doc("1\n2\n", CpUtf8);
		REQUIRE(doc.document.LinesTotal() == 3);
		Sci::Position length = doc.document.Length();
		doc.document.AnnotationSetText(0, "1");
		doc.document.AnnotationSetText(1, "1\n2");
		doc.document.AnnotationSetText(2, "1\n2\n3");
		REQUIRE(doc.document.AnnotationLines(0) == 1);
		REQUIRE(doc.document.AnnotationLines(1) == 2);
		REQUIRE(doc.document.AnnotationLines(2) == 3);
		REQUIRE(doc.document.AnnotationLines(3) == 0);

		// delete last line
		length -= 1;
		doc.document.DeleteChars(length, 1);
		// Deleting the last line moves its 3-line annotation to previous line,
		// deleting the 2-line annotation of the previous line.
		REQUIRE(doc.document.LinesTotal() == 2);
		REQUIRE(doc.document.AnnotationLines(0) == 1);
		REQUIRE(doc.document.AnnotationLines(1) == 3);
		REQUIRE(doc.document.AnnotationLines(2) == 0);

		// delete last character, no change
		length -= 1;
		REQUIRE(doc.document.CharAt(length) == '2');
		doc.document.DeleteChars(length, 1);
		REQUIRE(doc.document.LinesTotal() == 2);
		REQUIRE(doc.document.AnnotationLines(0) == 1);
		REQUIRE(doc.document.AnnotationLines(1) == 3);
		REQUIRE(doc.document.AnnotationLines(2) == 0);
	}
}
