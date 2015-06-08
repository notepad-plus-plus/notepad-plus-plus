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
	bool containsCaret;
	int edgeColumn;
	char *chars;
	unsigned char *styles;
	XYPOSITION *positions;
	char bracePreviousStyles[2];

	// Hotspot support
	Range hotspot;

	// Wrapped line support
	int widthLine;
	int lines;
	XYPOSITION wrapIndent; // In pixels

	explicit LineLayout(int maxLineLength_);
	virtual ~LineLayout();
	void Resize(int maxLineLength_);
	void Free();
	void Invalidate(validLevel validity_);
	int LineStart(int line) const;
	int LineLastVisible(int line) const;
	Range SubLineRange(int line) const;
	bool InLine(int offset, int line) const;
	void SetLineStart(int line, int start);
	void SetBracesHighlight(Range rangeLine, const Position braces[],
		char bracesMatchStyle, int xHighlight, bool ignoreStyle);
	void RestoreBracesHighlight(Range rangeLine, const Position braces[], bool ignoreStyle);
	int FindBefore(XYPOSITION x, int lower, int upper) const;
	int FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const;
	Point PointFromPosition(int posInLine, int lineHeight) const;
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
	void Set(unsigned int styleNumber_, const char *s_, unsigned int len_, XYPOSITION *positions_, unsigned int clock_);
	void Clear();
	bool Retrieve(unsigned int styleNumber_, const char *s_, unsigned int len_, XYPOSITION *positions_) const;
	static unsigned int Hash(unsigned int styleNumber_, const char *s, unsigned int len);
	bool NewerThan(const PositionCacheEntry &other) const;
	void ResetClock();
};

class Representation {
public:
	std::string stringRep;
	explicit Representation(const char *value="") : stringRep(value) {
	}
};

typedef std::map<int, Representation> MapRepresentation;

class SpecialRepresentations {
	MapRepresentation mapReprs;
	short startByteHasReprs[0x100];
public:
	SpecialRepresentations();
	void SetRepresentation(const char *charBytes, const char *value);
	void ClearRepresentation(const char *charBytes);
	const Representation *RepresentationFromCharacter(const char *charBytes, size_t len) const;
	bool Contains(const char *charBytes, size_t len) const;
	void Clear();
};

struct TextSegment {
	int start;
	int length;
	const Representation *representation;
	TextSegment(int start_=0, int length_=0, const Representation *representation_=0) :
		start(start_), length(length_), representation(representation_) {
	}
	int end() const {
		return start + length;
	}
};

// Class to break a line of text into shorter runs at sensible places.
class BreakFinder {
	const LineLayout *ll;
	Range lineRange;
	int posLineStart;
	int nextBreak;
	std::vector<int> selAndEdge;
	unsigned int saeCurrentPos;
	int saeNext;
	int subBreak;
	const Document *pdoc;
	EncodingFamily encodingFamily;
	const SpecialRepresentations *preprs;
	void Insert(int val);
	// Private so BreakFinder objects can not be copied
	BreakFinder(const BreakFinder &);
public:
	// If a whole run is longer than lengthStartSubdivision then subdivide
	// into smaller runs at spaces or punctuation.
	enum { lengthStartSubdivision = 300 };
	// Try to make each subdivided run lengthEachSubdivision or shorter.
	enum { lengthEachSubdivision = 100 };
	BreakFinder(const LineLayout *ll_, const Selection *psel, Range rangeLine_, int posLineStart_,
		int xStart, bool breakForSelection, const Document *pdoc_, const SpecialRepresentations *preprs_, const ViewStyle *pvsDraw);
	~BreakFinder();
	TextSegment Next();
	bool More() const;
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
	void MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
		const char *s, unsigned int len, XYPOSITION *positions, Document *pdoc);
};

inline bool IsSpaceOrTab(int ch) {
	return ch == ' ' || ch == '\t';
}

#ifdef SCI_NAMESPACE
}
#endif

#endif
