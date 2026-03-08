// Win32 Shim: String conversion and path functions for macOS

#import <Foundation/Foundation.h>
#include <cwchar>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iconv.h>
#include "windows.h"
#include "shlwapi.h"
#include "shellapi.h"

// ============================================================
// Helper: Convert wchar_t* (UTF-32 on macOS) to NSString
// ============================================================
static NSString* NSStringFromWide(LPCWSTR wide)
{
	if (!wide) return nil;
	return [[NSString alloc] initWithBytes:wide
	                               length:wcslen(wide) * sizeof(wchar_t)
	                             encoding:NSUTF32LittleEndianStringEncoding];
}

// Helper: Convert NSString to wchar_t buffer, returns length
static int NSStringToWide(NSString* str, LPWSTR buf, int bufSize)
{
	if (!str) return 0;
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	int charCount = static_cast<int>(data.length / sizeof(wchar_t));
	if (!buf || bufSize <= 0) return charCount;
	int copyCount = (charCount < bufSize) ? charCount : bufSize - 1;
	memcpy(buf, data.bytes, copyCount * sizeof(wchar_t));
	buf[copyCount] = L'\0';
	return copyCount;
}

// ============================================================
// MultiByteToWideChar
// ============================================================
int MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                        int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
	if (!lpMultiByteStr) return 0;

	NSStringEncoding encoding;
	switch (CodePage) {
		case CP_UTF8: encoding = NSUTF8StringEncoding; break;
		case CP_ACP:
		case CP_OEMCP: encoding = NSWindowsCP1252StringEncoding; break;
		default: encoding = NSUTF8StringEncoding; break;
	}

	int len = cbMultiByte;
	if (len == -1) len = static_cast<int>(strlen(lpMultiByteStr) + 1);

	NSString* str = [[NSString alloc] initWithBytes:lpMultiByteStr
	                                         length:len
	                                       encoding:encoding];
	if (!str) {
		// Fallback: try UTF-8 with lossy conversion
		str = [[NSString alloc] initWithBytes:lpMultiByteStr
		                               length:len
		                             encoding:NSUTF8StringEncoding];
		if (!str) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
	}

	// Remove null terminator from string if it was included
	if (cbMultiByte == -1 && [str length] > 0 && [str characterAtIndex:[str length] - 1] == 0) {
		str = [str substringToIndex:[str length] - 1];
	}

	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	int charCount = static_cast<int>(data.length / sizeof(wchar_t));

	// If input included null terminator, add one
	if (cbMultiByte == -1) charCount++;

	if (cchWideChar == 0) return charCount;

	if (charCount > cchWideChar) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	// Copy the converted data
	int copyChars = charCount;
	if (cbMultiByte == -1) copyChars--; // Don't count the null we'll add
	if (copyChars > 0) {
		memcpy(lpWideCharStr, data.bytes, copyChars * sizeof(wchar_t));
	}
	if (cbMultiByte == -1 && charCount <= cchWideChar) {
		lpWideCharStr[copyChars] = L'\0';
	}

	return charCount;
}

// ============================================================
// WideCharToMultiByte
// ============================================================
int WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                        int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                        LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
	if (!lpWideCharStr) return 0;
	if (lpUsedDefaultChar) *lpUsedDefaultChar = FALSE;

	int len = cchWideChar;
	if (len == -1) len = static_cast<int>(wcslen(lpWideCharStr) + 1);

	// On macOS, wchar_t is 32-bit, convert to NSString first
	NSString* str = [[NSString alloc] initWithBytes:lpWideCharStr
	                                         length:(len - (cchWideChar == -1 ? 1 : 0)) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	if (!str) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	NSStringEncoding encoding;
	switch (CodePage) {
		case CP_UTF8: encoding = NSUTF8StringEncoding; break;
		case CP_ACP:
		case CP_OEMCP: encoding = NSWindowsCP1252StringEncoding; break;
		default: encoding = NSUTF8StringEncoding; break;
	}

	NSData* data = [str dataUsingEncoding:encoding allowLossyConversion:YES];
	if (!data) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	int byteCount = static_cast<int>(data.length);
	if (cchWideChar == -1) byteCount++; // For null terminator

	if (cbMultiByte == 0) return byteCount;

	if (byteCount > cbMultiByte) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	int copyBytes = byteCount;
	if (cchWideChar == -1) copyBytes--;
	memcpy(lpMultiByteStr, data.bytes, copyBytes);
	if (cchWideChar == -1) {
		lpMultiByteStr[copyBytes] = '\0';
	}

	return byteCount;
}

