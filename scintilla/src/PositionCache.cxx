// Scintilla source code edit control
/** @file PositionCache.cxx
 ** Classes for caching layout information.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <string>
#include <vector>
#include <map>

#include "Platform.h"

#include "Scintilla.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "ILexer.h"
#include "CaseFolder.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsControlCharacter(int ch) {
	// iscntrl returns true for lots of chars > 127 which are displayable
	return ch >= 0 && ch < ' ';
}

LineLayout::LineLayout(int maxLineLength_) :
	lineStarts(0),
	lenLineStarts(0),
	lineNumber(-1),
	inCache(false),
	maxLineLength(-1),
	numCharsInLine(0),
	numCharsBeforeEOL(0),
	validity(llInvalid),
	xHighlightGuide(0),
	highlightColumn(0),
	psel(NULL),
	containsCaret(false),
	edgeColumn(0),
	chars(0),
	styles(0),
	styleBitsSet(0),
	indicators(0),
	positions(0),
	hsStart(0),
	hsEnd(0),
	widthLine(wrapWidthInfinite),
	lines(1),
	wrapIndent(0) {
	bracePreviousStyles[0] = 0;
	bracePreviousStyles[1] = 0;
	Resize(maxLineLength_);
}

LineLayout::~LineLayout() {
	Free();
}

void LineLayout::Resize(int maxLineLength_) {
	if (maxLineLength_ > maxLineLength) {
		Free();
		chars = new char[maxLineLength_ + 1];
		styles = new unsigned char[maxLineLength_ + 1];
		indicators = new char[maxLineLength_ + 1];
		// Extra position allocated as sometimes the Windows
		// GetTextExtentExPoint API writes an extra element.
		positions = new XYPOSITION[maxLineLength_ + 1 + 1];
		maxLineLength = maxLineLength_;
	}
}

void LineLayout::Free() {
	delete []chars;
	chars = 0;
	delete []styles;
	styles = 0;
	delete []indicators;
	indicators = 0;
	delete []positions;
	positions = 0;
	delete []lineStarts;
	lineStarts = 0;
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

int LineLayout::LineLastVisible(int line) const {
	if (line < 0) {
		return 0;
	} else if ((line >= lines-1) || !lineStarts) {
		return numCharsBeforeEOL;
	} else {
		return lineStarts[line+1];
	}
}

bool LineLayout::InLine(int offset, int line) const {
	return ((offset >= LineStart(line)) && (offset < LineStart(line + 1))) ||
		((offset == numCharsInLine) && (line == (lines-1)));
}

void LineLayout::SetLineStart(int line, int start) {
	if ((line >= lenLineStarts) && (line != 0)) {
		int newMaxLines = line + 20;
		int *newLineStarts = new int[newMaxLines];
		for (int i = 0; i < newMaxLines; i++) {
			if (i < lenLineStarts)
				newLineStarts[i] = lineStarts[i];
			else
				newLineStarts[i] = 0;
		}
		delete []lineStarts;
		lineStarts = newLineStarts;
		lenLineStarts = newMaxLines;
	}
	lineStarts[line] = start;
}

void LineLayout::SetBracesHighlight(Range rangeLine, Position braces[],
                                    char bracesMatchStyle, int xHighlight, bool ignoreStyle) {
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[0] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[1])) {
		int braceOffset = braces[1] - rangeLine.start;
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

void LineLayout::RestoreBracesHighlight(Range rangeLine, Position braces[], bool ignoreStyle) {
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[0];
		}
	}
	if (!ignoreStyle && rangeLine.ContainsCharacter(braces[1])) {
		int braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[1];
		}
	}
	xHighlightGuide = 0;
}

int LineLayout::FindBefore(XYPOSITION x, int lower, int upper) const {
	do {
		int middle = (upper + lower + 1) / 2; 	// Round high
		XYPOSITION posMiddle = positions[middle];
		if (x < posMiddle) {
			upper = middle - 1;
		} else {
			lower = middle;
		}
	} while (lower < upper);
	return lower;
}

int LineLayout::EndLineStyle() const {
	return styles[numCharsBeforeEOL > 0 ? numCharsBeforeEOL-1 : 0];
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

void LineLayoutCache::AllocateForLevel(int linesOnScreen, int linesInDoc) {
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
				delete cache[i];
				cache[i] = 0;
			}
		}
		cache.resize(lengthForLevel);
	}
	PLATFORM_ASSERT(cache.size() == lengthForLevel);
}

void LineLayoutCache::Deallocate() {
	PLATFORM_ASSERT(useCount == 0);
	for (size_t i = 0; i < cache.size(); i++)
		delete cache[i];
	cache.clear();
}

void LineLayoutCache::Invalidate(LineLayout::validLevel validity_) {
	if (!cache.empty() && !allInvalidated) {
		for (size_t i = 0; i < cache.size(); i++) {
			if (cache[i]) {
				cache[i]->Invalidate(validity_);
			}
		}
		if (validity_ == LineLayout::llInvalid) {
			allInvalidated = true;
		}
	}
}

void LineLayoutCache::SetLevel(int level_) {
	allInvalidated = false;
	if ((level_ != -1) && (level != level_)) {
		level = level_;
		Deallocate();
	}
}

LineLayout *LineLayoutCache::Retrieve(int lineNumber, int lineCaret, int maxChars, int styleClock_,
                                      int linesOnScreen, int linesInDoc) {
	AllocateForLevel(linesOnScreen, linesInDoc);
	if (styleClock != styleClock_) {
		Invalidate(LineLayout::llCheckTextAndStyle);
		styleClock = styleClock_;
	}
	allInvalidated = false;
	int pos = -1;
	LineLayout *ret = 0;
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
					delete cache[pos];
					cache[pos] = 0;
				}
			}
			if (!cache[pos]) {
				cache[pos] = new LineLayout(maxChars);
			}
			cache[pos]->lineNumber = lineNumber;
			cache[pos]->inCache = true;
			ret = cache[pos];
			useCount++;
		}
	}

	if (!ret) {
		ret = new LineLayout(maxChars);
		ret->lineNumber = lineNumber;
	}

	return ret;
}

void LineLayoutCache::Dispose(LineLayout *ll) {
	allInvalidated = false;
	if (ll) {
		if (!ll->inCache) {
			delete ll;
		} else {
			useCount--;
		}
	}
}

void BreakFinder::Insert(int val) {
	if (val >= nextBreak) {
		for (std::vector<int>::iterator it = selAndEdge.begin(); it != selAndEdge.end(); ++it) {
			if (val == *it) {
				return;
			}
			if (val <*it) {
				selAndEdge.insert(it, 1, val);
				return;
			}
		}
		// Not less than any so append
		selAndEdge.push_back(val);
	}
}

extern bool BadUTF(const char *s, int len, int &trailBytes);

static int NextBadU(const char *s, int p, int len, int &trailBytes) {
	while (p < len) {
		p++;
		if (BadUTF(s + p, len - p, trailBytes))
			return p;
	}
	return -1;
}

BreakFinder::BreakFinder(LineLayout *ll_, int lineStart_, int lineEnd_, int posLineStart_,
	int xStart, bool breakForSelection, Document *pdoc_) :
	ll(ll_),
	lineStart(lineStart_),
	lineEnd(lineEnd_),
	posLineStart(posLineStart_),
	nextBreak(lineStart_),
	saeCurrentPos(0),
	saeNext(0),
	subBreak(-1),
	pdoc(pdoc_) {

	// Search for first visible break
	// First find the first visible character
	nextBreak = ll->FindBefore(xStart, lineStart, lineEnd);
	// Now back to a style break
	while ((nextBreak > lineStart) && (ll->styles[nextBreak] == ll->styles[nextBreak - 1])) {
		nextBreak--;
	}

	if (breakForSelection) {
		SelectionPosition posStart(posLineStart);
		SelectionPosition posEnd(posLineStart + lineEnd);
		SelectionSegment segmentLine(posStart, posEnd);
		for (size_t r=0; r<ll->psel->Count(); r++) {
			SelectionSegment portion = ll->psel->Range(r).Intersect(segmentLine);
			if (!(portion.start == portion.end)) {
				if (portion.start.IsValid())
					Insert(portion.start.Position() - posLineStart - 1);
				if (portion.end.IsValid())
					Insert(portion.end.Position() - posLineStart - 1);
			}
		}
	}

	Insert(ll->edgeColumn - 1);
	Insert(lineEnd - 1);

	if (pdoc && (SC_CP_UTF8 == pdoc->dbcsCodePage)) {
		int trailBytes=0;
		for (int pos = -1;;) {
			pos = NextBadU(ll->chars, pos, lineEnd, trailBytes);
			if (pos < 0)
				break;
			Insert(pos-1);
			Insert(pos);
		}
	}
	saeNext = (!selAndEdge.empty()) ? selAndEdge[0] : -1;
}

BreakFinder::~BreakFinder() {
}

int BreakFinder::First() const {
	return nextBreak;
}

int BreakFinder::Next() {
	if (subBreak == -1) {
		int prev = nextBreak;
		while (nextBreak < lineEnd) {
			if ((ll->styles[nextBreak] != ll->styles[nextBreak + 1]) ||
					(nextBreak == saeNext) ||
					IsControlCharacter(ll->chars[nextBreak]) || IsControlCharacter(ll->chars[nextBreak + 1])) {
				if (nextBreak == saeNext) {
					saeCurrentPos++;
					saeNext = (saeCurrentPos < selAndEdge.size()) ? selAndEdge[saeCurrentPos] : -1;
				}
				nextBreak++;
				if ((nextBreak - prev) < lengthStartSubdivision) {
					return nextBreak;
				}
				break;
			}
			nextBreak++;
		}
		if ((nextBreak - prev) < lengthStartSubdivision) {
			return nextBreak;
		}
		subBreak = prev;
	}
	// Splitting up a long run from prev to nextBreak in lots of approximately lengthEachSubdivision.
	// For very long runs add extra breaks after spaces or if no spaces before low punctuation.
	if ((nextBreak - subBreak) <= lengthEachSubdivision) {
		subBreak = -1;
		return nextBreak;
	} else {
		subBreak += pdoc->SafeSegment(ll->chars + subBreak, nextBreak-subBreak, lengthEachSubdivision);
		if (subBreak >= nextBreak) {
			subBreak = -1;
			return nextBreak;
		} else {
			return subBreak;
		}
	}
}

PositionCacheEntry::PositionCacheEntry() :
	styleNumber(0), len(0), clock(0), positions(0) {
}

void PositionCacheEntry::Set(unsigned int styleNumber_, const char *s_,
	unsigned int len_, XYPOSITION *positions_, unsigned int clock_) {
	Clear();
	styleNumber = styleNumber_;
	len = len_;
	clock = clock_;
	if (s_ && positions_) {
		positions = new XYPOSITION[len + (len / 4) + 1];
		for (unsigned int i=0; i<len; i++) {
			positions[i] = static_cast<XYPOSITION>(positions_[i]);
		}
		memcpy(reinterpret_cast<char *>(positions + len), s_, len);
	}
}

PositionCacheEntry::~PositionCacheEntry() {
	Clear();
}

void PositionCacheEntry::Clear() {
	delete []positions;
	positions = 0;
	styleNumber = 0;
	len = 0;
	clock = 0;
}

bool PositionCacheEntry::Retrieve(unsigned int styleNumber_, const char *s_,
	unsigned int len_, XYPOSITION *positions_) const {
	if ((styleNumber == styleNumber_) && (len == len_) &&
		(memcmp(reinterpret_cast<char *>(positions + len), s_, len)== 0)) {
		for (unsigned int i=0; i<len; i++) {
			positions_[i] = positions[i];
		}
		return true;
	} else {
		return false;
	}
}

int PositionCacheEntry::Hash(unsigned int styleNumber_, const char *s, unsigned int len_) {
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

bool PositionCacheEntry::NewerThan(const PositionCacheEntry &other) const {
	return clock > other.clock;
}

void PositionCacheEntry::ResetClock() {
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

void PositionCache::Clear() {
	if (!allClear) {
		for (size_t i=0; i<pces.size(); i++) {
			pces[i].Clear();
		}
	}
	clock = 1;
	allClear = true;
}

void PositionCache::SetSize(size_t size_) {
	Clear();
	pces.resize(size_);
}

void PositionCache::MeasureWidths(Surface *surface, ViewStyle &vstyle, unsigned int styleNumber,
	const char *s, unsigned int len, XYPOSITION *positions, Document *pdoc) {

	allClear = false;
	int probe = -1;
	if ((!pces.empty()) && (len < 30)) {
		// Only store short strings in the cache so it doesn't churn with
		// long comments with only a single comment.

		// Two way associative: try two probe positions.
		int hashValue = PositionCacheEntry::Hash(styleNumber, s, len);
		probe = static_cast<int>(hashValue % pces.size());
		if (pces[probe].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		int probe2 = static_cast<int>((hashValue * 37) % pces.size());
		if (pces[probe2].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		// Not found. Choose the oldest of the two slots to replace
		if (pces[probe].NewerThan(pces[probe2])) {
			probe = probe2;
		}
	}
	if (len > BreakFinder::lengthStartSubdivision) {
		// Break up into segments
		unsigned int startSegment = 0;
		XYPOSITION xStartSegment = 0;
		while (startSegment < len) {
			unsigned int lenSegment = pdoc->SafeSegment(s + startSegment, len - startSegment, BreakFinder::lengthEachSubdivision);
			surface->MeasureWidths(vstyle.styles[styleNumber].font, s + startSegment, lenSegment, positions + startSegment);
			for (unsigned int inSeg = 0; inSeg < lenSegment; inSeg++) {
				positions[startSegment + inSeg] += xStartSegment;
			}
			xStartSegment = positions[startSegment + lenSegment - 1];
			startSegment += lenSegment;
		}
	} else {
		surface->MeasureWidths(vstyle.styles[styleNumber].font, s, len, positions);
	}
	if (probe >= 0) {
		clock++;
		if (clock > 60000) {
			// Since there are only 16 bits for the clock, wrap it round and
			// reset all cache entries so none get stuck with a high clock.
			for (size_t i=0; i<pces.size(); i++) {
				pces[i].ResetClock();
			}
			clock = 2;
		}
		pces[probe].Set(styleNumber, s, len, positions, clock);
	}
}
