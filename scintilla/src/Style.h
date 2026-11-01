// Scintilla source code edit control
/** @file Style.h
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLE_H
#define STYLE_H

namespace Scintilla::Internal {

struct FontSpecification {
	static constexpr int sensibleFontSize = 10;
	static constexpr int defaultFontSize = 10 * Scintilla::FontSizeMultiplier;
	// fontName is allocated by a ViewStyle container object and may be null
	const char *fontName = nullptr;
	int size = defaultFontSize;
	Scintilla::FontWeight weight = Scintilla::FontWeight::Normal;
	Scintilla::FontStretch stretch = Scintilla::FontStretch::Normal;
	bool italic = false;
	Scintilla::CharacterSet characterSet = Scintilla::CharacterSet::Default;
	Scintilla::FontQuality extraFontFlag = Scintilla::FontQuality::QualityDefault;
	bool checkMonospaced = false;

	constexpr FontSpecification(const char *fontName_, int size_) noexcept :
		fontName(fontName_), size(size_) {
	}
	bool operator==(const FontSpecification &other) const noexcept;
	bool operator<(const FontSpecification &other) const noexcept;
};

struct FontMeasurements {
	XYPOSITION ascent = 1;
	XYPOSITION descent = 1;
	XYPOSITION capitalHeight = 1;	// Top of capital letter to baseline: ascent - internal leading
	XYPOSITION aveCharWidth = 1;
	XYPOSITION monospaceCharacterWidth = 1;
	XYPOSITION spaceWidth = 1;
	bool monospaceASCII = false;
	int sizeZoomed = 2;
};

/**
 */
class Style : public FontSpecification, public FontMeasurements {
public:
	ColourRGBA fore = black;
	ColourRGBA back = white;
	bool eolFilled = false;
	bool underline = false;
	enum class CaseForce {mixed, upper, lower, camel};
	CaseForce caseForce = CaseForce::mixed;
	bool visible = true;
	bool changeable = true;
	bool hotspot = false;
	static constexpr size_t maxRepresentationLength = 5;	// UTF-8 + NUL
	char invisibleRepresentation[maxRepresentationLength] {};

	std::shared_ptr<Font> font;

	explicit Style(const char *fontName_=nullptr) noexcept;
	void Copy(std::shared_ptr<Font> font_, const FontMeasurements &fm_) noexcept;
	[[nodiscard]] bool IsProtected() const noexcept { return !(changeable && visible);}
};

}

#endif
