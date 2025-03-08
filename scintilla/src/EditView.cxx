// Scintilla source code edit control
/** @file EditView.cxx
 ** Defines the appearance of the main text area of the editor window.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <forward_list>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <future>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
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
#include "PerLine.h"
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
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "ElapsedPeriod.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

PrintParameters::PrintParameters() noexcept {
	magnification = 0;
	colourMode = PrintOption::Normal;
	wrapState = Wrap::Word;
}

namespace {

int WidthStyledText(Surface *surface, const ViewStyle &vs, int styleOffset,
	const char *text, const unsigned char *styles, size_t len) {
	int width = 0;
	size_t start = 0;
	while (start < len) {
		const unsigned char style = styles[start];
		size_t endSegment = start;
		while ((endSegment + 1 < len) && (styles[endSegment + 1] == style))
			endSegment++;
		const Font *fontText = vs.styles[style + styleOffset].font.get();
		const std::string_view sv(text + start, endSegment - start + 1);
		width += static_cast<int>(surface->WidthText(fontText, sv));
		start = endSegment + 1;
	}
	return width;
}

}

namespace Scintilla::Internal {

bool ValidStyledText(const ViewStyle &vs, size_t styleOffset, const StyledText &st) noexcept {
	if (st.multipleStyles) {
		for (size_t iStyle = 0; iStyle<st.length; iStyle++) {
			if (!vs.ValidStyle(styleOffset + st.styles[iStyle]))
				return false;
		}
	} else {
		if (!vs.ValidStyle(styleOffset + st.style))
			return false;
	}
	return true;
}

int WidestLineWidth(Surface *surface, const ViewStyle &vs, int styleOffset, const StyledText &st) {
	int widthMax = 0;
	size_t start = 0;
	while (start < st.length) {
		const size_t lenLine = st.LineLength(start);
		int widthSubLine;
		if (st.multipleStyles) {
			widthSubLine = WidthStyledText(surface, vs, styleOffset, st.text + start, st.styles + start, lenLine);
		} else {
			const Font *fontText = vs.styles[styleOffset + st.style].font.get();
			const std::string_view text(st.text + start, lenLine);
			widthSubLine = static_cast<int>(surface->WidthText(fontText, text));
		}
		if (widthSubLine > widthMax)
			widthMax = widthSubLine;
		start += lenLine + 1;
	}
	return widthMax;
}

void DrawTextNoClipPhase(Surface *surface, PRectangle rc, const Style &style, XYPOSITION ybase,
	std::string_view text, DrawPhase phase) {
	const Font *fontText = style.font.get();
	if (FlagSet(phase, DrawPhase::back)) {
		if (FlagSet(phase, DrawPhase::text)) {
			// Drawing both
			surface->DrawTextNoClip(rc, fontText, ybase, text,
				style.fore, style.back);
		} else {
			surface->FillRectangleAligned(rc, Fill(style.back));
		}
	} else if (FlagSet(phase, DrawPhase::text)) {
		surface->DrawTextTransparent(rc, fontText, ybase, text, style.fore);
	}
}

void DrawStyledText(Surface *surface, const ViewStyle &vs, int styleOffset, PRectangle rcText,
	const StyledText &st, size_t start, size_t length, DrawPhase phase) {

	if (st.multipleStyles) {
		int x = static_cast<int>(rcText.left);
		size_t i = 0;
		while (i < length) {
			size_t end = i;
			size_t style = st.styles[i + start];
			while (end < length - 1 && st.styles[start + end + 1] == style)
				end++;
			style += styleOffset;
			const Font *fontText = vs.styles[style].font.get();
			const std::string_view text(st.text + start + i, end - i + 1);
			const int width = static_cast<int>(surface->WidthText(fontText, text));
			PRectangle rcSegment = rcText;
			rcSegment.left = static_cast<XYPOSITION>(x);
			rcSegment.right = static_cast<XYPOSITION>(x + width + 1);
			DrawTextNoClipPhase(surface, rcSegment, vs.styles[style],
				rcText.top + vs.maxAscent, text, phase);
			x += width;
			i = end + 1;
		}
	} else {
		const size_t style = st.style + styleOffset;
		DrawTextNoClipPhase(surface, rcText, vs.styles[style],
			rcText.top + vs.maxAscent,
			std::string_view(st.text + start, length), phase);
	}
}

}

EditView::EditView() {
	tabWidthMinimumPixels = 2; // needed for calculating tab stops for fractional proportional fonts
	drawOverstrikeCaret = true;
	bufferedDraw = true;
	phasesDraw = PhasesDraw::Two;
	lineWidthMaxSeen = 0;
	additionalCaretsBlink = true;
	additionalCaretsVisible = true;
	imeCaretBlockOverride = false;
	llc.SetLevel(LineCache::Caret);
	posCache = CreatePositionCache();
	posCache->SetSize(0x400);
	maxLayoutThreads = 1;
	tabArrowHeight = 4;
	customDrawTabArrow = nullptr;
	customDrawWrapMarker = nullptr;
}

EditView::~EditView() = default;

bool EditView::SetTwoPhaseDraw(bool twoPhaseDraw) noexcept {
	const PhasesDraw phasesDrawNew = twoPhaseDraw ? PhasesDraw::Two : PhasesDraw::One;
	const bool redraw = phasesDraw != phasesDrawNew;
	phasesDraw = phasesDrawNew;
	return redraw;
}

bool EditView::SetPhasesDraw(int phases) noexcept {
	const PhasesDraw phasesDrawNew = static_cast<PhasesDraw>(phases);
	const bool redraw = phasesDraw != phasesDrawNew;
	phasesDraw = phasesDrawNew;
	return redraw;
}

bool EditView::LinesOverlap() const noexcept {
	return phasesDraw == PhasesDraw::Multiple;
}

void EditView::SetLayoutThreads(unsigned int threads) noexcept {
	maxLayoutThreads = std::clamp(threads, 1U, std::thread::hardware_concurrency());
}

unsigned int EditView::GetLayoutThreads() const noexcept {
	return maxLayoutThreads;
}

void EditView::ClearAllTabstops() noexcept {
	ldTabstops.reset();
}

XYPOSITION EditView::NextTabstopPos(Sci::Line line, XYPOSITION x, XYPOSITION tabWidth) const noexcept {
	const int next = GetNextTabstop(line, static_cast<int>(x + tabWidthMinimumPixels));
	if (next > 0)
		return static_cast<XYPOSITION>(next);
	return (static_cast<int>((x + tabWidthMinimumPixels) / tabWidth) + 1) * tabWidth;
}

bool EditView::ClearTabstops(Sci::Line line) noexcept {
	return ldTabstops && ldTabstops->ClearTabstops(line);
}

bool EditView::AddTabstop(Sci::Line line, int x) {
	if (!ldTabstops) {
		ldTabstops = std::make_unique<LineTabstops>();
	}
	return ldTabstops && ldTabstops->AddTabstop(line, x);
}

int EditView::GetNextTabstop(Sci::Line line, int x) const noexcept {
	if (ldTabstops) {
		return ldTabstops->GetNextTabstop(line, x);
	} else {
		return 0;
	}
}

void EditView::LinesAddedOrRemoved(Sci::Line lineOfPos, Sci::Line linesAdded) {
	if (ldTabstops) {
		if (linesAdded > 0) {
			for (Sci::Line line = lineOfPos; line < lineOfPos + linesAdded; line++) {
				ldTabstops->InsertLine(line);
			}
		} else {
			for (Sci::Line line = (lineOfPos + -linesAdded) - 1; line >= lineOfPos; line--) {
				ldTabstops->RemoveLine(line);
			}
		}
	}
}

void EditView::DropGraphics() noexcept {
	pixmapLine.reset();
	pixmapIndentGuide.reset();
	pixmapIndentGuideHighlight.reset();
}

void EditView::RefreshPixMaps(Surface *surfaceWindow, const ViewStyle &vsDraw) {
	if (!pixmapIndentGuide) {
		// 1 extra pixel in height so can handle odd/even positions and so produce a continuous line
		pixmapIndentGuide = surfaceWindow->AllocatePixMap(1, vsDraw.lineHeight + 1);
		pixmapIndentGuideHighlight = surfaceWindow->AllocatePixMap(1, vsDraw.lineHeight + 1);
		const PRectangle rcIG = PRectangle::FromInts(0, 0, 1, vsDraw.lineHeight);
		pixmapIndentGuide->FillRectangle(rcIG, vsDraw.styles[StyleIndentGuide].back);
		pixmapIndentGuideHighlight->FillRectangle(rcIG, vsDraw.styles[StyleBraceLight].back);
		for (int stripe = 1; stripe < vsDraw.lineHeight + 1; stripe += 2) {
			const PRectangle rcPixel = PRectangle::FromInts(0, stripe, 1, stripe + 1);
			pixmapIndentGuide->FillRectangle(rcPixel, vsDraw.styles[StyleIndentGuide].fore);
			pixmapIndentGuideHighlight->FillRectangle(rcPixel, vsDraw.styles[StyleBraceLight].fore);
		}
		pixmapIndentGuide->FlushDrawing();
		pixmapIndentGuideHighlight->FlushDrawing();
	}
}

std::shared_ptr<LineLayout> EditView::RetrieveLineLayout(Sci::Line lineNumber, const EditModel &model) {
	const Sci::Position posLineStart = model.pdoc->LineStart(lineNumber);
	const Sci::Position posLineEnd = model.pdoc->LineStart(lineNumber + 1);
	PLATFORM_ASSERT(posLineEnd >= posLineStart);
	const Sci::Line lineCaret = model.pdoc->SciLineFromPosition(model.sel.MainCaret());
	return llc.Retrieve(lineNumber, lineCaret,
		static_cast<int>(posLineEnd - posLineStart), model.pdoc->GetStyleClock(),
		model.LinesOnScreen() + 1, model.pdoc->LinesTotal());
}

namespace {

constexpr XYPOSITION epsilon = 0.0001f;	// A small nudge to avoid floating point precision issues

/**
* Return the chDoc argument with case transformed as indicated by the caseForce argument.
* chPrevious is needed for camel casing.
* This only affects ASCII characters and is provided for languages with case-insensitive
* ASCII keywords where the user wishes to view keywords in a preferred case.
*/
inline char CaseForce(Style::CaseForce caseForce, char chDoc, char chPrevious) noexcept {
	switch (caseForce) {
	case Style::CaseForce::mixed:
		return chDoc;
	case Style::CaseForce::lower:
		return MakeLowerCase(chDoc);
	case Style::CaseForce::upper:
		return MakeUpperCase(chDoc);
	case Style::CaseForce::camel:
	default:	// default should not occur, included to avoid warnings
		if (IsUpperOrLowerCase(chDoc) && !IsUpperOrLowerCase(chPrevious)) {
			return MakeUpperCase(chDoc);
		} else {
			return MakeLowerCase(chDoc);
		}
	}
}

void LayoutSegments(IPositionCache *pCache,
	Surface *surface,
	const ViewStyle &vstyle,
	LineLayout *ll,
	const std::vector<TextSegment> &segments,
	std::atomic<uint32_t> &nextIndex,
	const bool textUnicode,
	const bool multiThreaded) {
	while (true) {
		const uint32_t i = nextIndex.fetch_add(1, std::memory_order_acq_rel);
		if (i >= segments.size()) {
			break;
		}
		const TextSegment &ts = segments[i];
		const unsigned int styleSegment = ll->styles[ts.start];
		XYPOSITION *positions = &ll->positions[ts.start + 1];
		if (vstyle.styles[styleSegment].visible) {
			if (ts.representation) {
				XYPOSITION representationWidth = 0.0;
				// Tab is a special case of representation, taking a variable amount of space
				// which will be filled in later.
				if (ll->chars[ts.start] != '\t') {
					representationWidth = vstyle.controlCharWidth;
					if (representationWidth <= 0.0) {
						assert(ts.representation->stringRep.length() <= Representation::maxLength);
						XYPOSITION positionsRepr[Representation::maxLength + 1];
						// ts.representation->stringRep is UTF-8.
						pCache->MeasureWidths(surface, vstyle, StyleControlChar, true, ts.representation->stringRep,
							positionsRepr, multiThreaded);
						representationWidth = positionsRepr[ts.representation->stringRep.length() - 1];
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Blob)) {
							representationWidth += vstyle.ctrlCharPadding;
						}
					}
				}
				std::fill(positions, positions + ts.length, representationWidth);
			} else {
				if ((ts.length == 1) && (' ' == ll->chars[ts.start])) {
					// Over half the segments are single characters and of these about half are space characters.
					positions[0] = vstyle.styles[styleSegment].spaceWidth;
				} else {
					pCache->MeasureWidths(surface, vstyle, styleSegment, textUnicode,
						std::string_view(&ll->chars[ts.start], ts.length), positions, multiThreaded);
				}
			}
		} else if (vstyle.styles[styleSegment].invisibleRepresentation[0]) {
			const std::string_view text = vstyle.styles[styleSegment].invisibleRepresentation;
			XYPOSITION positionsRepr[Representation::maxLength + 1];
			// invisibleRepresentation is UTF-8.
			pCache->MeasureWidths(surface, vstyle, styleSegment, true, text, positionsRepr, multiThreaded);
			const XYPOSITION representationWidth = positionsRepr[text.length() - 1];
			std::fill(positions, positions + ts.length, representationWidth);
		}
	}
}

}

