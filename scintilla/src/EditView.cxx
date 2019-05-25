// Scintilla source code edit control
/** @file EditView.cxx
 ** Defines the appearance of the main text area of the editor window.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <forward_list>
#include <algorithm>
#include <iterator>
#include <memory>
#include <chrono>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterSet.h"
#include "Position.h"
#include "IntegerRectangle.h"
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

static constexpr bool IsControlCharacter(int ch) noexcept {
	// iscntrl returns true for lots of chars > 127 which are displayable
	return ch >= 0 && ch < ' ';
}

PrintParameters::PrintParameters() noexcept {
	magnification = 0;
	colourMode = SC_PRINT_NORMAL;
	wrapState = eWrapWord;
}

namespace Scintilla {

bool ValidStyledText(const ViewStyle &vs, size_t styleOffset, const StyledText &st) {
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
		FontAlias fontText = vs.styles[style + styleOffset].font;
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
			FontAlias fontText = vs.styles[styleOffset + st.style].font;
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
	FontAlias fontText = style.font;
	if (phase & drawBack) {
		if (phase & drawText) {
			// Drawing both
			surface->DrawTextNoClip(rc, fontText, ybase, text,
				style.fore, style.back);
		} else {
			surface->FillRectangle(rc, style.back);
		}
	} else if (phase & drawText) {
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
			FontAlias fontText = vs.styles[style].font;
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

const XYPOSITION epsilon = 0.0001f;	// A small nudge to avoid floating point precision issues

EditView::EditView() {
	tabWidthMinimumPixels = 2; // needed for calculating tab stops for fractional proportional fonts
	hideSelection = false;
	drawOverstrikeCaret = true;
	bufferedDraw = true;
	phasesDraw = phasesTwo;
	lineWidthMaxSeen = 0;
	additionalCaretsBlink = true;
	additionalCaretsVisible = true;
	imeCaretBlockOverride = false;
	llc.SetLevel(LineLayoutCache::llcCaret);
	posCache.SetSize(0x400);
	tabArrowHeight = 4;
	customDrawTabArrow = nullptr;
	customDrawWrapMarker = nullptr;
}

EditView::~EditView() {
}

bool EditView::SetTwoPhaseDraw(bool twoPhaseDraw) noexcept {
	const PhasesDraw phasesDrawNew = twoPhaseDraw ? phasesTwo : phasesOne;
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
	return phasesDraw == phasesMultiple;
}

void EditView::ClearAllTabstops() noexcept {
	ldTabstops.reset();
}

XYPOSITION EditView::NextTabstopPos(Sci::Line line, XYPOSITION x, XYPOSITION tabWidth) const {
	const int next = GetNextTabstop(line, static_cast<int>(x + tabWidthMinimumPixels));
	if (next > 0)
		return static_cast<XYPOSITION>(next);
	return (static_cast<int>((x + tabWidthMinimumPixels) / tabWidth) + 1) * tabWidth;
}

bool EditView::ClearTabstops(Sci::Line line) {
	return ldTabstops && ldTabstops->ClearTabstops(line);
}

bool EditView::AddTabstop(Sci::Line line, int x) {
	if (!ldTabstops) {
		ldTabstops = std::make_unique<LineTabstops>();
	}
	return ldTabstops && ldTabstops->AddTabstop(line, x);
}

int EditView::GetNextTabstop(Sci::Line line, int x) const {
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

void EditView::DropGraphics(bool freeObjects) {
	if (freeObjects) {
		pixmapLine.reset();
		pixmapIndentGuide.reset();
		pixmapIndentGuideHighlight.reset();
	} else {
		if (pixmapLine)
			pixmapLine->Release();
		if (pixmapIndentGuide)
			pixmapIndentGuide->Release();
		if (pixmapIndentGuideHighlight)
			pixmapIndentGuideHighlight->Release();
	}
}

void EditView::AllocateGraphics(const ViewStyle &vsDraw) {
	if (!pixmapLine)
		pixmapLine.reset(Surface::Allocate(vsDraw.technology));
	if (!pixmapIndentGuide)
		pixmapIndentGuide.reset(Surface::Allocate(vsDraw.technology));
	if (!pixmapIndentGuideHighlight)
		pixmapIndentGuideHighlight.reset(Surface::Allocate(vsDraw.technology));
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

static void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid, const ViewStyle &vsDraw) {
	const IntegerRectangle ircTab(rcTab);
	if ((rcTab.left + 2) < (rcTab.right - 1))
		surface->MoveTo(ircTab.left + 2, ymid);
	else
		surface->MoveTo(ircTab.right - 1, ymid);
	surface->LineTo(ircTab.right - 1, ymid);

	// Draw the arrow head if needed
	if (vsDraw.tabDrawMode == tdLongArrow) {
		int ydiff = (ircTab.bottom - ircTab.top) / 2;
		int xhead = ircTab.right - 1 - ydiff;
		if (xhead <= rcTab.left) {
			ydiff -= ircTab.left - xhead - 1;
			xhead = ircTab.left - 1;
		}
		surface->LineTo(xhead, ymid - ydiff);
		surface->MoveTo(ircTab.right - 1, ymid);
		surface->LineTo(xhead, ymid + ydiff);
	}
}

void EditView::RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw) {
	if (!pixmapIndentGuide->Initialised()) {
		// 1 extra pixel in height so can handle odd/even positions and so produce a continuous line
		pixmapIndentGuide->InitPixMap(1, vsDraw.lineHeight + 1, surfaceWindow, wid);
		pixmapIndentGuideHighlight->InitPixMap(1, vsDraw.lineHeight + 1, surfaceWindow, wid);
		const PRectangle rcIG = PRectangle::FromInts(0, 0, 1, vsDraw.lineHeight);
		pixmapIndentGuide->FillRectangle(rcIG, vsDraw.styles[STYLE_INDENTGUIDE].back);
		pixmapIndentGuide->PenColour(vsDraw.styles[STYLE_INDENTGUIDE].fore);
		pixmapIndentGuideHighlight->FillRectangle(rcIG, vsDraw.styles[STYLE_BRACELIGHT].back);
		pixmapIndentGuideHighlight->PenColour(vsDraw.styles[STYLE_BRACELIGHT].fore);
		for (int stripe = 1; stripe < vsDraw.lineHeight + 1; stripe += 2) {
			const PRectangle rcPixel = PRectangle::FromInts(0, stripe, 1, stripe + 1);
			pixmapIndentGuide->FillRectangle(rcPixel, vsDraw.styles[STYLE_INDENTGUIDE].fore);
			pixmapIndentGuideHighlight->FillRectangle(rcPixel, vsDraw.styles[STYLE_BRACELIGHT].fore);
		}
	}
}

LineLayout *EditView::RetrieveLineLayout(Sci::Line lineNumber, const EditModel &model) {
	const Sci::Position posLineStart = model.pdoc->LineStart(lineNumber);
	const Sci::Position posLineEnd = model.pdoc->LineStart(lineNumber + 1);
	PLATFORM_ASSERT(posLineEnd >= posLineStart);
	const Sci::Line lineCaret = model.pdoc->SciLineFromPosition(model.sel.MainCaret());
	return llc.Retrieve(lineNumber, lineCaret,
		static_cast<int>(posLineEnd - posLineStart), model.pdoc->GetStyleClock(),
		model.LinesOnScreen() + 1, model.pdoc->LinesTotal());
}

/**
* Fill in the LineLayout data for the given line.
* Copy the given @a line and its styles from the document into local arrays.
* Also determine the x position at which each character starts.
*/
void EditView::LayoutLine(const EditModel &model, Sci::Line line, Surface *surface, const ViewStyle &vstyle, LineLayout *ll, int width) {
	if (!ll)
		return;

	PLATFORM_ASSERT(line < model.pdoc->LinesTotal());
	PLATFORM_ASSERT(ll->chars != NULL);
	const Sci::Position posLineStart = model.pdoc->LineStart(line);
	Sci::Position posLineEnd = model.pdoc->LineStart(line + 1);
	// If the line is very long, limit the treatment to a length that should fit in the viewport
	if (posLineEnd >(posLineStart + ll->maxLineLength)) {
		posLineEnd = posLineStart + ll->maxLineLength;
	}
	if (ll->validity == LineLayout::llCheckTextAndStyle) {
		Sci::Position lineLength = posLineEnd - posLineStart;
		if (!vstyle.viewEOL) {
			lineLength = model.pdoc->LineEnd(line) - posLineStart;
		}
		if (lineLength == ll->numCharsInLine) {
			// See if chars, styles, indicators, are all the same
			bool allSame = true;
			// Check base line layout
			int styleByte = 0;
			int numCharsInLine = 0;
			while (numCharsInLine < lineLength) {
				const Sci::Position charInDoc = numCharsInLine + posLineStart;
				const char chDoc = model.pdoc->CharAt(charInDoc);
				styleByte = model.pdoc->StyleIndexAt(charInDoc);
				allSame = allSame &&
					(ll->styles[numCharsInLine] == styleByte);
				if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseMixed)
					allSame = allSame &&
					(ll->chars[numCharsInLine] == chDoc);
				else if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseLower)
					allSame = allSame &&
					(ll->chars[numCharsInLine] == MakeLowerCase(chDoc));
				else if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseUpper)
					allSame = allSame &&
					(ll->chars[numCharsInLine] == MakeUpperCase(chDoc));
				else	{ // Style::caseCamel
					if ((model.pdoc->IsASCIIWordByte(ll->chars[numCharsInLine])) &&
					  ((numCharsInLine == 0) || (!model.pdoc->IsASCIIWordByte(ll->chars[numCharsInLine - 1])))) {
						allSame = allSame && (ll->chars[numCharsInLine] == MakeUpperCase(chDoc));
					} else {
						allSame = allSame && (ll->chars[numCharsInLine] == MakeLowerCase(chDoc));
					}
				}
				numCharsInLine++;
			}
			allSame = allSame && (ll->styles[numCharsInLine] == styleByte);	// For eolFilled
			if (allSame) {
				ll->validity = LineLayout::llPositions;
			} else {
				ll->validity = LineLayout::llInvalid;
			}
		} else {
			ll->validity = LineLayout::llInvalid;
		}
	}
	if (ll->validity == LineLayout::llInvalid) {
		ll->widthLine = LineLayout::wrapWidthInfinite;
		ll->lines = 1;
		if (vstyle.edgeState == EDGE_BACKGROUND) {
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
		for (Sci::Position styleInLine = 0; styleInLine < numCharsInLine; styleInLine++) {
			const unsigned char styleByte = ll->styles[styleInLine];
			ll->styles[styleInLine] = styleByte;
		}
		const unsigned char styleByteLast = (lineLength > 0) ? ll->styles[lineLength - 1] : 0;
		if (vstyle.someStylesForceCase) {
			for (int charInLine = 0; charInLine<lineLength; charInLine++) {
				const char chDoc = ll->chars[charInLine];
				if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseUpper)
					ll->chars[charInLine] = MakeUpperCase(chDoc);
				else if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseLower)
					ll->chars[charInLine] = MakeLowerCase(chDoc);
				else if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseCamel) {
					if ((model.pdoc->IsASCIIWordByte(ll->chars[charInLine])) &&
					  ((charInLine == 0) || (!model.pdoc->IsASCIIWordByte(ll->chars[charInLine - 1])))) {
						ll->chars[charInLine] = MakeUpperCase(chDoc);
					} else {
						ll->chars[charInLine] = MakeLowerCase(chDoc);
					}
				}
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

		BreakFinder bfLayout(ll, nullptr, Range(0, numCharsInLine), posLineStart, 0, false, model.pdoc, &model.reprs, nullptr);
		while (bfLayout.More()) {

			const TextSegment ts = bfLayout.Next();

			std::fill(&ll->positions[ts.start + 1], &ll->positions[ts.end() + 1], 0.0f);
			if (vstyle.styles[ll->styles[ts.start]].visible) {
				if (ts.representation) {
					XYPOSITION representationWidth = vstyle.controlCharWidth;
					if (ll->chars[ts.start] == '\t') {
						// Tab is a special case of representation, taking a variable amount of space
						const XYPOSITION x = ll->positions[ts.start];
						representationWidth = NextTabstopPos(line, x, vstyle.tabWidth) - ll->positions[ts.start];
					} else {
						if (representationWidth <= 0.0) {
							XYPOSITION positionsRepr[256];	// Should expand when needed
							posCache.MeasureWidths(surface, vstyle, STYLE_CONTROLCHAR, ts.representation->stringRep.c_str(),
								static_cast<unsigned int>(ts.representation->stringRep.length()), positionsRepr, model.pdoc);
							representationWidth = positionsRepr[ts.representation->stringRep.length() - 1] + vstyle.ctrlCharPadding;
						}
					}
					for (int ii = 0; ii < ts.length; ii++)
						ll->positions[ts.start + 1 + ii] = representationWidth;
				} else {
					if ((ts.length == 1) && (' ' == ll->chars[ts.start])) {
						// Over half the segments are single characters and of these about half are space characters.
						ll->positions[ts.start + 1] = vstyle.styles[ll->styles[ts.start]].spaceWidth;
					} else {
						posCache.MeasureWidths(surface, vstyle, ll->styles[ts.start], &ll->chars[ts.start],
							ts.length, &ll->positions[ts.start + 1], model.pdoc);
					}
				}
				lastSegItalics = (!ts.representation) && ((ll->chars[ts.end() - 1] != ' ') && vstyle.styles[ll->styles[ts.start]].italic);
			}

			for (Sci::Position posToIncrease = ts.start + 1; posToIncrease <= ts.end(); posToIncrease++) {
				ll->positions[posToIncrease] += ll->positions[ts.start];
			}
		}

		// Small hack to make lines that end with italics not cut off the edge of the last character
		if (lastSegItalics) {
			ll->positions[numCharsInLine] += vstyle.lastSegItalicsOffset;
		}
		ll->numCharsInLine = numCharsInLine;
		ll->numCharsBeforeEOL = numCharsBeforeEOL;
		ll->validity = LineLayout::llPositions;
	}
	// Hard to cope when too narrow, so just assume there is space
	if (width < 20) {
		width = 20;
	}
	if ((ll->validity == LineLayout::llPositions) || (ll->widthLine != width)) {
		ll->widthLine = width;
		if (width == LineLayout::wrapWidthInfinite) {
			ll->lines = 1;
		} else if (width > ll->positions[ll->numCharsInLine]) {
			// Simple common case where line does not need wrapping.
			ll->lines = 1;
		} else {
			if (vstyle.wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
				width -= static_cast<int>(vstyle.aveCharWidth); // take into account the space for end wrap mark
			}
			XYPOSITION wrapAddIndent = 0; // This will be added to initial indent of line
			switch (vstyle.wrapIndentMode) {
			case SC_WRAPINDENT_FIXED:
				wrapAddIndent = vstyle.wrapVisualStartIndent * vstyle.aveCharWidth;
				break;
			case SC_WRAPINDENT_INDENT:
				wrapAddIndent = model.pdoc->IndentSize() * vstyle.spaceWidth;
				break;
			case SC_WRAPINDENT_DEEPINDENT:
				wrapAddIndent = model.pdoc->IndentSize() * 2 * vstyle.spaceWidth;
				break;
			}
			ll->wrapIndent = wrapAddIndent;
			if (vstyle.wrapIndentMode != SC_WRAPINDENT_FIXED) {
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
			if ((vstyle.wrapVisualFlags & SC_WRAPVISUALFLAG_START) && (ll->wrapIndent < vstyle.aveCharWidth))
				ll->wrapIndent = vstyle.aveCharWidth; // Indent to show start visual
			ll->lines = 0;
			// Calculate line start positions based upon width.
			Sci::Position lastGoodBreak = 0;
			Sci::Position lastLineStart = 0;
			XYACCUMULATOR startOffset = 0;
			Sci::Position p = 0;
			while (p < ll->numCharsInLine) {
				if ((ll->positions[p + 1] - startOffset) >= width) {
					if (lastGoodBreak == lastLineStart) {
						// Try moving to start of last character
						if (p > 0) {
							lastGoodBreak = model.pdoc->MovePositionOutsideChar(p + posLineStart, -1)
								- posLineStart;
						}
						if (lastGoodBreak == lastLineStart) {
							// Ensure at least one character on line.
							lastGoodBreak = model.pdoc->MovePositionOutsideChar(lastGoodBreak + posLineStart + 1, 1)
								- posLineStart;
						}
					}
					lastLineStart = lastGoodBreak;
					ll->lines++;
					ll->SetLineStart(ll->lines, static_cast<int>(lastGoodBreak));
					startOffset = ll->positions[lastGoodBreak];
					// take into account the space for start wrap mark and indent
					startOffset -= ll->wrapIndent;
					p = lastGoodBreak + 1;
					continue;
				}
				if (p > 0) {
					if (vstyle.wrapState == eWrapChar) {
						lastGoodBreak = model.pdoc->MovePositionOutsideChar(p + posLineStart, -1)
							- posLineStart;
						p = model.pdoc->MovePositionOutsideChar(p + 1 + posLineStart, 1) - posLineStart;
						continue;
					} else if ((vstyle.wrapState == eWrapWord) && (ll->styles[p] != ll->styles[p - 1])) {
						lastGoodBreak = p;
					} else if (IsSpaceOrTab(ll->chars[p - 1]) && !IsSpaceOrTab(ll->chars[p])) {
						lastGoodBreak = p;
					}
				}
				p++;
			}
			ll->lines++;
		}
		ll->validity = LineLayout::llLines;
	}
}

