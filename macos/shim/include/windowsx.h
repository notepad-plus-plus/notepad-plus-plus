#pragma once
// Win32 Shim: Windows message helper macros for macOS

#include "windef.h"

// Message parameter extraction macros
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define GET_WM_COMMAND_ID(wp, lp)   LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp) ((HWND)(lp))
#define GET_WM_COMMAND_CMD(wp, lp)  HIWORD(wp)

// Button macros
#define Button_GetCheck(hwnd) ((int)(DWORD)SendMessage((hwnd), BM_GETCHECK, 0, 0))
#define Button_SetCheck(hwnd, check) ((void)SendMessage((hwnd), BM_SETCHECK, (WPARAM)(int)(check), 0))

// Edit macros
#define Edit_GetText(hwnd, lpch, cchMax) GetWindowText((hwnd), (lpch), (cchMax))
#define Edit_SetText(hwnd, lpsz) SetWindowText((hwnd), (lpsz))
#define Edit_GetTextLength(hwnd) GetWindowTextLength((hwnd))

// ComboBox macros
#define ComboBox_AddString(hwnd, lpsz) ((int)(DWORD)SendMessage((hwnd), CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)(lpsz)))
#define ComboBox_GetCurSel(hwnd) ((int)(DWORD)SendMessage((hwnd), CB_GETCURSEL, 0, 0))
#define ComboBox_SetCurSel(hwnd, index) ((int)(DWORD)SendMessage((hwnd), CB_SETCURSEL, (WPARAM)(int)(index), 0))
#define ComboBox_ResetContent(hwnd) ((int)(DWORD)SendMessage((hwnd), CB_RESETCONTENT, 0, 0))

// ListBox macros
#define ListBox_AddString(hwnd, lpsz) ((int)(DWORD)SendMessage((hwnd), LB_ADDSTRING, 0, (LPARAM)(LPCWSTR)(lpsz)))
#define ListBox_GetCurSel(hwnd) ((int)(DWORD)SendMessage((hwnd), LB_GETCURSEL, 0, 0))
#define ListBox_SetCurSel(hwnd, index) ((int)(DWORD)SendMessage((hwnd), LB_SETCURSEL, (WPARAM)(int)(index), 0))

// Static macros
#define Static_SetText(hwnd, lpsz) SetWindowText((hwnd), (lpsz))

// Forwarding macros for void messages
#define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
	(void)(fn)((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))

// SubclassWindow
#define SubclassWindow(hwnd, lpfn) \
	((WNDPROC)SetWindowLongPtr((hwnd), GWLP_WNDPROC, (LONG_PTR)(WNDPROC)(lpfn)))
