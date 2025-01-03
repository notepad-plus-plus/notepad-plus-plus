// Scintilla source code edit control
/** @file Style.cxx
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "ScintillaTypes.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Style.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

bool FontSpecification::operator==(const FontSpecification &other) const noexcept {
	return fontName == other.fontName &&
	       weight == other.weight &&
	       italic == other.italic &&
	       size == other.size &&
	       stretch == other.stretch &&
	       characterSet == other.characterSet &&
	       extraFontFlag == other.extraFontFlag &&
	       checkMonospaced == other.checkMonospaced;
}

bool FontSpecification::operator<(const FontSpecification &other) const noexcept {
	if (fontName != other.fontName)
		return fontName < other.fontName;
	if (weight != other.weight)
		return weight < other.weight;
	if (italic != other.italic)
		return !italic;
	if (size != other.size)
		return size < other.size;
	if (stretch != other.stretch)
		return stretch < other.stretch;
	if (characterSet != other.characterSet)
		return characterSet < other.characterSet;
	if (extraFontFlag != other.extraFontFlag)
		return extraFontFlag < other.extraFontFlag;
	if (checkMonospaced != other.checkMonospaced)
		return checkMonospaced < other.checkMonospaced;
	return false;
}

namespace {

// noexcept Platform::DefaultFontSize
int DefaultFontSize() noexcept {
	try {
		return Platform::DefaultFontSize();
	} catch (...) {
		return 10;
	}
}

}

Style::Style(const char *fontName_) noexcept :
	FontSpecification(fontName_, DefaultFontSize() * FontSizeMultiplier),
	fore(black),
	back(white),
	eolFilled(false),
	underline(false),
	caseForce(CaseForce::mixed),
	visible(true),
	changeable(true),
	hotspot(false),
	invisibleRepresentation{} {
}

void Style::Copy(std::shared_ptr<Font> font_, const FontMeasurements &fm_) noexcept {
	font = std::move(font_);
	(FontMeasurements &)(*this) = fm_;
}