// Fill the LineLayout bidirectional data fields according to each char style

void EditView::UpdateBidiData(const EditModel &model, const ViewStyle &vstyle, LineLayout *ll) {
	if (model.BidirectionalEnabled()) {
		ll->EnsureBidiData();
		for (int stylesInLine = 0; stylesInLine < ll->numCharsInLine; stylesInLine++) {
			ll->bidiData->stylesFonts[stylesInLine].MakeAlias(vstyle.styles[ll->styles[stylesInLine]].font);
		}
		ll->bidiData->stylesFonts[ll->numCharsInLine].ClearFont();

		for (int charsInLine = 0; charsInLine < ll->numCharsInLine; charsInLine++) {
			const int charWidth = UTF8DrawBytes(reinterpret_cast<unsigned char *>(&ll->chars[charsInLine]), ll->numCharsInLine - charsInLine);
			const Representation *repr = model.reprs.RepresentationFromCharacter(&ll->chars[charsInLine], charWidth);

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
	if (pos.Position() == INVALID_POSITION)
		return pt;
	Sci::Line lineDoc = model.pdoc->SciLineFromPosition(pos.Position());
	Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
	if ((pe & peLineEnd) && (lineDoc > 0) && (pos.Position() == posLineStart)) {
		// Want point at end of first line
		lineDoc--;
		posLineStart = model.pdoc->LineStart(lineDoc);
	}
	const Sci::Line lineVisible = model.pcs->DisplayFromDoc(lineDoc);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
		const int posInLine = static_cast<int>(pos.Position() - posLineStart);
		pt = ll->PointFromPosition(posInLine, vs.lineHeight, pe);
		pt.x += vs.textStart - model.xOffset;

		if (model.BidirectionalEnabled()) {
			// Fill the line bidi data
			UpdateBidiData(model, vs, ll);

			// Find subLine
			const int subLine = ll->SubLineFromPosition(posInLine, pe);
			const int caretPosition = posInLine - ll->LineStart(subLine);

			// Get the point from current position
			const ScreenLine screenLine(ll, subLine, vs, rcClient.right, tabWidthMinimumPixels);
			std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);
			pt.x = slLayout->XFromPosition(caretPosition);

			pt.x += vs.textStart - model.xOffset;

			pt.y = 0;
			if (posInLine >= ll->LineStart(subLine)) {
				pt.y = static_cast<XYPOSITION>(subLine*vs.lineHeight);
			}
		}
		pt.y += (lineVisible - topLine) * vs.lineHeight;
	}
	pt.x += pos.VirtualSpace() * vs.styles[ll->EndLineStyle()].spaceWidth;
	return pt;
}

