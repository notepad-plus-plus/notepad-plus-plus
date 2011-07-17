// Scintilla source code edit control
/** @file ViewStyle.cxx
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

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
	size = 8;
	names = new char *[size];
	max = 0;
}

FontNames::~FontNames() {
	Clear();
	delete []names;
	names = 0;
}

void FontNames::Clear() {
	for (int i=0; i<max; i++) {
		delete []names[i];
	}
	max = 0;
}

const char *FontNames::Save(const char *name) {
	if (!name)
		return 0;
	for (int i=0; i<max; i++) {
		if (strcmp(names[i], name) == 0) {
			return names[i];
		}
	}
	if (max >= size) {
		// Grow array
		int sizeNew = size * 2;
		char **namesNew = new char *[sizeNew];
		for (int j=0; j<max; j++) {
			namesNew[j] = names[j];
		}
		delete []names;
		names = namesNew;
		size = sizeNew;
	}
	names[max] = new char[strlen(name) + 1];
	strcpy(names[max], name);
	max++;
	return names[max-1];
}

FontRealised::FontRealised(const FontSpecification &fs) {
	frNext = NULL;
	(FontSpecification &)(*this) = fs;
}

FontRealised::~FontRealised() {
	font.Release();
	delete frNext;
	frNext = 0;
}

void FontRealised::Realise(Surface &surface, int zoomLevel) {
	PLATFORM_ASSERT(fontName);
	sizeZoomed = size + zoomLevel;
	if (sizeZoomed <= 2)	// Hangs if sizeZoomed <= 1
		sizeZoomed = 2;

	int deviceHeight = surface.DeviceHeightFont(sizeZoomed);
	font.Create(fontName, characterSet, deviceHeight, bold, italic, extraFontFlag);

	ascent = surface.Ascent(font);
	descent = surface.Descent(font);
	externalLeading = surface.ExternalLeading(font);
	lineHeight = surface.Height(font);
	aveCharWidth = surface.AverageCharWidth(font);
	spaceWidth = surface.WidthChar(font, ' ');
	if (frNext) {
		frNext->Realise(surface, zoomLevel);
	}
}

FontRealised *FontRealised::Find(const FontSpecification &fs) {
	if (!fs.fontName)
		return this;
	FontRealised *fr = this;
	while (fr) {
		if (fr->EqualTo(fs))
			return fr;
		fr = fr->frNext;
	}
	return 0;
}

void FontRealised::FindMaxAscentDescent(unsigned int &maxAscent, unsigned int &maxDescent) {
	FontRealised *fr = this;
	while (fr) {
		if (maxAscent < fr->ascent)
			maxAscent = fr->ascent;
		if (maxDescent < fr->descent)
			maxDescent = fr->descent;
		fr = fr->frNext;
	}
}

ViewStyle::ViewStyle() {
	Init();
}

ViewStyle::ViewStyle(const ViewStyle &source) {
	frFirst = NULL;
	Init(source.stylesSize);
	for (unsigned int sty=0; sty<source.stylesSize; sty++) {
		styles[sty] = source.styles[sty];
		// Can't just copy fontname as its lifetime is relative to its owning ViewStyle
		styles[sty].fontName = fontNames.Save(source.styles[sty].fontName);
	}
	for (int mrk=0; mrk<=MARKER_MAX; mrk++) {
		markers[mrk] = source.markers[mrk];
	}
	for (int ind=0; ind<=INDIC_MAX; ind++) {
		indicators[ind] = source.indicators[ind];
	}

	selforeset = source.selforeset;
	selforeground.desired = source.selforeground.desired;
	selAdditionalForeground.desired = source.selAdditionalForeground.desired;
	selbackset = source.selbackset;
	selbackground.desired = source.selbackground.desired;
	selAdditionalBackground.desired = source.selAdditionalBackground.desired;
	selbackground2.desired = source.selbackground2.desired;
	selAlpha = source.selAlpha;
	selAdditionalAlpha = source.selAdditionalAlpha;
	selEOLFilled = source.selEOLFilled;

	foldmarginColourSet = source.foldmarginColourSet;
	foldmarginColour.desired = source.foldmarginColour.desired;
	foldmarginHighlightColourSet = source.foldmarginHighlightColourSet;
	foldmarginHighlightColour.desired = source.foldmarginHighlightColour.desired;

	hotspotForegroundSet = source.hotspotForegroundSet;
	hotspotForeground.desired = source.hotspotForeground.desired;
	hotspotBackgroundSet = source.hotspotBackgroundSet;
	hotspotBackground.desired = source.hotspotBackground.desired;
	hotspotUnderline = source.hotspotUnderline;
	hotspotSingleLine = source.hotspotSingleLine;

	whitespaceForegroundSet = source.whitespaceForegroundSet;
	whitespaceForeground.desired = source.whitespaceForeground.desired;
	whitespaceBackgroundSet = source.whitespaceBackgroundSet;
	whitespaceBackground.desired = source.whitespaceBackground.desired;
	selbar.desired = source.selbar.desired;
	selbarlight.desired = source.selbarlight.desired;
	caretcolour.desired = source.caretcolour.desired;
	additionalCaretColour.desired = source.additionalCaretColour.desired;
	showCaretLineBackground = source.showCaretLineBackground;
	showCaretLineBackgroundAlways = source.showCaretLineBackgroundAlways;
	caretLineBackground.desired = source.caretLineBackground.desired;
	caretLineAlpha = source.caretLineAlpha;
	edgecolour.desired = source.edgecolour.desired;
	edgeState = source.edgeState;
	caretStyle = source.caretStyle;
	caretWidth = source.caretWidth;
	someStylesProtected = false;
	someStylesForceCase = false;
	leftMarginWidth = source.leftMarginWidth;
	rightMarginWidth = source.rightMarginWidth;
	for (int i=0; i < margins; i++) {
		ms[i] = source.ms[i];
	}
	symbolMargin = source.symbolMargin;
	maskInLine = source.maskInLine;
	fixedColumnWidth = source.fixedColumnWidth;
	zoomLevel = source.zoomLevel;
	viewWhitespace = source.viewWhitespace;
	whitespaceSize = source.whitespaceSize;
	viewIndentationGuides = source.viewIndentationGuides;
	viewEOL = source.viewEOL;
	showMarkedLines = source.showMarkedLines;
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
}

ViewStyle::~ViewStyle() {
	delete []styles;
	styles = NULL;
	delete frFirst;
	frFirst = NULL;
}

void ViewStyle::Init(size_t stylesSize_) {
	frFirst = NULL;
	stylesSize = 0;
	styles = NULL;
	AllocStyles(stylesSize_);
	fontNames.Clear();
	ResetDefaultStyle();

	indicators[0].style = INDIC_SQUIGGLE;
	indicators[0].under = false;
	indicators[0].fore = ColourDesired(0, 0x7f, 0);
	indicators[1].style = INDIC_TT;
	indicators[1].under = false;
	indicators[1].fore = ColourDesired(0, 0, 0xff);
	indicators[2].style = INDIC_PLAIN;
	indicators[2].under = false;
	indicators[2].fore = ColourDesired(0xff, 0, 0);

	lineHeight = 1;
	maxAscent = 1;
	maxDescent = 1;
	aveCharWidth = 8;
	spaceWidth = 8;

	selforeset = false;
	selforeground.desired = ColourDesired(0xff, 0, 0);
	selAdditionalForeground.desired = ColourDesired(0xff, 0, 0);
	selbackset = true;
	selbackground.desired = ColourDesired(0xc0, 0xc0, 0xc0);
	selAdditionalBackground.desired = ColourDesired(0xd7, 0xd7, 0xd7);
	selbackground2.desired = ColourDesired(0xb0, 0xb0, 0xb0);
	selAlpha = SC_ALPHA_NOALPHA;
	selAdditionalAlpha = SC_ALPHA_NOALPHA;
	selEOLFilled = false;

	foldmarginColourSet = false;
	foldmarginColour.desired = ColourDesired(0xff, 0, 0);
	foldmarginHighlightColourSet = false;
	foldmarginHighlightColour.desired = ColourDesired(0xc0, 0xc0, 0xc0);

	whitespaceForegroundSet = false;
	whitespaceForeground.desired = ColourDesired(0, 0, 0);
	whitespaceBackgroundSet = false;
	whitespaceBackground.desired = ColourDesired(0xff, 0xff, 0xff);
	selbar.desired = Platform::Chrome();
	selbarlight.desired = Platform::ChromeHighlight();
	styles[STYLE_LINENUMBER].fore.desired = ColourDesired(0, 0, 0);
	styles[STYLE_LINENUMBER].back.desired = Platform::Chrome();
	caretcolour.desired = ColourDesired(0, 0, 0);
	additionalCaretColour.desired = ColourDesired(0x7f, 0x7f, 0x7f);
	showCaretLineBackground = false;
	showCaretLineBackgroundAlways = false;
	caretLineBackground.desired = ColourDesired(0xff, 0xff, 0);
	caretLineAlpha = SC_ALPHA_NOALPHA;
	edgecolour.desired = ColourDesired(0xc0, 0xc0, 0xc0);
	edgeState = EDGE_NONE;
	caretStyle = CARETSTYLE_LINE;
	caretWidth = 1;
	someStylesProtected = false;
	someStylesForceCase = false;

	hotspotForegroundSet = false;
	hotspotForeground.desired = ColourDesired(0, 0, 0xff);
	hotspotBackgroundSet = false;
	hotspotBackground.desired = ColourDesired(0xff, 0xff, 0xff);
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
	fixedColumnWidth = leftMarginWidth;
	symbolMargin = false;
	maskInLine = 0xffffffff;
	for (int margin=0; margin < margins; margin++) {
		fixedColumnWidth += ms[margin].width;
		symbolMargin = symbolMargin || (ms[margin].style != SC_MARGIN_NUMBER);
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
	zoomLevel = 0;
	viewWhitespace = wsInvisible;
	whitespaceSize = 1;
	viewIndentationGuides = ivNone;
	viewEOL = false;
	showMarkedLines = true;
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
}

void ViewStyle::RefreshColourPalette(Palette &pal, bool want) {
	unsigned int i;
	for (i=0; i<stylesSize; i++) {
		pal.WantFind(styles[i].fore, want);
		pal.WantFind(styles[i].back, want);
	}
	for (i=0; i<(sizeof(indicators)/sizeof(indicators[0])); i++) {
		pal.WantFind(indicators[i].fore, want);
	}
	for (i=0; i<(sizeof(markers)/sizeof(markers[0])); i++) {
		markers[i].RefreshColourPalette(pal, want);
	}
	pal.WantFind(selforeground, want);
	pal.WantFind(selAdditionalForeground, want);
	pal.WantFind(selbackground, want);
	pal.WantFind(selAdditionalBackground, want);
	pal.WantFind(selbackground2, want);

	pal.WantFind(foldmarginColour, want);
	pal.WantFind(foldmarginHighlightColour, want);

	pal.WantFind(whitespaceForeground, want);
	pal.WantFind(whitespaceBackground, want);
	pal.WantFind(selbar, want);
	pal.WantFind(selbarlight, want);
	pal.WantFind(caretcolour, want);
	pal.WantFind(additionalCaretColour, want);
	pal.WantFind(caretLineBackground, want);
	pal.WantFind(edgecolour, want);
	pal.WantFind(hotspotForeground, want);
	pal.WantFind(hotspotBackground, want);
}

void ViewStyle::CreateFont(const FontSpecification &fs) {
	if (fs.fontName) {
		for (FontRealised *cur=frFirst; cur; cur=cur->frNext) {
			if (cur->EqualTo(fs))
				return;
			if (!cur->frNext) {
				cur->frNext = new FontRealised(fs);
				return;
			}
		}
		frFirst = new FontRealised(fs);
	}
}

void ViewStyle::Refresh(Surface &surface) {
	delete frFirst;
	frFirst = NULL;
	selbar.desired = Platform::Chrome();
	selbarlight.desired = Platform::ChromeHighlight();

	for (unsigned int i=0; i<stylesSize; i++) {
		styles[i].extraFontFlag = extraFontFlag;
	}

	CreateFont(styles[STYLE_DEFAULT]);
	for (unsigned int j=0; j<stylesSize; j++) {
		CreateFont(styles[j]);
	}

	frFirst->Realise(surface, zoomLevel);

	for (unsigned int k=0; k<stylesSize; k++) {
		FontRealised *fr = frFirst->Find(styles[k]);
		styles[k].Copy(fr->font, *fr);
	}
	maxAscent = 1;
	maxDescent = 1;
	frFirst->FindMaxAscentDescent(maxAscent, maxDescent);
	maxAscent += extraAscent;
	maxDescent += extraDescent;
	lineHeight = maxAscent + maxDescent;

	someStylesProtected = false;
	someStylesForceCase = false;
	for (unsigned int l=0; l<stylesSize; l++) {
		if (styles[l].IsProtected()) {
			someStylesProtected = true;
		}
		if (styles[l].caseForce != Style::caseMixed) {
			someStylesForceCase = true;
		}
	}

	aveCharWidth = styles[STYLE_DEFAULT].aveCharWidth;
	spaceWidth = styles[STYLE_DEFAULT].spaceWidth;

	fixedColumnWidth = leftMarginWidth;
	symbolMargin = false;
	maskInLine = 0xffffffff;
	for (int margin=0; margin < margins; margin++) {
		fixedColumnWidth += ms[margin].width;
		symbolMargin = symbolMargin || (ms[margin].style != SC_MARGIN_NUMBER);
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
}

void ViewStyle::AllocStyles(size_t sizeNew) {
	Style *stylesNew = new Style[sizeNew];
	size_t i=0;
	for (; i<stylesSize; i++) {
		stylesNew[i] = styles[i];
		stylesNew[i].fontName = styles[i].fontName;
	}
	if (stylesSize > STYLE_DEFAULT) {
		for (; i<sizeNew; i++) {
			if (i != STYLE_DEFAULT) {
				stylesNew[i].ClearTo(styles[STYLE_DEFAULT]);
			}
		}
	}
	delete []styles;
	styles = stylesNew;
	stylesSize = sizeNew;
}

void ViewStyle::EnsureStyle(size_t index) {
	if (index >= stylesSize) {
		size_t sizeNew = stylesSize * 2;
		while (sizeNew <= index)
			sizeNew *= 2;
		AllocStyles(sizeNew);
	}
}

void ViewStyle::ResetDefaultStyle() {
	styles[STYLE_DEFAULT].Clear(ColourDesired(0,0,0),
	        ColourDesired(0xff,0xff,0xff),
	        Platform::DefaultFontSize(), fontNames.Save(Platform::DefaultFont()),
	        SC_CHARSET_DEFAULT,
	        false, false, false, false, Style::caseMixed, true, true, false);
}

void ViewStyle::ClearStyles() {
	// Reset all styles to be like the default style
	for (unsigned int i=0; i<stylesSize; i++) {
		if (i != STYLE_DEFAULT) {
			styles[i].ClearTo(styles[STYLE_DEFAULT]);
		}
	}
	styles[STYLE_LINENUMBER].back.desired = Platform::Chrome();

	// Set call tip fore/back to match the values previously set for call tips
	styles[STYLE_CALLTIP].back.desired = ColourDesired(0xff, 0xff, 0xff);
	styles[STYLE_CALLTIP].fore.desired = ColourDesired(0x80, 0x80, 0x80);
}

void ViewStyle::SetStyleFontName(int styleIndex, const char *name) {
	styles[styleIndex].fontName = fontNames.Save(name);
}

bool ViewStyle::ProtectionActive() const {
	return someStylesProtected;
}

bool ViewStyle::ValidStyle(size_t styleIndex) const {
	return styleIndex < stylesSize;
}

