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

static int WidthStyledText(Surface *surface, const ViewStyle &vs, int styleOffset,
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

void Hexits(char *hexits, int ch) noexcept {
	hexits[0] = 'x';
	hexits[1] = "0123456789ABCDEF"[ch / 0x10];
	hexits[2] = "0123456789ABCDEF"[ch % 0x10];
	hexits[3] = 0;
}

}

EditView::EditView() {
	tabWidthMinimumPixels = 2; // needed for calculating tab stops for fractional proportional fonts
	hideSelection = false;
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

static const char *ControlCharacterString(unsigned char ch) noexcept {
	const char * const reps[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
	};
	if (ch < std::size(reps)) {
		return reps[ch];
	} else {
		return "BAD";
	}
}

static void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid,
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

bool ViewIsASCII(std::string_view text) {
	return std::all_of(text.cbegin(), text.cend(), IsASCII);
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
		if (vstyle.styles[ll->styles[ts.start]].visible) {
			if (ts.representation) {
				XYPOSITION representationWidth = vstyle.controlCharWidth;
				if (ll->chars[ts.start] == '\t') {
					// Tab is a special case of representation, taking a variable amount of space
					// which will be filled in later.
					representationWidth = 0;
				} else {
					if (representationWidth <= 0.0) {
						assert(ts.representation->stringRep.length() <= Representation::maxLength);
						XYPOSITION positionsRepr[Representation::maxLength + 1];
						// ts.representation->stringRep is UTF-8 which only matches cache if document is UTF-8
						// or it only contains ASCII which is a subset of all currently supported encodings.
						if (textUnicode || ViewIsASCII(ts.representation->stringRep)) {
							pCache->MeasureWidths(surface, vstyle, StyleControlChar, ts.representation->stringRep,
								positionsRepr, multiThreaded);
						} else {
							surface->MeasureWidthsUTF8(vstyle.styles[StyleControlChar].font.get(), ts.representation->stringRep, positionsRepr);
						}
						representationWidth = positionsRepr[ts.representation->stringRep.length() - 1];
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Blob)) {
							representationWidth += vstyle.ctrlCharPadding;
						}
					}
				}
				for (int ii = 0; ii < ts.length; ii++) {
					ll->positions[ts.start + 1 + ii] = representationWidth;
				}
			} else {
				if ((ts.length == 1) && (' ' == ll->chars[ts.start])) {
					// Over half the segments are single characters and of these about half are space characters.
					ll->positions[ts.start + 1] = vstyle.styles[ll->styles[ts.start]].spaceWidth;
				} else {
					pCache->MeasureWidths(surface, vstyle, ll->styles[ts.start],
						std::string_view(&ll->chars[ts.start], ts.length), &ll->positions[ts.start + 1], multiThreaded);
				}
			}
		}
	}
}

}

