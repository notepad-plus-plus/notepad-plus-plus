// Unit Tests for Scintilla internal data structures

#include <string.h>

#include <algorithm>

#include "Platform.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"

#include "catch.hpp"

// Test ContractionState.

TEST_CASE("ContractionState") {

	ContractionState cs;

	SECTION("IsEmptyInitially") {
		REQUIRE(1 == cs.LinesInDoc());
		REQUIRE(1 == cs.LinesDisplayed());
		REQUIRE(0 == cs.DisplayFromDoc(0));
		REQUIRE(0 == cs.DocFromDisplay(0));
	}

	SECTION("OneLine") {
		cs.InsertLine(0);
		REQUIRE(2 == cs.LinesInDoc());
		REQUIRE(2 == cs.LinesDisplayed());
		REQUIRE(0 == cs.DisplayFromDoc(0));
		REQUIRE(0 == cs.DocFromDisplay(0));
		REQUIRE(1 == cs.DisplayFromDoc(1));
		REQUIRE(1 == cs.DocFromDisplay(1));
	}

	SECTION("InsertionThenDeletions") {
		cs.InsertLines(0,4);
		cs.DeleteLine(1);

		REQUIRE(4 == cs.LinesInDoc());
		REQUIRE(4 == cs.LinesDisplayed());
		for (int l=0;l<4;l++) {
			REQUIRE(l == cs.DisplayFromDoc(l));
			REQUIRE(l == cs.DocFromDisplay(l));
		}

		cs.DeleteLines(0,2);
		REQUIRE(2 == cs.LinesInDoc());
		REQUIRE(2 == cs.LinesDisplayed());
		for (int l=0;l<2;l++) {
			REQUIRE(l == cs.DisplayFromDoc(l));
			REQUIRE(l == cs.DocFromDisplay(l));
		}
	}

	SECTION("ShowHide") {
		cs.InsertLines(0,4);
		REQUIRE(true == cs.GetVisible(0));
		REQUIRE(true == cs.GetVisible(1));
		REQUIRE(true == cs.GetVisible(2));
		REQUIRE(5 == cs.LinesDisplayed());

		cs.SetVisible(1, 1, false);
		REQUIRE(true == cs.GetVisible(0));
		REQUIRE(0 == cs.GetVisible(1));
		REQUIRE(true == cs.GetVisible(2));
		REQUIRE(4 == cs.LinesDisplayed());
		REQUIRE(1 == cs.HiddenLines());

		cs.SetVisible(1, 2, true);
		for (int l=0;l<4;l++) {
			REQUIRE(true == cs.GetVisible(0));
		}

		cs.SetVisible(1, 1, false);
		REQUIRE(0 == cs.GetVisible(1));
		cs.ShowAll();
		for (int l=0;l<4;l++) {
			REQUIRE(true == cs.GetVisible(0));
		}
		REQUIRE(0 == cs.HiddenLines());
	}

	SECTION("Hidden") {
		cs.InsertLines(0,1);
		for (int l=0;l<2;l++) {
			REQUIRE(true == cs.GetVisible(0));
		}
		REQUIRE(0 == cs.HiddenLines());

		cs.SetVisible(1, 1, false);
		REQUIRE(true == cs.GetVisible(0));
		REQUIRE(0 == cs.GetVisible(1));
		REQUIRE(1 == cs.HiddenLines());

		cs.SetVisible(1, 1, true);
		for (int l=0;l<2;l++) {
			REQUIRE(true == cs.GetVisible(0));
		}
		REQUIRE(0 == cs.HiddenLines());
	}

	SECTION("Contracting") {
		cs.InsertLines(0,4);
		for (int l=0;l<4;l++) {
			REQUIRE(true == cs.GetExpanded(l));
		}

		cs.SetExpanded(2, false);
		REQUIRE(true == cs.GetExpanded(1));
		REQUIRE(0 == cs.GetExpanded(2));
		REQUIRE(true == cs.GetExpanded(3));

		REQUIRE(2 == cs.ContractedNext(0));
		REQUIRE(2 == cs.ContractedNext(1));
		REQUIRE(2 == cs.ContractedNext(2));
		REQUIRE(-1 == cs.ContractedNext(3));

		cs.SetExpanded(2, true);
		REQUIRE(true == cs.GetExpanded(1));
		REQUIRE(true == cs.GetExpanded(2));
		REQUIRE(true == cs.GetExpanded(3));
	}

	SECTION("ChangeHeight") {
		cs.InsertLines(0,4);
		for (int l=0;l<4;l++) {
			REQUIRE(1 == cs.GetHeight(l));
		}

		cs.SetHeight(1, 2);
		REQUIRE(1 == cs.GetHeight(0));
		REQUIRE(2 == cs.GetHeight(1));
		REQUIRE(1 == cs.GetHeight(2));
	}

}
