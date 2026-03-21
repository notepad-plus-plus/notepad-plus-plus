#pragma once
// Win32 Status Bar shim: data + Win32StatusBarView backing
// Private implementation header — not part of public API.

#ifdef __APPLE__

#include <cstdint>

// Create the native status bar view. Returns void* (NSView*).
void* Win32StatusBar_CreateView(void* parentView, int width);

// Initialize status bar data for a newly created control.
void Win32StatusBar_Init(void* hwndVoid);

// Destroy status bar data.
void Win32StatusBar_Destroy(void* hwndVoid);

// Handle a status-bar-specific message (SB_*).
// Returns true if the message was handled.
bool Win32StatusBar_HandleMessage(void* hwndVoid, unsigned int msg,
                                   uintptr_t wParam, intptr_t lParam,
                                   intptr_t& result);

#endif // __APPLE__
