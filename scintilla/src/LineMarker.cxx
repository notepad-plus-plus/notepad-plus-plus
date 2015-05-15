// Scintilla source code edit control
/** @file LineMarker.cxx
 ** Defines the look of a line marker in the margin .
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
#include <math.h>

#include <vector>
#include <map>

#include "Platform.h"

#include "Scintilla.h"
#include "XPM.h"
#include "LineMarker.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

void LineMarker::SetXPM(const char *textForm) {
	delete pxpm;
	pxpm = new XPM(textForm);
	markType = SC_MARK_PIXMAP;
}

void LineMarker::SetXPM(const char *const *linesForm) {
	delete pxpm;
	pxpm = new XPM(linesForm);
	markType = SC_MARK_PIXMAP;
}

void LineMarker::SetRGBAImage(Point sizeRGBAImage, float scale, const unsigned char *pixelsRGBAImage) {
	delete image;
	image = new RGBAImage(sizeRGBAImage.x, sizeRGBAImage.y, scale, pixelsRGBAImage);
	markType = SC_MARK_RGBAIMAGE;
}

static void DrawBox(Surface *surface, int centreX, int centreY, int armSize, ColourDesired fore, ColourDesired back) {
	PRectangle rc;
	rc.left = centreX - armSize;
	rc.top = centreY - armSize;
	rc.right = centreX + armSize + 1;
	rc.bottom = centreY + armSize + 1;
	surface->RectangleDraw(rc, back, fore);
}

static void DrawCircle(Surface *surface, int centreX, int centreY, int armSize, ColourDesired fore, ColourDesired back) {
	PRectangle rcCircle;
	rcCircle.left = centreX - armSize;
	rcCircle.top = centreY - armSize;
	rcCircle.right = centreX + armSize + 1;
	rcCircle.bottom = centreY + armSize + 1;
	surface->Ellipse(rcCircle, back, fore);
}

static void DrawPlus(Surface *surface, int centreX, int centreY, int armSize, ColourDesired fore) {
	PRectangle rcV(centreX, centreY - armSize + 2, centreX + 1, centreY + armSize - 2 + 1);
	surface->FillRectangle(rcV, fore);
	PRectangle rcH(centreX - armSize + 2, centreY, centreX + armSize - 2 + 1, centreY+1);
	surface->FillRectangle(rcH, fore);
}

static void DrawMinus(Surface *surface, int centreX, int centreY, int armSize, ColourDesired fore) {
	PRectangle rcH(centreX - armSize + 2, centreY, centreX + armSize - 2 + 1, centreY+1);
	surface->FillRectangle(rcH, fore);
}

void LineMarker::Draw(Surface *surface, PRectangle &rcWhole, Font &fontForCharacter, typeOfFold tFold, int marginStyle) {
	ColourDesired head = back;
	ColourDesired body = back;
	ColourDesired tail = back;

	switch (tFold) {
	case LineMarker::head :
	case LineMarker::headWithTail :
		head = backSelected;
		tail = backSelected;
		break;
	case LineMarker::body :
		head = backSelected;
		body = backSelected;
		break;
	case LineMarker::tail :
		body = backSelected;
		tail = backSelected;
		break;
	default :
		// LineMarker::undefined
		break;
	}

	if ((markType == SC_MARK_PIXMAP) && (pxpm)) {
		pxpm->Draw(surface, rcWhole);
		return;
	}
	if ((markType == SC_MARK_RGBAIMAGE) && (image)) {
		// Make rectangle just large enough to fit image centred on centre of rcWhole
		PRectangle rcImage;
		rcImage.top = static_cast<int>(((rcWhole.top + rcWhole.bottom) - image->GetScaledHeight()) / 2);
		rcImage.bottom = rcImage.top + image->GetScaledHeight();
		rcImage.left = static_cast<int>(((rcWhole.left + rcWhole.right) - image->GetScaledWidth()) / 2);
		rcImage.right = rcImage.left + image->GetScaledWidth();
		surface->DrawRGBAImage(rcImage, image->GetWidth(), image->GetHeight(), image->Pixels());
		return;
	}
	// Restrict most shapes a bit
	PRectangle rc = rcWhole;
	rc.top++;
	rc.bottom--;
	int minDim = Platform::Minimum(rc.Width(), rc.Height());
	minDim--;	// Ensure does not go beyond edge
	int centreX = floor((rc.right + rc.left) / 2.0);
	int centreY = floor((rc.bottom + rc.top) / 2.0);
	int dimOn2 = minDim / 2;
	int dimOn4 = minDim / 4;
	int blobSize = dimOn2-1;
	int armSize = dimOn2-2;
	if (marginStyle == SC_MARGIN_NUMBER || marginStyle == SC_MARGIN_TEXT || marginStyle == SC_MARGIN_RTEXT) {
		// On textual margins move marker to the left to try to avoid overlapping the text
		centreX = rc.left + dimOn2 + 1;
	}
	if (markType == SC_MARK_ROUNDRECT) {
		PRectangle rcRounded = rc;
		rcRounded.left = rc.left + 1;
		rcRounded.right = rc.right - 1;
		surface->RoundedRectangle(rcRounded, fore, back);
	} else if (markType == SC_MARK_CIRCLE) {
		PRectangle rcCircle;
		rcCircle.left = centreX - dimOn2;
		rcCircle.top = centreY - dimOn2;
		rcCircle.right = centreX + dimOn2;
		rcCircle.bottom = centreY + dimOn2;
		surface->Ellipse(rcCircle, fore, back);
	} else if (markType == SC_MARK_ARROW) {
		Point pts[] = {
    		Point(centreX - dimOn4, centreY - dimOn2),
    		Point(centreX - dimOn4, centreY + dimOn2),
    		Point(centreX + dimOn2 - dimOn4, centreY),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore, back);

	} else if (markType == SC_MARK_ARROWDOWN) {
		Point pts[] = {
    		Point(centreX - dimOn2, centreY - dimOn4),
    		Point(centreX + dimOn2, centreY - dimOn4),
    		Point(centreX, centreY + dimOn2 - dimOn4),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore, back);

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
                 		fore, back);

	} else if (markType == SC_MARK_MINUS) {
		Point pts[] = {
    		Point(centreX - armSize, centreY - 1),
    		Point(centreX + armSize, centreY -1),
    		Point(centreX + armSize, centreY +1),
    		Point(centreX - armSize, centreY + 1),
		};
		surface->Polygon(pts, sizeof(pts) / sizeof(pts[0]),
                 		fore, back);

	} else if (markType == SC_MARK_SMALLRECT) {
		PRectangle rcSmall;
		rcSmall.left = rc.left + 1;
		rcSmall.top = rc.top + 2;
		rcSmall.right = rc.right - 1;
		rcSmall.bottom = rc.bottom - 2;
		surface->RectangleDraw(rcSmall, fore, back);

	} else if (markType == SC_MARK_EMPTY || markType == SC_MARK_BACKGROUND ||
		markType == SC_MARK_UNDERLINE || markType == SC_MARK_AVAILABLE) {
		// An invisible marker so don't draw anything

	} else if (markType == SC_MARK_VLINE) {
		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_LCORNER) {
		surface->PenColour(tail);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY);
		surface->LineTo(rc.right - 1, centreY);

	} else if (markType == SC_MARK_TCORNER) {
		surface->PenColour(tail);
		surface->MoveTo(centreX, centreY);
		surface->LineTo(rc.right - 1, centreY);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY + 1);

		surface->PenColour(head);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_LCORNERCURVE) {
		surface->PenColour(tail);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY-3);
		surface->LineTo(centreX+3, centreY);
		surface->LineTo(rc.right - 1, centreY);

	} else if (markType == SC_MARK_TCORNERCURVE) {
		surface->PenColour(tail);
		surface->MoveTo(centreX, centreY-3);
		surface->LineTo(centreX+3, centreY);
		surface->LineTo(rc.right - 1, centreY);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY-2);

		surface->PenColour(head);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_BOXPLUS) {
		DrawBox(surface, centreX, centreY, blobSize, fore, head);
		DrawPlus(surface, centreX, centreY, blobSize, tail);

	} else if (markType == SC_MARK_BOXPLUSCONNECTED) {
		if (tFold == LineMarker::headWithTail)
			surface->PenColour(tail);
		else
			surface->PenColour(body);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

		DrawBox(surface, centreX, centreY, blobSize, fore, head);
		DrawPlus(surface, centreX, centreY, blobSize, tail);

		if (tFold == LineMarker::body) {
			surface->PenColour(tail);
			surface->MoveTo(centreX + 1, centreY + blobSize);
			surface->LineTo(centreX + blobSize + 1, centreY + blobSize);

			surface->MoveTo(centreX + blobSize, centreY + blobSize);
			surface->LineTo(centreX + blobSize, centreY - blobSize);

			surface->MoveTo(centreX + 1, centreY - blobSize);
			surface->LineTo(centreX + blobSize + 1, centreY - blobSize);
		}
	} else if (markType == SC_MARK_BOXMINUS) {
		DrawBox(surface, centreX, centreY, blobSize, fore, head);
		DrawMinus(surface, centreX, centreY, blobSize, tail);

		surface->PenColour(head);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_BOXMINUSCONNECTED) {
		DrawBox(surface, centreX, centreY, blobSize, fore, head);
		DrawMinus(surface, centreX, centreY, blobSize, tail);

		surface->PenColour(head);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

		if (tFold == LineMarker::body) {
			surface->PenColour(tail);
			surface->MoveTo(centreX + 1, centreY + blobSize);
			surface->LineTo(centreX + blobSize + 1, centreY + blobSize);

			surface->MoveTo(centreX + blobSize, centreY + blobSize);
			surface->LineTo(centreX + blobSize, centreY - blobSize);

			surface->MoveTo(centreX + 1, centreY - blobSize);
			surface->LineTo(centreX + blobSize + 1, centreY - blobSize);
		}
	} else if (markType == SC_MARK_CIRCLEPLUS) {
		DrawCircle(surface, centreX, centreY, blobSize, fore, head);
		DrawPlus(surface, centreX, centreY, blobSize, tail);

	} else if (markType == SC_MARK_CIRCLEPLUSCONNECTED) {
		if (tFold == LineMarker::headWithTail)
			surface->PenColour(tail);
		else
			surface->PenColour(body);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

		DrawCircle(surface, centreX, centreY, blobSize, fore, head);
		DrawPlus(surface, centreX, centreY, blobSize, tail);

	} else if (markType == SC_MARK_CIRCLEMINUS) {
		DrawCircle(surface, centreX, centreY, blobSize, fore, head);
		DrawMinus(surface, centreX, centreY, blobSize, tail);

		surface->PenColour(head);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

	} else if (markType == SC_MARK_CIRCLEMINUSCONNECTED) {
		DrawCircle(surface, centreX, centreY, blobSize, fore, head);
		DrawMinus(surface, centreX, centreY, blobSize, tail);

		surface->PenColour(head);
		surface->MoveTo(centreX, centreY + blobSize);
		surface->LineTo(centreX, rcWhole.bottom);

		surface->PenColour(body);
		surface->MoveTo(centreX, rcWhole.top);
		surface->LineTo(centreX, centreY - blobSize);

	} else if (markType >= SC_MARK_CHARACTER) {
		char character[1];
		character[0] = static_cast<char>(markType - SC_MARK_CHARACTER);
		XYPOSITION width = surface->WidthText(fontForCharacter, character, 1);
		rc.left += (rc.Width() - width) / 2;
		rc.right = rc.left + width;
		surface->DrawTextClipped(rc, fontForCharacter, rc.bottom - 2,
			character, 1, fore, back);

	} else if (markType == SC_MARK_DOTDOTDOT) {
		int right = centreX - 6;
		for (int b=0; b<3; b++) {
			PRectangle rcBlob(right, rc.bottom - 4, right + 2, rc.bottom-2);
			surface->FillRectangle(rcBlob, fore);
			right += 5;
		}
	} else if (markType == SC_MARK_ARROWS) {
		surface->PenColour(fore);
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
				fore, back);
	} else if (markType == SC_MARK_LEFTRECT) {
		PRectangle rcLeft = rcWhole;
		rcLeft.right = rcLeft.left + 4;
		surface->FillRectangle(rcLeft, back);
	} else { // SC_MARK_FULLRECT
		surface->FillRectangle(rcWhole, back);
	}
}
