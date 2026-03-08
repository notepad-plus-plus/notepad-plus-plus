#pragma once
// HandleRegistry: Bidirectional mapping between HWND and native views
// HWND is an opaque pointer. This registry maps each HWND to its backing
// NSView*/NSWindow* and per-window state (WNDPROC, user data, styles, etc.).
//
// This header is included AFTER windows.h by all shim .mm files,
// so all Win32 types (HWND, WNDPROC, DWORD, etc.) are already available.

#ifdef __APPLE__

#include <cstdint>
#include <string>

// We rely on windows.h (our shim) being included before this header.
// The shim defines HWND, WNDPROC, DWORD, HINSTANCE, LPARAM, LONG_PTR, etc.

namespace HandleRegistry {

struct WindowInfo
{
	void* nativeView = nullptr;    // NSView* (or ScintillaView* cast to void*)
	void* nativeWindow = nullptr;  // NSWindow* (for top-level windows only)
	WNDPROC wndProc = nullptr;
	LONG_PTR userData = 0;
	LONG_PTR wndExtra[8] = {};     // Extra window memory (for DWL_xxx / GWLP_xxx)
	HWND parent = nullptr;
	DWORD style = 0;
	DWORD exStyle = 0;
	HINSTANCE hInst = nullptr;
	int controlId = 0;
	std::wstring className;
	std::wstring windowName;
	bool isScintilla = false;
	LPARAM createParam = 0;        // lpParam from CreateWindowEx
};

// Registered window class info (from RegisterClass/RegisterClassEx)
struct ClassInfo
{
	WNDPROC wndProc = nullptr;
	HINSTANCE hInstance = nullptr;
	int cbWndExtra = 0;
	DWORD style = 0;
	std::wstring className;
};

// Class registration
bool registerClass(const std::wstring& name, WNDPROC wndProc, HINSTANCE hInst,
                   int cbWndExtra = 0, DWORD style = 0);
const ClassInfo* findClass(const std::wstring& name);

// Window handle management
HWND createWindow(WindowInfo info);
WindowInfo* getWindowInfo(HWND hwnd);
void destroyWindow(HWND hwnd);

// Find HWND by native view pointer
HWND findByNativeView(void* nativeView);

// Find HWND by native window pointer
HWND findByNativeWindow(void* nativeWindow);

// Get all child HWNDs of a parent
void getChildren(HWND parent, void (*callback)(HWND child, void* context), void* context);

// Get the main application window (first top-level window created)
HWND getMainWindow();

} // namespace HandleRegistry

#endif // __APPLE__
