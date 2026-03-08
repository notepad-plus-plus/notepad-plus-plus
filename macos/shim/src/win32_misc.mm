// Win32 Shim: Miscellaneous API implementations for macOS
// GetLastError, OutputDebugString, tick count, system info, etc.

#import <AppKit/AppKit.h>
#import <mach/mach_time.h>
#import <sys/sysctl.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <malloc/malloc.h>
#include "windows.h"

// ============================================================
// Thread-local error code
// ============================================================
static thread_local DWORD g_lastError = 0;

DWORD GetLastError()
{
	return g_lastError;
}

void SetLastError(DWORD dwErrCode)
{
	g_lastError = dwErrCode;
}

// ============================================================
// OutputDebugString
// ============================================================
void OutputDebugStringW(LPCWSTR lpOutputString)
{
	if (!lpOutputString) return;
	NSString* str = [[NSString alloc] initWithBytes:lpOutputString
	                                         length:wcslen(lpOutputString) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	NSLog(@"%@", str);
}

void OutputDebugStringA(LPCSTR lpOutputString)
{
	if (!lpOutputString) return;
	NSLog(@"%s", lpOutputString);
}

// ============================================================
// Tick count
// ============================================================
static mach_timebase_info_data_t s_timebaseInfo = {0, 0};

static void ensureTimebase()
{
	if (s_timebaseInfo.denom == 0)
		mach_timebase_info(&s_timebaseInfo);
}

DWORD GetTickCount()
{
	ensureTimebase();
	uint64_t elapsed = mach_absolute_time();
	uint64_t nanos = elapsed * s_timebaseInfo.numer / s_timebaseInfo.denom;
	return static_cast<DWORD>(nanos / 1000000);
}

ULONGLONG GetTickCount64()
{
	ensureTimebase();
	uint64_t elapsed = mach_absolute_time();
	uint64_t nanos = elapsed * s_timebaseInfo.numer / s_timebaseInfo.denom;
	return nanos / 1000000;
}

// ============================================================
// Performance counter
// ============================================================
BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
	if (!lpPerformanceCount) return FALSE;
	lpPerformanceCount->QuadPart = static_cast<LONGLONG>(mach_absolute_time());
	return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
	if (!lpFrequency) return FALSE;
	ensureTimebase();
	// Convert timebase to frequency: freq = 1e9 * denom / numer
	lpFrequency->QuadPart = static_cast<LONGLONG>(1000000000ULL * s_timebaseInfo.denom / s_timebaseInfo.numer);
	return TRUE;
}

// ============================================================
// System info
// ============================================================
void GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	if (!lpSystemInfo) return;
	memset(lpSystemInfo, 0, sizeof(SYSTEM_INFO));

	lpSystemInfo->dwPageSize = 4096;
	lpSystemInfo->lpMinimumApplicationAddress = reinterpret_cast<LPVOID>(0x10000);
	lpSystemInfo->lpMaximumApplicationAddress = reinterpret_cast<LPVOID>(0x7FFFFFFEFFFF);

	int ncpu = 1;
	size_t len = sizeof(ncpu);
	sysctlbyname("hw.ncpu", &ncpu, &len, nullptr, 0);
	lpSystemInfo->dwNumberOfProcessors = ncpu;

#ifdef __aarch64__
	lpSystemInfo->wProcessorArchitecture = 12; // PROCESSOR_ARCHITECTURE_ARM64
#else
	lpSystemInfo->wProcessorArchitecture = 9;  // PROCESSOR_ARCHITECTURE_AMD64
#endif
	lpSystemInfo->dwAllocationGranularity = 65536;
}

void GetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	GetSystemInfo(lpSystemInfo);
}

// ============================================================
// Version info (report Windows 10 for compatibility checks)
// ============================================================
BOOL GetVersionExW(LPOSVERSIONINFOW lpVersionInformation)
{
	if (!lpVersionInformation) return FALSE;
	lpVersionInformation->dwMajorVersion = 10;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwBuildNumber = 19041;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	wcscpy(lpVersionInformation->szCSDVersion, L"");
	return TRUE;
}

