// Win32 Shim: Window management for macOS
// Phase 1: Real NSView/NSWindow-backed windows via HandleRegistry.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "scintilla_bridge.h"

// ============================================================
// Window class registration
// ============================================================
ATOM RegisterClassExW(const WNDCLASSEXW* lpWndClass)
{
	if (!lpWndClass || !lpWndClass->lpszClassName)
		return 0;

	std::wstring name(lpWndClass->lpszClassName);
	HandleRegistry::registerClass(name, lpWndClass->lpfnWndProc,
	                              lpWndClass->hInstance,
	                              lpWndClass->cbWndExtra,
	                              lpWndClass->style);

	static ATOM nextAtom = 1;
	return nextAtom++;
}

ATOM RegisterClassW(const WNDCLASSW* lpWndClass)
{
	if (!lpWndClass || !lpWndClass->lpszClassName)
		return 0;

	std::wstring name(lpWndClass->lpszClassName);
	HandleRegistry::registerClass(name, lpWndClass->lpfnWndProc,
	                              lpWndClass->hInstance,
	                              lpWndClass->cbWndExtra,
	                              lpWndClass->style);

	static ATOM nextAtom = 100;
	return nextAtom++;
}

BOOL UnregisterClassW(LPCWSTR lpClassName, HINSTANCE hInstance)
{
	return TRUE;
}

// ============================================================
// Window creation / destruction
// ============================================================

// Helper: Convert wchar_t* (32-bit on macOS) to NSString
static NSString* WideToNSString(const wchar_t* wstr)
{
	if (!wstr)
		return @"";
	size_t len = wcslen(wstr);
	// On macOS, wchar_t is 32-bit (UTF-32)
	NSString* str = [[NSString alloc] initWithBytes:wstr
	                                         length:len * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ?: @"";
}

HWND CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
                     DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                     HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	std::wstring className = lpClassName ? lpClassName : L"";
	std::wstring windowName = lpWindowName ? lpWindowName : L"";

	HandleRegistry::WindowInfo info;
	info.className = className;
	info.windowName = windowName;
	info.style = dwStyle;
	info.exStyle = dwExStyle;
	info.hInst = hInstance;
	info.parent = hWndParent;
	info.createParam = reinterpret_cast<LPARAM>(lpParam);

	// Look up registered class for WndProc
	const auto* classInfo = HandleRegistry::findClass(className);
	if (classInfo)
		info.wndProc = classInfo->wndProc;

	// Determine control ID from hMenu for child windows
	if (dwStyle & WS_CHILD)
		info.controlId = static_cast<int>(reinterpret_cast<intptr_t>(hMenu));

	// Check if this is a Scintilla editor
	if (className == L"Scintilla")
	{
		info.isScintilla = true;

		// Get parent NSView
		void* parentView = nullptr;
		if (hWndParent)
		{
			auto* parentInfo = HandleRegistry::getWindowInfo(hWndParent);
			if (parentInfo)
				parentView = parentInfo->nativeView;
		}

		if (parentView)
		{
			void* sciView = ScintillaBridge_createView(parentView, X, Y, nWidth, nHeight);
			if (sciView)
			{
				info.nativeView = sciView;
				HWND hwnd = HandleRegistry::createWindow(info);
				return hwnd;
			}
		}

		// If no parent view, can't create Scintilla yet — return a placeholder
		HWND hwnd = HandleRegistry::createWindow(info);
		return hwnd;
	}

	// Top-level window (has WS_OVERLAPPEDWINDOW or similar, no WS_CHILD)
	if (!(dwStyle & WS_CHILD))
	{
		NSUInteger styleMask = NSWindowStyleMaskTitled |
		                       NSWindowStyleMaskClosable |
		                       NSWindowStyleMaskMiniaturizable |
		                       NSWindowStyleMaskResizable;

		// Default size if not specified
		if (nWidth <= 0) nWidth = 800;
		if (nHeight <= 0) nHeight = 600;
		if (X == CW_USEDEFAULT || X < 0) X = 100;
		if (Y == CW_USEDEFAULT || Y < 0) Y = 100;

		NSRect contentRect = NSMakeRect(X, Y, nWidth, nHeight);
		NSWindow* nsWindow = [[NSWindow alloc] initWithContentRect:contentRect
		                                       styleMask:styleMask
		                                       backing:NSBackingStoreBuffered
		                                       defer:NO];

		NSString* title = WideToNSString(lpWindowName);
		[nsWindow setTitle:title];
		[nsWindow setReleasedWhenClosed:NO];

		info.nativeWindow = (__bridge void*)nsWindow;
		info.nativeView = (__bridge void*)[nsWindow contentView];

		HWND hwnd = HandleRegistry::createWindow(info);

		// Send WM_CREATE to the WndProc
		if (info.wndProc)
		{
			CREATESTRUCTW cs = {};
			cs.lpCreateParams = lpParam;
			cs.hInstance = hInstance;
			cs.hMenu = hMenu;
			cs.hwndParent = hWndParent;
			cs.cx = nWidth;
			cs.cy = nHeight;
			cs.x = X;
			cs.y = Y;
			cs.style = static_cast<LONG>(dwStyle);
			cs.lpszName = lpWindowName;
			cs.lpszClass = lpClassName;
			cs.dwExStyle = dwExStyle;

			LRESULT result = info.wndProc(hwnd, WM_CREATE, 0, reinterpret_cast<LPARAM>(&cs));
			if (result == -1)
			{
				HandleRegistry::destroyWindow(hwnd);
				return nullptr;
			}
		}

		return hwnd;
	}

	// Child window (WS_CHILD set)
	{
		void* parentView = nullptr;
		if (hWndParent)
		{
			auto* parentInfo = HandleRegistry::getWindowInfo(hWndParent);
			if (parentInfo)
				parentView = parentInfo->nativeView;
		}

		if (parentView)
		{
			NSView* parent = (__bridge NSView*)parentView;
			CGFloat parentH = parent.bounds.size.height;
			NSRect frame = NSMakeRect(X, parentH - Y - nHeight, nWidth, nHeight);

			NSView* childView = [[NSView alloc] initWithFrame:frame];
			[parent addSubview:childView];

			info.nativeView = (__bridge void*)childView;
		}

		HWND hwnd = HandleRegistry::createWindow(info);

		// Send WM_CREATE
		if (info.wndProc)
		{
			CREATESTRUCTW cs = {};
			cs.lpCreateParams = lpParam;
			cs.hInstance = hInstance;
			cs.hMenu = hMenu;
			cs.hwndParent = hWndParent;
			cs.cx = nWidth;
			cs.cy = nHeight;
			cs.x = X;
			cs.y = Y;
			cs.style = static_cast<LONG>(dwStyle);
			cs.lpszName = lpWindowName;
			cs.lpszClass = lpClassName;
			cs.dwExStyle = dwExStyle;

			info.wndProc(hwnd, WM_CREATE, 0, reinterpret_cast<LPARAM>(&cs));
		}

		return hwnd;
	}
}

