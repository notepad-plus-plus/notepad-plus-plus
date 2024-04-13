/** @file testCellBuffer.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
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
#include "UndoHistory.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test CellBuffer.
bool Equal(const char *ptr, std::string_view sv) noexcept {
	return memcmp(ptr, sv.data(), sv.length()) == 0;
}

TEST_CASE("ScrapStack") {

	ScrapStack ss;

	SECTION("Push") {
		const char *t = ss.Push("abc", 3);
		REQUIRE(memcmp(t, "abc", 3) == 0);

		ss.MoveBack(3);
		const char *text = ss.CurrentText();
		REQUIRE(memcmp(text, "abc", 3) == 0);

		ss.MoveForward(1);
		const char *text2 = ss.CurrentText();
		REQUIRE(memcmp(text2, "bc", 2) == 0);

		ss.SetCurrent(1);
		const char *text3 = ss.CurrentText();
		REQUIRE(memcmp(text3, "bc", 2) == 0);

		const char *text4 = ss.TextAt(2);
		REQUIRE(memcmp(text4, "c", 1) == 0);

		ss.Clear();
		const char *text5 = ss.Push("1", 1);
		REQUIRE(memcmp(text5, "1", 1) == 0);
	}
}

TEST_CASE("CellBuffer") {

	constexpr std::string_view sText = "Scintilla";
	constexpr Sci::Position sLength = sText.length();

	CellBuffer cb(true, false);

	SECTION("InsertOneLine") {
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText.data(), sLength, startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(Equal(cpChange, sText));
		REQUIRE(1 == cb.Lines());
		REQUIRE(0 == cb.LineStart(0));
		REQUIRE(0 == cb.LineFromPosition(0));
		REQUIRE(sLength == cb.LineStart(1));
		REQUIRE(0 == cb.LineFromPosition(static_cast<int>(sLength)));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("InsertTwoLines") {
		constexpr std::string_view sText2 = "Two\nLines";
		constexpr Sci::Position sLength2 = sText2.length();
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText2.data(), sLength2, startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength2 == cb.Length());
		REQUIRE(Equal(cpChange, sText2));
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

	SECTION("LineEnds") {
		// Check that various line ends produce correct result from LineEnd.
		cb.SetLineEndTypes(LineEndType::Unicode);
		bool startSequence = false;
		{
			// Unix \n
			constexpr std::string_view sText2 = "Two\nLines";
			constexpr Sci::Position sLength2 = sText2.length();
			cb.InsertString(0, sText2.data(), sLength2, startSequence);
			REQUIRE(3 == cb.LineEnd(0));
			REQUIRE(sLength2 == cb.LineEnd(1));
			cb.DeleteChars(0, sLength2, startSequence);
		}
		{
			// Windows \r\n
			constexpr std::string_view sText2 = "Two\r\nLines";
			constexpr Sci::Position sLength2 = sText2.length();
			cb.InsertString(0, sText2.data(), sLength2, startSequence);
			REQUIRE(3 == cb.LineEnd(0));
			REQUIRE(sLength2 == cb.LineEnd(1));
			cb.DeleteChars(0, sLength2, startSequence);
		}
		{
			// Old macOS \r
			constexpr std::string_view sText2 = "Two\rLines";
			constexpr Sci::Position sLength2 = sText2.length();
			cb.InsertString(0, sText2.data(), sLength2, startSequence);
			REQUIRE(3 == cb.LineEnd(0));
			REQUIRE(sLength2 == cb.LineEnd(1));
			cb.DeleteChars(0, sLength2, startSequence);
		}
		{
			// Unicode NEL is U+0085 \xc2\x85
			constexpr std::string_view sText2 = "Two\xc2\x85Lines";
			constexpr Sci::Position sLength2 = sText2.length();
			cb.InsertString(0, sText2.data(), sLength2, startSequence);
			REQUIRE(3 == cb.LineEnd(0));
			REQUIRE(sLength2 == cb.LineEnd(1));
			cb.DeleteChars(0, sLength2, startSequence);
		}
		{
			// Unicode LS line separator is U+2028 \xe2\x80\xa8
			constexpr std::string_view sText2 = "Two\xe2\x80\xa8Lines";
			constexpr Sci::Position sLength2 = sText2.length();
			cb.InsertString(0, sText2.data(), sLength2, startSequence);
			REQUIRE(3 == cb.LineEnd(0));
			REQUIRE(sLength2 == cb.LineEnd(1));
			cb.DeleteChars(0, sLength2, startSequence);
		}
		cb.SetLineEndTypes(LineEndType::Default);
	}

	SECTION("UndoOff") {
		REQUIRE(cb.IsCollectingUndo());
		cb.SetUndoCollection(false);
		REQUIRE(!cb.IsCollectingUndo());
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText.data(), sLength, startSequence);
		REQUIRE(!startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(Equal(cpChange, sText));
		REQUIRE(!cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("UndoRedo") {
		constexpr std::string_view sTextDeleted = "ci";
		constexpr std::string_view sTextAfterDeletion = "Sntilla";
		bool startSequence = false;
		const char *cpChange = cb.InsertString(0, sText.data(), sLength, startSequence);
		REQUIRE(startSequence);
		REQUIRE(sLength == cb.Length());
		REQUIRE(Equal(cpChange, sText));
		REQUIRE(Equal(cb.BufferPointer(), sText));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
		const char *cpDeletion = cb.DeleteChars(1, 2, startSequence);
		REQUIRE(startSequence);
		REQUIRE(Equal(cpDeletion, sTextDeleted));
		REQUIRE(Equal(cb.BufferPointer(), sTextAfterDeletion));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());

		int steps = cb.StartUndo();
		REQUIRE(steps == 1);
		cb.PerformUndoStep();
		REQUIRE(Equal(cb.BufferPointer(), sText));
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
		REQUIRE(Equal(cb.BufferPointer(), sText));
		REQUIRE(cb.CanUndo());
		REQUIRE(cb.CanRedo());

		steps = cb.StartRedo();
		REQUIRE(steps == 1);
		cb.PerformRedoStep();
		REQUIRE(Equal(cb.BufferPointer(), sTextAfterDeletion));
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
		cb.InsertString(0, sText.data(), sLength, startSequence);
		REQUIRE(cb.Length() == 0);
	}

}

bool Equal(const Action &a, ActionType at, Sci::Position position, std::string_view value) noexcept {
	// Currently ignores mayCoalesce since this is not set consistently when following
	// start action implies it.
	if (a.at != at)
		return false;
	if (a.position != position)
		return false;
	if (a.lenData != static_cast<Sci::Position>(value.length()))
		return false;
	if (memcmp(a.data, value.data(), a.lenData) != 0)
		return false;
	return true;
}

bool EqualContainerAction(const Action &a, Sci::Position token) noexcept {
	// Currently ignores mayCoalesce
	if (a.at != ActionType::container)
		return false;
	if (a.position != token)
		return false;
	if (a.lenData != 0)
		return false;
	if (a.data)
		return false;
	return true;
}

void TentativeUndo(UndoHistory &uh) noexcept {
	const int steps = uh.TentativeSteps();
	for (int step = 0; step < steps; step++) {
		/* const Action &actionStep = */ uh.GetUndoStep();
		uh.CompletedUndoStep();
	}
	uh.TentativeCommit();
}

