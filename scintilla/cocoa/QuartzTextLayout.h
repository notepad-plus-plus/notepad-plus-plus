/*
 *  QuartzTextLayout.h
 *
 *  Original Code by Evan Jones on Wed Oct 02 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */

#ifndef _QUARTZ_TEXT_LAYOUT_H
#define _QUARTZ_TEXT_LAYOUT_H

#include <Cocoa/Cocoa.h>

#include "QuartzTextStyle.h"


class QuartzTextLayout
{
public:
    /** Create a text layout for drawing on the specified context. */
    QuartzTextLayout( CGContextRef context )
    {
		mString = NULL;
		mLine = NULL;
		stringLength = 0;
        setContext(context);
    }

    ~QuartzTextLayout()
    {
		if ( mString != NULL )
		{
			CFRelease(mString);
			mString = NULL;
		}
		if ( mLine != NULL )
		{
			CFRelease(mLine);
			mLine = NULL;
		}	
    }

    inline void setText( const UInt8* buffer, size_t byteLength, CFStringEncoding encoding, const QuartzTextStyle& r )
    {
		CFStringRef str = CFStringCreateWithBytes( NULL, buffer, byteLength, encoding, false );
        if (!str)
            return;
		
	        stringLength = CFStringGetLength(str);

		CFMutableDictionaryRef stringAttribs = r.getCTStyle();
		
		if (mString != NULL)
			CFRelease(mString);
		mString = ::CFAttributedStringCreate(NULL, str, stringAttribs);
		
		if (mLine != NULL)
			CFRelease(mLine);
		mLine = ::CTLineCreateWithAttributedString(mString);
		
		CFRelease( str );
    }

    /** Draw the text layout into the current CGContext at the specified position.
    * @param x The x axis position to draw the baseline in the current CGContext.
    * @param y The y axis position to draw the baseline in the current CGContext. */
    void draw( float x, float y )
    {
		if (mLine == NULL)
			return;
		
		::CGContextSetTextMatrix(gc, CGAffineTransformMakeScale(1.0, -1.0));
		
		// Set the text drawing position.
		::CGContextSetTextPosition(gc, x, y);
		
		// And finally, draw!
		::CTLineDraw(mLine, gc);
    }
	
	float MeasureStringWidth()
	{		
		if (mLine == NULL)
			return 0.0f;
		
		return ::CTLineGetTypographicBounds(mLine, NULL, NULL, NULL);
	}
	
    CTLineRef getCTLine() {
        return mLine;
    }
	
    CFIndex getStringLength() {
	    return stringLength;
    }

    inline void setContext (CGContextRef context)
    {
        gc = context;
    }

private:
    CGContextRef gc;
	CFAttributedStringRef mString;
	CTLineRef mLine;
	CFIndex stringLength;
};

#endif
