// Scintilla source code edit control
/** @file PositionCache.cxx
 ** Classes for caching layout information.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterType.h"
#include "CharacterCategoryMap.h"
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
using namespace Scintilla::Internal;

void BidiData::Resize(size_t maxLineLength_) {
	stylesFonts.resize(maxLineLength_ + 1);
	widthReprs.resize(maxLineLength_ + 1);
}

LineLayout::LineLayout(Sci::Line lineNumber_, int maxLineLength_) :
	lenLineStarts(0),
	lineNumber(lineNumber_),
	maxLineLength(-1),
	numCharsInLine(0),
	numCharsBeforeEOL(0),
	validity(ValidLevel::invalid),
	xHighlightGuide(0),
	highlightColumn(false),
	containsCaret(false),
	edgeColumn(0),
	bracePreviousStyles{},
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

void LineLayout::Invalidate(ValidLevel validity_) noexcept {
	if (validity > validity_)
		validity = validity_;
}

Sci::Line LineLayout::LineNumber() const noexcept {
	return lineNumber;
}

bool LineLayout::CanHold(Sci::Line lineDoc, int lineLength_) const noexcept {
	return (lineNumber == lineDoc) && (lineLength_ <= maxLineLength);
}

int LineLayout::LineStart(int line) const noexcept {
	if (line <= 0) {
		return 0;
	} else if ((line >= lines) || !lineStarts) {
		return numCharsInLine;
	} else {
		return lineStarts[line];
	}
}

int LineLayout::LineLength(int line) const noexcept {
	if (!lineStarts) {
		return numCharsInLine;
	} if (line >= lines - 1) {
		return numCharsInLine - lineStarts[line];
	} else {
		return lineStarts[line + 1] - lineStarts[line];
	}
}

int LineLayout::LineLastVisible(int line, Scope scope) const noexcept {
	if (line < 0) {
		return 0;
	} else if ((line >= lines-1) || !lineStarts) {
		return scope == Scope::visibleOnly ? numCharsBeforeEOL : numCharsInLine;
	} else {
		return lineStarts[line+1];
	}
}

Range LineLayout::SubLineRange(int subLine, Scope scope) const noexcept {
	return Range(LineStart(subLine), LineLastVisible(subLine, scope));
}

bool LineLayout::InLine(int offset, int line) const noexcept {
	return ((offset >= LineStart(line)) && (offset < LineStart(line + 1))) ||
		((offset == numCharsInLine) && (line == (lines-1)));
}

int LineLayout::SubLineFromPosition(int posInLine, PointEnd pe) const noexcept {
	if (!lineStarts || (posInLine > maxLineLength)) {
		return lines - 1;
	}

	for (int line = 0; line < lines; line++) {
		if (FlagSet(pe, PointEnd::subLineEnd)) {
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
		if (lenLineStarts) {
			std::copy(lineStarts.get(), lineStarts.get() + lenLineStarts, newLineStarts.get());
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

int LineLayout::FindBefore(XYPOSITION x, Range range) const noexcept {
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


int LineLayout::FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const noexcept {
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

Point LineLayout::PointFromPosition(int posInLine, int lineHeight, PointEnd pe) const noexcept {
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
				if (FlagSet(pe, PointEnd::subLineEnd))	// Return end of first subline not start of next
					break;
			} else if (FlagSet(pe, PointEnd::lineEnd) && (subLine == (lines-1))) {
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

int LineLayout::EndLineStyle() const noexcept {
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
		[](XYPOSITION w) noexcept { return w > 0.0f; });
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
	return ll->bidiData->stylesFonts[start + position].get();
}

XYPOSITION ScreenLine::RepresentationWidth(size_t position) const {
	return ll->bidiData->widthReprs[start + position];
}

XYPOSITION ScreenLine::TabPositionAfter(XYPOSITION xPosition) const {
	return (std::floor((xPosition + TabWidthMinimumPixels()) / TabWidth()) + 1) * TabWidth();
}

LineLayoutCache::LineLayoutCache() :
	level(LineCache::None),
	allInvalidated(false), styleClock(-1) {
}

LineLayoutCache::~LineLayoutCache() = default;

namespace {

constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
	return ((value - 1) / alignment + 1) * alignment;
}

constexpr size_t alignmentLLC = 20;

constexpr bool GraphicASCII(char ch) noexcept {
	return ch >= ' ' && ch <= '~';
}

bool AllGraphicASCII(std::string_view text) noexcept {
	return std::all_of(text.cbegin(), text.cend(), GraphicASCII);
}

}


size_t LineLayoutCache::EntryForLine(Sci::Line line) const noexcept {
	switch (level) {
	case LineCache::None:
		return 0;
	case LineCache::Caret:
		return 0;
	case LineCache::Page:
		return 1 + (line % (cache.size() - 1));
	case LineCache::Document:
		return line;
	}
	return 0;
}

void LineLayoutCache::AllocateForLevel(Sci::Line linesOnScreen, Sci::Line linesInDoc) {
	size_t lengthForLevel = 0;
	if (level == LineCache::Caret) {
		lengthForLevel = 1;
	} else if (level == LineCache::Page) {
		lengthForLevel = AlignUp(linesOnScreen + 1, alignmentLLC);
	} else if (level == LineCache::Document) {
		lengthForLevel = AlignUp(linesInDoc, alignmentLLC);
	}

	if (lengthForLevel != cache.size()) {
		allInvalidated = false;
		cache.resize(lengthForLevel);
		// Cache::none -> no entries
		// Cache::caret -> 1 entry can take any line
		// Cache::document -> entry per line so each line in correct entry after resize
		if (level == LineCache::Page) {
			// Cache::page -> locates lines in particular entries which may be incorrect after
			// a resize so move them to correct entries.
			for (size_t i = 1; i < cache.size();) {
				size_t increment = 1;
				if (cache[i]) {
					const size_t posForLine = EntryForLine(cache[i]->LineNumber());
					if (posForLine != i) {
						if (cache[posForLine]) {
							if (EntryForLine(cache[posForLine]->LineNumber()) == posForLine) {
								// [posForLine] already holds line that is in correct place
								cache[i].reset();	// This line has nowhere to go so reset it.
							} else {
								std::swap(cache[i], cache[posForLine]);
								increment = 0;
								// Don't increment as newly swapped in value may have to move
							}
						} else {
							cache[posForLine] = std::move(cache[i]);
						}
					}
				}
				i += increment;
			}

#ifdef CHECK_LLC
			for (size_t i = 1; i < cache.size(); i++) {
				if (cache[i]) {
					PLATFORM_ASSERT(EntryForLine(cache[i]->LineNumber()) == i);
				}
			}
#endif
		}
	}
	PLATFORM_ASSERT(cache.size() == lengthForLevel);
}

void LineLayoutCache::Deallocate() noexcept {
	cache.clear();
}

void LineLayoutCache::Invalidate(LineLayout::ValidLevel validity_) noexcept {
	if (!cache.empty() && !allInvalidated) {
		for (const std::shared_ptr<LineLayout> &ll : cache) {
			if (ll) {
				ll->Invalidate(validity_);
			}
		}
		if (validity_ == LineLayout::ValidLevel::invalid) {
			allInvalidated = true;
		}
	}
}

void LineLayoutCache::SetLevel(LineCache level_) noexcept {
	if (level != level_) {
		level = level_;
		allInvalidated = false;
		cache.clear();
	}
}

std::shared_ptr<LineLayout> LineLayoutCache::Retrieve(Sci::Line lineNumber, Sci::Line lineCaret, int maxChars, int styleClock_,
                                      Sci::Line linesOnScreen, Sci::Line linesInDoc) {
	AllocateForLevel(linesOnScreen, linesInDoc);
	if (styleClock != styleClock_) {
		Invalidate(LineLayout::ValidLevel::checkTextAndStyle);
		styleClock = styleClock_;
	}
	allInvalidated = false;
	size_t pos = 0;
	if (level == LineCache::Page) {
		// If first entry is this line then just reuse it.
		if (!(cache[0] && (cache[0]->lineNumber == lineNumber))) {
			const size_t posForLine = EntryForLine(lineNumber);
			if (lineNumber == lineCaret) {
				// Use position 0 for caret line.
				if (cache[0]) {
					// Another line is currently in [0] so move it out to its normal position.
					// Since it was recently the caret line its likely to be needed soon.
					const size_t posNewForEntry0 = EntryForLine(cache[0]->lineNumber);
					if (posForLine == posNewForEntry0) {
						std::swap(cache[0], cache[posNewForEntry0]);
					} else {
						cache[posNewForEntry0] = std::move(cache[0]);
					}
				}
				if (cache[posForLine] && (cache[posForLine]->lineNumber == lineNumber)) {
					// Caret line is currently somewhere else so move it to [0].
					cache[0] = std::move(cache[posForLine]);
				}
			} else {
				pos = posForLine;
			}
		}
	} else if (level == LineCache::Document) {
		pos = lineNumber;
	}

	if (pos < cache.size()) {
		if (cache[pos] && !cache[pos]->CanHold(lineNumber, maxChars)) {
			cache[pos].reset();
		}
		if (!cache[pos]) {
			cache[pos] = std::make_shared<LineLayout>(lineNumber, maxChars);
		}
#ifdef CHECK_LLC
		// Expensive check that there is only one entry for any line number
		std::vector<bool> linesInCache(linesInDoc);
		for (const auto &entry : cache) {
			if (entry) {
				PLATFORM_ASSERT(!linesInCache[entry->LineNumber()]);
				linesInCache[entry->LineNumber()] = true;
			}
		}
#endif
		return cache[pos];
	}

	// Only reach here for level == Cache::none
	return std::make_shared<LineLayout>(lineNumber, maxChars);
}

namespace {

// Simply pack the (maximum 4) character bytes into an int
constexpr unsigned int KeyFromString(std::string_view charBytes) noexcept {
	PLATFORM_ASSERT(charBytes.length() <= 4);
	unsigned int k=0;
	for (const unsigned char uc : charBytes) {
		k = k * 0x100 + uc;
	}
	return k;
}

constexpr unsigned int representationKeyCrLf = KeyFromString("\r\n");

}

void SpecialRepresentations::SetRepresentation(std::string_view charBytes, std::string_view value) {
	if ((charBytes.length() <= 4) && (value.length() <= Representation::maxLength)) {
		const unsigned int key = KeyFromString(charBytes);
		const bool inserted = mapReprs.insert_or_assign(key, Representation(value)).second;
		if (inserted) {
			// New entry so increment for first byte
			const unsigned char ucStart = charBytes.empty() ? 0 : charBytes[0];
			startByteHasReprs[ucStart]++;
			if (key > maxKey) {
				maxKey = key;
			}
			if (key == representationKeyCrLf) {
				crlf = true;
			}
		}
	}
}

void SpecialRepresentations::SetRepresentationAppearance(std::string_view charBytes, RepresentationAppearance appearance) {
	if (charBytes.length() <= 4) {
		const unsigned int key = KeyFromString(charBytes);
		const MapRepresentation::iterator it = mapReprs.find(key);
		if (it == mapReprs.end()) {
			// Not present so fail
			return;
		}
		it->second.appearance = appearance;
	}
}

void SpecialRepresentations::SetRepresentationColour(std::string_view charBytes, ColourRGBA colour) {
	if (charBytes.length() <= 4) {
		const unsigned int key = KeyFromString(charBytes);
		const MapRepresentation::iterator it = mapReprs.find(key);
		if (it == mapReprs.end()) {
			// Not present so fail
			return;
		}
		it->second.appearance = it->second.appearance | RepresentationAppearance::Colour;
		it->second.colour = colour;
	}
}

void SpecialRepresentations::ClearRepresentation(std::string_view charBytes) {
	if (charBytes.length() <= 4) {
		const unsigned int key = KeyFromString(charBytes);
		const MapRepresentation::iterator it = mapReprs.find(key);
		if (it != mapReprs.end()) {
			mapReprs.erase(it);
			const unsigned char ucStart = charBytes.empty() ? 0 : charBytes[0];
			startByteHasReprs[ucStart]--;
			if (key == maxKey && startByteHasReprs[ucStart] == 0) {
				maxKey = mapReprs.empty() ? 0 : mapReprs.crbegin()->first;
			}
			if (key == representationKeyCrLf) {
				crlf = false;
			}
		}
	}
}

const Representation *SpecialRepresentations::GetRepresentation(std::string_view charBytes) const {
	const unsigned int key = KeyFromString(charBytes);
	if (key > maxKey) {
		return nullptr;
	}
	const MapRepresentation::const_iterator it = mapReprs.find(key);
	if (it != mapReprs.end()) {
		return &(it->second);
	}
	return nullptr;
}

const Representation *SpecialRepresentations::RepresentationFromCharacter(std::string_view charBytes) const {
	if (charBytes.length() <= 4) {
		const unsigned char ucStart = charBytes.empty() ? 0 : charBytes[0];
		if (!startByteHasReprs[ucStart])
			return nullptr;
		return GetRepresentation(charBytes);
	}
	return nullptr;
}

void SpecialRepresentations::Clear() {
	mapReprs.clear();
	constexpr unsigned short none = 0;
	std::fill(startByteHasReprs, std::end(startByteHasReprs), none);
	maxKey = 0;
	crlf = false;
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

BreakFinder::BreakFinder(const LineLayout *ll_, const Selection *psel, Range lineRange_, Sci::Position posLineStart,
	XYPOSITION xStart, BreakFor breakFor, const Document *pdoc_, const SpecialRepresentations *preprs_, const ViewStyle *pvsDraw) :
	ll(ll_),
	lineRange(lineRange_),
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
		nextBreak = ll->FindBefore(xStart, lineRange);
	// Now back to a style break
	while ((nextBreak > lineRange.start) && (ll->styles[nextBreak] == ll->styles[nextBreak - 1])) {
		nextBreak--;
	}

	if (FlagSet(breakFor, BreakFor::Selection)) {
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
		// On the curses platform, the terminal is drawing its own caret, so add breaks around the
		// caret in the main selection in order to help prevent the selection from being drawn in
		// the caret's cell.
		if (FlagSet(pvsDraw->caret.style, CaretStyle::Curses) && !psel->RangeMain().Empty()) {
			const Sci::Position caretPos = psel->RangeMain().caret.Position();
			const Sci::Position anchorPos = psel->RangeMain().anchor.Position();
			if (caretPos < anchorPos) {
				const Sci::Position nextPos = pdoc->MovePositionOutsideChar(caretPos + 1, 1);
				Insert(nextPos - posLineStart);
			} else if (caretPos > anchorPos && pvsDraw->DrawCaretInsideSelection(false, false)) {
				const Sci::Position prevPos = pdoc->MovePositionOutsideChar(caretPos - 1, -1);
				if (prevPos > anchorPos)
					Insert(prevPos - posLineStart);
			}
		}
	}
	if (FlagSet(breakFor, BreakFor::Foreground) && pvsDraw->indicatorsSetFore) {
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

BreakFinder::~BreakFinder() noexcept = default;

TextSegment BreakFinder::Next() {
	if (subBreak < 0) {
		const int prev = nextBreak;
		const Representation *repr = nullptr;
		while (nextBreak < lineRange.end) {
			int charWidth = 1;
			const char * const chars = &ll->chars[nextBreak];
			const unsigned char ch = chars[0];
			if (!UTF8IsAscii(ch) && encodingFamily != EncodingFamily::eightBit) {
				if (encodingFamily == EncodingFamily::unicode) {
					charWidth = UTF8DrawBytes(reinterpret_cast<const unsigned char *>(chars), static_cast<int>(lineRange.end - nextBreak));
				} else {
					charWidth = pdoc->DBCSDrawBytes(std::string_view(chars, lineRange.end - nextBreak));
				}
			}
			repr = nullptr;
			if (preprs->MayContain(ch)) {
				// Special case \r\n line ends if there is a representation
				if (ch == '\r' && preprs->ContainsCrLf() && chars[1] == '\n') {
					charWidth = 2;
				}
				repr = preprs->GetRepresentation(std::string_view(chars, charWidth));
			}
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
					break;
				}
			}
			nextBreak += charWidth;
		}

		const int lengthSegment = nextBreak - prev;
		if (lengthSegment < lengthStartSubdivision) {
			return TextSegment(prev, lengthSegment, repr);
		}
		subBreak = prev;
	}

	// Splitting up a long run from prev to nextBreak in lots of approximately lengthEachSubdivision.
	const int startSegment = subBreak;
	const int remaining = nextBreak - startSegment;
	int lengthSegment = remaining;
	if (lengthSegment > lengthEachSubdivision) {
		lengthSegment = static_cast<int>(pdoc->SafeSegment(std::string_view(&ll->chars[startSegment], lengthEachSubdivision)));
	}
	if (lengthSegment < remaining) {
		subBreak += lengthSegment;
	} else {
		subBreak = -1;
	}
	return TextSegment(startSegment, lengthSegment);
}

bool BreakFinder::More() const noexcept {
	return (subBreak >= 0) || (nextBreak < lineRange.end);
}

class PositionCacheEntry {
	uint16_t styleNumber;
	uint16_t len;
	uint16_t clock;
	std::unique_ptr<XYPOSITION[]> positions;
public:
	PositionCacheEntry() noexcept;
	// Copy constructor not currently used, but needed for being element in std::vector.
	PositionCacheEntry(const PositionCacheEntry &);
	PositionCacheEntry(PositionCacheEntry &&) noexcept = default;
	// Deleted so PositionCacheEntry objects can not be assigned.
	void operator=(const PositionCacheEntry &) = delete;
	void operator=(PositionCacheEntry &&) = delete;
	~PositionCacheEntry();
	void Set(unsigned int styleNumber_, std::string_view sv, const XYPOSITION *positions_, uint16_t clock_);
	void Clear() noexcept;
	bool Retrieve(unsigned int styleNumber_, std::string_view sv, XYPOSITION *positions_) const noexcept;
	static size_t Hash(unsigned int styleNumber_, std::string_view sv) noexcept;
	bool NewerThan(const PositionCacheEntry &other) const noexcept;
	void ResetClock() noexcept;
};

class PositionCache : public IPositionCache {
	std::vector<PositionCacheEntry> pces;
	std::mutex mutex;
	uint16_t clock;
	bool allClear;
public:
	PositionCache();
	// Deleted so LineAnnotation objects can not be copied.
	PositionCache(const PositionCache &) = delete;
	PositionCache(PositionCache &&) = delete;
	void operator=(const PositionCache &) = delete;
	void operator=(PositionCache &&) = delete;
	~PositionCache() override = default;

	void Clear() noexcept override;
	void SetSize(size_t size_) override;
	size_t GetSize() const noexcept override;
	void MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
		std::string_view sv, XYPOSITION *positions, bool needsLocking) override;
};

PositionCacheEntry::PositionCacheEntry() noexcept :
	styleNumber(0), len(0), clock(0) {
}

// Copy constructor not currently used, but needed for being element in std::vector.
PositionCacheEntry::PositionCacheEntry(const PositionCacheEntry &other) :
	styleNumber(other.styleNumber), len(other.len), clock(other.clock) {
	if (other.positions) {
		const size_t lenData = len + (len / sizeof(XYPOSITION)) + 1;
		positions = std::make_unique<XYPOSITION[]>(lenData);
		memcpy(positions.get(), other.positions.get(), lenData * sizeof(XYPOSITION));
	}
}

void PositionCacheEntry::Set(unsigned int styleNumber_, std::string_view sv,
	const XYPOSITION *positions_, uint16_t clock_) {
	Clear();
	styleNumber = static_cast<uint16_t>(styleNumber_);
	len = static_cast<uint16_t>(sv.length());
	clock = clock_;
	if (sv.data() && positions_) {
		positions = std::make_unique<XYPOSITION[]>(len + (len / sizeof(XYPOSITION)) + 1);
		for (unsigned int i=0; i<len; i++) {
			positions[i] = positions_[i];
		}
		memcpy(&positions[len], sv.data(), sv.length());
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

bool PositionCacheEntry::Retrieve(unsigned int styleNumber_, std::string_view sv, XYPOSITION *positions_) const noexcept {
	if ((styleNumber == styleNumber_) && (len == sv.length()) &&
		(memcmp(&positions[len], sv.data(), sv.length())== 0)) {
		for (unsigned int i=0; i<len; i++) {
			positions_[i] = positions[i];
		}
		return true;
	} else {
		return false;
	}
}

size_t PositionCacheEntry::Hash(unsigned int styleNumber_, std::string_view sv) noexcept {
	const size_t h1 = std::hash<std::string_view>{}(sv);
	const size_t h2 = std::hash<unsigned int>{}(styleNumber_);
	return h1 ^ (h2 << 1);
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

size_t PositionCache::GetSize() const noexcept {
	return pces.size();
}

void PositionCache::MeasureWidths(Surface *surface, const ViewStyle &vstyle, unsigned int styleNumber,
	std::string_view sv, XYPOSITION *positions, bool needsLocking) {
	const Style &style = vstyle.styles[styleNumber];
	if (style.monospaceASCII) {
		if (AllGraphicASCII(sv)) {
			const XYPOSITION monospaceCharacterWidth = style.monospaceCharacterWidth;
			for (size_t i = 0; i < sv.length(); i++) {
				positions[i] = monospaceCharacterWidth * (i+1);
			}
			return;
		}
	}

	size_t probe = pces.size();	// Out of bounds
	if ((!pces.empty()) && (sv.length() < 30)) {
		// Only store short strings in the cache so it doesn't churn with
		// long comments with only a single comment.

		// Two way associative: try two probe positions.
		const size_t hashValue = PositionCacheEntry::Hash(styleNumber, sv);
		probe = hashValue % pces.size();
		std::unique_lock<std::mutex> guard(mutex, std::defer_lock);
		if (needsLocking) {
			guard.lock();
		}
		if (pces[probe].Retrieve(styleNumber, sv, positions)) {
			return;
		}
		const size_t probe2 = (hashValue * 37) % pces.size();
		if (pces[probe2].Retrieve(styleNumber, sv, positions)) {
			return;
		}
		// Not found. Choose the oldest of the two slots to replace
		if (pces[probe].NewerThan(pces[probe2])) {
			probe = probe2;
		}
	}

	const Font *fontStyle = style.font.get();
	surface->MeasureWidths(fontStyle, sv, positions);
	if (probe < pces.size()) {
		// Store into cache
		std::unique_lock<std::mutex> guard(mutex, std::defer_lock);
		if (needsLocking) {
			guard.lock();
		}
		clock++;
		if (clock > 60000) {
			// Since there are only 16 bits for the clock, wrap it round and
			// reset all cache entries so none get stuck with a high clock.
			for (PositionCacheEntry &pce : pces) {
				pce.ResetClock();
			}
			clock = 2;
		}
		allClear = false;
		pces[probe].Set(styleNumber, sv, positions, clock);
	}
}

std::unique_ptr<IPositionCache> Scintilla::Internal::CreatePositionCache() {
	return std::make_unique<PositionCache>();
}
