// This file is part of Notepad++ project
// Copyright (c) 2023 ozone10 and Notepad++ team

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

class DPIManagerV2
{
public:
	DPIManagerV2()
	{
		setDpiSystem();
	}
	virtual ~DPIManagerV2() = default;

	enum class FontType { none, menu, status, message };

	static UINT getDpiSystem()
	{
		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		HDC hdc = ::GetDC(nullptr);
		if (hdc != nullptr)
		{
			dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
			//dpiY = ::GetDeviceCaps(hdc, LOGPIXELSY);
			::ReleaseDC(nullptr, hdc);
		}
		return dpi;
	}

	static UINT getDpiFromWindow(HWND hWnd)
	{
		if (NppDarkMode::isWindows10())
		{
			return ::GetDpiForWindow(hWnd);
		}
		return getDpiSystem();
	}

	static UINT getDpiFromParent(HWND hWnd)
	{
		return getDpiFromWindow(::GetParent(hWnd));
	}

	void setDpiSystem()
	{
		setDpiPrev();
		_dpi = getDpiSystem();
	}

	void setDpi(WPARAM wParam)
	{
		setDpiPrev();
		_dpi = LOWORD(wParam);
		// _dpiY = HIWORD(wParam);
	}

	void setDpiValue(UINT newDpi)
	{
		setDpiPrev();
		_dpi = newDpi;
	}

	void setDpi(HWND hWnd)
	{
		if (NppDarkMode::isWindows10())
		{
			setDpiPrev();
			_dpi = ::GetDpiForWindow(hWnd);
		}
		else
		{
			setDpiSystem();
		}
	}

	UINT getDpi() { return _dpi; }
	UINT getDpiX() { return getDpi(); }
	UINT getDpiY() { return getDpi(); }

	static void setPositionDpi(LPARAM lParam, HWND hWnd)
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

	static int scale(int x, UINT dpi, UINT dpi2) { return MulDiv(x, dpi, dpi2); }
	static int scale(int x, UINT dpi) { return scale(x, dpi, USER_DEFAULT_SCREEN_DPI); }
	static int unscale(int x, UINT dpi) { return scale(x, USER_DEFAULT_SCREEN_DPI, dpi); }

	int scale(int x) { return scale(x, _dpi); }
	int unscale(int x) { return unscale(x, _dpi); }

	int scaleX(int x) { return scale(x); }
	int unscaleX(int x) { return unscale(x); }

	int scaleY(int y) { return scale(y); }
	int unscaleY(int y) { return unscale(y); }

	int scaleWithPrev(int x) { return scale(x, _dpi, _dpiPrev); }

	static int scaleFont(int pt, UINT dpi) { return -(scale(pt, dpi, 72)); }
	int scaleFont(int pt) { return scaleFont(pt, _dpi); }

	static LOGFONT getFontDpi(UINT dpi, FontType type = FontType::message)
	{
		int result = 0;
		LOGFONT lf{};

		if (type != FontType::none)
		{
			NONCLIENTMETRICS nonClientMetrics{};
			nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
			if (NppDarkMode::isWindows10()
				&& (::SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0, dpi) != FALSE))
			{
				result = 1;
			}
			else if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0) != FALSE)
			{
				result = 2;
			}

			if (result > 0)
			{
				switch (type)
				{
					case FontType::menu:
					{
						lf = nonClientMetrics.lfMenuFont;
						break;
					}
					case FontType::status:
					{
						lf = nonClientMetrics.lfStatusFont;
						break;
					}
					case FontType::message:
					{
						lf = nonClientMetrics.lfMessageFont;
						break;
					}
					// case FontType::none: should never happen
					default:
					{
						auto hf = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
						::GetObject(hf, sizeof(LOGFONT), &lf);
						break;
					}
				}
			}
		}
		else
		{
			auto hf = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
			::GetObject(hf, sizeof(LOGFONT), &lf);
		}

		if (result != 1)
		{
			lf.lfHeight = scaleFont(lf.lfHeight, dpi);
		}

		return lf;
	}

	static LOGFONT getFontDpi(HWND hWnd, FontType type = FontType::message)
	{
		if (NppDarkMode::isWindows10())
		{
			return getFontDpi(::GetDpiForWindow(hWnd), type);
		}

		return getFontDpi(getDpiSystem(), type);
	}

private:
	void setDpiPrev()
	{
		_dpiPrev = _dpi;
	}

	UINT _dpi = USER_DEFAULT_SCREEN_DPI;
	UINT _dpiPrev = USER_DEFAULT_SCREEN_DPI;
};
