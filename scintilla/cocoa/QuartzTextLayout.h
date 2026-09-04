/*
 *  QuartzTextLayout.h
 *
 *  Original Code by Evan Jones on Wed Oct 02 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */

#ifndef QUARTZTEXTLAYOUT_H
#define QUARTZTEXTLAYOUT_H

#include <Cocoa/Cocoa.h>

#include "QuartzTextStyle.h"


class QuartzTextLayout {
public:
	/** Create a text layout for drawing. */
	QuartzTextLayout(std::string_view sv, CFStringEncoding encoding, const QuartzTextStyle *r) {
		encodingUsed = encoding;
		const UInt8 *puiBuffer = reinterpret_cast<const UInt8 *>(sv.data());
		CFStringRef str = CFStringCreateWithBytes(NULL, puiBuffer, sv.length(), encodingUsed, false);
		if (!str) {
			// Failed to decode bytes into string with given encoding so try
			// MacRoman which should accept any byte.
			encodingUsed = kCFStringEncodingMacRoman;
			str = CFStringCreateWithBytes(NULL, puiBuffer, sv.length(), encodingUsed, false);
		}
		if (!str) {
			return;
		}

		stringLength = CFStringGetLength(str);

		CFMutableDictionaryRef stringAttribs = r->getCTStyle();

		mString = ::CFAttributedStringCreate(NULL, str, stringAttribs);

		mLine = ::CTLineCreateWithAttributedString(mString);

		CFRelease(str);
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

	/** Draw the text layout into a CGContext at the specified position.
	* @param gc The CGContext in which to draw the text.
	* @param x The x axis position to draw the baseline in the current CGContext.
	* @param y The y axis position to draw the baseline in the current CGContext. */
	void draw(CGContextRef gc, double x, double y) {
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

	CFStringEncoding getEncoding() {
		return encodingUsed;
	}

private:
	CFAttributedStringRef mString = NULL;
	CTLineRef mLine = NULL;
	CFIndex stringLength = 0;
	CFStringEncoding encodingUsed = kCFStringEncodingMacRoman;
};

#endif