BOOL DestroyWindow(HWND hWnd)
{
	if (!hWnd)
		return FALSE;

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->wndProc)
		info->wndProc(hWnd, WM_DESTROY, 0, 0);

	HandleRegistry::destroyWindow(hWnd);
	return TRUE;
}

// ============================================================
// Window state
// ============================================================

BOOL ShowWindow(HWND hWnd, int nCmdShow)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return FALSE;

	if (info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		switch (nCmdShow)
		{
			case SW_SHOW:
			case SW_SHOWNORMAL:
			case SW_SHOWDEFAULT:
				[window makeKeyAndOrderFront:nil];
				break;
			case SW_MAXIMIZE: // same as SW_SHOWMAXIMIZED
				[window makeKeyAndOrderFront:nil];
				if (![window isZoomed])
					[window zoom:nil];
				break;
			case SW_MINIMIZE:
			case SW_SHOWMINIMIZED:
				[window miniaturize:nil];
				break;
			case SW_HIDE:
				[window orderOut:nil];
				break;
			case SW_RESTORE:
				if ([window isZoomed])
					[window zoom:nil];
				if ([window isMiniaturized])
					[window deminiaturize:nil];
				[window makeKeyAndOrderFront:nil];
				break;
			default:
				[window makeKeyAndOrderFront:nil];
				break;
		}
	}
	else if (info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		[view setHidden:(nCmdShow == SW_HIDE)];
	}

	return TRUE;
}

BOOL UpdateWindow(HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		[view setNeedsDisplay:YES];
	}
	return TRUE;
}

