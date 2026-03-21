#pragma once
// Win32 Tab Control shim: data + NSSegmentedControl backing
// Private implementation header — not part of public API.

#ifdef __APPLE__

#include <cstdint>

// Forward declarations
typedef struct HWND__* HWND;

// Initialize tab control data for a newly created control.
void Win32TabControl_Init(void* hwndVoid, void* parentVoid);

// Destroy tab control data.
void Win32TabControl_Destroy(void* hwndVoid);

// Handle a tab-control-specific message (TCM_*).
// Returns true if the message was handled.
bool Win32TabControl_HandleMessage(void* hwndVoid, unsigned int msg,
                                    uintptr_t wParam, intptr_t lParam,
                                    intptr_t& result);

#endif // __APPLE__