// ============================================================
// Character type functions
// ============================================================
BOOL IsDBCSLeadByte(BYTE TestChar)
{
	return FALSE; // Not applicable on macOS with UTF
}

BOOL IsDBCSLeadByteEx(UINT CodePage, BYTE TestChar)
{
	return FALSE;
}

int CompareStringW(DWORD Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	if (!lpString1 && !lpString2) return CSTR_EQUAL;
	if (!lpString1) return CSTR_LESS_THAN;
	if (!lpString2) return CSTR_GREATER_THAN;

	NSString* s1 = NSStringFromWide(lpString1);
	NSString* s2 = NSStringFromWide(lpString2);

	if (cchCount1 >= 0) s1 = [s1 substringToIndex:cchCount1];
	if (cchCount2 >= 0) s2 = [s2 substringToIndex:cchCount2];

	NSStringCompareOptions opts = 0;
	if (dwCmpFlags & NORM_IGNORECASE) opts |= NSCaseInsensitiveSearch;

	NSComparisonResult result = [s1 compare:s2 options:opts];
	switch (result) {
		case NSOrderedAscending: return CSTR_LESS_THAN;
		case NSOrderedSame: return CSTR_EQUAL;
		case NSOrderedDescending: return CSTR_GREATER_THAN;
	}
	return CSTR_EQUAL;
}

// ============================================================
// Path functions (shlwapi.h)
// ============================================================

// Helper: convert path separators
static void normalizePath(LPWSTR path)
{
	for (LPWSTR p = path; *p; ++p) {
		if (*p == L'\\') *p = L'/';
	}
}

