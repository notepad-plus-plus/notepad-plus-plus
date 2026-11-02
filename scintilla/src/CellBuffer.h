// Scintilla source code edit control
/** @file CellBuffer.h
 ** Manages the text of the document.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CELLBUFFER_H
#define CELLBUFFER_H

namespace Scintilla::Internal {

struct Failure : public std::runtime_error {
	Status status;
	explicit Failure(Status status_) : std::runtime_error("failure with status"), status(status_) {}
};

// Interface to per-line data that wants to see each line insertion and deletion
class PerLine {
public:
	virtual ~PerLine() = default;
	virtual void Init()=0;
	virtual void InsertLine(Sci::Line line)=0;
	virtual void InsertLines(Sci::Line line, Sci::Line lines) = 0;
	virtual void RemoveLine(Sci::Line line)=0;
};

class UndoHistory;
class ChangeHistory;

/**
 * The line vector contains information about each of the lines in a cell buffer.
 */
class ILineVector;

enum class ActionType : unsigned char { insert, remove, container };

/**
 * Actions are used to return the information required to report one undo/redo step.
 */
struct Action {
	ActionType at = ActionType::insert;
	bool mayCoalesce = false;
	Sci::Position position = 0;
	const char *data = nullptr;
	Sci::Position lenData = 0;
};

struct SplitView {
	const char *segment1 = nullptr;
	size_t length1 = 0;
	const char *segment2 = nullptr;
	size_t length = 0;

	bool operator==(const SplitView &other) const noexcept {
		return segment1 == other.segment1 && length1 == other.length1 &&
			segment2 == other.segment2 && length == other.length;
	}
	bool operator!=(const SplitView &other) const noexcept {
		return !(*this == other);
	}

	[[nodiscard]] char CharAt(size_t position) const noexcept {
		if (position < length1) {
			return segment1[position];
		}
		if (position < length) {
			return segment2[position];
		}
		return 0;
	}
};

struct ChangedRange {
	Sci::Position start = Sci::invalidPosition;
	Sci::Position end = Sci::invalidPosition;
	ChangedRange() noexcept = default;
	ChangedRange(Sci::Position start_, Sci::Position end_) noexcept : start(start_), end(end_) {}
	[[nodiscard]] bool Empty() const noexcept {
		return start < 0;
	}
	void Merge(const ChangedRange &cr2) noexcept {
		if (cr2.start >= 0) {
			if (start < 0) {
				*this = cr2;
			} else {
				end = cr2.end;
			}
		}
	}
};

/**
 * Holder for an expandable array of characters that supports undo and line markers.
 * Based on article "Data Structures in a Bit-Mapped Text Editor"
 * by Wilfred J. Hansen, Byte January 1987, page 183.
 */
class CellBuffer {
private:
	bool hasStyles;
	bool largeDocument;
	SplitVector<char> substance;
	SplitVector<char> style;
	bool readOnly;
	bool utf8Substance;
	Scintilla::LineEndType utf8LineEnds;

	bool collectingUndo;
	std::unique_ptr<UndoHistory> uh;

	std::unique_ptr<ChangeHistory> changeHistory;

	std::unique_ptr<ILineVector> plv;

	[[nodiscard]] bool UTF8LineEndOverlaps(Sci::Position position) const noexcept;
	[[nodiscard]] bool UTF8IsCharacterBoundary(Sci::Position position) const;
	void ResetLineEnds();
	void RecalculateIndexLineStarts(Sci::Line lineFirst, Sci::Line lineLast);
	[[nodiscard]] bool MaintainingLineCharacterIndex() const noexcept;
	/// Actions without undo
	void BasicInsertString(Sci::Position position, const char *s, Sci::Position insertLength);
	void BasicDeleteChars(Sci::Position position, Sci::Position deleteLength);

public:

	CellBuffer(bool hasStyles_, bool largeDocument_);
	// Deleted so CellBuffer objects can not be copied.
	CellBuffer(const CellBuffer &) = delete;
	CellBuffer(CellBuffer &&) = delete;
	CellBuffer &operator=(const CellBuffer &) = delete;
	CellBuffer &operator=(CellBuffer &&) = delete;
	~CellBuffer() noexcept;

