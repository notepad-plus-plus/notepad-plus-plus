// Scintilla source code edit control
/** @file Style.cxx
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdexcept>
#include <string_view>
#include <vector>
#include <memory>

#include "Platform.h"

#include "Scintilla.h"
#include "Style.h"

using namespace Scintilla;

FontAlias::FontAlias() noexcept {
}

FontAlias::FontAlias(const FontAlias &other) noexcept : Font() {
	SetID(other.fid);
}

FontAlias::~FontAlias() {
	SetID(FontID{});
	// ~Font will not release the actual font resource since it is now 0
}

void FontAlias::MakeAlias(const Font &fontOrigin) noexcept {
	SetID(fontOrigin.GetID());
}

void FontAlias::ClearFont() noexcept {
	SetID(FontID{});
}

bool FontSpecification::operator==(const FontSpecification &other) const noexcept {
	return fontName == other.fontName &&
	       weight == other.weight &&
	       italic == other.italic &&
	       size == other.size &&
	       characterSet == other.characterSet &&
	       extraFontFlag == other.extraFontFlag;
}

bool FontSpecification::operator<(const FontSpecification &other) const noexcept {
	if (fontName != other.fontName)
		return fontName < other.fontName;
	if (weight != other.weight)
		return weight < other.weight;
	if (italic != other.italic)
		return italic == false;
	if (size != other.size)
		return size < other.size;
	if (characterSet != other.characterSet)
		return characterSet < other.characterSet;
	if (extraFontFlag != other.extraFontFlag)
		return extraFontFlag < other.extraFontFlag;
	return false;
}

FontMeasurements::FontMeasurements() noexcept {
	ClearMeasurements();
}

void FontMeasurements::ClearMeasurements() noexcept {
	ascent = 1;
	descent = 1;
	capitalHeight = 1;
	aveCharWidth = 1;
	spaceWidth = 1;
	sizeZoomed = 2;
}

Style::Style() : FontSpecification() {
	Clear(ColourDesired(0, 0, 0), ColourDesired(0xff, 0xff, 0xff),
	      Platform::DefaultFontSize() * SC_FONT_SIZE_MULTIPLIER, nullptr, SC_CHARSET_DEFAULT,
	      SC_WEIGHT_NORMAL, false, false, false, caseMixed, true, true, false);
}

Style::Style(const Style &source) : FontSpecification(), FontMeasurements() {
	Clear(ColourDesired(0, 0, 0), ColourDesired(0xff, 0xff, 0xff),
	      0, nullptr, 0,
	      SC_WEIGHT_NORMAL, false, false, false, caseMixed, true, true, false);
	fore = source.fore;
	back = source.back;
	characterSet = source.characterSet;
	weight = source.weight;
	italic = source.italic;
	size = source.size;
	fontName = source.fontName;
	eolFilled = source.eolFilled;
	underline = source.underline;
	caseForce = source.caseForce;
	visible = source.visible;
	changeable = source.changeable;
	hotspot = source.hotspot;
}

Style::~Style() {
}

Style &Style::operator=(const Style &source) {
	if (this == &source)
		return * this;
	Clear(ColourDesired(0, 0, 0), ColourDesired(0xff, 0xff, 0xff),
	      0, nullptr, SC_CHARSET_DEFAULT,
	      SC_WEIGHT_NORMAL, false, false, false, caseMixed, true, true, false);
	fore = source.fore;
	back = source.back;
	characterSet = source.characterSet;
	weight = source.weight;
	italic = source.italic;
	size = source.size;
	fontName = source.fontName;
	eolFilled = source.eolFilled;
	underline = source.underline;
	caseForce = source.caseForce;
	visible = source.visible;
	changeable = source.changeable;
	return *this;
}

void Style::Clear(ColourDesired fore_, ColourDesired back_, int size_,
        const char *fontName_, int characterSet_,
        int weight_, bool italic_, bool eolFilled_,
        bool underline_, ecaseForced caseForce_,
        bool visible_, bool changeable_, bool hotspot_) {
	fore = fore_;
	back = back_;
	characterSet = characterSet_;
	weight = weight_;
	italic = italic_;
	size = size_;
	fontName = fontName_;
	eolFilled = eolFilled_;
	underline = underline_;
	caseForce = caseForce_;
	visible = visible_;
	changeable = changeable_;
	hotspot = hotspot_;
	font.ClearFont();
	FontMeasurements::ClearMeasurements();
}

void Style::ClearTo(const Style &source) {
	Clear(
	    source.fore,
	    source.back,
	    source.size,
	    source.fontName,
	    source.characterSet,
	    source.weight,
	    source.italic,
	    source.eolFilled,
	    source.underline,
	    source.caseForce,
	    source.visible,
	    source.changeable,
	    source.hotspot);
}

void Style::Copy(Font &font_, const FontMeasurements &fm_) {
	font.MakeAlias(font_);
	(FontMeasurements &)(*this) = fm_;
}
