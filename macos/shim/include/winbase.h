#pragma once
// Win32 Shim: Base API declarations for macOS
// File I/O, DLL loading, string functions, threading, etc.

#include "windef.h"
#include "winnt.h"
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <unistd.h>

// ============================================================
// Error codes
// ============================================================
#define ERROR_SUCCESS          0L
#define NO_ERROR               0L
#define ERROR_FILE_NOT_FOUND   2L
#define ERROR_PATH_NOT_FOUND   3L
#define ERROR_ACCESS_DENIED    5L
#define ERROR_INVALID_HANDLE   6L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_OUTOFMEMORY      14L
#define ERROR_INVALID_DRIVE    15L
#define ERROR_NO_MORE_FILES    18L
#define ERROR_NOT_READY        21L
#define ERROR_SHARING_VIOLATION 32L
#define ERROR_LOCK_VIOLATION   33L
#define ERROR_HANDLE_EOF       38L
#define ERROR_FILE_EXISTS      80L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_ALREADY_EXISTS   183L
#define ERROR_MORE_DATA        234L
#define ERROR_NO_MORE_ITEMS    259L
#define ERROR_NOT_FOUND        1168L
#define ERROR_TIMEOUT          1460L

typedef LONG LSTATUS;

// ============================================================
// WIN32_FIND_DATA
// ============================================================
typedef struct _WIN32_FIND_DATAW {
	DWORD    dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD    nFileSizeHigh;
	DWORD    nFileSizeLow;
	DWORD    dwReserved0;
	DWORD    dwReserved1;
	WCHAR    cFileName[MAX_PATH];
	WCHAR    cAlternateFileName[14];
} WIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;
typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
typedef LPWIN32_FIND_DATAW LPWIN32_FIND_DATA;

// ============================================================
// WIN32_FILE_ATTRIBUTE_DATA
// ============================================================
typedef enum _GET_FILEEX_INFO_LEVELS {
	GetFileExInfoStandard,
	GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
	DWORD    dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD    nFileSizeHigh;
	DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

BOOL GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
                          LPVOID lpFileInformation);
#define GetFileAttributesEx GetFileAttributesExW

// ============================================================
// STARTUPINFO / PROCESS_INFORMATION
// ============================================================
typedef struct _STARTUPINFOW {
	DWORD  cb;
	LPWSTR lpReserved;
	LPWSTR lpDesktop;
	LPWSTR lpTitle;
	DWORD  dwX;
	DWORD  dwY;
	DWORD  dwXSize;
	DWORD  dwYSize;
	DWORD  dwXCountChars;
	DWORD  dwYCountChars;
	DWORD  dwFillAttribute;
	DWORD  dwFlags;
	WORD   wShowWindow;
	WORD   cbReserved2;
	LPBYTE lpReserved2;
	HANDLE hStdInput;
	HANDLE hStdOutput;
	HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;
typedef STARTUPINFOW STARTUPINFO;

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD  dwProcessId;
	DWORD  dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#define STARTF_USESHOWWINDOW 0x00000001
#define STARTF_USESIZE       0x00000002
#define STARTF_USEPOSITION   0x00000004
#define STARTF_USESTDHANDLES 0x00000100

// ============================================================
// Format message flags
// ============================================================
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF

// ============================================================
// Global memory flags
// ============================================================
#define GMEM_FIXED    0x0000
#define GMEM_MOVEABLE 0x0002
#define GMEM_ZEROINIT 0x0040
#define GHND          (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR          (GMEM_FIXED | GMEM_ZEROINIT)

#define LMEM_FIXED    0x0000
#define LMEM_MOVEABLE 0x0002
#define LMEM_ZEROINIT 0x0040
#define LPTR          (LMEM_FIXED | LMEM_ZEROINIT)

// ============================================================
// Memory macros
// ============================================================
#define ZeroMemory(dest, len) memset((dest), 0, (len))
#define CopyMemory(dest, src, len) memcpy((dest), (src), (len))
#define FillMemory(dest, len, fill) memset((dest), (fill), (len))
#define MoveMemory(dest, src, len) memmove((dest), (src), (len))

// ============================================================
// Thread / Synchronization
// ============================================================
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID lpParameter);

// ============================================================
// String functions (inline implementations)
// ============================================================

inline int lstrlenW(LPCWSTR lpString)
{
	if (!lpString) return 0;
	return static_cast<int>(wcslen(lpString));
}
#define lstrlen lstrlenW

inline int lstrlenA(LPCSTR lpString)
{
	if (!lpString) return 0;
	return static_cast<int>(strlen(lpString));
}

