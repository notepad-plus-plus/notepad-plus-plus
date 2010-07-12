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
		positions = new int[maxLineLength_ + 1 + 1];
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
                                    char bracesMatchStyle, int xHighlight) {
	if (rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[0] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if (rangeLine.ContainsCharacter(braces[1])) {
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

void LineLayout::RestoreBracesHighlight(Range rangeLine, Position braces[]) {
	if (rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[0];
		}
	}
	if (rangeLine.ContainsCharacter(braces[1])) {
		int braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[1];
		}
	}
	xHighlightGuide = 0;
}

int LineLayout::FindBefore(int x, int lower, int upper) const {
	do {
		int middle = (upper + lower + 1) / 2; 	// Round high
		int posMiddle = positions[middle];
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
	level(0), length(0), size(0), cache(0),
	allInvalidated(false), styleClock(-1), useCount(0) {
	Allocate(0);
}

LineLayoutCache::~LineLayoutCache() {
	Deallocate();
}

void LineLayoutCache::Allocate(int length_) {
	PLATFORM_ASSERT(cache == NULL);
	allInvalidated = false;
	length = length_;
	size = length;
	if (size > 1) {
		size = (size / 16 + 1) * 16;
	}
	if (size > 0) {
		cache = new LineLayout * [size];
	}
	for (int i = 0; i < size; i++)
		cache[i] = 0;
}

void LineLayoutCache::AllocateForLevel(int linesOnScreen, int linesInDoc) {
	PLATFORM_ASSERT(useCount == 0);
	int lengthForLevel = 0;
	if (level == llcCaret) {
		lengthForLevel = 1;
	} else if (level == llcPage) {
		lengthForLevel = linesOnScreen + 1;
	} else if (level == llcDocument) {
		lengthForLevel = linesInDoc;
	}
	if (lengthForLevel > size) {
		Deallocate();
		Allocate(lengthForLevel);
	} else {
		if (lengthForLevel < length) {
			for (int i = lengthForLevel; i < length; i++) {
				delete cache[i];
				cache[i] = 0;
			}
		}
		length = lengthForLevel;
	}
	PLATFORM_ASSERT(length == lengthForLevel);
	PLATFORM_ASSERT(cache != NULL || length == 0);
}

void LineLayoutCache::Deallocate() {
	PLATFORM_ASSERT(useCount == 0);
	for (int i = 0; i < length; i++)
		delete cache[i];
	delete []cache;
	cache = 0;
	length = 0;
	size = 0;
}

void LineLayoutCache::Invalidate(LineLayout::validLevel validity_) {
	if (cache && !allInvalidated) {
		for (int i = 0; i < length; i++) {
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
		} else if (length > 1) {
			pos = 1 + (lineNumber % (length - 1));
		}
	} else if (level == llcDocument) {
		pos = lineNumber;
	}
	if (pos >= 0) {
		PLATFORM_ASSERT(useCount == 0);
		if (cache && (pos < length)) {
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
			if (cache[pos]) {
				cache[pos]->lineNumber = lineNumber;
				cache[pos]->inCache = true;
				ret = cache[pos];
				useCount++;
			}
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
	// Expand if needed
	if (saeLen >= saeSize) {
		saeSize *= 2;
		int *selAndEdgeNew = new int[saeSize];
		for (unsigned int j = 0; j<saeLen; j++) {
			selAndEdgeNew[j] = selAndEdge[j];
		}
		delete []selAndEdge;
		selAndEdge = selAndEdgeNew;
	}

	if (val >= nextBreak) {
		for (unsigned int j = 0; j<saeLen; j++) {
			if (val == selAndEdge[j]) {
				return;
			}
			if (val < selAndEdge[j]) {
				for (unsigned int k = saeLen; k>j; k--) {
					selAndEdge[k] = selAndEdge[k-1];
				}
				saeLen++;
				selAndEdge[j] = val;
				return;
			}
		}
		// Not less than any so append
		selAndEdge[saeLen++] = val;
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

BreakFinder::BreakFinder(LineLayout *ll_, int lineStart_, int lineEnd_, int posLineStart_, bool utf8_, int xStart, bool breakForSelection) :
	ll(ll_),
	lineStart(lineStart_),
	lineEnd(lineEnd_),
	posLineStart(posLineStart_),
	utf8(utf8_),
	nextBreak(lineStart_),
	saeSize(0),
	saeLen(0),
	saeCurrentPos(0),
	saeNext(0),
	subBreak(-1) {
	saeSize = 8;
	selAndEdge = new int[saeSize];
	for (unsigned int j=0; j < saeSize; j++) {
		selAndEdge[j] = 0;
	}

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

	if (utf8) {
		int trailBytes=0;
		for (int pos = -1;;) {
			pos = NextBadU(ll->chars, pos, lineEnd, trailBytes);
			if (pos < 0)
				break;
			Insert(pos-1);
			Insert(pos);
		}
	}
	saeNext = (saeLen > 0) ? selAndEdge[0] : -1;
}

BreakFinder::~BreakFinder() {
	delete []selAndEdge;
}

int BreakFinder::First() const {
	return nextBreak;
}

static bool IsTrailByte(int ch) {
	return (ch >= 0x80) && (ch < (0x80 + 0x40));
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
					saeNext = (saeLen > saeCurrentPos) ? selAndEdge[saeCurrentPos] : -1;
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
		int lastGoodBreak = -1;
		int lastOKBreak = -1;
		int lastUTF8Break = -1;
		int j;
		for (j = subBreak + 1; j <= nextBreak; j++) {
			if (IsSpaceOrTab(ll->chars[j - 1]) && !IsSpaceOrTab(ll->chars[j])) {
				lastGoodBreak = j;
			}
			if (static_cast<unsigned char>(ll->chars[j]) < 'A') {
				lastOKBreak = j;
			}
			if (utf8 && !IsTrailByte(static_cast<unsigned char>(ll->chars[j]))) {
				lastUTF8Break = j;
			}
			if (((j - subBreak) >= lengthEachSubdivision) &&
				((lastGoodBreak >= 0) || (lastOKBreak >= 0) || (lastUTF8Break >= 0))) {
				break;
			}
		}
		if (lastGoodBreak >= 0) {
			subBreak = lastGoodBreak;
		} else if (lastOKBreak >= 0) {
			subBreak = lastOKBreak;
		} else if (lastUTF8Break >= 0) {
			subBreak = lastUTF8Break;
		} else {
			subBreak = nextBreak;
		}
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
	unsigned int len_, int *positions_, unsigned int clock_) {
	Clear();
	styleNumber = styleNumber_;
	len = len_;
	clock = clock_;
	if (s_ && positions_) {
		positions = new short[len + (len + 1) / 2];
		for (unsigned int i=0; i<len; i++) {
			positions[i] = static_cast<short>(positions_[i]);
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
	unsigned int len_, int *positions_) const {
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

int PositionCacheEntry::Hash(unsigned int styleNumber, const char *s, unsigned int len) {
	unsigned int ret = s[0] << 7;
	for (unsigned int i=0; i<len; i++) {
		ret *= 1000003;
		ret ^= s[i];
	}
	ret *= 1000003;
	ret ^= len;
	ret *= 1000003;
	ret ^= styleNumber;
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
	size = 0x400;
	clock = 1;
	pces = new PositionCacheEntry[size];
	allClear = true;
}

PositionCache::~PositionCache() {
	Clear();
	delete []pces;
}

void PositionCache::Clear() {
	if (!allClear) {
		for (size_t i=0; i<size; i++) {
			pces[i].Clear();
		}
	}
	clock = 1;
	allClear = true;
}

void PositionCache::SetSize(size_t size_) {
	Clear();
	delete []pces;
	size = size_;
	pces = new PositionCacheEntry[size];
}

void PositionCache::MeasureWidths(Surface *surface, ViewStyle &vstyle, unsigned int styleNumber,
	const char *s, unsigned int len, int *positions) {
	allClear = false;
	int probe = -1;
	if ((size > 0) && (len < 30)) {
		// Only store short strings in the cache so it doesn't churn with
		// long comments with only a single comment.

		// Two way associative: try two probe positions.
		int hashValue = PositionCacheEntry::Hash(styleNumber, s, len);
		probe = hashValue % size;
		if (pces[probe].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		int probe2 = (hashValue * 37) % size;
		if (pces[probe2].Retrieve(styleNumber, s, len, positions)) {
			return;
		}
		// Not found. Choose the oldest of the two slots to replace
		if (pces[probe].NewerThan(pces[probe2])) {
			probe = probe2;
		}
	}
	surface->MeasureWidths(vstyle.styles[styleNumber].font, s, len, positions);
	if (probe >= 0) {
		clock++;
		if (clock > 60000) {
			// Since there are only 16 bits for the clock, wrap it round and
			// reset all cache entries so none get stuck with a high clock.
			for (size_t i=0; i<size; i++) {
				pces[i].ResetClock();
			}
			clock = 2;
		}
		pces[probe].Set(styleNumber, s, len, positions, clock);
	}
}
