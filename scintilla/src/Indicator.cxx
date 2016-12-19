// Scintilla source code edit control
/** @file Indicator.cxx
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <vector>
#include <map>

#include "Platform.h"

#include "Scintilla.h"
#include "Indicator.h"
#include "XPM.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static PRectangle PixelGridAlign(const PRectangle &rc) {
	// Move left and right side to nearest pixel to avoid blurry visuals
	return PRectangle::FromInts(int(rc.left + 0.5), int(rc.top), int(rc.right + 0.5), int(rc.bottom));
}

void Indicator::Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine, DrawState drawState, int value) const {
	StyleAndColour sacDraw = sacNormal;
	if (Flags() & SC_INDICFLAG_VALUEFORE) {
		sacDraw.fore = value & SC_INDICVALUEMASK;
	}
	if (drawState == drawHover) {
		sacDraw = sacHover;
	}
	surface->PenColour(sacDraw.fore);
	int ymid = static_cast<int>(rc.bottom + rc.top) / 2;
	if (sacDraw.style == INDIC_SQUIGGLE) {
		int x = int(rc.left+0.5);
		int xLast = int(rc.right+0.5);
		int y = 0;
		surface->MoveTo(x, static_cast<int>(rc.top) + y);
		while (x < xLast) {
			if ((x + 2) > xLast) {
				if (xLast > x)
					y = 1;
				x = xLast;
			} else {
				x += 2;
				y = 2 - y;
			}
			surface->LineTo(x, static_cast<int>(rc.top) + y);
		}
	} else if (sacDraw.style == INDIC_SQUIGGLEPIXMAP) {
		PRectangle rcSquiggle = PixelGridAlign(rc);

		int width = Platform::Minimum(4000, static_cast<int>(rcSquiggle.Width()));
		RGBAImage image(width, 3, 1.0, 0);
		enum { alphaFull = 0xff, alphaSide = 0x2f, alphaSide2=0x5f };
		for (int x = 0; x < width; x++) {
			if (x%2) {
				// Two halfway columns have a full pixel in middle flanked by light pixels
				image.SetPixel(x, 0, sacDraw.fore, alphaSide);
				image.SetPixel(x, 1, sacDraw.fore, alphaFull);
				image.SetPixel(x, 2, sacDraw.fore, alphaSide);
			} else {
				// Extreme columns have a full pixel at bottom or top and a mid-tone pixel in centre
				image.SetPixel(x, (x % 4) ? 0 : 2, sacDraw.fore, alphaFull);
				image.SetPixel(x, 1, sacDraw.fore, alphaSide2);
			}
		}
		surface->DrawRGBAImage(rcSquiggle, image.GetWidth(), image.GetHeight(), image.Pixels());
	} else if (sacDraw.style == INDIC_SQUIGGLELOW) {
		surface->MoveTo(static_cast<int>(rc.left), static_cast<int>(rc.top));
		int x = static_cast<int>(rc.left) + 3;
		int y = 0;
		while (x < rc.right) {
			surface->LineTo(x - 1, static_cast<int>(rc.top) + y);
			y = 1 - y;
			surface->LineTo(x, static_cast<int>(rc.top) + y);
			x += 3;
		}
		surface->LineTo(static_cast<int>(rc.right), static_cast<int>(rc.top) + y);	// Finish the line
	} else if (sacDraw.style == INDIC_TT) {
		surface->MoveTo(static_cast<int>(rc.left), ymid);
		int x = static_cast<int>(rc.left) + 5;
		while (x < rc.right) {
			surface->LineTo(x, ymid);
			surface->MoveTo(x-3, ymid);
			surface->LineTo(x-3, ymid+2);
			x++;
			surface->MoveTo(x, ymid);
			x += 5;
		}
		surface->LineTo(static_cast<int>(rc.right), ymid);	// Finish the line
		if (x - 3 <= rc.right) {
			surface->MoveTo(x-3, ymid);
			surface->LineTo(x-3, ymid+2);
		}
	} else if (sacDraw.style == INDIC_DIAGONAL) {
		int x = static_cast<int>(rc.left);
		while (x < rc.right) {
			surface->MoveTo(x, static_cast<int>(rc.top) + 2);
			int endX = x+3;
			int endY = static_cast<int>(rc.top) - 1;
			if (endX > rc.right) {
				endY += endX - static_cast<int>(rc.right);
				endX = static_cast<int>(rc.right);
			}
			surface->LineTo(endX, endY);
			x += 4;
		}
	} else if (sacDraw.style == INDIC_STRIKE) {
		surface->MoveTo(static_cast<int>(rc.left), static_cast<int>(rc.top) - 4);
		surface->LineTo(static_cast<int>(rc.right), static_cast<int>(rc.top) - 4);
	} else if ((sacDraw.style == INDIC_HIDDEN) || (sacDraw.style == INDIC_TEXTFORE)) {
		// Draw nothing
	} else if (sacDraw.style == INDIC_BOX) {
		surface->MoveTo(static_cast<int>(rc.left), ymid + 1);
		surface->LineTo(static_cast<int>(rc.right), ymid + 1);
		surface->LineTo(static_cast<int>(rc.right), static_cast<int>(rcLine.top) + 1);
		surface->LineTo(static_cast<int>(rc.left), static_cast<int>(rcLine.top) + 1);
		surface->LineTo(static_cast<int>(rc.left), ymid + 1);
	} else if (sacDraw.style == INDIC_ROUNDBOX ||
		sacDraw.style == INDIC_STRAIGHTBOX ||
		sacDraw.style == INDIC_FULLBOX) {
		PRectangle rcBox = rcLine;
		if (sacDraw.style != INDIC_FULLBOX)
			rcBox.top = rcLine.top + 1;
		rcBox.left = rc.left;
		rcBox.right = rc.right;
		surface->AlphaRectangle(rcBox, (sacDraw.style == INDIC_ROUNDBOX) ? 1 : 0, 
			sacDraw.fore, fillAlpha, sacDraw.fore, outlineAlpha, 0);
	} else if (sacDraw.style == INDIC_DOTBOX) {
		PRectangle rcBox = PixelGridAlign(rc);
		rcBox.top = rcLine.top + 1;
		rcBox.bottom = rcLine.bottom;
		// Cap width at 4000 to avoid large allocations when mistakes made
		int width = Platform::Minimum(static_cast<int>(rcBox.Width()), 4000);
		RGBAImage image(width, static_cast<int>(rcBox.Height()), 1.0, 0);
		// Draw horizontal lines top and bottom
		for (int x=0; x<width; x++) {
			for (int y = 0; y<static_cast<int>(rcBox.Height()); y += static_cast<int>(rcBox.Height()) - 1) {
				image.SetPixel(x, y, sacDraw.fore, ((x + y) % 2) ? outlineAlpha : fillAlpha);
			}
		}
		// Draw vertical lines left and right
		for (int y = 1; y<static_cast<int>(rcBox.Height()); y++) {
			for (int x=0; x<width; x += width-1) {
				image.SetPixel(x, y, sacDraw.fore, ((x + y) % 2) ? outlineAlpha : fillAlpha);
			}
		}
		surface->DrawRGBAImage(rcBox, image.GetWidth(), image.GetHeight(), image.Pixels());
	} else if (sacDraw.style == INDIC_DASH) {
		int x = static_cast<int>(rc.left);
		while (x < rc.right) {
			surface->MoveTo(x, ymid);
			surface->LineTo(Platform::Minimum(x + 4, static_cast<int>(rc.right)), ymid);
			x += 7;
		}
	} else if (sacDraw.style == INDIC_DOTS) {
		int x = static_cast<int>(rc.left);
		while (x < static_cast<int>(rc.right)) {
			PRectangle rcDot = PRectangle::FromInts(x, ymid, x + 1, ymid + 1);
			surface->FillRectangle(rcDot, sacDraw.fore);
			x += 2;
		}
	} else if (sacDraw.style == INDIC_COMPOSITIONTHICK) {
		PRectangle rcComposition(rc.left+1, rcLine.bottom-2, rc.right-1, rcLine.bottom);
		surface->FillRectangle(rcComposition, sacDraw.fore);
	} else if (sacDraw.style == INDIC_COMPOSITIONTHIN) {
		PRectangle rcComposition(rc.left+1, rcLine.bottom-2, rc.right-1, rcLine.bottom-1);
		surface->FillRectangle(rcComposition, sacDraw.fore);
	} else {	// Either INDIC_PLAIN or unknown
		surface->MoveTo(static_cast<int>(rc.left), ymid);
		surface->LineTo(static_cast<int>(rc.right), ymid);
	}
}

void Indicator::SetFlags(int attributes_) {
	attributes = attributes_;
}
