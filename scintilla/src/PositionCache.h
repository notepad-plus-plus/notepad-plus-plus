// Scintilla source code edit control
/** @file PositionCache.h
 ** Classes for caching layout information.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef POSITIONCACHE_H
#define POSITIONCACHE_H

namespace Scintilla {

inline constexpr bool IsEOLChar(int ch) noexcept {
	return (ch == '\r') || (ch == '\n');
}

inline constexpr bool IsSpaceOrTab(int ch) noexcept {
	return ch == ' ' || ch == '\t';
}

/**
* A point in document space.
* Uses double for sufficient resolution in large (>20,000,000 line) documents.
*/
class PointDocument {
public:
	double x;
	double y;

	explicit PointDocument(double x_ = 0, double y_ = 0) noexcept : x(x_), y(y_) {
	}

	// Conversion from Point.
	explicit PointDocument(Point pt) noexcept : x(pt.x), y(pt.y) {
	}
};

// There are two points for some positions and this enumeration
// can choose between the end of the first line or subline
// and the start of the next line or subline.
enum PointEnd {
	peDefault = 0x0,
	peLineEnd = 0x1,
	peSubLineEnd = 0x2
};

class BidiData {
public:
	std::vector<FontAlias> stylesFonts;
	std::vector<XYPOSITION> widthReprs;
	void Resize(size_t maxLineLength_);
};

/**
 */
class LineLayout {
private:
	friend class LineLayoutCache;
	std::unique_ptr<int []>lineStarts;
	int lenLineStarts;
	/// Drawing is only performed for @a maxLineLength characters on each line.
	Sci::Line lineNumber;
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
	std::unique_ptr<char[]> chars;
	std::unique_ptr<unsigned char[]> styles;
	std::unique_ptr<XYPOSITION[]> positions;
	char bracePreviousStyles[2];

	std::unique_ptr<BidiData> bidiData;

	// Hotspot support
	Range hotspot;

	// Wrapped line support
	int widthLine;
	int lines;
	XYPOSITION wrapIndent; // In pixels

	explicit LineLayout(int maxLineLength_);
	// Deleted so LineLayout objects can not be copied.
	LineLayout(const LineLayout &) = delete;
	LineLayout(LineLayout &&) = delete;
	void operator=(const LineLayout &) = delete;
	void operator=(LineLayout &&) = delete;
	virtual ~LineLayout();
	void Resize(int maxLineLength_);
	void EnsureBidiData();
	void Free() noexcept;
	void Invalidate(validLevel validity_);
	int LineStart(int line) const;
	int LineLength(int line) const;
	enum class Scope { visibleOnly, includeEnd };
	int LineLastVisible(int line, Scope scope) const;
	Range SubLineRange(int subLine, Scope scope) const;
	bool InLine(int offset, int line) const;
	int SubLineFromPosition(int posInLine, PointEnd pe) const;
	void SetLineStart(int line, int start);
	void SetBracesHighlight(Range rangeLine, const Sci::Position braces[],
		char bracesMatchStyle, int xHighlight, bool ignoreStyle);
	void RestoreBracesHighlight(Range rangeLine, const Sci::Position braces[], bool ignoreStyle);
	int FindBefore(XYPOSITION x, Range range) const;
	int FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const;
	Point PointFromPosition(int posInLine, int lineHeight, PointEnd pe) const;
	int EndLineStyle() const;
};

struct ScreenLine : public IScreenLine {
	const LineLayout *ll;
	size_t start;
	size_t len;
	XYPOSITION width;
	XYPOSITION height;
	int ctrlCharPadding;
	XYPOSITION tabWidth;
	int tabWidthMinimumPixels;

	ScreenLine(const LineLayout *ll_, int subLine, const ViewStyle &vs, XYPOSITION width_, int tabWidthMinimumPixels_);
	// Deleted so ScreenLine objects can not be copied.
	ScreenLine(const ScreenLine &) = delete;
	ScreenLine(ScreenLine &&) = delete;
	void operator=(const ScreenLine &) = delete;
	void operator=(ScreenLine &&) = delete;
	virtual ~ScreenLine();

	std::string_view Text() const override;
	size_t Length() const override;
	size_t RepresentationCount() const override;
	XYPOSITION Width() const override;
	XYPOSITION Height() const override;
	XYPOSITION TabWidth() const override;
	XYPOSITION TabWidthMinimumPixels() const override;
	const Font *FontOfPosition(size_t position) const override;
	XYPOSITION RepresentationWidth(size_t position) const override;
	XYPOSITION TabPositionAfter(XYPOSITION xPosition) const override;
};

/**
 */
