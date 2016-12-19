// Unit Tests for Scintilla internal data structures

#include <string.h>

#include <algorithm>

#include "Platform.h"

#include "SplitVector.h"

#include "catch.hpp"

// Test SplitVector.

const int lengthTestArray = 4;
static const int testArray[4] = {3, 4, 5, 6};

TEST_CASE("SplitVector") {

	SplitVector<int> sv;

	SECTION("IsEmptyInitially") {
		REQUIRE(0 == sv.Length());
	}

	SECTION("InsertOne") {
		sv.InsertValue(0, 10, 0);
		sv.Insert(5, 3);
		REQUIRE(11 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i == 5) ? 3 : 0) == sv.ValueAt(i));
		}
	}

	SECTION("Insertion") {
		sv.InsertValue(0, 10, 0);
		REQUIRE(10 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(0 == sv.ValueAt(i));
		}
	}

	SECTION("EnsureLength") {
		sv.EnsureLength(4);
		REQUIRE(4 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(0 == sv.ValueAt(i));
		}
	}

	SECTION("InsertFromArray") {
		sv.InsertFromArray(0, testArray, 0, lengthTestArray);
		REQUIRE(lengthTestArray == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE((i+3) == sv.ValueAt(i));
		}
	}

	SECTION("SetValue") {
		sv.InsertValue(0, 10, 0);
		sv.SetValueAt(5, 3);
		REQUIRE(10 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i == 5) ? 3 : 0) == sv.ValueAt(i));
		}
		// Move the gap
		sv.InsertValue(7, 1, 17);
		REQUIRE(17 == sv.ValueAt(7));
		REQUIRE(0 == sv.ValueAt(8));
		// Set after the gap
		sv.SetValueAt(8, 19);
		REQUIRE(19 == sv.ValueAt(8));
	}

	SECTION("Indexing") {
		sv.InsertValue(0, 10, 0);
		sv.SetValueAt(5, 3);
		REQUIRE(10 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i == 5) ? 3 : 0) == sv[i]);
		}
}

	SECTION("Fill") {
		sv.InsertValue(0, 10, 0);
		REQUIRE(10 == sv.Length());
		sv.InsertValue(7, 1, 1);
		REQUIRE(11 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i == 7) ? 1 : 0) == sv.ValueAt(i));
		}
	}

	SECTION("DeleteOne") {
		sv.InsertFromArray(0, testArray, 0, lengthTestArray);
		sv.Delete(2);
		REQUIRE((lengthTestArray-1) == sv.Length());
		REQUIRE(3 == sv[0]);
		REQUIRE(4 == sv[1]);
		REQUIRE(6 == sv[2]);
	}

	SECTION("DeleteRange") {
		sv.InsertValue(0, 10, 0);
		REQUIRE(10 == sv.Length());
		sv.InsertValue(7, 1, 1);
		REQUIRE(11 == sv.Length());
		sv.DeleteRange(2, 3);
		REQUIRE(8 == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i == 4) ? 1 : 0) == sv.ValueAt(i));
		}
	}

	SECTION("DeleteAll") {
		sv.InsertValue(0, 10, 0);
		sv.InsertValue(7, 1, 1);
		sv.DeleteRange(2, 3);
		sv.DeleteAll();
		REQUIRE(0 == sv.Length());
	}

	SECTION("GetRange") {
		sv.InsertValue(0, 10, 0);
		sv.InsertValue(7, 1, 1);
		int retrieveArray[11] = {0};
		sv.GetRange(retrieveArray, 0, 11);
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE(((i==7) ? 1 : 0) == retrieveArray[i]);
		}
	}

	SECTION("GetRangeOverGap") {
		sv.InsertFromArray(0, testArray, 0, lengthTestArray);
		REQUIRE(lengthTestArray == sv.Length());
		int retrieveArray[lengthTestArray] = {0};
		sv.GetRange(retrieveArray, 0, lengthTestArray);
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE((i+3) == retrieveArray[i]);
		}
	}

	SECTION("ReplaceUp") {
		// Replace each element by inserting and then deleting the displaced element
		// This should perform many moves
		const int testLength=105;
		sv.EnsureLength(testLength);
		for (int i=0; i<testLength; i++)
			sv.SetValueAt(i, i+2);
		REQUIRE(testLength == sv.Length());
		for (int i=0; i<sv.Length(); i++) {
			sv.InsertValue(i, 1, i+9);
			sv.Delete(i+1);
		}
		for (int i=0; i<sv.Length(); i++)
			REQUIRE((i+9) == sv.ValueAt(i));
	}

	SECTION("ReplaceDown") {
		// From the end, replace each element by inserting and then deleting the displaced element
		// This should perform many moves
		const int testLength=24;
		sv.EnsureLength(testLength);
		for (int i=0; i<testLength; i++)
			sv.SetValueAt(i, i+12);
		REQUIRE(testLength == sv.Length());
		for (int i=sv.Length()-1; i>=0; i--) {
			sv.InsertValue(i, 1, i+5);
			sv.Delete(i+1);
		}
		for (int i=0; i<sv.Length(); i++)
			REQUIRE((i+5) == sv.ValueAt(i));
	}

	SECTION("BufferPointer") {
		sv.InsertFromArray(0, testArray, 0, lengthTestArray);
		int *retrievePointer = sv.BufferPointer();
		for (int i=0; i<sv.Length(); i++) {
			REQUIRE((i+3) == retrievePointer[i]);
		}
	}

	SECTION("DeleteBackAndForth") {
		sv.InsertValue(0, 10, 87);
		for (int i=0; i<10; i+=2) {
			int len = 10 - i;
			REQUIRE(len == sv.Length());
			for (int i=0; i<sv.Length(); i++) {
				REQUIRE(87 == sv.ValueAt(i));
			}
			sv.Delete(len-1);
			sv.Delete(0);
		}
	}

	SECTION("GrowSize") {
		sv.SetGrowSize(5);
		REQUIRE(5 == sv.GetGrowSize());
	}

	SECTION("OutsideBounds") {
		sv.InsertValue(0, 10, 87);
		REQUIRE(0 == sv.ValueAt(-1));
		REQUIRE(0 == sv.ValueAt(10));

		/* Could be a death test as this asserts:
		sv.SetValueAt(-1,98);
		sv.SetValueAt(10,99);
		REQUIRE(0 == sv.ValueAt(-1));
		REQUIRE(0 == sv.ValueAt(10));
		*/
	}

}
