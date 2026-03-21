#pragma once
// Win32 dialog controls shim: Button, Edit, Static, ComboBox
// Private implementation header — not part of public API.

#ifdef __APPLE__

#include <cstdint>

// Initialize button control (set up action target).
void Win32Button_Init(void* hwndVoid);

// Destroy button control data.
void Win32Button_Destroy(void* hwndVoid);

// Handle button-specific messages (BM_*, WM_SETTEXT, etc.).
bool Win32Button_HandleMessage(void* hwndVoid, unsigned int msg,
                                uintptr_t wParam, intptr_t lParam,
                                intptr_t& result);

// Initialize edit control (set up text change delegate).
void Win32Edit_Init(void* hwndVoid);

// Destroy edit control data.
void Win32Edit_Destroy(void* hwndVoid);

// Handle edit-specific messages (EM_*, WM_SETTEXT, etc.).
bool Win32Edit_HandleMessage(void* hwndVoid, unsigned int msg,
                               uintptr_t wParam, intptr_t lParam,
                               intptr_t& result);

// Handle static control messages (WM_SETTEXT, STM_*, etc.).
bool Win32Static_HandleMessage(void* hwndVoid, unsigned int msg,
                                 uintptr_t wParam, intptr_t lParam,
                                 intptr_t& result);

// Handle combo box messages (CB_*, WM_SETTEXT, etc.).
bool Win32ComboBox_HandleMessage(void* hwndVoid, unsigned int msg,
                                   uintptr_t wParam, intptr_t lParam,
                                   intptr_t& result);

#endif // __APPLE__
