// This file is part of Notepad++ project
// Copyright (C)2021 adzm / Adam D. Walling

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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "NppDarkMode.h"

#include "DarkMode/DarkMode.h"
#include "DarkMode/UAHMenuBar.h"

#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>

#include "Parameters.h"
#include "resource.h"
#include "dpiManagerV2.h"

#include <shlwapi.h>

#include <array>

#ifdef __GNUC__
#include <cmath>
#define WINAPI_LAMBDA WINAPI
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#else
#define WINAPI_LAMBDA
#endif

// already added in dpiManagerV2.h
// keep for plugin authors
//#ifndef WM_DPICHANGED
//#define WM_DPICHANGED 0x02E0
//#endif
//
//#ifndef WM_DPICHANGED_BEFOREPARENT
//#define WM_DPICHANGED_BEFOREPARENT 0x02E2
//#endif
//
//#ifndef WM_DPICHANGED_AFTERPARENT
//#define WM_DPICHANGED_AFTERPARENT 0x02E3
//#endif
//
//#ifndef WM_GETDPISCALEDSIZE
//#define WM_GETDPISCALEDSIZE 0x02E4
//#endif

// already added in project files
// keep for plugin authors
//#ifdef _MSC_VER
//#pragma comment(lib, "dwmapi.lib")
//#pragma comment(lib, "uxtheme.lib")
//#endif

static constexpr COLORREF HEXRGB(DWORD rrggbb) {
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return
		((rrggbb & 0xFF0000) >> 16) |
		((rrggbb & 0x00FF00)) |
		((rrggbb & 0x0000FF) << 16);
}

static std::wstring getWndClassName(HWND hWnd)
{
	constexpr int strLen = 32;
	std::wstring className(strLen, 0);
	className.resize(::GetClassName(hWnd, &className[0], strLen));
	return className;
}

namespace NppDarkMode
{
	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH ctrlBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH dlgBackground = nullptr;
		HBRUSH errorBackground = nullptr;

