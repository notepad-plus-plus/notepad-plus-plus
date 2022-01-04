// Scintilla source code edit control
/** @file CellBuffer.cxx
 ** Manages a buffer of cells.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

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
#include "CellBuffer.h"
#include "UniConversion.h"

namespace Scintilla::Internal {

struct CountWidths {
	// Measures the number of characters in a string divided into those
	// from the Base Multilingual Plane and those from other planes.
	Sci::Position countBasePlane;
	Sci::Position countOtherPlanes;
	explicit CountWidths(Sci::Position countBasePlane_=0, Sci::Position countOtherPlanes_=0) noexcept :
		countBasePlane(countBasePlane_),
		countOtherPlanes(countOtherPlanes_) {
	}
	CountWidths operator-() const noexcept {
		return CountWidths(-countBasePlane , -countOtherPlanes);
	}
	Sci::Position WidthUTF32() const noexcept {
		// All code points take one code unit in UTF-32.
		return countBasePlane + countOtherPlanes;
	}
	Sci::Position WidthUTF16() const noexcept {
		// UTF-16 takes 2 code units for other planes
		return countBasePlane + 2 * countOtherPlanes;
	}
	void CountChar(int lenChar) noexcept {
		if (lenChar == 4) {
			countOtherPlanes++;
		} else {
			countBasePlane++;
		}
	}
};

class ILineVector {
public:
	virtual void Init() = 0;
	virtual void SetPerLine(PerLine *pl) noexcept = 0;
	virtual void InsertText(Sci::Line line, Sci::Position delta) noexcept = 0;
	virtual void InsertLine(Sci::Line line, Sci::Position position, bool lineStart) = 0;
	virtual void InsertLines(Sci::Line line, const Sci::Position *positions, size_t lines, bool lineStart) = 0;
	virtual void SetLineStart(Sci::Line line, Sci::Position position) noexcept = 0;
	virtual void RemoveLine(Sci::Line line) = 0;
	virtual Sci::Line Lines() const noexcept = 0;
	virtual void AllocateLines(Sci::Line lines) = 0;
	virtual Sci::Line LineFromPosition(Sci::Position pos) const noexcept = 0;
	virtual Sci::Position LineStart(Sci::Line line) const noexcept = 0;
	virtual void InsertCharacters(Sci::Line line, CountWidths delta) noexcept = 0;
	virtual void SetLineCharactersWidth(Sci::Line line, CountWidths width) noexcept = 0;
	virtual Scintilla::LineCharacterIndexType LineCharacterIndex() const noexcept = 0;
	virtual bool AllocateLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex, Sci::Line lines) = 0;
	virtual bool ReleaseLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex) = 0;
	virtual Sci::Position IndexLineStart(Sci::Line line, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept = 0;
	virtual Sci::Line LineFromPositionIndex(Sci::Position pos, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept = 0;
	virtual ~ILineVector() {}
};

}

using namespace Scintilla;
using namespace Scintilla::Internal;

template <typename POS>
class LineStartIndex {
public:
	int refCount;
	Partitioning<POS> starts;

	LineStartIndex() : refCount(0), starts(4) {
		// Minimal initial allocation
	}
	// Deleted so LineStartIndex objects can not be copied.
	LineStartIndex(const LineStartIndex &) = delete;
	LineStartIndex(LineStartIndex &&) = delete;
	void operator=(const LineStartIndex &) = delete;
	void operator=(LineStartIndex &&) = delete;
	virtual ~LineStartIndex() {
	}
	bool Allocate(Sci::Line lines) {
		refCount++;
		Sci::Position length = starts.PositionFromPartition(starts.Partitions());
		for (Sci::Line line = starts.Partitions(); line < lines; line++) {
			// Produce an ascending sequence that will be filled in with correct widths later
			length++;
			starts.InsertPartition(static_cast<POS>(line), static_cast<POS>(length));
		}
		return refCount == 1;
	}
	bool Release() {
		if (refCount == 1) {
			starts.DeleteAll();
		}
		refCount--;
		return refCount == 0;
	}
	bool Active() const noexcept {
		return refCount > 0;
	}
	Sci::Position LineWidth(Sci::Line line) const noexcept {
		return starts.PositionFromPartition(static_cast<POS>(line) + 1) -
			starts.PositionFromPartition(static_cast<POS>(line));
	}
	void SetLineWidth(Sci::Line line, Sci::Position width) noexcept {
		const Sci::Position widthCurrent = LineWidth(line);
		starts.InsertText(static_cast<POS>(line), static_cast<POS>(width - widthCurrent));
	}
	void AllocateLines(Sci::Line lines) {
		if (lines > starts.Partitions()) {
			starts.ReAllocate(lines);
		}
	}
	void InsertLines(Sci::Line line, Sci::Line lines) {
		// Insert multiple lines with each temporarily 1 character wide.
		// The line widths will be fixed up by later measuring code.
		const POS lineAsPos = static_cast<POS>(line);
		const POS lineStart = starts.PositionFromPartition(lineAsPos - 1) + 1;
		for (POS l = 0; l < static_cast<POS>(lines); l++) {
			starts.InsertPartition(lineAsPos + l, lineStart + l);
		}
	}
};

template <typename POS>
class LineVector : public ILineVector {
	Partitioning<POS> starts;
	PerLine *perLine;
	LineStartIndex<POS> startsUTF16;
	LineStartIndex<POS> startsUTF32;
	LineCharacterIndexType activeIndices;

	void SetActiveIndices() noexcept {
		activeIndices =
			  (startsUTF32.Active() ? LineCharacterIndexType::Utf32 : LineCharacterIndexType::None)
			| (startsUTF16.Active() ? LineCharacterIndexType::Utf16 : LineCharacterIndexType::None);
	}

public:
	LineVector() : starts(256), perLine(nullptr), activeIndices(LineCharacterIndexType::None) {
	}
	// Deleted so LineVector objects can not be copied.
	LineVector(const LineVector &) = delete;
	LineVector(LineVector &&) = delete;
	LineVector &operator=(const LineVector &) = delete;
	LineVector &operator=(LineVector &&) = delete;
	~LineVector() override {
	}
	void Init() override {
		starts.DeleteAll();
		if (perLine) {
			perLine->Init();
		}
		startsUTF32.starts.DeleteAll();
		startsUTF16.starts.DeleteAll();
	}
	void SetPerLine(PerLine *pl) noexcept override {
		perLine = pl;
	}
	void InsertText(Sci::Line line, Sci::Position delta) noexcept override {
		starts.InsertText(static_cast<POS>(line), static_cast<POS>(delta));
	}
	void InsertLine(Sci::Line line, Sci::Position position, bool lineStart) override {
		const POS lineAsPos = static_cast<POS>(line);
		starts.InsertPartition(lineAsPos, static_cast<POS>(position));
		if (activeIndices != LineCharacterIndexType::None) {
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
				startsUTF32.InsertLines(line, 1);
			}
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
				startsUTF16.InsertLines(line, 1);
			}
		}
		if (perLine) {
			if ((line > 0) && lineStart)
				line--;
			perLine->InsertLine(line);
		}
	}
	void InsertLines(Sci::Line line, const Sci::Position *positions, size_t lines, bool lineStart) override {
		const POS lineAsPos = static_cast<POS>(line);
		if constexpr (sizeof(Sci::Position) == sizeof(POS)) {
			starts.InsertPartitions(lineAsPos, positions, lines);
		} else {
			starts.InsertPartitionsWithCast(lineAsPos, positions, lines);
		}
		if (activeIndices != LineCharacterIndexType::None) {
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
				startsUTF32.InsertLines(line, lines);
			}
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
				startsUTF16.InsertLines(line, lines);
			}
		}
		if (perLine) {
			if ((line > 0) && lineStart)
				line--;
			perLine->InsertLines(line, lines);
		}
	}
	void SetLineStart(Sci::Line line, Sci::Position position) noexcept override {
		starts.SetPartitionStartPosition(static_cast<POS>(line), static_cast<POS>(position));
	}
	void RemoveLine(Sci::Line line) override {
		starts.RemovePartition(static_cast<POS>(line));
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
			startsUTF32.starts.RemovePartition(static_cast<POS>(line));
		}
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
			startsUTF16.starts.RemovePartition(static_cast<POS>(line));
		}
		if (perLine) {
			perLine->RemoveLine(line);
		}
	}
	Sci::Line Lines() const noexcept override {
		return static_cast<Sci::Line>(starts.Partitions());
	}
	void AllocateLines(Sci::Line lines) override {
		if (lines > Lines()) {
			starts.ReAllocate(lines);
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
				startsUTF32.AllocateLines(lines);
			}
			if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
				startsUTF16.AllocateLines(lines);
			}
		}
	}
	Sci::Line LineFromPosition(Sci::Position pos) const noexcept override {
		return static_cast<Sci::Line>(starts.PartitionFromPosition(static_cast<POS>(pos)));
	}
	Sci::Position LineStart(Sci::Line line) const noexcept override {
		return starts.PositionFromPartition(static_cast<POS>(line));
	}
	void InsertCharacters(Sci::Line line, CountWidths delta) noexcept override {
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
			startsUTF32.starts.InsertText(static_cast<POS>(line), static_cast<POS>(delta.WidthUTF32()));
		}
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
			startsUTF16.starts.InsertText(static_cast<POS>(line), static_cast<POS>(delta.WidthUTF16()));
		}
	}
	void SetLineCharactersWidth(Sci::Line line, CountWidths width) noexcept override {
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
			assert(startsUTF32.starts.Partitions() == starts.Partitions());
			startsUTF32.SetLineWidth(line, width.WidthUTF32());
		}
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
			assert(startsUTF16.starts.Partitions() == starts.Partitions());
			startsUTF16.SetLineWidth(line, width.WidthUTF16());
		}
	}

	LineCharacterIndexType LineCharacterIndex() const noexcept override {
		return activeIndices;
	}
	bool AllocateLineCharacterIndex(LineCharacterIndexType lineCharacterIndex, Sci::Line lines) override {
		const LineCharacterIndexType activeIndicesStart = activeIndices;
		if (FlagSet(lineCharacterIndex, LineCharacterIndexType::Utf32)) {
			startsUTF32.Allocate(lines);
			assert(startsUTF32.starts.Partitions() == starts.Partitions());
		}
		if (FlagSet(lineCharacterIndex, LineCharacterIndexType::Utf16)) {
			startsUTF16.Allocate(lines);
			assert(startsUTF16.starts.Partitions() == starts.Partitions());
		}
		SetActiveIndices();
		return activeIndicesStart != activeIndices;
	}
	bool ReleaseLineCharacterIndex(LineCharacterIndexType lineCharacterIndex) override {
		const LineCharacterIndexType activeIndicesStart = activeIndices;
		if (FlagSet(lineCharacterIndex, LineCharacterIndexType::Utf32)) {
			startsUTF32.Release();
		}
		if (FlagSet(lineCharacterIndex, LineCharacterIndexType::Utf16)) {
			startsUTF16.Release();
		}
		SetActiveIndices();
		return activeIndicesStart != activeIndices;
	}
	Sci::Position IndexLineStart(Sci::Line line, LineCharacterIndexType lineCharacterIndex) const noexcept override {
		if (lineCharacterIndex == LineCharacterIndexType::Utf32) {
			return startsUTF32.starts.PositionFromPartition(static_cast<POS>(line));
		} else {
			return startsUTF16.starts.PositionFromPartition(static_cast<POS>(line));
		}
	}
	Sci::Line LineFromPositionIndex(Sci::Position pos, LineCharacterIndexType lineCharacterIndex) const noexcept override {
		if (lineCharacterIndex == LineCharacterIndexType::Utf32) {
			return static_cast<Sci::Line>(startsUTF32.starts.PartitionFromPosition(static_cast<POS>(pos)));
		} else {
			return static_cast<Sci::Line>(startsUTF16.starts.PartitionFromPosition(static_cast<POS>(pos)));
		}
	}
};

Action::Action() noexcept {
	at = ActionType::start;
	position = 0;
	lenData = 0;
	mayCoalesce = false;
}

Action::~Action() {
}

void Action::Create(ActionType at_, Sci::Position position_, const char *data_, Sci::Position lenData_, bool mayCoalesce_) {
	data = nullptr;
	position = position_;
	at = at_;
	if (lenData_) {
		data = std::make_unique<char[]>(lenData_);
		memcpy(&data[0], data_, lenData_);
	}
	lenData = lenData_;
	mayCoalesce = mayCoalesce_;
}

void Action::Clear() noexcept {
	data = nullptr;
	lenData = 0;
}

// The undo history stores a sequence of user operations that represent the user's view of the
// commands executed on the text.
// Each user operation contains a sequence of text insertion and text deletion actions.
// All the user operations are stored in a list of individual actions with 'start' actions used
// as delimiters between user operations.
// Initially there is one start action in the history.
// As each action is performed, it is recorded in the history. The action may either become
// part of the current user operation or may start a new user operation. If it is to be part of the
// current operation, then it overwrites the current last action. If it is to be part of a new
// operation, it is appended after the current last action.
// After writing the new action, a new start action is appended at the end of the history.
// The decision of whether to start a new user operation is based upon two factors. If a
// compound operation has been explicitly started by calling BeginUndoAction and no matching
// EndUndoAction (these calls nest) has been called, then the action is coalesced into the current
// operation. If there is no outstanding BeginUndoAction call then a new operation is started
// unless it looks as if the new action is caused by the user typing or deleting a stream of text.
// Sequences that look like typing or deletion are coalesced into a single user operation.

UndoHistory::UndoHistory() {

	actions.resize(3);
	maxAction = 0;
	currentAction = 0;
	undoSequenceDepth = 0;
	savePoint = 0;
	tentativePoint = -1;

	actions[currentAction].Create(ActionType::start);
}

UndoHistory::~UndoHistory() {
}

void UndoHistory::EnsureUndoRoom() {
	// Have to test that there is room for 2 more actions in the array
	// as two actions may be created by the calling function
	if (static_cast<size_t>(currentAction) >= (actions.size() - 2)) {
		// Run out of undo nodes so extend the array
		actions.resize(actions.size() * 2);
	}
}

const char *UndoHistory::AppendAction(ActionType at, Sci::Position position, const char *data, Sci::Position lengthData,
	bool &startSequence, bool mayCoalesce) {
	EnsureUndoRoom();
	//Platform::DebugPrintf("%% %d action %d %d %d\n", at, position, lengthData, currentAction);
	//Platform::DebugPrintf("^ %d action %d %d\n", actions[currentAction - 1].at,
	//	actions[currentAction - 1].position, actions[currentAction - 1].lenData);
	if (currentAction < savePoint) {
		savePoint = -1;
	}
	int oldCurrentAction = currentAction;
	if (currentAction >= 1) {
		if (0 == undoSequenceDepth) {
			// Top level actions may not always be coalesced
			int targetAct = -1;
			const Action *actPrevious = &(actions[currentAction + targetAct]);
			// Container actions may forward the coalesce state of Scintilla Actions.
			while ((actPrevious->at == ActionType::container) && actPrevious->mayCoalesce) {
				targetAct--;
				actPrevious = &(actions[currentAction + targetAct]);
			}
			// See if current action can be coalesced into previous action
			// Will work if both are inserts or deletes and position is same
			if ((currentAction == savePoint) || (currentAction == tentativePoint)) {
				currentAction++;
			} else if (!actions[currentAction].mayCoalesce) {
				// Not allowed to coalesce if this set
				currentAction++;
			} else if (!mayCoalesce || !actPrevious->mayCoalesce) {
				currentAction++;
			} else if (at == ActionType::container || actions[currentAction].at == ActionType::container) {
				;	// A coalescible containerAction
			} else if ((at != actPrevious->at) && (actPrevious->at != ActionType::start)) {
				currentAction++;
			} else if ((at == ActionType::insert) &&
			           (position != (actPrevious->position + actPrevious->lenData))) {
				// Insertions must be immediately after to coalesce
				currentAction++;
			} else if (at == ActionType::remove) {
				if ((lengthData == 1) || (lengthData == 2)) {
					if ((position + lengthData) == actPrevious->position) {
						; // Backspace -> OK
					} else if (position == actPrevious->position) {
						; // Delete -> OK
					} else {
						// Removals must be at same position to coalesce
						currentAction++;
					}
				} else {
					// Removals must be of one character to coalesce
					currentAction++;
				}
			} else {
				// Action coalesced.
			}

		} else {
			// Actions not at top level are always coalesced unless this is after return to top level
			if (!actions[currentAction].mayCoalesce)
				currentAction++;
		}
	} else {
		currentAction++;
	}
	startSequence = oldCurrentAction != currentAction;
	const int actionWithData = currentAction;
	actions[currentAction].Create(at, position, data, lengthData, mayCoalesce);
	currentAction++;
	actions[currentAction].Create(ActionType::start);
	maxAction = currentAction;
	return actions[actionWithData].data.get();
}

void UndoHistory::BeginUndoAction() {
	EnsureUndoRoom();
	if (undoSequenceDepth == 0) {
		if (actions[currentAction].at != ActionType::start) {
			currentAction++;
			actions[currentAction].Create(ActionType::start);
			maxAction = currentAction;
		}
		actions[currentAction].mayCoalesce = false;
	}
	undoSequenceDepth++;
}

void UndoHistory::EndUndoAction() {
	PLATFORM_ASSERT(undoSequenceDepth > 0);
	EnsureUndoRoom();
	undoSequenceDepth--;
	if (0 == undoSequenceDepth) {
		if (actions[currentAction].at != ActionType::start) {
			currentAction++;
			actions[currentAction].Create(ActionType::start);
			maxAction = currentAction;
		}
		actions[currentAction].mayCoalesce = false;
	}
}

void UndoHistory::DropUndoSequence() {
	undoSequenceDepth = 0;
}

void UndoHistory::DeleteUndoHistory() {
	for (int i = 1; i < maxAction; i++)
		actions[i].Clear();
	maxAction = 0;
	currentAction = 0;
	actions[currentAction].Create(ActionType::start);
	savePoint = 0;
	tentativePoint = -1;
}

void UndoHistory::SetSavePoint() noexcept {
	savePoint = currentAction;
}

bool UndoHistory::IsSavePoint() const noexcept {
	return savePoint == currentAction;
}

void UndoHistory::TentativeStart() {
	tentativePoint = currentAction;
}

void UndoHistory::TentativeCommit() {
	tentativePoint = -1;
	// Truncate undo history
	maxAction = currentAction;
}

bool UndoHistory::TentativeActive() const noexcept {
	return tentativePoint >= 0;
}

int UndoHistory::TentativeSteps() noexcept {
	// Drop any trailing startAction
	if (actions[currentAction].at == ActionType::start && currentAction > 0)
		currentAction--;
	if (tentativePoint >= 0)
		return currentAction - tentativePoint;
	else
		return -1;
}

bool UndoHistory::CanUndo() const noexcept {
	return (currentAction > 0) && (maxAction > 0);
}

int UndoHistory::StartUndo() {
	// Drop any trailing startAction
	if (actions[currentAction].at == ActionType::start && currentAction > 0)
		currentAction--;

	// Count the steps in this action
	int act = currentAction;
	while (actions[act].at != ActionType::start && act > 0) {
		act--;
	}
	return currentAction - act;
}

const Action &UndoHistory::GetUndoStep() const {
	return actions[currentAction];
}

void UndoHistory::CompletedUndoStep() {
	currentAction--;
}

bool UndoHistory::CanRedo() const noexcept {
	return maxAction > currentAction;
}

int UndoHistory::StartRedo() {
	// Drop any leading startAction
	if (currentAction < maxAction && actions[currentAction].at == ActionType::start)
		currentAction++;

	// Count the steps in this action
	int act = currentAction;
	while (act < maxAction && actions[act].at != ActionType::start) {
		act++;
	}
	return act - currentAction;
}

const Action &UndoHistory::GetRedoStep() const {
	return actions[currentAction];
}

void UndoHistory::CompletedRedoStep() {
	currentAction++;
}

CellBuffer::CellBuffer(bool hasStyles_, bool largeDocument_) :
	hasStyles(hasStyles_), largeDocument(largeDocument_) {
	readOnly = false;
	utf8Substance = false;
	utf8LineEnds = LineEndType::Default;
	collectingUndo = true;
	if (largeDocument)
		plv = std::make_unique<LineVector<Sci::Position>>();
	else
		plv = std::make_unique<LineVector<int>>();
}

CellBuffer::~CellBuffer() {
}

char CellBuffer::CharAt(Sci::Position position) const noexcept {
	return substance.ValueAt(position);
}

unsigned char CellBuffer::UCharAt(Sci::Position position) const noexcept {
	return substance.ValueAt(position);
}

void CellBuffer::GetCharRange(char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const {
	if (lengthRetrieve <= 0)
		return;
	if (position < 0)
		return;
	if ((position + lengthRetrieve) > substance.Length()) {
		Platform::DebugPrintf("Bad GetCharRange %.0f for %.0f of %.0f\n",
				      static_cast<double>(position),
				      static_cast<double>(lengthRetrieve),
				      static_cast<double>(substance.Length()));
		return;
	}
	substance.GetRange(buffer, position, lengthRetrieve);
}

char CellBuffer::StyleAt(Sci::Position position) const noexcept {
	return hasStyles ? style.ValueAt(position) : 0;
}

void CellBuffer::GetStyleRange(unsigned char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const {
	if (lengthRetrieve < 0)
		return;
	if (position < 0)
		return;
	if (!hasStyles) {
		std::fill(buffer, buffer + lengthRetrieve, static_cast<unsigned char>(0));
		return;
	}
	if ((position + lengthRetrieve) > style.Length()) {
		Platform::DebugPrintf("Bad GetStyleRange %.0f for %.0f of %.0f\n",
				      static_cast<double>(position),
				      static_cast<double>(lengthRetrieve),
				      static_cast<double>(style.Length()));
		return;
	}
	style.GetRange(reinterpret_cast<char *>(buffer), position, lengthRetrieve);
}

const char *CellBuffer::BufferPointer() {
	return substance.BufferPointer();
}

const char *CellBuffer::RangePointer(Sci::Position position, Sci::Position rangeLength) noexcept {
	return substance.RangePointer(position, rangeLength);
}

Sci::Position CellBuffer::GapPosition() const noexcept {
	return substance.GapPosition();
}

SplitView CellBuffer::AllView() const noexcept {
	const size_t length = substance.Length();
	size_t length1 = substance.GapPosition();
	if (length1 == 0) {
		// Assign segment2 to segment1 / length1 to avoid useless test against 0 length1
		length1 = length;
	}
	return SplitView {
		substance.ElementPointer(0),
		length1,
		substance.ElementPointer(length1) - length1,
		length
	};
}

// The char* returned is to an allocation owned by the undo history
const char *CellBuffer::InsertString(Sci::Position position, const char *s, Sci::Position insertLength, bool &startSequence) {
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	const char *data = s;
	if (!readOnly) {
		if (collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			// This takes up about half load time
			data = uh.AppendAction(ActionType::insert, position, s, insertLength, startSequence);
		}

		BasicInsertString(position, s, insertLength);
	}
	return data;
}

bool CellBuffer::SetStyleAt(Sci::Position position, char styleValue) noexcept {
	if (!hasStyles) {
		return false;
	}
	const char curVal = style.ValueAt(position);
	if (curVal != styleValue) {
		style.SetValueAt(position, styleValue);
		return true;
	} else {
		return false;
	}
}

bool CellBuffer::SetStyleFor(Sci::Position position, Sci::Position lengthStyle, char styleValue) noexcept {
	if (!hasStyles) {
		return false;
	}
	bool changed = false;
	PLATFORM_ASSERT(lengthStyle == 0 ||
		(lengthStyle > 0 && lengthStyle + position <= style.Length()));
	while (lengthStyle--) {
		const char curVal = style.ValueAt(position);
		if (curVal != styleValue) {
			style.SetValueAt(position, styleValue);
			changed = true;
		}
		position++;
	}
	return changed;
}

// The char* returned is to an allocation owned by the undo history
const char *CellBuffer::DeleteChars(Sci::Position position, Sci::Position deleteLength, bool &startSequence) {
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	PLATFORM_ASSERT(deleteLength > 0);
	const char *data = nullptr;
	if (!readOnly) {
		if (collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			// The gap would be moved to position anyway for the deletion so this doesn't cost extra
			data = substance.RangePointer(position, deleteLength);
			data = uh.AppendAction(ActionType::remove, position, data, deleteLength, startSequence);
		}

		BasicDeleteChars(position, deleteLength);
	}
	return data;
}

Sci::Position CellBuffer::Length() const noexcept {
	return substance.Length();
}

void CellBuffer::Allocate(Sci::Position newSize) {
	substance.ReAllocate(newSize);
	if (hasStyles) {
		style.ReAllocate(newSize);
	}
}

void CellBuffer::SetUTF8Substance(bool utf8Substance_) noexcept {
	utf8Substance = utf8Substance_;
}

void CellBuffer::SetLineEndTypes(LineEndType utf8LineEnds_) {
	if (utf8LineEnds != utf8LineEnds_) {
		const LineCharacterIndexType indexes = plv->LineCharacterIndex();
		utf8LineEnds = utf8LineEnds_;
		ResetLineEnds();
		AllocateLineCharacterIndex(indexes);
	}
}

bool CellBuffer::ContainsLineEnd(const char *s, Sci::Position length) const noexcept {
	unsigned char chBeforePrev = 0;
	unsigned char chPrev = 0;
	for (Sci::Position i = 0; i < length; i++) {
		const unsigned char ch = s[i];
		if ((ch == '\r') || (ch == '\n')) {
			return true;
		} else if (utf8LineEnds == LineEndType::Unicode) {
			if (UTF8IsMultibyteLineEnd(chBeforePrev, chPrev, ch)) {
				return true;
			}
		}
		chBeforePrev = chPrev;
		chPrev = ch;
	}
	return false;
}

void CellBuffer::SetPerLine(PerLine *pl) noexcept {
	plv->SetPerLine(pl);
}

LineCharacterIndexType CellBuffer::LineCharacterIndex() const noexcept {
	return plv->LineCharacterIndex();
}

void CellBuffer::AllocateLineCharacterIndex(LineCharacterIndexType lineCharacterIndex) {
	if (utf8Substance) {
		if (plv->AllocateLineCharacterIndex(lineCharacterIndex, Lines())) {
			// Changed so recalculate whole file
			RecalculateIndexLineStarts(0, Lines() - 1);
		}
	}
}

void CellBuffer::ReleaseLineCharacterIndex(LineCharacterIndexType lineCharacterIndex) {
	plv->ReleaseLineCharacterIndex(lineCharacterIndex);
}

Sci::Line CellBuffer::Lines() const noexcept {
	return plv->Lines();
}

void CellBuffer::AllocateLines(Sci::Line lines) {
	plv->AllocateLines(lines);
}

Sci::Position CellBuffer::LineStart(Sci::Line line) const noexcept {
	if (line < 0)
		return 0;
	else if (line >= Lines())
		return Length();
	else
		return plv->LineStart(line);
}

Sci::Line CellBuffer::LineFromPosition(Sci::Position pos) const noexcept {
	return plv->LineFromPosition(pos);
}

Sci::Position CellBuffer::IndexLineStart(Sci::Line line, LineCharacterIndexType lineCharacterIndex) const noexcept {
	return plv->IndexLineStart(line, lineCharacterIndex);
}

Sci::Line CellBuffer::LineFromPositionIndex(Sci::Position pos, LineCharacterIndexType lineCharacterIndex) const noexcept {
	return plv->LineFromPositionIndex(pos, lineCharacterIndex);
}

bool CellBuffer::IsReadOnly() const noexcept {
	return readOnly;
}

void CellBuffer::SetReadOnly(bool set) noexcept {
	readOnly = set;
}

bool CellBuffer::IsLarge() const noexcept {
	return largeDocument;
}

bool CellBuffer::HasStyles() const noexcept {
	return hasStyles;
}

void CellBuffer::SetSavePoint() {
	uh.SetSavePoint();
}

bool CellBuffer::IsSavePoint() const noexcept {
	return uh.IsSavePoint();
}

void CellBuffer::TentativeStart() {
	uh.TentativeStart();
}

void CellBuffer::TentativeCommit() {
	uh.TentativeCommit();
}

int CellBuffer::TentativeSteps() noexcept {
	return uh.TentativeSteps();
}

bool CellBuffer::TentativeActive() const noexcept {
	return uh.TentativeActive();
}

// Without undo

void CellBuffer::InsertLine(Sci::Line line, Sci::Position position, bool lineStart) {
	plv->InsertLine(line, position, lineStart);
}

void CellBuffer::RemoveLine(Sci::Line line) {
	plv->RemoveLine(line);
}

bool CellBuffer::UTF8LineEndOverlaps(Sci::Position position) const noexcept {
	const unsigned char bytes[] = {
		static_cast<unsigned char>(substance.ValueAt(position-2)),
		static_cast<unsigned char>(substance.ValueAt(position-1)),
		static_cast<unsigned char>(substance.ValueAt(position)),
		static_cast<unsigned char>(substance.ValueAt(position+1)),
	};
	return UTF8IsSeparator(bytes) || UTF8IsSeparator(bytes+1) || UTF8IsNEL(bytes+1);
}

bool CellBuffer::UTF8IsCharacterBoundary(Sci::Position position) const {
	assert(position >= 0 && position <= Length());
	if (position > 0) {
		std::string back;
		for (int i = 0; i < UTF8MaxBytes; i++) {
			const Sci::Position posBack = position - i;
			if (posBack < 0) {
				return false;
			}
			back.insert(0, 1, substance.ValueAt(posBack));
			if (!UTF8IsTrailByte(back.front())) {
				if (i > 0) {
					// Have reached a non-trail
					const int cla = UTF8Classify(back);
					if ((cla & UTF8MaskInvalid) || (cla != i)) {
						return false;
					}
				}
				break;
			}
		}
	}
	if (position < Length()) {
		const unsigned char fore = substance.ValueAt(position);
		if (UTF8IsTrailByte(fore)) {
			return false;
		}
	}
	return true;
}

void CellBuffer::ResetLineEnds() {
	// Reinitialize line data -- too much work to preserve
	const Sci::Line lines = plv->Lines();
	plv->Init();
	plv->AllocateLines(lines);

	constexpr Sci::Position position = 0;
	const Sci::Position length = Length();
	plv->InsertText(0, length);
	Sci::Line lineInsert = 1;
	constexpr bool atLineStart = true;
	unsigned char chBeforePrev = 0;
	unsigned char chPrev = 0;
	for (Sci::Position i = 0; i < length; i++) {
		const unsigned char ch = substance.ValueAt(position + i);
		if (ch == '\r') {
			InsertLine(lineInsert, (position + i) + 1, atLineStart);
			lineInsert++;
		} else if (ch == '\n') {
			if (chPrev == '\r') {
				// Patch up what was end of line
				plv->SetLineStart(lineInsert - 1, (position + i) + 1);
			} else {
				InsertLine(lineInsert, (position + i) + 1, atLineStart);
				lineInsert++;
			}
		} else if (utf8LineEnds == LineEndType::Unicode) {
			if (UTF8IsMultibyteLineEnd(chBeforePrev, chPrev, ch)) {
				InsertLine(lineInsert, (position + i) + 1, atLineStart);
				lineInsert++;
			}
		}
		chBeforePrev = chPrev;
		chPrev = ch;
	}
}

namespace {

CountWidths CountCharacterWidthsUTF8(std::string_view sv) noexcept {
	CountWidths cw;
	size_t remaining = sv.length();
	while (remaining > 0) {
		const int utf8Status = UTF8Classify(sv);
		const int lenChar = utf8Status & UTF8MaskWidth;
		cw.CountChar(lenChar);
		sv.remove_prefix(lenChar);
		remaining -= lenChar;
	}
	return cw;
}

}

bool CellBuffer::MaintainingLineCharacterIndex() const noexcept {
	return plv->LineCharacterIndex() != LineCharacterIndexType::None;
}

void CellBuffer::RecalculateIndexLineStarts(Sci::Line lineFirst, Sci::Line lineLast) {
	std::string text;
	Sci::Position posLineEnd = LineStart(lineFirst);
	for (Sci::Line line = lineFirst; line <= lineLast; line++) {
		// Find line start and end, retrieve text of line, count characters and update line width
		const Sci::Position posLineStart = posLineEnd;
		posLineEnd = LineStart(line+1);
		const Sci::Position width = posLineEnd - posLineStart;
		text.resize(width);
		GetCharRange(text.data(), posLineStart, width);
		const CountWidths cw = CountCharacterWidthsUTF8(text);
		plv->SetLineCharactersWidth(line, cw);
	}
}

void CellBuffer::BasicInsertString(Sci::Position position, const char *s, Sci::Position insertLength) {
	if (insertLength == 0)
		return;
	PLATFORM_ASSERT(insertLength > 0);

	const unsigned char chAfter = substance.ValueAt(position);
	bool breakingUTF8LineEnd = false;
	if (utf8LineEnds == LineEndType::Unicode && UTF8IsTrailByte(chAfter)) {
		breakingUTF8LineEnd = UTF8LineEndOverlaps(position);
	}

	const Sci::Line linePosition = plv->LineFromPosition(position);
	Sci::Line lineInsert = linePosition + 1;

	// A simple insertion is one that inserts valid text on a single line at a character boundary
	bool simpleInsertion = false;

	const bool maintainingIndex = MaintainingLineCharacterIndex();

	// Check for breaking apart a UTF-8 sequence and inserting invalid UTF-8
	if (utf8Substance && maintainingIndex) {
		// Actually, don't need to check that whole insertion is valid just that there
		// are no potential fragments at ends.
		simpleInsertion = UTF8IsCharacterBoundary(position) &&
			UTF8IsValid(std::string_view(s, insertLength));
	}

	substance.InsertFromArray(position, s, 0, insertLength);
	if (hasStyles) {
		style.InsertValue(position, insertLength, 0);
	}

	const bool atLineStart = plv->LineStart(lineInsert-1) == position;
	// Point all the lines after the insertion point further along in the buffer
	plv->InsertText(lineInsert-1, insertLength);
	unsigned char chBeforePrev = substance.ValueAt(position - 2);
	unsigned char chPrev = substance.ValueAt(position - 1);
	if (chPrev == '\r' && chAfter == '\n') {
		// Splitting up a crlf pair at position
		InsertLine(lineInsert, position, false);
		lineInsert++;
	}
	if (breakingUTF8LineEnd) {
		RemoveLine(lineInsert);
	}

	constexpr size_t PositionBlockSize = 128;
	Sci::Position positions[PositionBlockSize]{};
	size_t nPositions = 0;
	const Sci::Line lineStart = lineInsert;

	// s may not NULL-terminated, ensure *ptr == '\n' or *next == '\n' is valid.
	const char * const end = s + insertLength - 1;
	const char *ptr = s;
	unsigned char ch = 0;

	if (chPrev == '\r' && *ptr == '\n') {
		++ptr;
		// Patch up what was end of line
		plv->SetLineStart(lineInsert - 1, (position + ptr - s));
		simpleInsertion = false;
	}

	if (ptr < end) {
		uint8_t eolTable[256]{};
		eolTable[static_cast<uint8_t>('\n')] = 1;
		eolTable[static_cast<uint8_t>('\r')] = 2;
		if (utf8LineEnds == LineEndType::Unicode) {
			// see UniConversion.h for LS, PS and NEL
			eolTable[0x85] = 4;
			eolTable[0xa8] = 3;
			eolTable[0xa9] = 3;
		}

		do {
			// skip to line end
			ch = *ptr++;
			uint8_t type;
			while ((type = eolTable[ch]) == 0 && ptr < end) {
				chBeforePrev = chPrev;
				chPrev = ch;
				ch = *ptr++;
			}
			switch (type) {
			case 2: // '\r'
				if (*ptr == '\n') {
					++ptr;
				}
				[[fallthrough]];
			case 1: // '\n'
				positions[nPositions++] = position + ptr - s;
				if (nPositions == PositionBlockSize) {
					plv->InsertLines(lineInsert, positions, nPositions, atLineStart);
					lineInsert += nPositions;
					nPositions = 0;
				}
				break;
			case 3:
			case 4:
				// LS, PS and NEL
				if ((type == 3 && chPrev == 0x80 && chBeforePrev == 0xe2) || (type == 4 && chPrev == 0xc2)) {
					positions[nPositions++] = position + ptr - s;
					if (nPositions == PositionBlockSize) {
						plv->InsertLines(lineInsert, positions, nPositions, atLineStart);
						lineInsert += nPositions;
						nPositions = 0;
					}
				}
				break;
			}

			chBeforePrev = chPrev;
			chPrev = ch;
		} while (ptr < end);
	}

	if (nPositions != 0) {
		plv->InsertLines(lineInsert, positions, nPositions, atLineStart);
		lineInsert += nPositions;
	}

	ch = *end;
	if (ptr == end) {
		++ptr;
		if (ch == '\r' || ch == '\n') {
			InsertLine(lineInsert, (position + ptr - s), atLineStart);
			lineInsert++;
		} else if (utf8LineEnds == LineEndType::Unicode && !UTF8IsAscii(ch)) {
			if (UTF8IsMultibyteLineEnd(chBeforePrev, chPrev, ch)) {
				InsertLine(lineInsert, (position + ptr - s), atLineStart);
				lineInsert++;
			}
		}
	}

	// Joining two lines where last insertion is cr and following substance starts with lf
	if (chAfter == '\n') {
		if (ch == '\r') {
			// End of line already in buffer so drop the newly created one
			RemoveLine(lineInsert - 1);
			simpleInsertion = false;
		}
	} else if (utf8LineEnds == LineEndType::Unicode && !UTF8IsAscii(chAfter)) {
		chBeforePrev = chPrev;
		chPrev = ch;
		// May have end of UTF-8 line end in buffer and start in insertion
		for (int j = 0; j < UTF8SeparatorLength-1; j++) {
			const unsigned char chAt = substance.ValueAt(position + insertLength + j);
			const unsigned char back3[3] = {chBeforePrev, chPrev, chAt};
			if (UTF8IsSeparator(back3)) {
				InsertLine(lineInsert, (position + insertLength + j) + 1, atLineStart);
				lineInsert++;
			}
			if ((j == 0) && UTF8IsNEL(back3+1)) {
				InsertLine(lineInsert, (position + insertLength + j) + 1, atLineStart);
				lineInsert++;
			}
			chBeforePrev = chPrev;
			chPrev = chAt;
		}
	}
	if (maintainingIndex) {
		if (simpleInsertion && (lineInsert == lineStart)) {
			const CountWidths cw = CountCharacterWidthsUTF8(std::string_view(s, insertLength));
			plv->InsertCharacters(linePosition, cw);
		} else {
			RecalculateIndexLineStarts(linePosition, lineInsert - 1);
		}
	}
}

void CellBuffer::BasicDeleteChars(Sci::Position position, Sci::Position deleteLength) {
	if (deleteLength == 0)
		return;

	Sci::Line lineRecalculateStart = Sci::invalidPosition;

	if ((position == 0) && (deleteLength == substance.Length())) {
		// If whole buffer is being deleted, faster to reinitialise lines data
		// than to delete each line.
		plv->Init();
	} else {
		// Have to fix up line positions before doing deletion as looking at text in buffer
		// to work out which lines have been removed

		const Sci::Line linePosition = plv->LineFromPosition(position);
		Sci::Line lineRemove = linePosition + 1;

		plv->InsertText(lineRemove-1, - (deleteLength));
		const unsigned char chPrev = substance.ValueAt(position - 1);
		const unsigned char chBefore = chPrev;
		unsigned char chNext = substance.ValueAt(position);

		// Check for breaking apart a UTF-8 sequence
		// Needs further checks that text is UTF-8 or that some other break apart is occurring
		if (utf8Substance && MaintainingLineCharacterIndex()) {
			const Sci::Position posEnd = position + deleteLength;
			const Sci::Line lineEndRemove = plv->LineFromPosition(posEnd);
			const bool simpleDeletion =
				(linePosition == lineEndRemove) &&
				UTF8IsCharacterBoundary(position) && UTF8IsCharacterBoundary(posEnd);
			if (simpleDeletion) {
				std::string text(deleteLength, '\0');
				GetCharRange(text.data(), position, deleteLength);
				if (UTF8IsValid(text)) {
					// Everything is good
					const CountWidths cw = CountCharacterWidthsUTF8(text);
					plv->InsertCharacters(linePosition, -cw);
				} else {
					lineRecalculateStart = linePosition;
				}
			} else {
				lineRecalculateStart = linePosition;
			}
		}

		bool ignoreNL = false;
		if (chPrev == '\r' && chNext == '\n') {
			// Move back one
			plv->SetLineStart(lineRemove, position);
			lineRemove++;
			ignoreNL = true; 	// First \n is not real deletion
		}
		if (utf8LineEnds == LineEndType::Unicode && UTF8IsTrailByte(chNext)) {
			if (UTF8LineEndOverlaps(position)) {
				RemoveLine(lineRemove);
			}
		}

		unsigned char ch = chNext;
		for (Sci::Position i = 0; i < deleteLength; i++) {
			chNext = substance.ValueAt(position + i + 1);
			if (ch == '\r') {
				if (chNext != '\n') {
					RemoveLine(lineRemove);
				}
			} else if (ch == '\n') {
				if (ignoreNL) {
					ignoreNL = false; 	// Further \n are real deletions
				} else {
					RemoveLine(lineRemove);
				}
			} else if (utf8LineEnds == LineEndType::Unicode) {
				if (!UTF8IsAscii(ch)) {
					const unsigned char next3[3] = {ch, chNext,
						static_cast<unsigned char>(substance.ValueAt(position + i + 2))};
					if (UTF8IsSeparator(next3) || UTF8IsNEL(next3)) {
						RemoveLine(lineRemove);
					}
				}
			}

			ch = chNext;
		}
		// May have to fix up end if last deletion causes cr to be next to lf
		// or removes one of a crlf pair
		const char chAfter = substance.ValueAt(position + deleteLength);
		if (chBefore == '\r' && chAfter == '\n') {
			// Using lineRemove-1 as cr ended line before start of deletion
			RemoveLine(lineRemove - 1);
			plv->SetLineStart(lineRemove - 1, position + 1);
		}
	}
	substance.DeleteRange(position, deleteLength);
	if (lineRecalculateStart >= 0) {
		RecalculateIndexLineStarts(lineRecalculateStart, lineRecalculateStart);
	}
	if (hasStyles) {
		style.DeleteRange(position, deleteLength);
	}
}

bool CellBuffer::SetUndoCollection(bool collectUndo) {
	collectingUndo = collectUndo;
	uh.DropUndoSequence();
	return collectingUndo;
}

bool CellBuffer::IsCollectingUndo() const noexcept {
	return collectingUndo;
}

void CellBuffer::BeginUndoAction() {
	uh.BeginUndoAction();
}

void CellBuffer::EndUndoAction() {
	uh.EndUndoAction();
}

void CellBuffer::AddUndoAction(Sci::Position token, bool mayCoalesce) {
	bool startSequence = false;
	uh.AppendAction(ActionType::container, token, nullptr, 0, startSequence, mayCoalesce);
}

void CellBuffer::DeleteUndoHistory() {
	uh.DeleteUndoHistory();
}

bool CellBuffer::CanUndo() const noexcept {
	return uh.CanUndo();
}

int CellBuffer::StartUndo() {
	return uh.StartUndo();
}

const Action &CellBuffer::GetUndoStep() const {
	return uh.GetUndoStep();
}

void CellBuffer::PerformUndoStep() {
	const Action &actionStep = uh.GetUndoStep();
	if (actionStep.at == ActionType::insert) {
		if (substance.Length() < actionStep.lenData) {
			throw std::runtime_error(
				"CellBuffer::PerformUndoStep: deletion must be less than document length.");
		}
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	} else if (actionStep.at == ActionType::remove) {
		BasicInsertString(actionStep.position, actionStep.data.get(), actionStep.lenData);
	}
	uh.CompletedUndoStep();
}

bool CellBuffer::CanRedo() const noexcept {
	return uh.CanRedo();
}

int CellBuffer::StartRedo() {
	return uh.StartRedo();
}

const Action &CellBuffer::GetRedoStep() const {
	return uh.GetRedoStep();
}

void CellBuffer::PerformRedoStep() {
	const Action &actionStep = uh.GetRedoStep();
	if (actionStep.at == ActionType::insert) {
		BasicInsertString(actionStep.position, actionStep.data.get(), actionStep.lenData);
	} else if (actionStep.at == ActionType::remove) {
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	}
	uh.CompletedRedoStep();
}

