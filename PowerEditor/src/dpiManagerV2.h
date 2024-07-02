// This file is part of Notepad++ project
// Copyright (c) 2024 ozone10 and Notepad++ team

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#pragma once
#include "NppDarkMode.h"

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02E4
#endif

class DPIManagerV2
{
public:
	DPIManagerV2() {
		setDpiWithSystem();
	}
	virtual ~DPIManagerV2() = default;

	enum class FontType { menu, status, message, caption, smcaption };

	static void initDpiAPI();

	static int getSystemMetricsForDpi(int nIndex, UINT dpi);
	int getSystemMetricsForDpi(int nIndex) const {
		return getSystemMetricsForDpi(nIndex, _dpi);
	}
	static DPI_AWARENESS_CONTEXT setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext);
	static BOOL adjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);


	static UINT getDpiForSystem();
	static UINT getDpiForWindow(HWND hWnd);
	static UINT getDpiForParent(HWND hWnd) {
		return getDpiForWindow(::GetParent(hWnd));
	}

	void setDpiWithSystem() {
		_dpi = getDpiForSystem();
	}

	// parameter is WPARAM
	void setDpiWP(WPARAM wParam) {
		_dpi = LOWORD(wParam);
	}

	void setDpi(UINT newDpi) {
		_dpi = newDpi;
	}

	void setDpi(HWND hWnd) {
		setDpi(getDpiForWindow(hWnd));
	}

	void setDpiWithParent(HWND hWnd) {
		setDpi(::GetParent(hWnd));
	}

	UINT getDpi() const {
		return _dpi;
	}

	static void setPositionDpi(LPARAM lParam, HWND hWnd, UINT flags = SWP_NOZORDER | SWP_NOACTIVATE);

	static int scale(int x, UINT toDpi, UINT fromDpi) {
		return MulDiv(x, toDpi, fromDpi);
	}

	static int scale(int x, UINT dpi) {
		return scale(x, dpi, USER_DEFAULT_SCREEN_DPI);
	}

	static int unscale(int x, UINT dpi) {
		return scale(x, USER_DEFAULT_SCREEN_DPI, dpi);
	}

	static int scale(int x, HWND hWnd) {
		return scale(x, getDpiForWindow(hWnd), USER_DEFAULT_SCREEN_DPI);
	}

	static int unscale(int x, HWND hWnd) {
		return scale(x, USER_DEFAULT_SCREEN_DPI, getDpiForWindow(hWnd));
	}

	int scale(int x) const {
		return scale(x, _dpi);
	}

	int unscale(int x) const {
		return unscale(x, _dpi);
	}

	static int scaleFont(int pt, UINT dpi) {
		return -(scale(pt, dpi, 72));
	}

	static int scaleFont(int pt, HWND hWnd) {
		return -(scale(pt, getDpiForWindow(hWnd), 72));
	}

	int scaleFont(int pt) const {
		return scaleFont(pt, _dpi);
	}

	static LOGFONT getDefaultGUIFontForDpi(UINT dpi, FontType type = FontType::message);
	static LOGFONT getDefaultGUIFontForDpi(HWND hWnd, FontType type = FontType::message) {
		return getDefaultGUIFontForDpi(getDpiForWindow(hWnd), type);
	}
	LOGFONT getDefaultGUIFontForDpi(FontType type = FontType::message) const {
		return getDefaultGUIFontForDpi(_dpi, type);
	}

	static void loadIcon(HINSTANCE hinst, const wchar_t* pszName, int cx, int cy, HICON* phico, UINT fuLoad = LR_DEFAULTCOLOR);

private:
	UINT _dpi = USER_DEFAULT_SCREEN_DPI;
};
