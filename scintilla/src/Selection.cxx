// Scintilla source code edit control
/** @file Selection.cxx
 ** Classes maintaining the selection.
 **/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "Debugging.h"

#include "Position.h"
#include "Selection.h"

using namespace Scintilla::Internal;

void SelectionPosition::MoveForInsertDelete(bool insertion, Sci::Position startChange, Sci::Position length, bool moveForEqual) noexcept {
	if (insertion) {
		if (position == startChange) {
			// Always consume virtual space
			const Sci::Position virtualLengthRemove = std::min(length, virtualSpace);
			virtualSpace -= virtualLengthRemove;
			position += virtualLengthRemove;
			if (moveForEqual) {
				const Sci::Position lengthAfterVirtualRemove = length - virtualLengthRemove;
				position += lengthAfterVirtualRemove;
			}
		} else if (position > startChange) {
			position += length;
		}
	} else {
		if (position == startChange) {
			virtualSpace = 0;
		}
		if (position > startChange) {
			const Sci::Position endDeletion = startChange + length;
			if (position > endDeletion) {
				position -= length;
			} else {
				position = startChange;
				virtualSpace = 0;
			}
		}
	}
}

bool SelectionPosition::operator <(const SelectionPosition &other) const noexcept {
	if (position == other.position)
		return virtualSpace < other.virtualSpace;
	else
		return position < other.position;
}

bool SelectionPosition::operator >(const SelectionPosition &other) const noexcept {
	if (position == other.position)
		return virtualSpace > other.virtualSpace;
	else
		return position > other.position;
}

bool SelectionPosition::operator <=(const SelectionPosition &other) const noexcept {
	if (position == other.position && virtualSpace == other.virtualSpace)
		return true;
	else
		return other > *this;
}

bool SelectionPosition::operator >=(const SelectionPosition &other) const noexcept {
	if (position == other.position && virtualSpace == other.virtualSpace)
		return true;
	else
		return *this > other;
}

Sci::Position SelectionRange::Length() const noexcept {
	if (anchor > caret) {
		return anchor.Position() - caret.Position();
	} else {
		return caret.Position() - anchor.Position();
	}
}

void SelectionRange::MoveForInsertDelete(bool insertion, Sci::Position startChange, Sci::Position length) noexcept {
	// For insertions that occur at the start of the selection move both the start
	// and end of the selection to preserve the selected length.
	// The end will automatically move since it is after the insertion, so determine
	// which position is the start and pass this into
	// SelectionPosition::MoveForInsertDelete.
	// There isn't any reason to move an empty selection so don't move it.
	const bool caretStart = caret.Position() < anchor.Position();
	const bool anchorStart = anchor.Position() < caret.Position();

	caret.MoveForInsertDelete(insertion, startChange, length, caretStart);
	anchor.MoveForInsertDelete(insertion, startChange, length, anchorStart);
}

bool SelectionRange::Contains(Sci::Position pos) const noexcept {
	if (anchor > caret)
		return (pos >= caret.Position()) && (pos <= anchor.Position());
	else
		return (pos >= anchor.Position()) && (pos <= caret.Position());
}

bool SelectionRange::Contains(SelectionPosition sp) const noexcept {
	if (anchor > caret)
		return (sp >= caret) && (sp <= anchor);
	else
		return (sp >= anchor) && (sp <= caret);
}

bool SelectionRange::ContainsCharacter(Sci::Position posCharacter) const noexcept {
	if (anchor > caret)
		return (posCharacter >= caret.Position()) && (posCharacter < anchor.Position());
	else
		return (posCharacter >= anchor.Position()) && (posCharacter < caret.Position());
}

