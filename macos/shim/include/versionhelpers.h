// versionhelpers.h — macOS shim for Windows version helper macros
// On macOS, all version checks return true (we're "at least Windows 10").

#pragma once

inline bool IsWindows7OrGreater() { return true; }
inline bool IsWindows8OrGreater() { return true; }
inline bool IsWindows10OrGreater() { return true; }
