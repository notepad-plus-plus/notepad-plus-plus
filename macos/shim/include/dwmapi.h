#pragma once
// Win32 Shim: Desktop Window Manager API stubs for macOS

#include "windef.h"

#define DWM_BB_ENABLE 0x00000001
#define DWM_BB_BLURREGION 0x00000002
#define DWM_BB_TRANSITIONONMAXIMIZED 0x00000004

#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWA_BORDER_COLOR 34
#define DWMWA_CAPTION_COLOR 35
#define DWMWA_TEXT_COLOR 36

typedef struct _DWM_BLURBEHIND {
	DWORD dwFlags;
	BOOL  fEnable;
	HRGN  hRgnBlur;
	BOOL  fTransitionOnMaximized;
} DWM_BLURBEHIND, *PDWM_BLURBEHIND;

inline HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) { return 0; }
inline HRESULT DwmEnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind) { return 0; }
inline HRESULT DwmExtendFrameIntoClientArea(HWND hWnd, const void* pMarInset) { return 0; }
inline HRESULT DwmIsCompositionEnabled(BOOL* pfEnabled) { if (pfEnabled) *pfEnabled = FALSE; return 0; }
inline HRESULT DwmDefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { return 1; }
inline HRESULT DwmGetColorizationColor(DWORD* pcrColorization, BOOL* pfOpaqueBlend) {
	if (pcrColorization) *pcrColorization = 0xFF000000;
	if (pfOpaqueBlend) *pfOpaqueBlend = TRUE;
	return 0;
}
