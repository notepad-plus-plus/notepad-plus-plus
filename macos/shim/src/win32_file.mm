// Win32 Shim: File I/O implementations for macOS
// Maps Win32 file operations to POSIX equivalents

#import <Foundation/Foundation.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <fnmatch.h>
#include "windows.h"
#include "shlwapi.h"

// ============================================================
// Helper: Convert wchar_t* to UTF-8 path
// ============================================================
static std::string WideToUTF8(LPCWSTR wide)
{
	if (!wide) return "";
	NSString* str = [[NSString alloc] initWithBytes:wide
	                                         length:wcslen(wide) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ? std::string([str UTF8String]) : "";
}

static void UTF8ToWide(const char* utf8, LPWSTR wide, int maxChars)
{
	if (!utf8 || !wide || maxChars <= 0) return;
	NSString* str = [NSString stringWithUTF8String:utf8];
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	int charCount = static_cast<int>(data.length / sizeof(wchar_t));
	int copyCount = (charCount < maxChars) ? charCount : maxChars - 1;
	memcpy(wide, data.bytes, copyCount * sizeof(wchar_t));
	wide[copyCount] = L'\0';
}

// Normalize path separators (backslash to forward slash)
static std::string normalizePath(const std::string& path)
{
	std::string result = path;
	for (auto& c : result) {
		if (c == '\\') c = '/';
	}
	return result;
}

// ============================================================
// CreateFile → open()
// ============================================================
HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if (!lpFileName) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}

	std::string path = normalizePath(WideToUTF8(lpFileName));

	int flags = 0;
	if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE))
		flags = O_RDWR;
	else if (dwDesiredAccess & GENERIC_WRITE)
		flags = O_WRONLY;
	else
		flags = O_RDONLY;

	switch (dwCreationDisposition) {
		case CREATE_NEW:
			flags |= O_CREAT | O_EXCL;
			break;
		case CREATE_ALWAYS:
			flags |= O_CREAT | O_TRUNC;
			break;
		case OPEN_EXISTING:
			break;
		case OPEN_ALWAYS:
			flags |= O_CREAT;
			break;
		case TRUNCATE_EXISTING:
			flags |= O_TRUNC;
			break;
	}

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd = open(path.c_str(), flags, mode);
	if (fd < 0) {
		switch (errno) {
			case ENOENT: SetLastError(ERROR_FILE_NOT_FOUND); break;
			case EACCES: SetLastError(ERROR_ACCESS_DENIED); break;
			case EEXIST: SetLastError(ERROR_FILE_EXISTS); break;
			default: SetLastError(ERROR_INVALID_PARAMETER); break;
		}
		return INVALID_HANDLE_VALUE;
	}

	// Store fd as handle (offset by 1 to avoid confusion with NULL)
	return reinterpret_cast<HANDLE>(static_cast<intptr_t>(fd + 1));
}

static int handleToFd(HANDLE h)
{
	return static_cast<int>(reinterpret_cast<intptr_t>(h)) - 1;
}

// ============================================================
// ReadFile
// ============================================================
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	int fd = handleToFd(hFile);
	ssize_t bytesRead = read(fd, lpBuffer, nNumberOfBytesToRead);
	if (bytesRead < 0) {
		if (lpNumberOfBytesRead) *lpNumberOfBytesRead = 0;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (lpNumberOfBytesRead) *lpNumberOfBytesRead = static_cast<DWORD>(bytesRead);
	return TRUE;
}

// ============================================================
// WriteFile
// ============================================================
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	int fd = handleToFd(hFile);
	ssize_t bytesWritten = write(fd, lpBuffer, nNumberOfBytesToWrite);
	if (bytesWritten < 0) {
		if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = 0;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = static_cast<DWORD>(bytesWritten);
	return TRUE;
}

