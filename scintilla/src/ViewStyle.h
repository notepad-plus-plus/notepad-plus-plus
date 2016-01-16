// Scintilla source code edit control
/** @file ViewStyle.h
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef VIEWSTYLE_H
#define VIEWSTYLE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class MarginStyle {
public:
	int style;
	int width;
	int mask;
	bool sensitive;
	int cursor;
	MarginStyle();
};

/**
 */
class FontNames {
private:
	std::vector<char *> names;

	// Private so FontNames objects can not be copied
	FontNames(const FontNames &);
public:
	FontNames();
	~FontNames();
	void Clear();
	const char *Save(const char *name);
};

class FontRealised : public FontMeasurements {
	// Private so FontRealised objects can not be copied
	FontRealised(const FontRealised &);
	FontRealised &operator=(const FontRealised &);
public:
	Font font;
	FontRealised();
	virtual ~FontRealised();
	void Realise(Surface &surface, int zoomLevel, int technology, const FontSpecification &fs);
};

enum IndentView {ivNone, ivReal, ivLookForward, ivLookBoth};

enum WhiteSpaceVisibility {wsInvisible=0, wsVisibleAlways=1, wsVisibleAfterIndent=2, wsVisibleOnlyInIndent=3};

typedef std::map<FontSpecification, FontRealised *> FontMap;

enum WrapMode { eWrapNone, eWrapWord, eWrapChar, eWrapWhitespace };

class ColourOptional : public ColourDesired {
public:
	bool isSet;
	ColourOptional(ColourDesired colour_=ColourDesired(0,0,0), bool isSet_=false) : ColourDesired(colour_), isSet(isSet_) {
	}
	ColourOptional(uptr_t wParam, sptr_t lParam) : ColourDesired(static_cast<long>(lParam)), isSet(wParam != 0) {
	}
};

struct ForeBackColours {
	ColourOptional fore;
	ColourOptional back;
};

/**
 */
class ViewStyle {
	FontNames fontNames;
	FontMap fonts;
public:
	std::vector<Style> styles;
	size_t nextExtendedStyle;
	LineMarker markers[MARKER_MAX + 1];
	int largestMarkerHeight;
	Indicator indicators[INDIC_MAX + 1];
	unsigned int indicatorsDynamic;
	unsigned int indicatorsSetFore;
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
	MarginStyle ms[SC_MAX_MARGIN+1];
	int fixedColumnWidth;	///< Total width of margins
	bool marginInside;	///< true: margin included in text view, false: separate views
	int textStart;	///< Starting x position of text within the view
	int zoomLevel;
	WhiteSpaceVisibility viewWhitespace;
	int whitespaceSize;
	IndentView viewIndentationGuides;
	bool viewEOL;
	ColourDesired caretcolour;
	ColourDesired additionalCaretColour;
	bool showCaretLineBackground;
	bool alwaysShowCaretLineBackground;
	ColourDesired caretLineBackground;
	int caretLineAlpha;
	ColourDesired edgecolour;
	int edgeState;
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
	int theEdge;
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
	~ViewStyle();
	void CalculateMarginWidthAndMask();
	void Init(size_t stylesSize_=256);
	void Refresh(Surface &surface, int tabInChars);
	void ReleaseAllExtendedStyles();
	int AllocateExtendedStyles(int numberStyles);
	void EnsureStyle(size_t index);
	void ResetDefaultStyle();
	void ClearStyles();
	void SetStyleFontName(int styleIndex, const char *name);
	bool ProtectionActive() const;
	int ExternalMarginWidth() const;
	bool ValidStyle(size_t styleIndex) const;
	void CalcLargestMarkerHeight();
	ColourOptional Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const;
	bool SelectionBackgroundDrawn() const;
	bool WhitespaceBackgroundDrawn() const;
	ColourDesired WrapColour() const;

	bool SetWrapState(int wrapState_);
	bool SetWrapVisualFlags(int wrapVisualFlags_);
	bool SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation_);
	bool SetWrapVisualStartIndent(int wrapVisualStartIndent_);
	bool SetWrapIndentMode(int wrapIndentMode_);

	bool WhiteSpaceVisible(bool inIndent) const;

private:
	void AllocStyles(size_t sizeNew);
	void CreateAndAddFont(const FontSpecification &fs);
	FontRealised *Find(const FontSpecification &fs);
	void FindMaxAscentDescent();
	// Private so can only be copied through copy constructor which ensures font names initialised correctly
	ViewStyle &operator=(const ViewStyle &);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