inline LPWSTR lstrcpyW(LPWSTR lpString1, LPCWSTR lpString2)
{
	if (!lpString1) return nullptr;
	if (!lpString2) { lpString1[0] = L'\0'; return lpString1; }
	wcscpy(lpString1, lpString2);
	return lpString1;
}
#define lstrcpy lstrcpyW

inline LPSTR lstrcpyA(LPSTR lpString1, LPCSTR lpString2)
{
	if (!lpString1) return nullptr;
	if (!lpString2) { lpString1[0] = '\0'; return lpString1; }
	strcpy(lpString1, lpString2);
	return lpString1;
}

inline LPWSTR lstrcpynW(LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength)
{
	if (!lpString1 || iMaxLength <= 0) return lpString1;
	if (!lpString2) { lpString1[0] = L'\0'; return lpString1; }
	wcsncpy(lpString1, lpString2, iMaxLength - 1);
	lpString1[iMaxLength - 1] = L'\0';
	return lpString1;
}
inline LPSTR lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength)
{
	if (!lpString1 || iMaxLength <= 0) return lpString1;
	if (!lpString2) { lpString1[0] = '\0'; return lpString1; }
	strncpy(lpString1, lpString2, iMaxLength - 1);
	lpString1[iMaxLength - 1] = '\0';
	return lpString1;
}
#define lstrcpyn lstrcpynW

inline int lstrcmpW(LPCWSTR lpString1, LPCWSTR lpString2)
{
	if (!lpString1 && !lpString2) return 0;
	if (!lpString1) return -1;
	if (!lpString2) return 1;
	return wcscmp(lpString1, lpString2);
}
#define lstrcmp lstrcmpW

inline int lstrcmpiW(LPCWSTR lpString1, LPCWSTR lpString2)
{
	if (!lpString1 && !lpString2) return 0;
	if (!lpString1) return -1;
	if (!lpString2) return 1;
	return wcscasecmp(lpString1, lpString2);
}
inline int lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2)
{
	if (!lpString1 && !lpString2) return 0;
	if (!lpString1) return -1;
	if (!lpString2) return 1;
	return strcasecmp(lpString1, lpString2);
}
#define lstrcmpi lstrcmpiW

inline LPWSTR lstrcatW(LPWSTR lpString1, LPCWSTR lpString2)
{
	if (!lpString1) return nullptr;
	if (!lpString2) return lpString1;
	wcscat(lpString1, lpString2);
	return lpString1;
}
#define lstrcat lstrcatW

// wsprintf → wrapper that adds buffer size (Win32 wsprintf doesn't take size arg)
// Win32 wsprintf assumes buffer is at least 1024 chars
#include <cstdarg>
inline int wsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, ...)
{
	va_list args;
	va_start(args, lpFmt);
	int ret = vswprintf(lpOut, 1024, lpFmt, args);
	va_end(args);
	return ret;
}
#define wsprintf wsprintfW
inline int wvsprintfW(LPWSTR lpOut, LPCWSTR lpFmt, va_list argList)
{
	return vswprintf(lpOut, 1024, lpFmt, argList);
}
#define wvsprintf wvsprintfW

// ============================================================
// MSVC non-standard C functions
// ============================================================
inline char* _itoa(int value, char* str, int radix)
{
	if (radix == 10) snprintf(str, 34, "%d", value);
	else if (radix == 16) snprintf(str, 34, "%x", value);
	else if (radix == 8) snprintf(str, 34, "%o", value);
	else str[0] = '\0';
	return str;
}

#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _wcsicmp wcscasecmp
#define _wcsnicmp wcsncasecmp
#define wcsnicmp wcsncasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _strlwr(s) ({ char* _p = (s); while (*_p) { *_p = tolower(*_p); ++_p; } (s); })
#define _strupr(s) ({ char* _p = (s); while (*_p) { *_p = toupper(*_p); ++_p; } (s); })

// MSVC wide-char classification macros
#define _istascii(c) (static_cast<unsigned int>(c) <= 0x7F)

