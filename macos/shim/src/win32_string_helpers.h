#pragma once
// Shared string conversion helpers for Win32 shim modules.
// wchar_t is UTF-32 on macOS; these convert to/from NSString.

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#include <cstring>

static inline NSString* WideToNS(const wchar_t* wstr)
{
	if (!wstr) return @"";
	size_t len = wcslen(wstr);
	NSString* s = [[NSString alloc] initWithBytes:wstr
	                                       length:len * sizeof(wchar_t)
	                                     encoding:NSUTF32LittleEndianStringEncoding];
	return s ?: @"";
}

static inline void NSStringToWide(NSString* str, wchar_t* buffer, size_t maxChars)
{
	if (!buffer || maxChars == 0) return;
	if (!str || str.length == 0)
	{
		buffer[0] = L'\0';
		return;
	}
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	size_t charCount = data.length / sizeof(wchar_t);
	size_t copyLen = (charCount < maxChars - 1) ? charCount : maxChars - 1;
	memcpy(buffer, data.bytes, copyLen * sizeof(wchar_t));
	buffer[copyLen] = L'\0';
}

#endif // __OBJC__
