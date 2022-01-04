// Scintilla source code edit control
/** @file ViewStyle.h
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef VIEWSTYLE_H
#define VIEWSTYLE_H

namespace Scintilla::Internal {

/**
 */
class MarginStyle {
public:
	Scintilla::MarginType style;
	ColourRGBA back;
	int width;
	int mask;
	bool sensitive;
	Scintilla::CursorShape cursor;
	MarginStyle(Scintilla::MarginType style_= Scintilla::MarginType::Symbol, int width_=0, int mask_=0) noexcept;
	bool ShowsFolding() const noexcept;
};

/**
 */


class FontRealised {
public:
	FontMeasurements measurements;
	std::shared_ptr<Font> font;
	void Realise(Surface &surface, int zoomLevel, Scintilla::Technology technology, const FontSpecification &fs, const char *localeName);
};

typedef std::map<FontSpecification, std::unique_ptr<FontRealised>> FontMap;

inline std::optional<ColourRGBA> OptionalColour(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) {
	if (wParam) {
		return ColourRGBA::FromIpRGB(lParam);
	} else {
		return {};
	}
}

struct SelectionAppearance {
	// Whether to draw on base layer or over text
	Scintilla::Layer layer = Layer::Base;
	// Draw selection past line end characters up to right border
	bool eolFilled = false;
};

struct CaretLineAppearance {
	// Whether to draw on base layer or over text
	Scintilla::Layer layer = Layer::Base;
	// Also show when non-focused
	bool alwaysShow = false;
	// highlight sub line instead of whole line
	bool subLine = false;
	// Non-0: draw a rectangle around line instead of filling line. Value is pixel width of frame
	int frame = 0;
};

struct CaretAppearance {
	// Line, block, over-strike bar ...
	Scintilla::CaretStyle style = CaretStyle::Line;
	// Width in pixels
	int width = 1;
};

struct WrapAppearance {
	// No wrapping, word, character, whitespace appearance
	Scintilla::Wrap state = Wrap::None;
	// Show indication of wrap at line end, line start, or in margin
	Scintilla::WrapVisualFlag visualFlags = WrapVisualFlag::None;
	// Show indication near margin or near text
	Scintilla::WrapVisualLocation visualFlagsLocation = WrapVisualLocation::Default;
	// How much indentation to show wrapping
	int visualStartIndent = 0;
	// WrapIndentMode::Fixed, Same, Indent, DeepIndent
	Scintilla::WrapIndentMode indentMode = WrapIndentMode::Fixed;
};

struct EdgeProperties {
	int column = 0;
	ColourRGBA colour;
	constexpr EdgeProperties(int column_ = 0, ColourRGBA colour_ = ColourRGBA::FromRGB(0)) noexcept :
		column(column_), colour(colour_) {
	}
};

// This is an old style enum so that its members can be used directly as indices without casting
enum StyleIndices {
	StyleDefault = static_cast<int>(Scintilla::StylesCommon::Default),
	StyleLineNumber = static_cast<int>(Scintilla::StylesCommon::LineNumber),
	StyleBraceLight = static_cast<int>(Scintilla::StylesCommon::BraceLight),
	StyleBraceBad = static_cast<int>(Scintilla::StylesCommon::BraceBad),
	StyleControlChar = static_cast<int>(Scintilla::StylesCommon::ControlChar),
	StyleIndentGuide = static_cast<int>(Scintilla::StylesCommon::IndentGuide),
	StyleCallTip = static_cast<int>(Scintilla::StylesCommon::CallTip),
	StyleFoldDisplayText = static_cast<int>(Scintilla::StylesCommon::FoldDisplayText),
};

/**
 */
class ViewStyle {
	UniqueStringSet fontNames;
	FontMap fonts;
public:
	std::vector<Style> styles;
	int nextExtendedStyle;
	std::vector<LineMarker> markers;
	int largestMarkerHeight;
	std::vector<Indicator> indicators;
	bool indicatorsDynamic;
	bool indicatorsSetFore;
	Scintilla::Technology technology;
	int lineHeight;
	int lineOverlap;
	XYPOSITION maxAscent;
	XYPOSITION maxDescent;
	XYPOSITION aveCharWidth;
	XYPOSITION spaceWidth;
	XYPOSITION tabWidth;

	SelectionAppearance selection;

	int controlCharSymbol;
	XYPOSITION controlCharWidth;
	ColourRGBA selbar;
	ColourRGBA selbarlight;
	std::optional<ColourRGBA> foldmarginColour;
	std::optional<ColourRGBA> foldmarginHighlightColour;
	bool hotspotUnderline;
	/// Margins are ordered: Line Numbers, Selection Margin, Spacing Margin
	int leftMarginWidth;	///< Spacing margin on left of text
	int rightMarginWidth;	///< Spacing margin on right of text
	int maskInLine = 0;	///< Mask for markers to be put into text because there is nowhere for them to go in margin
	int maskDrawInText = 0;	///< Mask for markers that always draw in text
	std::vector<MarginStyle> ms;
	int fixedColumnWidth = 0;	///< Total width of margins
	bool marginInside;	///< true: margin included in text view, false: separate views
	int textStart;	///< Starting x position of text within the view
	int zoomLevel;
	Scintilla::WhiteSpace viewWhitespace;
	Scintilla::TabDrawMode tabDrawMode;
	int whitespaceSize;
	Scintilla::IndentView viewIndentationGuides;
	bool viewEOL;

