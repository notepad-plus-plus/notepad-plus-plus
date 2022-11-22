/** @file testLexerSimple.cxx
 ** Unit Tests for Lexilla internal data structures
 **/

#include <cassert>
#include <cstring>

#include <string_view>
#include <iostream>

#include "ILexer.h"
#include "Scintilla.h"

#include "PropSetSimple.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "LexerSimple.h"

#include "catch.hpp"

using namespace Lexilla;

// Test LexerSimple.

constexpr const char *propertyName = "lexer.tex.comment.process";
constexpr const char *propertyValue = "1";

static void ColouriseDocument(Sci_PositionU, Sci_Position, int,	WordList *[], Accessor &) {
	// Do no styling
}

LexerModule lmSimpleExample(123456, ColouriseDocument, "simpleexample");

TEST_CASE("LexerNoExceptions") {

	SECTION("Identifier") {
		LexerSimple lexSimple(&lmSimpleExample);
		REQUIRE(lexSimple.GetIdentifier() == 123456);
	}

	SECTION("Identifier") {
		LexerSimple lexSimple(&lmSimpleExample);
		REQUIRE(strcmp(lexSimple.GetName(), "simpleexample") == 0);
	}

	SECTION("SetAndGet") {
		LexerSimple lexSimple(&lmSimpleExample);

		// New setting -> 0
		const Sci_Position pos0 = lexSimple.PropertySet(propertyName, "8");
		REQUIRE(pos0 == 0);
		// Changed setting -> 0
		const Sci_Position pos1 = lexSimple.PropertySet(propertyName, propertyValue);
		REQUIRE(pos1 == 0);
		// Same setting -> -1
		const Sci_Position pos2 = lexSimple.PropertySet(propertyName, propertyValue);
		REQUIRE(pos2 == -1);

		const char *value = lexSimple.PropertyGet(propertyName);
		REQUIRE(strcmp(propertyValue, value) == 0);
	}

}