// ============================================================
// System time
// ============================================================
static void timeToSystemTime(const struct tm* tm, LPSYSTEMTIME st)
{
	st->wYear = tm->tm_year + 1900;
	st->wMonth = tm->tm_mon + 1;
	st->wDayOfWeek = tm->tm_wday;
	st->wDay = tm->tm_mday;
	st->wHour = tm->tm_hour;
	st->wMinute = tm->tm_min;
	st->wSecond = tm->tm_sec;
	st->wMilliseconds = 0;
}

void GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	if (!lpSystemTime) return;
	time_t now = time(nullptr);
	struct tm tm;
	localtime_r(&now, &tm);
	timeToSystemTime(&tm, lpSystemTime);
}

void GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	if (!lpSystemTime) return;
	time_t now = time(nullptr);
	struct tm tm;
	gmtime_r(&now, &tm);
	timeToSystemTime(&tm, lpSystemTime);
}

void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	if (!lpSystemTimeAsFileTime) return;
	SYSTEMTIME st;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, lpSystemTimeAsFileTime);
}

BOOL SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime, LPFILETIME lpFileTime)
{
	if (!lpSystemTime || !lpFileTime) return FALSE;
	struct tm tm = {};
	tm.tm_year = lpSystemTime->wYear - 1900;
	tm.tm_mon = lpSystemTime->wMonth - 1;
	tm.tm_mday = lpSystemTime->wDay;
	tm.tm_hour = lpSystemTime->wHour;
	tm.tm_min = lpSystemTime->wMinute;
	tm.tm_sec = lpSystemTime->wSecond;
	time_t t = timegm(&tm);
	// Windows FILETIME epoch: Jan 1, 1601. Unix epoch: Jan 1, 1970.
	// Difference: 11644473600 seconds
	uint64_t ft = (static_cast<uint64_t>(t) + 11644473600ULL) * 10000000ULL;
	ft += lpSystemTime->wMilliseconds * 10000ULL;
	lpFileTime->dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFF);
	lpFileTime->dwHighDateTime = static_cast<DWORD>(ft >> 32);
	return TRUE;
}

BOOL FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime)
{
	if (!lpFileTime || !lpSystemTime) return FALSE;
	uint64_t ft = (static_cast<uint64_t>(lpFileTime->dwHighDateTime) << 32) | lpFileTime->dwLowDateTime;
	uint64_t unixTime = ft / 10000000ULL - 11644473600ULL;
	time_t t = static_cast<time_t>(unixTime);
	struct tm tm;
	gmtime_r(&t, &tm);
	timeToSystemTime(&tm, lpSystemTime);
	lpSystemTime->wMilliseconds = static_cast<WORD>((ft / 10000) % 1000);
	return TRUE;
}

BOOL FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime)
{
	if (!lpFileTime || !lpLocalFileTime) return FALSE;
	// Simplified: just copy. A proper implementation would apply timezone offset.
	*lpLocalFileTime = *lpFileTime;
	return TRUE;
}

BOOL LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime)
{
	if (!lpLocalFileTime || !lpFileTime) return FALSE;
	*lpFileTime = *lpLocalFileTime;
	return TRUE;
}

DWORD GetTimeZoneInformation(void* lpTimeZoneInformation)
{
	return 0; // TIME_ZONE_ID_UNKNOWN
}

// ============================================================
// FormatMessage (basic stub)
// ============================================================
DWORD FormatMessageW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                     DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list* Arguments)
{
	if (!lpBuffer || nSize == 0) return 0;
	swprintf(lpBuffer, nSize, L"Error %u", static_cast<unsigned>(dwMessageId));
	return static_cast<DWORD>(wcslen(lpBuffer));
}

// ============================================================
// GetCommandLine
// ============================================================
static WCHAR s_commandLine[MAX_PATH * 4] = L"notepad++.exe";

LPWSTR GetCommandLineW()
{
	return s_commandLine;
}

// ============================================================
// Beep
// ============================================================
BOOL Beep(DWORD dwFreq, DWORD dwDuration)
{
	NSBeep();
	return TRUE;
}

// ============================================================
// Global/Local memory (simple malloc wrappers)
// ============================================================
HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
	void* ptr = malloc(dwBytes);
	if (ptr && (uFlags & GMEM_ZEROINIT))
		memset(ptr, 0, dwBytes);
	return ptr;
}

