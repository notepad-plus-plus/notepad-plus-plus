// Scintilla source code edit control
/** @file CallTip.cxx
 ** Code for displaying call tips.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
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

CallTip::CallTip() {
	wCallTip = 0;
	inCallTipMode = false;
	posStartCallTip = 0;
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	lineHeight = 1;
	offsetMain = 0;
	startHighlight = 0;
	endHighlight = 0;
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

// Although this test includes 0, we should never see a \0 character.
static bool IsArrowCharacter(char ch) {
	return (ch == 0) || (ch == '\001') || (ch == '\002');
}

// We ignore tabs unless a tab width has been set.
bool CallTip::IsTabCharacter(char ch) const {
	return (tabSize > 0) && (ch == '\t');
}

int CallTip::NextTabPos(int x) const {
	if (tabSize > 0) {              // paranoia... not called unless this is true
		x -= insetX;                // position relative to text
		x = (x + tabSize) / tabSize;  // tab "number"
		return tabSize*x + insetX;  // position of next tab
	} else {
		return x + 1;                 // arbitrary
	}
}

// Draw a section of the call tip that does not include \n in one colour.
// The text may include up to numEnds tabs or arrow characters.
void CallTip::DrawChunk(Surface *surface, int &x, const char *s,
	int posStart, int posEnd, int ytext, PRectangle rcClient,
	bool highlight, bool draw) {
	s += posStart;
	const int len = posEnd - posStart;

	// Divide the text into sections that are all text, or that are
	// single arrows or single tab characters (if tabSize > 0).
	int maxEnd = 0;
	const int numEnds = 10;
	int ends[numEnds + 2];
	for (int i=0; i<len; i++) {
		if ((maxEnd < numEnds) &&
		        (IsArrowCharacter(s[i]) || IsTabCharacter(s[i]))) {
			if (i > 0)
				ends[maxEnd++] = i;
			ends[maxEnd++] = i+1;
		}
	}
	ends[maxEnd++] = len;
	int startSeg = 0;
	int xEnd;
	for (int seg = 0; seg<maxEnd; seg++) {
		const int endSeg = ends[seg];
		if (endSeg > startSeg) {
			if (IsArrowCharacter(s[startSeg])) {
				xEnd = x + widthArrow;
				const bool upArrow = s[startSeg] == '\001';
				rcClient.left = static_cast<XYPOSITION>(x);
				rcClient.right = static_cast<XYPOSITION>(xEnd);
				if (draw) {
					const int halfWidth = widthArrow / 2 - 3;
					const int quarterWidth = halfWidth / 2;
					const int centreX = x + widthArrow / 2 - 1;
					const int centreY = static_cast<int>(rcClient.top + rcClient.bottom) / 2;
					surface->FillRectangle(rcClient, colourBG);
					const PRectangle rcClientInner(rcClient.left + 1, rcClient.top + 1,
					                         rcClient.right - 2, rcClient.bottom - 1);
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
				offsetMain = xEnd;
				if (upArrow) {
					rectUp = rcClient;
				} else {
					rectDown = rcClient;
				}
			} else if (IsTabCharacter(s[startSeg])) {
				xEnd = NextTabPos(x);
			} else {
				std::string_view segText(s + startSeg, endSeg - startSeg);
				xEnd = x + static_cast<int>(std::lround(surface->WidthText(font, segText)));
				if (draw) {
					rcClient.left = static_cast<XYPOSITION>(x);
					rcClient.right = static_cast<XYPOSITION>(xEnd);
					surface->DrawTextTransparent(rcClient, font, static_cast<XYPOSITION>(ytext),
										segText, highlight ? colourSel : colourUnSel);
				}
			}
			x = xEnd;
			startSeg = endSeg;
		}
	}
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
	const char *chunkVal = val.c_str();
	bool moreChunks = true;
	int maxWidth = 0;

	while (moreChunks) {
		const char *chunkEnd = strchr(chunkVal, '\n');
		if (!chunkEnd) {
			chunkEnd = chunkVal + strlen(chunkVal);
			moreChunks = false;
		}
		const int chunkOffset = static_cast<int>(chunkVal - val.c_str());
		const int chunkLength = static_cast<int>(chunkEnd - chunkVal);
		const int chunkEndOffset = chunkOffset + chunkLength;
		int thisStartHighlight = std::max(startHighlight, chunkOffset);
		thisStartHighlight = std::min(thisStartHighlight, chunkEndOffset);
		thisStartHighlight -= chunkOffset;
		int thisEndHighlight = std::max(endHighlight, chunkOffset);
		thisEndHighlight = std::min(thisEndHighlight, chunkEndOffset);
		thisEndHighlight -= chunkOffset;
		rcClient.top = static_cast<XYPOSITION>(ytext - ascent - 1);

		int x = insetX;     // start each line at this inset

		DrawChunk(surfaceWindow, x, chunkVal, 0, thisStartHighlight,
			ytext, rcClient, false, draw);
		DrawChunk(surfaceWindow, x, chunkVal, thisStartHighlight, thisEndHighlight,
			ytext, rcClient, true, draw);
		DrawChunk(surfaceWindow, x, chunkVal, thisEndHighlight, chunkLength,
			ytext, rcClient, false, draw);

		chunkVal = chunkEnd + 1;
		ytext += lineHeight;
		rcClient.bottom += lineHeight;
		maxWidth = std::max(maxWidth, x);
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

void CallTip::MouseClick(Point pt) {
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
	startHighlight = 0;
	endHighlight = 0;
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

void CallTip::SetHighlight(int start, int end) {
	// Avoid flashing by checking something has really changed
	if ((start != startHighlight) || (end != endHighlight)) {
		startHighlight = start;
		endHighlight = (end > start) ? end : start;
		if (wCallTip.Created()) {
			wCallTip.InvalidateAll();
		}
	}
}

// Set the tab size (sizes > 0 enable the use of tabs). This also enables the
// use of the STYLE_CALLTIP.
void CallTip::SetTabSize(int tabSz) {
	tabSize = tabSz;
	useStyleCallTip = true;
}

// Set the calltip position, below the text by default or if above is false
// else above the text.
void CallTip::SetPosition(bool aboveText) {
	above = aboveText;
}

// It might be better to have two access functions for this and to use
// them for all settings of colours.
void CallTip::SetForeBack(const ColourDesired &fore, const ColourDesired &back) {
	colourBG = back;
	colourUnSel = fore;
}
