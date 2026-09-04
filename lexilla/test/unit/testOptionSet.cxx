/** @file testOptionSet.cxx
 ** Unit Tests for Lexilla internal data structures
 ** Tests OptionSet.
 **/

#include <string>
#include <string_view>
#include <vector>
#include <map>

#include "Scintilla.h"

#include "OptionSet.h"

#include "catch.hpp"

using namespace Lexilla;

// Test OptionSet.

namespace {

// Simple example options structure with each type: string, bool, int
struct Options {
	std::string so;
	bool bo = false;
	int io = 0;
};

const char *const denseWordLists[] = {
	"Keywords 1",
	"Keywords 2",
	"Keywords 3",
	"Keywords 4",
	nullptr,
};

const char *const sparseWordLists[] = {
	"",
	"",
	"Keywords 1",
	"",
	"Keywords 2",
	nullptr,
};

}

using Catch::Matchers::Equals;

TEST_CASE("OptionSet") {

	OptionSet<Options> os;
	Options options;

	SECTION("IsEmptyInitially") {
		REQUIRE_THAT(os.PropertyNames(), Equals(""));
	}

	SECTION("MissingOption") {
		// Check for not present option
		REQUIRE_FALSE(os.PropertyGet("missing"));
		REQUIRE(SC_TYPE_BOOLEAN == os.PropertyType("missing"));
		REQUIRE_FALSE(os.PropertySet(&options, "missing", "1"));
	}

	SECTION("Define") {
		os.DefineProperty("string.option", &Options::so, "StringOption");
		REQUIRE_THAT(os.PropertyGet("string.option"), Equals(""));
		REQUIRE(SC_TYPE_STRING == os.PropertyType("string.option"));
		REQUIRE_THAT(os.DescribeProperty("string.option"), Equals("StringOption"));

		os.DefineProperty("bool.option", &Options::bo, "BoolOption");
		REQUIRE_THAT(os.PropertyGet("bool.option"), Equals(""));
		REQUIRE(SC_TYPE_BOOLEAN == os.PropertyType("bool.option"));
		REQUIRE_THAT(os.DescribeProperty("bool.option"), Equals("BoolOption"));

		os.DefineProperty("int.option", &Options::io, "IntOption");
		REQUIRE_THAT(os.PropertyGet("int.option"), Equals(""));
		REQUIRE(SC_TYPE_INTEGER == os.PropertyType("int.option"));
		REQUIRE_THAT(os.DescribeProperty("int.option"), Equals("IntOption"));

		// This is really a set and could be reordered but is currently in definition order
		REQUIRE_THAT(os.PropertyNames(), Equals("string.option\nbool.option\nint.option"));
	}

	SECTION("Set") {
		os.DefineProperty("string.option", &Options::so, "StringOption");
		REQUIRE_THAT(os.PropertyGet("string.option"), Equals(""));
		REQUIRE(os.PropertySet(&options, "string.option", "string"));
		REQUIRE_THAT(os.PropertyGet("string.option"), Equals("string"));
		// Setting to same as before returns false
		REQUIRE_FALSE(os.PropertySet(&options, "string.option", "string"));
		REQUIRE(os.PropertySet(&options, "string.option", "anotherString"));
		REQUIRE_THAT(os.PropertyGet("string.option"), Equals("anotherString"));

		os.DefineProperty("bool.option", &Options::so, "BoolOption");
		REQUIRE(os.PropertySet(&options, "bool.option", "1"));
		REQUIRE_THAT(os.PropertyGet("bool.option"), Equals("1"));
		// Setting to same as before returns false
		REQUIRE_FALSE(os.PropertySet(&options, "bool.option", "1"));
		REQUIRE(os.PropertySet(&options, "bool.option", "0"));

		os.DefineProperty("int.option", &Options::so, "IntOption");
		REQUIRE(os.PropertySet(&options, "int.option", "2"));
		REQUIRE_THAT(os.PropertyGet("int.option"), Equals("2"));
		// Setting to same as before returns false
		REQUIRE_FALSE(os.PropertySet(&options, "int.option", "2"));
		REQUIRE(os.PropertySet(&options, "int.option", "3"));
	}

	// WordListSets feature is really completely separate from options

	SECTION("WordListSets") {
		REQUIRE_THAT(os.DescribeWordListSets(), Equals(""));
		os.DefineWordListSets(denseWordLists);
		REQUIRE_THAT(os.DescribeWordListSets(),
			Equals("Keywords 1\nKeywords 2\nKeywords 3\nKeywords 4"));

		OptionSet<Options> os2;
		REQUIRE_THAT(os2.DescribeWordListSets(), Equals(""));
		os2.DefineWordListSets(sparseWordLists);
		REQUIRE_THAT(os2.DescribeWordListSets(),
			Equals("\n\nKeywords 1\n\nKeywords 2"));
	}
}
