#pragma once
// Win32 Shim: Visual Styles / Theme API for macOS (stubs)

#include "windef.h"
#include "wingdi.h"

// Theme handles
typedef void* HTHEME_HANDLE;
#ifndef HTHEME
// HTHEME already defined in windef.h as void*
#endif

// Theme functions (stubs - dark mode handled by NSAppearance)
HTHEME OpenThemeData(HWND hwnd, LPCWSTR pszClassList);
HRESULT CloseThemeData(HTHEME hTheme);
HRESULT DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect);
HRESULT DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT* pRect);
HRESULT GetThemeColor(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF* pColor);
HRESULT GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* prc, int eSize, SIZE* psz);
HRESULT GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pBoundingRect, RECT* pContentRect);
BOOL IsThemeActive();
BOOL IsAppThemed();
HRESULT SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
HRESULT EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags);
HRESULT GetThemeMetric(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, int* piVal);
HRESULT GetThemeFont(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LOGFONTW* pFont);

// Theme part/state IDs used in Notepad++
// Button parts
#define BP_PUSHBUTTON 1
#define BP_RADIOBUTTON 2
#define BP_CHECKBOX   3
#define BP_GROUPBOX   4

// Button states
#define PBS_NORMAL   1
#define PBS_HOT      2
#define PBS_PRESSED  3
#define PBS_DISABLED 4
#define PBS_DEFAULTED 5

// Tab parts
#define TABP_TABITEM          1
#define TABP_TABITEMLEFTEDGE  2
#define TABP_TABITEMRIGHTEDGE 3
#define TABP_TABITEMBOTHEDGE  4
#define TABP_TOPTABITEM       5
#define TABP_PANE             9
#define TABP_BODY             10

// Tab states
#define TIS_NORMAL   1
#define TIS_HOT      2
#define TIS_SELECTED 3
#define TIS_DISABLED 4
#define TIS_FOCUSED  5

// Scrollbar parts
#define SBP_ARROWBTN 1
#define SBP_THUMBBTNHORZ 2
#define SBP_THUMBBTNVERT 3
#define SBP_LOWERTRACKHORZ 4
#define SBP_UPPERTRACKHORZ 5
#define SBP_LOWERTRACKVERT 6
#define SBP_UPPERTRACKVERT 7
#define SBP_GRIPPERHORZ    8
#define SBP_GRIPPERVERT    9
#define SBP_SIZEBOX        10

// Theme property IDs
#define TMT_COLOR       204
#define TMT_TEXTCOLOR   3803
#define TMT_FILLCOLOR   3802

// Size type for GetThemePartSize
#define TS_MIN  0
#define TS_TRUE 1
#define TS_DRAW 2

// EnableThemeDialogTexture flags
#define ETDT_DISABLE       0x00000001
#define ETDT_ENABLE        0x00000002
#define ETDT_USETABTEXTURE 0x00000004
#define ETDT_ENABLETAB     (ETDT_ENABLE | ETDT_USETABTEXTURE)

// DWM stubs — declarations moved to dwmapi.h
// Include dwmapi.h for DwmSetWindowAttribute, DwmDefWindowProc, etc.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

// DrawThemeParentBackground
inline HRESULT DrawThemeParentBackground(HWND hwnd, HDC hdc, const RECT* prc) { return 0; }

// DrawThemeTextEx options
#define DTT_TEXTCOLOR  (1UL << 0)
#define DTT_BORDERCOLOR (1UL << 1)
#define DTT_SHADOWCOLOR (1UL << 2)
#define DTT_SHADOWTYPE (1UL << 3)
#define DTT_SHADOWOFFSET (1UL << 4)
#define DTT_BORDERSIZE (1UL << 5)
#define DTT_FONTPROP   (1UL << 6)
#define DTT_COLORPROP  (1UL << 7)
#define DTT_STATEID    (1UL << 8)
#define DTT_CALCRECT   (1UL << 9)
#define DTT_APPLYOVERLAY (1UL << 10)
#define DTT_GLOWSIZE   (1UL << 11)
#define DTT_CALLBACK   (1UL << 12)
#define DTT_COMPOSITED (1UL << 13)
#define DTT_VALIDBITS  (DTT_TEXTCOLOR | DTT_BORDERCOLOR | DTT_SHADOWCOLOR | DTT_SHADOWTYPE | DTT_SHADOWOFFSET | DTT_BORDERSIZE | DTT_FONTPROP | DTT_COLORPROP | DTT_STATEID | DTT_CALCRECT | DTT_APPLYOVERLAY | DTT_GLOWSIZE | DTT_COMPOSITED)

