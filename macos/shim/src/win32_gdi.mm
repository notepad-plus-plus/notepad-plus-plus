// Win32 Shim: GDI function stubs for macOS
// Phase 0: Stub implementations. Phase 1+ will add CoreGraphics-backed drawing.

#import <Foundation/Foundation.h>
#include "windows.h"

// ============================================================
// GDI Object management (stubs)
// ============================================================
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h) { return nullptr; }
BOOL DeleteObject(HGDIOBJ ho) { return TRUE; }
HGDIOBJ GetStockObject(int i) { return reinterpret_cast<HGDIOBJ>(static_cast<uintptr_t>(i + 1)); }
int GetObjectW(HANDLE h, int c, LPVOID pv) { if (pv && c > 0) memset(pv, 0, c); return 0; }
DWORD GetObjectType(HGDIOBJ h) { return 0; }

// ============================================================
// DC management
// ============================================================
HDC CreateCompatibleDC(HDC hdc) { return nullptr; }
BOOL DeleteDC(HDC hdc) { return TRUE; }

// ============================================================
// Bitmap
// ============================================================
HBITMAP CreateCompatibleBitmap(HDC hdc, int cx, int cy) { return nullptr; }
HBITMAP CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitCount, const void* lpBits) { return nullptr; }
HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* pbmi, UINT usage, void** ppvBits, HANDLE hSection, DWORD offset)
{
	if (ppvBits) *ppvBits = nullptr;
	return nullptr;
}
int GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage) { return 0; }
int SetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, const void* lpBits, const BITMAPINFO* lpbmi, UINT ColorUse) { return 0; }
int SetDIBitsToDevice(HDC hdc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT StartScan, UINT cLines, const void* lpvBits, const BITMAPINFO* lpbmi, UINT ColorUse) { return 0; }

// ============================================================
// Pen / Brush / Font / Region creation
// ============================================================
HPEN CreatePen(int iStyle, int cWidth, COLORREF color) { return reinterpret_cast<HPEN>(1); }
HBRUSH CreateSolidBrush(COLORREF color) { return reinterpret_cast<HBRUSH>(1); }
HBRUSH CreateHatchBrush(int iHatch, COLORREF color) { return reinterpret_cast<HBRUSH>(1); }
HBRUSH CreatePatternBrush(HBITMAP hbm) { return reinterpret_cast<HBRUSH>(1); }

HFONT CreateFontIndirectW(const LOGFONTW* lplf) { return reinterpret_cast<HFONT>(1); }
HFONT CreateFontW(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight,
                  DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet,
                  DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality,
                  DWORD iPitchAndFamily, LPCWSTR pszFaceName)
{
	return reinterpret_cast<HFONT>(1);
}

HRGN CreateRectRgn(int x1, int y1, int x2, int y2) { return reinterpret_cast<HRGN>(1); }
HRGN CreateRectRgnIndirect(const RECT* lprect) { return reinterpret_cast<HRGN>(1); }
HRGN CreateRoundRectRgn(int x1, int y1, int x2, int y2, int w, int h) { return reinterpret_cast<HRGN>(1); }
int CombineRgn(HRGN hrgnDst, HRGN hrgnSrc1, HRGN hrgnSrc2, int iMode) { return SIMPLEREGION; }
int SelectClipRgn(HDC hdc, HRGN hrgn) { return SIMPLEREGION; }
int GetClipRgn(HDC hdc, HRGN hrgn) { return 0; }
int ExcludeClipRect(HDC hdc, int left, int top, int right, int bottom) { return SIMPLEREGION; }
int IntersectClipRect(HDC hdc, int left, int top, int right, int bottom) { return SIMPLEREGION; }
BOOL RectVisible(HDC hdc, const RECT* lprect) { return TRUE; }
BOOL DeleteRgn(HRGN hrgn) { return TRUE; }

// ============================================================
// Text / Color
// ============================================================
COLORREF SetTextColor(HDC hdc, COLORREF color) { return 0; }
COLORREF SetBkColor(HDC hdc, COLORREF color) { return 0; }
int SetBkMode(HDC hdc, int mode) { return 0; }
COLORREF GetTextColor(HDC hdc) { return RGB(0, 0, 0); }
COLORREF GetBkColor(HDC hdc) { return RGB(255, 255, 255); }
BOOL SetBrushOrgEx(HDC hdc, int x, int y, LPPOINT lppt) { if (lppt) { lppt->x = 0; lppt->y = 0; } return TRUE; }

