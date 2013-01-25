// Unit Tests for Scintilla internal data structures

#include <string.h>

#include "Platform.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"

#include <gtest/gtest.h>

// Test RunStyles.

class RunStylesTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		prs = new RunStyles();
	}

	virtual void TearDown() {
		delete prs;
		prs = 0;
	}

	RunStyles *prs;
};

TEST_F(RunStylesTest, IsEmptyInitially) {
	EXPECT_EQ(0, prs->Length());
	EXPECT_EQ(1, prs->Runs());
}

TEST_F(RunStylesTest, SimpleInsert) {
	prs->InsertSpace(0, 1);
	EXPECT_EQ(1, prs->Length());
	EXPECT_EQ(1, prs->Runs());
	EXPECT_EQ(0, prs->ValueAt(0));
	EXPECT_EQ(1, prs->FindNextChange(0, prs->Length()));
	EXPECT_EQ(2, prs->FindNextChange(1, prs->Length()));
}

TEST_F(RunStylesTest, TwoRuns) {
	prs->InsertSpace(0, 2);
	EXPECT_EQ(2, prs->Length());
	EXPECT_EQ(1, prs->Runs());
	prs->SetValueAt(0, 2);
	EXPECT_EQ(2, prs->Runs());
	EXPECT_EQ(2, prs->ValueAt(0));
	EXPECT_EQ(0, prs->ValueAt(1));
	EXPECT_EQ(1, prs->FindNextChange(0, prs->Length()));
	EXPECT_EQ(2, prs->FindNextChange(1, prs->Length()));
	EXPECT_EQ(3, prs->FindNextChange(2, prs->Length()));
}

TEST_F(RunStylesTest, LongerRuns) {
	prs->InsertSpace(0, 5);
	prs->SetValueAt(0, 3);
	prs->SetValueAt(1, 3);
	EXPECT_EQ(3, prs->ValueAt(0));
	EXPECT_EQ(3, prs->ValueAt(1));
	EXPECT_EQ(0, prs->ValueAt(2));
	EXPECT_EQ(2, prs->Runs());

	EXPECT_EQ(0, prs->StartRun(0));
	EXPECT_EQ(2, prs->EndRun(0));

	EXPECT_EQ(0, prs->StartRun(1));
	EXPECT_EQ(2, prs->EndRun(1));

	EXPECT_EQ(2, prs->StartRun(2));
	EXPECT_EQ(5, prs->EndRun(2));

	EXPECT_EQ(2, prs->StartRun(3));
	EXPECT_EQ(5, prs->EndRun(3));

	EXPECT_EQ(2, prs->StartRun(4));
	EXPECT_EQ(5, prs->EndRun(4));

	// At end
	EXPECT_EQ(2, prs->StartRun(5));
	EXPECT_EQ(5, prs->EndRun(5));

	// After end is same as end
	EXPECT_EQ(2, prs->StartRun(6));
	EXPECT_EQ(5, prs->EndRun(6));

	EXPECT_EQ(2, prs->FindNextChange(0, prs->Length()));
	EXPECT_EQ(5, prs->FindNextChange(2, prs->Length()));
	EXPECT_EQ(6, prs->FindNextChange(5, prs->Length()));
}

TEST_F(RunStylesTest, FillRange) {
	prs->InsertSpace(0, 5);
	int startFill = 1;
	int lengthFill = 3;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(1, startFill);
	EXPECT_EQ(3, lengthFill);

	EXPECT_EQ(0, prs->ValueAt(0));
	EXPECT_EQ(99, prs->ValueAt(1));
	EXPECT_EQ(99, prs->ValueAt(2));
	EXPECT_EQ(99, prs->ValueAt(3));
	EXPECT_EQ(0, prs->ValueAt(4));

	EXPECT_EQ(0, prs->StartRun(0));
	EXPECT_EQ(1, prs->EndRun(0));

	EXPECT_EQ(1, prs->StartRun(1));
	EXPECT_EQ(4, prs->EndRun(1));
}

TEST_F(RunStylesTest, FillRangeAlreadyFilled) {
	prs->InsertSpace(0, 5);
	int startFill = 1;
	int lengthFill = 3;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(1, startFill);
	EXPECT_EQ(3, lengthFill);

	int startFill2 = 2;
	int lengthFill2 = 1;
	// Compiler warnings if 'false' used instead of '0' as expected value:
	EXPECT_EQ(0, prs->FillRange(startFill2, 99, lengthFill2));
	EXPECT_EQ(2, startFill2);
	EXPECT_EQ(1, lengthFill2);
	EXPECT_EQ(0, prs->ValueAt(0));
	EXPECT_EQ(99, prs->ValueAt(1));
	EXPECT_EQ(99, prs->ValueAt(2));
	EXPECT_EQ(99, prs->ValueAt(3));
	EXPECT_EQ(0, prs->ValueAt(4));
	EXPECT_EQ(3, prs->Runs());
}

TEST_F(RunStylesTest, FillRangeAlreadyPartFilled) {
	prs->InsertSpace(0, 5);
	int startFill = 1;
	int lengthFill = 2;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(1, startFill);
	EXPECT_EQ(2, lengthFill);

	int startFill2 = 2;
	int lengthFill2 = 2;
	EXPECT_EQ(true, prs->FillRange(startFill2, 99, lengthFill2));
	EXPECT_EQ(3, startFill2);
	EXPECT_EQ(1, lengthFill2);
	EXPECT_EQ(3, prs->Runs());
}

