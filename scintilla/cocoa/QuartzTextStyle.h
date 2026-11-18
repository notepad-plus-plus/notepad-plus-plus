/*
 *  QuartzTextStyle.h
 *
 *  Created by Evan Jones on Wed Oct 02 2002.
 *
 */

#ifndef QUARTZTEXTSTYLE_H
#define QUARTZTEXTSTYLE_H

#include "QuartzTextStyleAttribute.h"

class QuartzTextStyle {
public:
	QuartzTextStyle() {
		fontRef = NULL;
		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
						      &kCFTypeDictionaryKeyCallBacks,
						      &kCFTypeDictionaryValueCallBacks);

		characterSet = Scintilla::CharacterSet::Ansi;
	}

	QuartzTextStyle(const QuartzTextStyle *other) {
		// Does not copy font colour attribute
		fontRef = static_cast<CTFontRef>(CFRetain(other->fontRef));
		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
						      &kCFTypeDictionaryKeyCallBacks,
						      &kCFTypeDictionaryValueCallBacks);
		CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);
		characterSet = other->characterSet;
	}

	~QuartzTextStyle() {
		if (styleDict != NULL) {
			CFRelease(styleDict);
			styleDict = NULL;
		}

		if (fontRef) {
			CFRelease(fontRef);
			fontRef = NULL;
		}
	}

	CFMutableDictionaryRef getCTStyle() const {
		return styleDict;
	}

	void setCTStyleColour(CGColor *inColour) {
		CFDictionarySetValue(styleDict, kCTForegroundColorAttributeName, inColour);
	}

	float getAscent() const {
		return static_cast<float>(::CTFontGetAscent(fontRef));
	}

	float getDescent() const {
		return static_cast<float>(::CTFontGetDescent(fontRef));
	}

	float getLeading() const {
		return static_cast<float>(::CTFontGetLeading(fontRef));
	}

	void setFontRef(CTFontRef inRef, Scintilla::CharacterSet characterSet_) {
		fontRef = inRef;
		characterSet = characterSet_;

		if (styleDict != NULL)
			CFRelease(styleDict);

		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
						      &kCFTypeDictionaryKeyCallBacks,
						      &kCFTypeDictionaryValueCallBacks);

		CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);
	}

	CTFontRef getFontRef() const noexcept {
		return fontRef;
	}

	Scintilla::CharacterSet getCharacterSet() const noexcept {
		return characterSet;
	}

private:
	CFMutableDictionaryRef styleDict;
	CTFontRef fontRef;
	Scintilla::CharacterSet characterSet;
};

#endif

