/** @file testPerLine.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <forward_list>
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
#include "PerLine.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

constexpr int FoldBase = static_cast<int>(Scintilla::FoldLevel::Base);

// Test MarkerHandleSet.

TEST_CASE("MarkerHandleSet") {

	MarkerHandleSet mhs;

	SECTION("Initial") {
		// Initial State
		REQUIRE(mhs.Empty());
		REQUIRE(0 == mhs.MarkValue());
		REQUIRE(!mhs.Contains(1));
	}

	SECTION("InsertDelete") {
		// Test knows that InsertHandle inserts at front (0)
		// Insert 1 with handle 100
		REQUIRE(mhs.InsertHandle(100,1));
		REQUIRE(!mhs.Empty());
		REQUIRE(2 == mhs.MarkValue());
		REQUIRE(mhs.Contains(100));

		// Insert 2 with handle 200
		REQUIRE(mhs.InsertHandle(200,2));
		REQUIRE(!mhs.Empty());
		REQUIRE(mhs.Contains(100));
		REQUIRE(mhs.Contains(200));
		REQUIRE(6 == mhs.MarkValue());

		const MarkerHandleNumber *mhn0 = mhs.GetMarkerHandleNumber(0);
		REQUIRE(200 == mhn0->handle);
		REQUIRE(2 == mhn0->number);
		const MarkerHandleNumber *mhn1 = mhs.GetMarkerHandleNumber(1);
		REQUIRE(100 == mhn1->handle);
		REQUIRE(1 == mhn1->number);
		const MarkerHandleNumber *mhn2 = mhs.GetMarkerHandleNumber(2);
		REQUIRE(nullptr == mhn2);

		// Remove first insertion
		mhs.RemoveHandle(100);
		REQUIRE(!mhs.Empty());
		REQUIRE(mhs.Contains(200));
		REQUIRE(4 == mhs.MarkValue());

		// Remove remaining element
		REQUIRE(mhs.RemoveNumber(2, true));
		REQUIRE(mhs.Empty());
		REQUIRE(!mhs.Contains(200));
		REQUIRE(0 == mhs.MarkValue());
	}

	SECTION("Combine") {
		mhs.InsertHandle(100, 1);
		MarkerHandleSet mhsOther;
		mhsOther.InsertHandle(200, 2);
		mhs.CombineWith(&mhsOther);
		REQUIRE(mhsOther.Empty());
		mhs.RemoveHandle(100);
		mhs.RemoveHandle(200);
		REQUIRE(mhs.Empty());
	}
}

TEST_CASE("LineMarkers") {

	LineMarkers lm;

	SECTION("Initial") {
		// Initial State
		REQUIRE(0 == lm.MarkValue(0));
	}

	SECTION("AddDelete") {
		// Test knows the way handles are allocated, starting from 1
		lm.InsertLines(0, 5);
		const int handle1 = lm.AddMark(0, 1, 5);
		REQUIRE(1 == handle1);
		REQUIRE(2 == lm.MarkValue(0));
		REQUIRE(1 == lm.HandleFromLine(0, 0));
		REQUIRE(1 == lm.NumberFromLine(0, 0));
		REQUIRE(-1 == lm.HandleFromLine(0, 1));
		REQUIRE(-1 == lm.NumberFromLine(0, 1));
		REQUIRE(0 == lm.LineFromHandle(handle1));

		REQUIRE(lm.DeleteMark(0, 1, true));
		REQUIRE(0 == lm.MarkValue(0));

		const int handle2 = lm.AddMark(0, 2, 5);
		REQUIRE(2 == handle2);
		REQUIRE(4 == lm.MarkValue(0));
		lm.DeleteMarkFromHandle(handle2);
		REQUIRE(0 == lm.MarkValue(0));
	}

	SECTION("MarkerNext") {
		lm.AddMark(1, 1, 5);
		lm.AddMark(2, 2, 5);
		const Sci::Line line1 = lm.MarkerNext(0, 6);
		REQUIRE(1 == line1);
		const Sci::Line line2 = lm.MarkerNext(line1+1, 6);
		REQUIRE(2 == line2);
		const Sci::Line line3 = lm.MarkerNext(line2+1, 6);
		REQUIRE(-1 == line3);
	}

	SECTION("MergeMarkers") {
		lm.AddMark(1, 1, 5);
		lm.AddMark(2, 2, 5);
		lm.MergeMarkers(1);
		REQUIRE(6 == lm.MarkValue(1));
		REQUIRE(0 == lm.MarkValue(2));
	}

	SECTION("InsertRemoveLine") {
		const int handle1 = lm.AddMark(1, 1, 5);
		const int handle2 = lm.AddMark(2, 2, 5);
		lm.InsertLine(2);
		REQUIRE(0 == lm.MarkValue(0));
		REQUIRE(2 == lm.MarkValue(1));
		REQUIRE(0 == lm.MarkValue(2));
		REQUIRE(4 == lm.MarkValue(3));
		REQUIRE(0 == lm.MarkValue(4));
		lm.RemoveLine(2);
		REQUIRE(0 == lm.MarkValue(0));
		REQUIRE(2 == lm.MarkValue(1));
		REQUIRE(4 == lm.MarkValue(2));
		REQUIRE(0 == lm.MarkValue(3));
		lm.InsertLines(2, 2);
		REQUIRE(0 == lm.MarkValue(0));
		REQUIRE(2 == lm.MarkValue(1));
		REQUIRE(0 == lm.MarkValue(2));
		REQUIRE(0 == lm.MarkValue(3));
		REQUIRE(4 == lm.MarkValue(4));
		REQUIRE(0 == lm.MarkValue(5));
		REQUIRE(1 == lm.LineFromHandle(handle1));
		REQUIRE(4 == lm.LineFromHandle(handle2));
	}
}

TEST_CASE("LineLevels") {

	LineLevels ll;

	SECTION("Initial") {
		// Initial State
		REQUIRE(FoldBase == ll.GetLevel(0));
	}

	SECTION("SetLevel") {
		REQUIRE(FoldBase == ll.SetLevel(1, 200, 5));
		REQUIRE(FoldBase == ll.GetLevel(0));
		REQUIRE(200 == ll.GetLevel(1));
		REQUIRE(FoldBase == ll.GetLevel(2));
		ll.ClearLevels();
		REQUIRE(FoldBase == ll.GetLevel(1));
		ll.ExpandLevels(5);
		REQUIRE(FoldBase == ll.GetLevel(7));
	}

	SECTION("InsertRemoveLine") {
		ll.SetLevel(1, 1, 5);
		ll.SetLevel(2, 2, 5);
		ll.InsertLine(2);
		REQUIRE(FoldBase == ll.GetLevel(0));
		REQUIRE(1 == ll.GetLevel(1));
		REQUIRE(2 == ll.GetLevel(2));
		REQUIRE(2 == ll.GetLevel(3));
		REQUIRE(FoldBase == ll.GetLevel(4));
		ll.RemoveLine(2);
		REQUIRE(FoldBase == ll.GetLevel(0));
		REQUIRE(1 == ll.GetLevel(1));
		REQUIRE(2 == ll.GetLevel(2));
		REQUIRE(FoldBase == ll.GetLevel(3));
		ll.InsertLines(2, 2);
		REQUIRE(FoldBase == ll.GetLevel(0));
		REQUIRE(1 == ll.GetLevel(1));
		REQUIRE(2 == ll.GetLevel(2));
		REQUIRE(2 == ll.GetLevel(3));
		REQUIRE(2 == ll.GetLevel(4));
		REQUIRE(FoldBase == ll.GetLevel(5));
	}
}

TEST_CASE("LineState") {

	LineState ls;

	SECTION("Initial") {
		// Initial State
		REQUIRE(0 == ls.GetMaxLineState());
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(1 == ls.GetMaxLineState());
		ls.Init();
		REQUIRE(0 == ls.GetMaxLineState());
		REQUIRE(0 == ls.GetLineState(0));
	}

	SECTION("SetLineState") {
		REQUIRE(0 == ls.SetLineState(1, 200));
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(200 == ls.GetLineState(1));
		REQUIRE(0 == ls.GetLineState(2));
		REQUIRE(0 == ls.SetLineState(2, 400));
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(200 == ls.GetLineState(1));
		REQUIRE(400 == ls.GetLineState(2));
		REQUIRE(0 == ls.GetLineState(3));
		// GetLineState(3) expands to 4 lines
		REQUIRE(4 == ls.GetMaxLineState());
		ls.Init();
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(1 == ls.GetMaxLineState());
	}

	SECTION("InsertRemoveLine") {
		REQUIRE(0 == ls.GetMaxLineState());
		ls.SetLineState(1, 1);
		ls.SetLineState(2, 2);
		REQUIRE(3 == ls.GetMaxLineState());
		ls.InsertLine(2);
		REQUIRE(4 == ls.GetMaxLineState());
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(1 == ls.GetLineState(1));
		REQUIRE(2 == ls.GetLineState(2));
		REQUIRE(2 == ls.GetLineState(3));
		REQUIRE(0 == ls.GetLineState(4));
		REQUIRE(5 == ls.GetMaxLineState());
		ls.RemoveLine(2);
		REQUIRE(4 == ls.GetMaxLineState());
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(1 == ls.GetLineState(1));
		REQUIRE(2 == ls.GetLineState(2));
		REQUIRE(0 == ls.GetLineState(3));
		ls.InsertLines(2, 2);
		REQUIRE(6 == ls.GetMaxLineState());
		REQUIRE(0 == ls.GetLineState(0));
		REQUIRE(1 == ls.GetLineState(1));
		REQUIRE(2 == ls.GetLineState(2));
		REQUIRE(2 == ls.GetLineState(3));
		REQUIRE(2 == ls.GetLineState(4));
		REQUIRE(0 == ls.GetLineState(5));
	}
}

TEST_CASE("LineAnnotation") {

	LineAnnotation la;

	SECTION("Initial") {
		// Initial State
		REQUIRE(0 == la.Length(0));
		REQUIRE(0 == la.Lines(0));
		REQUIRE(0 == la.Style(0));
		REQUIRE(false == la.MultipleStyles(0));
	}

	SECTION("SetText") {
		la.SetText(0, "Text");
		REQUIRE(4 == la.Length(0));
		REQUIRE(1 == la.Lines(0));
		REQUIRE(memcmp(la.Text(0), "Text", 4) == 0);
		REQUIRE(nullptr == la.Styles(0));
		REQUIRE(0 == la.Style(0));
		la.SetStyle(0, 9);
		REQUIRE(9 == la.Style(0));

		la.SetText(0, "Ant\nBird\nCat");
		REQUIRE(3 == la.Lines(0));

		la.ClearAll();
		REQUIRE(nullptr == la.Text(0));
	}

	SECTION("SetStyles") {
		la.SetText(0, "Text");
		const unsigned char styles[] { 1,2,3,4 };
		la.SetStyles(0, styles);
		REQUIRE(memcmp(la.Text(0), "Text", 4) == 0);
		REQUIRE(memcmp(la.Styles(0), styles, 4) == 0);
		REQUIRE(la.MultipleStyles(0));
	}

	SECTION("InsertRemoveLine") {
		la.SetText(0, "Ant");
		la.SetText(1, "Bird");
		REQUIRE(3 == la.Length(0));
		REQUIRE(4 == la.Length(1));
		REQUIRE(1 == la.Lines(0));
		la.InsertLine(1);
		REQUIRE(3 == la.Length(0));
		REQUIRE(0 == la.Length(1));
		REQUIRE(4 == la.Length(2));
		la.RemoveLine(2);
		REQUIRE(3 == la.Length(0));
		REQUIRE(4 == la.Length(1));
		la.InsertLines(1, 2);
		REQUIRE(3 == la.Length(0));
		REQUIRE(0 == la.Length(1));
		REQUIRE(0 == la.Length(2));
		REQUIRE(4 == la.Length(3));
	}
}

TEST_CASE("LineTabstops") {

	LineTabstops lt;

	SECTION("Initial") {
		// Initial State
		REQUIRE(0 == lt.GetNextTabstop(0, 0));
	}

	SECTION("AddClearTabstops") {
		lt.AddTabstop(0, 100);
		REQUIRE(100 == lt.GetNextTabstop(0, 0));
		REQUIRE(0 == lt.GetNextTabstop(0, 100));
		lt.ClearTabstops(0);
		REQUIRE(0 == lt.GetNextTabstop(0, 0));
	}

	SECTION("InsertRemoveLine") {
		lt.AddTabstop(0, 100);
		lt.AddTabstop(1, 200);
		lt.InsertLine(1);
		REQUIRE(100 == lt.GetNextTabstop(0, 0));
		REQUIRE(0 == lt.GetNextTabstop(1, 0));
		REQUIRE(200 == lt.GetNextTabstop(2, 0));
		lt.RemoveLine(1);
		REQUIRE(100 == lt.GetNextTabstop(0, 0));
		REQUIRE(200 == lt.GetNextTabstop(1, 0));
		lt.InsertLines(1, 2);
		REQUIRE(100 == lt.GetNextTabstop(0, 0));
		REQUIRE(0 == lt.GetNextTabstop(1, 0));
		REQUIRE(0 == lt.GetNextTabstop(2, 0));
		REQUIRE(200 == lt.GetNextTabstop(3, 0));
		lt.Init();
		REQUIRE(0 == lt.GetNextTabstop(0, 0));
	}
}