// _wsplitpath_s: split a wide path into components (MSVC extension)
#include <cwchar>
inline int _wsplitpath_s(const wchar_t* path,
	wchar_t* drive, size_t driveLen,
	wchar_t* dir, size_t dirLen,
	wchar_t* fname, size_t fnameLen,
	wchar_t* ext, size_t extLen)
{
	if (!path) return -1;
	// Drive: always empty on macOS
	if (drive && driveLen > 0) drive[0] = L'\0';
	// Find last separator
	const wchar_t* lastSep = nullptr;
	for (const wchar_t* p = path; *p; ++p) {
		if (*p == L'/' || *p == L'\\') lastSep = p;
	}
	// Directory
	if (dir && dirLen > 0) {
		if (lastSep) {
			size_t len = static_cast<size_t>(lastSep - path + 1);
			if (len >= dirLen) len = dirLen - 1;
			wcsncpy(dir, path, len);
			dir[len] = L'\0';
		} else {
			dir[0] = L'\0';
		}
	}
	// Filename + extension
	const wchar_t* base = lastSep ? lastSep + 1 : path;
	const wchar_t* dot = nullptr;
	for (const wchar_t* p = base; *p; ++p) {
		if (*p == L'.') dot = p;
	}
	if (!dot) dot = base + wcslen(base);
	if (fname && fnameLen > 0) {
		size_t len = static_cast<size_t>(dot - base);
		if (len >= fnameLen) len = fnameLen - 1;
		wcsncpy(fname, base, len);
		fname[len] = L'\0';
	}
	if (ext && extLen > 0) {
		size_t len = wcslen(dot);
		if (len >= extLen) len = extLen - 1;
		wcsncpy(ext, dot, len);
		ext[len] = L'\0';
	}
	return 0;
}

inline int _wmakepath_s(wchar_t* path, size_t sizeInWords,
	const wchar_t* drive, const wchar_t* dir, const wchar_t* fname, const wchar_t* ext)
{
	if (!path || sizeInWords == 0) return -1;
	path[0] = L'\0';
	if (drive && drive[0]) wcsncat(path, drive, sizeInWords - wcslen(path) - 1);
	if (dir && dir[0]) wcsncat(path, dir, sizeInWords - wcslen(path) - 1);
	if (fname && fname[0]) wcsncat(path, fname, sizeInWords - wcslen(path) - 1);
	if (ext && ext[0]) {
		if (ext[0] != L'.') wcsncat(path, L".", sizeInWords - wcslen(path) - 1);
		wcsncat(path, ext, sizeInWords - wcslen(path) - 1);
	}
	return 0;
}

// _wfopen: MSVC extension, not available natively on macOS
// Converts wide filename + mode to UTF-8 and calls fopen
#include <string>
inline FILE* _wfopen(const wchar_t* filename, const wchar_t* mode)
{
	if (!filename || !mode) return nullptr;
	// Convert wchar_t (UTF-32 on macOS) to UTF-8
	auto toUtf8 = [](const wchar_t* ws) -> std::string {
		std::string result;
		for (const wchar_t* p = ws; *p; ++p) {
			wchar_t c = *p;
			if (c < 0x80) {
				result += static_cast<char>(c);
			} else if (c < 0x800) {
				result += static_cast<char>(0xC0 | (c >> 6));
				result += static_cast<char>(0x80 | (c & 0x3F));
			} else if (c < 0x10000) {
				result += static_cast<char>(0xE0 | (c >> 12));
				result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
				result += static_cast<char>(0x80 | (c & 0x3F));
			} else {
				result += static_cast<char>(0xF0 | (c >> 18));
				result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
				result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
				result += static_cast<char>(0x80 | (c & 0x3F));
			}
		}
		return result;
	};
	return fopen(toUtf8(filename).c_str(), toUtf8(mode).c_str());
}

// ============================================================
// MulDiv
// ============================================================
inline int MulDiv(int nNumber, int nNumerator, int nDenominator)
{
	if (nDenominator == 0) return -1;
	long long result = (static_cast<long long>(nNumber) * static_cast<long long>(nNumerator)) / nDenominator;
	return static_cast<int>(result);
}

// ============================================================
// OutputDebugString
// ============================================================
// Implemented in win32_misc.mm

// ============================================================
// GetLastError / SetLastError
// ============================================================
// Thread-local error code
DWORD GetLastError();
void SetLastError(DWORD dwErrCode);

// ============================================================
// Critical Section functions
// ============================================================
inline void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lpCriticalSection->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

inline void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	pthread_mutex_destroy(&lpCriticalSection->mutex);
}

inline void EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	pthread_mutex_lock(&lpCriticalSection->mutex);
}

inline void LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	pthread_mutex_unlock(&lpCriticalSection->mutex);
}

inline BOOL TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	return pthread_mutex_trylock(&lpCriticalSection->mutex) == 0 ? TRUE : FALSE;
}

// ============================================================
// Sleep
// ============================================================
inline void Sleep(DWORD dwMilliseconds)
{
	if (dwMilliseconds == INFINITE) {
		while (true) { usleep(1000000); }
	}
	usleep(dwMilliseconds * 1000);
}

// ============================================================
// Tick count
// ============================================================
DWORD GetTickCount();
ULONGLONG GetTickCount64();

// ============================================================
// File I/O function declarations (implemented in win32_file.mm)
// ============================================================
HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#define CreateFile CreateFileW

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

BOOL CloseHandle(HANDLE hObject);

