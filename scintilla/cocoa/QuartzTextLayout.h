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


class QuartzTextLayout {
public:
	/** Create a text layout for drawing. */
	QuartzTextLayout() : mString(NULL), mLine(NULL), stringLength(0) {
	}

	~QuartzTextLayout() {
		if (mString) {
			CFRelease(mString);
			mString = NULL;
		}
		if (mLine) {
			CFRelease(mLine);
			mLine = NULL;
		}
	}

	CFStringEncoding setText(std::string_view sv, CFStringEncoding encoding, const QuartzTextStyle &r) {
		// First clear current values in case of failure.
		if (mString) {
			CFRelease(mString);
			mString = NULL;
		}
		if (mLine) {
			CFRelease(mLine);
			mLine = NULL;
		}

		const UInt8 *puiBuffer = reinterpret_cast<const UInt8 *>(sv.data());
		CFStringRef str = CFStringCreateWithBytes(NULL, puiBuffer, sv.length(), encoding, false);
		if (!str) {
			// Failed to decode bytes into string with given encoding so try
			// MacRoman which should accept any byte.
			encoding = kCFStringEncodingMacRoman;
			str = CFStringCreateWithBytes(NULL, puiBuffer, sv.length(), encoding, false);
		}
		if (!str) {
			return encoding;
		}

		stringLength = CFStringGetLength(str);

		CFMutableDictionaryRef stringAttribs = r.getCTStyle();

		mString = ::CFAttributedStringCreate(NULL, str, stringAttribs);

		mLine = ::CTLineCreateWithAttributedString(mString);

		CFRelease(str);
		return encoding;
	}

	/** Draw the text layout into a CGContext at the specified position.
	* @param gc The CGContext in which to draw the text.
	* @param x The x axis position to draw the baseline in the current CGContext.
	* @param y The y axis position to draw the baseline in the current CGContext. */
	void draw(CGContextRef gc, float x, float y) {
		if (!mLine)
			return;

		::CGContextSetTextMatrix(gc, CGAffineTransformMakeScale(1.0, -1.0));

		// Set the text drawing position.
		::CGContextSetTextPosition(gc, x, y);

		// And finally, draw!
		::CTLineDraw(mLine, gc);
	}

	float MeasureStringWidth() {
		if (mLine == NULL)
			return 0.0f;

		return static_cast<float>(::CTLineGetTypographicBounds(mLine, NULL, NULL, NULL));
	}

	CTLineRef getCTLine() {
		return mLine;
	}

	CFIndex getStringLength() {
		return stringLength;
	}

private:
	CFAttributedStringRef mString;
	CTLineRef mLine;
	CFIndex stringLength;
};

#endif
