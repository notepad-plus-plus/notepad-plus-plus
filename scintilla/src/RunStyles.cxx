/** @file RunStyles.cxx
 ** Data structure used to store sparse styles.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <stdexcept>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// Find the first run at a position
int RunStyles::RunFromPosition(int position) const {
	int run = starts->PartitionFromPosition(position);
	// Go to first element with this position
	while ((run > 0) && (position == starts->PositionFromPartition(run-1))) {
		run--;
	}
	return run;
}

// If there is no run boundary at position, insert one continuing style.
int RunStyles::SplitRun(int position) {
	int run = RunFromPosition(position);
	int posRun = starts->PositionFromPartition(run);
	if (posRun < position) {
		int runStyle = ValueAt(position);
		run++;
		starts->InsertPartition(run, position);
		styles->InsertValue(run, 1, runStyle);
	}
	return run;
}

void RunStyles::RemoveRun(int run) {
	starts->RemovePartition(run);
	styles->DeleteRange(run, 1);
}

void RunStyles::RemoveRunIfEmpty(int run) {
	if ((run < starts->Partitions()) && (starts->Partitions() > 1)) {
		if (starts->PositionFromPartition(run) == starts->PositionFromPartition(run+1)) {
			RemoveRun(run);
		}
	}
}

void RunStyles::RemoveRunIfSameAsPrevious(int run) {
	if ((run > 0) && (run < starts->Partitions())) {
		if (styles->ValueAt(run-1) == styles->ValueAt(run)) {
			RemoveRun(run);
		}
	}
}

RunStyles::RunStyles() {
	starts = new Partitioning(8);
	styles = new SplitVector<int>();
	styles->InsertValue(0, 2, 0);
}

RunStyles::~RunStyles() {
	delete starts;
	starts = NULL;
	delete styles;
	styles = NULL;
}

int RunStyles::Length() const {
	return starts->PositionFromPartition(starts->Partitions());
}

int RunStyles::ValueAt(int position) const {
	return styles->ValueAt(starts->PartitionFromPosition(position));
}

int RunStyles::FindNextChange(int position, int end) const {
	int run = starts->PartitionFromPosition(position);
	if (run < starts->Partitions()) {
		int runChange = starts->PositionFromPartition(run);
		if (runChange > position)
			return runChange;
		int nextChange = starts->PositionFromPartition(run + 1);
		if (nextChange > position) {
			return nextChange;
		} else if (position < end) {
			return end;
		} else {
			return end + 1;
		}
	} else {
		return end + 1;
	}
}

int RunStyles::StartRun(int position) const {
	return starts->PositionFromPartition(starts->PartitionFromPosition(position));
}

int RunStyles::EndRun(int position) const {
	return starts->PositionFromPartition(starts->PartitionFromPosition(position) + 1);
}

bool RunStyles::FillRange(int &position, int value, int &fillLength) {
	if (fillLength <= 0) {
		return false;
	}
	int end = position + fillLength;
	if (end > Length()) {
		return false;
	}
	int runEnd = RunFromPosition(end);
	if (styles->ValueAt(runEnd) == value) {
		// End already has value so trim range.
		end = starts->PositionFromPartition(runEnd);
		if (position >= end) {
			// Whole range is already same as value so no action
			return false;
		}
		fillLength = end - position;
	} else {
		runEnd = SplitRun(end);
	}
	int runStart = RunFromPosition(position);
	if (styles->ValueAt(runStart) == value) {
		// Start is in expected value so trim range.
		runStart++;
		position = starts->PositionFromPartition(runStart);
		fillLength = end - position;
	} else {
		if (starts->PositionFromPartition(runStart) < position) {
			runStart = SplitRun(position);
			runEnd++;
		}
	}
	if (runStart < runEnd) {
		styles->SetValueAt(runStart, value);
		// Remove each old run over the range
		for (int run=runStart+1; run<runEnd; run++) {
			RemoveRun(runStart+1);
		}
		runEnd = RunFromPosition(end);
		RemoveRunIfSameAsPrevious(runEnd);
		RemoveRunIfSameAsPrevious(runStart);
		runEnd = RunFromPosition(end);
		RemoveRunIfEmpty(runEnd);
		return true;
	} else {
		return false;
	}
}

void RunStyles::SetValueAt(int position, int value) {
	int len = 1;
	FillRange(position, value, len);
}

void RunStyles::InsertSpace(int position, int insertLength) {
	int runStart = RunFromPosition(position);
	if (starts->PositionFromPartition(runStart) == position) {
		int runStyle = ValueAt(position);
		// Inserting at start of run so make previous longer
		if (runStart == 0) {
			// Inserting at start of document so ensure 0
			if (runStyle) {
				styles->SetValueAt(0, 0);
				starts->InsertPartition(1, 0);
				styles->InsertValue(1, 1, runStyle);
				starts->InsertText(0, insertLength);
			} else {
				starts->InsertText(runStart, insertLength);
			}
		} else {
			if (runStyle) {
				starts->InsertText(runStart-1, insertLength);
			} else {
				// Insert at end of run so do not extend style
				starts->InsertText(runStart, insertLength);
			}
		}
	} else {
		starts->InsertText(runStart, insertLength);
	}
}

void RunStyles::DeleteAll() {
	delete starts;
	starts = NULL;
	delete styles;
	styles = NULL;
	starts = new Partitioning(8);
	styles = new SplitVector<int>();
	styles->InsertValue(0, 2, 0);
}

void RunStyles::DeleteRange(int position, int deleteLength) {
	int end = position + deleteLength;
	int runStart = RunFromPosition(position);
	int runEnd = RunFromPosition(end);
	if (runStart == runEnd) {
		// Deleting from inside one run
		starts->InsertText(runStart, -deleteLength);
		RemoveRunIfEmpty(runStart);
	} else {
		runStart = SplitRun(position);
		runEnd = SplitRun(end);
		starts->InsertText(runStart, -deleteLength);
		// Remove each old run over the range
		for (int run=runStart; run<runEnd; run++) {
			RemoveRun(runStart);
		}
		RemoveRunIfEmpty(runStart);
		RemoveRunIfSameAsPrevious(runStart);
	}
}

int RunStyles::Runs() const {
	return starts->Partitions();
}

bool RunStyles::AllSame() const {
	for (int run = 1; run < starts->Partitions(); run++) {
		if (styles->ValueAt(run) != styles->ValueAt(run - 1))
			return false;
	}
	return true;
}

bool RunStyles::AllSameAs(int value) const {
	return AllSame() && (styles->ValueAt(0) == value);
}

int RunStyles::Find(int value, int start) const {
	if (start < Length()) {
		int run = start ? RunFromPosition(start) : 0;
		if (styles->ValueAt(run) == value)
			return start;
		run++;
		while (run < starts->Partitions()) {
			if (styles->ValueAt(run) == value)
				return starts->PositionFromPartition(run);
			run++;
		}
	}
	return -1;
}

void RunStyles::Check() const {
	if (Length() < 0) {
		throw std::runtime_error("RunStyles: Length can not be negative.");
	}
	if (starts->Partitions() < 1) {
		throw std::runtime_error("RunStyles: Must always have 1 or more partitions.");
	}
	if (starts->Partitions() != styles->Length()-1) {
		throw std::runtime_error("RunStyles: Partitions and styles different lengths.");
	}
	int start=0;
	while (start < Length()) {
		int end = EndRun(start);
		if (start >= end) {
			throw std::runtime_error("RunStyles: Partition is 0 length.");
		}
		start = end;
	}
	if (styles->ValueAt(styles->Length()-1) != 0) {
		throw std::runtime_error("RunStyles: Unused style at end changed.");
	}
	for (int j=1; j<styles->Length()-1; j++) {
		if (styles->ValueAt(j) == styles->ValueAt(j-1)) {
			throw std::runtime_error("RunStyles: Style of a partition same as previous.");
		}
	}
}
