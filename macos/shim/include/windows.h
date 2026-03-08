#pragma once
// Win32 Shim: Master header for macOS
// This file replaces <windows.h> when building on macOS.
// It includes all Win32 type definitions and API declarations
// backed by Cocoa/AppKit/CoreGraphics implementations.

#ifdef __APPLE__

// Prevent re-inclusion via other paths
#ifndef _WINDOWS_H_SHIM_
#define _WINDOWS_H_SHIM_

// Indicate we're using the shim
#define WIN32_SHIM_MACOS 1

// These are normally set by the build system on Windows
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// Note: Do NOT define WIN32, _WIN32, or _WIN64 here.
// These macros cause the C++ standard library (<locale>, <ctype.h>)
// to use Windows-specific code paths that don't work on macOS.
// N++ code rarely checks these directly (only json.hpp and one RC header).
// The __APPLE__ check + WIN32_SHIM_MACOS is sufficient for our purposes.

// Core type definitions
#include "windef.h"
#include "winnt.h"

// GDI types and constants
#include "wingdi.h"

// Window management, messages, controls
#include "winuser.h"

// Base API: file I/O, strings, DLL loading, threading
#include "winbase.h"

// Shell API (on Windows, apps typically include shellapi.h separately,
// but many N++ headers expect these types to be available)
#include "shellapi.h"

// Common controls (ListView, TreeView, Toolbar, StatusBar, etc.)
// N++ code expects these from transitive includes
#include "commctrl.h"

// Common dialogs (PRINTDLG, OPENFILENAME, etc.)
#include "commdlg.h"

// Crypto stubs (WinCrypt + WinTrust)
#include "wincrypt.h"
#include "wintrust.h"

// SEH macros (on MSVC these are compiler builtins, no header needed)
#ifndef __try
#define __try try
#endif
#ifndef __except
#define __except(filter) catch(...)
#endif
#ifndef __finally
#define __finally catch(...) {} /* finally */
#endif
#ifndef GetExceptionCode
#define GetExceptionCode() 0
#endif
#ifndef GetExceptionInformation
#define GetExceptionInformation() nullptr
#endif

// ============================================================
// COM basics (from objbase.h on Windows, included via windows.h)
// ============================================================
#ifndef COINIT_DEFINED
#define COINIT_DEFINED
typedef enum {
	COINIT_APARTMENTTHREADED = 0x2,
	COINIT_MULTITHREADED     = 0x0,
	COINIT_DISABLE_OLE1DDE   = 0x4,
	COINIT_SPEED_OVER_MEMORY = 0x8
} COINIT;
#endif

#include <cstdlib>
inline HRESULT CoInitialize(LPVOID pvReserved) { (void)pvReserved; return S_OK; }
inline HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit) { (void)pvReserved; (void)dwCoInit; return S_OK; }
inline void CoUninitialize() {}
inline void CoTaskMemFree(LPVOID pv) { free(pv); }
inline HRESULT CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
	(void)rclsid; (void)pUnkOuter; (void)dwClsContext; (void)riid;
	if (ppv) *ppv = nullptr;
	return E_NOTIMPL;
}

// ============================================================
// Win32 API function declarations
// (Implementations in macos/shim/src/*.mm)
// ============================================================

// Window management
ATOM RegisterClassExW(const WNDCLASSEXW* lpWndClass);
#define RegisterClassEx RegisterClassExW

ATOM RegisterClassW(const WNDCLASSW* lpWndClass);
#define RegisterClass RegisterClassW

BOOL UnregisterClassW(LPCWSTR lpClassName, HINSTANCE hInstance);
#define UnregisterClass UnregisterClassW

HWND CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
                     DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                     HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#define CreateWindowEx CreateWindowExW
#define CreateWindowW(cls, name, style, x, y, w, h, parent, menu, inst, param) \
	CreateWindowExW(0, cls, name, style, x, y, w, h, parent, menu, inst, param)
#define CreateWindow CreateWindowW

