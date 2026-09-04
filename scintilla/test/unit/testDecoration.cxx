/** @file testDecoration.cxx
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

#include "Debugging.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "Decoration.h"

#include "catch.hpp"

constexpr int indicator=4;

using namespace Scintilla::Internal;

// Test Decoration.

TEST_CASE("Decoration") {

	std::unique_ptr<IDecoration> deco = DecorationCreate(false, indicator);

	SECTION("HasCorrectIndicator") {
		REQUIRE(indicator == deco->Indicator());
	}

	SECTION("IsEmptyInitially") {
		REQUIRE(0 == deco->Length());
		REQUIRE(1 == deco->Runs());
		REQUIRE(deco->Empty());
	}

	SECTION("SimpleSpace") {
		deco->InsertSpace(0, 1);
		REQUIRE(deco->Empty());
	}

	SECTION("SimpleRun") {
		deco->InsertSpace(0, 1);
		deco->SetValueAt(0, 2);
		REQUIRE(!deco->Empty());
	}
}

// Test DecorationList.

TEST_CASE("DecorationList") {

	std::unique_ptr<IDecorationList> decol = DecorationListCreate(false);

	SECTION("HasCorrectIndicator") {
		decol->SetCurrentIndicator(indicator);
		REQUIRE(indicator == decol->GetCurrentIndicator());
	}

	SECTION("HasCorrectCurrentValue") {
		constexpr int value = 55;
		decol->SetCurrentValue(value);
		REQUIRE(value == decol->GetCurrentValue());
	}

	SECTION("ExpandSetValues") {
		decol->SetCurrentIndicator(indicator);
		decol->InsertSpace(0, 9);
		constexpr int value = 59;
		constexpr Sci::Position position = 4;
		constexpr Sci::Position fillLength = 3;
		auto fr = decol->FillRange(position, value, fillLength);
		REQUIRE(fr.changed);
		REQUIRE(fr.position == 4);
		REQUIRE(fr.fillLength == 3);
		REQUIRE(decol->ValueAt(indicator, 5) == value);
		REQUIRE(decol->AllOnFor(5) == (1 << indicator));
		REQUIRE(decol->Start(indicator, 5) == 4);
		REQUIRE(decol->End(indicator, 5) == 7);
		constexpr int indicatorB=6;
		decol->SetCurrentIndicator(indicatorB);
		fr = decol->FillRange(position, value, fillLength);
		REQUIRE(fr.changed);
		REQUIRE(decol->AllOnFor(5) == ((1 << indicator) | (1 << indicatorB)));
		decol->DeleteRange(5, 1);
		REQUIRE(decol->Start(indicatorB, 5) == 4);
		REQUIRE(decol->End(indicatorB, 5) == 6);
	}

}