BOOL EnableWindow(HWND hWnd, BOOL bEnable) { return TRUE; }
BOOL IsWindowEnabled(HWND hWnd) { return TRUE; }

BOOL IsWindowVisible(HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return FALSE;
	if (info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		return [window isVisible] ? TRUE : FALSE;
	}
	if (info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		return ![view isHidden];
	}
	return FALSE;
}

BOOL IsWindow(HWND hWnd)
{
	return HandleRegistry::getWindowInfo(hWnd) != nullptr;
}

BOOL IsChild(HWND hWndParent, HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	return (info && info->parent == hWndParent) ? TRUE : FALSE;
}

HWND SetFocus(HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		[window makeFirstResponder:(__bridge NSView*)info->nativeView];
	}
	return hWnd;
}

HWND GetFocus() { return nullptr; }
HWND SetActiveWindow(HWND hWnd) { return hWnd; }
HWND GetActiveWindow() { return HandleRegistry::getMainWindow(); }
HWND GetForegroundWindow() { return HandleRegistry::getMainWindow(); }
BOOL SetForegroundWindow(HWND hWnd) { return TRUE; }
BOOL BringWindowToTop(HWND hWnd) { return TRUE; }

HWND GetAncestor(HWND hWnd, UINT gaFlags)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return hWnd;

	if (gaFlags == GA_PARENT)
		return info->parent ? info->parent : hWnd;

	// GA_ROOT / GA_ROOTOWNER: walk up to top-level
	HWND current = hWnd;
	while (true)
	{
		auto* cur = HandleRegistry::getWindowInfo(current);
		if (!cur || !cur->parent)
			return current;
		current = cur->parent;
	}
}

HWND GetParent(HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	return info ? info->parent : nullptr;
}

HWND SetParent(HWND hWndChild, HWND hWndNewParent)
{
	auto* info = HandleRegistry::getWindowInfo(hWndChild);
	if (!info)
		return nullptr;
	HWND oldParent = info->parent;
	info->parent = hWndNewParent;
	return oldParent;
}

HWND GetWindow(HWND hWnd, UINT uCmd) { return nullptr; }
HWND GetDlgItem(HWND hDlg, int nIDDlgItem) { return nullptr; }
int GetDlgCtrlID(HWND hWnd) { return 0; }

// ============================================================
// Window long / class long
// ============================================================

LONG_PTR GetWindowLongPtrW(HWND hWnd, int nIndex)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return 0;

	switch (nIndex)
	{
		case GWL_STYLE:     return static_cast<LONG_PTR>(info->style);
		case GWL_EXSTYLE:   return static_cast<LONG_PTR>(info->exStyle);
		case GWLP_WNDPROC:  return reinterpret_cast<LONG_PTR>(info->wndProc);
		case GWLP_USERDATA: return info->userData;
		case GWLP_HINSTANCE: return reinterpret_cast<LONG_PTR>(info->hInst);
		case GWLP_ID:       return info->controlId;
		default:
			// Extra window bytes
			if (nIndex >= 0 && nIndex < static_cast<int>(sizeof(info->wndExtra)/sizeof(info->wndExtra[0])) * static_cast<int>(sizeof(LONG_PTR)))
			{
				int idx = nIndex / static_cast<int>(sizeof(LONG_PTR));
				return info->wndExtra[idx];
			}
			return 0;
	}
}

LONG_PTR SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return 0;

	LONG_PTR old = 0;
	switch (nIndex)
	{
		case GWL_STYLE:
			old = static_cast<LONG_PTR>(info->style);
			info->style = static_cast<DWORD>(dwNewLong);
			break;
		case GWL_EXSTYLE:
			old = static_cast<LONG_PTR>(info->exStyle);
			info->exStyle = static_cast<DWORD>(dwNewLong);
			break;
		case GWLP_WNDPROC:
			old = reinterpret_cast<LONG_PTR>(info->wndProc);
			info->wndProc = reinterpret_cast<WNDPROC>(dwNewLong);
			break;
		case GWLP_USERDATA:
			old = info->userData;
			info->userData = dwNewLong;
			break;
		case GWLP_ID:
			old = info->controlId;
			info->controlId = static_cast<int>(dwNewLong);
			break;
		default:
			if (nIndex >= 0 && nIndex < static_cast<int>(sizeof(info->wndExtra)/sizeof(info->wndExtra[0])) * static_cast<int>(sizeof(LONG_PTR)))
			{
				int idx = nIndex / static_cast<int>(sizeof(LONG_PTR));
				old = info->wndExtra[idx];
				info->wndExtra[idx] = dwNewLong;
			}
			break;
	}
	return old;
}