Range EditView::RangeDisplayLine(Surface *surface, const EditModel &model, Sci::Line lineVisible, const ViewStyle &vs) {
	Range rangeSubLine = Range(0, 0);
	if (lineVisible < 0) {
		return rangeSubLine;
	}
	const Sci::Line lineDoc = model.pcs->DocFromDisplay(lineVisible);
	const Sci::Position positionLineStart = model.pdoc->LineStart(lineDoc);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
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
	Sci::Line visibleLine = static_cast<int>(floor(pt.y / vs.lineHeight));
	if (!canReturnInvalid && (visibleLine < 0))
		visibleLine = 0;
	const Sci::Line lineDoc = model.pcs->DocFromDisplay(visibleLine);
	if (canReturnInvalid && (lineDoc < 0))
		return SelectionPosition(INVALID_POSITION);
	if (lineDoc >= model.pdoc->LinesTotal())
		return SelectionPosition(canReturnInvalid ? INVALID_POSITION :
			model.pdoc->Length());
	const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
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
				UpdateBidiData(model, vs, ll);

				const ScreenLine screenLine(ll, subLine, vs, rcClient.right, tabWidthMinimumPixels);
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
	return SelectionPosition(canReturnInvalid ? INVALID_POSITION : posLineStart);
}

/**
* Find the document position corresponding to an x coordinate on a particular document line.
* Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
* This method is used for rectangular selections and does not work on wrapped lines.
*/
SelectionPosition EditView::SPositionFromLineX(Surface *surface, const EditModel &model, Sci::Line lineDoc, int x, const ViewStyle &vs) {
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		const Sci::Position posLineStart = model.pdoc->LineStart(lineDoc);
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
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
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
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
	AutoLineLayout ll(llc, RetrieveLineLayout(line, model));
	Sci::Position posRet = INVALID_POSITION;
	if (surface && ll) {
		const Sci::Position posLineStart = model.pdoc->LineStart(line);
		LayoutLine(model, line, surface, vs, ll, model.wrapWidth);
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
							posRet = ll->LineStart(subLine + 1) + posLineStart - 1;
					}
				}
			}
		}
	}
	return posRet;
}

static ColourDesired SelectionBackground(const ViewStyle &vsDraw, bool main, bool primarySelection) noexcept {
	return main ?
		(primarySelection ? vsDraw.selColours.back : vsDraw.selBackground2) :
		vsDraw.selAdditionalBackground;
}

static ColourDesired TextBackground(const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	ColourOptional background, int inSelection, bool inHotspot, int styleMain, Sci::Position i) {
	if (inSelection == 1) {
		if (vsDraw.selColours.back.isSet && (vsDraw.selAlpha == SC_ALPHA_NOALPHA)) {
			return SelectionBackground(vsDraw, true, model.primarySelection);
		}
	} else if (inSelection == 2) {
		if (vsDraw.selColours.back.isSet && (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA)) {
			return SelectionBackground(vsDraw, false, model.primarySelection);
		}
	} else {
		if ((vsDraw.edgeState == EDGE_BACKGROUND) &&
			(i >= ll->edgeColumn) &&
			(i < ll->numCharsBeforeEOL))
			return vsDraw.theEdge.colour;
		if (inHotspot && vsDraw.hotspotColours.back.isSet)
			return vsDraw.hotspotColours.back;
	}
	if (background.isSet && (styleMain != STYLE_BRACELIGHT) && (styleMain != STYLE_BRACEBAD)) {
		return background;
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

static void SimpleAlphaRectangle(Surface *surface, PRectangle rc, ColourDesired fill, int alpha) {
	if (alpha != SC_ALPHA_NOALPHA) {
		surface->AlphaRectangle(rc, 0, fill, alpha, fill, alpha, 0);
	}
}

static void DrawTextBlob(Surface *surface, const ViewStyle &vsDraw, PRectangle rcSegment,
	std::string_view text, ColourDesired textBack, ColourDesired textFore, bool fillBackground) {
	if (rcSegment.Empty())
		return;
	if (fillBackground) {
		surface->FillRectangle(rcSegment, textBack);
	}
	FontAlias ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;
	const int normalCharHeight = static_cast<int>(ceil(vsDraw.styles[STYLE_CONTROLCHAR].capitalHeight));
	PRectangle rcCChar = rcSegment;
	rcCChar.left = rcCChar.left + 1;
	rcCChar.top = rcSegment.top + vsDraw.maxAscent - normalCharHeight;
	rcCChar.bottom = rcSegment.top + vsDraw.maxAscent + 1;
	PRectangle rcCentral = rcCChar;
	rcCentral.top++;
	rcCentral.bottom--;
	surface->FillRectangle(rcCentral, textFore);
	PRectangle rcChar = rcCChar;
	rcChar.left++;
	rcChar.right--;
	surface->DrawTextClipped(rcChar, ctrlCharsFont,
		rcSegment.top + vsDraw.maxAscent, text,
		textBack, textFore);
}

static void DrawFrame(Surface *surface, ColourDesired colour, int alpha, PRectangle rcFrame) {
	if (alpha != SC_ALPHA_NOALPHA)
		surface->AlphaRectangle(rcFrame, 0, colour, alpha, colour, alpha, 0);
	else
		surface->FillRectangle(rcFrame, colour);
}

static void DrawCaretLineFramed(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine, int subLine) {
	const int width = vsDraw.GetFrameWidth();
	if (subLine == 0 || ll->wrapIndent == 0 || vsDraw.caretLineAlpha != SC_ALPHA_NOALPHA) {
		// Left
		DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
			PRectangle(rcLine.left, rcLine.top, rcLine.left + width, rcLine.bottom));
	}
	if (subLine == 0) {
		// Top
		DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
			PRectangle(rcLine.left + width, rcLine.top, rcLine.right - width, rcLine.top + width));
	}
	if (subLine == ll->lines - 1 || vsDraw.caretLineAlpha != SC_ALPHA_NOALPHA) {
		// Right
		DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
			PRectangle(rcLine.right - width, rcLine.top, rcLine.right, rcLine.bottom));
	}
	if (subLine == ll->lines - 1) {
		// Bottom
		DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
			PRectangle(rcLine.left + width, rcLine.bottom - width, rcLine.right - width, rcLine.bottom));
	}
}

