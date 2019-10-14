// Scintilla source code edit control
/** @file ViewStyle.h
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef VIEWSTYLE_H
#define VIEWSTYLE_H

namespace Scintilla {

/**
 */
class MarginStyle {
public:
	int style;
	ColourDesired back;
	int width;
	int mask;
	bool sensitive;
	int cursor;
	MarginStyle(int style_= SC_MARGIN_SYMBOL, int width_=0, int mask_=0) noexcept;
};

/**
 */


class FontRealised : public FontMeasurements {
public:
	Font font;
	FontRealised() noexcept;
	// FontRealised objects can not be copied.
	FontRealised(const FontRealised &) = delete;
	FontRealised(FontRealised &&) = delete;
	FontRealised &operator=(const FontRealised &) = delete;
	FontRealised &operator=(FontRealised &&) = delete;
	virtual ~FontRealised();
	void Realise(Surface &surface, int zoomLevel, int technology, const FontSpecification &fs);
};

enum IndentView {ivNone, ivReal, ivLookForward, ivLookBoth};

enum WhiteSpaceVisibility {wsInvisible=0, wsVisibleAlways=1, wsVisibleAfterIndent=2, wsVisibleOnlyInIndent=3};

enum TabDrawMode {tdLongArrow=0, tdStrikeOut=1};

typedef std::map<FontSpecification, std::unique_ptr<FontRealised>> FontMap;

enum WrapMode { eWrapNone, eWrapWord, eWrapChar, eWrapWhitespace };

class ColourOptional : public ColourDesired {
public:
	bool isSet;
	ColourOptional(ColourDesired colour_=ColourDesired(0,0,0), bool isSet_=false) noexcept : ColourDesired(colour_), isSet(isSet_) {
	}
	ColourOptional(uptr_t wParam, sptr_t lParam) noexcept : ColourDesired(static_cast<int>(lParam)), isSet(wParam != 0) {
	}
};

struct ForeBackColours {
	ColourOptional fore;
	ColourOptional back;
};

struct EdgeProperties {
	int column;
	ColourDesired colour;
	EdgeProperties(int column_ = 0, ColourDesired colour_ = ColourDesired(0)) noexcept :
		column(column_), colour(colour_) {
	}
	EdgeProperties(uptr_t wParam, sptr_t lParam) noexcept :
		column(static_cast<int>(wParam)), colour(static_cast<int>(lParam)) {
	}
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
	int technology;
	int lineHeight;
	int lineOverlap;
	unsigned int maxAscent;
	unsigned int maxDescent;
	XYPOSITION aveCharWidth;
	XYPOSITION spaceWidth;
	XYPOSITION tabWidth;
	ForeBackColours selColours;
	ColourDesired selAdditionalForeground;
	ColourDesired selAdditionalBackground;
	ColourDesired selBackground2;
	int selAlpha;
	int selAdditionalAlpha;
	bool selEOLFilled;
	ForeBackColours whitespaceColours;
	int controlCharSymbol;
	XYPOSITION controlCharWidth;
	ColourDesired selbar;
	ColourDesired selbarlight;
	ColourOptional foldmarginColour;
	ColourOptional foldmarginHighlightColour;
	ForeBackColours hotspotColours;
	bool hotspotUnderline;
	bool hotspotSingleLine;
	/// Margins are ordered: Line Numbers, Selection Margin, Spacing Margin
	int leftMarginWidth;	///< Spacing margin on left of text
	int rightMarginWidth;	///< Spacing margin on right of text
	int maskInLine;	///< Mask for markers to be put into text because there is nowhere for them to go in margin
	int maskDrawInText;	///< Mask for markers that always draw in text
	std::vector<MarginStyle> ms;
	int fixedColumnWidth;	///< Total width of margins
	bool marginInside;	///< true: margin included in text view, false: separate views
	int textStart;	///< Starting x position of text within the view
	int zoomLevel;
	WhiteSpaceVisibility viewWhitespace;
	TabDrawMode tabDrawMode;
	int whitespaceSize;
	IndentView viewIndentationGuides;
	bool viewEOL;
	ColourDesired caretcolour;
	ColourDesired additionalCaretColour;
	int caretLineFrame;
	bool showCaretLineBackground;
	bool alwaysShowCaretLineBackground;
	ColourDesired caretLineBackground;
	int caretLineAlpha;
	int caretStyle;
	int caretWidth;
	bool someStylesProtected;
	bool someStylesForceCase;
	int extraFontFlag;
	int extraAscent;
	int extraDescent;
	int marginStyleOffset;
	int annotationVisible;
	int annotationStyleOffset;
	bool braceHighlightIndicatorSet;
	int braceHighlightIndicator;
	bool braceBadLightIndicatorSet;
	int braceBadLightIndicator;
	int edgeState;
	EdgeProperties theEdge;
	std::vector<EdgeProperties> theMultiEdge;
	int marginNumberPadding; // the right-side padding of the number margin
	int ctrlCharPadding; // the padding around control character text blobs
	int lastSegItalicsOffset; // the offset so as not to clip italic characters at EOLs

	// Wrapping support
	WrapMode wrapState;
	int wrapVisualFlags;
	int wrapVisualFlagsLocation;
	int wrapVisualStartIndent;
	int wrapIndentMode; // SC_WRAPINDENT_FIXED, _SAME, _INDENT

	ViewStyle();
	ViewStyle(const ViewStyle &source);
	ViewStyle(ViewStyle &&) = delete;
	// Can only be copied through copy constructor which ensures font names initialised correctly
	ViewStyle &operator=(const ViewStyle &) = delete;
	ViewStyle &operator=(ViewStyle &&) = delete;
	~ViewStyle();
	void CalculateMarginWidthAndMask();
	void Init(size_t stylesSize_=256);
	void Refresh(Surface &surface, int tabInChars);
	void ReleaseAllExtendedStyles() noexcept;
	int AllocateExtendedStyles(int numberStyles);
	void EnsureStyle(size_t index);
	void ResetDefaultStyle();
	void ClearStyles();
	void SetStyleFontName(int styleIndex, const char *name);
	bool ProtectionActive() const noexcept;
	int ExternalMarginWidth() const noexcept;
	int MarginFromLocation(Point pt) const;
	bool ValidStyle(size_t styleIndex) const noexcept;
	void CalcLargestMarkerHeight();
	int GetFrameWidth() const noexcept;
	bool IsLineFrameOpaque(bool caretActive, bool lineContainsCaret) const noexcept;
	ColourOptional Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const;
	bool SelectionBackgroundDrawn() const noexcept;
	bool WhitespaceBackgroundDrawn() const noexcept;
	ColourDesired WrapColour() const;

	bool SetWrapState(int wrapState_) noexcept;
	bool SetWrapVisualFlags(int wrapVisualFlags_) noexcept;
	bool SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation_) noexcept;
	bool SetWrapVisualStartIndent(int wrapVisualStartIndent_) noexcept;
	bool SetWrapIndentMode(int wrapIndentMode_) noexcept;

	bool WhiteSpaceVisible(bool inIndent) const noexcept;

	enum class CaretShape { invisible, line, block, bar };
	bool IsBlockCaretStyle() const noexcept;
	bool DrawCaretInsideSelection(bool inOverstrike, bool imeCaretBlockOverride) const noexcept;
	CaretShape CaretShapeForMode(bool inOverstrike) const noexcept;

private:
	void AllocStyles(size_t sizeNew);
	void CreateAndAddFont(const FontSpecification &fs);
	FontRealised *Find(const FontSpecification &fs);
	void FindMaxAscentDescent();
};

}

#endif
