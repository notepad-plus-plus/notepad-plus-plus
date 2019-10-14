// Scintilla source code edit control
/** @file ViewStyle.cxx
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Scintilla.h"
#include "Position.h"
#include "UniqueString.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"

using namespace Scintilla;

MarginStyle::MarginStyle(int style_, int width_, int mask_) noexcept :
	style(style_), width(width_), mask(mask_), sensitive(false), cursor(SC_CURSORREVERSEARROW) {
}

FontRealised::FontRealised() noexcept = default;

FontRealised::~FontRealised() {
	font.Release();
}

void FontRealised::Realise(Surface &surface, int zoomLevel, int technology, const FontSpecification &fs) {
	PLATFORM_ASSERT(fs.fontName);
	sizeZoomed = fs.size + zoomLevel * SC_FONT_SIZE_MULTIPLIER;
	if (sizeZoomed <= 2 * SC_FONT_SIZE_MULTIPLIER)	// Hangs if sizeZoomed <= 1
		sizeZoomed = 2 * SC_FONT_SIZE_MULTIPLIER;

	const float deviceHeight = static_cast<float>(surface.DeviceHeightFont(sizeZoomed));
	const FontParameters fp(fs.fontName, deviceHeight / SC_FONT_SIZE_MULTIPLIER, fs.weight, fs.italic, fs.extraFontFlag, technology, fs.characterSet);
	font.Create(fp);

	ascent = static_cast<unsigned int>(surface.Ascent(font));
	descent = static_cast<unsigned int>(surface.Descent(font));
	capitalHeight = surface.Ascent(font) - surface.InternalLeading(font);
	aveCharWidth = surface.AverageCharWidth(font);
	spaceWidth = surface.WidthText(font, " ");
}

ViewStyle::ViewStyle() : markers(MARKER_MAX + 1), indicators(INDICATOR_MAX + 1) {
	Init();
}

// Copy constructor only called when printing copies the screen ViewStyle so it can be
// modified for printing styles.
ViewStyle::ViewStyle(const ViewStyle &source) : markers(MARKER_MAX + 1), indicators(INDICATOR_MAX + 1) {
	Init(source.styles.size());
	styles = source.styles;
	for (size_t sty=0; sty<source.styles.size(); sty++) {
		// Can't just copy fontName as its lifetime is relative to its owning ViewStyle
		styles[sty].fontName = fontNames.Save(source.styles[sty].fontName);
	}
	nextExtendedStyle = source.nextExtendedStyle;
	markers = source.markers;
	CalcLargestMarkerHeight();

	indicators = source.indicators;

	indicatorsDynamic = source.indicatorsDynamic;
	indicatorsSetFore = source.indicatorsSetFore;

	selColours = source.selColours;
	selAdditionalForeground = source.selAdditionalForeground;
	selAdditionalBackground = source.selAdditionalBackground;
	selBackground2 = source.selBackground2;
	selAlpha = source.selAlpha;
	selAdditionalAlpha = source.selAdditionalAlpha;
	selEOLFilled = source.selEOLFilled;

	foldmarginColour = source.foldmarginColour;
	foldmarginHighlightColour = source.foldmarginHighlightColour;

	hotspotColours = source.hotspotColours;
	hotspotUnderline = source.hotspotUnderline;
	hotspotSingleLine = source.hotspotSingleLine;

	whitespaceColours = source.whitespaceColours;
	controlCharSymbol = source.controlCharSymbol;
	controlCharWidth = source.controlCharWidth;
	selbar = source.selbar;
	selbarlight = source.selbarlight;
	caretcolour = source.caretcolour;
	additionalCaretColour = source.additionalCaretColour;
	caretLineFrame = source.caretLineFrame;
	showCaretLineBackground = source.showCaretLineBackground;
	alwaysShowCaretLineBackground = source.alwaysShowCaretLineBackground;
	caretLineBackground = source.caretLineBackground;
	caretLineAlpha = source.caretLineAlpha;
	caretStyle = source.caretStyle;
	caretWidth = source.caretWidth;
	someStylesProtected = false;
	someStylesForceCase = false;
	leftMarginWidth = source.leftMarginWidth;
	rightMarginWidth = source.rightMarginWidth;
	ms = source.ms;
	maskInLine = source.maskInLine;
	maskDrawInText = source.maskDrawInText;
	fixedColumnWidth = source.fixedColumnWidth;
	marginInside = source.marginInside;
	textStart = source.textStart;
	zoomLevel = source.zoomLevel;
	viewWhitespace = source.viewWhitespace;
	tabDrawMode = source.tabDrawMode;
	whitespaceSize = source.whitespaceSize;
	viewIndentationGuides = source.viewIndentationGuides;
	viewEOL = source.viewEOL;
	extraFontFlag = source.extraFontFlag;
	extraAscent = source.extraAscent;
	extraDescent = source.extraDescent;
	marginStyleOffset = source.marginStyleOffset;
	annotationVisible = source.annotationVisible;
	annotationStyleOffset = source.annotationStyleOffset;
	braceHighlightIndicatorSet = source.braceHighlightIndicatorSet;
	braceHighlightIndicator = source.braceHighlightIndicator;
	braceBadLightIndicatorSet = source.braceBadLightIndicatorSet;
	braceBadLightIndicator = source.braceBadLightIndicator;

	edgeState = source.edgeState;
	theEdge = source.theEdge;
	theMultiEdge = source.theMultiEdge;

	marginNumberPadding = source.marginNumberPadding;
	ctrlCharPadding = source.ctrlCharPadding;
	lastSegItalicsOffset = source.lastSegItalicsOffset;

	wrapState = source.wrapState;
	wrapVisualFlags = source.wrapVisualFlags;
	wrapVisualFlagsLocation = source.wrapVisualFlagsLocation;
	wrapVisualStartIndent = source.wrapVisualStartIndent;
	wrapIndentMode = source.wrapIndentMode;
}

ViewStyle::~ViewStyle() {
	styles.clear();
	fonts.clear();
}

void ViewStyle::CalculateMarginWidthAndMask() {
	fixedColumnWidth = marginInside ? leftMarginWidth : 0;
	maskInLine = 0xffffffff;
	int maskDefinedMarkers = 0;
	for (const MarginStyle &m : ms) {
		fixedColumnWidth += m.width;
		if (m.width > 0)
			maskInLine &= ~m.mask;
		maskDefinedMarkers |= m.mask;
	}
	maskDrawInText = 0;
	for (int markBit = 0; markBit < 32; markBit++) {
		const int maskBit = 1U << markBit;
		switch (markers[markBit].markType) {
		case SC_MARK_EMPTY:
			maskInLine &= ~maskBit;
			break;
		case SC_MARK_BACKGROUND:
		case SC_MARK_UNDERLINE:
			maskInLine &= ~maskBit;
			maskDrawInText |= maskDefinedMarkers & maskBit;
			break;
		}
	}
}

void ViewStyle::Init(size_t stylesSize_) {
	AllocStyles(stylesSize_);
	nextExtendedStyle = 256;
	fontNames.Clear();
	ResetDefaultStyle();

	// There are no image markers by default, so no need for calling CalcLargestMarkerHeight()
	largestMarkerHeight = 0;

	indicators[0] = Indicator(INDIC_SQUIGGLE, ColourDesired(0, 0x7f, 0));
	indicators[1] = Indicator(INDIC_TT, ColourDesired(0, 0, 0xff));
	indicators[2] = Indicator(INDIC_PLAIN, ColourDesired(0xff, 0, 0));

	technology = SC_TECHNOLOGY_DEFAULT;
	indicatorsDynamic = false;
	indicatorsSetFore = false;
	lineHeight = 1;
	lineOverlap = 0;
	maxAscent = 1;
	maxDescent = 1;
	aveCharWidth = 8;
	spaceWidth = 8;
	tabWidth = spaceWidth * 8;

	selColours.fore = ColourOptional(ColourDesired(0xff, 0, 0));
	selColours.back = ColourOptional(ColourDesired(0xc0, 0xc0, 0xc0), true);
	selAdditionalForeground = ColourDesired(0xff, 0, 0);
	selAdditionalBackground = ColourDesired(0xd7, 0xd7, 0xd7);
	selBackground2 = ColourDesired(0xb0, 0xb0, 0xb0);
	selAlpha = SC_ALPHA_NOALPHA;
	selAdditionalAlpha = SC_ALPHA_NOALPHA;
	selEOLFilled = false;

	foldmarginColour = ColourOptional(ColourDesired(0xff, 0, 0));
	foldmarginHighlightColour = ColourOptional(ColourDesired(0xc0, 0xc0, 0xc0));

	whitespaceColours.fore = ColourOptional();
	whitespaceColours.back = ColourOptional(ColourDesired(0xff, 0xff, 0xff));
	controlCharSymbol = 0;	/* Draw the control characters */
	controlCharWidth = 0;
	selbar = Platform::Chrome();
	selbarlight = Platform::ChromeHighlight();
	styles[STYLE_LINENUMBER].fore = ColourDesired(0, 0, 0);
	styles[STYLE_LINENUMBER].back = Platform::Chrome();
	caretcolour = ColourDesired(0, 0, 0);
	additionalCaretColour = ColourDesired(0x7f, 0x7f, 0x7f);
	caretLineFrame = 0;
	showCaretLineBackground = false;
	alwaysShowCaretLineBackground = false;
	caretLineBackground = ColourDesired(0xff, 0xff, 0);
	caretLineAlpha = SC_ALPHA_NOALPHA;
	caretStyle = CARETSTYLE_LINE;
	caretWidth = 1;
	someStylesProtected = false;
	someStylesForceCase = false;

	hotspotColours.fore = ColourOptional(ColourDesired(0, 0, 0xff));
	hotspotColours.back = ColourOptional(ColourDesired(0xff, 0xff, 0xff));
	hotspotUnderline = true;
	hotspotSingleLine = true;

	leftMarginWidth = 1;
	rightMarginWidth = 1;
	ms.resize(SC_MAX_MARGIN + 1);
	ms[0] = MarginStyle(SC_MARGIN_NUMBER);
	ms[1] = MarginStyle(SC_MARGIN_SYMBOL, 16, ~SC_MASK_FOLDERS);
	ms[2] = MarginStyle(SC_MARGIN_SYMBOL);
	marginInside = true;
	CalculateMarginWidthAndMask();
	textStart = marginInside ? fixedColumnWidth : leftMarginWidth;
	zoomLevel = 0;
	viewWhitespace = wsInvisible;
	tabDrawMode = tdLongArrow;
	whitespaceSize = 1;
	viewIndentationGuides = ivNone;
	viewEOL = false;
	extraFontFlag = 0;
	extraAscent = 0;
	extraDescent = 0;
	marginStyleOffset = 0;
	annotationVisible = ANNOTATION_HIDDEN;
	annotationStyleOffset = 0;
	braceHighlightIndicatorSet = false;
	braceHighlightIndicator = 0;
	braceBadLightIndicatorSet = false;
	braceBadLightIndicator = 0;

	edgeState = EDGE_NONE;
	theEdge = EdgeProperties(0, ColourDesired(0xc0, 0xc0, 0xc0));

	marginNumberPadding = 3;
	ctrlCharPadding = 3; // +3 For a blank on front and rounded edge each side
	lastSegItalicsOffset = 2;

	wrapState = eWrapNone;
	wrapVisualFlags = 0;
	wrapVisualFlagsLocation = 0;
	wrapVisualStartIndent = 0;
	wrapIndentMode = SC_WRAPINDENT_FIXED;
}

