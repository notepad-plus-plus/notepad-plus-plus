// Scintilla source code edit control
/** @file Selection.cxx
 ** Classes maintaining the selection.
 **/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>

#include <vector>
#include <algorithm>

#include "Platform.h"

#include "Scintilla.h"

#include "Selection.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

void SelectionPosition::MoveForInsertDelete(bool insertion, int startChange, int length) {
	if (insertion) {
		if (position == startChange) {
			int virtualLengthRemove = std::min(length, virtualSpace);
			virtualSpace -= virtualLengthRemove;
			position += virtualLengthRemove;
		} else if (position > startChange) {
			position += length;
		}
	} else {
		if (position == startChange) {
			virtualSpace = 0;
		}
		if (position > startChange) {
			int endDeletion = startChange + length;
			if (position > endDeletion) {
				position -= length;
			} else {
				position = startChange;
				virtualSpace = 0;
			}
		}
	}
}

bool SelectionPosition::operator <(const SelectionPosition &other) const {
	if (position == other.position)
		return virtualSpace < other.virtualSpace;
	else
		return position < other.position;
}

bool SelectionPosition::operator >(const SelectionPosition &other) const {
	if (position == other.position)
		return virtualSpace > other.virtualSpace;
	else
		return position > other.position;
}

bool SelectionPosition::operator <=(const SelectionPosition &other) const {
	if (position == other.position && virtualSpace == other.virtualSpace)
		return true;
	else
		return other > *this;
}

bool SelectionPosition::operator >=(const SelectionPosition &other) const {
	if (position == other.position && virtualSpace == other.virtualSpace)
		return true;
	else
		return *this > other;
}

int SelectionRange::Length() const {
	if (anchor > caret) {
		return anchor.Position() - caret.Position();
	} else {
		return caret.Position() - anchor.Position();
	}
}

bool SelectionRange::Contains(int pos) const {
	if (anchor > caret)
		return (pos >= caret.Position()) && (pos <= anchor.Position());
	else
		return (pos >= anchor.Position()) && (pos <= caret.Position());
}

bool SelectionRange::Contains(SelectionPosition sp) const {
	if (anchor > caret)
		return (sp >= caret) && (sp <= anchor);
	else
		return (sp >= anchor) && (sp <= caret);
}

bool SelectionRange::ContainsCharacter(int posCharacter) const {
	if (anchor > caret)
		return (posCharacter >= caret.Position()) && (posCharacter < anchor.Position());
	else
		return (posCharacter >= anchor.Position()) && (posCharacter < caret.Position());
}

SelectionSegment SelectionRange::Intersect(SelectionSegment check) const {
	SelectionSegment inOrder(caret, anchor);
	if ((inOrder.start <= check.end) || (inOrder.end >= check.start)) {
		SelectionSegment portion = check;
		if (portion.start < inOrder.start)
			portion.start = inOrder.start;
		if (portion.end > inOrder.end)
			portion.end = inOrder.end;
		if (portion.start > portion.end)
			return SelectionSegment();
		else
			return portion;
	} else {
		return SelectionSegment();
	}
}

bool SelectionRange::Trim(SelectionRange range) {
	SelectionPosition startRange = range.Start();
	SelectionPosition endRange = range.End();
	SelectionPosition start = Start();
	SelectionPosition end = End();
	PLATFORM_ASSERT(start <= end);
	PLATFORM_ASSERT(startRange <= endRange);
	if ((startRange <= end) && (endRange >= start)) {
		if ((start > startRange) && (end < endRange)) {
			// Completely covered by range -> empty at start
			end = start;
		} else if ((start < startRange) && (end > endRange)) {
			// Completely covers range -> empty at start
			end = start;
		} else if (start <= startRange) {
			// Trim end
			end = startRange;
		} else { //
			PLATFORM_ASSERT(end >= endRange);
			// Trim start
			start = endRange;
		}
		if (anchor > caret) {
			caret = start;
			anchor = end;
		} else {
			anchor = start;
			caret = end;
		}
		return Empty();
	} else {
		return false;
	}
}

// If range is all virtual collapse to start of virtual space
void SelectionRange::MinimizeVirtualSpace() {
	if (caret.Position() == anchor.Position()) {
		int virtualSpace = caret.VirtualSpace();
		if (virtualSpace > anchor.VirtualSpace())
			virtualSpace = anchor.VirtualSpace();
		caret.SetVirtualSpace(virtualSpace);
		anchor.SetVirtualSpace(virtualSpace);
	}
}

Selection::Selection() : mainRange(0), moveExtends(false), tentativeMain(false), selType(selStream) {
	AddSelection(SelectionPosition(0));
}

Selection::~Selection() {
}

bool Selection::IsRectangular() const {
	return (selType == selRectangle) || (selType == selThin);
}

int Selection::MainCaret() const {
	return ranges[mainRange].caret.Position();
}

int Selection::MainAnchor() const {
	return ranges[mainRange].anchor.Position();
}

SelectionRange &Selection::Rectangular() {
	return rangeRectangular;
}