/**
* Fill in the LineLayout data for the given line.
* Copy the given @a line and its styles from the document into local arrays.
* Also determine the x position at which each character starts.
*/
void EditView::LayoutLine(const EditModel &model, Surface *surface, const ViewStyle &vstyle, LineLayout *ll, int width) {
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
		BreakFinder bfLayout(ll, nullptr, Range(0, numCharsInLine), posLineStart, 0, BreakFinder::BreakFor::Text, model.pdoc, &model.reprs, nullptr);
		while (bfLayout.More()) {
			segments.push_back(bfLayout.Next());
		}

		std::fill(&ll->positions[0], &ll->positions[numCharsInLine], 0.0f);

		if (!segments.empty()) {

			const size_t threadsForLength = std::max(1, numCharsInLine / bytesPerLayoutThread);
			size_t threads = std::min<size_t>({ segments.size(), threadsForLength, maxLayoutThreads });
			if (!surface->SupportsFeature(Supports::ThreadSafeMeasureWidths)) {
				threads = 1;
			}

			std::atomic<uint32_t> nextIndex = 0;

			const bool textUnicode = CpUtf8 == model.pdoc->dbcsCodePage;
			const bool multiThreaded = threads > 1;
			IPositionCache *pCache = posCache.get();

			// If only 1 thread needed then use the main thread, else spin up multiple
			const std::launch policy = (multiThreaded) ? std::launch::async : std::launch::deferred;

			std::vector<std::future<void>> futures;
			for (size_t th = 0; th < threads; th++) {
				// Find relative positions of everything except for tabs
				std::future<void> fut = std::async(policy,
					[pCache, surface, &vstyle, &ll, &segments, &nextIndex, textUnicode, multiThreaded]() {
					LayoutSegments(pCache, surface, vstyle, ll, segments, nextIndex, textUnicode, multiThreaded);
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
			ll->lines = 0;
			// Calculate line start positions based upon width.
			Sci::Position lastLineStart = 0;
			XYACCUMULATOR startOffset = width;
			Sci::Position p = 0;
			const Wrap wrapState = vstyle.wrap.state;
			const Sci::Position numCharsInLine = ll->numCharsInLine;
			while (p < numCharsInLine) {
				while (p < numCharsInLine && ll->positions[p + 1] < startOffset) {
					p++;
				}
				if (p < numCharsInLine) {
					// backtrack to find lastGoodBreak
					Sci::Position lastGoodBreak = p;
					if (p > 0) {
						lastGoodBreak = model.pdoc->MovePositionOutsideChar(p + posLineStart, -1) - posLineStart;
					}
					if (wrapState != Wrap::Char) {
						Sci::Position pos = lastGoodBreak;
						while (pos > lastLineStart) {
							// style boundary and space
							if (wrapState != Wrap::WhiteSpace && (ll->styles[pos - 1] != ll->styles[pos])) {
								break;
							}
							if (IsBreakSpace(ll->chars[pos - 1]) && !IsBreakSpace(ll->chars[pos])) {
								break;
							}
							pos = model.pdoc->MovePositionOutsideChar(pos + posLineStart - 1, -1) - posLineStart;
						}
						if (pos > lastLineStart) {
							lastGoodBreak = pos;
						}
					}
					if (lastGoodBreak == lastLineStart) {
						// Try moving to start of last character
						if (p > 0) {
							lastGoodBreak = model.pdoc->MovePositionOutsideChar(p + posLineStart, -1) - posLineStart;
						}
						if (lastGoodBreak == lastLineStart) {
							// Ensure at least one character on line.
							lastGoodBreak = model.pdoc->MovePositionOutsideChar(lastGoodBreak + posLineStart + 1, 1) - posLineStart;
						}
					}
					lastLineStart = lastGoodBreak;
					ll->lines++;
					ll->SetLineStart(ll->lines, static_cast<int>(lastLineStart));
					startOffset = ll->positions[lastLineStart];
					// take into account the space for start wrap mark and indent
					startOffset += width - ll->wrapIndent;
					p = lastLineStart + 1;
				}
			}
			ll->lines++;
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
			const int charWidth = UTF8DrawBytes(reinterpret_cast<unsigned char *>(&ll->chars[charsInLine]), ll->numCharsInLine - charsInLine);
			const Representation *repr = model.reprs.RepresentationFromCharacter(std::string_view(&ll->chars[charsInLine], charWidth));

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
				positionInLine = slLayout->PositionFromX(static_cast<XYPOSITION>(pt.x), charPosition) +
					rangeSubLine.start;
			} else {
				positionInLine = ll->FindPositionFromX(static_cast<XYPOSITION>(pt.x + subLineStart),
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

constexpr ColourRGBA bugColour = ColourRGBA(0xff, 0, 0xff, 0xf0);

// Selection background colours are always defined, the value_or is to show if bug

ColourRGBA SelectionBackground(const EditModel &model, const ViewStyle &vsDraw, InSelection inSelection) {
	if (inSelection == InSelection::inNone)
		return bugColour;	// Not selected is a bug

	Element element = Element::SelectionBack;
	if (inSelection == InSelection::inAdditional)
		element = Element::SelectionAdditionalBack;
	if (!model.primarySelection)
		element = Element::SelectionSecondaryBack;
	if (!model.hasFocus && vsDraw.ElementColour(Element::SelectionInactiveBack))
		element = Element::SelectionInactiveBack;
	return vsDraw.ElementColour(element).value_or(bugColour);
}

std::optional<ColourRGBA> SelectionForeground(const EditModel &model, const ViewStyle &vsDraw, InSelection inSelection) {
	if (inSelection == InSelection::inNone)
		return {};
	Element element = Element::SelectionText;
	if (inSelection == InSelection::inAdditional)
		element = Element::SelectionAdditionalText;
	if (!model.primarySelection)	// Secondary selection
		element = Element::SelectionSecondaryText;
	if (!model.hasFocus) {
		if (vsDraw.ElementColour(Element::SelectionInactiveText)) {
			element = Element::SelectionInactiveText;
		} else {
			return {};
		}
	}
	return vsDraw.ElementColour(element);
}

}

static ColourRGBA TextBackground(const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	std::optional<ColourRGBA> background, InSelection inSelection, bool inHotspot, int styleMain, Sci::Position i) {
	if (inSelection && (vsDraw.selection.layer == Layer::Base)) {
		return SelectionBackground(model, vsDraw, inSelection).Opaque();
	}
	if ((vsDraw.edgeState == EdgeVisualStyle::Background) &&
		(i >= ll->edgeColumn) &&
		(i < ll->numCharsBeforeEOL))
		return vsDraw.theEdge.colour;
	if (inHotspot && vsDraw.ElementColour(Element::HotSpotActiveBack))
		return vsDraw.ElementColour(Element::HotSpotActiveBack)->Opaque();
	if (background && (styleMain != StyleBraceLight) && (styleMain != StyleBraceBad)) {
		return *background;
	} else {
		return vsDraw.styles[styleMain].back;
	}
}

void EditView::DrawIndentGuide(Surface *surface, Sci::Line lineVisible, int lineHeight, XYPOSITION start, PRectangle rcSegment, bool highlight) {
	const Point from = Point::FromInts(0, ((lineVisible & 1) && (lineHeight & 1)) ? 1 : 0);
	const PRectangle rcCopyArea(start + 1, rcSegment.top,
		start + 2, rcSegment.bottom);
	surface->Copy(rcCopyArea, from,
		highlight ? *pixmapIndentGuideHighlight : *pixmapIndentGuide);
}

static void DrawTextBlob(Surface *surface, const ViewStyle &vsDraw, PRectangle rcSegment,
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

static void DrawCaretLineFramed(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine, int subLine) {
	const std::optional<ColourRGBA> caretlineBack = vsDraw.ElementColour(Element::CaretLineBack);
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

void EditView::DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, Sci::Line line, Sci::Position lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
	std::optional<ColourRGBA> background) {

	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	PRectangle rcSegment = rcLine;

	const bool lastSubLine = subLine == (ll->lines - 1);
	XYPOSITION virtualSpace = 0;
	if (lastSubLine) {
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		virtualSpace = model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line)) * spaceWidth;
	}
	const XYPOSITION xEol = static_cast<XYPOSITION>(ll->positions[lineEnd] - subLineStart);

	// Fill the virtual space and show selections within it
	if (virtualSpace > 0.0f) {
		rcSegment.left = xEol + xStart;
		rcSegment.right = xEol + xStart + virtualSpace;
		const ColourRGBA backgroundFill = background.value_or(vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		surface->FillRectangleAligned(rcSegment, backgroundFill);
		if (!hideSelection && (vsDraw.selection.layer == Layer::Base)) {
			const SelectionSegment virtualSpaceRange(SelectionPosition(model.pdoc->LineEnd(line)),
				SelectionPosition(model.pdoc->LineEnd(line),
					model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line))));
			for (size_t r = 0; r<model.sel.Count(); r++) {
				const SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
				if (!portion.Empty()) {
					const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
					rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart)+portion.start.VirtualSpace() * spaceWidth;
					rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart)+portion.end.VirtualSpace() * spaceWidth;
					rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
					rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
					surface->FillRectangleAligned(rcSegment, Fill(
						SelectionBackground(model, vsDraw, model.sel.RangeType(r)).Opaque()));
				}
			}
		}
	}

	InSelection eolInSelection = InSelection::inNone;
	if (!hideSelection && lastSubLine) {
		eolInSelection = model.LineEndInSelection(line);
	}

	const ColourRGBA selectionBack = SelectionBackground(model, vsDraw, eolInSelection);

	// Draw the [CR], [LF], or [CR][LF] blobs if visible line ends are on
	XYPOSITION blobsWidth = 0;
	if (lastSubLine) {
		for (Sci::Position eolPos = ll->numCharsBeforeEOL; eolPos<ll->numCharsInLine;) {
			const int styleMain = ll->styles[eolPos];
			const std::optional<ColourRGBA> selectionFore = SelectionForeground(model, vsDraw, eolInSelection);
			ColourRGBA textFore = selectionFore.value_or(vsDraw.styles[styleMain].fore);
			char hexits[4] = "";
			std::string_view ctrlChar;
			Sci::Position widthBytes = 1;
			RepresentationAppearance appearance = RepresentationAppearance::Blob;
			const Representation *repr = model.reprs.RepresentationFromCharacter(std::string_view(&ll->chars[eolPos], ll->numCharsInLine - eolPos));
			if (repr) {
				// Representation of whole text
				widthBytes = ll->numCharsInLine - eolPos;
			} else {
				repr = model.reprs.RepresentationFromCharacter(std::string_view(&ll->chars[eolPos], 1));
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

			rcSegment.left = xStart + ll->positions[eolPos] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
			rcSegment.right = xStart + ll->positions[eolPos + widthBytes] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
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
				vsDraw.ElementColour(Element::CaretLineBack)->Opaque());
		}
	}

	if (drawWrapMarkEnd) {
		PRectangle rcPlace = rcSegment;

		if (FlagSet(vsDraw.wrap.visualFlagsLocation, WrapVisualLocation::EndByText)) {
			rcPlace.left = xEol + xStart + virtualSpace;
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;
		} else {
			// rcLine is clipped to text area
			rcPlace.right = rcLine.right;
			rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
		}
		if (!customDrawWrapMarker) {
			DrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		} else {
			customDrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		}
	}
}

static void DrawIndicator(int indicNum, Sci::Position startPos, Sci::Position endPos, Surface *surface, const ViewStyle &vsDraw,
	const LineLayout *ll, int xStart, PRectangle rcLine, Sci::Position secondCharacter, int subLine, Indicator::State state,
	int value, bool bidiEnabled, int tabWidthMinimumPixels) {

	const XYPOSITION subLineStart = ll->positions[ll->LineStart(subLine)];

	std::vector<PRectangle> rectangles;

	const PRectangle rcIndic(
		ll->positions[startPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent,
		ll->positions[endPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent + 3);

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
			rcFirstCharacter.right = ll->positions[secondCharacter] + xStart - subLineStart;
		} else {
			// Indicator continued from earlier line so make an empty box and don't draw
			rcFirstCharacter.right = rcFirstCharacter.left;
		}
		vsDraw.indicators[indicNum].Draw(surface, rc, rcLine, rcFirstCharacter, state, value);
	}
}

static void DrawIndicators(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, Sci::Position lineEnd, bool under, int tabWidthMinimumPixels) {
	// Draw decorators
	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	const Sci::Position lineStart = ll->LineStart(subLine);
	const Sci::Position posLineEnd = posLineStart + lineEnd;

	for (const IDecoration *deco : model.pdoc->decorations->View()) {
		if (under == vsDraw.indicators[deco->Indicator()].under) {
			Sci::Position startPos = posLineStart + lineStart;
			if (!deco->ValueAt(startPos)) {
				startPos = deco->EndRun(startPos);
			}
			while ((startPos < posLineEnd) && (deco->ValueAt(startPos))) {
				const Range rangeRun(deco->StartRun(startPos), deco->EndRun(startPos));
				const Sci::Position endPos = std::min(rangeRun.end, posLineEnd);
				const bool hover = vsDraw.indicators[deco->Indicator()].IsDynamic() &&
					rangeRun.ContainsCharacter(model.hoverIndicatorPos);
				const int value = deco->ValueAt(startPos);
				const Indicator::State state = hover ? Indicator::State::hover : Indicator::State::normal;
				const Sci::Position posSecond = model.pdoc->MovePositionOutsideChar(rangeRun.First() + 1, 1);
				DrawIndicator(deco->Indicator(), startPos - posLineStart, endPos - posLineStart,
					surface, vsDraw, ll, xStart, rcLine, posSecond - posLineStart, subLine, state,
					value, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				startPos = endPos;
				if (!deco->ValueAt(startPos)) {
					startPos = deco->EndRun(startPos);
				}
			}
		}
	}

	// Use indicators to highlight matching braces
	if ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == StyleBraceLight)) ||
		(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == StyleBraceBad))) {
		const int braceIndicator = (model.bracesMatchStyle == StyleBraceLight) ? vsDraw.braceHighlightIndicator : vsDraw.braceBadLightIndicator;
		if (under == vsDraw.indicators[braceIndicator].under) {
			const Range rangeLine(posLineStart + lineStart, posLineEnd);
			if (rangeLine.ContainsCharacter(model.braces[0])) {
				const Sci::Position braceOffset = model.braces[0] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					const Sci::Position secondOffset = model.pdoc->MovePositionOutsideChar(model.braces[0] + 1, 1) - posLineStart;
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, secondOffset,
						subLine, Indicator::State::normal, 1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				}
			}
			if (rangeLine.ContainsCharacter(model.braces[1])) {
				const Sci::Position braceOffset = model.braces[1] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					const Sci::Position secondOffset = model.pdoc->MovePositionOutsideChar(model.braces[1] + 1, 1) - posLineStart;
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, secondOffset,
						subLine, Indicator::State::normal, 1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				}
			}
		}
	}
}