BOOL PathFileExistsW(LPCWSTR pszPath)
{
	if (!pszPath) return FALSE;
	NSString* path = NSStringFromWide(pszPath);
	// Convert backslashes
	path = [path stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
	return [[NSFileManager defaultManager] fileExistsAtPath:path];
}

BOOL PathIsDirectoryW(LPCWSTR pszPath)
{
	if (!pszPath) return FALSE;
	NSString* path = NSStringFromWide(pszPath);
	path = [path stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
	BOOL isDir = NO;
	BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir];
	return exists && isDir;
}

BOOL PathIsRelativeW(LPCWSTR pszPath)
{
	if (!pszPath || *pszPath == L'\0') return TRUE;
	return (*pszPath != L'/' && *pszPath != L'\\');
}

BOOL PathRemoveFileSpecW(LPWSTR pszPath)
{
	if (!pszPath) return FALSE;
	LPWSTR lastSep = nullptr;
	for (LPWSTR p = pszPath; *p; ++p) {
		if (*p == L'/' || *p == L'\\') lastSep = p;
	}
	if (lastSep) {
		*lastSep = L'\0';
		return TRUE;
	}
	return FALSE;
}

BOOL PathAppendW(LPWSTR pszPath, LPCWSTR pszMore)
{
	if (!pszPath || !pszMore) return FALSE;
	size_t len = wcslen(pszPath);
	if (len > 0 && pszPath[len - 1] != L'/' && pszPath[len - 1] != L'\\') {
		pszPath[len] = L'/';
		pszPath[len + 1] = L'\0';
	}
	wcscat(pszPath, pszMore);
	return TRUE;
}

LPWSTR PathFindFileNameW(LPCWSTR pszPath)
{
	if (!pszPath) return const_cast<LPWSTR>(pszPath);
	LPCWSTR lastSep = pszPath;
	for (LPCWSTR p = pszPath; *p; ++p) {
		if (*p == L'/' || *p == L'\\') lastSep = p + 1;
	}
	return const_cast<LPWSTR>(lastSep);
}

LPWSTR PathFindExtensionW(LPCWSTR pszPath)
{
	if (!pszPath) return const_cast<LPWSTR>(pszPath);
	LPCWSTR lastDot = nullptr;
	LPCWSTR fileName = PathFindFileNameW(pszPath);
	for (LPCWSTR p = fileName; *p; ++p) {
		if (*p == L'.') lastDot = p;
	}
	if (!lastDot) return const_cast<LPWSTR>(pszPath + wcslen(pszPath));
	return const_cast<LPWSTR>(lastDot);
}

BOOL PathRenameExtensionW(LPWSTR pszPath, LPCWSTR pszExt)
{
	if (!pszPath || !pszExt) return FALSE;
	LPWSTR ext = PathFindExtensionW(pszPath);
	wcscpy(ext, pszExt);
	return TRUE;
}

BOOL PathRemoveExtensionW(LPWSTR pszPath)
{
	if (!pszPath) return FALSE;
	LPWSTR ext = PathFindExtensionW(pszPath);
	*ext = L'\0';
	return TRUE;
}

void PathRemoveBlanksW(LPWSTR pszPath)
{
	if (!pszPath) return;
	// Trim leading spaces
	LPWSTR src = pszPath;
	while (*src == L' ') ++src;
	if (src != pszPath) {
		size_t len = wcslen(src);
		memmove(pszPath, src, (len + 1) * sizeof(wchar_t));
	}
	// Trim trailing spaces
	size_t len = wcslen(pszPath);
	while (len > 0 && pszPath[len - 1] == L' ') {
		pszPath[--len] = L'\0';
	}
}

void PathUnquoteSpacesW(LPWSTR pszPath)
{
	if (!pszPath) return;
	size_t len = wcslen(pszPath);
	if (len >= 2 && pszPath[0] == L'"' && pszPath[len - 1] == L'"') {
		memmove(pszPath, pszPath + 1, (len - 2) * sizeof(wchar_t));
		pszPath[len - 2] = L'\0';
	}
}

BOOL PathStripToRootW(LPWSTR pszPath)
{
	if (!pszPath || *pszPath == L'\0') return FALSE;
	if (*pszPath == L'/') {
		pszPath[1] = L'\0';
		return TRUE;
	}
	return FALSE;
}

LPWSTR PathCombineW(LPWSTR pszDest, LPCWSTR pszDir, LPCWSTR pszFile)
{
	if (!pszDest) return nullptr;
	pszDest[0] = L'\0';
	if (pszDir) wcscpy(pszDest, pszDir);
	if (pszFile) PathAppendW(pszDest, pszFile);
	return pszDest;
}

int PathCommonPrefixW(LPCWSTR pszFile1, LPCWSTR pszFile2, LPWSTR pszPath)
{
	if (!pszFile1 || !pszFile2) return 0;
	int common = 0;
	int lastSep = 0;
	while (pszFile1[common] && pszFile2[common] && pszFile1[common] == pszFile2[common]) {
		if (pszFile1[common] == L'/' || pszFile1[common] == L'\\') lastSep = common + 1;
		++common;
	}
	if (pszPath) {
		wcsncpy(pszPath, pszFile1, lastSep);
		pszPath[lastSep] = L'\0';
	}
	return lastSep;
}

BOOL PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath)
{
	if (!pszBuf || !pszPath) return FALSE;
	wcscpy(pszBuf, pszPath);
	normalizePath(pszBuf);
	return TRUE;
}

BOOL PathMatchSpecW(LPCWSTR pszFile, LPCWSTR pszSpec)
{
	if (!pszFile || !pszSpec) return FALSE;
	NSString* file = NSStringFromWide(pszFile);
	NSString* spec = NSStringFromWide(pszSpec);
	// Simple glob matching via NSPredicate
	NSPredicate* pred = [NSPredicate predicateWithFormat:@"SELF LIKE[c] %@", spec];
	return [pred evaluateWithObject:file];
}

