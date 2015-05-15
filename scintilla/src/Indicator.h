// Scintilla source code edit control
/** @file Indicator.h
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef INDICATOR_H
#define INDICATOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class Indicator {
public:
	int style;
	bool under;
	ColourDesired fore;
	int fillAlpha;
	int outlineAlpha;
	Indicator() : style(INDIC_PLAIN), under(false), fore(ColourDesired(0,0,0)), fillAlpha(30), outlineAlpha(50) {
	}
	void Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
