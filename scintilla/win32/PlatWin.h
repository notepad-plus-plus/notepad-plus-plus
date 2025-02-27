// Scintilla source code edit control
/** @file PlatWin.h
 ** Implementation of platform facilities on Windows.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PLATWIN_H
#define PLATWIN_H

namespace Scintilla::Internal {

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI		96
#endif

extern void Platform_Initialise(void *hInstance) noexcept;

extern void Platform_Finalise(bool fromDllMain) noexcept;

constexpr RECT RectFromPRectangle(PRectangle prc) noexcept {
	const RECT rc = { static_cast<LONG>(prc.left), static_cast<LONG>(prc.top),
		static_cast<LONG>(prc.right), static_cast<LONG>(prc.bottom) };
	return rc;
}

constexpr POINT POINTFromPoint(Point pt) noexcept {
	return POINT{ static_cast<LONG>(pt.x), static_cast<LONG>(pt.y) };
}

constexpr Point PointFromPOINT(POINT pt) noexcept {
	return Point::FromInts(pt.x, pt.y);
}

constexpr SIZE SizeOfRect(RECT rc) noexcept {
	return { rc.right - rc.left, rc.bottom - rc.top };
}

constexpr HWND HwndFromWindowID(WindowID wid) noexcept {
	return static_cast<HWND>(wid);
}

inline HWND HwndFromWindow(const Window &w) noexcept {
	return HwndFromWindowID(w.GetID());
}

void *PointerFromWindow(HWND hWnd) noexcept;
void SetWindowPointer(HWND hWnd, void *ptr) noexcept;

HMONITOR MonitorFromWindowHandleScaling(HWND hWnd) noexcept;

UINT DpiForWindow(WindowID wid) noexcept;
float GetDeviceScaleFactorWhenGdiScalingActive(HWND hWnd) noexcept;

int SystemMetricsForDpi(int nIndex, UINT dpi) noexcept;

HCURSOR LoadReverseArrowCursor(UINT dpi) noexcept;

class MouseWheelDelta {
	int wheelDelta = 0;
public:
	bool Accumulate(WPARAM wParam) noexcept {
		wheelDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
		return std::abs(wheelDelta) >= WHEEL_DELTA;
	}
	int Actions() noexcept {
		const int actions = wheelDelta / WHEEL_DELTA;
		wheelDelta = wheelDelta % WHEEL_DELTA;
		return actions;
	}
};

#if defined(USE_D2D)
extern bool LoadD2D() noexcept;
extern ID2D1Factory1 *pD2DFactory;
extern IDWriteFactory1 *pIDWriteFactory;

using DCRenderTarget = ComPtr<ID2D1DCRenderTarget>;

using D3D11Device = ComPtr<ID3D11Device1>;

HRESULT CreateDCRenderTarget(const D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, DCRenderTarget &dcRT) noexcept;
extern HRESULT CreateD3D(D3D11Device &device) noexcept;

using WriteRenderingParams = ComPtr<IDWriteRenderingParams1>;

struct RenderingParams {
	WriteRenderingParams defaultRenderingParams;
	WriteRenderingParams customRenderingParams;
};

struct ISetRenderingParams {
	virtual void SetRenderingParams(std::shared_ptr<RenderingParams> renderingParams_) = 0;
};
#endif

}

#endif
