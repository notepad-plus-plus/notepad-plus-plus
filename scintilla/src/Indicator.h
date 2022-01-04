// Scintilla source code edit control
/** @file Indicator.h
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef INDICATOR_H
#define INDICATOR_H

namespace Scintilla::Internal {

struct StyleAndColour {
	Scintilla::IndicatorStyle style;
	ColourRGBA fore;
	StyleAndColour() noexcept : style(Scintilla::IndicatorStyle::Plain), fore(0, 0, 0) {
	}
	StyleAndColour(Scintilla::IndicatorStyle style_, ColourRGBA fore_ = ColourRGBA(0, 0, 0)) noexcept : style(style_), fore(fore_) {
	}
	bool operator==(const StyleAndColour &other) const noexcept {
		return (style == other.style) && (fore == other.fore);
	}
};

/**
 */
class Indicator {
public:
	enum class State { normal, hover };
	StyleAndColour sacNormal;
	StyleAndColour sacHover;
	bool under;
	int fillAlpha;
	int outlineAlpha;
	Scintilla::IndicFlag attributes;
	XYPOSITION strokeWidth = 1.0f;
	Indicator() noexcept : under(false), fillAlpha(30), outlineAlpha(50), attributes(Scintilla::IndicFlag::None) {
	}
	Indicator(Scintilla::IndicatorStyle style_, ColourRGBA fore_= ColourRGBA(0,0,0), bool under_=false, int fillAlpha_=30, int outlineAlpha_=50) noexcept :
		sacNormal(style_, fore_), sacHover(style_, fore_), under(under_), fillAlpha(fillAlpha_), outlineAlpha(outlineAlpha_), attributes(Scintilla::IndicFlag::None) {
	}
	void Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine, const PRectangle &rcCharacter, State drawState, int value) const;
	bool IsDynamic() const noexcept {
		return !(sacNormal == sacHover);
	}
	bool OverridesTextFore() const noexcept {
		return sacNormal.style == Scintilla::IndicatorStyle::TextFore || sacHover.style == Scintilla::IndicatorStyle::TextFore || sacNormal.style == Scintilla::IndicatorStyle::ExplorerLink || sacHover.style == Scintilla::IndicatorStyle::ExplorerLink;
	}
	Scintilla::IndicFlag Flags() const noexcept {
		return attributes;
	}
	void SetFlags(Scintilla::IndicFlag attributes_) noexcept;
};

}

#endif