/**
* Fill in the LineLayout data for the given line.
* Copy the given @a line and its styles from the document into local arrays.
* Also determine the x position at which each character starts.
*/
void EditView::LayoutLine(const EditModel &model, Surface *surface, const ViewStyle &vstyle, LineLayout *ll, int width, bool callerMultiThreaded) {
	if (!ll)
		return;
	const Sci::Line line = ll->LineNumber();
	PLATFORM_ASSERT(line < model.pdoc->LinesTotal());
	PLATFORM_ASSERT(ll->chars);
	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	Sci::Position posLineEnd = model.pdoc->LineStart(line + 1);
	// If the line is very long, limit the treatment to a length that should fit in the viewport
	if (posLineEnd >(posLineStart + ll->maxLineLength)) {
		posLineEnd = posLineStart + ll->maxLineLength;
	}
	// Hard to cope when too narrow, so just assume there is space
	width = std::max(width, 20);

	if (ll->validity == LineLayout::ValidLevel::checkTextAndStyle) {
		Sci::Position lineLength = posLineEnd - posLineStart;
		if (!vstyle.viewEOL) {
			lineLength = model.pdoc->LineEnd(line) - posLineStart;
		}
		if (lineLength == ll->numCharsInLine) {
			// See if chars, styles, indicators, are all the same
			bool allSame = true;
			// Check base line layout
			char chPrevious = 0;
			for (Sci::Position numCharsInLine = 0; numCharsInLine < lineLength; numCharsInLine++) {
				const Sci::Position charInDoc = numCharsInLine + posLineStart;
				const char chDoc = model.pdoc->CharAt(charInDoc);
				const int styleByte = model.pdoc->StyleIndexAt(charInDoc);
				allSame = allSame &&
					(ll->styles[numCharsInLine] == styleByte);
				allSame = allSame &&
					(ll->chars[numCharsInLine] == CaseForce(vstyle.styles[styleByte].caseForce, chDoc, chPrevious));
				chPrevious = chDoc;
			}
			const int styleByteLast = (posLineEnd > posLineStart) ? model.pdoc->StyleIndexAt(posLineEnd - 1) : 0;
			allSame = allSame && (ll->styles[lineLength] == styleByteLast);	// For eolFilled
			if (allSame) {
				ll->validity = (ll->widthLine != width) ? LineLayout::ValidLevel::positions : LineLayout::ValidLevel::lines;
			} else {
				ll->validity = LineLayout::ValidLevel::invalid;
			}
		} else {
			ll->validity = LineLayout::ValidLevel::invalid;
		}
	}
	if (ll->validity == LineLayout::ValidLevel::invalid) {
		ll->widthLine = LineLayout::wrapWidthInfinite;
		ll->lines = 1;
		if (vstyle.edgeState == EdgeVisualStyle::Background) {
			Sci::Position edgePosition = model.pdoc->FindColumn(line, vstyle.theEdge.column);
			if (edgePosition >= posLineStart) {
				edgePosition -= posLineStart;
			}
			ll->edgeColumn = static_cast<int>(edgePosition);
		} else {
			ll->edgeColumn = -1;
		}

		// Fill base line layout
		const int lineLength = static_cast<int>(posLineEnd - posLineStart);
		model.pdoc->GetCharRange(ll->chars.get(), posLineStart, lineLength);
		model.pdoc->GetStyleRange(ll->styles.get(), posLineStart, lineLength);
		const int numCharsBeforeEOL = static_cast<int>(model.pdoc->LineEnd(line) - posLineStart);
		const int numCharsInLine = (vstyle.viewEOL) ? lineLength : numCharsBeforeEOL;
		const unsigned char styleByteLast = (lineLength > 0) ? ll->styles[lineLength - 1] : 0;
		if (vstyle.someStylesForceCase) {
			char chPrevious = 0;
			for (int charInLine = 0; charInLine<lineLength; charInLine++) {
				const char chDoc = ll->chars[charInLine];
				ll->chars[charInLine] = CaseForce(vstyle.styles[ll->styles[charInLine]].caseForce, chDoc, chPrevious);
				chPrevious = chDoc;
			}
		}
		ll->xHighlightGuide = 0;
		// Extra element at the end of the line to hold end x position and act as
		ll->chars[numCharsInLine] = 0;   // Also triggers processing in the loops as this is a control character
		ll->styles[numCharsInLine] = styleByteLast;	// For eolFilled

		// Layout the line, determining the position of each character,
		// with an extra element at the end for the end of the line.
		ll->positions[0] = 0;
		bool lastSegItalics = false;

		std::vector<TextSegment> segments;
		BreakFinder bfLayout(ll, nullptr, Range(0, numCharsInLine), posLineStart, 0, BreakFinder::BreakFor::Text, model.pdoc, model.reprs.get(), nullptr);
		while (bfLayout.More()) {
			segments.push_back(bfLayout.Next());
		}

		ll->ClearPositions();

		if (!segments.empty()) {

			const size_t threadsForLength = std::max(1, numCharsInLine / bytesPerLayoutThread);
			size_t threads = std::min<size_t>({ segments.size(), threadsForLength, maxLayoutThreads });
			if (!surface->SupportsFeature(Supports::ThreadSafeMeasureWidths) || callerMultiThreaded) {
				threads = 1;
			}

			std::atomic<uint32_t> nextIndex = 0;

			const bool textUnicode = CpUtf8 == model.pdoc->dbcsCodePage;
			const bool multiThreaded = threads > 1;
			const bool multiThreadedContext = multiThreaded || callerMultiThreaded;
			IPositionCache *pCache = posCache.get();

			// If only 1 thread needed then use the main thread, else spin up multiple
			const std::launch policy = (multiThreaded) ? std::launch::async : std::launch::deferred;

			std::vector<std::future<void>> futures;
			for (size_t th = 0; th < threads; th++) {
				// Find relative positions of everything except for tabs
				std::future<void> fut = std::async(policy,
					[pCache, surface, &vstyle, &ll, &segments, &nextIndex, textUnicode, multiThreadedContext]() {
					LayoutSegments(pCache, surface, vstyle, ll, segments, nextIndex, textUnicode, multiThreadedContext);
				});
				futures.push_back(std::move(fut));
			}
			for (const std::future<void> &f : futures) {
				f.wait();
			}
		}

		// Accumulate absolute positions from relative positions within segments and expand tabs
		XYPOSITION xPosition = 0.0;
		size_t iByte = 0;
		ll->positions[iByte++] = xPosition;
		for (const TextSegment &ts : segments) {
			if (vstyle.styles[ll->styles[ts.start]].visible &&
				ts.representation &&
				(ll->chars[ts.start] == '\t')) {
				// Simple visible tab, go to next tab stop
				const XYPOSITION startTab = ll->positions[ts.start];
				const XYPOSITION nextTab = NextTabstopPos(line, startTab, vstyle.tabWidth);
				xPosition += nextTab - startTab;
			}
			const XYPOSITION xBeginSegment = xPosition;
			for (int i = 0; i < ts.length; i++) {
				xPosition = ll->positions[iByte] + xBeginSegment;
				ll->positions[iByte++] = xPosition;
			}
		}

		if (!segments.empty()) {
			// Not quite the same as before which would effectively ignore trailing invisible segments
			const TextSegment &ts = segments.back();
			lastSegItalics = (!ts.representation) && ((ll->chars[ts.end() - 1] != ' ') && vstyle.styles[ll->styles[ts.start]].italic);
		}

		// Small hack to make lines that end with italics not cut off the edge of the last character
		if (lastSegItalics) {
			ll->positions[numCharsInLine] += vstyle.lastSegItalicsOffset;
		}
		ll->numCharsInLine = numCharsInLine;
		ll->numCharsBeforeEOL = numCharsBeforeEOL;
		ll->validity = LineLayout::ValidLevel::positions;
	}
	if ((ll->validity == LineLayout::ValidLevel::positions) || (ll->widthLine != width)) {
		ll->widthLine = width;
		if (width == LineLayout::wrapWidthInfinite) {
			ll->lines = 1;
		} else if (width > ll->positions[ll->numCharsInLine]) {
			// Simple common case where line does not need wrapping.
			ll->lines = 1;
		} else {
			if (FlagSet(vstyle.wrap.visualFlags, WrapVisualFlag::End)) {
				width -= static_cast<int>(vstyle.aveCharWidth); // take into account the space for end wrap mark
			}
			XYPOSITION wrapAddIndent = 0; // This will be added to initial indent of line
			switch (vstyle.wrap.indentMode) {
			case WrapIndentMode::Fixed:
				wrapAddIndent = vstyle.wrap.visualStartIndent * vstyle.aveCharWidth;
				break;
			case WrapIndentMode::Indent:
				wrapAddIndent = model.pdoc->IndentSize() * vstyle.spaceWidth;
				break;
			case WrapIndentMode::DeepIndent:
				wrapAddIndent = model.pdoc->IndentSize() * 2 * vstyle.spaceWidth;
				break;
			default:	// No additional indent for WrapIndentMode::Fixed
				break;
			}
			ll->wrapIndent = wrapAddIndent;
			if (vstyle.wrap.indentMode != WrapIndentMode::Fixed) {
				for (int i = 0; i < ll->numCharsInLine; i++) {
					if (!IsSpaceOrTab(ll->chars[i])) {
						ll->wrapIndent += ll->positions[i]; // Add line indent
						break;
					}
				}
			}
			// Check for text width minimum
			if (ll->wrapIndent > width - static_cast<int>(vstyle.aveCharWidth) * 15)
				ll->wrapIndent = wrapAddIndent;
			// Check for wrapIndent minimum
			if ((FlagSet(vstyle.wrap.visualFlags, WrapVisualFlag::Start)) && (ll->wrapIndent < vstyle.aveCharWidth))
				ll->wrapIndent = vstyle.aveCharWidth; // Indent to show start visual
			ll->WrapLine(model.pdoc, posLineStart, vstyle.wrap.state, width);
		}
		ll->validity = LineLayout::ValidLevel::lines;
	}
}

// Fill the LineLayout bidirectional data fields according to each char style

void EditView::UpdateBidiData(const EditModel &model, const ViewStyle &vstyle, LineLayout *ll) {
	if (model.BidirectionalEnabled()) {
		ll->EnsureBidiData();
		for (int stylesInLine = 0; stylesInLine < ll->numCharsInLine; stylesInLine++) {
			ll->bidiData->stylesFonts[stylesInLine] = vstyle.styles[ll->styles[stylesInLine]].font;
		}
		ll->bidiData->stylesFonts[ll->numCharsInLine].reset();

		for (int charsInLine = 0; charsInLine < ll->numCharsInLine; charsInLine++) {
			const int charWidth = UTF8DrawBytes(&ll->chars[charsInLine], ll->numCharsInLine - charsInLine);
			const Representation *repr = model.reprs->RepresentationFromCharacter(std::string_view(&ll->chars[charsInLine], charWidth));

			ll->bidiData->widthReprs[charsInLine] = 0.0f;
			if (repr && ll->chars[charsInLine] != '\t') {
				ll->bidiData->widthReprs[charsInLine] = ll->positions[charsInLine + charWidth] - ll->positions[charsInLine];
			}
			if (charWidth > 1) {
				for (int c = 1; c < charWidth; c++) {
					charsInLine++;
					ll->bidiData->widthReprs[charsInLine] = 0.0f;
				}
			}
		}
		ll->bidiData->widthReprs[ll->numCharsInLine] = 0.0f;
	} else {
		ll->bidiData.reset();
	}
}

Point EditView::LocationFromPosition(Surface *surface, const EditModel &model, SelectionPosition pos, Sci::Line topLine,
				     const ViewStyle &vs, PointEnd pe, const PRectangle rcClient) {
	Point pt;
	if (pos.Position() == Sci::invalidPosition)
		return pt;
	Sci::Line lineDoc = model.pdoc->SciLineFromPosition(pos.Position());
	Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
	if (FlagSet(pe, PointEnd::lineEnd) && (lineDoc > 0) && (pos.Position() == posLineStart)) {
		// Want point at end of first line
		lineDoc--;
		posLineStart = model.pdoc->LineStart(lineDoc);
	}
	const Sci::Line lineVisible = model.pcs->DisplayFromDoc(lineDoc);
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(lineDoc, model);
	if (surface && ll) {
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const int posInLine = static_cast<int>(pos.Position() - posLineStart);
		pt = ll->PointFromPosition(posInLine, vs.lineHeight, pe);
		pt.x += vs.textStart - model.xOffset;

		if (model.BidirectionalEnabled()) {
			// Fill the line bidi data
			UpdateBidiData(model, vs, ll.get());

			// Find subLine
			const int subLine = ll->SubLineFromPosition(posInLine, pe);
			const int caretPosition = posInLine - ll->LineStart(subLine);

			// Get the point from current position
			const ScreenLine screenLine(ll.get(), subLine, vs, rcClient.right, tabWidthMinimumPixels);
			std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);
			pt.x = slLayout->XFromPosition(caretPosition);

			pt.x += vs.textStart - model.xOffset;

			pt.y = 0;
			if (posInLine >= ll->LineStart(subLine)) {
				pt.y = static_cast<XYPOSITION>(subLine*vs.lineHeight);
			}
		}
		pt.y += (lineVisible - topLine) * vs.lineHeight;
		pt.x += pos.VirtualSpace() * vs.styles[ll->EndLineStyle()].spaceWidth;
	}
	return pt;
}

Range EditView::RangeDisplayLine(Surface *surface, const EditModel &model, Sci::Line lineVisible, const ViewStyle &vs) {
	Range rangeSubLine = Range(0, 0);
	if (lineVisible < 0) {
		return rangeSubLine;
	}
	const Sci::Line lineDoc = model.pcs->DocFromDisplay(lineVisible);
	const Sci::Position positionLineStart = model.pdoc->LineStart(lineDoc);
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(lineDoc, model);
	if (surface && ll) {
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const Sci::Line lineStartSet = model.pcs->DisplayFromDoc(lineDoc);
		const int subLine = static_cast<int>(lineVisible - lineStartSet);
		if (subLine < ll->lines) {
			rangeSubLine = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);
			if (subLine == ll->lines-1) {
				rangeSubLine.end = model.pdoc->LineStart(lineDoc + 1) -
					positionLineStart;
			}
		}
	}
	rangeSubLine.start += positionLineStart;
	rangeSubLine.end += positionLineStart;
	return rangeSubLine;
}

SelectionPosition EditView::SPositionFromLocation(Surface *surface, const EditModel &model, PointDocument pt, bool canReturnInvalid,
	bool charPosition, bool virtualSpace, const ViewStyle &vs, const PRectangle rcClient) {
	pt.x = pt.x - vs.textStart;
	Sci::Line visibleLine = static_cast<int>(std::floor(pt.y / vs.lineHeight));
	if (!canReturnInvalid && (visibleLine < 0))
		visibleLine = 0;
	const Sci::Line lineDoc = model.pcs->DocFromDisplay(visibleLine);
	if (canReturnInvalid && (lineDoc < 0))
		return SelectionPosition(Sci::invalidPosition);
	if (lineDoc >= model.pdoc->LinesTotal())
		return SelectionPosition(canReturnInvalid ? Sci::invalidPosition :
			model.pdoc->Length());
	const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(lineDoc, model);
	if (surface && ll) {
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const Sci::Line lineStartSet = model.pcs->DisplayFromDoc(lineDoc);
		const int subLine = static_cast<int>(visibleLine - lineStartSet);
		if (subLine < ll->lines) {
			const Range rangeSubLine = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);
			const XYPOSITION subLineStart = ll->positions[rangeSubLine.start];
			if (subLine > 0)	// Wrapped
				pt.x -= ll->wrapIndent;
			Sci::Position positionInLine = 0;
			if (model.BidirectionalEnabled()) {
				// Fill the line bidi data
				UpdateBidiData(model, vs, ll.get());

				const ScreenLine screenLine(ll.get(), subLine, vs, rcClient.right, tabWidthMinimumPixels);
				std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);
				positionInLine = slLayout->PositionFromX(pt.x, charPosition) +
					rangeSubLine.start;
			} else {
				positionInLine = ll->FindPositionFromX(pt.x + subLineStart,
					rangeSubLine, charPosition);
			}
			if (positionInLine < rangeSubLine.end) {
				return SelectionPosition(model.pdoc->MovePositionOutsideChar(positionInLine + posLineStart, 1));
			}
			if (virtualSpace) {
				const XYPOSITION spaceWidth = vs.styles[ll->EndLineStyle()].spaceWidth;
				const int spaceOffset = static_cast<int>(
					(pt.x + subLineStart - ll->positions[rangeSubLine.end] + spaceWidth / 2) / spaceWidth);
				return SelectionPosition(rangeSubLine.end + posLineStart, spaceOffset);
			} else if (canReturnInvalid) {
				if (pt.x < (ll->positions[rangeSubLine.end] - subLineStart)) {
					return SelectionPosition(model.pdoc->MovePositionOutsideChar(rangeSubLine.end + posLineStart, 1));
				}
			} else {
				return SelectionPosition(rangeSubLine.end + posLineStart);
			}
		}
		if (!canReturnInvalid)
			return SelectionPosition(ll->numCharsInLine + posLineStart);
	}
	return SelectionPosition(canReturnInvalid ? Sci::invalidPosition : posLineStart);
}

/**
* Find the document position corresponding to an x coordinate on a particular document line.
* Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
* This method is used for rectangular selections and does not work on wrapped lines.
*/
SelectionPosition EditView::SPositionFromLineX(Surface *surface, const EditModel &model, Sci::Line lineDoc, int x, const ViewStyle &vs) {
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(lineDoc, model);
	if (surface && ll) {
		const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const Range rangeSubLine = ll->SubLineRange(0, LineLayout::Scope::visibleOnly);
		const XYPOSITION subLineStart = ll->positions[rangeSubLine.start];
		const Sci::Position positionInLine = ll->FindPositionFromX(x + subLineStart, rangeSubLine, false);
		if (positionInLine < rangeSubLine.end) {
			return SelectionPosition(model.pdoc->MovePositionOutsideChar(positionInLine + posLineStart, 1));
		}
		const XYPOSITION spaceWidth = vs.styles[ll->EndLineStyle()].spaceWidth;
		const int spaceOffset = static_cast<int>(
			(x + subLineStart - ll->positions[rangeSubLine.end] + spaceWidth / 2) / spaceWidth);
		return SelectionPosition(rangeSubLine.end + posLineStart, spaceOffset);
	}
	return SelectionPosition(0);
}

