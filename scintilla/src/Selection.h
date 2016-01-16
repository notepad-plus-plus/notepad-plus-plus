// Scintilla source code edit control
/** @file Selection.h
 ** Classes maintaining the selection.
 **/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SELECTION_H
#define SELECTION_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class SelectionPosition {
	int position;
	int virtualSpace;
public:
	explicit SelectionPosition(int position_=INVALID_POSITION, int virtualSpace_=0) : position(position_), virtualSpace(virtualSpace_) {
		PLATFORM_ASSERT(virtualSpace < 800000);
		if (virtualSpace < 0)
			virtualSpace = 0;
	}
	void Reset() {
		position = 0;
		virtualSpace = 0;
	}
	void MoveForInsertDelete(bool insertion, int startChange, int length);
	bool operator ==(const SelectionPosition &other) const {
		return position == other.position && virtualSpace == other.virtualSpace;
	}
	bool operator <(const SelectionPosition &other) const;
	bool operator >(const SelectionPosition &other) const;
	bool operator <=(const SelectionPosition &other) const;
	bool operator >=(const SelectionPosition &other) const;
	int Position() const {
		return position;
	}
	void SetPosition(int position_) {
		position = position_;
		virtualSpace = 0;
	}
	int VirtualSpace() const {
		return virtualSpace;
	}
	void SetVirtualSpace(int virtualSpace_) {
		PLATFORM_ASSERT(virtualSpace_ < 800000);
		if (virtualSpace_ >= 0)
			virtualSpace = virtualSpace_;
	}
	void Add(int increment) {
		position = position + increment;
	}
	bool IsValid() const {
		return position >= 0;
	}
};

// Ordered range to make drawing simpler
struct SelectionSegment {
	SelectionPosition start;
	SelectionPosition end;
	SelectionSegment() : start(), end() {
	}
	SelectionSegment(SelectionPosition a, SelectionPosition b) {
		if (a < b) {
			start = a;
			end = b;
		} else {
			start = b;
			end = a;
		}
	}
	bool Empty() const {
		return start == end;
	}
	void Extend(SelectionPosition p) {
		if (start > p)
			start = p;
		if (end < p)
			end = p;
	}
};

struct SelectionRange {
	SelectionPosition caret;
	SelectionPosition anchor;

	SelectionRange() : caret(), anchor() {
	}
	explicit SelectionRange(SelectionPosition single) : caret(single), anchor(single) {
	}
	explicit SelectionRange(int single) : caret(single), anchor(single) {
	}
	SelectionRange(SelectionPosition caret_, SelectionPosition anchor_) : caret(caret_), anchor(anchor_) {
	}
	SelectionRange(int caret_, int anchor_) : caret(caret_), anchor(anchor_) {
	}
	bool Empty() const {
		return anchor == caret;
	}
	int Length() const;
	// int Width() const;	// Like Length but takes virtual space into account
	bool operator ==(const SelectionRange &other) const {
		return caret == other.caret && anchor == other.anchor;
	}
	bool operator <(const SelectionRange &other) const {
		return caret < other.caret || ((caret == other.caret) && (anchor < other.anchor));
	}
	void Reset() {
		anchor.Reset();
		caret.Reset();
	}
	void ClearVirtualSpace() {
		anchor.SetVirtualSpace(0);
		caret.SetVirtualSpace(0);
	}
	void MoveForInsertDelete(bool insertion, int startChange, int length);
	bool Contains(int pos) const;
	bool Contains(SelectionPosition sp) const;
	bool ContainsCharacter(int posCharacter) const;
	SelectionSegment Intersect(SelectionSegment check) const;
	SelectionPosition Start() const {
		return (anchor < caret) ? anchor : caret;
	}
	SelectionPosition End() const {
		return (anchor < caret) ? caret : anchor;
	}
	void Swap();
	bool Trim(SelectionRange range);
	// If range is all virtual collapse to start of virtual space
	void MinimizeVirtualSpace();
};

class Selection {
	std::vector<SelectionRange> ranges;
	std::vector<SelectionRange> rangesSaved;
	SelectionRange rangeRectangular;
	size_t mainRange;
	bool moveExtends;
	bool tentativeMain;
public:
	enum selTypes { noSel, selStream, selRectangle, selLines, selThin };
	selTypes selType;

	Selection();
	~Selection();
	bool IsRectangular() const;
	int MainCaret() const;
	int MainAnchor() const;
	SelectionRange &Rectangular();
	SelectionSegment Limits() const;
	// This is for when you want to move the caret in response to a
	// user direction command - for rectangular selections, use the range
	// that covers all selected text otherwise return the main selection.
	SelectionSegment LimitsForRectangularElseMain() const;
	size_t Count() const;
	size_t Main() const;
	void SetMain(size_t r);
	SelectionRange &Range(size_t r);
	const SelectionRange &Range(size_t r) const;
	SelectionRange &RangeMain();
	const SelectionRange &RangeMain() const;
	SelectionPosition Start() const;
	bool MoveExtends() const;
	void SetMoveExtends(bool moveExtends_);
	bool Empty() const;
	SelectionPosition Last() const;
	int Length() const;
	void MovePositions(bool insertion, int startChange, int length);
	void TrimSelection(SelectionRange range);
	void TrimOtherSelections(size_t r, SelectionRange range);
	void SetSelection(SelectionRange range);
	void AddSelection(SelectionRange range);
	void AddSelectionWithoutTrim(SelectionRange range);
	void DropSelection(size_t r);
	void DropAdditionalRanges();
	void TentativeSelection(SelectionRange range);
	void CommitTentative();
	int CharacterInSelection(int posCharacter) const;
	int InSelectionForEOL(int pos) const;
	int VirtualSpaceFor(int pos) const;
	void Clear();
	void RemoveDuplicates();
	void RotateMain();
	bool Tentative() const { return tentativeMain; }
	std::vector<SelectionRange> RangesCopy() const {
		return ranges;
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
