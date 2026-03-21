#pragma once
// Win32 Toolbar, ReBar, and ImageList shim (data-only)
// Private implementation header — not part of public API.

#ifdef __APPLE__

#include <cstdint>

// Initialize toolbar data for a newly created control.
void Win32Toolbar_Init(void* hwndVoid);

// Destroy toolbar data.
void Win32Toolbar_Destroy(void* hwndVoid);

// Handle a toolbar-specific message (TB_*).
// Returns true if the message was handled.
bool Win32Toolbar_HandleMessage(void* hwndVoid, unsigned int msg,
                                 uintptr_t wParam, intptr_t lParam,
                                 intptr_t& result);

// Initialize rebar data for a newly created control.
void Win32ReBar_Init(void* hwndVoid);

// Destroy rebar data.
void Win32ReBar_Destroy(void* hwndVoid);

// Handle a rebar-specific message (RB_*).
// Returns true if the message was handled.
bool Win32ReBar_HandleMessage(void* hwndVoid, unsigned int msg,
                                uintptr_t wParam, intptr_t lParam,
                                intptr_t& result);

#endif // __APPLE__