Sci::Line EditView::DisplayFromPosition(Surface *surface, const EditModel &model, Sci::Position pos, const ViewStyle &vs) {
	const Sci::Line lineDoc = model.pdoc->SciLineFromPosition(pos);
	Sci::Line lineDisplay = model.pcs->DisplayFromDoc(lineDoc);
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(lineDoc, model);
	if (surface && ll) {
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
		const Sci::Position posInLine = pos - posLineStart;
		lineDisplay--; // To make up for first increment ahead.
		for (int subLine = 0; subLine < ll->lines; subLine++) {
			if (posInLine >= ll->LineStart(subLine)) {
				lineDisplay++;
			}
		}
	}
	return lineDisplay;
}

Sci::Position EditView::StartEndDisplayLine(Surface *surface, const EditModel &model, Sci::Position pos, bool start, const ViewStyle &vs) {
	const Sci::Line line = model.pdoc->SciLineFromPosition(pos);
	std::shared_ptr<LineLayout> ll = RetrieveLineLayout(line, model);
	Sci::Position posRet = Sci::invalidPosition;
	if (surface && ll) {
		const Sci::Position posLineStart = model.pdoc->LineStart(line);
		LayoutLine(model, surface, vs, ll.get(), model.wrapWidth);
		const Sci::Position posInLine = pos - posLineStart;
		if (posInLine <= ll->maxLineLength) {
			for (int subLine = 0; subLine < ll->lines; subLine++) {
				if ((posInLine >= ll->LineStart(subLine)) &&
				    (posInLine <= ll->LineStart(subLine + 1)) &&
				    (posInLine <= ll->numCharsBeforeEOL)) {
					if (start) {
						posRet = ll->LineStart(subLine) + posLineStart;
					} else {
						if (subLine == ll->lines - 1)
							posRet = ll->numCharsBeforeEOL + posLineStart;
						else
							posRet = model.pdoc->MovePositionOutsideChar(ll->LineStart(subLine + 1) + posLineStart - 1, -1, false);
					}
				}
			}
		}
	}
	return posRet;
}

namespace {

constexpr ColourRGBA colourBug(0xff, 0, 0xfe, 0xf0);

// Selection background colours are always defined, the value_or is to show if bug

ColourRGBA SelectionBackground(const EditModel &model, const ViewStyle &vsDraw, InSelection inSelection) {
	if (inSelection == InSelection::inNone)
		return colourBug;	// Not selected is a bug

	Element element = Element::SelectionBack;
	if (inSelection == InSelection::inAdditional)
		element = Element::SelectionAdditionalBack;
	if (!model.primarySelection)
		element = Element::SelectionSecondaryBack;
	if (!model.hasFocus) {
		if (inSelection == InSelection::inAdditional) {
			if (ColourOptional colour = vsDraw.ElementColour(Element::SelectionInactiveAdditionalBack)) {
				return *colour;
			}
		}
		if (ColourOptional colour = vsDraw.ElementColour(Element::SelectionInactiveBack)) {
			return *colour;
		}
	}
	return vsDraw.ElementColour(element).value_or(colourBug);
}

ColourOptional SelectionForeground(const EditModel &model, const ViewStyle &vsDraw, InSelection inSelection) {
	if (inSelection == InSelection::inNone)
		return {};
	Element element = Element::SelectionText;
	if (inSelection == InSelection::inAdditional)
		element = Element::SelectionAdditionalText;
	if (!model.primarySelection)	// Secondary selection
		element = Element::SelectionSecondaryText;
	if (!model.hasFocus) {
		if (inSelection == InSelection::inAdditional) {
			if (ColourOptional colour = vsDraw.ElementColour(Element::SelectionInactiveAdditionalText)) {
				return colour;
			}
		}
		element = Element::SelectionInactiveText;
	}
	return vsDraw.ElementColour(element);
}

ColourRGBA TextBackground(const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	ColourOptional background, InSelection inSelection, bool inHotspot, int styleMain, Sci::Position i) {
	if (inSelection && (vsDraw.selection.layer == Layer::Base)) {
		return SelectionBackground(model, vsDraw, inSelection).Opaque();
	}
	if ((vsDraw.edgeState == EdgeVisualStyle::Background) &&
		(i >= ll->edgeColumn) &&
		(i < ll->numCharsBeforeEOL))
		return vsDraw.theEdge.colour;
	if (inHotspot) {
		if (const ColourOptional colourHotSpotBack = vsDraw.ElementColour(Element::HotSpotActiveBack)) {
			return colourHotSpotBack->Opaque();
		}
	}
	if (background && (styleMain != StyleBraceLight) && (styleMain != StyleBraceBad)) {
		return *background;
	} else {
		return vsDraw.styles[styleMain].back;
	}
}

void DrawTextBlob(Surface *surface, const ViewStyle &vsDraw, PRectangle rcSegment,
	std::string_view text, ColourRGBA textBack, ColourRGBA textFore, bool fillBackground) {
	if (rcSegment.Empty())
		return;
	if (fillBackground) {
		surface->FillRectangleAligned(rcSegment, Fill(textBack));
	}
	const Font *ctrlCharsFont = vsDraw.styles[StyleControlChar].font.get();
	const int normalCharHeight = static_cast<int>(std::ceil(vsDraw.styles[StyleControlChar].capitalHeight));
	PRectangle rcCChar = rcSegment;
	rcCChar.left = rcCChar.left + 1;
	rcCChar.top = rcSegment.top + vsDraw.maxAscent - normalCharHeight;
	rcCChar.bottom = rcSegment.top + vsDraw.maxAscent + 1;
	PRectangle rcCentral = rcCChar;
	rcCentral.top++;
	rcCentral.bottom--;
	surface->FillRectangleAligned(rcCentral, Fill(textFore));
	PRectangle rcChar = rcCChar;
	rcChar.left++;
	rcChar.right--;
	surface->DrawTextClippedUTF8(rcChar, ctrlCharsFont,
		rcSegment.top + vsDraw.maxAscent, text,
		textBack, textFore);
}

void FillLineRemainder(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcArea, int subLine) {
	InSelection eolInSelection = InSelection::inNone;
	if (vsDraw.selection.visible && (subLine == (ll->lines - 1))) {
		eolInSelection = model.LineEndInSelection(line);
	}

	if (eolInSelection && vsDraw.selection.eolFilled && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer == Layer::Base)) {
		surface->FillRectangleAligned(rcArea, Fill(SelectionBackground(model, vsDraw, eolInSelection).Opaque()));
	} else {
		const ColourOptional background = vsDraw.Background(model.GetMark(line), model.caret.active, ll->containsCaret);
		if (background) {
			surface->FillRectangleAligned(rcArea, Fill(*background));
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine]].eolFilled) {
			surface->FillRectangleAligned(rcArea, Fill(vsDraw.styles[ll->styles[ll->numCharsInLine]].back));
		} else {
			surface->FillRectangleAligned(rcArea, Fill(vsDraw.styles[StyleDefault].back));
		}
		if (eolInSelection && vsDraw.selection.eolFilled && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer != Layer::Base)) {
			surface->FillRectangleAligned(rcArea, SelectionBackground(model, vsDraw, eolInSelection));
		}
	}
}

}

void EditView::DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, Sci::Position lineEnd, XYPOSITION subLineStart, ColourOptional background) {

	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	PRectangle rcSegment = rcLine;

	const bool lastSubLine = subLine == (ll->lines - 1);
	XYPOSITION virtualSpace = 0;
	if (lastSubLine) {
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		virtualSpace = model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line)) * spaceWidth;
	}
	const XYPOSITION xEol = ll->positions[lineEnd] - subLineStart;

	// Fill the virtual space and show selections within it
	if (virtualSpace > 0.0f) {
		rcSegment.left = xEol + xStart;
		rcSegment.right = xEol + xStart + virtualSpace;
		const ColourRGBA backgroundFill = background.value_or(vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		surface->FillRectangleAligned(rcSegment, backgroundFill);
		if (vsDraw.selection.visible && (vsDraw.selection.layer == Layer::Base)) {
			const SelectionSegment virtualSpaceRange(SelectionPosition(model.pdoc->LineEnd(line)),
				SelectionPosition(model.pdoc->LineEnd(line),
					model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line))));
			for (size_t r = 0; r<model.sel.Count(); r++) {
				const SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
				if (!portion.Empty()) {
					const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
					rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] -
						subLineStart+portion.start.VirtualSpace() * spaceWidth;
					rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] -
						subLineStart+portion.end.VirtualSpace() * spaceWidth;
					rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
					rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
					surface->FillRectangleAligned(rcSegment, Fill(
						SelectionBackground(model, vsDraw, model.sel.RangeType(r)).Opaque()));
				}
			}
		}
	}

	InSelection eolInSelection = InSelection::inNone;
	if (vsDraw.selection.visible && lastSubLine) {
		eolInSelection = model.LineEndInSelection(line);
	}

	const ColourRGBA selectionBack = SelectionBackground(model, vsDraw, eolInSelection);

	// Draw the [CR], [LF], or [CR][LF] blobs if visible line ends are on
	XYPOSITION blobsWidth = 0;
	if (lastSubLine) {
		for (Sci::Position eolPos = ll->numCharsBeforeEOL; eolPos<ll->numCharsInLine;) {
			const int styleMain = ll->styles[eolPos];
			const ColourOptional selectionFore = SelectionForeground(model, vsDraw, eolInSelection);
			ColourRGBA textFore = selectionFore.value_or(vsDraw.styles[styleMain].fore);
			char hexits[4] = "";
			std::string_view ctrlChar;
			Sci::Position widthBytes = 1;
			RepresentationAppearance appearance = RepresentationAppearance::Blob;
			const Representation *repr = model.reprs->RepresentationFromCharacter(std::string_view(&ll->chars[eolPos], ll->numCharsInLine - eolPos));
			if (repr) {
				// Representation of whole text
				widthBytes = ll->numCharsInLine - eolPos;
			} else {
				repr = model.reprs->RepresentationFromCharacter(std::string_view(&ll->chars[eolPos], 1));
			}
			if (repr) {
				ctrlChar = repr->stringRep;
				appearance = repr->appearance;
				if (FlagSet(appearance, RepresentationAppearance::Colour)) {
					textFore = repr->colour;
				}
			} else {
				const unsigned char chEOL = ll->chars[eolPos];
				if (UTF8IsAscii(chEOL)) {
					ctrlChar = ControlCharacterString(chEOL);
				} else {
					Hexits(hexits, chEOL);
					ctrlChar = hexits;
				}
			}

			rcSegment.left = xStart + ll->positions[eolPos] - subLineStart + virtualSpace;
			rcSegment.right = xStart + ll->positions[eolPos + widthBytes] - subLineStart + virtualSpace;
			blobsWidth += rcSegment.Width();
			const ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, eolInSelection, false, styleMain, eolPos);
			if (eolInSelection && (line < model.pdoc->LinesTotal() - 1)) {
				if (vsDraw.selection.layer == Layer::Base) {
					surface->FillRectangleAligned(rcSegment, Fill(selectionBack.Opaque()));
				} else {
					surface->FillRectangleAligned(rcSegment, Fill(textBack));
				}
			} else {
				surface->FillRectangleAligned(rcSegment, Fill(textBack));
			}
			const bool drawEOLSelection = eolInSelection && (line < model.pdoc->LinesTotal() - 1);
			ColourRGBA blobText = textBack;
			if (drawEOLSelection && (vsDraw.selection.layer == Layer::UnderText)) {
				surface->FillRectangleAligned(rcSegment, selectionBack);
				blobText = textBack.MixedWith(selectionBack, selectionBack.GetAlphaComponent());
			}
			if (FlagSet(appearance, RepresentationAppearance::Blob)) {
				DrawTextBlob(surface, vsDraw, rcSegment, ctrlChar, blobText, textFore, phasesDraw == PhasesDraw::One);
			} else {
				surface->DrawTextTransparentUTF8(rcSegment, vsDraw.styles[StyleControlChar].font.get(),
					rcSegment.top + vsDraw.maxAscent, ctrlChar, textFore);
			}
			if (drawEOLSelection && (vsDraw.selection.layer == Layer::OverText)) {
				surface->FillRectangleAligned(rcSegment, selectionBack);
			}
			eolPos += widthBytes;
		}
	}

	// Draw the eol-is-selected rectangle
	rcSegment.left = xEol + xStart + virtualSpace + blobsWidth;
	rcSegment.right = rcSegment.left + vsDraw.aveCharWidth;

	if (eolInSelection && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer == Layer::Base)) {
		surface->FillRectangleAligned(rcSegment, Fill(selectionBack.Opaque()));
	} else {
		if (background) {
			surface->FillRectangleAligned(rcSegment, Fill(*background));
		} else if (line < model.pdoc->LinesTotal() - 1) {
			surface->FillRectangleAligned(rcSegment, Fill(vsDraw.styles[ll->styles[ll->numCharsInLine]].back));
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine]].eolFilled) {
			surface->FillRectangleAligned(rcSegment, Fill(vsDraw.styles[ll->styles[ll->numCharsInLine]].back));
		} else {
			surface->FillRectangleAligned(rcSegment, Fill(vsDraw.styles[StyleDefault].back));
		}
		if (eolInSelection && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer != Layer::Base)) {
			surface->FillRectangleAligned(rcSegment, selectionBack);
		}
	}

	rcSegment.left = rcSegment.right;
	if (rcSegment.left < rcLine.left)
		rcSegment.left = rcLine.left;
	rcSegment.right = rcLine.right;

	const bool drawEOLAnnotationStyledText = (vsDraw.eolAnnotationVisible != EOLAnnotationVisible::Hidden) && model.pdoc->EOLAnnotationStyledText(line).text;
	const bool fillRemainder = (!lastSubLine || (!model.GetFoldDisplayText(line) && !drawEOLAnnotationStyledText));
	if (fillRemainder) {
		// Fill the remainder of the line
		FillLineRemainder(surface, model, vsDraw, ll, line, rcSegment, subLine);
	}

	bool drawWrapMarkEnd = false;

	if (subLine + 1 < ll->lines) {
		if (FlagSet(vsDraw.wrap.visualFlags, WrapVisualFlag::End)) {
			drawWrapMarkEnd = ll->LineStart(subLine + 1) != 0;
		}
		if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret)) {
			// Draw right of frame under marker
			surface->FillRectangleAligned(Side(rcLine, Edge::right, vsDraw.GetFrameWidth()),
				vsDraw.ElementColourForced(Element::CaretLineBack).Opaque());
		}
	}

	if (drawWrapMarkEnd) {
		PRectangle rcPlace = rcSegment;
		const XYPOSITION maxLeft = rcPlace.right - vsDraw.aveCharWidth;

		if (FlagSet(vsDraw.wrap.visualFlagsLocation, WrapVisualLocation::EndByText)) {
			rcPlace.left = std::min(xEol + xStart + virtualSpace, maxLeft);
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;
		} else {
			// rcLine is clipped to text area
			rcPlace.right = rcLine.right;
			rcPlace.left = maxLeft;
		}
		if (!customDrawWrapMarker) {
			DrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		} else {
			customDrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		}
	}
}