void EditView::DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, Sci::Line line, Sci::Position lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
	ColourOptional background) {

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
		surface->FillRectangle(rcSegment, background.isSet ? background : vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		if (!hideSelection && ((vsDraw.selAlpha == SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA))) {
			const SelectionSegment virtualSpaceRange(SelectionPosition(model.pdoc->LineEnd(line)),
				SelectionPosition(model.pdoc->LineEnd(line),
					model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line))));
			for (size_t r = 0; r<model.sel.Count(); r++) {
				const int alpha = (r == model.sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
				if (alpha == SC_ALPHA_NOALPHA) {
					const SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
					if (!portion.Empty()) {
						const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
						rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] -
							static_cast<XYPOSITION>(subLineStart)+portion.start.VirtualSpace() * spaceWidth;
						rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] -
							static_cast<XYPOSITION>(subLineStart)+portion.end.VirtualSpace() * spaceWidth;
						rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
						rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
						surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, r == model.sel.Main(), model.primarySelection));
					}
				}
			}
		}
	}

	int eolInSelection = 0;
	int alpha = SC_ALPHA_NOALPHA;
	if (!hideSelection) {
		const Sci::Position posAfterLineEnd = model.pdoc->LineStart(line + 1);
		eolInSelection = lastSubLine ? model.sel.InSelectionForEOL(posAfterLineEnd) : 0;
		alpha = (eolInSelection == 1) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
	}

	// Draw the [CR], [LF], or [CR][LF] blobs if visible line ends are on
	XYPOSITION blobsWidth = 0;
	if (lastSubLine) {
		for (Sci::Position eolPos = ll->numCharsBeforeEOL; eolPos<ll->numCharsInLine; eolPos++) {
			rcSegment.left = xStart + ll->positions[eolPos] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
			rcSegment.right = xStart + ll->positions[eolPos + 1] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
			blobsWidth += rcSegment.Width();
			char hexits[4] = "";
			const char *ctrlChar;
			const unsigned char chEOL = ll->chars[eolPos];
			const int styleMain = ll->styles[eolPos];
			const ColourDesired textBack = TextBackground(model, vsDraw, ll, background, eolInSelection, false, styleMain, eolPos);
			if (UTF8IsAscii(chEOL)) {
				ctrlChar = ControlCharacterString(chEOL);
			} else {
				const Representation *repr = model.reprs.RepresentationFromCharacter(&ll->chars[eolPos], ll->numCharsInLine - eolPos);
				if (repr) {
					ctrlChar = repr->stringRep.c_str();
					eolPos = ll->numCharsInLine;
				} else {
					sprintf(hexits, "x%2X", chEOL);
					ctrlChar = hexits;
				}
			}
			ColourDesired textFore = vsDraw.styles[styleMain].fore;
			if (eolInSelection && vsDraw.selColours.fore.isSet) {
				textFore = (eolInSelection == 1) ? vsDraw.selColours.fore : vsDraw.selAdditionalForeground;
			}
			if (eolInSelection && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1)) {
				if (alpha == SC_ALPHA_NOALPHA) {
					surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection));
				} else {
					surface->FillRectangle(rcSegment, textBack);
				}
			} else {
				surface->FillRectangle(rcSegment, textBack);
			}
			DrawTextBlob(surface, vsDraw, rcSegment, ctrlChar, textBack, textFore, phasesDraw == phasesOne);
			if (eolInSelection && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
				SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection), alpha);
			}
		}
	}

	// Draw the eol-is-selected rectangle
	rcSegment.left = xEol + xStart + virtualSpace + blobsWidth;
	rcSegment.right = rcSegment.left + vsDraw.aveCharWidth;

	if (eolInSelection && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection));
	} else {
		if (background.isSet) {
			surface->FillRectangle(rcSegment, background);
		} else if (line < model.pdoc->LinesTotal() - 1) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine]].eolFilled) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back);
		}
		if (eolInSelection && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection), alpha);
		}
	}

	rcSegment.left = rcSegment.right;
	if (rcSegment.left < rcLine.left)
		rcSegment.left = rcLine.left;
	rcSegment.right = rcLine.right;

	const bool fillRemainder = !lastSubLine || model.foldDisplayTextStyle == SC_FOLDDISPLAYTEXT_HIDDEN || !model.pcs->GetFoldDisplayTextShown(line);
	if (fillRemainder) {
		// Fill the remainder of the line
		FillLineRemainder(surface, model, vsDraw, ll, line, rcSegment, subLine);
	}

	bool drawWrapMarkEnd = false;

	if (subLine + 1 < ll->lines) {
		if (vsDraw.wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
			drawWrapMarkEnd = ll->LineStart(subLine + 1) != 0;
		}
		if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret)) {
			const int width = vsDraw.GetFrameWidth();
			// Draw right of frame under marker
			DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
				PRectangle(rcLine.right - width, rcLine.top, rcLine.right, rcLine.bottom));
		}
	}

	if (drawWrapMarkEnd) {
		PRectangle rcPlace = rcSegment;

		if (vsDraw.wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_END_BY_TEXT) {
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
	const LineLayout *ll, int xStart, PRectangle rcLine, Sci::Position secondCharacter, int subLine, Indicator::DrawState drawState,
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
		vsDraw.indicators[indicNum].Draw(surface, rc, rcLine, rcFirstCharacter, drawState, value);
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
				const Indicator::DrawState drawState = hover ? Indicator::drawHover : Indicator::drawNormal;
				const Sci::Position posSecond = model.pdoc->MovePositionOutsideChar(rangeRun.First() + 1, 1);
				DrawIndicator(deco->Indicator(), startPos - posLineStart, endPos - posLineStart,
					surface, vsDraw, ll, xStart, rcLine, posSecond - posLineStart, subLine, drawState,
					value, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				startPos = endPos;
				if (!deco->ValueAt(startPos)) {
					startPos = deco->EndRun(startPos);
				}
			}
		}
	}

	// Use indicators to highlight matching braces
	if ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACELIGHT)) ||
		(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACEBAD))) {
		const int braceIndicator = (model.bracesMatchStyle == STYLE_BRACELIGHT) ? vsDraw.braceHighlightIndicator : vsDraw.braceBadLightIndicator;
		if (under == vsDraw.indicators[braceIndicator].under) {
			const Range rangeLine(posLineStart + lineStart, posLineEnd);
			if (rangeLine.ContainsCharacter(model.braces[0])) {
				const Sci::Position braceOffset = model.braces[0] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					const Sci::Position secondOffset = model.pdoc->MovePositionOutsideChar(model.braces[0] + 1, 1) - posLineStart;
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, secondOffset,
						subLine, Indicator::drawNormal, 1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
				}
			}
			if (rangeLine.ContainsCharacter(model.braces[1])) {
				const Sci::Position braceOffset = model.braces[1] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					const Sci::Position secondOffset = model.pdoc->MovePositionOutsideChar(model.braces[1] + 1, 1) - posLineStart;
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, secondOffset,
						subLine, Indicator::drawNormal, 1, model.BidirectionalEnabled(), tabWidthMinimumPixels);
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

	if ((model.foldDisplayTextStyle == SC_FOLDDISPLAYTEXT_HIDDEN) || !model.pcs->GetFoldDisplayTextShown(line))
		return;

	PRectangle rcSegment = rcLine;
	const std::string_view foldDisplayText = model.pcs->GetFoldDisplayText(line);
	FontAlias fontText = vsDraw.styles[STYLE_FOLDDISPLAYTEXT].font;
	const int widthFoldDisplayText = static_cast<int>(surface->WidthText(fontText, foldDisplayText));

	int eolInSelection = 0;
	int alpha = SC_ALPHA_NOALPHA;
	if (!hideSelection) {
		const Sci::Position posAfterLineEnd = model.pdoc->LineStart(line + 1);
		eolInSelection = (subLine == (ll->lines - 1)) ? model.sel.InSelectionForEOL(posAfterLineEnd) : 0;
		alpha = (eolInSelection == 1) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
	}

	const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
	const XYPOSITION virtualSpace = model.sel.VirtualSpaceFor(
		model.pdoc->LineEnd(line)) * spaceWidth;
	rcSegment.left = xStart + static_cast<XYPOSITION>(ll->positions[ll->numCharsInLine] - subLineStart) + virtualSpace + vsDraw.aveCharWidth;
	rcSegment.right = rcSegment.left + static_cast<XYPOSITION>(widthFoldDisplayText);

	const ColourOptional background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);
	FontAlias textFont = vsDraw.styles[STYLE_FOLDDISPLAYTEXT].font;
	ColourDesired textFore = vsDraw.styles[STYLE_FOLDDISPLAYTEXT].fore;
	if (eolInSelection && (vsDraw.selColours.fore.isSet)) {
		textFore = (eolInSelection == 1) ? vsDraw.selColours.fore : vsDraw.selAdditionalForeground;
	}
	const ColourDesired textBack = TextBackground(model, vsDraw, ll, background, eolInSelection,
											false, STYLE_FOLDDISPLAYTEXT, -1);

	if (model.trackLineWidth) {
		if (rcSegment.right + 1> lineWidthMaxSeen) {
			// Fold display text border drawn on rcSegment.right with width 1 is the last visble object of the line
			lineWidthMaxSeen = static_cast<int>(rcSegment.right + 1);
		}
	}

	if (phase & drawBack) {
		surface->FillRectangle(rcSegment, textBack);

		// Fill Remainder of the line
		PRectangle rcRemainder = rcSegment;
		rcRemainder.left = rcRemainder.right;
		if (rcRemainder.left < rcLine.left)
			rcRemainder.left = rcLine.left;
		rcRemainder.right = rcLine.right;
		FillLineRemainder(surface, model, vsDraw, ll, line, rcRemainder, subLine);
	}

	if (phase & drawText) {
		if (phasesDraw != phasesOne) {
			surface->DrawTextTransparent(rcSegment, textFont,
				rcSegment.top + vsDraw.maxAscent, foldDisplayText,
				textFore);
		} else {
			surface->DrawTextNoClip(rcSegment, textFont,
				rcSegment.top + vsDraw.maxAscent, foldDisplayText,
				textFore, textBack);
		}
	}

	if (phase & drawIndicatorsFore) {
		if (model.foldDisplayTextStyle == SC_FOLDDISPLAYTEXT_BOXED) {
			surface->PenColour(textFore);
			PRectangle rcBox = rcSegment;
			rcBox.left = round(rcSegment.left);
			rcBox.right = round(rcSegment.right);
			const IntegerRectangle ircBox(rcBox);
			surface->MoveTo(ircBox.left, ircBox.top);
			surface->LineTo(ircBox.left, ircBox.bottom);
			surface->MoveTo(ircBox.right, ircBox.top);
			surface->LineTo(ircBox.right, ircBox.bottom);
			surface->MoveTo(ircBox.left, ircBox.top);
			surface->LineTo(ircBox.right, ircBox.top);
			surface->MoveTo(ircBox.left, ircBox.bottom - 1);
			surface->LineTo(ircBox.right, ircBox.bottom - 1);
		}
	}

	if (phase & drawSelectionTranslucent) {
		if (eolInSelection && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && alpha != SC_ALPHA_NOALPHA) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection), alpha);
		}
	}
}

static constexpr bool AnnotationBoxedOrIndented(int annotationVisible) noexcept {
	return annotationVisible == ANNOTATION_BOXED || annotationVisible == ANNOTATION_INDENTED;
}

