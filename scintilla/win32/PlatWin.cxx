// Scintilla source code edit control
/** @file PlatWin.cxx
 ** Implementation of platform facilities on Windows.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <climits>

#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>

// Want to use std::min and std::max so don't want Windows.h version of min and max
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef WINVER
#define WINVER 0x0A00
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>
#include <shellscalingapi.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#if !defined(DISABLE_D2D)
#define USE_D2D 1
#endif

#if defined(USE_D2D)
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#endif

#include "ScintillaTypes.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"
#include "XPM.h"
#include "UniConversion.h"
#include "DBCS.h"

#include "WinTypes.h"
#include "PlatWin.h"
#include "ListBox.h"
#if defined(USE_D2D)
#include "SurfaceD2D.h"
#endif

using namespace Scintilla;

namespace Scintilla::Internal {

HINSTANCE hinstPlatformRes{};

void *PointerFromWindow(HWND hWnd) noexcept {
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, 0));
}

void SetWindowPointer(HWND hWnd, void *ptr) noexcept {
	::SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(ptr));
}

namespace {

// system DPI, same for all monitor.
UINT uSystemDPI = USER_DEFAULT_SCREEN_DPI;

using GetDpiForWindowSig = UINT(WINAPI *)(HWND hwnd);
GetDpiForWindowSig fnGetDpiForWindow = nullptr;

HMODULE hDLLShcore{};
using GetDpiForMonitorSig = HRESULT(WINAPI *)(HMONITOR hmonitor, /*MONITOR_DPI_TYPE*/int dpiType, UINT *dpiX, UINT *dpiY);
GetDpiForMonitorSig fnGetDpiForMonitor = nullptr;

using GetSystemMetricsForDpiSig = int(WINAPI *)(int nIndex, UINT dpi);
GetSystemMetricsForDpiSig fnGetSystemMetricsForDpi = nullptr;

using AdjustWindowRectExForDpiSig = BOOL(WINAPI *)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
AdjustWindowRectExForDpiSig fnAdjustWindowRectExForDpi = nullptr;

using AreDpiAwarenessContextsEqualSig = BOOL(WINAPI *)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
AreDpiAwarenessContextsEqualSig fnAreDpiAwarenessContextsEqual = nullptr;

using GetWindowDpiAwarenessContextSig = DPI_AWARENESS_CONTEXT(WINAPI *)(HWND);
GetWindowDpiAwarenessContextSig fnGetWindowDpiAwarenessContext = nullptr;

using GetScaleFactorForMonitorSig = HRESULT(WINAPI *)(HMONITOR, DEVICE_SCALE_FACTOR *);
GetScaleFactorForMonitorSig fnGetScaleFactorForMonitor = nullptr;

using SetThreadDpiAwarenessContextSig = DPI_AWARENESS_CONTEXT(WINAPI *)(DPI_AWARENESS_CONTEXT);
SetThreadDpiAwarenessContextSig fnSetThreadDpiAwarenessContext = nullptr;

void LoadDpiForWindow() noexcept {
	HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
	fnGetDpiForWindow = DLLFunction<GetDpiForWindowSig>(user32, "GetDpiForWindow");
	fnGetSystemMetricsForDpi = DLLFunction<GetSystemMetricsForDpiSig>(user32, "GetSystemMetricsForDpi");
	fnAdjustWindowRectExForDpi = DLLFunction<AdjustWindowRectExForDpiSig>(user32, "AdjustWindowRectExForDpi");
	fnSetThreadDpiAwarenessContext = DLLFunction<SetThreadDpiAwarenessContextSig>(user32, "SetThreadDpiAwarenessContext");

	using GetDpiForSystemSig = UINT(WINAPI *)(void);
	GetDpiForSystemSig fnGetDpiForSystem = DLLFunction<GetDpiForSystemSig>(user32, "GetDpiForSystem");
	if (fnGetDpiForSystem) {
		uSystemDPI = fnGetDpiForSystem();
	} else {
		HDC hdcMeasure = ::CreateCompatibleDC({});
		uSystemDPI = ::GetDeviceCaps(hdcMeasure, LOGPIXELSY);
		::DeleteDC(hdcMeasure);
	}

	fnGetWindowDpiAwarenessContext = DLLFunction<GetWindowDpiAwarenessContextSig>(user32, "GetWindowDpiAwarenessContext");
	fnAreDpiAwarenessContextsEqual = DLLFunction<AreDpiAwarenessContextsEqualSig>(user32, "AreDpiAwarenessContextsEqual");

	hDLLShcore = ::LoadLibraryExW(L"shcore.dll", {}, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDLLShcore) {
		fnGetScaleFactorForMonitor = DLLFunction<GetScaleFactorForMonitorSig>(hDLLShcore, "GetScaleFactorForMonitor");
		fnGetDpiForMonitor = DLLFunction<GetDpiForMonitorSig>(hDLLShcore, "GetDpiForMonitor");
	}
}

}

