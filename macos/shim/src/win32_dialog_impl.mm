// Win32 Clipboard, Shell API, Theme/DWM stubs for macOS
// Extracted from win32_menu.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "shellapi.h"
#include "uxtheme.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"

// ============================================================
// Clipboard
// ============================================================

BOOL OpenClipboard(HWND hWndNewOwner) { return TRUE; }
BOOL CloseClipboard() { return TRUE; }

BOOL EmptyClipboard()
{
	@autoreleasepool {
		[[NSPasteboard generalPasteboard] clearContents];
	}
	return TRUE;
}

HANDLE SetClipboardData(UINT uFormat, HANDLE hMem)
{
	if (!hMem) return nullptr;
	@autoreleasepool {
		if (uFormat == CF_UNICODETEXT)
		{
			const wchar_t* wstr = static_cast<const wchar_t*>(hMem);
			NSString* str = WideToNS(wstr);
			[[NSPasteboard generalPasteboard] clearContents];
			[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
		}
		else if (uFormat == CF_TEXT)
		{
			const char* cstr = static_cast<const char*>(hMem);
			NSString* str = [NSString stringWithUTF8String:cstr];
			[[NSPasteboard generalPasteboard] clearContents];
			[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
		}
	}
	return hMem;
}

HANDLE GetClipboardData(UINT uFormat)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSString* str = [pb stringForType:NSPasteboardTypeString];
		if (!str) return nullptr;

		if (uFormat == CF_UNICODETEXT)
		{
			// Return a wide string buffer (caller is responsible for GlobalFree)
			NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
			size_t bufSize = data.length + sizeof(wchar_t); // +null terminator
			void* buf = GlobalAlloc(GMEM_FIXED, bufSize);
			memcpy(buf, data.bytes, data.length);
			memset(static_cast<char*>(buf) + data.length, 0, sizeof(wchar_t));
			return buf;
		}
		else if (uFormat == CF_TEXT)
		{
			const char* utf8 = [str UTF8String];
			size_t len = strlen(utf8) + 1;
			void* buf = GlobalAlloc(GMEM_FIXED, len);
			memcpy(buf, utf8, len);
			return buf;
		}
	}
	return nullptr;
}

BOOL IsClipboardFormatAvailable(UINT format)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		if (format == CF_UNICODETEXT || format == CF_TEXT)
			return [pb stringForType:NSPasteboardTypeString] != nil;
	}
	return FALSE;
}

int CountClipboardFormats()
{
	@autoreleasepool {
		return static_cast<int>([[[NSPasteboard generalPasteboard] types] count]);
	}
}

UINT EnumClipboardFormats(UINT format) { return 0; }
HWND GetClipboardOwner() { return nullptr; }

// ============================================================
// Shell API
// ============================================================

HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	@autoreleasepool {
		NSString* filePath = WideToNS(lpFile);
		if (filePath.length > 0)
		{
			NSURL* url = nil;
			if ([filePath hasPrefix:@"http://"] || [filePath hasPrefix:@"https://"] ||
			    [filePath hasPrefix:@"ftp://"] || [filePath hasPrefix:@"mailto:"])
			{
				url = [NSURL URLWithString:filePath];
			}
			else
			{
				url = [NSURL fileURLWithPath:filePath];
			}
			if (url)
				[[NSWorkspace sharedWorkspace] openURL:url];
		}
	}
	return reinterpret_cast<HINSTANCE>(33); // > 32 means success
}

// Overload for filesystem::path::c_str() which returns const char* on macOS
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, const char* lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	@autoreleasepool {
		if (lpFile)
		{
			NSString* filePath = [NSString stringWithUTF8String:lpFile];
			NSURL* url = nil;
			if ([filePath hasPrefix:@"http://"] || [filePath hasPrefix:@"https://"])
				url = [NSURL URLWithString:filePath];
			else
				url = [NSURL fileURLWithPath:filePath];
			if (url)
				[[NSWorkspace sharedWorkspace] openURL:url];
		}
	}
	return reinterpret_cast<HINSTANCE>(33);
}

BOOL ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo) { return TRUE; }

UINT DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch) { return 0; }
BOOL DragQueryPoint(HDROP hDrop, LPPOINT lppt) { return FALSE; }
void DragAcceptFiles(HWND hWnd, BOOL fAccept) {}
void DragFinish(HDROP hDrop) {}

BOOL Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData) { return TRUE; }

HICON ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex) { return nullptr; }

// ============================================================
// Theme stubs
// ============================================================
HTHEME OpenThemeData(HWND hwnd, LPCWSTR pszClassList) { return nullptr; }
HRESULT CloseThemeData(HTHEME hTheme) { return S_OK; }
HRESULT DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect) { return E_NOTIMPL; }
HRESULT DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT* pRect) { return E_NOTIMPL; }
HRESULT GetThemeColor(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF* pColor) { return E_NOTIMPL; }
HRESULT GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* prc, int eSize, SIZE* psz) { return E_NOTIMPL; }
HRESULT GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pBoundingRect, RECT* pContentRect) { return E_NOTIMPL; }
BOOL IsThemeActive() { return FALSE; }
BOOL IsAppThemed() { return FALSE; }
HRESULT SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) { return S_OK; }
HRESULT EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags) { return S_OK; }
HRESULT GetThemeMetric(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, int* piVal) { return E_NOTIMPL; }
HRESULT GetThemeFont(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LOGFONTW* pFont) { return E_NOTIMPL; }

HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) { return S_OK; }
HRESULT DwmExtendFrameIntoClientArea(HWND hWnd, const void* pMarInset) { return S_OK; }
BOOL DwmDefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { return FALSE; }