void EditView::DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase) {
	const int indent = static_cast<int>(model.pdoc->GetLineIndentation(line) * vsDraw.spaceWidth);
	PRectangle rcSegment = rcLine;
	const int annotationLine = subLine - ll->lines;
	const StyledText stAnnotation = model.pdoc->AnnotationStyledText(line);
	if (stAnnotation.text && ValidStyledText(vsDraw, vsDraw.annotationStyleOffset, stAnnotation)) {
		if (phase & drawBack) {
			surface->FillRectangle(rcSegment, vsDraw.styles[0].back);
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
		if ((phase & drawBack) && AnnotationBoxedOrIndented(vsDraw.annotationVisible)) {
			surface->FillRectangle(rcText,
				vsDraw.styles[stAnnotation.StyleAt(start) + vsDraw.annotationStyleOffset].back);
			rcText.left += vsDraw.spaceWidth;
		}
		DrawStyledText(surface, vsDraw, vsDraw.annotationStyleOffset, rcText,
			stAnnotation, start, lengthAnnotation, phase);
		if ((phase & drawBack) && (vsDraw.annotationVisible == ANNOTATION_BOXED)) {
			surface->PenColour(vsDraw.styles[vsDraw.annotationStyleOffset].fore);
			const IntegerRectangle ircSegment(rcSegment);
			surface->MoveTo(ircSegment.left, ircSegment.top);
			surface->LineTo(ircSegment.left, ircSegment.bottom);
			surface->MoveTo(ircSegment.right, ircSegment.top);
			surface->LineTo(ircSegment.right, ircSegment.bottom);
			if (subLine == ll->lines) {
				surface->MoveTo(ircSegment.left, ircSegment.top);
				surface->LineTo(ircSegment.right, ircSegment.top);
			}
			if (subLine == ll->lines + annotationLines - 1) {
				surface->MoveTo(ircSegment.left, ircSegment.bottom - 1);
				surface->LineTo(ircSegment.right, ircSegment.bottom - 1);
			}
		}
	}
}

static void DrawBlockCaret(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int subLine, int xStart, Sci::Position offset, Sci::Position posCaret, PRectangle rcCaret, ColourDesired caretColour) {

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
	FontAlias fontText = vsDraw.styles[styleMain].font;
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
		if ((vsDraw.IsBlockCaretStyle() || imeCaretBlockOverride) && !drawDrag && posCaret > model.sel.Range(r).anchor) {
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

				const int caretPosition = static_cast<int>(posCaret.Position() - posLineStart - ll->LineStart(subLine));

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
			if ((xposCaret >= 0) && (vsDraw.caretWidth > 0) && (vsDraw.caretStyle != CARETSTYLE_INVISIBLE) &&
				(drawDrag || (caretBlinkState && caretVisibleState))) {
				bool caretAtEOF = false;
				bool caretAtEOL = false;
				bool drawBlockCaret = false;
				XYPOSITION widthOverstrikeCaret;
				XYPOSITION caretWidthOffset = 0;
				PRectangle rcCaret = rcLine;

				if (posCaret.Position() == model.pdoc->Length()) {   // At end of document
					caretAtEOF = true;
					widthOverstrikeCaret = vsDraw.aveCharWidth;
				} else if ((posCaret.Position() - posLineStart) >= ll->numCharsInLine) {	// At end of line
					caretAtEOL = true;
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
				const ViewStyle::CaretShape caretShape = drawDrag ? ViewStyle::CaretShape::line : vsDraw.CaretShapeForMode(model.inOverstrike);
				if (drawDrag) {
					/* Dragging text, use a line caret */
					rcCaret.left = round(xposCaret - caretWidthOffset);
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				} else if ((caretShape == ViewStyle::CaretShape::bar) && drawOverstrikeCaret) {
					/* Overstrike (insert mode), use a modified bar caret */
					rcCaret.top = rcCaret.bottom - 2;
					rcCaret.left = xposCaret + 1;
					rcCaret.right = rcCaret.left + widthOverstrikeCaret - 1;
				} else if ((caretShape == ViewStyle::CaretShape::block) || imeCaretBlockOverride) {
					/* Block caret */
					rcCaret.left = xposCaret;
					if (!caretAtEOL && !caretAtEOF && (ll->chars[offset] != '\t') && !(IsControlCharacter(ll->chars[offset]))) {
						drawBlockCaret = true;
						rcCaret.right = xposCaret + widthOverstrikeCaret;
					} else {
						rcCaret.right = xposCaret + vsDraw.aveCharWidth;
					}
				} else {
					/* Line caret */
					rcCaret.left = round(xposCaret - caretWidthOffset);
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				}
				const ColourDesired caretColour = mainCaret ? vsDraw.caretcolour : vsDraw.additionalCaretColour;
				if (drawBlockCaret) {
					DrawBlockCaret(surface, model, vsDraw, ll, subLine, xStart, offset, posCaret.Position(), rcCaret, caretColour);
				} else {
					surface->FillRectangle(rcCaret, caretColour);
				}
			}
		}
		if (drawDrag)
			break;
	}
}

static void DrawWrapIndentAndMarker(Surface *surface, const ViewStyle &vsDraw, const LineLayout *ll,
	int xStart, PRectangle rcLine, ColourOptional background, DrawWrapMarkerFn customDrawWrapMarker,
	bool caretActive) {
	// default bgnd here..
	surface->FillRectangle(rcLine, background.isSet ? background :
		vsDraw.styles[STYLE_DEFAULT].back);

	if (vsDraw.IsLineFrameOpaque(caretActive, ll->containsCaret)) {
		const int width = vsDraw.GetFrameWidth();
		// Draw left of frame under marker
		DrawFrame(surface, vsDraw.caretLineBackground, vsDraw.caretLineAlpha,
			PRectangle(rcLine.left, rcLine.top, rcLine.left + width, rcLine.bottom));
	}

	if (vsDraw.wrapVisualFlags & SC_WRAPVISUALFLAG_START) {

		// draw continuation rect
		PRectangle rcPlace = rcLine;

		rcPlace.left = static_cast<XYPOSITION>(xStart);
		rcPlace.right = rcPlace.left + ll->wrapIndent;

		if (vsDraw.wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_START_BY_TEXT)
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

void EditView::DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, Range lineRange, Sci::Position posLineStart, int xStart,
	int subLine, ColourOptional background) const {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
	// Does not take margin into account but not significant
	const int xStartVisible = static_cast<int>(subLineStart)-xStart;

	BreakFinder bfBack(ll, &model.sel, lineRange, posLineStart, xStartVisible, selBackDrawn, model.pdoc, &model.reprs, nullptr);

	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background.isSet;

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

			const int inSelection = hideSelection ? 0 : model.sel.CharacterInSelection(iDoc);
			const bool inHotspot = (ll->hotspot.Valid()) && ll->hotspot.ContainsCharacter(iDoc);
			ColourDesired textBack = TextBackground(model, vsDraw, ll, background, inSelection,
				inHotspot, ll->styles[i], i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation))
						textBack = vsDraw.whitespaceColours.back;
				} else {
					// Blob display
					inIndentation = false;
				}
				surface->FillRectangle(rcSegment, textBack);
			} else {
				// Normal text display
				surface->FillRectangle(rcSegment, textBack);
				if (vsDraw.viewWhitespace != wsInvisible) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation)) {
								const PRectangle rcSpace(
									ll->positions[cpos + ts.start] + xStart - static_cast<XYPOSITION>(subLineStart),
									rcSegment.top,
									ll->positions[cpos + ts.start + 1] + xStart - static_cast<XYPOSITION>(subLineStart),
									rcSegment.bottom);
								surface->FillRectangle(rcSpace, vsDraw.whitespaceColours.back);
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
	if (vsDraw.edgeState == EDGE_LINE) {
		PRectangle rcSegment = rcLine;
		const int edgeX = static_cast<int>(vsDraw.theEdge.column * vsDraw.spaceWidth);
		rcSegment.left = static_cast<XYPOSITION>(edgeX + xStart);
		if ((ll->wrapIndent != 0) && (lineRange.start != 0))
			rcSegment.left -= ll->wrapIndent;
		rcSegment.right = rcSegment.left + 1;
		surface->FillRectangle(rcSegment, vsDraw.theEdge.colour);
	} else if (vsDraw.edgeState == EDGE_MULTILINE) {
		for (size_t edge = 0; edge < vsDraw.theMultiEdge.size(); edge++) {
			if (vsDraw.theMultiEdge[edge].column >= 0) {
				PRectangle rcSegment = rcLine;
				const int edgeX = static_cast<int>(vsDraw.theMultiEdge[edge].column * vsDraw.spaceWidth);
				rcSegment.left = static_cast<XYPOSITION>(edgeX + xStart);
				if ((ll->wrapIndent != 0) && (lineRange.start != 0))
					rcSegment.left -= ll->wrapIndent;
				rcSegment.right = rcSegment.left + 1;
				surface->FillRectangle(rcSegment, vsDraw.theMultiEdge[edge].colour);
			}
		}
	}
}

// Draw underline mark as part of background if not transparent
static void DrawMarkUnderline(Surface *surface, const EditModel &model, const ViewStyle &vsDraw,
	Sci::Line line, PRectangle rcLine) {
	int marks = model.pdoc->GetMark(line);
	for (int markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_UNDERLINE) &&
			(vsDraw.markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
			PRectangle rcUnderline = rcLine;
			rcUnderline.top = rcUnderline.bottom - 2;
			surface->FillRectangle(rcUnderline, vsDraw.markers[markBit].back);
		}
		marks >>= 1;
	}
}

static void DrawTranslucentSelection(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine, Range lineRange, int xStart, int tabWidthMinimumPixels) {
	if ((vsDraw.selAlpha != SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha != SC_ALPHA_NOALPHA)) {
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
			const int alpha = (r == model.sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
			if (alpha != SC_ALPHA_NOALPHA) {
				const SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
				if (!portion.Empty()) {
					const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
					if (model.BidirectionalEnabled()) {
						const int selectionStart = static_cast<int>(portion.start.Position() - posLineStart - lineRange.start);
						const int selectionEnd = static_cast<int>(portion.end.Position() - posLineStart - lineRange.start);

						const ColourDesired background = SelectionBackground(vsDraw, r == model.sel.Main(), model.primarySelection);

						const ScreenLine screenLine(ll, subLine, vsDraw, rcLine.right, tabWidthMinimumPixels);
						std::unique_ptr<IScreenLineLayout> slLayout = surface->Layout(&screenLine);

						const std::vector<Interval> intervals = slLayout->FindRangeIntervals(selectionStart, selectionEnd);
						for (const Interval &interval : intervals) {
							const XYPOSITION rcRight = interval.right + xStart;
							const XYPOSITION rcLeft = interval.left + xStart;
							const PRectangle rcSelection(rcLeft, rcLine.top, rcRight, rcLine.bottom);
							SimpleAlphaRectangle(surface, rcSelection, background, alpha);
						}

						if (portion.end.VirtualSpace()) {
							const XYPOSITION xStartVirtual = ll->positions[lineRange.end] -
								static_cast<XYPOSITION>(subLineStart) + xStart;
							PRectangle rcSegment = rcLine;
							rcSegment.left = xStartVirtual + portion.start.VirtualSpace() * spaceWidth;
							rcSegment.right = xStartVirtual + portion.end.VirtualSpace() * spaceWidth;
							SimpleAlphaRectangle(surface, rcSegment, background, alpha);
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
							SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, r == model.sel.Main(), model.primarySelection), alpha);
					}
				}
			}
		}
	}
}

