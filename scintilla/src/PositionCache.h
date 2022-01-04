// Scintilla source code edit control
/** @file PositionCache.h
 ** Classes for caching layout information.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef POSITIONCACHE_H
#define POSITIONCACHE_H

namespace Scintilla::Internal {

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
enum class PointEnd {
	start = 0x0,
	lineEnd = 0x1,
	subLineEnd = 0x2,
	endEither = lineEnd | subLineEnd,
};

class BidiData {
public:
	std::vector<std::shared_ptr<Font>> stylesFonts;
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
public:
	enum { wrapWidthInfinite = 0x7ffffff };

	int maxLineLength;
	int numCharsInLine;
	int numCharsBeforeEOL;
	enum class ValidLevel { invalid, checkTextAndStyle, positions, lines } validity;
	int xHighlightGuide;
	bool highlightColumn;
	bool containsCaret;
	int edgeColumn;
	std::unique_ptr<char[]> chars;
	std::unique_ptr<unsigned char[]> styles;
	std::unique_ptr<XYPOSITION[]> positions;
	char bracePreviousStyles[2];

	std::unique_ptr<BidiData> bidiData;

	// Wrapped line support
	int widthLine;
	int lines;
	XYPOSITION wrapIndent; // In pixels

	LineLayout(Sci::Line lineNumber_, int maxLineLength_);
	// Deleted so LineLayout objects can not be copied.
	LineLayout(const LineLayout &) = delete;
	LineLayout(LineLayout &&) = delete;
	void operator=(const LineLayout &) = delete;
	void operator=(LineLayout &&) = delete;
	virtual ~LineLayout();
	void Resize(int maxLineLength_);
	void EnsureBidiData();
	void Free() noexcept;
	void Invalidate(ValidLevel validity_) noexcept;
	Sci::Line LineNumber() const noexcept;
	bool CanHold(Sci::Line lineDoc, int lineLength_) const noexcept;
	int LineStart(int line) const noexcept;
	int LineLength(int line) const noexcept;
	enum class Scope { visibleOnly, includeEnd };
	int LineLastVisible(int line, Scope scope) const noexcept;
	Range SubLineRange(int subLine, Scope scope) const noexcept;
	bool InLine(int offset, int line) const noexcept;
	int SubLineFromPosition(int posInLine, PointEnd pe) const noexcept;
	void SetLineStart(int line, int start);
	void SetBracesHighlight(Range rangeLine, const Sci::Position braces[],
		char bracesMatchStyle, int xHighlight, bool ignoreStyle);
	void RestoreBracesHighlight(Range rangeLine, const Sci::Position braces[], bool ignoreStyle);
	int FindBefore(XYPOSITION x, Range range) const noexcept;
	int FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const noexcept;
	Point PointFromPosition(int posInLine, int lineHeight, PointEnd pe) const noexcept;
	int EndLineStyle() const noexcept;
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
public:
private:
	Scintilla::LineCache level;
	std::vector<std::shared_ptr<LineLayout>>cache;
	bool allInvalidated;
	int styleClock;
	size_t EntryForLine(Sci::Line line) const noexcept;
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
	void Invalidate(LineLayout::ValidLevel validity_) noexcept;
	void SetLevel(Scintilla::LineCache level_) noexcept;
	Scintilla::LineCache GetLevel() const noexcept { return level; }
	std::shared_ptr<LineLayout> Retrieve(Sci::Line lineNumber, Sci::Line lineCaret, int maxChars, int styleClock_,
		Sci::Line linesOnScreen, Sci::Line linesInDoc);
};

class Representation {
public:
	static constexpr size_t maxLength = 200;
	std::string stringRep;
	RepresentationAppearance appearance;
	ColourRGBA colour;
	explicit Representation(std::string_view value="", RepresentationAppearance appearance_= RepresentationAppearance::Blob) :
		stringRep(value), appearance(appearance_) {
	}
};

typedef std::map<unsigned int, Representation> MapRepresentation;

class SpecialRepresentations {
	MapRepresentation mapReprs;
	unsigned short startByteHasReprs[0x100] {};
	unsigned int maxKey = 0;
	bool crlf = false;
public:
	void SetRepresentation(std::string_view charBytes, std::string_view value);
	void SetRepresentationAppearance(std::string_view charBytes, RepresentationAppearance appearance);
	void SetRepresentationColour(std::string_view charBytes, ColourRGBA colour);
	void ClearRepresentation(std::string_view charBytes);
	const Representation *GetRepresentation(std::string_view charBytes) const;
	const Representation *RepresentationFromCharacter(std::string_view charBytes) const;
	bool ContainsCrLf() const noexcept {
		return crlf;
	}
	bool MayContain(unsigned char ch) const noexcept {
		return startByteHasReprs[ch] != 0;
	}
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
	const Range lineRange;
	int nextBreak;
	std::vector<int> selAndEdge;
	unsigned int saeCurrentPos;
	int saeNext;
	int subBreak;
	const Document *pdoc;
	const EncodingFamily encodingFamily;
	const SpecialRepresentations *preprs;
	void Insert(Sci::Position val);
public:
	// If a whole run is longer than lengthStartSubdivision then subdivide
	// into smaller runs at spaces or punctuation.
	enum { lengthStartSubdivision = 300 };
	// Try to make each subdivided run lengthEachSubdivision or shorter.
	enum { lengthEachSubdivision = 100 };
	enum class BreakFor {
		Text = 0,
		Selection = 1,
		Foreground = 2,
		ForegroundAndSelection = 3,
	};
	BreakFinder(const LineLayout *ll_, const Selection *psel, Range lineRange_, Sci::Position posLineStart,
		XYPOSITION xStart, BreakFor breakFor, const Document *pdoc_, const SpecialRepresentations *preprs_, const ViewStyle *pvsDraw);
	// Deleted so BreakFinder objects can not be copied.
	BreakFinder(const BreakFinder &) = delete;
	BreakFinder(BreakFinder &&) = delete;
	void operator=(const BreakFinder &) = delete;
	void operator=(BreakFinder &&) = delete;
	~BreakFinder() noexcept;
	TextSegment Next();
	bool More() const noexcept;
};

class IPositionCache {
public:
	virtual ~IPositionCache() = default;
	virtual void Clear() noexcept = 0;
	virtual void SetSize(size_t size_) = 0;
	virtual size_t GetSize() const noexcept = 0;
	virtual void MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
		std::string_view sv, XYPOSITION *positions, bool needsLocking) = 0;
};

std::unique_ptr<IPositionCache> CreatePositionCache();

}

#endif
