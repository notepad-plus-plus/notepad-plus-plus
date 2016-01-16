// Unit Tests for Scintilla internal data structures

#include <string.h>

#include <stdexcept>
#include <algorithm>

#include "Platform.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "Decoration.h"

#include "catch.hpp"

const int indicator=4;

// Test Decoration.

TEST_CASE("Decoration") {

	Decoration deco(indicator);

	SECTION("HasCorrectIndicator") {
		REQUIRE(indicator == deco.indicator);
	}

	SECTION("IsEmptyInitially") {
		REQUIRE(0 == deco.rs.Length());
		REQUIRE(1 == deco.rs.Runs());
		REQUIRE(deco.Empty());
	}

	SECTION("SimpleSpace") {
		deco.rs.InsertSpace(0, 1);
		REQUIRE(deco.Empty());
	}

	SECTION("SimpleRun") {
		deco.rs.InsertSpace(0, 1);
		deco.rs.SetValueAt(0, 2);
		REQUIRE(!deco.Empty());
	}
}

// Test DecorationList.

TEST_CASE("DecorationList") {

	DecorationList decol;

	SECTION("HasCorrectIndicator") {
		decol.SetCurrentIndicator(indicator);
		REQUIRE(indicator == decol.GetCurrentIndicator());
	}

	SECTION("HasCorrectCurrentValue") {
		const int value = 55;
		decol.SetCurrentValue(value);
		REQUIRE(value == decol.GetCurrentValue());
	}

	SECTION("ExpandSetValues") {
		decol.SetCurrentIndicator(indicator);
		decol.InsertSpace(0, 9);
		const int value = 59;
		int position = 4;
		int fillLength = 3;
		bool changed = decol.FillRange(position, value, fillLength);
		REQUIRE(changed);
		REQUIRE(position == 4);
		REQUIRE(fillLength == 3);
		REQUIRE(fillLength == 3);
		REQUIRE(decol.ValueAt(indicator, 5) == value);
		REQUIRE(decol.AllOnFor(5) == (1 << indicator));
		REQUIRE(decol.Start(indicator, 5) == 4);
		REQUIRE(decol.End(indicator, 5) == 7);
		const int indicatorB=6;
		decol.SetCurrentIndicator(indicatorB);
		changed = decol.FillRange(position, value, fillLength);
		REQUIRE(changed);
		REQUIRE(decol.AllOnFor(5) == ((1 << indicator) | (1 << indicatorB)));
		decol.DeleteRange(5, 1);
		REQUIRE(decol.Start(indicatorB, 5) == 4);
		REQUIRE(decol.End(indicatorB, 5) == 6);
	}

}
