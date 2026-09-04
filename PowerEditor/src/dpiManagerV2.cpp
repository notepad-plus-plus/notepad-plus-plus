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


#include "dpiManagerV2.h"

#include <windows.h>

#include <commctrl.h>

namespace NppDarkMode
{
	bool isWindows10();
}

template <typename P>
inline auto LoadFn(HMODULE handle, P& pointer, const char* name) noexcept -> bool
{
	if (auto* proc = ::GetProcAddress(handle, name); proc != nullptr)
	{
		pointer = reinterpret_cast<P>(reinterpret_cast<INT_PTR>(proc));
		return true;
	}
	return false;
}

[[nodiscard]] static UINT WINAPI DummyGetDpiForSystem()
{
	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	if (HDC hdc = ::GetDC(nullptr); hdc != nullptr)
	{
		dpi = static_cast<UINT>(::GetDeviceCaps(hdc, LOGPIXELSX));
		::ReleaseDC(nullptr, hdc);
	}
	return dpi;
}

[[nodiscard]] static UINT WINAPI DummyGetDpiForWindow([[maybe_unused]] HWND hwnd)
{
	return DummyGetDpiForSystem();
}

[[nodiscard]] static int WINAPI DummyGetSystemMetricsForDpi(int nIndex, UINT dpi)
{
	return DPIManagerV2::scale(::GetSystemMetrics(nIndex), dpi);
}

[[nodiscard]] static BOOL WINAPI DummySystemParametersInfoForDpi(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, [[maybe_unused]] UINT dpi)
{
	return ::SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}

[[nodiscard]] static BOOL WINAPI DummyIsValidDpiAwarenessContext([[maybe_unused]] DPI_AWARENESS_CONTEXT value)
{
	return FALSE;
}

[[nodiscard]] static DPI_AWARENESS_CONTEXT WINAPI DummySetThreadDpiAwarenessContext([[maybe_unused]] DPI_AWARENESS_CONTEXT dpiContext)
{
	return nullptr;
}

static BOOL WINAPI DummyAdjustWindowRectExForDpi(
	[[maybe_unused]] LPRECT lpRect,
	[[maybe_unused]] DWORD dwStyle,
	[[maybe_unused]] BOOL bMenu,
	[[maybe_unused]] DWORD dwExStyle,
	[[maybe_unused]] UINT dpi
)
{
	return FALSE;
}

using fnGetDpiForSystem = UINT (WINAPI*)(VOID);
using fnGetDpiForWindow = UINT (WINAPI*)(HWND hwnd);
using fnGetSystemMetricsForDpi = int (WINAPI*)(int nIndex, UINT dpi);
using fnSystemParametersInfoForDpi = BOOL (WINAPI*)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
using fnIsValidDpiAwarenessContext = BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT value);
using fnSetThreadDpiAwarenessContext = DPI_AWARENESS_CONTEXT (WINAPI*)(DPI_AWARENESS_CONTEXT dpiContext);
using fnAdjustWindowRectExForDpi = BOOL (WINAPI*)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static fnGetDpiForSystem _fnGetDpiForSystem = DummyGetDpiForSystem;
static fnGetDpiForWindow _fnGetDpiForWindow = DummyGetDpiForWindow;
static fnGetSystemMetricsForDpi _fnGetSystemMetricsForDpi = DummyGetSystemMetricsForDpi;
static fnSystemParametersInfoForDpi _fnSystemParametersInfoForDpi = DummySystemParametersInfoForDpi;
static fnIsValidDpiAwarenessContext _fnIsValidDpiAwarenessContext = DummyIsValidDpiAwarenessContext;
static fnSetThreadDpiAwarenessContext _fnSetThreadDpiAwarenessContext = DummySetThreadDpiAwarenessContext;
static fnAdjustWindowRectExForDpi _fnAdjustWindowRectExForDpi = DummyAdjustWindowRectExForDpi;