LONG_PTR GetClassLongPtrW(HWND hWnd, int nIndex) { return 0; }
LONG_PTR SetClassLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong) { return 0; }

// ============================================================
// Window geometry
// ============================================================

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect)
{
	if (!lpRect)
		return FALSE;

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		NSRect frame = [window frame];
		// Convert to screen coordinates (Win32 style: top-left origin)
		NSRect screenFrame = [[NSScreen mainScreen] frame];
		lpRect->left = static_cast<LONG>(frame.origin.x);
		lpRect->top = static_cast<LONG>(screenFrame.size.height - frame.origin.y - frame.size.height);
		lpRect->right = lpRect->left + static_cast<LONG>(frame.size.width);
		lpRect->bottom = lpRect->top + static_cast<LONG>(frame.size.height);
		return TRUE;
	}
	if (info && info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		NSRect frame = [view frame];
		lpRect->left = static_cast<LONG>(frame.origin.x);
		lpRect->top = static_cast<LONG>(frame.origin.y);
		lpRect->right = lpRect->left + static_cast<LONG>(frame.size.width);
		lpRect->bottom = lpRect->top + static_cast<LONG>(frame.size.height);
		return TRUE;
	}

	// Default fallback
	lpRect->left = 0; lpRect->top = 0;
	lpRect->right = 800; lpRect->bottom = 600;
	return TRUE;
}

BOOL GetClientRect(HWND hWnd, LPRECT lpRect)
{
	if (!lpRect)
		return FALSE;

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		NSRect contentRect = [[window contentView] bounds];
		lpRect->left = 0;
		lpRect->top = 0;
		lpRect->right = static_cast<LONG>(contentRect.size.width);
		lpRect->bottom = static_cast<LONG>(contentRect.size.height);
		return TRUE;
	}
	if (info && info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		NSRect bounds = [view bounds];
		lpRect->left = 0;
		lpRect->top = 0;
		lpRect->right = static_cast<LONG>(bounds.size.width);
		lpRect->bottom = static_cast<LONG>(bounds.size.height);
		return TRUE;
	}

	lpRect->left = 0; lpRect->top = 0;
	lpRect->right = 800; lpRect->bottom = 600;
	return TRUE;
}

BOOL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeWindow)
	{
		NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
		NSRect screenFrame = [[NSScreen mainScreen] frame];
		CGFloat cocoaY = screenFrame.size.height - Y - nHeight;
		NSRect frame = NSMakeRect(X, cocoaY, nWidth, nHeight);
		[window setFrame:frame display:bRepaint];
		return TRUE;
	}
	if (info && info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		NSView* parent = [view superview];
		CGFloat parentH = parent ? parent.bounds.size.height : nHeight;
		NSRect frame = NSMakeRect(X, parentH - Y - nHeight, nWidth, nHeight);
		[view setFrame:frame];
		if (bRepaint)
			[view setNeedsDisplay:YES];
		return TRUE;
	}
	return TRUE;
}

BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	if (uFlags & SWP_NOMOVE && uFlags & SWP_NOSIZE)
		return TRUE; // Nothing to do

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return TRUE;

	if (!(uFlags & SWP_NOSIZE) || !(uFlags & SWP_NOMOVE))
	{
		if (info->nativeView)
		{
			NSView* view = (__bridge NSView*)info->nativeView;
			NSRect currentFrame = [view frame];

			CGFloat newX = (uFlags & SWP_NOMOVE) ? currentFrame.origin.x : X;
			CGFloat newW = (uFlags & SWP_NOSIZE) ? currentFrame.size.width : cx;
			CGFloat newH = (uFlags & SWP_NOSIZE) ? currentFrame.size.height : cy;
			CGFloat newY;

			if (uFlags & SWP_NOMOVE)
			{
				newY = currentFrame.origin.y;
			}
			else
			{
				NSView* parent = [view superview];
				CGFloat parentH = parent ? parent.bounds.size.height : newH;
				newY = parentH - Y - newH;
			}

			[view setFrame:NSMakeRect(newX, newY, newW, newH)];
		}
	}

	if (!(uFlags & SWP_NOZORDER) && info->nativeWindow)
	{
		// TODO: z-order management
	}

	if (uFlags & SWP_SHOWWINDOW)
		ShowWindow(hWnd, SW_SHOW);
	if (uFlags & SWP_HIDEWINDOW)
		ShowWindow(hWnd, SW_HIDE);

	return TRUE;
}