HGLOBAL GlobalFree(HGLOBAL hMem)
{
	free(hMem);
	return nullptr;
}

LPVOID GlobalLock(HGLOBAL hMem)
{
	return hMem; // For GMEM_FIXED, lock is a no-op
}

BOOL GlobalUnlock(HGLOBAL hMem)
{
	return FALSE; // Returns FALSE when lock count reaches 0
}

SIZE_T GlobalSize(HGLOBAL hMem)
{
	return malloc_size(hMem);
}

HLOCAL LocalAlloc(UINT uFlags, SIZE_T uBytes)
{
	return GlobalAlloc(uFlags, uBytes);
}

HLOCAL LocalFree(HLOCAL hMem)
{
	return GlobalFree(hMem);
}

// ============================================================
// Process
// ============================================================
HANDLE GetCurrentProcess()
{
	return reinterpret_cast<HANDLE>(-1); // Pseudo-handle
}

DWORD GetCurrentProcessId()
{
	return static_cast<DWORD>(getpid());
}

DWORD GetCurrentThreadId()
{
	uint64_t tid = 0;
	pthread_threadid_np(nullptr, &tid);
	return static_cast<DWORD>(tid);
}

// ============================================================
// Environment variables
// ============================================================
DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
	if (!lpName) return 0;

	// Convert wide name to UTF-8
	NSString* name = [[NSString alloc] initWithBytes:lpName
	                                          length:wcslen(lpName) * sizeof(wchar_t)
	                                        encoding:NSUTF32LittleEndianStringEncoding];
	const char* value = getenv([name UTF8String]);
	if (!value) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return 0;
	}

	NSString* valStr = [NSString stringWithUTF8String:value];
	NSData* data = [valStr dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	DWORD len = static_cast<DWORD>(data.length / sizeof(wchar_t));

	if (len >= nSize) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return len + 1;
	}

	if (lpBuffer) {
		memcpy(lpBuffer, data.bytes, data.length);
		lpBuffer[len] = L'\0';
	}
	return len;
}

BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue)
{
	if (!lpName) return FALSE;

	NSString* name = [[NSString alloc] initWithBytes:lpName
	                                          length:wcslen(lpName) * sizeof(wchar_t)
	                                        encoding:NSUTF32LittleEndianStringEncoding];
	if (!lpValue) {
		unsetenv([name UTF8String]);
	} else {
		NSString* value = [[NSString alloc] initWithBytes:lpValue
		                                          length:wcslen(lpValue) * sizeof(wchar_t)
		                                        encoding:NSUTF32LittleEndianStringEncoding];
		setenv([name UTF8String], [value UTF8String], 1);
	}
	return TRUE;
}

// ============================================================
// Registry stubs (all return failure)
// ============================================================
LONG RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY* phkResult)
{
	if (phkResult) *phkResult = nullptr;
	return ERROR_FILE_NOT_FOUND;
}

LONG RegCloseKey(HKEY hKey)
{
	return ERROR_SUCCESS;
}

LONG RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	return ERROR_FILE_NOT_FOUND;
}

LONG RegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData)
{
	return ERROR_ACCESS_DENIED;
}

LONG RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass,
                     DWORD dwOptions, DWORD samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     HKEY* phkResult, LPDWORD lpdwDisposition)
{
	if (phkResult) *phkResult = nullptr;
	return ERROR_ACCESS_DENIED;
}

LONG RegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey)
{
	return ERROR_ACCESS_DENIED;
}

LONG RegDeleteValueW(HKEY hKey, LPCWSTR lpValueName)
{
	return ERROR_ACCESS_DENIED;
}

LONG RegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcchName,
                   LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcchClass, PFILETIME lpftLastWriteTime)
{
	return ERROR_NO_MORE_ITEMS;
}

LONG RegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName,
                   LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	return ERROR_NO_MORE_ITEMS;
}

LONG RegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
                      LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen,
                      LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
                      LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
{
	if (lpcSubKeys) *lpcSubKeys = 0;
	if (lpcbMaxSubKeyLen) *lpcbMaxSubKeyLen = 0;
	if (lpcbMaxClassLen) *lpcbMaxClassLen = 0;
	if (lpcValues) *lpcValues = 0;
	if (lpcbMaxValueNameLen) *lpcbMaxValueNameLen = 0;
	if (lpcbMaxValueLen) *lpcbMaxValueLen = 0;
	if (lpcbSecurityDescriptor) *lpcbSecurityDescriptor = 0;
	return ERROR_SUCCESS;
}

