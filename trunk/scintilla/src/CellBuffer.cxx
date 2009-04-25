// Scintilla source code edit control
/** @file CellBuffer.cxx
 ** Manages a buffer of cells.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

MarkerHandleSet::MarkerHandleSet() {
	root = 0;
}

MarkerHandleSet::~MarkerHandleSet() {
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		MarkerHandleNumber *mhnToFree = mhn;
		mhn = mhn->next;
		delete mhnToFree;
	}
	root = 0;
}

int MarkerHandleSet::Length() const {
	int c = 0;
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		c++;
		mhn = mhn->next;
	}
	return c;
}

int MarkerHandleSet::NumberFromHandle(int handle) const {
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		if (mhn->handle == handle) {
			return mhn->number;
		}
		mhn = mhn->next;
	}
	return - 1;
}

int MarkerHandleSet::MarkValue() const {
	unsigned int m = 0;
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		m |= (1 << mhn->number);
		mhn = mhn->next;
	}
	return m;
}

bool MarkerHandleSet::Contains(int handle) const {
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		if (mhn->handle == handle) {
			return true;
		}
		mhn = mhn->next;
	}
	return false;
}

bool MarkerHandleSet::InsertHandle(int handle, int markerNum) {
	MarkerHandleNumber *mhn = new MarkerHandleNumber;
	if (!mhn)
		return false;
	mhn->handle = handle;
	mhn->number = markerNum;
	mhn->next = root;
	root = mhn;
	return true;
}

void MarkerHandleSet::RemoveHandle(int handle) {
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		MarkerHandleNumber *mhn = *pmhn;
		if (mhn->handle == handle) {
			*pmhn = mhn->next;
			delete mhn;
			return;
		}
		pmhn = &((*pmhn)->next);
	}
}

bool MarkerHandleSet::RemoveNumber(int markerNum) {
	bool performedDeletion = false;
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		MarkerHandleNumber *mhn = *pmhn;
		if (mhn->number == markerNum) {
			*pmhn = mhn->next;
			delete mhn;
			performedDeletion = true;
		} else {
			pmhn = &((*pmhn)->next);
		}
	}
	return performedDeletion;
}

void MarkerHandleSet::CombineWith(MarkerHandleSet *other) {
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		pmhn = &((*pmhn)->next);
	}
	*pmhn = other->root;
	other->root = 0;
}

LineVector::LineVector() : starts(256) {
	handleCurrent = 1;

	Init();
}

LineVector::~LineVector() {
	starts.DeleteAll();
	for (int line = 0; line < markers.Length(); line++) {
		delete markers[line];
		markers[line] = 0;
	}
	markers.DeleteAll();
	levels.DeleteAll();
}

void LineVector::Init() {
	starts.DeleteAll();
	for (int line = 0; line < markers.Length(); line++) {
		delete markers[line];
		markers[line] = 0;
	}
	markers.DeleteAll();
	levels.DeleteAll();
}

void LineVector::ExpandLevels(int sizeNew) {
	levels.InsertValue(levels.Length(), sizeNew - levels.Length(), SC_FOLDLEVELBASE);
}

void LineVector::ClearLevels() {
	levels.DeleteAll();
}

int LineVector::SetLevel(int line, int level) {
	int prev = 0;
	if ((line >= 0) && (line < Lines())) {
		if (!levels.Length()) {
			ExpandLevels(Lines() + 1);
		}
		prev = levels[line];
		if (prev != level) {
			levels[line] = level;
		}
	}
	return prev;
}

int LineVector::GetLevel(int line) {
	if (levels.Length() && (line >= 0) && (line < Lines())) {
		return levels[line];
	} else {
		return SC_FOLDLEVELBASE;
	}
}

void LineVector::InsertText(int line, int delta) {
	starts.InsertText(line, delta);
}

void LineVector::InsertLine(int line, int position) {
	starts.InsertPartition(line, position);
	if (markers.Length()) {
		markers.Insert(line, 0);
	}
	if (levels.Length()) {
		int level = SC_FOLDLEVELBASE;
		if ((line > 0) && (line < Lines())) {
			level = levels[line-1] & ~SC_FOLDLEVELWHITEFLAG;
		}
		levels.InsertValue(line, 1, level);
	}
}

void LineVector::SetLineStart(int line, int position) {
	starts.SetPartitionStartPosition(line, position);
}

void LineVector::RemoveLine(int line) {
	starts.RemovePartition(line);
	// Retain the markers from the deleted line by oring them into the previous line
	if (markers.Length()) {
		if (line > 0) {
			MergeMarkers(line - 1);
		}
		markers.Delete(line);
	}
	if (levels.Length()) {
		// Move up following lines but merge header flag from this line
		// to line before to avoid a temporary disappearence causing expansion.
		int firstHeader = levels[line] & SC_FOLDLEVELHEADERFLAG;
		levels.Delete(line);
		if (line > 0)
			levels[line-1] |= firstHeader;
	}
}

int LineVector::LineFromPosition(int pos) {
	return starts.PartitionFromPosition(pos);
}

int LineVector::MarkValue(int line) {
	if (markers.Length() && markers[line])
		return markers[line]->MarkValue();
	else
		return 0;
}

int LineVector::AddMark(int line, int markerNum) {
	handleCurrent++;
	if (!markers.Length()) {
		// No existing markers so allocate one element per line
		markers.InsertValue(0, Lines(), 0);
	}
	if (!markers[line]) {
		// Need new structure to hold marker handle
		markers[line] = new MarkerHandleSet();
		if (!markers[line])
			return - 1;
	}
	markers[line]->InsertHandle(handleCurrent, markerNum);

	return handleCurrent;
}

void LineVector::MergeMarkers(int pos) {
	if (markers[pos + 1] != NULL) {
		if (markers[pos] == NULL)
			markers[pos] = new MarkerHandleSet;
		markers[pos]->CombineWith(markers[pos + 1]);
		delete markers[pos + 1];
		markers[pos + 1] = NULL;
	}
}

void LineVector::DeleteMark(int line, int markerNum, bool all) {
	if (markers.Length() && markers[line]) {
		if (markerNum == -1) {
			delete markers[line];
			markers[line] = NULL;
		} else {
			bool performedDeletion = markers[line]->RemoveNumber(markerNum);
			while (all && performedDeletion) {
				performedDeletion = markers[line]->RemoveNumber(markerNum);
			}
			if (markers[line]->Length() == 0) {
				delete markers[line];
				markers[line] = NULL;
			}
		}
	}
}

void LineVector::DeleteMarkFromHandle(int markerHandle) {
	int line = LineFromHandle(markerHandle);
	if (line >= 0) {
		markers[line]->RemoveHandle(markerHandle);
		if (markers[line]->Length() == 0) {
			delete markers[line];
			markers[line] = NULL;
		}
	}
}

int LineVector::LineFromHandle(int markerHandle) {
	if (markers.Length()) {
		for (int line = 0; line < Lines(); line++) {
			if (markers[line]) {
				if (markers[line]->Contains(markerHandle)) {
					return line;
				}
			}
		}
	}
	return -1;
}

Action::Action() {
	at = startAction;
	position = 0;
	data = 0;
	lenData = 0;
}

Action::~Action() {
	Destroy();
}

void Action::Create(actionType at_, int position_, char *data_, int lenData_, bool mayCoalesce_) {
	delete []data;
	position = position_;
	at = at_;
	data = data_;
	lenData = lenData_;
	mayCoalesce = mayCoalesce_;
}

void Action::Destroy() {
	delete []data;
	data = 0;
}

void Action::Grab(Action *source) {
	delete []data;

	position = source->position;
	at = source->at;
	data = source->data;
	lenData = source->lenData;
	mayCoalesce = source->mayCoalesce;

	// Ownership of source data transferred to this
	source->position = 0;
	source->at = startAction;
	source->data = 0;
	source->lenData = 0;
	source->mayCoalesce = true;
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

	lenActions = 100;
	actions = new Action[lenActions];
	maxAction = 0;
	currentAction = 0;
	undoSequenceDepth = 0;
	savePoint = 0;

	actions[currentAction].Create(startAction);
}

UndoHistory::~UndoHistory() {
	delete []actions;
	actions = 0;
}

void UndoHistory::EnsureUndoRoom() {
	// Have to test that there is room for 2 more actions in the array
	// as two actions may be created by the calling function
	if (currentAction >= (lenActions - 2)) {
		// Run out of undo nodes so extend the array
		int lenActionsNew = lenActions * 2;
		Action *actionsNew = new Action[lenActionsNew];
		if (!actionsNew)
			return;
		for (int act = 0; act <= currentAction; act++)
			actionsNew[act].Grab(&actions[act]);
		delete []actions;
		lenActions = lenActionsNew;
		actions = actionsNew;
	}
}

void UndoHistory::AppendAction(actionType at, int position, char *data, int lengthData,
	bool &startSequence) {
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
			Action &actPrevious = actions[currentAction - 1];
			// See if current action can be coalesced into previous action
			// Will work if both are inserts or deletes and position is same
			if (at != actPrevious.at) {
				currentAction++;
			} else if (currentAction == savePoint) {
				currentAction++;
			} else if ((at == insertAction) &&
			           (position != (actPrevious.position + actPrevious.lenData))) {
				// Insertions must be immediately after to coalesce
				currentAction++;
			} else if (!actions[currentAction].mayCoalesce) {
				// Not allowed to coalesce if this set
				currentAction++;
			} else if (at == removeAction) {
				if ((lengthData == 1) || (lengthData == 2)){
					if ((position + lengthData) == actPrevious.position) {
						; // Backspace -> OK
					} else if (position == actPrevious.position) {
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
	actions[currentAction].Create(at, position, data, lengthData);
	currentAction++;
	actions[currentAction].Create(startAction);
	maxAction = currentAction;
}

void UndoHistory::BeginUndoAction() {
	EnsureUndoRoom();
	if (undoSequenceDepth == 0) {
		if (actions[currentAction].at != startAction) {
			currentAction++;
			actions[currentAction].Create(startAction);
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
		if (actions[currentAction].at != startAction) {
			currentAction++;
			actions[currentAction].Create(startAction);
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
		actions[i].Destroy();
	maxAction = 0;
	currentAction = 0;
	actions[currentAction].Create(startAction);
	savePoint = 0;
}

void UndoHistory::SetSavePoint() {
	savePoint = currentAction;
}

bool UndoHistory::IsSavePoint() const {
	return savePoint == currentAction;
}

bool UndoHistory::CanUndo() const {
	return (currentAction > 0) && (maxAction > 0);
}

int UndoHistory::StartUndo() {
	// Drop any trailing startAction
	if (actions[currentAction].at == startAction && currentAction > 0)
		currentAction--;

	// Count the steps in this action
	int act = currentAction;
	while (actions[act].at != startAction && act > 0) {
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

bool UndoHistory::CanRedo() const {
	return maxAction > currentAction;
}

int UndoHistory::StartRedo() {
	// Drop any leading startAction
	if (actions[currentAction].at == startAction && currentAction < maxAction)
		currentAction++;

	// Count the steps in this action
	int act = currentAction;
	while (actions[act].at != startAction && act < maxAction) {
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

CellBuffer::CellBuffer() {
	readOnly = false;
	collectingUndo = true;
}

CellBuffer::~CellBuffer() {
}

char CellBuffer::CharAt(int position) const {
	return substance.ValueAt(position);
}

void CellBuffer::GetCharRange(char *buffer, int position, int lengthRetrieve) {
	if (lengthRetrieve < 0)
		return;
	if (position < 0)
		return;
	if ((position + lengthRetrieve) > substance.Length()) {
		Platform::DebugPrintf("Bad GetCharRange %d for %d of %d\n", position,
		                      lengthRetrieve, substance.Length());
		return;
	}
	
	for (int i=0; i<lengthRetrieve; i++) {
		*buffer++ = substance.ValueAt(position + i);
	}
}

char CellBuffer::StyleAt(int position) {
	return style.ValueAt(position);
}

const char *CellBuffer::BufferPointer() { 
	return substance.BufferPointer();
}

// The char* returned is to an allocation owned by the undo history
const char *CellBuffer::InsertString(int position, const char *s, int insertLength, bool &startSequence) {
	char *data = 0;
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	if (!readOnly) {
		if (collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			// This takes up about half load time
			data = new char[insertLength];
			for (int i = 0; i < insertLength; i++) {
				data[i] = s[i];
			}
			uh.AppendAction(insertAction, position, data, insertLength, startSequence);
		}

		BasicInsertString(position, s, insertLength);
	}
	return data;
}

bool CellBuffer::SetStyleAt(int position, char styleValue, char mask) {
	styleValue &= mask;
	char curVal = style.ValueAt(position);
	if ((curVal & mask) != styleValue) {
		style.SetValueAt(position, static_cast<char>((curVal & ~mask) | styleValue));
		return true;
	} else {
		return false;
	}
}

bool CellBuffer::SetStyleFor(int position, int lengthStyle, char styleValue, char mask) {
	bool changed = false;
	PLATFORM_ASSERT(lengthStyle == 0 ||
		(lengthStyle > 0 && lengthStyle + position <= style.Length()));
	while (lengthStyle--) {
		char curVal = style.ValueAt(position);
		if ((curVal & mask) != styleValue) {
			style.SetValueAt(position, static_cast<char>((curVal & ~mask) | styleValue));
			changed = true;
		}
		position++;
	}
	return changed;
}

// The char* returned is to an allocation owned by the undo history
const char *CellBuffer::DeleteChars(int position, int deleteLength, bool &startSequence) {
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	PLATFORM_ASSERT(deleteLength > 0);
	char *data = 0;
	if (!readOnly) {
		if (collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			data = new char[deleteLength];
			for (int i = 0; i < deleteLength; i++) {
				data[i] = substance.ValueAt(position + i);
			}
			uh.AppendAction(removeAction, position, data, deleteLength, startSequence);
		}

		BasicDeleteChars(position, deleteLength);
	}
	return data;
}

int CellBuffer::Length() const {
	return substance.Length();
}

void CellBuffer::Allocate(int newSize) {
	substance.ReAllocate(newSize);
	style.ReAllocate(newSize);
}

int CellBuffer::Lines() const {
	return lv.Lines();
}

int CellBuffer::LineStart(int line) const {
	if (line < 0)
		return 0;
	else if (line >= Lines())
		return Length();
	else
		return lv.LineStart(line);
}

bool CellBuffer::IsReadOnly() {
	return readOnly;
}

void CellBuffer::SetReadOnly(bool set) {
	readOnly = set;
}

void CellBuffer::SetSavePoint() {
	uh.SetSavePoint();
}

bool CellBuffer::IsSavePoint() {
	return uh.IsSavePoint();
}

int CellBuffer::AddMark(int line, int markerNum) {
	if ((line >= 0) && (line < Lines())) {
		return lv.AddMark(line, markerNum);
	}
	return - 1;
}

void CellBuffer::DeleteMark(int line, int markerNum) {
	if ((line >= 0) && (line < Lines())) {
		lv.DeleteMark(line, markerNum, false);
	}
}

void CellBuffer::DeleteMarkFromHandle(int markerHandle) {
	lv.DeleteMarkFromHandle(markerHandle);
}

int CellBuffer::GetMark(int line) {
	if ((line >= 0) && (line < Lines()))
		return lv.MarkValue(line);
	return 0;
}

void CellBuffer::DeleteAllMarks(int markerNum) {
	for (int line = 0; line < Lines(); line++) {
		lv.DeleteMark(line, markerNum, true);
	}
}

int CellBuffer::LineFromHandle(int markerHandle) {
	return lv.LineFromHandle(markerHandle);
}

// Without undo

void CellBuffer::InsertLine(int line, int position) {
	lv.InsertLine(line, position);
	if (lineStates.Length()) {
		lineStates.EnsureLength(line);
		lineStates.Insert(line, 0);
	}
}

void CellBuffer::RemoveLine(int line) {
	lv.RemoveLine(line);
	if (lineStates.Length() > line) {
		lineStates.Delete(line);
	}
}

void CellBuffer::BasicInsertString(int position, const char *s, int insertLength) {
	if (insertLength == 0)
		return;
	PLATFORM_ASSERT(insertLength > 0);

	substance.InsertFromArray(position, s, 0, insertLength);
	style.InsertValue(position, insertLength, 0);

	int lineInsert = lv.LineFromPosition(position) + 1;
	// Point all the lines after the insertion point further along in the buffer
	lv.InsertText(lineInsert-1, insertLength);
	char chPrev = substance.ValueAt(position - 1);
	char chAfter = substance.ValueAt(position + insertLength);
	if (chPrev == '\r' && chAfter == '\n') {
		// Splitting up a crlf pair at position
		InsertLine(lineInsert, position);
		lineInsert++;
	}
	char ch = ' ';
	for (int i = 0; i < insertLength; i++) {
		ch = s[i];
		if (ch == '\r') {
			InsertLine(lineInsert, (position + i) + 1);
			lineInsert++;
		} else if (ch == '\n') {
			if (chPrev == '\r') {
				// Patch up what was end of line
				lv.SetLineStart(lineInsert - 1, (position + i) + 1);
			} else {
				InsertLine(lineInsert, (position + i) + 1);
				lineInsert++;
			}
		}
		chPrev = ch;
	}
	// Joining two lines where last insertion is cr and following substance starts with lf
	if (chAfter == '\n') {
		if (ch == '\r') {
			// End of line already in buffer so drop the newly created one
			RemoveLine(lineInsert - 1);
		}
	}
}

void CellBuffer::BasicDeleteChars(int position, int deleteLength) {
	if (deleteLength == 0)
		return;

	if ((position == 0) && (deleteLength == substance.Length())) {
		// If whole buffer is being deleted, faster to reinitialise lines data
		// than to delete each line.
		lv.Init();
	} else {
		// Have to fix up line positions before doing deletion as looking at text in buffer
		// to work out which lines have been removed

		int lineRemove = lv.LineFromPosition(position) + 1;
		lv.InsertText(lineRemove-1, - (deleteLength));
		char chPrev = substance.ValueAt(position - 1);
		char chBefore = chPrev;
		char chNext = substance.ValueAt(position);
		bool ignoreNL = false;
		if (chPrev == '\r' && chNext == '\n') {
			// Move back one
			lv.SetLineStart(lineRemove, position);
			lineRemove++;
			ignoreNL = true; 	// First \n is not real deletion
		}

		char ch = chNext;
		for (int i = 0; i < deleteLength; i++) {
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
			}

			ch = chNext;
		}
		// May have to fix up end if last deletion causes cr to be next to lf
		// or removes one of a crlf pair
		char chAfter = substance.ValueAt(position + deleteLength);
		if (chBefore == '\r' && chAfter == '\n') {
			// Using lineRemove-1 as cr ended line before start of deletion
			RemoveLine(lineRemove - 1);
			lv.SetLineStart(lineRemove - 1, position + 1);
		}
	}
	substance.DeleteRange(position, deleteLength);
	style.DeleteRange(position, deleteLength);
}

bool CellBuffer::SetUndoCollection(bool collectUndo) {
	collectingUndo = collectUndo;
	uh.DropUndoSequence();
	return collectingUndo;
}

bool CellBuffer::IsCollectingUndo() {
	return collectingUndo;
}

void CellBuffer::BeginUndoAction() {
	uh.BeginUndoAction();
}

void CellBuffer::EndUndoAction() {
	uh.EndUndoAction();
}

void CellBuffer::DeleteUndoHistory() {
	uh.DeleteUndoHistory();
}

bool CellBuffer::CanUndo() {
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
	if (actionStep.at == insertAction) {
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	} else if (actionStep.at == removeAction) {
		BasicInsertString(actionStep.position, actionStep.data, actionStep.lenData);
	}
	uh.CompletedUndoStep();
}

bool CellBuffer::CanRedo() {
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
	if (actionStep.at == insertAction) {
		BasicInsertString(actionStep.position, actionStep.data, actionStep.lenData);
	} else if (actionStep.at == removeAction) {
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	}
	uh.CompletedRedoStep();
}

int CellBuffer::SetLineState(int line, int state) {
	lineStates.EnsureLength(line + 1);
	int stateOld = lineStates[line];
	lineStates[line] = state;
	return stateOld;
}

int CellBuffer::GetLineState(int line) {
	lineStates.EnsureLength(line + 1);
	return lineStates[line];
}

int CellBuffer::GetMaxLineState() {
	return lineStates.Length();
}

int CellBuffer::SetLevel(int line, int level) {
	return lv.SetLevel(line, level);
}

int CellBuffer::GetLevel(int line) {
	return lv.GetLevel(line);
}

void CellBuffer::ClearLevels() {
	lv.ClearLevels();
}