// File mapping
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define SEC_IMAGE              0x01000000
#define SEC_RESERVE            0x04000000
#define SEC_COMMIT             0x08000000

#define FILE_MAP_COPY    0x0001
#define FILE_MAP_WRITE   0x0002
#define FILE_MAP_READ    0x0004
#define FILE_MAP_EXECUTE 0x0020

HANDLE CreateFileMappingW(HANDLE hFile, void* lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName);
#define CreateFileMapping CreateFileMappingW
LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
BOOL UnmapViewOfFile(LPCVOID lpBaseAddress);

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod);

BOOL SetEndOfFile(HANDLE hFile);

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);

BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);

DWORD GetFileAttributesW(LPCWSTR lpFileName);
#define GetFileAttributes GetFileAttributesW

BOOL SetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
#define SetFileAttributes SetFileAttributesW

BOOL DeleteFileW(LPCWSTR lpFileName);
#define DeleteFile DeleteFileW

BOOL CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
#define CopyFile CopyFileW

#define COPY_FILE_FAIL_IF_EXISTS     0x00000001
#define COPY_FILE_NO_BUFFERING       0x00001000

typedef DWORD (CALLBACK* LPPROGRESS_ROUTINE)(LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID);

BOOL CopyFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags);
#define CopyFileEx CopyFileExW

BOOL MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
#define MoveFile MoveFileW

BOOL MoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);
#define MoveFileEx MoveFileExW

#define MOVEFILE_REPLACE_EXISTING 0x00000001
#define MOVEFILE_COPY_ALLOWED     0x00000002

BOOL CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#define CreateDirectory CreateDirectoryW

BOOL RemoveDirectoryW(LPCWSTR lpPathName);
#define RemoveDirectory RemoveDirectoryW

HANDLE FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
#define FindFirstFile FindFirstFileW

BOOL FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
#define FindNextFile FindNextFileW

BOOL FindClose(HANDLE hFindFile);

DWORD GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);
#define GetCurrentDirectory GetCurrentDirectoryW

BOOL SetCurrentDirectoryW(LPCWSTR lpPathName);
#define SetCurrentDirectory SetCurrentDirectoryW

DWORD GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer);
#define GetTempPath GetTempPathW

inline UINT GetWindowsDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	// No Windows directory on macOS; return empty string
	if (lpBuffer && uSize > 0) lpBuffer[0] = L'\0';
	return 0;
}
#define GetWindowsDirectory GetWindowsDirectoryW

inline UINT GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	if (lpBuffer && uSize > 0) lpBuffer[0] = L'\0';
	return 0;
}
#define GetSystemDirectory GetSystemDirectoryW

UINT GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);
#define GetTempFileName GetTempFileNameW

DWORD GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart);
#define GetFullPathName GetFullPathNameW

DWORD GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
#define GetModuleFileName GetModuleFileNameW

HMODULE GetModuleHandleW(LPCWSTR lpModuleName);
#define GetModuleHandle GetModuleHandleW

// ============================================================
// DLL / dylib loading (implemented in win32_dll.mm)
// ============================================================
HMODULE LoadLibraryW(LPCWSTR lpLibFileName);
#define LoadLibrary LoadLibraryW

HMODULE LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
#define LoadLibraryEx LoadLibraryExW

BOOL FreeLibrary(HMODULE hLibModule);

typedef void* FARPROC;
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

#define LOAD_LIBRARY_AS_DATAFILE       0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH  0x00000008
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0x00000100

// ============================================================
// Process functions
// ============================================================
HANDLE GetCurrentProcess();
DWORD GetCurrentProcessId();
DWORD GetCurrentThreadId();

BOOL CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                    BOOL bInheritHandles, DWORD dwCreationFlags,
                    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                    LPSTARTUPINFOW lpStartupInfo,
                    LPPROCESS_INFORMATION lpProcessInformation);
#define CreateProcess CreateProcessW

// ============================================================
// Environment
// ============================================================
DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
#define GetEnvironmentVariable GetEnvironmentVariableW

BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue);
#define SetEnvironmentVariable SetEnvironmentVariableW

// ============================================================
// String conversion (implemented in win32_string.mm)
// ============================================================
int MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                        int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

int WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                        int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                        LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

#define MB_PRECOMPOSED     0x00000001
#define MB_COMPOSITE       0x00000002
#define MB_USEGLYPHCHARS   0x00000004
#define MB_ERR_INVALID_CHARS 0x00000008

#define WC_COMPOSITECHECK  0x00000200
#define WC_DISCARDNS       0x00000010
#define WC_SEPCHARS        0x00000020
#define WC_DEFAULTCHAR     0x00000040
#define WC_NO_BEST_FIT_CHARS 0x00000400

