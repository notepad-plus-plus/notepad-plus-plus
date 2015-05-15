// Unit Tests for Scintilla internal data structures

#include <string>
#include <vector>
#include <algorithm>

#include "Platform.h"

#include "SparseState.h"

#include <gtest/gtest.h>

// Test SparseState.

class SparseStateTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		pss = new SparseState<int>();
	}

	virtual void TearDown() {
		delete pss;
		pss = 0;
	}

	SparseState<int> *pss;
};

TEST_F(SparseStateTest, IsEmptyInitially) {
	EXPECT_EQ(0u, pss->size());
	int val = pss->ValueAt(0);
	EXPECT_EQ(0, val);
}

TEST_F(SparseStateTest, SimpleSetAndGet) {
	pss->Set(0, 22);
	pss->Set(1, 23);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(22, pss->ValueAt(0));
	EXPECT_EQ(23, pss->ValueAt(1));
	EXPECT_EQ(23, pss->ValueAt(2));
}

TEST_F(SparseStateTest, RetrieveBetween) {
	pss->Set(0, 10);
	pss->Set(2, 12);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(10, pss->ValueAt(0));
	EXPECT_EQ(10, pss->ValueAt(1));
	EXPECT_EQ(12, pss->ValueAt(2));
}

TEST_F(SparseStateTest, RetrieveBefore) {
	pss->Set(2, 12);
	EXPECT_EQ(1u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(0, pss->ValueAt(0));
	EXPECT_EQ(0, pss->ValueAt(1));
	EXPECT_EQ(12, pss->ValueAt(2));
}

TEST_F(SparseStateTest, Delete) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Delete(2);
	EXPECT_EQ(1u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(30, pss->ValueAt(0));
	EXPECT_EQ(30, pss->ValueAt(1));
	EXPECT_EQ(30, pss->ValueAt(2));
}

TEST_F(SparseStateTest, DeleteBetweeen) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Delete(1);
	EXPECT_EQ(1u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(30, pss->ValueAt(0));
	EXPECT_EQ(30, pss->ValueAt(1));
	EXPECT_EQ(30, pss->ValueAt(2));
}

TEST_F(SparseStateTest, ReplaceLast) {
	pss->Set(0, 30);
	pss->Set(2, 31);
	pss->Set(2, 32);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(0, pss->ValueAt(-1));
	EXPECT_EQ(30, pss->ValueAt(0));
	EXPECT_EQ(30, pss->ValueAt(1));
	EXPECT_EQ(32, pss->ValueAt(2));
	EXPECT_EQ(32, pss->ValueAt(3));
}

TEST_F(SparseStateTest, OnlyChangeAppended) {
	pss->Set(0, 30);
	pss->Set(2, 31);
	pss->Set(3, 31);
	EXPECT_EQ(2u, pss->size());
}

TEST_F(SparseStateTest, MergeBetween) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);
	EXPECT_EQ(3u, pss->size());

	SparseState<int> ssAdditions(3);
	ssAdditions.Set(4, 34);
	EXPECT_EQ(1u, ssAdditions.size());
	bool mergeChanged = pss->Merge(ssAdditions,5);
	EXPECT_EQ(0, mergeChanged);

	ssAdditions.Set(4, 44);
	EXPECT_EQ(1u, ssAdditions.size());
	mergeChanged = pss->Merge(ssAdditions,5);
	EXPECT_EQ(true, mergeChanged);
	EXPECT_EQ(3u, pss->size());
	EXPECT_EQ(44, pss->ValueAt(4));
}

TEST_F(SparseStateTest, MergeAtExisting) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);
	EXPECT_EQ(3u, pss->size());

	SparseState<int> ssAdditions(4);
	ssAdditions.Set(4, 34);
	EXPECT_EQ(1u, ssAdditions.size());
	bool mergeChanged = pss->Merge(ssAdditions,5);
	EXPECT_EQ(0, mergeChanged);

	ssAdditions.Set(4, 44);
	EXPECT_EQ(1u, ssAdditions.size());
	mergeChanged = pss->Merge(ssAdditions,5);
	EXPECT_EQ(true, mergeChanged);
	EXPECT_EQ(3u, pss->size());
	EXPECT_EQ(44, pss->ValueAt(4));
}

TEST_F(SparseStateTest, MergeWhichRemoves) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);
	EXPECT_EQ(3u, pss->size());

	SparseState<int> ssAdditions(2);
	ssAdditions.Set(2, 22);
	EXPECT_EQ(1u, ssAdditions.size());
	EXPECT_EQ(22, ssAdditions.ValueAt(2));
	bool mergeChanged = pss->Merge(ssAdditions,5);
	EXPECT_EQ(true, mergeChanged);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(22, pss->ValueAt(2));
}

TEST_F(SparseStateTest, MergeIgnoreSome) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);

	SparseState<int> ssAdditions(2);
	ssAdditions.Set(2, 32);
	bool mergeChanged = pss->Merge(ssAdditions,3);

	EXPECT_EQ(0, mergeChanged);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(32, pss->ValueAt(2));
}

TEST_F(SparseStateTest, MergeIgnoreSomeStart) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);

	SparseState<int> ssAdditions(2);
	ssAdditions.Set(2, 32);
	bool mergeChanged = pss->Merge(ssAdditions,2);

	EXPECT_EQ(0, mergeChanged);
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ(32, pss->ValueAt(2));
}

TEST_F(SparseStateTest, MergeIgnoreRepeat) {
	pss->Set(0, 30);
	pss->Set(2, 32);
	pss->Set(4, 34);

	SparseState<int> ssAdditions(5);
	// Appending same value as at end of pss.
	ssAdditions.Set(5, 34);
	bool mergeChanged = pss->Merge(ssAdditions,6);

	EXPECT_EQ(0, mergeChanged);
	EXPECT_EQ(3u, pss->size());
	EXPECT_EQ(34, pss->ValueAt(4));
}

class SparseStateStringTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		pss = new SparseState<std::string>();
	}

	virtual void TearDown() {
		delete pss;
		pss = 0;
	}

	SparseState<std::string> *pss;
};

TEST_F(SparseStateStringTest, IsEmptyInitially) {
	EXPECT_EQ(0u, pss->size());
	std::string val = pss->ValueAt(0);
	EXPECT_EQ("", val);
}

TEST_F(SparseStateStringTest, SimpleSetAndGet) {
	EXPECT_EQ(0u, pss->size());
	pss->Set(0, "22");
	pss->Set(1, "23");
	EXPECT_EQ(2u, pss->size());
	EXPECT_EQ("", pss->ValueAt(-1));
	EXPECT_EQ("22", pss->ValueAt(0));
	EXPECT_EQ("23", pss->ValueAt(1));
	EXPECT_EQ("23", pss->ValueAt(2));
}

TEST_F(SparseStateStringTest, DeleteBetweeen) {
	pss->Set(0, "30");
	pss->Set(2, "32");
	pss->Delete(1);
	EXPECT_EQ(1u, pss->size());
	EXPECT_EQ("", pss->ValueAt(-1));
	EXPECT_EQ("30", pss->ValueAt(0));
	EXPECT_EQ("30", pss->ValueAt(1));
	EXPECT_EQ("30", pss->ValueAt(2));
}
