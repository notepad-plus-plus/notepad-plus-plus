/** @file testSplitVector.cxx
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
#include "SplitVector.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

// Test SplitVector.

struct StringSetHolder {
	SplitVector<std::string> sa;
	bool Check() const noexcept {
		for (int i = 0; i < sa.Length(); i++) {
			if (sa[i].empty()) {
				return false;
			}
		}
		return true;
	}
};

constexpr int lengthTestArray = 4;
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

	SECTION("InsertionString") {
		// This test failed an earlier version of SplitVector that copied backwards incorrectly
		StringSetHolder ssh;
		ssh.sa.Insert(0, "Alpha");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Beta");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Cat");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Dog");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Elephant");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Fox");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Grass");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Hat");
		REQUIRE(ssh.Check());
		ssh.sa.Delete(4);
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Indigo");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Jackal");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Kanga");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Lion");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(0, "Mango");
		REQUIRE(ssh.Check());
		ssh.sa.Insert(1, "Neon");
		REQUIRE(ssh.Check());
	}

	SECTION("InsertionPattern") {
		sv.Insert(0, 1);	// 1
		sv.Insert(0, 2);	// 21
		sv.Insert(0, 3);	// 321
		sv.Insert(1, 4);	// 3421
		sv.Insert(0, 5);	// 53421
		sv.Insert(1, 6);	// 563421
		sv.Insert(0, 7);	// 7563421
		sv.Insert(1, 8);	// 78563421

		REQUIRE(8 == sv.Length());

		REQUIRE(7 == sv.ValueAt(0));
		REQUIRE(8 == sv.ValueAt(1));
		REQUIRE(5 == sv.ValueAt(2));
		REQUIRE(6 == sv.ValueAt(3));
		REQUIRE(3 == sv.ValueAt(4));
		REQUIRE(4 == sv.ValueAt(5));
		REQUIRE(2 == sv.ValueAt(6));
		REQUIRE(1 == sv.ValueAt(7));

		sv.Delete(4);	// 7856421

		REQUIRE(7 == sv.Length());

		REQUIRE(7 == sv.ValueAt(0));
		REQUIRE(8 == sv.ValueAt(1));
		REQUIRE(5 == sv.ValueAt(2));
		REQUIRE(6 == sv.ValueAt(3));
		REQUIRE(4 == sv.ValueAt(4));
		REQUIRE(2 == sv.ValueAt(5));
		REQUIRE(1 == sv.ValueAt(6));

		sv.Insert(0, 9);		// 97856421
		sv.Insert(1, 0xa);	// 9a7856421
		sv.Insert(0, 0xb);	// b9a7856421
		sv.Insert(1, 0xc);	// bc9a7856421
		sv.Insert(0, 0xd);	// dbc9a7856421
		sv.Insert(1, 0xe);	// debc9a7856421

		REQUIRE(13 == sv.Length());

		REQUIRE(0xd == sv.ValueAt(0));
		REQUIRE(0xe == sv.ValueAt(1));
		REQUIRE(0xb == sv.ValueAt(2));
		REQUIRE(0xc == sv.ValueAt(3));
		REQUIRE(9 == sv.ValueAt(4));
		REQUIRE(0xa == sv.ValueAt(5));
		REQUIRE(7 == sv.ValueAt(6));
		REQUIRE(8 == sv.ValueAt(7));
		REQUIRE(5 == sv.ValueAt(8));
		REQUIRE(6 == sv.ValueAt(9));
		REQUIRE(4 == sv.ValueAt(10));
		REQUIRE(2 == sv.ValueAt(11));
		REQUIRE(1 == sv.ValueAt(12));
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

	SECTION("InsertEmpty") {
		sv.InsertEmpty(0, 0);
		REQUIRE(0 == sv.Length());
		int *pi = sv.InsertEmpty(0, 2);
		REQUIRE(2 == sv.Length());
		REQUIRE(0 == sv.ValueAt(0));
		REQUIRE(0 == sv.ValueAt(1));
		pi[0] = 4;
		pi[1] = 5;
		REQUIRE(4 == sv.ValueAt(0));
		REQUIRE(5 == sv.ValueAt(1));
		pi = sv.InsertEmpty(1, 2);
		pi[0] = 6;
		pi[1] = 7;
		REQUIRE(4 == sv.Length());
		REQUIRE(4 == sv.ValueAt(0));
		REQUIRE(6 == sv.ValueAt(1));
		REQUIRE(7 == sv.ValueAt(2));
		REQUIRE(5 == sv.ValueAt(3));
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
		constexpr int testLength=105;
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
		constexpr int testLength=24;
		sv.EnsureLength(testLength);
		for (int i=0; i<testLength; i++)
			sv.SetValueAt(i, i+12);
		REQUIRE(testLength == sv.Length());
		for (ptrdiff_t i=sv.Length()-1; i>=0; i--) {
			sv.InsertValue(i, 1, static_cast<int>(i+5));
			sv.Delete(i+1);
		}
		for (int i=0; i<sv.Length(); i++)
			REQUIRE((i+5) == sv.ValueAt(i));
	}

	SECTION("BufferPointer") {
		// Low-level access to the data
		sv.InsertFromArray(0, testArray, 0, lengthTestArray);
		sv.Insert(0, 99);	// This moves the gap so that BufferPointer() must also move
		REQUIRE(1 == sv.GapPosition());
		constexpr int lengthAfterInsertion = 1 + lengthTestArray;
		REQUIRE(lengthAfterInsertion == (sv.Length()));
		const int *retrievePointer = sv.BufferPointer();
		for (int i=1; i<sv.Length(); i++) {
			REQUIRE((i+3-1) == retrievePointer[i]);
		}
		REQUIRE(lengthAfterInsertion == sv.Length());
		// Gap was moved to end.
		REQUIRE(lengthAfterInsertion == sv.GapPosition());
	}

	SECTION("DeleteBackAndForth") {
		sv.InsertValue(0, 10, 87);
		for (int i=0; i<10; i+=2) {
			int len = 10 - i;
			REQUIRE(len == sv.Length());
			for (int j=0; j<sv.Length(); j++) {
				REQUIRE(87 == sv.ValueAt(j));
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