// ============================================================
// Character type functions
// ============================================================
BOOL IsDBCSLeadByte(BYTE TestChar);
BOOL IsDBCSLeadByteEx(UINT CodePage, BYTE TestChar);
int CompareStringW(DWORD Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
#define CompareString CompareStringW

#define CSTR_LESS_THAN    1
#define CSTR_EQUAL        2
#define CSTR_GREATER_THAN 3

#define NORM_IGNORECASE    0x00000001
#define NORM_IGNORENONSPACE 0x00000002
#define NORM_IGNORESYMBOLS 0x00000004
#define NORM_LINGUISTIC_CASING 0x08000000
#define SORT_STRINGSORT    0x00001000
#define SORT_DIGITSASNUMBERS 0x00000008
#define LINGUISTIC_IGNORECASE 0x00000010
#define LINGUISTIC_IGNOREDIACRITIC 0x00000020
#define LCMAP_SORTKEY      0x00000400

int LCMapStringEx(LPCWSTR lpLocaleName, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc,
                  LPWSTR lpDestStr, int cchDest, LPVOID lpVersionInformation, LPVOID lpReserved, LPARAM sortHandle);

// ============================================================
// Char classification / conversion (inline)
// ============================================================
inline BOOL CharUpperBuffW(LPWSTR lpsz, DWORD cchLength)
{
	for (DWORD i = 0; i < cchLength; ++i)
		lpsz[i] = towupper(lpsz[i]);
	return cchLength;
}

inline BOOL CharLowerBuffW(LPWSTR lpsz, DWORD cchLength)
{
	for (DWORD i = 0; i < cchLength; ++i)
		lpsz[i] = towlower(lpsz[i]);
	return cchLength;
}

inline LPWSTR CharUpperW(LPWSTR lpsz)
{
	if (reinterpret_cast<uintptr_t>(lpsz) <= 0xFFFF)
		return reinterpret_cast<LPWSTR>(static_cast<uintptr_t>(towupper(static_cast<wchar_t>(reinterpret_cast<uintptr_t>(lpsz)))));
	for (LPWSTR p = lpsz; *p; ++p)
		*p = towupper(*p);
	return lpsz;
}

inline LPWSTR CharLowerW(LPWSTR lpsz)
{
	if (reinterpret_cast<uintptr_t>(lpsz) <= 0xFFFF)
		return reinterpret_cast<LPWSTR>(static_cast<uintptr_t>(towlower(static_cast<wchar_t>(reinterpret_cast<uintptr_t>(lpsz)))));
	for (LPWSTR p = lpsz; *p; ++p)
		*p = towlower(*p);
	return lpsz;
}

#define CharUpper CharUpperW
#define CharLower CharLowerW
#define CharUpperBuff CharUpperBuffW
#define CharLowerBuff CharLowerBuffW

// ============================================================
// Memory allocation
// ============================================================
HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
HGLOBAL GlobalFree(HGLOBAL hMem);
LPVOID GlobalLock(HGLOBAL hMem);
BOOL GlobalUnlock(HGLOBAL hMem);
SIZE_T GlobalSize(HGLOBAL hMem);

HLOCAL LocalAlloc(UINT uFlags, SIZE_T uBytes);
HLOCAL LocalFree(HLOCAL hMem);

// ============================================================
// System info
// ============================================================
typedef struct _SYSTEM_INFO {
	union {
		DWORD dwOemId;
		struct {
			WORD wProcessorArchitecture;
			WORD wReserved;
		};
	};
	DWORD     dwPageSize;
	LPVOID    lpMinimumApplicationAddress;
	LPVOID    lpMaximumApplicationAddress;
	DWORD_PTR dwActiveProcessorMask;
	DWORD     dwNumberOfProcessors;
	DWORD     dwProcessorType;
	DWORD     dwAllocationGranularity;
	WORD      wProcessorLevel;
	WORD      wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

void GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);
void GetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo);

