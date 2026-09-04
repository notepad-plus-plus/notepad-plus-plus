/**
 * Scintilla source code edit control
 * @file DictionaryForCF.h - Wrapper for CFMutableDictionary
 *
 * Copyright 2024 Neil Hodgson.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#ifndef DICTIONARYFORCF_H
#define DICTIONARYFORCF_H

class DictionaryForCF {
	CFMutableDictionaryRef dict;
public:
	DictionaryForCF() noexcept :
	dict(::CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
					 &kCFTypeDictionaryKeyCallBacks,
					 &kCFTypeDictionaryValueCallBacks)) {
	}
	~DictionaryForCF() {
		::CFRelease(dict);
	}
	CFMutableDictionaryRef get() const noexcept {
		return dict;
	}
	void SetValue(const void *key, const void *value) noexcept {
		::CFDictionarySetValue(dict, key, value);
	}
	void SetItem(const void *key, CFNumberType theType, const void *valuePtr) noexcept {
		CFNumberRef number = ::CFNumberCreate(kCFAllocatorDefault, theType, valuePtr);
		::CFDictionarySetValue(dict, key, number);
		::CFRelease(number);
	}
};

#endif