BOOL TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c) { return TRUE; }
int DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format) { return 0; }
int DrawTextExW(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp) { return 0; }

int GetROP2(HDC hdc) { return R2_COPYPEN; }
int SetROP2(HDC hdc, int rop2) { return R2_COPYPEN; }

BOOL GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int c, LPSIZE psizl)
{
	if (psizl) { psizl->cx = c * 8; psizl->cy = 16; }
	return TRUE;
}

BOOL GetTextMetricsW(HDC hdc, LPTEXTMETRICW lptm)
{
	if (lptm) {
		memset(lptm, 0, sizeof(TEXTMETRICW));
		lptm->tmHeight = 16;
		lptm->tmAscent = 13;
		lptm->tmDescent = 3;
		lptm->tmAveCharWidth = 8;
		lptm->tmMaxCharWidth = 16;
	}
	return TRUE;
}

int GetTextFaceW(HDC hdc, int c, LPWSTR lpName)
{
	if (lpName && c > 0) {
		wcsncpy(lpName, L"Menlo", c);
		return static_cast<int>(wcslen(lpName));
	}
	return 0;
}

// ============================================================
// Drawing primitives
// ============================================================
BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT lppt) { return TRUE; }
BOOL LineTo(HDC hdc, int x, int y) { return TRUE; }
BOOL Rectangle(HDC hdc, int left, int top, int right, int bottom) { return TRUE; }
BOOL Ellipse(HDC hdc, int left, int top, int right, int bottom) { return TRUE; }
BOOL Win32Polygon(HDC hdc, const POINT* apt, int cpt) { return TRUE; }
BOOL Polyline(HDC hdc, const POINT* apt, int cpt) { return TRUE; }
BOOL RoundRect(HDC hdc, int left, int top, int right, int bottom, int width, int height) { return TRUE; }

COLORREF GetPixel(HDC hdc, int x, int y) { return 0; }
COLORREF SetPixel(HDC hdc, int x, int y, COLORREF color) { return color; }

BOOL FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr) { return TRUE; }
int FrameRect(HDC hDC, const RECT* lprc, HBRUSH hbr) { return 1; }
BOOL DrawEdge(HDC hdc, LPRECT qrc, UINT edge, UINT grfFlags) { return TRUE; }
BOOL DrawFrameControl(HDC hdc, LPRECT lprc, UINT uType, UINT uState) { return TRUE; }
BOOL DrawFocusRect(HDC hDC, const RECT* lprc) { return TRUE; }

// ============================================================
// Blit operations
// ============================================================
BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) { return TRUE; }
BOOL StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
                HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop) { return TRUE; }
BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop) { return TRUE; }
int SetStretchBltMode(HDC hdc, int mode) { return 0; }
BOOL AlphaBlend(HDC hdcDest, int xDest, int yDest, int wDest, int hDest,
                HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, BLENDFUNCTION ftn) { return TRUE; }

// ============================================================
// DC state
// ============================================================
int SaveDC(HDC hdc) { return 1; }
BOOL RestoreDC(HDC hdc, int nSavedDC) { return TRUE; }
int GetDeviceCaps(HDC hdc, int index)
{
	switch (index) {
		case LOGPIXELSX: return 96;
		case LOGPIXELSY: return 96;
		case BITSPIXEL: return 32;
		case PLANES: return 1;
		case HORZRES: return 1920;
		case VERTRES: return 1080;
		default: return 0;
	}
}

