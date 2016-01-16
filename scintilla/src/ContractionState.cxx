// Scintilla source code edit control
/** @file ContractionState.cxx
 ** Manages visibility of lines for folding and wrapping.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

#include <stdexcept>
#include <algorithm>

#include "Platform.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

ContractionState::ContractionState() : visible(0), expanded(0), heights(0), displayLines(0), linesInDocument(1) {
	//InsertLine(0);
}

ContractionState::~ContractionState() {
	Clear();
}

void ContractionState::EnsureData() {
	if (OneToOne()) {
		visible = new RunStyles();
		expanded = new RunStyles();
		heights = new RunStyles();
		displayLines = new Partitioning(4);
		InsertLines(0, linesInDocument);
	}
}

void ContractionState::Clear() {
	delete visible;
	visible = 0;
	delete expanded;
	expanded = 0;
	delete heights;
	heights = 0;
	delete displayLines;
	displayLines = 0;
	linesInDocument = 1;
}

int ContractionState::LinesInDoc() const {
	if (OneToOne()) {
		return linesInDocument;
	} else {
		return displayLines->Partitions() - 1;
	}
}

int ContractionState::LinesDisplayed() const {
	if (OneToOne()) {
		return linesInDocument;
	} else {
		return displayLines->PositionFromPartition(LinesInDoc());
	}
}

int ContractionState::DisplayFromDoc(int lineDoc) const {
	if (OneToOne()) {
		return (lineDoc <= linesInDocument) ? lineDoc : linesInDocument;
	} else {
		if (lineDoc > displayLines->Partitions())
			lineDoc = displayLines->Partitions();
		return displayLines->PositionFromPartition(lineDoc);
	}
}

int ContractionState::DisplayLastFromDoc(int lineDoc) const {
	return DisplayFromDoc(lineDoc) + GetHeight(lineDoc) - 1;
}

int ContractionState::DocFromDisplay(int lineDisplay) const {
	if (OneToOne()) {
		return lineDisplay;
	} else {
		if (lineDisplay <= 0) {
			return 0;
		}
		if (lineDisplay > LinesDisplayed()) {
			return displayLines->PartitionFromPosition(LinesDisplayed());
		}
		int lineDoc = displayLines->PartitionFromPosition(lineDisplay);
		PLATFORM_ASSERT(GetVisible(lineDoc));
		return lineDoc;
	}
}

void ContractionState::InsertLine(int lineDoc) {
	if (OneToOne()) {
		linesInDocument++;
	} else {
		visible->InsertSpace(lineDoc, 1);
		visible->SetValueAt(lineDoc, 1);
		expanded->InsertSpace(lineDoc, 1);
		expanded->SetValueAt(lineDoc, 1);
		heights->InsertSpace(lineDoc, 1);
		heights->SetValueAt(lineDoc, 1);
		int lineDisplay = DisplayFromDoc(lineDoc);
		displayLines->InsertPartition(lineDoc, lineDisplay);
		displayLines->InsertText(lineDoc, 1);
	}
}

void ContractionState::InsertLines(int lineDoc, int lineCount) {
	for (int l = 0; l < lineCount; l++) {
		InsertLine(lineDoc + l);
	}
	Check();
}

void ContractionState::DeleteLine(int lineDoc) {
	if (OneToOne()) {
		linesInDocument--;
	} else {
		if (GetVisible(lineDoc)) {
			displayLines->InsertText(lineDoc, -heights->ValueAt(lineDoc));
		}
		displayLines->RemovePartition(lineDoc);
		visible->DeleteRange(lineDoc, 1);
		expanded->DeleteRange(lineDoc, 1);
		heights->DeleteRange(lineDoc, 1);
	}
}

void ContractionState::DeleteLines(int lineDoc, int lineCount) {
	for (int l = 0; l < lineCount; l++) {
		DeleteLine(lineDoc);
	}
	Check();
}

bool ContractionState::GetVisible(int lineDoc) const {
	if (OneToOne()) {
		return true;
	} else {
		if (lineDoc >= visible->Length())
			return true;
		return visible->ValueAt(lineDoc) == 1;
	}
}

bool ContractionState::SetVisible(int lineDocStart, int lineDocEnd, bool isVisible) {
	if (OneToOne() && isVisible) {
		return false;
	} else {
		EnsureData();
		int delta = 0;
		Check();
		if ((lineDocStart <= lineDocEnd) && (lineDocStart >= 0) && (lineDocEnd < LinesInDoc())) {
			for (int line = lineDocStart; line <= lineDocEnd; line++) {
				if (GetVisible(line) != isVisible) {
					int difference = isVisible ? heights->ValueAt(line) : -heights->ValueAt(line);
					visible->SetValueAt(line, isVisible ? 1 : 0);
					displayLines->InsertText(line, difference);
					delta += difference;
				}
			}
		} else {
			return false;
		}
		Check();
		return delta != 0;
	}
}

bool ContractionState::HiddenLines() const {
	if (OneToOne()) {
		return false;
	} else {
		return !visible->AllSameAs(1);
	}
}

bool ContractionState::GetExpanded(int lineDoc) const {
	if (OneToOne()) {
		return true;
	} else {
		Check();
		return expanded->ValueAt(lineDoc) == 1;
	}
}

bool ContractionState::SetExpanded(int lineDoc, bool isExpanded) {
	if (OneToOne() && isExpanded) {
		return false;
	} else {
		EnsureData();
		if (isExpanded != (expanded->ValueAt(lineDoc) == 1)) {
			expanded->SetValueAt(lineDoc, isExpanded ? 1 : 0);
			Check();
			return true;
		} else {
			Check();
			return false;
		}
	}
}

int ContractionState::ContractedNext(int lineDocStart) const {
	if (OneToOne()) {
		return -1;
	} else {
		Check();
		if (!expanded->ValueAt(lineDocStart)) {
			return lineDocStart;
		} else {
			int lineDocNextChange = expanded->EndRun(lineDocStart);
			if (lineDocNextChange < LinesInDoc())
				return lineDocNextChange;
			else
				return -1;
		}
	}
}

int ContractionState::GetHeight(int lineDoc) const {
	if (OneToOne()) {
		return 1;
	} else {
		return heights->ValueAt(lineDoc);
	}
}

// Set the number of display lines needed for this line.
// Return true if this is a change.
bool ContractionState::SetHeight(int lineDoc, int height) {
	if (OneToOne() && (height == 1)) {
		return false;
	} else if (lineDoc < LinesInDoc()) {
		EnsureData();
		if (GetHeight(lineDoc) != height) {
			if (GetVisible(lineDoc)) {
				displayLines->InsertText(lineDoc, height - GetHeight(lineDoc));
			}
			heights->SetValueAt(lineDoc, height);
			Check();
			return true;
		} else {
			Check();
			return false;
		}
	} else {
		return false;
	}
}

void ContractionState::ShowAll() {
	int lines = LinesInDoc();
	Clear();
	linesInDocument = lines;
}

// Debugging checks

void ContractionState::Check() const {
#ifdef CHECK_CORRECTNESS
	for (int vline = 0; vline < LinesDisplayed(); vline++) {
		const int lineDoc = DocFromDisplay(vline);
		PLATFORM_ASSERT(GetVisible(lineDoc));
	}
	for (int lineDoc = 0; lineDoc < LinesInDoc(); lineDoc++) {
		const int displayThis = DisplayFromDoc(lineDoc);
		const int displayNext = DisplayFromDoc(lineDoc + 1);
		const int height = displayNext - displayThis;
		PLATFORM_ASSERT(height >= 0);
		if (GetVisible(lineDoc)) {
			PLATFORM_ASSERT(GetHeight(lineDoc) == height);
		} else {
			PLATFORM_ASSERT(0 == height);
		}
	}
#endif
}
