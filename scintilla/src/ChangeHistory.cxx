// Scintilla source code edit control
/** @file ChangeHistory.cxx
 ** Manages a history of changes in a document.
 **/
// Copyright 2022 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"

#include "Debugging.h"

#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "SparseVector.h"
#include "ChangeHistory.h"

namespace Scintilla::Internal {

void ChangeStack::Clear() noexcept {
	steps.clear();
	changes.clear();
}

void ChangeStack::AddStep() {
	steps.push_back(0);
}

namespace {

constexpr bool InsertionSpanSameDeletion(const ChangeSpan &is, Sci::Position positionDeletion, int edition) {
	// Equal except for count
	return
		is.direction == ChangeSpan::Direction::deletion &&
		is.start == positionDeletion &&
		is.length == 0 &&
		is.edition == edition;
}

}

void ChangeStack::PushDeletion(Sci::Position positionDeletion, const EditionCount &ec) {
	steps.back() += ec.count;
	if (changes.empty() || !InsertionSpanSameDeletion(changes.back(), positionDeletion, ec.edition)) {
		changes.push_back({ positionDeletion, 0, ec.edition, ec.count, ChangeSpan::Direction::deletion });
	} else {
		changes.back().count += ec.count;
	}
}

void ChangeStack::PushInsertion(Sci::Position positionInsertion, Sci::Position length, int edition) {
	steps.back()++;
	changes.push_back({ positionInsertion, length, edition, 1, ChangeSpan::Direction::insertion });
}

int ChangeStack::PopStep() noexcept {
	const int spans = steps.back();
	steps.pop_back();
	return spans;
}

ChangeSpan ChangeStack::PopSpan(int maxSteps) noexcept {
	ChangeSpan span = changes.back();
	const int remove = std::min(maxSteps, span.count);
	if (span.count == remove) {
		changes.pop_back();
	} else {
		changes.back().count -= remove;
		span.count = remove;
	}
	return span;
}

void ChangeStack::SetSavePoint() noexcept {
	// Switch changeUnsaved to changeSaved
	for (ChangeSpan &x : changes) {
		if (x.edition == changeModified) {
			x.edition = changeSaved;
		}
	}
}

void ChangeStack::Check() const noexcept {
#ifdef _DEBUG
	// Ensure count in steps same as insertions;
	int sizeSteps = 0;
	for (const int c : steps) {
		sizeSteps += c;
	}
	int sizeInsertions = 0;
	for (const ChangeSpan &is: changes) {
		sizeInsertions += is.count;
	}
	assert(sizeSteps == sizeInsertions);
#endif
}

void ChangeLog::Clear(Sci::Position length) {
	changeStack.Clear();
	insertEdition.DeleteAll();
	deleteEdition.DeleteAll();
	InsertSpace(0, length);
}

void ChangeLog::InsertSpace(Sci::Position position, Sci::Position insertLength) {
	assert(insertEdition.Length() == deleteEdition.Length());
	insertEdition.InsertSpace(position, insertLength);
	deleteEdition.InsertSpace(position, insertLength);
}

void ChangeLog::DeleteRange(Sci::Position position, Sci::Position deleteLength) {
	insertEdition.DeleteRange(position, deleteLength);
	const EditionSetOwned &editions = deleteEdition.ValueAt(position);
	if (editions) {
		const EditionSet savedEditions = *editions;
		deleteEdition.DeleteRange(position, deleteLength);
		EditionSetOwned reset = std::make_unique<EditionSet>(savedEditions);
		deleteEdition.SetValueAt(position, std::move(reset));
	} else {
		deleteEdition.DeleteRange(position, deleteLength);
	}
	assert(insertEdition.Length() == deleteEdition.Length());
}

void ChangeLog::Insert(Sci::Position start, Sci::Position length, int edition) {
	insertEdition.FillRange(start, edition, length);
}

void ChangeLog::CollapseRange(Sci::Position position, Sci::Position deleteLength) {
	const Sci::Position positionMax = position + deleteLength;
	Sci::Position positionDeletion = position + 1;
	while (positionDeletion <= positionMax) {
		const EditionSetOwned &editions = deleteEdition.ValueAt(positionDeletion);
		if (editions) {
			for (const EditionCount &ec : *editions) {
				PushDeletionAt(position, ec);
			}
			EditionSetOwned empty;
			deleteEdition.SetValueAt(positionDeletion, std::move(empty));
		}
		positionDeletion = deleteEdition.PositionNext(positionDeletion);
	}
}

namespace {

// EditionSets have repeat counts on items so push and pop may just
// manipulate the count field or may push/pop items.

void EditionSetPush(EditionSet &set, EditionCount ec) {
	if (set.empty() || (set.back().edition != ec.edition)) {
		set.push_back(ec);
	} else {
		set.back().count += ec.count;
	}
}

void EditionSetPop(EditionSet &set) noexcept {
	if (set.back().count == 1) {
		set.pop_back();
	} else {
		set.back().count--;
	}
}

int EditionSetCount(const EditionSet &set) noexcept {
	int count = 0;
	for (const EditionCount &ec : set) {
		count += ec.count;
	}
	return count;
}

}

void ChangeLog::PushDeletionAt(Sci::Position position, EditionCount ec) {
	if (!deleteEdition.ValueAt(position)) {
		deleteEdition.SetValueAt(position, std::make_unique<EditionSet>());
	}
	EditionSetPush(*deleteEdition.ValueAt(position), ec);
}

void ChangeLog::InsertFrontDeletionAt(Sci::Position position, EditionCount ec) {
	if (!deleteEdition.ValueAt(position)) {
		deleteEdition.SetValueAt(position, std::make_unique<EditionSet>());
	}
	const EditionSetOwned &editions = deleteEdition.ValueAt(position);
	editions->insert(editions->begin(), ec);
}

void ChangeLog::SaveRange(Sci::Position position, Sci::Position length) {
	// Save insertEdition range into undo stack
	changeStack.AddStep();
	Sci::Position positionInsertion = position;
	const ptrdiff_t editionStart = insertEdition.ValueAt(positionInsertion);
	if (editionStart == 0) {
		positionInsertion = insertEdition.EndRun(positionInsertion);
	}
	const Sci::Position positionMax = position + length;
	while (positionInsertion < positionMax) {
		const Sci::Position positionEndInsertion = insertEdition.EndRun(positionInsertion);
		changeStack.PushInsertion(positionInsertion, std::min(positionEndInsertion, positionMax) - positionInsertion,
			insertEdition.ValueAt(positionInsertion));
		positionInsertion = insertEdition.EndRun(positionEndInsertion);
	}
	Sci::Position positionDeletion = position + 1;
	while (positionDeletion <= positionMax) {
		const EditionSetOwned &editions = deleteEdition.ValueAt(positionDeletion);
		if (editions) {
			for (const EditionCount &ec : *editions) {
				changeStack.PushDeletion(positionDeletion, ec);
			}
		}
		positionDeletion = deleteEdition.PositionNext(positionDeletion);
	}
}

void ChangeLog::PopDeletion(Sci::Position position, Sci::Position deleteLength) {
	// Just performed InsertSpace(position, deleteLength) so *this* element in
	// deleteEdition moved forward by deleteLength
	EditionSetOwned eso = deleteEdition.Extract(position + deleteLength);
	deleteEdition.SetValueAt(position, std::move(eso));
	const EditionSetOwned &editions = deleteEdition.ValueAt(position);
	assert(editions);
	EditionSetPop(*editions);
	const int inserts = changeStack.PopStep();
	for (int i = 0; i < inserts;) {
		const ChangeSpan span = changeStack.PopSpan(inserts);
		if (span.direction == ChangeSpan::Direction::insertion) {
			assert(span.count == 1);	// Insertions are never compressed
			insertEdition.FillRange(span.start, span.edition, span.length);
			i++;
		} else {
			assert(editions);
			assert(editions->back().edition == span.edition);
			for (int j = 0; j < span.count; j++) {
				EditionSetPop(*editions);
			}
			// Iterating backwards (pop) through changeStack, reverse order of insertion
			// and original deletion list.
			// Therefore need to insert at front to recreate original order.
			InsertFrontDeletionAt(span.start, { span.edition, span.count });
			i += span.count;
		}
	}

	if (editions->empty()) {
		deleteEdition.SetValueAt(position, EditionSetOwned{});
	}
}

void ChangeLog::SaveHistoryForDelete(Sci::Position position, Sci::Position deleteLength) {
	assert(position >= 0);
	assert(deleteLength >= 0);
	assert(position + deleteLength <= Length());
	SaveRange(position, deleteLength);
	CollapseRange(position, deleteLength);
}

void ChangeLog::DeleteRangeSavingHistory(Sci::Position position, Sci::Position deleteLength) {
	SaveHistoryForDelete(position, deleteLength);
	DeleteRange(position, deleteLength);
}

void ChangeLog::SetSavePoint() {
	// Switch changeUnsaved to changeSaved
	changeStack.SetSavePoint();

	const Sci::Position length = insertEdition.Length();

	for (Sci::Position startRun = 0; startRun < length;) {
		const Sci::Position endRun = insertEdition.EndRun(startRun);
		if (insertEdition.ValueAt(startRun) == changeModified) {
			insertEdition.FillRange(startRun, changeSaved, endRun - startRun);
		}
		startRun = endRun;
	}

	for (Sci::Position positionDeletion = 0; positionDeletion <= length;) {
		const EditionSetOwned &editions = deleteEdition.ValueAt(positionDeletion);
		if (editions) {
			for (EditionCount &ec : *editions) {
				if (ec.edition == changeModified) {
					ec.edition = changeSaved;
				}
			}
		}
		positionDeletion = deleteEdition.PositionNext(positionDeletion);
	}
}

Sci::Position ChangeLog::Length() const noexcept {
	return insertEdition.Length();
}

size_t ChangeLog::DeletionCount(Sci::Position start, Sci::Position length) const noexcept {
	const Sci::Position end = start + length;
	size_t count = 0;
	while (start <= end) {
		const EditionSetOwned &editions = deleteEdition.ValueAt(start);
		if (editions) {
			count += EditionSetCount(*editions);
		}
		start = deleteEdition.PositionNext(start);
	}
	return count;
}

void ChangeLog::Check() const noexcept {
	assert(insertEdition.Length() == deleteEdition.Length());
	changeStack.Check();
}

ChangeHistory::ChangeHistory(Sci::Position length) {
	changeLog.Clear(length);
}

void ChangeHistory::Insert(Sci::Position position, Sci::Position insertLength, bool collectingUndo, bool beforeSave) {
	Check();
	changeLog.InsertSpace(position, insertLength);
	const int edition = collectingUndo ? (beforeSave ? changeSaved : changeModified) :
		changeOriginal;
	changeLog.Insert(position, insertLength, edition);
	if (changeLogReversions) {
		changeLogReversions->InsertSpace(position, insertLength);
		if (beforeSave) {
			changeLogReversions->PopDeletion(position, insertLength);
		}
	}
	Check();
}

void ChangeHistory::DeleteRange(Sci::Position position, Sci::Position deleteLength, bool reverting) {
	Check();
	assert(DeletionCount(position, deleteLength-1) == 0);
	changeLog.DeleteRange(position, deleteLength);
	if (changeLogReversions) {
		changeLogReversions->DeleteRangeSavingHistory(position, deleteLength);
		if (reverting) {
			changeLogReversions->PushDeletionAt(position, { changeRevertedOriginal, 1 });
		}
	}
	Check();
}

void ChangeHistory::DeleteRangeSavingHistory(Sci::Position position, Sci::Position deleteLength, bool beforeSave, bool isDetached) {
	changeLog.DeleteRangeSavingHistory(position, deleteLength);
	changeLog.PushDeletionAt(position, { beforeSave ? changeSaved : changeModified, 1 });
	if (changeLogReversions) {
		if (isDetached) {
			changeLogReversions->SaveHistoryForDelete(position, deleteLength);
		}
		changeLogReversions->DeleteRange(position, deleteLength);
	}
	Check();
}

void ChangeHistory::StartReversion() {
	if (!changeLogReversions) {
		changeLogReversions = std::make_unique<ChangeLog>();
		changeLogReversions->Clear(changeLog.Length());
	}
	Check();
}

void ChangeHistory::EndReversion() noexcept {
	changeLogReversions.reset();
	Check();
}

void ChangeHistory::SetSavePoint() {
	changeLog.SetSavePoint();
	EndReversion();
}

void ChangeHistory::UndoDeleteStep(Sci::Position position, Sci::Position deleteLength, bool isDetached) {
	Check();
	changeLog.InsertSpace(position, deleteLength);
	changeLog.PopDeletion(position, deleteLength);
	if (changeLogReversions) {
		changeLogReversions->InsertSpace(position, deleteLength);
		if (!isDetached) {
			changeLogReversions->Insert(position, deleteLength, 1);
		}
	}
	Check();
}

Sci::Position ChangeHistory::Length() const noexcept {
	return changeLog.Length();
}

void ChangeHistory::SetEpoch(int epoch) noexcept {
	historicEpoch = epoch;
}

void ChangeHistory::EditionCreateHistory(Sci::Position start, Sci::Position length) {
	if (start <= changeLog.Length()) {
		if (length) {
			changeLog.insertEdition.FillRange(start, historicEpoch, length);
		} else {
			changeLog.PushDeletionAt(start, { historicEpoch, 1 });
		}
	}
}

// Editions:
// <0 History
// 0 Original unchanged
// 1 Reverted to origin
// 2 Saved
// 3 Unsaved
// 4 Reverted to change
int ChangeHistory::EditionAt(Sci::Position pos) const noexcept {
	const int edition = changeLog.insertEdition.ValueAt(pos);
	if (changeLogReversions) {
		const int editionReversion = changeLogReversions->insertEdition.ValueAt(pos);
		if (editionReversion) {
			if (edition < 0)	// Historical revision
				return changeRevertedOriginal;
			return edition ? changeRevertedToChange : changeRevertedOriginal;
		}
	}
	return edition;
}

Sci::Position ChangeHistory::EditionEndRun(Sci::Position pos) const noexcept {
	if (changeLogReversions) {
		assert(changeLogReversions->Length() == changeLog.Length());
		const Sci::Position nextReversion = changeLogReversions->insertEdition.EndRun(pos);
		const Sci::Position next = changeLog.insertEdition.EndRun(pos);
		return std::min(next, nextReversion);
	}
	return changeLog.insertEdition.EndRun(pos);
}

// Produce a 4-bit value from the deletions at a position
unsigned int ChangeHistory::EditionDeletesAt(Sci::Position pos) const noexcept {
	unsigned int editionSet = 0;
	const EditionSetOwned &editionSetDeletions = changeLog.deleteEdition.ValueAt(pos);
	if (editionSetDeletions) {
		for (const EditionCount &ec : *editionSetDeletions) {
			editionSet = editionSet | (1u << (ec.edition-1));
		}
	}
	if (changeLogReversions) {
		const EditionSetOwned &editionSetReversions = changeLogReversions->deleteEdition.ValueAt(pos);
		if (editionSetReversions) {
			// If there is no saved or modified -> revertedToOrigin
			if (!(editionSet & (bitSaved | bitModified))) {
				editionSet = editionSet | bitRevertedToOriginal;
			} else {
				editionSet = editionSet | bitRevertedToModified;
			}
		}
	}
	return editionSet;
}

Sci::Position ChangeHistory::EditionNextDelete(Sci::Position pos) const noexcept {
	const Sci::Position next = changeLog.deleteEdition.PositionNext(pos);
	if (changeLogReversions) {
		const Sci::Position nextReversion = changeLogReversions->deleteEdition.PositionNext(pos);
		return std::min(next, nextReversion);
	}
	return next;
}

size_t ChangeHistory::DeletionCount(Sci::Position start, Sci::Position length) const noexcept {
	return changeLog.DeletionCount(start, length);
}

EditionSet ChangeHistory::DeletionsAt(Sci::Position pos) const {
	const EditionSetOwned &editions = changeLog.deleteEdition.ValueAt(pos);
	if (editions) {
		return *editions;
	}
	return {};
}

void ChangeHistory::Check() noexcept {
	changeLog.Check();
	if (changeLogReversions) {
		changeLogReversions->Check();
		assert(changeLogReversions->Length() == changeLog.Length());
	}
}

}
