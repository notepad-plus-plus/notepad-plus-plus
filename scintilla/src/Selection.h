// Scintilla source code edit control
/** @file Selection.h
 ** Classes maintaining the selection.
 **/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SELECTION_H
#define SELECTION_H

namespace Scintilla::Internal {

class SelectionPosition {
	Sci::Position position;
	Sci::Position virtualSpace;
public:
	explicit SelectionPosition(Sci::Position position_= Sci::invalidPosition, Sci::Position virtualSpace_=0) noexcept : position(position_), virtualSpace(virtualSpace_) {
		PLATFORM_ASSERT(virtualSpace < 800000000);
		if (virtualSpace < 0)
			virtualSpace = 0;
	}
	void Reset() noexcept {
		position = 0;
		virtualSpace = 0;
	}
	void MoveForInsertDelete(bool insertion, Sci::Position startChange, Sci::Position length, bool moveForEqual) noexcept;
	bool operator ==(const SelectionPosition &other) const noexcept {
		return position == other.position && virtualSpace == other.virtualSpace;
	}
	bool operator <(const SelectionPosition &other) const noexcept;
	bool operator >(const SelectionPosition &other) const noexcept;
	bool operator <=(const SelectionPosition &other) const noexcept;
	bool operator >=(const SelectionPosition &other) const noexcept;
	Sci::Position Position() const noexcept {
		return position;
	}
	void SetPosition(Sci::Position position_) noexcept {
		position = position_;
		virtualSpace = 0;
	}
	Sci::Position VirtualSpace() const noexcept {
		return virtualSpace;
	}
	void SetVirtualSpace(Sci::Position virtualSpace_) noexcept {
		PLATFORM_ASSERT(virtualSpace_ < 800000000);
		if (virtualSpace_ >= 0)
			virtualSpace = virtualSpace_;
	}
	void Add(Sci::Position increment) noexcept {
		position = position + increment;
	}
	bool IsValid() const noexcept {
		return position >= 0;
	}
};

// Ordered range to make drawing simpler
struct SelectionSegment {
	SelectionPosition start;
	SelectionPosition end;
	SelectionSegment() noexcept : start(), end() {
	}
	SelectionSegment(SelectionPosition a, SelectionPosition b) noexcept {
		if (a < b) {
			start = a;
			end = b;
		} else {
			start = b;
			end = a;
		}
	}
	bool Empty() const noexcept {
		return start == end;
	}
	Sci::Position Length() const noexcept {
		return end.Position() - start.Position();
	}
	void Extend(SelectionPosition p) noexcept {
		if (start > p)
			start = p;
		if (end < p)
			end = p;
	}
};

struct SelectionRange {
	SelectionPosition caret;
	SelectionPosition anchor;

	SelectionRange() noexcept : caret(), anchor() {
	}
	explicit SelectionRange(SelectionPosition single) noexcept : caret(single), anchor(single) {
	}
	explicit SelectionRange(Sci::Position single) noexcept : caret(single), anchor(single) {
	}
	SelectionRange(SelectionPosition caret_, SelectionPosition anchor_) noexcept : caret(caret_), anchor(anchor_) {
	}
	SelectionRange(Sci::Position caret_, Sci::Position anchor_) noexcept : caret(caret_), anchor(anchor_) {
	}
	bool Empty() const noexcept {
		return anchor == caret;
	}
	Sci::Position Length() const noexcept;
	// Sci::Position Width() const;	// Like Length but takes virtual space into account
	bool operator ==(const SelectionRange &other) const noexcept {
		return caret == other.caret && anchor == other.anchor;
	}
	bool operator <(const SelectionRange &other) const noexcept {
		return caret < other.caret || ((caret == other.caret) && (anchor < other.anchor));
	}
	void Reset() noexcept {
		anchor.Reset();
		caret.Reset();
	}
	void ClearVirtualSpace() noexcept {
		anchor.SetVirtualSpace(0);
		caret.SetVirtualSpace(0);
	}
	void MoveForInsertDelete(bool insertion, Sci::Position startChange, Sci::Position length) noexcept;
	bool Contains(Sci::Position pos) const noexcept;
	bool Contains(SelectionPosition sp) const noexcept;
	bool ContainsCharacter(Sci::Position posCharacter) const noexcept;
	SelectionSegment Intersect(SelectionSegment check) const noexcept;
	SelectionPosition Start() const noexcept {
		return (anchor < caret) ? anchor : caret;
	}
	SelectionPosition End() const noexcept {
		return (anchor < caret) ? caret : anchor;
	}
	void Swap() noexcept;
	bool Trim(SelectionRange range) noexcept;
	// If range is all virtual collapse to start of virtual space
	void MinimizeVirtualSpace() noexcept;
};

// Deliberately an enum rather than an enum class to allow treating as bool
enum InSelection { inNone, inMain, inAdditional };

class Selection {
	std::vector<SelectionRange> ranges;
	std::vector<SelectionRange> rangesSaved;
	SelectionRange rangeRectangular;
	size_t mainRange;
	bool moveExtends;
	bool tentativeMain;
public:
	enum class SelTypes { none, stream, rectangle, lines, thin };
	SelTypes selType;

	Selection();
	bool IsRectangular() const noexcept;
	Sci::Position MainCaret() const noexcept;
	Sci::Position MainAnchor() const noexcept;
	SelectionRange &Rectangular() noexcept;
	SelectionSegment Limits() const noexcept;
	// This is for when you want to move the caret in response to a
	// user direction command - for rectangular selections, use the range
	// that covers all selected text otherwise return the main selection.
	SelectionSegment LimitsForRectangularElseMain() const;
	size_t Count() const noexcept;
	size_t Main() const noexcept;
	void SetMain(size_t r) noexcept;
	SelectionRange &Range(size_t r) noexcept;
	const SelectionRange &Range(size_t r) const noexcept;
	SelectionRange &RangeMain() noexcept;
	const SelectionRange &RangeMain() const noexcept;
	SelectionPosition Start() const noexcept;
	bool MoveExtends() const noexcept;
	void SetMoveExtends(bool moveExtends_) noexcept;
	bool Empty() const noexcept;
	SelectionPosition Last() const noexcept;
	Sci::Position Length() const noexcept;
	void MovePositions(bool insertion, Sci::Position startChange, Sci::Position length) noexcept;
	void TrimSelection(SelectionRange range) noexcept;
	void TrimOtherSelections(size_t r, SelectionRange range) noexcept;
	void SetSelection(SelectionRange range);
	void AddSelection(SelectionRange range);
	void AddSelectionWithoutTrim(SelectionRange range);
	void DropSelection(size_t r);
	void DropAdditionalRanges();
	void TentativeSelection(SelectionRange range);
	void CommitTentative() noexcept;
	InSelection RangeType(size_t r) const noexcept;
	InSelection CharacterInSelection(Sci::Position posCharacter) const noexcept;
	InSelection InSelectionForEOL(Sci::Position pos) const noexcept;
	Sci::Position VirtualSpaceFor(Sci::Position pos) const noexcept;
	void Clear();
	void RemoveDuplicates();
	void RotateMain() noexcept;
	bool Tentative() const noexcept { return tentativeMain; }
	std::vector<SelectionRange> RangesCopy() const {
		return ranges;
	}
};

}

#endif