void EditView::DrawFoldDisplayText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
							  Sci::Line line, int xStart, PRectangle rcLine, int subLine, XYACCUMULATOR subLineStart, DrawPhase phase) {
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
	if (!hideSelection) {
		eolInSelection = model.LineEndInSelection(line);
	}

	const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
	const XYPOSITION virtualSpace = model.sel.VirtualSpaceFor(
		model.pdoc->LineEnd(line)) * spaceWidth;
	rcSegment.left = xStart + static_cast<XYPOSITION>(ll->positions[ll->numCharsInLine] - subLineStart) + virtualSpace + vsDraw.aveCharWidth;
	rcSegment.right = rcSegment.left + static_cast<XYPOSITION>(widthFoldDisplayText);

	const std::optional<ColourRGBA> background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);
	const std::optional<ColourRGBA> selectionFore = SelectionForeground(model, vsDraw, eolInSelection);
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

void EditView::DrawEOLAnnotationText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, Sci::Line line, int xStart, PRectangle rcLine, int subLine, XYACCUMULATOR subLineStart, DrawPhase phase) {

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
		static_cast<XYPOSITION>(ll->positions[ll->numCharsInLine] - subLineStart)
		+ virtualSpace + vsDraw.aveCharWidth;

	const char *textFoldDisplay = model.GetFoldDisplayText(line);
	if (textFoldDisplay) {
		const std::string_view foldDisplayText(textFoldDisplay);
		rcSegment.left += static_cast<int>(
			surface->WidthText(vsDraw.styles[StyleFoldDisplayText].font.get(), foldDisplayText)) +
			vsDraw.aveCharWidth;
	}
	rcSegment.right = rcSegment.left + static_cast<XYPOSITION>(widthEOLAnnotationText);

	const std::optional<ColourRGBA> background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);
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
		if (vsDraw.eolAnnotationVisible >= EOLAnnotationVisible::Boxed) {
			PRectangle rcBox = rcSegment;
			rcBox.left = std::round(rcSegment.left);
			rcBox.right = std::round(rcSegment.right);
			if (vsDraw.eolAnnotationVisible == EOLAnnotationVisible::Boxed) {
				surface->RectangleFrame(rcBox, Stroke(textFore));
			} else {
				if (phasesDraw == PhasesDraw::One) {
					// Draw an outline around the text
					surface->Stadium(rcBox, FillStroke(ColourRGBA(textBack, 0), textFore, 1.0), ends);
				} else {
					// Draw with a fill to fill the edges of the shape.
					surface->Stadium(rcBox, FillStroke(textBack, textFore, 1.0), ends);
				}
			}
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

static constexpr bool AnnotationBoxedOrIndented(AnnotationVisible annotationVisible) noexcept {
	return annotationVisible == AnnotationVisible::Boxed || annotationVisible == AnnotationVisible::Indented;
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
	}
}

