/** @file testInList.cxx
 ** Unit Tests for Lexilla internal data structures
 **/

#include <cstdlib>
#include <cassert>

#include <string_view>
#include <initializer_list>

#include "InList.h"

#include "catch.hpp"

using namespace Lexilla;

// Test InList.

TEST_CASE("InList") {

	SECTION("Basic") {
		REQUIRE(InList("dog", {"cat", "dog", "frog"}));
		REQUIRE(!InList("fly", {"cat", "dog", "frog"}));

		REQUIRE(InListCaseInsensitive("DOG", {"cat", "dog", "frog"}));
		REQUIRE(!InListCaseInsensitive("fly", {"cat", "dog", "frog"}));
	}
}
