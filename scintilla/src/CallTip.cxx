// Scintilla source code edit control
/** @file CallTip.cxx
 ** Code for displaying call tips.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
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
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Position.h"
#include "CallTip.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

size_t Chunk::Length() const noexcept {
	return end - start;
}

CallTip::CallTip() noexcept {
	wCallTip = {};
	inCallTipMode = false;
	posStartCallTip = 0;
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	lineHeight = 1;
	offsetMain = 0;
	tabSize = 0;
	above = false;
	useStyleCallTip = false;    // for backwards compatibility

	insetX = 5;
	widthArrow = 14;
	borderHeight = 2; // Extra line for border and an empty line at top and bottom.
	verticalOffset = 1;

#ifdef __APPLE__
	// proper apple colours for the default
	colourBG = ColourRGBA(0xff, 0xff, 0xc6);
	colourUnSel = ColourRGBA(0, 0, 0);
#else
	colourBG = ColourRGBA(0xff, 0xff, 0xff);
	colourUnSel = ColourRGBA(0x80, 0x80, 0x80);
#endif
	colourSel = ColourRGBA(0, 0, 0x80);
	colourShade = ColourRGBA(0, 0, 0);
	colourLight = ColourRGBA(0xc0, 0xc0, 0xc0);
	codePage = 0;
	clickPlace = 0;
}

CallTip::~CallTip() {
	wCallTip.Destroy();
}

// We ignore tabs unless a tab width has been set.
bool CallTip::IsTabCharacter(char ch) const noexcept {
	return (tabSize > 0) && (ch == '\t');
}

int CallTip::NextTabPos(int x) const noexcept {
	if (tabSize > 0) {              // paranoia... not called unless this is true
		x -= insetX;                // position relative to text
		x = (x + tabSize) / tabSize;  // tab "number"
		return tabSize*x + insetX;  // position of next tab
	} else {
		return x + 1;                 // arbitrary
	}
}

namespace {

// Although this test includes 0, we should never see a \0 character.
constexpr bool IsArrowCharacter(char ch) noexcept {
	return (ch == 0) || (ch == '\001') || (ch == '\002');
}

void DrawArrow(Surface *surface, const PRectangle &rc, bool upArrow, ColourRGBA colourBG, ColourRGBA colourUnSel) {
	surface->FillRectangle(rc, colourBG);
	const PRectangle rcClientInner = Clamp(rc.Inset(1), Edge::right, rc.right - 2);
	surface->FillRectangle(rcClientInner, colourUnSel);

	const XYPOSITION width = std::floor(rcClientInner.Width());
	const XYPOSITION halfWidth = std::floor(width / 2) - 1;
	const XYPOSITION quarterWidth = std::floor(halfWidth / 2);
	const XYPOSITION centreX = rcClientInner.left + width / 2;
	const XYPOSITION centreY = std::floor((rcClientInner.top + rcClientInner.bottom) / 2);

	constexpr XYPOSITION pixelMove = 0.0f;
	if (upArrow) {      // Up arrow
		Point pts[] = {
			Point(centreX - halfWidth + pixelMove, centreY + quarterWidth + 0.5f),
			Point(centreX + halfWidth + pixelMove, centreY + quarterWidth + 0.5f),
			Point(centreX + pixelMove, centreY - halfWidth + quarterWidth + 0.5f),
		};
		surface->Polygon(pts, std::size(pts), FillStroke(colourBG));
	} else {            // Down arrow
		Point pts[] = {
			Point(centreX - halfWidth + pixelMove, centreY - quarterWidth + 0.5f),
			Point(centreX + halfWidth + pixelMove, centreY - quarterWidth + 0.5f),
			Point(centreX + pixelMove, centreY + halfWidth - quarterWidth + 0.5f),
		};
		surface->Polygon(pts, std::size(pts), FillStroke(colourBG));
	}
}

}

// Draw a section of the call tip that does not include \n in one colour.
// The text may include tabs or arrow characters.
int CallTip::DrawChunk(Surface *surface, int x, std::string_view sv,
	int ytext, PRectangle rcClient, bool asHighlight, bool draw) {

	if (sv.empty()) {
		return x;
	}

	// Divide the text into sections that are all text, or that are
	// single arrows or single tab characters (if tabSize > 0).
	// Start with single element 0 to simplify append checks.
	std::vector<size_t> ends(1);
	for (size_t i=0; i<sv.length(); i++) {
		if (IsArrowCharacter(sv[i]) || IsTabCharacter(sv[i])) {
			if (ends.back() != i)
				ends.push_back(i);
			ends.push_back(i+1);
		}
	}
	if (ends.back() != sv.length())
		ends.push_back(sv.length());
	ends.erase(ends.begin());	// Remove initial 0.

	size_t startSeg = 0;
	for (const size_t endSeg : ends) {
		assert(endSeg > 0);
		int xEnd;
		if (IsArrowCharacter(sv[startSeg])) {
			xEnd = x + widthArrow;
			const bool upArrow = sv[startSeg] == '\001';
			rcClient.left = static_cast<XYPOSITION>(x);
			rcClient.right = static_cast<XYPOSITION>(xEnd);
			if (draw) {
				DrawArrow(surface, rcClient, upArrow, colourBG, colourUnSel);
			}
			offsetMain = xEnd;
			if (upArrow) {
				rectUp = rcClient;
			} else {
				rectDown = rcClient;
			}
		} else if (IsTabCharacter(sv[startSeg])) {
			xEnd = NextTabPos(x);
		} else {
			const std::string_view segText = sv.substr(startSeg, endSeg - startSeg);
			xEnd = x + static_cast<int>(std::lround(surface->WidthText(font.get(), segText)));
			if (draw) {
				rcClient.left = static_cast<XYPOSITION>(x);
				rcClient.right = static_cast<XYPOSITION>(xEnd);
				surface->DrawTextTransparent(rcClient, font.get(), static_cast<XYPOSITION>(ytext),
									segText, asHighlight ? colourSel : colourUnSel);
			}
		}
		x = xEnd;
		startSeg = endSeg;
	}
	return x;
}

int CallTip::PaintContents(Surface *surfaceWindow, bool draw) {
	const PRectangle rcClientPos = wCallTip.GetClientPosition();
	const PRectangle rcClientSize(0.0f, 0.0f, rcClientPos.right - rcClientPos.left,
	                        rcClientPos.bottom - rcClientPos.top);
	PRectangle rcClient(1.0f, 1.0f, rcClientSize.right - 1, rcClientSize.bottom - 1);

	// To make a nice small call tip window, it is only sized to fit most normal characters without accents
	const int ascent = static_cast<int>(std::round(surfaceWindow->Ascent(font.get()) - surfaceWindow->InternalLeading(font.get())));

	// For each line...
	// Draw the definition in three parts: before highlight, highlighted, after highlight
	int ytext = static_cast<int>(rcClient.top) + ascent + 1;
	rcClient.bottom = ytext + surfaceWindow->Descent(font.get()) + 1;
	std::string_view remaining(val);
	int maxWidth = 0;
	size_t lineStart = 0;
	while (!remaining.empty()) {
		const std::string_view chunkVal = remaining.substr(0, remaining.find_first_of('\n'));
		remaining.remove_prefix(chunkVal.length());
		if (!remaining.empty()) {
			remaining.remove_prefix(1);	// Skip \n
		}

		const Chunk chunkLine(lineStart, lineStart + chunkVal.length());
		Chunk chunkHighlight(
			std::clamp(highlight.start, chunkLine.start, chunkLine.end),
			std::clamp(highlight.end, chunkLine.start, chunkLine.end)
		);
		chunkHighlight.start -= lineStart;
		chunkHighlight.end -= lineStart;

		rcClient.top = static_cast<XYPOSITION>(ytext - ascent - 1);

		int x = insetX;     // start each line at this inset

		x = DrawChunk(surfaceWindow, x,
			chunkVal.substr(0, chunkHighlight.start),
			ytext, rcClient, false, draw);
		x = DrawChunk(surfaceWindow, x,
			chunkVal.substr(chunkHighlight.start, chunkHighlight.Length()),
			ytext, rcClient, true, draw);
		x = DrawChunk(surfaceWindow, x,
			chunkVal.substr(chunkHighlight.end),
			ytext, rcClient, false, draw);

		ytext += lineHeight;
		rcClient.bottom += lineHeight;
		maxWidth = std::max(maxWidth, x);
		lineStart += chunkVal.length() + 1;
	}
	return maxWidth;
}

void CallTip::PaintCT(Surface *surfaceWindow) {
	if (val.empty())
		return;
	const PRectangle rcClientPos = wCallTip.GetClientPosition();
	const PRectangle rcClientSize(0.0f, 0.0f, rcClientPos.right - rcClientPos.left,
	                        rcClientPos.bottom - rcClientPos.top);
	const PRectangle rcClient(1.0f, 1.0f, rcClientSize.right - 1, rcClientSize.bottom - 1);

	surfaceWindow->FillRectangle(rcClient, colourBG);

	offsetMain = insetX;    // initial alignment assuming no arrows
	PaintContents(surfaceWindow, true);

#if !defined(__APPLE__) && !PLAT_CURSES
	// OSX doesn't put borders on "help tags"
	// Draw a raised border around the edges of the window
	constexpr XYPOSITION border = 1.0f;
	surfaceWindow->FillRectangle(Side(rcClientSize, Edge::left, border), colourLight);
	surfaceWindow->FillRectangle(Side(rcClientSize, Edge::right, border), colourShade);
	surfaceWindow->FillRectangle(Side(rcClientSize, Edge::bottom, border), colourShade);
	surfaceWindow->FillRectangle(Side(rcClientSize, Edge::top, border), colourLight);
#endif
}

void CallTip::MouseClick(Point pt) noexcept {
	clickPlace = 0;
	if (rectUp.Contains(pt))
		clickPlace = 1;
	if (rectDown.Contains(pt))
		clickPlace = 2;
}

PRectangle CallTip::CallTipStart(Sci::Position pos, Point pt, int textHeight, const char *defn,
                                 int codePage_, Surface *surfaceMeasure, std::shared_ptr<Font> font_) {
	clickPlace = 0;
	val = defn;
	codePage = codePage_;
	highlight = Chunk();
	inCallTipMode = true;
	posStartCallTip = pos;
	font = font_;
	// Look for multiple lines in the text
	// Only support \n here - simply means container must avoid \r!
	const int numLines = 1 + static_cast<int>(std::count(val.begin(), val.end(), '\n'));
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	offsetMain = insetX;            // changed to right edge of any arrows
	lineHeight = static_cast<int>(std::lround(surfaceMeasure->Height(font.get())));
#if !PLAT_CURSES
	widthArrow = lineHeight * 9 / 10;
#endif
	const int width = PaintContents(surfaceMeasure, false) + insetX;

	// The returned
	// rectangle is aligned to the right edge of the last arrow encountered in
	// the tip text, else to the tip text left edge.
	const int height = lineHeight * numLines - static_cast<int>(surfaceMeasure->InternalLeading(font.get())) + borderHeight * 2;
	if (above) {
		return PRectangle(pt.x - offsetMain, pt.y - verticalOffset - height, pt.x + width - offsetMain, pt.y - verticalOffset);
	} else {
		return PRectangle(pt.x - offsetMain, pt.y + verticalOffset + textHeight, pt.x + width - offsetMain, pt.y + verticalOffset + textHeight + height);
	}
}

void CallTip::CallTipCancel() noexcept {
	inCallTipMode = false;
	if (wCallTip.Created()) {
		wCallTip.Destroy();
	}
}

void CallTip::SetHighlight(size_t start, size_t end) {
	// Avoid flashing by checking something has really changed
	if ((start != highlight.start) || (end != highlight.end)) {
		highlight.start = start;
		highlight.end = (end > start) ? end : start;
		if (wCallTip.Created()) {
			wCallTip.InvalidateAll();
		}
	}
}

// Set the tab size (sizes > 0 enable the use of tabs). This also enables the
// use of the StyleCallTip.
void CallTip::SetTabSize(int tabSz) noexcept {
	tabSize = tabSz;
	useStyleCallTip = true;
}

// Set the calltip position, below the text by default or if above is false
// else above the text.
void CallTip::SetPosition(bool aboveText) noexcept {
	above = aboveText;
}

bool CallTip::UseStyleCallTip() const noexcept {
	return useStyleCallTip;
}

// It might be better to have two access functions for this and to use
// them for all settings of colours.
void CallTip::SetForeBack(ColourRGBA fore, ColourRGBA back) noexcept {
	colourBG = back;
	colourUnSel = fore;
}