static void DrawBlockCaret(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
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

void EditView::DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line lineDoc, int xStart, PRectangle rcLine, int subLine) const {
	// When drag is active it is the only caret drawn
	const bool drawDrag = model.posDrag.IsValid();
	if (hideSelection && !drawDrag)
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
				const ColourRGBA caretColour = *vsDraw.ElementColour(elementCaret);
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

static void DrawWrapIndentAndMarker(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, std::optional<ColourRGBA> background, DrawWrapMarkerFn customDrawWrapMarker,
	bool caretActive) {
	// default bgnd here..
	surface->FillRectangleAligned(rcLine, Fill(background ? *background :
		vsDraw.styles[StyleDefault].back));

	if (vsDraw.IsLineFrameOpaque(caretActive, ll->containsCaret)) {
		// Draw left of frame under marker
		surface->FillRectangleAligned(Side(rcLine, Edge::left, vsDraw.GetFrameWidth()),
			vsDraw.ElementColour(Element::CaretLineBack)->Opaque());
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
static InSelection CharacterInCursesSelection(Sci::Position iDoc, const EditModel &model, const ViewStyle &vsDraw) {
	const SelectionPosition &posCaret = model.sel.RangeMain().caret;
	const bool caretAtStart = posCaret < model.sel.RangeMain().anchor && posCaret.Position() == iDoc;
	const bool caretAtEnd = posCaret > model.sel.RangeMain().anchor &&
		vsDraw.DrawCaretInsideSelection(false, false) &&
		model.pdoc->MovePositionOutsideChar(posCaret.Position() - 1, -1) == iDoc;
	return (caretAtStart || caretAtEnd) ? InSelection::inNone : InSelection::inMain;
}

void EditView::DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, Range lineRange, Sci::Position posLineStart, int xStart,
	int subLine, std::optional<ColourRGBA> background) const {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
	// Does not take margin into account but not significant
	const XYPOSITION xStartVisible = static_cast<XYPOSITION>(subLineStart-xStart);

	const BreakFinder::BreakFor breakFor = selBackDrawn ? BreakFinder::BreakFor::Selection : BreakFinder::BreakFor::Text;
	BreakFinder bfBack(ll, &model.sel, lineRange, posLineStart, xStartVisible, breakFor, model.pdoc, &model.reprs, &vsDraw);

	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background;

	// Background drawing loop
	while (bfBack.More()) {

		const TextSegment ts = bfBack.Next();
		const Sci::Position i = ts.end() - 1;
		const Sci::Position iDoc = i + posLineStart;

		PRectangle rcSegment = rcLine;
		rcSegment.left = ll->positions[ts.start] + xStart - static_cast<XYPOSITION>(subLineStart);
		rcSegment.right = ll->positions[ts.end()] + xStart - static_cast<XYPOSITION>(subLineStart);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (!rcSegment.Empty() && rcSegment.Intersects(rcLine)) {
			// Clip to line rectangle, since may have a huge position which will not work with some platforms
			if (rcSegment.left < rcLine.left)
				rcSegment.left = rcLine.left;
			if (rcSegment.right > rcLine.right)
				rcSegment.right = rcLine.right;

			InSelection inSelection = hideSelection ? InSelection::inNone : model.sel.CharacterInSelection(iDoc);
			if (FlagSet(vsDraw.caret.style, CaretStyle::Curses) && (inSelection == InSelection::inMain))
				inSelection = CharacterInCursesSelection(iDoc, model, vsDraw);
			const bool inHotspot = model.hotspot.Valid() && model.hotspot.ContainsCharacter(iDoc);
			ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, inSelection,
				inHotspot, ll->styles[i], i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation)) {
						textBack = vsDraw.ElementColour(Element::WhiteSpaceBack)->Opaque();
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
								const PRectangle rcSpace(
									ll->positions[cpos + ts.start] + xStart - static_cast<XYPOSITION>(subLineStart),
									rcSegment.top,
									ll->positions[cpos + ts.start + 1] + xStart - static_cast<XYPOSITION>(subLineStart),
									rcSegment.bottom);
								surface->FillRectangleAligned(rcSpace,
									vsDraw.ElementColour(Element::WhiteSpaceBack)->Opaque());
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
		} else if (rcSegment.left > rcLine.right) {
			break;
		}
	}
}

static void DrawEdgeLine(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
	Range lineRange, int xStart) {
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
static void DrawMarkUnderline(Surface *surface, const EditModel &model, const ViewStyle &vsDraw,
	Sci::Line line, PRectangle rcLine) {
	int marks = model.pdoc->GetMark(line);
	for (int markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == MarkerSymbol::Underline) &&
			(vsDraw.markers[markBit].layer == Layer::Base)) {
			PRectangle rcUnderline = rcLine;
			rcUnderline.top = rcUnderline.bottom - 2;
			surface->FillRectangleAligned(rcUnderline, Fill(vsDraw.markers[markBit].back));
		}
		marks >>= 1;
	}
}