void ViewStyle::Refresh(Surface &surface, int tabInChars) {
	fonts.clear();

	selbar = Platform::Chrome();
	selbarlight = Platform::ChromeHighlight();

	// Apply the extra font flag which controls text drawing quality to each style.
	for (Style &style : styles) {
		style.extraFontFlag = extraFontFlag;
	}

	// Create a FontRealised object for each unique font in the styles.
	CreateAndAddFont(styles[STYLE_DEFAULT]);
	for (const Style &style : styles) {
		CreateAndAddFont(style);
	}

	// Ask platform to allocate each unique font.
	for (std::pair<const FontSpecification, std::unique_ptr<FontRealised>> &font : fonts) {
		font.second->Realise(surface, zoomLevel, technology, font.first);
	}

	// Set the platform font handle and measurements for each style.
	for (Style &style : styles) {
		FontRealised *fr = Find(style);
		style.Copy(fr->font, *fr);
	}

	indicatorsDynamic = std::any_of(indicators.cbegin(), indicators.cend(),
		[](const Indicator &indicator) { return indicator.IsDynamic(); });

	indicatorsSetFore = std::any_of(indicators.cbegin(), indicators.cend(),
		[](const Indicator &indicator) { return indicator.OverridesTextFore(); });

	maxAscent = 1;
	maxDescent = 1;
	FindMaxAscentDescent();
	maxAscent += extraAscent;
	maxDescent += extraDescent;
	lineHeight = maxAscent + maxDescent;
	lineOverlap = lineHeight / 10;
	if (lineOverlap < 2)
		lineOverlap = 2;
	if (lineOverlap > lineHeight)
		lineOverlap = lineHeight;

	someStylesProtected = std::any_of(styles.cbegin(), styles.cend(),
		[](const Style &style) { return style.IsProtected(); });

	someStylesForceCase = std::any_of(styles.cbegin(), styles.cend(),
		[](const Style &style) { return style.caseForce != Style::caseMixed; });

	aveCharWidth = styles[STYLE_DEFAULT].aveCharWidth;
	spaceWidth = styles[STYLE_DEFAULT].spaceWidth;
	tabWidth = spaceWidth * tabInChars;

	controlCharWidth = 0.0;
	if (controlCharSymbol >= 32) {
		const char cc[2] = { static_cast<char>(controlCharSymbol), '\0' };
		controlCharWidth = surface.WidthText(styles[STYLE_CONTROLCHAR].font, cc);
	}

	CalculateMarginWidthAndMask();
	textStart = marginInside ? fixedColumnWidth : leftMarginWidth;
}

