// Scintilla source code edit control
/** @file CallTip.cxx
 ** Code for displaying call tips.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>

#include "Platform.h"

#include "Scintilla.h"
#include "CallTip.h"
#include <stdio.h>

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static const int insetX = 5;    // text inset in x from calltip border
static const int widthArrow = 14;

CallTip::CallTip() {
	wCallTip = 0;
	inCallTipMode = false;
	posStartCallTip = 0;
	val = 0;
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	lineHeight = 1;
	offsetMain = 0;
	startHighlight = 0;
	endHighlight = 0;
	tabSize = 0;
	useStyleCallTip = false;    // for backwards compatibility

#ifdef __APPLE__
	// proper apple colours for the default
	colourBG.desired = ColourDesired(0xff, 0xff, 0xc6);
	colourUnSel.desired = ColourDesired(0, 0, 0);
#else
	colourBG.desired = ColourDesired(0xff, 0xff, 0xff);
	colourUnSel.desired = ColourDesired(0x80, 0x80, 0x80);
#endif
	colourSel.desired = ColourDesired(0, 0, 0x80);
	colourShade.desired = ColourDesired(0, 0, 0);
	colourLight.desired = ColourDesired(0xc0, 0xc0, 0xc0);
	codePage = 0;
	clickPlace = 0;
}

CallTip::~CallTip() {
	font.Release();
	wCallTip.Destroy();
	delete []val;
	val = 0;
}

void CallTip::RefreshColourPalette(Palette &pal, bool want) {
	pal.WantFind(colourBG, want);
	pal.WantFind(colourUnSel, want);
	pal.WantFind(colourSel, want);
	pal.WantFind(colourShade, want);
	pal.WantFind(colourLight, want);
}

// Although this test includes 0, we should never see a \0 character.
static bool IsArrowCharacter(char ch) {
	return (ch == 0) || (ch == '\001') || (ch == '\002');
}

// We ignore tabs unless a tab width has been set.
bool CallTip::IsTabCharacter(char ch) const {
	return (tabSize > 0) && (ch == '\t');
}

int CallTip::NextTabPos(int x) {
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
	int len = posEnd - posStart;

	// Divide the text into sections that are all text, or that are
	// single arrows or single tab characters (if tabSize > 0).
	int maxEnd = 0;
	const int numEnds = 10;
	int ends[numEnds + 2];
	for (int i=0; i<len; i++) {
		if ((maxEnd < numEnds) &&
		        (IsArrowCharacter(s[i]) || IsTabCharacter(s[i])) ) {
			if (i > 0)
				ends[maxEnd++] = i;
			ends[maxEnd++] = i+1;
		}
	}
	ends[maxEnd++] = len;
	int startSeg = 0;
	int xEnd;
	for (int seg = 0; seg<maxEnd; seg++) {
		int endSeg = ends[seg];
		if (endSeg > startSeg) {
			if (IsArrowCharacter(s[startSeg])) {
				bool upArrow = s[startSeg] == '\001';
				rcClient.left = x;
				rcClient.right = rcClient.left + widthArrow;
				if (draw) {
					const int halfWidth = widthArrow / 2 - 3;
					const int centreX = rcClient.left + widthArrow / 2 - 1;
					const int centreY = (rcClient.top + rcClient.bottom) / 2;
					surface->FillRectangle(rcClient, colourBG.allocated);
					PRectangle rcClientInner(rcClient.left + 1, rcClient.top + 1,
					                         rcClient.right - 2, rcClient.bottom - 1);
					surface->FillRectangle(rcClientInner, colourUnSel.allocated);

					if (upArrow) {      // Up arrow
						Point pts[] = {
    						Point(centreX - halfWidth, centreY + halfWidth / 2),
    						Point(centreX + halfWidth, centreY + halfWidth / 2),
    						Point(centreX, centreY - halfWidth + halfWidth / 2),
						};
						surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 						colourBG.allocated, colourBG.allocated);
					} else {            // Down arrow
						Point pts[] = {
    						Point(centreX - halfWidth, centreY - halfWidth / 2),
    						Point(centreX + halfWidth, centreY - halfWidth / 2),
    						Point(centreX, centreY + halfWidth - halfWidth / 2),
						};
						surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 						colourBG.allocated, colourBG.allocated);
					}
				}
				xEnd = rcClient.right;
				offsetMain = xEnd;
				if (upArrow) {
					rectUp = rcClient;
				} else {
					rectDown = rcClient;
				}
			} else if (IsTabCharacter(s[startSeg])) {
				xEnd = NextTabPos(x);
			} else {
				xEnd = x + surface->WidthText(font, s + startSeg, endSeg - startSeg);
				if (draw) {
					rcClient.left = x;
					rcClient.right = xEnd;
					surface->DrawTextTransparent(rcClient, font, ytext,
										s+startSeg, endSeg - startSeg,
					                             highlight ? colourSel.allocated : colourUnSel.allocated);
				}
			}
			x = xEnd;
			startSeg = endSeg;
		}
	}
}

int CallTip::PaintContents(Surface *surfaceWindow, bool draw) {
	PRectangle rcClientPos = wCallTip.GetClientPosition();
	PRectangle rcClientSize(0, 0, rcClientPos.right - rcClientPos.left,
	                        rcClientPos.bottom - rcClientPos.top);
	PRectangle rcClient(1, 1, rcClientSize.right - 1, rcClientSize.bottom - 1);

	// To make a nice small call tip window, it is only sized to fit most normal characters without accents
	int ascent = surfaceWindow->Ascent(font) - surfaceWindow->InternalLeading(font);

	// For each line...
	// Draw the definition in three parts: before highlight, highlighted, after highlight
	int ytext = rcClient.top + ascent + 1;
	rcClient.bottom = ytext + surfaceWindow->Descent(font) + 1;
	char *chunkVal = val;
	bool moreChunks = true;
	int maxWidth = 0;

	while (moreChunks) {
		char *chunkEnd = strchr(chunkVal, '\n');
		if (chunkEnd == NULL) {
			chunkEnd = chunkVal + strlen(chunkVal);
			moreChunks = false;
		}
		int chunkOffset = chunkVal - val;
		int chunkLength = chunkEnd - chunkVal;
		int chunkEndOffset = chunkOffset + chunkLength;
		int thisStartHighlight = Platform::Maximum(startHighlight, chunkOffset);
		thisStartHighlight = Platform::Minimum(thisStartHighlight, chunkEndOffset);
		thisStartHighlight -= chunkOffset;
		int thisEndHighlight = Platform::Maximum(endHighlight, chunkOffset);
		thisEndHighlight = Platform::Minimum(thisEndHighlight, chunkEndOffset);
		thisEndHighlight -= chunkOffset;
		rcClient.top = ytext - ascent - 1;

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
		maxWidth = Platform::Maximum(maxWidth, x);
	}
	return maxWidth;
}

void CallTip::PaintCT(Surface *surfaceWindow) {
	if (!val)
		return;
	PRectangle rcClientPos = wCallTip.GetClientPosition();
	PRectangle rcClientSize(0, 0, rcClientPos.right - rcClientPos.left,
	                        rcClientPos.bottom - rcClientPos.top);
	PRectangle rcClient(1, 1, rcClientSize.right - 1, rcClientSize.bottom - 1);

	surfaceWindow->FillRectangle(rcClient, colourBG.allocated);

	offsetMain = insetX;    // initial alignment assuming no arrows
	PaintContents(surfaceWindow, true);

#ifndef __APPLE__
	// OSX doesn't put borders on "help tags"
	// Draw a raised border around the edges of the window
	surfaceWindow->MoveTo(0, rcClientSize.bottom - 1);
	surfaceWindow->PenColour(colourShade.allocated);
	surfaceWindow->LineTo(rcClientSize.right - 1, rcClientSize.bottom - 1);
	surfaceWindow->LineTo(rcClientSize.right - 1, 0);
	surfaceWindow->PenColour(colourLight.allocated);
	surfaceWindow->LineTo(0, 0);
	surfaceWindow->LineTo(0, rcClientSize.bottom - 1);
#endif
}

void CallTip::MouseClick(Point pt) {
	clickPlace = 0;
	if (rectUp.Contains(pt))
		clickPlace = 1;
	if (rectDown.Contains(pt))
		clickPlace = 2;
}

PRectangle CallTip::CallTipStart(int pos, Point pt, const char *defn,
                                 const char *faceName, int size,
                                 int codePage_, int characterSet, Window &wParent) {
	clickPlace = 0;
	delete []val;
	val = 0;
	val = new char[strlen(defn) + 1];
	strcpy(val, defn);
	codePage = codePage_;
	Surface *surfaceMeasure = Surface::Allocate();
	if (!surfaceMeasure)
		return PRectangle();
	surfaceMeasure->Init(wParent.GetID());
	surfaceMeasure->SetUnicodeMode(SC_CP_UTF8 == codePage);
	surfaceMeasure->SetDBCSMode(codePage);
	startHighlight = 0;
	endHighlight = 0;
	inCallTipMode = true;
	posStartCallTip = pos;
	int deviceHeight = surfaceMeasure->DeviceHeightFont(size);
	font.Create(faceName, characterSet, deviceHeight, false, false);
	// Look for multiple lines in the text
	// Only support \n here - simply means container must avoid \r!
	int numLines = 1;
	const char *newline;
	const char *look = val;
	rectUp = PRectangle(0,0,0,0);
	rectDown = PRectangle(0,0,0,0);
	offsetMain = insetX;            // changed to right edge of any arrows
	int width = PaintContents(surfaceMeasure, false) + insetX;
	while ((newline = strchr(look, '\n')) != NULL) {
		look = newline + 1;
		numLines++;
	}
	lineHeight = surfaceMeasure->Height(font);

	// Extra line for border and an empty line at top and bottom. The returned
	// rectangle is aligned to the right edge of the last arrow encountered in
	// the tip text, else to the tip text left edge.
	int height = lineHeight * numLines - surfaceMeasure->InternalLeading(font) + 2 + 2;
	delete surfaceMeasure;
	return PRectangle(pt.x - offsetMain, pt.y + 1, pt.x + width - offsetMain, pt.y + 1 + height);
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
		endHighlight = end;
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

// It might be better to have two access functions for this and to use
// them for all settings of colours.
void CallTip::SetForeBack(const ColourPair &fore, const ColourPair &back) {
	colourBG = back;
	colourUnSel = fore;
}