void EditView::DrawFoldDisplayText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
							  Sci::Line line, int xStart, PRectangle rcLine, int subLine, XYPOSITION subLineStart, DrawPhase phase) {
	const bool lastSubLine = subLine == (ll->lines - 1);
	if (!lastSubLine)
		return;

	const char *text = model.GetFoldDisplayText(line);
	if (!text)
		return;

	PRectangle rcSegment = rcLine;
	const std::string_view foldDisplayText(text);
	const Font *fontText = vsDraw.styles[StyleFoldDisplayText].font.get();
	const int widthFoldDisplayText = static_cast<int>(surface->WidthText(fontText, foldDisplayText));

	InSelection eolInSelection = InSelection::inNone;
	if (vsDraw.selection.visible) {
		eolInSelection = model.LineEndInSelection(line);
	}

	const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
	const XYPOSITION virtualSpace = model.sel.VirtualSpaceFor(
		model.pdoc->LineEnd(line)) * spaceWidth;
	rcSegment.left = xStart + ll->positions[ll->numCharsInLine] - subLineStart + virtualSpace + vsDraw.aveCharWidth;
	rcSegment.right = rcSegment.left + static_cast<XYPOSITION>(widthFoldDisplayText);

	const ColourOptional background = vsDraw.Background(model.GetMark(line), model.caret.active, ll->containsCaret);
	const ColourOptional selectionFore = SelectionForeground(model, vsDraw, eolInSelection);
	const ColourRGBA textFore = selectionFore.value_or(vsDraw.styles[StyleFoldDisplayText].fore);
	const ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, eolInSelection,
											false, StyleFoldDisplayText, -1);

	if (model.trackLineWidth) {
		if (rcSegment.right + 1> lineWidthMaxSeen) {
			// Fold display text border drawn on rcSegment.right with width 1 is the last visible object of the line
			lineWidthMaxSeen = static_cast<int>(rcSegment.right + 1);
		}
	}

	if (FlagSet(phase, DrawPhase::back)) {
		surface->FillRectangleAligned(rcSegment, Fill(textBack));

		// Fill Remainder of the line
		PRectangle rcRemainder = rcSegment;
		rcRemainder.left = rcRemainder.right;
		if (rcRemainder.left < rcLine.left)
			rcRemainder.left = rcLine.left;
		rcRemainder.right = rcLine.right;
		FillLineRemainder(surface, model, vsDraw, ll, line, rcRemainder, subLine);
	}

	if (FlagSet(phase, DrawPhase::text)) {
		if (phasesDraw != PhasesDraw::One) {
			surface->DrawTextTransparent(rcSegment, fontText,
				rcSegment.top + vsDraw.maxAscent, foldDisplayText,
				textFore);
		} else {
			surface->DrawTextNoClip(rcSegment, fontText,
				rcSegment.top + vsDraw.maxAscent, foldDisplayText,
				textFore, textBack);
		}
	}

	if (FlagSet(phase, DrawPhase::indicatorsFore)) {
		if (model.foldDisplayTextStyle == FoldDisplayTextStyle::Boxed) {
			PRectangle rcBox = rcSegment;
			rcBox.left = std::round(rcSegment.left);
			rcBox.right = std::round(rcSegment.right);
			surface->RectangleFrame(rcBox, Stroke(textFore));
		}
	}

	if (FlagSet(phase, DrawPhase::selectionTranslucent)) {
		if (eolInSelection && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer != Layer::Base)) {
			surface->FillRectangleAligned(rcSegment, SelectionBackground(model, vsDraw, eolInSelection));
		}
	}
}

void EditView::DrawEOLAnnotationText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, XYPOSITION subLineStart, DrawPhase phase) {

	const bool lastSubLine = subLine == (ll->lines - 1);
	if (!lastSubLine)
		return;

	if (vsDraw.eolAnnotationVisible == EOLAnnotationVisible::Hidden) {
		return;
	}
	const StyledText stEOLAnnotation = model.pdoc->EOLAnnotationStyledText(line);
	if (!stEOLAnnotation.text || !ValidStyledText(vsDraw, vsDraw.eolAnnotationStyleOffset, stEOLAnnotation)) {
		return;
	}
	const std::string_view eolAnnotationText(stEOLAnnotation.text, stEOLAnnotation.length);
	const size_t style = stEOLAnnotation.style + vsDraw.eolAnnotationStyleOffset;

	PRectangle rcSegment = rcLine;
	const Font *fontText = vsDraw.styles[style].font.get();

	const Surface::Ends ends = static_cast<Surface::Ends>(static_cast<int>(vsDraw.eolAnnotationVisible) & 0xff);
	const Surface::Ends leftSide = static_cast<Surface::Ends>(static_cast<int>(ends) & 0xf);
	const Surface::Ends rightSide = static_cast<Surface::Ends>(static_cast<int>(ends) & 0xf0);

	XYPOSITION leftBoxSpace = 0;
	XYPOSITION rightBoxSpace = 0;
	if (vsDraw.eolAnnotationVisible >= EOLAnnotationVisible::Boxed) {
		leftBoxSpace = 1;
		rightBoxSpace = 1;
		if (vsDraw.eolAnnotationVisible != EOLAnnotationVisible::Boxed) {
			switch (leftSide) {
			case Surface::Ends::leftFlat:
				leftBoxSpace = 1;
				break;
			case Surface::Ends::leftAngle:
				leftBoxSpace = rcLine.Height() / 2.0;
				break;
			case Surface::Ends::semiCircles:
			default:
				leftBoxSpace = rcLine.Height() / 3.0;
			   break;
			}
			switch (rightSide) {
			case Surface::Ends::rightFlat:
				rightBoxSpace = 1;
				break;
			case Surface::Ends::rightAngle:
				rightBoxSpace = rcLine.Height() / 2.0;
				break;
			case Surface::Ends::semiCircles:
			default:
				rightBoxSpace = rcLine.Height() / 3.0;
			   break;
			}
		}
	}
	const int widthEOLAnnotationText = static_cast<int>(surface->WidthTextUTF8(fontText, eolAnnotationText) +
		leftBoxSpace + rightBoxSpace);

	const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
	const XYPOSITION virtualSpace = model.sel.VirtualSpaceFor(
		model.pdoc->LineEnd(line)) * spaceWidth;
	rcSegment.left = xStart +
		ll->positions[ll->numCharsInLine] - subLineStart
		+ virtualSpace + vsDraw.aveCharWidth;

	const char *textFoldDisplay = model.GetFoldDisplayText(line);
	if (textFoldDisplay) {
		const std::string_view foldDisplayText(textFoldDisplay);
		rcSegment.left += static_cast<int>(
			surface->WidthText(vsDraw.styles[StyleFoldDisplayText].font.get(), foldDisplayText)) +
			vsDraw.aveCharWidth;
	}
	rcSegment.right = rcSegment.left + static_cast<XYPOSITION>(widthEOLAnnotationText);

	const ColourOptional background = vsDraw.Background(model.GetMark(line), model.caret.active, ll->containsCaret);
	const ColourRGBA textFore = vsDraw.styles[style].fore;
	const ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, InSelection::inNone,
											false, static_cast<int>(style), -1);

	if (model.trackLineWidth) {
		if (rcSegment.right + 1> lineWidthMaxSeen) {
			// EOL Annotation text border drawn on rcSegment.right with width 1 is the last visible object of the line
			lineWidthMaxSeen = static_cast<int>(rcSegment.right + 1);
		}
	}

	if (FlagSet(phase, DrawPhase::back)) {
		// This fills in the whole remainder of the line even though
		// it may be double drawing. This is to allow stadiums with
		// curved or angled ends to have the area outside in the correct
		// background colour.
		PRectangle rcRemainder = rcSegment;
		rcRemainder.right = rcLine.right;
		FillLineRemainder(surface, model, vsDraw, ll, line, rcRemainder, subLine);
	}

	PRectangle rcText = rcSegment;
	rcText.left += leftBoxSpace;
	rcText.right -= rightBoxSpace;

	// For single phase drawing, draw the text then any box over it
	if (FlagSet(phase, DrawPhase::text)) {
		if (phasesDraw == PhasesDraw::One) {
			surface->DrawTextNoClipUTF8(rcText, fontText,
			rcText.top + vsDraw.maxAscent, eolAnnotationText,
			textFore, textBack);
		}
	}

	// Draw any box or stadium shape
	if (FlagSet(phase, DrawPhase::indicatorsBack)) {
		const PRectangle rcBox = PixelAlign(rcSegment, 1);

		switch (vsDraw.eolAnnotationVisible) {
		case EOLAnnotationVisible::Standard:
			if (phasesDraw != PhasesDraw::One) {
				surface->FillRectangle(rcBox, textBack);
			}
			break;

		case EOLAnnotationVisible::Boxed:
			if (phasesDraw == PhasesDraw::One) {
				// Draw a rectangular outline around the text
				surface->RectangleFrame(rcBox, textFore);
			} else {
				// Draw with a fill to fill the edges of the rectangle.
				surface->RectangleDraw(rcBox, FillStroke(textBack, textFore));
			}
			break;

		default:
			if (phasesDraw == PhasesDraw::One) {
				// Draw an outline around the text
				surface->Stadium(rcBox, FillStroke(ColourRGBA(textBack, 0), textFore), ends);
			} else {
				// Draw with a fill to fill the edges of the shape.
				surface->Stadium(rcBox, FillStroke(textBack, textFore), ends);
			}
			break;
		}
	}

	// For multi-phase drawing draw the text last as transparent over any box
	if (FlagSet(phase, DrawPhase::text)) {
		if (phasesDraw != PhasesDraw::One) {
			surface->DrawTextTransparentUTF8(rcText, fontText,
				rcText.top + vsDraw.maxAscent, eolAnnotationText,
				textFore);
		}
	}
}

namespace {

constexpr bool AnnotationBoxedOrIndented(AnnotationVisible annotationVisible) noexcept {
	return annotationVisible == AnnotationVisible::Boxed || annotationVisible == AnnotationVisible::Indented;
}

}

void EditView::DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase) {
	const int indent = static_cast<int>(model.pdoc->GetLineIndentation(line) * vsDraw.spaceWidth);
	PRectangle rcSegment = rcLine;
	const int annotationLine = subLine - ll->lines;
	const StyledText stAnnotation = model.pdoc->AnnotationStyledText(line);
	if (stAnnotation.text && ValidStyledText(vsDraw, vsDraw.annotationStyleOffset, stAnnotation)) {
		if (FlagSet(phase, DrawPhase::back)) {
			surface->FillRectangleAligned(rcSegment, Fill(vsDraw.styles[0].back));
		}
		rcSegment.left = static_cast<XYPOSITION>(xStart);
		if (model.trackLineWidth || AnnotationBoxedOrIndented(vsDraw.annotationVisible)) {
			// Only care about calculating width if tracking or need to draw indented box
			int widthAnnotation = WidestLineWidth(surface, vsDraw, vsDraw.annotationStyleOffset, stAnnotation);
			if (AnnotationBoxedOrIndented(vsDraw.annotationVisible)) {
				widthAnnotation += static_cast<int>(vsDraw.spaceWidth * 2); // Margins
				rcSegment.left = static_cast<XYPOSITION>(xStart + indent);
				rcSegment.right = rcSegment.left + widthAnnotation;
			}
			if (widthAnnotation > lineWidthMaxSeen)
				lineWidthMaxSeen = widthAnnotation;
		}
		const int annotationLines = model.pdoc->AnnotationLines(line);
		size_t start = 0;
		size_t lengthAnnotation = stAnnotation.LineLength(start);
		int lineInAnnotation = 0;
		while ((lineInAnnotation < annotationLine) && (start < stAnnotation.length)) {
			start += lengthAnnotation + 1;
			lengthAnnotation = stAnnotation.LineLength(start);
			lineInAnnotation++;
		}
		PRectangle rcText = rcSegment;
		if ((FlagSet(phase, DrawPhase::back)) && AnnotationBoxedOrIndented(vsDraw.annotationVisible)) {
			surface->FillRectangleAligned(rcText,
				Fill(vsDraw.styles[stAnnotation.StyleAt(start) + vsDraw.annotationStyleOffset].back));
			rcText.left += vsDraw.spaceWidth;
		}
		DrawStyledText(surface, vsDraw, vsDraw.annotationStyleOffset, rcText,
			stAnnotation, start, lengthAnnotation, phase);
		if ((FlagSet(phase, DrawPhase::back)) && (vsDraw.annotationVisible == AnnotationVisible::Boxed)) {
			const ColourRGBA colourBorder = vsDraw.styles[vsDraw.annotationStyleOffset].fore;
			const PRectangle rcBorder = PixelAlignOutside(rcSegment, surface->PixelDivisions());
			surface->FillRectangle(Side(rcBorder, Edge::left, 1), colourBorder);
			surface->FillRectangle(Side(rcBorder, Edge::right, 1), colourBorder);
			if (subLine == ll->lines) {
				surface->FillRectangle(Side(rcBorder, Edge::top, 1), colourBorder);
			}
			if (subLine == ll->lines + annotationLines - 1) {
				surface->FillRectangle(Side(rcBorder, Edge::bottom, 1), colourBorder);
			}
		}
	} else {
		// No annotation to draw so show bug with colourBug
		if (FlagSet(phase, DrawPhase::back)) {
			surface->FillRectangle(rcSegment, colourBug.Opaque());
		}
	}
}

namespace {

void DrawBlockCaret(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int subLine, int xStart, Sci::Position offset, Sci::Position posCaret, PRectangle rcCaret, ColourRGBA caretColour) {

	const Sci::Position lineStart = ll->LineStart(subLine);
	Sci::Position posBefore = posCaret;
	Sci::Position posAfter = model.pdoc->MovePositionOutsideChar(posCaret + 1, 1);
	Sci::Position numCharsToDraw = posAfter - posCaret;

	// Work out where the starting and ending offsets are. We need to
	// see if the previous character shares horizontal space, such as a
	// glyph / combining character. If so we'll need to draw that too.
	Sci::Position offsetFirstChar = offset;
	Sci::Position offsetLastChar = offset + (posAfter - posCaret);
	while ((posBefore > 0) && ((offsetLastChar - numCharsToDraw) >= lineStart)) {
		if ((ll->positions[offsetLastChar] - ll->positions[offsetLastChar - numCharsToDraw]) > 0) {
			// The char does not share horizontal space
			break;
		}
		// Char shares horizontal space, update the numChars to draw
		// Update posBefore to point to the prev char
		posBefore = model.pdoc->MovePositionOutsideChar(posBefore - 1, -1);
		numCharsToDraw = posAfter - posBefore;
		offsetFirstChar = offset - (posCaret - posBefore);
	}

	// See if the next character shares horizontal space, if so we'll
	// need to draw that too.
	if (offsetFirstChar < 0)
		offsetFirstChar = 0;
	numCharsToDraw = offsetLastChar - offsetFirstChar;
	while ((offsetLastChar < ll->LineStart(subLine + 1)) && (offsetLastChar <= ll->numCharsInLine)) {
		// Update posAfter to point to the 2nd next char, this is where
		// the next character ends, and 2nd next begins. We'll need
		// to compare these two
		posBefore = posAfter;
		posAfter = model.pdoc->MovePositionOutsideChar(posAfter + 1, 1);
		offsetLastChar = offset + (posAfter - posCaret);
		if ((ll->positions[offsetLastChar] - ll->positions[offsetLastChar - (posAfter - posBefore)]) > 0) {
			// The char does not share horizontal space
			break;
		}
		// Char shares horizontal space, update the numChars to draw
		numCharsToDraw = offsetLastChar - offsetFirstChar;
	}

	// We now know what to draw, update the caret drawing rectangle
	rcCaret.left = ll->positions[offsetFirstChar] - ll->positions[lineStart] + xStart;
	rcCaret.right = ll->positions[offsetFirstChar + numCharsToDraw] - ll->positions[lineStart] + xStart;

	// Adjust caret position to take into account any word wrapping symbols.
	if ((ll->wrapIndent != 0) && (lineStart != 0)) {
		const XYPOSITION wordWrapCharWidth = ll->wrapIndent;
		rcCaret.left += wordWrapCharWidth;
		rcCaret.right += wordWrapCharWidth;
	}

	// This character is where the caret block is, we override the colours
	// (inversed) for drawing the caret here.
	const int styleMain = ll->styles[offsetFirstChar];
	const Font *fontText = vsDraw.styles[styleMain].font.get();
	const std::string_view text(&ll->chars[offsetFirstChar], numCharsToDraw);
	surface->DrawTextClipped(rcCaret, fontText,
		rcCaret.top + vsDraw.maxAscent, text, vsDraw.styles[styleMain].back,
		caretColour);
}

}