// ============================================================
// ImageList (stubs)
// ============================================================
HIMAGELIST ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow) { return reinterpret_cast<HIMAGELIST>(1); }
BOOL ImageList_Destroy(HIMAGELIST himl) { return TRUE; }
int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask) { return 0; }
int ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask) { return 0; }
int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon) { return 0; }
BOOL ImageList_Remove(HIMAGELIST himl, int i) { return TRUE; }
int ImageList_GetImageCount(HIMAGELIST himl) { return 0; }
BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount) { return TRUE; }
BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle) { return TRUE; }
BOOL ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy) { (void)himl; (void)cx; (void)cy; return TRUE; }
HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags) { (void)himl; (void)i; (void)flags; return nullptr; }
BOOL ImageList_GetIconSize(HIMAGELIST himl, int* cx, int* cy) { if (cx) *cx = 16; if (cy) *cy = 16; return TRUE; }
BOOL ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO* pImageInfo) { if (pImageInfo) memset(pImageInfo, 0, sizeof(IMAGEINFO)); return FALSE; }
BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle) { return TRUE; }

// ImageList drag stubs
BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot) { return TRUE; }
BOOL ImageList_DragEnter(HWND hwndLock, int x, int y) { return TRUE; }
BOOL ImageList_DragMove(int x, int y) { return TRUE; }
BOOL ImageList_DragShowNolock(BOOL fShow) { return TRUE; }
BOOL ImageList_DragLeave(HWND hwndLock) { return TRUE; }
void ImageList_EndDrag() {}
HIMAGELIST ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy) { return nullptr; }

// ============================================================
// Icon management (stubs)
// ============================================================
// DestroyIcon is inline in winuser.h
HICON CopyIcon(HICON hIcon) { return hIcon; }

// ============================================================
// Color conversion (HLS ↔ RGB)
// ============================================================
void ColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation)
{
	double r = GetRValue(clrRGB) / 255.0;
	double g = GetGValue(clrRGB) / 255.0;
	double b = GetBValue(clrRGB) / 255.0;

	double maxVal = fmax(r, fmax(g, b));
	double minVal = fmin(r, fmin(g, b));
	double delta = maxVal - minVal;

	double l = (maxVal + minVal) / 2.0;
	double h = 0, s = 0;

	if (delta > 0.0001) {
		s = (l <= 0.5) ? (delta / (maxVal + minVal)) : (delta / (2.0 - maxVal - minVal));
		if (r == maxVal)
			h = (g - b) / delta;
		else if (g == maxVal)
			h = 2.0 + (b - r) / delta;
		else
			h = 4.0 + (r - g) / delta;
		h *= 60.0;
		if (h < 0.0) h += 360.0;
	}

	*pwHue = static_cast<WORD>(h * 240.0 / 360.0);
	*pwLuminance = static_cast<WORD>(l * 240.0);
	*pwSaturation = static_cast<WORD>(s * 240.0);
}

static double HLSToRGBHelper(double v1, double v2, double vH)
{
	if (vH < 0.0) vH += 1.0;
	if (vH > 1.0) vH -= 1.0;
	if (6.0 * vH < 1.0) return v1 + (v2 - v1) * 6.0 * vH;
	if (2.0 * vH < 1.0) return v2;
	if (3.0 * vH < 2.0) return v1 + (v2 - v1) * (2.0 / 3.0 - vH) * 6.0;
	return v1;
}

COLORREF ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation)
{
	double h = wHue / 240.0;
	double l = wLuminance / 240.0;
	double s = wSaturation / 240.0;

	if (s < 0.001) {
		BYTE val = static_cast<BYTE>(l * 255.0);
		return RGB(val, val, val);
	}

	double v2 = (l < 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
	double v1 = 2.0 * l - v2;

	BYTE r = static_cast<BYTE>(HLSToRGBHelper(v1, v2, h + 1.0 / 3.0) * 255.0);
	BYTE g = static_cast<BYTE>(HLSToRGBHelper(v1, v2, h) * 255.0);
	BYTE b = static_cast<BYTE>(HLSToRGBHelper(v1, v2, h - 1.0 / 3.0) * 255.0);
	return RGB(r, g, b);
}

// ============================================================
// Font enumeration (stub)
// ============================================================
int EnumFontFamiliesExW(HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpProc, LPARAM lParam, DWORD dwFlags)
{
	(void)hdc; (void)lpLogfont; (void)lpProc; (void)lParam; (void)dwFlags;
	// Stub: don't enumerate any fonts. Real implementation would use CoreText.
	return 1;
}
