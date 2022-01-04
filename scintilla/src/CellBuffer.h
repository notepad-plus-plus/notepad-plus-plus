// Scintilla source code edit control
/** @file CellBuffer.h
 ** Manages the text of the document.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CELLBUFFER_H
#define CELLBUFFER_H

namespace Scintilla::Internal {

// Interface to per-line data that wants to see each line insertion and deletion
class PerLine {
public:
	virtual ~PerLine() {}
	virtual void Init()=0;
	virtual void InsertLine(Sci::Line line)=0;
	virtual void InsertLines(Sci::Line line, Sci::Line lines) = 0;
	virtual void RemoveLine(Sci::Line line)=0;
};

/**
 * The line vector contains information about each of the lines in a cell buffer.
 */
class ILineVector;

enum class ActionType { insert, remove, start, container };

/**
 * Actions are used to store all the information required to perform one undo/redo step.
 */
class Action {
public:
	ActionType at;
	Sci::Position position;
	std::unique_ptr<char[]> data;
	Sci::Position lenData;
	bool mayCoalesce;

	Action() noexcept;
	// Deleted so Action objects can not be copied.
	Action(const Action &other) = delete;
	Action &operator=(const Action &other) = delete;
	Action &operator=(const Action &&other) = delete;
	// Move constructor allows vector to be resized without reallocating.
	Action(Action &&other) noexcept = default;
	~Action();
	void Create(ActionType at_, Sci::Position position_=0, const char *data_=nullptr, Sci::Position lenData_=0, bool mayCoalesce_=true);
	void Clear() noexcept;
};

/**
 *
 */
class UndoHistory {
	std::vector<Action> actions;
	int maxAction;
	int currentAction;
	int undoSequenceDepth;
	int savePoint;
	int tentativePoint;

	void EnsureUndoRoom();

public:
	UndoHistory();
	// Deleted so UndoHistory objects can not be copied.
	UndoHistory(const UndoHistory &) = delete;
	UndoHistory(UndoHistory &&) = delete;
	void operator=(const UndoHistory &) = delete;
	void operator=(UndoHistory &&) = delete;
	~UndoHistory();

	const char *AppendAction(ActionType at, Sci::Position position, const char *data, Sci::Position lengthData, bool &startSequence, bool mayCoalesce=true);

	void BeginUndoAction();
	void EndUndoAction();
	void DropUndoSequence();
	void DeleteUndoHistory();

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint() noexcept;
	bool IsSavePoint() const noexcept;

	// Tentative actions are used for input composition so that it can be undone cleanly
	void TentativeStart();
	void TentativeCommit();
	bool TentativeActive() const noexcept;
	int TentativeSteps() noexcept;

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const noexcept;
	int StartUndo();
	const Action &GetUndoStep() const;
	void CompletedUndoStep();
	bool CanRedo() const noexcept;
	int StartRedo();
	const Action &GetRedoStep() const;
	void CompletedRedoStep();
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

	char CharAt(size_t position) const noexcept {
		if (position < length1) {
			return segment1[position];
		}
		if (position < length) {
			return segment2[position];
		}
		return 0;
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
	UndoHistory uh;

	std::unique_ptr<ILineVector> plv;

	bool UTF8LineEndOverlaps(Sci::Position position) const noexcept;
	bool UTF8IsCharacterBoundary(Sci::Position position) const;
	void ResetLineEnds();
	void RecalculateIndexLineStarts(Sci::Line lineFirst, Sci::Line lineLast);
	bool MaintainingLineCharacterIndex() const noexcept;
	/// Actions without undo
	void BasicInsertString(Sci::Position position, const char *s, Sci::Position insertLength);
	void BasicDeleteChars(Sci::Position position, Sci::Position deleteLength);

public:

	CellBuffer(bool hasStyles_, bool largeDocument_);
	// Deleted so CellBuffer objects can not be copied.
	CellBuffer(const CellBuffer &) = delete;
	CellBuffer(CellBuffer &&) = delete;
	void operator=(const CellBuffer &) = delete;
	void operator=(CellBuffer &&) = delete;
	~CellBuffer();

	/// Retrieving positions outside the range of the buffer works and returns 0
	char CharAt(Sci::Position position) const noexcept;
	unsigned char UCharAt(Sci::Position position) const noexcept;
	void GetCharRange(char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const;
	char StyleAt(Sci::Position position) const noexcept;
	void GetStyleRange(unsigned char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const;
	const char *BufferPointer();
	const char *RangePointer(Sci::Position position, Sci::Position rangeLength) noexcept;
	Sci::Position GapPosition() const noexcept;
	SplitView AllView() const noexcept;

	Sci::Position Length() const noexcept;
	void Allocate(Sci::Position newSize);
	void SetUTF8Substance(bool utf8Substance_) noexcept;
	Scintilla::LineEndType GetLineEndTypes() const noexcept { return utf8LineEnds; }
	void SetLineEndTypes(Scintilla::LineEndType utf8LineEnds_);
	bool ContainsLineEnd(const char *s, Sci::Position length) const noexcept;
	void SetPerLine(PerLine *pl) noexcept;
	Scintilla::LineCharacterIndexType LineCharacterIndex() const noexcept;
	void AllocateLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	void ReleaseLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	Sci::Line Lines() const noexcept;
	void AllocateLines(Sci::Line lines);
	Sci::Position LineStart(Sci::Line line) const noexcept;
	Sci::Position IndexLineStart(Sci::Line line, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	Sci::Line LineFromPosition(Sci::Position pos) const noexcept;
	Sci::Line LineFromPositionIndex(Sci::Position pos, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	void InsertLine(Sci::Line line, Sci::Position position, bool lineStart);
	void RemoveLine(Sci::Line line);
	const char *InsertString(Sci::Position position, const char *s, Sci::Position insertLength, bool &startSequence);

	/// Setting styles for positions outside the range of the buffer is safe and has no effect.
	/// @return true if the style of a character is changed.
	bool SetStyleAt(Sci::Position position, char styleValue) noexcept;
	bool SetStyleFor(Sci::Position position, Sci::Position lengthStyle, char styleValue) noexcept;

	const char *DeleteChars(Sci::Position position, Sci::Position deleteLength, bool &startSequence);

	bool IsReadOnly() const noexcept;
	void SetReadOnly(bool set) noexcept;
	bool IsLarge() const noexcept;
	bool HasStyles() const noexcept;

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint() const noexcept;

	void TentativeStart();
	void TentativeCommit();
	bool TentativeActive() const noexcept;
	int TentativeSteps() noexcept;

	bool SetUndoCollection(bool collectUndo);
	bool IsCollectingUndo() const noexcept;
	void BeginUndoAction();
	void EndUndoAction();
	void AddUndoAction(Sci::Position token, bool mayCoalesce);
	void DeleteUndoHistory();

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const noexcept;
	int StartUndo();
	const Action &GetUndoStep() const;
	void PerformUndoStep();
	bool CanRedo() const noexcept;
	int StartRedo();
	const Action &GetRedoStep() const;
	void PerformRedoStep();
};

}

#endif
