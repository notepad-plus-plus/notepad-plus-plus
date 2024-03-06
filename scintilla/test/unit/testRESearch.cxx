/** @file testRESearch.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <array>
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
	explicit StringCI(std::string_view sv_) : s(sv_) {
	}
	virtual ~StringCI() = default;
	[[nodiscard]] Sci::Position Length() const noexcept {
		return s.length();
	}
	char CharAt(Sci::Position index) const override {
		return s.at(index);
	}
	Sci::Position MovePositionOutsideChar(Sci::Position pos, [[maybe_unused]] Sci::Position moveDir) const noexcept override {
		return pos;
	}
	[[nodiscard]] std::string GetCharRange(Sci::Position position, Sci::Position lengthRetrieve) const {
		return s.substr(position, lengthRetrieve);
	}
};

// Test RESearch.

TEST_CASE("RESearch") {

	CharClassify cc;
	constexpr std::string_view sTextSpace = "Scintilla ";
	constexpr std::string_view pattern = "[a-z]+";

	SECTION("Compile") {
		RESearch re(&cc);
		const char *msg = re.Compile(pattern.data(), pattern.length(), true, false);
		REQUIRE(nullptr == msg);
	}

	SECTION("Bug2413") {
		// Check for https://sourceforge.net/p/scintilla/bugs/2413/
		RESearch re(&cc);
		constexpr std::string_view BOW = "\\<";
		constexpr std::string_view EOW = "\\>";
		const char *msg = re.Compile(BOW.data(), BOW.length(), true, false);
		REQUIRE(nullptr == msg);
		msg = re.Compile(EOW.data(), EOW.length(), true, false);
		REQUIRE(nullptr == msg);
	}

	SECTION("Execute") {
		RESearch re(&cc);
		re.Compile(pattern.data(), pattern.length(), true, false);
		const StringCI sci(sTextSpace);
		const int x = re.Execute(sci, 0, sci.Length());
		REQUIRE(x == 1);
		REQUIRE(re.bopat[0] == 1);
		REQUIRE(re.eopat[0] == sci.Length() - 1);
	}

	SECTION("Grab") {
		RESearch re(&cc);
		re.Compile(pattern.data(), pattern.length(), true, false);
		const StringCI sci(sTextSpace);
		re.Execute(sci, 0, sci.Length());
		const std::string pat = sci.GetCharRange(re.bopat[0], re.eopat[0] - re.bopat[0]);
		REQUIRE(pat == "cintilla");
	}

}