SelectionSegment SelectionRange::Intersect(SelectionSegment check) const noexcept {
	const SelectionSegment inOrder(caret, anchor);
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

void SelectionRange::Swap() noexcept {
	std::swap(caret, anchor);
}

bool SelectionRange::Trim(SelectionRange range) noexcept {
	const SelectionPosition startRange = range.Start();
	const SelectionPosition endRange = range.End();
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
void SelectionRange::MinimizeVirtualSpace() noexcept {
	if (caret.Position() == anchor.Position()) {
		Sci::Position virtualSpace = caret.VirtualSpace();
		if (virtualSpace > anchor.VirtualSpace())
			virtualSpace = anchor.VirtualSpace();
		caret.SetVirtualSpace(virtualSpace);
		anchor.SetVirtualSpace(virtualSpace);
	}
}

Selection::Selection() : mainRange(0), moveExtends(false), tentativeMain(false), selType(SelTypes::stream) {
	AddSelection(SelectionRange(SelectionPosition(0)));
}

bool Selection::IsRectangular() const noexcept {
	return (selType == SelTypes::rectangle) || (selType == SelTypes::thin);
}

Sci::Position Selection::MainCaret() const noexcept {
	return ranges[mainRange].caret.Position();
}

Sci::Position Selection::MainAnchor() const noexcept {
	return ranges[mainRange].anchor.Position();
}

SelectionRange &Selection::Rectangular() noexcept {
	return rangeRectangular;
}

SelectionSegment Selection::Limits() const noexcept {
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

size_t Selection::Count() const noexcept {
	return ranges.size();
}

size_t Selection::Main() const noexcept {
	return mainRange;
}

void Selection::SetMain(size_t r) noexcept {
	PLATFORM_ASSERT(r < ranges.size());
	mainRange = r;
}

SelectionRange &Selection::Range(size_t r) noexcept {
	return ranges[r];
}

const SelectionRange &Selection::Range(size_t r) const noexcept {
	return ranges[r];
}

SelectionRange &Selection::RangeMain() noexcept {
	return ranges[mainRange];
}

const SelectionRange &Selection::RangeMain() const noexcept {
	return ranges[mainRange];
}

SelectionPosition Selection::Start() const noexcept {
	if (IsRectangular()) {
		return rangeRectangular.Start();
	} else {
		return ranges[mainRange].Start();
	}
}

bool Selection::MoveExtends() const noexcept {
	return moveExtends;
}

void Selection::SetMoveExtends(bool moveExtends_) noexcept {
	moveExtends = moveExtends_;
}

bool Selection::Empty() const noexcept {
	for (const SelectionRange &range : ranges) {
		if (!range.Empty())
			return false;
	}
	return true;
}

SelectionPosition Selection::Last() const noexcept {
	SelectionPosition lastPosition;
	for (const SelectionRange &range : ranges) {
		if (lastPosition < range.caret)
			lastPosition = range.caret;
		if (lastPosition < range.anchor)
			lastPosition = range.anchor;
	}
	return lastPosition;
}

Sci::Position Selection::Length() const noexcept {
	Sci::Position len = 0;
	for (const SelectionRange &range : ranges) {
		len += range.Length();
	}
	return len;
}

void Selection::MovePositions(bool insertion, Sci::Position startChange, Sci::Position length) noexcept {
	for (SelectionRange &range : ranges) {
		range.MoveForInsertDelete(insertion, startChange, length);
	}
	if (selType == SelTypes::rectangle) {
		rangeRectangular.MoveForInsertDelete(insertion, startChange, length);
	}
}

void Selection::TrimSelection(SelectionRange range) noexcept {
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

void Selection::TrimOtherSelections(size_t r, SelectionRange range) noexcept {
	for (size_t i = 0; i<ranges.size(); ++i) {
		if (i != r) {
			ranges[i].Trim(range);
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

void Selection::DropSelection(size_t r) {
	if ((ranges.size() > 1) && (r < ranges.size())) {
		size_t mainNew = mainRange;
		if (mainNew >= r) {
			if (mainNew == 0) {
				mainNew = ranges.size() - 2;
			} else {
				mainNew--;
			}
		}
		ranges.erase(ranges.begin() + r);
		mainRange = mainNew;
	}
}

void Selection::DropAdditionalRanges() {
	SetSelection(RangeMain());
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

void Selection::CommitTentative() noexcept {
	rangesSaved.clear();
	tentativeMain = false;
}

InSelection Selection::RangeType(size_t r) const noexcept {
	return r == Main() ? InSelection::inMain : InSelection::inAdditional;
}

InSelection Selection::CharacterInSelection(Sci::Position posCharacter) const noexcept {
	for (size_t i=0; i<ranges.size(); i++) {
		if (ranges[i].ContainsCharacter(posCharacter))
			return RangeType(i);
	}
	return InSelection::inNone;
}

InSelection Selection::InSelectionForEOL(Sci::Position pos) const noexcept {
	for (size_t i=0; i<ranges.size(); i++) {
		if (!ranges[i].Empty() && (pos > ranges[i].Start().Position()) && (pos <= ranges[i].End().Position()))
			return RangeType(i);
	}
	return InSelection::inNone;
}

Sci::Position Selection::VirtualSpaceFor(Sci::Position pos) const noexcept {
	Sci::Position virtualSpace = 0;
	for (const SelectionRange &range : ranges) {
		if ((range.caret.Position() == pos) && (virtualSpace < range.caret.VirtualSpace()))
			virtualSpace = range.caret.VirtualSpace();
		if ((range.anchor.Position() == pos) && (virtualSpace < range.anchor.VirtualSpace()))
			virtualSpace = range.anchor.VirtualSpace();
	}
	return virtualSpace;
}

void Selection::Clear() {
	ranges.clear();
	ranges.emplace_back();
	mainRange = ranges.size() - 1;
	selType = SelTypes::stream;
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

void Selection::RotateMain() noexcept {
	mainRange = (mainRange + 1) % ranges.size();
}