void ViewStyle::ReleaseAllExtendedStyles() noexcept {
	nextExtendedStyle = 256;
}

int ViewStyle::AllocateExtendedStyles(int numberStyles) {
	const int startRange = nextExtendedStyle;
	nextExtendedStyle += numberStyles;
	EnsureStyle(nextExtendedStyle);
	for (int i=startRange; i<nextExtendedStyle; i++) {
		styles[i].ClearTo(styles[STYLE_DEFAULT]);
	}
	return startRange;
}

void ViewStyle::EnsureStyle(size_t index) {
	if (index >= styles.size()) {
		AllocStyles(index+1);
	}
}

void ViewStyle::ResetDefaultStyle() {
	styles[STYLE_DEFAULT].Clear(ColourDesired(0,0,0),
	        ColourDesired(0xff,0xff,0xff),
	        Platform::DefaultFontSize() * SC_FONT_SIZE_MULTIPLIER, fontNames.Save(Platform::DefaultFont()),
	        SC_CHARSET_DEFAULT,
	        SC_WEIGHT_NORMAL, false, false, false, Style::caseMixed, true, true, false);
}

void ViewStyle::ClearStyles() {
	// Reset all styles to be like the default style
	for (size_t i=0; i<styles.size(); i++) {
		if (i != STYLE_DEFAULT) {
			styles[i].ClearTo(styles[STYLE_DEFAULT]);
		}
	}
	styles[STYLE_LINENUMBER].back = Platform::Chrome();

	// Set call tip fore/back to match the values previously set for call tips
	styles[STYLE_CALLTIP].back = ColourDesired(0xff, 0xff, 0xff);
	styles[STYLE_CALLTIP].fore = ColourDesired(0x80, 0x80, 0x80);
}

