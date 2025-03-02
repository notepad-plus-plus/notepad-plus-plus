// Scintilla source code edit control
/** @file CellBuffer.cxx
 ** Manages a buffer of cells.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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
		return CountWidths(-countBasePlane, -countOtherPlanes);
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
	// line_cast(): cast Sci::Line to either 32-bit or 64-bit value
	// This avoids warnings from Visual C++ Code Analysis and shortens code
	static constexpr POS line_cast(Sci::Line pos) noexcept {
		return static_cast<POS>(pos);
	}
public:
	int refCount;
	Partitioning<POS> starts;

	LineStartIndex() : refCount(0), starts(4) {
		// Minimal initial allocation
	}
	bool Allocate(Sci::Line lines) {
		refCount++;
		Sci::Position length = starts.PositionFromPartition(starts.Partitions());
		for (Sci::Line line = starts.Partitions(); line < lines; line++) {
			// Produce an ascending sequence that will be filled in with correct widths later
			length++;
			starts.InsertPartition(line_cast(line), line_cast(length));
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
		return starts.PositionFromPartition(line_cast(line) + 1) -
			starts.PositionFromPartition(line_cast(line));
	}
	void SetLineWidth(Sci::Line line, Sci::Position width) noexcept {
		const Sci::Position widthCurrent = LineWidth(line);
		starts.InsertText(line_cast(line), line_cast(width - widthCurrent));
	}
	void AllocateLines(Sci::Line lines) {
		if (lines > starts.Partitions()) {
			starts.ReAllocate(lines);
		}
	}
	void InsertLines(Sci::Line line, Sci::Line lines) {
		// Insert multiple lines with each temporarily 1 character wide.
		// The line widths will be fixed up by later measuring code.
		const POS lineAsPos = line_cast(line);
		const POS lineStart = starts.PositionFromPartition(lineAsPos - 1) + 1;
		for (POS l = 0; l < line_cast(lines); l++) {
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

	// pos_cast(): cast Sci::Line and Sci::Position to either 32-bit or 64-bit value
	// This avoids warnings from Visual C++ Code Analysis and shortens code
	static constexpr POS pos_cast(Sci::Position pos) noexcept {
		return static_cast<POS>(pos);
	}

	// line_from_pos_cast(): return 32-bit or 64-bit value as Sci::Line
	// This avoids warnings from Visual C++ Code Analysis and shortens code
	static constexpr Sci::Line line_from_pos_cast(POS line) noexcept {
		return static_cast<Sci::Line>(line);
	}

public:
	LineVector() : starts(256), perLine(nullptr), activeIndices(LineCharacterIndexType::None) {
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
		starts.InsertText(pos_cast(line), pos_cast(delta));
	}
	void InsertLine(Sci::Line line, Sci::Position position, bool lineStart) override {
		const POS lineAsPos = pos_cast(line);
		starts.InsertPartition(lineAsPos, pos_cast(position));
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
		const POS lineAsPos = pos_cast(line);
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
		starts.SetPartitionStartPosition(pos_cast(line), pos_cast(position));
	}
	void RemoveLine(Sci::Line line) override {
		starts.RemovePartition(pos_cast(line));
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
			startsUTF32.starts.RemovePartition(pos_cast(line));
		}
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
			startsUTF16.starts.RemovePartition(pos_cast(line));
		}
		if (perLine) {
			perLine->RemoveLine(line);
		}
	}
	Sci::Line Lines() const noexcept override {
		return line_from_pos_cast(starts.Partitions());
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
		return line_from_pos_cast(starts.PartitionFromPosition(pos_cast(pos)));
	}
	Sci::Position LineStart(Sci::Line line) const noexcept override {
		return starts.PositionFromPartition(pos_cast(line));
	}
	void InsertCharacters(Sci::Line line, CountWidths delta) noexcept override {
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf32)) {
			startsUTF32.starts.InsertText(pos_cast(line), pos_cast(delta.WidthUTF32()));
		}
		if (FlagSet(activeIndices, LineCharacterIndexType::Utf16)) {
			startsUTF16.starts.InsertText(pos_cast(line), pos_cast(delta.WidthUTF16()));
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
			return startsUTF32.starts.PositionFromPartition(pos_cast(line));
		} else {
			return startsUTF16.starts.PositionFromPartition(pos_cast(line));
		}
	}
	Sci::Line LineFromPositionIndex(Sci::Position pos, LineCharacterIndexType lineCharacterIndex) const noexcept override {
		if (lineCharacterIndex == LineCharacterIndexType::Utf32) {
			return line_from_pos_cast(startsUTF32.starts.PartitionFromPosition(pos_cast(pos)));
		} else {
			return line_from_pos_cast(startsUTF16.starts.PartitionFromPosition(pos_cast(pos)));
		}
	}
};

CellBuffer::CellBuffer(bool hasStyles_, bool largeDocument_) :
	hasStyles(hasStyles_), largeDocument(largeDocument_) {
	readOnly = false;
	utf8Substance = false;
	utf8LineEnds = LineEndType::Default;
	collectingUndo = true;
	uh = std::make_unique<UndoHistory>();
	if (largeDocument)
		plv = std::make_unique<LineVector<Sci::Position>>();
	else
		plv = std::make_unique<LineVector<int>>();
}

CellBuffer::~CellBuffer() noexcept = default;

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
	return hasStyles ? style.ValueAt(position) : '\0';
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
			data = uh->AppendAction(ActionType::insert, position, s, insertLength, startSequence);
		}

		BasicInsertString(position, s, insertLength);
		if (changeHistory) {
			changeHistory->Insert(position, insertLength, collectingUndo, uh->BeforeReachableSavePoint());
		}
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
			data = uh->AppendAction(ActionType::remove, position, data, deleteLength, startSequence);
		}

		if (changeHistory) {
			changeHistory->DeleteRangeSavingHistory(position, deleteLength,
				uh->BeforeReachableSavePoint(), uh->AfterOrAtDetachPoint());
		}

		BasicDeleteChars(position, deleteLength);
	}
	return data;
}

Sci::Position CellBuffer::Length() const noexcept {
	return substance.Length();
}

void CellBuffer::Allocate(Sci::Position newSize) {
	if (!largeDocument && (newSize > INT32_MAX)) {
		throw std::runtime_error("CellBuffer::Allocate: size of standard document limited to 2G.");
	}
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

Sci::Position CellBuffer::LineEnd(Sci::Line line) const noexcept {
	if (line >= Lines() - 1) {
		return LineStart(line + 1);
	} else {
		Sci::Position position = LineStart(line + 1);
		if (LineEndType::Unicode == GetLineEndTypes()) {
			const unsigned char bytes[] = {
				UCharAt(position - 3),
				UCharAt(position - 2),
				UCharAt(position - 1),
			};
			if (UTF8IsSeparator(bytes)) {
				return position - UTF8SeparatorLength;
			}
			if (UTF8IsNEL(bytes + 1)) {
				return position - UTF8NELLength;
			}
		}
		position--; // Back over CR or LF
		// When line terminator is CR+LF, may need to go back one more
		if ((position > LineStart(line)) && (CharAt(position - 1) == '\r')) {
			position--;
		}
		return position;
	}
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
	uh->SetSavePoint();
	if (changeHistory) {
		changeHistory->SetSavePoint();
	}
}

bool CellBuffer::IsSavePoint() const noexcept {
	return uh->IsSavePoint();
}

void CellBuffer::TentativeStart() noexcept {
	uh->TentativeStart();
}

void CellBuffer::TentativeCommit() noexcept {
	uh->TentativeCommit();
}

int CellBuffer::TentativeSteps() noexcept {
	return uh->TentativeSteps();
}

bool CellBuffer::TentativeActive() const noexcept {
	return uh->TentativeActive();
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
	const char *const end = s + insertLength - 1;
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

bool CellBuffer::SetUndoCollection(bool collectUndo) noexcept {
	collectingUndo = collectUndo;
	uh->DropUndoSequence();
	return collectingUndo;
}

bool CellBuffer::IsCollectingUndo() const noexcept {
	return collectingUndo;
}

void CellBuffer::BeginUndoAction(bool mayCoalesce) noexcept {
	uh->BeginUndoAction(mayCoalesce);
}

void CellBuffer::EndUndoAction() noexcept {
	uh->EndUndoAction();
}

int CellBuffer::UndoSequenceDepth() const noexcept {
	return uh->UndoSequenceDepth();
}

bool CellBuffer::AfterUndoSequenceStart() const noexcept {
	return uh->AfterUndoSequenceStart();
}

void CellBuffer::AddUndoAction(Sci::Position token, bool mayCoalesce) {
	bool startSequence = false;
	uh->AppendAction(ActionType::container, token, nullptr, 0, startSequence, mayCoalesce);
}

void CellBuffer::DeleteUndoHistory() noexcept {
	uh->DeleteUndoHistory();
}

bool CellBuffer::CanUndo() const noexcept {
	return uh->CanUndo();
}

int CellBuffer::StartUndo() noexcept {
	return uh->StartUndo();
}

Action CellBuffer::GetUndoStep() const noexcept {
	return uh->GetUndoStep();
}

void CellBuffer::PerformUndoStep() {
	const Action previousStep = uh->GetUndoStep();
	// PreviousBeforeSavePoint and AfterDetachPoint are called since acting on the previous action,
	// that is currentAction-1
	if (changeHistory && uh->PreviousBeforeSavePoint()) {
		changeHistory->StartReversion();
	}
	if (previousStep.at == ActionType::insert) {
		if (substance.Length() < previousStep.lenData) {
			throw std::runtime_error(
				"CellBuffer::PerformUndoStep: deletion must be less than document length.");
		}
		if (changeHistory) {
			changeHistory->DeleteRange(previousStep.position, previousStep.lenData,
				uh->PreviousBeforeSavePoint() && !uh->AfterDetachPoint());
		}
		BasicDeleteChars(previousStep.position, previousStep.lenData);
	} else if (previousStep.at == ActionType::remove) {
		BasicInsertString(previousStep.position, previousStep.data, previousStep.lenData);
		if (changeHistory) {
			changeHistory->UndoDeleteStep(previousStep.position, previousStep.lenData, uh->AfterDetachPoint());
		}
	}
	uh->CompletedUndoStep();
}

bool CellBuffer::CanRedo() const noexcept {
	return uh->CanRedo();
}

int CellBuffer::StartRedo() noexcept {
	return uh->StartRedo();
}

Action CellBuffer::GetRedoStep() const noexcept {
	return uh->GetRedoStep();
}

void CellBuffer::PerformRedoStep() {
	const Action actionStep = uh->GetRedoStep();
	if (actionStep.at == ActionType::insert) {
		BasicInsertString(actionStep.position, actionStep.data, actionStep.lenData);
		if (changeHistory) {
			changeHistory->Insert(actionStep.position, actionStep.lenData, collectingUndo,
				uh->BeforeSavePoint() && !uh->AfterOrAtDetachPoint());
		}
	} else if (actionStep.at == ActionType::remove) {
		if (changeHistory) {
			changeHistory->DeleteRangeSavingHistory(actionStep.position, actionStep.lenData,
				uh->BeforeReachableSavePoint(), uh->AfterOrAtDetachPoint());
		}
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	}
	if (changeHistory && uh->AfterSavePoint()) {
		changeHistory->EndReversion();
	}
	uh->CompletedRedoStep();
}

int CellBuffer::UndoActions() const noexcept {
	return uh->Actions();
}

void CellBuffer::SetUndoSavePoint(int action) noexcept {
	uh->SetSavePoint(action);
}

int CellBuffer::UndoSavePoint() const noexcept {
	return uh->SavePoint();
}

void CellBuffer::SetUndoDetach(int action) noexcept {
	uh->SetDetachPoint(action);
}

int CellBuffer::UndoDetach() const noexcept {
	return uh->DetachPoint();
}

void CellBuffer::SetUndoTentative(int action) noexcept {
	uh->SetTentative(action);
}

int CellBuffer::UndoTentative() const noexcept {
	return uh->TentativePoint();
}

namespace {

void RestoreChangeHistory(const UndoHistory *uh, ChangeHistory *changeHistory) {
	// Replay all undo actions into changeHistory
	const int savePoint = uh->SavePoint();
	const int detachPoint = uh->DetachPoint();
	const int currentPoint = uh->Current();
	for (int act = 0; act < uh->Actions(); act++) {
		const ActionType type = static_cast<ActionType>(uh->Type(act) & ~coalesceFlag);
		const Sci::Position position = uh->Position(act);
		const Sci::Position length = uh->Length(act);
		const bool beforeSave = act < savePoint || ((detachPoint >= 0) && (detachPoint > act));
		const bool afterDetach = (detachPoint >= 0) && (detachPoint < act);
		switch (type) {
		case ActionType::insert:
			changeHistory->Insert(position, length, true, beforeSave);
			break;
		case ActionType::remove:
			changeHistory->DeleteRangeSavingHistory(position, length, beforeSave, afterDetach);
			break;
		default:
			// Only insertions and deletions go into change history
			break;
		}
		changeHistory->Check();
	}
	// Undo back to currentPoint, updating change history
	for (int act = uh->Actions() - 1; act >= currentPoint; act--) {
		const ActionType type = static_cast<ActionType>(uh->Type(act) & ~coalesceFlag);
		const Sci::Position position = uh->Position(act);
		const Sci::Position length = uh->Length(act);
		const bool beforeSave = act < savePoint;
		const bool afterDetach = (detachPoint >= 0) && (detachPoint < act);
		if (beforeSave) {
			changeHistory->StartReversion();
		}
		switch (type) {
		case ActionType::insert:
			changeHistory->DeleteRange(position, length, beforeSave && !afterDetach);
			break;
		case ActionType::remove:
			changeHistory->UndoDeleteStep(position, length, afterDetach);
			break;
		default:
			// Only insertions and deletions go into change history
			break;
		}
		changeHistory->Check();
	}
}

}

void CellBuffer::SetUndoCurrent(int action) {
	uh->SetCurrent(action, Length());
	if (changeHistory) {
		if ((uh->DetachPoint() >= 0) && (uh->SavePoint() >= 0)) {
			// Can't have a valid save point and a valid detach point at same time
			uh->DeleteUndoHistory();
			changeHistory.reset();
			throw std::runtime_error("UndoHistory::SetCurrent: invalid undo history.");
		}
		const intptr_t sizeChange = uh->Delta(action);
		const intptr_t lengthOriginal = Length() - sizeChange;
		// Recreate empty change history
		changeHistory = std::make_unique<ChangeHistory>(lengthOriginal);
		RestoreChangeHistory(uh.get(), changeHistory.get());
		if (Length() != changeHistory->Length()) {
			uh->DeleteUndoHistory();
			changeHistory.reset();
			throw std::runtime_error("UndoHistory::SetCurrent: invalid undo history.");
		}
	}
}

int CellBuffer::UndoCurrent() const noexcept {
	return uh->Current();
}

int CellBuffer::UndoActionType(int action) const noexcept {
	return uh->Type(action);
}

Sci::Position CellBuffer::UndoActionPosition(int action) const noexcept {
	return uh->Position(action);
}

std::string_view CellBuffer::UndoActionText(int action) const noexcept {
	return uh->Text(action);
}

void CellBuffer::PushUndoActionType(int type, Sci::Position position) {
	uh->PushUndoActionType(type, position);
}

void CellBuffer::ChangeLastUndoActionText(size_t length, const char *text) {
	uh->ChangeLastUndoActionText(length, text);
}

void CellBuffer::ChangeHistorySet(bool set) {
	if (set) {
		if (!changeHistory && !uh->CanUndo()) {
			changeHistory = std::make_unique<ChangeHistory>(Length());
		}
	} else {
		changeHistory.reset();
	}
}

int CellBuffer::EditionAt(Sci::Position pos) const noexcept {
	if (changeHistory) {
		return changeHistory->EditionAt(pos);
	}
	return 0;
}

Sci::Position CellBuffer::EditionEndRun(Sci::Position pos) const noexcept {
	if (changeHistory) {
		return changeHistory->EditionEndRun(pos);
	}
	return Length();
}

unsigned int CellBuffer::EditionDeletesAt(Sci::Position pos) const noexcept {
	if (changeHistory) {
		return changeHistory->EditionDeletesAt(pos);
	}
	return 0;
}

Sci::Position CellBuffer::EditionNextDelete(Sci::Position pos) const noexcept {
	if (changeHistory) {
		return changeHistory->EditionNextDelete(pos);
	}
	return Length() + 1;
}