BOOL DestroyWindow(HWND hWnd);
BOOL ShowWindow(HWND hWnd, int nCmdShow);
BOOL UpdateWindow(HWND hWnd);
BOOL EnableWindow(HWND hWnd, BOOL bEnable);
BOOL IsWindowEnabled(HWND hWnd);
BOOL IsWindowVisible(HWND hWnd);
BOOL IsWindow(HWND hWnd);
BOOL IsChild(HWND hWndParent, HWND hWnd);

HWND SetFocus(HWND hWnd);
HWND GetFocus();
HWND SetActiveWindow(HWND hWnd);
HWND GetActiveWindow();
HWND GetForegroundWindow();
BOOL SetForegroundWindow(HWND hWnd);
BOOL BringWindowToTop(HWND hWnd);

HWND GetParent(HWND hWnd);
HWND SetParent(HWND hWndChild, HWND hWndNewParent);
HWND GetWindow(HWND hWnd, UINT uCmd);
HWND GetAncestor(HWND hWnd, UINT gaFlags);
HWND GetDlgItem(HWND hDlg, int nIDDlgItem);
int GetDlgCtrlID(HWND hWnd);

LONG_PTR GetWindowLongPtrW(HWND hWnd, int nIndex);
LONG_PTR SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#define GetWindowLongPtr GetWindowLongPtrW
#define SetWindowLongPtr SetWindowLongPtrW
inline LONG GetWindowLongW(HWND h, int i) { return static_cast<LONG>(GetWindowLongPtrW(h, i)); }
inline LONG SetWindowLongW(HWND h, int i, LONG v) { return static_cast<LONG>(SetWindowLongPtrW(h, i, static_cast<LONG_PTR>(v))); }
#define GetWindowLong GetWindowLongW
#define SetWindowLong SetWindowLongW

LONG_PTR GetClassLongPtrW(HWND hWnd, int nIndex);
LONG_PTR SetClassLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#define GetClassLongPtr GetClassLongPtrW
#define SetClassLongPtr SetClassLongPtrW

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL GetClientRect(HWND hWnd, LPRECT lpRect);
BOOL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

int GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
BOOL SetWindowTextW(HWND hWnd, LPCWSTR lpString);
int GetWindowTextLengthW(HWND hWnd);
#define GetWindowText GetWindowTextW
#define SetWindowText SetWindowTextW
#define GetWindowTextLength GetWindowTextLengthW

int GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount);
HWND GetLastActivePopup(HWND hWnd);
BOOL LockWindowUpdate(HWND hWndLock);

LONG GetWindowLongW_impl(HWND hWnd, int nIndex);
LONG SetWindowLongW_impl(HWND hWnd, int nIndex, LONG dwNewLong);

BOOL InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
BOOL RedrawWindow(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags);

HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
BOOL EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
HDC GetDC(HWND hWnd);
HDC GetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags);
HDC GetWindowDC(HWND hWnd);

#define DCX_WINDOW            0x00000001L
#define DCX_CACHE             0x00000002L
#define DCX_NORESETATTRS      0x00000004L
#define DCX_CLIPCHILDREN      0x00000008L
#define DCX_CLIPSIBLINGS      0x00000010L
#define DCX_PARENTCLIP        0x00000020L
#define DCX_EXCLUDERGN        0x00000040L
#define DCX_INTERSECTRGN      0x00000080L
#define DCX_EXCLUDEUPDATE     0x00000100L
#define DCX_INTERSECTUPDATE   0x00000200L
#define DCX_LOCKWINDOWUPDATE  0x00000400L
#define DCX_VALIDATE          0x00200000L
int ReleaseDC(HWND hWnd, HDC hDC);

BOOL ScreenToClient(HWND hWnd, LPPOINT lpPoint);
BOOL ClientToScreen(HWND hWnd, LPPOINT lpPoint);
HWND WindowFromPoint(POINT Point);
HWND ChildWindowFromPoint(HWND hWndParent, POINT Point);

BOOL SetCapture_shim(HWND hWnd);
BOOL ReleaseCapture();
HWND GetCapture();
#define SetCapture SetCapture_shim

