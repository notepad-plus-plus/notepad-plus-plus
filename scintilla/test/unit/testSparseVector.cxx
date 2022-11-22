/** @file testSparseVector.cxx
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

#include "Debugging.h"

#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "SparseVector.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

// Test SparseVector.

using UniqueInt = std::unique_ptr<int>;

TEST_CASE("CompileCopying SparseVector") {

	// These are compile-time tests to check that basic copy and move
	// operations are defined correctly.

	SECTION("CopyingMoving") {
		SparseVector<int> s;
		SparseVector<int> s2;

		// Copy constructor
		SparseVector<int> sa(s);
		// Copy assignment
		SparseVector<int> sb;
		sb = s;

		// Move constructor
		SparseVector<int> sc(std::move(s));
		// Move assignment
		SparseVector<int> sd;
		sd = (std::move(s2));
	}

	SECTION("MoveOnly") {
		SparseVector<UniqueInt> s;

#if defined(SHOW_COPY_BUILD_FAILURES)
		// Copy is not defined for std::unique_ptr
		// Copy constructor fails
		SparseVector<UniqueInt> sa(s);
		// Copy assignment fails
		SparseVector<UniqueInt> sb;
		sb = s;
#endif

		// Move constructor
		SparseVector<UniqueInt> sc(std::move(s));
		// Move assignment
		SparseVector<UniqueInt> sd;
		sd = (std::move(s));
	}

}

// Helper to produce a string representation of a SparseVector<const char *>
// to simplify checks.
static std::string Representation(const SparseVector<UniqueString> &st) {
	std::string ret;
	for (int i = 0;i <= st.Length();i++) {
		const char *value = st.ValueAt(i).get();
		if (value && *value)
			ret += value;
		else
			ret += "-";
	}
	return ret;
}

TEST_CASE("SparseVector") {

	SparseVector<UniqueString> st;

	SECTION("IsEmptyInitially") {
		REQUIRE(1 == st.Elements());
		REQUIRE(0 == st.Length());
		REQUIRE("-" == Representation(st));
		st.Check();
	}

	SECTION("InsertSpace") {
		st.InsertSpace(0, 5);
		REQUIRE(1 == st.Elements());
		REQUIRE(static_cast<const char *>(nullptr) == st.ValueAt(0).get());
		REQUIRE(static_cast<const char *>(nullptr) == st.ValueAt(1).get());
		REQUIRE(static_cast<const char *>(nullptr) == st.ValueAt(4).get());
		st.Check();
	}

	SECTION("InsertValue") {
		st.InsertSpace(0, 5);
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE(2 == st.Elements());
		REQUIRE("---3--" == Representation(st));
		st.Check();
	}

	SECTION("InsertAndChangeAndDeleteValue") {
		st.InsertSpace(0, 5);
		REQUIRE(5 == st.Length());
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE(2 == st.Elements());
		st.SetValueAt(3, UniqueStringCopy("4"));
		REQUIRE(2 == st.Elements());
		st.DeletePosition(3);
		REQUIRE(1 == st.Elements());
		REQUIRE(4 == st.Length());
		REQUIRE("-----" == Representation(st));
		st.Check();
	}

	SECTION("InsertAndDeleteAtStart") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(0, UniqueStringCopy("3"));
		REQUIRE(1 == st.Elements());
		REQUIRE("3-----" == Representation(st));
		st.DeletePosition(0);
		REQUIRE(1 == st.Elements());
		REQUIRE("-----" == Representation(st));
		st.SetValueAt(0, UniqueStringCopy("4"));
		REQUIRE(1 == st.Elements());
		REQUIRE("4----" == Representation(st));
		st.DeletePosition(0);
		REQUIRE(1 == st.Elements());
		REQUIRE("----" == Representation(st));
		st.SetValueAt(0, UniqueStringCopy("4"));
		REQUIRE(1 == st.Elements());
		REQUIRE("4---" == Representation(st));
		st.DeletePosition(0);
		REQUIRE(1 == st.Elements());
		REQUIRE("---" == Representation(st));
		st.Check();
	}

	SECTION("InsertStringAtStartThenInsertSpaceAtStart") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(0, UniqueStringCopy("3"));
		REQUIRE(1 == st.Elements());
		REQUIRE("3-----" == Representation(st));
		st.InsertSpace(0, 1);
		REQUIRE(2 == st.Elements());
		REQUIRE("-3-----" == Representation(st));
		st.Check();
	}

	SECTION("InsertSpaceAfterStart") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(1, UniqueStringCopy("1"));
		REQUIRE(2 == st.Elements());
		REQUIRE("-1----" == Representation(st));
		st.InsertSpace(1, 1);
		REQUIRE(2 == st.Elements());
		REQUIRE("--1----" == Representation(st));
		st.Check();
	}

	SECTION("InsertStringAt1ThenInsertLettersAt1") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(1, UniqueStringCopy("9"));
		REQUIRE(2 == st.Elements());
		REQUIRE("-9----" == Representation(st));
		st.InsertSpace(0, 1);
		REQUIRE(2 == st.Elements());
		REQUIRE("--9----" == Representation(st));
		// Initial st has allocation of 9 values so this should cause reallocation
		const std::string letters("ABCDEFGHIJKLMNOP");	// 16 letters
		for (const char letter : letters) {
			const char sLetter[] = { letter, 0 };
			st.InsertSpace(0, 1);
			st.SetValueAt(1, UniqueStringCopy(sLetter));
		}
		REQUIRE("-PONMLKJIHGFEDCBA-9----" == Representation(st));
		st.Check();
	}

	SECTION("InsertAndDeleteAtEnd") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(4, UniqueStringCopy("5"));
		REQUIRE(2 == st.Elements());
		REQUIRE("----5-" == Representation(st));
		st.SetValueAt(5, UniqueStringCopy("6"));
		REQUIRE(2 == st.Elements());
		REQUIRE("----56" == Representation(st));
		st.DeletePosition(4);
		REQUIRE(1 == st.Elements());
		REQUIRE("----6" == Representation(st));
		st.SetValueAt(4, UniqueStringCopy("7"));
		REQUIRE(1 == st.Elements());
		REQUIRE("----7" == Representation(st));
		st.Check();
	}

	SECTION("SetNULL") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 5);
		st.SetValueAt(4, UniqueStringCopy("5"));
		REQUIRE(2 == st.Elements());
		REQUIRE("----5-" == Representation(st));
		st.SetValueAt(4, nullptr);
		REQUIRE(1 == st.Elements());
		REQUIRE("------" == Representation(st));
		st.Check();
		st.SetValueAt(5, nullptr);
		REQUIRE(1 == st.Elements());
		REQUIRE("------" == Representation(st));
		st.Check();
	}

	SECTION("CheckDeletionLeavesOrdered") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 1);
		st.SetValueAt(0, UniqueStringCopy("1"));
		REQUIRE("1-" == Representation(st));
		REQUIRE(1 == st.Elements());
		st.InsertSpace(1, 1);
		st.SetValueAt(1, UniqueStringCopy("2"));
		REQUIRE("12-" == Representation(st));
		st.DeletePosition(0);
		REQUIRE("2-" == Representation(st));
		REQUIRE(1 == st.Elements());
		st.DeletePosition(0);
		REQUIRE("-" == Representation(st));
	}

	SECTION("DeleteAll") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 10);
		st.SetValueAt(9, UniqueStringCopy("9"));
		st.SetValueAt(7, UniqueStringCopy("7"));
		st.SetValueAt(4, UniqueStringCopy("4"));
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE(5 == st.Elements());
		REQUIRE("---34--7-9-" == Representation(st));
		st.DeleteAll();
		REQUIRE(1 == st.Elements());
		REQUIRE("-" == Representation(st));
		st.Check();
	}

	SECTION("DeleteStarting") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 2);
		st.SetValueAt(0, UniqueStringCopy("1"));
		st.SetValueAt(1, UniqueStringCopy("2"));
		REQUIRE("12-" == Representation(st));
		st.DeletePosition(0);
		REQUIRE("2-" == Representation(st));
		st.DeletePosition(0);
		REQUIRE("-" == Representation(st));
	}

	SECTION("DeleteRange") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 10);
		st.SetValueAt(9, UniqueStringCopy("9"));
		st.SetValueAt(7, UniqueStringCopy("7"));
		st.SetValueAt(4, UniqueStringCopy("4"));
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE(5 == st.Elements());
		REQUIRE(10 == st.Length());
		REQUIRE("---34--7-9-" == Representation(st));
		// Delete in space
		st.DeleteRange(1, 1);
		REQUIRE(5 == st.Elements());
		REQUIRE(9 == st.Length());
		REQUIRE("--34--7-9-" == Representation(st));
		// Delete 2 values
		st.DeleteRange(3, 4);
		REQUIRE(3 == st.Elements());
		REQUIRE(5 == st.Length());
		REQUIRE("--3-9-" == Representation(st));
		// Deletion at start
		st.DeleteRange(0, 1);
		REQUIRE(3 == st.Elements());
		REQUIRE(4 == st.Length());
		REQUIRE("-3-9-" == Representation(st));
	}

	SECTION("DeleteRangeAtEnds") {
		// There are always elements at start and end although they can be nulled
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 4);
		REQUIRE(4 == st.Length());
		st.SetValueAt(1, UniqueStringCopy("3"));
		st.SetValueAt(4, UniqueStringCopy("9"));
		REQUIRE("-3--9" == Representation(st));
		REQUIRE(2 == st.Elements());
		// Empty deletion at end -> no effect
		st.DeleteRange(4, 0);
		REQUIRE(2 == st.Elements());
		REQUIRE(4 == st.Length());
		REQUIRE("-3--9" == Representation(st));
		// Delete value at start
		st.InsertSpace(0, 1);
		st.SetValueAt(0, UniqueStringCopy("0"));
		REQUIRE(2 == st.Elements());
		REQUIRE(5 == st.Length());
		REQUIRE("0-3--9" == Representation(st));
		st.DeleteRange(0, 1);
		REQUIRE(2 == st.Elements());
		REQUIRE(4 == st.Length());
		REQUIRE("03--9" == Representation(st));
		// Empty deletion at start -> no effect
		st.InsertSpace(0, 1);
		st.SetValueAt(0, UniqueStringCopy("1"));
		REQUIRE(3 == st.Elements());
		REQUIRE(5 == st.Length());
		REQUIRE("103--9" == Representation(st));
		st.DeleteRange(0, 0);
		REQUIRE(3 == st.Elements());
		REQUIRE(5 == st.Length());
		REQUIRE("103--9" == Representation(st));
	}

	SECTION("DeleteStartingRange") {
		REQUIRE(1 == st.Elements());
		st.InsertSpace(0, 2);
		st.SetValueAt(0, UniqueStringCopy("1"));
		st.SetValueAt(1, UniqueStringCopy("2"));
		REQUIRE(2 == st.Length());
		REQUIRE("12-" == Representation(st));
		st.DeleteRange(0,1);
		REQUIRE(1 == st.Length());
		REQUIRE("2-" == Representation(st));
		st.DeleteRange(0,1);
		REQUIRE(0 == st.Length());
		REQUIRE("-" == Representation(st));
		st.InsertSpace(0, 2);
		st.SetValueAt(1, UniqueStringCopy("1"));
		REQUIRE(2 == st.Length());
		REQUIRE("-1-" == Representation(st));
		st.DeleteRange(0, 2);
		REQUIRE("-" == Representation(st));
		st.InsertSpace(0, 4);
		st.SetValueAt(1, UniqueStringCopy("1"));
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE(4 == st.Length());
		REQUIRE("-1-3-" == Representation(st));
		st.DeleteRange(0, 3);
		REQUIRE("3-" == Representation(st));
		st.DeleteRange(0, 1);
		REQUIRE("-" == Representation(st));
		st.InsertSpace(0, 4);
		st.SetValueAt(1, UniqueStringCopy("1"));
		st.SetValueAt(4, UniqueStringCopy("4"));
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE("-1-34" == Representation(st));
		st.DeleteRange(1, 3);
		REQUIRE("-4" == Representation(st));
		st.InsertSpace(1, 3);
		REQUIRE("----4" == Representation(st));
		st.SetValueAt(4, UniqueStringCopy("4"));
		st.SetValueAt(3, UniqueStringCopy("3"));
		REQUIRE("---34" == Representation(st));
		st.DeleteRange(1, 3);
		REQUIRE("-4" == Representation(st));
	}
}

TEST_CASE("SparseTextInt") {

	SparseVector<int> st;

	SECTION("InsertAndDeleteValue") {
		st.InsertSpace(0, 5);
		st.SetValueAt(3, 3);
		REQUIRE(2 == st.Elements());
		REQUIRE(0 == st.ValueAt(0));
		REQUIRE(0 == st.ValueAt(1));
		REQUIRE(0 == st.ValueAt(2));
		REQUIRE(3 == st.ValueAt(3));
		REQUIRE(0 == st.ValueAt(4));
		st.SetValueAt(3, -3);
		REQUIRE(2 == st.Elements());
		REQUIRE(0 == st.ValueAt(0));
		REQUIRE(0 == st.ValueAt(1));
		REQUIRE(0 == st.ValueAt(2));
		REQUIRE(-3 == st.ValueAt(3));
		REQUIRE(0 == st.ValueAt(4));
		st.SetValueAt(3, 0);
		REQUIRE(1 == st.Elements());
		REQUIRE(0 == st.ValueAt(0));
		REQUIRE(0 == st.ValueAt(1));
		REQUIRE(0 == st.ValueAt(2));
		REQUIRE(0 == st.ValueAt(3));
		REQUIRE(0 == st.ValueAt(4));
		st.Check();
	}

	SECTION("IndexAfter") {
		st.InsertSpace(0, 5);
		REQUIRE(1 == st.Elements());
		REQUIRE(0 == st.IndexAfter(-1));
		REQUIRE(0 == st.PositionOfElement(0));
		REQUIRE(1 == st.IndexAfter(0));
		REQUIRE(5 == st.PositionOfElement(1));
		st.SetValueAt(3, 3);
		REQUIRE(2 == st.Elements());
		REQUIRE(0 == st.IndexAfter(-1));
		REQUIRE(0 == st.PositionOfElement(0));
		REQUIRE(1 == st.IndexAfter(0));
		REQUIRE(3 == st.PositionOfElement(1));
		REQUIRE(2 == st.IndexAfter(3));
		REQUIRE(5 == st.PositionOfElement(2));
		REQUIRE(2 == st.IndexAfter(4));
	}

	SECTION("PositionNext") {
		st.InsertSpace(0, 5);
		REQUIRE(1 == st.Elements());
		REQUIRE(5 == st.PositionNext(-1));
		REQUIRE(5 == st.PositionNext(0));
		REQUIRE(6 == st.PositionNext(5));
		st.SetValueAt(3, 3);
		REQUIRE(2 == st.Elements());
		REQUIRE(3 == st.PositionNext(-1));
		REQUIRE(3 == st.PositionNext(0));
		REQUIRE(5 == st.PositionNext(3));
		REQUIRE(6 == st.PositionNext(5));
	}
}

TEST_CASE("SparseTextString") {

	SparseVector<std::string> st;

	SECTION("InsertAndDeleteValue") {
		st.InsertSpace(0, 5);
		REQUIRE(5 == st.Length());
		st.SetValueAt(3, std::string("3"));
		REQUIRE(2 == st.Elements());
		REQUIRE("" == st.ValueAt(0));
		REQUIRE("" == st.ValueAt(2));
		REQUIRE("3" == st.ValueAt(3));
		REQUIRE("" == st.ValueAt(4));
		st.DeletePosition(0);
		REQUIRE(4 == st.Length());
		REQUIRE("3" == st.ValueAt(2));
		st.DeletePosition(2);
		REQUIRE(1 == st.Elements());
		REQUIRE(3 == st.Length());
		REQUIRE("" == st.ValueAt(0));
		REQUIRE("" == st.ValueAt(1));
		REQUIRE("" == st.ValueAt(2));
		st.Check();
	}

	SECTION("SetAndMoveString") {
		st.InsertSpace(0, 2);
		REQUIRE(2u == st.Length());
		std::string s24("24");
		st.SetValueAt(0, s24);
		REQUIRE("24" == s24);	// Not moved from
		REQUIRE("" == st.ValueAt(-1));
		REQUIRE("24" == st.ValueAt(0));
		REQUIRE("" == st.ValueAt(1));
		std::string s25("25");
		st.SetValueAt(1, std::move(s25));
		REQUIRE("" == s25);	// moved from
		REQUIRE("25" == st.ValueAt(1));
	}

}
