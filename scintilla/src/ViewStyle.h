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
	char **names;
	int size;
	int max;

public:
	FontNames();
	~FontNames();
	void Clear();
	const char *Save(const char *name);
};

class FontRealised : public FontSpecification, public FontMeasurements {
	// Private so FontRealised objects can not be copied
	FontRealised(const FontRealised &);
	FontRealised &operator=(const FontRealised &);
public:
	Font font;
	FontRealised *frNext;
	FontRealised(const FontSpecification &fs);
	virtual ~FontRealised();
	void Realise(Surface &surface, int zoomLevel);
	FontRealised *Find(const FontSpecification &fs);
	void FindMaxAscentDescent(unsigned int &maxAscent, unsigned int &maxDescent);
};

enum IndentView {ivNone, ivReal, ivLookForward, ivLookBoth};

enum WhiteSpaceVisibility {wsInvisible=0, wsVisibleAlways=1, wsVisibleAfterIndent=2};

/**
 */
class ViewStyle {
public:
	FontNames fontNames;
	FontRealised *frFirst;
	size_t stylesSize;
	Style *styles;
	LineMarker markers[MARKER_MAX + 1];
	Indicator indicators[INDIC_MAX + 1];
	int lineHeight;
	unsigned int maxAscent;
	unsigned int maxDescent;
	unsigned int aveCharWidth;
	unsigned int spaceWidth;
	bool selforeset;
	ColourPair selforeground;
	ColourPair selAdditionalForeground;
	bool selbackset;
	ColourPair selbackground;
	ColourPair selAdditionalBackground;
	ColourPair selbackground2;
	int selAlpha;
	int selAdditionalAlpha;
	bool selEOLFilled;
	bool whitespaceForegroundSet;
	ColourPair whitespaceForeground;
	bool whitespaceBackgroundSet;
	ColourPair whitespaceBackground;
	ColourPair selbar;
	ColourPair selbarlight;
	bool foldmarginColourSet;
	ColourPair foldmarginColour;
	bool foldmarginHighlightColourSet;
	ColourPair foldmarginHighlightColour;
	bool hotspotForegroundSet;
	ColourPair hotspotForeground;
	bool hotspotBackgroundSet;
	ColourPair hotspotBackground;
	bool hotspotUnderline;
	bool hotspotSingleLine;
	/// Margins are ordered: Line Numbers, Selection Margin, Spacing Margin
	enum { margins=5 };
	int leftMarginWidth;	///< Spacing margin on left of text
	int rightMarginWidth;	///< Spacing margin on left of text
	bool symbolMargin;
	int maskInLine;	///< Mask for markers to be put into text because there is nowhere for them to go in margin
	MarginStyle ms[margins];
	int fixedColumnWidth;
	int zoomLevel;
	WhiteSpaceVisibility viewWhitespace;
	int whitespaceSize;
	IndentView viewIndentationGuides;
	bool viewEOL;
	bool showMarkedLines;
	ColourPair caretcolour;
	ColourPair additionalCaretColour;
	bool showCaretLineBackground;
	bool showCaretLineBackgroundAlways;
	ColourPair caretLineBackground;
	int caretLineAlpha;
	ColourPair edgecolour;
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

	ViewStyle();
	ViewStyle(const ViewStyle &source);
	~ViewStyle();
	void Init(size_t stylesSize_=64);
	void CreateFont(const FontSpecification &fs);
	void RefreshColourPalette(Palette &pal, bool want);
	void Refresh(Surface &surface);
	void AllocStyles(size_t sizeNew);
	void EnsureStyle(size_t index);
	void ResetDefaultStyle();
	void ClearStyles();
	void SetStyleFontName(int styleIndex, const char *name);
	bool ProtectionActive() const;
	bool ValidStyle(size_t styleIndex) const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