TEST_CASE("ScaledVector") {

	ScaledVector sv;
	
	SECTION("ScalingUp") {
		sv.ReSize(1);
		REQUIRE(sv.SizeInBytes() == 1);
		REQUIRE(sv.ValueAt(0) == 0);
		sv.SetValueAt(0, 1);
		REQUIRE(sv.ValueAt(0) == 1);
		REQUIRE(sv.SignedValueAt(0) == 1);
		sv.ClearValueAt(0);
		REQUIRE(sv.ValueAt(0) == 0);

		// Check boundary of 1-byte values
		sv.SetValueAt(0, 0xff);
		REQUIRE(sv.ValueAt(0) == 0xff);
		REQUIRE(sv.SizeInBytes() == 1);
		// Require expansion to 2 byte elements
		sv.SetValueAt(0, 0x100);
		REQUIRE(sv.ValueAt(0) == 0x100);
		REQUIRE(sv.SizeInBytes() == 2);
		// Only ever expands, never diminishes element size
		sv.SetValueAt(0, 0xff);
		REQUIRE(sv.ValueAt(0) == 0xff);
		REQUIRE(sv.SizeInBytes() == 2);

		// Check boundary of 2-byte values
		sv.SetValueAt(0, 0xffff);
		REQUIRE(sv.ValueAt(0) == 0xffff);
		REQUIRE(sv.SizeInBytes() == 2);
		// Require expansion to 2 byte elements
		sv.SetValueAt(0, 0x10000);
		REQUIRE(sv.ValueAt(0) == 0x10000);
		REQUIRE(sv.SizeInBytes() == 3);

		// Check that its not just simple bit patterns that work
		sv.SetValueAt(0, 0xd4381);
		REQUIRE(sv.ValueAt(0) == 0xd4381);

		// Add a second item
		sv.ReSize(2);
		REQUIRE(sv.SizeInBytes() == 6);
		// Truncate
		sv.Truncate(1);
		REQUIRE(sv.SizeInBytes() == 3);
		REQUIRE(sv.ValueAt(0) == 0xd4381);

		sv.Clear();
		REQUIRE(sv.Size() == 0);
		sv.PushBack();
		REQUIRE(sv.Size() == 1);
		REQUIRE(sv.SizeInBytes() == 3);
		sv.SetValueAt(0, 0x1fd4381);
		REQUIRE(sv.SizeInBytes() == 4);
		REQUIRE(sv.ValueAt(0) == 0x1fd4381);
	}
}