SelectionSegment Selection::Limits() const {
	if (ranges.empty()) {
		return SelectionSegment();
	} else {
		SelectionSegment sr(ranges[0].anchor, ranges[0].caret);
		for (size_t i=1; i<ranges.size(); i++) {
			sr.Extend(ranges[i].anchor);
			sr.Extend(ranges[i].caret);
		}
		return sr;
	}
}

SelectionSegment Selection::LimitsForRectangularElseMain() const {
	if (IsRectangular()) {
		return Limits();
	} else {
		return SelectionSegment(ranges[mainRange].caret, ranges[mainRange].anchor);
	}
}

size_t Selection::Count() const {
	return ranges.size();
}

size_t Selection::Main() const {
	return mainRange;
}

void Selection::SetMain(size_t r) {
	PLATFORM_ASSERT(r < ranges.size());
	mainRange = r;
}

SelectionRange &Selection::Range(size_t r) {
	return ranges[r];
}

SelectionRange &Selection::RangeMain() {
	return ranges[mainRange];
}

bool Selection::MoveExtends() const {
	return moveExtends;
}

void Selection::SetMoveExtends(bool moveExtends_) {
	moveExtends = moveExtends_;
}

bool Selection::Empty() const {
	for (size_t i=0; i<ranges.size(); i++) {
		if (!ranges[i].Empty())
			return false;
	}
	return true;
}

SelectionPosition Selection::Last() const {
	SelectionPosition lastPosition;
	for (size_t i=0; i<ranges.size(); i++) {
		if (lastPosition < ranges[i].caret)
			lastPosition = ranges[i].caret;
		if (lastPosition < ranges[i].anchor)
			lastPosition = ranges[i].anchor;
	}
	return lastPosition;
}

int Selection::Length() const {
	int len = 0;
	for (size_t i=0; i<ranges.size(); i++) {
		len += ranges[i].Length();
	}
	return len;
}

void Selection::MovePositions(bool insertion, int startChange, int length) {
	for (size_t i=0; i<ranges.size(); i++) {
		ranges[i].caret.MoveForInsertDelete(insertion, startChange, length);
		ranges[i].anchor.MoveForInsertDelete(insertion, startChange, length);
	}
}

void Selection::TrimSelection(SelectionRange range) {
	for (size_t i=0; i<ranges.size();) {
		if ((i != mainRange) && (ranges[i].Trim(range))) {
			// Trimmed to empty so remove
			for (size_t j=i; j<ranges.size()-1; j++) {
				ranges[j] = ranges[j+1];
				if (j == mainRange-1)
					mainRange--;
			}
			ranges.pop_back();
		} else {
			i++;
		}
	}
}

void Selection::SetSelection(SelectionRange range) {
	ranges.clear();
	ranges.push_back(range);
	mainRange = ranges.size() - 1;
}

void Selection::AddSelection(SelectionRange range) {
	TrimSelection(range);
	ranges.push_back(range);
	mainRange = ranges.size() - 1;
}

void Selection::AddSelectionWithoutTrim(SelectionRange range) {
	ranges.push_back(range);
	mainRange = ranges.size() - 1;
}

void Selection::TentativeSelection(SelectionRange range) {
	if (!tentativeMain) {
		rangesSaved = ranges;
	}
	ranges = rangesSaved;
	AddSelection(range);
	TrimSelection(ranges[mainRange]);
	tentativeMain = true;
}

void Selection::CommitTentative() {
	rangesSaved.clear();
	tentativeMain = false;
}

int Selection::CharacterInSelection(int posCharacter) const {
	for (size_t i=0; i<ranges.size(); i++) {
		if (ranges[i].ContainsCharacter(posCharacter))
			return i == mainRange ? 1 : 2;
	}
	return 0;
}

int Selection::InSelectionForEOL(int pos) const {
	for (size_t i=0; i<ranges.size(); i++) {
		if (!ranges[i].Empty() && (pos > ranges[i].Start().Position()) && (pos <= ranges[i].End().Position()))
			return i == mainRange ? 1 : 2;
	}
	return 0;
}

int Selection::VirtualSpaceFor(int pos) const {
	int virtualSpace = 0;
	for (size_t i=0; i<ranges.size(); i++) {
		if ((ranges[i].caret.Position() == pos) && (virtualSpace < ranges[i].caret.VirtualSpace()))
			virtualSpace = ranges[i].caret.VirtualSpace();
		if ((ranges[i].anchor.Position() == pos) && (virtualSpace < ranges[i].anchor.VirtualSpace()))
			virtualSpace = ranges[i].anchor.VirtualSpace();
	}
	return virtualSpace;
}

void Selection::Clear() {
	ranges.clear();
	ranges.push_back(SelectionRange());
	mainRange = ranges.size() - 1;
	selType = selStream;
	moveExtends = false;
	ranges[mainRange].Reset();
	rangeRectangular.Reset();
}

void Selection::RemoveDuplicates() {
	for (size_t i=0; i<ranges.size()-1; i++) {
		if (ranges[i].Empty()) {
			size_t j=i+1;
			while (j<ranges.size()) {
				if (ranges[i] == ranges[j]) {
					ranges.erase(ranges.begin() + j);
					if (mainRange >= j)
						mainRange--;
				} else {
					j++;
				}
			}
		}
	}
}

void Selection::RotateMain() {
	mainRange = (mainRange + 1) % ranges.size();
}

