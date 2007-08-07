// Scintilla source code edit control
/** @file ViewStyle.h
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef VIEWSTYLE_H
#define VIEWSTYLE_H

/**
 */
class MarginStyle {
public:
	int style;
	int width;
	int mask;
	bool sensitive;
	MarginStyle();
};

/**
 */
class FontNames {
private:
	char *names[STYLE_MAX + 1];
	int max;

public:
	FontNames();
	~FontNames();
	void Clear();
	const char *Save(const char *name);
};

enum WhiteSpaceVisibility {wsInvisible=0, wsVisibleAlways=1, wsVisibleAfterIndent=2};

/**
 */
class ViewStyle {
public:
	FontNames fontNames;
	Style styles[STYLE_MAX + 1];
	LineMarker markers[MARKER_MAX + 1];
	Indicator indicators[INDIC_MAX + 1];
	int lineHeight;
	unsigned int maxAscent;
	unsigned int maxDescent;
	unsigned int aveCharWidth;
	unsigned int spaceWidth;
	bool selforeset;
	ColourPair selforeground;
	bool selbackset;
	ColourPair selbackground;
	ColourPair selbackground2;
	int selAlpha;
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
	bool viewIndentationGuides;
	bool viewEOL;
	bool showMarkedLines;
	ColourPair caretcolour;
	bool showCaretLineBackground;
	ColourPair caretLineBackground;
	int caretLineAlpha;
	ColourPair edgecolour;
	int edgeState;
	int caretWidth;
	bool someStylesProtected;
	bool extraFontFlag;

	ViewStyle();
	ViewStyle(const ViewStyle &source);
	~ViewStyle();
	void Init();
	void RefreshColourPalette(Palette &pal, bool want);
	void Refresh(Surface &surface);
	void ResetDefaultStyle();
	void ClearStyles();
	void SetStyleFontName(int styleIndex, const char *name);
	bool ProtectionActive() const;
};

#endif
