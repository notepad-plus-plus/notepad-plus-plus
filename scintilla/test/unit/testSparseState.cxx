// Unit Tests for Scintilla internal data structures

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Sci_Position.h"

#include "SparseState.h"

#include "catch.hpp"

using namespace Scintilla;

// Test SparseState.

TEST_CASE("SparseState") {

	SparseState<int> ss;

	SECTION("IsEmptyInitially") {
		REQUIRE(0u == ss.size());
		int val = ss.ValueAt(0);
		REQUIRE(0 == val);
	}

	SECTION("SimpleSetAndGet") {
		ss.Set(0, 22);
		ss.Set(1, 23);
		REQUIRE(2u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(22 == ss.ValueAt(0));
		REQUIRE(23 == ss.ValueAt(1));
		REQUIRE(23 == ss.ValueAt(2));
	}

	SECTION("RetrieveBetween") {
		ss.Set(0, 10);
		ss.Set(2, 12);
		REQUIRE(2u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(10 == ss.ValueAt(0));
		REQUIRE(10 == ss.ValueAt(1));
		REQUIRE(12 == ss.ValueAt(2));
	}

	SECTION("RetrieveBefore") {
		ss.Set(2, 12);
		REQUIRE(1u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(0 == ss.ValueAt(0));
		REQUIRE(0 == ss.ValueAt(1));
		REQUIRE(12 == ss.ValueAt(2));
	}

	SECTION("Delete") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Delete(2);
		REQUIRE(1u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(30 == ss.ValueAt(0));
		REQUIRE(30 == ss.ValueAt(1));
		REQUIRE(30 == ss.ValueAt(2));
	}

	SECTION("DeleteBetween") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Delete(1);
		REQUIRE(1u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(30 == ss.ValueAt(0));
		REQUIRE(30 == ss.ValueAt(1));
		REQUIRE(30 == ss.ValueAt(2));
	}

	SECTION("ReplaceLast") {
		ss.Set(0, 30);
		ss.Set(2, 31);
		ss.Set(2, 32);
		REQUIRE(2u == ss.size());
		REQUIRE(0 == ss.ValueAt(-1));
		REQUIRE(30 == ss.ValueAt(0));
		REQUIRE(30 == ss.ValueAt(1));
		REQUIRE(32 == ss.ValueAt(2));
		REQUIRE(32 == ss.ValueAt(3));
	}

	SECTION("OnlyChangeAppended") {
		ss.Set(0, 30);
		ss.Set(2, 31);
		ss.Set(3, 31);
		REQUIRE(2u == ss.size());
	}

	SECTION("MergeBetween") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);
		REQUIRE(3u == ss.size());

		SparseState<int> ssAdditions(3);
		ssAdditions.Set(4, 34);
		REQUIRE(1u == ssAdditions.size());
		bool mergeChanged = ss.Merge(ssAdditions,5);
		REQUIRE(false == mergeChanged);

		ssAdditions.Set(4, 44);
		REQUIRE(1u == ssAdditions.size());
		mergeChanged = ss.Merge(ssAdditions,5);
		REQUIRE(true == mergeChanged);
		REQUIRE(3u == ss.size());
		REQUIRE(44 == ss.ValueAt(4));
	}

	SECTION("MergeAtExisting") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);
		REQUIRE(3u == ss.size());

		SparseState<int> ssAdditions(4);
		ssAdditions.Set(4, 34);
		REQUIRE(1u == ssAdditions.size());
		bool mergeChanged = ss.Merge(ssAdditions,5);
		REQUIRE(false == mergeChanged);

		ssAdditions.Set(4, 44);
		REQUIRE(1u == ssAdditions.size());
		mergeChanged = ss.Merge(ssAdditions,5);
		REQUIRE(true == mergeChanged);
		REQUIRE(3u == ss.size());
		REQUIRE(44 == ss.ValueAt(4));
	}

	SECTION("MergeWhichRemoves") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);
		REQUIRE(3u == ss.size());

		SparseState<int> ssAdditions(2);
		ssAdditions.Set(2, 22);
		REQUIRE(1u == ssAdditions.size());
		REQUIRE(22 == ssAdditions.ValueAt(2));
		bool mergeChanged = ss.Merge(ssAdditions,5);
		REQUIRE(true == mergeChanged);
		REQUIRE(2u == ss.size());
		REQUIRE(22 == ss.ValueAt(2));
	}

	SECTION("MergeIgnoreSome") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);

		SparseState<int> ssAdditions(2);
		ssAdditions.Set(2, 32);
		bool mergeChanged = ss.Merge(ssAdditions,3);

		REQUIRE(false == mergeChanged);
		REQUIRE(2u == ss.size());
		REQUIRE(32 == ss.ValueAt(2));
	}

	SECTION("MergeIgnoreSomeStart") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);

		SparseState<int> ssAdditions(2);
		ssAdditions.Set(2, 32);
		bool mergeChanged = ss.Merge(ssAdditions,2);

		REQUIRE(false == mergeChanged);
		REQUIRE(2u == ss.size());
		REQUIRE(32 == ss.ValueAt(2));
	}

	SECTION("MergeIgnoreRepeat") {
		ss.Set(0, 30);
		ss.Set(2, 32);
		ss.Set(4, 34);

		SparseState<int> ssAdditions(5);
		// Appending same value as at end of pss.
		ssAdditions.Set(5, 34);
		bool mergeChanged = ss.Merge(ssAdditions,6);

		REQUIRE(false == mergeChanged);
		REQUIRE(3u == ss.size());
		REQUIRE(34 == ss.ValueAt(4));
	}

}

TEST_CASE("SparseStateString") {

	SparseState<std::string> ss;

	SECTION("IsEmptyInitially") {
		REQUIRE(0u == ss.size());
		std::string val = ss.ValueAt(0);
		REQUIRE("" == val);
	}

	SECTION("SimpleSetAndGet") {
		REQUIRE(0u == ss.size());
		ss.Set(0, "22");
		ss.Set(1, "23");
		REQUIRE(2u == ss.size());
		REQUIRE("" == ss.ValueAt(-1));
		REQUIRE("22" == ss.ValueAt(0));
		REQUIRE("23" == ss.ValueAt(1));
		REQUIRE("23" == ss.ValueAt(2));
	}

	SECTION("DeleteBetween") {
		ss.Set(0, "30");
		ss.Set(2, "32");
		ss.Delete(1);
		REQUIRE(1u == ss.size());
		REQUIRE("" == ss.ValueAt(-1));
		REQUIRE("30" == ss.ValueAt(0));
		REQUIRE("30" == ss.ValueAt(1));
		REQUIRE("30" == ss.ValueAt(2));
	}

}
