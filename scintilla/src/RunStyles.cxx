/** @file RunStyles.cxx
 ** Data structure used to store sparse styles.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <climits>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "Debugging.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"

using namespace Scintilla::Internal;

// Find the first run at a position
template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::RunFromPosition(DISTANCE position) const noexcept {
	DISTANCE run = starts->PartitionFromPosition(position);
	// Go to first element with this position
	while ((run > 0) && (position == starts->PositionFromPartition(run-1))) {
		run--;
	}
	return run;
}

// If there is no run boundary at position, insert one continuing style.
template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::SplitRun(DISTANCE position) {
	DISTANCE run = RunFromPosition(position);
	const DISTANCE posRun = starts->PositionFromPartition(run);
	if (posRun < position) {
		STYLE runStyle = ValueAt(position);
		run++;
		starts->InsertPartition(run, position);
		styles->InsertValue(run, 1, runStyle);
	}
	return run;
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::RemoveRun(DISTANCE run) {
	starts->RemovePartition(run);
	styles->DeleteRange(run, 1);
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::RemoveRunIfEmpty(DISTANCE run) {
	if ((run < starts->Partitions()) && (starts->Partitions() > 1)) {
		if (starts->PositionFromPartition(run) == starts->PositionFromPartition(run+1)) {
			RemoveRun(run);
		}
	}
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::RemoveRunIfSameAsPrevious(DISTANCE run) {
	if ((run > 0) && (run < starts->Partitions())) {
		if (styles->ValueAt(run-1) == styles->ValueAt(run)) {
			RemoveRun(run);
		}
	}
}

template <typename DISTANCE, typename STYLE>
RunStyles<DISTANCE, STYLE>::RunStyles() {
	starts = std::make_unique<Partitioning<DISTANCE>>(8);
	styles = std::make_unique<SplitVector<STYLE>>();
	styles->InsertValue(0, 2, 0);
}

template <typename DISTANCE, typename STYLE>
RunStyles<DISTANCE, STYLE>::~RunStyles() {
}

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::Length() const noexcept {
	return starts->PositionFromPartition(starts->Partitions());
}

template <typename DISTANCE, typename STYLE>
STYLE RunStyles<DISTANCE, STYLE>::ValueAt(DISTANCE position) const noexcept {
	return styles->ValueAt(starts->PartitionFromPosition(position));
}

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::FindNextChange(DISTANCE position, DISTANCE end) const noexcept {
	const DISTANCE run = starts->PartitionFromPosition(position);
	if (run < starts->Partitions()) {
		const DISTANCE runChange = starts->PositionFromPartition(run);
		if (runChange > position)
			return runChange;
		const DISTANCE nextChange = starts->PositionFromPartition(run + 1);
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

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::StartRun(DISTANCE position) const noexcept {
	return starts->PositionFromPartition(starts->PartitionFromPosition(position));
}

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::EndRun(DISTANCE position) const noexcept {
	return starts->PositionFromPartition(starts->PartitionFromPosition(position) + 1);
}

template <typename DISTANCE, typename STYLE>
FillResult<DISTANCE> RunStyles<DISTANCE, STYLE>::FillRange(DISTANCE position, STYLE value, DISTANCE fillLength) {
	const FillResult<DISTANCE> resultNoChange{false, position, fillLength};
	if (fillLength <= 0) {
		return resultNoChange;
	}
	DISTANCE end = position + fillLength;
	if (end > Length()) {
		return resultNoChange;
	}
	DISTANCE runEnd = RunFromPosition(end);
	if (styles->ValueAt(runEnd) == value) {
		// End already has value so trim range.
		end = starts->PositionFromPartition(runEnd);
		if (position >= end) {
			// Whole range is already same as value so no action
			return resultNoChange;
		}
		fillLength = end - position;
	} else {
		runEnd = SplitRun(end);
	}
	DISTANCE runStart = RunFromPosition(position);
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
		const FillResult<DISTANCE> result{ true, position, fillLength };
		styles->SetValueAt(runStart, value);
		// Remove each old run over the range
		for (DISTANCE run=runStart+1; run<runEnd; run++) {
			RemoveRun(runStart+1);
		}
		runEnd = RunFromPosition(end);
		RemoveRunIfSameAsPrevious(runEnd);
		RemoveRunIfSameAsPrevious(runStart);
		runEnd = RunFromPosition(end);
		RemoveRunIfEmpty(runEnd);
		return result;
	} else {
		return resultNoChange;
	}
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::SetValueAt(DISTANCE position, STYLE value) {
	FillRange(position, value, 1);
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::InsertSpace(DISTANCE position, DISTANCE insertLength) {
	DISTANCE runStart = RunFromPosition(position);
	if (starts->PositionFromPartition(runStart) == position) {
		STYLE runStyle = ValueAt(position);
		// Inserting at start of run so make previous longer
		if (runStart == 0) {
			// Inserting at start of document so ensure 0
			if (runStyle) {
				styles->SetValueAt(0, STYLE());
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

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::DeleteAll() {
	starts = std::make_unique<Partitioning<DISTANCE>>(8);
	styles = std::make_unique<SplitVector<STYLE>>();
	styles->InsertValue(0, 2, 0);
}

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::DeleteRange(DISTANCE position, DISTANCE deleteLength) {
	DISTANCE end = position + deleteLength;
	DISTANCE runStart = RunFromPosition(position);
	DISTANCE runEnd = RunFromPosition(end);
	if (runStart == runEnd) {
		// Deleting from inside one run
		starts->InsertText(runStart, -deleteLength);
		RemoveRunIfEmpty(runStart);
	} else {
		runStart = SplitRun(position);
		runEnd = SplitRun(end);
		starts->InsertText(runStart, -deleteLength);
		// Remove each old run over the range
		for (DISTANCE run=runStart; run<runEnd; run++) {
			RemoveRun(runStart);
		}
		RemoveRunIfEmpty(runStart);
		RemoveRunIfSameAsPrevious(runStart);
	}
}

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::Runs() const noexcept {
	return starts->Partitions();
}

template <typename DISTANCE, typename STYLE>
bool RunStyles<DISTANCE, STYLE>::AllSame() const noexcept {
	for (DISTANCE run = 1; run < starts->Partitions(); run++) {
		if (styles->ValueAt(run) != styles->ValueAt(run - 1))
			return false;
	}
	return true;
}

template <typename DISTANCE, typename STYLE>
bool RunStyles<DISTANCE, STYLE>::AllSameAs(STYLE value) const noexcept {
	return AllSame() && (styles->ValueAt(0) == value);
}

template <typename DISTANCE, typename STYLE>
DISTANCE RunStyles<DISTANCE, STYLE>::Find(STYLE value, DISTANCE start) const noexcept {
	if (start < Length()) {
		DISTANCE run = start ? RunFromPosition(start) : 0;
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

template <typename DISTANCE, typename STYLE>
void RunStyles<DISTANCE, STYLE>::Check() const {
	if (Length() < 0) {
		throw std::runtime_error("RunStyles: Length can not be negative.");
	}
	if (starts->Partitions() < 1) {
		throw std::runtime_error("RunStyles: Must always have 1 or more partitions.");
	}
	if (starts->Partitions() != styles->Length()-1) {
		throw std::runtime_error("RunStyles: Partitions and styles different lengths.");
	}
	DISTANCE start=0;
	while (start < Length()) {
		const DISTANCE end = EndRun(start);
		if (start >= end) {
			throw std::runtime_error("RunStyles: Partition is 0 length.");
		}
		start = end;
	}
	if (styles->ValueAt(styles->Length()-1) != 0) {
		throw std::runtime_error("RunStyles: Unused style at end changed.");
	}
	for (ptrdiff_t j=1; j<styles->Length()-1; j++) {
		if (styles->ValueAt(j) == styles->ValueAt(j-1)) {
			throw std::runtime_error("RunStyles: Style of a partition same as previous.");
		}
	}
}

template class Scintilla::Internal::RunStyles<int, int>;
template class Scintilla::Internal::RunStyles<int, char>;
#if (PTRDIFF_MAX != INT_MAX) || defined(__HAIKU__)
template class Scintilla::Internal::RunStyles<ptrdiff_t, int>;
template class Scintilla::Internal::RunStyles<ptrdiff_t, char>;
#endif
