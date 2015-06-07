// Scintilla source code edit control
/** @file Editor.cxx
 ** Defines the appearance of the main text area of the editor window.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "StringCopy.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
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

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static inline bool IsControlCharacter(int ch) {
	// iscntrl returns true for lots of chars > 127 which are displayable
	return ch >= 0 && ch < ' ';
}

PrintParameters::PrintParameters() {
	magnification = 0;
	colourMode = SC_PRINT_NORMAL;
	wrapState = eWrapWord;
}

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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
		size_t style = styles[start];
		size_t endSegment = start;
		while ((endSegment + 1 < len) && (static_cast<size_t>(styles[endSegment + 1]) == style))
			endSegment++;
		FontAlias fontText = vs.styles[style + styleOffset].font;
		width += static_cast<int>(surface->WidthText(fontText, text + start,
			static_cast<int>(endSegment - start + 1)));
		start = endSegment + 1;
	}
	return width;
}

int WidestLineWidth(Surface *surface, const ViewStyle &vs, int styleOffset, const StyledText &st) {
	int widthMax = 0;
	size_t start = 0;
	while (start < st.length) {
		size_t lenLine = st.LineLength(start);
		int widthSubLine;
		if (st.multipleStyles) {
			widthSubLine = WidthStyledText(surface, vs, styleOffset, st.text + start, st.styles + start, lenLine);
		} else {
			FontAlias fontText = vs.styles[styleOffset + st.style].font;
			widthSubLine = static_cast<int>(surface->WidthText(fontText,
				st.text + start, static_cast<int>(lenLine)));
		}
		if (widthSubLine > widthMax)
			widthMax = widthSubLine;
		start += lenLine + 1;
	}
	return widthMax;
}

void DrawTextNoClipPhase(Surface *surface, PRectangle rc, const Style &style, XYPOSITION ybase,
	const char *s, int len, DrawPhase phase) {
	FontAlias fontText = style.font;
	if (phase & drawBack) {
		if (phase & drawText) {
			// Drawing both
			surface->DrawTextNoClip(rc, fontText, ybase, s, len,
				style.fore, style.back);
		} else {
			surface->FillRectangle(rc, style.back);
		}
	} else if (phase & drawText) {
		surface->DrawTextTransparent(rc, fontText, ybase, s, len, style.fore);
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
			const int width = static_cast<int>(surface->WidthText(fontText,
				st.text + start + i, static_cast<int>(end - i + 1)));
			PRectangle rcSegment = rcText;
			rcSegment.left = static_cast<XYPOSITION>(x);
			rcSegment.right = static_cast<XYPOSITION>(x + width + 1);
			DrawTextNoClipPhase(surface, rcSegment, vs.styles[style],
				rcText.top + vs.maxAscent, st.text + start + i,
				static_cast<int>(end - i + 1), phase);
			x += width;
			i = end + 1;
		}
	} else {
		const size_t style = st.style + styleOffset;
		DrawTextNoClipPhase(surface, rcText, vs.styles[style],
			rcText.top + vs.maxAscent, st.text + start,
			static_cast<int>(length), phase);
	}
}

#ifdef SCI_NAMESPACE
}
#endif

const XYPOSITION epsilon = 0.0001f;	// A small nudge to avoid floating point precision issues

EditView::EditView() {
	ldTabstops = NULL;
	tabWidthMinimumPixels = 2; // needed for calculating tab stops for fractional proportional fonts
	hideSelection = false;
	drawOverstrikeCaret = true;
	bufferedDraw = true;
	phasesDraw = phasesTwo;
	lineWidthMaxSeen = 0;
	additionalCaretsBlink = true;
	additionalCaretsVisible = true;
	imeCaretBlockOverride = false;
	pixmapLine = 0;
	pixmapIndentGuide = 0;
	pixmapIndentGuideHighlight = 0;
	llc.SetLevel(LineLayoutCache::llcCaret);
	posCache.SetSize(0x400);
	tabArrowHeight = 4;
	customDrawTabArrow = NULL;
	customDrawWrapMarker = NULL;
}

EditView::~EditView() {
	delete ldTabstops;
	ldTabstops = NULL;
}

bool EditView::SetTwoPhaseDraw(bool twoPhaseDraw) {
	const PhasesDraw phasesDrawNew = twoPhaseDraw ? phasesTwo : phasesOne;
	const bool redraw = phasesDraw != phasesDrawNew;
	phasesDraw = phasesDrawNew;
	return redraw;
}

bool EditView::SetPhasesDraw(int phases) {
	const PhasesDraw phasesDrawNew = static_cast<PhasesDraw>(phases);
	const bool redraw = phasesDraw != phasesDrawNew;
	phasesDraw = phasesDrawNew;
	return redraw;
}

bool EditView::LinesOverlap() const {
	return phasesDraw == phasesMultiple;
}

void EditView::ClearAllTabstops() {
	delete ldTabstops;
	ldTabstops = 0;
}

XYPOSITION EditView::NextTabstopPos(int line, XYPOSITION x, XYPOSITION tabWidth) const {
	int next = GetNextTabstop(line, static_cast<int>(x + tabWidthMinimumPixels));
	if (next > 0)
		return static_cast<XYPOSITION>(next);
	return (static_cast<int>((x + tabWidthMinimumPixels) / tabWidth) + 1) * tabWidth;
}

bool EditView::ClearTabstops(int line) {
	LineTabstops *lt = static_cast<LineTabstops *>(ldTabstops);
	return lt && lt->ClearTabstops(line);
}

bool EditView::AddTabstop(int line, int x) {
	if (!ldTabstops) {
		ldTabstops = new LineTabstops();
	}
	LineTabstops *lt = static_cast<LineTabstops *>(ldTabstops);
	return lt && lt->AddTabstop(line, x);
}

int EditView::GetNextTabstop(int line, int x) const {
	LineTabstops *lt = static_cast<LineTabstops *>(ldTabstops);
	if (lt) {
		return lt->GetNextTabstop(line, x);
	} else {
		return 0;
	}
}

void EditView::LinesAddedOrRemoved(int lineOfPos, int linesAdded) {
	if (ldTabstops) {
		if (linesAdded > 0) {
			for (int line = lineOfPos; line < lineOfPos + linesAdded; line++) {
				ldTabstops->InsertLine(line);
			}
		} else {
			for (int line = (lineOfPos + -linesAdded) - 1; line >= lineOfPos; line--) {
				ldTabstops->RemoveLine(line);
			}
		}
	}
}

void EditView::DropGraphics(bool freeObjects) {
	if (freeObjects) {
		delete pixmapLine;
		pixmapLine = 0;
		delete pixmapIndentGuide;
		pixmapIndentGuide = 0;
		delete pixmapIndentGuideHighlight;
		pixmapIndentGuideHighlight = 0;
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
		pixmapLine = Surface::Allocate(vsDraw.technology);
	if (!pixmapIndentGuide)
		pixmapIndentGuide = Surface::Allocate(vsDraw.technology);
	if (!pixmapIndentGuideHighlight)
		pixmapIndentGuideHighlight = Surface::Allocate(vsDraw.technology);
}

const char *ControlCharacterString(unsigned char ch) {
	const char *reps[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
	};
	if (ch < ELEMENTS(reps)) {
		return reps[ch];
	} else {
		return "BAD";
	}
}

void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid) {
	int ydiff = static_cast<int>(rcTab.bottom - rcTab.top) / 2;
	int xhead = static_cast<int>(rcTab.right) - 1 - ydiff;
	if (xhead <= rcTab.left) {
		ydiff -= static_cast<int>(rcTab.left) - xhead - 1;
		xhead = static_cast<int>(rcTab.left) - 1;
	}
	if ((rcTab.left + 2) < (rcTab.right - 1))
		surface->MoveTo(static_cast<int>(rcTab.left) + 2, ymid);
	else
		surface->MoveTo(static_cast<int>(rcTab.right) - 1, ymid);
	surface->LineTo(static_cast<int>(rcTab.right) - 1, ymid);
	surface->LineTo(xhead, ymid - ydiff);
	surface->MoveTo(static_cast<int>(rcTab.right) - 1, ymid);
	surface->LineTo(xhead, ymid + ydiff);
}

void EditView::RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw) {
	if (!pixmapIndentGuide->Initialised()) {
		// 1 extra pixel in height so can handle odd/even positions and so produce a continuous line
		pixmapIndentGuide->InitPixMap(1, vsDraw.lineHeight + 1, surfaceWindow, wid);
		pixmapIndentGuideHighlight->InitPixMap(1, vsDraw.lineHeight + 1, surfaceWindow, wid);
		PRectangle rcIG = PRectangle::FromInts(0, 0, 1, vsDraw.lineHeight);
		pixmapIndentGuide->FillRectangle(rcIG, vsDraw.styles[STYLE_INDENTGUIDE].back);
		pixmapIndentGuide->PenColour(vsDraw.styles[STYLE_INDENTGUIDE].fore);
		pixmapIndentGuideHighlight->FillRectangle(rcIG, vsDraw.styles[STYLE_BRACELIGHT].back);
		pixmapIndentGuideHighlight->PenColour(vsDraw.styles[STYLE_BRACELIGHT].fore);
		for (int stripe = 1; stripe < vsDraw.lineHeight + 1; stripe += 2) {
			PRectangle rcPixel = PRectangle::FromInts(0, stripe, 1, stripe + 1);
			pixmapIndentGuide->FillRectangle(rcPixel, vsDraw.styles[STYLE_INDENTGUIDE].fore);
			pixmapIndentGuideHighlight->FillRectangle(rcPixel, vsDraw.styles[STYLE_BRACELIGHT].fore);
		}
	}
}

LineLayout *EditView::RetrieveLineLayout(int lineNumber, const EditModel &model) {
	int posLineStart = model.pdoc->LineStart(lineNumber);
	int posLineEnd = model.pdoc->LineStart(lineNumber + 1);
	PLATFORM_ASSERT(posLineEnd >= posLineStart);
	int lineCaret = model.pdoc->LineFromPosition(model.sel.MainCaret());
	return llc.Retrieve(lineNumber, lineCaret,
		posLineEnd - posLineStart, model.pdoc->GetStyleClock(),
		model.LinesOnScreen() + 1, model.pdoc->LinesTotal());
}

/**
* Fill in the LineLayout data for the given line.
* Copy the given @a line and its styles from the document into local arrays.
* Also determine the x position at which each character starts.
*/
void EditView::LayoutLine(const EditModel &model, int line, Surface *surface, const ViewStyle &vstyle, LineLayout *ll, int width) {
	if (!ll)
		return;

	PLATFORM_ASSERT(line < model.pdoc->LinesTotal());
	PLATFORM_ASSERT(ll->chars != NULL);
	int posLineStart = model.pdoc->LineStart(line);
	int posLineEnd = model.pdoc->LineStart(line + 1);
	// If the line is very long, limit the treatment to a length that should fit in the viewport
	if (posLineEnd >(posLineStart + ll->maxLineLength)) {
		posLineEnd = posLineStart + ll->maxLineLength;
	}
	if (ll->validity == LineLayout::llCheckTextAndStyle) {
		int lineLength = posLineEnd - posLineStart;
		if (!vstyle.viewEOL) {
			lineLength = model.pdoc->LineEnd(line) - posLineStart;
		}
		if (lineLength == ll->numCharsInLine) {
			// See if chars, styles, indicators, are all the same
			bool allSame = true;
			// Check base line layout
			char styleByte = 0;
			int numCharsInLine = 0;
			while (numCharsInLine < lineLength) {
				int charInDoc = numCharsInLine + posLineStart;
				char chDoc = model.pdoc->CharAt(charInDoc);
				styleByte = model.pdoc->StyleAt(charInDoc);
				allSame = allSame &&
					(ll->styles[numCharsInLine] == static_cast<unsigned char>(styleByte));
				if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseMixed)
					allSame = allSame &&
					(ll->chars[numCharsInLine] == chDoc);
				else if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseLower)
					allSame = allSame &&
					(ll->chars[numCharsInLine] == static_cast<char>(tolower(chDoc)));
				else	// Style::caseUpper
					allSame = allSame &&
					(ll->chars[numCharsInLine] == static_cast<char>(toupper(chDoc)));
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
			ll->edgeColumn = model.pdoc->FindColumn(line, vstyle.theEdge);
			if (ll->edgeColumn >= posLineStart) {
				ll->edgeColumn -= posLineStart;
			}
		} else {
			ll->edgeColumn = -1;
		}

		// Fill base line layout
		const int lineLength = posLineEnd - posLineStart;
		model.pdoc->GetCharRange(ll->chars, posLineStart, lineLength);
		model.pdoc->GetStyleRange(ll->styles, posLineStart, lineLength);
		int numCharsBeforeEOL = model.pdoc->LineEnd(line) - posLineStart;
		const int numCharsInLine = (vstyle.viewEOL) ? lineLength : numCharsBeforeEOL;
		for (int styleInLine = 0; styleInLine < numCharsInLine; styleInLine++) {
			const unsigned char styleByte = ll->styles[styleInLine];
			ll->styles[styleInLine] = styleByte;
		}
		const unsigned char styleByteLast = (lineLength > 0) ? ll->styles[lineLength - 1] : 0;
		if (vstyle.someStylesForceCase) {
			for (int charInLine = 0; charInLine<lineLength; charInLine++) {
				char chDoc = ll->chars[charInLine];
				if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseUpper)
					ll->chars[charInLine] = static_cast<char>(toupper(chDoc));
				else if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseLower)
					ll->chars[charInLine] = static_cast<char>(tolower(chDoc));
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

		BreakFinder bfLayout(ll, NULL, Range(0, numCharsInLine), posLineStart, 0, false, model.pdoc, &model.reprs, NULL);
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
						posCache.MeasureWidths(surface, vstyle, ll->styles[ts.start], ll->chars + ts.start,
							ts.length, ll->positions + ts.start + 1, model.pdoc);
					}
				}
				lastSegItalics = (!ts.representation) && ((ll->chars[ts.end() - 1] != ' ') && vstyle.styles[ll->styles[ts.start]].italic);
			}

			for (int posToIncrease = ts.start + 1; posToIncrease <= ts.end(); posToIncrease++) {
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
			if (vstyle.wrapIndentMode == SC_WRAPINDENT_INDENT) {
				wrapAddIndent = model.pdoc->IndentSize() * vstyle.spaceWidth;
			} else if (vstyle.wrapIndentMode == SC_WRAPINDENT_FIXED) {
				wrapAddIndent = vstyle.wrapVisualStartIndent * vstyle.aveCharWidth;
			}
			ll->wrapIndent = wrapAddIndent;
			if (vstyle.wrapIndentMode != SC_WRAPINDENT_FIXED)
			for (int i = 0; i < ll->numCharsInLine; i++) {
				if (!IsSpaceOrTab(ll->chars[i])) {
					ll->wrapIndent += ll->positions[i]; // Add line indent
					break;
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
			int lastGoodBreak = 0;
			int lastLineStart = 0;
			XYACCUMULATOR startOffset = 0;
			int p = 0;
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
					ll->SetLineStart(ll->lines, lastGoodBreak);
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

Point EditView::LocationFromPosition(Surface *surface, const EditModel &model, SelectionPosition pos, int topLine, const ViewStyle &vs) {
	Point pt;
	if (pos.Position() == INVALID_POSITION)
		return pt;
	const int line = model.pdoc->LineFromPosition(pos.Position());
	const int lineVisible = model.cs.DisplayFromDoc(line);
	//Platform::DebugPrintf("line=%d\n", line);
	AutoLineLayout ll(llc, RetrieveLineLayout(line, model));
	if (surface && ll) {
		const int posLineStart = model.pdoc->LineStart(line);
		LayoutLine(model, line, surface, vs, ll, model.wrapWidth);
		const int posInLine = pos.Position() - posLineStart;
		pt = ll->PointFromPosition(posInLine, vs.lineHeight);
		pt.y += (lineVisible - topLine) * vs.lineHeight;
		pt.x += vs.textStart - model.xOffset;
	}
	pt.x += pos.VirtualSpace() * vs.styles[ll->EndLineStyle()].spaceWidth;
	return pt;
}

SelectionPosition EditView::SPositionFromLocation(Surface *surface, const EditModel &model, Point pt, bool canReturnInvalid, bool charPosition, bool virtualSpace, const ViewStyle &vs) {
	pt.x = pt.x - vs.textStart;
	int visibleLine = static_cast<int>(floor(pt.y / vs.lineHeight));
	if (!canReturnInvalid && (visibleLine < 0))
		visibleLine = 0;
	const int lineDoc = model.cs.DocFromDisplay(visibleLine);
	if (canReturnInvalid && (lineDoc < 0))
		return SelectionPosition(INVALID_POSITION);
	if (lineDoc >= model.pdoc->LinesTotal())
		return SelectionPosition(canReturnInvalid ? INVALID_POSITION : model.pdoc->Length());
	const int posLineStart = model.pdoc->LineStart(lineDoc);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
		const int lineStartSet = model.cs.DisplayFromDoc(lineDoc);
		const int subLine = visibleLine - lineStartSet;
		if (subLine < ll->lines) {
			const Range rangeSubLine = ll->SubLineRange(subLine);
			const XYPOSITION subLineStart = ll->positions[rangeSubLine.start];
			if (subLine > 0)	// Wrapped
				pt.x -= ll->wrapIndent;
			const int positionInLine = ll->FindPositionFromX(pt.x + subLineStart, rangeSubLine, charPosition);
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
SelectionPosition EditView::SPositionFromLineX(Surface *surface, const EditModel &model, int lineDoc, int x, const ViewStyle &vs) {
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		const int posLineStart = model.pdoc->LineStart(lineDoc);
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
		const Range rangeSubLine = ll->SubLineRange(0);
		const XYPOSITION subLineStart = ll->positions[rangeSubLine.start];
		const int positionInLine = ll->FindPositionFromX(x + subLineStart, rangeSubLine, false);
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

int EditView::DisplayFromPosition(Surface *surface, const EditModel &model, int pos, const ViewStyle &vs) {
	int lineDoc = model.pdoc->LineFromPosition(pos);
	int lineDisplay = model.cs.DisplayFromDoc(lineDoc);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc, model));
	if (surface && ll) {
		LayoutLine(model, lineDoc, surface, vs, ll, model.wrapWidth);
		unsigned int posLineStart = model.pdoc->LineStart(lineDoc);
		int posInLine = pos - posLineStart;
		lineDisplay--; // To make up for first increment ahead.
		for (int subLine = 0; subLine < ll->lines; subLine++) {
			if (posInLine >= ll->LineStart(subLine)) {
				lineDisplay++;
			}
		}
	}
	return lineDisplay;
}

int EditView::StartEndDisplayLine(Surface *surface, const EditModel &model, int pos, bool start, const ViewStyle &vs) {
	int line = model.pdoc->LineFromPosition(pos);
	AutoLineLayout ll(llc, RetrieveLineLayout(line, model));
	int posRet = INVALID_POSITION;
	if (surface && ll) {
		unsigned int posLineStart = model.pdoc->LineStart(line);
		LayoutLine(model, line, surface, vs, ll, model.wrapWidth);
		int posInLine = pos - posLineStart;
		if (posInLine <= ll->maxLineLength) {
			for (int subLine = 0; subLine < ll->lines; subLine++) {
				if ((posInLine >= ll->LineStart(subLine)) && (posInLine <= ll->LineStart(subLine + 1))) {
					if (start) {
						posRet = ll->LineStart(subLine) + posLineStart;
					} else {
						if (subLine == ll->lines - 1)
							posRet = ll->LineStart(subLine + 1) + posLineStart;
						else
							posRet = ll->LineStart(subLine + 1) + posLineStart - 1;
					}
				}
			}
		}
	}
	return posRet;
}

static ColourDesired SelectionBackground(const ViewStyle &vsDraw, bool main, bool primarySelection) {
	return main ?
		(primarySelection ? vsDraw.selColours.back : vsDraw.selBackground2) :
		vsDraw.selAdditionalBackground;
}

static ColourDesired TextBackground(const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	ColourOptional background, int inSelection, bool inHotspot, int styleMain, int i) {
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
			return vsDraw.edgecolour;
		if (inHotspot && vsDraw.hotspotColours.back.isSet)
			return vsDraw.hotspotColours.back;
	}
	if (background.isSet && (styleMain != STYLE_BRACELIGHT) && (styleMain != STYLE_BRACEBAD)) {
		return background;
	} else {
		return vsDraw.styles[styleMain].back;
	}
}

void EditView::DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight) {
	Point from = Point::FromInts(0, ((lineVisible & 1) && (lineHeight & 1)) ? 1 : 0);
	PRectangle rcCopyArea = PRectangle::FromInts(start + 1, static_cast<int>(rcSegment.top), start + 2, static_cast<int>(rcSegment.bottom));
	surface->Copy(rcCopyArea, from,
		highlight ? *pixmapIndentGuideHighlight : *pixmapIndentGuide);
}

static void SimpleAlphaRectangle(Surface *surface, PRectangle rc, ColourDesired fill, int alpha) {
	if (alpha != SC_ALPHA_NOALPHA) {
		surface->AlphaRectangle(rc, 0, fill, alpha, fill, alpha, 0);
	}
}

static void DrawTextBlob(Surface *surface, const ViewStyle &vsDraw, PRectangle rcSegment,
	const char *s, ColourDesired textBack, ColourDesired textFore, bool fillBackground) {
	if (fillBackground) {
		surface->FillRectangle(rcSegment, textBack);
	}
	FontAlias ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;
	int normalCharHeight = static_cast<int>(surface->Ascent(ctrlCharsFont) -
		surface->InternalLeading(ctrlCharsFont));
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
		rcSegment.top + vsDraw.maxAscent, s, static_cast<int>(s ? strlen(s) : 0),
		textBack, textFore);
}

void EditView::DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, int line, int lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
	ColourOptional background) {

	const int posLineStart = model.pdoc->LineStart(line);
	PRectangle rcSegment = rcLine;

	const bool lastSubLine = subLine == (ll->lines - 1);
	XYPOSITION virtualSpace = 0;
	if (lastSubLine) {
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		virtualSpace = model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line)) * spaceWidth;
	}
	XYPOSITION xEol = static_cast<XYPOSITION>(ll->positions[lineEnd] - subLineStart);

	// Fill the virtual space and show selections within it
	if (virtualSpace > 0.0f) {
		rcSegment.left = xEol + xStart;
		rcSegment.right = xEol + xStart + virtualSpace;
		surface->FillRectangle(rcSegment, background.isSet ? background : vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		if (!hideSelection && ((vsDraw.selAlpha == SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA))) {
			SelectionSegment virtualSpaceRange(SelectionPosition(model.pdoc->LineEnd(line)), SelectionPosition(model.pdoc->LineEnd(line), model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line))));
			for (size_t r = 0; r<model.sel.Count(); r++) {
				int alpha = (r == model.sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
				if (alpha == SC_ALPHA_NOALPHA) {
					SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
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
		int posAfterLineEnd = model.pdoc->LineStart(line + 1);
		eolInSelection = (subLine == (ll->lines - 1)) ? model.sel.InSelectionForEOL(posAfterLineEnd) : 0;
		alpha = (eolInSelection == 1) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
	}

	// Draw the [CR], [LF], or [CR][LF] blobs if visible line ends are on
	XYPOSITION blobsWidth = 0;
	if (lastSubLine) {
		for (int eolPos = ll->numCharsBeforeEOL; eolPos<ll->numCharsInLine; eolPos++) {
			rcSegment.left = xStart + ll->positions[eolPos] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
			rcSegment.right = xStart + ll->positions[eolPos + 1] - static_cast<XYPOSITION>(subLineStart)+virtualSpace;
			blobsWidth += rcSegment.Width();
			char hexits[4];
			const char *ctrlChar;
			unsigned char chEOL = ll->chars[eolPos];
			int styleMain = ll->styles[eolPos];
			ColourDesired textBack = TextBackground(model, vsDraw, ll, background, eolInSelection, false, styleMain, eolPos);
			if (UTF8IsAscii(chEOL)) {
				ctrlChar = ControlCharacterString(chEOL);
			} else {
				const Representation *repr = model.reprs.RepresentationFromCharacter(ll->chars + eolPos, ll->numCharsInLine - eolPos);
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

	// Fill the remainder of the line
	rcSegment.left = rcSegment.right;
	if (rcSegment.left < rcLine.left)
		rcSegment.left = rcLine.left;
	rcSegment.right = rcLine.right;

	if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection));
	} else {
		if (background.isSet) {
			surface->FillRectangle(rcSegment, background);
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine]].eolFilled) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine]].back);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back);
		}
		if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selColours.back.isSet && (line < model.pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1, model.primarySelection), alpha);
		}
	}

	bool drawWrapMarkEnd = false;

	if (vsDraw.wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
		if (subLine + 1 < ll->lines) {
			drawWrapMarkEnd = ll->LineStart(subLine + 1) != 0;
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
		if (customDrawWrapMarker == NULL) {
			DrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		} else {
			customDrawWrapMarker(surface, rcPlace, true, vsDraw.WrapColour());
		}
	}
}

