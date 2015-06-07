// Unit Tests for Scintilla internal data structures

#include <string.h>

#include <algorithm>

#include "Platform.h"

#include "UnicodeFromUTF8.h"

#include "catch.hpp"

// Test UnicodeFromUTF8.
// Use examples from Wikipedia:
// http://en.wikipedia.org/wiki/UTF-8

TEST_CASE("UnicodeFromUTF8") {

	SECTION("ASCII") {
		const unsigned char s[]={'a', 0};
		REQUIRE(UnicodeFromUTF8(s) == 'a');
	}

	SECTION("Example1") {
		const unsigned char s[]={0x24, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x24);
	}

	SECTION("Example2") {
		const unsigned char s[]={0xC2, 0xA2, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0xA2);
	}

	SECTION("Example3") {
		const unsigned char s[]={0xE2, 0x82, 0xAC, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x20AC);
	}

	SECTION("Example4") {
		const unsigned char s[]={0xF0, 0x90, 0x8D, 0x88, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x10348);
	}

}
