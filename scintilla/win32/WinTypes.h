// Scintilla source code edit control
/** @file WinTypes.h
 ** Implement safe release of COM objects and access to functions in DLLs.
 ** Header contains all implementation - there is no .cxx file.
 **/
// Copyright 2020-2021 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WINTYPES_H
#define WINTYPES_H

namespace Scintilla::Internal {

// Release an IUnknown* and set to nullptr.
// While IUnknown::Release must be noexcept, it isn't marked as such so produces
// warnings which are avoided by the catch.
template <class T>
inline void ReleaseUnknown(T *&ppUnknown) noexcept {
	if (ppUnknown) {
		try {
			ppUnknown->Release();
		} catch (...) {
			// Never occurs
		}
		ppUnknown = nullptr;
	}
}

struct UnknownReleaser {
	// Called by unique_ptr to destroy/free the resource
	template <class T>
	void operator()(T *pUnknown) noexcept {
		try {
			pUnknown->Release();
		} catch (...) {
			// IUnknown::Release must not throw, ignore if it does.
		}
	}
};

/// Find a function in a DLL and convert to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
inline T DLLFunction(HMODULE hModule, LPCSTR lpProcName) noexcept {
	if (!hModule) {
		return nullptr;
	}
	FARPROC function = ::GetProcAddress(hModule, lpProcName);
	static_assert(sizeof(T) == sizeof(function));
	T fp {};
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

inline void ReleaseLibrary(HMODULE &hLib) noexcept {
	if (hLib) {
		FreeLibrary(hLib);
		hLib = {};
	}
}

}

#endif