TEST_CASE("UndoHistory") {

	UndoHistory uh;

	SECTION("Basics") {
		REQUIRE(uh.IsSavePoint());
		REQUIRE(uh.AfterSavePoint());
		REQUIRE(!uh.BeforeSavePoint());
		REQUIRE(!uh.BeforeReachableSavePoint());
		REQUIRE(!uh.CanUndo());
		REQUIRE(!uh.CanRedo());

		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);
		REQUIRE(!uh.IsSavePoint());
		REQUIRE(uh.AfterSavePoint());
		REQUIRE(uh.CanUndo());
		REQUIRE(!uh.CanRedo());
		val = uh.AppendAction(ActionType::remove, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);

		// Undoing
		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 1);
			const Action action = uh.GetUndoStep();
			REQUIRE(Equal(action, ActionType::remove, 0, "ab"));
			uh.CompletedUndoStep();
		}
		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 1);
			const Action action = uh.GetUndoStep();
			REQUIRE(Equal(action, ActionType::insert, 0, "ab"));
			uh.CompletedUndoStep();
		}

		REQUIRE(uh.IsSavePoint());

		// Redoing
		{
			const int steps = uh.StartRedo();
			REQUIRE(steps == 1);
			const Action action = uh.GetRedoStep();
			REQUIRE(Equal(action, ActionType::insert, 0, "ab"));
			uh.CompletedRedoStep();
		}
		{
			const int steps = uh.StartRedo();
			REQUIRE(steps == 1);
			const Action action = uh.GetRedoStep();
			REQUIRE(Equal(action, ActionType::remove, 0, "ab"));
			uh.CompletedRedoStep();
		}

		REQUIRE(!uh.IsSavePoint());
	}

	SECTION("EnsureTruncationAfterUndo") {

		REQUIRE(uh.Actions() == 0);
		bool startSequence = false;
		uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(uh.Actions() == 1);
		uh.AppendAction(ActionType::insert, 2, "cd", 2, startSequence, true);
		REQUIRE(uh.Actions() == 2);
		REQUIRE(uh.CanUndo());
		REQUIRE(!uh.CanRedo());

		// Undoing
		const int steps = uh.StartUndo();
		REQUIRE(steps == 2);
		uh.GetUndoStep();
		uh.CompletedUndoStep();
		REQUIRE(uh.Actions() == 2);	// Not truncated until forward action
		uh.GetUndoStep();
		uh.CompletedUndoStep();
		REQUIRE(uh.Actions() == 2);

		// Perform action which should truncate history
		uh.AppendAction(ActionType::insert, 0, "12", 2, startSequence, true);
		REQUIRE(uh.Actions() == 1);
	}

	SECTION("Coalesce") {

		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);
		REQUIRE(!uh.IsSavePoint());
		REQUIRE(uh.AfterSavePoint());
		REQUIRE(uh.CanUndo());
		REQUIRE(!uh.CanRedo());
		val = uh.AppendAction(ActionType::insert, 2, "cd", 2, startSequence, true);
		REQUIRE(memcmp(val, "cd", 2) == 0);
		REQUIRE(!startSequence);

		// Undoing
		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 2);
			const Action action2 = uh.GetUndoStep();
			REQUIRE(Equal(action2, ActionType::insert, 2, "cd"));
			uh.CompletedUndoStep();
			const Action action1 = uh.GetUndoStep();
			REQUIRE(Equal(action1, ActionType::insert, 0, "ab"));
			uh.CompletedUndoStep();
		}

		REQUIRE(uh.IsSavePoint());

		// Redoing
		{
			const int steps = uh.StartRedo();
			REQUIRE(steps == 2);
			const Action action1 = uh.GetRedoStep();
			REQUIRE(Equal(action1, ActionType::insert, 0, "ab"));
			uh.CompletedRedoStep();
			const Action action2 = uh.GetRedoStep();
			REQUIRE(Equal(action2, ActionType::insert, 2, "cd"));
			uh.CompletedRedoStep();
		}

		REQUIRE(!uh.IsSavePoint());

	}

	SECTION("SimpleContainer") {
		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::container, 1000, nullptr, 0, startSequence, true);
		REQUIRE(startSequence);
		REQUIRE(!val);
		val = uh.AppendAction(ActionType::container, 1001, nullptr, 0, startSequence, true);
		REQUIRE(!startSequence);
		REQUIRE(!val);
	}

	SECTION("CoalesceContainer") {
		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);
		val = uh.AppendAction(ActionType::container, 1000, nullptr, 0, startSequence, true);
		REQUIRE(!startSequence);
		// container actions do not have text data, just the token store in position
		REQUIRE(!val);
		uh.AppendAction(ActionType::container, 1001, nullptr, 0, startSequence, true);
		REQUIRE(!startSequence);
		// This is a coalescible change since the container actions are skipped to determine compatibility
		val = uh.AppendAction(ActionType::insert, 2, "cd", 2, startSequence, true);
		REQUIRE(memcmp(val, "cd", 2) == 0);
		REQUIRE(!startSequence);
		// Break the sequence with a non-coalescible container action
		uh.AppendAction(ActionType::container, 1002, nullptr, 0, startSequence, false);
		REQUIRE(startSequence);

		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 1);
			const Action actionContainer = uh.GetUndoStep();
			REQUIRE(EqualContainerAction(actionContainer, 1002));
			REQUIRE(actionContainer.mayCoalesce == false);
			uh.CompletedUndoStep();
		}
		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 4);
			const Action actionInsert = uh.GetUndoStep();
			REQUIRE(Equal(actionInsert, ActionType::insert, 2, "cd"));
			uh.CompletedUndoStep();
			{
				const Action actionContainer = uh.GetUndoStep();
				REQUIRE(EqualContainerAction(actionContainer, 1001));
				uh.CompletedUndoStep();
			}
			{
				const Action actionContainer = uh.GetUndoStep();
				REQUIRE(EqualContainerAction(actionContainer, 1000));
				uh.CompletedUndoStep();
			}
			{
				const Action actionInsert1 = uh.GetUndoStep();
				REQUIRE(Equal(actionInsert1, ActionType::insert, 0, "ab"));
				uh.CompletedUndoStep();
			}
		}
		// Reached beginning
		REQUIRE(!uh.CanUndo());
	}

	SECTION("Grouping") {

		uh.BeginUndoAction();

		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);
		REQUIRE(!uh.IsSavePoint());
		REQUIRE(uh.AfterSavePoint());
		REQUIRE(uh.CanUndo());
		REQUIRE(!uh.CanRedo());
		val = uh.AppendAction(ActionType::remove, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(!startSequence);
		val = uh.AppendAction(ActionType::insert, 0, "cde", 3, startSequence, true);
		REQUIRE(memcmp(val, "cde", 3) == 0);
		REQUIRE(!startSequence);

		uh.EndUndoAction();

		// Undoing
		{
			const int steps = uh.StartUndo();
			REQUIRE(steps == 3);
			const Action action3 = uh.GetUndoStep();
			REQUIRE(Equal(action3, ActionType::insert, 0, "cde"));
			uh.CompletedUndoStep();
			const Action action2 = uh.GetUndoStep();
			REQUIRE(Equal(action2, ActionType::remove, 0, "ab"));
			uh.CompletedUndoStep();
			const Action action1 = uh.GetUndoStep();
			REQUIRE(Equal(action1, ActionType::insert, 0, "ab"));
			uh.CompletedUndoStep();
		}

		REQUIRE(uh.IsSavePoint());

		// Redoing
		{
			const int steps = uh.StartRedo();
			REQUIRE(steps == 3);
			const Action action1 = uh.GetRedoStep();
			REQUIRE(Equal(action1, ActionType::insert, 0, "ab"));
			uh.CompletedRedoStep();
			const Action action2 = uh.GetRedoStep();
			REQUIRE(Equal(action2, ActionType::remove, 0, "ab"));
			uh.CompletedRedoStep();
			const Action action3 = uh.GetRedoStep();
			REQUIRE(Equal(action3, ActionType::insert, 0, "cde"));
			uh.CompletedRedoStep();
		}

		REQUIRE(!uh.IsSavePoint());

	}

	SECTION("DeepGroup") {

		uh.BeginUndoAction();
		uh.BeginUndoAction();

		bool startSequence = false;
		const char *val = uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(startSequence);
		val = uh.AppendAction(ActionType::container, 1000, nullptr, 0, startSequence, false);
		REQUIRE(!val);
		REQUIRE(!startSequence);
		val = uh.AppendAction(ActionType::remove, 0, "ab", 2, startSequence, true);
		REQUIRE(memcmp(val, "ab", 2) == 0);
		REQUIRE(!startSequence);
		val = uh.AppendAction(ActionType::insert, 0, "cde", 3, startSequence, true);
		REQUIRE(memcmp(val, "cde", 3) == 0);
		REQUIRE(!startSequence);

		uh.EndUndoAction();
		uh.EndUndoAction();

		const int steps = uh.StartUndo();
		REQUIRE(steps == 4);
	}

	SECTION("Tentative") {

		REQUIRE(!uh.TentativeActive());
		REQUIRE(uh.TentativeSteps() == -1);
		uh.TentativeStart();
		REQUIRE(uh.TentativeActive());
		REQUIRE(uh.TentativeSteps() == 0);
		bool startSequence = false;
		uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(uh.TentativeActive());
		REQUIRE(uh.TentativeSteps() == 1);
		REQUIRE(uh.CanUndo());
		uh.TentativeCommit();
		REQUIRE(!uh.TentativeActive());
		REQUIRE(uh.TentativeSteps() == -1);
		REQUIRE(uh.CanUndo());

		// TentativeUndo is the other important operation but it is performed by Document so add a local equivalent
		uh.TentativeStart();
		uh.AppendAction(ActionType::remove, 0, "ab", 2, startSequence, false);
		uh.AppendAction(ActionType::insert, 0, "ab", 2, startSequence, true);
		REQUIRE(uh.TentativeActive());
		// The first TentativeCommit didn't seal off the first action so it is still undoable
		REQUIRE(uh.TentativeSteps() == 2);
		REQUIRE(uh.CanUndo());
		TentativeUndo(uh);
		REQUIRE(!uh.TentativeActive());
		REQUIRE(uh.TentativeSteps() == -1);
		REQUIRE(uh.CanUndo());
	}
}