class LineLayoutCache {
	int level;
	std::vector<std::unique_ptr<LineLayout>>cache;
	bool allInvalidated;
	int styleClock;
	int useCount;
	void Allocate(size_t length_);
	void AllocateForLevel(Sci::Line linesOnScreen, Sci::Line linesInDoc);
public:
	LineLayoutCache();
	// Deleted so LineLayoutCache objects can not be copied.
	LineLayoutCache(const LineLayoutCache &) = delete;
	LineLayoutCache(LineLayoutCache &&) = delete;
	void operator=(const LineLayoutCache &) = delete;
	void operator=(LineLayoutCache &&) = delete;
	virtual ~LineLayoutCache();
	void Deallocate() noexcept;
	enum {
		llcNone=SC_CACHE_NONE,
		llcCaret=SC_CACHE_CARET,
		llcPage=SC_CACHE_PAGE,
		llcDocument=SC_CACHE_DOCUMENT
	};
	void Invalidate(LineLayout::validLevel validity_);
	void SetLevel(int level_) noexcept;
	int GetLevel() const noexcept { return level; }
	LineLayout *Retrieve(Sci::Line lineNumber, Sci::Line lineCaret, int maxChars, int styleClock_,
		Sci::Line linesOnScreen, Sci::Line linesInDoc);
	void Dispose(LineLayout *ll) noexcept;
};

class PositionCacheEntry {
	unsigned int styleNumber:8;
	unsigned int len:8;
	unsigned int clock:16;
	std::unique_ptr<XYPOSITION []> positions;
public:
	PositionCacheEntry() noexcept;
	// Copy constructor not currently used, but needed for being element in std::vector.
	PositionCacheEntry(const PositionCacheEntry &);
	// Deleted so PositionCacheEntry objects can not be assigned.
	PositionCacheEntry(PositionCacheEntry &&) = delete;
	void operator=(const PositionCacheEntry &) = delete;
	void operator=(PositionCacheEntry &&) = delete;
	~PositionCacheEntry();
	void Set(unsigned int styleNumber_, const char *s_, unsigned int len_, const XYPOSITION *positions_, unsigned int clock_);
	void Clear() noexcept;
	bool Retrieve(unsigned int styleNumber_, const char *s_, unsigned int len_, XYPOSITION *positions_) const;
	static unsigned int Hash(unsigned int styleNumber_, const char *s, unsigned int len_) noexcept;
	bool NewerThan(const PositionCacheEntry &other) const noexcept;
	void ResetClock() noexcept;
};

class Representation {
public:
	std::string stringRep;
	explicit Representation(const char *value="") : stringRep(value) {
	}
};

typedef std::map<unsigned int, Representation> MapRepresentation;

class SpecialRepresentations {
	MapRepresentation mapReprs;
	short startByteHasReprs[0x100];
public:
	SpecialRepresentations() noexcept;
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
	TextSegment(int start_=0, int length_=0, const Representation *representation_=nullptr) noexcept :
		start(start_), length(length_), representation(representation_) {
	}
	int end() const noexcept {
		return start + length;
	}
};

// Class to break a line of text into shorter runs at sensible places.
class BreakFinder {
	const LineLayout *ll;
	Range lineRange;
	Sci::Position posLineStart;
	int nextBreak;
	std::vector<int> selAndEdge;
	unsigned int saeCurrentPos;
	int saeNext;
	int subBreak;
	const Document *pdoc;
	EncodingFamily encodingFamily;
	const SpecialRepresentations *preprs;
	void Insert(Sci::Position val);
public:
	// If a whole run is longer than lengthStartSubdivision then subdivide
	// into smaller runs at spaces or punctuation.
	enum { lengthStartSubdivision = 300 };
	// Try to make each subdivided run lengthEachSubdivision or shorter.
	enum { lengthEachSubdivision = 100 };
	BreakFinder(const LineLayout *ll_, const Selection *psel, Range lineRange_, Sci::Position posLineStart_,
		int xStart, bool breakForSelection, const Document *pdoc_, const SpecialRepresentations *preprs_, const ViewStyle *pvsDraw);
	// Deleted so BreakFinder objects can not be copied.
	BreakFinder(const BreakFinder &) = delete;
	BreakFinder(BreakFinder &&) = delete;
	void operator=(const BreakFinder &) = delete;
	void operator=(BreakFinder &&) = delete;
	~BreakFinder();
	TextSegment Next();
	bool More() const noexcept;
};

class PositionCache {
	std::vector<PositionCacheEntry> pces;
	unsigned int clock;
	bool allClear;
public:
	PositionCache();
	// Deleted so PositionCache objects can not be copied.
	PositionCache(const PositionCache &) = delete;
	PositionCache(PositionCache &&) = delete;
	void operator=(const PositionCache &) = delete;
	void operator=(PositionCache &&) = delete;
	~PositionCache();
	void Clear() noexcept;
	void SetSize(size_t size_);
	size_t GetSize() const noexcept { return pces.size(); }
	void MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
		const char *s, unsigned int len, XYPOSITION *positions, const Document *pdoc);
};

}

#endif