HMONITOR MonitorFromWindowHandleScaling(HWND hWnd) noexcept {
	constexpr DWORD monitorFlags = MONITOR_DEFAULTTONEAREST;

	if (!fnSetThreadDpiAwarenessContext) {
		return ::MonitorFromWindow(hWnd, monitorFlags);
	}

	// Temporarily switching to PerMonitorV2 to retrieve correct monitor via MonitorFromRect() in case of active GDI scaling.
	const DPI_AWARENESS_CONTEXT oldContext = fnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	PLATFORM_ASSERT(oldContext != nullptr);

	RECT rect;
	::GetWindowRect(hWnd, &rect);
	const HMONITOR monitor = ::MonitorFromRect(&rect, monitorFlags);

	fnSetThreadDpiAwarenessContext(oldContext);
	return monitor;
}

float GetDeviceScaleFactorWhenGdiScalingActive(HWND hWnd) noexcept {
	if (fnAreDpiAwarenessContextsEqual) {
		PLATFORM_ASSERT(fnGetWindowDpiAwarenessContext && fnGetScaleFactorForMonitor);
		if (fnAreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED, fnGetWindowDpiAwarenessContext(hWnd))) {
			const HWND hRootWnd = ::GetAncestor(hWnd, GA_ROOT); // Scale factor applies to entire (root) window.
			const HMONITOR hMonitor = MonitorFromWindowHandleScaling(hRootWnd);
			DEVICE_SCALE_FACTOR deviceScaleFactor;
			if (S_OK == fnGetScaleFactorForMonitor(hMonitor, &deviceScaleFactor))
				return static_cast<int>(deviceScaleFactor) / 100.f;
		}
	}
	return 1.f;
}

