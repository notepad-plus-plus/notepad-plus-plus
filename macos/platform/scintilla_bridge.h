#pragma once
// Scintilla Cocoa Bridge
// Provides a C interface to ScintillaView (Cocoa) that can be used by
// Win32 shim code without header conflicts. ScintillaView.h defines
// WM_COMMAND/WM_NOTIFY with Cocoa-specific values (1001/1002) which
// conflict with our Win32 shim values (0x0111/0x004E).
// This bridge isolates the two worlds.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Create a ScintillaView as a subview of the given parent NSView.
// parentView: NSView* (passed as void*)
// Returns: ScintillaView* (passed as void*), or NULL on failure.
void* ScintillaBridge_createView(void* parentView, int x, int y, int width, int height);

// Remove and release a ScintillaView.
void ScintillaBridge_destroyView(void* scintillaView);

// Send a Scintilla message (SCI_xxx). Returns the result.
intptr_t ScintillaBridge_sendMessage(void* scintillaView, unsigned int message,
                                     uintptr_t wParam, intptr_t lParam);

// Get the Scintilla direct function pointer.
// Returns a function pointer: intptr_t (*)(void* ptr, unsigned int msg, uintptr_t wParam, intptr_t lParam)
void* ScintillaBridge_getDirectFunction(void* scintillaView);

// Get the Scintilla direct pointer (context for the direct function).
void* ScintillaBridge_getDirectPointer(void* scintillaView);

// Resize the ScintillaView to fill its superview's bounds.
void ScintillaBridge_resizeToFit(void* scintillaView);

// Notification callback type.
// iMessage is either 1001 (WM_COMMAND in Cocoa Scintilla) or 1002 (WM_NOTIFY in Cocoa Scintilla).
typedef void (*ScintillaBridgeNotifyFunc)(intptr_t windowid, unsigned int iMessage,
                                          uintptr_t wParam, uintptr_t lParam);

// Register a notification callback on the ScintillaView.
// windowid: opaque value passed back in the callback (typically the HWND).
void ScintillaBridge_setNotifyCallback(void* scintillaView, intptr_t windowid,
                                        ScintillaBridgeNotifyFunc callback);

#ifdef __cplusplus
}
#endif