HCURSOR SetCursor(HCURSOR hCursor);
inline int ShowCursor(BOOL bShow) { return bShow ? 1 : 0; }
HCURSOR LoadCursorW(HINSTANCE hInstance, LPCWSTR lpCursorName);
#define LoadCursor LoadCursorW

HICON LoadIconW(HINSTANCE hInstance, LPCWSTR lpIconName);
#define LoadIcon LoadIconW
BOOL DestroyIcon(HICON hIcon);
HICON CopyIcon(HICON hIcon);

HANDLE LoadImageW(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad);
#define LoadImage LoadImageW
inline HBITMAP LoadBitmapW(HINSTANCE hInst, LPCWSTR lpBitmapName) { return static_cast<HBITMAP>(LoadImageW(hInst, lpBitmapName, 0/*IMAGE_BITMAP*/, 0, 0, 0)); }
#define LoadBitmap LoadBitmapW

UINT_PTR SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
BOOL KillTimer(HWND hWnd, UINT_PTR uIDEvent);

int MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
#define MessageBox MessageBoxW

// Message functions
LRESULT SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define SendMessage SendMessageW

BOOL PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define PostMessage PostMessageW

LRESULT DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define DefWindowProc DefWindowProcW

LRESULT CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define CallWindowProc CallWindowProcW

BOOL PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
#define PeekMessage PeekMessageW

BOOL GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
#define GetMessage GetMessageW

BOOL TranslateMessage(const MSG* lpMsg);
LRESULT DispatchMessageW(const MSG* lpMsg);
#define DispatchMessage DispatchMessageW

void PostQuitMessage(int nExitCode);

UINT RegisterWindowMessageW(LPCWSTR lpString);
#define RegisterWindowMessage RegisterWindowMessageW

BOOL IsDialogMessageW(HWND hDlg, LPMSG lpMsg);
#define IsDialogMessage IsDialogMessageW

// Dialog functions
HWND CreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND CreateDialogIndirectParamW(HINSTANCE hInstance, const void* lpTemplate,
                                HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
INT_PTR DialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
INT_PTR DialogBoxIndirectParamW(HINSTANCE hInstance, const void* hDialogTemplate,
                                HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
BOOL EndDialog(HWND hDlg, INT_PTR nResult);

#define CreateDialogParam CreateDialogParamW
#define CreateDialogIndirectParam CreateDialogIndirectParamW
#define DialogBoxParam DialogBoxParamW
#define DialogBoxIndirectParam DialogBoxIndirectParamW

BOOL SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
BOOL SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
UINT GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax);
UINT GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax);
BOOL SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned);
UINT GetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned);
LRESULT SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck);
UINT IsDlgButtonChecked(HWND hDlg, int nIDButton);
BOOL CheckRadioButton(HWND hDlg, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
HWND GetNextDlgTabItem(HWND hDlg, HWND hCtl, BOOL bPrevious);
BOOL MapDialogRect(HWND hDlg, LPRECT lpRect);
LONG_PTR SetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong);

#define SetDlgItemText SetDlgItemTextW
#define GetDlgItemText GetDlgItemTextW
#define SendDlgItemMessage SendDlgItemMessageW

// Menu functions
HMENU CreateMenu();
HMENU CreatePopupMenu();
HMENU LoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName);
#define LoadMenu LoadMenuW
BOOL DestroyMenu(HMENU hMenu);
HMENU GetMenu(HWND hWnd);
BOOL SetMenu(HWND hWnd, HMENU hMenu);
HMENU GetSubMenu(HMENU hMenu, int nPos);
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert);
BOOL AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL InsertMenuItemW(HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmi);
BOOL SetMenuItemInfoW(HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmii);
BOOL GetMenuItemInfoW(HMENU hMenu, UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii);
BOOL RemoveMenu(HMENU hMenu, UINT uPosition, UINT uFlags);
BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags);
BOOL EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable);
BOOL CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck);
BOOL CheckMenuRadioItem(HMENU hMenu, UINT first, UINT last, UINT check, UINT flags);
int GetMenuItemCount(HMENU hMenu);
UINT GetMenuItemID(HMENU hMenu, int nPos);
BOOL TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, const RECT* prcRect);
BOOL TrackPopupMenuEx(HMENU hMenu, UINT fuFlags, int x, int y, HWND hwnd, void* lptpm);
BOOL DrawMenuBar(HWND hWnd);
int GetMenuStringW(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags);
UINT GetMenuState(HMENU hMenu, UINT uId, UINT uFlags);
BOOL ModifyMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);

