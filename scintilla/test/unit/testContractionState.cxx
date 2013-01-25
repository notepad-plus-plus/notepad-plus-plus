// Unit Tests for Scintilla internal data structures

#include <string.h>

#include "Platform.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"

#include <gtest/gtest.h>

// Test ContractionState.

class ContractionStateTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		pcs = new ContractionState();
	}

	virtual void TearDown() {
		delete pcs;
		pcs = 0;
	}

	ContractionState *pcs;
};

TEST_F(ContractionStateTest, IsEmptyInitially) {
	EXPECT_EQ(1, pcs->LinesInDoc());
	EXPECT_EQ(1, pcs->LinesDisplayed());
	EXPECT_EQ(0, pcs->DisplayFromDoc(0));
	EXPECT_EQ(0, pcs->DocFromDisplay(0));
}

TEST_F(ContractionStateTest, OneLine) {
	pcs->InsertLine(0);
	EXPECT_EQ(2, pcs->LinesInDoc());
	EXPECT_EQ(2, pcs->LinesDisplayed());
	EXPECT_EQ(0, pcs->DisplayFromDoc(0));
	EXPECT_EQ(0, pcs->DocFromDisplay(0));
	EXPECT_EQ(1, pcs->DisplayFromDoc(1));
	EXPECT_EQ(1, pcs->DocFromDisplay(1));
}

TEST_F(ContractionStateTest, InsertionThenDeletions) {
	pcs->InsertLines(0,4);
	pcs->DeleteLine(1);
	
	EXPECT_EQ(4, pcs->LinesInDoc());
	EXPECT_EQ(4, pcs->LinesDisplayed());
	for (int l=0;l<4;l++) {
		EXPECT_EQ(l, pcs->DisplayFromDoc(l));
		EXPECT_EQ(l, pcs->DocFromDisplay(l));
	}
	
	pcs->DeleteLines(0,2);
	EXPECT_EQ(2, pcs->LinesInDoc());
	EXPECT_EQ(2, pcs->LinesDisplayed());
	for (int l=0;l<2;l++) {
		EXPECT_EQ(l, pcs->DisplayFromDoc(l));
		EXPECT_EQ(l, pcs->DocFromDisplay(l));
	}
}

TEST_F(ContractionStateTest, ShowHide) {
	pcs->InsertLines(0,4);
	EXPECT_EQ(true, pcs->GetVisible(0));
	EXPECT_EQ(true, pcs->GetVisible(1));
	EXPECT_EQ(true, pcs->GetVisible(2));
	EXPECT_EQ(5, pcs->LinesDisplayed());

	pcs->SetVisible(1, 1, false);
	EXPECT_EQ(true, pcs->GetVisible(0));
	EXPECT_EQ(0, pcs->GetVisible(1));
	EXPECT_EQ(true, pcs->GetVisible(2));
	EXPECT_EQ(4, pcs->LinesDisplayed());
	EXPECT_EQ(1, pcs->HiddenLines());

	pcs->SetVisible(1, 2, true);
	for (int l=0;l<4;l++) {
		EXPECT_EQ(true, pcs->GetVisible(0));
	}

	pcs->SetVisible(1, 1, false);
	EXPECT_EQ(0, pcs->GetVisible(1));
	pcs->ShowAll();
	for (int l=0;l<4;l++) {
		EXPECT_EQ(true, pcs->GetVisible(0));
	}
	EXPECT_EQ(0, pcs->HiddenLines());
}

TEST_F(ContractionStateTest, Hidden) {
	pcs->InsertLines(0,1);
	for (int l=0;l<2;l++) {
		EXPECT_EQ(true, pcs->GetVisible(0));
	}
	EXPECT_EQ(0, pcs->HiddenLines());

	pcs->SetVisible(1, 1, false);
	EXPECT_EQ(true, pcs->GetVisible(0));
	EXPECT_EQ(0, pcs->GetVisible(1));
	EXPECT_EQ(1, pcs->HiddenLines());

	pcs->SetVisible(1, 1, true);
	for (int l=0;l<2;l++) {
		EXPECT_EQ(true, pcs->GetVisible(0));
	}
	EXPECT_EQ(0, pcs->HiddenLines());
}

TEST_F(ContractionStateTest, Contracting) {
	pcs->InsertLines(0,4);
	for (int l=0;l<4;l++) {
		EXPECT_EQ(true, pcs->GetExpanded(l));
	}

	pcs->SetExpanded(2, false);
	EXPECT_EQ(true, pcs->GetExpanded(1));
	EXPECT_EQ(0, pcs->GetExpanded(2));
	EXPECT_EQ(true, pcs->GetExpanded(3));
	
	EXPECT_EQ(2, pcs->ContractedNext(0));
	EXPECT_EQ(2, pcs->ContractedNext(1));
	EXPECT_EQ(2, pcs->ContractedNext(2));
	EXPECT_EQ(-1, pcs->ContractedNext(3));

	pcs->SetExpanded(2, true);
	EXPECT_EQ(true, pcs->GetExpanded(1));
	EXPECT_EQ(true, pcs->GetExpanded(2));
	EXPECT_EQ(true, pcs->GetExpanded(3));
}

TEST_F(ContractionStateTest, ChangeHeight) {
	pcs->InsertLines(0,4);
	for (int l=0;l<4;l++) {
		EXPECT_EQ(1, pcs->GetHeight(l));
	}

	pcs->SetHeight(1, 2);
	EXPECT_EQ(1, pcs->GetHeight(0));
	EXPECT_EQ(2, pcs->GetHeight(1));
	EXPECT_EQ(1, pcs->GetHeight(2));
}
