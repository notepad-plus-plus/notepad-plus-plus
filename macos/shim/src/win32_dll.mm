// Win32 Shim: DLL/dylib loading for macOS
// Maps LoadLibrary/GetProcAddress to dlopen/dlsym

#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <cstring>
#include <string>
#include "windows.h"

// ============================================================
// Helper: Convert wide path to UTF-8
// ============================================================
static std::string WideToUTF8(LPCWSTR wide)
{
	if (!wide) return "";
	NSString* str = [[NSString alloc] initWithBytes:wide
	                                         length:wcslen(wide) * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ? std::string([str UTF8String]) : "";
}

// ============================================================
// LoadLibrary → dlopen
// ============================================================
HMODULE LoadLibraryW(LPCWSTR lpLibFileName)
{
	return LoadLibraryExW(lpLibFileName, nullptr, 0);
}

HMODULE LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	if (!lpLibFileName) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return nullptr;
	}

	std::string libPath = WideToUTF8(lpLibFileName);

	// Convert backslashes to forward slashes
	for (auto& c : libPath) {
		if (c == '\\') c = '/';
	}

	// Convert .dll extension to .dylib
	size_t dllPos = libPath.rfind(".dll");
	if (dllPos != std::string::npos && dllPos == libPath.length() - 4) {
		libPath.replace(dllPos, 4, ".dylib");
	}

	// If LOAD_LIBRARY_AS_DATAFILE, just check existence
	if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) {
		// Return a non-null sentinel
		if (access(libPath.c_str(), F_OK) == 0) {
			return reinterpret_cast<HMODULE>(1);
		}
		SetLastError(ERROR_FILE_NOT_FOUND);
		return nullptr;
	}

	void* handle = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
	if (!handle) {
		// Try without path modification
		handle = dlopen(WideToUTF8(lpLibFileName).c_str(), RTLD_LAZY | RTLD_LOCAL);
	}
	if (!handle) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return nullptr;
	}
	return reinterpret_cast<HMODULE>(handle);
}

// ============================================================
// FreeLibrary → dlclose
// ============================================================
BOOL FreeLibrary(HMODULE hLibModule)
{
	if (!hLibModule || hLibModule == reinterpret_cast<HMODULE>(1)) return TRUE;
	return dlclose(hLibModule) == 0;
}

// ============================================================
// GetProcAddress → dlsym
// ============================================================
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if (!hModule || !lpProcName) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return nullptr;
	}
	void* sym = dlsym(hModule, lpProcName);
	if (!sym) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return nullptr;
	}
	return reinterpret_cast<FARPROC>(sym);
}

// ============================================================
// Thread functions (basic pthreads wrappers)
// ============================================================
struct ThreadData {
	LPTHREAD_START_ROUTINE startAddress;
	LPVOID parameter;
};

static void* threadProc(void* arg)
{
	auto* td = static_cast<ThreadData*>(arg);
	DWORD result = td->startAddress(td->parameter);
	delete td;
	return reinterpret_cast<void*>(static_cast<uintptr_t>(result));
}

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                    DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	auto* td = new ThreadData{lpStartAddress, lpParameter};

	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (dwStackSize > 0) {
		pthread_attr_setstacksize(&attr, dwStackSize);
	}

	int result = pthread_create(&thread, &attr, threadProc, td);
	pthread_attr_destroy(&attr);

	if (result != 0) {
		delete td;
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return nullptr;
	}

	if (lpThreadId) {
		*lpThreadId = static_cast<DWORD>(reinterpret_cast<uintptr_t>(thread) & 0xFFFFFFFF);
	}

	// Return pthread_t as handle
	auto* handle = new pthread_t(thread);
	return reinterpret_cast<HANDLE>(handle);
}

BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode)
{
	if (!hThread) return FALSE;
	auto* thread = reinterpret_cast<pthread_t*>(hThread);
	pthread_cancel(*thread);
	return TRUE;
}

BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode)
{
	if (!hThread || !lpExitCode) return FALSE;
	*lpExitCode = 0;
	return TRUE;
}

DWORD ResumeThread(HANDLE hThread)
{
	return 0; // Not directly supported
}

DWORD SuspendThread(HANDLE hThread)
{
	return 0; // Not directly supported
}

// ============================================================
// Event objects (basic implementation using condition variables)
// ============================================================
struct EventObject {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool signaled;
	bool manualReset;
};

HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
{
	auto* event = new EventObject;
	pthread_mutex_init(&event->mutex, nullptr);
	pthread_cond_init(&event->cond, nullptr);
	event->signaled = bInitialState != FALSE;
	event->manualReset = bManualReset != FALSE;
	return reinterpret_cast<HANDLE>(event);
}

BOOL SetEvent(HANDLE hEvent)
{
	if (!hEvent) return FALSE;
	auto* event = reinterpret_cast<EventObject*>(hEvent);
	pthread_mutex_lock(&event->mutex);
	event->signaled = true;
	if (event->manualReset)
		pthread_cond_broadcast(&event->cond);
	else
		pthread_cond_signal(&event->cond);
	pthread_mutex_unlock(&event->mutex);
	return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
	if (!hEvent) return FALSE;
	auto* event = reinterpret_cast<EventObject*>(hEvent);
	pthread_mutex_lock(&event->mutex);
	event->signaled = false;
	pthread_mutex_unlock(&event->mutex);
	return TRUE;
}

HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName)
{
	auto* mutex = new pthread_mutex_t;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	if (bInitialOwner) pthread_mutex_lock(mutex);
	return reinterpret_cast<HANDLE>(mutex);
}

BOOL ReleaseMutex(HANDLE hMutex)
{
	if (!hMutex) return FALSE;
	auto* mutex = reinterpret_cast<pthread_mutex_t*>(hMutex);
	return pthread_mutex_unlock(mutex) == 0;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	if (!hHandle) return WAIT_FAILED;
	// Basic implementation - only handles events for now
	auto* event = reinterpret_cast<EventObject*>(hHandle);
	pthread_mutex_lock(&event->mutex);

	if (dwMilliseconds == 0) {
		if (event->signaled) {
			if (!event->manualReset) event->signaled = false;
			pthread_mutex_unlock(&event->mutex);
			return WAIT_OBJECT_0;
		}
		pthread_mutex_unlock(&event->mutex);
		return WAIT_TIMEOUT;
	}

	while (!event->signaled) {
		if (dwMilliseconds == INFINITE) {
			pthread_cond_wait(&event->cond, &event->mutex);
		} else {
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += dwMilliseconds / 1000;
			ts.tv_nsec += (dwMilliseconds % 1000) * 1000000;
			if (ts.tv_nsec >= 1000000000) {
				ts.tv_sec++;
				ts.tv_nsec -= 1000000000;
			}
			int result = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
			if (result != 0) {
				pthread_mutex_unlock(&event->mutex);
				return WAIT_TIMEOUT;
			}
		}
	}

	if (!event->manualReset) event->signaled = false;
	pthread_mutex_unlock(&event->mutex);
	return WAIT_OBJECT_0;
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
	// Simplified: just wait for the first one
	if (nCount == 0 || !lpHandles) return WAIT_FAILED;
	return WaitForSingleObject(lpHandles[0], dwMilliseconds);
}

// ============================================================
// CreateProcess (basic implementation via posix_spawn)
// ============================================================
BOOL CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                    BOOL bInheritHandles, DWORD dwCreationFlags,
                    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                    LPSTARTUPINFOW lpStartupInfo,
                    LPPROCESS_INFORMATION lpProcessInformation)
{
	// Stub - will be properly implemented when needed
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return FALSE;
}