BOOL PathIsURLW(LPCWSTR pszPath)
{
	if (!pszPath) return FALSE;
	return (wcsncmp(pszPath, L"http://", 7) == 0 ||
	        wcsncmp(pszPath, L"https://", 8) == 0 ||
	        wcsncmp(pszPath, L"ftp://", 6) == 0 ||
	        wcsncmp(pszPath, L"file://", 7) == 0);
}

int PathGetDriveNumberW(LPCWSTR pszPath)
{
	return -1; // No drive letters on macOS
}

BOOL PathAddExtensionW(LPWSTR pszPath, LPCWSTR pszExt)
{
	if (!pszPath) return FALSE;
	LPWSTR ext = PathFindExtensionW(pszPath);
	if (*ext != L'\0') return FALSE; // Already has extension
	if (pszExt) wcscat(pszPath, pszExt);
	return TRUE;
}

BOOL PathAddBackslashW(LPWSTR pszPath)
{
	if (!pszPath) return FALSE;
	size_t len = wcslen(pszPath);
	if (len > 0 && pszPath[len - 1] != L'/' && pszPath[len - 1] != L'\\') {
		pszPath[len] = L'/';
		pszPath[len + 1] = L'\0';
	}
	return TRUE;
}

void PathStripPathW(LPWSTR pszPath)
{
	if (!pszPath) return;
	LPWSTR fileName = PathFindFileNameW(pszPath);
	if (fileName != pszPath) {
		size_t len = wcslen(fileName);
		memmove(pszPath, fileName, (len + 1) * sizeof(wchar_t));
	}
}

BOOL PathIsUNCW(LPCWSTR pszPath)
{
	return FALSE; // No UNC paths on macOS
}

// ============================================================
// String functions (shlwapi.h)
// ============================================================
LPWSTR StrDupW(LPCWSTR pszSrch)
{
	if (!pszSrch) return nullptr;
	return wcsdup(pszSrch);
}

int StrCmpIW(LPCWSTR psz1, LPCWSTR psz2)
{
	return wcscasecmp(psz1 ? psz1 : L"", psz2 ? psz2 : L"");
}

int StrCmpNIW(LPCWSTR psz1, LPCWSTR psz2, int nChar)
{
	return wcsncasecmp(psz1 ? psz1 : L"", psz2 ? psz2 : L"", nChar);
}

LPWSTR StrStrIW(LPCWSTR pszFirst, LPCWSTR pszSrch)
{
	if (!pszFirst || !pszSrch) return nullptr;
	NSString* first = NSStringFromWide(pszFirst);
	NSString* srch = NSStringFromWide(pszSrch);
	NSRange range = [first rangeOfString:srch options:NSCaseInsensitiveSearch];
	if (range.location == NSNotFound) return nullptr;
	return const_cast<LPWSTR>(pszFirst + range.location);
}

LPWSTR StrStrW(LPCWSTR pszFirst, LPCWSTR pszSrch)
{
	if (!pszFirst || !pszSrch) return nullptr;
	return const_cast<LPWSTR>(wcsstr(pszFirst, pszSrch));
}

int StrToIntW(LPCWSTR pszSrc)
{
	if (!pszSrc) return 0;
	return static_cast<int>(wcstol(pszSrc, nullptr, 10));
}

BOOL StrToIntExW(LPCWSTR pszString, DWORD dwFlags, int* piRet)
{
	if (!pszString || !piRet) return FALSE;
	wchar_t* end = nullptr;
	int base = (dwFlags & STIF_SUPPORT_HEX) ? 0 : 10;
	*piRet = static_cast<int>(wcstol(pszString, &end, base));
	return (end != pszString);
}

// ============================================================
// URL functions
// ============================================================
HRESULT UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags)
{
	if (!pszUrl || !pszCanonicalized || !pcchCanonicalized) return E_INVALIDARG;
	size_t len = wcslen(pszUrl);
	if (len >= *pcchCanonicalized) {
		*pcchCanonicalized = static_cast<DWORD>(len + 1);
		return E_POINTER;
	}
	wcscpy(pszCanonicalized, pszUrl);
	*pcchCanonicalized = static_cast<DWORD>(len);
	return S_OK;
}