// Draw any translucent whole line states
static void DrawTranslucentLineState(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, PRectangle rcLine, int subLine) {
	if ((model.caret.active || vsDraw.alwaysShowCaretLineBackground) && vsDraw.showCaretLineBackground && ll->containsCaret &&
		vsDraw.caretLineAlpha != SC_ALPHA_NOALPHA) {
		if (vsDraw.caretLineFrame) {
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		} else {
			SimpleAlphaRectangle(surface, rcLine, vsDraw.caretLineBackground, vsDraw.caretLineAlpha);
		}
	}
	const int marksOfLine = model.pdoc->GetMark(line);
	int marksDrawnInText = marksOfLine & vsDraw.maskDrawInText;
	for (int markBit = 0; (markBit < 32) && marksDrawnInText; markBit++) {
		if (marksDrawnInText & 1) {
			if (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND) {
				SimpleAlphaRectangle(surface, rcLine, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
			} else if (vsDraw.markers[markBit].markType == SC_MARK_UNDERLINE) {
				PRectangle rcUnderline = rcLine;
				rcUnderline.top = rcUnderline.bottom - 2;
				SimpleAlphaRectangle(surface, rcUnderline, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
			}
		}
		marksDrawnInText >>= 1;
	}
	int marksDrawnInLine = marksOfLine & vsDraw.maskInLine;
	for (int markBit = 0; (markBit < 32) && marksDrawnInLine; markBit++) {
		if (marksDrawnInLine & 1) {
			SimpleAlphaRectangle(surface, rcLine, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
		}
		marksDrawnInLine >>= 1;
	}
}

void EditView::DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line lineVisible, PRectangle rcLine, Range lineRange, Sci::Position posLineStart, int xStart,
	int subLine, ColourOptional background) {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background.isSet;
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.

	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
	const XYPOSITION indentWidth = model.pdoc->IndentSize() * vsDraw.spaceWidth;

	// Does not take margin into account but not significant
	const int xStartVisible = static_cast<int>(subLineStart)-xStart;

	// Foreground drawing loop
	BreakFinder bfFore(ll, &model.sel, lineRange, posLineStart, xStartVisible,
		(((phasesDraw == phasesOne) && selBackDrawn) || vsDraw.selColours.fore.isSet), model.pdoc, &model.reprs, &vsDraw);

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
			ColourDesired textFore = vsDraw.styles[styleMain].fore;
			FontAlias textFont = vsDraw.styles[styleMain].font;
			//hotspot foreground
			const bool inHotspot = (ll->hotspot.Valid()) && ll->hotspot.ContainsCharacter(iDoc);
			if (inHotspot) {
				if (vsDraw.hotspotColours.fore.isSet)
					textFore = vsDraw.hotspotColours.fore;
			}
			if (vsDraw.indicatorsSetFore) {
				// At least one indicator sets the text colour so see if it applies to this segment
				for (const IDecoration *deco : model.pdoc->decorations->View()) {
					const int indicatorValue = deco->ValueAt(ts.start + posLineStart);
					if (indicatorValue) {
						const Indicator &indicator = vsDraw.indicators[deco->Indicator()];
						const bool hover = indicator.IsDynamic() &&
							((model.hoverIndicatorPos >= ts.start + posLineStart) &&
							(model.hoverIndicatorPos <= ts.end() + posLineStart));
						if (hover) {
							if (indicator.sacHover.style == INDIC_TEXTFORE) {
								textFore = indicator.sacHover.fore;
							}
						} else {
							if (indicator.sacNormal.style == INDIC_TEXTFORE) {
								if (indicator.Flags() & SC_INDICFLAG_VALUEFORE)
									textFore = ColourDesired(indicatorValue & SC_INDICVALUEMASK);
								else
									textFore = indicator.sacNormal.fore;
							}
						}
					}
				}
			}
			const int inSelection = hideSelection ? 0 : model.sel.CharacterInSelection(iDoc);
			if (inSelection && (vsDraw.selColours.fore.isSet)) {
				textFore = (inSelection == 1) ? vsDraw.selColours.fore : vsDraw.selAdditionalForeground;
			}
			ColourDesired textBack = TextBackground(model, vsDraw, ll, background, inSelection, inHotspot, styleMain, i);
			if (ts.representation) {
				if (ll->chars[i] == '\t') {
					// Tab display
					if (phasesDraw == phasesOne) {
						if (drawWhitespaceBackground && vsDraw.WhiteSpaceVisible(inIndentation))
							textBack = vsDraw.whitespaceColours.back;
						surface->FillRectangle(rcSegment, textBack);
					}
					if (inIndentation && vsDraw.viewIndentationGuides == ivReal) {
						for (int indentCount = static_cast<int>((ll->positions[i] + epsilon) / indentWidth);
							indentCount <= (ll->positions[i + 1] - epsilon) / indentWidth;
							indentCount++) {
							if (indentCount > 0) {
								const XYPOSITION xIndent = floor(indentCount * indentWidth);
								DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
									(ll->xHighlightGuide == xIndent));
							}
						}
					}
					if (vsDraw.viewWhitespace != wsInvisible) {
						if (vsDraw.WhiteSpaceVisible(inIndentation)) {
							if (vsDraw.whitespaceColours.fore.isSet)
								textFore = vsDraw.whitespaceColours.fore;
							surface->PenColour(textFore);
							const PRectangle rcTab(rcSegment.left + 1, rcSegment.top + tabArrowHeight,
								rcSegment.right - 1, rcSegment.bottom - vsDraw.maxDescent);
							const int segmentTop = static_cast<int>(rcSegment.top + vsDraw.lineHeight / 2);
							if (!customDrawTabArrow)
								DrawTabArrow(surface, rcTab, segmentTop, vsDraw);
							else
								customDrawTabArrow(surface, rcTab, segmentTop);
						}
					}
				} else {
					inIndentation = false;
					if (vsDraw.controlCharSymbol >= 32) {
						// Using one font for all control characters so it can be controlled independently to ensure
						// the box goes around the characters tightly. Seems to be no way to work out what height
						// is taken by an individual character - internal leading gives varying results.
						FontAlias ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;
						const char cc[2] = { static_cast<char>(vsDraw.controlCharSymbol), '\0' };
						surface->DrawTextNoClip(rcSegment, ctrlCharsFont,
							rcSegment.top + vsDraw.maxAscent,
							cc, textBack, textFore);
					} else {
						DrawTextBlob(surface, vsDraw, rcSegment, ts.representation->stringRep,
							textBack, textFore, phasesDraw == phasesOne);
					}
				}
			} else {
				// Normal text display
				if (vsDraw.styles[styleMain].visible) {
					const std::string_view text(&ll->chars[ts.start], i - ts.start + 1);
					if (phasesDraw != phasesOne) {
						surface->DrawTextTransparent(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, text, textFore);
					} else {
						surface->DrawTextNoClip(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, text, textFore, textBack);
					}
				}
				if (vsDraw.viewWhitespace != wsInvisible ||
					(inIndentation && vsDraw.viewIndentationGuides != ivNone)) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (vsDraw.viewWhitespace != wsInvisible) {
								if (vsDraw.whitespaceColours.fore.isSet)
									textFore = vsDraw.whitespaceColours.fore;
								if (vsDraw.WhiteSpaceVisible(inIndentation)) {
									const XYPOSITION xmid = (ll->positions[cpos + ts.start] + ll->positions[cpos + ts.start + 1]) / 2;
									if ((phasesDraw == phasesOne) && drawWhitespaceBackground) {
										textBack = vsDraw.whitespaceColours.back;
										const PRectangle rcSpace(
											ll->positions[cpos + ts.start] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.top,
											ll->positions[cpos + ts.start + 1] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.bottom);
										surface->FillRectangle(rcSpace, textBack);
									}
									const int halfDotWidth = vsDraw.whitespaceSize / 2;
									PRectangle rcDot(xmid + xStart - halfDotWidth - static_cast<XYPOSITION>(subLineStart),
										rcSegment.top + vsDraw.lineHeight / 2, 0.0f, 0.0f);
									rcDot.right = rcDot.left + vsDraw.whitespaceSize;
									rcDot.bottom = rcDot.top + vsDraw.whitespaceSize;
									surface->FillRectangle(rcDot, textFore);
								}
							}
							if (inIndentation && vsDraw.viewIndentationGuides == ivReal) {
								for (int indentCount = static_cast<int>((ll->positions[cpos + ts.start] + epsilon) / indentWidth);
									indentCount <= (ll->positions[cpos + ts.start + 1] - epsilon) / indentWidth;
									indentCount++) {
									if (indentCount > 0) {
										const XYPOSITION xIndent = floor(indentCount * indentWidth);
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
			if (ll->hotspot.Valid() && vsDraw.hotspotUnderline && ll->hotspot.ContainsCharacter(iDoc)) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				if (vsDraw.hotspotColours.fore.isSet)
					surface->FillRectangle(rcUL, vsDraw.hotspotColours.fore);
				else
					surface->FillRectangle(rcUL, textFore);
			} else if (vsDraw.styles[styleMain].underline) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				surface->FillRectangle(rcUL, textFore);
			}
		} else if (rcSegment.left > rcLine.right) {
			break;
		}
	}
}

void EditView::DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	Sci::Line line, Sci::Line lineVisible, PRectangle rcLine, int xStart, int subLine) {
	if ((vsDraw.viewIndentationGuides == ivLookForward || vsDraw.viewIndentationGuides == ivLookBoth)
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
			const int isFoldHeader = model.pdoc->GetLevel(lineLastWithText) & SC_FOLDLEVELHEADERFLAG;
			if (isFoldHeader) {
				// Level is one more level than parent
				indentLastWithText += model.pdoc->IndentSize();
			}
			if (vsDraw.viewIndentationGuides == ivLookForward) {
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
			const XYPOSITION xIndent = floor(indentPos * vsDraw.spaceWidth);
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

	// See if something overrides the line background color.
	const ColourOptional background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);

	const Sci::Position posLineStart = model.pdoc->LineStart(line);

	const Range lineRange = ll->SubLineRange(subLine, LineLayout::Scope::visibleOnly);
	const Range lineRangeIncludingEnd = ll->SubLineRange(subLine, LineLayout::Scope::includeEnd);
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];

	if ((ll->wrapIndent != 0) && (subLine > 0)) {
		if (phase & drawBack) {
			DrawWrapIndentAndMarker(surface, vsDraw, ll, xStart, rcLine, background, customDrawWrapMarker, model.caret.active);
		}
		xStart += static_cast<int>(ll->wrapIndent);
	}

	if (phasesDraw != phasesOne) {
		if (phase & drawBack) {
			DrawBackground(surface, model, vsDraw, ll, rcLine, lineRange, posLineStart, xStart,
				subLine, background);
			DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, drawBack);
			phase = static_cast<DrawPhase>(phase & ~drawBack);	// Remove drawBack to not draw again in DrawFoldDisplayText
			DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
				xStart, subLine, subLineStart, background);
			if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
				DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		}

		if (phase & drawIndicatorsBack) {
			DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
				lineRangeIncludingEnd.end, true, tabWidthMinimumPixels);
			DrawEdgeLine(surface, vsDraw, ll, rcLine, lineRange, xStart);
			DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
		}
	}

	if (phase & drawText) {
		DrawForeground(surface, model, vsDraw, ll, lineVisible, rcLine, lineRange, posLineStart, xStart,
			subLine, background);
	}

	if (phase & drawIndentationGuides) {
		DrawIndentGuidesOverEmpty(surface, model, vsDraw, ll, line, lineVisible, rcLine, xStart, subLine);
	}

	if (phase & drawIndicatorsFore) {
		DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine,
			lineRangeIncludingEnd.end, false, tabWidthMinimumPixels);
	}

	DrawFoldDisplayText(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, subLineStart, phase);

	if (phasesDraw == phasesOne) {
		DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
			xStart, subLine, subLineStart, background);
		if (vsDraw.IsLineFrameOpaque(model.caret.active, ll->containsCaret))
			DrawCaretLineFramed(surface, vsDraw, ll, rcLine, subLine);
		DrawEdgeLine(surface, vsDraw, ll, rcLine, lineRange, xStart);
		DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
	}

	if (!hideSelection && (phase & drawSelectionTranslucent)) {
		DrawTranslucentSelection(surface, model, vsDraw, ll, line, rcLine, subLine, lineRange, xStart, tabWidthMinimumPixels);
	}

	if (phase & drawLineTranslucent) {
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine, subLine);
	}
}