typedef int (WINAPI *DTT_CALLBACK_PROC)(HDC hdc, LPWSTR pszText, int cchText, LPRECT prc, UINT dwFlags, LPARAM lParam);

typedef struct _DTTOPTS {
	DWORD    dwSize;
	DWORD    dwFlags;
	COLORREF crText;
	COLORREF crBorder;
	COLORREF crShadow;
	int      iTextShadowType;
	POINT    ptShadowOffset;
	int      iBorderSize;
	int      iFontPropId;
	int      iColorPropId;
	int      iStateId;
	BOOL     fApplyOverlay;
	int      iGlowSize;
	DTT_CALLBACK_PROC pfnDrawTextCallback;
	LPARAM   lParam;
} DTTOPTS, *PDTTOPTS;

inline HRESULT DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, LPRECT pRect, const DTTOPTS* pOptions) { return 0; }

// Buffered painting (stubs)
typedef void* HPAINTBUFFER;
typedef void* HANIMATIONBUFFER;

typedef enum _BP_BUFFERFORMAT {
	BPBF_COMPATIBLEBITMAP,
	BPBF_DIB,
	BPBF_TOPDOWNDIB,
	BPBF_TOPDOWNMONODIB
} BP_BUFFERFORMAT;

typedef struct _BP_PAINTPARAMS {
	DWORD cbSize;
	DWORD dwFlags;
	const RECT* prcExclude;
	const void* pBlendFunction;
} BP_PAINTPARAMS, *PBP_PAINTPARAMS;

typedef enum _BP_ANIMATIONSTYLE {
	BPAS_NONE,
	BPAS_LINEAR,
	BPAS_CUBIC,
	BPAS_SINE
} BP_ANIMATIONSTYLE;

typedef struct _BP_ANIMATIONPARAMS {
	DWORD cbSize;
	DWORD dwFlags;
	BP_ANIMATIONSTYLE style;
	DWORD dwDuration;
} BP_ANIMATIONPARAMS, *PBP_ANIMATIONPARAMS;

inline HRESULT BufferedPaintInit() { return 0; }
inline HRESULT BufferedPaintUnInit() { return 0; }
inline HPAINTBUFFER BeginBufferedPaint(HDC hdcTarget, const RECT* prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS* pPaintParams, HDC* phdc) { if (phdc) *phdc = hdcTarget; return nullptr; }
inline HRESULT EndBufferedPaint(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget) { return 0; }
inline HRESULT BufferedPaintSetAlpha(HPAINTBUFFER hBufferedPaint, const RECT* prc, BYTE alpha) { return 0; }
inline BOOL BufferedPaintRenderAnimation(HWND hwnd, HDC hdcTarget) { return FALSE; }
inline HRESULT BufferedPaintStopAllAnimations(HWND hwnd) { return 0; }
inline HANIMATIONBUFFER BeginBufferedAnimation(HWND hwnd, HDC hdcTarget, const RECT* prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS* pPaintParams, BP_ANIMATIONPARAMS* pAnimationParams, HDC* phdcFrom, HDC* phdcTo) {
	if (phdcFrom) *phdcFrom = nullptr;
	if (phdcTo) *phdcTo = nullptr;
	return nullptr;
}
inline HRESULT EndBufferedAnimation(HANIMATIONBUFFER hbpAnimation, BOOL fUpdateTarget) { return 0; }
inline HRESULT GetThemeTransitionDuration(HTHEME hTheme, int iPartId, int iStateIdFrom, int iStateIdTo, int iPropId, DWORD* pdwDuration) { if (pdwDuration) *pdwDuration = 0; return 0; }