// ============================================================
// CloseHandle
// ============================================================
BOOL CloseHandle(HANDLE hObject)
{
	if (hObject == INVALID_HANDLE_VALUE || hObject == nullptr) return FALSE;
	int fd = handleToFd(hObject);
	if (fd >= 0) {
		close(fd);
		return TRUE;
	}
	return FALSE;
}

// ============================================================
// SetFilePointer
// ============================================================
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	if (hFile == INVALID_HANDLE_VALUE) return static_cast<DWORD>(-1);
	int fd = handleToFd(hFile);
	int whence;
	switch (dwMoveMethod) {
		case FILE_BEGIN: whence = SEEK_SET; break;
		case FILE_CURRENT: whence = SEEK_CUR; break;
		case FILE_END: whence = SEEK_END; break;
		default: return static_cast<DWORD>(-1);
	}

	off_t offset = lDistanceToMove;
	if (lpDistanceToMoveHigh) {
		offset |= (static_cast<off_t>(*lpDistanceToMoveHigh) << 32);
	}

	off_t result = lseek(fd, offset, whence);
	if (result == -1) return static_cast<DWORD>(-1);

	if (lpDistanceToMoveHigh) {
		*lpDistanceToMoveHigh = static_cast<LONG>(result >> 32);
	}
	return static_cast<DWORD>(result & 0xFFFFFFFF);
}

// ============================================================
// SetEndOfFile
// ============================================================
BOOL SetEndOfFile(HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	int fd = handleToFd(hFile);
	off_t pos = lseek(fd, 0, SEEK_CUR);
	return ftruncate(fd, pos) == 0;
}

// ============================================================
// GetFileSize
// ============================================================
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
	if (hFile == INVALID_HANDLE_VALUE) return static_cast<DWORD>(-1);
	int fd = handleToFd(hFile);
	struct stat st;
	if (fstat(fd, &st) != 0) return static_cast<DWORD>(-1);
	if (lpFileSizeHigh) *lpFileSizeHigh = static_cast<DWORD>(st.st_size >> 32);
	return static_cast<DWORD>(st.st_size & 0xFFFFFFFF);
}

BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
	if (hFile == INVALID_HANDLE_VALUE || !lpFileSize) return FALSE;
	int fd = handleToFd(hFile);
	struct stat st;
	if (fstat(fd, &st) != 0) return FALSE;
	lpFileSize->QuadPart = st.st_size;
	return TRUE;
}

// ============================================================
// GetFileAttributes
// ============================================================
DWORD GetFileAttributesW(LPCWSTR lpFileName)
{
	if (!lpFileName) return static_cast<DWORD>(-1);
	std::string path = normalizePath(WideToUTF8(lpFileName));
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return static_cast<DWORD>(-1);
	}
	DWORD attrs = 0;
	if (S_ISDIR(st.st_mode)) attrs |= FILE_ATTRIBUTE_DIRECTORY;
	if (!(st.st_mode & S_IWUSR)) attrs |= FILE_ATTRIBUTE_READONLY;
	if (attrs == 0) attrs = FILE_ATTRIBUTE_NORMAL;
	return attrs;
}

BOOL GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId,
                          LPVOID lpFileInformation)
{
	(void)fInfoLevelId;
	if (!lpFileName || !lpFileInformation) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpFileName));
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}
	auto* data = static_cast<WIN32_FILE_ATTRIBUTE_DATA*>(lpFileInformation);
	memset(data, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
	if (S_ISDIR(st.st_mode)) data->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	if (!(st.st_mode & S_IWUSR)) data->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
	if (data->dwFileAttributes == 0) data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	data->nFileSizeLow = static_cast<DWORD>(st.st_size & 0xFFFFFFFF);
	data->nFileSizeHigh = static_cast<DWORD>((st.st_size >> 32) & 0xFFFFFFFF);
	return TRUE;
}

BOOL SetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
	if (!lpFileName) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpFileName));
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return FALSE;

	mode_t mode = st.st_mode;
	if (dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
		mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	} else {
		mode |= S_IWUSR;
	}
	return chmod(path.c_str(), mode) == 0;
}