static void DrawFoldLines(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, Sci::Line line, PRectangle rcLine) {
	const bool expanded = model.pcs->GetExpanded(line);
	const int level = model.pdoc->GetLevel(line);
	const int levelNext = model.pdoc->GetLevel(line + 1);
	if ((level & SC_FOLDLEVELHEADERFLAG) &&
		(LevelNumber(level) < LevelNumber(levelNext))) {
		// Paint the line above the fold
		if ((expanded && (model.foldFlags & SC_FOLDFLAG_LINEBEFORE_EXPANDED))
			||
			(!expanded && (model.foldFlags & SC_FOLDFLAG_LINEBEFORE_CONTRACTED))) {
			PRectangle rcFoldLine = rcLine;
			rcFoldLine.bottom = rcFoldLine.top + 1;
			surface->FillRectangle(rcFoldLine, vsDraw.styles[STYLE_DEFAULT].fore);
		}
		// Paint the line below the fold
		if ((expanded && (model.foldFlags & SC_FOLDFLAG_LINEAFTER_EXPANDED))
			||
			(!expanded && (model.foldFlags & SC_FOLDFLAG_LINEAFTER_CONTRACTED))) {
			PRectangle rcFoldLine = rcLine;
			rcFoldLine.top = rcFoldLine.bottom - 1;
			surface->FillRectangle(rcFoldLine, vsDraw.styles[STYLE_DEFAULT].fore);
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
		surface->SetUnicodeMode(SC_CP_UTF8 == model.pdoc->dbcsCodePage);
		surface->SetDBCSMode(model.pdoc->dbcsCodePage);
		surface->SetBidiR2L(model.BidirectionalR2L());

		const Point ptOrigin = model.GetVisibleOriginInMain();

		const int screenLinePaintFirst = static_cast<int>(rcArea.top) / vsDraw.lineHeight;
		const int xStart = vsDraw.textStart - model.xOffset + static_cast<int>(ptOrigin.x);

		SelectionPosition posCaret = model.sel.RangeMain().caret;
		if (model.posDrag.IsValid())
			posCaret = model.posDrag;
		const Sci::Line lineCaret = model.pdoc->SciLineFromPosition(posCaret.Position());

		PRectangle rcTextArea = rcClient;
		if (vsDraw.marginInside) {
			rcTextArea.left += vsDraw.textStart;
			rcTextArea.right -= vsDraw.rightMarginWidth;
		} else {
			rcTextArea = rcArea;
		}

		// Remove selection margin from drawing area so text will not be drawn
		// on it in unbuffered mode.
		if (!bufferedDraw && vsDraw.marginInside) {
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
		const bool bracesIgnoreStyle = ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACELIGHT)) ||
			(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACEBAD)));

		Sci::Line lineDocPrevious = -1;	// Used to avoid laying out one document line multiple times
		AutoLineLayout ll(llc, nullptr);
		std::vector<DrawPhase> phases;
		if ((phasesDraw == phasesMultiple) && !bufferedDraw) {
			for (DrawPhase phase = drawBack; phase <= drawCarets; phase = static_cast<DrawPhase>(phase * 2)) {
				phases.push_back(phase);
			}
		} else {
			phases.push_back(drawAll);
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
					ll.Set(nullptr);
					ll.Set(RetrieveLineLayout(lineDoc, model));
					LayoutLine(model, lineDoc, surface, vsDraw, ll, model.wrapWidth);
					lineDocPrevious = lineDoc;
				}
#if defined(TIME_PAINTING)
				durLayout += ep.Duration(true);
#endif
				if (ll) {
					ll->containsCaret = !hideSelection && (lineDoc == lineCaret);
					ll->hotspot = model.GetHotSpotRange();

					PRectangle rcLine = rcTextArea;
					rcLine.top = static_cast<XYPOSITION>(ypos);
					rcLine.bottom = static_cast<XYPOSITION>(ypos + vsDraw.lineHeight);

					const Range rangeLine(model.pdoc->LineStart(lineDoc),
						model.pdoc->LineStart(lineDoc + 1));

					// Highlight the current braces if any
					ll->SetBracesHighlight(rangeLine, model.braces, static_cast<char>(model.bracesMatchStyle),
						static_cast<int>(model.highlightGuideColumn * vsDraw.spaceWidth), bracesIgnoreStyle);

					if (leftTextOverlap && (bufferedDraw || ((phasesDraw < phasesMultiple) && (phase & drawBack)))) {
						// Clear the left margin
						PRectangle rcSpacer = rcLine;
						rcSpacer.right = rcSpacer.left;
						rcSpacer.left -= 1;
						surface->FillRectangle(rcSpacer, vsDraw.styles[STYLE_DEFAULT].back);
					}

					if (model.BidirectionalEnabled()) {
						// Fill the line bidi data
						UpdateBidiData(model, vsDraw, ll);
					}

					DrawLine(surface, model, vsDraw, ll, lineDoc, visibleLine, xStart, rcLine, subLine, phase);
#if defined(TIME_PAINTING)
					durPaint += ep.Duration(true);
#endif
					// Restore the previous styles for the brace highlights in case layout is in cache.
					ll->RestoreBracesHighlight(rangeLine, model.braces, bracesIgnoreStyle);

					if (phase & drawFoldLines) {
						DrawFoldLines(surface, model, vsDraw, lineDoc, rcLine);
					}

					if (phase & drawCarets) {
						DrawCarets(surface, model, vsDraw, ll, lineDoc, xStart, rcLine, subLine);
					}

					if (bufferedDraw) {
						const Point from = Point::FromInts(vsDraw.textStart - leftTextOverlap, 0);
						const PRectangle rcCopyArea = PRectangle::FromInts(vsDraw.textStart - leftTextOverlap, yposScreen,
							static_cast<int>(rcClient.right - vsDraw.rightMarginWidth),
							yposScreen + vsDraw.lineHeight);
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
		ll.Set(nullptr);
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
			surfaceWindow->FillRectangle(rcBeyondEOF, vsDraw.styles[STYLE_DEFAULT].back);
			if (vsDraw.edgeState == EDGE_LINE) {
				const int edgeX = static_cast<int>(vsDraw.theEdge.column * vsDraw.spaceWidth);
				rcBeyondEOF.left = static_cast<XYPOSITION>(edgeX + xStart);
				rcBeyondEOF.right = rcBeyondEOF.left + 1;
				surfaceWindow->FillRectangle(rcBeyondEOF, vsDraw.theEdge.colour);
			} else if (vsDraw.edgeState == EDGE_MULTILINE) {
				for (size_t edge = 0; edge < vsDraw.theMultiEdge.size(); edge++) {
					if (vsDraw.theMultiEdge[edge].column >= 0) {
						const int edgeX = static_cast<int>(vsDraw.theMultiEdge[edge].column * vsDraw.spaceWidth);
						rcBeyondEOF.left = static_cast<XYPOSITION>(edgeX + xStart);
						rcBeyondEOF.right = rcBeyondEOF.left + 1;
						surfaceWindow->FillRectangle(rcBeyondEOF, vsDraw.theMultiEdge[edge].colour);
					}
				}
			}
		}
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
		int eolInSelection = 0;
		int alpha = SC_ALPHA_NOALPHA;
		if (!hideSelection) {
			const Sci::Position posAfterLineEnd = model.pdoc->LineStart(line + 1);
			eolInSelection = (subLine == (ll->lines - 1)) ? model.sel.InSelectionForEOL(posAfterLineEnd) : 0;
			alpha = (eolInSelection == 1) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
		}

		const ColourOptional background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);

		if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha == SC_ALPHA_NOALPHA)) {
			surface->FillRectangle(rcArea, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection));
		} else {
			if (background.isSet) {
				surface->FillRectangle(rcArea, background);
			} else if (vsDraw.styles[ll->styles[ll->numCharsInLine]].eolFilled) {
				surface->FillRectangle(rcArea, vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
			} else {
				surface->FillRectangle(rcArea, vsDraw.styles[STYLE_DEFAULT].back);
			}
			if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
				SimpleAlphaRectangle(surface, rcArea, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection), alpha);
			}
		}
}