void EditView::DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line lineDoc, int xStart, PRectangle rcLine, int subLine) const {
	// When drag is active it is the only caret drawn
	const bool drawDrag = model.posDrag.IsValid();
	if (!vsDraw.selection.visible && !drawDrag)
		return;
	const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
	// For each selection draw
	for (size_t r = 0; (r<model.sel.Count()) || drawDrag; r++) {
		const bool mainCaret = r == model.sel.Main();
		SelectionPosition posCaret = (drawDrag ? model.posDrag : model.sel.Range(r).caret);
		if ((vsDraw.DrawCaretInsideSelection(model.inOverstrike, imeCaretBlockOverride)) &&
			!drawDrag &&
			posCaret > model.sel.Range(r).anchor) {
			if (posCaret.VirtualSpace() > 0)
				posCaret.SetVirtualSpace(posCaret.VirtualSpace() - 1);
			else
				posCaret.SetPosition(model.pdoc->MovePositionOutsideChar(posCaret.Position()-1, -1));
		}
		const int offset = static_cast<int>(posCaret.Position() - posLineStart);
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		const XYPOSITION virtualOffset = posCaret.VirtualSpace() * spaceWidth;
		if (ll->InLine(offset, subLine) && offset <= ll->numCharsBeforeEOL) {
			XYPOSITION xposCaret = ll->positions[offset] + virtualOffset - ll->positions[ll->LineStart(subLine)];
			if (model.BidirectionalEnabled() && (posCaret.VirtualSpace() == 0)) {
				// Get caret point
				const ScreenLine screenLine(ll, subLine, vsDraw, rcLine.right, tabWidthMinimumPixels);

				const int caretPosition = offset - ll->LineStart(subLine);

				std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);
				const XYPOSITION caretLeft = slLayout->XFromPosition(caretPosition);

				// In case of start of line, the cursor should be at the right
				xposCaret = caretLeft + virtualOffset;
			}
			if (ll->wrapIndent != 0) {
				const Sci::Position lineStart = ll->LineStart(subLine);
				if (lineStart != 0)	// Wrapped
					xposCaret += ll->wrapIndent;
			}
			const bool caretBlinkState = (model.caret.active && model.caret.on) || (!additionalCaretsBlink && !mainCaret);
			const bool caretVisibleState = additionalCaretsVisible || mainCaret;
			if ((xposCaret >= 0) && vsDraw.IsCaretVisible(mainCaret) &&
				(drawDrag || (caretBlinkState && caretVisibleState))) {
				bool canDrawBlockCaret = true;
				bool drawBlockCaret = false;
				XYPOSITION widthOverstrikeCaret;
				XYPOSITION caretWidthOffset = 0;
				PRectangle rcCaret = rcLine;

				if (posCaret.Position() == model.pdoc->Length()) {   // At end of document
					canDrawBlockCaret = false;
					widthOverstrikeCaret = vsDraw.aveCharWidth;
				} else if ((posCaret.Position() - posLineStart) >= ll->numCharsInLine) {	// At end of line
					canDrawBlockCaret = false;
					widthOverstrikeCaret = vsDraw.aveCharWidth;
				} else {
					const int widthChar = model.pdoc->LenChar(posCaret.Position());
					widthOverstrikeCaret = ll->positions[offset + widthChar] - ll->positions[offset];
				}
				if (widthOverstrikeCaret < 3)	// Make sure its visible
					widthOverstrikeCaret = 3;

				if (xposCaret > 0)
					caretWidthOffset = 0.51f;	// Move back so overlaps both character cells.
				xposCaret += xStart;
				const ViewStyle::CaretShape caretShape = drawDrag ? ViewStyle::CaretShape::line :
					vsDraw.CaretShapeForMode(model.inOverstrike, mainCaret);
				if (drawDrag) {
					/* Dragging text, use a line caret */
					rcCaret.left = std::round(xposCaret - caretWidthOffset);
					rcCaret.right = rcCaret.left + vsDraw.caret.width;
				} else if ((caretShape == ViewStyle::CaretShape::bar) && drawOverstrikeCaret) {
					/* Over-strike (insert mode), use a modified bar caret */
					rcCaret.top = rcCaret.bottom - 2;
					rcCaret.left = xposCaret + 1;
					rcCaret.right = rcCaret.left + widthOverstrikeCaret - 1;
				} else if ((caretShape == ViewStyle::CaretShape::block) || imeCaretBlockOverride) {
					/* Block caret */
					rcCaret.left = xposCaret;
					if (canDrawBlockCaret && !(IsControl(ll->chars[offset]))) {
						drawBlockCaret = true;
						rcCaret.right = xposCaret + widthOverstrikeCaret;
					} else {
						rcCaret.right = xposCaret + vsDraw.aveCharWidth;
					}
				} else {
					/* Line caret */
					rcCaret.left = std::round(xposCaret - caretWidthOffset);
					rcCaret.right = rcCaret.left + vsDraw.caret.width;
				}
				const Element elementCaret = mainCaret ? Element::Caret : Element::CaretAdditional;
				const ColourRGBA caretColour = vsDraw.ElementColourForced(elementCaret);
				//assert(caretColour.IsOpaque());
				if (drawBlockCaret) {
					DrawBlockCaret(surface, model, vsDraw, ll, subLine, xStart, offset, posCaret.Position(), rcCaret, caretColour);
				} else {
					surface->FillRectangleAligned(rcCaret, Fill(caretColour));
				}
			}
		}
		if (drawDrag)
			break;
	}
}

namespace {

void DrawWrapIndentAndMarker(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, ColourOptional background, DrawWrapMarkerFn customDrawWrapMarker,
	bool caretActive) {
	// default background here..
	surface->FillRectangleAligned(rcLine, Fill(background.value_or(vsDraw.styles[StyleDefault].back)));

	if (vsDraw.IsLineFrameOpaque(caretActive, ll->containsCaret)) {
		// Draw left of frame under marker
		surface->FillRectangleAligned(Side(rcLine, Edge::left, vsDraw.GetFrameWidth()),
			vsDraw.ElementColourForced(Element::CaretLineBack).Opaque());
	}

	if (FlagSet(vsDraw.wrap.visualFlags, WrapVisualFlag::Start)) {

		// draw continuation rect
		PRectangle rcPlace = rcLine;

		rcPlace.left = static_cast<XYPOSITION>(xStart);
		rcPlace.right = rcPlace.left + ll->wrapIndent;

		if (FlagSet(vsDraw.wrap.visualFlagsLocation, WrapVisualLocation::StartByText))
			rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
		else
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;

		if (!customDrawWrapMarker) {
			DrawWrapMarker(surface, rcPlace, false, vsDraw.WrapColour());
		} else {
			customDrawWrapMarker(surface, rcPlace, false, vsDraw.WrapColour());
		}
	}
}

// On the curses platform, the terminal is drawing its own caret, so if the caret is within
// the main selection, do not draw the selection at that position.
// Use iDoc from DrawBackground and DrawForeground here because TextSegment has been adjusted
// such that, if the caret is inside the main selection, the beginning or end of that selection
// is at the end of a text segment.
// This function should only be called if iDoc is within the main selection.
InSelection CharacterInCursesSelection(Sci::Position iDoc, const EditModel &model, const ViewStyle &vsDraw) noexcept {
	const SelectionPosition &posCaret = model.sel.RangeMain().caret;
	const bool caretAtStart = posCaret < model.sel.RangeMain().anchor && posCaret.Position() == iDoc;
	const bool caretAtEnd = posCaret > model.sel.RangeMain().anchor &&
		vsDraw.DrawCaretInsideSelection(false, false) &&
		model.pdoc->MovePositionOutsideChar(posCaret.Position() - 1, -1) == iDoc;
	return (caretAtStart || caretAtEnd) ? InSelection::inNone : InSelection::inMain;
}

void DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, int subLine, Range lineRange, Sci::Position posLineStart,
	ColourOptional background) {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	const XYPOSITION subLineStart = ll->positions[lineRange.start];
	const XYPOSITION horizontalOffset = xStart - subLineStart;
	// Does not take margin into account but not significant
	const XYPOSITION xStartVisible = subLineStart - xStart;

	const BreakFinder::BreakFor breakFor = selBackDrawn ? BreakFinder::BreakFor::Selection : BreakFinder::BreakFor::Text;
	BreakFinder bfBack(ll, &model.sel, lineRange, posLineStart, xStartVisible, breakFor, model.pdoc, model.reprs.get(), &vsDraw);

	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background;

	// Background drawing loop
	while (bfBack.More()) {

		const TextSegment ts = bfBack.Next();
		const Sci::Position i = ts.end() - 1;
		const Sci::Position iDoc = i + posLineStart;

		const Interval horizontal = ll->Span(ts.start, ts.end()).Offset(horizontalOffset);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (!horizontal.Empty() && rcLine.Intersects(horizontal)) {
			const PRectangle rcSegment = Intersection(rcLine, horizontal);

			InSelection inSelection = vsDraw.selection.visible ? model.sel.CharacterInSelection(iDoc) : InSelection::inNone;
			if (FlagSet(vsDraw.caret.style, CaretStyle::Curses) && (inSelection == InSelection::inMain))
				inSelection = CharacterInCursesSelection(iDoc, model, vsDraw);
			const bool inHotspot = model.hotspot.Valid() && model.hotspot.ContainsCharacter(iDoc);
			ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, inSelection,
				inHotspot, ll->styles[i], i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation)) {
						textBack = vsDraw.ElementColourForced(Element::WhiteSpaceBack).Opaque();
					}
				} else {
					// Blob display
					inIndentation = false;
				}
				surface->FillRectangleAligned(rcSegment, Fill(textBack));
			} else {
				// Normal text display
				surface->FillRectangleAligned(rcSegment, Fill(textBack));
				if (vsDraw.viewWhitespace != WhiteSpace::Invisible) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation)) {
								const PRectangle rcSpace = Intersection(rcLine,
									ll->SpanByte(cpos + ts.start).Offset(horizontalOffset));
								surface->FillRectangleAligned(rcSpace,
									vsDraw.ElementColourForced(Element::WhiteSpaceBack).Opaque());
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
		} else if (horizontal.left > rcLine.right) {
			break;
		}
	}
}

void DrawEdgeLine(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, Range lineRange) {
	if (vsDraw.edgeState == EdgeVisualStyle::Line) {
		PRectangle rcSegment = rcLine;
		const int edgeX = static_cast<int>(vsDraw.theEdge.column * vsDraw.spaceWidth);
		rcSegment.left = static_cast<XYPOSITION>(edgeX + xStart);
		if ((ll->wrapIndent != 0) && (lineRange.start != 0))
			rcSegment.left -= ll->wrapIndent;
		rcSegment.right = rcSegment.left + 1;
		surface->FillRectangleAligned(rcSegment, Fill(vsDraw.theEdge.colour));
	} else if (vsDraw.edgeState == EdgeVisualStyle::MultiLine) {
		for (size_t edge = 0; edge < vsDraw.theMultiEdge.size(); edge++) {
			if (vsDraw.theMultiEdge[edge].column >= 0) {
				PRectangle rcSegment = rcLine;
				const int edgeX = static_cast<int>(vsDraw.theMultiEdge[edge].column * vsDraw.spaceWidth);
				rcSegment.left = static_cast<XYPOSITION>(edgeX + xStart);
				if ((ll->wrapIndent != 0) && (lineRange.start != 0))
					rcSegment.left -= ll->wrapIndent;
				rcSegment.right = rcSegment.left + 1;
				surface->FillRectangleAligned(rcSegment, Fill(vsDraw.theMultiEdge[edge].colour));
			}
		}
	}
}

// Draw underline mark as part of background if on base layer
void DrawMarkUnderline(Surface *surface, const EditModel &model, const ViewStyle &vsDraw,
	Sci::Line line, PRectangle rcLine) {
	int marks = model.GetMark(line);
	for (int markBit = 0; (markBit <= MarkerMax) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == MarkerSymbol::Underline) &&
			(vsDraw.markers[markBit].layer == Layer::Base)) {
			PRectangle rcUnderline = rcLine;
			rcUnderline.top = rcUnderline.bottom - 2;
			surface->FillRectangleAligned(rcUnderline, Fill(vsDraw.markers[markBit].back));
		}
		marks >>= 1;
	}
}

void DrawTranslucentSelection(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, Range lineRange, int tabWidthMinimumPixels, Layer layer) {
	if (vsDraw.selection.layer == layer) {
		const Sci::Position posLineStart = model.pdoc->LineStart(line);
		const XYPOSITION subLineStart = ll->positions[lineRange.start];
		const XYPOSITION horizontalOffset = xStart - subLineStart;
		// For each selection draw
		Sci::Position virtualSpaces = 0;
		if (subLine == (ll->lines - 1)) {
			virtualSpaces = model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line));
		}
		const SelectionPosition posStart(posLineStart + lineRange.start);
		const SelectionPosition posEnd(posLineStart + lineRange.end, virtualSpaces);
		const SelectionSegment virtualSpaceRange(posStart, posEnd);
		for (size_t r = 0; r < model.sel.Count(); r++) {
			const SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
			if (!portion.Empty()) {
				const SelectionSegment portionInLine = portion.Subtract(posLineStart);
				const ColourRGBA selectionBack = SelectionBackground(
					model, vsDraw, model.sel.RangeType(r));
				const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
				const Interval intervalVirtual{ portion.start.VirtualSpace() * spaceWidth, portion.end.VirtualSpace() * spaceWidth };
				if (model.BidirectionalEnabled()) {
					const SelectionSegment portionInSubLine = portionInLine.Subtract(lineRange.start);

					const ScreenLine screenLine(ll, subLine, vsDraw, rcLine.right, tabWidthMinimumPixels);
					std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);

					if (slLayout) {
						const std::vector<Interval> intervals = slLayout->FindRangeIntervals(
							portionInSubLine.start.Position(), portionInSubLine.end.Position());
						for (const Interval &interval : intervals) {
							const PRectangle rcSelection = rcLine.WithHorizontalBounds(interval.Offset(xStart));
							surface->FillRectangleAligned(rcSelection, selectionBack);
						}
					}

					if (portion.end.VirtualSpace()) {
						const XYPOSITION xStartVirtual = ll->positions[lineRange.end] + horizontalOffset;
						const PRectangle rcSegment = rcLine.WithHorizontalBounds(intervalVirtual.Offset(xStartVirtual));
						surface->FillRectangleAligned(rcSegment, selectionBack);
					}
				} else {
					Interval intervalSegment = ll->Span(
						static_cast<int>(portionInLine.start.Position()),
						static_cast<int>(portionInLine.end.Position()))
						.Offset(horizontalOffset);
					intervalSegment.left += intervalVirtual.left;
					intervalSegment.right += intervalVirtual.right;
					if ((ll->wrapIndent != 0) && (lineRange.start != 0)) {
						if ((portionInLine.start.Position() == lineRange.start) &&
							model.sel.Range(r).ContainsCharacter(portion.start.Position() - 1))
							intervalSegment.left -= static_cast<int>(ll->wrapIndent); // indentation added to xStart was truncated to int, so we do the same here
					}
					const PRectangle rcSegment = Intersection(rcLine, intervalSegment);
					if (rcSegment.right > rcLine.left)
						surface->FillRectangleAligned(rcSegment, selectionBack);
				}
			}
		}
	}
}

