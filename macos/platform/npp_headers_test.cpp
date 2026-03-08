// Phase 0: Test that key Win32 types and shim headers work in C++ context
// This verifies the shim is sufficient for basic N++ type usage.

#include "windows.h"
#include "commctrl.h"
#include "tchar.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "excpt.h"
#include "commdlg.h"
#include "uxtheme.h"
#include "richedit.h"
#include "shellapi.h"
#include "wintrust.h"

// N++ headers that should work through the shim
#include "NppConstants.h"
#include "Common.h"
#include "menuCmdID.h"
#include "NppDarkMode.h"
#include "Window.h"
#include "dpiManagerV2.h"

// Try including Parameters.h - the most important N++ header
#include "Parameters.h"

// Document management headers
#include "FileInterface.h"
#include "Utf8_16.h"
#include "Buffer.h"

// Editor view headers
#include "ScintillaEditView.h"

// Main application header - the ultimate test
#include "Notepad_plus.h"

// Test: Win32 types used throughout Notepad++ source
namespace phase0_test {

// RECT operations (used extensively in WinControls)
void testRect()
{
	RECT rc = {0, 0, 100, 200};
	POINT pt = {50, 100};
	BOOL inside = PtInRect(&rc, pt);
	InflateRect(&rc, -5, -5);
	OffsetRect(&rc, 10, 10);
	(void)inside;
}

// COLORREF operations (used in styling)
void testColor()
{
	COLORREF c = RGB(128, 64, 32);
	BYTE r = GetRValue(c);
	BYTE g = GetGValue(c);
	BYTE b = GetBValue(c);
	(void)r; (void)g; (void)b;
}

// String operations (used pervasively)
void testStrings()
{
	TCHAR buf[MAX_PATH] = _T("Hello World");
	int len = lstrlen(buf);
	TCHAR buf2[MAX_PATH];
	lstrcpy(buf2, buf);
	int cmp = lstrcmpi(buf, buf2);
	(void)len; (void)cmp;
}

// WPARAM/LPARAM macros (used in message handling)
void testMessages()
{
	WPARAM wParam = MAKEWPARAM(100, 200);
	LPARAM lParam = MAKELPARAM(300, 400);
	WORD lo = LOWORD(wParam);
	WORD hi = HIWORD(wParam);
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	(void)lo; (void)hi; (void)x; (void)y;
}

// Critical section (used in threading)
void testCriticalSection()
{
	CRITICAL_SECTION cs;
	InitializeCriticalSection(&cs);
	EnterCriticalSection(&cs);
	LeaveCriticalSection(&cs);
	DeleteCriticalSection(&cs);
}

// LARGE_INTEGER / FILETIME (used in file operations)
void testFileTypes()
{
	LARGE_INTEGER li;
	li.QuadPart = 1234567890LL;
	FILETIME ft;
	ft.dwLowDateTime = 0;
	ft.dwHighDateTime = 0;
	(void)li; (void)ft;
}

// Window styles and messages (constants should be available)
void testConstants()
{
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	UINT msg = WM_COMMAND;
	int vk = VK_RETURN;
	int sw = SW_SHOW;
	int mb = MB_OKCANCEL | MB_ICONQUESTION;
	(void)style; (void)msg; (void)vk; (void)sw; (void)mb;
}

// Tab control types (used by DocTabView)
void testTabControl()
{
	TCITEM ti;
	ti.mask = TCIF_TEXT | TCIF_PARAM;
	ti.pszText = nullptr;
	ti.lParam = 0;
	(void)ti;
}

// ListView types (used by various panels)
void testListView()
{
	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = nullptr;
	lvi.lParam = 0;
	(void)lvi;
}

// TreeView types (used by FunctionList, FileBrowser)
void testTreeView()
{
	TVITEM tvi;
	tvi.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.hItem = nullptr;
	tvi.pszText = nullptr;
	tvi.lParam = 0;
	(void)tvi;
}

// SEH types (used by Common.cpp)
void testExceptionTypes()
{
	EXCEPTION_POINTERS* ep = nullptr;
	(void)ep;
	int code = EXCEPTION_ACCESS_VIOLATION;
	int action = EXCEPTION_EXECUTE_HANDLER;
	(void)code; (void)action;
}

// Shell/COM types (used by Parameters.cpp, NppIO.cpp)
void testShellTypes()
{
	HRESULT hr = CoInitialize(nullptr);
	(void)hr;
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	CoUninitialize();

	BROWSEINFOW bi = {};
	bi.hwndOwner = nullptr;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	(void)bi;
}

// Common dialog types (used by file open/save)
void testCommonDialogs()
{
	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
	(void)ofn;

	CHOOSEFONTW cf = {};
	cf.lStructSize = sizeof(CHOOSEFONTW);
	(void)cf;
}

// NppConstants (real N++ header through the shim)
void testNppConstants()
{
	COLORREF bg = BCKGRD_COLOR;
	COLORREF fg = TXT_COLOR;
	DWORD sz = REBARBAND_SIZE;
	(void)bg; (void)fg; (void)sz;
}

} // namespace phase0_test
