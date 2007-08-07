// Scintilla source code edit control
/** @file Style.h
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLE_H
#define STYLE_H

/**
 */
class Style {
public:
	ColourPair fore;
	ColourPair back;
	bool aliasOfDefaultFont;
	bool bold;
	bool italic;
	int size;
	const char *fontName;
	int characterSet;
	bool eolFilled;
	bool underline;
	enum ecaseForced {caseMixed, caseUpper, caseLower};
	ecaseForced caseForce;
	bool visible;
	bool changeable;
	bool hotspot;

	Font font;
	int sizeZoomed;
	unsigned int lineHeight;
	unsigned int ascent;
	unsigned int descent;
	unsigned int externalLeading;
	unsigned int aveCharWidth;
	unsigned int spaceWidth;

	Style();
	Style(const Style &source);
	~Style();
	Style &operator=(const Style &source);
	void Clear(ColourDesired fore_, ColourDesired back_,
	           int size_,
	           const char *fontName_, int characterSet_,
	           bool bold_, bool italic_, bool eolFilled_,
	           bool underline_, ecaseForced caseForce_,
		   bool visible_, bool changeable_, bool hotspot_);
	void ClearTo(const Style &source);
	bool EquivalentFontTo(const Style *other) const;
	void Realise(Surface &surface, int zoomLevel, Style *defaultStyle = 0, bool extraFontFlag = false);
	bool IsProtected() const { return !(changeable && visible);};
};

#endif