// ============================================================
// Module path
// ============================================================
DWORD GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
{
	if (!lpFilename || nSize == 0) return 0;
	NSString* path = [[NSBundle mainBundle] executablePath];
	if (!path) {
		lpFilename[0] = L'\0';
		return 0;
	}
	NSData* data = [path dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	DWORD len = static_cast<DWORD>(data.length / sizeof(wchar_t));
	if (len >= nSize) len = nSize - 1;
	memcpy(lpFilename, data.bytes, len * sizeof(wchar_t));
	lpFilename[len] = L'\0';
	return len;
}

HMODULE GetModuleHandleW(LPCWSTR lpModuleName)
{
	if (!lpModuleName) {
		// Return handle to main executable
		return (__bridge HMODULE)[NSBundle mainBundle];
	}
	return nullptr;
}

// ============================================================
// Version info stubs
// ============================================================
DWORD GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle)
{
	if (lpdwHandle) *lpdwHandle = 0;
	return 0;
}

BOOL GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	return FALSE;
}

BOOL VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
	if (puLen) *puLen = 0;
	if (lplpBuffer) *lplpBuffer = nullptr;
	return FALSE;
}

// ============================================================
// Resources (stubs)
// ============================================================
HRSRC FindResourceW(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType)
{
	return nullptr;
}

HRSRC FindResourceExW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage)
{
	return nullptr;
}

HGLOBAL LoadResource(HMODULE hModule, HRSRC hResInfo)
{
	return nullptr;
}

LPVOID LockResource(HGLOBAL hResData)
{
	return nullptr;
}

DWORD SizeofResource(HMODULE hModule, HRSRC hResInfo)
{
	return 0;
}

int LoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cchBufferMax)
{
	if (!lpBuffer || cchBufferMax <= 0) return 0;
	lpBuffer[0] = L'\0';
	return 0;
}

// ============================================================
// System metrics / colors (reasonable defaults)
// ============================================================
int GetSystemMetrics(int nIndex)
{
	switch (nIndex) {
		case SM_CXSCREEN: return 1920;
		case SM_CYSCREEN: return 1080;
		case SM_CXVSCROLL: return 15;
		case SM_CYHSCROLL: return 15;
		case SM_CYCAPTION: return 22;
		case SM_CXBORDER: return 1;
		case SM_CYBORDER: return 1;
		case SM_CXDLGFRAME: return 3;
		case SM_CYDLGFRAME: return 3;
		case SM_CXEDGE: return 2;
		case SM_CYEDGE: return 2;
		case SM_CXSMICON: return 16;
		case SM_CYSMICON: return 16;
		case SM_CXICON: return 32;
		case SM_CYICON: return 32;
		case SM_CYMENU: return 19;
		case SM_CXFRAME: return 4;
		case SM_CYFRAME: return 4;
		case SM_CXMINTRACK: return 112;
		case SM_CYMINTRACK: return 27;
		case SM_CXMAXTRACK: return 1920;
		case SM_CYMAXTRACK: return 1080;
		case SM_CXFULLSCREEN: return 1920;
		case SM_CYFULLSCREEN: return 1040;
		case SM_MOUSEPRESENT: return 1;
		case SM_CMOUSEBUTTONS: return 3;
		default: return 0;
	}
}