// ============================================================
// DeleteFile / CopyFile / MoveFile
// ============================================================
BOOL DeleteFileW(LPCWSTR lpFileName)
{
	if (!lpFileName) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpFileName));
	return unlink(path.c_str()) == 0;
}

BOOL CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
	if (!lpExistingFileName || !lpNewFileName) return FALSE;
	NSString* src = [[NSString alloc] initWithBytes:lpExistingFileName
	                                         length:wcslen(lpExistingFileName) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	NSString* dst = [[NSString alloc] initWithBytes:lpNewFileName
	                                         length:wcslen(lpNewFileName) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	src = [src stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
	dst = [dst stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];

	NSFileManager* fm = [NSFileManager defaultManager];
	if (bFailIfExists && [fm fileExistsAtPath:dst]) {
		SetLastError(ERROR_FILE_EXISTS);
		return FALSE;
	}
	if (!bFailIfExists && [fm fileExistsAtPath:dst]) {
		[fm removeItemAtPath:dst error:nil];
	}
	NSError* error = nil;
	return [fm copyItemAtPath:src toPath:dst error:&error];
}

BOOL CopyFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags)
{
	(void)lpProgressRoutine; (void)lpData; (void)pbCancel;
	BOOL bFailIfExists = (dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS) ? TRUE : FALSE;
	return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

BOOL MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	return MoveFileExW(lpExistingFileName, lpNewFileName, 0);
}

BOOL MoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
	if (!lpExistingFileName || !lpNewFileName) return FALSE;
	std::string src = normalizePath(WideToUTF8(lpExistingFileName));
	std::string dst = normalizePath(WideToUTF8(lpNewFileName));

	if (dwFlags & MOVEFILE_REPLACE_EXISTING) {
		unlink(dst.c_str());
	}
	return rename(src.c_str(), dst.c_str()) == 0;
}

// ============================================================
// CreateDirectory / RemoveDirectory
// ============================================================
BOOL CreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	if (!lpPathName) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpPathName));
	return mkdir(path.c_str(), 0755) == 0;
}

BOOL RemoveDirectoryW(LPCWSTR lpPathName)
{
	if (!lpPathName) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpPathName));
	return rmdir(path.c_str()) == 0;
}

// ============================================================
// FindFirstFile / FindNextFile / FindClose
// ============================================================
struct FindFileData {
	DIR* dir;
	std::string dirPath;
	std::string pattern;
};

HANDLE FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (!lpFileName || !lpFindFileData) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}

	std::string fullPath = normalizePath(WideToUTF8(lpFileName));

	// Split into directory and pattern
	std::string dirPath, pattern;
	size_t lastSep = fullPath.rfind('/');
	if (lastSep != std::string::npos) {
		dirPath = fullPath.substr(0, lastSep);
		pattern = fullPath.substr(lastSep + 1);
	} else {
		dirPath = ".";
		pattern = fullPath;
	}

	DIR* dir = opendir(dirPath.c_str());
	if (!dir) {
		SetLastError(ERROR_PATH_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}

	auto* ffd = new FindFileData{dir, dirPath, pattern};

	// Find first matching entry
	if (FindNextFileW(reinterpret_cast<HANDLE>(ffd), lpFindFileData)) {
		return reinterpret_cast<HANDLE>(ffd);
	}

	closedir(dir);
	delete ffd;
	SetLastError(ERROR_FILE_NOT_FOUND);
	return INVALID_HANDLE_VALUE;
}

