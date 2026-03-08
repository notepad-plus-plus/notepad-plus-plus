#pragma once
// Win32 Shim: WinInet API stubs for macOS
// Internet connectivity will use NSURLSession on macOS.

#include "windef.h"

typedef void* HINTERNET;

#define INTERNET_OPEN_TYPE_PRECONFIG 0
#define INTERNET_OPEN_TYPE_DIRECT    1
#define INTERNET_FLAG_RELOAD         0x80000000
#define INTERNET_FLAG_NO_CACHE_WRITE 0x04000000

inline HINTERNET InternetOpenW(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags)
{
	(void)lpszAgent; (void)dwAccessType; (void)lpszProxy; (void)lpszProxyBypass; (void)dwFlags;
	return nullptr;
}
#define InternetOpen InternetOpenW

inline HINTERNET InternetOpenUrlW(HINTERNET hInternet, LPCWSTR lpszUrl, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
	(void)hInternet; (void)lpszUrl; (void)lpszHeaders; (void)dwHeadersLength; (void)dwFlags; (void)dwContext;
	return nullptr;
}
#define InternetOpenUrl InternetOpenUrlW

inline BOOL InternetReadFile(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
{
	if (lpdwNumberOfBytesRead) *lpdwNumberOfBytesRead = 0;
	return FALSE;
}

inline BOOL InternetCloseHandle(HINTERNET hInternet) { return TRUE; }

inline BOOL HttpQueryInfoW(HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
	return FALSE;
}
#define HttpQueryInfo HttpQueryInfoW

#define HTTP_QUERY_STATUS_CODE 19
#define HTTP_QUERY_FLAG_NUMBER 0x20000000

// ============================================================
// URL_COMPONENTS for InternetCrackUrl
// ============================================================
typedef struct {
	DWORD   dwStructSize;
	LPWSTR  lpszScheme;
	DWORD   dwSchemeLength;
	int     nScheme;
	LPWSTR  lpszHostName;
	DWORD   dwHostNameLength;
	WORD    nPort;
	LPWSTR  lpszUserName;
	DWORD   dwUserNameLength;
	LPWSTR  lpszPassword;
	DWORD   dwPasswordLength;
	LPWSTR  lpszUrlPath;
	DWORD   dwUrlPathLength;
	LPWSTR  lpszExtraInfo;
	DWORD   dwExtraInfoLength;
} URL_COMPONENTSW, *LPURL_COMPONENTSW;
typedef URL_COMPONENTSW URL_COMPONENTS;
typedef LPURL_COMPONENTSW LPURL_COMPONENTS;

inline BOOL InternetCrackUrlW(LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTSW lpUrlComponents)
{
	(void)lpszUrl; (void)dwUrlLength; (void)dwFlags; (void)lpUrlComponents;
	// Stub: always succeeds (URL validation is best-effort)
	return TRUE;
}
#define InternetCrackUrl InternetCrackUrlW
