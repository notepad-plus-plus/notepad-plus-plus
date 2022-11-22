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
	RECT rc = { static_cast<LONG>(prc.left), static_cast<LONG>(prc.top),
		static_cast<LONG>(prc.right), static_cast<LONG>(prc.bottom) };
	return rc;
}

constexpr POINT POINTFromPoint(Point pt) noexcept {
	return POINT{ static_cast<LONG>(pt.x), static_cast<LONG>(pt.y) };
}

constexpr Point PointFromPOINT(POINT pt) noexcept {
	return Point::FromInts(pt.x, pt.y);
}

constexpr HWND HwndFromWindowID(WindowID wid) noexcept {
	return static_cast<HWND>(wid);
}

inline HWND HwndFromWindow(const Window &w) noexcept {
	return HwndFromWindowID(w.GetID());
}

void *PointerFromWindow(HWND hWnd) noexcept;
void SetWindowPointer(HWND hWnd, void *ptr) noexcept;

UINT DpiForWindow(WindowID wid) noexcept;

int SystemMetricsForDpi(int nIndex, UINT dpi) noexcept;

HCURSOR LoadReverseArrowCursor(UINT dpi) noexcept;

#if defined(USE_D2D)
extern bool LoadD2D();
extern ID2D1Factory *pD2DFactory;
extern IDWriteFactory *pIDWriteFactory;

struct RenderingParams {
	std::unique_ptr<IDWriteRenderingParams, UnknownReleaser> defaultRenderingParams;
	std::unique_ptr<IDWriteRenderingParams, UnknownReleaser> customRenderingParams;
};

struct ISetRenderingParams {
	virtual void SetRenderingParams(std::shared_ptr<RenderingParams> renderingParams_) = 0;
};
#endif

}

#endif
