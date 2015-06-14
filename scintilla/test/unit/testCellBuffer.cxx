// Unit Tests for Scintilla internal data structures

#include <cstring>
#include <stdexcept>
#include <algorithm>

#include "Platform.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"

#include "catch.hpp"

// Test CellBuffer.

TEST_CASE("CellBuffer") {

	const char sText[] = "Scintilla";
	const size_t sLength = strlen(sText);

	CellBuffer cb;
	
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
		REQUIRE(0 == cb.LineFromPosition(sLength));
		REQUIRE(cb.CanUndo());
		REQUIRE(!cb.CanRedo());
	}

	SECTION("InsertTwoLines") {
		const char sText2[] = "Two\nLines";
		const size_t sLength2 = strlen(sText2);
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
		REQUIRE(cb.GetLineEndTypes() == 0);
		cb.SetLineEndTypes(1);
		REQUIRE(cb.GetLineEndTypes() == 1);
		cb.SetLineEndTypes(0);
		REQUIRE(cb.GetLineEndTypes() == 0);
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