UINT DpiForWindow(WindowID wid) noexcept {
	if (fnGetDpiForWindow) {
		return fnGetDpiForWindow(HwndFromWindowID(wid));
	}
	if (fnGetDpiForMonitor) {
		HMONITOR hMonitor = ::MonitorFromWindow(HwndFromWindowID(wid), MONITOR_DEFAULTTONEAREST);
		UINT dpiX = 0;
		UINT dpiY = 0;
		if (fnGetDpiForMonitor(hMonitor, 0 /*MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY) == S_OK) {
			return dpiY;
		}
	}
	return uSystemDPI;
}

int SystemMetricsForDpi(int nIndex, UINT dpi) noexcept {
	if (fnGetSystemMetricsForDpi) {
		return fnGetSystemMetricsForDpi(nIndex, dpi);
	}

	int value = ::GetSystemMetrics(nIndex);
	value = (dpi == uSystemDPI) ? value : ::MulDiv(value, dpi, uSystemDPI);
	return value;
}

void AdjustWindowRectForDpi(LPRECT lpRect, DWORD dwStyle, UINT dpi) noexcept {
	if (fnAdjustWindowRectExForDpi) {
		fnAdjustWindowRectExForDpi(lpRect, dwStyle, false, WS_EX_WINDOWEDGE, dpi);
	} else {
		::AdjustWindowRectEx(lpRect, dwStyle, false, WS_EX_WINDOWEDGE);
	}
}

namespace {

constexpr BITMAPV5HEADER BitMapHeader(int width, int height) noexcept {
	constexpr int pixelBits = 32;

	// Divide each pixel up in the expected BGRA manner.
	// Compatible with DXGI_FORMAT_B8G8R8A8_UNORM.
	constexpr DWORD maskRed = 0x00FF0000U;
	constexpr DWORD maskGreen = 0x0000FF00U;
	constexpr DWORD maskBlue = 0x000000FFU;
	constexpr DWORD maskAlpha = 0xFF000000U;

	BITMAPV5HEADER bi{};
	bi.bV5Size = sizeof(BITMAPV5HEADER);
	bi.bV5Width = width;
	bi.bV5Height = height;
	bi.bV5Planes = 1;
	bi.bV5BitCount = pixelBits;
	bi.bV5Compression = BI_BITFIELDS;
	// The following mask specification specifies a supported 32 BPP alpha format for Windows XP.
	bi.bV5RedMask = maskRed;
	bi.bV5GreenMask = maskGreen;
	bi.bV5BlueMask = maskBlue;
	bi.bV5AlphaMask = maskAlpha;
	return bi;
}

HBITMAP BitMapSection(HDC hdc, int width, int height, DWORD **pixels) noexcept {
	const BITMAPV5HEADER bi = BitMapHeader(width, height);
	void *image = nullptr;
	HBITMAP hbm = ::CreateDIBSection(hdc, reinterpret_cast<const BITMAPINFO *>(&bi), DIB_RGB_COLORS, &image, {}, 0);
	if (pixels) {
		*pixels = static_cast<DWORD *>(image);
	}
	return hbm;
}

}

GDIBitMap::~GDIBitMap() noexcept {
	Release();
}

void GDIBitMap::Create(HDC hdcBase, int width, int height, DWORD **pixels) noexcept {
	Release();

	hdc = CreateCompatibleDC(hdcBase);
	if (!hdc) {
		return;
	}

	hbm = BitMapSection(hdc, width, height, pixels);
	if (!hbm) {
		return;
	}
	hbmOriginal = SelectBitmap(hdc, hbm);
}

void GDIBitMap::Release() noexcept {
	if (hbmOriginal) {
		// Deselect HBITMAP from HDC so it may be deleted.
		SelectBitmap(hdc, hbmOriginal);
	}
	hbmOriginal = {};
	if (hbm) {
		::DeleteObject(hbm);
	}
	hbm = {};
	if (hdc) {
		::DeleteDC(hdc);
	}
	hdc = {};
}

HBITMAP GDIBitMap::Extract() noexcept {
	// Deselect HBITMAP from HDC but keep so can delete.
	// The caller will make a copy, not take ownership.
	HBITMAP ret = hbm;
	if (hbmOriginal) {
		SelectBitmap(hdc, hbmOriginal);
		hbmOriginal = {};
	}
	return ret;
}

Window::~Window() noexcept = default;

void Window::Destroy() noexcept {
	if (wid)
		::DestroyWindow(HwndFromWindowID(wid));
	wid = nullptr;
}

PRectangle Window::GetPosition() const {
	RECT rc;
	::GetWindowRect(HwndFromWindowID(wid), &rc);
	return PRectangleFromRECT(rc);
}

void Window::SetPosition(PRectangle rc) {
	::SetWindowPos(HwndFromWindowID(wid),
		{}, static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), SWP_NOZORDER | SWP_NOACTIVATE);
}

namespace {

RECT RectFromMonitor(HMONITOR hMonitor) noexcept {
	MONITORINFO mi = {};
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfo(hMonitor, &mi)) {
		return mi.rcWork;
	}
	RECT rc = {0, 0, 0, 0};
	if (::SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc, 0) == 0) {
		rc.left = 0;
		rc.top = 0;
		rc.right = 0;
		rc.bottom = 0;
	}
	return rc;
}

}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
	const DWORD style = GetWindowStyle(HwndFromWindowID(wid));
	if (style & WS_POPUP) {
		POINT ptOther = {0, 0};
		::ClientToScreen(HwndFromWindow(*relativeTo), &ptOther);
		rc.Move(static_cast<XYPOSITION>(ptOther.x), static_cast<XYPOSITION>(ptOther.y));

		const RECT rcMonitor = RectFromPRectangle(rc);

		HMONITOR hMonitor = MonitorFromRect(&rcMonitor, MONITOR_DEFAULTTONEAREST);
		// If hMonitor is NULL, that's just the main screen anyways.
		const RECT rcWork = RectFromMonitor(hMonitor);

		if (rcWork.left < rcWork.right) {
			// Now clamp our desired rectangle to fit inside the work area
			// This way, the menu will fit wholly on one screen. An improvement even
			// if you don't have a second monitor on the left... Menu's appears half on
			// one screen and half on the other are just U.G.L.Y.!
			if (rc.right > rcWork.right)
				rc.Move(rcWork.right - rc.right, 0);
			if (rc.bottom > rcWork.bottom)
				rc.Move(0, rcWork.bottom - rc.bottom);
			if (rc.left < rcWork.left)
				rc.Move(rcWork.left - rc.left, 0);
			if (rc.top < rcWork.top)
				rc.Move(0, rcWork.top - rc.top);
		}
	}
	SetPosition(rc);
}

PRectangle Window::GetClientPosition() const {
	RECT rc={0,0,0,0};
	if (wid)
		::GetClientRect(HwndFromWindowID(wid), &rc);
	return PRectangleFromRECT(rc);
}

void Window::Show(bool show) {
	if (show)
		::ShowWindow(HwndFromWindowID(wid), SW_SHOWNOACTIVATE);
	else
		::ShowWindow(HwndFromWindowID(wid), SW_HIDE);
}

void Window::InvalidateAll() {
	::InvalidateRect(HwndFromWindowID(wid), nullptr, FALSE);
}

void Window::InvalidateRectangle(PRectangle rc) {
	const RECT rcw = RectFromPRectangle(rc);
	::InvalidateRect(HwndFromWindowID(wid), &rcw, FALSE);
}

namespace {

std::optional<DWORD> RegGetDWORD(HKEY hKey, LPCWSTR valueName) noexcept {
	DWORD value = 0;
	DWORD type = REG_NONE;
	DWORD size = sizeof(DWORD);
	const LSTATUS status = ::RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
	if (status == ERROR_SUCCESS && type == REG_DWORD) {
		return value;
	}
	return {};
}

class CursorHelper {
	GDIBitMap bm;
	DWORD *pixels = nullptr;
	const int width;
	const int height;

	static constexpr float arrow[][2] = {
		{ 32.0f - 12.73606f,32.0f - 19.04075f },
		{ 32.0f - 7.80159f, 32.0f - 19.04075f },
		{ 32.0f - 9.82813f, 32.0f - 14.91828f },
		{ 32.0f - 6.88341f, 32.0f - 13.42515f },
		{ 32.0f - 4.62301f, 32.0f - 18.05872f },
		{ 32.0f - 1.26394f, 32.0f - 14.78295f },
		{ 32.0f - 1.26394f, 32.0f - 30.57485f },
	};

public:
	~CursorHelper() = default;

	CursorHelper(int width_, int height_) noexcept : width{width_}, height{height_} {
		// https://learn.microsoft.com/en-us/windows/win32/menurc/using-cursors#creating-a-cursor
		bm.Create({}, width, height, &pixels);
	}

	[[nodiscard]] explicit operator bool() const noexcept {
		return static_cast<bool>(bm);
	}

	HCURSOR Create() noexcept {
		HCURSOR cursor {};
		// Create an empty mask bitmap.
		HBITMAP hMonoBitmap = ::CreateBitmap(width, height, 1, 1, nullptr);
		if (hMonoBitmap) {
			// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createiconindirect
			// hBitmap should not already be selected into a device context
			HBITMAP hBitmap = bm.Extract();
			ICONINFO info = {false, static_cast<DWORD>(width - 1), 0, hMonoBitmap, hBitmap};
			cursor = ::CreateIconIndirect(&info);
			::DeleteObject(hMonoBitmap);
		}
		return cursor;
	}

#if defined(USE_D2D)

	bool DrawD2D(COLORREF fillColour, COLORREF strokeColour) noexcept {
		if (!LoadD2D()) {
			return false;
		}

		const D2D1_RENDER_TARGET_PROPERTIES drtp = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED });

		DCRenderTarget pTarget;
		HRESULT hr = CreateDCRenderTarget(&drtp, pTarget);
		if (FAILED(hr) || !pTarget) {
			return false;
		}

		const RECT rc = {0, 0, width, height};
		hr = pTarget->BindDC(bm.DC(), &rc);
		if (FAILED(hr)) {
			return false;
		}

		pTarget->BeginDraw();

		// Draw something on the bitmap section.
		constexpr size_t nPoints = std::size(arrow);
		D2D1_POINT_2F points[nPoints]{};
		const FLOAT scale = width/32.0f;
		for (size_t i = 0; i < nPoints; i++) {
			points[i].x = arrow[i][0] * scale;
			points[i].y = arrow[i][1] * scale;
		}

		const Geometry geometry = GeometryCreate();
		if (!geometry) {
			return false;
		}

		const GeometrySink sink = GeometrySinkCreate(geometry.Get());
		if (!sink) {
			return false;
		}

		sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
		for (size_t i = 1; i < nPoints; i++) {
			sink->AddLine(points[i]);
		}
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		hr = sink->Close();
		if (FAILED(hr)) {
			return false;
		}

		if (const BrushSolid pBrushFill = BrushSolidCreate(pTarget.Get(), fillColour)) {
			pTarget->FillGeometry(geometry.Get(), pBrushFill.Get());
		}

		if (const BrushSolid pBrushStroke = BrushSolidCreate(pTarget.Get(), strokeColour)) {
			pTarget->DrawGeometry(geometry.Get(), pBrushStroke.Get(), scale);
		}

		hr = pTarget->EndDraw();
		return SUCCEEDED(hr);
	}
#endif

	void Draw(COLORREF fillColour, COLORREF strokeColour) noexcept {
#if defined(USE_D2D)
		if (DrawD2D(fillColour, strokeColour)) {
			return;
		}
#endif

		// Draw something on the DIB section.
		constexpr size_t nPoints = std::size(arrow);
		POINT points[nPoints]{};
		const float scale = width/32.0f;
		for (size_t i = 0; i < nPoints; i++) {
			points[i].x = std::lround(arrow[i][0] * scale);
			points[i].y = std::lround(arrow[i][1] * scale);
		}

		const DWORD penWidth = std::lround(scale);
		HPEN pen{};
		if (penWidth > 1) {
			const LOGBRUSH brushParameters { BS_SOLID, strokeColour, 0 };
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_MITER,
				penWidth,
				&brushParameters,
				0,
				nullptr);
		} else {
			pen = ::CreatePen(PS_INSIDEFRAME, 1, strokeColour);
		}

		HPEN penOld = SelectPen(bm.DC(), pen);
		HBRUSH brush = ::CreateSolidBrush(fillColour);
		HBRUSH brushOld = SelectBrush(bm.DC(), brush);
		::Polygon(bm.DC(), points, static_cast<int>(nPoints));
		SelectPen(bm.DC(), penOld);
		SelectBrush(bm.DC(), brushOld);
		::DeleteObject(pen);
		::DeleteObject(brush);

		// Set the alpha values for each pixel in the cursor.
		constexpr DWORD opaque = 0xFF000000U;
		for (int i = 0; i < width*height; i++) {
			if (*pixels != 0) {
				*pixels |= opaque;
			}
			pixels++;
		}
	}
};

void ChooseCursor(LPCTSTR cursor) noexcept {
	::SetCursor(::LoadCursor({}, cursor));
}

void ChooseCursor(Window::Cursor curs) noexcept {
	switch (curs) {
	case Window::Cursor::text:
		ChooseCursor(IDC_IBEAM);
		break;
	case Window::Cursor::up:
		ChooseCursor(IDC_UPARROW);
		break;
	case Window::Cursor::wait:
		ChooseCursor(IDC_WAIT);
		break;
	case Window::Cursor::horizontal:
		ChooseCursor(IDC_SIZEWE);
		break;
	case Window::Cursor::vertical:
		ChooseCursor(IDC_SIZENS);
		break;
	case Window::Cursor::hand:
		ChooseCursor(IDC_HAND);
		break;
	case Window::Cursor::reverseArrow:
	case Window::Cursor::arrow:
	case Window::Cursor::invalid:	// Should not occur, but just in case.
	default:
		ChooseCursor(IDC_ARROW);
		break;
	}
}

}

HCURSOR LoadReverseArrowCursor(UINT dpi) noexcept {
	// https://learn.microsoft.com/en-us/answers/questions/815036/windows-cursor-size
	constexpr DWORD defaultCursorBaseSize = 32;
	constexpr DWORD maxCursorBaseSize = 16*(1 + 15); // 16*(1 + CursorSize)
	DWORD cursorBaseSize = 0;
	HKEY hKey {};
	LSTATUS status = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Cursors", 0, KEY_QUERY_VALUE, &hKey);
	if (status == ERROR_SUCCESS) {
		if (std::optional<DWORD> baseSize = RegGetDWORD(hKey, L"CursorBaseSize")) {
			// CursorBaseSize is multiple of 16
			cursorBaseSize = std::min(*baseSize & ~15, maxCursorBaseSize);
		}
		::RegCloseKey(hKey);
	}

	int width = 0;
	int height = 0;
	if (cursorBaseSize > defaultCursorBaseSize) {
		width = ::MulDiv(cursorBaseSize, dpi, USER_DEFAULT_SCREEN_DPI);
		height = width;
	} else {
		width = SystemMetricsForDpi(SM_CXCURSOR, dpi);
		height = SystemMetricsForDpi(SM_CYCURSOR, dpi);
		PLATFORM_ASSERT(width == height);
	}

	CursorHelper cursorHelper(width, height);
	if (!cursorHelper) {
		return {};
	}

	COLORREF fillColour = RGB(0xff, 0xff, 0xfe);
	COLORREF strokeColour = RGB(0, 0, 1);
	status = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Accessibility", 0, KEY_QUERY_VALUE, &hKey);
	if (status == ERROR_SUCCESS) {
		if (std::optional<DWORD> cursorType = RegGetDWORD(hKey, L"CursorType")) {
			switch (*cursorType) {
			case 1: // black
			case 4: // black
				std::swap(fillColour, strokeColour);
				break;
			case 6: // custom
				if (std::optional<DWORD> cursorColor = RegGetDWORD(hKey, L"CursorColor")) {
					fillColour = *cursorColor;
				}
				break;
			default: // 0, 3 white, 2, 5 invert
				break;
			}
		}
		::RegCloseKey(hKey);
	}

	cursorHelper.Draw(fillColour, strokeColour);
	HCURSOR cursor = cursorHelper.Create();
	return cursor;
}

void Window::SetCursor(Cursor curs) {
	ChooseCursor(curs);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
	const PRectangle rcPosition = GetPosition();
	const POINT ptDesktop = {static_cast<LONG>(pt.x + rcPosition.left),
		static_cast<LONG>(pt.y + rcPosition.top)};
	HMONITOR hMonitor = MonitorFromPoint(ptDesktop, MONITOR_DEFAULTTONEAREST);

	const RECT rcWork = RectFromMonitor(hMonitor);
	if (rcWork.left < rcWork.right) {
		PRectangle rcMonitor(
			rcWork.left - rcPosition.left,
			rcWork.top - rcPosition.top,
			rcWork.right - rcPosition.left,
			rcWork.bottom - rcPosition.top);
		return rcMonitor;
	}
	return PRectangle();
}

Menu::Menu() noexcept : mid{} {
}

void Menu::CreatePopUp() {
	Destroy();
	mid = ::CreatePopupMenu();
}

void Menu::Destroy() noexcept {
	if (mid)
		::DestroyMenu(static_cast<HMENU>(mid));
	mid = {};
}

void Menu::Show(Point pt, const Window &w) {
	::TrackPopupMenu(static_cast<HMENU>(mid),
		TPM_RIGHTBUTTON, static_cast<int>(pt.x - 4), static_cast<int>(pt.y), 0,
		HwndFromWindow(w), nullptr);
	Destroy();
}

ColourRGBA ColourFromSys(int nIndex) noexcept {
	const DWORD colourValue = ::GetSysColor(nIndex);
	return ColourRGBA::FromRGB(colourValue);
}

ColourRGBA Platform::Chrome() {
	return ColourFromSys(COLOR_3DFACE);
}

ColourRGBA Platform::ChromeHighlight() {
	return ColourFromSys(COLOR_3DHIGHLIGHT);
}

const char *Platform::DefaultFont() {
	return "Verdana";
}

int Platform::DefaultFontSize() {
	return 8;
}

unsigned int Platform::DoubleClickTime() {
	return ::GetDoubleClickTime();
}

void Platform::DebugDisplay(const char *s) noexcept {
	::OutputDebugStringA(s);
}

//#define TRACE

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) noexcept {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, std::size(buffer), format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) noexcept {
}
#endif

namespace {

bool assertionPopUps = true;

}

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) noexcept {
	const bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) noexcept {
	char buffer[2000] {};
	snprintf(buffer, std::size(buffer), "Assertion [%s] failed at %s %d%s", c, file, line, assertionPopUps ? "" : "\r\n");
	if (assertionPopUps) {
		const int idButton = ::MessageBoxA({}, buffer, "Assertion failure",
			MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
		if (idButton == IDRETRY) {
			::DebugBreak();
		} else if (idButton == IDIGNORE) {
			// all OK
		} else {
			abort();
		}
	} else {
		Platform::DebugDisplay(buffer);
		::DebugBreak();
		abort();
	}
}

void Platform_Initialise(void *hInstance) noexcept {
	hinstPlatformRes = static_cast<HINSTANCE>(hInstance);
	LoadDpiForWindow();
	ListBoxX_Register();
}

void Platform_Finalise(bool fromDllMain) noexcept {
	if (!fromDllMain) {
#if defined(USE_D2D)
		ReleaseD2D();
#endif
		ReleaseLibrary(hDLLShcore);
	}
	ListBoxX_Unregister();
}

}