void DrawCaretLineFramed(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, int subLine) {
	const ColourOptional caretlineBack = vsDraw.ElementColour(Element::CaretLineBack);
	if (!caretlineBack) {
		return;
	}

	const ColourRGBA colourFrame = (vsDraw.caretLine.layer == Layer::Base) ?
		caretlineBack->Opaque() : *caretlineBack;

	const int width = vsDraw.GetFrameWidth();

	// Avoid double drawing the corners by removing the left and right sides when drawing top and bottom borders
	const PRectangle rcWithoutLeftRight = rcLine.Inset(Point(width, 0.0));

	if (subLine == 0 || ll->wrapIndent == 0 || vsDraw.caretLine.layer != Layer::Base || vsDraw.caretLine.subLine) {
		// Left
		surface->FillRectangleAligned(Side(rcLine, Edge::left, width), colourFrame);
	}
	if (subLine == 0 || vsDraw.caretLine.subLine) {
		// Top
		surface->FillRectangleAligned(Side(rcWithoutLeftRight, Edge::top, width), colourFrame);
	}
	if (subLine == ll->lines - 1 || vsDraw.caretLine.layer != Layer::Base || vsDraw.caretLine.subLine) {
		// Right
		surface->FillRectangleAligned(Side(rcLine, Edge::right, width), colourFrame);
	}
	if (subLine == ll->lines - 1 || vsDraw.caretLine.subLine) {
		// Bottom
		surface->FillRectangleAligned(Side(rcWithoutLeftRight, Edge::bottom, width), colourFrame);
	}
}

// Draw any translucent whole line states
void DrawTranslucentLineState(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine, Layer layer) {
	if ((model.caret.active || vsDraw.caretLine.alwaysShow) && vsDraw.ElementColour(Element::CaretLineBack) && ll->containsCaret &&
		vsDraw.caretLine.layer == layer) {
		if (vsDraw.caretLine.frame) {
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		} else {
			surface->FillRectangleAligned(rcLine, vsDraw.ElementColourForced(Element::CaretLineBack));
		}
	}
	const int marksOfLine = model.GetMark(line);
	int marksDrawnInText = marksOfLine & vsDraw.maskDrawInText;
	for (int markBit = 0; (markBit <= MarkerMax) && marksDrawnInText; markBit++) {
		if ((marksDrawnInText & 1) && (vsDraw.markers[markBit].layer == layer)) {
			if (vsDraw.markers[markBit].markType == MarkerSymbol::Background) {
				surface->FillRectangleAligned(rcLine, vsDraw.markers[markBit].BackWithAlpha());
			} else if (vsDraw.markers[markBit].markType == MarkerSymbol::Underline) {
				PRectangle rcUnderline = rcLine;
				rcUnderline.top = rcUnderline.bottom - 2;
				surface->FillRectangleAligned(rcUnderline, vsDraw.markers[markBit].BackWithAlpha());
			}
		}
		marksDrawnInText >>= 1;
	}
	int marksDrawnInLine = marksOfLine & vsDraw.maskInLine;
	for (int markBit = 0; (markBit <= MarkerMax) && marksDrawnInLine; markBit++) {
		if ((marksDrawnInLine & 1) && (vsDraw.markers[markBit].layer == layer)) {
			surface->FillRectangleAligned(rcLine, vsDraw.markers[markBit].BackWithAlpha());
		}
		marksDrawnInLine >>= 1;
	}
}

void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid,
	const ViewStyle &vsDraw, Stroke stroke) {

	const XYPOSITION halfWidth = stroke.width / 2.0;

	const XYPOSITION leftStroke = std::round(std::min(rcTab.left + 2, rcTab.right - 1)) + halfWidth;
	const XYPOSITION rightStroke = std::max(leftStroke, std::round(rcTab.right) - 1.0f - halfWidth);
	const XYPOSITION yMidAligned = ymid + halfWidth;
	const Point arrowPoint(rightStroke, yMidAligned);
	if (rightStroke > leftStroke) {
		// When not enough room, don't draw the arrow shaft
		surface->LineDraw(Point(leftStroke, yMidAligned), arrowPoint, stroke);
	}

	// Draw the arrow head if needed
	if (vsDraw.tabDrawMode == TabDrawMode::LongArrow) {
		XYPOSITION ydiff = std::floor(rcTab.Height() / 2.0f);
		XYPOSITION xhead = rightStroke - ydiff;
		if (xhead <= rcTab.left) {
			ydiff -= rcTab.left - xhead;
			xhead = rcTab.left;
		}
		const Point ptsHead[] = {
			Point(xhead, yMidAligned - ydiff),
			arrowPoint,
			Point(xhead, yMidAligned + ydiff)
		};
		surface->PolyLine(ptsHead, std::size(ptsHead), stroke);
	}
}

void DrawIndicator(int indicNum, Sci::Position startPos, Sci::Position endPos, Surface *surface, const ViewStyle &vsDraw,
	const LineLayout *ll, int xStart, PRectangle rcLine, Sci::Position secondCharacter, int subLine, Indicator::State state,
	int value, bool bidiEnabled, int tabWidthMinimumPixels) {

	const XYPOSITION subLineStart = ll->positions[ll->LineStart(subLine)];
	const XYPOSITION horizontalOffset = xStart - subLineStart;

	std::vector<PRectangle> rectangles;

	const XYPOSITION left = ll->XInLine(startPos) + horizontalOffset;
	const XYPOSITION right = ll->XInLine(endPos) + horizontalOffset;
	const PRectangle rcIndic(left, rcLine.top + vsDraw.maxAscent, right,
		std::max(rcLine.top + vsDraw.maxAscent + 3, rcLine.bottom));

	if (bidiEnabled) {
		ScreenLine screenLine(ll, subLine, vsDraw, rcLine.right - xStart, tabWidthMinimumPixels);
		const Range lineRange = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);

		std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);
		std::vector<Interval> intervals = slLayout->FindRangeIntervals(
			startPos - lineRange.start, endPos - lineRange.start);
		for (const Interval &interval : intervals) {
			PRectangle rcInterval = rcIndic;
			rcInterval.left = interval.left + xStart;
			rcInterval.right = interval.right + xStart;
			rectangles.push_back(rcInterval);
		}
	} else {
		rectangles.push_back(rcIndic);
	}

	for (const PRectangle &rc : rectangles) {
		PRectangle rcFirstCharacter = rc;
		// Allow full descent space for character indicators
		rcFirstCharacter.bottom = rcLine.top + vsDraw.maxAscent + vsDraw.maxDescent;
		if (secondCharacter >= 0) {
			rcFirstCharacter.right = ll->XInLine(secondCharacter) + horizontalOffset;
		} else {
			// Indicator continued from earlier line so make an empty box and don't draw
			rcFirstCharacter.right = rcFirstCharacter.left;
		}
		vsDraw.indicators[indicNum].Draw(surface, rc, rcLine, rcFirstCharacter, state, value);
	}
}

void DrawIndicators(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, Sci::Position lineEnd, bool under, int tabWidthMinimumPixels) {
	// Draw decorators
	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	const Sci::Position lineStart = ll->LineStart(subLine);
	const Sci::Position posLineEnd = posLineStart + lineEnd;

	for (const IDecoration *deco : model.pdoc->decorations->View()) {
		if (under == vsDraw.indicators[deco->Indicator()].under) {
			Sci::Position startPos = posLineStart + lineStart;
			while (startPos < posLineEnd) {
				const Range rangeRun(deco->StartRun(startPos), deco->EndRun(startPos));
				const Sci::Position endPos = std::min(rangeRun.end, posLineEnd);
				const int value = deco->ValueAt(startPos);
				if (value) {
					const bool hover = vsDraw.indicators[deco->Indicator()].IsDynamic() &&
						rangeRun.ContainsCharacter(model.hoverIndicatorPos);
					const Indicator::State state = hover ? Indicator::State::hover : Indicator::State::normal;
					const Sci::Position posSecond = model.pdoc->MovePositionOutsideChar(rangeRun.First() + 1, 1);
					DrawIndicator(deco->Indicator(), startPos - posLineStart, endPos - posLineStart,
						surface, vsDraw, ll, xStart, rcLine, posSecond - posLineStart, subLine, state,
						value, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				}
				startPos = endPos;
			}
		}
	}

	// Use indicators to highlight matching braces
	if ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == StyleBraceLight)) ||
		(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == StyleBraceBad))) {
		const int braceIndicator = (model.bracesMatchStyle == StyleBraceLight) ? vsDraw.braceHighlightIndicator : vsDraw.braceBadLightIndicator;
		if (under == vsDraw.indicators[braceIndicator].under) {
			const Range rangeLine(posLineStart + lineStart, posLineEnd);
			for (size_t brace = 0; brace <= 1; brace++) {
				if (rangeLine.ContainsCharacter(model.braces[brace])) {
					const Sci::Position braceOffset = model.braces[brace] - posLineStart;
					if (braceOffset < ll->numCharsInLine) {
						const Sci::Position braceEnd = model.pdoc->MovePositionOutsideChar(model.braces[brace] + 1, 1) - posLineStart;
						DrawIndicator(braceIndicator, braceOffset, braceEnd,
							surface, vsDraw, ll, xStart, rcLine, braceEnd, subLine, Indicator::State::normal,
							1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
					}
				}
			}
		}
	}

	if (FlagSet(model.changeHistoryOption, ChangeHistoryOption::Indicators)) {
		// Draw editions
		constexpr int indexHistory = static_cast<int>(IndicatorNumbers::HistoryRevertedToOriginInsertion);
		{
			// Draw insertions
			Sci::Position startPos = posLineStart + lineStart;
			while (startPos < posLineEnd) {
				const Range rangeRun(startPos, model.pdoc->EditionEndRun(startPos));
				const Sci::Position endPos = std::min(rangeRun.end, posLineEnd);
				const int edition = model.pdoc->EditionAt(startPos);
				if (edition != 0) {
					const int indicator = (edition - 1) * 2 + indexHistory;
					const Sci::Position posSecond = model.pdoc->MovePositionOutsideChar(rangeRun.First() + 1, 1);
					DrawIndicator(indicator, startPos - posLineStart, endPos - posLineStart,
						surface, vsDraw, ll, xStart, rcLine, posSecond - posLineStart, subLine, Indicator::State::normal,
						1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				}
				startPos = endPos;
			}
		}
		{
			// Draw deletions
			Sci::Position startPos = posLineStart + lineStart;
			while (startPos <= posLineEnd) {
				const unsigned int editions = model.pdoc->EditionDeletesAt(startPos);
				const Sci::Position posSecond = model.pdoc->MovePositionOutsideChar(startPos + 1, 1);
				for (unsigned int edition = 0; edition < 4; edition++) {
					if (editions & (1 << edition)) {
						const int indicator = edition * 2 + indexHistory + 1;
						DrawIndicator(indicator, startPos - posLineStart, posSecond - posLineStart,
							surface, vsDraw, ll, xStart, rcLine, posSecond - posLineStart, subLine, Indicator::State::normal,
							1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
					}
				}
				startPos = model.pdoc->EditionNextDelete(startPos);
			}
		}
	}
}

void DrawFoldLines(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine) {
	const bool lastSubLine = subLine == (ll->lines - 1);
	const bool expanded = model.pcs->GetExpanded(line);
	const FoldLevel level = model.pdoc->GetFoldLevel(line);
	const FoldLevel levelNext = model.pdoc->GetFoldLevel(line + 1);
	if (LevelIsHeader(level) &&
		(LevelNumber(level) < LevelNumber(levelNext))) {
		const ColourRGBA foldLineColour = vsDraw.ElementColour(Element::FoldLine).value_or(
			vsDraw.styles[StyleDefault].fore);
		// Paint the line above the fold
		if ((subLine == 0) && FlagSet(model.foldFlags, (expanded ? FoldFlag::LineBeforeExpanded: FoldFlag::LineBeforeContracted))) {
			surface->FillRectangleAligned(Side(rcLine, Edge::top, 1.0), foldLineColour);
		}
		// Paint the line below the fold
		if (lastSubLine && FlagSet(model.foldFlags, (expanded ? FoldFlag::LineAfterExpanded : FoldFlag::LineAfterContracted))) {
			surface->FillRectangleAligned(Side(rcLine, Edge::bottom, 1.0), foldLineColour);
			// If contracted fold line drawn then don't overwrite with hidden line
			// as fold lines are more specific then hidden lines.
			if (!expanded) {
				return;
			}
		}
	}
	if (lastSubLine && model.pcs->GetVisible(line) && !model.pcs->GetVisible(line + 1)) {
		if (const ColourOptional hiddenLineColour = vsDraw.ElementColour(Element::HiddenLine)) {
			surface->FillRectangleAligned(Side(rcLine, Edge::bottom, 1.0), *hiddenLineColour);
		}
	}
}

ColourRGBA InvertedLight(ColourRGBA orig) noexcept {
	unsigned int r = orig.GetRed();
	unsigned int g = orig.GetGreen();
	unsigned int b = orig.GetBlue();
	const unsigned int l = (r + g + b) / 3; 	// There is a better calculation for this that matches human eye
	const unsigned int il = 0xff - l;
	if (l == 0)
		return white;
	r = r * il / l;
	g = g * il / l;
	b = b * il / l;
	return ColourRGBA(std::min(r, 0xffu), std::min(g, 0xffu), std::min(b, 0xffu));
}

}

void EditView::DrawIndentGuide(Surface *surface, XYPOSITION start, PRectangle rcSegment, bool highlight, bool offset) {
	const Point from = Point::FromInts(0, offset ? 1 : 0);
	const PRectangle rcCopyArea(start + 1, rcSegment.top,
		start + 2, rcSegment.bottom);
	surface->Copy(rcCopyArea, from,
		highlight ? *pixmapIndentGuideHighlight : *pixmapIndentGuide);
}

void EditView::DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, int subLine, Sci::Line lineVisible, Range lineRange, Sci::Position posLineStart,
	ColourOptional background) {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background;
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.

	const XYPOSITION subLineStart = ll->positions[lineRange.start];
	const XYPOSITION horizontalOffset = xStart - subLineStart;
	const XYPOSITION indentWidth = model.pdoc->IndentSize() * vsDraw.spaceWidth;

	// Does not take margin into account but not significant
	const XYPOSITION xStartVisible = subLineStart - xStart;

	// When lineHeight is odd, dotted indent guides are drawn offset by 1 on odd lines to join together.
	const bool offsetGuide = (lineVisible & 1) && (vsDraw.lineHeight & 1);

	// Same baseline used for all text
	const XYPOSITION ybase = rcLine.top + vsDraw.maxAscent;

	// Foreground drawing loop
	const BreakFinder::BreakFor breakFor = (((phasesDraw == PhasesDraw::One) && selBackDrawn) || vsDraw.SelectionTextDrawn())
		? BreakFinder::BreakFor::ForegroundAndSelection : BreakFinder::BreakFor::Foreground;
	BreakFinder bfFore(ll, &model.sel, lineRange, posLineStart, xStartVisible, breakFor, model.pdoc, model.reprs.get(), &vsDraw);

	while (bfFore.More()) {

		const TextSegment ts = bfFore.Next();
		const Sci::Position i = ts.end() - 1;
		const Sci::Position iDoc = i + posLineStart;

		const Interval horizontal = ll->Span(ts.start, ts.end()).Offset(horizontalOffset);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (rcLine.Intersects(horizontal)) {
			const PRectangle rcSegment = rcLine.WithHorizontalBounds(horizontal);
			const int styleMain = ll->styles[i];
			ColourRGBA textFore = vsDraw.styles[styleMain].fore;
			const Font *textFont = vsDraw.styles[styleMain].font.get();
			// Hot-spot foreground
			const bool inHotspot = model.hotspot.Valid() && model.hotspot.ContainsCharacter(iDoc);
			if (inHotspot) {
				if (const ColourOptional colourHotSpot = vsDraw.ElementColour(Element::HotSpotActive)) {
					textFore = *colourHotSpot;
				}
			}
			if (vsDraw.indicatorsSetFore) {
				// At least one indicator sets the text colour so see if it applies to this segment
				for (const IDecoration *deco : model.pdoc->decorations->View()) {
					const int indicatorValue = deco->ValueAt(ts.start + posLineStart);
					if (indicatorValue) {
						const Indicator &indicator = vsDraw.indicators[deco->Indicator()];
						bool hover = false;
						if (indicator.IsDynamic()) {
							const Sci::Position startPos = ts.start + posLineStart;
							const Range rangeRun(deco->StartRun(startPos), deco->EndRun(startPos));
							hover =	rangeRun.ContainsCharacter(model.hoverIndicatorPos);
						}
						if (hover) {
							if (indicator.sacHover.style == IndicatorStyle::TextFore || (indicator.sacHover.style == IndicatorStyle::ExplorerLink)) {
								textFore = indicator.sacHover.fore;
							}
						} else {
							if (indicator.sacNormal.style == IndicatorStyle::TextFore) {
								if (FlagSet(indicator.Flags(), IndicFlag::ValueFore))
									textFore = ColourRGBA::FromRGB(indicatorValue & static_cast<int>(IndicValue::Mask));
								else
									textFore = indicator.sacNormal.fore;
							}
						}
					}
				}
			}
			InSelection inSelection = vsDraw.selection.visible ? model.sel.CharacterInSelection(iDoc) : InSelection::inNone;
			if (FlagSet(vsDraw.caret.style, CaretStyle::Curses) && (inSelection == InSelection::inMain))
				inSelection = CharacterInCursesSelection(iDoc, model, vsDraw);
			if (const ColourOptional selectionFore = SelectionForeground(model, vsDraw, inSelection)) {
				textFore = *selectionFore;
			}
			ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, inSelection, inHotspot, styleMain, i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (phasesDraw == PhasesDraw::One) {
						if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation))
							textBack = vsDraw.ElementColourForced(Element::WhiteSpaceBack).Opaque();
						surface->FillRectangleAligned(rcSegment, Fill(textBack));
					}
					if (inIndentation && vsDraw.viewIndentationGuides == IndentView::Real) {
						const Interval intervalCharacter = ll->SpanByte(static_cast<int>(i));
						for (int indentCount = static_cast<int>((intervalCharacter.left + epsilon) / indentWidth);
							indentCount <= (intervalCharacter.right - epsilon) / indentWidth;
							indentCount++) {
							if (indentCount > 0) {
								const XYPOSITION xIndent = std::floor(indentCount * indentWidth);
								DrawIndentGuide(surface, xIndent + xStart, rcSegment, ll->xHighlightGuide == xIndent, offsetGuide);
							}
						}
					}
					if (vsDraw.viewWhitespace != WhiteSpace::Invisible) {
						if (vsDraw.WhiteSpaceVisible(inIndentation)) {
							const PRectangle rcTab(rcSegment.left + 1, rcSegment.top + tabArrowHeight,
								rcSegment.right - 1, rcSegment.bottom - vsDraw.maxDescent);
							const int segmentTop = static_cast<int>(rcSegment.top) + vsDraw.lineHeight / 2;
							const ColourRGBA whiteSpaceFore = vsDraw.ElementColour(Element::WhiteSpace).value_or(textFore);
							if (!customDrawTabArrow)
								DrawTabArrow(surface, rcTab, segmentTop, vsDraw, Stroke(whiteSpaceFore, 1.0f));
							else
								customDrawTabArrow(surface, rcTab, segmentTop, vsDraw, Stroke(whiteSpaceFore, 1.0f));
						}
					}
				} else {
					inIndentation = false;
					if (vsDraw.controlCharSymbol >= 32) {
						// Using one font for all control characters so it can be controlled independently to ensure
						// the box goes around the characters tightly. Seems to be no way to work out what height
						// is taken by an individual character - internal leading gives varying results.
						const Font *ctrlCharsFont = vsDraw.styles[StyleControlChar].font.get();
						const char cc[2] = { static_cast<char>(vsDraw.controlCharSymbol), '\0' };
						surface->DrawTextNoClip(rcSegment, ctrlCharsFont,
							ybase, cc, textBack, textFore);
					} else {
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Colour)) {
							textFore = ts.representation->colour;
						}
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Blob)) {
							DrawTextBlob(surface, vsDraw, rcSegment, ts.representation->stringRep,
								textBack, textFore, phasesDraw == PhasesDraw::One);
						} else {
							surface->DrawTextTransparentUTF8(rcSegment, vsDraw.styles[StyleControlChar].font.get(),
								ybase, ts.representation->stringRep, textFore);
						}
					}
				}
			} else {
				// Normal text display
				if (vsDraw.styles[styleMain].visible) {
					const std::string_view text(&ll->chars[ts.start], i - ts.start + 1);
					if (phasesDraw != PhasesDraw::One) {
						surface->DrawTextTransparent(rcSegment, textFont,
							ybase, text, textFore);
					} else {
						surface->DrawTextNoClip(rcSegment, textFont,
							ybase, text, textFore, textBack);
					}
				} else if (vsDraw.styles[styleMain].invisibleRepresentation[0]) {
					const std::string_view text = vsDraw.styles[styleMain].invisibleRepresentation;
  					if (phasesDraw != PhasesDraw::One) {
						surface->DrawTextTransparentUTF8(rcSegment, textFont,
							ybase, text, textFore);
					} else {
						surface->DrawTextNoClipUTF8(rcSegment, textFont,
							ybase, text, textFore, textBack);
					}
				}
				if (vsDraw.viewWhitespace != WhiteSpace::Invisible ||
					(inIndentation && vsDraw.viewIndentationGuides != IndentView::None)) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (vsDraw.viewWhitespace != WhiteSpace::Invisible) {
								if (vsDraw.WhiteSpaceVisible(inIndentation)) {
									const Interval intervalSpace = ll->SpanByte(cpos + ts.start).Offset(horizontalOffset);
									const XYPOSITION xmid = (intervalSpace.left + intervalSpace.right) / 2;
									if ((phasesDraw == PhasesDraw::One) && drawWhitespaceBackground) {
										textBack = vsDraw.ElementColourForced(Element::WhiteSpaceBack).Opaque();
										const PRectangle rcSpace = rcLine.WithHorizontalBounds(intervalSpace);
										surface->FillRectangleAligned(rcSpace, Fill(textBack));
									}
									const int halfDotWidth = vsDraw.whitespaceSize / 2;
									PRectangle rcDot(xmid - halfDotWidth,
										rcSegment.top + vsDraw.lineHeight / 2, 0.0f, 0.0f);
									rcDot.right = rcDot.left + vsDraw.whitespaceSize;
									rcDot.bottom = rcDot.top + vsDraw.whitespaceSize;
									const ColourRGBA whiteSpaceFore = vsDraw.ElementColour(Element::WhiteSpace).value_or(textFore);
									surface->FillRectangleAligned(rcDot, Fill(whiteSpaceFore));
								}
							}
							if (inIndentation && vsDraw.viewIndentationGuides == IndentView::Real) {
								const Interval intervalCharacter = ll->SpanByte(cpos + ts.start);
								for (int indentCount = static_cast<int>((intervalCharacter.left + epsilon) / indentWidth);
									indentCount <= (intervalCharacter.right - epsilon) / indentWidth;
									indentCount++) {
									if (indentCount > 0) {
										const XYPOSITION xIndent = std::floor(indentCount * indentWidth);
										DrawIndentGuide(surface, xIndent + xStart, rcSegment, ll->xHighlightGuide == xIndent, offsetGuide);
									}
								}
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
			if ((inHotspot && vsDraw.hotspotUnderline) || vsDraw.styles[styleMain].underline) {
				PRectangle rcUL = rcSegment;
				rcUL.top = ybase + 1;
				rcUL.bottom = ybase + 2;
				ColourRGBA colourUnderline = textFore;
				if (inHotspot && vsDraw.hotspotUnderline) {
					colourUnderline = vsDraw.ElementColour(Element::HotSpotActive).value_or(textFore);
				}
				surface->FillRectangleAligned(rcUL, colourUnderline);
			}
		} else if (horizontal.left > rcLine.right) {
			break;
		}
	}
}

