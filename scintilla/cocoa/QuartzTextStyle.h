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
		styleDict = CFDictionaryCreateMutable(NULL, 1, NULL, NULL);
    }

    ~QuartzTextStyle()
    {
		if (styleDict != NULL)
		{
			CFRelease(styleDict);
			styleDict = NULL;
		}
    }
	
	CFMutableDictionaryRef getCTStyle() const
	{
		return styleDict;
	}
	 
	void setCTStyleColor(CGColor* inColor )
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
	
	void setFontRef(CTFontRef inRef)
	{
		fontRef = inRef;
		
		if (styleDict != NULL)
			CFRelease(styleDict);

		styleDict = CFDictionaryCreateMutable(NULL, 1, NULL, NULL);
		
		CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);
	}
	
	CTFontRef getFontRef()
	{
		return fontRef;
	}
	
private:
	CFMutableDictionaryRef styleDict;
	CTFontRef fontRef;
};

#endif

