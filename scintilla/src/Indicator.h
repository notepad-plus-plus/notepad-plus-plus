// Scintilla source code edit control
/** @file Indicator.h
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef INDICATOR_H
#define INDICATOR_H

namespace Scintilla {

struct StyleAndColour {
	int style;
	ColourDesired fore;
	StyleAndColour() noexcept : style(INDIC_PLAIN), fore(0, 0, 0) {
	}
	StyleAndColour(int style_, ColourDesired fore_ = ColourDesired(0, 0, 0)) noexcept : style(style_), fore(fore_) {
	}
	bool operator==(const StyleAndColour &other) const noexcept {
		return (style == other.style) && (fore == other.fore);
	}
};

/**
 */
class Indicator {
public:
	enum DrawState { drawNormal, drawHover };
	StyleAndColour sacNormal;
	StyleAndColour sacHover;
	bool under;
	int fillAlpha;
	int outlineAlpha;
	int attributes;
	Indicator() noexcept : under(false), fillAlpha(30), outlineAlpha(50), attributes(0) {
	}
	Indicator(int style_, ColourDesired fore_=ColourDesired(0,0,0), bool under_=false, int fillAlpha_=30, int outlineAlpha_=50) noexcept :
		sacNormal(style_, fore_), sacHover(style_, fore_), under(under_), fillAlpha(fillAlpha_), outlineAlpha(outlineAlpha_), attributes(0) {
	}
	void Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine, const PRectangle &rcCharacter, DrawState drawState, int value) const;
	bool IsDynamic() const noexcept {
		return !(sacNormal == sacHover);
	}
	bool OverridesTextFore() const noexcept {
		return sacNormal.style == INDIC_TEXTFORE || sacHover.style == INDIC_TEXTFORE;
	}
	int Flags() const noexcept {
		return attributes;
	}
	void SetFlags(int attributes_);
};

}

#endif
