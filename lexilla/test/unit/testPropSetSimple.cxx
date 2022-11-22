/** @file testPropSetSimple.cxx
 ** Unit Tests for Lexilla internal data structures
 **/

#include <cassert>
#include <cstring>

#include <string_view>
#include <iostream>

#include "PropSetSimple.h"

#include "catch.hpp"

using namespace Lexilla;

// Test PropSetSimple.

constexpr const char *propertyName = "lexer.tex.comment.process";
constexpr const char *propertyValue = "1";

TEST_CASE("PropSetSimple") {

	SECTION("IsEmptyInitially") {
		PropSetSimple pss;
		const char *value = pss.Get(propertyName);
		REQUIRE(strcmp("", value) == 0);
	}

	SECTION("SetAndGet") {
		PropSetSimple pss;
		pss.Set(propertyName, propertyValue);
		const char *value = pss.Get(propertyName);
		REQUIRE(strcmp(propertyValue, value) == 0);
	}

	SECTION("GetInt") {
		PropSetSimple pss;
		const int valueStart = pss.GetInt(propertyName);
		REQUIRE(0 == valueStart);
		const int valueDefault = pss.GetInt(propertyName, 3);
		REQUIRE(3 == valueDefault);
		pss.Set(propertyName, propertyValue);
		const int value = pss.GetInt(propertyName);
		REQUIRE(1 == value);
	}

}