#define AppendMenu AppendMenuW
#define InsertMenu InsertMenuW
#define InsertMenuItem InsertMenuItemW
#define SetMenuItemInfo SetMenuItemInfoW
#define GetMenuItemInfo GetMenuItemInfoW
#define GetMenuString GetMenuStringW
#define ModifyMenu ModifyMenuW

// Accelerator functions
HACCEL LoadAcceleratorsW(HINSTANCE hInstance, LPCWSTR lpTableName);
int TranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
HACCEL CreateAcceleratorTableW(LPACCEL paccel, int cAccel);
BOOL DestroyAcceleratorTable(HACCEL hAccel);
#define LoadAccelerators LoadAcceleratorsW
#define TranslateAccelerator TranslateAcceleratorW
#define CreateAcceleratorTable CreateAcceleratorTableW

// Clipboard functions
BOOL OpenClipboard(HWND hWndNewOwner);
BOOL CloseClipboard();
BOOL EmptyClipboard();
HANDLE SetClipboardData(UINT uFormat, HANDLE hMem);
HANDLE GetClipboardData(UINT uFormat);
BOOL IsClipboardFormatAvailable(UINT format);
UINT RegisterClipboardFormatW(LPCWSTR lpszFormat);
#define RegisterClipboardFormat RegisterClipboardFormatW
int CountClipboardFormats();
UINT EnumClipboardFormats(UINT format);
HWND GetClipboardOwner();

// GDI functions
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h);
BOOL DeleteObject(HGDIOBJ ho);
HGDIOBJ GetStockObject(int i);
int GetObjectW(HANDLE h, int c, LPVOID pv);
#define GetObject GetObjectW
DWORD GetObjectType(HGDIOBJ h);

HDC CreateCompatibleDC(HDC hdc);
BOOL DeleteDC(HDC hdc);
HBITMAP CreateCompatibleBitmap(HDC hdc, int cx, int cy);
HBITMAP CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitCount, const void* lpBits);
HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* pbmi, UINT usage, void** ppvBits, HANDLE hSection, DWORD offset);
int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage);
int SetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, const void* lpBits, const BITMAPINFO* lpbmi, UINT ColorUse);
int SetDIBitsToDevice(HDC hdc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT StartScan, UINT cLines, const void* lpvBits, const BITMAPINFO* lpbmi, UINT ColorUse);

HPEN CreatePen(int iStyle, int cWidth, COLORREF color);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreateHatchBrush(int iHatch, COLORREF color);
HBRUSH CreatePatternBrush(HBITMAP hbm);
HFONT CreateFontIndirectW(const LOGFONTW* lplf);
#define CreateFontIndirect CreateFontIndirectW
HFONT CreateFontW(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight,
                  DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet,
                  DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality,
                  DWORD iPitchAndFamily, LPCWSTR pszFaceName);
#define CreateFont CreateFontW
HRGN CreateRectRgn(int x1, int y1, int x2, int y2);
HRGN CreateRectRgnIndirect(const RECT* lprect);
HRGN CreateRoundRectRgn(int x1, int y1, int x2, int y2, int w, int h);
int CombineRgn(HRGN hrgnDst, HRGN hrgnSrc1, HRGN hrgnSrc2, int iMode);
int SelectClipRgn(HDC hdc, HRGN hrgn);
int GetClipRgn(HDC hdc, HRGN hrgn);
int ExcludeClipRect(HDC hdc, int left, int top, int right, int bottom);
int IntersectClipRect(HDC hdc, int left, int top, int right, int bottom);
BOOL RectVisible(HDC hdc, const RECT* lprect);
BOOL DeleteRgn(HRGN hrgn);