// Space (3 space characters) between line numbers and text when printing.
#define lineNumberPrintSpace "   "

static ColourDesired InvertedLight(ColourDesired orig) noexcept {
	unsigned int r = orig.GetRed();
	unsigned int g = orig.GetGreen();
	unsigned int b = orig.GetBlue();
	const unsigned int l = (r + g + b) / 3; 	// There is a better calculation for this that matches human eye
	const unsigned int il = 0xff - l;
	if (l == 0)
		return ColourDesired(0xff, 0xff, 0xff);
	r = r * il / l;
	g = g * il / l;
	b = b * il / l;
	return ColourDesired(std::min(r, 0xffu), std::min(g, 0xffu), std::min(b, 0xffu));
}

Sci::Position EditView::FormatRange(bool draw, const Sci_RangeToFormat *pfr, Surface *surface, Surface *surfaceMeasure,
	const EditModel &model, const ViewStyle &vs) {
	// Can't use measurements cached for screen
	posCache.Clear();

	ViewStyle vsPrint(vs);
	vsPrint.technology = SC_TECHNOLOGY_DEFAULT;

	// Modify the view style for printing as do not normally want any of the transient features to be printed
	// Printing supports only the line number margin.
	int lineNumberIndex = -1;
	for (size_t margin = 0; margin < vs.ms.size(); margin++) {
		if ((vsPrint.ms[margin].style == SC_MARGIN_NUMBER) && (vsPrint.ms[margin].width > 0)) {
			lineNumberIndex = static_cast<int>(margin);
		} else {
			vsPrint.ms[margin].width = 0;
		}
	}
	vsPrint.fixedColumnWidth = 0;
	vsPrint.zoomLevel = printParameters.magnification;
	// Don't show indentation guides
	// If this ever gets changed, cached pixmap would need to be recreated if technology != SC_TECHNOLOGY_DEFAULT
	vsPrint.viewIndentationGuides = ivNone;
	// Don't show the selection when printing
	vsPrint.selColours.back.isSet = false;
	vsPrint.selColours.fore.isSet = false;
	vsPrint.selAlpha = SC_ALPHA_NOALPHA;
	vsPrint.selAdditionalAlpha = SC_ALPHA_NOALPHA;
	vsPrint.whitespaceColours.back.isSet = false;
	vsPrint.whitespaceColours.fore.isSet = false;
	vsPrint.showCaretLineBackground = false;
	vsPrint.alwaysShowCaretLineBackground = false;
	// Don't highlight matching braces using indicators
	vsPrint.braceHighlightIndicatorSet = false;
	vsPrint.braceBadLightIndicatorSet = false;

	// Set colours for printing according to users settings
	for (size_t sty = 0; sty < vsPrint.styles.size(); sty++) {
		if (printParameters.colourMode == SC_PRINT_INVERTLIGHT) {
			vsPrint.styles[sty].fore = InvertedLight(vsPrint.styles[sty].fore);
			vsPrint.styles[sty].back = InvertedLight(vsPrint.styles[sty].back);
		} else if (printParameters.colourMode == SC_PRINT_BLACKONWHITE) {
			vsPrint.styles[sty].fore = ColourDesired(0, 0, 0);
			vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
		} else if (printParameters.colourMode == SC_PRINT_COLOURONWHITE) {
			vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
		} else if (printParameters.colourMode == SC_PRINT_COLOURONWHITEDEFAULTBG) {
			if (sty <= STYLE_DEFAULT) {
				vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
			}
		}
	}
	// White background for the line numbers if SC_PRINT_SCREENCOLOURS isn't used
	if (printParameters.colourMode != SC_PRINT_SCREENCOLOURS)
		vsPrint.styles[STYLE_LINENUMBER].back = ColourDesired(0xff, 0xff, 0xff);

	// Printing uses different margins, so reset screen margins
	vsPrint.leftMarginWidth = 0;
	vsPrint.rightMarginWidth = 0;

	vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);
	// Determining width must happen after fonts have been realised in Refresh
	int lineNumberWidth = 0;
	if (lineNumberIndex >= 0) {
		lineNumberWidth = static_cast<int>(surfaceMeasure->WidthText(vsPrint.styles[STYLE_LINENUMBER].font,
			"99999" lineNumberPrintSpace));
		vsPrint.ms[lineNumberIndex].width = lineNumberWidth;
		vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);	// Recalculate fixedColumnWidth
	}

	const Sci::Line linePrintStart =
		model.pdoc->SciLineFromPosition(static_cast<Sci::Position>(pfr->chrg.cpMin));
	Sci::Line linePrintLast = linePrintStart + (pfr->rc.bottom - pfr->rc.top) / vsPrint.lineHeight - 1;
	if (linePrintLast < linePrintStart)
		linePrintLast = linePrintStart;
	const Sci::Line linePrintMax =
		model.pdoc->SciLineFromPosition(static_cast<Sci::Position>(pfr->chrg.cpMax));
	if (linePrintLast > linePrintMax)
		linePrintLast = linePrintMax;
	//Platform::DebugPrintf("Formatting lines=[%0d,%0d,%0d] top=%0d bottom=%0d line=%0d %0d\n",
	//      linePrintStart, linePrintLast, linePrintMax, pfr->rc.top, pfr->rc.bottom, vsPrint.lineHeight,
	//      surfaceMeasure->Height(vsPrint.styles[STYLE_LINENUMBER].font));
	Sci::Position endPosPrint = model.pdoc->Length();
	if (linePrintLast < model.pdoc->LinesTotal())
		endPosPrint = model.pdoc->LineStart(linePrintLast + 1);

	// Ensure we are styled to where we are formatting.
	model.pdoc->EnsureStyledTo(endPosPrint);

	const int xStart = vsPrint.fixedColumnWidth + pfr->rc.left;
	int ypos = pfr->rc.top;

	Sci::Line lineDoc = linePrintStart;

	Sci::Position nPrintPos = static_cast<Sci::Position>(pfr->chrg.cpMin);
	int visibleLine = 0;
	int widthPrint = pfr->rc.right - pfr->rc.left - vsPrint.fixedColumnWidth;
	if (printParameters.wrapState == eWrapNone)
		widthPrint = LineLayout::wrapWidthInfinite;

	while (lineDoc <= linePrintLast && ypos < pfr->rc.bottom) {

		// When printing, the hdc and hdcTarget may be the same, so
		// changing the state of surfaceMeasure may change the underlying
		// state of surface. Therefore, any cached state is discarded before
		// using each surface.
		surfaceMeasure->FlushCachedState();

		// Copy this line and its styles from the document into local arrays
		// and determine the x position at which each character starts.
		LineLayout ll(static_cast<int>(model.pdoc->LineStart(lineDoc + 1) - model.pdoc->LineStart(lineDoc) + 1));
		LayoutLine(model, lineDoc, surfaceMeasure, vsPrint, &ll, widthPrint);

		ll.containsCaret = false;

		PRectangle rcLine = PRectangle::FromInts(
			pfr->rc.left,
			ypos,
			pfr->rc.right - 1,
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
			(ypos + vsPrint.lineHeight <= pfr->rc.bottom) &&
			(visibleLine >= 0)) {
			const std::string number = std::to_string(lineDoc + 1) + lineNumberPrintSpace;
			PRectangle rcNumber = rcLine;
			rcNumber.right = rcNumber.left + lineNumberWidth;
			// Right justify
			rcNumber.left = rcNumber.right - surfaceMeasure->WidthText(
				vsPrint.styles[STYLE_LINENUMBER].font, number);
			surface->FlushCachedState();
			surface->DrawTextNoClip(rcNumber, vsPrint.styles[STYLE_LINENUMBER].font,
				static_cast<XYPOSITION>(ypos + vsPrint.maxAscent), number,
				vsPrint.styles[STYLE_LINENUMBER].fore,
				vsPrint.styles[STYLE_LINENUMBER].back);
		}

		// Draw the line
		surface->FlushCachedState();

		for (int iwl = 0; iwl < ll.lines; iwl++) {
			if (ypos + vsPrint.lineHeight <= pfr->rc.bottom) {
				if (visibleLine >= 0) {
					if (draw) {
						rcLine.top = static_cast<XYPOSITION>(ypos);
						rcLine.bottom = static_cast<XYPOSITION>(ypos + vsPrint.lineHeight);
						DrawLine(surface, model, vsPrint, &ll, lineDoc, visibleLine, xStart, rcLine, iwl, drawAll);
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
	posCache.Clear();

	return nPrintPos;
}