COLORREF GetSysColor(int nIndex)
{
	switch (nIndex) {
		case COLOR_WINDOW: return RGB(255, 255, 255);
		case COLOR_WINDOWTEXT: return RGB(0, 0, 0);
		case COLOR_BTNFACE: return RGB(240, 240, 240);
		case COLOR_BTNTEXT: return RGB(0, 0, 0);
		case COLOR_BTNSHADOW: return RGB(160, 160, 160);
		case COLOR_BTNHIGHLIGHT: return RGB(255, 255, 255);
		case COLOR_HIGHLIGHT: return RGB(0, 120, 215);
		case COLOR_HIGHLIGHTTEXT: return RGB(255, 255, 255);
		case COLOR_GRAYTEXT: return RGB(109, 109, 109);
		case COLOR_MENUTEXT: return RGB(0, 0, 0);
		case COLOR_MENU: return RGB(240, 240, 240);
		case COLOR_ACTIVECAPTION: return RGB(0, 120, 215);
		case COLOR_INACTIVECAPTION: return RGB(191, 205, 219);
		case COLOR_3DLIGHT: return RGB(227, 227, 227);
		case COLOR_3DDKSHADOW: return RGB(105, 105, 105);
		case COLOR_SCROLLBAR: return RGB(200, 200, 200);
		case COLOR_INFOBK: return RGB(255, 255, 225);
		case COLOR_INFOTEXT: return RGB(0, 0, 0);
		case COLOR_HOTLIGHT: return RGB(0, 102, 204);
		default: return RGB(0, 0, 0);
	}
}

HBRUSH GetSysColorBrush(int nIndex)
{
	// Return a non-null sentinel value. Actual brush creation will be in GDI shim.
	return reinterpret_cast<HBRUSH>(static_cast<uintptr_t>(nIndex + 1));
}

BOOL SystemParametersInfoW(UINT uiAction, UINT uiParam, LPVOID pvParam, UINT fWinIni)
{
	switch (uiAction) {
		case SPI_GETWHEELSCROLLLINES:
			if (pvParam) *static_cast<UINT*>(pvParam) = 3;
			return TRUE;
		case SPI_GETNONCLIENTMETRICS:
			if (pvParam) memset(pvParam, 0, uiParam);
			return TRUE;
		case SPI_GETWORKAREA:
			if (pvParam) {
				RECT* rc = static_cast<RECT*>(pvParam);
				rc->left = 0; rc->top = 0;
				rc->right = 1920; rc->bottom = 1080;
			}
			return TRUE;
		default:
			return FALSE;
	}
}

// ============================================================
// Coordinate / rectangle helpers
// ============================================================
BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
	if (!lprc) return FALSE;
	lprc->left = xLeft;
	lprc->top = yTop;
	lprc->right = xRight;
	lprc->bottom = yBottom;
	return TRUE;
}

BOOL SetRectEmpty(LPRECT lprc)
{
	return SetRect(lprc, 0, 0, 0, 0);
}

BOOL CopyRect(LPRECT lprcDst, const RECT* lprcSrc)
{
	if (!lprcDst || !lprcSrc) return FALSE;
	*lprcDst = *lprcSrc;
	return TRUE;
}

BOOL InflateRect(LPRECT lprc, int dx, int dy)
{
	if (!lprc) return FALSE;
	lprc->left -= dx;
	lprc->top -= dy;
	lprc->right += dx;
	lprc->bottom += dy;
	return TRUE;
}

BOOL IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2)
{
	if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;
	lprcDst->left = (lprcSrc1->left > lprcSrc2->left) ? lprcSrc1->left : lprcSrc2->left;
	lprcDst->top = (lprcSrc1->top > lprcSrc2->top) ? lprcSrc1->top : lprcSrc2->top;
	lprcDst->right = (lprcSrc1->right < lprcSrc2->right) ? lprcSrc1->right : lprcSrc2->right;
	lprcDst->bottom = (lprcSrc1->bottom < lprcSrc2->bottom) ? lprcSrc1->bottom : lprcSrc2->bottom;
	if (lprcDst->left >= lprcDst->right || lprcDst->top >= lprcDst->bottom) {
		SetRectEmpty(lprcDst);
		return FALSE;
	}
	return TRUE;
}

BOOL UnionRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2)
{
	if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;
	lprcDst->left = (lprcSrc1->left < lprcSrc2->left) ? lprcSrc1->left : lprcSrc2->left;
	lprcDst->top = (lprcSrc1->top < lprcSrc2->top) ? lprcSrc1->top : lprcSrc2->top;
	lprcDst->right = (lprcSrc1->right > lprcSrc2->right) ? lprcSrc1->right : lprcSrc2->right;
	lprcDst->bottom = (lprcSrc1->bottom > lprcSrc2->bottom) ? lprcSrc1->bottom : lprcSrc2->bottom;
	return TRUE;
}

