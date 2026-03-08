#pragma once
// Win32 Shim: Shell Lightweight Utility API for macOS

#include "windef.h"

// ============================================================
// Path functions (implemented in win32_string.mm)
// ============================================================
BOOL PathFileExistsW(LPCWSTR pszPath);
#define PathFileExists PathFileExistsW

BOOL PathIsDirectoryW(LPCWSTR pszPath);
#define PathIsDirectory PathIsDirectoryW

BOOL PathIsRelativeW(LPCWSTR pszPath);
#define PathIsRelative PathIsRelativeW

BOOL PathRemoveFileSpecW(LPWSTR pszPath);
#define PathRemoveFileSpec PathRemoveFileSpecW

BOOL PathAppendW(LPWSTR pszPath, LPCWSTR pszMore);
#define PathAppend PathAppendW

LPWSTR PathFindFileNameW(LPCWSTR pszPath);
#define PathFindFileName PathFindFileNameW

LPWSTR PathFindExtensionW(LPCWSTR pszPath);
#define PathFindExtension PathFindExtensionW

BOOL PathRenameExtensionW(LPWSTR pszPath, LPCWSTR pszExt);
#define PathRenameExtension PathRenameExtensionW

BOOL PathRemoveExtensionW(LPWSTR pszPath);
#define PathRemoveExtension PathRemoveExtensionW

void PathRemoveBlanksW(LPWSTR pszPath);
#define PathRemoveBlanks PathRemoveBlanksW

void PathUnquoteSpacesW(LPWSTR pszPath);
#define PathUnquoteSpaces PathUnquoteSpacesW

BOOL PathStripToRootW(LPWSTR pszPath);
#define PathStripToRoot PathStripToRootW

LPWSTR PathCombineW(LPWSTR pszDest, LPCWSTR pszDir, LPCWSTR pszFile);
#define PathCombine PathCombineW

int PathCommonPrefixW(LPCWSTR pszFile1, LPCWSTR pszFile2, LPWSTR pszPath);
#define PathCommonPrefix PathCommonPrefixW

BOOL PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath);
#define PathCanonicalize PathCanonicalizeW

BOOL PathMatchSpecW(LPCWSTR pszFile, LPCWSTR pszSpec);
#define PathMatchSpec PathMatchSpecW

BOOL PathIsURLW(LPCWSTR pszPath);
#define PathIsURL PathIsURLW

int PathGetDriveNumberW(LPCWSTR pszPath);
#define PathGetDriveNumber PathGetDriveNumberW

BOOL PathAddExtensionW(LPWSTR pszPath, LPCWSTR pszExt);
#define PathAddExtension PathAddExtensionW

BOOL PathAddBackslashW(LPWSTR pszPath);
#define PathAddBackslash PathAddBackslashW

void PathStripPathW(LPWSTR pszPath);
#define PathStripPath PathStripPathW

BOOL PathIsUNCW(LPCWSTR pszPath);
#define PathIsUNC PathIsUNCW

BOOL PathCompactPathExW(LPWSTR pszOut, LPCWSTR pszSrc, UINT cchMax, DWORD dwFlags);
#define PathCompactPathEx PathCompactPathExW

// ============================================================
// String functions
// ============================================================
LPWSTR StrDupW(LPCWSTR pszSrch);
#define StrDup StrDupW

int StrCmpIW(LPCWSTR psz1, LPCWSTR psz2);
#define StrCmpI StrCmpIW

int StrCmpNIW(LPCWSTR psz1, LPCWSTR psz2, int nChar);
#define StrCmpNI StrCmpNIW

LPWSTR StrStrIW(LPCWSTR pszFirst, LPCWSTR pszSrch);
#define StrStrI StrStrIW

LPWSTR StrStrW(LPCWSTR pszFirst, LPCWSTR pszSrch);
#define StrStr StrStrW

int StrToIntW(LPCWSTR pszSrc);
#define StrToInt StrToIntW

BOOL StrToIntExW(LPCWSTR pszString, DWORD dwFlags, int* piRet);
#define StrToIntEx StrToIntExW

#define STIF_DEFAULT 0x00000000L
#define STIF_SUPPORT_HEX 0x00000001L

// ============================================================
// URL functions (stubs)
// ============================================================
#define URL_UNESCAPE          0x10000000
#define URL_ESCAPE_UNSAFE     0x20000000
#define URL_PLUGGABLE_PROTOCOL 0x40000000

HRESULT UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags);
#define UrlCanonicalize UrlCanonicalizeW

// ============================================================
// Shell association API (stubs)
// ============================================================
typedef enum {
	ASSOCF_NONE                = 0x00000000,
	ASSOCF_INIT_NOREMAPCLSID   = 0x00000001,
	ASSOCF_INIT_BYEXENAME      = 0x00000002,
	ASSOCF_OPEN_BYEXENAME      = 0x00000002,
	ASSOCF_INIT_DEFAULTTOSTAR   = 0x00000004,
	ASSOCF_INIT_DEFAULTTOFOLDER = 0x00000008,
	ASSOCF_NOUSERSETTINGS       = 0x00000010,
	ASSOCF_NOTRUNCATE           = 0x00000020,
	ASSOCF_VERIFY               = 0x00000040,
	ASSOCF_REMAPRUNDLL          = 0x00000080,
	ASSOCF_NOFIXUPS             = 0x00000100,
	ASSOCF_IGNOREBASECLASS      = 0x00000200,
	ASSOCF_INIT_IGNOREUNKNOWN   = 0x00000400,
} ASSOCF;

typedef enum {
	ASSOCSTR_COMMAND = 1,
	ASSOCSTR_EXECUTABLE,
	ASSOCSTR_FRIENDLYDOCNAME,
	ASSOCSTR_FRIENDLYAPPNAME,
	ASSOCSTR_NOOPEN,
	ASSOCSTR_SHELLNEWVALUE,
	ASSOCSTR_DDECOMMAND,
	ASSOCSTR_DDEIFEXEC,
	ASSOCSTR_DDEAPPLICATION,
	ASSOCSTR_DDETOPIC,
	ASSOCSTR_INFOTIP,
	ASSOCSTR_QUICKTIP,
	ASSOCSTR_TILEINFO,
	ASSOCSTR_CONTENTTYPE,
	ASSOCSTR_DEFAULTICON,
	ASSOCSTR_SHELLEXTENSION,
} ASSOCSTR;

inline HRESULT AssocQueryStringW(DWORD flags, DWORD str, LPCWSTR pszAssoc, LPCWSTR pszExtra,
                                  LPWSTR pszOut, DWORD* pcchOut)
{
	if (pszOut && pcchOut && *pcchOut > 0) pszOut[0] = L'\0';
	return 0x80070002L; // HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
}
#define AssocQueryString AssocQueryStringW

// ============================================================
// Color conversion (HLS ↔ RGB)
// ============================================================
void ColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation);
COLORREF ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation);
#define ColorAdjustLuma ColorAdjustLumaStub
inline COLORREF ColorAdjustLumaStub(COLORREF clrRGB, int n, BOOL fScale) {
	(void)n; (void)fScale;
	return clrRGB;
}