static void DrawIndicator(int indicNum, int startPos, int endPos, Surface *surface, const ViewStyle &vsDraw,
	const LineLayout *ll, int xStart, PRectangle rcLine, int subLine, Indicator::DrawState drawState, int value) {
	const XYPOSITION subLineStart = ll->positions[ll->LineStart(subLine)];
	PRectangle rcIndic(
		ll->positions[startPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent,
		ll->positions[endPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent + 3);
	vsDraw.indicators[indicNum].Draw(surface, rcIndic, rcLine, drawState, value);
}

static void DrawIndicators(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int line, int xStart, PRectangle rcLine, int subLine, int lineEnd, bool under, int hoverIndicatorPos) {
	// Draw decorators
	const int posLineStart = model.pdoc->LineStart(line);
	const int lineStart = ll->LineStart(subLine);
	const int posLineEnd = posLineStart + lineEnd;

	for (Decoration *deco = model.pdoc->decorations.root; deco; deco = deco->next) {
		if (under == vsDraw.indicators[deco->indicator].under) {
			int startPos = posLineStart + lineStart;
			if (!deco->rs.ValueAt(startPos)) {
				startPos = deco->rs.EndRun(startPos);
			}
			while ((startPos < posLineEnd) && (deco->rs.ValueAt(startPos))) {
				int endPos = deco->rs.EndRun(startPos);
				if (endPos > posLineEnd)
					endPos = posLineEnd;
				const bool hover = vsDraw.indicators[deco->indicator].IsDynamic() &&
					((hoverIndicatorPos >= startPos) && (hoverIndicatorPos <= endPos));
				const int value = deco->rs.ValueAt(startPos);
				Indicator::DrawState drawState = hover ? Indicator::drawHover : Indicator::drawNormal;
				DrawIndicator(deco->indicator, startPos - posLineStart, endPos - posLineStart,
					surface, vsDraw, ll, xStart, rcLine, subLine, drawState, value);
				startPos = endPos;
				if (!deco->rs.ValueAt(startPos)) {
					startPos = deco->rs.EndRun(startPos);
				}
			}
		}
	}

	// Use indicators to highlight matching braces
	if ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACELIGHT)) ||
		(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACEBAD))) {
		int braceIndicator = (model.bracesMatchStyle == STYLE_BRACELIGHT) ? vsDraw.braceHighlightIndicator : vsDraw.braceBadLightIndicator;
		if (under == vsDraw.indicators[braceIndicator].under) {
			Range rangeLine(posLineStart + lineStart, posLineEnd);
			if (rangeLine.ContainsCharacter(model.braces[0])) {
				int braceOffset = model.braces[0] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, subLine, Indicator::drawNormal, 1);
				}
			}
			if (rangeLine.ContainsCharacter(model.braces[1])) {
				int braceOffset = model.braces[1] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, ll, xStart, rcLine, subLine, Indicator::drawNormal, 1);
				}
			}
		}
	}
}

