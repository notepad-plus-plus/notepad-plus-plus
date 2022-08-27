/** @file testCellBuffer.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cassert>
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
#include "SparseVector.h"
#include "ChangeHistory.h"
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
		const char *cpChange = cb.InsertString(0, sText, sLength, startSequence);
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
		const char *cpChange = cb.InsertString(0, sText2, sLength2, startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength2 == cb.Length());
		REQUIRE(memcmp(cpChange, sText2, sLength2) == 0);
		REQUIRE(2 == cb.Lines());
		REQUIRE(0 == cb.LineStart(0));
		REQUIRE(0 == cb.LineFromPosition(0));
		REQUIRE(4 == cb.LineStart(1));
		REQUIRE(1 == cb.LineFromPosition(5));
		REQUIRE(sLength2 == cb.LineStart(2));
		REQUIRE(1 == cb.LineFromPosition(sLength2));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("UndoOff") {
		REQUIRE(cb.IsCollectingUndo());
		cb.SetUndoCollection(false);
		REQUIRE(!cb.IsCollectingUndo());
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText, sLength, startSequence);
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
		const char *cpChange = cb.InsertString(0, sText, sLength, startSequence);
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
		cb.InsertString(0, sText, sLength, startSequence);
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

TEST_CASE("ChangeHistory") {

	ChangeHistory il;
	const EditionSet empty;
	struct Spanner {
		Sci::Position position = 0;
		Sci::Position length = 0;
	};

	SECTION("Start") {
		REQUIRE(il.Length() == 0);
		REQUIRE(il.DeletionCount(0,0) == 0);
		REQUIRE(il.EditionAt(0) == 0);
		REQUIRE(il.EditionEndRun(0) == 0);
		REQUIRE(il.EditionDeletesAt(0) == 0);
		REQUIRE(il.EditionNextDelete(0) == 1);
	}

	SECTION("Some Space") {
		il.Insert(0, 10, false, true);
		REQUIRE(il.Length() == 10);
		REQUIRE(il.DeletionCount(0,10) == 0);
		REQUIRE(il.EditionAt(0) == 0);
		REQUIRE(il.EditionEndRun(0) == 10);
		REQUIRE(il.EditionDeletesAt(0) == 0);
		REQUIRE(il.EditionNextDelete(0) == 10);
		REQUIRE(il.EditionDeletesAt(10) == 0);
		REQUIRE(il.EditionNextDelete(10) == 11);
	}

	SECTION("An insert") {
		il.Insert(0, 7, false, true);
		il.SetSavePoint();
		il.Insert(2, 3, true, true);
		REQUIRE(il.Length() == 10);
		REQUIRE(il.DeletionCount(0,10) == 0);

		REQUIRE(il.EditionAt(0) == 0);
		REQUIRE(il.EditionEndRun(0) == 2);
		REQUIRE(il.EditionAt(2) == 2);
		REQUIRE(il.EditionEndRun(2) == 5);
		REQUIRE(il.EditionAt(5) == 0);
		REQUIRE(il.EditionEndRun(5) == 10);
		REQUIRE(il.EditionAt(10) == 0);

		REQUIRE(il.EditionDeletesAt(0) == 0);
		REQUIRE(il.EditionNextDelete(0) == 10);
		REQUIRE(il.EditionDeletesAt(10) == 0);
	}

	SECTION("A delete") {
		il.Insert(0, 10, false, true);
		il.SetSavePoint();
		il.DeleteRangeSavingHistory(2, 3, true, false);
		REQUIRE(il.Length() == 7);
		REQUIRE(il.DeletionCount(0,7) == 1);

		REQUIRE(il.EditionAt(0) == 0);
		REQUIRE(il.EditionEndRun(0) == 7);
		REQUIRE(il.EditionAt(7) == 0);

		REQUIRE(il.EditionDeletesAt(0) == 0);
		const EditionSet one{ 2 };
		REQUIRE(il.EditionNextDelete(0) == 2);
		REQUIRE(il.EditionDeletesAt(2) == 2);
		REQUIRE(il.EditionNextDelete(2) == 7);
		REQUIRE(il.EditionDeletesAt(7) == 0);
	}

	SECTION("Insert, delete, and undo") {
		il.Insert(0, 9, false, true);
		il.SetSavePoint();
		il.Insert(3, 1, true, true);
		REQUIRE(il.EditionEndRun(0) == 3);
		REQUIRE(il.EditionAt(3) == 2);
		REQUIRE(il.EditionEndRun(3) == 4);
		REQUIRE(il.EditionAt(4) == 0);

		il.DeleteRangeSavingHistory(2, 3, true, false);
		REQUIRE(il.Length() == 7);
		REQUIRE(il.DeletionCount(0,7) == 1);

		REQUIRE(il.EditionAt(0) == 0);
		REQUIRE(il.EditionEndRun(0) == 7);
		REQUIRE(il.EditionAt(7) == 0);

		REQUIRE(il.EditionDeletesAt(0) == 0);
		const EditionSet one{ 2 };
		REQUIRE(il.EditionNextDelete(0) == 2);
		REQUIRE(il.EditionDeletesAt(2) == 2);
		REQUIRE(il.EditionNextDelete(2) == 7);
		REQUIRE(il.EditionDeletesAt(7) == 0);

		// Undo in detail (normally inside CellBuffer::PerformUndoStep)
		il.UndoDeleteStep(2, 3, false);
		REQUIRE(il.Length() == 10);
		REQUIRE(il.DeletionCount(0, 10) == 0);
		// The insertion has reappeared
		REQUIRE(il.EditionEndRun(0) == 3);
		REQUIRE(il.EditionAt(3) == 2);
		REQUIRE(il.EditionEndRun(3) == 4);
		REQUIRE(il.EditionAt(4) == 0);
	}

	SECTION("Deletes") {
		il.Insert(0, 10, false, true);
		il.SetSavePoint();
		il.DeleteRangeSavingHistory(2, 3, true, false);
		REQUIRE(il.Length() == 7);
		REQUIRE(il.DeletionCount(0,7) == 1);

		REQUIRE(il.EditionDeletesAt(0) == 0);
		REQUIRE(il.EditionNextDelete(0) == 2);
		REQUIRE(il.EditionDeletesAt(2) == 2);
		REQUIRE(il.EditionNextDelete(2) == 7);
		REQUIRE(il.EditionDeletesAt(7) == 0);

		il.DeleteRangeSavingHistory(2, 1, true, false);
		REQUIRE(il.Length() == 6);
		REQUIRE(il.DeletionCount(0,6) == 2);

		REQUIRE(il.EditionDeletesAt(0) == 0);
		REQUIRE(il.EditionNextDelete(0) == 2);
		REQUIRE(il.EditionDeletesAt(2) == 2);
		REQUIRE(il.EditionNextDelete(2) == 6);
		REQUIRE(il.EditionDeletesAt(6) == 0);

		// Undo in detail (normally inside CellBuffer::PerformUndoStep)
		il.UndoDeleteStep(2, 1, false);
		REQUIRE(il.Length() == 7);
		REQUIRE(il.DeletionCount(0, 7) == 1);

		// Undo in detail (normally inside CellBuffer::PerformUndoStep)
		il.UndoDeleteStep(2, 3, false);
		REQUIRE(il.Length() == 10);
		REQUIRE(il.DeletionCount(0, 10) == 0);
	}

	SECTION("Deletes 101") {
		// Deletes that hit the start and end permanent positions
		il.Insert(0, 3, false, true);
		il.SetSavePoint();
		REQUIRE(il.DeletionCount(0, 2) == 0);
		il.DeleteRangeSavingHistory(1, 1, true, false);
		REQUIRE(il.DeletionCount(0,2) == 1);
		const EditionSet at1 = {2};
		REQUIRE(il.DeletionsAt(1) == at1);
		il.DeleteRangeSavingHistory(1, 1, false, false);
		REQUIRE(il.DeletionCount(0,1) == 2);
		const EditionSet at2 = { 2, 3 };
		REQUIRE(il.DeletionsAt(1) == at2);
		il.DeleteRangeSavingHistory(0, 1, false, false);
		const EditionSet at3 = { 2, 3, 3 };
		REQUIRE(il.DeletionsAt(0) == at3);
		REQUIRE(il.DeletionCount(0,0) == 3);

		// Undo them
		il.UndoDeleteStep(0, 1, false);
		REQUIRE(il.DeletionCount(0, 1) == 2);
		REQUIRE(il.DeletionsAt(1) == at2);
		il.UndoDeleteStep(1, 1, false);
		REQUIRE(il.DeletionCount(0, 2) == 1);
		REQUIRE(il.DeletionsAt(1) == at1);
		il.UndoDeleteStep(1, 1, false);
		REQUIRE(il.DeletionCount(0, 3) == 0);
	}

	SECTION("Deletes Stack") {
		std::vector<Spanner> spans = {
			{5, 1},
			{4, 3},
			{1, 1},
			{1, 1},
			{0, 1},
			{0, 3},
		};

		// Deletes that hit the start and end permanent positions
		il.Insert(0, 10, false, true);
		REQUIRE(il.Length() == 10);
		il.SetSavePoint();
		REQUIRE(il.DeletionCount(0, 10) == 0);
		for (size_t i = 0; i < std::size(spans); i++) {
			il.DeleteRangeSavingHistory(spans[i].position, spans[i].length, false, false);
		}
		REQUIRE(il.Length() == 0);
		for (size_t j = 0; j < std::size(spans); j++) {
			const size_t i = std::size(spans) - j - 1;
			il.UndoDeleteStep(spans[i].position, spans[i].length, false);
		}
		REQUIRE(il.DeletionCount(0, 10) == 0);
		REQUIRE(il.Length() == 10);
	}
}

struct InsertionResult {
	Sci::Position position;
	Sci::Position length;
	int state;
	bool operator==(const InsertionResult &other) const noexcept {
		return position == other.position &&
			length == other.length &&
			state == other.state;
	}
};

std::ostream &operator << (std::ostream &os, InsertionResult const &value) {
	os << value.position << " " << value.length << " " << value.state;
	return os;
}

using Insertions = std::vector<InsertionResult>;

std::ostream &operator << (std::ostream &os, Insertions const &value) {
	os << "(";
	for (const InsertionResult &el : value) {
		os << "(" << el << ") ";
	}
	os << ")";
	return os;
}

Insertions HistoryInsertions(const CellBuffer &cb) {
	Insertions result;
	Sci::Position startPos = 0;
	while (startPos < cb.Length()) {
		const Sci::Position endPos = cb.EditionEndRun(startPos);
		const int ed = cb.EditionAt(startPos);
		if (ed) {
			result.push_back({ startPos, endPos - startPos, ed });
		}
		startPos = endPos;
	}
	return result;
}

struct DeletionResult {
	Sci::Position position;
	int state;
	bool operator==(const DeletionResult &other) const noexcept {
		return position == other.position &&
			state == other.state;
	}
};

std::ostream &operator << (std::ostream &os, DeletionResult const &value) {
	os << value.position << " " << value.state;
	return os;
}

using Deletions = std::vector<DeletionResult>;

std::ostream &operator << (std::ostream &os, Deletions const &value) {
	os << "(";
	for (const DeletionResult &el : value) {
		os << "(" << el << ") ";
	}
	os << ")";
	return os;
}

Deletions HistoryDeletions(const CellBuffer &cb) {
	Deletions result;
	Sci::Position positionDeletion = 0;
	while (positionDeletion <= cb.Length()) {
		const unsigned int editions = cb.EditionDeletesAt(positionDeletion);
		if (editions & 1) {
			result.push_back({ positionDeletion, 1 });
		}
		if (editions & 2) {
			result.push_back({ positionDeletion, 2 });
		}
		if (editions & 4) {
			result.push_back({ positionDeletion, 3 });
		}
		if (editions & 8) {
			result.push_back({ positionDeletion, 4 });
		}
		positionDeletion = cb.EditionNextDelete(positionDeletion);
	}
	return result;
}

struct History {
	Insertions insertions;
	Deletions deletions;
	bool operator==(const History &other) const {
		return insertions == other.insertions &&
			deletions == other.deletions;
	}
};

std::ostream &operator << (std::ostream &os, History const &value) {
	os << value.insertions << " " << value.deletions;
	return os;
}

History HistoryOf(const CellBuffer &cb) {
	return { HistoryInsertions(cb), HistoryDeletions(cb) };
}

void UndoBlock(CellBuffer &cb) {
	const int steps = cb.StartUndo();
	for (int step = 0; step < steps; step++) {
		cb.PerformUndoStep();
	}
}

void RedoBlock(CellBuffer &cb) {
	const int steps = cb.StartRedo();
	for (int step = 0; step < steps; step++) {
		cb.PerformRedoStep();
	}
}

TEST_CASE("CellBufferWithChangeHistory") {

	SECTION("StraightUndoRedoSaveRevertRedo") {
		CellBuffer cb(true, false);
		cb.SetUndoCollection(false);
		std::string sInsert = "abcdefghijklmnopqrstuvwxyz";
		bool startSequence = false;
		cb.InsertString(0, sInsert.c_str(), sInsert.length(), startSequence);
		cb.SetUndoCollection(true);
		cb.SetSavePoint();
		cb.ChangeHistorySet(true);

		const History history0 { {}, {} };
		REQUIRE(HistoryOf(cb) == history0);

		// 1
		cb.InsertString(4, "_", 1, startSequence);
		const History history1{ {{4, 1, 3}}, {} };
		REQUIRE(HistoryOf(cb) == history1);

		// 2
		cb.DeleteChars(2, 1, startSequence);
		const History history2{ {{3, 1, 3}},
			{{2, 3}} };
		REQUIRE(HistoryOf(cb) == history2);

		// 3
		cb.InsertString(1, "[!]", 3, startSequence);
		const History history3{ { {1, 3, 3}, {6, 1, 3} },
			{ {5, 3} } };
		REQUIRE(HistoryOf(cb) == history3);

		// 4
		cb.DeleteChars(2, 1, startSequence);	// Inside an insertion
		const History history4{ { {1, 2, 3}, {5, 1, 3} },
			{ {2, 3}, {4, 3} }};
		REQUIRE(HistoryOf(cb) == history4);

		// 5 Delete all the insertions and deletions
		cb.DeleteChars(1, 6, startSequence);	// Inside an insertion
		const History history5{ { },
			{ {1, 3} } };
		REQUIRE(HistoryOf(cb) == history5);

		// Undo all
		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history4);

		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history3);

		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history2);

		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history1);

		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history0);

		// Redo all
		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history1);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history2);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history3);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history4);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history5);

		cb.SetSavePoint();
		const History history5s{ { },
			{ {1, 2} } };
		REQUIRE(HistoryOf(cb) == history5s);

		// Change past save point
		cb.InsertString(4, "123", 3, startSequence);
		const History history6{ { {4, 3, 3} },
			{ {1, 2} } };
		REQUIRE(HistoryOf(cb) == history6);

		// Undo to save point: same as 5 but with save state instead of unsaved
		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == history5s);

		// Reverting past save point, similar to 4 but with most saved and
		// reverted delete at 1
		UndoBlock(cb);	// Reinsert most of original changes
		const History history4s{ { {1, 2, 4}, {3, 2, 1}, {5, 1, 4}, {6, 1, 1} },
			{ {2, 2}, {4, 2} } };
		REQUIRE(HistoryOf(cb) == history4s);

		UndoBlock(cb);	// Reinsert "!", 
		const History history3s{ { {1, 3, 4}, {4, 2, 1}, {6, 1, 4}, {7, 1, 1} },
			{ {5, 2} } };
		REQUIRE(HistoryOf(cb) == history3s);

		UndoBlock(cb);	// Revert insertion of [!]
		const History history2s{ { {1, 2, 1}, {3, 1, 4}, {4, 1, 1} },
			{ {1, 1}, {2, 2} } };
		REQUIRE(HistoryOf(cb) == history2s);

		UndoBlock(cb);	// Revert deletion, inserts at 2
		const History history1s{ { {1, 3, 1}, {4, 1, 4}, {5, 1, 1} },
			{ {1, 1} } };
		REQUIRE(HistoryOf(cb) == history1s);

		UndoBlock(cb);	// Revert insertion of _ at 4, drops middle insertion run
		// So merges down to 1 insertion
		const History history0s{ { {1, 4, 1} },
			{ {1, 1}, {4, 1} } };
		REQUIRE(HistoryOf(cb) == history0s);

		// At origin but with changes from disk
		// Now redo the steps

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history1s);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history2s);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history3s);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history4s);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history5s);

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == history6);
	}

	SECTION("Detached") {
		CellBuffer cb(true, false);
		cb.SetUndoCollection(false);
		std::string sInsert = "abcdefghijklmnopqrstuvwxyz";
		bool startSequence = false;
		cb.InsertString(0, sInsert.c_str(), sInsert.length(), startSequence);
		cb.SetUndoCollection(true);
		cb.SetSavePoint();
		cb.ChangeHistorySet(true);

		const History history0{ {}, {} };
		REQUIRE(HistoryOf(cb) == history0);

		// 1
		cb.InsertString(4, "_", 1, startSequence);
		const History history1{ {{4, 1, 3}}, {} };
		REQUIRE(HistoryOf(cb) == history1);

		// 2
		cb.DeleteChars(2, 1, startSequence);
		const History history2{ {{3, 1, 3}},
			{{2, 3}} };
		REQUIRE(HistoryOf(cb) == history2);

		cb.SetSavePoint();

		UndoBlock(cb);
		const History history1s{ {{2, 1, 1}, {4, 1, 2}}, {} };
		REQUIRE(HistoryOf(cb) == history1s);

		cb.InsertString(6, "()", 2, startSequence);
		const History detached2{ {{2, 1, 1}, {4, 1, 2}, {6, 2, 3}}, {} };
		REQUIRE(HistoryOf(cb) == detached2);

		cb.DeleteChars(9, 3, startSequence);
		const History detached3{ {{2, 1, 1}, {4, 1, 2}, {6, 2, 3}}, {{9,3}} };
		REQUIRE(HistoryOf(cb) == detached3);

		UndoBlock(cb);
		REQUIRE(HistoryOf(cb) == detached2);
		UndoBlock(cb);
		const History detached1{ {{2, 1, 1}, {4, 1, 2}}, {} };
		REQUIRE(HistoryOf(cb) == detached1);
		UndoBlock(cb);
		const History detached0{ {{2, 1, 1}}, {{4,1}} };
		REQUIRE(HistoryOf(cb) == detached0);
		REQUIRE(!cb.CanUndo());

		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == detached1);
		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == detached2);
		RedoBlock(cb);
		REQUIRE(HistoryOf(cb) == detached3);
	}
}

namespace {

// Implement low quality reproducible pseudo-random numbers.
// Pseudo-random algorithm based on R. G. Dromey "How to Solve it by Computer" page 122.

class RandomSequence {
	static constexpr int mult = 109;
	static constexpr int incr = 853;
	static constexpr int modulus = 4096;
	int randomValue = 127;
public:
	int Next() noexcept {
		randomValue = (mult * randomValue + incr) % modulus;
		return randomValue;
	}
};

}

#if 1
TEST_CASE("CellBufferLong") {

	// Call methods on CellBuffer pseudo-randomly trying  to trigger assertion failures

	CellBuffer cb(true, false);

	SECTION("Random") {
		RandomSequence rseq;
		for (size_t i = 0l; i < 20000; i++) {
			const int r = rseq.Next() % 10;
			if (r <= 2) {			// 30%
				// Insert text
				const int pos = rseq.Next() % (cb.Length() + 1);
				const int len = rseq.Next() % 10 + 1;
				std::string sInsert;
				for (int j = 0; j < len; j++) {
					sInsert.push_back(static_cast<char>('a' + j));
				}
				bool startSequence = false;
				cb.InsertString(pos, sInsert.c_str(), len, startSequence);
			} else if (r <= 5) {	// 30%
				// Delete Text
				const Sci::Position pos = rseq.Next() % (cb.Length() + 1);
				const int len = rseq.Next() % 10 + 1;
				if (pos + len <= cb.Length()) {
					bool startSequence = false;
					cb.DeleteChars(pos, len, startSequence);
				}
			} else if (r <= 8) {	// 30%
				// Undo or redo
				const bool undo = rseq.Next() % 2 == 1;
				if (undo) {
					UndoBlock(cb);
				} else {
					RedoBlock(cb);
				}
			} else {	// 10%
				// Save
				cb.SetSavePoint();
			}
		}
	}
}
#endif