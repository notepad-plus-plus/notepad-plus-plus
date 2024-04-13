/** @file testCharacterCategoryMap.cxx
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

#include "Debugging.h"

#include "CharacterCategoryMap.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test CharacterCategoryMap.

TEST_CASE("CharacterCategoryMap") {

	const CharacterCategoryMap ccm;

	SECTION("LowerCaseLetter") {
		const CharacterCategory cc = ccm.CategoryFor('a');
		REQUIRE(cc == CharacterCategory::ccLl);
	}

	SECTION("All") {
		REQUIRE(ccm.CategoryFor('A') == CharacterCategory::ccLu);
		REQUIRE(ccm.CategoryFor('a') == CharacterCategory::ccLl);
		REQUIRE(ccm.CategoryFor(0x01C5) == CharacterCategory::ccLt);
		REQUIRE(ccm.CategoryFor(0x0E46) == CharacterCategory::ccLm);
		REQUIRE(ccm.CategoryFor(0x4E00) == CharacterCategory::ccLo);

		REQUIRE(ccm.CategoryFor(0x0300) == CharacterCategory::ccMn);
		REQUIRE(ccm.CategoryFor(0x0903) == CharacterCategory::ccMc);
		REQUIRE(ccm.CategoryFor(0x20E0) == CharacterCategory::ccMe);

		REQUIRE(ccm.CategoryFor('7') == CharacterCategory::ccNd);
		REQUIRE(ccm.CategoryFor(0x2160) == CharacterCategory::ccNl);
		REQUIRE(ccm.CategoryFor(0x00BC) == CharacterCategory::ccNo);

		REQUIRE(ccm.CategoryFor('_') == CharacterCategory::ccPc);
		REQUIRE(ccm.CategoryFor('-') == CharacterCategory::ccPd);
		REQUIRE(ccm.CategoryFor('(') == CharacterCategory::ccPs);
		REQUIRE(ccm.CategoryFor('}') == CharacterCategory::ccPe);
		REQUIRE(ccm.CategoryFor(0x00AB) == CharacterCategory::ccPi);
		REQUIRE(ccm.CategoryFor(0x00BB) == CharacterCategory::ccPf);
		REQUIRE(ccm.CategoryFor('"') == CharacterCategory::ccPo);

		REQUIRE(ccm.CategoryFor('+') == CharacterCategory::ccSm);
		REQUIRE(ccm.CategoryFor('$') == CharacterCategory::ccSc);
		REQUIRE(ccm.CategoryFor(0x02C2) == CharacterCategory::ccSk);
		REQUIRE(ccm.CategoryFor(0x00A6) == CharacterCategory::ccSo);

		REQUIRE(ccm.CategoryFor(' ') == CharacterCategory::ccZs);
		REQUIRE(ccm.CategoryFor(0x2028) == CharacterCategory::ccZl);
		REQUIRE(ccm.CategoryFor(0x2029) == CharacterCategory::ccZp);

		REQUIRE(ccm.CategoryFor('\n') == CharacterCategory::ccCc);
		REQUIRE(ccm.CategoryFor(0x00AD) == CharacterCategory::ccCf);
		REQUIRE(ccm.CategoryFor(0xD800) == CharacterCategory::ccCs);
		REQUIRE(ccm.CategoryFor(0xE000) == CharacterCategory::ccCo);
		REQUIRE(ccm.CategoryFor(0xFFFE) == CharacterCategory::ccCn);
	}

}
