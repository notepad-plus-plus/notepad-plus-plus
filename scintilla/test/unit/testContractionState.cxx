/** @file testContractionState.cxx
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

#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

// Test ContractionState.

TEST_CASE("ContractionState") {

	std::unique_ptr<IContractionState> pcs = ContractionStateCreate(false);

	SECTION("IsEmptyInitially") {
		REQUIRE(1 == pcs->LinesInDoc());
		REQUIRE(1 == pcs->LinesDisplayed());
		REQUIRE(0 == pcs->DisplayFromDoc(0));
		REQUIRE(0 == pcs->DocFromDisplay(0));
	}

	SECTION("OneLine") {
		pcs->InsertLines(0, 1);
		REQUIRE(2 == pcs->LinesInDoc());
		REQUIRE(2 == pcs->LinesDisplayed());
		REQUIRE(0 == pcs->DisplayFromDoc(0));
		REQUIRE(0 == pcs->DocFromDisplay(0));
		REQUIRE(1 == pcs->DisplayFromDoc(1));
		REQUIRE(1 == pcs->DocFromDisplay(1));
	}

	SECTION("InsertionThenDeletions") {
		pcs->InsertLines(0,4);
		pcs->DeleteLines(1, 1);

		REQUIRE(4 == pcs->LinesInDoc());
		REQUIRE(4 == pcs->LinesDisplayed());
		for (int l=0;l<4;l++) {
			REQUIRE(l == pcs->DisplayFromDoc(l));
			REQUIRE(l == pcs->DocFromDisplay(l));
		}

		pcs->DeleteLines(0,2);
		REQUIRE(2 == pcs->LinesInDoc());
		REQUIRE(2 == pcs->LinesDisplayed());
		for (int l=0;l<2;l++) {
			REQUIRE(l == pcs->DisplayFromDoc(l));
			REQUIRE(l == pcs->DocFromDisplay(l));
		}
	}

	SECTION("ShowHide") {
		pcs->InsertLines(0,4);
		REQUIRE(true == pcs->GetVisible(0));
		REQUIRE(true == pcs->GetVisible(1));
		REQUIRE(true == pcs->GetVisible(2));
		REQUIRE(5 == pcs->LinesDisplayed());

		pcs->SetVisible(1, 1, false);
		REQUIRE(true == pcs->GetVisible(0));
		REQUIRE(false == pcs->GetVisible(1));
		REQUIRE(true == pcs->GetVisible(2));
		REQUIRE(4 == pcs->LinesDisplayed());
		REQUIRE(true == pcs->HiddenLines());

		pcs->SetVisible(1, 2, true);
		for (int l=0;l<4;l++) {
			REQUIRE(true == pcs->GetVisible(0));
		}

		pcs->SetVisible(1, 1, false);
		REQUIRE(false == pcs->GetVisible(1));
		pcs->ShowAll();
		for (int l=0;l<4;l++) {
			REQUIRE(true == pcs->GetVisible(0));
		}
		REQUIRE(false == pcs->HiddenLines());
	}

	SECTION("Hidden") {
		pcs->InsertLines(0,1);
		for (int l=0;l<2;l++) {
			REQUIRE(true == pcs->GetVisible(0));
		}
		REQUIRE(false == pcs->HiddenLines());

		pcs->SetVisible(1, 1, false);
		REQUIRE(true == pcs->GetVisible(0));
		REQUIRE(false == pcs->GetVisible(1));
		REQUIRE(true == pcs->HiddenLines());
		REQUIRE(1 == pcs->LinesDisplayed());

		pcs->SetVisible(1, 1, true);
		for (int l=0;l<2;l++) {
			REQUIRE(true == pcs->GetVisible(0));
		}
		REQUIRE(false == pcs->HiddenLines());
	}

	SECTION("Hide All") {
		pcs->InsertLines(0,1);
		for (int l=0;l<2;l++) {
			REQUIRE(true == pcs->GetVisible(0));
		}
		REQUIRE(false == pcs->HiddenLines());

		pcs->SetVisible(0, 1, false);
		REQUIRE(false == pcs->GetVisible(0));
		REQUIRE(false == pcs->GetVisible(1));
		REQUIRE(true == pcs->HiddenLines());
		REQUIRE(0 == pcs->LinesDisplayed());
	}

	SECTION("Contracting") {
		pcs->InsertLines(0,4);
		for (int l=0;l<4;l++) {
			REQUIRE(true == pcs->GetExpanded(l));
		}

		pcs->SetExpanded(2, false);
		REQUIRE(true == pcs->GetExpanded(1));
		REQUIRE(false == pcs->GetExpanded(2));
		REQUIRE(true == pcs->GetExpanded(3));

		REQUIRE(2 == pcs->ContractedNext(0));
		REQUIRE(2 == pcs->ContractedNext(1));
		REQUIRE(2 == pcs->ContractedNext(2));
		REQUIRE(-1 == pcs->ContractedNext(3));

		pcs->SetExpanded(2, true);
		REQUIRE(true == pcs->GetExpanded(1));
		REQUIRE(true == pcs->GetExpanded(2));
		REQUIRE(true == pcs->GetExpanded(3));
	}

	SECTION("ExpandAll") {
		pcs->InsertLines(0,4);
		for (int l=0;l<4;l++) {
			REQUIRE(true == pcs->GetExpanded(l));
		}

		pcs->SetExpanded(2, false);
		REQUIRE(true == pcs->GetExpanded(1));
		REQUIRE(false == pcs->GetExpanded(2));
		REQUIRE(true == pcs->GetExpanded(3));

		pcs->SetExpanded(1, false);
		REQUIRE(false == pcs->GetExpanded(1));
		REQUIRE(false == pcs->GetExpanded(2));
		REQUIRE(true == pcs->GetExpanded(3));

		REQUIRE(true == pcs->ExpandAll());
		REQUIRE(true == pcs->GetExpanded(1));
		REQUIRE(true == pcs->GetExpanded(2));
		REQUIRE(true == pcs->GetExpanded(3));
	}

	SECTION("ChangeHeight") {
		pcs->InsertLines(0,4);
		for (int l=0;l<4;l++) {
			REQUIRE(1 == pcs->GetHeight(l));
		}

		pcs->SetHeight(1, 2);
		REQUIRE(1 == pcs->GetHeight(0));
		REQUIRE(2 == pcs->GetHeight(1));
		REQUIRE(1 == pcs->GetHeight(2));
	}

	SECTION("SetFoldDisplayText") {
		pcs->InsertLines(0, 4);
		REQUIRE(5 == pcs->LinesInDoc());
		pcs->SetFoldDisplayText(1, "abc");
		REQUIRE(strcmp(pcs->GetFoldDisplayText(1), "abc") == 0);
		pcs->SetFoldDisplayText(1, "def");
		REQUIRE(strcmp(pcs->GetFoldDisplayText(1), "def") == 0);
		pcs->SetFoldDisplayText(1, nullptr);
		REQUIRE(static_cast<const char *>(nullptr) == pcs->GetFoldDisplayText(1));
		// At end
		pcs->SetFoldDisplayText(5, "xyz");
		REQUIRE(strcmp(pcs->GetFoldDisplayText(5), "xyz") == 0);
		pcs->DeleteLines(4, 1);
		REQUIRE(strcmp(pcs->GetFoldDisplayText(4), "xyz") == 0);
	}

}