void EditView::DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, Sci::Line lineVisible) {
	if ((vsDraw.viewIndentationGuides == IndentView::LookForward || vsDraw.viewIndentationGuides == IndentView::LookBoth)
		&& (subLine == 0)) {
		const Sci::Position posLineStart = model.pdoc->LineStart(line);
		int indentSpace = model.pdoc->GetLineIndentation(line);
		int xStartText = static_cast<int>(ll->positions[model.pdoc->GetLineIndentPosition(line) - posLineStart]);

		// Find the most recent line with some text

		Sci::Line lineLastWithText = line;
		while (lineLastWithText > std::max(line - 20, static_cast<Sci::Line>(0)) && model.pdoc->IsWhiteLine(lineLastWithText)) {
			lineLastWithText--;
		}
		if (lineLastWithText < line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of last line with text
			int indentLastWithText = model.pdoc->GetLineIndentation(lineLastWithText);
			const int isFoldHeader = LevelIsHeader(model.pdoc->GetFoldLevel(lineLastWithText));
			if (isFoldHeader) {
				// Level is one more level than parent
				indentLastWithText += model.pdoc->IndentSize();
			}
			if (vsDraw.viewIndentationGuides == IndentView::LookForward) {
				// In viLookForward mode, previous line only used if it is a fold header
				if (isFoldHeader) {
					indentSpace = std::max(indentSpace, indentLastWithText);
				}
			} else {	// viLookBoth
				indentSpace = std::max(indentSpace, indentLastWithText);
			}
		}

		Sci::Line lineNextWithText = line;
		while (lineNextWithText < std::min(line + 20, model.pdoc->LinesTotal()) && model.pdoc->IsWhiteLine(lineNextWithText)) {
			lineNextWithText++;
		}
		if (lineNextWithText > line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of first next line with text
			indentSpace = std::max(indentSpace,
				model.pdoc->GetLineIndentation(lineNextWithText));
		}

		const bool offsetGuide = (lineVisible & 1) && (vsDraw.lineHeight & 1);
		for (int indentPos = model.pdoc->IndentSize(); indentPos < indentSpace; indentPos += model.pdoc->IndentSize()) {
			const XYPOSITION xIndent = std::floor(indentPos * vsDraw.spaceWidth);
			if (xIndent < xStartText) {
				DrawIndentGuide(surface, xIndent + xStart, rcLine,	ll->xHighlightGuide == xIndent, offsetGuide);
			}
		}
	}
}

void EditView::DrawLine(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, Sci::Line lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase) {

	if (subLine >= ll->lines) {
		DrawAnnotation(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, phase);
		return; // No further drawing
	}

	const bool clipLine = !bufferedDraw && !LinesOverlap();
	if (clipLine) {
		surface->SetClip(rcLine);
	}

	// See if something overrides the line background colour.
	const ColourOptional background = vsDraw.Background(model.GetMark(line), model.caret.active, ll->containsCaret);

	const Sci::Position posLineStart = model.pdoc->LineStart(line);

	const Range lineRange = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);
	const Range lineRangeIncludingEnd = ll->SubLineRange(subLine, LineLayout::Scope::includeEnd);
	const XYPOSITION subLineStart = ll->positions[lineRange.start];

	if ((ll->wrapIndent != 0) && (subLine > 0)) {
		if (FlagSet(phase, DrawPhase::back)) {
			DrawWrapIndentAndMarker(surface, vsDraw, ll, xStart, rcLine, background, customDrawWrapMarker, model.caret.active);
		}
		xStart += static_cast<int>(ll->wrapIndent);
	}

	if (phasesDraw != PhasesDraw::One) {
		if (FlagSet(phase, DrawPhase::back)) {
			DrawBackground(surface, model, vsDraw, ll,
				xStart, rcLine, subLine, lineRange, posLineStart,
				background);
			DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, DrawPhase::back);
			DrawEOLAnnotationText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, DrawPhase::back);
			// Remove drawBack to not draw again in DrawFoldDisplayText
			phase = static_cast<DrawPhase>(static_cast<int>(phase) & ~static_cast<int>(DrawPhase::back));
			DrawEOL(surface, model, vsDraw, ll,
				line, xStart, rcLine, subLine, lineRange.end, subLineStart, background);
			if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
				DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		}

		if (FlagSet(phase, DrawPhase::indicatorsBack)) {
			DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
				lineRangeIncludingEnd.end, true, tabWidthMinimumPixels);
			DrawEdgeLine(surface, vsDraw, ll, xStart, rcLine, lineRange);
			DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
		}
	}

	if (FlagSet(phase, DrawPhase::text)) {
		if (vsDraw.selection.visible) {
			DrawTranslucentSelection(surface, model, vsDraw, ll,
				line, xStart, rcLine, subLine, lineRange, tabWidthMinimumPixels, Layer::UnderText);
		}
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine, subLine, Layer::UnderText);
		DrawForeground(surface, model, vsDraw, ll,
			xStart, rcLine, subLine, lineVisible, lineRange, posLineStart,
			background);
	}

	if (FlagSet(phase, DrawPhase::indentationGuides)) {
		DrawIndentGuidesOverEmpty(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, lineVisible);
	}

	if (FlagSet(phase, DrawPhase::indicatorsFore)) {
		DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
			lineRangeIncludingEnd.end, false, tabWidthMinimumPixels);
	}

	DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, phase);
	DrawEOLAnnotationText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, phase);

	if (phasesDraw == PhasesDraw::One) {
		DrawEOL(surface, model, vsDraw, ll,
			line, xStart, rcLine, subLine, lineRange.end, subLineStart, background);
		if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		DrawEdgeLine(surface, vsDraw, ll, xStart, rcLine, lineRange);
		DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
	}

	if (vsDraw.selection.visible && FlagSet(phase, DrawPhase::selectionTranslucent)) {
		DrawTranslucentSelection(surface, model, vsDraw, ll,
			line, xStart, rcLine, subLine, lineRange, tabWidthMinimumPixels, Layer::OverText);
	}

	if (FlagSet(phase, DrawPhase::lineTranslucent)) {
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine, subLine, Layer::OverText);
	}

	if (clipLine) {
		surface->PopClip();
	}
}

