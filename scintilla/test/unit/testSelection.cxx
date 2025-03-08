/** @file testSelection.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstdint>

#include <stdexcept>
#include <string_view>
#include <vector>

#include "Debugging.h"

#include "Position.h"
#include "Selection.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test Selection.

namespace {

constexpr SelectionPosition invalid;
constexpr SelectionPosition zero(0);
constexpr SelectionRange rangeInvalid;
constexpr SelectionRange rangeZero(0);

}

TEST_CASE("SelectionPosition") {

	SECTION("SelectionPosition") {
		SelectionPosition sel;
		REQUIRE(sel.Position() == Sci::invalidPosition);
		REQUIRE(sel.VirtualSpace() == 0);
		REQUIRE(!sel.IsValid());
		REQUIRE(sel.VirtualSpace() == 0);

		REQUIRE(sel == invalid);
		REQUIRE(sel != zero);
		sel.Reset();
		REQUIRE(sel != invalid);
		REQUIRE(sel == zero);
	}

	SECTION("Comparison") {
		constexpr SelectionPosition sel(2,3);
		REQUIRE(sel > invalid);
		REQUIRE(sel > zero);
		REQUIRE(sel >= zero);
		REQUIRE(zero < sel);
		REQUIRE(zero <= sel);

		SelectionPosition virtuous(0, 4);
		REQUIRE(virtuous > zero);
		REQUIRE(virtuous >= zero);
		REQUIRE(zero < virtuous);
		REQUIRE(zero <= virtuous);

		REQUIRE(virtuous.Position() == 0);
		REQUIRE(virtuous.VirtualSpace() == 4);

		virtuous.SetPosition(1);	// Also resets virtualSpace
		REQUIRE(virtuous.Position() == 1);
		REQUIRE(virtuous.VirtualSpace() == 0);
		virtuous.SetVirtualSpace(3);	// Does not reset position
		REQUIRE(virtuous.Position() == 1);
		REQUIRE(virtuous.VirtualSpace() == 3);
	}

	SECTION("Add") {
		SelectionPosition sel(2,3);
		sel.Add(1);
		REQUIRE(sel.Position() == 3);
		REQUIRE(sel.VirtualSpace() == 3);
		sel.AddVirtualSpace(2);
		REQUIRE(sel.Position() == 3);
		REQUIRE(sel.VirtualSpace() == 5);
	}

	SECTION("MoveForInsertDelete") {
		// There are multiple details implemented in MoveForInsertDelete that are supposed to
		// move selections in a way that appears to be natural to a user.

		SelectionPosition sel(2,3);
		sel.MoveForInsertDelete(true, 0,1, false);
		REQUIRE(sel == SelectionPosition(3,3));

		// Converts a virtual space to real space
		sel.MoveForInsertDelete(true, 3,1, false);
		REQUIRE(sel == SelectionPosition(4,2));

		// Deletion at position clears virtual space
		sel.MoveForInsertDelete(false, 4,1, false);
		REQUIRE(sel == SelectionPosition(4,0));

		sel.MoveForInsertDelete(false, 3,1, false);
		REQUIRE(sel == SelectionPosition(3,0));

		// Insert at position with and without move for equal
		sel.MoveForInsertDelete(true, 3, 1, false);
		REQUIRE(sel == SelectionPosition(3, 0));
		sel.MoveForInsertDelete(true, 3, 1, true);
		REQUIRE(sel == SelectionPosition(4, 0));

		// Deletion over the position moves to start of deletion
		sel.MoveForInsertDelete(false, 2, 5, false);
		REQUIRE(sel == SelectionPosition(2, 0));
	}

	SECTION("Serialization") {
		// Conversion to/from string form

		const std::string invalidString(invalid.ToString());
		REQUIRE(invalidString == "-1");
		const SelectionPosition invalidReturned(invalidString);
		REQUIRE(invalidReturned == invalid);

		const std::string zeroString(zero.ToString());
		REQUIRE(zeroString == "0");
		const SelectionPosition zeroReturned(zeroString);
		REQUIRE(zeroReturned == zero);

		const SelectionPosition virtue(2, 3);
		const std::string virtueString(virtue.ToString());
		REQUIRE(virtueString == "2v3");
		const SelectionPosition virtueReturned(virtueString);
		REQUIRE(virtueReturned == virtue);
	}

}

TEST_CASE("SelectionSegment") {

	SECTION("SelectionSegment") {
		const SelectionSegment ss;
		REQUIRE(ss.start == invalid);
		REQUIRE(ss.end == invalid);
	}

}

TEST_CASE("SelectionRange") {

	SECTION("SelectionRange") {
		const SelectionRange sr;
		REQUIRE(sr.anchor == invalid);
		REQUIRE(sr.caret == invalid);
	}

	SECTION("Serialization") {
		// Conversion to/from string form

		// Range from 1 to 2 with 3 virtual spaces
		const SelectionRange range123(SelectionPosition(2, 3), SelectionPosition(1));
		const std::string range123String(range123.ToString());
		// Opposite order to constructor: from anchor to caret 
		REQUIRE(range123String == "1-2v3");
		const SelectionRange range123Returned(range123String);
		REQUIRE(range123Returned == range123);
	}

	SECTION("Intersect") {
		constexpr SelectionSegment segmentEmpty;

		// Range from 1 to 2 with 3 virtual spaces
		const SelectionRange range123(SelectionPosition(2, 3), SelectionPosition(1));
		const SelectionSegment segment12(1, 2);
		const SelectionSegment inside = range123.Intersect(segment12);
		REQUIRE(inside == segment12);

		const SelectionSegment segment121(SelectionPosition(1), SelectionPosition(2, 1));
		const SelectionSegment withVirtual = range123.Intersect(segment121);
		REQUIRE(withVirtual == segment121);

		const SelectionSegment segment052(SelectionPosition(0), SelectionPosition(5, 2));
		const SelectionSegment internal = range123.Intersect(segment052);	// All inside
		REQUIRE(internal == range123.AsSegment());

		const SelectionSegment wayOut(SelectionPosition(100), SelectionPosition(105, 2));
		const SelectionSegment nothing = range123.Intersect(wayOut);
		REQUIRE(nothing == segmentEmpty);

		const SelectionSegment edge(1, 1);
		const SelectionSegment nowt = range123.Intersect(edge);
		REQUIRE(nowt == edge);

		// (0, 1) and (1, 2v3) touch so intersection is a single position.
		const SelectionSegment front(0, 1);
		const SelectionSegment single(1, 1);
		const SelectionSegment thin = range123.Intersect(front);
		REQUIRE(thin == single);
	}

}

TEST_CASE("Selection") {

	SECTION("Selection") {
		Selection sel;
		
		REQUIRE(sel.selType == Selection::SelTypes::stream);
		REQUIRE(!sel.IsRectangular());
		REQUIRE(sel.Count() == 1);
		REQUIRE(sel.Main() == 0);

		REQUIRE(sel.Range(0) == rangeZero);
		REQUIRE(sel.RangeMain() == rangeZero);
		REQUIRE(sel.Rectangular() == rangeInvalid);
		REQUIRE(sel.Empty());
	}

	SECTION("Serialization") {
		// Conversion to/from string form

		// Range from 5 with 3 virtual spaces to 2
		const SelectionRange range532(SelectionPosition(2), SelectionPosition(5, 3));
		Selection selection;
		selection.SetSelection(range532);
		const std::string selectionString(selection.ToString());
		// Opposite order to constructor: from anchor to caret 
		REQUIRE(selectionString == "5v3-2");
		const SelectionRange selectionReturned(selectionString);

		REQUIRE(selection.selType == Selection::SelTypes::stream);
		REQUIRE(!selection.IsRectangular());
		REQUIRE(selection.Count() == 1);
		REQUIRE(selection.Main() == 0);

		REQUIRE(selection.Range(0) == range532);
		REQUIRE(selection.RangeMain() == range532);
		REQUIRE(selection.Rectangular() == rangeInvalid);
		REQUIRE(!selection.Empty());
	}

	SECTION("SerializationMultiple") {
		// Conversion to/from string form

		// Range from 5 with 3 virtual spaces to 2
		const SelectionRange range532(SelectionPosition(2), SelectionPosition(5, 3));
		const SelectionRange range1(SelectionPosition(1));
		Selection selection;
		selection.SetSelection(range532);
		selection.AddSelection(range1);
		selection.SetMain(1);
		const std::string selectionString(selection.ToString());
		REQUIRE(selectionString == "5v3-2,1#1");
		const SelectionRange selectionReturned(selectionString);

		REQUIRE(selection.selType == Selection::SelTypes::stream);
		REQUIRE(!selection.IsRectangular());
		REQUIRE(selection.Count() == 2);
		REQUIRE(selection.Main() == 1);

		REQUIRE(selection.Range(0) == range532);
		REQUIRE(selection.Range(1) == range1);
		REQUIRE(selection.RangeMain() == range1);
		REQUIRE(selection.Rectangular() == rangeInvalid);
		REQUIRE(!selection.Empty());
	}

	SECTION("SerializationRectangular") {
		// Conversion to/from string form

		// Range from 5 with 3 virtual spaces to 2
		const SelectionRange range532(SelectionPosition(2), SelectionPosition(5, 3));
		
		// Create a single-line rectangular selection
		Selection selection;
		selection.selType = Selection::SelTypes::rectangle;
		selection.Rectangular() = range532;
		// Set arbitrary realized range - inside editor ranges would be calculated from line layout
		selection.SetSelection(rangeZero);

		const std::string selectionString(selection.ToString());
		REQUIRE(selectionString == "R5v3-2");
		const Selection selectionReturned(selectionString);

		REQUIRE(selection.selType == Selection::SelTypes::rectangle);
		REQUIRE(selection.IsRectangular());
		REQUIRE(selection.Count() == 1);
		REQUIRE(selection.Main() == 0);

		REQUIRE(selection.Range(0) == rangeZero);
		REQUIRE(selection.RangeMain() == rangeZero);
		REQUIRE(selection.Rectangular() == range532);

		selection.selType = Selection::SelTypes::thin;
		const std::string thinString(selection.ToString());
		REQUIRE(thinString == "T5v3-2");
	}

}
