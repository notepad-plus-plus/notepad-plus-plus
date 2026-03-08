#pragma once
// Win32 Shim: Safe string functions for macOS
// StringCchCopy, StringCchPrintf, etc.

#include "windef.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdarg>

#define STRSAFE_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007AL)

inline HRESULT StringCchCopyW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc)
{
	if (!pszDest || cchDest == 0) return E_INVALIDARG;
	if (!pszSrc) { pszDest[0] = L'\0'; return S_OK; }
	size_t srcLen = wcslen(pszSrc);
	if (srcLen >= cchDest) {
		wcsncpy(pszDest, pszSrc, cchDest - 1);
		pszDest[cchDest - 1] = L'\0';
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	wcscpy(pszDest, pszSrc);
	return S_OK;
}
#define StringCchCopy StringCchCopyW

inline HRESULT StringCchCatW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc)
{
	if (!pszDest || cchDest == 0) return E_INVALIDARG;
	size_t destLen = wcslen(pszDest);
	if (destLen >= cchDest) return STRSAFE_E_INSUFFICIENT_BUFFER;
	return StringCchCopyW(pszDest + destLen, cchDest - destLen, pszSrc);
}
#define StringCchCat StringCchCatW

inline HRESULT StringCchPrintfW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszFormat, ...)
{
	if (!pszDest || cchDest == 0) return E_INVALIDARG;
	va_list args;
	va_start(args, pszFormat);
	int result = vswprintf(pszDest, cchDest, pszFormat, args);
	va_end(args);
	if (result < 0) {
		pszDest[0] = L'\0';
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	return S_OK;
}
#define StringCchPrintf StringCchPrintfW

inline HRESULT StringCchLengthW(LPCWSTR psz, size_t cchMax, size_t* pcchLength)
{
	if (!psz || !pcchLength) return E_INVALIDARG;
	for (size_t i = 0; i < cchMax; ++i) {
		if (psz[i] == L'\0') {
			*pcchLength = i;
			return S_OK;
		}
	}
	*pcchLength = cchMax;
	return STRSAFE_E_INSUFFICIENT_BUFFER;
}
#define StringCchLength StringCchLengthW

inline HRESULT StringCchCopyA(LPSTR pszDest, size_t cchDest, LPCSTR pszSrc)
{
	if (!pszDest || cchDest == 0) return E_INVALIDARG;
	if (!pszSrc) { pszDest[0] = '\0'; return S_OK; }
	size_t srcLen = strlen(pszSrc);
	if (srcLen >= cchDest) {
		strncpy(pszDest, pszSrc, cchDest - 1);
		pszDest[cchDest - 1] = '\0';
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}
	strcpy(pszDest, pszSrc);
	return S_OK;
}