TEST_F(RunStylesTest, DeleteRange) {
	prs->InsertSpace(0, 5);
	prs->SetValueAt(0, 3);
	EXPECT_EQ(2, prs->Runs());
	prs->SetValueAt(1, 3);
	EXPECT_EQ(2, prs->Runs());
	prs->DeleteRange(1, 1);
	EXPECT_EQ(4, prs->Length());
	EXPECT_EQ(2, prs->Runs());
	EXPECT_EQ(3, prs->ValueAt(0));
	EXPECT_EQ(0, prs->ValueAt(1));

	EXPECT_EQ(0, prs->StartRun(0));
	EXPECT_EQ(1, prs->EndRun(0));

	EXPECT_EQ(1, prs->StartRun(1));
	EXPECT_EQ(4, prs->EndRun(1));

	EXPECT_EQ(1, prs->StartRun(2));
	EXPECT_EQ(4, prs->EndRun(2));
}

TEST_F(RunStylesTest, Find) {
	prs->InsertSpace(0, 5);
	int startFill = 1;
	int lengthFill = 3;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(1, startFill);
	EXPECT_EQ(3, lengthFill);

	EXPECT_EQ(0, prs->Find(0,0));
	EXPECT_EQ(1, prs->Find(99,0));
	EXPECT_EQ(-1, prs->Find(3,0));
	
	EXPECT_EQ(4, prs->Find(0,1));
	EXPECT_EQ(1, prs->Find(99,1));
	EXPECT_EQ(-1, prs->Find(3,1));
	
	EXPECT_EQ(4, prs->Find(0,2));
	EXPECT_EQ(2, prs->Find(99,2));
	EXPECT_EQ(-1, prs->Find(3,2));

	EXPECT_EQ(4, prs->Find(0,4));
	EXPECT_EQ(-1, prs->Find(99,4));
	EXPECT_EQ(-1, prs->Find(3,4));

	EXPECT_EQ(-1, prs->Find(0,5));
	EXPECT_EQ(-1, prs->Find(99,5));
	EXPECT_EQ(-1, prs->Find(3,5));

	EXPECT_EQ(-1, prs->Find(0,6));
	EXPECT_EQ(-1, prs->Find(99,6));
	EXPECT_EQ(-1, prs->Find(3,6));
}

TEST_F(RunStylesTest, AllSame) {
	EXPECT_EQ(true, prs->AllSame());
	prs->InsertSpace(0, 5);
	EXPECT_EQ(true, prs->AllSame());
	EXPECT_EQ(0, prs->AllSameAs(88));
	EXPECT_EQ(true, prs->AllSameAs(0));
	int startFill = 1;
	int lengthFill = 3;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(0, prs->AllSame());
	EXPECT_EQ(0, prs->AllSameAs(88));
	EXPECT_EQ(0, prs->AllSameAs(0));
	EXPECT_EQ(true, prs->FillRange(startFill, 0, lengthFill));
	EXPECT_EQ(true, prs->AllSame());
	EXPECT_EQ(0, prs->AllSameAs(88));
	EXPECT_EQ(true, prs->AllSameAs(0));
}

TEST_F(RunStylesTest, FindWithReversion) {
	prs->InsertSpace(0, 5);
	EXPECT_EQ(1, prs->Runs());

	int startFill = 1;
	int lengthFill = 1;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(1, startFill);
	EXPECT_EQ(1, lengthFill);
	EXPECT_EQ(3, prs->Runs());

	startFill = 2;
	lengthFill = 1;
	EXPECT_EQ(true, prs->FillRange(startFill, 99, lengthFill));
	EXPECT_EQ(2, startFill);
	EXPECT_EQ(1, lengthFill);
	EXPECT_EQ(3, prs->Runs());

	startFill = 1;
	lengthFill = 1;
	EXPECT_EQ(true, prs->FillRange(startFill, 0, lengthFill));
	EXPECT_EQ(3, prs->Runs());
	EXPECT_EQ(1, lengthFill);

	startFill = 2;
	lengthFill = 1;
	EXPECT_EQ(true, prs->FillRange(startFill, 0, lengthFill));
	EXPECT_EQ(1, prs->Runs());
	EXPECT_EQ(1, lengthFill);

	EXPECT_EQ(-1, prs->Find(0,6));
}

TEST_F(RunStylesTest, FinalRunInversion) {
	EXPECT_EQ(1, prs->Runs());
	prs->InsertSpace(0, 1);
	EXPECT_EQ(1, prs->Runs());
	prs->SetValueAt(0, 1);
	EXPECT_EQ(1, prs->Runs());
	prs->InsertSpace(1, 1);
	EXPECT_EQ(1, prs->Runs());
	prs->SetValueAt(1, 1);
	EXPECT_EQ(1, prs->Runs());
	prs->SetValueAt(1, 0);
	EXPECT_EQ(2, prs->Runs());
	prs->SetValueAt(1, 1);
	EXPECT_EQ(1, prs->Runs());
}	

TEST_F(RunStylesTest, DeleteAll) {
	prs->InsertSpace(0, 5);
	prs->SetValueAt(0, 3);
	prs->SetValueAt(1, 3);
	prs->DeleteAll();
	EXPECT_EQ(0, prs->Length());
	EXPECT_EQ(0, prs->ValueAt(0));
	EXPECT_EQ(1, prs->Runs());
}

