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

template <typename P>
bool ptrFn(HMODULE handle, P& pointer, const char* name)
{
	auto p = reinterpret_cast<P>(::GetProcAddress(handle, name));
	if (p != nullptr)
	{
		pointer = p;
		return true;
	}
	return false;
}

using fnGetDpiForSystem = UINT (WINAPI*)(VOID);
using fnGetDpiForWindow = UINT (WINAPI*)(HWND hwnd);
using fnGetSystemMetricsForDpi = int (WINAPI*)(int nIndex, UINT dpi);
using fnSystemParametersInfoForDpi = BOOL (WINAPI*)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
using fnSetThreadDpiAwarenessContext = DPI_AWARENESS_CONTEXT (WINAPI*)(DPI_AWARENESS_CONTEXT dpiContext);

fnGetDpiForSystem _fnGetDpiForSystem = nullptr;
fnGetDpiForWindow _fnGetDpiForWindow = nullptr;
fnGetSystemMetricsForDpi _fnGetSystemMetricsForDpi = nullptr;
fnSystemParametersInfoForDpi _fnSystemParametersInfoForDpi = nullptr;
fnSetThreadDpiAwarenessContext _fnSetThreadDpiAwarenessContext = nullptr;

void DPIManagerV2::initDpiAPI()
{
	if (NppDarkMode::isWindows10())
	{
		HMODULE hUser32 = ::GetModuleHandleW(L"user32.dll");
		if (hUser32 != nullptr)
		{
			ptrFn(hUser32, _fnGetDpiForSystem, "GetDpiForSystem");
			ptrFn(hUser32, _fnGetDpiForWindow, "GetDpiForWindow");
			ptrFn(hUser32, _fnGetSystemMetricsForDpi, "GetSystemMetricsForDpi");
			ptrFn(hUser32, _fnSystemParametersInfoForDpi, "SystemParametersInfoForDpi");
			ptrFn(hUser32, _fnSetThreadDpiAwarenessContext, "SetThreadDpiAwarenessContext");
		}
	}
}

int DPIManagerV2::getSystemMetricsForDpi(int nIndex, UINT dpi)
{
	if (_fnGetSystemMetricsForDpi != nullptr)
	{
		return _fnGetSystemMetricsForDpi(nIndex, dpi);
	}
	return DPIManagerV2::scale(::GetSystemMetrics(nIndex), dpi);
}

DPI_AWARENESS_CONTEXT DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext)
{
	if (_fnSetThreadDpiAwarenessContext != nullptr)
	{
		return _fnSetThreadDpiAwarenessContext(dpiContext);
	}
	return NULL;
}

UINT DPIManagerV2::getDpiForSystem()
{
	if (_fnGetDpiForSystem != nullptr)
	{
		return _fnGetDpiForSystem();
	}

	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	HDC hdc = ::GetDC(nullptr);
	if (hdc != nullptr)
	{
		dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
		::ReleaseDC(nullptr, hdc);
	}
	return dpi;
}

UINT DPIManagerV2::getDpiForWindow(HWND hWnd)
{
	if (_fnGetDpiForWindow != nullptr)
	{
		const auto dpi = _fnGetDpiForWindow(hWnd);
		if (dpi > 0)
		{
			return dpi;
		}
	}
	return getDpiForSystem();
}

void DPIManagerV2::setPositionDpi(LPARAM lParam, HWND hWnd)
{
	const auto prcNewWindow = reinterpret_cast<RECT*>(lParam);

	::SetWindowPos(hWnd,
		nullptr,
		prcNewWindow->left,
		prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left,
		prcNewWindow->bottom - prcNewWindow->top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

LOGFONT DPIManagerV2::getDefaultGUIFontForDpi(UINT dpi, FontType type)
{
	int result = 0;
	LOGFONT lf{};
	NONCLIENTMETRICS ncm{};
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (_fnSystemParametersInfoForDpi != nullptr
		&& (_fnSystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dpi) != FALSE))
	{
		result = 2;
	}
	else if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0) != FALSE)
	{
		result = 1;
	}

	if (result > 0)
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
			//case FontType::message:
			default:
			{
				lf = ncm.lfMessageFont;
				break;
			}
		}
	}
	else // should not happen, fallback
	{
		auto hf = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
		::GetObject(hf, sizeof(LOGFONT), &lf);
	}

	if (result < 2)
	{
		lf.lfHeight = scaleFont(lf.lfHeight, dpi);
	}

	return lf;
}
