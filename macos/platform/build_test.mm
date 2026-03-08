// Phase 0 Build Test
// Verifies that the Win32 shim compiles and links correctly on macOS,
// and that Scintilla/Lexilla build successfully.

#import <Foundation/Foundation.h>

// Test: Win32 shim headers compile
#include "windows.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "richedit.h"
#include "uxtheme.h"
#include "tchar.h"

// Test: Scintilla headers compile
#include "Scintilla.h"
#include "ILexer.h"
#include "SciLexer.h"

// Test: Lexilla headers compile
#include "Lexilla.h"

// Test: basic Win32 type usage
static void testTypes()
{
	HWND hwnd = nullptr;
	HINSTANCE hInst = nullptr;
	RECT rc = {0, 0, 100, 100};
	POINT pt = {50, 50};
	COLORREF color = RGB(255, 128, 0);
	TCHAR buf[MAX_PATH] = _T("Hello");

	BOOL result = PtInRect(&rc, pt);
	int r = GetRValue(color);
	int g = GetGValue(color);
	int b = GetBValue(color);
	int len = lstrlen(buf);

	WPARAM wParam = MAKEWPARAM(1, 2);
	LPARAM lParam = MAKELPARAM(3, 4);
	WORD lo = LOWORD(wParam);
	WORD hi = HIWORD(wParam);

	// Suppress unused warnings
	(void)hwnd; (void)hInst; (void)result;
	(void)r; (void)g; (void)b; (void)len;
	(void)wParam; (void)lParam; (void)lo; (void)hi;
}

// Test: Win32 API calls compile
static void testAPIs()
{
	// String functions
	WCHAR buf[256];
	lstrcpy(buf, L"Test");
	int len = lstrlen(buf);
	(void)len;

	// MulDiv
	int result = MulDiv(100, 96, 72);
	(void)result;

	// Critical section
	CRITICAL_SECTION cs;
	InitializeCriticalSection(&cs);
	EnterCriticalSection(&cs);
	LeaveCriticalSection(&cs);
	DeleteCriticalSection(&cs);

	// GetTickCount
	DWORD tick = GetTickCount();
	(void)tick;

	// File attributes
	DWORD attrs = GetFileAttributes(L"/tmp");
	(void)attrs;

	// Error handling
	SetLastError(0);
	DWORD err = GetLastError();
	(void)err;
}

int main(int argc, const char* argv[])
{
	@autoreleasepool {
		NSLog(@"=== Notepad++ macOS Port - Phase 0 Build Test ===");

		testTypes();
		NSLog(@"  Win32 types: OK");

		testAPIs();
		NSLog(@"  Win32 APIs: OK");

		// Test Scintilla constant availability
		int sciMsg = SCI_SETTEXT;
		(void)sciMsg;
		NSLog(@"  Scintilla constants: OK");

		NSLog(@"=== Phase 0 Build Test PASSED ===");
	}
	return 0;
}
