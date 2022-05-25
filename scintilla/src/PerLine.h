// Scintilla source code edit control
/** @file PerLine.h
 ** Manages data associated with each line of the document
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PERLINE_H
#define PERLINE_H

namespace Scintilla::Internal {

/**
 * This holds the marker identifier and the marker type to display.
 * MarkerHandleNumbers are members of lists.
 */
struct MarkerHandleNumber {
	int handle;
	int number;
	MarkerHandleNumber(int handle_, int number_) noexcept : handle(handle_), number(number_) {}
};

/**
 * A marker handle set contains any number of MarkerHandleNumbers.
 */
class MarkerHandleSet {
	std::forward_list<MarkerHandleNumber> mhList;

public:
	MarkerHandleSet();
	// Deleted so MarkerHandleSet objects can not be copied.
	MarkerHandleSet(const MarkerHandleSet &) = delete;
	MarkerHandleSet(MarkerHandleSet &&) = delete;
	void operator=(const MarkerHandleSet &) = delete;
	void operator=(MarkerHandleSet &&) = delete;
	~MarkerHandleSet();
	bool Empty() const noexcept;
	int MarkValue() const noexcept;	///< Bit set of marker numbers.
	bool Contains(int handle) const noexcept;
	bool InsertHandle(int handle, int markerNum);
	void RemoveHandle(int handle);
	bool RemoveNumber(int markerNum, bool all);
	void CombineWith(MarkerHandleSet *other) noexcept;
	MarkerHandleNumber const *GetMarkerHandleNumber(int which) const noexcept;
};

class LineMarkers : public PerLine {
	SplitVector<std::unique_ptr<MarkerHandleSet>> markers;
	/// Handles are allocated sequentially and should never have to be reused as 32 bit ints are very big.
	int handleCurrent;
public:
	LineMarkers() : handleCurrent(0) {
	}
	// Deleted so LineMarkers objects can not be copied.
	LineMarkers(const LineMarkers &) = delete;
	LineMarkers(LineMarkers &&) = delete;
	void operator=(const LineMarkers &) = delete;
	void operator=(LineMarkers &&) = delete;
	~LineMarkers() override;
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	int MarkValue(Sci::Line line) const noexcept;
	Sci::Line MarkerNext(Sci::Line lineStart, int mask) const noexcept;
	int AddMark(Sci::Line line, int markerNum, Sci::Line lines);
	void MergeMarkers(Sci::Line line);
	bool DeleteMark(Sci::Line line, int markerNum, bool all);
	void DeleteMarkFromHandle(int markerHandle);
	Sci::Line LineFromHandle(int markerHandle) const noexcept;
	int HandleFromLine(Sci::Line line, int which) const noexcept;
	int NumberFromLine(Sci::Line line, int which) const noexcept;
};

class LineLevels : public PerLine {
	SplitVector<int> levels;
public:
	LineLevels() {
	}
	// Deleted so LineLevels objects can not be copied.
	LineLevels(const LineLevels &) = delete;
	LineLevels(LineLevels &&) = delete;
	void operator=(const LineLevels &) = delete;
	void operator=(LineLevels &&) = delete;
	~LineLevels() override;
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	void ExpandLevels(Sci::Line sizeNew=-1);
	void ClearLevels();
	int SetLevel(Sci::Line line, int level, Sci::Line lines);
	int GetLevel(Sci::Line line) const noexcept;
};

class LineState : public PerLine {
	SplitVector<int> lineStates;
public:
	LineState() {
	}
	// Deleted so LineState objects can not be copied.
	LineState(const LineState &) = delete;
	LineState(LineState &&) = delete;
	void operator=(const LineState &) = delete;
	void operator=(LineState &&) = delete;
	~LineState() override;
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	int SetLineState(Sci::Line line, int state);
	int GetLineState(Sci::Line line);
	Sci::Line GetMaxLineState() const noexcept;
};

class LineAnnotation : public PerLine {
	SplitVector<std::unique_ptr<char []>> annotations;
public:
	LineAnnotation() {
	}
	// Deleted so LineAnnotation objects can not be copied.
	LineAnnotation(const LineAnnotation &) = delete;
	LineAnnotation(LineAnnotation &&) = delete;
	void operator=(const LineAnnotation &) = delete;
	void operator=(LineAnnotation &&) = delete;
	~LineAnnotation() override;

	[[nodiscard]] bool Empty() const noexcept;
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	bool MultipleStyles(Sci::Line line) const noexcept;
	int Style(Sci::Line line) const noexcept;
	const char *Text(Sci::Line line) const noexcept;
	const unsigned char *Styles(Sci::Line line) const noexcept;
	void SetText(Sci::Line line, const char *text);
	void ClearAll();
	void SetStyle(Sci::Line line, int style);
	void SetStyles(Sci::Line line, const unsigned char *styles);
	int Length(Sci::Line line) const noexcept;
	int Lines(Sci::Line line) const noexcept;
};

typedef std::vector<int> TabstopList;

class LineTabstops : public PerLine {
	SplitVector<std::unique_ptr<TabstopList>> tabstops;
public:
	LineTabstops() {
	}
	// Deleted so LineTabstops objects can not be copied.
	LineTabstops(const LineTabstops &) = delete;
	LineTabstops(LineTabstops &&) = delete;
	void operator=(const LineTabstops &) = delete;
	void operator=(LineTabstops &&) = delete;
	~LineTabstops() override;
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	bool ClearTabstops(Sci::Line line) noexcept;
	bool AddTabstop(Sci::Line line, int x);
	int GetNextTabstop(Sci::Line line, int x) const noexcept;
};

}

#endif