static bool AnnotationBoxedOrIndented(int annotationVisible) {
	return annotationVisible == ANNOTATION_BOXED || annotationVisible == ANNOTATION_INDENTED;
}

void EditView::DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase) {
	int indent = static_cast<int>(model.pdoc->GetLineIndentation(line) * vsDraw.spaceWidth);
	PRectangle rcSegment = rcLine;
	int annotationLine = subLine - ll->lines;
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
			surface->MoveTo(static_cast<int>(rcSegment.left), static_cast<int>(rcSegment.top));
			surface->LineTo(static_cast<int>(rcSegment.left), static_cast<int>(rcSegment.bottom));
			surface->MoveTo(static_cast<int>(rcSegment.right), static_cast<int>(rcSegment.top));
			surface->LineTo(static_cast<int>(rcSegment.right), static_cast<int>(rcSegment.bottom));
			if (subLine == ll->lines) {
				surface->MoveTo(static_cast<int>(rcSegment.left), static_cast<int>(rcSegment.top));
				surface->LineTo(static_cast<int>(rcSegment.right), static_cast<int>(rcSegment.top));
			}
			if (subLine == ll->lines + annotationLines - 1) {
				surface->MoveTo(static_cast<int>(rcSegment.left), static_cast<int>(rcSegment.bottom - 1));
				surface->LineTo(static_cast<int>(rcSegment.right), static_cast<int>(rcSegment.bottom - 1));
			}
		}
	}
}