// ============================================================
// Version info
// ============================================================
typedef struct _OSVERSIONINFOW {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *LPOSVERSIONINFOW;
typedef OSVERSIONINFOW OSVERSIONINFO;

typedef struct _OSVERSIONINFOEXW {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	WCHAR szCSDVersion[128];
	WORD  wServicePackMajor;
	WORD  wServicePackMinor;
	WORD  wSuiteMask;
	BYTE  wProductType;
	BYTE  wReserved;
} OSVERSIONINFOEXW, *LPOSVERSIONINFOEXW;
typedef OSVERSIONINFOEXW OSVERSIONINFOEX;

#define VER_PLATFORM_WIN32s        0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT      2

#define VER_NT_WORKSTATION       0x0000001
#define VER_NT_DOMAIN_CONTROLLER 0x0000002
#define VER_NT_SERVER            0x0000003

#define PROCESSOR_ARCHITECTURE_INTEL   0
#define PROCESSOR_ARCHITECTURE_MIPS    1
#define PROCESSOR_ARCHITECTURE_ALPHA   2
#define PROCESSOR_ARCHITECTURE_PPC     3
#define PROCESSOR_ARCHITECTURE_IA64    6
#define PROCESSOR_ARCHITECTURE_AMD64   9
#define PROCESSOR_ARCHITECTURE_ARM64   12

inline UINT GetACP() { return 65001; } // UTF-8

// GetVersionExW is stubbed to report Windows 10
BOOL GetVersionExW(LPOSVERSIONINFOW lpVersionInformation);
#define GetVersionEx GetVersionExW

inline DWORD GetVersion()
{
	// Return Windows 10 version info
	return 0x0A000000;
}

// ============================================================
// System time
// ============================================================
void GetLocalTime(LPSYSTEMTIME lpSystemTime);
void GetSystemTime(LPSYSTEMTIME lpSystemTime);
void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);
BOOL SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime, LPFILETIME lpFileTime);
BOOL FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
BOOL FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime);
BOOL LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime);
DWORD GetTimeZoneInformation(void* lpTimeZoneInformation);

// ============================================================
// Misc
// ============================================================
void OutputDebugStringW(LPCWSTR lpOutputString);
void OutputDebugStringA(LPCSTR lpOutputString);
#define OutputDebugString OutputDebugStringW

BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

// FormatMessage stub
DWORD FormatMessageW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                     DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list* Arguments);
#define FormatMessage FormatMessageW

// Command line
LPWSTR GetCommandLineW();
#define GetCommandLine GetCommandLineW

// Beep
BOOL Beep(DWORD dwFreq, DWORD dwDuration);

// Registry stubs (return failure)
#define HKEY_CLASSES_ROOT    ((HKEY)(ULONG_PTR)0x80000000)
#define HKEY_CURRENT_USER    ((HKEY)(ULONG_PTR)0x80000001)
#define HKEY_LOCAL_MACHINE   ((HKEY)(ULONG_PTR)0x80000002)
#define HKEY_USERS           ((HKEY)(ULONG_PTR)0x80000003)
#define HKEY_CURRENT_CONFIG  ((HKEY)(ULONG_PTR)0x80000005)

#define KEY_READ        0x20019
#define KEY_WRITE       0x20006
#define KEY_ALL_ACCESS  0xF003F
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE   0x0002
#define KEY_CREATE_SUB_KEY 0x0004
#define KEY_ENUMERATE_SUB_KEYS 0x0008

#define REG_NONE      0
#define REG_SZ        1
#define REG_EXPAND_SZ 2
#define REG_BINARY    3
#define REG_DWORD     4
#define REG_MULTI_SZ  7
#define REG_QWORD     11

LONG RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY* phkResult);
LONG RegCloseKey(HKEY hKey);
LONG RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG RegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData);
LONG RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass,
                     DWORD dwOptions, DWORD samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     HKEY* phkResult, LPDWORD lpdwDisposition);
LONG RegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey);
LONG RegDeleteValueW(HKEY hKey, LPCWSTR lpValueName);
LONG RegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcchName,
                   LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcchClass, PFILETIME lpftLastWriteTime);
LONG RegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName,
                   LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

#define RegOpenKeyEx RegOpenKeyExW
#define RegQueryValueEx RegQueryValueExW
#define RegSetValueEx RegSetValueExW
#define RegCreateKeyEx RegCreateKeyExW
#define RegDeleteKey RegDeleteKeyW
#define RegDeleteValue RegDeleteValueW
#define RegEnumKeyEx RegEnumKeyExW
#define RegEnumValue RegEnumValueW

// RegGetValue
#define RRF_RT_REG_DWORD  0x00000010
#define RRF_RT_REG_SZ     0x00000002
#define RRF_RT_REG_BINARY 0x00000008
inline LONG RegGetValueW(HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD dwFlags,
                          LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
	(void)hkey; (void)lpSubKey; (void)lpValue; (void)dwFlags; (void)pdwType; (void)pvData; (void)pcbData;
	return ERROR_FILE_NOT_FOUND;
}
#define RegGetValue RegGetValueW

LONG RegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
                      LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen,
                      LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
                      LPDWORD lpcbSecurityDescriptor, void* lpftLastWriteTime);
#define RegQueryInfoKey RegQueryInfoKeyW

// ============================================================
// Waitable objects
// ============================================================
HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
#define CreateEvent CreateEventW

BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);

HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName);
#define CreateMutex CreateMutexW

BOOL ReleaseMutex(HANDLE hMutex);

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);

// ============================================================
// Thread functions
// ============================================================
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                    DWORD dwCreationFlags, LPDWORD lpThreadId);

BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode);
BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);
DWORD ResumeThread(HANDLE hThread);
DWORD SuspendThread(HANDLE hThread);

#define CREATE_SUSPENDED 0x00000004
#define STILL_ACTIVE     259

// ============================================================
// Interlocked (pointer variants)
// ============================================================
inline void* InterlockedExchangePointer(void* volatile* Target, void* Value)
{
	return __sync_lock_test_and_set(Target, Value);
}

inline void* InterlockedCompareExchangePointer(void* volatile* Destination, void* Exchange, void* Comparand)
{
	return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}

// ============================================================
// Additional error codes
// ============================================================
#define ERROR_NO_UNICODE_TRANSLATION 1113L

// ============================================================
// File time comparison
// ============================================================
inline LONG CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
{
	if (!lpFileTime1 || !lpFileTime2) return 0;
	uint64_t t1 = (static_cast<uint64_t>(lpFileTime1->dwHighDateTime) << 32) | lpFileTime1->dwLowDateTime;
	uint64_t t2 = (static_cast<uint64_t>(lpFileTime2->dwHighDateTime) << 32) | lpFileTime2->dwLowDateTime;
	if (t1 < t2) return -1;
	if (t1 > t2) return 1;
	return 0;
}

// ============================================================
// Additional file operations
// ============================================================
BOOL FlushFileBuffers(HANDLE hFile);
BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, LARGE_INTEGER* lpNewFilePointer, DWORD dwMoveMethod);
BOOL SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime);
#define MOVEFILE_REPLACE_EXISTING 0x00000001
#define MOVEFILE_COPY_ALLOWED     0x00000002
#define MOVEFILE_WRITE_THROUGH    0x00000008

DWORD GetLongPathNameW(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer);
#define GetLongPathName GetLongPathNameW

DWORD GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);
#define GetFinalPathNameByHandle GetFinalPathNameByHandleW
#define FILE_NAME_NORMALIZED 0x0
#define FILE_NAME_OPENED     0x8
#define VOLUME_NAME_DOS      0x0
#define VOLUME_NAME_GUID     0x1
#define VOLUME_NAME_NT       0x2
#define VOLUME_NAME_NONE     0x4

DWORD ExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
#define ExpandEnvironmentStrings ExpandEnvironmentStringsW

BOOL GetDiskFreeSpaceExW(LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                         PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes);
#define GetDiskFreeSpaceEx GetDiskFreeSpaceExW

BOOL GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);

// FindFirstStreamW (Alternate Data Streams — stub on macOS)
typedef struct _WIN32_FIND_STREAM_DATA {
	LARGE_INTEGER StreamSize;
	WCHAR         cStreamName[MAX_PATH + 36];
} WIN32_FIND_STREAM_DATA, *PWIN32_FIND_STREAM_DATA;

typedef enum _STREAM_INFO_LEVELS {
	FindStreamInfoStandard = 0
} STREAM_INFO_LEVELS;

inline HANDLE FindFirstStreamW(LPCWSTR lpFileName, STREAM_INFO_LEVELS InfoLevel, LPVOID lpFindStreamData, DWORD dwFlags)
{
	(void)lpFileName; (void)InfoLevel; (void)lpFindStreamData; (void)dwFlags;
	return INVALID_HANDLE_VALUE; // No ADS on macOS
}

// PathIsNetworkPath
inline BOOL PathIsNetworkPathW(LPCWSTR pszPath) { (void)pszPath; return FALSE; }
#define PathIsNetworkPath PathIsNetworkPathW

// ============================================================
// Time/Date formatting
// ============================================================
#define DATE_SHORTDATE       0x00000001
#define DATE_LONGDATE        0x00000002
#define DATE_YEARMONTH       0x00000008
#define TIME_NOSECONDS       0x00000002
#define TIME_NOTIMEMARKER    0x00000004
#define TIME_FORCE24HOURFORMAT 0x00000008
BOOL SystemTimeToTzSpecificLocalTime(const void* lpTimeZone, const SYSTEMTIME* lpUniversalTime, LPSYSTEMTIME lpLocalTime);
int GetDateFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME* lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate);
int GetTimeFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME* lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime);
int GetTimeFormatEx(LPCWSTR lpLocaleName, DWORD dwFlags, const SYSTEMTIME* lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime);
int GetDateFormatEx(LPCWSTR lpLocaleName, DWORD dwFlags, const SYSTEMTIME* lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate, LPCWSTR lpCalendar);
#define GetDateFormat GetDateFormatW
#define GetTimeFormat GetTimeFormatW

