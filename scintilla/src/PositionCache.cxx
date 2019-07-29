// Scintilla source code edit control
/** @file PositionCache.cxx
 ** Classes for caching layout information.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <memory>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterCategory.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"

using namespace Scintilla;

void BidiData::Resize(size_t maxLineLength_) {
	stylesFonts.resize(maxLineLength_ + 1);
	widthReprs.resize(maxLineLength_ + 1);
}

LineLayout::LineLayout(int maxLineLength_) :
	lenLineStarts(0),
	lineNumber(-1),
	inCache(false),
	maxLineLength(-1),
	numCharsInLine(0),
	numCharsBeforeEOL(0),
	validity(llInvalid),
	xHighlightGuide(0),
	highlightColumn(false),
	containsCaret(false),
	edgeColumn(0),
	bracePreviousStyles{},
	hotspot(0,0),
	widthLine(wrapWidthInfinite),
	lines(1),
	wrapIndent(0) {
	Resize(maxLineLength_);
}

LineLayout::~LineLayout() {
	Free();
}

void LineLayout::Resize(int maxLineLength_) {
	if (maxLineLength_ > maxLineLength) {
		Free();
		chars = std::make_unique<char[]>(maxLineLength_ + 1);
		styles = std::make_unique<unsigned char []>(maxLineLength_ + 1);
		// Extra position allocated as sometimes the Windows
		// GetTextExtentExPoint API writes an extra element.
		positions = std::make_unique<XYPOSITION []>(maxLineLength_ + 1 + 1);
		if (bidiData) {
			bidiData->Resize(maxLineLength_);
		}

		maxLineLength = maxLineLength_;
	}
}

void LineLayout::EnsureBidiData() {
	if (!bidiData) {
		bidiData = std::make_unique<BidiData>();
		bidiData->Resize(maxLineLength);
	}
}

void LineLayout::Free() noexcept {
	chars.reset();
	styles.reset();
	positions.reset();
	lineStarts.reset();
	bidiData.reset();
}

void LineLayout::Invalidate(validLevel validity_) {
	if (validity > validity_)
		validity = validity_;
}

int LineLayout::LineStart(int line) const {
	if (line <= 0) {
		return 0;
	} else if ((line >= lines) || !lineStarts) {
		return numCharsInLine;
	} else {
		return lineStarts[line];
	}
}

int Scintilla::LineLayout::LineLength(int line) const {
	if (!lineStarts) {
		return numCharsInLine;
	} if (line >= lines - 1) {
		return numCharsInLine - lineStarts[line];
	} else {
		return lineStarts[line + 1] - lineStarts[line];
	}
}

int LineLayout::LineLastVisible(int line, Scope scope) const {
	if (line < 0) {
		return 0;
	} else if ((line >= lines-1) || !lineStarts) {
		return scope == Scope::visibleOnly ? numCharsBeforeEOL : numCharsInLine;
	} else {
		return lineStarts[line+1];
	}
}

Range LineLayout::SubLineRange(int subLine, Scope scope) const {
	return Range(LineStart(subLine), LineLastVisible(subLine, scope));
}

bool LineLayout::InLine(int offset, int line) const {
	return ((offset >= LineStart(line)) && (offset < LineStart(line + 1))) ||
		((offset == numCharsInLine) && (line == (lines-1)));
}

int LineLayout::SubLineFromPosition(int posInLine, PointEnd pe) const {
	if (!lineStarts || (posInLine > maxLineLength)) {
		return lines - 1;
	}

	for (int line = 0; line < lines; line++) {
		if (pe & peSubLineEnd) {
			// Return subline not start of next
			if (lineStarts[line + 1] <= posInLine + 1)
				return line;
		} else {
			if (lineStarts[line + 1] <= posInLine)
				return line;
		}
	}

	return lines - 1;
}

void LineLayout::SetLineStart(int line, int start) {
	if ((line >= lenLineStarts) && (line != 0)) {
		const int newMaxLines = line + 20;
		std::unique_ptr<int[]> newLineStarts = std::make_unique<int[]>(newMaxLines);
		for (int i = 0; i < newMaxLines; i++) {
			if (i < lenLineStarts)
				newLineStarts[i] = lineStarts[i];
			else
				newLineStarts[i] = 0;
		}
		lineStarts = std::move(newLineStarts);
		lenLineStarts = newMaxLines;
	}
	lineStarts[line] = start;
}

void LineLayout::SetBracesHighlight(Range rangeLine, const Sci::Position braces[],
                                    char bracesMatchStyle, int xHighlight, bool ignoreStyle) {
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[0])) {
		const Sci::Position braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[0] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[1])) {
		const Sci::Position braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[1] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if ((braces[0] >= rangeLine.start && braces[1] <= rangeLine.end) ||
	        (braces[1] >= rangeLine.start && braces[0] <= rangeLine.end)) {
		xHighlightGuide = xHighlight;
	}
}

void LineLayout::RestoreBracesHighlight(Range rangeLine, const Sci::Position braces[], bool ignoreStyle) {
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[0])) {
		const Sci::Position braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[0];
		}
	}
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[1])) {
		const Sci::Position braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[1];
		}
	}
	xHighlightGuide = 0;
}

int LineLayout::FindBefore(XYPOSITION x, Range range) const {
	Sci::Position lower = range.start;
	Sci::Position upper = range.end;
	do {
		const Sci::Position middle = (upper + lower + 1) / 2; 	// Round high
		const XYPOSITION posMiddle = positions[middle];
		if (x < posMiddle) {
			upper = middle - 1;
		} else {
			lower = middle;
		}
	} while (lower < upper);
	return static_cast<int>(lower);
}


int LineLayout::FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const {
	int pos = FindBefore(x, range);
	while (pos < range.end) {
		if (charPosition) {
			if (x < (positions[pos + 1])) {
				return pos;
			}
		} else {
			if (x < ((positions[pos] + positions[pos + 1]) / 2)) {
				return pos;
			}
		}
		pos++;
	}
	return static_cast<int>(range.end);
}

Point LineLayout::PointFromPosition(int posInLine, int lineHeight, PointEnd pe) const {
	Point pt;
	// In case of very long line put x at arbitrary large position
	if (posInLine > maxLineLength) {
		pt.x = positions[maxLineLength] - positions[LineStart(lines)];
	}

	for (int subLine = 0; subLine < lines; subLine++) {
		const Range rangeSubLine = SubLineRange(subLine, Scope::visibleOnly);
		if (posInLine >= rangeSubLine.start) {
			pt.y = static_cast<XYPOSITION>(subLine*lineHeight);
			if (posInLine <= rangeSubLine.end) {
				pt.x = positions[posInLine] - positions[rangeSubLine.start];
				if (rangeSubLine.start != 0)	// Wrapped lines may be indented
					pt.x += wrapIndent;
				if (pe & peSubLineEnd)	// Return end of first subline not start of next
					break;
			} else if ((pe & peLineEnd) && (subLine == (lines-1))) {
				pt.x = positions[numCharsInLine] - positions[rangeSubLine.start];
				if (rangeSubLine.start != 0)	// Wrapped lines may be indented
					pt.x += wrapIndent;
			}
		} else {
			break;
		}
	}
	return pt;
}

int LineLayout::EndLineStyle() const {
	return styles[numCharsBeforeEOL > 0 ? numCharsBeforeEOL-1 : 0];
}

ScreenLine::ScreenLine(
	const LineLayout *ll_,
	int subLine,
	const ViewStyle &vs,
	XYPOSITION width_,
	int tabWidthMinimumPixels_) :
	ll(ll_),
	start(ll->LineStart(subLine)),
	len(ll->LineLength(subLine)),
	width(width_),
	height(static_cast<float>(vs.lineHeight)),
	ctrlCharPadding(vs.ctrlCharPadding),
	tabWidth(vs.tabWidth),
	tabWidthMinimumPixels(tabWidthMinimumPixels_) {
}

ScreenLine::~ScreenLine() {
}

std::string_view ScreenLine::Text() const {
	return std::string_view(&ll->chars[start], len);
}

size_t ScreenLine::Length() const {
	return len;
}

size_t ScreenLine::RepresentationCount() const {
	return std::count_if(&ll->bidiData->widthReprs[start],
		&ll->bidiData->widthReprs[start + len],
		[](XYPOSITION w) {return w > 0.0f; });
}

XYPOSITION ScreenLine::Width() const {
	return width;
}

XYPOSITION ScreenLine::Height() const {
	return height;
}

XYPOSITION ScreenLine::TabWidth() const {
	return tabWidth;
}

XYPOSITION ScreenLine::TabWidthMinimumPixels() const {
	return static_cast<XYPOSITION>(tabWidthMinimumPixels);
}

const Font *ScreenLine::FontOfPosition(size_t position) const {
	return &ll->bidiData->stylesFonts[start + position];
}

XYPOSITION ScreenLine::RepresentationWidth(size_t position) const {
	return ll->bidiData->widthReprs[start + position];
}

XYPOSITION ScreenLine::TabPositionAfter(XYPOSITION xPosition) const {
	return (std::floor((xPosition + TabWidthMinimumPixels()) / TabWidth()) + 1) * TabWidth();
}

LineLayoutCache::LineLayoutCache() :
	level(0),
	allInvalidated(false), styleClock(-1), useCount(0) {
	Allocate(0);
}

LineLayoutCache::~LineLayoutCache() {
	Deallocate();
}

void LineLayoutCache::Allocate(size_t length_) {
	PLATFORM_ASSERT(cache.empty());
	allInvalidated = false;
	cache.resize(length_);
}

void LineLayoutCache::AllocateForLevel(Sci::Line linesOnScreen, Sci::Line linesInDoc) {
	PLATFORM_ASSERT(useCount == 0);
	size_t lengthForLevel = 0;
	if (level == llcCaret) {
		lengthForLevel = 1;
	} else if (level == llcPage) {
		lengthForLevel = linesOnScreen + 1;
	} else if (level == llcDocument) {
		lengthForLevel = linesInDoc;
	}
	if (lengthForLevel > cache.size()) {
		Deallocate();
		Allocate(lengthForLevel);
	} else {
		if (lengthForLevel < cache.size()) {
			for (size_t i = lengthForLevel; i < cache.size(); i++) {
				cache[i].reset();
			}
		}
		cache.resize(lengthForLevel);
	}
	PLATFORM_ASSERT(cache.size() == lengthForLevel);
}

void LineLayoutCache::Deallocate() noexcept {
	PLATFORM_ASSERT(useCount == 0);
	cache.clear();
}

void LineLayoutCache::Invalidate(LineLayout::validLevel validity_) {
	if (!cache.empty() && !allInvalidated) {
		for (const std::unique_ptr<LineLayout> &ll : cache) {
			if (ll) {
				ll->Invalidate(validity_);
			}
		}
		if (validity_ == LineLayout::llInvalid) {
			allInvalidated = true;
		}
	}
}

void LineLayoutCache::SetLevel(int level_) noexcept {
	allInvalidated = false;
	if ((level_ != -1) && (level != level_)) {
		level = level_;
		Deallocate();
	}
}

LineLayout *LineLayoutCache::Retrieve(Sci::Line lineNumber, Sci::Line lineCaret, int maxChars, int styleClock_,
                                      Sci::Line linesOnScreen, Sci::Line linesInDoc) {
	AllocateForLevel(linesOnScreen, linesInDoc);
	if (styleClock != styleClock_) {
		Invalidate(LineLayout::llCheckTextAndStyle);
		styleClock = styleClock_;
	}
	allInvalidated = false;
	Sci::Position pos = -1;
	LineLayout *ret = nullptr;
	if (level == llcCaret) {
		pos = 0;
	} else if (level == llcPage) {
		if (lineNumber == lineCaret) {
			pos = 0;
		} else if (cache.size() > 1) {
			pos = 1 + (lineNumber % (cache.size() - 1));
		}
	} else if (level == llcDocument) {
		pos = lineNumber;
	}
	if (pos >= 0) {
		PLATFORM_ASSERT(useCount == 0);
		if (!cache.empty() && (pos < static_cast<int>(cache.size()))) {
			if (cache[pos]) {
				if ((cache[pos]->lineNumber != lineNumber) ||
				        (cache[pos]->maxLineLength < maxChars)) {
					cache[pos].reset();
				}
			}
			if (!cache[pos]) {
				cache[pos] = std::make_unique<LineLayout>(maxChars);
			}
			cache[pos]->lineNumber = lineNumber;
			cache[pos]->inCache = true;
			ret = cache[pos].get();
			useCount++;
		}
	}

	if (!ret) {
		ret = new LineLayout(maxChars);
		ret->lineNumber = lineNumber;
	}

	return ret;
}

void LineLayoutCache::Dispose(LineLayout *ll) noexcept {
	allInvalidated = false;
	if (ll) {
		if (!ll->inCache) {
			delete ll;
		} else {
			useCount--;
		}
	}
}

// Simply pack the (maximum 4) character bytes into an int
static unsigned int KeyFromString(const char *charBytes, size_t len) {
	PLATFORM_ASSERT(len <= 4);
	unsigned int k=0;
	for (size_t i=0; i<len && charBytes[i]; i++) {
		k = k * 0x100;
		const unsigned char uc = charBytes[i];
		k += uc;
	}
	return k;
}

SpecialRepresentations::SpecialRepresentations() noexcept {
	const short none = 0;
	std::fill(startByteHasReprs, std::end(startByteHasReprs), none);
}

void SpecialRepresentations::SetRepresentation(const char *charBytes, const char *value) {
	const unsigned int key = KeyFromString(charBytes, UTF8MaxBytes);
	MapRepresentation::iterator it = mapReprs.find(key);
	if (it == mapReprs.end()) {
		// New entry so increment for first byte
		const unsigned char ucStart = charBytes[0];
		startByteHasReprs[ucStart]++;
	}
	mapReprs[key] = Representation(value);
}

void SpecialRepresentations::ClearRepresentation(const char *charBytes) {
	MapRepresentation::iterator it = mapReprs.find(KeyFromString(charBytes, UTF8MaxBytes));
	if (it != mapReprs.end()) {
		mapReprs.erase(it);
		const unsigned char ucStart = charBytes[0];
		startByteHasReprs[ucStart]--;
	}
}

const Representation *SpecialRepresentations::RepresentationFromCharacter(const char *charBytes, size_t len) const {
	PLATFORM_ASSERT(len <= 4);
	const unsigned char ucStart = charBytes[0];
	if (!startByteHasReprs[ucStart])
		return nullptr;
	MapRepresentation::const_iterator it = mapReprs.find(KeyFromString(charBytes, len));
	if (it != mapReprs.end()) {
		return &(it->second);
	}
	return nullptr;
}

bool SpecialRepresentations::Contains(const char *charBytes, size_t len) const {
	PLATFORM_ASSERT(len <= 4);
	const unsigned char ucStart = charBytes[0];
	if (!startByteHasReprs[ucStart])
		return false;
	MapRepresentation::const_iterator it = mapReprs.find(KeyFromString(charBytes, len));
	return it != mapReprs.end();
}

void SpecialRepresentations::Clear() {
	mapReprs.clear();
	const short none = 0;
	std::fill(startByteHasReprs, std::end(startByteHasReprs), none);
}

void BreakFinder::Insert(Sci::Position val) {
	const int posInLine = static_cast<int>(val);
	if (posInLine > nextBreak) {
		const std::vector<int>::iterator it = std::lower_bound(selAndEdge.begin(), selAndEdge.end(), posInLine);
		if (it == selAndEdge.end()) {
			selAndEdge.push_back(posInLine);
		} else if (*it != posInLine) {
			selAndEdge.insert(it, 1, posInLine);
		}
	}
}

BreakFinder::BreakFinder(const LineLayout *ll_, const Selection *psel, Range lineRange_, Sci::Position posLineStart_,
	int xStart, bool breakForSelection, const Document *pdoc_, const SpecialRepresentations *preprs_, const ViewStyle *pvsDraw) :
	ll(ll_),
	lineRange(lineRange_),
	posLineStart(posLineStart_),
	nextBreak(static_cast<int>(lineRange_.start)),
	saeCurrentPos(0),
	saeNext(0),
	subBreak(-1),
	pdoc(pdoc_),
	encodingFamily(pdoc_->CodePageFamily()),
	preprs(preprs_) {

	// Search for first visible break
	// First find the first visible character
	if (xStart > 0.0f)
		nextBreak = ll->FindBefore(static_cast<XYPOSITION>(xStart), lineRange);
	// Now back to a style break
	while ((nextBreak > lineRange.start) && (ll->styles[nextBreak] == ll->styles[nextBreak - 1])) {
		nextBreak--;
	}

	if (breakForSelection) {
		const SelectionPosition posStart(posLineStart);
		const SelectionPosition posEnd(posLineStart + lineRange.end);
		const SelectionSegment segmentLine(posStart, posEnd);
		for (size_t r=0; r<psel->Count(); r++) {
			const SelectionSegment portion = psel->Range(r).Intersect(segmentLine);
			if (!(portion.start == portion.end)) {
				if (portion.start.IsValid())
					Insert(portion.start.Position() - posLineStart);
				if (portion.end.IsValid())
					Insert(portion.end.Position() - posLineStart);
			}
		}
	}
	if (pvsDraw && pvsDraw->indicatorsSetFore) {
		for (const IDecoration *deco : pdoc->decorations->View()) {
			if (pvsDraw->indicators[deco->Indicator()].OverridesTextFore()) {
				Sci::Position startPos = deco->EndRun(posLineStart);
				while (startPos < (posLineStart + lineRange.end)) {
					Insert(startPos - posLineStart);
					startPos = deco->EndRun(startPos);
				}
			}
		}
	}
	Insert(ll->edgeColumn);
	Insert(lineRange.end);
	saeNext = (!selAndEdge.empty()) ? selAndEdge[0] : -1;
}

BreakFinder::~BreakFinder() {
}

TextSegment BreakFinder::Next() {
	if (subBreak == -1) {
		const int prev = nextBreak;
		while (nextBreak < lineRange.end) {
			int charWidth = 1;
			if (encodingFamily == efUnicode)
				charWidth = UTF8DrawBytes(reinterpret_cast<unsigned char *>(&ll->chars[nextBreak]),
					static_cast<int>(lineRange.end - nextBreak));
			else if (encodingFamily == efDBCS)
				charWidth = pdoc->DBCSDrawBytes(
					std::string_view(&ll->chars[nextBreak], lineRange.end - nextBreak));
			const Representation *repr = preprs->RepresentationFromCharacter(&ll->chars[nextBreak], charWidth);
			if (((nextBreak > 0) && (ll->styles[nextBreak] != ll->styles[nextBreak - 1])) ||
					repr ||
					(nextBreak == saeNext)) {
				while ((nextBreak >= saeNext) && (saeNext < lineRange.end)) {
					saeCurrentPos++;
					saeNext = static_cast<int>((saeCurrentPos < selAndEdge.size()) ? selAndEdge[saeCurrentPos] : lineRange.end);
				}
				if ((nextBreak > prev) || repr) {
					// Have a segment to report
					if (nextBreak == prev) {
						nextBreak += charWidth;
					} else {
						repr = nullptr;	// Optimize -> should remember repr
					}
					if ((nextBreak - prev) < lengthStartSubdivision) {
						return TextSegment(prev, nextBreak - prev, repr);
					} else {
						break;
					}
				}
			}
			nextBreak += charWidth;
		}
		if ((nextBreak - prev) < lengthStartSubdivision) {
			return TextSegment(prev, nextBreak - prev);
		}
		subBreak = prev;
	}
	// Splitting up a long run from prev to nextBreak in lots of approximately lengthEachSubdivision.
	// For very long runs add extra breaks after spaces or if no spaces before low punctuation.
	const int startSegment = subBreak;
	if ((nextBreak - subBreak) <= lengthEachSubdivision) {
		subBreak = -1;
		return TextSegment(startSegment, nextBreak - startSegment);
	} else {
		subBreak += pdoc->SafeSegment(&ll->chars[subBreak], nextBreak-subBreak, lengthEachSubdivision);
		if (subBreak >= nextBreak) {
			subBreak = -1;
			return TextSegment(startSegment, nextBreak - startSegment);
		} else {
			return TextSegment(startSegment, subBreak - startSegment);
		}
	}
}

bool BreakFinder::More() const noexcept {
	return (subBreak >= 0) || (nextBreak < lineRange.end);
}

PositionCacheEntry::PositionCacheEntry() noexcept :
	styleNumber(0), len(0), clock(0), positions(nullptr) {
}

// Copy constructor not currently used, but needed for being element in std::vector.
PositionCacheEntry::PositionCacheEntry(const PositionCacheEntry &other) :
	styleNumber(other.styleNumber), len(other.styleNumber), clock(other.styleNumber), positions(nullptr) {
	if (other.positions) {
		const size_t lenData = len + (len / sizeof(XYPOSITION)) + 1;
		positions = std::make_unique<XYPOSITION[]>(lenData);
		memcpy(positions.get(), other.positions.get(), lenData * sizeof(XYPOSITION));
	}
}

void PositionCacheEntry::Set(unsigned int styleNumber_, const char *s_,
	unsigned int len_, const XYPOSITION *positions_, unsigned int clock_) {
	Clear();
	styleNumber = styleNumber_;
	len = len_;
	clock = clock_;
	if (s_ && positions_) {
		positions = std::make_unique<XYPOSITION[]>(len + (len / sizeof(XYPOSITION)) + 1);
		for (unsigned int i=0; i<len; i++) {
			positions[i] = positions_[i];
		}
		memcpy(&positions[len], s_, len);
	}
}

PositionCacheEntry::~PositionCacheEntry() {
	Clear();
}

void PositionCacheEntry::Clear() noexcept {
	positions.reset();
	styleNumber = 0;
	len = 0;
	clock = 0;
}

bool PositionCacheEntry::Retrieve(unsigned int styleNumber_, const char *s_,
	unsigned int len_, XYPOSITION *positions_) const {
	if ((styleNumber == styleNumber_) && (len == len_) &&
		(memcmp(&positions[len], s_, len)== 0)) {
		for (unsigned int i=0; i<len; i++) {
			positions_[i] = positions[i];
		}
		return true;
	} else {
		return false;
	}
}

unsigned int PositionCacheEntry::Hash(unsigned int styleNumber_, const char *s, unsigned int len_) noexcept {
	unsigned int ret = s[0] << 7;
	for (unsigned int i=0; i<len_; i++) {
		ret *= 1000003;
		ret ^= s[i];
	}
	ret *= 1000003;
	ret ^= len_;
	ret *= 1000003;
	ret ^= styleNumber_;
	return ret;
}

bool PositionCacheEntry::NewerThan(const PositionCacheEntry &other) const noexcept {
	return clock > other.clock;
}

void PositionCacheEntry::ResetClock() noexcept {
	if (clock > 0) {
		clock = 1;
	}
}

PositionCache::PositionCache() {
	clock = 1;
	pces.resize(0x400);
	allClear = true;
}

PositionCache::~PositionCache() {
	Clear();
}

void PositionCache::Clear() noexcept {
	if (!allClear) {
		for (PositionCacheEntry &pce : pces) {
			pce.Clear();
		}
	}
	clock = 1;
	allClear = true;
}

void PositionCache::SetSize(size_t size_) {
	Clear();
	pces.resize(size_);
}

void PositionCache::MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
	const char *s, unsigned int len, XYPOSITION *positions, const Document *pdoc) {

	allClear = false;
	size_t probe = pces.size();	// Out of bounds
	if ((!pces.empty()) && (len < 30)) {
		// Only store short strings in the cache so it doesn't churn with
		// long comments with only a single comment.

		// Two way associative: try two probe positions.
		const unsigned int hashValue = PositionCacheEntry::Hash(styleNumber, s, len);
		probe = hashValue % pces.size();
		if (pces[probe].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		const unsigned int probe2 = (hashValue * 37) % pces.size();
		if (pces[probe2].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		// Not found. Choose the oldest of the two slots to replace
		if (pces[probe].NewerThan(pces[probe2])) {
			probe = probe2;
		}
	}
	FontAlias fontStyle = vstyle.styles[styleNumber].font;
	if (len > BreakFinder::lengthStartSubdivision) {
		// Break up into segments
		unsigned int startSegment = 0;
		XYPOSITION xStartSegment = 0;
		while (startSegment < len) {
			const unsigned int lenSegment = pdoc->SafeSegment(s + startSegment, len - startSegment, BreakFinder::lengthEachSubdivision);
			surface->MeasureWidths(fontStyle, std::string_view(s + startSegment, lenSegment), positions + startSegment);
			for (unsigned int inSeg = 0; inSeg < lenSegment; inSeg++) {
				positions[startSegment + inSeg] += xStartSegment;
			}
			xStartSegment = positions[startSegment + lenSegment - 1];
			startSegment += lenSegment;
		}
	} else {
		surface->MeasureWidths(fontStyle, std::string_view(s, len), positions);
	}
	if (probe < pces.size()) {
		// Store into cache
		clock++;
		if (clock > 60000) {
			// Since there are only 16 bits for the clock, wrap it round and
			// reset all cache entries so none get stuck with a high clock.
			for (PositionCacheEntry &pce : pces) {
				pce.ResetClock();
			}
			clock = 2;
		}
		pces[probe].Set(styleNumber, s, len, positions, clock);
	}
}
