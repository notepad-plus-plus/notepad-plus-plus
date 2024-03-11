// Scintilla source code edit control
/** @file UndoHistory.cxx
 ** Manages undo for the document.
 **/
// Copyright 1998-2024 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <climits>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
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
#include "CellBuffer.h"
#include "UndoHistory.h"

namespace Scintilla::Internal {

template <typename T>
void VectorTruncate(std::vector<T> &v, size_t length) noexcept {
	v.erase(v.begin() + length, v.end());
}

constexpr size_t byteMask = UINT8_MAX;
constexpr size_t byteBits = 8;

size_t ReadValue(const uint8_t *bytes, size_t length) noexcept {
	size_t value = 0;
	for (size_t i = 0; i < length; i++) {
		value = (value << byteBits) + bytes[i];
	}
	return value;
}

void WriteValue(uint8_t *bytes, size_t length, size_t value) noexcept {
	for (size_t i = 0; i < length; i++) {
		bytes[length - i - 1] = value & byteMask;
		value = value >> byteBits;
	}
}

size_t ScaledVector::Size() const noexcept {
	return bytes.size() / element.size;
}

size_t ScaledVector::ValueAt(size_t index) const noexcept {
	return ReadValue(bytes.data() + index * element.size, element.size);
}

intptr_t ScaledVector::SignedValueAt(size_t index) const noexcept {
	return ReadValue(bytes.data() + index * element.size, element.size);
}

constexpr SizeMax ElementForValue(size_t value) noexcept {
	size_t maxN = byteMask;
	size_t i = 1;
	while (value > byteMask) {
		i++;
		value >>= byteBits;
		maxN = (maxN << byteBits) + byteMask;
	}
	return { i, maxN };
}

void ScaledVector::SetValueAt(size_t index, size_t value) {
	// Check if value fits, if not then expand
	if (value > element.maxValue) {
		const SizeMax elementForValue = ElementForValue(value);
		const size_t length = bytes.size() / element.size;
		std::vector<uint8_t> bytesNew(elementForValue.size * length);
		for (size_t i = 0; i < length; i++) {
			const uint8_t *source = bytes.data() + i * element.size;
			uint8_t *destination = bytesNew.data() + (i+1) * elementForValue.size - element.size;
			memcpy(destination, source, element.size);
		}
		std::swap(bytes, bytesNew);
		element = elementForValue;
	}
	WriteValue(bytes.data() + index * element.size, element.size, value);
}

void ScaledVector::ClearValueAt(size_t index) noexcept {
	// 0 fits in any size element so no expansion needed so no exceptions
	WriteValue(bytes.data() + index * element.size, element.size, 0);
}

void ScaledVector::Clear() noexcept {
	bytes.clear();
}

void ScaledVector::Truncate(size_t length) noexcept {
	VectorTruncate(bytes, length * element.size);
	assert(bytes.size() == length * element.size);
}

void ScaledVector::ReSize(size_t length) {
	bytes.resize(length * element.size);
}

void ScaledVector::PushBack() {
	ReSize(Size() + 1);
}

size_t ScaledVector::SizeInBytes() const noexcept {
	return bytes.size();
}

UndoActionType::UndoActionType() noexcept : at(ActionType::insert), mayCoalesce(false) {
}

UndoActions::UndoActions() noexcept = default;

void UndoActions::Truncate(size_t length) noexcept {
	VectorTruncate(types, length);
	assert(types.size() == length);
	positions.Truncate(length);
	lengths.Truncate(length);
}

void UndoActions::PushBack() {
	types.emplace_back();
	positions.PushBack();
	lengths.PushBack();
}

void UndoActions::Clear() noexcept {
	types.clear();
	positions.Clear();
	lengths.Clear();
}

intptr_t UndoActions::SSize() const noexcept {
	return types.size();
}

void UndoActions::Create(size_t index, ActionType at_, Sci::Position position_, Sci::Position lenData_, bool mayCoalesce_) {
	types[index].at = at_;
	types[index].mayCoalesce = mayCoalesce_;
	positions.SetValueAt(index, position_);
	lengths.SetValueAt(index, lenData_);
}

bool UndoActions::AtStart(size_t index) const noexcept {
	if (index == 0) {
		return true;
	}
	return !types[index-1].mayCoalesce;
}

size_t UndoActions::LengthTo(size_t index) const noexcept {
	size_t sum = 0;
	for (size_t act = 0; act < index; act++) {
		sum += lengths.ValueAt(act);
	}
	return sum;
}

void ScrapStack::Clear() noexcept {
	stack.clear();
	current = 0;
}

const char *ScrapStack::Push(const char *text, size_t length) {
	if (current < stack.length()) {
		stack.resize(current);
	}
	stack.append(text, length);
	current = stack.length();
	return stack.data() + current - length;
}

void ScrapStack::SetCurrent(size_t position) noexcept {
	current = position;
}

void ScrapStack::MoveForward(size_t length) noexcept {
	if ((current + length) <= stack.length()) {
		current += length;
	}
}

void ScrapStack::MoveBack(size_t length) noexcept {
	if (current >= length) {
		current -= length;
	}
}

const char *ScrapStack::CurrentText() const noexcept {
	return stack.data() + current;
}

const char *ScrapStack::TextAt(size_t position) const noexcept {
	return stack.data() + position;
}

// The undo history stores a sequence of user operations that represent the user's view of the
// commands executed on the text.
// Each user operation contains a sequence of text insertion and text deletion actions.
// All the user operations are stored in a list of individual actions.
// A false 'mayCoalesce' flag acts as an end to a user operation.
// Initially there are no actions in the history.
// As each action is performed, it is recorded in the history. The action may either become
// part of the current user operation or may start a new user operation. If it is to be part of the
// current operation, then 'mayCoalesce' is true. If it is to be part of a new operation, the
// 'mayCoalesce' flag of the previous action is set false.
// The decision of whether to start a new user operation is based upon two factors. If a
// compound operation has been explicitly started by calling BeginUndoAction and no matching
// EndUndoAction (these calls nest) has been called, then the action is coalesced into the current
// operation. If there is no outstanding BeginUndoAction call then a new operation is started
// unless it looks as if the new action is caused by the user typing or deleting a stream of text.
// Sequences that look like typing or deletion are coalesced into a single user operation.

int UndoHistory::PreviousAction() const noexcept {
	return currentAction - 1;
}

UndoHistory::UndoHistory() {
	scraps = std::make_unique<ScrapStack>();
}

UndoHistory::~UndoHistory() noexcept = default;

const char *UndoHistory::AppendAction(ActionType at, Sci::Position position, const char *data, Sci::Position lengthData,
	bool &startSequence, bool mayCoalesce) {
	//Platform::DebugPrintf("%% %d action %d %d %d\n", at, position, lengthData, currentAction);
	//Platform::DebugPrintf("^ %d action %d %d\n", actions[currentAction - 1].at,
	//	actions[currentAction - 1].position, actions[currentAction - 1].lenData);
	if (currentAction < savePoint) {
		savePoint = -1;
		if (!detach) {
			detach = currentAction;
		}
	} else if (detach && (*detach > currentAction)) {
		detach = currentAction;
	}
	if (undoSequenceDepth > 0) {
		// Actions not at top level are always coalesced unless this is after return to top level
		mayCoalesce = true;
	}
	bool coalesce = true;
	if (currentAction >= 1) {
		int targetAct = currentAction - 1;
		if (0 == undoSequenceDepth) {
			// Top level actions may not always be coalesced
			// Container actions may forward the coalesce state of Scintilla Actions.
			while ((targetAct > 0) && (actions.types[targetAct].at == ActionType::container) && actions.types[targetAct].mayCoalesce) {
				targetAct--;
			}
			// See if current action can be coalesced into previous action
			// Will work if both are inserts or deletes and position is same
			if ((currentAction == savePoint) || (currentAction == tentativePoint)) {
				coalesce = false;
			} else if (!mayCoalesce || !actions.types[targetAct].mayCoalesce) {
				coalesce = false;
			} else if (at == ActionType::container || actions.types[targetAct].at == ActionType::container) {
				;	// A coalescible containerAction
			} else if ((at != actions.types[targetAct].at)) { // } && (!actions.AtStart(targetAct))) {
				coalesce = false;
			} else if ((at == ActionType::insert) &&
			           (position != (actions.positions.SignedValueAt(targetAct) + actions.lengths.SignedValueAt(targetAct)))) {
				// Insertions must be immediately after to coalesce
				coalesce = false;
			} else if (at == ActionType::remove) {
				if ((lengthData == 1) || (lengthData == 2)) {
					if ((position + lengthData) == actions.positions.SignedValueAt(targetAct)) {
						; // Backspace -> OK
					} else if (position == actions.positions.SignedValueAt(targetAct)) {
						; // Delete -> OK
					} else {
						// Removals must be at same position to coalesce
						coalesce = false;
					}
				} else {
					// Removals must be of one character to coalesce
					coalesce = false;
				}
			} else {
				// Action coalesced.
			}

		} else {
			// Actions not at top level are always coalesced unless this is after return to top level
			if (!actions.types[targetAct].mayCoalesce)
				coalesce = false;
		}
	} else {
		coalesce = false;
	}
	startSequence = !coalesce;
	// Maybe close previous action
	if ((currentAction > 0) && startSequence) {
		actions.types[PreviousAction()].mayCoalesce = false;
	}
	const char *dataNew = lengthData ? scraps->Push(data, lengthData) : nullptr;
	if (currentAction >= actions.SSize()) {
		actions.PushBack();
	} else {
		actions.Truncate(currentAction+1);
	}
	actions.Create(currentAction, at, position, lengthData, mayCoalesce);
	currentAction++;
	return dataNew;
}

void UndoHistory::BeginUndoAction(bool mayCoalesce) noexcept {
	if (undoSequenceDepth == 0) {
		if (currentAction > 0) {
			actions.types[PreviousAction()].mayCoalesce = mayCoalesce;
		}
	}
	undoSequenceDepth++;
}

void UndoHistory::EndUndoAction() noexcept {
	PLATFORM_ASSERT(undoSequenceDepth > 0);
	undoSequenceDepth--;
	if (0 == undoSequenceDepth) {
		if (currentAction > 0) {
			actions.types[PreviousAction()].mayCoalesce = false;
		}
	}
}

void UndoHistory::DropUndoSequence() noexcept {
	undoSequenceDepth = 0;
}

void UndoHistory::DeleteUndoHistory() noexcept {
	actions.Clear();
	currentAction = 0;
	savePoint = 0;
	tentativePoint = -1;
	scraps->Clear();
	memory = {};
}

int UndoHistory::Actions() const noexcept {
	return static_cast<int>(actions.SSize());
}

void UndoHistory::SetSavePoint(int action) noexcept {
	savePoint = action;
}

int UndoHistory::SavePoint() const noexcept {
	return savePoint;
}

void UndoHistory::SetSavePoint() noexcept {
	savePoint = currentAction;
	detach.reset();
}

bool UndoHistory::IsSavePoint() const noexcept {
	return savePoint == currentAction;
}

bool UndoHistory::BeforeSavePoint() const noexcept {
	return (savePoint < 0) || (savePoint > currentAction);
}

bool UndoHistory::PreviousBeforeSavePoint() const noexcept {
	return (savePoint < 0) || (savePoint >= currentAction);
}

bool UndoHistory::BeforeReachableSavePoint() const noexcept {
	return (savePoint > 0) && (savePoint > currentAction);
}

bool UndoHistory::AfterSavePoint() const noexcept {
	return (savePoint >= 0) && (savePoint <= currentAction);
}

void UndoHistory::SetDetachPoint(int action) noexcept {
	if (action == -1) {
		detach = {};
	} else {
		detach = action;
	}
}

int UndoHistory::DetachPoint() const noexcept {
	return detach.value_or(-1);
}

bool UndoHistory::AfterDetachPoint() const noexcept {
	return detach && (*detach < currentAction);
}

bool UndoHistory::AfterOrAtDetachPoint() const noexcept {
	return detach && (*detach <= currentAction);
}

intptr_t UndoHistory::Delta(int action) noexcept {
	intptr_t sizeChange = 0;
	for (int act = 0; act < action; act++) {
		const intptr_t lengthChange = actions.lengths.SignedValueAt(act);
		sizeChange += (actions.types[act].at == ActionType::insert) ? lengthChange : -lengthChange;
	}
	return sizeChange;
}

bool UndoHistory::Validate(intptr_t lengthDocument) noexcept {
	// Check history for validity
	const intptr_t sizeChange = Delta(currentAction);
	if (sizeChange > lengthDocument) {
		// Current document size too small for changes made in undo history.
		return false;
	}
	const intptr_t lengthOriginal = lengthDocument - sizeChange;
	intptr_t lengthCurrent = lengthOriginal;
	for (int act = 0; act < actions.SSize(); act++) {
		const intptr_t lengthChange = actions.lengths.SignedValueAt(act);
		if (actions.positions.SignedValueAt(act) > lengthCurrent) {
			// Change outside document.
			return false;
		}
		lengthCurrent += (actions.types[act].at == ActionType::insert) ? lengthChange : -lengthChange;
		if (lengthCurrent < 0) {
			return false;
		}
	}
	return true;
}

void UndoHistory::SetCurrent(int action, intptr_t lengthDocument) {
	// Find position in scraps for action
	memory = {};
	const size_t lengthSum = actions.LengthTo(action);
	scraps->SetCurrent(lengthSum);
	currentAction = action;
	if (!Validate(lengthDocument)) {
		currentAction = 0;
		DeleteUndoHistory();
		throw std::runtime_error("UndoHistory::SetCurrent: invalid undo history.");
	}
}

int UndoHistory::Current() const noexcept {
	return currentAction;
}

int UndoHistory::Type(int action) const noexcept {
	const int baseType = static_cast<int>(actions.types[action].at);
	const int open = actions.types[action].mayCoalesce ? coalesceFlag : 0;
	return baseType | open;
}

Sci::Position UndoHistory::Position(int action) const noexcept {
	return actions.positions.SignedValueAt(action);
}

Sci::Position UndoHistory::Length(int action) const noexcept {
	return actions.lengths.SignedValueAt(action);
}

std::string_view UndoHistory::Text(int action) noexcept {
	// Assumes first call after any changes is for action 0.
	// TODO: may need to invalidate memory in other circumstances
	if (action == 0) {
		memory = {};
	}
	int act = 0;
	size_t position = 0;
	if (memory && memory->act <= action) {
		act = memory->act;
		position = memory->position;
	}
	for (; act < action; act++) {
		position += actions.lengths.ValueAt(act);
	}
	const size_t length = actions.lengths.ValueAt(action);
	const char *scrap = scraps->TextAt(position);
	memory = {action, position};
	return {scrap, length};
}

void UndoHistory::PushUndoActionType(int type, Sci::Position position) {
	actions.PushBack();
	actions.Create(actions.SSize()-1, static_cast<ActionType>(type & byteMask),
		position, 0, type & coalesceFlag);
}

void UndoHistory::ChangeLastUndoActionText(size_t length, const char *text) {
	assert(actions.lengths.ValueAt(actions.SSize()-1) == 0);
	actions.lengths.SetValueAt(actions.SSize()-1, length);
	scraps->Push(text, length);
}

void UndoHistory::SetTentative(int action) noexcept {
	tentativePoint = action;
}

int UndoHistory::TentativePoint() const noexcept {
	return tentativePoint;
}

void UndoHistory::TentativeStart() noexcept {
	tentativePoint = currentAction;
}

void UndoHistory::TentativeCommit() noexcept {
	tentativePoint = -1;
	// Truncate undo history
	actions.Truncate(currentAction);
}

bool UndoHistory::TentativeActive() const noexcept {
	return tentativePoint >= 0;
}

int UndoHistory::TentativeSteps() const noexcept {
	// Drop any trailing startAction
	if (tentativePoint >= 0)
		return currentAction - tentativePoint;
	return -1;
}

bool UndoHistory::CanUndo() const noexcept {
	return (currentAction > 0) && (actions.SSize() != 0);
}

int UndoHistory::StartUndo() const noexcept {
	assert(currentAction >= 0);

	// Count the steps in this action
	if (currentAction == 0) {
		return 0;
	}

	int act = currentAction - 1;

	while (act > 0 && !actions.AtStart(act)) {
		act--;
	}
	return currentAction - act;
}

Action UndoHistory::GetUndoStep() const noexcept {
	const int previousAction = PreviousAction();
	Action acta {
		actions.types[previousAction].at,
		actions.types[previousAction].mayCoalesce,
		actions.positions.SignedValueAt(previousAction),
		nullptr,
		actions.lengths.SignedValueAt(previousAction)
	};
	if (acta.lenData) {
		acta.data = scraps->CurrentText() - acta.lenData;
	}
	return acta;
}

void UndoHistory::CompletedUndoStep() noexcept {
	scraps->MoveBack(actions.lengths.ValueAt(PreviousAction()));
	currentAction--;
}

bool UndoHistory::CanRedo() const noexcept {
	return actions.SSize() > currentAction;
}

int UndoHistory::StartRedo() const noexcept {
	// Count the steps in this action

	if (currentAction >= actions.SSize()) {
		// Already at end so can't redo
		return 0;
	}

	// Slightly unusual logic handles case where last action still has mayCoalesce.
	// Could set mayCoalesce of last action to false in StartUndo but this state is
	// visible to applications so should not be changed.
	const int maxAction = Actions() - 1;
	int act = currentAction;
	while (act <= maxAction && actions.types[act].mayCoalesce) {
		act++;
	}
	act = std::min(act, maxAction);
	return act - currentAction + 1;
}

Action UndoHistory::GetRedoStep() const noexcept {
	Action acta{
		actions.types[currentAction].at,
		actions.types[currentAction].mayCoalesce,
		actions.positions.SignedValueAt(currentAction),
		nullptr,
		actions.lengths.SignedValueAt(currentAction)
	};
	if (acta.lenData) {
		acta.data = scraps->CurrentText();
	}
	return acta;
}

void UndoHistory::CompletedRedoStep() noexcept {
	scraps->MoveForward(actions.lengths.ValueAt(currentAction));
	currentAction++;
}

}
