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

// Convert from a FontWeight value to a floating point value to pass to CoreText.
// This was defined based on Cocoa's NSFontWeight* values, discussion by other open
// source projects then tweaked until most values produced visibly different results.
inline double WeightFromEnumeration(Scintilla::FontWeight weight) {
	switch (static_cast<int>(weight)/100) {
		case 0: return -1.0;
		case 1: return -0.7;
		case 2: return -0.5;
		case 3: return -0.23;
		case 4: return 0.0;
		case 5: return 0.2;
		case 6: return 0.3;
		case 7: return 0.4;
		case 8: return 0.6;
		case 9: return 0.8;
	}
	return 0.0;
}

// Convert from a FontStretch value to a floating point value to pass to CoreText.
// This was defined based on values used by other open source projects then tweaked
// until most values produced reasonable results.
inline double StretchFromEnumeration(Scintilla::FontStretch stretch) {
	switch (stretch) {
		case Scintilla::FontStretch::UltraCondensed: return -0.8;
		case Scintilla::FontStretch::ExtraCondensed: return -0.3;
		case Scintilla::FontStretch::Condensed: return -0.23;
		case Scintilla::FontStretch::SemiCondensed: return -0.1;
		case Scintilla::FontStretch::Normal: return 0.0;
		case Scintilla::FontStretch::SemiExpanded: return 0.1;
		case Scintilla::FontStretch::Expanded: return 0.2;
		case Scintilla::FontStretch::ExtraExpanded: return 0.3;
		case Scintilla::FontStretch::UltraExpanded: return 0.7;
	}
}

class QuartzFont {
public:
	/** Create a font style from a name. */
	QuartzFont(const char *name, size_t length, float size, Scintilla::FontWeight weight, Scintilla::FontStretch stretch, bool italic) {
		assert(name != NULL && length > 0 && name[length] == '\0');

		CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingMacRoman);
		assert(fontName != NULL);

		// Specify the weight, stretch, and italics
		DictionaryForCF traits;
		const double weightValue = WeightFromEnumeration(weight);
		traits.SetItem(kCTFontWeightTrait, kCFNumberCGFloatType, &weightValue);
		const double stretchValue = StretchFromEnumeration(stretch);
		traits.SetItem(kCTFontWidthTrait, kCFNumberCGFloatType, &stretchValue);
		if (italic) {
			const int italicValue = kCTFontTraitItalic;
			traits.SetItem(kCTFontSymbolicTrait, kCFNumberIntType, &italicValue);
		}

		// create a font decriptor and then a font with that descriptor
		DictionaryForCF attributes;
		attributes.SetValue(kCTFontTraitsAttribute, traits.get());
		attributes.SetValue(kCTFontNameAttribute, fontName);

		CTFontDescriptorRef desc = ::CTFontDescriptorCreateWithAttributes(attributes.get());
		fontid = ::CTFontCreateWithFontDescriptor(desc, size, NULL);
		CFRelease(desc);
		if (!fontid) {
			// Traits failed so use base font
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

