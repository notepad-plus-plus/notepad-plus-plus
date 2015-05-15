/*
 *  QuartzTextStyle.h
 *
 *  Created by Evan Jones on Wed Oct 02 2002.
 *
 */

#ifndef _QUARTZ_TEXT_STYLE_H
#define _QUARTZ_TEXT_STYLE_H

#include "QuartzTextStyleAttribute.h"

class QuartzTextStyle
{
public:
	QuartzTextStyle()
	{
		fontRef = NULL;
		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
		        &kCFTypeDictionaryKeyCallBacks,
		        &kCFTypeDictionaryValueCallBacks);

		characterSet = 0;
	}

	QuartzTextStyle(const QuartzTextStyle &other)
	{
		// Does not copy font colour attribute
		fontRef = static_cast<CTFontRef>(CFRetain(other.fontRef));
		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
		        &kCFTypeDictionaryKeyCallBacks,
		        &kCFTypeDictionaryValueCallBacks);
		CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);
		characterSet = other.characterSet;
	}

	~QuartzTextStyle()
	{
		if (styleDict != NULL)
		{
			CFRelease(styleDict);
			styleDict = NULL;
		}

		if (fontRef)
		{
			CFRelease(fontRef);
			fontRef = NULL;
		}
	}

	CFMutableDictionaryRef getCTStyle() const
	{
		return styleDict;
	}

	void setCTStyleColor(CGColor *inColor)
	{
		CFDictionarySetValue(styleDict, kCTForegroundColorAttributeName, inColor);
	}

	float getAscent() const
	{
		return ::CTFontGetAscent(fontRef);
	}

	float getDescent() const
	{
		return ::CTFontGetDescent(fontRef);
	}

	float getLeading() const
	{
		return ::CTFontGetLeading(fontRef);
	}

	void setFontRef(CTFontRef inRef, int characterSet_)
	{
		fontRef = inRef;
		characterSet = characterSet_;

		if (styleDict != NULL)
			CFRelease(styleDict);

		styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
		        &kCFTypeDictionaryKeyCallBacks,
		        &kCFTypeDictionaryValueCallBacks);

		CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);
	}

	CTFontRef getFontRef()
	{
		return fontRef;
	}

	int getCharacterSet()
	{
		return characterSet;
	}

private:
	CFMutableDictionaryRef styleDict;
	CTFontRef fontRef;
	int characterSet;
};

#endif

