// Win32 Shim: Menu function stubs for macOS
// Phase 0: Stub implementations. Phase 2 will add real NSMenu backing.

#import <Foundation/Foundation.h>
#include "windows.h"
#include "commdlg.h"
#include "shellapi.h"
#include "uxtheme.h"

HMENU CreateMenu() { return reinterpret_cast<HMENU>(1); }
HMENU CreatePopupMenu() { return reinterpret_cast<HMENU>(1); }
BOOL DestroyMenu(HMENU hMenu) { return TRUE; }
HMENU LoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName) { (void)hInstance; (void)lpMenuName; return nullptr; }
HMENU GetMenu(HWND hWnd) { return nullptr; }
BOOL SetMenu(HWND hWnd, HMENU hMenu) { return TRUE; }
HMENU GetSubMenu(HMENU hMenu, int nPos) { return nullptr; }
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert) { return nullptr; }

BOOL AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem) { return TRUE; }
BOOL InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem) { return TRUE; }
BOOL InsertMenuItemW(HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmi) { return TRUE; }
BOOL SetMenuItemInfoW(HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmii) { return TRUE; }
BOOL GetMenuItemInfoW(HMENU hMenu, UINT item, BOOL fByPosition, LPMENUITEMINFOW lpmii)
{
	if (lpmii) memset(lpmii, 0, sizeof(MENUITEMINFOW));
	return FALSE;
}
BOOL RemoveMenu(HMENU hMenu, UINT uPosition, UINT uFlags) { return TRUE; }
BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags) { return TRUE; }
BOOL EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable) { return TRUE; }
BOOL CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck) { return TRUE; }
BOOL CheckMenuRadioItem(HMENU hMenu, UINT first, UINT last, UINT check, UINT flags) { return TRUE; }
int GetMenuItemCount(HMENU hMenu) { return 0; }
UINT GetMenuItemID(HMENU hMenu, int nPos) { return 0; }
BOOL TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, const RECT* prcRect) { return FALSE; }
BOOL TrackPopupMenuEx(HMENU hMenu, UINT fuFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm) { return FALSE; }
BOOL DrawMenuBar(HWND hWnd) { return TRUE; }
int GetMenuStringW(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags)
{
	if (lpString && cchMax > 0) lpString[0] = L'\0';
	return 0;
}
UINT GetMenuState(HMENU hMenu, UINT uId, UINT uFlags) { return 0; }
BOOL ModifyMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem) { return TRUE; }

// ============================================================
// Accelerators (stubs)
// ============================================================
HACCEL LoadAcceleratorsW(HINSTANCE hInstance, LPCWSTR lpTableName) { return nullptr; }
int TranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg) { return 0; }
HACCEL CreateAcceleratorTableW(LPACCEL paccel, int cAccel) { return nullptr; }
BOOL DestroyAcceleratorTable(HACCEL hAccel) { return TRUE; }

// ============================================================
// Clipboard (stubs)
// ============================================================
BOOL OpenClipboard(HWND hWndNewOwner) { return TRUE; }
BOOL CloseClipboard() { return TRUE; }
BOOL EmptyClipboard() { return TRUE; }
HANDLE SetClipboardData(UINT uFormat, HANDLE hMem) { return hMem; }
HANDLE GetClipboardData(UINT uFormat) { return nullptr; }
BOOL IsClipboardFormatAvailable(UINT format) { return FALSE; }
int CountClipboardFormats() { return 0; }
UINT EnumClipboardFormats(UINT format) { return 0; }
HWND GetClipboardOwner() { return nullptr; }

// ============================================================
// Shell API stubs
// ============================================================
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	return reinterpret_cast<HINSTANCE>(33); // > 32 means success
}

// Overload for filesystem::path::c_str() which returns const char* on macOS
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, const char* lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	(void)hwnd; (void)lpOperation; (void)lpFile; (void)lpParameters; (void)lpDirectory; (void)nShowCmd;
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
// Common dialog stubs
// ============================================================
BOOL GetOpenFileNameW(LPOPENFILENAMEW lpofn) { return FALSE; }
BOOL GetSaveFileNameW(LPOPENFILENAMEW lpofn) { return FALSE; }
BOOL ChooseColorW(LPCHOOSECOLORW lpcc) { return FALSE; }
BOOL ChooseFontW(LPCHOOSEFONTW lpcf) { return FALSE; }
BOOL PrintDlgW(LPPRINTDLGW lppd) { return FALSE; }
HWND FindTextW(LPFINDREPLACEW lpfr) { return nullptr; }
HWND ReplaceTextW(LPFINDREPLACEW lpfr) { return nullptr; }
DWORD CommDlgExtendedError() { return 0; }

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