// ============================================================
// Window text
// ============================================================

int GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	if (!lpString || nMaxCount <= 0)
		return 0;
	lpString[0] = L'\0';

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info)
	{
		size_t maxLen = static_cast<size_t>(nMaxCount - 1);
		size_t copyLen = info->windowName.size() < maxLen ? info->windowName.size() : maxLen;
		wcsncpy(lpString, info->windowName.c_str(), copyLen);
		lpString[copyLen] = L'\0';
		return static_cast<int>(copyLen);
	}
	return 0;
}

BOOL SetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info)
	{
		info->windowName = lpString ? lpString : L"";
		if (info->nativeWindow)
		{
			NSWindow* window = (__bridge NSWindow*)info->nativeWindow;
			[window setTitle:WideToNSString(lpString)];
		}
	}
	return TRUE;
}

int GetWindowTextLengthW(HWND hWnd)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	return info ? static_cast<int>(info->windowName.size()) : 0;
}

int GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
	if (lpString && nMaxCount > 0) lpString[0] = '\0';
	return 0;
}

HWND GetLastActivePopup(HWND hWnd) { return hWnd; }
BOOL LockWindowUpdate(HWND hWndLock) { return TRUE; }

BOOL InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeView)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		if (lpRect)
		{
			CGFloat parentH = view.bounds.size.height;
			NSRect r = NSMakeRect(lpRect->left, parentH - lpRect->bottom,
			                      lpRect->right - lpRect->left,
			                      lpRect->bottom - lpRect->top);
			[view setNeedsDisplayInRect:r];
		}
		else
		{
			[view setNeedsDisplay:YES];
		}
	}
	return TRUE;
}

BOOL RedrawWindow(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags)
{
	return InvalidateRect(hWnd, lprcUpdate, TRUE);
}

// ============================================================
// Paint
// ============================================================
HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint)
{
	if (lpPaint) memset(lpPaint, 0, sizeof(PAINTSTRUCT));
	return nullptr;
}

BOOL EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint) { return TRUE; }
HDC GetDC(HWND hWnd) { return nullptr; }
HDC GetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags) { return nullptr; }
HDC GetWindowDC(HWND hWnd) { return nullptr; }
int ReleaseDC(HWND hWnd, HDC hDC) { return 1; }

// ============================================================
// Coordinate conversion
// ============================================================
BOOL ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (info && info->nativeView && lpPoint)
	{
		NSView* view = (__bridge NSView*)info->nativeView;
		NSWindow* window = [view window];
		if (window)
		{
			NSPoint screenPt = NSMakePoint(lpPoint->x, lpPoint->y);
			NSPoint windowPt = [window convertPointFromScreen:screenPt];
			NSPoint viewPt = [view convertPoint:windowPt fromView:nil];
			lpPoint->x = static_cast<LONG>(viewPt.x);
			lpPoint->y = static_cast<LONG>(view.bounds.size.height - viewPt.y);
		}
	}
	return TRUE;
}

BOOL ClientToScreen(HWND hWnd, LPPOINT lpPoint) { return TRUE; }
HWND WindowFromPoint(POINT Point) { return nullptr; }
HWND ChildWindowFromPoint(HWND hWndParent, POINT Point) { return nullptr; }
int MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints) { return 0; }

// ============================================================
// Capture
// ============================================================
BOOL SetCapture_shim(HWND hWnd) { return TRUE; }
BOOL ReleaseCapture() { return TRUE; }
HWND GetCapture() { return nullptr; }

// ============================================================
// Cursor / Icon
// ============================================================
HCURSOR SetCursor(HCURSOR hCursor) { return hCursor; }
HCURSOR LoadCursorW(HINSTANCE hInstance, LPCWSTR lpCursorName) { return nullptr; }
HICON LoadIconW(HINSTANCE hInstance, LPCWSTR lpIconName) { return nullptr; }
HANDLE LoadImageW(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad) { return nullptr; }