static void DrawBlockCaret(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int subLine, int xStart, int offset, int posCaret, PRectangle rcCaret, ColourDesired caretColour) {

	int lineStart = ll->LineStart(subLine);
	int posBefore = posCaret;
	int posAfter = model.pdoc->MovePositionOutsideChar(posCaret + 1, 1);
	int numCharsToDraw = posAfter - posCaret;

	// Work out where the starting and ending offsets are. We need to
	// see if the previous character shares horizontal space, such as a
	// glyph / combining character. If so we'll need to draw that too.
	int offsetFirstChar = offset;
	int offsetLastChar = offset + (posAfter - posCaret);
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
		XYPOSITION wordWrapCharWidth = ll->wrapIndent;
		rcCaret.left += wordWrapCharWidth;
		rcCaret.right += wordWrapCharWidth;
	}

	// This character is where the caret block is, we override the colours
	// (inversed) for drawing the caret here.
	int styleMain = ll->styles[offsetFirstChar];
	FontAlias fontText = vsDraw.styles[styleMain].font;
	surface->DrawTextClipped(rcCaret, fontText,
		rcCaret.top + vsDraw.maxAscent, ll->chars + offsetFirstChar,
		numCharsToDraw, vsDraw.styles[styleMain].back,
		caretColour);
}