void EditView::PaintText(Surface *surfaceWindow, const EditModel &model, const ViewStyle &vsDraw,
	PRectangle rcArea, PRectangle rcClient) {
	// Allow text at start of line to overlap 1 pixel into the margin as this displays
	// serifs and italic stems for aliased text.
	const int leftTextOverlap = ((model.xOffset == 0) && (vsDraw.leftMarginWidth > 0)) ? 1 : 0;

	// Do the painting
	if (rcArea.right > vsDraw.textStart - leftTextOverlap) {

		Surface *surface = surfaceWindow;
		if (bufferedDraw) {
			surface = pixmapLine.get();
			PLATFORM_ASSERT(pixmapLine->Initialised());
		}
		surface->SetMode(model.CurrentSurfaceMode());

		const Point ptOrigin = model.GetVisibleOriginInMain();

		const int screenLinePaintFirst = static_cast<int>(rcArea.top) / vsDraw.lineHeight;
		const int xStart = vsDraw.textStart - model.xOffset + static_cast<int>(ptOrigin.x);

		const SelectionPosition posCaret = model.posDrag.IsValid() ? model.posDrag : model.sel.RangeMain().caret;
		const Sci::Line lineCaret = model.pdoc->SciLineFromPosition(posCaret.Position());
		const int caretOffset = static_cast<int>(posCaret.Position() - model.pdoc->LineStart(lineCaret));

		PRectangle rcTextArea = rcClient;
		if (vsDraw.marginInside) {
			rcTextArea.left += vsDraw.textStart;
			rcTextArea.right -= vsDraw.rightMarginWidth;
		} else {
			rcTextArea = rcArea;
		}

		// Remove selection margin from drawing area so text will not be drawn
		// on it in unbuffered mode.
		const bool clipping = !bufferedDraw && vsDraw.marginInside;
		if (clipping) {
			PRectangle rcClipText = rcTextArea;
			rcClipText.left -= leftTextOverlap;
			surfaceWindow->SetClip(rcClipText);
		}

		// Loop on visible lines
#if defined(TIME_PAINTING)
		double durLayout = 0.0;
		double durPaint = 0.0;
		double durCopy = 0.0;
		ElapsedPeriod epWhole;
#endif
		const bool bracesIgnoreStyle = ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == StyleBraceLight)) ||
			(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == StyleBraceBad)));

		Sci::Line lineDocPrevious = -1;	// Used to avoid laying out one document line multiple times
		std::shared_ptr<LineLayout> ll;
		DrawPhase phase = DrawPhase::all;
		if ((phasesDraw == PhasesDraw::Multiple) && !bufferedDraw) {
			phase = DrawPhase::back;
		}
		for (;;) {
			int yposScreen = screenLinePaintFirst * vsDraw.lineHeight;
			int ypos = bufferedDraw ? 0 : yposScreen;
			Sci::Line visibleLine = model.TopLineOfMain() + screenLinePaintFirst;
			while (visibleLine < model.pcs->LinesDisplayed() && yposScreen < rcArea.bottom) {

				const Sci::Line lineDoc = model.pcs->DocFromDisplay(visibleLine);
				// Only visible lines should be handled by the code within the loop
				PLATFORM_ASSERT(model.pcs->GetVisible(lineDoc));
				const Sci::Line lineStartSet = model.pcs->DisplayFromDoc(lineDoc);
				const int subLine = static_cast<int>(visibleLine - lineStartSet);

				// Copy this line and its styles from the document into local arrays
				// and determine the x position at which each character starts.
#if defined(TIME_PAINTING)
				ElapsedPeriod ep;
#endif
				if (lineDoc != lineDocPrevious) {
					ll = RetrieveLineLayout(lineDoc, model);
					LayoutLine(model, surface, vsDraw, ll.get(), model.wrapWidth);
					lineDocPrevious = lineDoc;
					if (ll && model.BidirectionalEnabled()) {
						// Fill the line bidi data
						UpdateBidiData(model, vsDraw, ll.get());
					}
				}
#if defined(TIME_PAINTING)
				durLayout += ep.Duration(true);
#endif
				if (ll) {
					ll->containsCaret = vsDraw.selection.visible && (lineDoc == lineCaret)
						&& (ll->lines == 1 || !vsDraw.caretLine.subLine || ll->InLine(caretOffset, subLine));

					PRectangle rcLine = rcTextArea;
					rcLine.top = static_cast<XYPOSITION>(ypos);
					rcLine.bottom = static_cast<XYPOSITION>(ypos + vsDraw.lineHeight);

					const Range rangeLine(model.pdoc->LineStart(lineDoc),
						model.pdoc->LineStart(lineDoc + 1));

					// Highlight the current braces if any
					ll->SetBracesHighlight(rangeLine, model.braces, static_cast<char>(model.bracesMatchStyle),
						static_cast<int>(model.highlightGuideColumn * vsDraw.spaceWidth), bracesIgnoreStyle);

					if (leftTextOverlap && (bufferedDraw || ((phasesDraw < PhasesDraw::Multiple) && (FlagSet(phase, DrawPhase::back))))) {
						// Clear the left margin
						PRectangle rcSpacer = rcLine;
						rcSpacer.right = rcSpacer.left;
						rcSpacer.left -= 1;
						surface->FillRectangleAligned(rcSpacer, Fill(vsDraw.styles[StyleDefault].back));
					}

					DrawLine(surface, model, vsDraw, ll.get(), lineDoc, visibleLine, xStart, rcLine, subLine, phase);
#if defined(TIME_PAINTING)
					durPaint += ep.Duration(true);
#endif
					// Restore the previous styles for the brace highlights in case layout is in cache.
					ll->RestoreBracesHighlight(rangeLine, model.braces, bracesIgnoreStyle);

					if (FlagSet(phase, DrawPhase::foldLines)) {
						DrawFoldLines(surface, model, vsDraw, ll.get(), lineDoc, rcLine, subLine);
					}

					if (FlagSet(phase, DrawPhase::carets)) {
						DrawCarets(surface, model, vsDraw, ll.get(), lineDoc, xStart, rcLine, subLine);
					}

					if (bufferedDraw) {
						const Point from = Point::FromInts(vsDraw.textStart - leftTextOverlap, 0);
						const PRectangle rcCopyArea = PRectangle::FromInts(vsDraw.textStart - leftTextOverlap, yposScreen,
							static_cast<int>(rcClient.right - vsDraw.rightMarginWidth),
							yposScreen + vsDraw.lineHeight);
						pixmapLine->FlushDrawing();
						surfaceWindow->Copy(rcCopyArea, from, *pixmapLine);
					}

					lineWidthMaxSeen = std::max(
						lineWidthMaxSeen, static_cast<int>(ll->positions[ll->numCharsInLine]));
#if defined(TIME_PAINTING)
					durCopy += ep.Duration(true);
#endif
				}

				if (!bufferedDraw) {
					ypos += vsDraw.lineHeight;
				}

				yposScreen += vsDraw.lineHeight;
				visibleLine++;
			}

			if (phase >= DrawPhase::carets) {
				break;
			}
			phase = static_cast<DrawPhase>(static_cast<int>(phase) * 2);
		}
		ll.reset();
#if defined(TIME_PAINTING)
		if (durPaint < 0.00000001)
			durPaint = 0.00000001;
#endif
		// Right column limit indicator
		PRectangle rcBeyondEOF = (vsDraw.marginInside) ? rcClient : rcArea;
		rcBeyondEOF.left = static_cast<XYPOSITION>(vsDraw.textStart);
		rcBeyondEOF.right = rcBeyondEOF.right - ((vsDraw.marginInside) ? vsDraw.rightMarginWidth : 0);
		rcBeyondEOF.top = static_cast<XYPOSITION>((model.pcs->LinesDisplayed() - model.TopLineOfMain()) * vsDraw.lineHeight);
		if (rcBeyondEOF.top < rcBeyondEOF.bottom) {
			surfaceWindow->FillRectangleAligned(rcBeyondEOF, Fill(vsDraw.styles[StyleDefault].back));
			if (vsDraw.edgeState == EdgeVisualStyle::Line) {
				const int edgeX = static_cast<int>(vsDraw.theEdge.column * vsDraw.spaceWidth);
				rcBeyondEOF.left = static_cast<XYPOSITION>(edgeX + xStart);
				rcBeyondEOF.right = rcBeyondEOF.left + 1;
				surfaceWindow->FillRectangleAligned(rcBeyondEOF, Fill(vsDraw.theEdge.colour));
			} else if (vsDraw.edgeState == EdgeVisualStyle::MultiLine) {
				for (size_t edge = 0; edge < vsDraw.theMultiEdge.size(); edge++) {
					if (vsDraw.theMultiEdge[edge].column >= 0) {
						const int edgeX = static_cast<int>(vsDraw.theMultiEdge[edge].column * vsDraw.spaceWidth);
						rcBeyondEOF.left = static_cast<XYPOSITION>(edgeX + xStart);
						rcBeyondEOF.right = rcBeyondEOF.left + 1;
						surfaceWindow->FillRectangleAligned(rcBeyondEOF, Fill(vsDraw.theMultiEdge[edge].colour));
					}
				}
			}
		}

		if (clipping)
			surfaceWindow->PopClip();

		//Platform::DebugPrintf("start display %d, offset = %d\n", model.pdoc->Length(), model.xOffset);
#if defined(TIME_PAINTING)
		Platform::DebugPrintf(
		"Layout:%9.6g    Paint:%9.6g    Ratio:%9.6g   Copy:%9.6g   Total:%9.6g\n",
		durLayout, durPaint, durLayout / durPaint, durCopy, epWhole.Duration());
#endif
	}
}

// Space (3 space characters) between line numbers and text when printing.
#define lineNumberPrintSpace "   "

Sci::Position EditView::FormatRange(bool draw, CharacterRangeFull chrg, Rectangle rc, Surface *surface, Surface *surfaceMeasure,
	const EditModel &model, const ViewStyle &vs) {
	// Can't use measurements cached for screen
	posCache->Clear();

	ViewStyle vsPrint(vs);
	vsPrint.technology = Technology::Default;

	// Modify the view style for printing as do not normally want any of the transient features to be printed
	// Printing supports only the line number margin.
	int lineNumberIndex = -1;
	for (size_t margin = 0; margin < vs.ms.size(); margin++) {
		if ((vsPrint.ms[margin].style == MarginType::Number) && (vsPrint.ms[margin].width > 0)) {
			lineNumberIndex = static_cast<int>(margin);
		} else {
			vsPrint.ms[margin].width = 0;
		}
	}
	vsPrint.fixedColumnWidth = 0;
	vsPrint.zoomLevel = printParameters.magnification;
	// Don't show indentation guides
	// If this ever gets changed, cached pixmap would need to be recreated if technology != Technology::Default
	vsPrint.viewIndentationGuides = IndentView::None;
	// Don't show the selection when printing
	vsPrint.selection.visible = false;
	vsPrint.elementColours.clear();
	vsPrint.elementBaseColours.clear();
	vsPrint.caretLine.alwaysShow = false;
	// Don't highlight matching braces using indicators
	vsPrint.braceHighlightIndicatorSet = false;
	vsPrint.braceBadLightIndicatorSet = false;

	// Set colours for printing according to users settings
	const PrintOption colourMode = printParameters.colourMode;
	const std::vector<Style>::iterator endStyles = (colourMode == PrintOption::ColourOnWhiteDefaultBG) ?
		vsPrint.styles.begin() + StyleLineNumber : vsPrint.styles.end();
	for (std::vector<Style>::iterator it = vsPrint.styles.begin(); it != endStyles; ++it) {
		if (colourMode == PrintOption::InvertLight) {
			it->fore = InvertedLight(it->fore);
			it->back = InvertedLight(it->back);
		} else if (colourMode == PrintOption::BlackOnWhite) {
			it->fore = black;
			it->back = white;
		} else if (colourMode == PrintOption::ColourOnWhite || colourMode == PrintOption::ColourOnWhiteDefaultBG) {
			it->back = white;
		}
	}
	// White background for the line numbers if PrintOption::ScreenColours isn't used
	if (colourMode != PrintOption::ScreenColours) {
		vsPrint.styles[StyleLineNumber].back = white;
	}

	// Printing uses different margins, so reset screen margins
	vsPrint.leftMarginWidth = 0;
	vsPrint.rightMarginWidth = 0;

	vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);
	// Determining width must happen after fonts have been realised in Refresh
	int lineNumberWidth = 0;
	if (lineNumberIndex >= 0) {
		lineNumberWidth = static_cast<int>(surfaceMeasure->WidthText(vsPrint.styles[StyleLineNumber].font.get(),
			"99999" lineNumberPrintSpace));
		vsPrint.ms[lineNumberIndex].width = lineNumberWidth;
		vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);	// Recalculate fixedColumnWidth
	}

	// Turn off change history marker backgrounds
	constexpr unsigned int changeMarkers =
		1u << static_cast<unsigned int>(MarkerOutline::HistoryRevertedToOrigin) |
		1u << static_cast<unsigned int>(MarkerOutline::HistorySaved) |
		1u << static_cast<unsigned int>(MarkerOutline::HistoryModified) |
		1u << static_cast<unsigned int>(MarkerOutline::HistoryRevertedToModified);
	vsPrint.maskInLine &= ~changeMarkers;

	const Sci::Line linePrintStart = model.pdoc->SciLineFromPosition(chrg.cpMin);
	Sci::Line linePrintLast = linePrintStart + (rc.bottom - rc.top) / vsPrint.lineHeight - 1;
	if (linePrintLast < linePrintStart)
		linePrintLast = linePrintStart;
	const Sci::Line linePrintMax = model.pdoc->SciLineFromPosition(chrg.cpMax);
	if (linePrintLast > linePrintMax)
		linePrintLast = linePrintMax;
	//Platform::DebugPrintf("Formatting lines=[%0d,%0d,%0d] top=%0d bottom=%0d line=%0d %0d\n",
	//      linePrintStart, linePrintLast, linePrintMax, rc.top, rc.bottom, vsPrint.lineHeight,
	//      surfaceMeasure->Height(vsPrint.styles[StyleLineNumber].font));
	Sci::Position endPosPrint = model.pdoc->Length();
	if (linePrintLast < model.pdoc->LinesTotal())
		endPosPrint = model.pdoc->LineStart(linePrintLast + 1);

	// Ensure we are styled to where we are formatting.
	model.pdoc->EnsureStyledTo(endPosPrint);

	const int xStart = vsPrint.fixedColumnWidth + rc.left;
	int ypos = rc.top;

	Sci::Line lineDoc = linePrintStart;

	Sci::Position nPrintPos = chrg.cpMin;
	int visibleLine = 0;
	int widthPrint = rc.right - rc.left - vsPrint.fixedColumnWidth;
	if (printParameters.wrapState == Wrap::None)
		widthPrint = LineLayout::wrapWidthInfinite;

	while (lineDoc <= linePrintLast && ypos < rc.bottom) {

		// When printing, the hdc and hdcTarget may be the same, so
		// changing the state of surfaceMeasure may change the underlying
		// state of surface. Therefore, any cached state is discarded before
		// using each surface.
		surfaceMeasure->FlushCachedState();

		// Copy this line and its styles from the document into local arrays
		// and determine the x position at which each character starts.
		LineLayout ll(lineDoc, static_cast<int>(model.pdoc->LineStart(lineDoc + 1) - model.pdoc->LineStart(lineDoc) + 1));
		LayoutLine(model, surfaceMeasure, vsPrint, &ll, widthPrint);

		ll.containsCaret = false;

		PRectangle rcLine = PRectangle::FromInts(
			rc.left,
			ypos,
			rc.right - 1,
			ypos + vsPrint.lineHeight);

		// When document line is wrapped over multiple display lines, find where
		// to start printing from to ensure a particular position is on the first
		// line of the page.
		if (visibleLine == 0) {
			const Sci::Position startWithinLine = nPrintPos -
				model.pdoc->LineStart(lineDoc);
			for (int iwl = 0; iwl < ll.lines - 1; iwl++) {
				if (ll.LineStart(iwl) <= startWithinLine && ll.LineStart(iwl + 1) >= startWithinLine) {
					visibleLine = -iwl;
				}
			}

			if (ll.lines > 1 && startWithinLine >= ll.LineStart(ll.lines - 1)) {
				visibleLine = -(ll.lines - 1);
			}
		}

		if (draw && lineNumberWidth &&
			(ypos + vsPrint.lineHeight <= rc.bottom) &&
			(visibleLine >= 0)) {
			const std::string number = std::to_string(lineDoc + 1) + lineNumberPrintSpace;
			PRectangle rcNumber = rcLine;
			rcNumber.right = rcNumber.left + lineNumberWidth;
			// Right justify
			rcNumber.left = rcNumber.right - surfaceMeasure->WidthText(
				vsPrint.styles[StyleLineNumber].font.get(), number);
			surface->FlushCachedState();
			surface->DrawTextNoClip(rcNumber, vsPrint.styles[StyleLineNumber].font.get(),
				ypos + vsPrint.maxAscent, number,
				vsPrint.styles[StyleLineNumber].fore,
				vsPrint.styles[StyleLineNumber].back);
		}

		// Draw the line
		surface->FlushCachedState();

		for (int iwl = 0; iwl < ll.lines; iwl++) {
			if (ypos + vsPrint.lineHeight <= rc.bottom) {
				if (visibleLine >= 0) {
					if (draw) {
						rcLine.top = static_cast<XYPOSITION>(ypos);
						rcLine.bottom = static_cast<XYPOSITION>(ypos + vsPrint.lineHeight);
						DrawLine(surface, model, vsPrint, &ll, lineDoc, visibleLine, xStart, rcLine, iwl, DrawPhase::all);
					}
					ypos += vsPrint.lineHeight;
				}
				visibleLine++;
				if (iwl == ll.lines - 1)
					nPrintPos = model.pdoc->LineStart(lineDoc + 1);
				else
					nPrintPos += ll.LineStart(iwl + 1) - ll.LineStart(iwl);
			}
		}

		++lineDoc;
	}

	// Clear cache so measurements are not used for screen
	posCache->Clear();

	return nPrintPos;
}
