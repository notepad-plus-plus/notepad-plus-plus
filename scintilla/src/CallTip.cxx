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
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Scintilla.h"

#include "Position.h"
#include "IntegerRectangle.h"
#include "CallTip.h"

using namespace Scintilla;

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
	colourBG = ColourDesired(0xff, 0xff, 0xc6);
	colourUnSel = ColourDesired(0, 0, 0);
#else
	colourBG = ColourDesired(0xff, 0xff, 0xff);
	colourUnSel = ColourDesired(0x80, 0x80, 0x80);
#endif
	colourSel = ColourDesired(0, 0, 0x80);
	colourShade = ColourDesired(0, 0, 0);
	colourLight = ColourDesired(0xc0, 0xc0, 0xc0);
	codePage = 0;
	clickPlace = 0;
}

CallTip::~CallTip() {
	font.Release();
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

void DrawArrow(Scintilla::Surface *surface, const PRectangle &rc, bool upArrow, ColourDesired colourBG, ColourDesired colourUnSel) {
	surface->FillRectangle(rc, colourBG);
	const int width = static_cast<int>(rc.Width());
	const int halfWidth = width / 2 - 3;
	const int quarterWidth = halfWidth / 2;
	const int centreX = static_cast<int>(rc.left) + width / 2 - 1;
	const int centreY = static_cast<int>(rc.top + rc.bottom) / 2;
	const PRectangle rcClientInner(rc.left + 1, rc.top + 1, rc.right - 2, rc.bottom - 1);
	surface->FillRectangle(rcClientInner, colourUnSel);

	if (upArrow) {      // Up arrow
		Point pts[] = {
			Point::FromInts(centreX - halfWidth, centreY + quarterWidth),
			Point::FromInts(centreX + halfWidth, centreY + quarterWidth),
			Point::FromInts(centreX, centreY - halfWidth + quarterWidth),
		};
		surface->Polygon(pts, std::size(pts), colourBG, colourBG);
	} else {            // Down arrow
		Point pts[] = {
			Point::FromInts(centreX - halfWidth, centreY - quarterWidth),
			Point::FromInts(centreX + halfWidth, centreY - quarterWidth),
			Point::FromInts(centreX, centreY + halfWidth - quarterWidth),
		};
		surface->Polygon(pts, std::size(pts), colourBG, colourBG);
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
			xEnd = x + static_cast<int>(std::lround(surface->WidthText(font, segText)));
			if (draw) {
				rcClient.left = static_cast<XYPOSITION>(x);
				rcClient.right = static_cast<XYPOSITION>(xEnd);
				surface->DrawTextTransparent(rcClient, font, static_cast<XYPOSITION>(ytext),
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
	const int ascent = static_cast<int>(std::round(surfaceWindow->Ascent(font) - surfaceWindow->InternalLeading(font)));

	// For each line...
	// Draw the definition in three parts: before highlight, highlighted, after highlight
	int ytext = static_cast<int>(rcClient.top) + ascent + 1;
	rcClient.bottom = ytext + surfaceWindow->Descent(font) + 1;
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

#ifndef __APPLE__
	// OSX doesn't put borders on "help tags"
	// Draw a raised border around the edges of the window
	const IntegerRectangle ircClientSize(rcClientSize);
	surfaceWindow->MoveTo(0, ircClientSize.bottom - 1);
	surfaceWindow->PenColour(colourShade);
	surfaceWindow->LineTo(ircClientSize.right - 1, ircClientSize.bottom - 1);
	surfaceWindow->LineTo(ircClientSize.right - 1, 0);
	surfaceWindow->PenColour(colourLight);
	surfaceWindow->LineTo(0, 0);
	surfaceWindow->LineTo(0, ircClientSize.bottom - 1);
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
                                 const char *faceName, int size,
                                 int codePage_, int characterSet,
								 int technology, const Window &wParent) {
	clickPlace = 0;
	val = defn;
	codePage = codePage_;
	std::unique_ptr<Surface> surfaceMeasure(Surface::Allocate(technology));
	surfaceMeasure->Init(wParent.GetID());
	surfaceMeasure->SetUnicodeMode(SC_CP_UTF8 == codePage);
	surfaceMeasure->SetDBCSMode(codePage);
	highlight = Chunk();
	inCallTipMode = true;
	posStartCallTip = pos;
	const XYPOSITION deviceHeight = static_cast<XYPOSITION>(surfaceMeasure->DeviceHeightFont(size));
	const FontParameters fp(faceName, deviceHeight / SC_FONT_SIZE_MULTIPLIER, SC_WEIGHT_NORMAL, false, 0, technology, characterSet);
	font.Create(fp);
	// Look for multiple lines in the text
	// Only support \n here - simply means container must avoid \r!
	const int numLines = 1 + static_cast<int>(std::count(val.begin(), val.end(), '\n'));
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	offsetMain = insetX;            // changed to right edge of any arrows
	const int width = PaintContents(surfaceMeasure.get(), false) + insetX;
	lineHeight = static_cast<int>(std::lround(surfaceMeasure->Height(font)));

	// The returned
	// rectangle is aligned to the right edge of the last arrow encountered in
	// the tip text, else to the tip text left edge.
	const int height = lineHeight * numLines - static_cast<int>(surfaceMeasure->InternalLeading(font)) + borderHeight * 2;
	if (above) {
		return PRectangle(pt.x - offsetMain, pt.y - verticalOffset - height, pt.x + width - offsetMain, pt.y - verticalOffset);
	} else {
		return PRectangle(pt.x - offsetMain, pt.y + verticalOffset + textHeight, pt.x + width - offsetMain, pt.y + verticalOffset + textHeight + height);
	}
}

void CallTip::CallTipCancel() {
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
// use of the STYLE_CALLTIP.
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
void CallTip::SetForeBack(const ColourDesired &fore, const ColourDesired &back) noexcept {
	colourBG = back;
	colourUnSel = fore;
}