void EditView::DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int lineDoc, int xStart, PRectangle rcLine, int subLine) const {
	// When drag is active it is the only caret drawn
	bool drawDrag = model.posDrag.IsValid();
	if (hideSelection && !drawDrag)
		return;
	const int posLineStart = model.pdoc->LineStart(lineDoc);
	// For each selection draw
	for (size_t r = 0; (r<model.sel.Count()) || drawDrag; r++) {
		const bool mainCaret = r == model.sel.Main();
		const SelectionPosition posCaret = (drawDrag ? model.posDrag : model.sel.Range(r).caret);
		const int offset = posCaret.Position() - posLineStart;
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		const XYPOSITION virtualOffset = posCaret.VirtualSpace() * spaceWidth;
		if (ll->InLine(offset, subLine) && offset <= ll->numCharsBeforeEOL) {
			XYPOSITION xposCaret = ll->positions[offset] + virtualOffset - ll->positions[ll->LineStart(subLine)];
			if (ll->wrapIndent != 0) {
				int lineStart = ll->LineStart(subLine);
				if (lineStart != 0)	// Wrapped
					xposCaret += ll->wrapIndent;
			}
			bool caretBlinkState = (model.caret.active && model.caret.on) || (!additionalCaretsBlink && !mainCaret);
			bool caretVisibleState = additionalCaretsVisible || mainCaret;
			if ((xposCaret >= 0) && (vsDraw.caretWidth > 0) && (vsDraw.caretStyle != CARETSTYLE_INVISIBLE) &&
				((model.posDrag.IsValid()) || (caretBlinkState && caretVisibleState))) {
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
				if (model.posDrag.IsValid()) {
					/* Dragging text, use a line caret */
					rcCaret.left = static_cast<XYPOSITION>(RoundXYPosition(xposCaret - caretWidthOffset));
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				} else if (model.inOverstrike && drawOverstrikeCaret) {
					/* Overstrike (insert mode), use a modified bar caret */
					rcCaret.top = rcCaret.bottom - 2;
					rcCaret.left = xposCaret + 1;
					rcCaret.right = rcCaret.left + widthOverstrikeCaret - 1;
				} else if ((vsDraw.caretStyle == CARETSTYLE_BLOCK) || imeCaretBlockOverride) {
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
					rcCaret.left = static_cast<XYPOSITION>(RoundXYPosition(xposCaret - caretWidthOffset));
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				}
				ColourDesired caretColour = mainCaret ? vsDraw.caretcolour : vsDraw.additionalCaretColour;
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
	int xStart, PRectangle rcLine, ColourOptional background, DrawWrapMarkerFn customDrawWrapMarker) {
	// default bgnd here..
	surface->FillRectangle(rcLine, background.isSet ? background :
		vsDraw.styles[STYLE_DEFAULT].back);

	if (vsDraw.wrapVisualFlags & SC_WRAPVISUALFLAG_START) {

		// draw continuation rect
		PRectangle rcPlace = rcLine;

		rcPlace.left = static_cast<XYPOSITION>(xStart);
		rcPlace.right = rcPlace.left + ll->wrapIndent;

		if (vsDraw.wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_START_BY_TEXT)
			rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
		else
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;

		if (customDrawWrapMarker == NULL) {
			DrawWrapMarker(surface, rcPlace, false, vsDraw.WrapColour());
		} else {
			customDrawWrapMarker(surface, rcPlace, false, vsDraw.WrapColour());
		}
	}
}

void EditView::DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	PRectangle rcLine, Range lineRange, int posLineStart, int xStart,
	int subLine, ColourOptional background) const {

	const bool selBackDrawn = vsDraw.SelectionBackgroundDrawn();
	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
	// Does not take margin into account but not significant
	const int xStartVisible = static_cast<int>(subLineStart)-xStart;

	BreakFinder bfBack(ll, &model.sel, lineRange, posLineStart, xStartVisible, selBackDrawn, model.pdoc, &model.reprs, NULL);

	const bool drawWhitespaceBackground = vsDraw.WhitespaceBackgroundDrawn() && !background.isSet;

	// Background drawing loop
	while (bfBack.More()) {

		const TextSegment ts = bfBack.Next();
		const int i = ts.end() - 1;
		const int iDoc = i + posLineStart;

		PRectangle rcSegment = rcLine;
		rcSegment.left = ll->positions[ts.start] + xStart - static_cast<XYPOSITION>(subLineStart);
		rcSegment.right = ll->positions[ts.end()] + xStart - static_cast<XYPOSITION>(subLineStart);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (rcSegment.Intersects(rcLine)) {
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
					if (drawWhitespaceBackground &&
						(!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
						textBack = vsDraw.whitespaceColours.back;
				} else {
					// Blob display
					inIndentation = false;
				}
				surface->FillRectangle(rcSegment, textBack);
			} else {
				// Normal text display
				surface->FillRectangle(rcSegment, textBack);
				if (vsDraw.viewWhitespace != wsInvisible ||
					(inIndentation && vsDraw.viewIndentationGuides == ivReal)) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (drawWhitespaceBackground &&
								(!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
								PRectangle rcSpace(
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
		int edgeX = static_cast<int>(vsDraw.theEdge * vsDraw.spaceWidth);
		rcSegment.left = static_cast<XYPOSITION>(edgeX + xStart);
		if ((ll->wrapIndent != 0) && (lineRange.start != 0))
			rcSegment.left -= ll->wrapIndent;
		rcSegment.right = rcSegment.left + 1;
		surface->FillRectangle(rcSegment, vsDraw.edgecolour);
	}
}

// Draw underline mark as part of background if not transparent
static void DrawMarkUnderline(Surface *surface, const EditModel &model, const ViewStyle &vsDraw,
	int line, PRectangle rcLine) {
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
	int line, PRectangle rcLine, int subLine, Range lineRange, int xStart) {
	if ((vsDraw.selAlpha != SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha != SC_ALPHA_NOALPHA)) {
		const int posLineStart = model.pdoc->LineStart(line);
		const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];
		// For each selection draw
		int virtualSpaces = 0;
		if (subLine == (ll->lines - 1)) {
			virtualSpaces = model.sel.VirtualSpaceFor(model.pdoc->LineEnd(line));
		}
		SelectionPosition posStart(posLineStart + lineRange.start);
		SelectionPosition posEnd(posLineStart + lineRange.end, virtualSpaces);
		SelectionSegment virtualSpaceRange(posStart, posEnd);
		for (size_t r = 0; r < model.sel.Count(); r++) {
			int alpha = (r == model.sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
			if (alpha != SC_ALPHA_NOALPHA) {
				SelectionSegment portion = model.sel.Range(r).Intersect(virtualSpaceRange);
				if (!portion.Empty()) {
					const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
					PRectangle rcSegment = rcLine;
					rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart)+portion.start.VirtualSpace() * spaceWidth;
					rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] -
						static_cast<XYPOSITION>(subLineStart)+portion.end.VirtualSpace() * spaceWidth;
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

// Draw any translucent whole line states
static void DrawTranslucentLineState(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int line, PRectangle rcLine) {
	if ((model.caret.active || vsDraw.alwaysShowCaretLineBackground) && vsDraw.showCaretLineBackground && ll->containsCaret) {
		SimpleAlphaRectangle(surface, rcLine, vsDraw.caretLineBackground, vsDraw.caretLineAlpha);
	}
	int marks = model.pdoc->GetMark(line);
	for (int markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND)) {
			SimpleAlphaRectangle(surface, rcLine, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
		} else if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_UNDERLINE)) {
			PRectangle rcUnderline = rcLine;
			rcUnderline.top = rcUnderline.bottom - 2;
			SimpleAlphaRectangle(surface, rcUnderline, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
		}
		marks >>= 1;
	}
	if (vsDraw.maskInLine) {
		int marksMasked = model.pdoc->GetMark(line) & vsDraw.maskInLine;
		if (marksMasked) {
			for (int markBit = 0; (markBit < 32) && marksMasked; markBit++) {
				if ((marksMasked & 1) && (vsDraw.markers[markBit].markType != SC_MARK_EMPTY)) {
					SimpleAlphaRectangle(surface, rcLine, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
				}
				marksMasked >>= 1;
			}
		}
	}
}

void EditView::DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int lineVisible, PRectangle rcLine, Range lineRange, int posLineStart, int xStart,
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
		const int i = ts.end() - 1;
		const int iDoc = i + posLineStart;

		PRectangle rcSegment = rcLine;
		rcSegment.left = ll->positions[ts.start] + xStart - static_cast<XYPOSITION>(subLineStart);
		rcSegment.right = ll->positions[ts.end()] + xStart - static_cast<XYPOSITION>(subLineStart);
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if (rcSegment.Intersects(rcLine)) {
			int styleMain = ll->styles[i];
			ColourDesired textFore = vsDraw.styles[styleMain].fore;
			FontAlias textFont = vsDraw.styles[styleMain].font;
			//hotspot foreground
			const bool inHotspot = (ll->hotspot.Valid()) && ll->hotspot.ContainsCharacter(iDoc);
			if (inHotspot) {
				if (vsDraw.hotspotColours.fore.isSet)
					textFore = vsDraw.hotspotColours.fore;
			}
			if (vsDraw.indicatorsSetFore > 0) {
				// At least one indicator sets the text colour so see if it applies to this segment
				for (Decoration *deco = model.pdoc->decorations.root; deco; deco = deco->next) {
					const int indicatorValue = deco->rs.ValueAt(ts.start + posLineStart);
					if (indicatorValue) {
						const Indicator &indicator = vsDraw.indicators[deco->indicator];
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
									textFore = indicatorValue & SC_INDICVALUEMASK;
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
						if (drawWhitespaceBackground &&
							(!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
							textBack = vsDraw.whitespaceColours.back;
						surface->FillRectangle(rcSegment, textBack);
					}
					if (inIndentation && vsDraw.viewIndentationGuides == ivReal) {
						for (int indentCount = static_cast<int>((ll->positions[i] + epsilon) / indentWidth);
							indentCount <= (ll->positions[i + 1] - epsilon) / indentWidth;
							indentCount++) {
							if (indentCount > 0) {
								int xIndent = static_cast<int>(indentCount * indentWidth);
								DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
									(ll->xHighlightGuide == xIndent));
							}
						}
					}
					if (vsDraw.viewWhitespace != wsInvisible) {
						if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
							if (vsDraw.whitespaceColours.fore.isSet)
								textFore = vsDraw.whitespaceColours.fore;
							surface->PenColour(textFore);
							PRectangle rcTab(rcSegment.left + 1, rcSegment.top + tabArrowHeight,
								rcSegment.right - 1, rcSegment.bottom - vsDraw.maxDescent);
							if (customDrawTabArrow == NULL)
								DrawTabArrow(surface, rcTab, static_cast<int>(rcSegment.top + vsDraw.lineHeight / 2));
							else
								customDrawTabArrow(surface, rcTab, static_cast<int>(rcSegment.top + vsDraw.lineHeight / 2));
						}
					}
				} else {
					inIndentation = false;
					if (vsDraw.controlCharSymbol >= 32) {
						// Using one font for all control characters so it can be controlled independently to ensure
						// the box goes around the characters tightly. Seems to be no way to work out what height
						// is taken by an individual character - internal leading gives varying results.
						FontAlias ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;
						char cc[2] = { static_cast<char>(vsDraw.controlCharSymbol), '\0' };
						surface->DrawTextNoClip(rcSegment, ctrlCharsFont,
							rcSegment.top + vsDraw.maxAscent,
							cc, 1, textBack, textFore);
					} else {
						DrawTextBlob(surface, vsDraw, rcSegment, ts.representation->stringRep.c_str(),
							textBack, textFore, phasesDraw == phasesOne);
					}
				}
			} else {
				// Normal text display
				if (vsDraw.styles[styleMain].visible) {
					if (phasesDraw != phasesOne) {
						surface->DrawTextTransparent(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, ll->chars + ts.start,
							i - ts.start + 1, textFore);
					} else {
						surface->DrawTextNoClip(rcSegment, textFont,
							rcSegment.top + vsDraw.maxAscent, ll->chars + ts.start,
							i - ts.start + 1, textFore, textBack);
					}
				}
				if (vsDraw.viewWhitespace != wsInvisible ||
					(inIndentation && vsDraw.viewIndentationGuides != ivNone)) {
					for (int cpos = 0; cpos <= i - ts.start; cpos++) {
						if (ll->chars[cpos + ts.start] == ' ') {
							if (vsDraw.viewWhitespace != wsInvisible) {
								if (vsDraw.whitespaceColours.fore.isSet)
									textFore = vsDraw.whitespaceColours.fore;
								if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
									XYPOSITION xmid = (ll->positions[cpos + ts.start] + ll->positions[cpos + ts.start + 1]) / 2;
									if ((phasesDraw == phasesOne) && drawWhitespaceBackground &&
										(!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
										textBack = vsDraw.whitespaceColours.back;
										PRectangle rcSpace(
											ll->positions[cpos + ts.start] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.top,
											ll->positions[cpos + ts.start + 1] + xStart - static_cast<XYPOSITION>(subLineStart),
											rcSegment.bottom);
										surface->FillRectangle(rcSpace, textBack);
									}
									PRectangle rcDot(xmid + xStart - static_cast<XYPOSITION>(subLineStart),
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
										int xIndent = static_cast<int>(indentCount * indentWidth);
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
	int line, int lineVisible, PRectangle rcLine, int xStart, int subLine) {
	if ((vsDraw.viewIndentationGuides == ivLookForward || vsDraw.viewIndentationGuides == ivLookBoth)
		&& (subLine == 0)) {
		const int posLineStart = model.pdoc->LineStart(line);
		int indentSpace = model.pdoc->GetLineIndentation(line);
		int xStartText = static_cast<int>(ll->positions[model.pdoc->GetLineIndentPosition(line) - posLineStart]);

		// Find the most recent line with some text

		int lineLastWithText = line;
		while (lineLastWithText > Platform::Maximum(line - 20, 0) && model.pdoc->IsWhiteLine(lineLastWithText)) {
			lineLastWithText--;
		}
		if (lineLastWithText < line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of last line with text
			int indentLastWithText = model.pdoc->GetLineIndentation(lineLastWithText);
			int isFoldHeader = model.pdoc->GetLevel(lineLastWithText) & SC_FOLDLEVELHEADERFLAG;
			if (isFoldHeader) {
				// Level is one more level than parent
				indentLastWithText += model.pdoc->IndentSize();
			}
			if (vsDraw.viewIndentationGuides == ivLookForward) {
				// In viLookForward mode, previous line only used if it is a fold header
				if (isFoldHeader) {
					indentSpace = Platform::Maximum(indentSpace, indentLastWithText);
				}
			} else {	// viLookBoth
				indentSpace = Platform::Maximum(indentSpace, indentLastWithText);
			}
		}

		int lineNextWithText = line;
		while (lineNextWithText < Platform::Minimum(line + 20, model.pdoc->LinesTotal()) && model.pdoc->IsWhiteLine(lineNextWithText)) {
			lineNextWithText++;
		}
		if (lineNextWithText > line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of first next line with text
			indentSpace = Platform::Maximum(indentSpace,
				model.pdoc->GetLineIndentation(lineNextWithText));
		}

		for (int indentPos = model.pdoc->IndentSize(); indentPos < indentSpace; indentPos += model.pdoc->IndentSize()) {
			int xIndent = static_cast<int>(indentPos * vsDraw.spaceWidth);
			if (xIndent < xStartText) {
				DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcLine,
					(ll->xHighlightGuide == xIndent));
			}
		}
	}
}

void EditView::DrawLine(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
	int line, int lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase) {

	if (subLine >= ll->lines) {
		DrawAnnotation(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, phase);
		return; // No further drawing
	}

	// See if something overrides the line background color.
	const ColourOptional background = vsDraw.Background(model.pdoc->GetMark(line), model.caret.active, ll->containsCaret);

	const int posLineStart = model.pdoc->LineStart(line);

	const Range lineRange = ll->SubLineRange(subLine);
	const XYACCUMULATOR subLineStart = ll->positions[lineRange.start];

	if ((ll->wrapIndent != 0) && (subLine > 0)) {
		if (phase & drawBack) {
			DrawWrapIndentAndMarker(surface, vsDraw, ll, xStart, rcLine, background, customDrawWrapMarker);
		}
		xStart += static_cast<int>(ll->wrapIndent);
	}

	if ((phasesDraw != phasesOne) && (phase & drawBack)) {
		DrawBackground(surface, model, vsDraw, ll, rcLine, lineRange, posLineStart, xStart,
			subLine, background);
		DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
			xStart, subLine, subLineStart, background);
	}

	if (phase & drawIndicatorsBack) {
		DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, lineRange.end, true, model.hoverIndicatorPos);
		DrawEdgeLine(surface, vsDraw, ll, rcLine, lineRange, xStart);
		DrawMarkUnderline(surface, model, vsDraw, line, rcLine);
	}

	if (phase & drawText) {
		DrawForeground(surface, model, vsDraw, ll, lineVisible, rcLine, lineRange, posLineStart, xStart,
			subLine, background);
	}

	if (phase & drawIndentationGuides) {
		DrawIndentGuidesOverEmpty(surface, model, vsDraw, ll, line, lineVisible, rcLine, xStart, subLine);
	}

	if (phase & drawIndicatorsFore) {
		DrawIndicators(surface, model, vsDraw, ll, line, xStart, rcLine, subLine, lineRange.end, false, model.hoverIndicatorPos);
	}

	// End of the drawing of the current line
	if (phasesDraw == phasesOne) {
		DrawEOL(surface, model, vsDraw, ll, rcLine, line, lineRange.end,
			xStart, subLine, subLineStart, background);
	}

	if (!hideSelection && (phase & drawSelectionTranslucent)) {
		DrawTranslucentSelection(surface, model, vsDraw, ll, line, rcLine, subLine, lineRange, xStart);
	}

	if (phase & drawLineTranslucent) {
		DrawTranslucentLineState(surface, model, vsDraw, ll, line, rcLine);
	}
}

static void DrawFoldLines(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, int line, PRectangle rcLine) {
	bool expanded = model.cs.GetExpanded(line);
	const int level = model.pdoc->GetLevel(line);
	const int levelNext = model.pdoc->GetLevel(line + 1);
	if ((level & SC_FOLDLEVELHEADERFLAG) &&
		((level & SC_FOLDLEVELNUMBERMASK) < (levelNext & SC_FOLDLEVELNUMBERMASK))) {
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
			surface = pixmapLine;
			PLATFORM_ASSERT(pixmapLine->Initialised());
		}
		surface->SetUnicodeMode(SC_CP_UTF8 == model.pdoc->dbcsCodePage);
		surface->SetDBCSMode(model.pdoc->dbcsCodePage);

		const Point ptOrigin = model.GetVisibleOriginInMain();

		const int screenLinePaintFirst = static_cast<int>(rcArea.top) / vsDraw.lineHeight;
		const int xStart = vsDraw.textStart - model.xOffset + static_cast<int>(ptOrigin.x);

		SelectionPosition posCaret = model.sel.RangeMain().caret;
		if (model.posDrag.IsValid())
			posCaret = model.posDrag;
		const int lineCaret = model.pdoc->LineFromPosition(posCaret.Position());

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
		//double durLayout = 0.0;
		//double durPaint = 0.0;
		//double durCopy = 0.0;
		//ElapsedTime etWhole;

		const bool bracesIgnoreStyle = ((vsDraw.braceHighlightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACELIGHT)) ||
			(vsDraw.braceBadLightIndicatorSet && (model.bracesMatchStyle == STYLE_BRACEBAD)));

		int lineDocPrevious = -1;	// Used to avoid laying out one document line multiple times
		AutoLineLayout ll(llc, 0);
		std::vector<DrawPhase> phases;
		if ((phasesDraw == phasesMultiple) && !bufferedDraw) {
			for (DrawPhase phase = drawBack; phase <= drawCarets; phase = static_cast<DrawPhase>(phase * 2)) {
				phases.push_back(phase);
			}
		} else {
			phases.push_back(drawAll);
		}
		for (std::vector<DrawPhase>::iterator it = phases.begin(); it != phases.end(); ++it) {
			int ypos = 0;
			if (!bufferedDraw)
				ypos += screenLinePaintFirst * vsDraw.lineHeight;
			int yposScreen = screenLinePaintFirst * vsDraw.lineHeight;
			int visibleLine = model.TopLineOfMain() + screenLinePaintFirst;
			while (visibleLine < model.cs.LinesDisplayed() && yposScreen < rcArea.bottom) {

				const int lineDoc = model.cs.DocFromDisplay(visibleLine);
				// Only visible lines should be handled by the code within the loop
				PLATFORM_ASSERT(model.cs.GetVisible(lineDoc));
				const int lineStartSet = model.cs.DisplayFromDoc(lineDoc);
				const int subLine = visibleLine - lineStartSet;

				// Copy this line and its styles from the document into local arrays
				// and determine the x position at which each character starts.
				//ElapsedTime et;
				if (lineDoc != lineDocPrevious) {
					ll.Set(0);
					ll.Set(RetrieveLineLayout(lineDoc, model));
					LayoutLine(model, lineDoc, surface, vsDraw, ll, model.wrapWidth);
					lineDocPrevious = lineDoc;
				}
				//durLayout += et.Duration(true);

				if (ll) {
					ll->containsCaret = !hideSelection && (lineDoc == lineCaret);
					ll->hotspot = model.GetHotSpotRange();

					PRectangle rcLine = rcTextArea;
					rcLine.top = static_cast<XYPOSITION>(ypos);
					rcLine.bottom = static_cast<XYPOSITION>(ypos + vsDraw.lineHeight);

					Range rangeLine(model.pdoc->LineStart(lineDoc), model.pdoc->LineStart(lineDoc + 1));

					// Highlight the current braces if any
					ll->SetBracesHighlight(rangeLine, model.braces, static_cast<char>(model.bracesMatchStyle),
						static_cast<int>(model.highlightGuideColumn * vsDraw.spaceWidth), bracesIgnoreStyle);

					if (leftTextOverlap && bufferedDraw) {
						PRectangle rcSpacer = rcLine;
						rcSpacer.right = rcSpacer.left;
						rcSpacer.left -= 1;
						surface->FillRectangle(rcSpacer, vsDraw.styles[STYLE_DEFAULT].back);
					}

					DrawLine(surface, model, vsDraw, ll, lineDoc, visibleLine, xStart, rcLine, subLine, *it);
					//durPaint += et.Duration(true);

					// Restore the previous styles for the brace highlights in case layout is in cache.
					ll->RestoreBracesHighlight(rangeLine, model.braces, bracesIgnoreStyle);

					if (*it & drawFoldLines) {
						DrawFoldLines(surface, model, vsDraw, lineDoc, rcLine);
					}

					if (*it & drawCarets) {
						DrawCarets(surface, model, vsDraw, ll, lineDoc, xStart, rcLine, subLine);
					}

					if (bufferedDraw) {
						Point from = Point::FromInts(vsDraw.textStart - leftTextOverlap, 0);
						PRectangle rcCopyArea = PRectangle::FromInts(vsDraw.textStart - leftTextOverlap, yposScreen,
							static_cast<int>(rcClient.right - vsDraw.rightMarginWidth),
							yposScreen + vsDraw.lineHeight);
						surfaceWindow->Copy(rcCopyArea, from, *pixmapLine);
					}

					lineWidthMaxSeen = Platform::Maximum(
						lineWidthMaxSeen, static_cast<int>(ll->positions[ll->numCharsInLine]));
					//durCopy += et.Duration(true);
				}

				if (!bufferedDraw) {
					ypos += vsDraw.lineHeight;
				}

				yposScreen += vsDraw.lineHeight;
				visibleLine++;
			}
		}
		ll.Set(0);
		//if (durPaint < 0.00000001)
		//	durPaint = 0.00000001;

		// Right column limit indicator
		PRectangle rcBeyondEOF = (vsDraw.marginInside) ? rcClient : rcArea;
		rcBeyondEOF.left = static_cast<XYPOSITION>(vsDraw.textStart);
		rcBeyondEOF.right = rcBeyondEOF.right - ((vsDraw.marginInside) ? vsDraw.rightMarginWidth : 0);
		rcBeyondEOF.top = static_cast<XYPOSITION>((model.cs.LinesDisplayed() - model.TopLineOfMain()) * vsDraw.lineHeight);
		if (rcBeyondEOF.top < rcBeyondEOF.bottom) {
			surfaceWindow->FillRectangle(rcBeyondEOF, vsDraw.styles[STYLE_DEFAULT].back);
			if (vsDraw.edgeState == EDGE_LINE) {
				int edgeX = static_cast<int>(vsDraw.theEdge * vsDraw.spaceWidth);
				rcBeyondEOF.left = static_cast<XYPOSITION>(edgeX + xStart);
				rcBeyondEOF.right = rcBeyondEOF.left + 1;
				surfaceWindow->FillRectangle(rcBeyondEOF, vsDraw.edgecolour);
			}
		}
		//Platform::DebugPrintf("start display %d, offset = %d\n", pdoc->Length(), xOffset);

		//Platform::DebugPrintf(
		//"Layout:%9.6g    Paint:%9.6g    Ratio:%9.6g   Copy:%9.6g   Total:%9.6g\n",
		//durLayout, durPaint, durLayout / durPaint, durCopy, etWhole.Duration());
	}
}

// Space (3 space characters) between line numbers and text when printing.
#define lineNumberPrintSpace "   "

ColourDesired InvertedLight(ColourDesired orig) {
	unsigned int r = orig.GetRed();
	unsigned int g = orig.GetGreen();
	unsigned int b = orig.GetBlue();
	unsigned int l = (r + g + b) / 3; 	// There is a better calculation for this that matches human eye
	unsigned int il = 0xff - l;
	if (l == 0)
		return ColourDesired(0xff, 0xff, 0xff);
	r = r * il / l;
	g = g * il / l;
	b = b * il / l;
	return ColourDesired(Platform::Minimum(r, 0xff), Platform::Minimum(g, 0xff), Platform::Minimum(b, 0xff));
}

long EditView::FormatRange(bool draw, Sci_RangeToFormat *pfr, Surface *surface, Surface *surfaceMeasure,
	const EditModel &model, const ViewStyle &vs) {
	// Can't use measurements cached for screen
	posCache.Clear();

	ViewStyle vsPrint(vs);
	vsPrint.technology = SC_TECHNOLOGY_DEFAULT;

	// Modify the view style for printing as do not normally want any of the transient features to be printed
	// Printing supports only the line number margin.
	int lineNumberIndex = -1;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((vsPrint.ms[margin].style == SC_MARGIN_NUMBER) && (vsPrint.ms[margin].width > 0)) {
			lineNumberIndex = margin;
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
	// White background for the line numbers
	vsPrint.styles[STYLE_LINENUMBER].back = ColourDesired(0xff, 0xff, 0xff);

	// Printing uses different margins, so reset screen margins
	vsPrint.leftMarginWidth = 0;
	vsPrint.rightMarginWidth = 0;

	vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);
	// Determining width must happen after fonts have been realised in Refresh
	int lineNumberWidth = 0;
	if (lineNumberIndex >= 0) {
		lineNumberWidth = static_cast<int>(surfaceMeasure->WidthText(vsPrint.styles[STYLE_LINENUMBER].font,
			"99999" lineNumberPrintSpace, 5 + static_cast<int>(strlen(lineNumberPrintSpace))));
		vsPrint.ms[lineNumberIndex].width = lineNumberWidth;
		vsPrint.Refresh(*surfaceMeasure, model.pdoc->tabInChars);	// Recalculate fixedColumnWidth
	}

	int linePrintStart = model.pdoc->LineFromPosition(static_cast<int>(pfr->chrg.cpMin));
	int linePrintLast = linePrintStart + (pfr->rc.bottom - pfr->rc.top) / vsPrint.lineHeight - 1;
	if (linePrintLast < linePrintStart)
		linePrintLast = linePrintStart;
	int linePrintMax = model.pdoc->LineFromPosition(static_cast<int>(pfr->chrg.cpMax));
	if (linePrintLast > linePrintMax)
		linePrintLast = linePrintMax;
	//Platform::DebugPrintf("Formatting lines=[%0d,%0d,%0d] top=%0d bottom=%0d line=%0d %0d\n",
	//      linePrintStart, linePrintLast, linePrintMax, pfr->rc.top, pfr->rc.bottom, vsPrint.lineHeight,
	//      surfaceMeasure->Height(vsPrint.styles[STYLE_LINENUMBER].font));
	int endPosPrint = model.pdoc->Length();
	if (linePrintLast < model.pdoc->LinesTotal())
		endPosPrint = model.pdoc->LineStart(linePrintLast + 1);

	// Ensure we are styled to where we are formatting.
	model.pdoc->EnsureStyledTo(endPosPrint);

	int xStart = vsPrint.fixedColumnWidth + pfr->rc.left;
	int ypos = pfr->rc.top;

	int lineDoc = linePrintStart;

	int nPrintPos = static_cast<int>(pfr->chrg.cpMin);
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
		LineLayout ll(model.pdoc->LineStart(lineDoc + 1) - model.pdoc->LineStart(lineDoc) + 1);
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
			int startWithinLine = nPrintPos - model.pdoc->LineStart(lineDoc);
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
			char number[100];
			sprintf(number, "%d" lineNumberPrintSpace, lineDoc + 1);
			PRectangle rcNumber = rcLine;
			rcNumber.right = rcNumber.left + lineNumberWidth;
			// Right justify
			rcNumber.left = rcNumber.right - surfaceMeasure->WidthText(
				vsPrint.styles[STYLE_LINENUMBER].font, number, static_cast<int>(strlen(number)));
			surface->FlushCachedState();
			surface->DrawTextNoClip(rcNumber, vsPrint.styles[STYLE_LINENUMBER].font,
				static_cast<XYPOSITION>(ypos + vsPrint.maxAscent), number, static_cast<int>(strlen(number)),
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