void DPIManagerV2::initDpiAPI()
{
	if (NppDarkMode::isWindows10())
	{
		HMODULE hUser32 = ::GetModuleHandleW(L"user32.dll");
		if (hUser32 != nullptr)
		{
			LoadFn(hUser32, _fnGetDpiForSystem, "GetDpiForSystem");
			LoadFn(hUser32, _fnGetDpiForWindow, "GetDpiForWindow");
			LoadFn(hUser32, _fnGetSystemMetricsForDpi, "GetSystemMetricsForDpi");
			LoadFn(hUser32, _fnSystemParametersInfoForDpi, "SystemParametersInfoForDpi");
			LoadFn(hUser32, _fnIsValidDpiAwarenessContext, "IsValidDpiAwarenessContext");
			LoadFn(hUser32, _fnSetThreadDpiAwarenessContext, "SetThreadDpiAwarenessContext");
			LoadFn(hUser32, _fnAdjustWindowRectExForDpi, "AdjustWindowRectExForDpi");

		}
	}
}

int DPIManagerV2::getSystemMetricsForDpi(int nIndex, UINT dpi)
{
	return _fnGetSystemMetricsForDpi(nIndex, dpi);
}

bool DPIManagerV2::isValidDpiAwarenessContext(DPI_AWARENESS_CONTEXT value)
{
	return _fnIsValidDpiAwarenessContext(value) == TRUE;
}

DPI_AWARENESS_CONTEXT DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext)
{
	if (DPIManagerV2::isValidDpiAwarenessContext(dpiContext))
	{
		return _fnSetThreadDpiAwarenessContext(dpiContext);
	}
	return nullptr;
}

bool DPIManagerV2::adjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi)
{
	return _fnAdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, dpi) == TRUE;
}

UINT DPIManagerV2::getDpiForSystem()
{
	return _fnGetDpiForSystem();
}

UINT DPIManagerV2::getDpiForWindow(HWND hWnd)
{
	if (hWnd != nullptr)
	{
		if (const auto dpi = _fnGetDpiForWindow(hWnd); dpi > 0)
		{
			return dpi;
		}
	}
	return DPIManagerV2::getDpiForSystem();
}

void DPIManagerV2::setPositionDpi(LPARAM lParam, HWND hWnd, UINT flags)
{
	const auto prcNewWindow = reinterpret_cast<RECT*>(lParam);

	::SetWindowPos(hWnd,
		nullptr,
		prcNewWindow->left,
		prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left,
		prcNewWindow->bottom - prcNewWindow->top,
		flags);
}

LOGFONT DPIManagerV2::getDefaultGUIFontForDpi(UINT dpi, FontType type)
{
	LOGFONT lf{};
	NONCLIENTMETRICS ncm{};
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

	if (_fnSystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dpi) == TRUE)
	{
		switch (type)
		{
			case FontType::menu:
			{
				lf = ncm.lfMenuFont;
				break;
			}

			case FontType::status:
			{
				lf = ncm.lfStatusFont;
				break;
			}

			case FontType::message:
			{
				lf = ncm.lfMessageFont;
				break;
			}

			case FontType::caption:
			{
				lf = ncm.lfCaptionFont;
				break;
			}

			case FontType::smcaption:
			{
				lf = ncm.lfSmCaptionFont;
				break;
			}
		}
	}
	else // should not happen, fallback
	{
		auto* hf = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
		::GetObjectW(hf, sizeof(LOGFONT), &lf);
		lf.lfHeight = scaleFont(lf.lfHeight, dpi);
	}

	return lf;
}

void DPIManagerV2::loadIcon(HINSTANCE hinst, const wchar_t* pszName, int cx, int cy, HICON* phico, UINT fuLoad)
{
	if (::LoadIconWithScaleDown(hinst, pszName, cx, cy, phico) != S_OK)
	{
		*phico = static_cast<HICON>(::LoadImage(hinst, pszName, IMAGE_ICON, cx, cy, fuLoad));
	}
}

DWORD DPIManagerV2::getTextScaleFactor()
{
	static constexpr DWORD defaultVal = 100;
	DWORD data = defaultVal;
	DWORD dwBufSize = sizeof(data);
	static constexpr LPCWSTR lpSubKey = L"Software\\Microsoft\\Accessibility";
	static constexpr LPCWSTR lpValue = L"TextScaleFactor";

	if (::RegGetValueW(HKEY_CURRENT_USER, lpSubKey, lpValue, RRF_RT_REG_DWORD, nullptr, &data, &dwBufSize) == ERROR_SUCCESS)
	{
		return data;
	}
	return defaultVal;
}
