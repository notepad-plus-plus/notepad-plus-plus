// Scintilla source code edit control
/** @file PositionCache.h
 ** Classes for caching layout information.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef POSITIONCACHE_H
#define POSITIONCACHE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

static inline bool IsEOLChar(char ch) {
	return (ch == '\r') || (ch == '\n');
}

/**
 */
class LineLayout {
private:
	friend class LineLayoutCache;
	int *lineStarts;
	int lenLineStarts;
	/// Drawing is only performed for @a maxLineLength characters on each line.
	int lineNumber;
	bool inCache;
public:
	enum { wrapWidthInfinite = 0x7ffffff };
	int maxLineLength;
	int numCharsInLine;
	int numCharsBeforeEOL;
	enum validLevel { llInvalid, llCheckTextAndStyle, llPositions, llLines } validity;
	int xHighlightGuide;
	bool highlightColumn;
	Selection *psel;
	bool containsCaret;
	int edgeColumn;
	char *chars;
	unsigned char *styles;
	int styleBitsSet;
	char *indicators;
	XYPOSITION *positions;
	char bracePreviousStyles[2];

	// Hotspot support
	int hsStart;
	int hsEnd;

	// Wrapped line support
	int widthLine;
	int lines;
	XYPOSITION wrapIndent; // In pixels

	LineLayout(int maxLineLength_);
	virtual ~LineLayout();
	void Resize(int maxLineLength_);
	void Free();
	void Invalidate(validLevel validity_);
	int LineStart(int line) const;
	int LineLastVisible(int line) const;
	bool InLine(int offset, int line) const;
	void SetLineStart(int line, int start);
	void SetBracesHighlight(Range rangeLine, Position braces[],
		char bracesMatchStyle, int xHighlight, bool ignoreStyle);
	void RestoreBracesHighlight(Range rangeLine, Position braces[], bool ignoreStyle);
	int FindBefore(XYPOSITION x, int lower, int upper) const;
	int EndLineStyle() const;
};

/**
 */
class LineLayoutCache {
	int level;
	std::vector<LineLayout *>cache;
	bool allInvalidated;
	int styleClock;
	int useCount;
	void Allocate(size_t length_);
	void AllocateForLevel(int linesOnScreen, int linesInDoc);
public:
	LineLayoutCache();
	virtual ~LineLayoutCache();
	void Deallocate();
	enum {
		llcNone=SC_CACHE_NONE,
		llcCaret=SC_CACHE_CARET,
		llcPage=SC_CACHE_PAGE,
		llcDocument=SC_CACHE_DOCUMENT
	};
	void Invalidate(LineLayout::validLevel validity_);
	void SetLevel(int level_);
	int GetLevel() const { return level; }
	LineLayout *Retrieve(int lineNumber, int lineCaret, int maxChars, int styleClock_,
		int linesOnScreen, int linesInDoc);
	void Dispose(LineLayout *ll);
};

class PositionCacheEntry {
	unsigned int styleNumber:8;
	unsigned int len:8;
	unsigned int clock:16;
	XYPOSITION *positions;
public:
	PositionCacheEntry();
	~PositionCacheEntry();
	void Set(unsigned int styleNumber_, const char *s_, unsigned int len_, XYPOSITION *positions_, unsigned int clock);
	void Clear();
	bool Retrieve(unsigned int styleNumber_, const char *s_, unsigned int len_, XYPOSITION *positions_) const;
	static int Hash(unsigned int styleNumber_, const char *s, unsigned int len);
	bool NewerThan(const PositionCacheEntry &other) const;
	void ResetClock();
};

// Class to break a line of text into shorter runs at sensible places.
class BreakFinder {
	LineLayout *ll;
	int lineStart;
	int lineEnd;
	int posLineStart;
	int nextBreak;
	std::vector<int> selAndEdge;
	unsigned int saeCurrentPos;
	int saeNext;
	int subBreak;
	Document *pdoc;
	void Insert(int val);
	// Private so BreakFinder objects can not be copied
	BreakFinder(const BreakFinder &);
public:
	// If a whole run is longer than lengthStartSubdivision then subdivide
	// into smaller runs at spaces or punctuation.
	enum { lengthStartSubdivision = 300 };
	// Try to make each subdivided run lengthEachSubdivision or shorter.
	enum { lengthEachSubdivision = 100 };
	BreakFinder(LineLayout *ll_, int lineStart_, int lineEnd_, int posLineStart_,
		int xStart, bool breakForSelection, Document *pdoc_);
	~BreakFinder();
	int First() const;
	int Next();
};

class PositionCache {
	std::vector<PositionCacheEntry> pces;
	unsigned int clock;
	bool allClear;
	// Private so PositionCache objects can not be copied
	PositionCache(const PositionCache &);
public:
	PositionCache();
	~PositionCache();
	void Clear();
	void SetSize(size_t size_);
	size_t GetSize() const { return pces.size(); }
	void MeasureWidths(Surface *surface, ViewStyle &vstyle, unsigned int styleNumber,
		const char *s, unsigned int len, XYPOSITION *positions, Document *pdoc);
};

inline bool IsSpaceOrTab(int ch) {
	return ch == ' ' || ch == '\t';
}

#ifdef SCI_NAMESPACE
}
#endif

#endif
