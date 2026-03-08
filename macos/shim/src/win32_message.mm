// Win32 Shim: Message dispatch for macOS
// Phase 1: Real WndProc dispatch via HandleRegistry.
// Scintilla messages are forwarded to ScintillaView via the bridge.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "handle_registry.h"
#include "scintilla_bridge.h"

// Scintilla messages start at SCI_START (2000)
#define SCI_START 2000

// ============================================================
// SendMessage / PostMessage
// ============================================================

LRESULT SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!hWnd)
		return 0;

	auto* info = HandleRegistry::getWindowInfo(hWnd);
	if (!info)
		return 0;

	// Scintilla messages: forward to ScintillaView via bridge
	if (info->isScintilla && info->nativeView && Msg >= SCI_START)
	{
		return static_cast<LRESULT>(
			ScintillaBridge_sendMessage(info->nativeView, Msg,
			                           static_cast<uintptr_t>(wParam),
			                           static_cast<intptr_t>(lParam)));
	}

	// Regular Win32 messages: dispatch to WndProc
	if (info->wndProc)
		return info->wndProc(hWnd, Msg, wParam, lParam);

	return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

BOOL PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Post asynchronously on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		SendMessageW(hWnd, Msg, wParam, lParam);
	});
	return TRUE;
}

LRESULT DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LRESULT CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (lpPrevWndFunc)
		return lpPrevWndFunc(hWnd, Msg, wParam, lParam);
	return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

// ============================================================
// Message loop
// ============================================================

BOOL PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	return FALSE;
}

BOOL GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	// On macOS, the message loop is run by [NSApp run].
	// This function should not be called directly.
	return FALSE;
}

BOOL TranslateMessage(const MSG* lpMsg)
{
	return FALSE;
}

LRESULT DispatchMessageW(const MSG* lpMsg)
{
	return 0;
}

void PostQuitMessage(int nExitCode)
{
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSApp terminate:nil];
	});
}

BOOL IsDialogMessageW(HWND hDlg, LPMSG lpMsg)
{
	return FALSE;
}

// ============================================================
// Dialog functions (stubs)
// ============================================================
HWND CreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	return nullptr;
}

HWND CreateDialogIndirectParamW(HINSTANCE hInstance, const void* lpTemplate,
                                HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	return nullptr;
}

INT_PTR DialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	return -1;
}

INT_PTR DialogBoxIndirectParamW(HINSTANCE hInstance, const void* hDialogTemplate,
                                HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	return -1;
}

BOOL EndDialog(HWND hDlg, INT_PTR nResult)
{
	return TRUE;
}

BOOL SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString) { return TRUE; }
BOOL SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString) { return TRUE; }
UINT GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax)
{
	if (lpString && cchMax > 0) lpString[0] = L'\0';
	return 0;
}
UINT GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax)
{
	if (lpString && cchMax > 0) lpString[0] = '\0';
	return 0;
}
BOOL SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned) { return TRUE; }
UINT GetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned)
{
	if (lpTranslated) *lpTranslated = FALSE;
	return 0;
}
LRESULT SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) { return 0; }
BOOL CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck) { return TRUE; }
UINT IsDlgButtonChecked(HWND hDlg, int nIDButton) { return BST_UNCHECKED; }
BOOL CheckRadioButton(HWND hDlg, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) { return TRUE; }
HWND GetNextDlgTabItem(HWND hDlg, HWND hCtl, BOOL bPrevious) { return nullptr; }
BOOL MapDialogRect(HWND hDlg, LPRECT lpRect) { return TRUE; }