BOOL FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE || !lpFindFileData) return FALSE;
	auto* ffd = reinterpret_cast<FindFileData*>(hFindFile);

	struct dirent* entry;
	while ((entry = readdir(ffd->dir)) != nullptr) {
		// Skip . and ..
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// Match pattern
		if (fnmatch(ffd->pattern.c_str(), entry->d_name, 0) != 0)
			continue;

		// Fill in find data
		memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAW));
		UTF8ToWide(entry->d_name, lpFindFileData->cFileName, MAX_PATH);

		// Get stat info
		std::string entryPath = ffd->dirPath + "/" + entry->d_name;
		struct stat st;
		if (stat(entryPath.c_str(), &st) == 0) {
			if (S_ISDIR(st.st_mode))
				lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			else
				lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

			lpFindFileData->nFileSizeLow = static_cast<DWORD>(st.st_size & 0xFFFFFFFF);
			lpFindFileData->nFileSizeHigh = static_cast<DWORD>(st.st_size >> 32);

			// Convert modification time to FILETIME
			uint64_t ft = (static_cast<uint64_t>(st.st_mtime) + 11644473600ULL) * 10000000ULL;
			lpFindFileData->ftLastWriteTime.dwLowDateTime = static_cast<DWORD>(ft & 0xFFFFFFFF);
			lpFindFileData->ftLastWriteTime.dwHighDateTime = static_cast<DWORD>(ft >> 32);
		}

		return TRUE;
	}

	SetLastError(ERROR_NO_MORE_FILES);
	return FALSE;
}

BOOL FindClose(HANDLE hFindFile)
{
	if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) return FALSE;
	auto* ffd = reinterpret_cast<FindFileData*>(hFindFile);
	closedir(ffd->dir);
	delete ffd;
	return TRUE;
}

// ============================================================
// GetCurrentDirectory / SetCurrentDirectory
// ============================================================
DWORD GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	char buf[PATH_MAX];
	if (!getcwd(buf, sizeof(buf))) return 0;
	if (!lpBuffer || nBufferLength == 0) {
		return static_cast<DWORD>(strlen(buf) + 1);
	}
	UTF8ToWide(buf, lpBuffer, nBufferLength);
	return static_cast<DWORD>(wcslen(lpBuffer));
}

BOOL SetCurrentDirectoryW(LPCWSTR lpPathName)
{
	if (!lpPathName) return FALSE;
	std::string path = normalizePath(WideToUTF8(lpPathName));
	return chdir(path.c_str()) == 0;
}

// ============================================================
// Temp path / temp file
// ============================================================
DWORD GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	const char* tmp = getenv("TMPDIR");
	if (!tmp) tmp = "/tmp/";
	if (!lpBuffer || nBufferLength == 0) {
		return static_cast<DWORD>(strlen(tmp) + 1);
	}
	UTF8ToWide(tmp, lpBuffer, nBufferLength);
	return static_cast<DWORD>(wcslen(lpBuffer));
}

UINT GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName)
{
	if (!lpTempFileName) return 0;

	std::string dir = normalizePath(WideToUTF8(lpPathName));
	std::string prefix = WideToUTF8(lpPrefixString);

	char templ[PATH_MAX];
	snprintf(templ, sizeof(templ), "%s/%sXXXXXX", dir.c_str(), prefix.c_str());

	if (uUnique == 0) {
		int fd = mkstemp(templ);
		if (fd < 0) return 0;
		close(fd);
	}

	UTF8ToWide(templ, lpTempFileName, MAX_PATH);
	return 1;
}

// ============================================================
// GetFullPathName
// ============================================================
DWORD GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart)
{
	if (!lpFileName) return 0;
	std::string path = normalizePath(WideToUTF8(lpFileName));
	char resolved[PATH_MAX];
	if (!realpath(path.c_str(), resolved)) {
		// File doesn't exist yet, just normalize the path
		strncpy(resolved, path.c_str(), sizeof(resolved) - 1);
		resolved[sizeof(resolved) - 1] = '\0';
	}

	DWORD len = static_cast<DWORD>(strlen(resolved));
	if (!lpBuffer || nBufferLength == 0) return len + 1;
	if (len >= nBufferLength) return len + 1;

	UTF8ToWide(resolved, lpBuffer, nBufferLength);
	if (lpFilePart) {
		*lpFilePart = PathFindFileNameW(lpBuffer);
	}
	return static_cast<DWORD>(wcslen(lpBuffer));
}

