/** @file testWordList.cxx
 ** Unit Tests for Lexilla internal data structures
 **/

#include <string.h>

#include "WordList.h"

#include "catch.hpp"

using namespace Lexilla;

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

	SECTION("InListUnicode") {
		// "cheese" in English
		// "kase" ('k', 'a with diaeresis', 's', 'e') in German
		// "syr", ('CYRILLIC SMALL LETTER ES', 'CYRILLIC SMALL LETTER YERU', 'CYRILLIC SMALL LETTER ER') in Russian
		wl.Set("cheese \x6b\xc3\xa4\x73\x65 \xd1\x81\xd1\x8b\xd1\x80");
		REQUIRE(3 == wl.Length());
		REQUIRE(wl.InList("cheese"));
		REQUIRE(wl.InList("\x6b\xc3\xa4\x73\x65"));
		REQUIRE(wl.InList("\xd1\x81\xd1\x8b\xd1\x80"));
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
		wl.Set("else stru~ct w~hile \xd1\x81~\xd1\x8b\xd1\x80");
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

		// Russian syr
		REQUIRE(wl.InListAbbreviated("\xd1\x81\xd1\x8b\xd1\x80", '~'));
	}

	SECTION("InListAbridged") {
		wl.Set("list w.~.active bo~k a~z ~_frozen \xd1\x81~\xd1\x80");
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

		// Russian syr
		REQUIRE(wl.InListAbridged("\xd1\x81\xd1\x8b\xd1\x80", '~'));
	}
}