TEST_CASE("UndoActions") {

	UndoActions ua;

	SECTION("Basics") {
		ua.PushBack();
		REQUIRE(ua.SSize() == 1);
		ua.Create(0, ActionType::insert, 0, 2, false);
		REQUIRE(ua.AtStart(0));
		REQUIRE(ua.LengthTo(0) == 0);
		REQUIRE(ua.AtStart(1));
		REQUIRE(ua.LengthTo(1) == 2);
		ua.PushBack();
		REQUIRE(ua.SSize() == 2);
		ua.Create(0, ActionType::insert, 0, 2, false);
		REQUIRE(ua.SSize() == 2);
		ua.Truncate(1);
		REQUIRE(ua.SSize() == 1);
		ua.Clear();
		REQUIRE(ua.SSize() == 0);
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

		constexpr std::string_view hwair = "\xF0\x90\x8D\x88";
		cb.InsertString(0, hwair.data(), hwair.length(), startSequence);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 3);
		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 2);
	}

	SECTION("Deletion") {
		cb.SetUTF8Substance(true);

		cb.AllocateLineCharacterIndex(LineCharacterIndexType::Utf16 | LineCharacterIndexType::Utf32);

		bool startSequence = false;
		constexpr std::string_view hwair = "a\xF0\x90\x8D\x88z";
		cb.InsertString(0, hwair.data(), hwair.length(), startSequence);

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
		constexpr std::string_view data = "a\n\xF0\x90\x8D\x88\nz";
		cb.InsertString(0, data.data(), data.length(), startSequence);

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
		cb.InsertString(data.length(), "\n", 1, startSequence);

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
		cb.InsertString(data.length(), "\n", 1, startSequence);

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

		constexpr std::string_view euro = "\xE2\x82\xAC";
		cb.InsertString(0, euro.data(), euro.length(), startSequence);

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

		constexpr std::string_view lead = "\xEF";
		cb.InsertString(5, lead.data(), lead.length(), startSequence);

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

		constexpr std::string_view ascii = "!";
		cb.InsertString(1, ascii.data(), ascii.length(), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 6);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 10);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf32) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf32) == 6);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf32) == 9);

		// Insert a NEL after the '!' to trigger the utf8 line end case ->
		// "\xE2!\xC2\x85 \x82\xACa\n \EF\xF0\x90\x8D\x88\n z\n\n" 5 lines

		constexpr std::string_view nel = "\xC2\x85";
		cb.InsertString(2, nel.data(), nel.length(), startSequence);

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
		constexpr std::string_view data = "a\n\xF0\x90\x8D\x88\nz\nc";
		cb.InsertString(0, data.data(), data.length(), startSequence);

		// Delete first 2 new lines -> "az\nc"
		cb.DeleteChars(1, data.length() - 4, startSequence);

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
		constexpr std::string_view data = "a\n\xF0\x90\x8D\x88\nz";
		cb.InsertString(0, data.data(), data.length(), startSequence);

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

		constexpr std::string_view lead4 = "\xF0";
		cb.InsertString(1, lead4.data(), lead4.length(), startSequence);

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
		constexpr std::string_view data = "a\r\nb";
		cb.InsertString(0, data.data(), data.length(), startSequence);

		// 3 lines of text containing 5 bytes ->
		// "a\r!\nb"
		constexpr std::string_view ascii = "!";
		cb.InsertString(2, ascii.data(), ascii.length(), startSequence);

		REQUIRE(cb.IndexLineStart(0, LineCharacterIndexType::Utf16) == 0);
		REQUIRE(cb.IndexLineStart(1, LineCharacterIndexType::Utf16) == 2);
		REQUIRE(cb.IndexLineStart(2, LineCharacterIndexType::Utf16) == 4);
		REQUIRE(cb.IndexLineStart(3, LineCharacterIndexType::Utf16) == 5);
	}
}