void ViewStyle::SetStyleFontName(int styleIndex, const char *name) {
	styles[styleIndex].fontName = fontNames.Save(name);
}

bool ViewStyle::ProtectionActive() const noexcept {
	return someStylesProtected;
}

int ViewStyle::ExternalMarginWidth() const noexcept {
	return marginInside ? 0 : fixedColumnWidth;
}

int ViewStyle::MarginFromLocation(Point pt) const {
	int margin = -1;
	int x = marginInside ? 0 : -fixedColumnWidth;
	for (size_t i = 0; i < ms.size(); i++) {
		if ((pt.x >= x) && (pt.x < x + ms[i].width))
			margin = static_cast<int>(i);
		x += ms[i].width;
	}
	return margin;
}

bool ViewStyle::ValidStyle(size_t styleIndex) const noexcept {
	return styleIndex < styles.size();
}

void ViewStyle::CalcLargestMarkerHeight() {
	largestMarkerHeight = 0;
	for (const LineMarker &marker : markers) {
		switch (marker.markType) {
		case SC_MARK_PIXMAP:
			if (marker.pxpm && marker.pxpm->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = marker.pxpm->GetHeight();
			break;
		case SC_MARK_RGBAIMAGE:
			if (marker.image && marker.image->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = marker.image->GetHeight();
			break;
		}
	}
}

int ViewStyle::GetFrameWidth() const noexcept {
	return std::clamp(caretLineFrame, 1, lineHeight / 3);
}

bool ViewStyle::IsLineFrameOpaque(bool caretActive, bool lineContainsCaret) const noexcept {
	return caretLineFrame && (caretActive || alwaysShowCaretLineBackground) && showCaretLineBackground &&
		(caretLineAlpha == SC_ALPHA_NOALPHA) && lineContainsCaret;
}

// See if something overrides the line background colour:  Either if caret is on the line
// and background colour is set for that, or if a marker is defined that forces its background
// colour onto the line, or if a marker is defined but has no selection margin in which to
// display itself (as long as it's not an SC_MARK_EMPTY marker).  These are checked in order
// with the earlier taking precedence.  When multiple markers cause background override,
// the colour for the highest numbered one is used.
ColourOptional ViewStyle::Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const {
	ColourOptional background;
	if (!caretLineFrame && (caretActive || alwaysShowCaretLineBackground) && showCaretLineBackground &&
		(caretLineAlpha == SC_ALPHA_NOALPHA) && lineContainsCaret) {
		background = ColourOptional(caretLineBackground, true);
	}
	if (!background.isSet && marksOfLine) {
		int marks = marksOfLine;
		for (int markBit = 0; (markBit < 32) && marks; markBit++) {
			if ((marks & 1) && (markers[markBit].markType == SC_MARK_BACKGROUND) &&
				(markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
				background = ColourOptional(markers[markBit].back, true);
			}
			marks >>= 1;
		}
	}
	if (!background.isSet && maskInLine) {
		int marksMasked = marksOfLine & maskInLine;
		if (marksMasked) {
			for (int markBit = 0; (markBit < 32) && marksMasked; markBit++) {
				if ((marksMasked & 1) &&
					(markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
					background = ColourOptional(markers[markBit].back, true);
				}
				marksMasked >>= 1;
			}
		}
	}
	return background;
}

bool ViewStyle::SelectionBackgroundDrawn() const noexcept {
	return selColours.back.isSet &&
		((selAlpha == SC_ALPHA_NOALPHA) || (selAdditionalAlpha == SC_ALPHA_NOALPHA));
}

bool ViewStyle::WhitespaceBackgroundDrawn() const noexcept {
	return (viewWhitespace != wsInvisible) && (whitespaceColours.back.isSet);
}

bool ViewStyle::WhiteSpaceVisible(bool inIndent) const noexcept {
	return (!inIndent && viewWhitespace == wsVisibleAfterIndent) ||
		(inIndent && viewWhitespace == wsVisibleOnlyInIndent) ||
		viewWhitespace == wsVisibleAlways;
}

ColourDesired ViewStyle::WrapColour() const {
	if (whitespaceColours.fore.isSet)
		return whitespaceColours.fore;
	else
		return styles[STYLE_DEFAULT].fore;
}

bool ViewStyle::SetWrapState(int wrapState_) noexcept {
	WrapMode wrapStateWanted;
	switch (wrapState_) {
	case SC_WRAP_WORD:
		wrapStateWanted = eWrapWord;
		break;
	case SC_WRAP_CHAR:
		wrapStateWanted = eWrapChar;
		break;
	case SC_WRAP_WHITESPACE:
		wrapStateWanted = eWrapWhitespace;
		break;
	default:
		wrapStateWanted = eWrapNone;
		break;
	}
	const bool changed = wrapState != wrapStateWanted;
	wrapState = wrapStateWanted;
	return changed;
}

bool ViewStyle::SetWrapVisualFlags(int wrapVisualFlags_) noexcept {
	const bool changed = wrapVisualFlags != wrapVisualFlags_;
	wrapVisualFlags = wrapVisualFlags_;
	return changed;
}

bool ViewStyle::SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation_) noexcept {
	const bool changed = wrapVisualFlagsLocation != wrapVisualFlagsLocation_;
	wrapVisualFlagsLocation = wrapVisualFlagsLocation_;
	return changed;
}

bool ViewStyle::SetWrapVisualStartIndent(int wrapVisualStartIndent_) noexcept {
	const bool changed = wrapVisualStartIndent != wrapVisualStartIndent_;
	wrapVisualStartIndent = wrapVisualStartIndent_;
	return changed;
}

bool ViewStyle::SetWrapIndentMode(int wrapIndentMode_) noexcept {
	const bool changed = wrapIndentMode != wrapIndentMode_;
	wrapIndentMode = wrapIndentMode_;
	return changed;
}

bool ViewStyle::IsBlockCaretStyle() const noexcept {
	return ((caretStyle & CARETSTYLE_INS_MASK) == CARETSTYLE_BLOCK) ||
		(caretStyle & CARETSTYLE_OVERSTRIKE_BLOCK) != 0;
}

bool ViewStyle::DrawCaretInsideSelection(bool inOverstrike, bool imeCaretBlockOverride) const noexcept {
	if (caretStyle & CARETSTYLE_BLOCK_AFTER)
		return false;
	return ((caretStyle & CARETSTYLE_INS_MASK) == CARETSTYLE_BLOCK) ||
		(inOverstrike && (caretStyle & CARETSTYLE_OVERSTRIKE_BLOCK) != 0) ||
		imeCaretBlockOverride;
}

ViewStyle::CaretShape ViewStyle::CaretShapeForMode(bool inOverstrike) const noexcept {
	if (inOverstrike) {
		return (caretStyle & CARETSTYLE_OVERSTRIKE_BLOCK) ? CaretShape::block : CaretShape::bar;
	}

	const int caret = caretStyle & CARETSTYLE_INS_MASK;
	return (caret <= CARETSTYLE_BLOCK) ? static_cast<CaretShape>(caret) : CaretShape::line;
}

void ViewStyle::AllocStyles(size_t sizeNew) {
	size_t i=styles.size();
	styles.resize(sizeNew);
	if (styles.size() > STYLE_DEFAULT) {
		for (; i<sizeNew; i++) {
			if (i != STYLE_DEFAULT) {
				styles[i].ClearTo(styles[STYLE_DEFAULT]);
			}
		}
	}
}

void ViewStyle::CreateAndAddFont(const FontSpecification &fs) {
	if (fs.fontName) {
		FontMap::iterator it = fonts.find(fs);
		if (it == fonts.end()) {
			fonts[fs] = std::make_unique<FontRealised>();
		}
	}
}

FontRealised *ViewStyle::Find(const FontSpecification &fs) {
	if (!fs.fontName)	// Invalid specification so return arbitrary object
		return fonts.begin()->second.get();
	FontMap::iterator it = fonts.find(fs);
	if (it != fonts.end()) {
		// Should always reach here since map was just set for all styles
		return it->second.get();
	}
	return nullptr;
}

void ViewStyle::FindMaxAscentDescent() {
	for (FontMap::const_iterator it = fonts.cbegin(); it != fonts.cend(); ++it) {
		if (maxAscent < it->second->ascent)
			maxAscent = it->second->ascent;
		if (maxDescent < it->second->descent)
			maxDescent = it->second->descent;
	}
}
