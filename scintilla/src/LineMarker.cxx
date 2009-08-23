// Scintilla source code edit control
/** @file LineMarker.cxx
 ** Defines the look of a line marker in the margin .
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

#include "Platform.h"

#include "Scintilla.h"
#include "XPM.h"
#include "LineMarker.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

void LineMarker::RefreshColourPalette(Palette &pal, bool want) {
	pal.WantFind(fore, want);
	pal.WantFind(back, want);
	if (pxpm) {
		pxpm->RefreshColourPalette(pal, want);
	}
}

void LineMarker::SetXPM(const char *textForm) {
	delete pxpm;
	pxpm = new XPM(textForm);
	markType = SC_MARK_PIXMAP;
}

void LineMarker::SetXPM(const char * const *linesForm) {
	delete pxpm;
	pxpm = new XPM(linesForm);
	markType = SC_MARK_PIXMAP;
}

static void DrawBox(Surface *surface, int centreX, int centreY, int armSize, ColourAllocated fore, ColourAllocated back) {
	PRectangle rc;
	rc.left = centreX - armSize;
	rc.top = centreY - armSize;
	rc.right = centreX + armSize + 1;
	rc.bottom = centreY + armSize + 1;
	surface->RectangleDraw(rc, back, fore);
}

static void DrawCircle(Surface *surface, int centreX, int centreY, int armSize, ColourAllocated fore, ColourAllocated back) {
	PRectangle rcCircle;
	rcCircle.left = centreX - armSize;
	rcCircle.top = centreY - armSize;
	rcCircle.right = centreX + armSize + 1;
	rcCircle.bottom = centreY + armSize + 1;
	surface->Ellipse(rcCircle, back, fore);
}

static void DrawPlus(Surface *surface, int centreX, int centreY, int armSize, ColourAllocated fore) {
	PRectangle rcV(centreX, centreY - armSize + 2, centreX + 1, centreY + armSize - 2 + 1);
	surface->FillRectangle(rcV, fore);
	PRectangle rcH(centreX - armSize + 2, centreY, centreX + armSize - 2 + 1, centreY+1);
	surface->FillRectangle(rcH, fore);
}

static void DrawMinus(Surface *surface, int centreX, int centreY, int armSize, ColourAllocated fore) {
	PRectangle rcH(centreX - armSize + 2, centreY, centreX + armSize - 2 + 1, centreY+1);
	surface->FillRectangle(rcH, fore);
}

void LineMarker::Draw(Surface *surface, PRectangle &rcWhole, Font &fontForCharacter) {
	if ((markType == SC_MARK_PIXMAP) && (pxpm)) {
		pxpm->Draw(surface, rcWhole);
		return;
	}
	// Restrict most shapes a bit
	PRectangle rc = rcWhole;
	rc.top++;
	rc.bottom--;
	int minDim = Platform::Minimum(rc.Width(), rc.Height());
	minDim--;	// Ensure does not go beyond edge
	int centreX = (rc.right + rc.left) / 2;
	int centreY = (rc.bottom + rc.top) / 2;
	int dimOn2 = minDim / 2;
	int dimOn4 = minDim / 4;
	int blobSize = dimOn2-1;
	int armSize = dimOn2-2;
	if (rc.Width() > (rc.Height() * 2)) {
		// Wide column is line number so move to left to try to avoid overlapping number
		centreX = rc.left + dimOn2 + 1;
	}
	if (markType == SC_MARK_ROUNDRECT) {
		PRectangle rcRounded = rc;
		rcRounded.left = rc.left + 1;
		rcRounded.right = rc.right - 1;
		surface->RoundedRectangle(rcRounded, fore.allocated, back.allocated);
	} else if (markType == SC_MARK_CIRCLE) {
		PRectangle rcCircle;
		rcCircle.left = centreX - dimOn2;
		rcCircle.top = centreY - dimOn2;
		rcCircle.right = centreX + dimOn2;
		rcCircle.bottom = centreY + dimOn2;
		surface->Ellipse(rcCircle, fore.allocated, back.allocated);
	} else if (markType == SC_MARK_ARROW) {
		Point pts[] = {
    		Point(centreX - dimOn4, centreY - dimOn2),
    		Point(centreX - dimOn4, centreY + dimOn2),
    		Point(centreX + dimOn2 - dimOn4, centreY),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore.allocated, back.allocated);

	} else if (markType == SC_MARK_ARROWDOWN) {
		Point pts[] = {
    		Point(centreX - dimOn2, centreY - dimOn4),
    		Point(centreX + dimOn2, centreY - dimOn4),
    		Point(centreX, centreY + dimOn2 - dimOn4),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore.allocated, back.allocated);

	} else if (markType == SC_MARK_PLUS) {
		Point pts[] = {
    		Point(centreX - armSize, centreY - 1),
    		Point(centreX - 1, centreY - 1),
    		Point(centreX - 1, centreY - armSize),
    		Point(centreX + 1, centreY - armSize),
    		Point(centreX + 1, centreY - 1),
    		Point(centreX + armSize, centreY -1),
    		Point(centreX + armSize, centreY +1),
    		Point(centreX + 1, centreY + 1),
    		Point(centreX + 1, centreY + armSize),
    		Point(centreX - 1, centreY + armSize),
    		Point(centreX - 1, centreY + 1),
    		Point(centreX - armSize, centreY + 1),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore.allocated, back.allocated);

	} else if (markType == SC_MARK_MINUS) {
		Point pts[] = {
    		Point(centreX - armSize, centreY - 1),
    		Point(centreX + armSize, centreY -1),
    		Point(centreX + armSize, centreY +1),
    		Point(centreX - armSize, centreY + 1),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore.allocated, back.allocated);

	} else if (markType == SC_MARK_SMALLRECT) {
		PRectangle rcSmall;
		rcSmall.left = rc.left + 1;
		rcSmall.top = rc.top + 2;
		rcSmall.right = rc.right - 1;
		rcSmall.bottom = rc.bottom - 2;
		surface->RectangleDraw(rcSmall, fore.allocated, back.allocated);

	} else if (markType == SC_MARK_EMPTY || markType == SC_MARK_BACKGROUND || 
		markType == SC_MARK_UNDERLINE || markType == SC_MARK_AVAILABLE) {
		// An invisible marker so don't draw anything

	} else if (markType == SC_MARK_VLINE) {
		surface->PenColour(back.allocated);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_LCORNER) {
		surface->PenColour(back.allocated);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rc.top + dimOn2);
		surface->LineTo(rc.right - 2, rc.top + dimOn2);

	} else if (markType == SC_MARK_TCORNER) {
		surface->PenColour(back.allocated);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rcWhole.bottom);
		surface->MoveTo(centreX, rc.top + dimOn2);
		surface->LineTo(rc.right - 2, rc.top + dimOn2);

	} else if (markType == SC_MARK_LCORNERCURVE) {
		surface->PenColour(back.allocated);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rc.top + dimOn2-3);
		surface->LineTo(centreX+3, rc.top + dimOn2);
		surface->LineTo(rc.right - 1, rc.top + dimOn2);

	} else if (markType == SC_MARK_TCORNERCURVE) {
		surface->PenColour(back.allocated);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->MoveTo(centreX, rc.top + dimOn2-3);
		surface->LineTo(centreX+3, rc.top + dimOn2);
		surface->LineTo(rc.right - 1, rc.top + dimOn2);

	} else if (markType == SC_MARK_BOXPLUS) {
		surface->PenColour(back.allocated);
		DrawBox(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		DrawPlus(surface, centreX, centreY, blobSize, back.allocated);

	} else if (markType == SC_MARK_BOXPLUSCONNECTED) {
		surface->PenColour(back.allocated);
		DrawBox(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		DrawPlus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

	} else if (markType == SC_MARK_BOXMINUS) {
		surface->PenColour(back.allocated);
		DrawBox(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		DrawMinus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_BOXMINUSCONNECTED) {
		surface->PenColour(back.allocated);
		DrawBox(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		DrawMinus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

	} else if (markType == SC_MARK_CIRCLEPLUS) {
		DrawCircle(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		surface->PenColour(back.allocated);
		DrawPlus(surface, centreX, centreY, blobSize, back.allocated);

	} else if (markType == SC_MARK_CIRCLEPLUSCONNECTED) {
		DrawCircle(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		surface->PenColour(back.allocated);
		DrawPlus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

	} else if (markType == SC_MARK_CIRCLEMINUS) {
		DrawCircle(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		surface->PenColour(back.allocated);
		DrawMinus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_CIRCLEMINUSCONNECTED) {
		DrawCircle(surface, centreX, centreY, blobSize, fore.allocated, back.allocated);
		surface->PenColour(back.allocated);
		DrawMinus(surface, centreX, centreY, blobSize, back.allocated);

		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

	} else if (markType >= SC_MARK_CHARACTER) {
		char character[1];
		character[0] = static_cast<char>(markType - SC_MARK_CHARACTER);
		int width = surface->WidthText(fontForCharacter, character, 1);
		rc.left += (rc.Width() - width) / 2;
		rc.right = rc.left + width;
		surface->DrawTextClipped(rc, fontForCharacter, rc.bottom - 2,
			character, 1, fore.allocated, back.allocated);

	} else if (markType == SC_MARK_DOTDOTDOT) {
		int right = centreX - 6;
		for (int b=0; b<3; b++) {
			PRectangle rcBlob(right, rc.bottom - 4, right + 2, rc.bottom-2);
			surface->FillRectangle(rcBlob, fore.allocated);
			right += 5;
		}
	} else if (markType == SC_MARK_ARROWS) {
		surface->PenColour(fore.allocated);
		int right = centreX - 2;
		for (int b=0; b<3; b++) {
			surface->MoveTo(right - 4, centreY - 4);
			surface->LineTo(right, centreY);
			surface->LineTo(right - 5, centreY + 5);
			right += 4;
		}
	} else if (markType == SC_MARK_SHORTARROW) {
		Point pts[] = {
			Point(centreX, centreY + dimOn2),
			Point(centreX + dimOn2, centreY),
			Point(centreX, centreY - dimOn2),
			Point(centreX, centreY - dimOn4),
			Point(centreX - dimOn4, centreY - dimOn4),
			Point(centreX - dimOn4, centreY + dimOn4),
			Point(centreX, centreY + dimOn4),
			Point(centreX, centreY + dimOn2),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
				fore.allocated, back.allocated);
	} else if (markType == SC_MARK_LEFTRECT) {
		PRectangle rcLeft = rcWhole;
		rcLeft.right = rcLeft.left + 4;
		surface->FillRectangle(rcLeft, back.allocated);
	} else { // SC_MARK_FULLRECT
		surface->FillRectangle(rcWhole, back.allocated);
	}
}
