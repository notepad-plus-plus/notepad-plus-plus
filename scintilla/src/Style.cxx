// Scintilla source code edit control
/** @file Style.cxx
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

#include "Platform.h"

#include "Scintilla.h"
#include "Style.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

FontAlias::FontAlias() {
}

FontAlias::~FontAlias() {
	SetID(0);
	// ~Font will not release the actual font resource sine it is now 0
}

void FontAlias::MakeAlias(Font &fontOrigin) {
	SetID(fontOrigin.GetID());
}

void FontAlias::ClearFont() {
	SetID(0);
}

bool FontSpecification::EqualTo(const FontSpecification &other) const {
	return bold == other.bold &&
	       italic == other.italic &&
	       size == other.size &&
	       characterSet == other.characterSet &&
	       fontName == other.fontName;
}

FontMeasurements::FontMeasurements() {
	Clear();
}

void FontMeasurements::Clear() {
	lineHeight = 2;
	ascent = 1;
	descent = 1;
	externalLeading = 0;
	aveCharWidth = 1;
	spaceWidth = 1;
	sizeZoomed = 2;
}

Style::Style() : FontSpecification() {
	Clear(ColourDesired(0, 0, 0), ColourDesired(0xff, 0xff, 0xff),
	      Platform::DefaultFontSize(), 0, SC_CHARSET_DEFAULT,
	      false, false, false, false, caseMixed, true, true, false);
}

Style::Style(const Style &source) : FontSpecification(), FontMeasurements() {
	Clear(ColourDesired(0, 0, 0), ColourDesired(0xff, 0xff, 0xff),
	      0, 0, 0,
	      false, false, false, false, caseMixed, true, true, false);
	fore.desired = source.fore.desired;
	back.desired = source.back.desired;
	characterSet = source.characterSet;
	bold = source.bold;
	italic = source.italic;
	size = source.size;
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
	      0, 0, SC_CHARSET_DEFAULT,
	      false, false, false, false, caseMixed, true, true, false);
	fore.desired = source.fore.desired;
	back.desired = source.back.desired;
	characterSet = source.characterSet;
	bold = source.bold;
	italic = source.italic;
	size = source.size;
	eolFilled = source.eolFilled;
	underline = source.underline;
	caseForce = source.caseForce;
	visible = source.visible;
	changeable = source.changeable;
	return *this;
}

void Style::Clear(ColourDesired fore_, ColourDesired back_, int size_,
        const char *fontName_, int characterSet_,
        bool bold_, bool italic_, bool eolFilled_,
        bool underline_, ecaseForced caseForce_,
        bool visible_, bool changeable_, bool hotspot_) {
	fore.desired = fore_;
	back.desired = back_;
	characterSet = characterSet_;
	bold = bold_;
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
	FontMeasurements::Clear();
}

void Style::ClearTo(const Style &source) {
	Clear(
	    source.fore.desired,
	    source.back.desired,
	    source.size,
	    source.fontName,
	    source.characterSet,
	    source.bold,
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
#if PLAT_WX
	font.SetAscent(fm_.ascent);
#endif
	(FontMeasurements &)(*this) = fm_;
}
