// Scintilla source code edit control
/** @file ViewStyle.cxx
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
#include <assert.h>

#include <vector>
#include <map>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

MarginStyle::MarginStyle() :
	style(SC_MARGIN_SYMBOL), width(0), mask(0), sensitive(false), cursor(SC_CURSORREVERSEARROW) {
}

// A list of the fontnames - avoids wasting space in each style
FontNames::FontNames() {
}

FontNames::~FontNames() {
	Clear();
}

void FontNames::Clear() {
	for (std::vector<char *>::const_iterator it=names.begin(); it != names.end(); ++it) {
		delete []*it;
	}
	names.clear();
}

const char *FontNames::Save(const char *name) {
	if (!name)
		return 0;

	for (std::vector<char *>::const_iterator it=names.begin(); it != names.end(); ++it) {
		if (strcmp(*it, name) == 0) {
			return *it;
		}
	}
	const size_t lenName = strlen(name) + 1;
	char *nameSave = new char[lenName];
	memcpy(nameSave, name, lenName);
	names.push_back(nameSave);
	return nameSave;
}

FontRealised::FontRealised() {
}

FontRealised::~FontRealised() {
	font.Release();
}

void FontRealised::Realise(Surface &surface, int zoomLevel, int technology, const FontSpecification &fs) {
	PLATFORM_ASSERT(fs.fontName);
	sizeZoomed = fs.size + zoomLevel * SC_FONT_SIZE_MULTIPLIER;
	if (sizeZoomed <= 2 * SC_FONT_SIZE_MULTIPLIER)	// Hangs if sizeZoomed <= 1
		sizeZoomed = 2 * SC_FONT_SIZE_MULTIPLIER;

	float deviceHeight = static_cast<float>(surface.DeviceHeightFont(sizeZoomed));
	FontParameters fp(fs.fontName, deviceHeight / SC_FONT_SIZE_MULTIPLIER, fs.weight, fs.italic, fs.extraFontFlag, technology, fs.characterSet);
	font.Create(fp);

	ascent = static_cast<unsigned int>(surface.Ascent(font));
	descent = static_cast<unsigned int>(surface.Descent(font));
	aveCharWidth = surface.AverageCharWidth(font);
	spaceWidth = surface.WidthChar(font, ' ');
}

ViewStyle::ViewStyle() {
	Init();
}

ViewStyle::ViewStyle(const ViewStyle &source) {
	Init(source.styles.size());
	for (unsigned int sty=0; sty<source.styles.size(); sty++) {
		styles[sty] = source.styles[sty];
		// Can't just copy fontname as its lifetime is relative to its owning ViewStyle
		styles[sty].fontName = fontNames.Save(source.styles[sty].fontName);
	}
	nextExtendedStyle = source.nextExtendedStyle;
	for (int mrk=0; mrk<=MARKER_MAX; mrk++) {
		markers[mrk] = source.markers[mrk];
	}
	CalcLargestMarkerHeight();
	indicatorsDynamic = 0;
	indicatorsSetFore = 0;
	for (int ind=0; ind<=INDIC_MAX; ind++) {
		indicators[ind] = source.indicators[ind];
		if (indicators[ind].IsDynamic())
			indicatorsDynamic++;
		if (indicators[ind].OverridesTextFore())
			indicatorsSetFore++;
	}

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
	showCaretLineBackground = source.showCaretLineBackground;
	alwaysShowCaretLineBackground = source.alwaysShowCaretLineBackground;
	caretLineBackground = source.caretLineBackground;
	caretLineAlpha = source.caretLineAlpha;
	edgecolour = source.edgecolour;
	edgeState = source.edgeState;
	caretStyle = source.caretStyle;
	caretWidth = source.caretWidth;
	someStylesProtected = false;
	someStylesForceCase = false;
	leftMarginWidth = source.leftMarginWidth;
	rightMarginWidth = source.rightMarginWidth;
	for (int margin=0; margin <= SC_MAX_MARGIN; margin++) {
		ms[margin] = source.ms[margin];
	}
	maskInLine = source.maskInLine;
	fixedColumnWidth = source.fixedColumnWidth;
	marginInside = source.marginInside;
	textStart = source.textStart;
	zoomLevel = source.zoomLevel;
	viewWhitespace = source.viewWhitespace;
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

	theEdge = source.theEdge;

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
	for (FontMap::iterator it = fonts.begin(); it != fonts.end(); ++it) {
		delete it->second;
	}
	fonts.clear();
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
	indicatorsDynamic = 0;
	indicatorsSetFore = 0;
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
	showCaretLineBackground = false;
	alwaysShowCaretLineBackground = false;
	caretLineBackground = ColourDesired(0xff, 0xff, 0);
	caretLineAlpha = SC_ALPHA_NOALPHA;
	edgecolour = ColourDesired(0xc0, 0xc0, 0xc0);
	edgeState = EDGE_NONE;
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
	ms[0].style = SC_MARGIN_NUMBER;
	ms[0].width = 0;
	ms[0].mask = 0;
	ms[1].style = SC_MARGIN_SYMBOL;
	ms[1].width = 16;
	ms[1].mask = ~SC_MASK_FOLDERS;
	ms[2].style = SC_MARGIN_SYMBOL;
	ms[2].width = 0;
	ms[2].mask = 0;
	marginInside = true;
	fixedColumnWidth = marginInside ? leftMarginWidth : 0;
	maskInLine = 0xffffffff;
	for (int margin=0; margin <= SC_MAX_MARGIN; margin++) {
		fixedColumnWidth += ms[margin].width;
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
	textStart = marginInside ? fixedColumnWidth : leftMarginWidth;
	zoomLevel = 0;
	viewWhitespace = wsInvisible;
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

	theEdge = 0;

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
	for (FontMap::iterator it = fonts.begin(); it != fonts.end(); ++it) {
		delete it->second;
	}
	fonts.clear();

	selbar = Platform::Chrome();
	selbarlight = Platform::ChromeHighlight();

	for (unsigned int i=0; i<styles.size(); i++) {
		styles[i].extraFontFlag = extraFontFlag;
	}

	CreateAndAddFont(styles[STYLE_DEFAULT]);
	for (unsigned int j=0; j<styles.size(); j++) {
		CreateAndAddFont(styles[j]);
	}

	for (FontMap::iterator it = fonts.begin(); it != fonts.end(); ++it) {
		it->second->Realise(surface, zoomLevel, technology, it->first);
	}

	for (unsigned int k=0; k<styles.size(); k++) {
		FontRealised *fr = Find(styles[k]);
		styles[k].Copy(fr->font, *fr);
	}
	indicatorsDynamic = 0;
	indicatorsSetFore = 0;
	for (int ind = 0; ind <= INDIC_MAX; ind++) {
		if (indicators[ind].IsDynamic())
			indicatorsDynamic++;
		if (indicators[ind].OverridesTextFore())
			indicatorsSetFore++;
	}
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

	someStylesProtected = false;
	someStylesForceCase = false;
	for (unsigned int l=0; l<styles.size(); l++) {
		if (styles[l].IsProtected()) {
			someStylesProtected = true;
		}
		if (styles[l].caseForce != Style::caseMixed) {
			someStylesForceCase = true;
		}
	}

	aveCharWidth = styles[STYLE_DEFAULT].aveCharWidth;
	spaceWidth = styles[STYLE_DEFAULT].spaceWidth;
	tabWidth = spaceWidth * tabInChars;

	controlCharWidth = 0.0;
	if (controlCharSymbol >= 32) {
		controlCharWidth = surface.WidthChar(styles[STYLE_CONTROLCHAR].font, static_cast<char>(controlCharSymbol));
	}

	fixedColumnWidth = marginInside ? leftMarginWidth : 0;
	maskInLine = 0xffffffff;
	for (int margin=0; margin <= SC_MAX_MARGIN; margin++) {
		fixedColumnWidth += ms[margin].width;
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
	textStart = marginInside ? fixedColumnWidth : leftMarginWidth;
}

void ViewStyle::ReleaseAllExtendedStyles() {
	nextExtendedStyle = 256;
}

int ViewStyle::AllocateExtendedStyles(int numberStyles) {
	int startRange = static_cast<int>(nextExtendedStyle);
	nextExtendedStyle += numberStyles;
	EnsureStyle(nextExtendedStyle);
	for (size_t i=startRange; i<nextExtendedStyle; i++) {
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
	for (unsigned int i=0; i<styles.size(); i++) {
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

bool ViewStyle::ProtectionActive() const {
	return someStylesProtected;
}

int ViewStyle::ExternalMarginWidth() const {
	return marginInside ? 0 : fixedColumnWidth;
}

bool ViewStyle::ValidStyle(size_t styleIndex) const {
	return styleIndex < styles.size();
}

void ViewStyle::CalcLargestMarkerHeight() {
	largestMarkerHeight = 0;
	for (int m = 0; m <= MARKER_MAX; ++m) {
		switch (markers[m].markType) {
		case SC_MARK_PIXMAP:
			if (markers[m].pxpm && markers[m].pxpm->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = markers[m].pxpm->GetHeight();
			break;
		case SC_MARK_RGBAIMAGE:
			if (markers[m].image && markers[m].image->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = markers[m].image->GetHeight();
			break;
		}
	}
}

// See if something overrides the line background color:  Either if caret is on the line
// and background color is set for that, or if a marker is defined that forces its background
// color onto the line, or if a marker is defined but has no selection margin in which to
// display itself (as long as it's not an SC_MARK_EMPTY marker).  These are checked in order
// with the earlier taking precedence.  When multiple markers cause background override,
// the color for the highest numbered one is used.
ColourOptional ViewStyle::Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const {
	ColourOptional background;
	if ((caretActive || alwaysShowCaretLineBackground) && showCaretLineBackground && (caretLineAlpha == SC_ALPHA_NOALPHA) && lineContainsCaret) {
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
				if ((marksMasked & 1) && (markers[markBit].markType != SC_MARK_EMPTY) &&
					(markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
					background = ColourOptional(markers[markBit].back, true);
				}
				marksMasked >>= 1;
			}
		}
	}
	return background;
}

bool ViewStyle::SelectionBackgroundDrawn() const {
	return selColours.back.isSet &&
		((selAlpha == SC_ALPHA_NOALPHA) || (selAdditionalAlpha == SC_ALPHA_NOALPHA));
}

bool ViewStyle::WhitespaceBackgroundDrawn() const {
	return (viewWhitespace != wsInvisible) && (whitespaceColours.back.isSet);
}

ColourDesired ViewStyle::WrapColour() const {
	if (whitespaceColours.fore.isSet)
		return whitespaceColours.fore;
	else
		return styles[STYLE_DEFAULT].fore;
}

bool ViewStyle::SetWrapState(int wrapState_) {
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
	bool changed = wrapState != wrapStateWanted;
	wrapState = wrapStateWanted;
	return changed;
}

bool ViewStyle::SetWrapVisualFlags(int wrapVisualFlags_) {
	bool changed = wrapVisualFlags != wrapVisualFlags_;
	wrapVisualFlags = wrapVisualFlags_;
	return changed;
}

bool ViewStyle::SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation_) {
	bool changed = wrapVisualFlagsLocation != wrapVisualFlagsLocation_;
	wrapVisualFlagsLocation = wrapVisualFlagsLocation_;
	return changed;
}

bool ViewStyle::SetWrapVisualStartIndent(int wrapVisualStartIndent_) {
	bool changed = wrapVisualStartIndent != wrapVisualStartIndent_;
	wrapVisualStartIndent = wrapVisualStartIndent_;
	return changed;
}

bool ViewStyle::SetWrapIndentMode(int wrapIndentMode_) {
	bool changed = wrapIndentMode != wrapIndentMode_;
	wrapIndentMode = wrapIndentMode_;
	return changed;
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
			fonts[fs] = new FontRealised();
		}
	}
}

FontRealised *ViewStyle::Find(const FontSpecification &fs) {
	if (!fs.fontName)	// Invalid specification so return arbitrary object
		return fonts.begin()->second;
	FontMap::iterator it = fonts.find(fs);
	if (it != fonts.end()) {
		// Should always reach here since map was just set for all styles
		return it->second;
	}
	return 0;
}

void ViewStyle::FindMaxAscentDescent() {
	for (FontMap::const_iterator it = fonts.begin(); it != fonts.end(); ++it) {
		if (maxAscent < it->second->ascent)
			maxAscent = it->second->ascent;
		if (maxDescent < it->second->descent)
			maxDescent = it->second->descent;
	}
}
