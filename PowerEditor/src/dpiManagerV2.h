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

class DPIManagerV2
{
public:
	DPIManagerV2()
	{
		setDpiWithSystem();
	}
	virtual ~DPIManagerV2() = default;

	enum class FontType { menu, status, message, caption, smcaption };

	static UINT getDpiForSystem()
	{
		if (NppDarkMode::isWindows10())
		{
			return ::GetDpiForSystem();
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

	static UINT getDpiForWindow(HWND hWnd)
	{
		if (NppDarkMode::isWindows10())
		{
			const auto dpi = ::GetDpiForWindow(hWnd);
			if (dpi > 0)
			{
				return dpi;
			}
		}
		return getDpiForSystem();
	}

	static UINT getDpiForParent(HWND hWnd)
	{
		return getDpiForWindow(::GetParent(hWnd));
	}

	void setDpiWithSystem()
	{
		_dpi = getDpiForSystem();
	}

	void setDpi(WPARAM wParam)
	{
		_dpi = LOWORD(wParam);
	}

	void setDpiValue(UINT newDpi)
	{
		_dpi = newDpi;
	}

	void setDpi(HWND hWnd)
	{
		if (NppDarkMode::isWindows10())
		{
			setDpiValue(getDpiForWindow(hWnd));
		}
		else
		{
			setDpiWithSystem();
		}
	}

	void setDpiWithParent(HWND hWnd)
	{
		setDpi(::GetParent(hWnd));
	}

	UINT getDpi() const { return _dpi; }

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

	static int scaleFont(int pt, UINT dpi) { return -(scale(pt, dpi, 72)); }
	int scaleFont(int pt) { return scaleFont(pt, _dpi); }

	static LOGFONT getDefaultGUIFontForDpi(UINT dpi, FontType type = FontType::message)
	{
		int result = 0;
		LOGFONT lf{};
		NONCLIENTMETRICS ncm{};
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		if (NppDarkMode::isWindows10()
			&& (::SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dpi) != FALSE))
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

	static LOGFONT getDefaultGUIFontForDpi(HWND hWnd, FontType type = FontType::message)
	{
		if (NppDarkMode::isWindows10())
		{
			return getDefaultGUIFontForDpi(getDpiForWindow(hWnd), type);
		}
		return getDefaultGUIFontForDpi(getDpiForSystem(), type);
	}

private:
	UINT _dpi = USER_DEFAULT_SCREEN_DPI;
};
