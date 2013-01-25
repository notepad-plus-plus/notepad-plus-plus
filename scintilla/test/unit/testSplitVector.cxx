// Unit Tests for Scintilla internal data structures

#include <string.h>

#include "Platform.h"

#include "SplitVector.h"

#include <gtest/gtest.h>

// Test SplitVector.

class SplitVectorTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		psv = new SplitVector<int>;
	}

	virtual void TearDown() {
		delete psv;
		psv = 0;
	}

	SplitVector<int> *psv;
};

const int lengthTestArray = 4;
static const int testArray[4] = {3, 4, 5, 6};

TEST_F(SplitVectorTest, IsEmptyInitially) {
	EXPECT_EQ(0, psv->Length());
}

TEST_F(SplitVectorTest, InsertOne) {
	psv->InsertValue(0, 10, 0);
	psv->Insert(5, 3);
	EXPECT_EQ(11, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i == 5) ? 3 : 0, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, Insertion) {
	psv->InsertValue(0, 10, 0);
	EXPECT_EQ(10, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ(0, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, EnsureLength) {
	psv->EnsureLength(4);
	EXPECT_EQ(4, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ(0, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, InsertFromArray) {
	psv->InsertFromArray(0, testArray, 0, lengthTestArray);
	EXPECT_EQ(lengthTestArray, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ(i+3, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, SetValue) {
	psv->InsertValue(0, 10, 0);
	psv->SetValueAt(5, 3);
	EXPECT_EQ(10, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i == 5) ? 3 : 0, psv->ValueAt(i));
	}
	// Move the gap
	psv->InsertValue(7, 1, 17);
	EXPECT_EQ(17, psv->ValueAt(7));
	EXPECT_EQ(0, psv->ValueAt(8));
	// Set after the gap
	psv->SetValueAt(8, 19);
	EXPECT_EQ(19, psv->ValueAt(8));
}

TEST_F(SplitVectorTest, Indexing) {
	psv->InsertValue(0, 10, 0);
	psv->SetValueAt(5, 3);
	EXPECT_EQ(10, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i == 5) ? 3 : 0, (*psv)[i]);
	}
}

TEST_F(SplitVectorTest, Fill) {
	psv->InsertValue(0, 10, 0);
	EXPECT_EQ(10, psv->Length());
	psv->InsertValue(7, 1, 1);
	EXPECT_EQ(11, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i == 7) ? 1 : 0, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, DeleteOne) {
	psv->InsertFromArray(0, testArray, 0, lengthTestArray);
	psv->Delete(2);
	EXPECT_EQ(lengthTestArray-1, psv->Length());
	EXPECT_EQ(3, (*psv)[0]);
	EXPECT_EQ(4, (*psv)[1]);
	EXPECT_EQ(6, (*psv)[2]);
}

TEST_F(SplitVectorTest, DeleteRange) {
	psv->InsertValue(0, 10, 0);
	EXPECT_EQ(10, psv->Length());
	psv->InsertValue(7, 1, 1);
	EXPECT_EQ(11, psv->Length());
	psv->DeleteRange(2, 3);
	EXPECT_EQ(8, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i == 4) ? 1 : 0, psv->ValueAt(i));
	}
}

TEST_F(SplitVectorTest, DeleteAll) {
	psv->InsertValue(0, 10, 0);
	psv->InsertValue(7, 1, 1);
	psv->DeleteRange(2, 3);
	psv->DeleteAll();
	EXPECT_EQ(0, psv->Length());
}

TEST_F(SplitVectorTest, GetRange) {
	psv->InsertValue(0, 10, 0);
	psv->InsertValue(7, 1, 1);
	int retrieveArray[11] = {0};
	psv->GetRange(retrieveArray, 0, 11);
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ((i==7) ? 1 : 0, retrieveArray[i]);
	}
}

TEST_F(SplitVectorTest, GetRangeOverGap) {
	psv->InsertFromArray(0, testArray, 0, lengthTestArray);
	EXPECT_EQ(lengthTestArray, psv->Length());
	int retrieveArray[lengthTestArray] = {0};
	psv->GetRange(retrieveArray, 0, lengthTestArray);
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ(i+3, retrieveArray[i]);
	}
}

TEST_F(SplitVectorTest, ReplaceUp) {
	// Replace each element by inserting and then deleting the displaced element
	// This should perform many moves
	const int testLength=105;
	psv->EnsureLength(testLength);
	for (int i=0; i<testLength; i++)
		psv->SetValueAt(i, i+2);
	EXPECT_EQ(testLength, psv->Length());
	for (int i=0; i<psv->Length(); i++) {
		psv->InsertValue(i, 1, i+9);
		psv->Delete(i+1);
	}
	for (int i=0; i<psv->Length(); i++)
		EXPECT_EQ(i+9, psv->ValueAt(i));
}

TEST_F(SplitVectorTest, ReplaceDown) {
	// From the end, replace each element by inserting and then deleting the displaced element
	// This should perform many moves
	const int testLength=24;
	psv->EnsureLength(testLength);
	for (int i=0; i<testLength; i++)
		psv->SetValueAt(i, i+12);
	EXPECT_EQ(testLength, psv->Length());
	for (int i=psv->Length()-1; i>=0; i--) {
		psv->InsertValue(i, 1, i+5);
		psv->Delete(i+1);
	}
	for (int i=0; i<psv->Length(); i++)
		EXPECT_EQ(i+5, psv->ValueAt(i));
}

TEST_F(SplitVectorTest, BufferPointer) {
	psv->InsertFromArray(0, testArray, 0, lengthTestArray);
	int *retrievePointer = psv->BufferPointer();
	for (int i=0; i<psv->Length(); i++) {
		EXPECT_EQ(i+3, retrievePointer[i]);
	}
}

TEST_F(SplitVectorTest, DeleteBackAndForth) {
	psv->InsertValue(0, 10, 87);
	for (int i=0; i<10; i+=2) {
		int len = 10 - i;
		EXPECT_EQ(len, psv->Length());
		for (int i=0; i<psv->Length(); i++) {
			EXPECT_EQ(87, psv->ValueAt(i));
		}
		psv->Delete(len-1);
		psv->Delete(0);
	}
}

TEST_F(SplitVectorTest, GrowSize) {
	psv->SetGrowSize(5);
	EXPECT_EQ(5, psv->GetGrowSize());
}

TEST_F(SplitVectorTest, OutsideBounds) {
	psv->InsertValue(0, 10, 87);
	EXPECT_EQ(0, psv->ValueAt(-1));
	EXPECT_EQ(0, psv->ValueAt(10));

	/* Could be a death test as this asserts:
	psv->SetValueAt(-1,98);
	psv->SetValueAt(10,99);
	EXPECT_EQ(0, psv->ValueAt(-1));
	EXPECT_EQ(0, psv->ValueAt(10));
	*/
}
