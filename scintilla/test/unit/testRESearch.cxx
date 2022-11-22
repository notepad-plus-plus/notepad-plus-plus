/** @file testRESearch.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"

#include "Debugging.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "RESearch.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

class StringCI : public CharacterIndexer {
	std::string s;
public:
	StringCI(std::string_view sv_) : s(sv_) {
	}
	Sci::Position Length() const noexcept {
		return s.length();
	}
	char CharAt(Sci::Position index) const override {
		return s.at(index);
	}
};

// Test RESearch.

TEST_CASE("RESearch") {

	CharClassify cc;
	const char sTextSpace[] = "Scintilla ";
	const char pattern[] = "[a-z]+";

	SECTION("Compile") {
		std::unique_ptr<RESearch> re = std::make_unique<RESearch>(&cc);
		const char *msg = re->Compile(pattern, strlen(pattern), true, false);
		REQUIRE(nullptr == msg);
	}

	SECTION("Execute") {
		std::unique_ptr<RESearch> re = std::make_unique<RESearch>(&cc);
		re->Compile(pattern, strlen(pattern), true, false);
		StringCI sci(sTextSpace);
		const int x = re->Execute(sci, 0, sci.Length());
		REQUIRE(x == 1);
		REQUIRE(re->bopat[0] == 1);
		REQUIRE(re->eopat[0] == sci.Length() - 1);
	}

	SECTION("Grab") {
		std::unique_ptr<RESearch> re = std::make_unique<RESearch>(&cc);
		re->Compile(pattern, strlen(pattern), true, false);
		StringCI sci(sTextSpace);
		re->Execute(sci, 0, sci.Length());
		re->GrabMatches(sci);
		REQUIRE(re->pat[0] == "cintilla");
	}

}
