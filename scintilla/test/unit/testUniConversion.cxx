/** @file testUniConversion.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstring>

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "Debugging.h"

#include "UniConversion.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

// Test UniConversion.
// Use examples from Wikipedia:
// https://en.wikipedia.org/wiki/UTF-8

TEST_CASE("UTF16Length") {

	SECTION("UTF16Length ASCII") {
		// Latin Small Letter A
		const char *s = "a";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 1U);
	}

	SECTION("UTF16Length Example1") {
		// Dollar Sign
		const char *s = "\x24";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 1U);
	}

	SECTION("UTF16Length Example2") {
		// Cent Sign
		const char *s = "\xC2\xA2";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 1U);
	}

	SECTION("UTF16Length Example3") {
		// Euro Sign
		const char *s = "\xE2\x82\xAC";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 1U);
	}

	SECTION("UTF16Length Example4") {
		// Gothic Letter Hwair
		const char *s = "\xF0\x90\x8D\x88";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 2U);
	}

	SECTION("UTF16Length Invalid Trail byte in lead position") {
		const char *s = "a\xB5yz";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 4U);
	}

	SECTION("UTF16Length Invalid Lead byte at end") {
		const char *s = "a\xC2";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 2U);
	}

	SECTION("UTF16Length Invalid Lead byte implies 3 trails but only 2") {
		const char *s = "a\xF1yz";
		const size_t len = UTF16Length(s);
		REQUIRE(len == 2U);
	}
}

TEST_CASE("UniConversion") {

	// UnicodeFromUTF8

	SECTION("UnicodeFromUTF8 ASCII") {
		const unsigned char s[]={'a', 0, 0, 0};
		REQUIRE(UnicodeFromUTF8(s) == 'a');
	}

	SECTION("UnicodeFromUTF8 Example1") {
		const unsigned char s[]={0x24, 0, 0, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x24);
	}

	SECTION("UnicodeFromUTF8 Example2") {
		const unsigned char s[]={0xC2, 0xA2, 0, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0xA2);
	}

	SECTION("UnicodeFromUTF8 Example3") {
		const unsigned char s[]={0xE2, 0x82, 0xAC, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x20AC);
	}

	SECTION("UnicodeFromUTF8 Example4") {
		const unsigned char s[]={0xF0, 0x90, 0x8D, 0x88, 0};
		REQUIRE(UnicodeFromUTF8(s) == 0x10348);
	}

	SECTION("UnicodeFromUTF8 StringView") {
		const unsigned char s[]="\xF0\x90\x8D\x88";
		REQUIRE(UnicodeFromUTF8(s) == 0x10348);
	}

	// UTF16FromUTF8

	SECTION("UTF16FromUTF8 ASCII") {
		const char s[] = {'a', 0};
		wchar_t tbuf[1] = {0};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 'a');
		char back[4]{};
		UTF8FromUTF16(std::wstring_view(tbuf, tlen), back, sizeof(back));
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF16FromUTF8 Example1") {
		const char s[] = {'\x24', 0};
		wchar_t tbuf[1] = {0};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0x24);
		char back[4]{};
		UTF8FromUTF16(std::wstring_view(tbuf, tlen), back, sizeof(back));
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF16FromUTF8 Example2") {
		const char s[] = {'\xC2', '\xA2', 0};
		wchar_t tbuf[1] = {0};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0xA2);
		char back[4]{};
		UTF8FromUTF16(std::wstring_view(tbuf, tlen), back, sizeof(back));
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF16FromUTF8 Example3") {
		const char s[] = {'\xE2', '\x82', '\xAC', 0};
		wchar_t tbuf[1] = {0};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 1);;
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0x20AC);
		char back[4]{};
		UTF8FromUTF16(std::wstring_view(tbuf, tlen), back, sizeof(back));
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF16FromUTF8 Example4") {
		const char s[] = {'\xF0', '\x90', '\x8D', '\x88', 0};
		wchar_t tbuf[2] = {0, 0};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 2);
		REQUIRE(tlen == 2U);
		REQUIRE(tbuf[0] == 0xD800);
		REQUIRE(tbuf[1] == 0xDF48);
		char back[5]{};
		UTF8FromUTF16(std::wstring_view(tbuf, tlen), back, sizeof(back));
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF16FromUTF8 Invalid Trail byte in lead position") {
		const char s[] = "a\xB5yz";
		wchar_t tbuf[4] = {};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 4);
		REQUIRE(tlen == 4U);
		REQUIRE(tbuf[0] == 'a');
		REQUIRE(tbuf[1] == 0xB5);
		REQUIRE(tbuf[2] == 'y');
		REQUIRE(tbuf[3] == 'z');
		// Invalid so can't round trip
	}

	SECTION("UTF16FromUTF8 Invalid Lead byte at end") {
		const char s[] = "a\xC2";
		wchar_t tbuf[2] = {};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 2);
		REQUIRE(tlen == 2U);
		REQUIRE(tbuf[0] == 'a');
		REQUIRE(tbuf[1] == 0xC2);
		// Invalid so can't round trip
	}

	SECTION("UTF16FromUTF8 Invalid Lead byte implies 3 trails but only 2") {
		const char *s = "a\xF1yz";
		wchar_t tbuf[4] = {};
		const size_t tlen = UTF16FromUTF8(s, tbuf, 4);
		REQUIRE(tlen == 2U);
		REQUIRE(tbuf[0] == 'a');
		REQUIRE(tbuf[1] == 0xF1);
		// Invalid so can't round trip
	}

	// UTF32FromUTF8

	SECTION("UTF32FromUTF8 ASCII") {
		const char s[] = {'a', 0};
		unsigned int tbuf[1] = {0};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == static_cast<unsigned int>('a'));
		char back[5]{};
		UTF8FromUTF32Character(tbuf[0], back);
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF32FromUTF8 Example1") {
		const char s[] = {'\x24', 0};
		unsigned int tbuf[1] = {0};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0x24);
		char back[5]{};
		UTF8FromUTF32Character(tbuf[0], back);
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF32FromUTF8 Example2") {
		const char s[] = {'\xC2', '\xA2', 0};
		unsigned int tbuf[1] = {0};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0xA2);
		char back[5]{};
		UTF8FromUTF32Character(tbuf[0], back);
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF32FromUTF8 Example3") {
		const char s[] = {'\xE2', '\x82', '\xAC', 0};
		unsigned int tbuf[1] = {0};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0x20AC);
		char back[5]{};
		UTF8FromUTF32Character(tbuf[0], back);
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF32FromUTF8 Example4") {
		const char s[] = {'\xF0', '\x90', '\x8D', '\x88', 0};
		unsigned int tbuf[1] = {0};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 1);
		REQUIRE(tlen == 1U);
		REQUIRE(tbuf[0] == 0x10348);
		char back[5]{};
		UTF8FromUTF32Character(tbuf[0], back);
		REQUIRE(strcmp(s, back) == 0);
	}

	SECTION("UTF32FromUTF8 Invalid Trail byte in lead position") {
		const char s[] = "a\xB5yz";
		unsigned int tbuf[4] = {};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 4);
		REQUIRE(tlen == 4U);
		REQUIRE(tbuf[0] == static_cast<unsigned int>('a'));
		REQUIRE(tbuf[1] == 0xB5);
		REQUIRE(tbuf[2] == static_cast<unsigned int>('y'));
		REQUIRE(tbuf[3] == static_cast<unsigned int>('z'));
	}

	SECTION("UTF32FromUTF8 Invalid Lead byte at end") {
		const char s[] = "a\xC2";
		unsigned int tbuf[2] = {};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 2);
		REQUIRE(tlen == 2U);
		REQUIRE(tbuf[0] == static_cast<unsigned int>('a'));
		REQUIRE(tbuf[1] == 0xC2);
	}

	SECTION("UTF32FromUTF8 Invalid Lead byte implies 3 trails but only 2") {
		const char *s = "a\xF1yz";
		unsigned int tbuf[4] = {};
		const size_t tlen = UTF32FromUTF8(s, tbuf, 4);
		REQUIRE(tlen == 2U);
		REQUIRE(tbuf[0] == static_cast<unsigned int>('a'));
		REQUIRE(tbuf[1] == 0xF1);
	}
}

namespace {

// Simple adapter to avoid casting
int UTFClass(std::string_view sv) noexcept {
	return UTF8Classify(sv);
}

}

TEST_CASE("UTF8Classify") {

	// These tests are supposed to hit every return statement in UTF8Classify in order
	// with some hit multiple times.

	// Single byte

	SECTION("UTF8Classify Simple ASCII") {
		REQUIRE(UTFClass("a") == 1);
	}
	SECTION("UTF8Classify Invalid Too large lead") {
		REQUIRE(UTFClass("\xF5") == (1|UTF8MaskInvalid));
	}
	SECTION("UTF8Classify Overlong") {
		REQUIRE(UTFClass("\xC0\x80") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify single trail byte") {
		REQUIRE(UTFClass("\x80") == (1 | UTF8MaskInvalid));
	}

	// Invalid length tests

	SECTION("UTF8Classify 2 byte lead, string less than 2 long") {
		REQUIRE(UTFClass("\xD0") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 3 byte lead, string less than 3 long") {
		REQUIRE(UTFClass("\xEF") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte lead, string less than 4 long") {
		REQUIRE(UTFClass("\xF0") == (1 | UTF8MaskInvalid));
	}

	// Invalid first trail byte tests

	SECTION("UTF8Classify 2 byte lead trail is invalid") {
		REQUIRE(UTFClass("\xD0q") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 3 byte lead invalid trails") {
		REQUIRE(UTFClass("\xE2qq") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte bad trails") {
		REQUIRE(UTFClass("\xF0xyz") == (1 | UTF8MaskInvalid));
	}

	// 2 byte lead

	SECTION("UTF8Classify 2 byte valid character") {
		REQUIRE(UTFClass("\xD0\x80") == 2);
	}

	// 3 byte lead

	SECTION("UTF8Classify 3 byte lead, overlong") {
		REQUIRE(UTFClass("\xE0\x80\xAF") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 3 byte lead, surrogate") {
		REQUIRE(UTFClass("\xED\xA0\x80") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify FFFE non-character") {
		REQUIRE(UTFClass("\xEF\xBF\xBE") == (3 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify FFFF non-character") {
		REQUIRE(UTFClass("\xEF\xBF\xBF") == (3 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify FDD0 non-character") {
		REQUIRE(UTFClass("\xEF\xB7\x90") == (3 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 3 byte valid character") {
		REQUIRE(UTFClass("\xE2\x82\xAC") == 3);
	}

	// 4 byte lead

	SECTION("UTF8Classify 1FFFF non-character") {
		REQUIRE(UTFClass("\xF0\x9F\xBF\xBF") == (4 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 1 Greater than max Unicode 110000") {
		// Maximum Unicode value is 10FFFF so 110000 is out of range
		REQUIRE(UTFClass("\xF4\x90\x80\x80") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte overlong") {
		REQUIRE(UTFClass("\xF0\x80\x80\x80") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte valid character") {
		REQUIRE(UTFClass("\xF0\x9F\x8C\x90") == 4);
	}

	// Invalid 2nd or 3rd continuation bytes
	SECTION("UTF8Classify 3 byte lead invalid 2nd trail") {
		REQUIRE(UTFClass("\xE2\x82q") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte lead invalid 2nd trail") {
		REQUIRE(UTFClass("\xF0\x9Fq\x9F") == (1 | UTF8MaskInvalid));
	}
	SECTION("UTF8Classify 4 byte lead invalid 3rd trail") {
		REQUIRE(UTFClass("\xF0\x9F\x9Fq") == (1 | UTF8MaskInvalid));
	}
}
