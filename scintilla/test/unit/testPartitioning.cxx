// Unit Tests for Scintilla internal data structures

#include <string.h>

#include "Platform.h"

#include "SplitVector.h"
#include "Partitioning.h"

#include <gtest/gtest.h>

const int growSize = 4;

const int lengthTestArray = 8;
static const int testArray[lengthTestArray] = {3, 4, 5, 6, 7, 8, 9, 10};

// Test SplitVectorWithRangeAdd.

class SplitVectorWithRangeAddTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		psvwra = new SplitVectorWithRangeAdd(growSize);
	}

	virtual void TearDown() {
		delete psvwra;
		psvwra = 0;
	}

	SplitVectorWithRangeAdd *psvwra;
};

TEST_F(SplitVectorWithRangeAddTest, IsEmptyInitially) {
	EXPECT_EQ(0, psvwra->Length());
}

TEST_F(SplitVectorWithRangeAddTest, IncrementExceptEnds) {
	psvwra->InsertFromArray(0, testArray, 0, lengthTestArray);
	psvwra->RangeAddDelta(1, lengthTestArray-1, 1);
	for (int i=0; i<psvwra->Length(); i++) {
		if ((i == 0) || (i == lengthTestArray-1))
			EXPECT_EQ(i+3, psvwra->ValueAt(i));
		else
			EXPECT_EQ(i+4, psvwra->ValueAt(i));
	}
}

// Test Partitioning.

class PartitioningTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		pp = new Partitioning(growSize);
	}

	virtual void TearDown() {
		delete pp;
		pp = 0;
	}

	Partitioning *pp;
};

TEST_F(PartitioningTest, IsEmptyInitially) {
	EXPECT_EQ(1, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(pp->Partitions()));
	EXPECT_EQ(0, pp->PartitionFromPosition(0));
}

TEST_F(PartitioningTest, SimpleInsert) {
	pp->InsertText(0, 1);
	EXPECT_EQ(1, pp->Partitions());
	EXPECT_EQ(1, pp->PositionFromPartition(pp->Partitions()));
}

TEST_F(PartitioningTest, TwoPartitions) {
	pp->InsertText(0, 2);
	pp->InsertPartition(1, 1);
	EXPECT_EQ(2, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(1, pp->PositionFromPartition(1));
	EXPECT_EQ(2, pp->PositionFromPartition(2));
}

TEST_F(PartitioningTest, MoveStart) {
	pp->InsertText(0, 3);
	pp->InsertPartition(1, 2);
	pp->SetPartitionStartPosition(1,1);
	EXPECT_EQ(2, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(1, pp->PositionFromPartition(1));
	EXPECT_EQ(3, pp->PositionFromPartition(2));
}

TEST_F(PartitioningTest, InsertAgain) {
	pp->InsertText(0, 3);
	pp->InsertPartition(1, 2);
	pp->InsertText(0,3);
	pp->InsertText(1,2);
	EXPECT_EQ(2, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(5, pp->PositionFromPartition(1));
	EXPECT_EQ(8, pp->PositionFromPartition(2));
}

TEST_F(PartitioningTest, InsertReversed) {
	pp->InsertText(0, 3);
	pp->InsertPartition(1, 2);
	pp->InsertText(1,2);
	pp->InsertText(0,3);
	EXPECT_EQ(2, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(5, pp->PositionFromPartition(1));
	EXPECT_EQ(8, pp->PositionFromPartition(2));
}

TEST_F(PartitioningTest, InverseSearch) {
	pp->InsertText(0, 3);
	pp->InsertPartition(1, 2);
	pp->SetPartitionStartPosition(1,1);

	EXPECT_EQ(2, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(1, pp->PositionFromPartition(1));
	EXPECT_EQ(3, pp->PositionFromPartition(2));

	EXPECT_EQ(0, pp->PartitionFromPosition(0));
	EXPECT_EQ(1, pp->PartitionFromPosition(1));
	EXPECT_EQ(1, pp->PartitionFromPosition(2));

	EXPECT_EQ(1, pp->PartitionFromPosition(3));
}

TEST_F(PartitioningTest, DeletePartition) {
	pp->InsertText(0, 2);
	pp->InsertPartition(1, 1);
	pp->RemovePartition(1);
	EXPECT_EQ(1, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(2, pp->PositionFromPartition(1));
}

TEST_F(PartitioningTest, DeleteAll) {
	pp->InsertText(0, 3);
	pp->InsertPartition(1, 2);
	pp->SetPartitionStartPosition(1,1);
	pp->DeleteAll();
	// Back to initial state
	EXPECT_EQ(1, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(pp->Partitions()));
}

TEST_F(PartitioningTest, TestBackwards) {
	pp->InsertText(0, 10);
	pp->InsertPartition(1, 3);
	pp->InsertPartition(2, 6);
	pp->InsertPartition(3, 9);
	pp->InsertText(2,4);
	pp->InsertText(1,2);
	pp->InsertText(0,3);
	EXPECT_EQ(4, pp->Partitions());
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(6, pp->PositionFromPartition(1));
	EXPECT_EQ(11, pp->PositionFromPartition(2));
	EXPECT_EQ(18, pp->PositionFromPartition(3));
	EXPECT_EQ(19, pp->PositionFromPartition(4));
}

TEST_F(PartitioningTest, TestMany) {
	// Provoke backstep call
	pp->InsertText(0, 42);
	for (int i=0; i<20; i++) {
		pp->InsertPartition(i+1, (i+1) * 2);
	}
	for (int i=20; i>0; i--) {
		pp->InsertText(i,2);
	}
	EXPECT_EQ(21, pp->Partitions());
	for (int i=1; i<20; i++) {
		EXPECT_EQ(i*4 - 2, pp->PositionFromPartition(i));
		EXPECT_EQ(i, pp->PartitionFromPosition(i*4 - 2));
	}
	pp->InsertText(19,2);
	EXPECT_EQ(3, pp->PartitionFromPosition(10));
	pp->InsertText(0,2);
	pp->InsertText(0,-2);
	pp->RemovePartition(1);
	EXPECT_EQ(0, pp->PositionFromPartition(0));
	EXPECT_EQ(6, pp->PositionFromPartition(1));
	EXPECT_EQ(10, pp->PositionFromPartition(2));
	pp->RemovePartition(10);
	EXPECT_EQ(46, pp->PositionFromPartition(10));
	EXPECT_EQ(10, pp->PartitionFromPosition(46));
	EXPECT_EQ(50, pp->PositionFromPartition(11));
	EXPECT_EQ(11, pp->PartitionFromPosition(50));
}

#if !PLAT_WIN
// Omit death tests on Windows where they trigger a system "unitTest.exe has stopped working" popup.
TEST_F(PartitioningTest, OutOfRangeDeathTest) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";
	pp->InsertText(0, 2);
	pp->InsertPartition(1, 1);
	EXPECT_EQ(2, pp->Partitions());
	ASSERT_DEATH(pp->PositionFromPartition(-1), "Assertion");
	ASSERT_DEATH(pp->PositionFromPartition(3), "Assertion");
}
#endif
