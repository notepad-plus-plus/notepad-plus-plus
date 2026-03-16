// string_utils.h — wchar_t <-> NSString conversion helpers
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#import <Foundation/Foundation.h>
#include <string>
#include <cwchar>

inline NSString* WideToNSString(const wchar_t* wstr)
{
	if (!wstr) return @"";
	size_t len = wcslen(wstr);
	NSString* str = [[NSString alloc] initWithBytes:wstr
	                                         length:len * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ?: @"";
}

inline std::wstring NSStringToWide(NSString* str)
{
	if (!str) return L"";
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	if (!data || data.length == 0) return L"";
	return std::wstring(reinterpret_cast<const wchar_t*>(data.bytes),
	                    data.length / sizeof(wchar_t));
}