	CaretAppearance caret;

	CaretLineAppearance caretLine;

	bool someStylesProtected;
	bool someStylesForceCase;
	Scintilla::FontQuality extraFontFlag;
	int extraAscent;
	int extraDescent;
	int marginStyleOffset;
	Scintilla::AnnotationVisible annotationVisible;
	int annotationStyleOffset;
	Scintilla::EOLAnnotationVisible eolAnnotationVisible;
	int eolAnnotationStyleOffset;
	bool braceHighlightIndicatorSet;
	int braceHighlightIndicator;
	bool braceBadLightIndicatorSet;
	int braceBadLightIndicator;
	Scintilla::EdgeVisualStyle edgeState;
	EdgeProperties theEdge;
	std::vector<EdgeProperties> theMultiEdge;
	int marginNumberPadding; // the right-side padding of the number margin
	int ctrlCharPadding; // the padding around control character text blobs
	int lastSegItalicsOffset; // the offset so as not to clip italic characters at EOLs

	using ElementMap = std::map<Scintilla::Element, std::optional<ColourRGBA>>;
	ElementMap elementColours;
	ElementMap elementBaseColours;
	std::set<Scintilla::Element> elementAllowsTranslucent;

	WrapAppearance wrap;

	std::string localeName;

	ViewStyle(size_t stylesSize_=256);
	ViewStyle(const ViewStyle &source);
	ViewStyle(ViewStyle &&) = delete;
	// Can only be copied through copy constructor which ensures font names initialised correctly
	ViewStyle &operator=(const ViewStyle &) = delete;
	ViewStyle &operator=(ViewStyle &&) = delete;
	~ViewStyle();
	void CalculateMarginWidthAndMask() noexcept;
	void Refresh(Surface &surface, int tabInChars);
	void ReleaseAllExtendedStyles() noexcept;
	int AllocateExtendedStyles(int numberStyles);
	void EnsureStyle(size_t index);
	void ResetDefaultStyle();
	void ClearStyles();
	void SetStyleFontName(int styleIndex, const char *name);
	void SetFontLocaleName(const char *name);
	bool ProtectionActive() const noexcept;
	int ExternalMarginWidth() const noexcept;
	int MarginFromLocation(Point pt) const noexcept;
	bool ValidStyle(size_t styleIndex) const noexcept;
	void CalcLargestMarkerHeight() noexcept;
	int GetFrameWidth() const noexcept;
	bool IsLineFrameOpaque(bool caretActive, bool lineContainsCaret) const;
	std::optional<ColourRGBA> Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const;
	bool SelectionBackgroundDrawn() const noexcept;
	bool SelectionTextDrawn() const;
	bool WhitespaceBackgroundDrawn() const;
	ColourRGBA WrapColour() const;

	void AddMultiEdge(int column, ColourRGBA colour);

	std::optional<ColourRGBA> ElementColour(Scintilla::Element element) const;
	bool ElementAllowsTranslucent(Scintilla::Element element) const;
	bool ResetElement(Scintilla::Element element);
	bool SetElementColour(Scintilla::Element element, ColourRGBA colour);
	bool SetElementColourOptional(Scintilla::Element element, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void SetElementRGB(Scintilla::Element element, int rgb);
	void SetElementAlpha(Scintilla::Element element, int alpha);
	bool ElementIsSet(Scintilla::Element element) const;
	bool SetElementBase(Scintilla::Element element, ColourRGBA colour);

	bool SetWrapState(Scintilla::Wrap wrapState_) noexcept;
	bool SetWrapVisualFlags(Scintilla::WrapVisualFlag wrapVisualFlags_) noexcept;
	bool SetWrapVisualFlagsLocation(Scintilla::WrapVisualLocation wrapVisualFlagsLocation_) noexcept;
	bool SetWrapVisualStartIndent(int wrapVisualStartIndent_) noexcept;
	bool SetWrapIndentMode(Scintilla::WrapIndentMode wrapIndentMode_) noexcept;

	bool WhiteSpaceVisible(bool inIndent) const noexcept;

	enum class CaretShape { invisible, line, block, bar };
	bool IsBlockCaretStyle() const noexcept;
	bool IsCaretVisible(bool isMainSelection) const noexcept;
	bool DrawCaretInsideSelection(bool inOverstrike, bool imeCaretBlockOverride) const noexcept;
	CaretShape CaretShapeForMode(bool inOverstrike, bool isMainSelection) const noexcept;

private:
	void AllocStyles(size_t sizeNew);
	void CreateAndAddFont(const FontSpecification &fs);
	FontRealised *Find(const FontSpecification &fs);
	void FindMaxAscentDescent() noexcept;
};

}

#endif
