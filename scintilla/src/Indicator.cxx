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
#include "XPM.h"
#include "Indicator.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static PRectangle PixelGridAlign(const PRectangle &rc) {
	// Move left and right side to nearest pixel to avoid blurry visuals
	return PRectangle(int(rc.left + 0.5), rc.top, int(rc.right + 0.5), rc.bottom);
}

void Indicator::Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine) {
	surface->PenColour(fore);
	int ymid = (rc.bottom + rc.top) / 2;
	if (style == INDIC_SQUIGGLE) {
		int x = int(rc.left+0.5);
		int xLast = int(rc.right+0.5);
		int y = 0;
		surface->MoveTo(x, rc.top + y);
		while (x < xLast) {
			if ((x + 2) > xLast) {
				if (xLast > x)
					y = 1;
				x = xLast;
			} else {
				x += 2;
				y = 2 - y;
			}
			surface->LineTo(x, rc.top + y);
		}
	} else if (style == INDIC_SQUIGGLEPIXMAP) {
		PRectangle rcSquiggle = PixelGridAlign(rc);

		int width = Platform::Minimum(4000, rcSquiggle.Width());
		RGBAImage image(width, 3, 1.0, 0);
		enum { alphaFull = 0xff, alphaSide = 0x2f, alphaSide2=0x5f };
		for (int x = 0; x < width; x++) {
			if (x%2) {
				// Two halfway columns have a full pixel in middle flanked by light pixels
				image.SetPixel(x, 0, fore, alphaSide);
				image.SetPixel(x, 1, fore, alphaFull);
				image.SetPixel(x, 2, fore, alphaSide);
			} else {
				// Extreme columns have a full pixel at bottom or top and a mid-tone pixel in centre
				image.SetPixel(x, (x%4) ? 0 : 2, fore, alphaFull);
				image.SetPixel(x, 1, fore, alphaSide2);
			}
		}
		surface->DrawRGBAImage(rcSquiggle, image.GetWidth(), image.GetHeight(), image.Pixels());
	} else if (style == INDIC_SQUIGGLELOW) {
		surface->MoveTo(rc.left, rc.top);
		int x = rc.left + 3;
		int y = 0;
		while (x < rc.right) {
			surface->LineTo(x-1, rc.top + y);
			y = 1 - y;
        	surface->LineTo(x, rc.top + y);
			x += 3;
		}
		surface->LineTo(rc.right, rc.top + y);	// Finish the line
	} else if (style == INDIC_TT) {
		surface->MoveTo(rc.left, ymid);
		int x = rc.left + 5;
		while (x < rc.right) {
			surface->LineTo(x, ymid);
			surface->MoveTo(x-3, ymid);
			surface->LineTo(x-3, ymid+2);
			x++;
			surface->MoveTo(x, ymid);
			x += 5;
		}
		surface->LineTo(rc.right, ymid);	// Finish the line
		if (x - 3 <= rc.right) {
			surface->MoveTo(x-3, ymid);
			surface->LineTo(x-3, ymid+2);
		}
	} else if (style == INDIC_DIAGONAL) {
		int x = rc.left;
		while (x < rc.right) {
			surface->MoveTo(x, rc.top+2);
			int endX = x+3;
			int endY = rc.top - 1;
			if (endX > rc.right) {
				endY += endX - rc.right;
				endX = rc.right;
			}
			surface->LineTo(endX, endY);
			x += 4;
		}
	} else if (style == INDIC_STRIKE) {
		surface->MoveTo(rc.left, rc.top - 4);
		surface->LineTo(rc.right, rc.top - 4);
	} else if (style == INDIC_HIDDEN) {
		// Draw nothing
	} else if (style == INDIC_BOX) {
		surface->MoveTo(rc.left, ymid+1);
		surface->LineTo(rc.right, ymid+1);
		surface->LineTo(rc.right, rcLine.top+1);
		surface->LineTo(rc.left, rcLine.top+1);
		surface->LineTo(rc.left, ymid+1);
	} else if (style == INDIC_ROUNDBOX || style == INDIC_STRAIGHTBOX) {
		PRectangle rcBox = rcLine;
		rcBox.top = rcLine.top + 1;
		rcBox.left = rc.left;
		rcBox.right = rc.right;
		surface->AlphaRectangle(rcBox, (style == INDIC_ROUNDBOX) ? 1 : 0, fore, fillAlpha, fore, outlineAlpha, 0);
	} else if (style == INDIC_DOTBOX) {
		PRectangle rcBox = PixelGridAlign(rc);
		rcBox.top = rcLine.top + 1;
		rcBox.bottom = rcLine.bottom;
		// Cap width at 4000 to avoid large allocations when mistakes made
		int width = Platform::Minimum(rcBox.Width(), 4000);
		RGBAImage image(width, rcBox.Height(), 1.0, 0);
		// Draw horizontal lines top and bottom
		for (int x=0; x<width; x++) {
			for (int y=0; y<rcBox.Height(); y += rcBox.Height()-1) {
				image.SetPixel(x, y, fore, ((x + y) % 2) ? outlineAlpha : fillAlpha);
			}
		}
		// Draw vertical lines left and right
		for (int y=1; y<rcBox.Height(); y++) {
			for (int x=0; x<width; x += width-1) {
				image.SetPixel(x, y, fore, ((x + y) % 2) ? outlineAlpha : fillAlpha);
			}
		}
		surface->DrawRGBAImage(rcBox, image.GetWidth(), image.GetHeight(), image.Pixels());
	} else if (style == INDIC_DASH) {
		int x = rc.left;
		while (x < rc.right) {
			surface->MoveTo(x, ymid);
			surface->LineTo(Platform::Minimum(x + 4, rc.right), ymid);
			x += 7;
		}
	} else if (style == INDIC_DOTS) {
		int x = rc.left;
		while (x < rc.right) {
			PRectangle rcDot(x, ymid, x+1, ymid+1);
			surface->FillRectangle(rcDot, fore);
			x += 2;
		}
	} else if (style == INDIC_COMPOSITIONTHICK) {
		PRectangle rcComposition(rc.left+1, rcLine.bottom-2, rc.right-1, rcLine.bottom);
		surface->FillRectangle(rcComposition, fore);
	} else {	// Either INDIC_PLAIN or unknown
		surface->MoveTo(rc.left, ymid);
		surface->LineTo(rc.right, ymid);
	}
}