COLORREF SetTextColor(HDC hdc, COLORREF color);
COLORREF SetBkColor(HDC hdc, COLORREF color);
int SetBkMode(HDC hdc, int mode);
COLORREF GetTextColor(HDC hdc);
COLORREF GetBkColor(HDC hdc);
BOOL SetBrushOrgEx(HDC hdc, int x, int y, LPPOINT lppt);

BOOL TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c);
#define TextOut TextOutW
int DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format);
#define DrawText DrawTextW
typedef struct tagDRAWTEXTPARAMS {
	UINT cbSize;
	int  iTabLength;
	int  iLeftMargin;
	int  iRightMargin;
	UINT uiLengthDrawn;
} DRAWTEXTPARAMS, *LPDRAWTEXTPARAMS;
int DrawTextExW(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp);
#define DrawTextEx DrawTextExW
BOOL GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int c, LPSIZE psizl);
#define GetTextExtentPoint32 GetTextExtentPoint32W
#define GetTextExtentPoint GetTextExtentPoint32W
#define GetTextExtentPointW GetTextExtentPoint32W
BOOL GetTextMetricsW(HDC hdc, LPTEXTMETRICW lptm);
#define GetTextMetrics GetTextMetricsW
int GetTextFaceW(HDC hdc, int c, LPWSTR lpName);
#define GetTextFace GetTextFaceW

BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt);
BOOL LineTo(HDC hdc, int x, int y);
BOOL Rectangle(HDC hdc, int left, int top, int right, int bottom);
BOOL Ellipse(HDC hdc, int left, int top, int right, int bottom);
BOOL Win32Polygon(HDC hdc, const POINT* apt, int cpt);
#define Polygon Win32Polygon
BOOL Polyline(HDC hdc, const POINT* apt, int cpt);
BOOL RoundRect(HDC hdc, int left, int top, int right, int bottom, int width, int height);

COLORREF GetPixel(HDC hdc, int x, int y);
COLORREF SetPixel(HDC hdc, int x, int y, COLORREF color);

BOOL FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
int FrameRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
BOOL DrawEdge(HDC hdc, LPRECT qrc, UINT edge, UINT grfFlags);
BOOL DrawFrameControl(HDC hdc, LPRECT lprc, UINT uType, UINT uState);
BOOL DrawFocusRect(HDC hDC, const RECT* lprc);

BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);
BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
                HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop);
BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop);
int SetStretchBltMode(HDC hdc, int mode);
BOOL AlphaBlend(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
                HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, BLENDFUNCTION ftn);

int SaveDC(HDC hdc);
BOOL RestoreDC(HDC hdc, int nSavedDC);
int GetDeviceCaps(HDC hdc, int index);

COLORREF GetSysColor(int nIndex);
HBRUSH GetSysColorBrush(int nIndex);
int GetSystemMetrics(int nIndex);
BOOL SystemParametersInfoW(UINT uiAction, UINT uiParam, LPVOID pvParam, UINT fWinIni);
#define SystemParametersInfo SystemParametersInfoW

#define SPI_GETNONCLIENTMETRICS 0x0029
#define SPI_GETWHEELSCROLLLINES 0x0068
#define SPI_GETWORKAREA         0x0030

// Locale functions
int GetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData);
int GetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData);
#define GetLocaleInfo GetLocaleInfoW
LCID GetSystemDefaultLCID();
LCID GetUserDefaultLCID();

// Monitor
HMONITOR MonitorFromWindow(HWND hwnd, DWORD dwFlags);
HMONITOR MonitorFromRect(LPCRECT lprc, DWORD dwFlags);
HMONITOR MonitorFromPoint(POINT pt, DWORD dwFlags);
BOOL GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi);
#define GetMonitorInfo GetMonitorInfoW

