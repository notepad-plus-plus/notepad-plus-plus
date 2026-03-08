#pragma once
// Win32 Shim: TCHAR definitions for macOS
// Always Unicode (wide char) build

#include <cwchar>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef wchar_t TCHAR;
typedef wchar_t _TCHAR;
typedef wchar_t _TUCHAR;
#endif

typedef wchar_t* LPTSTR;
typedef const wchar_t* LPCTSTR;

#ifndef _T
#define _T(x) L##x
#endif

#ifndef TEXT
#define TEXT(x) L##x
#endif

#define _tcslen     wcslen
#define _tcscpy     wcscpy
#define _tcsncpy    wcsncpy
#define _tcscat     wcscat
#define _tcscmp     wcscmp
#define _tcsicmp    wcscasecmp
#define _tcsncmp    wcsncmp
#define _tcsnicmp   wcsncasecmp
#define _tcschr     wcschr
#define _tcsrchr    wcsrchr
#define _tcsstr     wcsstr
#define _tcstol     wcstol
#define _tcstoul    wcstoul
#define _tcstod     wcstod
#define _tcstok     wcstok
#define _tcsdup     wcsdup
#define _tcspbrk    wcspbrk
#define _tcsspn     wcsspn
#define _tcscspn    wcscspn
#define _tcstoi     _wtoi

#define _tprintf    wprintf
#define _ftprintf   fwprintf
#define _stprintf   swprintf
#define _stprintf_s swprintf
#define _sntprintf  swprintf
#define _vstprintf  vswprintf
#define _vsntprintf vswprintf

#define _tfopen     _wfopen
#define _topen      _wopen
#define _tremove    _wremove
#define _trename    _wrename

#define _ttoi       _wtoi
#define _ttol       wcstol
#define _ttof(s)    wcstod(s, nullptr)

#define _istalpha   iswalpha
#define _istdigit   iswdigit
#define _istalnum   iswalnum
#define _istspace   iswspace
#define _istupper   iswupper
#define _istlower   iswlower
#define _totlower   towlower
#define _totupper   towupper

// _stscanf → swscanf
#define _stscanf    swscanf

// _tcprintf → wprintf (console print)
#define _tcprintf   wprintf

// Non-standard MSVC functions
inline int _wtoi(const wchar_t* str)
{
	return static_cast<int>(wcstol(str, nullptr, 10));
}

inline long _wtol(const wchar_t* str)
{
	return wcstol(str, nullptr, 10);
}

inline long long _wtoi64(const wchar_t* str)
{
	return wcstoll(str, nullptr, 10);
}

// _itow
inline wchar_t* _itow(int value, wchar_t* str, int radix)
{
	if (radix == 10) {
		swprintf(str, 34, L"%d", value);
	} else if (radix == 16) {
		swprintf(str, 34, L"%x", value);
	} else if (radix == 8) {
		swprintf(str, 34, L"%o", value);
	} else {
		str[0] = L'\0';
	}
	return str;
}

// _i64tow
inline wchar_t* _i64tow(long long value, wchar_t* str, int radix)
{
	if (radix == 10) {
		swprintf(str, 66, L"%lld", value);
	} else if (radix == 16) {
		swprintf(str, 66, L"%llx", value);
	} else {
		str[0] = L'\0';
	}
	return str;
}

// _itoa, _ltoa, _ultoa are defined in winbase.h (included via windows.h)
// Only define here if winbase.h hasn't been included
#ifndef _WIN32_BOOL_WINBASE_INCLUDED
// These are available via winbase.h
#endif

// String formatting safety
#define _vsntprintf_s(buf, size, count, fmt, args) vswprintf(buf, size, fmt, args)
#define _sntprintf_s(buf, size, count, fmt, ...) swprintf(buf, size, fmt, ##__VA_ARGS__)
#define _tcscpy_s(dst, size, src) wcsncpy(dst, src, size)
#define _tcsncpy_s(dst, size, src, count) wcsncpy(dst, src, (count) < (size) ? (count) : (size) - 1)
#define _tcscat_s(dst, size, src) wcsncat(dst, src, (size) - wcslen(dst) - 1)

// MSVC-specific secure string functions
// wcsncpy_s is provided as inline overloads in winbase.h (3-arg and 4-arg)
// Other secure CRT functions as macros:
#define strcpy_s(dst, size, src) strncpy(dst, src, size)
#define strncpy_s(dst, size, src, count) strncpy(dst, src, (count) < (size) ? (count) : (size) - 1)
#define strcat_s(dst, size, src) strncat(dst, src, (size) - strlen(dst) - 1)
#define sprintf_s snprintf
// swprintf_s: 3-arg form swprintf_s(buf, fmt, ...) needs size deduction
// Keep the n-arg form as swprintf, plus provide a variadic template for array form
#include <cstdarg>
template<size_t N>
inline int swprintf_s(wchar_t (&buf)[N], const wchar_t* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int r = vswprintf(buf, N, fmt, args);
	va_end(args);
	return r;
}
// Also support swprintf_s(buf, count, fmt, ...) - explicit size form
inline int swprintf_s(wchar_t* buf, size_t count, const wchar_t* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int r = vswprintf(buf, count, fmt, args);
	va_end(args);
	return r;
}
#define _snprintf snprintf
#define _snwprintf swprintf
#define _vsnprintf vsnprintf
#define _vsnwprintf vswprintf
#define sscanf_s sscanf

// memcpy_s / memmove_s
#include <cstring>
#include <cerrno>
inline int memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
	if (!dest || count > destsz) return EINVAL;
	if (!src) { memset(dest, 0, destsz); return EINVAL; }
	memcpy(dest, src, count);
	return 0;
}
inline int memmove_s(void* dest, size_t destsz, const void* src, size_t count) {
	if (!dest || count > destsz) return EINVAL;
	memmove(dest, src, count);
	return 0;
}

// itoa / _itoa are defined in winbase.h
#ifndef itoa
#define itoa _itoa
#endif

// _create_locale / _free_locale (MSVC CRT locale API)
#include <locale.h>
#define _ENABLE_PER_THREAD_LOCALE_NEW 0x2
inline _locale_t _create_locale(int category, const char* locale) {
	(void)category;
	return newlocale(LC_ALL_MASK, locale, nullptr);
}
inline void _free_locale(_locale_t locale) {
	if (locale) freelocale(locale);
}