		HBRUSH edgeBrush = nullptr;
		HBRUSH hotEdgeBrush = nullptr;
		HBRUSH disabledEdgeBrush = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, ctrlBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, dlgBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))

			, edgeBrush(::CreateSolidBrush(colors.edge))
			, hotEdgeBrush(::CreateSolidBrush(colors.hotEdge))
			, disabledEdgeBrush(::CreateSolidBrush(colors.disabledEdge))
		{}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(ctrlBackground);	ctrlBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(dlgBackground);		dlgBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;

			::DeleteObject(edgeBrush);			edgeBrush = nullptr;
			::DeleteObject(hotEdgeBrush);		hotEdgeBrush = nullptr;
			::DeleteObject(disabledEdgeBrush);	disabledEdgeBrush = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(ctrlBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(dlgBackground);
			::DeleteObject(errorBackground);

			::DeleteObject(edgeBrush);
			::DeleteObject(hotEdgeBrush);
			::DeleteObject(disabledEdgeBrush);

			background = ::CreateSolidBrush(colors.background);
			ctrlBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			dlgBackground = ::CreateSolidBrush(colors.pureBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);

			edgeBrush = ::CreateSolidBrush(colors.edge);
			hotEdgeBrush = ::CreateSolidBrush(colors.hotEdge);
			disabledEdgeBrush = ::CreateSolidBrush(colors.disabledEdge);
		}
	};

	struct Pens
	{
		HPEN darkerTextPen = nullptr;
		HPEN edgePen = nullptr;
		HPEN hotEdgePen = nullptr;
		HPEN disabledEdgePen = nullptr;

		Pens(const Colors& colors)
			: darkerTextPen(::CreatePen(PS_SOLID, 1, colors.darkerText))
			, edgePen(::CreatePen(PS_SOLID, 1, colors.edge))
			, hotEdgePen(::CreatePen(PS_SOLID, 1, colors.hotEdge))
			, disabledEdgePen(::CreatePen(PS_SOLID, 1, colors.disabledEdge))
		{}

		~Pens()
		{
			::DeleteObject(darkerTextPen);		darkerTextPen = nullptr;
			::DeleteObject(edgePen);			edgePen = nullptr;
			::DeleteObject(hotEdgePen);			hotEdgePen = nullptr;
			::DeleteObject(disabledEdgePen);	disabledEdgePen = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(darkerTextPen);
			::DeleteObject(edgePen);
			::DeleteObject(hotEdgePen);
			::DeleteObject(disabledEdgePen);

			darkerTextPen = ::CreatePen(PS_SOLID, 1, colors.darkerText);
			edgePen = ::CreatePen(PS_SOLID, 1, colors.edge);
			hotEdgePen = ::CreatePen(PS_SOLID, 1, colors.hotEdge);
			disabledEdgePen = ::CreatePen(PS_SOLID, 1, colors.disabledEdge);
		}

	};

	// black (default)
	static const Colors darkColors{
		HEXRGB(0x202020),   // background
		HEXRGB(0x383838),   // softerBackground
		HEXRGB(0x454545),   // hotBackground
		HEXRGB(0x202020),   // pureBackground
		HEXRGB(0xB00000),   // errorBackground
		HEXRGB(0xE0E0E0),   // textColor
		HEXRGB(0xC0C0C0),   // darkerTextColor
		HEXRGB(0x808080),   // disabledTextColor
		HEXRGB(0xFFFF00),   // linkTextColor
		HEXRGB(0x646464),   // edgeColor
		HEXRGB(0x9B9B9B),   // hotEdgeColor
		HEXRGB(0x484848)    // disabledEdgeColor
	};

	constexpr int offsetEdge = HEXRGB(0x1C1C1C);

	// red tone
	constexpr int offsetRed = HEXRGB(0x100000);
	static const Colors darkRedColors{
		darkColors.background + offsetRed,
		darkColors.softerBackground + offsetRed,
		darkColors.hotBackground + offsetRed,
		darkColors.pureBackground + offsetRed,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetRed,
		darkColors.hotEdge + offsetRed,
		darkColors.disabledEdge + offsetRed
	};

	// green tone
	constexpr int offsetGreen = HEXRGB(0x001000);
	static const Colors darkGreenColors{
		darkColors.background + offsetGreen,
		darkColors.softerBackground + offsetGreen,
		darkColors.hotBackground + offsetGreen,
		darkColors.pureBackground + offsetGreen,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetGreen,
		darkColors.hotEdge + offsetGreen,
		darkColors.disabledEdge + offsetGreen
	};

	// blue tone
	constexpr int offsetBlue = HEXRGB(0x000020);
	static const Colors darkBlueColors{
		darkColors.background + offsetBlue,
		darkColors.softerBackground + offsetBlue,
		darkColors.hotBackground + offsetBlue,
		darkColors.pureBackground + offsetBlue,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetBlue,
		darkColors.hotEdge + offsetBlue,
		darkColors.disabledEdge + offsetBlue
	};

	// purple tone
	constexpr int offsetPurple = HEXRGB(0x100020);
	static const Colors darkPurpleColors{
		darkColors.background + offsetPurple,
		darkColors.softerBackground + offsetPurple,
		darkColors.hotBackground + offsetPurple,
		darkColors.pureBackground + offsetPurple,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetPurple,
		darkColors.hotEdge + offsetPurple,
		darkColors.disabledEdge + offsetPurple
	};

	// cyan tone
	constexpr int offsetCyan = HEXRGB(0x001020);
	static const Colors darkCyanColors{
		darkColors.background + offsetCyan,
		darkColors.softerBackground + offsetCyan,
		darkColors.hotBackground + offsetCyan,
		darkColors.pureBackground + offsetCyan,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetCyan,
		darkColors.hotEdge + offsetCyan,
		darkColors.disabledEdge + offsetCyan
	};

	// olive tone
	constexpr int offsetOlive = HEXRGB(0x101000);
	static const Colors darkOliveColors{
		darkColors.background + offsetOlive,
		darkColors.softerBackground + offsetOlive,
		darkColors.hotBackground + offsetOlive,
		darkColors.pureBackground + offsetOlive,
		darkColors.errorBackground,
		darkColors.text,
		darkColors.darkerText,
		darkColors.disabledText,
		darkColors.linkText,
		darkColors.edge + offsetEdge + offsetOlive,
		darkColors.hotEdge + offsetOlive,
		darkColors.disabledEdge + offsetOlive
	};

	// customized
	Colors darkCustomizedColors{ darkColors };

	ColorTone g_colorToneChoice = blackTone;

	void setDarkTone(ColorTone colorToneChoice)
	{
		g_colorToneChoice = colorToneChoice;
	}

	struct Theme
	{
		Colors _colors;
		Brushes _brushes;
		Pens _pens;

		Theme(const Colors& colors)
			: _colors(colors)
			, _brushes(colors)
			, _pens(colors)
		{}

		void change(const Colors& colors)
		{
			_colors = colors;
			_brushes.change(colors);
			_pens.change(colors);
		}
	};

	Theme tDefault(darkColors);
	Theme tR(darkRedColors);
	Theme tG(darkGreenColors);
	Theme tB(darkBlueColors);
	Theme tP(darkPurpleColors);
	Theme tC(darkCyanColors);
	Theme tO(darkOliveColors);

	Theme tCustom(darkCustomizedColors);


	static Theme& getTheme()
	{
		switch (g_colorToneChoice)
		{
			case redTone:
				return tR;

			case greenTone:
				return tG;

			case blueTone:
				return tB;

			case purpleTone:
				return tP;

			case cyanTone:
				return tC;

			case oliveTone:
				return tO;

			case customizedTone:
				return tCustom;

			default:
				return tDefault;
		}
	}

	static Options _options;			// actual runtime options
	static ::AdvancedOptions g_advOptions;

	static Options configuredOptions()
	{
		NppGUI nppGui = NppParameters::getInstance().getNppGUI();
		Options opt;
		opt.enable = nppGui._darkmode._isEnabled;
		opt.enableMenubar = opt.enable;
		opt.enablePlugin = nppGui._darkmode._isEnabledPlugin;

		g_colorToneChoice = nppGui._darkmode._colorTone;
		tCustom.change(nppGui._darkmode._customColors);

		return opt;
	}

	constexpr COLORREF cDefaultMainDark = RGB(0xDE, 0xDE, 0xDE);
	constexpr COLORREF cDefaultSecondaryDark = RGB(0x4C, 0xC2, 0xFF);
	constexpr COLORREF cDefaultMainLight = RGB(0x21, 0x21, 0x21);
	constexpr COLORREF cDefaultSecondaryLight = RGB(0x00, 0x78, 0xD4);

	static COLORREF cAccentDark = cDefaultSecondaryDark;
	static COLORREF cAccentLight = cDefaultSecondaryLight;

	static COLORREF adjustClrLightness(COLORREF clr, bool useDark)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		::ColorRGBToHLS(clr, &h, &l, &s);

		constexpr double lightnessThreshold = 50.0 - 3.0;
		if (NppDarkMode::calculatePerceivedLightness(clr) < lightnessThreshold)
		{
			s -= 20;
			l += 50;
			return useDark ? ::ColorHLSToRGB(h, l, s) : clr;
		}
		else
		{
			s += 20;
			l -= 50;
			return useDark ? clr : ::ColorHLSToRGB(h, l, s);
		}
	}

	static bool initAccentColor()
	{
		BOOL opaque = TRUE;
		COLORREF cAccent = 0;

		if (SUCCEEDED(::DwmGetColorizationColor(&cAccent, &opaque)))
		{
			cAccent = RGB(GetBValue(cAccent), GetGValue(cAccent), GetRValue(cAccent));

			cAccentDark = NppDarkMode::adjustClrLightness(cAccent, true);
			cAccentLight = NppDarkMode::adjustClrLightness(cAccent, false);
			return true;
		}

		cAccentDark = cDefaultSecondaryDark;
		cAccentLight = cDefaultSecondaryLight;
		return false;
	}


	static bool g_isAtLeastWindows10 = false;
	static bool g_isWine = false;

	void initDarkMode()
	{
		_options = configuredOptions();

		initExperimentalDarkMode();
		initAdvancedOptions();
		initAccentColor();

		g_isAtLeastWindows10 = NppDarkMode::isWindows10();

		if (!g_isAtLeastWindows10)
		{
			g_advOptions._enableWindowsMode = false;
		}
		else if (NppDarkMode::isWindowsModeEnabled())
		{
			NppParameters& nppParam = NppParameters::getInstance();
			NppGUI& nppGUI = nppParam.getNppGUI();
			nppGUI._darkmode._isEnabled = NppDarkMode::isDarkModeReg() && !IsHighContrast();
			_options.enable = nppGUI._darkmode._isEnabled;
			_options.enableMenubar = _options.enable;
		}

		setDarkMode(_options.enable, true);

		using PWINEGETVERSION = const CHAR* (__cdecl *)(void);

		PWINEGETVERSION pWGV = nullptr;
		auto hNtdllModule = GetModuleHandle(L"ntdll.dll");
		if (hNtdllModule)
		{
			pWGV = reinterpret_cast<PWINEGETVERSION>(GetProcAddress(hNtdllModule, "wine_get_version"));
		}

		g_isWine = pWGV != nullptr;
	}

	// attempts to apply new options from NppParameters, sends NPPM_INTERNAL_REFRESHDARKMODE to hwnd's top level parent
	void refreshDarkMode(HWND hwnd, bool forceRefresh)
	{
		bool supportedChanged = false;

		auto config = configuredOptions();

		if (_options.enable != config.enable)
		{
			supportedChanged = true;
			_options.enable = config.enable;
			setDarkMode(_options.enable, _options.enable);
		}

		if (_options.enableMenubar != config.enableMenubar)
		{
			supportedChanged = true;
			_options.enableMenubar = config.enableMenubar;
		}

		// other options not supported to change at runtime currently

		if (!supportedChanged && !forceRefresh)
		{
			// nothing to refresh, changes were not supported.
			return;
		}

		HWND hwndRoot = GetAncestor(hwnd, GA_ROOTOWNER);

		// wParam == true, will reset style and toolbar icon
		::SendMessage(hwndRoot, NPPM_INTERNAL_REFRESHDARKMODE, static_cast<WPARAM>(!forceRefresh), 0);
	}

	void initAdvancedOptions()
	{
		const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
		g_advOptions = nppGui._darkmode._advOptions;
	}

	bool isEnabled()
	{
		return _options.enable;
	}

	bool isEnabledForPlugins()
	{
		return _options.enablePlugin;
	}

	bool isDarkMenuEnabled()
	{
		return _options.enableMenubar;
	}

	bool isExperimentalActive()
	{
		return g_darkModeEnabled;
	}

	bool isExperimentalSupported()
	{
		return g_darkModeSupported;
	}

	bool isWindowsModeEnabled()
	{
		return g_advOptions._enableWindowsMode;
	}

	void setWindowsMode(bool enable)
	{
		g_advOptions._enableWindowsMode = enable;
	}

	void setThemeName(const std::wstring& newThemeName)
	{
		if (NppDarkMode::isEnabled())
			g_advOptions._darkDefaults._xmlFileName = newThemeName;
		else
			g_advOptions._lightDefaults._xmlFileName = newThemeName;
	}

	std::wstring getThemeName()
	{
		auto& theme = NppDarkMode::isEnabled() ? g_advOptions._darkDefaults._xmlFileName : g_advOptions._lightDefaults._xmlFileName;
		return (lstrcmp(theme.c_str(), L"stylers.xml") == 0) ? L"" : theme;
	}

	TbIconInfo getToolbarIconInfo(bool useDark)
	{
		auto& toolbarInfo = useDark ? g_advOptions._darkDefaults._tbIconInfo
			: g_advOptions._lightDefaults._tbIconInfo;

		if (toolbarInfo._tbCustomColor == 0)
			toolbarInfo._tbCustomColor = NppDarkMode::getAccentColor(useDark);

		return toolbarInfo;
	}

	TbIconInfo getToolbarIconInfo()
	{
		return NppDarkMode::getToolbarIconInfo(NppDarkMode::isEnabled());
	}

	void setToolbarIconSet(int state2Set, bool useDark)
	{
		if (useDark)
			g_advOptions._darkDefaults._tbIconInfo._tbIconSet = static_cast<toolBarStatusType>(state2Set);
		else
			g_advOptions._lightDefaults._tbIconInfo._tbIconSet = static_cast<toolBarStatusType>(state2Set);
	}

	void setToolbarIconSet(int state2Set)
	{
		NppDarkMode::setToolbarIconSet(state2Set, NppDarkMode::isEnabled());
	}

	void setToolbarFluentColor(FluentColor color2Set, bool useDark)
	{
		if (useDark)
			g_advOptions._darkDefaults._tbIconInfo._tbColor = color2Set;
		else
			g_advOptions._lightDefaults._tbIconInfo._tbColor = color2Set;
	}

	void setToolbarFluentColor(FluentColor color2Set)
	{
		NppDarkMode::setToolbarFluentColor(color2Set, NppDarkMode::isEnabled());
	}

	void setToolbarFluentMonochrome(bool setMonochrome, bool useDark)
	{
		if (useDark)
			g_advOptions._darkDefaults._tbIconInfo._tbUseMono = setMonochrome;
		else
			g_advOptions._lightDefaults._tbIconInfo._tbUseMono = setMonochrome;
	}

	void setToolbarFluentMonochrome(bool setMonochrome)
	{
		NppDarkMode::setToolbarFluentMonochrome(setMonochrome, NppDarkMode::isEnabled());
	}
	
	void setToolbarFluentCustomColor(COLORREF color, bool useDark)
	{
		if (useDark)
			g_advOptions._darkDefaults._tbIconInfo._tbCustomColor = color;
		else
			g_advOptions._lightDefaults._tbIconInfo._tbCustomColor = color;
	}

	void setToolbarFluentCustomColor(COLORREF color)
	{
		NppDarkMode::setToolbarFluentCustomColor(color, NppDarkMode::isEnabled());
	}

	void setTabIconSet(bool useAltIcons, bool useDark)
	{
		if (useDark)
			g_advOptions._darkDefaults._tabIconSet = useAltIcons ? 1 : 2;
		else	
			g_advOptions._lightDefaults._tabIconSet = useAltIcons ? 1 : 0;
	}

	int getTabIconSet(bool useDark)
	{
		return useDark ? g_advOptions._darkDefaults._tabIconSet : g_advOptions._lightDefaults._tabIconSet;
	}

	bool useTabTheme()
	{
		return NppDarkMode::isEnabled() ? g_advOptions._darkDefaults._tabUseTheme : g_advOptions._lightDefaults._tabUseTheme;
	}

	void setAdvancedOptions()
	{
		NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
		auto& advOpt = nppGui._darkmode._advOptions;

		advOpt = g_advOptions;
	}

	bool isWindows10()
	{
		return IsWindows10();
	}

	bool isWindows11()
	{
		return IsWindows11();
	}

	DWORD getWindowsBuildNumber()
	{
		return GetWindowsBuildNumber();
	}

	COLORREF invertLightness(COLORREF c)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(c, &h, &l, &s);

		l = 240 - l;

		COLORREF invert_c = ColorHLSToRGB(h, l, s);

		return invert_c;
	}

	static TreeViewStyle g_treeViewStylePrev = TreeViewStyle::classic;
	static TreeViewStyle g_treeViewStyle = TreeViewStyle::classic;
	static COLORREF g_treeViewBg = NppParameters::getInstance().getCurrentDefaultBgColor();
	static double g_lightnessTreeView = 50.0;

	// adapted from https://stackoverflow.com/a/56678483
	double calculatePerceivedLightness(COLORREF c)
	{
		auto linearValue = [](double colorChannel) -> double
		{
			colorChannel /= 255.0;
			if (colorChannel <= 0.04045)
				return colorChannel / 12.92;
			return std::pow(((colorChannel + 0.055) / 1.055), 2.4);
		};

		double r = linearValue(static_cast<double>(GetRValue(c)));
		double g = linearValue(static_cast<double>(GetGValue(c)));
		double b = linearValue(static_cast<double>(GetBValue(c)));

		double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

		double lightness = (luminance <= 216.0 / 24389.0) ? (luminance * 24389.0 / 27.0) : (std::pow(luminance, (1.0 / 3.0)) * 116.0 - 16.0);
		return lightness;
	}

	COLORREF getAccentColor(bool useDark)
	{
		return useDark ? cAccentDark : cAccentLight;
	}

	COLORREF getAccentColor()
	{
		return getAccentColor(NppDarkMode::isEnabled());
	}

	COLORREF getBackgroundColor()         { return getTheme()._colors.background; }
	COLORREF getCtrlBackgroundColor()     { return getTheme()._colors.softerBackground; }
	COLORREF getHotBackgroundColor()      { return getTheme()._colors.hotBackground; }
	COLORREF getDlgBackgroundColor()      { return getTheme()._colors.pureBackground; }
	COLORREF getErrorBackgroundColor()    { return getTheme()._colors.errorBackground; }
	COLORREF getTextColor()               { return getTheme()._colors.text; }
	COLORREF getDarkerTextColor()         { return getTheme()._colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme()._colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme()._colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme()._colors.edge; }
	COLORREF getHotEdgeColor()            { return getTheme()._colors.hotEdge; }
	COLORREF getDisabledEdgeColor()       { return getTheme()._colors.disabledEdge; }

	HBRUSH getBackgroundBrush()           { return getTheme()._brushes.background; }
	HBRUSH getCtrlBackgroundBrush()       { return getTheme()._brushes.ctrlBackground; }
	HBRUSH getHotBackgroundBrush()        { return getTheme()._brushes.hotBackground; }
	HBRUSH getDlgBackgroundBrush()        { return getTheme()._brushes.dlgBackground; }
	HBRUSH getErrorBackgroundBrush()      { return getTheme()._brushes.errorBackground; }

	HBRUSH getEdgeBrush()                 { return getTheme()._brushes.edgeBrush; }
	HBRUSH getHotEdgeBrush()              { return getTheme()._brushes.hotEdgeBrush; }
	HBRUSH getDisabledEdgeBrush()         { return getTheme()._brushes.disabledEdgeBrush; }

	HPEN getDarkerTextPen()               { return getTheme()._pens.darkerTextPen; }
	HPEN getEdgePen()                     { return getTheme()._pens.edgePen; }
	HPEN getHotEdgePen()                  { return getTheme()._pens.hotEdgePen; }
	HPEN getDisabledEdgePen()             { return getTheme()._pens.disabledEdgePen; }

	void setBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.background = c;
		getTheme().change(clrs);
	}

	void setCtrlBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.softerBackground = c;
		getTheme().change(clrs);
	}

	void setHotBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.hotBackground = c;
		getTheme().change(clrs);
	}

	void setDlgBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.pureBackground = c;
		getTheme().change(clrs);
	}

	void setErrorBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.errorBackground = c;
		getTheme().change(clrs);
	}

	void setTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.text = c;
		getTheme().change(clrs);
	}

	void setDarkerTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.darkerText = c;
		getTheme().change(clrs);
	}

	void setDisabledTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.disabledText = c;
		getTheme().change(clrs);
	}

	void setLinkTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.linkText = c;
		getTheme().change(clrs);
	}

	void setEdgeColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.edge = c;
		getTheme().change(clrs);
	}

	void setHotEdgeColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.hotEdge = c;
		getTheme().change(clrs);
	}

	void setDisabledEdgeColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.disabledEdge = c;
		getTheme().change(clrs);
	}

	Colors getDarkModeDefaultColors(ColorTone colorTone)
	{
		switch (colorTone)
		{
			case NppDarkMode::ColorTone::redTone:
				return darkRedColors;

			case NppDarkMode::ColorTone::greenTone:
				return darkGreenColors;

			case NppDarkMode::ColorTone::blueTone:
				return darkBlueColors;

			case NppDarkMode::ColorTone::purpleTone:
				return darkPurpleColors;

			case NppDarkMode::ColorTone::cyanTone:
				return darkCyanColors;

			case NppDarkMode::ColorTone::oliveTone:
				return darkOliveColors;

			case NppDarkMode::ColorTone::customizedTone:
			case NppDarkMode::ColorTone::blackTone:
			default:
				return darkColors;
		}
	}


	void changeCustomTheme(const Colors& colors)
	{
		tCustom.change(colors);
	}

	// handle events

	void handleSettingChange(HWND hwnd, LPARAM lParam, bool isFromBtn)
	{
		UNREFERENCED_PARAMETER(hwnd);

		if (!isExperimentalSupported())
		{
			return;
		}

		if (IsColorSchemeChangeMessage(lParam) || isFromBtn)
		{
			// ShouldAppsUseDarkMode() is not reliable from 1903+, use NppDarkMode::isDarkModeReg() instead
			g_darkModeEnabled = NppDarkMode::isDarkModeReg() && !IsHighContrast();
		}
	}

	bool isDarkModeReg()
	{
		DWORD data{};
		DWORD dwBufSize = sizeof(data);
		LPCTSTR lpSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
		LPCTSTR lpValue = L"AppsUseLightTheme";

		auto result = RegGetValue(HKEY_CURRENT_USER, lpSubKey, lpValue, RRF_RT_REG_DWORD, nullptr, &data, &dwBufSize);
		if (result != ERROR_SUCCESS)
		{
			return false;
		}

		// dark mode is 0, light mode is 1
		return data == 0UL;
	}

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM /*wParam*/, LPARAM lParam, LRESULT* lr)
	{
		static HTHEME g_menuTheme = nullptr;

		switch (message)
		{
		case WM_UAHDRAWMENU:
		{
			auto pUDM = reinterpret_cast<UAHMENU*>(lParam);
			RECT rc{};

			// get the menubar rect
			{
				MENUBARINFO mbi{};
				mbi.cbSize = sizeof(MENUBARINFO);
				GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

				RECT rcWindow{};
				GetWindowRect(hWnd, &rcWindow);

				// the rcBar is offset by the window rect
				rc = mbi.rcBar;
				OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

				rc.top -= 1;
			}

			FillRect(pUDM->hdc, &rc, NppDarkMode::getDlgBackgroundBrush());

			*lr = 0;

			return true;
		}

		case WM_UAHDRAWMENUITEM:
		{
			auto pUDMI = reinterpret_cast<UAHDRAWMENUITEM*>(lParam);

			// get the menu item string
			wchar_t menuString[256] = { '\0' };
			MENUITEMINFO mii{};
			{
				mii.cbSize = sizeof(MENUITEMINFO);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = menuString;
				mii.cch = (sizeof(menuString) / 2) - 1;

				GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
			}

			// get the item state for drawing

			DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

			int iTextStateID = MBI_NORMAL;
			int iBackgroundStateID = MBI_NORMAL;
			{
				if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT))
				{
					// normal display
					iTextStateID = MBI_NORMAL;
					iBackgroundStateID = MBI_NORMAL;
				}
				if (pUDMI->dis.itemState & ODS_HOTLIGHT)
				{
					// hot tracking
					iTextStateID = MBI_HOT;
					iBackgroundStateID = MBI_HOT;
				}
				if (pUDMI->dis.itemState & ODS_SELECTED)
				{
					// clicked
					iTextStateID = MBI_PUSHED;
					iBackgroundStateID = MBI_PUSHED;
				}
				if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED))
				{
					// disabled / grey text
					iTextStateID = MBI_DISABLED;
					iBackgroundStateID = MBI_DISABLED;
				}
				if (pUDMI->dis.itemState & ODS_NOACCEL)
				{
					dwFlags |= DT_HIDEPREFIX;
				}
			}

			if (!g_menuTheme)
			{
				g_menuTheme = OpenThemeData(hWnd, VSCLASS_MENU);
			}

			switch (iBackgroundStateID)
			{
				case MBI_NORMAL:
				case MBI_DISABLED:
				{
					::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getDlgBackgroundBrush());
					break;
				}

				case MBI_HOT:
				case MBI_DISABLEDHOT:
				{
					::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getHotBackgroundBrush());
					break;
				}

				case MBI_PUSHED:
				case MBI_DISABLEDPUSHED:
				{
					::FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getCtrlBackgroundBrush());
					break;
				}

				default:
				{
					::DrawThemeBackground(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
					break;
				}
			}

			DTTOPTS dttopts{};
			dttopts.dwSize = sizeof(DTTOPTS);
			if (iTextStateID == MBI_NORMAL || iTextStateID == MBI_HOT || iTextStateID == MBI_PUSHED)
			{
				dttopts.dwFlags |= DTT_TEXTCOLOR;
				dttopts.crText = NppDarkMode::getTextColor();
			}
			else if (iTextStateID == MBI_DISABLED || iTextStateID == MBI_DISABLEDHOT || iTextStateID == MBI_DISABLEDPUSHED)
			{
				dttopts.dwFlags |= DTT_TEXTCOLOR;
				dttopts.crText = NppDarkMode::getDisabledTextColor();
			}

			::DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, iTextStateID, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &dttopts);

			*lr = 0;

			return true;
		}

		case WM_DPICHANGED:
		case WM_DPICHANGED_AFTERPARENT:
		case WM_THEMECHANGED:
		{
			if (g_menuTheme)
			{
				CloseThemeData(g_menuTheme);
				g_menuTheme = nullptr;
			}
			// continue processing in main wndproc
			return false;
		}
		default:
			return false;
		}
	}

	void drawUAHMenuNCBottomLine(HWND hWnd)
	{
		MENUBARINFO mbi{};
		mbi.cbSize = sizeof(MENUBARINFO);
		if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
		{
			return;
		}

		RECT rcClient{};
		GetClientRect(hWnd, &rcClient);
		MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

		RECT rcWindow{};
		GetWindowRect(hWnd, &rcWindow);

		OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

		// the rcBar is offset by the window rect
		RECT rcAnnoyingLine = rcClient;
		rcAnnoyingLine.bottom = rcAnnoyingLine.top;
		rcAnnoyingLine.top--;


		HDC hdc = GetWindowDC(hWnd);
		FillRect(hdc, &rcAnnoyingLine, NppDarkMode::getDlgBackgroundBrush());
		ReleaseDC(hWnd, hdc);
	}

	// from DarkMode.h

	void initExperimentalDarkMode()
	{
		::InitDarkMode();
	}

	void setDarkMode(bool useDark, bool fixDarkScrollbar)
	{
		::SetDarkMode(useDark, fixDarkScrollbar);
	}

	void allowDarkModeForApp(bool allow)
	{
		::AllowDarkModeForApp(allow);
	}

	bool allowDarkModeForWindow(HWND hWnd, bool allow)
	{
		return ::AllowDarkModeForWindow(hWnd, allow);
	}

	void setTitleBarThemeColor(HWND hWnd)
	{
		::RefreshTitleBarThemeColor(hWnd);
	}

	void enableDarkScrollBarForWindowAndChildren(HWND hwnd)
	{
		::EnableDarkScrollBarForWindowAndChildren(hwnd);
	}

	void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width, int height)
	{
		auto holdBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
		auto holdPen = ::SelectObject(hdc, hpen);
		::RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, width, height);
		::SelectObject(hdc, holdBrush);
		::SelectObject(hdc, holdPen);
	}

	struct ThemeData
	{
		HTHEME _hTheme = nullptr;
		const wchar_t* _themeClass;

		ThemeData(const wchar_t* themeClass)
			: _themeClass(themeClass)
		{}

		ThemeData()
			: _themeClass(nullptr)
		{}

		~ThemeData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hWnd)
		{
			if (!_hTheme && _themeClass)
			{
				_hTheme = ::OpenThemeData(hWnd, _themeClass);
			}
			return _hTheme != nullptr;
		}

		void closeTheme()
		{
			if (_hTheme)
			{
				::CloseThemeData(_hTheme);
				_hTheme = nullptr;
			}
		}
	};

	struct BufferData
	{
		HDC _hMemDC = nullptr;
		HBITMAP _hMemBmp = nullptr;
		HBITMAP _hOldBmp = nullptr;
		SIZE _szBuffer{};

		BufferData() = default;

		~BufferData()
		{
			releaseBuffer();
		}

		bool ensureBuffer(HDC hdc, const RECT& rcClient)
		{
			int width = rcClient.right - rcClient.left;
			int height = rcClient.bottom - rcClient.top;

			if (_szBuffer.cx != width || _szBuffer.cy != height)
			{
				releaseBuffer();
				_hMemDC = ::CreateCompatibleDC(hdc);
				_hMemBmp = ::CreateCompatibleBitmap(hdc, width, height);
				_hOldBmp = static_cast<HBITMAP>(::SelectObject(_hMemDC, _hMemBmp));
				_szBuffer = { width, height };
			}

			return _hMemDC != nullptr && _hMemBmp != nullptr;
		}

		void releaseBuffer()
		{
			if (_hMemDC)
			{
				::SelectObject(_hMemDC, _hOldBmp);
				::DeleteObject(_hMemBmp);
				::DeleteDC(_hMemDC);

				_hMemDC = nullptr;
				_hMemBmp = nullptr;
				_hOldBmp = nullptr;
				_szBuffer = { 0, 0 };
			}
		}
	};

	struct ButtonData
	{
		HTHEME hTheme = nullptr;
		int iStateID = 0;

		bool isSizeSet = false;
		SIZE szBtn{};

		ButtonData() {};

		// Saves width and height from the resource file for use as restrictions.
		ButtonData(HWND hWnd)
		{
			// Notepad++ doesn't use BS_3STATE, BS_AUTO3STATE and BS_PUSHLIKE buttons.
			const auto nBtnStyle = ::GetWindowLongPtrW(hWnd, GWL_STYLE);
			switch (nBtnStyle & BS_TYPEMASK)
			{
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					if ((nBtnStyle & BS_MULTILINE) != BS_MULTILINE)
					{
						RECT rcBtn{};
						::GetClientRect(hWnd, &rcBtn);
						const UINT dpi = DPIManagerV2::getDpiForParent(hWnd);
						szBtn.cx = DPIManagerV2::unscale(rcBtn.right - rcBtn.left, dpi);
						szBtn.cy = DPIManagerV2::unscale(rcBtn.bottom - rcBtn.top, dpi);
						isSizeSet = (szBtn.cx != 0 && szBtn.cy != 0);
					}
					break;
				}
				default:
					break;
			}
		}

		~ButtonData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, VSCLASS_BUTTON);
			}
			return hTheme != nullptr;
		}

		void closeTheme()
		{
			if (hTheme)
			{
				CloseThemeData(hTheme);
				hTheme = nullptr;
			}
		}
	};

	static void renderButton(HWND hwnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
	{
		RECT rcClient{};
		wchar_t szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		DWORD uiState = static_cast<DWORD>(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0));
		auto nStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE);

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf{};
		if (SUCCEEDED(GetThemeFont(hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont) {
			hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

		DWORD dtFlags = DT_LEFT; // DT_LEFT is 0
		dtFlags |= (nStyle & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
		dtFlags |= ((nStyle & BS_CENTER) == BS_CENTER) ? DT_CENTER : (nStyle & BS_RIGHT) ? DT_RIGHT : 0;
		dtFlags |= ((nStyle & BS_VCENTER) == BS_VCENTER) ? DT_VCENTER : (nStyle & BS_BOTTOM) ? DT_BOTTOM : 0;
		dtFlags |= (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		if (!(nStyle & BS_MULTILINE) && !(nStyle & BS_BOTTOM) && !(nStyle & BS_TOP))
		{
			dtFlags |= DT_VCENTER;
		}

		GetClientRect(hwnd, &rcClient);
		GetWindowText(hwnd, szText, _countof(szText));

		SIZE szBox = { 13, 13 };
		GetThemePartSize(hTheme, hdc, iPartID, iStateID, NULL, TS_DRAW, &szBox);

		RECT rcText = rcClient;
		GetThemeBackgroundContentRect(hTheme, hdc, iPartID, iStateID, &rcClient, &rcText);

		RECT rcBackground = rcClient;
		if (dtFlags & DT_SINGLELINE)
		{
			rcBackground.top += (rcText.bottom - rcText.top - szBox.cy) / 2;
		}
		rcBackground.bottom = rcBackground.top + szBox.cy;
		rcBackground.right = rcBackground.left + szBox.cx;
		rcText.left = rcBackground.right + 3;

		DrawThemeParentBackground(hwnd, hdc, &rcClient);
		DrawThemeBackground(hTheme, hdc, iPartID, iStateID, &rcBackground, nullptr);

		DTTOPTS dtto{};
		dtto.dwSize = sizeof(DTTOPTS);
		dtto.dwFlags = DTT_TEXTCOLOR;
		dtto.crText = NppDarkMode::getTextColor();

		if (nStyle & WS_DISABLED)
		{
			dtto.crText = NppDarkMode::getDisabledTextColor();
		}

		DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if ((nState & BST_FOCUS) && !(uiState & UISF_HIDEFOCUS))
		{
			RECT rcTextOut = rcText;
			dtto.dwFlags |= DTT_CALCRECT;
			DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags | DT_CALCRECT, &rcTextOut, &dtto);
			RECT rcFocus = rcTextOut;
			rcFocus.bottom++;
			rcFocus.left--;
			rcFocus.right++;
			DrawFocusRect(hdc, &rcFocus);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	static void paintButton(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		const auto nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		const auto nButtonStyle = nStyle & BS_TYPEMASK;

		int iPartID = BP_CHECKBOX;

		// Plugin might use BS_3STATE and BS_AUTO3STATE button style
		if (nButtonStyle == BS_CHECKBOX || nButtonStyle == BS_AUTOCHECKBOX || nButtonStyle == BS_3STATE || nButtonStyle == BS_AUTO3STATE)
		{
			iPartID = BP_CHECKBOX;
		}
		else if (nButtonStyle == BS_RADIOBUTTON || nButtonStyle == BS_AUTORADIOBUTTON)
		{
			iPartID = BP_RADIOBUTTON;
		}
		else
		{
			assert(false);
		}

		// states of BP_CHECKBOX and BP_RADIOBUTTON are the same
		int iStateID = RBS_UNCHECKEDNORMAL;

		if (nStyle & WS_DISABLED)		iStateID = RBS_UNCHECKEDDISABLED;
		else if (nState & BST_PUSHED)	iStateID = RBS_UNCHECKEDPRESSED;
		else if (nState & BST_HOT)		iStateID = RBS_UNCHECKEDHOT;

		if (nState & BST_CHECKED)		iStateID += 4;

		if (BufferedPaintRenderAnimation(hwnd, hdc))
		{
			return;
		}

		BP_ANIMATIONPARAMS animParams{};
		animParams.cbSize = sizeof(BP_ANIMATIONPARAMS);
		animParams.style = BPAS_LINEAR;
		if (iStateID != buttonData.iStateID)
		{
			GetThemeTransitionDuration(buttonData.hTheme, iPartID, buttonData.iStateID, iStateID, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		}

		RECT rcClient{};
		GetClientRect(hwnd, &rcClient);

		HDC hdcFrom = nullptr;
		HDC hdcTo = nullptr;
		HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(hwnd, hdc, &rcClient, BPBF_COMPATIBLEBITMAP, nullptr, &animParams, &hdcFrom, &hdcTo);
		if (hbpAnimation)
		{
			if (hdcFrom)
			{
				renderButton(hwnd, hdcFrom, buttonData.hTheme, iPartID, buttonData.iStateID);
			}
			if (hdcTo)
			{
				renderButton(hwnd, hdcTo, buttonData.hTheme, iPartID, iStateID);
			}

			buttonData.iStateID = iStateID;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			renderButton(hwnd, hdc, buttonData.hTheme, iPartID, iStateID);

			buttonData.iStateID = iStateID;
		}
	}

	constexpr UINT_PTR g_buttonSubclassID = 42;

	static LRESULT CALLBACK ButtonSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);

		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
			case WM_UPDATEUISTATE:
				if (HIWORD(wParam) & (UISF_HIDEACCEL | UISF_HIDEFOCUS))
				{
					InvalidateRect(hWnd, nullptr, FALSE);
				}
				break;

			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ButtonSubclass, g_buttonSubclassID);
				delete pButtonData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					return TRUE;
				}
				break;
			}

			case WM_DPICHANGED:
			case WM_DPICHANGED_AFTERPARENT:
			{
				pButtonData->closeTheme();
				[[fallthrough]];
			}
			case WM_SETBUTTONIDEALSIZE:
			{
				if (pButtonData->isSizeSet)
				{
					SIZE szBtn{};
					if (Button_GetIdealSize(hWnd, &szBtn) == TRUE)
					{
						const UINT dpi = DPIManagerV2::getDpiForParent(hWnd);
						const int cx = std::min<LONG>(szBtn.cx, DPIManagerV2::scale(pButtonData->szBtn.cx, dpi));
						const int cy = std::min<LONG>(szBtn.cy, DPIManagerV2::scale(pButtonData->szBtn.cy, dpi));
						::SetWindowPos(hWnd, nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
					}
				}
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pButtonData->closeTheme();
				break;
			}

			case WM_PRINTCLIENT:
			case WM_PAINT:
				if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps{};
					HDC hdc = reinterpret_cast<HDC>(wParam);
					if (!hdc)
					{
						hdc = BeginPaint(hWnd, &ps);
					}

					paintButton(hWnd, hdc, *pButtonData);

					if (ps.hdc)
					{
						EndPaint(hWnd, &ps);
					}

					return 0;
				}
				else
				{
					break;
				}
			case WM_SIZE:
			case WM_DESTROY:
				BufferedPaintStopAllAnimations(hWnd);
				break;
			case WM_ENABLE:
				if (NppDarkMode::isEnabled())
				{
					// skip the button's normal wndproc so it won't redraw out of wm_paint
					LRESULT lr = DefWindowProc(hWnd, uMsg, wParam, lParam);
					InvalidateRect(hWnd, nullptr, FALSE);
					return lr;
				}
				break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassButtonControl(HWND hwnd)
	{
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData(hwnd));
		SetWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID, pButtonData);

		// The following code handles default English localization during Notepad++ launch for button size.
		// For other languages, NativeLangSpeaker::resizeCheckboxRadioBtn will adjust button dimensions.
		const auto nBtnStyle = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
		switch (nBtnStyle & BS_TYPEMASK)
		{
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
			{
				if ((nBtnStyle & BS_MULTILINE) != BS_MULTILINE)
				{
					::SendMessageW(hwnd, NppDarkMode::WM_SETBUTTONIDEALSIZE, 0, 0);
				}
				break;
			}

			default:
				break;
		}
	}

	static void paintGroupbox(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		auto nStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		bool isDisabled = (nStyle & WS_DISABLED) == WS_DISABLED;
		int iPartID = BP_GROUPBOX;
		int iStateID = isDisabled ? GBS_DISABLED : GBS_NORMAL;

		RECT rcClient{};
		GetClientRect(hwnd, &rcClient);

		RECT rcText = rcClient;
		RECT rcBackground = rcClient;

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf{};
		if (SUCCEEDED(GetThemeFont(buttonData.hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont)
		{
			hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(::SelectObject(hdc, hFont));

		wchar_t szText[256] = { '\0' };
		GetWindowText(hwnd, szText, _countof(szText));

		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool isCenter = (style & BS_CENTER) == BS_CENTER;

		if (szText[0])
		{
			SIZE textSize{};
			GetTextExtentPoint32(hdc, szText, static_cast<int>(wcslen(szText)), &textSize);

			int centerPosX = isCenter ? ((rcClient.right - rcClient.left - textSize.cx) / 2) : 7;

			rcBackground.top += textSize.cy / 2;
			rcText.left += centerPosX;
			rcText.bottom = rcText.top + textSize.cy;
			rcText.right = rcText.left + textSize.cx + 4;

			ExcludeClipRect(hdc, rcText.left, rcText.top, rcText.right, rcText.bottom);
		}
		else
		{
			SIZE textSize{};
			GetTextExtentPoint32(hdc, L"M", 1, &textSize);
			rcBackground.top += textSize.cy / 2;
		}

		RECT rcContent = rcBackground;
		GetThemeBackgroundContentRect(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, &rcContent);
		ExcludeClipRect(hdc, rcContent.left, rcContent.top, rcContent.right, rcContent.bottom);

		//DrawThemeParentBackground(hwnd, hdc, &rcClient);
		//DrawThemeBackground(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, nullptr);
		NppDarkMode::paintRoundFrameRect(hdc, rcBackground, NppDarkMode::getEdgePen());

		SelectClipRgn(hdc, nullptr);

		if (szText[0])
		{
			rcText.right -= 2;
			rcText.left += 2;

			DTTOPTS dtto{};
			dtto.dwSize = sizeof(DTTOPTS);
			dtto.dwFlags = DTT_TEXTCOLOR;
			dtto.crText = isDisabled ? NppDarkMode::getDisabledTextColor() : NppDarkMode::getTextColor();

			DWORD textFlags = isCenter ? DT_CENTER : DT_LEFT;

			if(::SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) != static_cast<LRESULT>(NULL))
			{
				textFlags |= DT_HIDEPREFIX;
			}

			DrawThemeTextEx(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, szText, -1, textFlags | DT_SINGLELINE, &rcText, &dtto);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	constexpr UINT_PTR g_groupboxSubclassID = 42;

	static LRESULT CALLBACK GroupboxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, GroupboxSubclass, uIdSubclass);
			delete pButtonData;
			break;

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				return TRUE;
			}
			break;
		}

		case WM_DPICHANGED:
		case WM_DPICHANGED_AFTERPARENT:
		{
			pButtonData->closeTheme();
			return 0;
		}

		case WM_THEMECHANGED:
		{
			pButtonData->closeTheme();
			break;
		}

		case WM_PRINTCLIENT:
		case WM_PAINT:
			if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				PAINTSTRUCT ps{};
				HDC hdc = reinterpret_cast<HDC>(wParam);
				if (!hdc)
				{
					hdc = BeginPaint(hWnd, &ps);
				}

				paintGroupbox(hWnd, hdc, *pButtonData);

				if (ps.hdc)
				{
					EndPaint(hWnd, &ps);
				}

				return 0;
			}
			break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassGroupboxControl(HWND hwnd)
	{
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
		SetWindowSubclass(hwnd, GroupboxSubclass, g_groupboxSubclassID, pButtonData);
	}

	constexpr UINT_PTR g_tabSubclassID = 42;

	static LRESULT CALLBACK TabSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled())
				{
					return TRUE;
				}
				break;
			}

		case WM_PAINT:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			if ((dwStyle & TCS_BUTTONS) || (dwStyle & TCS_VERTICAL))
			{
				break;
			}

			PAINTSTRUCT ps{};
			HDC hdc = ::BeginPaint(hWnd, &ps);
			::FillRect(hdc, &ps.rcPaint, NppDarkMode::getDlgBackgroundBrush());

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
			auto hOldFont = SelectObject(hdc, hFont);

			POINT ptCursor{};
			::GetCursorPos(&ptCursor);
			ScreenToClient(hWnd, &ptCursor);

			int nTabs = TabCtrl_GetItemCount(hWnd);

			int nSelTab = TabCtrl_GetCurSel(hWnd);
			for (int i = 0; i < nTabs; ++i)
			{
				RECT rcItem{};
				TabCtrl_GetItemRect(hWnd, i, &rcItem);
				RECT rcFrame = rcItem;

				RECT rcIntersect{};
				if (IntersectRect(&rcIntersect, &ps.rcPaint, &rcItem))
				{
					bool bHot = PtInRect(&rcItem, ptCursor);
					bool isSelectedTab = (i == nSelTab);

					HRGN hClip = CreateRectRgnIndirect(&rcItem);

					SelectClipRgn(hdc, hClip);

					SetTextColor(hdc, (bHot || isSelectedTab ) ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());

					::InflateRect(&rcItem, -1, -1);
					rcItem.right += 1;

					// for consistency getBackgroundBrush() 
					// would be better, than getCtrlBackgroundBrush(),
					// however default getBackgroundBrush() has same color
					// as getDlgBackgroundBrush()
					::FillRect(hdc, &rcItem, isSelectedTab ? NppDarkMode::getDlgBackgroundBrush() : bHot ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getCtrlBackgroundBrush());

					SetBkMode(hdc, TRANSPARENT);

					wchar_t label[MAX_PATH]{};
					TCITEM tci{};
					tci.mask = TCIF_TEXT;
					tci.pszText = label;
					tci.cchTextMax = MAX_PATH - 1;

					::SendMessage(hWnd, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tci));

					RECT rcText = rcItem;
					if (isSelectedTab)
					{
						::OffsetRect(&rcText, 0, -1);
						::InflateRect(&rcFrame, 0, 1);
					}

					if (i != nTabs - 1)
					{
						rcFrame.right += 1;
					}

					::FrameRect(hdc, &rcFrame, NppDarkMode::getEdgeBrush());

					DrawText(hdc, label, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

					DeleteObject(hClip);

					SelectClipRgn(hdc, holdClip);
				}
			}

			SelectObject(hdc, hOldFont);

			SelectClipRgn(hdc, holdClip);
			if (holdClip)
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			SelectObject(hdc, holdPen);

			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, TabSubclass, uIdSubclass);
			break;
		}

		case WM_PARENTNOTIFY:
		{
			switch (LOWORD(wParam))
			{
				case WM_CREATE:
				{
					auto hwndUpdown = reinterpret_cast<HWND>(lParam);
					if (NppDarkMode::subclassTabUpDownControl(hwndUpdown))
					{
						return 0;
					}
					break;
				}
			}
			return 0;
		}

		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassTabControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, TabSubclass, g_tabSubclassID, 0);
	}

	struct BorderMetricsData
	{
		UINT _dpi = USER_DEFAULT_SCREEN_DPI;
		LONG _xEdge = ::GetSystemMetrics(SM_CXEDGE);
		LONG _yEdge = ::GetSystemMetrics(SM_CYEDGE);
		LONG _xScroll = ::GetSystemMetrics(SM_CXVSCROLL);
		LONG _yScroll = ::GetSystemMetrics(SM_CYVSCROLL);

		BorderMetricsData() {};

		BorderMetricsData(HWND hWnd)
		{
			setMetricsForDpi(DPIManagerV2::getDpiForParent(hWnd));
		}

		void setMetricsForDpi(UINT dpi)
		{
			_dpi = dpi;
			_xEdge = DPIManagerV2::getSystemMetricsForDpi(SM_CXEDGE, _dpi);
			_yEdge = DPIManagerV2::getSystemMetricsForDpi(SM_CYEDGE, _dpi);
			_xScroll = DPIManagerV2::getSystemMetricsForDpi(SM_CXVSCROLL, _dpi);
			_yScroll = DPIManagerV2::getSystemMetricsForDpi(SM_CYVSCROLL, _dpi);
		}
	};

	constexpr UINT_PTR g_customBorderSubclassID = 42;

	static LRESULT CALLBACK CustomBorderSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pBorderMetricsData = reinterpret_cast<BorderMetricsData*>(dwRefData);

		static bool isHotStatic = false;

		switch (uMsg)
		{
			case WM_NCPAINT:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				DefSubclassProc(hWnd, uMsg, wParam, lParam);

				HDC hdc = ::GetWindowDC(hWnd);
				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);
				rcClient.right += (2 * pBorderMetricsData->_xEdge);

				auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				bool hasVerScrollbar = (style & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					rcClient.right += pBorderMetricsData->_xScroll;
				}

				rcClient.bottom += (2 * pBorderMetricsData->_yEdge);

				bool hasHorScrollbar = (style & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					rcClient.bottom += pBorderMetricsData->_yScroll;
				}

				HPEN hPen = ::CreatePen(PS_SOLID, 1, (::IsWindowEnabled(hWnd) == TRUE) ? NppDarkMode::getBackgroundColor() : NppDarkMode::getDlgBackgroundColor());
				RECT rcInner = rcClient;
				::InflateRect(&rcInner, -1, -1);
				NppDarkMode::paintRoundFrameRect(hdc, rcInner, hPen);
				::DeleteObject(hPen);

				bool hasFocus = ::GetFocus() == hWnd;

				POINT ptCursor{};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				bool isHot = ::PtInRect(&rcClient, ptCursor);

				bool isWindowEnabled = ::IsWindowEnabled(hWnd) == TRUE;
				HPEN hEnabledPen = ((isHotStatic && isHot) || hasFocus ? NppDarkMode::getHotEdgePen() : NppDarkMode::getEdgePen());

				NppDarkMode::paintRoundFrameRect(hdc, rcClient, isWindowEnabled ? hEnabledPen : NppDarkMode::getDisabledEdgePen());

				::ReleaseDC(hWnd, hdc);

				return 0;
			}
			break;

			case WM_NCCALCSIZE:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				auto lpRect = reinterpret_cast<LPRECT>(lParam);
				::InflateRect(lpRect, -(pBorderMetricsData->_xEdge), -(pBorderMetricsData->_yEdge));

				auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				bool hasVerScrollbar = (style & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					lpRect->right -= pBorderMetricsData->_xScroll;
				}

				bool hasHorScrollbar = (style & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					lpRect->bottom -= pBorderMetricsData->_yScroll;
				}

				return 0;
			}
			break;

			case WM_DPICHANGED:
			case WM_DPICHANGED_AFTERPARENT:
			{
				pBorderMetricsData->setMetricsForDpi((uMsg == WM_DPICHANGED) ? LOWORD(wParam) : DPIManagerV2::getDpiForParent(hWnd));
				::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return 0;
			}

			case WM_MOUSEMOVE:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				if (::GetFocus() == hWnd)
				{
					break;
				}

				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				TrackMouseEvent(&tme);

				if (!isHotStatic)
				{
					isHotStatic = true;
					::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}
			}
			break;

			case WM_MOUSELEAVE:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				if (isHotStatic)
				{
					isHotStatic = false;
					::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
				}

				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE | TME_CANCEL;
				tme.hwndTrack = hWnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				TrackMouseEvent(&tme);
			}
			break;

			case WM_NCDESTROY:
			{
				RemoveWindowSubclass(hWnd, CustomBorderSubclass, uIdSubclass);
				delete pBorderMetricsData;
			}
			break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassCustomBorderForListBoxAndEditControls(HWND hwnd)
	{
		auto pBorderMetricsData = reinterpret_cast<DWORD_PTR>(new BorderMetricsData(hwnd));
		SetWindowSubclass(hwnd, CustomBorderSubclass, g_customBorderSubclassID, pBorderMetricsData);
	}

	constexpr UINT_PTR g_comboBoxSubclassID = 42;

	struct ComboboxData
	{
		ThemeData _themeData{};
		BufferData _bufferData{};

		LONG_PTR _cbStyle = CBS_SIMPLE;

		ComboboxData()
			: _themeData(VSCLASS_COMBOBOX)
		{}

		ComboboxData(LONG_PTR cbStyle)
			: _themeData(VSCLASS_COMBOBOX)
			, _cbStyle(cbStyle)
		{}

		~ComboboxData() = default;
	};

	static void paintCombobox(HWND hWnd, HDC hdc, ComboboxData& comboboxData)
	{
		auto& themeData = comboboxData._themeData;
		const auto& hTheme = themeData._hTheme;

		const bool hasTheme = themeData.ensureTheme(hWnd) && (NppDarkMode::isExperimentalActive() == NppDarkMode::isEnabled());

		COMBOBOXINFO cbi{};
		cbi.cbSize = sizeof(COMBOBOXINFO);
		::GetComboBoxInfo(hWnd, &cbi);

		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		POINT ptCursor{};
		::GetCursorPos(&ptCursor);
		::ScreenToClient(hWnd, &ptCursor);

		const bool isDisabled = ::IsWindowEnabled(hWnd) == FALSE;
		const bool isHot = ::PtInRect(&rcClient, ptCursor) == TRUE && !isDisabled;

		bool hasFocus = false;

		::SelectObject(hdc, reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0)));
		::SetBkMode(hdc, TRANSPARENT); // for non-theme DrawText

		RECT rcArrow{ cbi.rcButton };
		rcArrow.left -= 1;

		HBRUSH hSelectedBrush = isDisabled ? NppDarkMode::getDlgBackgroundBrush() : (isHot ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getCtrlBackgroundBrush());

		// CBS_DROPDOWN text is handled by parent by WM_CTLCOLOREDIT
		if (comboboxData._cbStyle == CBS_DROPDOWNLIST)
		{
			// erase background on item change
			::FillRect(hdc, &rcClient, hSelectedBrush);

			auto index = static_cast<int>(::SendMessage(hWnd, CB_GETCURSEL, 0, 0));
			if (index != CB_ERR)
			{
				auto bufferLen = static_cast<size_t>(::SendMessage(hWnd, CB_GETLBTEXTLEN, index, 0));
				wchar_t* buffer = new wchar_t[(bufferLen + 1)];
				::SendMessage(hWnd, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));

				RECT rcText{ cbi.rcItem };
				::InflateRect(&rcText, -2, 0);

				constexpr DWORD dtFlags = DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE;
				if (hasTheme)
				{
					DTTOPTS dtto{};
					dtto.dwSize = sizeof(DTTOPTS);
					dtto.dwFlags = DTT_TEXTCOLOR;
					dtto.crText = isDisabled ? NppDarkMode::getDisabledTextColor() : NppDarkMode::getTextColor();

#ifdef __GNUC__
					constexpr int CP_DROPDOWNITEM = 9; // for some reason mingw use only enum up to 8
#endif
					::DrawThemeTextEx(hTheme, hdc, CP_DROPDOWNITEM, isDisabled ? CBXSR_DISABLED : CBXSR_NORMAL, buffer, -1, dtFlags, &rcText, &dtto);
				}
				else
				{
					::SetTextColor(hdc, isDisabled ? NppDarkMode::getDisabledTextColor() : NppDarkMode::getTextColor());
					::DrawText(hdc, buffer, -1, &rcText, dtFlags);
				}
				delete[] buffer;
			}

			hasFocus = ::GetFocus() == hWnd;
			if (!isDisabled && hasFocus && ::SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0) == FALSE)
			{
				::DrawFocusRect(hdc, &cbi.rcItem);
			}
		}
		else if (comboboxData._cbStyle == CBS_DROPDOWN && cbi.hwndItem != nullptr)
		{
			hasFocus = ::GetFocus() == cbi.hwndItem;

			::FillRect(hdc, &rcArrow, hSelectedBrush);
		}

		const auto hSelectedPen = isDisabled ? NppDarkMode::getDisabledEdgePen() : ((isHot || hasFocus) ? NppDarkMode::getHotEdgePen() : NppDarkMode::getEdgePen());
		auto holdPen = static_cast<HPEN>(::SelectObject(hdc, hSelectedPen));

		if (comboboxData._cbStyle != CBS_SIMPLE)
		{
			if (hasTheme)
			{
				RECT rcThemedArrow{ rcArrow.left, rcArrow.top - 1, rcArrow.right, rcArrow.bottom - 1 };
				::DrawThemeBackground(hTheme, hdc, CP_DROPDOWNBUTTONRIGHT, isDisabled ? CBXSR_DISABLED : CBXSR_NORMAL, &rcThemedArrow, nullptr);
			}
			else
			{
				const auto clrText = isDisabled ? NppDarkMode::getDisabledTextColor() : (isHot ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
				::SetTextColor(hdc, clrText);
				wchar_t arrow[] = L"˅";
				::DrawText(hdc, arrow, -1, &rcArrow, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
			}
		}

		if (comboboxData._cbStyle == CBS_DROPDOWNLIST)
		{
			RECT rcInner{ rcClient };
			::InflateRect(&rcInner, -1, -1);
			::ExcludeClipRect(hdc, rcInner.left, rcInner.top, rcInner.right, rcInner.bottom);
		}
		else if (comboboxData._cbStyle == CBS_DROPDOWN)
		{
			POINT edge[] = {
				{rcArrow.left - 1, rcArrow.top},
				{rcArrow.left - 1, rcArrow.bottom}
			};

			::Polyline(hdc, edge, _countof(edge));

			::ExcludeClipRect(hdc, cbi.rcItem.left, cbi.rcItem.top, cbi.rcItem.right, cbi.rcItem.bottom);
			::ExcludeClipRect(hdc, rcArrow.left - 1, rcArrow.top, rcArrow.right, rcArrow.bottom);

			HPEN hPen = ::CreatePen(PS_SOLID, 1, isDisabled ? NppDarkMode::getDlgBackgroundColor() : NppDarkMode::getBackgroundColor());
			RECT rcInner{ rcClient };
			::InflateRect(&rcInner, -1, -1);
			rcInner.right = rcArrow.left - 1;
			NppDarkMode::paintRoundFrameRect(hdc, rcInner, hPen);
			::DeleteObject(hPen);
			::InflateRect(&rcInner, -1, -1);
			::FillRect(hdc, &rcInner, isDisabled ? NppDarkMode::getDlgBackgroundBrush() : NppDarkMode::getCtrlBackgroundBrush());
		}

		const int roundCornerValue = NppDarkMode::isWindows11() ? 4 : 0;

		NppDarkMode::paintRoundFrameRect(hdc, rcClient, hSelectedPen, roundCornerValue, roundCornerValue);

		::SelectObject(hdc, holdPen);
	}

	static LRESULT CALLBACK ComboBoxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pComboboxData = reinterpret_cast<ComboboxData*>(dwRefData);
		auto& themeData = pComboboxData->_themeData;
		auto& bufferData = pComboboxData->_bufferData;
		auto& hMemDC = bufferData._hMemDC;

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ComboBoxSubclass, uIdSubclass);
				delete pComboboxData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled() && themeData.ensureTheme(hWnd))
				{
					auto hdc = reinterpret_cast<HDC>(wParam);
					if (pComboboxData->_cbStyle != CBS_DROPDOWN && hdc != hMemDC)
					{
						return FALSE;
					}
					return TRUE;
				}
				break;
			}

			case WM_PAINT:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				if (pComboboxData->_cbStyle != CBS_DROPDOWN)
				{
					if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
					{
						::EndPaint(hWnd, &ps);
						return 0;
					}

					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);

					if (bufferData.ensureBuffer(hdc, rcClient))
					{
						int savedState = ::SaveDC(hMemDC);
						::IntersectClipRect(
							hMemDC,
							ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom
						);

						NppDarkMode::paintCombobox(hWnd, hMemDC, *pComboboxData);

						::RestoreDC(hMemDC, savedState);

						::BitBlt(
							hdc,
							ps.rcPaint.left, ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							hMemDC,
							ps.rcPaint.left, ps.rcPaint.top,
							SRCCOPY
						);
					}
				}
				else // don't use double buffer for CBS_DROPDOWN since it has edit control which can cause flicker
				{
					NppDarkMode::paintCombobox(hWnd, hdc, *pComboboxData);
				}

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_ENABLE:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				LRESULT lr = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);
				return lr;
			}

			case WM_DPICHANGED:
			case WM_DPICHANGED_AFTERPARENT:
			{
				themeData.closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				themeData.closeTheme();
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassComboBoxControl(HWND hWnd)
	{
		if (::GetWindowSubclass(hWnd, ComboBoxSubclass, g_comboBoxSubclassID, nullptr) == FALSE)
		{
			auto cbStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE) & CBS_DROPDOWNLIST;
			auto pComboboxData = reinterpret_cast<DWORD_PTR>(new ComboboxData(cbStyle));
			::SetWindowSubclass(hWnd, ComboBoxSubclass, g_comboBoxSubclassID, pComboboxData);
		}
	}

	constexpr UINT_PTR g_listViewSubclassID = 42;

	static LRESULT CALLBACK ListViewSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ListViewSubclass, uIdSubclass);
				break;
			}

			case WM_NOTIFY:
			{
				switch (reinterpret_cast<LPNMHDR>(lParam)->code)
				{
					case NM_CUSTOMDRAW:
					{
						auto lpnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
						switch (lpnmcd->dwDrawStage)
						{
							case CDDS_PREPAINT:
							{
								if (NppDarkMode::isExperimentalSupported() && NppDarkMode::isEnabled())
								{
									return CDRF_NOTIFYITEMDRAW;
								}
								return CDRF_DODEFAULT;
							}

							case CDDS_ITEMPREPAINT:
							{
								SetTextColor(lpnmcd->hdc, NppDarkMode::getDarkerTextColor());

								return CDRF_NEWFONT;
							}

							default:
								return CDRF_DODEFAULT;
						}
					}
					break;
				}
				break;
			}
			break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassListViewControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, ListViewSubclass, g_listViewSubclassID, 0);
	}

	constexpr UINT_PTR g_upDownSubclassID = 42;

	static LRESULT CALLBACK UpDownSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
			case WM_PRINTCLIENT:
			case WM_PAINT:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				const auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				const bool isHorizontal = ((style & UDS_HORZ) == UDS_HORZ);

				bool hasTheme = pButtonData->ensureTheme(hWnd);

				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				::FillRect(hdc, &rcClient, NppDarkMode::getDlgBackgroundBrush());

				RECT rcArrowPrev{};
				RECT rcArrowNext{};

				if (isHorizontal)
				{
					RECT rcArrowLeft{
						rcClient.left, rcClient.top,
						rcClient.right - ((rcClient.right - rcClient.left) / 2), rcClient.bottom
					};

					RECT rcArrowRight{
						rcArrowLeft.right - 1, rcClient.top,
						rcClient.right, rcClient.bottom
					};

					rcArrowPrev = rcArrowLeft;
					rcArrowNext = rcArrowRight;
				}
				else
				{
					RECT rcArrowTop{
						rcClient.left, rcClient.top,
						rcClient.right, rcClient.bottom - ((rcClient.bottom - rcClient.top) / 2)
					};

					RECT rcArrowBottom{
						rcClient.left, rcArrowTop.bottom - 1,
						rcClient.right, rcClient.bottom
					};

					rcArrowPrev = rcArrowTop;
					rcArrowNext = rcArrowBottom;
				}

				POINT ptCursor{};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				bool isHotPrev = ::PtInRect(&rcArrowPrev, ptCursor);
				bool isHotNext = ::PtInRect(&rcArrowNext, ptCursor);

				::SetBkMode(hdc, TRANSPARENT);

				if (hasTheme)
				{
					::DrawThemeBackground(pButtonData->hTheme, hdc, BP_PUSHBUTTON, isHotPrev ? PBS_HOT : PBS_NORMAL, &rcArrowPrev, nullptr);
					::DrawThemeBackground(pButtonData->hTheme, hdc, BP_PUSHBUTTON, isHotNext ? PBS_HOT : PBS_NORMAL, &rcArrowNext, nullptr);
				}
				else
				{
					::FillRect(hdc, &rcArrowPrev, isHotPrev ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getBackgroundBrush());
					::FillRect(hdc, &rcArrowNext, isHotNext ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getBackgroundBrush());
				}

				const auto arrowTextFlags = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP;

				::SetTextColor(hdc, isHotPrev ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
				::DrawText(hdc, isHorizontal ? L"<" : L"˄", -1, &rcArrowPrev, arrowTextFlags);

				::SetTextColor(hdc, isHotNext ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
				::DrawText(hdc, isHorizontal ? L">" : L"˅", -1, &rcArrowNext, arrowTextFlags);

				if (!hasTheme)
				{
					NppDarkMode::paintRoundFrameRect(hdc, rcArrowPrev, NppDarkMode::getEdgePen());
					NppDarkMode::paintRoundFrameRect(hdc, rcArrowNext, NppDarkMode::getEdgePen());
				}

				::EndPaint(hWnd, &ps);
				return FALSE;
			}

			case WM_DPICHANGED:
			case WM_DPICHANGED_AFTERPARENT:
			{
				pButtonData->closeTheme();
				return 0;
			}

			case WM_THEMECHANGED:
			{
				pButtonData->closeTheme();
				break;
			}

			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, UpDownSubclass, uIdSubclass);
				delete pButtonData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled())
				{
					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);
					::FillRect(reinterpret_cast<HDC>(wParam), &rcClient, NppDarkMode::getDlgBackgroundBrush());
					return TRUE;
				}
				break;
			}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static void subclassAndThemeUpDownControl(HWND hwnd, NppDarkModeParams p)
	{
		if (p._subclass)
		{
			auto pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
			SetWindowSubclass(hwnd, UpDownSubclass, g_upDownSubclassID, pButtonData);
		}

		if (p._theme)
		{
			SetWindowTheme(hwnd, p._themeClassName, nullptr);
		}
	}

	bool subclassTabUpDownControl(HWND hwnd)
	{
		constexpr size_t classNameLen = 16;
		wchar_t className[classNameLen]{};
		GetClassName(hwnd, className, classNameLen);
		if (wcscmp(className, UPDOWN_CLASS) == 0)
		{
			auto pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
			SetWindowSubclass(hwnd, UpDownSubclass, g_upDownSubclassID, pButtonData);
			NppDarkMode::setDarkExplorerTheme(hwnd);
			return true;
		}

		return false;
	}

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass, bool theme)
	{
		NppDarkModeParams p{
			g_isAtLeastWindows10 && NppDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnableThemeDialogTexture(hwndParent, theme && !NppDarkMode::isEnabled() ? ETDT_ENABLETAB : ETDT_DISABLE);

		EnumChildWindows(hwndParent, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
			const auto& p = *reinterpret_cast<NppDarkModeParams*>(lParam);
			constexpr size_t classNameLen = 32;
			wchar_t className[classNameLen]{};
			GetClassName(hwnd, className, classNameLen);

			if (wcscmp(className, WC_BUTTON) == 0)
			{
				NppDarkMode::subclassAndThemeButton(hwnd, p);
				return TRUE;
			}

			if (wcscmp(className, WC_COMBOBOX) == 0)
			{
				NppDarkMode::subclassAndThemeComboBox(hwnd, p);
				return TRUE;
			}

			if (wcscmp(className, WC_EDIT) == 0)
			{
				if (!g_isWine)
				{
					NppDarkMode::subclassAndThemeListBoxOrEditControl(hwnd, p, false);
				}
				return TRUE;
			}

			if (wcscmp(className, WC_LISTBOX) == 0)
			{
				if (!g_isWine)
				{
					NppDarkMode::subclassAndThemeListBoxOrEditControl(hwnd, p, true);
				}
				return TRUE;
			}

			if (wcscmp(className, WC_LISTVIEW) == 0)
			{
				NppDarkMode::subclassAndThemeListView(hwnd, p);
				return TRUE;
			}

			if (wcscmp(className, WC_TREEVIEW) == 0)
			{
				NppDarkMode::themeTreeView(hwnd, p);
				return TRUE;
			}

			if (wcscmp(className, TOOLBARCLASSNAME) == 0)
			{
				NppDarkMode::themeToolbar(hwnd, p);
				return TRUE;
			}

			// Plugin might use rich edit control version 2.0 and later
			if (wcscmp(className, L"RichEdit20W") == 0 || wcscmp(className, L"RICHEDIT50W") == 0)
			{
				NppDarkMode::themeRichEdit(hwnd, p);
				return TRUE;
			}

			// For plugins
			if (wcscmp(className, UPDOWN_CLASS) == 0)
			{
				NppDarkMode::subclassAndThemeUpDownControl(hwnd, p);
				return TRUE;
			}

			/*
			// for debugging 
			if (wcscmp(className, L"#32770") == 0)
			{
				return TRUE;
			}

			if (wcscmp(className, WC_STATIC) == 0)
			{
				return TRUE;
			}

			if (wcscmp(className, TRACKBAR_CLASS) == 0)
			{
				return TRUE;
			}
			*/

			return TRUE;
		}, reinterpret_cast<LPARAM>(&p));
	}

	void autoThemeChildControls(HWND hwndParent)
	{
		autoSubclassAndThemeChildControls(hwndParent, false, g_isAtLeastWindows10);
	}

	void subclassAndThemeButton(HWND hwnd, NppDarkModeParams p)
	{
		auto nButtonStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		switch (nButtonStyle & BS_TYPEMASK)
		{
			// Plugin might use BS_3STATE, BS_AUTO3STATE and BS_PUSHLIKE button style
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_3STATE:
			case BS_AUTO3STATE:
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
			{
				if ((nButtonStyle & BS_PUSHLIKE) == BS_PUSHLIKE)
				{
					if (p._theme)
					{
						SetWindowTheme(hwnd, p._themeClassName, nullptr);
					}
					break;
				}

				if (NppDarkMode::isWindows11() && p._theme)
				{
					SetWindowTheme(hwnd, p._themeClassName, nullptr);
				}

				if (p._subclass)
				{
					NppDarkMode::subclassButtonControl(hwnd);
				}
				break;
			}

			case BS_GROUPBOX:
			{
				if (p._subclass)
				{
					NppDarkMode::subclassGroupboxControl(hwnd);
				}
				break;
			}

			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
			case BS_SPLITBUTTON:
			case BS_DEFSPLITBUTTON:
			{
				if (p._theme)
				{
					SetWindowTheme(hwnd, p._themeClassName, nullptr);
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}

	void subclassAndThemeComboBox(HWND hWnd, NppDarkModeParams p)
	{
		const auto nStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);

		if ((nStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || (nStyle & CBS_DROPDOWN) == CBS_DROPDOWN)
		{
			COMBOBOXINFO cbi{};
			cbi.cbSize = sizeof(COMBOBOXINFO);
			if (::GetComboBoxInfo(hWnd, &cbi) == TRUE)
			{
				if (p._theme && cbi.hwndList)
				{
					//dark scrollbar for listbox of combobox
					::SetWindowTheme(cbi.hwndList, p._themeClassName, nullptr);
				}
			}

			if (p._subclass)
			{
				HWND hParent = ::GetParent(hWnd);
				if ((hParent == nullptr || getWndClassName(hParent) != WC_COMBOBOXEX))
				{
					NppDarkMode::subclassComboBoxControl(hWnd);
				}
			}

			if (p._theme && NppDarkMode::isExperimentalSupported())
			{
				NppDarkMode::allowDarkModeForWindow(hWnd, NppDarkMode::isExperimentalActive());
				::SetWindowTheme(hWnd, L"CFD", nullptr);
			}
		}
	}

	void subclassAndThemeListBoxOrEditControl(HWND hwnd, NppDarkModeParams p, bool isListBox)
	{
		const auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		bool hasScrollBar = ((style & WS_HSCROLL) == WS_HSCROLL) || ((style & WS_VSCROLL) == WS_VSCROLL);
		if (p._theme && (isListBox || hasScrollBar))
		{
			//dark scrollbar for listbox or edit control
			SetWindowTheme(hwnd, p._themeClassName, nullptr);
		}

		const auto exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
		bool hasClientEdge = (exStyle & WS_EX_CLIENTEDGE) == WS_EX_CLIENTEDGE;
		bool isCBoxListBox = isListBox && (style & LBS_COMBOBOX) == LBS_COMBOBOX;

		if (p._subclass && hasClientEdge && !isCBoxListBox)
		{
			NppDarkMode::subclassCustomBorderForListBoxAndEditControls(hwnd);
		}

#ifndef __MINGW64__ // mingw build for 64 bit has issue with GetWindowSubclass, it is undefined

		bool changed = false;
		if (::GetWindowSubclass(hwnd, CustomBorderSubclass, g_customBorderSubclassID, nullptr) == TRUE)
		{
			if (NppDarkMode::isEnabled())
			{
				if (hasClientEdge)
				{
					::SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_CLIENTEDGE);
					changed = true;
				}
			}
			else if (!hasClientEdge)
			{
				::SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_CLIENTEDGE);
				changed = true;
			}
		}

		if (changed)
		{
			::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}

#endif // !__MINGW64__
	}

	void subclassAndThemeListView(HWND hwnd, NppDarkModeParams p)
	{
		if (p._theme)
		{
			NppDarkMode::setDarkListView(hwnd);
			NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::listview);
		}

		ListView_SetTextColor(hwnd, NppParameters::getInstance().getCurrentDefaultFgColor());
		ListView_SetTextBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());
		ListView_SetBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());

		if (p._subclass)
		{
			auto exStyle = ListView_GetExtendedListViewStyle(hwnd);
			ListView_SetExtendedListViewStyle(hwnd, exStyle | LVS_EX_DOUBLEBUFFER);
			NppDarkMode::subclassListViewControl(hwnd);
		}
	}

	void themeTreeView(HWND hwnd, NppDarkModeParams p)
	{
		TreeView_SetTextColor(hwnd, NppParameters::getInstance().getCurrentDefaultFgColor());
		TreeView_SetBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());

		//NppDarkMode::calculateTreeViewStyle();
		NppDarkMode::setTreeViewStyle(hwnd, p._theme);

		if (p._theme)
		{
			NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::treeview);
		}
	}

	void themeToolbar(HWND hwnd, NppDarkModeParams p)
	{
		NppDarkMode::setDarkLineAbovePanelToolbar(hwnd);

		if (p._theme)
		{
			NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::toolbar);
		}
	}

	void themeRichEdit(HWND hwnd, NppDarkModeParams p)
	{
		if (p._theme)
		{
			//dark scrollbar for rich edit control
			SetWindowTheme(hwnd, p._themeClassName, nullptr);
		}
	}

	static LRESULT darkToolBarNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto nmtbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
		static int roundCornerValue = 0;

		switch (nmtbcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = CDRF_DODEFAULT;
				if (NppDarkMode::isEnabled())
				{
					if (NppDarkMode::isWindows11())
					{
						roundCornerValue = 5;
					}

					::FillRect(nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, NppDarkMode::getDlgBackgroundBrush());
					lr |= CDRF_NOTIFYITEMDRAW;
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				nmtbcd->hbrMonoDither = NppDarkMode::getBackgroundBrush();
				nmtbcd->hbrLines = NppDarkMode::getEdgeBrush();
				nmtbcd->hpenLines = NppDarkMode::getEdgePen();
				nmtbcd->clrText = NppDarkMode::getTextColor();
				nmtbcd->clrTextHighlight = NppDarkMode::getTextColor();
				nmtbcd->clrBtnFace = NppDarkMode::getBackgroundColor();
				nmtbcd->clrBtnHighlight = NppDarkMode::getCtrlBackgroundColor();
				nmtbcd->clrHighlightHotTrack = NppDarkMode::getHotBackgroundColor();
				nmtbcd->nStringBkMode = TRANSPARENT;
				nmtbcd->nHLStringBkMode = TRANSPARENT;

				if ((nmtbcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
				{
					auto holdBrush = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getHotBackgroundBrush());
					auto holdPen = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getHotEdgePen());
					::RoundRect(nmtbcd->nmcd.hdc, nmtbcd->nmcd.rc.left, nmtbcd->nmcd.rc.top, nmtbcd->nmcd.rc.right, nmtbcd->nmcd.rc.bottom, roundCornerValue, roundCornerValue);
					::SelectObject(nmtbcd->nmcd.hdc, holdBrush);
					::SelectObject(nmtbcd->nmcd.hdc, holdPen);

					nmtbcd->nmcd.uItemState &= ~(CDIS_CHECKED | CDIS_HOT);
				}
				else if ((nmtbcd->nmcd.uItemState & CDIS_CHECKED) == CDIS_CHECKED)
				{
					auto holdBrush = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getCtrlBackgroundBrush());
					auto holdPen = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getEdgePen());
					::RoundRect(nmtbcd->nmcd.hdc, nmtbcd->nmcd.rc.left, nmtbcd->nmcd.rc.top, nmtbcd->nmcd.rc.right, nmtbcd->nmcd.rc.bottom, roundCornerValue, roundCornerValue);
					::SelectObject(nmtbcd->nmcd.hdc, holdBrush);
					::SelectObject(nmtbcd->nmcd.hdc, holdPen);

					nmtbcd->nmcd.uItemState &= ~CDIS_CHECKED;
				}

				LRESULT lr = TBCDRF_USECDCOLORS;
				if ((nmtbcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
				{
					lr |= TBCDRF_NOBACKGROUND;
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT darkListViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

		switch (lplvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = CDRF_NOTIFYITEMDRAW;
				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				auto isSelected = ListView_GetItemState(lplvcd->nmcd.hdr.hwndFrom, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;

				if (NppDarkMode::isEnabled())
				{
					if (isSelected)
					{
						lplvcd->clrText = NppDarkMode::getTextColor();
						lplvcd->clrTextBk = NppDarkMode::getCtrlBackgroundColor();

						::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, NppDarkMode::getCtrlBackgroundBrush());
					}
					else if ((lplvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
					{
						lplvcd->clrText = NppDarkMode::getTextColor();
						lplvcd->clrTextBk = NppDarkMode::getHotBackgroundColor();

						::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, NppDarkMode::getHotBackgroundBrush());
					}
				}

				if (isSelected)
				{
					::DrawFocusRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc);
				}

				LRESULT lr = CDRF_NEWFONT;

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT darkTreeViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lptvcd = reinterpret_cast<LPNMTVCUSTOMDRAW>(lParam);

		switch (lptvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = NppDarkMode::isEnabled() ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				LRESULT lr = CDRF_DODEFAULT;

				if (NppDarkMode::isEnabled())
				{
					if ((lptvcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
					{
						lptvcd->clrText = NppDarkMode::getTextColor();
						lptvcd->clrTextBk = NppDarkMode::getCtrlBackgroundColor();
						::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, NppDarkMode::getCtrlBackgroundBrush());

						lr |= CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
					}
					else if ((lptvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
					{
						lptvcd->clrText = NppDarkMode::getTextColor();
						lptvcd->clrTextBk = NppDarkMode::getHotBackgroundColor();

						if (g_isAtLeastWindows10 || g_treeViewStyle == TreeViewStyle::light)
						{
							::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, NppDarkMode::getHotBackgroundBrush());
							lr |= CDRF_NOTIFYPOSTPAINT;
						}
						lr |= CDRF_NEWFONT;
					}
				}

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPOSTPAINT:
			{
				if (NppDarkMode::isEnabled())
				{
					RECT rcFrame = lptvcd->nmcd.rc;
					rcFrame.left -= 1;
					rcFrame.right += 1;

					if ((lptvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
					{
						NppDarkMode::paintRoundFrameRect(lptvcd->nmcd.hdc, rcFrame, NppDarkMode::getHotEdgePen(), 0, 0);
					}
					else if ((lptvcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
					{
						NppDarkMode::paintRoundFrameRect(lptvcd->nmcd.hdc, rcFrame, NppDarkMode::getEdgePen(), 0, 0);
					}
				}

				if (isPlugin)
				{
					break;
				}

				return CDRF_DODEFAULT;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT darkTrackBarNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lpnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

		switch (lpnmcd->dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				LRESULT lr = NppDarkMode::isEnabled() ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

				if (isPlugin)
				{
					lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr;
			}

			case CDDS_ITEMPREPAINT:
			{
				switch (lpnmcd->dwItemSpec)
				{
					case TBCD_THUMB:
					{
						if (::IsWindowEnabled(lpnmcd->hdr.hwndFrom) == FALSE)
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, NppDarkMode::getDisabledEdgeBrush());
							LRESULT lr = CDRF_SKIPDEFAULT;
							if (isPlugin)
							{
								lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
							}
							return lr;
						}
						else if ((lpnmcd->uItemState & CDIS_SELECTED) == CDIS_SELECTED)
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, NppDarkMode::getCtrlBackgroundBrush());
							LRESULT lr = CDRF_SKIPDEFAULT;
							if (isPlugin)
							{
								lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
							}
							return lr;
						}
						break;
					}

					case TBCD_CHANNEL:
					{
						if (::IsWindowEnabled(lpnmcd->hdr.hwndFrom) == FALSE)
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, NppDarkMode::getDlgBackgroundBrush());
							NppDarkMode::paintRoundFrameRect(lpnmcd->hdc, lpnmcd->rc, NppDarkMode::getDisabledEdgePen(), 0, 0);
						}
						else
						{
							::FillRect(lpnmcd->hdc, &lpnmcd->rc, NppDarkMode::getCtrlBackgroundBrush());
						}

						LRESULT lr = CDRF_SKIPDEFAULT;
						if (isPlugin)
						{
							lr |= ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
						}
						return lr;
					}

					default:
						break;
				}
				break;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	constexpr UINT_PTR g_pluginDockWindowSubclassID = 42;

	static LRESULT CALLBACK PluginDockWindowSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(dwRefData);

		switch (uMsg)
		{
			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled())
				{
					RECT rect{};
					GetClientRect(hWnd, &rect);
					::FillRect(reinterpret_cast<HDC>(wParam), &rect, NppDarkMode::getDlgBackgroundBrush());
					return TRUE;
				}
				break;
			}

			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, PluginDockWindowSubclass, uIdSubclass);
				break;
			}

			case NPPM_INTERNAL_REFRESHDARKMODE:
			{
				NppDarkMode::autoThemeChildControls(hWnd);
				return TRUE;
			}

			case WM_CTLCOLOREDIT:
			{
				if (NppDarkMode::isEnabled())
				{
					return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_CTLCOLORLISTBOX:
			{
				if (NppDarkMode::isEnabled())
				{
					return NppDarkMode::onCtlColorListbox(wParam, lParam);
				}
				break;
			}

			case WM_CTLCOLORDLG:
			{

				if (NppDarkMode::isEnabled())
				{
					return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_CTLCOLORSTATIC:
			{
				if (NppDarkMode::isEnabled())
				{
					constexpr size_t classNameLen = 16;
					wchar_t className[classNameLen]{};
					auto hwndEdit = reinterpret_cast<HWND>(lParam);
					GetClassName(hwndEdit, className, classNameLen);
					if (wcscmp(className, WC_EDIT) == 0)
					{
						return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
					}
					return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
				}
				break;
			}

			case WM_PRINTCLIENT:
			{
				if (NppDarkMode::isEnabled())
				{
					return TRUE;
				}
				break;
			}

			case WM_NOTIFY:
			{
				const auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
				switch (nmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						std::wstring className = getWndClassName(nmhdr->hwndFrom);
						if (className == TOOLBARCLASSNAME)
						{
							return NppDarkMode::darkToolBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, true);
						}

						if (className == WC_LISTVIEW)
						{
							return NppDarkMode::darkListViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, true);
						}

						if (className == WC_TREEVIEW)
						{
							return NppDarkMode::darkTreeViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, true);
						}

						if (className == TRACKBAR_CLASS)
						{
							return NppDarkMode::darkTrackBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, true);
						}
						break;
					}

					default:
						break;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassAndThemePluginDockWindow(HWND hwnd)
	{
		SetWindowSubclass(hwnd, PluginDockWindowSubclass, g_pluginDockWindowSubclassID, 0);
		NppDarkMode::autoSubclassAndThemeChildControls(hwnd, true, g_isAtLeastWindows10);
	}

	ULONG autoSubclassAndThemePlugin(HWND hwnd, ULONG dmFlags)
	{
		// Used on parent of edit, listbox, static text, treeview, listview and toolbar controls.
		// Should be used only one time on parent control after its creation
		// even when starting in light mode.
		// e.g. in WM_INITDIALOG, in WM_CREATE or after CreateWindow.
		constexpr ULONG dmfSubclassParent =     0x00000001UL;
		// Should be used only one time on main control/window after initializations of all its children controls
		// even when starting in light mode.
		// Will also use dmfSetThemeChildren flag.
		// e.g. in WM_INITDIALOG, in WM_CREATE or after CreateWindow.
		constexpr ULONG dmfSubclassChildren =   0x00000002UL;
		// Will apply theme on buttons with style:
		// BS_PUSHLIKE, BS_PUSHBUTTON, BS_DEFPUSHBUTTON, BS_SPLITBUTTON or BS_DEFSPLITBUTTON.
		// Will apply theme for scrollbars on edit, listbox and rich edit controls.
		// Will apply theme for tooltips on listview, treeview and toolbar buttons.
		// Should be handled after controls initializations and in NPPN_DARKMODECHANGED.
		// Requires at least Windows 10 to work properly.
		constexpr ULONG dmfSetThemeChildren =   0x00000004UL;
		// Set dark title bar.
		// Should be handled after controls initializations and in NPPN_DARKMODECHANGED.
		// Requires at least Windows 10 and WS_CAPTION style to work properly.
		constexpr ULONG dmfSetTitleBar =        0x00000008UL;
		// Will apply dark explorer theme.
		// Used mainly for scrollbars and tooltips not handled with dmfSetThemeChildren.
		// Might also change style for other elements.
		// Should be handled after controls initializations and in NPPN_DARKMODECHANGED.
		// Requires at least Windows 10 to work properly.
		constexpr ULONG dmfSetThemeDirectly =   0x00000010UL;

		// defined in Notepad_plus_msgs.h
		//constexpr ULONG dmfInit =             dmfSubclassParent | dmfSubclassChildren | dmfSetTitleBar; // 0x000000BUL
		//constexpr ULONG dmfHandleChange =     dmfSetThemeChildren | dmfSetTitleBar;                     // 0x000000CUL

		constexpr ULONG dmfRequiredMask =       dmfSubclassParent | dmfSubclassChildren | dmfSetThemeChildren | dmfSetTitleBar | dmfSetThemeDirectly;
		//constexpr ULONG dmfAllMask =          dmfSubclassParent | dmfSubclassChildren | dmfSetThemeChildren | dmfSetTitleBar | dmfSetThemeDirectly;
		
		if (hwnd == nullptr || (dmFlags & dmfRequiredMask) == 0)
		{
			return 0;
		}

		auto dmfBitwiseCheck = [dmFlags](ULONG flag) -> bool {
			return (dmFlags & flag) == flag;
		};

		ULONG result = 0UL;

		if (dmfBitwiseCheck(dmfSubclassParent))
		{
			const bool success = ::SetWindowSubclass(hwnd, PluginDockWindowSubclass, g_pluginDockWindowSubclassID, 0) == TRUE;
			if (success)
			{
				result |= dmfSubclassParent;
			}
		}

		const bool subclassChildren = dmfBitwiseCheck(dmfSubclassChildren);
		if (dmfBitwiseCheck(dmfSetThemeChildren) || subclassChildren)
		{
			NppDarkMode::autoSubclassAndThemeChildControls(hwnd, subclassChildren, g_isAtLeastWindows10);
			result |= dmfSetThemeChildren;

			if (subclassChildren)
			{
				result |= dmfSubclassChildren;
			}
		}

		if (dmfBitwiseCheck(dmfSetTitleBar))
		{
			const auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
			if (NppDarkMode::isExperimentalSupported() && ((style & WS_CAPTION) == WS_CAPTION))
			{
				NppDarkMode::setDarkTitleBar(hwnd);
				result |= dmfSetTitleBar;
			}
		}

		if (dmfBitwiseCheck(dmfSetThemeDirectly))
		{
			if (NppDarkMode::isWindows10())
			{
				NppDarkMode::setDarkExplorerTheme(hwnd);
				result |= dmfSetThemeDirectly;
			}
		}

		return result;
	}

	constexpr UINT_PTR g_windowNotifySubclassID = 42;

	static LRESULT CALLBACK WindowNotifySubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(dwRefData);

		switch (uMsg)
		{
			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, WindowNotifySubclass, uIdSubclass);
				break;
			}

			case WM_NOTIFY:
			{
				auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
				switch (nmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						std::wstring className = getWndClassName(nmhdr->hwndFrom);
						if (className == TOOLBARCLASSNAME)
						{
							return NppDarkMode::darkToolBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == WC_LISTVIEW)
						{
							return NppDarkMode::darkListViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == WC_TREEVIEW)
						{
							return NppDarkMode::darkTreeViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}

						if (className == TRACKBAR_CLASS)
						{
							return NppDarkMode::darkTrackBarNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}
						break;
					}

					default:
						break;
				}
				break;
			}
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassAndThemeWindowNotify(HWND hwnd)
	{
		SetWindowSubclass(hwnd, WindowNotifySubclass, g_windowNotifySubclassID, 0);
	}

	void setDarkTitleBar(HWND hwnd)
	{
		constexpr DWORD win10Build2004 = 19041;
		if (NppDarkMode::getWindowsBuildNumber() >= win10Build2004)
		{
			BOOL value = NppDarkMode::isEnabled() ? TRUE : FALSE;
			::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
		}
		else
		{
			NppDarkMode::allowDarkModeForWindow(hwnd, NppDarkMode::isEnabled());
			NppDarkMode::setTitleBarThemeColor(hwnd);
		}
	}

	void setDarkExplorerTheme(HWND hwnd)
	{
		SetWindowTheme(hwnd, g_isAtLeastWindows10 && NppDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr, nullptr);
	}

	void setDarkScrollBar(HWND hwnd)
	{
		NppDarkMode::setDarkExplorerTheme(hwnd);
	}

	void setDarkTooltips(HWND hwnd, ToolTipsType type)
	{
		UINT msg = 0;
		switch (type)
		{
			case NppDarkMode::ToolTipsType::toolbar:
				msg = TB_GETTOOLTIPS;
				break;
			case NppDarkMode::ToolTipsType::listview:
				msg = LVM_GETTOOLTIPS;
				break;
			case NppDarkMode::ToolTipsType::treeview:
				msg = TVM_GETTOOLTIPS;
				break;
			case NppDarkMode::ToolTipsType::tabbar:
				msg = TCM_GETTOOLTIPS;
				break;
			case NppDarkMode::ToolTipsType::tooltip:
				msg = 0;
				break;
		}

		if (msg == 0)
		{
			NppDarkMode::setDarkExplorerTheme(hwnd);
		}
		else
		{
			auto hTips = reinterpret_cast<HWND>(::SendMessage(hwnd, msg, 0, 0));
			if (hTips != nullptr)
			{
				NppDarkMode::setDarkExplorerTheme(hTips);
			}
		}
	}

	void setDarkLineAbovePanelToolbar(HWND hwnd)
	{
		COLORSCHEME scheme{};
		scheme.dwSize = sizeof(COLORSCHEME);

		if (NppDarkMode::isEnabled())
		{
			scheme.clrBtnHighlight = NppDarkMode::getDlgBackgroundColor();
			scheme.clrBtnShadow = NppDarkMode::getDlgBackgroundColor();
		}
		else
		{
			scheme.clrBtnHighlight = CLR_DEFAULT;
			scheme.clrBtnShadow = CLR_DEFAULT;
		}

		::SendMessage(hwnd, TB_SETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));
	}

	void setDarkListView(HWND hwnd)
	{
		if (NppDarkMode::isExperimentalSupported())
		{
			bool useDark = NppDarkMode::isEnabled();

			HWND hHeader = ListView_GetHeader(hwnd);
			NppDarkMode::allowDarkModeForWindow(hHeader, useDark);
			SetWindowTheme(hHeader, useDark ? L"ItemsView" : nullptr, nullptr);

			NppDarkMode::allowDarkModeForWindow(hwnd, useDark);
			SetWindowTheme(hwnd, L"Explorer", nullptr);
		}
	}

	void disableVisualStyle(HWND hwnd, bool doDisable)
	{
		if (doDisable)
		{
			SetWindowTheme(hwnd, L"", L"");
		}
		else
		{
			SetWindowTheme(hwnd, nullptr, nullptr);
		}
	}

	// range to determine when it should be better to use classic style
	constexpr double g_middleGrayRange = 2.0;

	void calculateTreeViewStyle()
	{
		COLORREF bgColor = NppParameters::getInstance().getCurrentDefaultBgColor();

		if (g_treeViewBg != bgColor || g_lightnessTreeView == 50.0)
		{
			g_lightnessTreeView = calculatePerceivedLightness(bgColor);
			g_treeViewBg = bgColor;
		}

		if (g_lightnessTreeView < (50.0 - g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::dark;
		}
		else if (g_lightnessTreeView > (50.0 + g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::light;
		}
		else
		{
			g_treeViewStyle = TreeViewStyle::classic;
		}
	}

	void updateTreeViewStylePrev()
	{
		g_treeViewStylePrev = g_treeViewStyle;
	}

	TreeViewStyle getTreeViewStyle()
	{
		const auto style = g_treeViewStyle;
		return style;
	}

	void setTreeViewStyle(HWND hWnd, bool force)
	{
		if (force || g_treeViewStylePrev != g_treeViewStyle)
		{
			auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			const bool hasHotStyle = (style & TVS_TRACKSELECT) == TVS_TRACKSELECT;
			bool change = false;
			std::wstring strSubAppName;

			switch (g_treeViewStyle)
			{
				case TreeViewStyle::light:
				{
					if (!hasHotStyle)
					{
						style |= TVS_TRACKSELECT;
						change = true;
					}
					strSubAppName = L"Explorer";
					break;
				}

				case TreeViewStyle::dark:
				{
					if (NppDarkMode::isExperimentalSupported())
					{
						if (!hasHotStyle)
						{
							style |= TVS_TRACKSELECT;
							change = true;
						}
						strSubAppName = L"DarkMode_Explorer";
						break;
					}
					[[fallthrough]];
				}

				case TreeViewStyle::classic:
				{
					if (hasHotStyle)
					{
						style &= ~TVS_TRACKSELECT;
						change = true;
					}
					strSubAppName = L"";
					break;
				}
			}

			if (change)
			{
				::SetWindowLongPtr(hWnd, GWL_STYLE, style);
			}

			::SetWindowTheme(hWnd, strSubAppName.empty() ? nullptr : strSubAppName.c_str(), nullptr);
		}
	}

	bool isThemeDark()
	{
		return g_treeViewStyle == TreeViewStyle::dark;
	}

	void setBorder(HWND hwnd, bool border)
	{
		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool hasBorder = (style & WS_BORDER) == WS_BORDER;
		bool change = false;

		if (!hasBorder && border)
		{
			style |= WS_BORDER;
			change = true;
		}
		else if (hasBorder && !border)
		{
			style &= ~WS_BORDER;
			change = true;
		}

		if (change)
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
			::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}

	static BOOL CALLBACK enumAutocompleteProc(HWND hwnd, LPARAM /*lParam*/)
	{
		constexpr size_t classNameLen = 16;
		wchar_t className[classNameLen]{};
		GetClassName(hwnd, className, classNameLen);
		if ((wcscmp(className, L"ListBoxX") == 0))
		{
			NppDarkMode::setDarkTitleBar(hwnd);
			NppDarkMode::autoThemeChildControls(hwnd);

			return FALSE;
		}

		return TRUE;
	}

	// set dark scrollbar for autocomplete list
	void setDarkAutoCompletion()
	{
		::EnumThreadWindows(::GetCurrentThreadId(), enumAutocompleteProc, 0);
	}

	LRESULT onCtlColor(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getBackgroundBrush());
	}

	LRESULT onCtlColorCtrl(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getCtrlBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getCtrlBackgroundBrush());
	}

	LRESULT onCtlColorDlg(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getDlgBackgroundBrush());
	}

	LRESULT onCtlColorError(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getErrorBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getErrorBackgroundBrush());
	}
	
	LRESULT onCtlColorDlgStaticText(HDC hdc, bool isTextEnabled)
	{
		if (!NppDarkMode::isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
			return FALSE;
		}

		::SetTextColor(hdc, isTextEnabled ? NppDarkMode::getTextColor() : NppDarkMode::getDisabledTextColor());
		::SetBkColor(hdc, NppDarkMode::getDlgBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getDlgBackgroundBrush());
	}

	LRESULT onCtlColorListbox(WPARAM wParam, LPARAM lParam)
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		auto hwnd = reinterpret_cast<HWND>(lParam);

		auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		bool isComboBox = (style & LBS_COMBOBOX) == LBS_COMBOBOX;
		if ((!isComboBox || !NppDarkMode::isExperimentalActive()))
		{
			if (::IsWindowEnabled(hwnd))
				return NppDarkMode::onCtlColorCtrl(hdc);
			return NppDarkMode::onCtlColorDlg(hdc);
		}
		return NppDarkMode::onCtlColor(hdc);
	}
}