BOOL SubtractRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2)
{
	if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;
	*lprcDst = *lprcSrc1;
	return TRUE;
}

BOOL OffsetRect(LPRECT lprc, int dx, int dy)
{
	if (!lprc) return FALSE;
	lprc->left += dx;
	lprc->top += dy;
	lprc->right += dx;
	lprc->bottom += dy;
	return TRUE;
}

BOOL IsRectEmpty(const RECT* lprc)
{
	if (!lprc) return TRUE;
	return (lprc->left >= lprc->right) || (lprc->top >= lprc->bottom);
}

BOOL PtInRect(const RECT* lprc, POINT pt)
{
	if (!lprc) return FALSE;
	return (pt.x >= lprc->left && pt.x < lprc->right &&
	        pt.y >= lprc->top && pt.y < lprc->bottom);
}

BOOL EqualRect(const RECT* lprc1, const RECT* lprc2)
{
	if (!lprc1 || !lprc2) return FALSE;
	return (lprc1->left == lprc2->left && lprc1->top == lprc2->top &&
	        lprc1->right == lprc2->right && lprc1->bottom == lprc2->bottom);
}

BOOL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	// On macOS, just return the rect as-is (no non-client area to account for in the same way)
	return TRUE;
}

BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
	return AdjustWindowRectEx(lpRect, dwStyle, bMenu, 0);
}

// ============================================================
// Locale functions
// ============================================================
int GetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	(void)lpLocaleName;
	// Handle LOCALE_RETURN_NUMBER flag
	if (LCType & LOCALE_RETURN_NUMBER) {
		if (lpLCData && cchData >= static_cast<int>(sizeof(DWORD) / sizeof(WCHAR))) {
			DWORD cp = CP_UTF8; // Default to UTF-8 on macOS
			memcpy(lpLCData, &cp, sizeof(DWORD));
			return sizeof(DWORD) / sizeof(WCHAR);
		}
		return 0;
	}
	// For string queries, return a reasonable default
	if (lpLCData && cchData > 0) {
		lpLCData[0] = L'\0';
	}
	return 0;
}

int GetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	(void)Locale;
	return GetLocaleInfoEx(nullptr, LCType, lpLCData, cchData);
}

LCID GetSystemDefaultLCID() { return 0x0409; } // en-US
LCID GetUserDefaultLCID() { return 0x0409; }

// ============================================================
// Clipboard (stubs using NSPasteboard)
// ============================================================
static HWND s_clipboardViewer = nullptr;

HWND SetClipboardViewer(HWND hWndNewViewer)
{
	HWND prev = s_clipboardViewer;
	s_clipboardViewer = hWndNewViewer;
	return prev;
}

BOOL ChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext)
{
	if (s_clipboardViewer == hWndRemove)
		s_clipboardViewer = hWndNewNext;
	return TRUE;
}

BOOL OpenClipboard(HWND hWndNewOwner)
{
	return TRUE;
}

BOOL CloseClipboard()
{
	return TRUE;
}

BOOL EmptyClipboard()
{
	[[NSPasteboard generalPasteboard] clearContents];
	return TRUE;
}

HANDLE SetClipboardData(UINT uFormat, HANDLE hMem)
{
	if (uFormat == CF_UNICODETEXT && hMem) {
		const wchar_t* wstr = static_cast<const wchar_t*>(hMem);
		NSString* str = [[NSString alloc] initWithBytes:wstr
		                                         length:wcslen(wstr) * sizeof(wchar_t)
		                                       encoding:NSUTF32LittleEndianStringEncoding];
		[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
	}
	return hMem;
}

HANDLE GetClipboardData(UINT uFormat)
{
	// Stub - real implementation would convert NSPasteboard data to Win32 format
	return nullptr;
}

BOOL IsClipboardFormatAvailable(UINT format)
{
	if (format == CF_UNICODETEXT || format == CF_TEXT) {
		NSString* str = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
		return str != nil ? TRUE : FALSE;
	}
	return FALSE;
}

int CountClipboardFormats()
{
	return static_cast<int>([[[NSPasteboard generalPasteboard] types] count]);
}

UINT EnumClipboardFormats(UINT format)
{
	return 0;
}

HWND GetClipboardOwner()
{
	return nullptr;
}