// Scroll functions
int SetScrollInfo(HWND hwnd, int nBar, const SCROLLINFO* lpsi, BOOL redraw);
BOOL GetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
int GetScrollPos(HWND hwnd, int nBar);
int SetScrollPos(HWND hwnd, int nBar, int nPos, BOOL bRedraw);
BOOL SetScrollRange(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL bRedraw);
BOOL GetScrollRange(HWND hwnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos);
BOOL ShowScrollBar(HWND hWnd, int wBar, BOOL bShow);
BOOL EnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows);

// Coordinate / rectangle helpers
BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
BOOL SetRectEmpty(LPRECT lprc);
BOOL CopyRect(LPRECT lprcDst, const RECT* lprcSrc);
BOOL InflateRect(LPRECT lprc, int dx, int dy);
BOOL IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL UnionRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL SubtractRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL OffsetRect(LPRECT lprc, int dx, int dy);
BOOL IsRectEmpty(const RECT* lprc);
BOOL PtInRect(const RECT* lprc, POINT pt);
BOOL EqualRect(const RECT* lprc1, const RECT* lprc2);

// MapWindowPoints
int MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints);

// Misc window functions
BOOL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
HDWP BeginDeferWindowPos(int nNumWindows);
HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags);
BOOL EndDeferWindowPos(HDWP hWinPosInfo);

BOOL EnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam);
HWND FindWindowExW(HWND hWndParent, HWND hWndChildAfter, LPCWSTR lpszClass, LPCWSTR lpszWindow);
#define FindWindowEx FindWindowExW

inline HWND FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) { return nullptr; }
#define FindWindow FindWindowW

// Layered window
#define LWA_COLORKEY 0x00000001
#define LWA_ALPHA    0x00000002
inline BOOL SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags) { return TRUE; }

BOOL TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);
#define _TrackMouseEvent TrackMouseEvent

BOOL GetCursorPos(LPPOINT lpPoint);
BOOL SetCursorPos(int X, int Y);
SHORT GetKeyState(int nVirtKey);
SHORT GetAsyncKeyState(int vKey);
BOOL GetKeyboardState(LPBYTE lpKeyState);
int ToUnicode(UINT wVirtKey, UINT wScanCode, const BYTE* lpKeyState, LPWSTR pwszBuff, int cchBuff, UINT wFlags);

typedef void* HKL;
inline HKL GetKeyboardLayout(DWORD idThread) { return nullptr; }
inline int GetKeyboardLayoutList(int nBuff, HKL* lpList) { return 0; }
inline HKL ActivateKeyboardLayout(HKL hkl, UINT Flags) { return nullptr; }
inline LANGID LOWORD_LANG(HKL hkl) { return static_cast<LANGID>(reinterpret_cast<uintptr_t>(hkl) & 0xFFFF); }

#define MAPVK_VK_TO_VSC   0
#define MAPVK_VSC_TO_VK   1
#define MAPVK_VK_TO_CHAR  2
#define MAPVK_VSC_TO_VK_EX 3
#define MAPVK_VK_TO_VSC_EX 4
inline UINT MapVirtualKeyW(UINT uCode, UINT uMapType) { return 0; }
inline UINT MapVirtualKeyExW(UINT uCode, UINT uMapType, HKL dwhkl) { return 0; }
#define MapVirtualKey MapVirtualKeyW
#define MapVirtualKeyEx MapVirtualKeyExW

inline int GetKeyNameTextW(LONG lParam, LPWSTR lpString, int cchSize) { if (lpString && cchSize > 0) lpString[0] = L'\0'; return 0; }
#define GetKeyNameText GetKeyNameTextW

DWORD GetSysColor_impl(int nIndex);

// Resources (stubs - resources loaded differently on macOS)
HRSRC FindResourceW(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType);
HRSRC FindResourceExW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage);
HGLOBAL LoadResource(HMODULE hModule, HRSRC hResInfo);
LPVOID LockResource(HGLOBAL hResData);
DWORD SizeofResource(HMODULE hModule, HRSRC hResInfo);
#define FindResource FindResourceW
#define FindResourceEx FindResourceExW

#define MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEW
#define IS_INTRESOURCE(r) ((((ULONG_PTR)(r)) >> 16) == 0)

