// Scintilla source code edit control
/** @file Style.h
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLE_H
#define STYLE_H

namespace Scintilla {

struct FontSpecification {
	const char *fontName;
	int weight;
	bool italic;
	int size;
	int characterSet;
	int extraFontFlag;
	FontSpecification() noexcept :
		fontName(nullptr),
		weight(SC_WEIGHT_NORMAL),
		italic(false),
		size(10 * SC_FONT_SIZE_MULTIPLIER),
		characterSet(0),
		extraFontFlag(0) {
	}
	bool operator==(const FontSpecification &other) const noexcept;
	bool operator<(const FontSpecification &other) const noexcept;
};

// Just like Font but only has a copy of the FontID so should not delete it
class FontAlias : public Font {
public:
	FontAlias() noexcept;
	// FontAlias objects can not be assigned except for initialization
	FontAlias(const FontAlias &) noexcept;
	FontAlias(FontAlias &&)  = delete;
	FontAlias &operator=(const FontAlias &) = delete;
	FontAlias &operator=(FontAlias &&) = delete;
	~FontAlias() override;
	void MakeAlias(const Font &fontOrigin) noexcept;
	void ClearFont() noexcept;
};

struct FontMeasurements {
	unsigned int ascent;
	unsigned int descent;
	XYPOSITION capitalHeight;	// Top of capital letter to baseline: ascent - internal leading
	XYPOSITION aveCharWidth;
	XYPOSITION spaceWidth;
	int sizeZoomed;
	FontMeasurements() noexcept;
	void ClearMeasurements() noexcept;
};

/**
 */
class Style : public FontSpecification, public FontMeasurements {
public:
	ColourDesired fore;
	ColourDesired back;
	bool eolFilled;
	bool underline;
	enum ecaseForced {caseMixed, caseUpper, caseLower, caseCamel};
	ecaseForced caseForce;
	bool visible;
	bool changeable;
	bool hotspot;

	FontAlias font;

	Style();
	Style(const Style &source);
	Style(Style &&) = delete;
	~Style();
	Style &operator=(const Style &source);
	Style &operator=(Style &&) = delete;
	void Clear(ColourDesired fore_, ColourDesired back_,
	           int size_,
	           const char *fontName_, int characterSet_,
	           int weight_, bool italic_, bool eolFilled_,
	           bool underline_, ecaseForced caseForce_,
	           bool visible_, bool changeable_, bool hotspot_);
	void ClearTo(const Style &source);
	void Copy(Font &font_, const FontMeasurements &fm_);
	bool IsProtected() const noexcept { return !(changeable && visible);}
};

}

#endif