static void DrawTranslucentSelection(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine, Range lineRange, int xStart, int tabWidthMinimumPixels, Layer layer) {
	if (vsDraw.selection.layer == layer) {
		const Sci::Position posLineStart = model.pdoc->LineStart(line);
		const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
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
				const ColourRGBA selectionBack = SelectionBackground(
					model, vsDraw, model.sel.RangeType(r));
				const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
				if (model.BidirectionalEnabled()) {
					const int selectionStart = static_cast<int>(portion.start.Position() - posLineStart - lineRange.start);
					const int selectionEnd = static_cast<int>(portion.end.Position() - posLineStart - lineRange.start);

					const ScreenLine screenLine(ll, subLine, vsDraw, rcLine.right, tabWidthMinimumPixels);
					std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);

					const std::vector<Interval> intervals = slLayout->FindRangeIntervals(selectionStart, selectionEnd);
					for (const Interval &interval : intervals) {
						const XYPOSITION rcRight = interval.right + xStart;
						const XYPOSITION rcLeft = interval.left + xStart;
						const PRectangle rcSelection(rcLeft, rcLine.top, rcRight, rcLine.bottom);
						surface->FillRectangleAligned(rcSelection, selectionBack);
					}

					if (portion.end.VirtualSpace()) {
						const XYPOSITION xStartVirtual = ll->positions[lineRange.end] -
							static_cast<XYPOSITION>(subLineStart) + xStart;
						PRectangle rcSegment = rcLine;
						rcSegment.left = xStartVirtual + portion.start.VirtualSpace() * spaceWidth;
						rcSegment.right = xStartVirtual + portion.end.VirtualSpace() * spaceWidth;
						surface->FillRectangleAligned(rcSegment, selectionBack);
					}
				} else {
					PRectangle rcSegment = rcLine;
					rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart) + portion.start.VirtualSpace() * spaceWidth;
					rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart) + portion.end.VirtualSpace() * spaceWidth;
					if ((ll->wrapIndent != 0) && (lineRange.start != 0)) {
						if ((portion.start.Position() - posLineStart) == lineRange.start && model.sel.Range(r).ContainsCharacter(portion.start.Position() - 1))
							rcSegment.left -= static_cast<int>(ll->wrapIndent); // indentation added to xStart was truncated to int, so we do the same here
					}
					rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
					rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
					if (rcSegment.right > rcLine.left)
						surface->FillRectangleAligned(rcSegment, selectionBack);
				}
			}
		}
	}
}

