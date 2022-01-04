/** @file testCharacterSet.cxx
 ** Unit Tests for Lexilla internal data structures
 **/

#include <cassert>
#include <cstring>

#include "CharacterSet.h"

#include "catch.hpp"

using namespace Lexilla;

// Test CharacterSet.

TEST_CASE("CharacterSet") {

	SECTION("IsEmptyInitially") {
		CharacterSet cs;
		for (int i=0; i<0x80; i++) {
			REQUIRE(!cs.Contains(i));
		}
	}

	SECTION("InitialSet") {
		CharacterSet cs(CharacterSet::setDigits);
		for (int i=0; i<0x80; i++) {
			if (i >= '0' && i <= '9')
				REQUIRE(cs.Contains(i));
			else
				REQUIRE(!cs.Contains(i));
		}
	}

	SECTION("Set") {
		CharacterSet cs;
		cs.Add('a');
		for (int i=0; i<0x80; i++) {
			if (i == 'a')
				REQUIRE(cs.Contains(i));
			else
				REQUIRE(!cs.Contains(i));
		}
	}

	SECTION("After") {
		CharacterSet cs;
		REQUIRE(!cs.Contains(0x100));
		CharacterSet cs2(CharacterSet::setNone, "", 0x80, true);
		REQUIRE(cs2.Contains(0x100));
	}
}

TEST_CASE("Functions") {

	SECTION("IsASpace") {
		REQUIRE(IsASpace(' '));
		REQUIRE(!IsASpace('a'));

		REQUIRE(IsASpaceOrTab(' '));
		REQUIRE(!IsASpaceOrTab('a'));
	}

	SECTION("IsADigit") {
		REQUIRE(!IsADigit(' '));
		REQUIRE(!IsADigit('a'));
		REQUIRE(IsADigit('7'));

		REQUIRE(IsADigit('7', 16));
		REQUIRE(IsADigit('A', 16));
		REQUIRE(IsADigit('a', 16));
		REQUIRE(!IsADigit('8', 8));
	}

	SECTION("IsASCII") {
		REQUIRE(IsASCII(' '));
		REQUIRE(IsASCII('a'));
		REQUIRE(!IsASCII(-1));
		REQUIRE(!IsASCII(128));
	}

	SECTION("IsUpperOrLowerCase") {
		REQUIRE(IsLowerCase('a'));
		REQUIRE(!IsLowerCase('A'));

		REQUIRE(!IsUpperCase('a'));
		REQUIRE(IsUpperCase('A'));

		REQUIRE(IsUpperOrLowerCase('a'));
		REQUIRE(IsUpperOrLowerCase('A'));
		REQUIRE(!IsUpperOrLowerCase('9'));

		REQUIRE(IsAlphaNumeric('9'));
		REQUIRE(IsAlphaNumeric('a'));
		REQUIRE(IsAlphaNumeric('A'));
		REQUIRE(!IsAlphaNumeric(' '));
		REQUIRE(!IsAlphaNumeric('+'));
	}

	SECTION("isoperator") {
		REQUIRE(isspacechar(' '));
		REQUIRE(!isspacechar('a'));

		REQUIRE(!iswordchar(' '));
		REQUIRE(iswordchar('a'));
		REQUIRE(iswordchar('A'));
		REQUIRE(iswordchar('.'));
		REQUIRE(iswordchar('_'));

		REQUIRE(!iswordstart(' '));
		REQUIRE(iswordstart('a'));
		REQUIRE(iswordstart('A'));
		REQUIRE(iswordstart('_'));

		REQUIRE(!isoperator('a'));
		REQUIRE(isoperator('+'));
	}

	SECTION("MakeUpperCase") {
		REQUIRE(MakeUpperCase(' ') == ' ');
		REQUIRE(MakeUpperCase('A') == 'A');
		REQUIRE(MakeUpperCase('a') == 'A');

		REQUIRE(MakeLowerCase(' ') == ' ');
		REQUIRE(MakeLowerCase('A') == 'a');
		REQUIRE(MakeLowerCase('a') == 'a');
	}

	SECTION("CompareCaseInsensitive") {
		REQUIRE(CompareCaseInsensitive(" ", " ") == 0);
		REQUIRE(CompareCaseInsensitive("A", "A") == 0);
		REQUIRE(CompareCaseInsensitive("a", "A") == 0);
		REQUIRE(CompareCaseInsensitive("b", "A") != 0);
		REQUIRE(CompareCaseInsensitive("aa", "A") != 0);

		REQUIRE(CompareNCaseInsensitive(" ", " ", 1) == 0);
		REQUIRE(CompareNCaseInsensitive("b", "A", 1) != 0);
		REQUIRE(CompareNCaseInsensitive("aa", "A", 1) == 0);
	}

}