// ============================================================
// SHGetFolderPath
// ============================================================
HRESULT SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath)
{
	if (!pszPath) return E_INVALIDARG;

	NSString* path = nil;
	switch (csidl) {
		case CSIDL_APPDATA:
		case CSIDL_LOCAL_APPDATA: {
			NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
			path = [paths firstObject];
			break;
		}
		case CSIDL_COMMON_APPDATA: {
			NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSLocalDomainMask, YES);
			path = [paths firstObject];
			break;
		}
		case CSIDL_PERSONAL: {
			path = NSHomeDirectory();
			path = [path stringByAppendingPathComponent:@"Documents"];
			break;
		}
		case CSIDL_DESKTOP: {
			NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
			path = [paths firstObject];
			break;
		}
		case CSIDL_PROFILE: {
			path = NSHomeDirectory();
			break;
		}
		default:
			path = NSHomeDirectory();
			break;
	}

	if (!path) return E_FAIL;
	NSStringToWide(path, pszPath, MAX_PATH);
	return S_OK;
}

// ============================================================
// IsTextUnicode — heuristic detection of UTF-16
// ============================================================
BOOL IsTextUnicode(const void* lpv, int iSize, LPINT lpiResult)
{
	if (!lpv || iSize < 2) return FALSE;

	const BYTE* buf = static_cast<const BYTE*>(lpv);
	int tests = lpiResult ? *lpiResult : IS_TEXT_UNICODE_STATISTICS;
	int results = 0;

	// Check BOM signatures
	if (iSize >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
		results |= IS_TEXT_UNICODE_SIGNATURE;
	}
	if (iSize >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) {
		results |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
	}

	// Statistical test: check if it looks like UTF-16LE
	if (tests & IS_TEXT_UNICODE_STATISTICS) {
		int nullHigh = 0;
		int total = iSize / 2;
		for (int i = 0; i < iSize - 1; i += 2) {
			if (buf[i + 1] == 0 && buf[i] != 0) ++nullHigh;
		}
		// If most high bytes are 0, it's likely UTF-16LE
		if (total > 0 && nullHigh > total / 2) {
			results |= IS_TEXT_UNICODE_STATISTICS;
		}
	}

	if (lpiResult) *lpiResult = results;
	return results != 0;
}

// ============================================================
// LCMapStringEx (locale mapping / sort keys)
// ============================================================
int LCMapStringEx(LPCWSTR lpLocaleName, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc,
                  LPWSTR lpDestStr, int cchDest, LPVOID lpVersionInformation, LPVOID lpReserved, LPARAM sortHandle)
{
	(void)lpLocaleName; (void)lpVersionInformation; (void)lpReserved; (void)sortHandle;

	if (!lpSrcStr) return 0;
	if (cchSrc < 0) cchSrc = static_cast<int>(wcslen(lpSrcStr));

	if (dwMapFlags & LCMAP_SORTKEY) {
		// Generate a simple sort key by lowering case and copying bytes
		// This is a simplified stub — real implementation would use ICU or CFStringTransform
		int needed = cchSrc + 1;
		if (cchDest == 0) return needed;
		if (!lpDestStr || cchDest < needed) return 0;
		BYTE* dest = reinterpret_cast<BYTE*>(lpDestStr);
		for (int i = 0; i < cchSrc; ++i) {
			wchar_t ch = lpSrcStr[i];
			if (dwMapFlags & (NORM_IGNORECASE | LINGUISTIC_IGNORECASE))
				ch = towlower(ch);
			dest[i] = static_cast<BYTE>(ch & 0xFF);
		}
		dest[cchSrc] = 0;
		return needed;
	}

	// Non-sort-key mapping (e.g., case conversion)
	if (cchDest == 0) return cchSrc;
	if (!lpDestStr) return 0;
	int toCopy = (cchSrc < cchDest) ? cchSrc : cchDest;
	wcsncpy(lpDestStr, lpSrcStr, toCopy);
	return toCopy;
}
