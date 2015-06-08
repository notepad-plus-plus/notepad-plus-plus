// Scintilla source code edit control
/** @file LineMarker.h
 ** Defines the look of a line marker in the margin .
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LINEMARKER_H
#define LINEMARKER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

typedef void (*DrawLineMarkerFn)(Surface *surface, PRectangle &rcWhole, Font &fontForCharacter, int tFold, int marginStyle, const void *lineMarker);

/**
 */
class LineMarker {
public:
	enum typeOfFold { undefined, head, body, tail, headWithTail };

	int markType;
	ColourDesired fore;
	ColourDesired back;
	ColourDesired backSelected;
	int alpha;
	XPM *pxpm;
	RGBAImage *image;
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * Draw function for drawing line markers. Allow those platforms to override
	 * it instead of creating a new method(s) in the Surface class that existing
	 * platforms must implement as empty. */
	DrawLineMarkerFn customDraw;
	LineMarker() {
		markType = SC_MARK_CIRCLE;
		fore = ColourDesired(0,0,0);
		back = ColourDesired(0xff,0xff,0xff);
		backSelected = ColourDesired(0xff,0x00,0x00);
		alpha = SC_ALPHA_NOALPHA;
		pxpm = NULL;
		image = NULL;
		customDraw = NULL;
	}
	LineMarker(const LineMarker &) {
		// Defined to avoid pxpm being blindly copied, not as a complete copy constructor
		markType = SC_MARK_CIRCLE;
		fore = ColourDesired(0,0,0);
		back = ColourDesired(0xff,0xff,0xff);
		backSelected = ColourDesired(0xff,0x00,0x00);
		alpha = SC_ALPHA_NOALPHA;
		pxpm = NULL;
		image = NULL;
		customDraw = NULL;
	}
	~LineMarker() {
		delete pxpm;
		delete image;
	}
	LineMarker &operator=(const LineMarker &other) {
		// Defined to avoid pxpm being blindly copied, not as a complete assignment operator
		if (this != &other) {
			markType = SC_MARK_CIRCLE;
			fore = ColourDesired(0,0,0);
			back = ColourDesired(0xff,0xff,0xff);
			backSelected = ColourDesired(0xff,0x00,0x00);
			alpha = SC_ALPHA_NOALPHA;
			delete pxpm;
			pxpm = NULL;
			delete image;
			image = NULL;
			customDraw = NULL;
		}
		return *this;
	}
	void SetXPM(const char *textForm);
	void SetXPM(const char *const *linesForm);
	void SetRGBAImage(Point sizeRGBAImage, float scale, const unsigned char *pixelsRGBAImage);
	void Draw(Surface *surface, PRectangle &rc, Font &fontForCharacter, typeOfFold tFold, int marginStyle) const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
