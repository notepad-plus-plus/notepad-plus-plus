/** @file testCellBuffer.cxx
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

#include "ScintillaTypes.h"

#include "Debugging.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test CellBuffer.

TEST_CASE("CellBuffer") {

	const char sText[] = "Scintilla";
	const Sci::Position sLength = static_cast<Sci::Position>(strlen(sText));

	CellBuffer cb(true, false);

	SECTION("InsertOneLine") {
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText, static_cast<int>(sLength), startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(memcmp(cpChange, sText, sLength) == 0);
		REQUIRE(1 == cb.Lines());
		REQUIRE(0 == cb.LineStart(0));
		REQUIRE(0 == cb.LineFromPosition(0));
		REQUIRE(sLength == cb.LineStart(1));
		REQUIRE(0 == cb.LineFromPosition(static_cast<int>(sLength)));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("InsertTwoLines") {
		const char sText2[] = "Two\nLines";
		const Sci::Position sLength2 = static_cast<Sci::Position>(strlen(sText2));
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText2, static_cast<int>(sLength), startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength2 == cb.Length());
		REQUIRE(memcmp(cpChange, sText2, sLength2) == 0);
		REQUIRE(2 == cb.Lines());
		REQUIRE(0 == cb.LineStart(0));
		REQUIRE(0 == cb.LineFromPosition(0));
		REQUIRE(4 == cb.LineStart(1));
		REQUIRE(1 == cb.LineFromPosition(5));
		REQUIRE(sLength2 == cb.LineStart(2));
		REQUIRE(1 == cb.LineFromPosition(static_cast<int>(sLength)));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("UndoOff") {
		REQUIRE(cb.IsCollectingUndo());
		cb.SetUndoCollection(false);
		REQUIRE(!cb.IsCollectingUndo());
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText, static_cast<int>(sLength), startSequence);
		REQUIRE(!startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(memcmp(cpChange, sText, sLength) == 0);
		REQUIRE(!cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("UndoRedo") {
		const char sTextDeleted[] = "ci";
		const char sTextAfterDeletion[] = "Sntilla";
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText, static_cast<int>(sLength), startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(memcmp(cpChange, sText, sLength) == 0);
		REQUIRE(memcmp(cb.BufferPointer(), sText, sLength) == 0);
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
		const char *cpDeletion = cb.DeleteChars(1, 2, startSequence);
		REQUIRE(startSequence);
		REQUIRE(memcmp(cpDeletion, sTextDeleted, strlen(sTextDeleted)) == 0);
		REQUIRE(memcmp(cb.BufferPointer(), sTextAfterDeletion, strlen(sTextAfterDeletion)) == 0);
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());

		int steps = cb.StartUndo();
		REQUIRE(steps == 1);
		cb.PerformUndoStep();
		REQUIRE(memcmp(cb.BufferPointer(), sText, sLength) == 0);
		REQUIRE(cb.CanUndo());
		REQUIRE(cb.CanRedo());

		steps = cb.StartUndo();
		REQUIRE(steps == 1);
		cb.PerformUndoStep();
		REQUIRE(cb.Length() == 0);
		REQUIRE(!cb.CanUndo());
		REQUIRE(cb.CanRedo());

		steps = cb.StartRedo();
		REQUIRE(steps == 1);
		cb.PerformRedoStep();
		REQUIRE(memcmp(cb.BufferPointer(), sText, sLength) == 0);
		REQUIRE(cb.CanUndo());
		REQUIRE(cb.CanRedo());

		steps = cb.StartRedo();
		REQUIRE(steps == 1);
		cb.PerformRedoStep();
		REQUIRE(memcmp(cb.BufferPointer(), sTextAfterDeletion, strlen(sTextAfterDeletion)) == 0);
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());

		cb.DeleteUndoHistory();
		REQUIRE(!cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("LineEndTypes") {
		REQUIRE(cb.GetLineEndTypes() == LineEndType::Default);
		cb.SetLineEndTypes(LineEndType::Unicode);
		REQUIRE(cb.GetLineEndTypes() == LineEndType::Unicode);
		cb.SetLineEndTypes(LineEndType::Default);
		REQUIRE(cb.GetLineEndTypes() == LineEndType::Default);
	}

	SECTION("ReadOnly") {
		REQUIRE(!cb.IsReadOnly());
		cb.SetReadOnly(true);
		REQUIRE(cb.IsReadOnly());
		bool startSequence = false;
		cb.InsertString(0, sText, static_cast<int>(sLength), startSequence);
		REQUIRE(cb.Length() == 0);
	}

}

TEST_CASE("CharacterIndex") {

	CellBuffer cb(true, false);

	SECTION("Setup") {
		REQUIRE(cb.LineCharacterIndex() == LineCharacterIndexType::None);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 0);
		cb.SetUTF8Substance(true);

		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16);
		REQUIRE(cb.LineCharacterIndex() == LineCharacterIndexType::Utf16);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 0);

		cb.ReleaseLineCharacterIndex(LineCharacterIndexType::Utf16);
		REQUIRE(cb.LineCharacterIndex() == LineCharacterIndexType::None);
	}

	SECTION("Insertion") {
		cb.SetUTF8Substance(true);

		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		cb.InsertString(0, "a", 1, startSequence);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 1);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 1);

		const char *hwair = "\xF0\x90\x8D\x88";
		cb.InsertString(0, hwair, strlen(hwair), startSequence);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
	}

	SECTION("Deletion") {
		cb.SetUTF8Substance(true);

		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		const char *hwair = "a\xF0\x90\x8D\x88z";
		cb.InsertString(0, hwair, strlen(hwair), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 4);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);

		cb.DeleteChars(5, 1, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);

		cb.DeleteChars(1, 4, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 1);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 1);
	}

	SECTION("Insert Complex") {
		cb.SetUTF8Substance(true);
		cb.SetLineEndTypes(LineEndType::Unicode);
		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		// 3 lines of text containing 8 bytes
		const char *data = "a\n\xF0\x90\x8D\x88\nz";
		cb.InsertString(0, data, strlen(data), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 5);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 6);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 4);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 5);

		// Insert a new line at end -> "a\n\xF0\x90\x8D\x88\nz\n" 4 lines
		// Last line empty
		cb.InsertString(strlen(data), "\n", 1, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 5);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 7);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf16) == 7);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 4);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf32) == 6);

		// Insert a new line before end -> "a\n\xF0\x90\x8D\x88\nz\n\n" 5 lines
		cb.InsertString(strlen(data), "\n", 1, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 5);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 7);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf16) == 8);
		REQUIRE(cb.IndexLineStart(5, LineCharacterIndexType::Utf16) == 8);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 4);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf32) == 7);
		REQUIRE(cb.IndexLineStart(5, LineCharacterIndexType::Utf32) == 7);

		// Insert a valid 3-byte UTF-8 character at start ->
		// "\xE2\x82\xACa\n\xF0\x90\x8D\x88\nz\n\n" 5 lines

		const char *euro = "\xE2\x82\xAC";
		cb.InsertString(0, euro, strlen(euro), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 6);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 8);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf16) == 9);
		REQUIRE(cb.IndexLineStart(5, LineCharacterIndexType::Utf16) == 9);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 5);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 7);
		REQUIRE(cb.IndexLineStart(4, LineCharacterIndexType::Utf32) == 8);
		REQUIRE(cb.IndexLineStart(5, LineCharacterIndexType::Utf32) == 8);

		// Insert a lone lead byte implying a 3 byte character at start of line 2 ->
		// "\xE2\x82\xACa\n\EF\xF0\x90\x8D\x88\nz\n\n" 5 lines
		// Should be treated as a single byte character

		const char *lead = "\xEF";
		cb.InsertString(5, lead, strlen(lead), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 7);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 9);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 8);

		// Insert an ASCII lead byte inside the 3-byte initial character ->
		// "\xE2!\x82\xACa\n\EF\xF0\x90\x8D\x88\nz\n\n" 5 lines
		// It should b treated as a single character and should cause the
		// byte before and the 2 bytes after also be each treated as singles
		// so 3 more characters on line 0.

		const char *ascii = "!";
		cb.InsertString(1, ascii, strlen(ascii), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 6);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 10);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 9);

		// Insert a NEL after the '!' to trigger the utf8 line end case ->
		// "\xE2!\xC2\x85 \x82\xACa\n \EF\xF0\x90\x8D\x88\n z\n\n" 5 lines

		const char *nel = "\xC2\x85";
		cb.InsertString(2, nel, strlen(nel), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 7);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 11);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 7);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 10);
	}

	SECTION("Delete Multiple lines") {
		cb.SetUTF8Substance(true);
		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		// 3 lines of text containing 8 bytes
		const char *data = "a\n\xF0\x90\x8D\x88\nz\nc";
		cb.InsertString(0, data, strlen(data), startSequence);

		// Delete first 2 new lines -> "az\nc"
		cb.DeleteChars(1, strlen(data) - 4, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 4);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 4);
	}

	SECTION("Delete Complex") {
		cb.SetUTF8Substance(true);
		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		// 3 lines of text containing 8 bytes
		const char *data = "a\n\xF0\x90\x8D\x88\nz";
		cb.InsertString(0, data, strlen(data), startSequence);

		// Delete lead byte from character on line 1 ->
		// "a\n\x90\x8D\x88\nz"
		// line 1 becomes 4 single byte characters
		cb.DeleteChars(2, 1, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 6);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 7);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf32) == 7);

		// Delete first new line ->
		// "a\x90\x8D\x88\nz"
		// Only 2 lines with line 0 containing 5 single byte characters
		cb.DeleteChars(1, 1, startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 5);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 6);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 5);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 6);

		// Restore lead byte from character on line 0 making a 4-byte character ->
		// "a\xF0\x90\x8D\x88\nz"

		const char *lead4 = "\xF0";
		cb.InsertString(1, lead4, strlen(lead4), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 4);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 5);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 3);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 4);
	}

	SECTION("Insert separates new line bytes") {
		cb.SetUTF8Substance(true);
		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		// 2 lines of text containing 4 bytes
		const char *data = "a\r\nb";
		cb.InsertString(0, data, strlen(data), startSequence);

		// 3 lines of text containing 5 bytes ->
		// "a\r!\nb"
		const char *ascii = "!";
		cb.InsertString(2, ascii, strlen(ascii), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 4);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 5);
	}
}
