// Scintilla source code edit control
/** @file CellBuffer.h
 ** Manages the text of the document.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CELLBUFFER_H
#define CELLBUFFER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 * This holds the marker identifier and the marker type to display.
 * MarkerHandleNumbers are members of lists.
 */
struct MarkerHandleNumber {
	int handle;
	int number;
	MarkerHandleNumber *next;
};

/**
 * A marker handle set contains any number of MarkerHandleNumbers.
 */
class MarkerHandleSet {
	MarkerHandleNumber *root;

public:
	MarkerHandleSet();
	~MarkerHandleSet();
	int Length() const;
	int NumberFromHandle(int handle) const;
	int MarkValue() const;	///< Bit set of marker numbers.
	bool Contains(int handle) const;
	bool InsertHandle(int handle, int markerNum);
	void RemoveHandle(int handle);
	bool RemoveNumber(int markerNum);
	void CombineWith(MarkerHandleSet *other);
};

/**
 * The line vector contains information about each of the lines in a cell buffer.
 */
class LineVector {

	Partitioning starts;
	SplitVector<MarkerHandleSet *> markers;
	SplitVector<int> levels;
	/// Handles are allocated sequentially and should never have to be reused as 32 bit ints are very big.
	int handleCurrent;

public:

	LineVector();
	~LineVector();
	void Init();

	void ExpandLevels(int sizeNew=-1);
	void ClearLevels();
	int SetLevel(int line, int level);
	int GetLevel(int line);

	void InsertText(int line, int delta);
	void InsertLine(int line, int position);
	void SetLineStart(int line, int position);
	void RemoveLine(int line);
	int Lines() const {
		return starts.Partitions();
	}
	int LineFromPosition(int pos);
	int LineStart(int line) const {
		return starts.PositionFromPartition(line);
	}

	int MarkValue(int line);
	int AddMark(int line, int marker);
	void MergeMarkers(int pos);
	void DeleteMark(int line, int markerNum, bool all);
	void DeleteMarkFromHandle(int markerHandle);
	int LineFromHandle(int markerHandle);
};

enum actionType { insertAction, removeAction, startAction };

/**
 * Actions are used to store all the information required to perform one undo/redo step.
 */
class Action {
public:
	actionType at;
	int position;
	char *data;
	int lenData;
	bool mayCoalesce;

	Action();
	~Action();
	void Create(actionType at_, int position_=0, char *data_=0, int lenData_=0, bool mayCoalesce_=true);
	void Destroy();
	void Grab(Action *source);
};

/**
 *
 */
class UndoHistory {
	Action *actions;
	int lenActions;
	int maxAction;
	int currentAction;
	int undoSequenceDepth;
	int savePoint;

	void EnsureUndoRoom();

public:
	UndoHistory();
	~UndoHistory();

	void AppendAction(actionType at, int position, char *data, int length, bool &startSequence);

	void BeginUndoAction();
	void EndUndoAction();
	void DropUndoSequence();
	void DeleteUndoHistory();

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint() const;

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const;
	int StartUndo();
	const Action &GetUndoStep() const;
	void CompletedUndoStep();
	bool CanRedo() const;
	int StartRedo();
	const Action &GetRedoStep() const;
	void CompletedRedoStep();
};

/**
 * Holder for an expandable array of characters that supports undo and line markers.
 * Based on article "Data Structures in a Bit-Mapped Text Editor"
 * by Wilfred J. Hansen, Byte January 1987, page 183.
 */
class CellBuffer {
private:
	SplitVector<char> substance;
	SplitVector<char> style;
	bool readOnly;

	bool collectingUndo;
	UndoHistory uh;

	LineVector lv;

	SplitVector<int> lineStates;

public:

	CellBuffer();
	~CellBuffer();

	/// Retrieving positions outside the range of the buffer works and returns 0
	char CharAt(int position) const;
	void GetCharRange(char *buffer, int position, int lengthRetrieve);
	char StyleAt(int position);
	const char *BufferPointer();

	int Length() const;
	void Allocate(int newSize);
	int Lines() const;
	int LineStart(int line) const;
	int LineFromPosition(int pos) { return lv.LineFromPosition(pos); }
	void InsertLine(int line, int position);
	void RemoveLine(int line);
	const char *InsertString(int position, const char *s, int insertLength, bool &startSequence);

	/// Setting styles for positions outside the range of the buffer is safe and has no effect.
	/// @return true if the style of a character is changed.
	bool SetStyleAt(int position, char styleValue, char mask='\377');
	bool SetStyleFor(int position, int length, char styleValue, char mask);

	const char *DeleteChars(int position, int deleteLength, bool &startSequence);

	bool IsReadOnly();
	void SetReadOnly(bool set);

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint();

	/// Line marker functions
	int AddMark(int line, int markerNum);
	void DeleteMark(int line, int markerNum);
	void DeleteMarkFromHandle(int markerHandle);
	int GetMark(int line);
	void DeleteAllMarks(int markerNum);
	int LineFromHandle(int markerHandle);

	/// Actions without undo
	void BasicInsertString(int position, const char *s, int insertLength);
	void BasicDeleteChars(int position, int deleteLength);

	bool SetUndoCollection(bool collectUndo);
	bool IsCollectingUndo();
	void BeginUndoAction();
	void EndUndoAction();
	void DeleteUndoHistory();

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo();
	int StartUndo();
	const Action &GetUndoStep() const;
	void PerformUndoStep();
	bool CanRedo();
	int StartRedo();
	const Action &GetRedoStep() const;
	void PerformRedoStep();

	int SetLineState(int line, int state);
	int GetLineState(int line);
	int GetMaxLineState();

	int SetLevel(int line, int level);
	int GetLevel(int line);
	void ClearLevels();
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