	/// Retrieving positions outside the range of the buffer works and returns 0
	[[nodiscard]] char CharAt(Sci::Position position) const noexcept;
	[[nodiscard]] unsigned char UCharAt(Sci::Position position) const noexcept;
	void GetCharRange(char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const;
	[[nodiscard]] char StyleAt(Sci::Position position) const noexcept;
	void GetStyleRange(unsigned char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const;
	[[nodiscard]] const char *BufferPointer();
	[[nodiscard]] const char *RangePointer(Sci::Position position, Sci::Position rangeLength) noexcept;
	[[nodiscard]] Sci::Position GapPosition() const noexcept;
	[[nodiscard]] SplitView AllView() const noexcept;

	[[nodiscard]] Sci::Position Length() const noexcept;
	void Allocate(Sci::Position newSize);
	void SetUTF8Substance(bool utf8Substance_) noexcept;
	[[nodiscard]] Scintilla::LineEndType GetLineEndTypes() const noexcept { return utf8LineEnds; }
	void SetLineEndTypes(Scintilla::LineEndType utf8LineEnds_);
	[[nodiscard]] bool ContainsLineEnd(const char *s, Sci::Position length) const noexcept;
	void SetPerLine(PerLine *pl) noexcept;
	[[nodiscard]] Scintilla::LineCharacterIndexType LineCharacterIndex() const noexcept;
	void AllocateLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	void ReleaseLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	[[nodiscard]] Sci::Line Lines() const noexcept;
	void AllocateLines(Sci::Line lines);
	[[nodiscard]] Sci::Position LineStart(Sci::Line line) const noexcept;
	[[nodiscard]] Sci::Position LineEnd(Sci::Line line) const noexcept;
	[[nodiscard]] Sci::Position IndexLineStart(Sci::Line line, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	[[nodiscard]] Sci::Line LineFromPosition(Sci::Position pos) const noexcept;
	[[nodiscard]] Sci::Line LineFromPositionIndex(Sci::Position pos, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	void InsertLine(Sci::Line line, Sci::Position position, bool lineStart);
	void RemoveLine(Sci::Line line);
	const char *InsertString(Sci::Position position, const char *s, Sci::Position insertLength, bool &startSequence);

	/// Setting styles for positions outside the range of the buffer is safe and has no effect.
	/// @return range where style of characters changed.
	ChangedRange SetStyles(Sci::Position position, const char *styles, Sci::Position length) noexcept;
	ChangedRange SetStyleFor(Sci::Position position, Sci::Position length, char value) noexcept;

	const char *DeleteChars(Sci::Position position, Sci::Position deleteLength, bool &startSequence);

	[[nodiscard]] bool IsReadOnly() const noexcept;
	void SetReadOnly(bool set) noexcept;
	[[nodiscard]] bool IsLarge() const noexcept;
	[[nodiscard]] bool HasStyles() const noexcept;

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	[[nodiscard]] bool IsSavePoint() const noexcept;

	void TentativeStart() noexcept;
	void TentativeCommit() noexcept;
	[[nodiscard]] bool TentativeActive() const noexcept;
	[[nodiscard]] int TentativeSteps() noexcept;

	bool SetUndoCollection(bool collectUndo) noexcept;
	[[nodiscard]] bool IsCollectingUndo() const noexcept;
	void BeginUndoAction(bool mayCoalesce=false) noexcept;
	void EndUndoAction() noexcept;
	[[nodiscard]] int UndoSequenceDepth() const noexcept;
	[[nodiscard]] bool AfterUndoSequenceStart() const noexcept;
	void AddUndoAction(Sci::Position token, bool mayCoalesce);
	void DeleteUndoHistory() noexcept;

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	[[nodiscard]] bool CanUndo() const noexcept;
	int StartUndo() noexcept;
	[[nodiscard]] Action GetUndoStep() const noexcept;
	void PerformUndoStep();
	[[nodiscard]] bool CanRedo() const noexcept;
	int StartRedo() noexcept;
	[[nodiscard]] Action GetRedoStep() const noexcept;
	void PerformRedoStep();

	[[nodiscard]] int UndoActions() const noexcept;
	void SetUndoSavePoint(int action) noexcept;
	[[nodiscard]] int UndoSavePoint() const noexcept;
	void SetUndoDetach(int action) noexcept;
	[[nodiscard]] int UndoDetach() const noexcept;
	void SetUndoTentative(int action) noexcept;
	[[nodiscard]] int UndoTentative() const noexcept;
	void SetUndoCurrent(int action);
	[[nodiscard]] int UndoCurrent() const noexcept;
	[[nodiscard]] int UndoActionType(int action) const noexcept;
	[[nodiscard]] Sci::Position UndoActionPosition(int action) const noexcept;
	[[nodiscard]] std::string_view UndoActionText(int action) const noexcept;
	void PushUndoActionType(int type, Sci::Position position);
	void ChangeLastUndoActionText(size_t length, const char *text);

	void ChangeHistorySet(bool set);
	[[nodiscard]] int EditionAt(Sci::Position pos) const noexcept;
	[[nodiscard]] Sci::Position EditionEndRun(Sci::Position pos) const noexcept;
	[[nodiscard]] unsigned int EditionDeletesAt(Sci::Position pos) const noexcept;
	[[nodiscard]] Sci::Position EditionNextDelete(Sci::Position pos) const noexcept;
};

}

#endif