// FormatMessageA
DWORD FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                     DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments);

// ============================================================
// MSVC secure string functions
// ============================================================
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

#ifndef _wcstod_l
#define _wcstod_l wcstod_l
#endif

// MSVC secure string functions (C++ overloads)
inline int wcscpy_s(wchar_t* dest, size_t destsz, const wchar_t* src) {
	if (!dest || !src) return -1;
	wcsncpy(dest, src, destsz - 1);
	dest[destsz - 1] = L'\0';
	return 0;
}
inline int wcscpy_s(wchar_t* dest, const wchar_t* src) {
	if (!dest || !src) return -1;
	wcscpy(dest, src);
	return 0;
}

#ifndef wcscat_s
inline int wcscat_s(wchar_t* dest, size_t destsz, const wchar_t* src) {
	if (!dest || !src) return -1;
	size_t destLen = wcslen(dest);
	if (destLen >= destsz) return -1;
	wcsncpy(dest + destLen, src, destsz - destLen - 1);
	dest[destsz - 1] = L'\0';
	return 0;
}
inline int wcscat_s(wchar_t* dest, const wchar_t* src) {
	if (!dest || !src) return -1;
	wcscat(dest, src);
	return 0;
}
#endif

// wcsncpy_s (4-arg form)
inline int wcsncpy_s(wchar_t* dest, size_t destsz, const wchar_t* src, size_t count) {
	if (!dest || destsz == 0) return -1;
	if (!src) { dest[0] = L'\0'; return -1; }
	size_t toCopy = (count < destsz - 1) ? count : destsz - 1;
	size_t srcLen = wcslen(src);
	if (toCopy > srcLen) toCopy = srcLen;
	wcsncpy(dest, src, toCopy);
	dest[toCopy] = L'\0';
	return 0;
}
// wcsncpy_s (3-arg array template, MSVC-style)
template<size_t N>
inline int wcsncpy_s(wchar_t (&dest)[N], const wchar_t* src, size_t count) {
	return wcsncpy_s(dest, N, src, count);
}

// ReplaceFile stub
#define REPLACEFILE_WRITE_THROUGH       0x00000001
#define REPLACEFILE_IGNORE_MERGE_ERRORS 0x00000002
#define REPLACEFILE_IGNORE_ACL_ERRORS   0x00000004

BOOL ReplaceFileW(LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName,
                  LPCWSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved);
#define ReplaceFile ReplaceFileW

// ============================================================
// Product info (version stubs)
// ============================================================
#define PRODUCT_STANDARD_SERVER_CORE              0x0000000D
#define PRODUCT_STANDARD_A_SERVER_CORE            0x00000092
#define PRODUCT_STANDARD_SERVER_CORE_V            0x00000028
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE    0x00000035
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE 0x0000003F
#define PRODUCT_ENTERPRISE_SERVER_CORE            0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_CORE_V          0x00000029
#define PRODUCT_DATACENTER_SERVER_CORE            0x0000000C
#define PRODUCT_DATACENTER_A_SERVER_CORE          0x00000091
#define PRODUCT_DATACENTER_SERVER_CORE_V          0x00000027
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE      0x0000002C
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE     0x0000002D
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE    0x0000002E
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE       0x0000002B
#define PRODUCT_WEB_SERVER_CORE                   0x0000001D

inline BOOL GetProductInfo(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion,
                           DWORD dwSpMajorVersion, DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType)
{
	if (pdwReturnedProductType) *pdwReturnedProductType = 0;
	return TRUE;
}

// ============================================================
// Monitor enumeration
// ============================================================
typedef BOOL (CALLBACK* MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);
BOOL EnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);

// ============================================================
// Application Restart / Recovery (stubs)
// ============================================================
#define RESTART_MAX_CMD_LINE 1024
#define RESTART_NO_CRASH     1
#define RESTART_NO_HANG      2
#define RESTART_NO_PATCH     4
#define RESTART_NO_REBOOT    8

inline HRESULT RegisterApplicationRestart(LPCWSTR pwzCommandline, DWORD dwFlags)
{
	(void)pwzCommandline; (void)dwFlags;
	return S_OK;
}

inline HRESULT UnregisterApplicationRestart()
{
	return S_OK;
}

inline HRESULT GetApplicationRestartSettings(HANDLE hProcess, LPWSTR pwzCommandline, PDWORD pcchSize, PDWORD pdwFlags)
{
	(void)hProcess; (void)pwzCommandline;
	if (pcchSize) *pcchSize = 0;
	if (pdwFlags) *pdwFlags = 0;
	return E_NOTIMPL;
}