// Draw any translucent whole line states
static void DrawTranslucentLineState(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine, Layer layer) {
	if ((model.caret.active || vsDraw.caretLine.alwaysShow) && vsDraw.ElementColour(Element::CaretLineBack) && ll->containsCaret &&
		vsDraw.caretLine.layer == layer) {
		if (vsDraw.caretLine.frame) {
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		} else {
			surface->FillRectangleAligned(rcLine, *vsDraw.ElementColour(Element::CaretLineBack));
		}
	}
	const int marksOfLine = model.pdoc->GetMark(line);
	int marksDrawnInText = marksOfLine & vsDraw.maskDrawInText;
	for (int markBit = 0; (markBit < 32) && marksDrawnInText; markBit++) {
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
	for (int markBit = 0; (markBit < 32) && marksDrawnInLine; markBit++) {
		if ((marksDrawnInLine & 1) && (vsDraw.markers[markBit].layer == layer)) {
			surface->FillRectangleAligned(rcLine, vsDraw.markers[markBit].BackWithAlpha());
		}
		marksDrawnInLine >>= 1;
	}
}

void EditView::DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line lineVisible, PRectangle rcLine, Range lineRange, Sci::Position posLineStart, int xStart,
	int subLine, std::optional<ColourRGBA> background) {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background;
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.

	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
	const XYPOSITION indentWidth = model.pdoc->IndentSize() * vsDraw.spaceWidth;

	// Does not take margin into account but not significant
	const XYPOSITION xStartVisible = static_cast<XYPOSITION>(subLineStart-xStart);

	// Foreground drawing loop
	const BreakFinder::BreakFor breakFor = (((phasesDraw == PhasesDraw::One) && selBackDrawn) || vsDraw.SelectionTextDrawn())
		? BreakFinder::BreakFor::ForegroundAndSelection : BreakFinder::BreakFor::Foreground;
	BreakFinder bfFore(ll, &model.sel, lineRange, posLineStart, xStartVisible, breakFor, model.pdoc, &model.reprs, &vsDraw);

	while (bfFore.More()) {

		const TextSegment ts = bfFore.Next();
		const Sci::Position i = ts.end() - 1;
		const Sci::Position iDoc = i + posLineStart;

		PRectangle rcSegment = rcLine;
		rcSegment.left = ll->positions[ts.start] + xStart - static_cast<XYPOSITION>(subLineStart);
		rcSegment.right = ll->positions[ts.end()] + xStart - static_cast<XYPOSITION>(subLineStart);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (rcSegment.Intersects(rcLine)) {
			const int styleMain = ll->styles[i];
			ColourRGBA textFore = vsDraw.styles[styleMain].fore;
			const Font *textFont = vsDraw.styles[styleMain].font.get();
			// Hot-spot foreground
			const bool inHotspot = model.hotspot.Valid() && model.hotspot.ContainsCharacter(iDoc);
			if (inHotspot) {
				if (vsDraw.ElementColour(Element::HotSpotActive))
					textFore = *vsDraw.ElementColour(Element::HotSpotActive);
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
							if (indicator.sacNormal.style == IndicatorStyle::TextFore || (indicator.sacNormal.style == IndicatorStyle::ExplorerLink)) {
								if (FlagSet(indicator.Flags(), IndicFlag::ValueFore))
									textFore = ColourRGBA::FromRGB(indicatorValue & static_cast<int>(IndicValue::Mask));
								else
									textFore = indicator.sacNormal.fore;
							}
						}
					}
				}
			}
			InSelection inSelection = hideSelection ? InSelection::inNone : model.sel.CharacterInSelection(iDoc);
			if (FlagSet(vsDraw.caret.style, CaretStyle::Curses) && (inSelection == InSelection::inMain))
				inSelection = CharacterInCursesSelection(iDoc, model, vsDraw);
			const std::optional<ColourRGBA> selectionFore = SelectionForeground(model, vsDraw, inSelection);
			if (selectionFore) {
				textFore = *selectionFore;
			}
			ColourRGBA textBack = TextBackground(model, vsDraw, ll, background, inSelection, inHotspot, styleMain, i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (phasesDraw == PhasesDraw::One) {
						if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation))
							textBack = vsDraw.ElementColour(Element::WhiteSpaceBack)->Opaque();
						surface->FillRectangleAligned(rcSegment, Fill(textBack));
					}
					if (inIndentation && vsDraw.viewIndentationGuides == IndentView::Real) {
						for (int indentCount = static_cast<int>((ll->positions[i] + epsilon) / indentWidth);
							indentCount <= (ll->positions[i + 1] - epsilon) / indentWidth;
							indentCount++) {
							if (indentCount > 0) {
								const XYPOSITION xIndent = std::floor(indentCount * indentWidth);
								DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
									(ll->xHighlightGuide == xIndent));
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
							rcSegment.top + vsDraw.maxAscent,
							cc, textBack, textFore);
					} else {
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Colour)) {
							textFore = ts.representation->colour;
						}
						if (FlagSet(ts.representation->appearance, RepresentationAppearance::Blob)) {
							DrawTextBlob(surface, vsDraw, rcSegment, ts.representation->stringRep,
								textBack, textFore, phasesDraw == PhasesDraw::One);
						} else {
							surface->DrawTextTransparentUTF8(rcSegment, vsDraw.styles[StyleControlChar].font.get(),
								rcSegment.top + vsDraw.maxAscent, ts.representation->stringRep, textFore);
						}
					}
				}
			} else {
				// Normal text display
				if (vsDraw.styles[styleMain].visible) {
					const std::string_view text(&ll->chars[ts.start], i - ts.start + 1);
					if (phasesDraw != PhasesDraw::One) {
						surface->DrawTextTransparent(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, text, textFore);
					} else {
						surface->DrawTextNoClip(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, text, textFore, textBack);
					}
				}
				if (vsDraw.viewWhitespace != WhiteSpace::Invisible ||
					(inIndentation && vsDraw.viewIndentationGuides != IndentView::None)) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (vsDraw.viewWhitespace != WhiteSpace::Invisible) {
								if (vsDraw.WhiteSpaceVisible(inIndentation)) {
									const XYPOSITION xmid = (ll->positions[cpos + ts.start] + ll->positions[cpos + ts.start + 1]) / 2;
									if ((phasesDraw == PhasesDraw::One) && drawWhitespaceBackground) {
										textBack = vsDraw.ElementColour(Element::WhiteSpaceBack)->Opaque();
										const PRectangle rcSpace(
											ll->positions[cpos + ts.start] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.top,
											ll->positions[cpos + ts.start + 1] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.bottom);
										surface->FillRectangleAligned(rcSpace, Fill(textBack));
									}
									const int halfDotWidth = vsDraw.whitespaceSize / 2;
									PRectangle rcDot(xmid + xStart - halfDotWidth - static_cast<XYPOSITION>(subLineStart),
										rcSegment.top + vsDraw.lineHeight / 2, 0.0f, 0.0f);
									rcDot.right = rcDot.left + vsDraw.whitespaceSize;
									rcDot.bottom = rcDot.top + vsDraw.whitespaceSize;
									const ColourRGBA whiteSpaceFore = vsDraw.ElementColour(Element::WhiteSpace).value_or(textFore);
									surface->FillRectangleAligned(rcDot, Fill(whiteSpaceFore));
								}
							}
							if (inIndentation && vsDraw.viewIndentationGuides == IndentView::Real) {
								for (int indentCount = static_cast<int>((ll->positions[cpos + ts.start] + epsilon) / indentWidth);
									indentCount <= (ll->positions[cpos + ts.start + 1] - epsilon) / indentWidth;
									indentCount++) {
									if (indentCount > 0) {
										const XYPOSITION xIndent = std::floor(indentCount * indentWidth);
										DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
											(ll->xHighlightGuide == xIndent));
									}
								}
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
			if (inHotspot && vsDraw.hotspotUnderline) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				if (vsDraw.ElementColour(Element::HotSpotActive))
					surface->FillRectangleAligned(rcUL, Fill(*vsDraw.ElementColour(Element::HotSpotActive)));
				else
					surface->FillRectangleAligned(rcUL, Fill(textFore));
			} else if (vsDraw.styles[styleMain].underline) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				surface->FillRectangleAligned(rcUL, Fill(textFore));
			}
		} else if (rcSegment.left > rcLine.right) {
			break;
		}
	}
}

