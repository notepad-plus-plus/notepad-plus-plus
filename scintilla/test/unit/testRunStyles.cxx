// Unit Tests for Scintilla internal data structures

#include <cstddef>
#include <cstring>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"

#include "catch.hpp"

using namespace Scintilla;

// Test RunStyles.

namespace Scintilla {	// Xcode clang 9.0 doesn't like this when in the unnamed namespace
	bool operator==(const FillResult<int> &fra, const FillResult<int> &frb) {
		return fra.changed == frb.changed &&
			fra.position == frb.position &&
			fra.fillLength == frb.fillLength;
	}
}

TEST_CASE("RunStyles") {

	RunStyles<int, int> rs;

	SECTION("IsEmptyInitially") {
		REQUIRE(0 == rs.Length());
		REQUIRE(1 == rs.Runs());
	}

	SECTION("SimpleInsert") {
		rs.InsertSpace(0, 1);
		REQUIRE(1 == rs.Length());
		REQUIRE(1 == rs.Runs());
		REQUIRE(0 == rs.ValueAt(0));
		REQUIRE(1 == rs.FindNextChange(0, rs.Length()));
		REQUIRE(2 == rs.FindNextChange(1, rs.Length()));
	}

	SECTION("TwoRuns") {
		rs.InsertSpace(0, 2);
		REQUIRE(2 == rs.Length());
		REQUIRE(1 == rs.Runs());
		rs.SetValueAt(0, 2);
		REQUIRE(2 == rs.Runs());
		REQUIRE(2 == rs.ValueAt(0));
		REQUIRE(0 == rs.ValueAt(1));
		REQUIRE(1 == rs.FindNextChange(0, rs.Length()));
		REQUIRE(2 == rs.FindNextChange(1, rs.Length()));
		REQUIRE(3 == rs.FindNextChange(2, rs.Length()));
	}

	SECTION("LongerRuns") {
		rs.InsertSpace(0, 5);
		rs.SetValueAt(0, 3);
		rs.SetValueAt(1, 3);
		REQUIRE(3 == rs.ValueAt(0));
		REQUIRE(3 == rs.ValueAt(1));
		REQUIRE(0 == rs.ValueAt(2));
		REQUIRE(2 == rs.Runs());

		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(2 == rs.EndRun(0));

		REQUIRE(0 == rs.StartRun(1));
		REQUIRE(2 == rs.EndRun(1));

		REQUIRE(2 == rs.StartRun(2));
		REQUIRE(5 == rs.EndRun(2));

		REQUIRE(2 == rs.StartRun(3));
		REQUIRE(5 == rs.EndRun(3));

		REQUIRE(2 == rs.StartRun(4));
		REQUIRE(5 == rs.EndRun(4));

		// At end
		REQUIRE(2 == rs.StartRun(5));
		REQUIRE(5 == rs.EndRun(5));

		// After end is same as end
		REQUIRE(2 == rs.StartRun(6));
		REQUIRE(5 == rs.EndRun(6));

		REQUIRE(2 == rs.FindNextChange(0, rs.Length()));
		REQUIRE(5 == rs.FindNextChange(2, rs.Length()));
		REQUIRE(6 == rs.FindNextChange(5, rs.Length()));
	}

	SECTION("FillRange") {
		rs.InsertSpace(0, 5);
		int startFill = 1;
		int lengthFill = 3;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 3} == fr);

		REQUIRE(0 == rs.ValueAt(0));
		REQUIRE(99 == rs.ValueAt(1));
		REQUIRE(99 == rs.ValueAt(2));
		REQUIRE(99 == rs.ValueAt(3));
		REQUIRE(0 == rs.ValueAt(4));

		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(1 == rs.EndRun(0));

		REQUIRE(1 == rs.StartRun(1));
		REQUIRE(4 == rs.EndRun(1));
	}

	SECTION("FillRangeAlreadyFilled") {
		rs.InsertSpace(0, 5);
		int startFill = 1;
		int lengthFill = 3;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 3} == fr);

		int startFill2 = 2;
		int lengthFill2 = 1;
		// Compiler warnings if 'false' used instead of '0' as expected value:
		const auto fr2 = rs.FillRange(startFill2, 99, lengthFill2);
		REQUIRE(FillResult<int>{false, 2, 1} == fr2);
		REQUIRE(0 == rs.ValueAt(0));
		REQUIRE(99 == rs.ValueAt(1));
		REQUIRE(99 == rs.ValueAt(2));
		REQUIRE(99 == rs.ValueAt(3));
		REQUIRE(0 == rs.ValueAt(4));
		REQUIRE(3 == rs.Runs());
	}

	SECTION("FillRangeAlreadyPartFilled") {
		rs.InsertSpace(0, 5);
		int startFill = 1;
		int lengthFill = 2;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 2} == fr);

		int startFill2 = 2;
		int lengthFill2 = 2;
		const auto fr2 = rs.FillRange(startFill2, 99, lengthFill2);
		REQUIRE(FillResult<int>{true, 3, 1} == fr2);
		REQUIRE(3 == rs.Runs());
	}

	SECTION("DeleteRange") {
		rs.InsertSpace(0, 5);
		rs.SetValueAt(0, 3);
		REQUIRE(2 == rs.Runs());
		rs.SetValueAt(1, 3);
		REQUIRE(2 == rs.Runs());
		rs.DeleteRange(1, 1);
		REQUIRE(4 == rs.Length());
		REQUIRE(2 == rs.Runs());
		REQUIRE(3 == rs.ValueAt(0));
		REQUIRE(0 == rs.ValueAt(1));

		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(1 == rs.EndRun(0));

		REQUIRE(1 == rs.StartRun(1));
		REQUIRE(4 == rs.EndRun(1));

		REQUIRE(1 == rs.StartRun(2));
		REQUIRE(4 == rs.EndRun(2));
	}

	SECTION("Find") {
		rs.InsertSpace(0, 5);
		int startFill = 1;
		int lengthFill = 3;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 3} == fr);

		REQUIRE(0 == rs.Find(0,0));
		REQUIRE(1 == rs.Find(99,0));
		REQUIRE(-1 == rs.Find(3,0));

		REQUIRE(4 == rs.Find(0,1));
		REQUIRE(1 == rs.Find(99,1));
		REQUIRE(-1 == rs.Find(3,1));

		REQUIRE(4 == rs.Find(0,2));
		REQUIRE(2 == rs.Find(99,2));
		REQUIRE(-1 == rs.Find(3, 2));

		REQUIRE(4 == rs.Find(0,4));
		REQUIRE(-1 == rs.Find(99,4));
		REQUIRE(-1 == rs.Find(3,4));

		REQUIRE(-1 == rs.Find(0,5));
		REQUIRE(-1 == rs.Find(99,5));
		REQUIRE(-1 == rs.Find(3,5));

		REQUIRE(-1 == rs.Find(0,6));
		REQUIRE(-1 == rs.Find(99,6));
		REQUIRE(-1 == rs.Find(3,6));
	}

	SECTION("AllSame") {
		REQUIRE(true == rs.AllSame());
		rs.InsertSpace(0, 5);
		REQUIRE(true == rs.AllSame());
		REQUIRE(false == rs.AllSameAs(88));
		REQUIRE(true == rs.AllSameAs(0));
		int startFill = 1;
		int lengthFill = 3;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(true == fr.changed);
		REQUIRE(false == rs.AllSame());
		REQUIRE(false == rs.AllSameAs(88));
		REQUIRE(false == rs.AllSameAs(0));
		const auto fr2 = rs.FillRange(startFill, 0, lengthFill);
		REQUIRE(true == fr2.changed);
		REQUIRE(true == rs.AllSame());
		REQUIRE(false == rs.AllSameAs(88));
		REQUIRE(true == rs.AllSameAs(0));
	}

	SECTION("FindWithReversion") {
		rs.InsertSpace(0, 5);
		REQUIRE(1 == rs.Runs());

		int startFill = 1;
		int lengthFill = 1;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 1} == fr);
		REQUIRE(3 == rs.Runs());

		startFill = 2;
		lengthFill = 1;
		const auto fr2 = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(FillResult<int>{true, 2, 1} == fr2);
		REQUIRE(3 == rs.Runs());

		startFill = 1;
		lengthFill = 1;
		const auto fr3 = rs.FillRange(startFill, 0, lengthFill);
		REQUIRE(FillResult<int>{true, 1, 1} == fr3);
		REQUIRE(3 == rs.Runs());

		startFill = 2;
		lengthFill = 1;
		const auto fr4 = rs.FillRange(startFill, 0, lengthFill);
		REQUIRE(FillResult<int>{true, 2, 1} == fr4);
		REQUIRE(1 == rs.Runs());

		REQUIRE(-1 == rs.Find(0,6));
	}

	SECTION("FinalRunInversion") {
		REQUIRE(1 == rs.Runs());
		rs.InsertSpace(0, 1);
		REQUIRE(1 == rs.Runs());
		rs.SetValueAt(0, 1);
		REQUIRE(1 == rs.Runs());
		rs.InsertSpace(1, 1);
		REQUIRE(1 == rs.Runs());
		rs.SetValueAt(1, 1);
		REQUIRE(1 == rs.Runs());
		rs.SetValueAt(1, 0);
		REQUIRE(2 == rs.Runs());
		rs.SetValueAt(1, 1);
		REQUIRE(1 == rs.Runs());
	}

	SECTION("DeleteAll") {
		rs.InsertSpace(0, 5);
		rs.SetValueAt(0, 3);
		rs.SetValueAt(1, 3);
		rs.DeleteAll();
		REQUIRE(0 == rs.Length());
		REQUIRE(0 == rs.ValueAt(0));
		REQUIRE(1 == rs.Runs());
	}

	SECTION("DeleteSecond") {
		rs.InsertSpace(0, 3);
		int startFill = 1;
		int lengthFill = 1;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(true == fr.changed);
		REQUIRE(3 == rs.Length());
		REQUIRE(3 == rs.Runs());
		rs.DeleteRange(1, 1);
		REQUIRE(2 == rs.Length());
		REQUIRE(1 == rs.Runs());
	}

	SECTION("DeleteEndRun") {
		rs.InsertSpace(0, 2);
		int startFill = 1;
		int lengthFill = 1;
		const auto fr = rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(true == fr.changed);
		REQUIRE(2 == rs.Length());
		REQUIRE(2 == rs.Runs());
		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(1 == rs.EndRun(0));
		REQUIRE(1 == rs.StartRun(1));
		REQUIRE(2 == rs.EndRun(1));
		rs.DeleteRange(1, 1);
		REQUIRE(1 == rs.Length());
		REQUIRE(1 == rs.Runs());
		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(1 == rs.EndRun(0));
		REQUIRE(0 == rs.StartRun(1));
		REQUIRE(1 == rs.EndRun(1));
		rs.Check();
	}

	SECTION("OutsideBounds") {
		rs.InsertSpace(0, 1);
		int startFill = 1;
		int lengthFill = 1;
		rs.FillRange(startFill, 99, lengthFill);
		REQUIRE(1 == rs.Length());
		REQUIRE(1 == rs.Runs());
		REQUIRE(0 == rs.StartRun(0));
		REQUIRE(1 == rs.EndRun(0));
	}

}
