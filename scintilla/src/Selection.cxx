// Scintilla source code edit control
/** @file Selection.cxx
 ** Classes maintaining the selection.
 **/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>
#include <charconv>

#include "Debugging.h"

#include "Position.h"
#include "Selection.h"

using namespace Scintilla::Internal;

namespace {

// Generically convert a string to a integer value throwing if the conversion failed.
// Failures include values that are out of range for the destination variable.
template <typename T>
void ValueFromString(std::string_view sv, T &value) {
	const std::from_chars_result res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
	if (res.ec != std::errc{}) {
		if (res.ec == std::errc::result_out_of_range)
			throw std::runtime_error("from_chars out of range.");
		throw std::runtime_error("from_chars failed.");
	}
}

}

SelectionPosition::SelectionPosition(std::string_view sv) : position(0) {
	if (const size_t v = sv.find('v'); v != std::string_view::npos) {
		ValueFromString(sv.substr(v + 1), virtualSpace);
		sv = sv.substr(0, v);
	}
	ValueFromString(sv, position);
}

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

bool SelectionPosition::operator >(const SelectionPosition &other) const noexcept {
	if (position == other.position)
		return virtualSpace > other.virtualSpace;
	return position > other.position;
}

bool SelectionPosition::operator <=(const SelectionPosition &other) const noexcept {
	if (other == *this)
		return true;
	return other > *this;
}

bool SelectionPosition::operator >=(const SelectionPosition &other) const noexcept {
	if (other == *this)
		return true;
	return *this > other;
}

std::string SelectionPosition::ToString() const {
	std::string result = std::to_string(position);
	if (virtualSpace) {
		result += 'v';
		result += std::to_string(virtualSpace);
	}
	return result;
}

SelectionRange::SelectionRange(std::string_view sv) {
	const size_t dash = sv.find('-');
	if (dash == std::string_view::npos) {
		anchor = SelectionPosition(sv);
		caret = anchor;
	} else {
		anchor = SelectionPosition(sv.substr(0, dash));
		caret = SelectionPosition(sv.substr(dash + 1));
	}
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

bool SelectionRange::ContainsCharacter(SelectionPosition spCharacter) const noexcept {
	if (anchor > caret)
		return (spCharacter >= caret) && (spCharacter < anchor);
	else
		return (spCharacter >= anchor) && (spCharacter < caret);
}

SelectionSegment SelectionRange::Intersect(SelectionSegment check) const noexcept {
	const SelectionSegment inOrder = AsSegment();
	if ((inOrder.start > check.end) || (inOrder.end < check.start)) {
		// Nothing in common, not even touching so return empty *invalid* segment
		return {};
	}
	return {
		std::max(check.start, inOrder.start),
		std::min(check.end, inOrder.end)
	};
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
	}
	return false;
}

void SelectionRange::Truncate(Sci::Position length) noexcept {
	if (anchor.Position() > length)
		anchor.SetPosition(length);
	if (caret.Position() > length)
		caret.SetPosition(length);
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

std::string SelectionRange::ToString() const {
	std::string result = anchor.ToString();
	if (!(caret == anchor)) {
		result += '-';
		result += caret.ToString();
	}
	return result;
}

Selection::Selection() : mainRange(0), moveExtends(false), tentativeMain(false), selType(SelTypes::stream) {
	AddSelection(SelectionRange(SelectionPosition(0)));
}

Selection::Selection(std::string_view sv) : mainRange(0), moveExtends(false), tentativeMain(false), selType(SelTypes::stream) {
	if (sv.empty()) {
		return;
	}
	try {
		// Decode initial letter prefix if any
		switch (sv.front()) {
		case 'R':
			selType = SelTypes::rectangle;
			break;
		case 'L':
			selType = SelTypes::lines;
			break;
		case 'T':
			selType = SelTypes::thin;
			break;
		default:
			break;
		}
		if (selType != SelTypes::stream) {
			sv.remove_prefix(1);
		}

		// Non-zero main index at end after '#'
		if (const size_t hash = sv.find('#'); hash != std::string_view::npos) {
			ValueFromString(sv.substr(hash + 1), mainRange);
			sv = sv.substr(0, hash);
		}

		// Remainder is list of ranges
		if (selType == SelTypes::rectangle || selType == SelTypes::thin) {
			rangeRectangular = SelectionRange(sv);
			// Ensure enough ranges exist for mainRange to be in bounds
			for (size_t i = 0; i <= mainRange; i++) {
				ranges.emplace_back(SelectionPosition(0));
			}
		} else {
			size_t comma = sv.find(',');
			while (comma != std::string_view::npos) {
				ranges.emplace_back(sv.substr(0, comma));
				sv.remove_prefix(comma + 1);
				comma = sv.find(',');
			}
			ranges.emplace_back(sv);
			if (mainRange >= ranges.size()) {
				mainRange = ranges.size() - 1;
			}
		}
	} catch (std::runtime_error &) {
		// On failure, produce an empty selection.
		Clear();
	}
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

SelectionRange Selection::RectangularCopy() const noexcept {
	return rangeRectangular;
}

SelectionSegment Selection::Limits() const noexcept {
	PLATFORM_ASSERT(!ranges.empty());
	SelectionSegment sr = ranges[0].AsSegment();
	for (size_t i=1; i<ranges.size(); i++) {
		sr.Extend(ranges[i].anchor);
		sr.Extend(ranges[i].caret);
	}
	return sr;
}

SelectionSegment Selection::LimitsForRectangularElseMain() const noexcept {
	if (IsRectangular()) {
		return Limits();
	}
	return ranges[mainRange].AsSegment();
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
	}
	return ranges[mainRange].Start();
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

void Selection::SetSelection(SelectionRange range) noexcept {
	if (ranges.size() > 1) {
		ranges.erase(ranges.begin() + 1, ranges.end());
	}
	ranges[0] = range;
	mainRange = 0;
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

void Selection::DropSelection(size_t r) noexcept {
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

void Selection::DropAdditionalRanges() noexcept {
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

void Selection::Clear() noexcept {
	if (ranges.size() > 1) {
		ranges.erase(ranges.begin() + 1, ranges.end());
	}
	ranges[0].Reset();
	rangesSaved.clear();
	rangeRectangular.Reset();
	mainRange = 0;
	moveExtends = false;
	tentativeMain = false;
	selType = SelTypes::stream;
}

void Selection::RemoveDuplicates() noexcept {
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

void Selection::SetRanges(const Ranges &rangesToSet) {
	ranges = rangesToSet;
}

void Selection::Truncate(Sci::Position length) noexcept {
	// This may be needed when applying a persisted selection onto a document that has been shortened.
	for (SelectionRange &range : ranges) {
		range.Truncate(length);
	}
	// Above may have made some non-unique empty ranges.
	RemoveDuplicates();
	rangeRectangular.Truncate(length);
}

std::string Selection::ToString() const {
	std::string result;
	switch (selType) {
	case SelTypes::rectangle:
		result += 'R';
		break;
	case SelTypes::lines:
		result += 'L';
		break;
	case SelTypes::thin:
		result += 'T';
		break;
	default:
		// No handling of none as not a real value of enumeration, just used for empty arguments
		// No prefix.
		break;
	}
	if (selType == SelTypes::rectangle || selType == SelTypes::thin) {
		result += rangeRectangular.ToString();
	} else {
		for (size_t r = 0; r < ranges.size(); r++) {
			if (r > 0) {
				result += ',';
			}
			result += ranges[r].ToString();
		}
	}

	if (mainRange > 0) {
		result += '#';
		result += std::to_string(mainRange);
	}

	return result;
}