// Resource types
#define RT_CURSOR       MAKEINTRESOURCE(1)
#define RT_BITMAP       MAKEINTRESOURCE(2)
#define RT_ICON         MAKEINTRESOURCE(3)
#define RT_MENU         MAKEINTRESOURCE(4)
#define RT_DIALOG       MAKEINTRESOURCE(5)
#define RT_STRING       MAKEINTRESOURCE(6)
#define RT_FONTDIR      MAKEINTRESOURCE(7)
#define RT_FONT         MAKEINTRESOURCE(8)
#define RT_ACCELERATOR  MAKEINTRESOURCE(9)
#define RT_RCDATA       MAKEINTRESOURCE(10)
#define RT_MESSAGETABLE MAKEINTRESOURCE(11)
#define RT_GROUP_CURSOR MAKEINTRESOURCE(12)
#define RT_GROUP_ICON   MAKEINTRESOURCE(14)
#define RT_VERSION      MAKEINTRESOURCE(16)
#define RT_DLGINCLUDE   MAKEINTRESOURCE(17)
#define RT_PLUGPLAY     MAKEINTRESOURCE(19)
#define RT_VXD          MAKEINTRESOURCE(20)
#define RT_ANICURSOR    MAKEINTRESOURCE(21)
#define RT_ANIICON      MAKEINTRESOURCE(22)
#define RT_HTML         MAKEINTRESOURCE(23)
#define RT_MANIFEST     MAKEINTRESOURCE(24)

// LoadString
int LoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cchBufferMax);
#define LoadString LoadStringW

// String resource helpers
#define MAKEINTATOMW(i) ((LPCWSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTATOM MAKEINTATOMW

// DrawEdge flags
#define BDR_RAISEDOUTER 0x0001
#define BDR_SUNKENOUTER 0x0002
#define BDR_RAISEDINNER 0x0004
#define BDR_SUNKENINNER 0x0008
#define EDGE_RAISED     (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN     (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED     (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP       (BDR_RAISEDOUTER | BDR_SUNKENINNER)
#define BF_LEFT         0x0001
#define BF_TOP          0x0002
#define BF_RIGHT        0x0004
#define BF_BOTTOM       0x0008
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#define BF_DIAGONAL     0x0010
#define BF_MIDDLE       0x0800
#define BF_SOFT         0x1000
#define BF_ADJUST       0x2000
#define BF_FLAT         0x4000
#define BF_MONO         0x8000

// DrawFrameControl types
#define DFC_CAPTION  1
#define DFC_MENU     2
#define DFC_SCROLL   3
#define DFC_BUTTON   4
#define DFCS_CAPTIONCLOSE   0x0000
#define DFCS_CAPTIONMIN     0x0001
#define DFCS_CAPTIONMAX     0x0002
#define DFCS_CAPTIONRESTORE 0x0003
#define DFCS_CAPTIONHELP    0x0004
#define DFCS_SCROLLUP       0x0000
#define DFCS_SCROLLDOWN     0x0001
#define DFCS_SCROLLLEFT     0x0002
#define DFCS_SCROLLRIGHT    0x0003
#define DFCS_BUTTONCHECK    0x0000
#define DFCS_BUTTONRADIO    0x0004
#define DFCS_BUTTON3STATE   0x0008
#define DFCS_BUTTONPUSH     0x0010
#define DFCS_CHECKED        0x0400
#define DFCS_FLAT           0x4000
#define DFCS_MONO           0x8000

// Version info stubs
typedef struct tagVS_FIXEDFILEINFO {
	DWORD dwSignature;
	DWORD dwStrucVersion;
	DWORD dwFileVersionMS;
	DWORD dwFileVersionLS;
	DWORD dwProductVersionMS;
	DWORD dwProductVersionLS;
	DWORD dwFileFlagsMask;
	DWORD dwFileFlags;
	DWORD dwFileOS;
	DWORD dwFileType;
	DWORD dwFileSubtype;
	DWORD dwFileDateMS;
	DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;

DWORD GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle);
BOOL GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
BOOL VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen);
#define GetFileVersionInfoSize GetFileVersionInfoSizeW
#define GetFileVersionInfo GetFileVersionInfoW
#define VerQueryValue VerQueryValueW