// ============================================================
// Timer
// ============================================================
UINT_PTR SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
	// TODO Phase 2: NSTimer-based implementation
	return nIDEvent;
}

BOOL KillTimer(HWND hWnd, UINT_PTR uIDEvent) { return TRUE; }

// ============================================================
// MessageBox
// ============================================================
int MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	@autoreleasepool {
		NSAlert* alert = [[NSAlert alloc] init];
		[alert setMessageText:WideToNSString(lpCaption)];
		[alert setInformativeText:WideToNSString(lpText)];

		UINT type = uType & 0x0F;
		if (type == MB_YESNO || type == MB_YESNOCANCEL)
		{
			[alert addButtonWithTitle:@"Yes"];
			[alert addButtonWithTitle:@"No"];
			if (type == MB_YESNOCANCEL)
				[alert addButtonWithTitle:@"Cancel"];
		}
		else if (type == MB_OKCANCEL)
		{
			[alert addButtonWithTitle:@"OK"];
			[alert addButtonWithTitle:@"Cancel"];
		}
		else if (type == MB_RETRYCANCEL)
		{
			[alert addButtonWithTitle:@"Retry"];
			[alert addButtonWithTitle:@"Cancel"];
		}
		else
		{
			[alert addButtonWithTitle:@"OK"];
		}

		UINT icon = uType & 0xF0;
		if (icon == MB_ICONERROR)
			[alert setAlertStyle:NSAlertStyleCritical];
		else if (icon == MB_ICONWARNING)
			[alert setAlertStyle:NSAlertStyleWarning];
		else
			[alert setAlertStyle:NSAlertStyleInformational];

		NSModalResponse response = [alert runModal];

		if (type == MB_YESNO || type == MB_YESNOCANCEL)
		{
			if (response == NSAlertFirstButtonReturn) return IDYES;
			if (response == NSAlertSecondButtonReturn) return IDNO;
			return IDCANCEL;
		}
		if (type == MB_OKCANCEL)
		{
			return (response == NSAlertFirstButtonReturn) ? IDOK : IDCANCEL;
		}
		if (type == MB_RETRYCANCEL)
		{
			return (response == NSAlertFirstButtonReturn) ? IDRETRY : IDCANCEL;
		}
		return IDOK;
	}
}

int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	NSString* text = lpText ? [NSString stringWithUTF8String:lpText] : @"";
	NSString* caption = lpCaption ? [NSString stringWithUTF8String:lpCaption] : @"";

	// Convert to wide and call MessageBoxW
	std::wstring wtext, wcaption;
	for (char c : std::string(lpText ?: "")) wtext += static_cast<wchar_t>(c);
	for (char c : std::string(lpCaption ?: "")) wcaption += static_cast<wchar_t>(c);
	return MessageBoxW(hWnd, wtext.c_str(), wcaption.c_str(), uType);
}

// ============================================================
// Keyboard
// ============================================================
SHORT GetKeyState(int nVirtKey) { return 0; }
SHORT GetAsyncKeyState(int vKey) { return 0; }
BOOL GetKeyboardState(LPBYTE lpKeyState) { if (lpKeyState) memset(lpKeyState, 0, 256); return TRUE; }
int ToUnicode(UINT wVirtKey, UINT wScanCode, const BYTE* lpKeyState, LPWSTR pwszBuff, int cchBuff, UINT wFlags) { return 0; }

// ============================================================
// Cursor
// ============================================================
BOOL GetCursorPos(LPPOINT lpPoint)
{
	if (lpPoint)
	{
		NSPoint mouseLoc = [NSEvent mouseLocation];
		NSRect screenFrame = [[NSScreen mainScreen] frame];
		lpPoint->x = static_cast<LONG>(mouseLoc.x);
		lpPoint->y = static_cast<LONG>(screenFrame.size.height - mouseLoc.y);
	}
	return TRUE;
}

BOOL SetCursorPos(int X, int Y) { return TRUE; }

// ============================================================
// Subclassing
// ============================================================
BOOL SetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) { return TRUE; }
BOOL GetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR* pdwRefData) { if (pdwRefData) *pdwRefData = 0; return FALSE; }
BOOL RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass) { return TRUE; }
LRESULT DefSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }

