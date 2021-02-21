// Unit Tests for Scintilla internal data structures

#include <string.h>

#include "WordList.h"

#include "catch.hpp"

using namespace Scintilla;

// Test WordList.

TEST_CASE("WordList") {

	WordList wl;

	SECTION("IsEmptyInitially") {
		REQUIRE(0 == wl.Length());
		REQUIRE(!wl.InList("struct"));
	}

	SECTION("InList") {
		wl.Set("else struct");
		REQUIRE(2 == wl.Length());
		REQUIRE(wl.InList("struct"));
		REQUIRE(!wl.InList("class"));
	}

	SECTION("Set") {
		// Check whether Set returns whether it has changed correctly
		const bool changed = wl.Set("else struct");
		REQUIRE(changed);
		// Changing to same thing
		const bool changed2 = wl.Set("else struct");
		REQUIRE(!changed2);
		// Changed order shouldn't be seen as a change
		const bool changed3 = wl.Set("struct else");
		REQUIRE(!changed3);
		// Removing word is a change
		const bool changed4 = wl.Set("struct");
		REQUIRE(changed4);
	}

	SECTION("WordAt") {
		wl.Set("else struct");
		REQUIRE(0 == strcmp(wl.WordAt(0), "else"));
	}

	SECTION("InListAbbreviated") {
		wl.Set("else stru~ct w~hile");
		REQUIRE(wl.InListAbbreviated("else", '~'));

		REQUIRE(wl.InListAbbreviated("struct", '~'));
		REQUIRE(wl.InListAbbreviated("stru", '~'));
		REQUIRE(wl.InListAbbreviated("struc", '~'));
		REQUIRE(!wl.InListAbbreviated("str", '~'));

		REQUIRE(wl.InListAbbreviated("while", '~'));
		REQUIRE(wl.InListAbbreviated("wh", '~'));
		// TODO: Next line fails but should allow single character prefixes
		//REQUIRE(wl.InListAbbreviated("w", '~'));
		REQUIRE(!wl.InListAbbreviated("", '~'));
	}

	SECTION("InListAbridged") {
		wl.Set("list w.~.active bo~k a~z ~_frozen");
		REQUIRE(wl.InListAbridged("list", '~'));

		REQUIRE(wl.InListAbridged("w.front.active", '~'));
		REQUIRE(wl.InListAbridged("w.x.active", '~'));
		REQUIRE(wl.InListAbridged("w..active", '~'));
		REQUIRE(!wl.InListAbridged("w.active", '~'));
		REQUIRE(!wl.InListAbridged("w.x.closed", '~'));

		REQUIRE(wl.InListAbridged("book", '~'));
		REQUIRE(wl.InListAbridged("bok", '~'));
		REQUIRE(!wl.InListAbridged("bk", '~'));

		REQUIRE(wl.InListAbridged("a_frozen", '~'));
		REQUIRE(wl.InListAbridged("_frozen", '~'));
		REQUIRE(!wl.InListAbridged("frozen", '~'));

		REQUIRE(wl.InListAbridged("abcz", '~'));
		REQUIRE(wl.InListAbridged("abz", '~'));
		REQUIRE(wl.InListAbridged("az", '~'));
	}
}
