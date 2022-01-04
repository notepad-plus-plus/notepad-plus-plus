/**
 *  QuartzTextStyleAttribute.h
 *
 *  Original Code by Evan Jones on Wed Oct 02 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */


#ifndef QUARTZTEXTSTYLEATTRIBUTE_H
#define QUARTZTEXTSTYLEATTRIBUTE_H

class QuartzFont {
public:
	/** Create a font style from a name. */
	QuartzFont(const char *name, size_t length, float size, Scintilla::FontWeight weight, bool italic) {
		assert(name != NULL && length > 0 && name[length] == '\0');

		CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingMacRoman);
		assert(fontName != NULL);
		bool bold = weight > Scintilla::FontWeight::Normal;

		if (bold || italic) {
			CTFontSymbolicTraits desiredTrait = 0;
			CTFontSymbolicTraits traitMask = 0;

			// if bold was specified, add the trait
			if (bold) {
				desiredTrait |= kCTFontBoldTrait;
				traitMask |= kCTFontBoldTrait;
			}

			// if italic was specified, add the trait
			if (italic) {
				desiredTrait |= kCTFontItalicTrait;
				traitMask |= kCTFontItalicTrait;
			}

			// create a font and then a copy of it with the sym traits
			CTFontRef iFont = ::CTFontCreateWithName(fontName, size, NULL);
			fontid = ::CTFontCreateCopyWithSymbolicTraits(iFont, size, NULL, desiredTrait, traitMask);
			if (fontid) {
				CFRelease(iFont);
			} else {
				// Traits failed so use base font
				fontid = iFont;
			}
		} else {
			// create the font, no traits
			fontid = ::CTFontCreateWithName(fontName, size, NULL);
		}

		if (!fontid) {
			// Failed to create requested font so use font always present
			fontid = ::CTFontCreateWithName((CFStringRef)@"Monaco", size, NULL);
		}

		CFRelease(fontName);
	}

	CTFontRef getFontID() {
		return fontid;
	}

private:
	CTFontRef fontid;
};

#endif

