// Scintilla source code edit control
/** @file Indicator.cxx
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "Platform.h"

#include "Scintilla.h"
#include "Indicator.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

void Indicator::Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine) {
	surface->PenColour(fore.allocated);
	int ymid = (rc.bottom + rc.top) / 2;
	if (style == INDIC_SQUIGGLE) {
		surface->MoveTo(rc.left, rc.top);
		int x = rc.left + 2;
		int y = 2;
		while (x < rc.right) {
			surface->LineTo(x, rc.top + y);
			x += 2;
			y = 2 - y;
		}
		surface->LineTo(rc.right, rc.top + y);	// Finish the line
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
		surface->AlphaRectangle(rcBox, (style == INDIC_ROUNDBOX) ? 1 : 0, fore.allocated, fillAlpha, fore.allocated, outlineAlpha, 0);
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
			surface->FillRectangle(rcDot, fore.allocated);
			x += 2;
		}
	} else {	// Either INDIC_PLAIN or unknown
		surface->MoveTo(rc.left, ymid);
		surface->LineTo(rc.right, ymid);
	}
}