TEST_CASE("ChangeHistory") {

	ChangeHistory il;
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
		const EditionSet at1 = { {2, 1} };
		REQUIRE(il.DeletionsAt(1) == at1);
		il.DeleteRangeSavingHistory(1, 1, false, false);
		REQUIRE(il.DeletionCount(0,1) == 2);
		const EditionSet at2 = { {2, 1}, {3, 1} };
		REQUIRE(il.DeletionsAt(1) == at2);
		il.DeleteRangeSavingHistory(0, 1, false, false);
		const EditionSet at3 = { {2, 1}, {3, 2} };
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

	SECTION("Delete Contiguous Backward") {
		// Deletes that touch
		constexpr Sci::Position length = 20;
		constexpr Sci::Position rounds = 8;
		il.Insert(0, length, false, true);
		REQUIRE(il.Length() == length);
		il.SetSavePoint();
		for (Sci::Position i = 0; i < rounds; i++) {
			il.DeleteRangeSavingHistory(9-i, 1, false, false);
		}

		constexpr Sci::Position lengthAfterDeletions = length - rounds;
		REQUIRE(il.Length() == lengthAfterDeletions);
		REQUIRE(il.DeletionCount(0, lengthAfterDeletions) == rounds);

		for (Sci::Position j = 0; j < rounds; j++) {
			il.UndoDeleteStep(2+j, 1, false);
		}

		// Restored to original
		REQUIRE(il.DeletionCount(0, length) == 0);
		REQUIRE(il.Length() == length);
	}

	SECTION("Delete Contiguous Forward") {
		// Deletes that touch
		constexpr size_t length = 20;
		constexpr size_t rounds = 8;
		il.Insert(0, length, false, true);
		REQUIRE(il.Length() == length);
		il.SetSavePoint();
		for (size_t i = 0; i < rounds; i++) {
			il.DeleteRangeSavingHistory(2,1, false, false);
		}

		constexpr size_t lengthAfterDeletions = length - rounds;
		REQUIRE(il.Length() == lengthAfterDeletions);
		REQUIRE(il.DeletionCount(0, lengthAfterDeletions) == rounds);

		for (size_t j = 0; j < rounds; j++) {
			il.UndoDeleteStep(2, 1, false);
		}

		// Restored to original
		REQUIRE(il.Length() == length);
		REQUIRE(il.DeletionCount(0, length) == 0);
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
		constexpr std::string_view sInsert = "abcdefghijklmnopqrstuvwxyz";
		bool startSequence = false;
		cb.InsertString(0, sInsert.data(), sInsert.length(), startSequence);
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
		constexpr std::string_view sInsert = "abcdefghijklmnopqrstuvwxyz";
		bool startSequence = false;
		cb.InsertString(0, sInsert.data(), sInsert.length(), startSequence);
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

void PushUndoAction(CellBuffer &cb, int type, Sci::Position pos, std::string_view sv) {
	cb.PushUndoActionType(type, pos);
	cb.ChangeLastUndoActionText(sv.length(), sv.data());
}

}

TEST_CASE("CellBufferLoadUndoHistory") {

	CellBuffer cb(false, false);
	constexpr int remove = 1;
	constexpr int insert = 0;

	SECTION("Basics") {
		cb.SetUndoCollection(false);
		constexpr std::string_view sInsert = "abcdef";
		bool startSequence = false;
		cb.InsertString(0, sInsert.data(), sInsert.length(), startSequence);
		cb.SetUndoCollection(true);
		cb.ChangeHistorySet(true);

		// Create an undo history that matches the contents at current point 2
		// So, 2 actions; current point; 2 actions
		// a_cdef
		PushUndoAction(cb, remove, 1, "_");
		// acdef
		PushUndoAction(cb, insert, 1, "b");
		// abcdef -> current
		PushUndoAction(cb, remove, 3, "d");
		// abcef -> save
		PushUndoAction(cb, insert, 3, "*");
		// abc*ef
		cb.SetUndoSavePoint(3);
		cb.SetUndoDetach(-1);
		cb.SetUndoTentative(-1);
		cb.SetUndoCurrent(2);

		// 2nd insertion is removed from change history as it isn't visible and isn't saved
		// 2nd deletion is visible (as insertion) as it was saved but then reverted to original
		// 1st insertion and 1st deletion are both visible as saved
		const History hist{ {{1, 1, changeSaved}, {3, 1, changeRevertedOriginal}}, {{2, changeSaved}} };
		REQUIRE(HistoryOf(cb) == hist);
	}

	SECTION("Detached") {
		cb.SetUndoCollection(false);
		constexpr std::string_view sInsert = "a-b=cdef";
		bool startSequence = false;
		cb.InsertString(0, sInsert.data(), sInsert.length(), startSequence);
		cb.SetUndoCollection(true);
		cb.ChangeHistorySet(true);

		// Create an undo history that matches the contents at current point 2 which detached at 1
		// So, insert saved; insert detached; current point
		// abcdef
		PushUndoAction(cb, insert, 1, "-");
		// a-bcdef
		PushUndoAction(cb, insert, 3, "=");
		// a-b=cdef
		cb.SetUndoSavePoint(-1);
		cb.SetUndoDetach(1);
		cb.SetUndoTentative(-1);
		cb.SetUndoCurrent(2);

		// This doesn't show elements due to undo.
		// There was also a modified delete (reverting the insert) at 3 in the original but that is missing.
		const History hist{ {{1, 1, changeSaved}, {3, 1, changeModified}}, {} };
		REQUIRE(HistoryOf(cb) == hist);
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
		for (size_t i = 0; i < 20000; i++) {
			const int r = rseq.Next() % 10;
			if (r <= 2) {			// 30%
				// Insert text
				const Sci::Position pos = rseq.Next() % (cb.Length() + 1);
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
