/** @file testWordList.cxx
 ** Unit Tests for Lexilla internal data structures
 ** Tests WordList, WordClassifier, and SubStyles
 **/

#include <cassert>

#include <string>
#include <string_view>
#include <vector>
#include <map>

#include "WordList.h"
#include "CharacterSet.h"
#include "SubStyles.h"

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

	SECTION("StringInList") {
		wl.Set("else struct");
		std::string sStruct = "struct";
		REQUIRE(wl.InList(sStruct));
		std::string sClass = "class";
		REQUIRE(!wl.InList(sClass));
	}

	SECTION("StringViewInList") {
		wl.Set("else struct i ^gtk");
		std::string_view svStruct = "struct";
		REQUIRE(wl.InList(svStruct));
		std::string_view svClass = "class";
		REQUIRE(!wl.InList(svClass));

		// Test single characters
		std::string_view svI = "i";
		REQUIRE(wl.InList(svI));
		std::string_view svA = "a";
		REQUIRE(!wl.InList(svA));

		// Test prefix
		std::string_view svPrefix = "gtk_prefix";
		REQUIRE(wl.InList(svPrefix));
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
		REQUIRE_THAT(wl.WordAt(0), Catch::Matchers::Equals("else"));
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

// Test WordClassifier.

TEST_CASE("WordClassifier") {

	constexpr int base = 1;
	constexpr int key = 10;
	constexpr int type = 11;
	constexpr int other = 40;

	WordClassifier wc(1);

	SECTION("Base") {
		REQUIRE(wc.Base() == base);
		wc.Allocate(key, 2);
		REQUIRE(wc.Start() == key);
		REQUIRE(wc.Last() == type);
		REQUIRE(wc.Length() == 2);
		REQUIRE(wc.IncludesStyle(key));
		REQUIRE(wc.IncludesStyle(type));
		REQUIRE(!wc.IncludesStyle(other));

		wc.Clear();
		REQUIRE(wc.Base() == base);
		REQUIRE(wc.Start() == 0);
		REQUIRE(wc.Last() == -1);
		REQUIRE(wc.Length() == 0);
	}

	SECTION("ValueFor") {
		wc.Allocate(key, 2);
		wc.SetIdentifiers(key, "else if then", false);
		wc.SetIdentifiers(type, "double float int long", false);
		REQUIRE(wc.ValueFor("if") == key);
		REQUIRE(wc.ValueFor("double") == type);
		REQUIRE(wc.ValueFor("fish") < 0);
		wc.RemoveStyle(type);
		REQUIRE(wc.ValueFor("double") < 0);
	}

}

// Test SubStyles.

TEST_CASE("SubStyles") {

	constexpr char bases[] = "\002\005";
	constexpr int base = 2;
	constexpr int base2 = 5;
	constexpr int styleFirst = 0x80;
	constexpr int stylesAvailable = 0x40;
	constexpr int distanceToSecondary = 0x40;

	SubStyles subStyles(bases, styleFirst, stylesAvailable, distanceToSecondary);

	SECTION("All") {
		REQUIRE(subStyles.DistanceToSecondaryStyles() == distanceToSecondary);
		// Before allocation
		REQUIRE(subStyles.Start(base) == 0);

		const int startSubStyles = subStyles.Allocate(base, 3);
		REQUIRE(startSubStyles == styleFirst);
		REQUIRE(subStyles.Start(base) == styleFirst);
		REQUIRE(subStyles.Length(base) == 3);
		REQUIRE(subStyles.BaseStyle(128) == 2);

		// Not a substyle so returns argument.
		REQUIRE(subStyles.BaseStyle(96) == 96);

		REQUIRE(subStyles.FirstAllocated() == styleFirst);
		REQUIRE(subStyles.LastAllocated() == styleFirst + 3 - 1);
		subStyles.SetIdentifiers(styleFirst, "int long size_t");
		const WordClassifier &wc = subStyles.Classifier(base);
		REQUIRE(wc.ValueFor("int") == styleFirst);
		REQUIRE(wc.ValueFor("double") < 0);

		// Add second set of substyles which shouldn't affect first
		const int startSecondSet = subStyles.Allocate(base2, 2);
		constexpr int expectedStylesSecond = styleFirst + 3;
		REQUIRE(startSecondSet == expectedStylesSecond);
		REQUIRE(subStyles.Start(base) == styleFirst);
		REQUIRE(subStyles.Start(base2) == expectedStylesSecond);
		REQUIRE(subStyles.LastAllocated() == styleFirst + 5 - 1);

		// Clear and check that earlier call no longer works
		subStyles.Free();
		REQUIRE(subStyles.Start(base) == 0);
	}

}