// ============================================================
// FlushFileBuffers / SetFilePointerEx / SetFileTime
// ============================================================
BOOL FlushFileBuffers(HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	int fd = handleToFd(hFile);
	return fsync(fd) == 0;
}

BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, LARGE_INTEGER* lpNewFilePointer, DWORD dwMoveMethod)
{
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	int fd = handleToFd(hFile);
	int whence = SEEK_SET;
	if (dwMoveMethod == FILE_CURRENT) whence = SEEK_CUR;
	else if (dwMoveMethod == FILE_END) whence = SEEK_END;
	off_t result = lseek(fd, static_cast<off_t>(liDistanceToMove.QuadPart), whence);
	if (result == -1) return FALSE;
	if (lpNewFilePointer) lpNewFilePointer->QuadPart = result;
	return TRUE;
}

BOOL SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime)
{
	(void)hFile; (void)lpCreationTime; (void)lpLastAccessTime; (void)lpLastWriteTime;
	return TRUE; // Stub
}

// ============================================================
// GetLongPathName / GetFinalPathNameByHandle / ExpandEnvironmentStrings
// ============================================================
DWORD GetLongPathNameW(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer)
{
	if (!lpszShortPath) return 0;
	DWORD len = static_cast<DWORD>(wcslen(lpszShortPath));
	if (!lpszLongPath || cchBuffer == 0) return len + 1;
	if (len >= cchBuffer) return len + 1;
	wcscpy(lpszLongPath, lpszShortPath);
	return len;
}

DWORD GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
	(void)hFile; (void)dwFlags;
	if (lpszFilePath && cchFilePath > 0) lpszFilePath[0] = L'\0';
	return 0;
}

DWORD ExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize)
{
	if (!lpSrc) return 0;
	DWORD len = static_cast<DWORD>(wcslen(lpSrc));
	if (!lpDst || nSize == 0) return len + 1;
	if (len >= nSize) return len + 1;
	wcscpy(lpDst, lpSrc);
	return len + 1;
}

BOOL GetDiskFreeSpaceExW(LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                         PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
	(void)lpDirectoryName;
	if (lpFreeBytesAvailableToCaller) lpFreeBytesAvailableToCaller->QuadPart = 100ULL * 1024 * 1024 * 1024;
	if (lpTotalNumberOfBytes) lpTotalNumberOfBytes->QuadPart = 500ULL * 1024 * 1024 * 1024;
	if (lpTotalNumberOfFreeBytes) lpTotalNumberOfFreeBytes->QuadPart = 100ULL * 1024 * 1024 * 1024;
	return TRUE;
}

BOOL GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode)
{
	if (lpExitCode) *lpExitCode = 0;
	return TRUE;
}

int SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
	(void)lpFileOp;
	return 1; // Stub — return failure
}

// ============================================================
// Time formatting stubs
// ============================================================
BOOL SystemTimeToTzSpecificLocalTime(const void* lpTimeZone, const SYSTEMTIME* lpUniversalTime, LPSYSTEMTIME lpLocalTime)
{
	if (!lpUniversalTime || !lpLocalTime) return FALSE;
	*lpLocalTime = *lpUniversalTime;
	return TRUE;
}

int GetDateFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME* lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate)
{
	(void)Locale; (void)dwFlags; (void)lpFormat;
	if (!lpDate) return 0;
	wchar_t buf[64];
	int len = swprintf(buf, 64, L"%04d/%02d/%02d", lpDate->wYear, lpDate->wMonth, lpDate->wDay);
	if (cchDate == 0) return len + 1;
	if (lpDateStr && cchDate > len) { wcscpy(lpDateStr, buf); return len; }
	return 0;
}

int GetTimeFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME* lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
	(void)Locale; (void)dwFlags; (void)lpFormat;
	if (!lpTime) return 0;
	wchar_t buf[64];
	int len = swprintf(buf, 64, L"%02d:%02d:%02d", lpTime->wHour, lpTime->wMinute, lpTime->wSecond);
	if (cchTime == 0) return len + 1;
	if (lpTimeStr && cchTime > len) { wcscpy(lpTimeStr, buf); return len; }
	return 0;
}

int GetTimeFormatEx(LPCWSTR lpLocaleName, DWORD dwFlags, const SYSTEMTIME* lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
	(void)lpLocaleName;
	return GetTimeFormatW(0, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
}

int GetDateFormatEx(LPCWSTR lpLocaleName, DWORD dwFlags, const SYSTEMTIME* lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate, LPCWSTR lpCalendar)
{
	(void)lpLocaleName; (void)lpCalendar;
	return GetDateFormatW(0, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
}

DWORD FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                     DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments)
{
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		char* buf = static_cast<char*>(LocalAlloc(LMEM_FIXED, 64));
		snprintf(buf, 64, "Error %u", static_cast<unsigned>(dwMessageId));
		*reinterpret_cast<char**>(lpBuffer) = buf;
		return static_cast<DWORD>(strlen(buf));
	}
	if (!lpBuffer || nSize == 0) return 0;
	snprintf(lpBuffer, nSize, "Error %u", static_cast<unsigned>(dwMessageId));
	return static_cast<DWORD>(strlen(lpBuffer));
}

BOOL EnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
	(void)hdc; (void)lprcClip;
	if (!lpfnEnum) return FALSE;
	RECT monitorRect = {0, 0, 1920, 1080};
	lpfnEnum(reinterpret_cast<HMONITOR>(1), nullptr, &monitorRect, dwData);
	return TRUE;
}

// PathCompactPathExW stub
BOOL PathCompactPathExW(LPWSTR pszOut, LPCWSTR pszSrc, UINT cchMax, DWORD dwFlags)
{
	(void)dwFlags;
	if (!pszOut || !pszSrc || cchMax == 0) return FALSE;
	size_t len = wcslen(pszSrc);
	if (len < cchMax) {
		wcscpy(pszOut, pszSrc);
	} else if (cchMax > 4) {
		// Simplified: "...end_of_path"
		pszOut[0] = L'.'; pszOut[1] = L'.'; pszOut[2] = L'.';
		size_t tailLen = cchMax - 4;
		wcscpy(pszOut + 3, pszSrc + len - tailLen);
	} else {
		wcsncpy(pszOut, pszSrc, cchMax - 1);
		pszOut[cchMax - 1] = L'\0';
	}
	return TRUE;
}

BOOL ReplaceFileW(LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName,
                  LPCWSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved)
{
	(void)lpBackupFileName; (void)dwReplaceFlags; (void)lpExclude; (void)lpReserved;
	if (!lpReplacedFileName || !lpReplacementFileName) return FALSE;
	// Simple implementation: remove target, rename replacement to target
	std::string target = normalizePath(WideToUTF8(lpReplacedFileName));
	std::string replacement = normalizePath(WideToUTF8(lpReplacementFileName));
	unlink(target.c_str());
	return rename(replacement.c_str(), target.c_str()) == 0;
}

// ============================================================
// File mapping (stubs)
// ============================================================
#include <sys/mman.h>
HANDLE CreateFileMappingW(HANDLE hFile, void* lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName)
{
	(void)lpFileMappingAttributes; (void)lpName;
	// Return the file handle itself as the mapping handle (simplified)
	return hFile;
}

LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap)
{
	(void)hFileMappingObject; (void)dwDesiredAccess; (void)dwFileOffsetHigh; (void)dwFileOffsetLow; (void)dwNumberOfBytesToMap;
	return nullptr;
}

BOOL UnmapViewOfFile(LPCVOID lpBaseAddress)
{
	(void)lpBaseAddress;
	return TRUE;
}
