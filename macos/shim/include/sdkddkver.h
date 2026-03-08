#pragma once
// Win32 Shim: SDK/DDK version defines for macOS
// Provides Windows SDK version macros as no-ops

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00  // Windows 10
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

#ifndef WINVER
#define WINVER 0x0A00
#endif