void EditView::DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, Sci::Line lineVisible, PRectangle rcLine, int xStart, int subLine) {
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

		for (int indentPos = model.pdoc->IndentSize(); indentPos < indentSpace; indentPos += model.pdoc->IndentSize()) {
			const XYPOSITION xIndent = std::floor(indentPos * vsDraw.spaceWidth);
			if (xIndent < xStartText) {
				DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcLine,
					(ll->xHighlightGuide == xIndent));
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

	// See if something overrides the line background colour.
	const std::optional<ColourRGBA> background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);

	const Sci::Position posLineStart = model.pdoc->LineStart(line);

	const Range lineRange = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);
	const Range lineRangeIncludingEnd = ll->SubLineRange(subLine, LineLayout::Scope::includeEnd);
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];

	if ((ll->wrapIndent != 0) && (subLine > 0)) {
		if (FlagSet(phase, DrawPhase::back)) {
			DrawWrapIndentAndMarker(surface, vsDraw, ll, xStart, rcLine, background, customDrawWrapMarker, model.caret.active);
		}
		xStart += static_cast<int>(ll->wrapIndent);
	}

	if (phasesDraw != PhasesDraw::One) {
		if (FlagSet(phase, DrawPhase::back)) {
			DrawBackground(surface, model, vsDraw, ll, rcLine, lineRange, posLineStart, xStart,
				subLine, background);
			DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, DrawPhase::back);
			DrawEOLAnnotationText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, DrawPhase::back);
			// Remove drawBack to not draw again in DrawFoldDisplayText
			phase = static_cast<DrawPhase>(static_cast<int>(phase) & ~static_cast<int>(DrawPhase::back));
			DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
				xStart, subLine, subLineStart, background);
			if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
				DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		}

		if (FlagSet(phase, DrawPhase::indicatorsBack)) {
			DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
				lineRangeIncludingEnd.end, true, tabWidthMinimumPixels);
			DrawEdgeLine(surface, vsDraw, ll, rcLine, lineRange, xStart);
			DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
		}
	}

	if (FlagSet(phase, DrawPhase::text)) {
		DrawTranslucentSelection(surface, model, vsDraw, ll, line, rcLine, subLine, lineRange, xStart, tabWidthMinimumPixels, Layer::UnderText);
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine, subLine, Layer::UnderText);
		DrawForeground(surface, model, vsDraw, ll, lineVisible, rcLine, lineRange, posLineStart, xStart,
			subLine, background);
	}

	if (FlagSet(phase, DrawPhase::indentationGuides)) {
		DrawIndentGuidesOverEmpty(surface, model, vsDraw, ll, line, lineVisible, rcLine, xStart, subLine);
	}

	if (FlagSet(phase, DrawPhase::indicatorsFore)) {
		DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
			lineRangeIncludingEnd.end, false, tabWidthMinimumPixels);
	}

	DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, phase);
	DrawEOLAnnotationText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, phase);

	if (phasesDraw == PhasesDraw::One) {
		DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
			xStart, subLine, subLineStart, background);
		if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		DrawEdgeLine(surface, vsDraw, ll, rcLine, lineRange, xStart);
		DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
	}

	if (!hideSelection && FlagSet(phase, DrawPhase::selectionTranslucent)) {
		DrawTranslucentSelection(surface, model, vsDraw, ll, line, rcLine, subLine, lineRange, xStart, tabWidthMinimumPixels, Layer::OverText);
	}

	if (FlagSet(phase, DrawPhase::lineTranslucent)) {
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine, subLine, Layer::OverText);
	}
}