// ============================================================
// Misc window functions
// ============================================================
BOOL TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack) { return TRUE; }
BOOL EnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam) { return TRUE; }
HWND FindWindowExW(HWND hWndParent, HWND hWndChildAfter, LPCWSTR lpszClass, LPCWSTR lpszWindow) { return nullptr; }
HDWP BeginDeferWindowPos(int nNumWindows) { return reinterpret_cast<HDWP>(1); }
HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) { return hWinPosInfo; }
BOOL EndDeferWindowPos(HDWP hWinPosInfo) { return TRUE; }

// ============================================================
// Monitor
// ============================================================
HMONITOR MonitorFromWindow(HWND hwnd, DWORD dwFlags) { return reinterpret_cast<HMONITOR>(1); }
HMONITOR MonitorFromRect(LPCRECT lprc, DWORD dwFlags) { return reinterpret_cast<HMONITOR>(1); }
HMONITOR MonitorFromPoint(POINT pt, DWORD dwFlags) { return reinterpret_cast<HMONITOR>(1); }
BOOL GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
	if (lpmi)
	{
		NSRect screenFrame = [[NSScreen mainScreen] frame];
		NSRect visibleFrame = [[NSScreen mainScreen] visibleFrame];
		lpmi->dwFlags = MONITORINFOF_PRIMARY;
		SetRect(&lpmi->rcMonitor, 0, 0,
		        static_cast<int>(screenFrame.size.width),
		        static_cast<int>(screenFrame.size.height));
		// Visible area excludes menu bar and dock
		int menuBarHeight = static_cast<int>(screenFrame.size.height - visibleFrame.origin.y - visibleFrame.size.height);
		SetRect(&lpmi->rcWork, 0, menuBarHeight,
		        static_cast<int>(visibleFrame.size.width),
		        static_cast<int>(menuBarHeight + visibleFrame.size.height));
	}
	return TRUE;
}

// ============================================================
// Scroll
// ============================================================
int SetScrollInfo(HWND hwnd, int nBar, const SCROLLINFO* lpsi, BOOL redraw) { return 0; }
BOOL GetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO lpsi) { return FALSE; }
int GetScrollPos(HWND hwnd, int nBar) { return 0; }
int SetScrollPos(HWND hwnd, int nBar, int nPos, BOOL bRedraw) { return 0; }
BOOL SetScrollRange(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL bRedraw) { return TRUE; }
BOOL GetScrollRange(HWND hwnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos) { if (lpMinPos) *lpMinPos = 0; if (lpMaxPos) *lpMaxPos = 0; return TRUE; }
BOOL ShowScrollBar(HWND hWnd, int wBar, BOOL bShow) { return TRUE; }
BOOL EnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows) { return TRUE; }

// ============================================================
// Registered messages
// ============================================================
UINT RegisterWindowMessageW(LPCWSTR lpString)
{
	static UINT nextMsg = 0xC000;
	return nextMsg++;
}

UINT RegisterClipboardFormatW(LPCWSTR lpszFormat)
{
	static UINT nextFormat = 0xC000;
	return nextFormat++;
}

LONG_PTR SetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong) { return 0; }

// ============================================================
// Window hooks (stubs)
// ============================================================
HHOOK SetWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId)
{
	return reinterpret_cast<HHOOK>(1);
}

BOOL UnhookWindowsHookEx(HHOOK hhk) { return TRUE; }
LRESULT CallNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam) { return 0; }

// ============================================================
// Window placement
// ============================================================
BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl)
{
	if (lpwndpl)
	{
		memset(lpwndpl, 0, sizeof(WINDOWPLACEMENT));
		lpwndpl->length = sizeof(WINDOWPLACEMENT);
		lpwndpl->showCmd = SW_SHOWNORMAL;

		RECT rc;
		GetWindowRect(hWnd, &rc);
		lpwndpl->rcNormalPosition = rc;
	}
	return TRUE;
}

BOOL SetWindowPlacement(HWND hWnd, const WINDOWPLACEMENT* lpwndpl)
{
	if (lpwndpl)
	{
		const RECT& rc = lpwndpl->rcNormalPosition;
		MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		ShowWindow(hWnd, lpwndpl->showCmd);
	}
	return TRUE;
}
