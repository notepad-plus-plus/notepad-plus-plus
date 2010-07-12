// Scintilla source code edit control
/** @file LineMarker.h
 ** Defines the look of a line marker in the margin .
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LINEMARKER_H
#define LINEMARKER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class LineMarker {
public:
	int markType;
	ColourPair fore;
	ColourPair back;
	int alpha;
	XPM *pxpm;
	LineMarker() {
		markType = SC_MARK_CIRCLE;
		fore = ColourDesired(0,0,0);
		back = ColourDesired(0xff,0xff,0xff);
		alpha = SC_ALPHA_NOALPHA;
		pxpm = NULL;
	}
	LineMarker(const LineMarker &) {
		// Defined to avoid pxpm being blindly copied, not as real copy constructor
		markType = SC_MARK_CIRCLE;
		fore = ColourDesired(0,0,0);
		back = ColourDesired(0xff,0xff,0xff);
		alpha = SC_ALPHA_NOALPHA;
		pxpm = NULL;
	}
	~LineMarker() {
		delete pxpm;
	}
	LineMarker &operator=(const LineMarker &) {
		// Defined to avoid pxpm being blindly copied, not as real assignment operator
		markType = SC_MARK_CIRCLE;
		fore = ColourDesired(0,0,0);
		back = ColourDesired(0xff,0xff,0xff);
		alpha = SC_ALPHA_NOALPHA;
		delete pxpm;
		pxpm = NULL;
		return *this;
	}
	void RefreshColourPalette(Palette &pal, bool want);
	void SetXPM(const char *textForm);
	void SetXPM(const char *const *linesForm);
	void Draw(Surface *surface, PRectangle &rc, Font &fontForCharacter);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