// ============================================================
// PM_NOREMOVE / PM_REMOVE (for PeekMessage)
// ============================================================
#define PM_NOREMOVE 0x0000
#define PM_REMOVE   0x0001
#define PM_NOYIELD  0x0002

// ============================================================
// SPI constants
// ============================================================
#define SPI_GETMOUSE            3
#define SPI_SETMOUSE            4
#define SPI_SETDESKWALLPAPER    20
#define SPI_SETDESKPATTERN      21

// NONCLIENTMETRICS
typedef struct tagNONCLIENTMETRICSW {
	UINT    cbSize;
	int     iBorderWidth;
	int     iScrollWidth;
	int     iScrollHeight;
	int     iCaptionWidth;
	int     iCaptionHeight;
	LOGFONTW lfCaptionFont;
	int     iSmCaptionWidth;
	int     iSmCaptionHeight;
	LOGFONTW lfSmCaptionFont;
	int     iMenuWidth;
	int     iMenuHeight;
	LOGFONTW lfMenuFont;
	LOGFONTW lfStatusFont;
	LOGFONTW lfMessageFont;
	int     iPaddedBorderWidth;
} NONCLIENTMETRICSW, *PNONCLIENTMETRICSW, *LPNONCLIENTMETRICSW;
typedef NONCLIENTMETRICSW NONCLIENTMETRICS;

// ============================================================
// Subclassing helpers
// ============================================================
typedef LRESULT (*SUBCLASSPROC)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
BOOL SetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
BOOL GetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR* pdwRefData);
BOOL RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass);
LRESULT DefSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ============================================================
// Caret functions
// ============================================================
inline BOOL CreateCaret(HWND hWnd, HBITMAP hBitmap, int nWidth, int nHeight) { return TRUE; }
inline BOOL DestroyCaret() { return TRUE; }
inline BOOL ShowCaret(HWND hWnd) { return TRUE; }
inline BOOL HideCaret(HWND hWnd) { return TRUE; }
inline BOOL SetCaretPos(int X, int Y) { return TRUE; }
inline BOOL GetCaretPos(LPPOINT lpPoint) { if (lpPoint) { lpPoint->x = 0; lpPoint->y = 0; } return TRUE; }
inline UINT SetCaretBlinkTime(UINT uMSeconds) { return 500; }
inline UINT GetCaretBlinkTime() { return 500; }

// ============================================================
// GDI: ROP2 (raster operations), DrawTextEx
// ============================================================
#define R2_BLACK        1
#define R2_NOTMERGEPEN  2
#define R2_MASKNOTPEN   3
#define R2_NOTCOPYPEN   4
#define R2_MASKPENNOT   5
#define R2_NOT          6
#define R2_XORPEN       7
#define R2_NOTMASKPEN   8
#define R2_MASKPEN      9
#define R2_NOTXORPEN    10
#define R2_NOP          11
#define R2_MERGENOTPEN  12
#define R2_COPYPEN      13
#define R2_MERGEPENNOT  14
#define R2_MERGEPEN     15
#define R2_WHITE        16

int GetROP2(HDC hdc);
int SetROP2(HDC hdc, int rop2);

// ============================================================
// Keyboard
// ============================================================
inline int ToAscii(UINT uVirtKey, UINT uScanCode, const BYTE* lpKeyState, LPWORD lpChar, UINT uFlags) { return 0; }

// ============================================================
// Scintilla Win32-specific stubs
// ============================================================
// These are normally in Scintilla.h under #if defined(_WIN32)
// but since we don't define _WIN32 (to avoid poisoning system headers),
// we provide them here as inline stubs.
#ifdef __cplusplus
extern "C" {
#endif
inline int Scintilla_RegisterClasses(void *hInstance) { (void)hInstance; return 1; }
inline int Scintilla_ReleaseResources(void) { return 1; }
#ifdef __cplusplus
}
#endif

#endif // _WINDOWS_H_SHIM_
#endif // __APPLE__