static void DrawFoldLines(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
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
		if ((subLine == 0) &&
			((expanded && (FlagSet(model.foldFlags, FoldFlag::LineBeforeExpanded)))
			||
			(!expanded && (FlagSet(model.foldFlags, FoldFlag::LineBeforeContracted))))) {
			surface->FillRectangleAligned(Side(rcLine, Edge::top, 1.0), foldLineColour);
		}
		// Paint the line below the fold
		if (lastSubLine &&
			((expanded && (FlagSet(model.foldFlags, FoldFlag::LineAfterExpanded)))
			||
			(!expanded && (FlagSet(model.foldFlags, FoldFlag::LineAfterContracted))))) {
			surface->FillRectangleAligned(Side(rcLine, Edge::bottom, 1.0), foldLineColour);
			// If contracted fold line drawn then don't overwrite with hidden line
			// as fold lines are more specific then hidden lines.
			if (!expanded) {
				return;
			}
		}
	}
	if (lastSubLine && model.pcs->GetVisible(line) && !model.pcs->GetVisible(line + 1)) {
		std::optional<ColourRGBA> hiddenLineColour = vsDraw.ElementColour(Element::HiddenLine);
		if (hiddenLineColour) {
			surface->FillRectangleAligned(Side(rcLine, Edge::bottom, 1.0), *hiddenLineColour);
		}
	}
}

void EditView::PaintText(Surface *surfaceWindow, const EditModel &model, PRectangle rcArea,
	PRectangle rcClient, const ViewStyle &vsDraw) {
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
		std::vector<DrawPhase> phases;
		if ((phasesDraw == PhasesDraw::Multiple) && !bufferedDraw) {
			for (DrawPhase phase = DrawPhase::back; phase <= DrawPhase::carets; phase = static_cast<DrawPhase>(static_cast<int>(phase) * 2)) {
				phases.push_back(phase);
			}
		} else {
			phases.push_back(DrawPhase::all);
		}
		for (const DrawPhase &phase : phases) {
			int ypos = 0;
			if (!bufferedDraw)
				ypos += screenLinePaintFirst * vsDraw.lineHeight;
			int yposScreen = screenLinePaintFirst * vsDraw.lineHeight;
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
				}
#if defined(TIME_PAINTING)
				durLayout += ep.Duration(true);
#endif
				if (ll) {
					ll->containsCaret = !hideSelection && (lineDoc == lineCaret)
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

					if (model.BidirectionalEnabled()) {
						// Fill the line bidi data
						UpdateBidiData(model, vsDraw, ll.get());
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

void EditView::FillLineRemainder(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcArea, int subLine) const {
		InSelection eolInSelection = InSelection::inNone;
	if ((!hideSelection) && (subLine == (ll->lines - 1))) {
		eolInSelection = model.LineEndInSelection(line);
	}

	const std::optional<ColourRGBA> background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);

	if (eolInSelection && vsDraw.selection.eolFilled && (line < model.pdoc->LinesTotal() - 1) && (vsDraw.selection.layer == Layer::Base)) {
		surface->FillRectangleAligned(rcArea, Fill(SelectionBackground(model, vsDraw, eolInSelection).Opaque()));
	} else {
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

// Space (3 space characters) between line numbers and text when printing.
#define lineNumberPrintSpace "   "

static ColourRGBA InvertedLight(ColourRGBA orig) noexcept {
	unsigned int r = orig.GetRed();
	unsigned int g = orig.GetGreen();
	unsigned int b = orig.GetBlue();
	const unsigned int l = (r + g + b) / 3; 	// There is a better calculation for this that matches human eye
	const unsigned int il = 0xff - l;
	if (l == 0)
		return ColourRGBA(0xff, 0xff, 0xff);
	r = r * il / l;
	g = g * il / l;
	b = b * il / l;
	return ColourRGBA(std::min(r, 0xffu), std::min(g, 0xffu), std::min(b, 0xffu));
}

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
	vsPrint.elementColours.clear();
	vsPrint.elementBaseColours.clear();
	// Transparent:
	vsPrint.elementBaseColours[Element::SelectionBack] = ColourRGBA(0xc0, 0xc0, 0xc0, 0x0);
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
			it->fore = ColourRGBA(0, 0, 0);
			it->back = ColourRGBA(0xff, 0xff, 0xff);
		} else if (colourMode == PrintOption::ColourOnWhite || colourMode == PrintOption::ColourOnWhiteDefaultBG) {
			it->back = ColourRGBA(0xff, 0xff, 0xff);
		}
	}
	// White background for the line numbers if PrintOption::ScreenColours isn't used
	if (colourMode != PrintOption::ScreenColours) {
		vsPrint.styles[StyleLineNumber].back = ColourRGBA(0xff, 0xff, 0xff);
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
