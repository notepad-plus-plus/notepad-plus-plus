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

#include <Uxtheme.h>
#include <Vssym32.h>

#include "Parameters.h"
#include "resource.h"

#include <Shlwapi.h>

#ifdef __GNUC__
#include <cmath>
#define WINAPI_LAMBDA WINAPI
#else
#define WINAPI_LAMBDA
#endif

#pragma comment(lib, "uxtheme.lib")

namespace NppDarkMode
{
	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH softerBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH pureBackground = nullptr;
		HBRUSH errorBackground = nullptr;

		HBRUSH edgeBrush = nullptr;
		HBRUSH hotEdgeBrush = nullptr;
		HBRUSH disabledEdgeBrush = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, softerBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, pureBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))

			, edgeBrush(::CreateSolidBrush(colors.edge))
			, hotEdgeBrush(::CreateSolidBrush(colors.hotEdge))
			, disabledEdgeBrush(::CreateSolidBrush(colors.disabledEdge))
		{}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(softerBackground);	softerBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(pureBackground);		pureBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;

			::DeleteObject(edgeBrush);			edgeBrush = nullptr;
			::DeleteObject(hotEdgeBrush);		hotEdgeBrush = nullptr;
			::DeleteObject(disabledEdgeBrush);	disabledEdgeBrush = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(softerBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(pureBackground);
			::DeleteObject(errorBackground);

			::DeleteObject(edgeBrush);
			::DeleteObject(hotEdgeBrush);
			::DeleteObject(disabledEdgeBrush);

			background = ::CreateSolidBrush(colors.background);
			softerBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			pureBackground = ::CreateSolidBrush(colors.pureBackground);
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
		HEXRGB(0x202020),	// background
		HEXRGB(0x404040),	// softerBackground
		HEXRGB(0x404040),	// hotBackground
		HEXRGB(0x202020),	// pureBackground
		HEXRGB(0xB00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x646464),	// edgeColor
		HEXRGB(0x9B9B9B),	// hotEdgeColor
		HEXRGB(0x484848)	// disabledEdgeColor
	};

	// red tone
	static const Colors darkRedColors{
		HEXRGB(0x302020),	// background
		HEXRGB(0x504040),	// softerBackground
		HEXRGB(0x504040),	// hotBackground
		HEXRGB(0x302020),	// pureBackground
		HEXRGB(0xC00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x908080),	// edgeColor
		HEXRGB(0xBBABAB),	// hotEdgeColor
		HEXRGB(0x584848)	// disabledEdgeColor
	};

	// green tone
	static const Colors darkGreenColors{
		HEXRGB(0x203020),	// background
		HEXRGB(0x405040),	// softerBackground
		HEXRGB(0x405040),	// hotBackground
		HEXRGB(0x203020),	// pureBackground
		HEXRGB(0xB01000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x809080),	// edgeColor
		HEXRGB(0xABBBAB),	// hotEdgeColor
		HEXRGB(0x485848)	// disabledEdgeColor
	};

	// blue tone
	static const Colors darkBlueColors{
		HEXRGB(0x202040),	// background
		HEXRGB(0x404060),	// softerBackground
		HEXRGB(0x404060),	// hotBackground
		HEXRGB(0x202040),	// pureBackground
		HEXRGB(0xB00020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x8080A0),	// edgeColor
		HEXRGB(0xABABCB),	// hotEdgeColor
		HEXRGB(0x484868)	// disabledEdgeColor
	};

	// purple tone
	static const Colors darkPurpleColors{
		HEXRGB(0x302040),	// background
		HEXRGB(0x504060),	// softerBackground
		HEXRGB(0x504060),	// hotBackground
		HEXRGB(0x302040),	// pureBackground
		HEXRGB(0xC00020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x9080A0),	// edgeColor
		HEXRGB(0xBBABCB),	// hotEdgeColor
		HEXRGB(0x584868)	// disabledEdgeColor
	};

	// cyan tone
	static const Colors darkCyanColors{
		HEXRGB(0x203040),	// background
		HEXRGB(0x405060),	// softerBackground
		HEXRGB(0x405060),	// hotBackground
		HEXRGB(0x203040),	// pureBackground
		HEXRGB(0xB01020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x8090A0),	// edgeColor
		HEXRGB(0xBBBBCB),	// hotEdgeColor
		HEXRGB(0x485868)	// disabledEdgeColor
	};

	// olive tone
	static const Colors darkOliveColors{
		HEXRGB(0x303020),	// background
		HEXRGB(0x505040),	// softerBackground
		HEXRGB(0x505040),	// hotBackground
		HEXRGB(0x303020),	// pureBackground
		HEXRGB(0xC01000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x909080),	// edgeColor
		HEXRGB(0xBBBBAB),	// hotEdgeColor
		HEXRGB(0x585848)	// disabledEdgeColor
	};

	// customized
	Colors darkCustomizedColors{
		HEXRGB(0x202020),	// background
		HEXRGB(0x404040),	// softerBackground
		HEXRGB(0x404040),	// hotBackground
		HEXRGB(0x202020),	// pureBackground
		HEXRGB(0xB00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x646464),	// edgeColor
		HEXRGB(0x9B9B9B),	// hotEdgeColor
		HEXRGB(0x484848)	// disabledEdgeColor
	};

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


	Theme& getTheme()
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

	Options configuredOptions()
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

	static bool g_isAtLeastWindows10 = false;

	void initDarkMode()
	{
		_options = configuredOptions();

		initExperimentalDarkMode();
		setDarkMode(_options.enable, true);

		g_isAtLeastWindows10 = NppDarkMode::isWindows10();
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

	bool isWindows10()
	{
		return IsWindows10();
	}

	bool isWindows11()
	{
		return IsWindows11();
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

	COLORREF invertLightnessSofter(COLORREF c)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(c, &h, &l, &s);

		l = min(240 - l, 211);

		COLORREF invert_c = ColorHLSToRGB(h, l, s);

		return invert_c;
	}

	static TreeViewStyle g_treeViewStyle = TreeViewStyle::classic;
	static COLORREF g_treeViewBg = NppParameters::getInstance().getCurrentDefaultBgColor();
	static double g_lighnessTreeView = 50.0;

	// adapted from https://stackoverflow.com/a/56678483
	double calculatePerceivedLighness(COLORREF c)
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

		double lighness = (luminance <= 216.0 / 24389.0) ? (luminance * 24389.0 / 27.0) : (std::pow(luminance, (1.0 / 3.0)) * 116.0 - 16.0);
		return lighness;
	}

	COLORREF getBackgroundColor()         { return getTheme()._colors.background; }
	COLORREF getSofterBackgroundColor()   { return getTheme()._colors.softerBackground; }
	COLORREF getHotBackgroundColor()      { return getTheme()._colors.hotBackground; }
	COLORREF getDarkerBackgroundColor()   { return getTheme()._colors.pureBackground; }
	COLORREF getErrorBackgroundColor()    { return getTheme()._colors.errorBackground; }
	COLORREF getTextColor()               { return getTheme()._colors.text; }
	COLORREF getDarkerTextColor()         { return getTheme()._colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme()._colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme()._colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme()._colors.edge; }
	COLORREF getHotEdgeColor()            { return getTheme()._colors.hotEdge; }
	COLORREF getDisabledEdgeColor()       { return getTheme()._colors.disabledEdge; }

	HBRUSH getBackgroundBrush()           { return getTheme()._brushes.background; }
	HBRUSH getSofterBackgroundBrush()     { return getTheme()._brushes.softerBackground; }
	HBRUSH getHotBackgroundBrush()        { return getTheme()._brushes.hotBackground; }
	HBRUSH getDarkerBackgroundBrush()     { return getTheme()._brushes.pureBackground; }
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

	void setSofterBackgroundColor(COLORREF c)
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

	void setDarkerBackgroundColor(COLORREF c)
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

	Colors getDarkModeDefaultColors()
	{
		return darkColors;
	}

	void changeCustomTheme(const Colors& colors)
	{
		tCustom.change(colors);
	}

	// handle events

	void handleSettingChange(HWND hwnd, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(hwnd);

		if (!isExperimentalSupported())
		{
			return;
		}

		if (IsColorSchemeChangeMessage(lParam))
		{
			g_darkModeEnabled = ShouldAppsUseDarkMode() && !IsHighContrast();
		}
	}

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
	{
		static HTHEME g_menuTheme = nullptr;

		UNREFERENCED_PARAMETER(wParam);
		switch (message)
		{
		case WM_UAHDRAWMENU:
		{
			UAHMENU* pUDM = (UAHMENU*)lParam;
			RECT rc = {};

			// get the menubar rect
			{
				MENUBARINFO mbi = { sizeof(mbi) };
				GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

				RECT rcWindow;
				GetWindowRect(hWnd, &rcWindow);

				// the rcBar is offset by the window rect
				rc = mbi.rcBar;
				OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

				rc.top -= 1;
			}

			FillRect(pUDM->hdc, &rc, NppDarkMode::getDarkerBackgroundBrush());

			*lr = 0;

			return true;
		}
		case WM_UAHDRAWMENUITEM:
		{
			UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

			// get the menu item string
			wchar_t menuString[256] = { '\0' };
			MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
			{
				mii.dwTypeData = menuString;
				mii.cch = (sizeof(menuString) / 2) - 1;

				GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
			}

			// get the item state for drawing

			DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

			int iTextStateID = MPI_NORMAL;
			int iBackgroundStateID = MPI_NORMAL;
			{
				if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT))
				{
					// normal display
					iTextStateID = MPI_NORMAL;
					iBackgroundStateID = MPI_NORMAL;
				}
				if (pUDMI->dis.itemState & ODS_HOTLIGHT)
				{
					// hot tracking
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if (pUDMI->dis.itemState & ODS_SELECTED)
				{
					// clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED))
				{
					// disabled / grey text
					iTextStateID = MPI_DISABLED;
					iBackgroundStateID = MPI_DISABLED;
				}
				if (pUDMI->dis.itemState & ODS_NOACCEL)
				{
					dwFlags |= DT_HIDEPREFIX;
				}
			}

			if (!g_menuTheme)
			{
				g_menuTheme = OpenThemeData(hWnd, L"Menu");
			}

			if (iBackgroundStateID == MPI_NORMAL || iBackgroundStateID == MPI_DISABLED)
			{
				FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getDarkerBackgroundBrush());
			}
			else if (iBackgroundStateID == MPI_HOT || iBackgroundStateID == MPI_DISABLEDHOT)
			{
				FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getHotBackgroundBrush());
			}
			else
			{
				DrawThemeBackground(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
			}
			DTTOPTS dttopts = { sizeof(dttopts) };
			if (iTextStateID == MPI_NORMAL || iTextStateID == MPI_HOT)
			{
				dttopts.dwFlags |= DTT_TEXTCOLOR;
				dttopts.crText = NppDarkMode::getTextColor();
			}
			DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iTextStateID, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &dttopts);

			*lr = 0;

			return true;
		}
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
		MENUBARINFO mbi = { sizeof(mbi) };
		if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
		{
			return;
		}

		RECT rcClient = {};
		GetClientRect(hWnd, &rcClient);
		MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

		RECT rcWindow = {};
		GetWindowRect(hWnd, &rcWindow);

		OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

		// the rcBar is offset by the window rect
		RECT rcAnnoyingLine = rcClient;
		rcAnnoyingLine.bottom = rcAnnoyingLine.top;
		rcAnnoyingLine.top--;


		HDC hdc = GetWindowDC(hWnd);
		FillRect(hdc, &rcAnnoyingLine, NppDarkMode::getDarkerBackgroundBrush());
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

	struct ButtonData
	{
		HTHEME hTheme = nullptr;
		int iStateID = 0;

		~ButtonData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, WC_BUTTON);
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

	void renderButton(HWND hwnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
	{
		RECT rcClient = {};
		WCHAR szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		DWORD uiState = static_cast<DWORD>(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0));
		DWORD nStyle = GetWindowLong(hwnd, GWL_STYLE);

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf = {};
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

		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
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

	void paintButton(HWND hwnd, HDC hdc, ButtonData& buttonData)
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

		BP_ANIMATIONPARAMS animParams = { sizeof(animParams) };
		animParams.style = BPAS_LINEAR;
		if (iStateID != buttonData.iStateID)
		{
			GetThemeTransitionDuration(buttonData.hTheme, iPartID, buttonData.iStateID, iStateID, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		}

		RECT rcClient = {};
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

	LRESULT CALLBACK ButtonSubclass(
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
				RemoveWindowSubclass(hWnd, ButtonSubclass, g_buttonSubclassID);
				delete pButtonData;
				break;
			case WM_ERASEBKGND:
				if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					return TRUE;
				}
				else
				{
					break;
				}
			case WM_THEMECHANGED:
				pButtonData->closeTheme();
				break;
			case WM_PRINTCLIENT:
			case WM_PAINT:
				if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
				{
					PAINTSTRUCT ps = {};
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
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
		SetWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID, pButtonData);
	}

	void paintGroupbox(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		DWORD nStyle = GetWindowLong(hwnd, GWL_STYLE);
		bool isDisabled = (nStyle & WS_DISABLED) == WS_DISABLED;
		int iPartID = BP_GROUPBOX;
		int iStateID = isDisabled ? GBS_DISABLED : GBS_NORMAL;

		RECT rcClient = {};
		GetClientRect(hwnd, &rcClient);

		RECT rcText = rcClient;
		RECT rcBackground = rcClient;

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf = {};
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

		WCHAR szText[256] = { '\0' };
		GetWindowText(hwnd, szText, _countof(szText));

		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool isCenter = (style & BS_CENTER) == BS_CENTER;

		if (szText[0])
		{
			SIZE textSize = {};
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
			SIZE textSize = {};
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

			DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
			dtto.crText = isDisabled ? NppDarkMode::getDisabledTextColor() : NppDarkMode::getTextColor();

			DWORD textFlags = isCenter ? DT_CENTER : DT_LEFT;

			DrawThemeTextEx(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, szText, -1, textFlags | DT_SINGLELINE, &rcText, &dtto);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	constexpr UINT_PTR g_groupboxSubclassID = 42;

	LRESULT CALLBACK GroupboxSubclass(
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
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, GroupboxSubclass, g_groupboxSubclassID);
			delete pButtonData;
			break;
		case WM_ERASEBKGND:
			if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				return TRUE;
			}
			else
			{
				break;
			}
		case WM_THEMECHANGED:
			pButtonData->closeTheme();
			break;
		case WM_PRINTCLIENT:
		case WM_PAINT:
			if (NppDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				PAINTSTRUCT ps = {};
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
			else
			{
				break;
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

	LRESULT CALLBACK TabSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);
		UNREFERENCED_PARAMETER(dwRefData);

		switch (uMsg)
		{
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

			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hWnd, &ps);
			::FillRect(hdc, &ps.rcPaint, NppDarkMode::getDarkerBackgroundBrush());

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
			auto hOldFont = SelectObject(hdc, hFont);

			POINT ptCursor = {};
			::GetCursorPos(&ptCursor);
			ScreenToClient(hWnd, &ptCursor);

			int nTabs = TabCtrl_GetItemCount(hWnd);

			int nSelTab = TabCtrl_GetCurSel(hWnd);
			for (int i = 0; i < nTabs; ++i)
			{
				RECT rcItem = {};
				TabCtrl_GetItemRect(hWnd, i, &rcItem);
				RECT rcFrame = rcItem;

				RECT rcIntersect = {};
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
					// would be better, than getSofterBackgroundBrush(),
					// however default getBackgroundBrush() has same color
					// as getDarkerBackgroundBrush()
					::FillRect(hdc, &rcItem, isSelectedTab ? NppDarkMode::getDarkerBackgroundBrush() : bHot ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getSofterBackgroundBrush());

					SetBkMode(hdc, TRANSPARENT);

					TCHAR label[MAX_PATH]{};
					TCITEM tci = {};
					tci.mask = TCIF_TEXT;
					tci.pszText = label;
					tci.cchTextMax = MAX_PATH - 1;

					::SendMessage(hWnd, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tci));

					auto dpiManager = NppParameters::getInstance()._dpiManager;

					RECT rcText = rcItem;
					rcText.left += dpiManager.scaleX(5);
					rcText.right -= dpiManager.scaleX(3);

					if (isSelectedTab)
					{
						rcText.bottom -= dpiManager.scaleY(4);
						::InflateRect(&rcFrame, 0, 1);
					}
					if (i != nTabs - 1)
					{
						rcFrame.right += 1;
					}

					::FrameRect(hdc, &rcFrame, NppDarkMode::getEdgeBrush());

					DrawText(hdc, label, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

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
			RemoveWindowSubclass(hWnd, TabSubclass, g_tabSubclassID);
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

	constexpr UINT_PTR g_customBorderSubclassID = 42;

	LRESULT CALLBACK CustomBorderSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(dwRefData);

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
				rcClient.right += (2 * ::GetSystemMetrics(SM_CXEDGE));

				auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				bool hasVerScrollbar = (style & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					rcClient.right += ::GetSystemMetrics(SM_CXVSCROLL);
				}

				rcClient.bottom += (2 * ::GetSystemMetrics(SM_CYEDGE));

				bool hasHorScrollbar = (style & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					rcClient.bottom += ::GetSystemMetrics(SM_CXHSCROLL);
				}

				HPEN hPen = ::CreatePen(PS_SOLID, 1, NppDarkMode::getBackgroundColor());
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
				::InflateRect(lpRect, -(::GetSystemMetrics(SM_CXEDGE)), -(::GetSystemMetrics(SM_CYEDGE)));

				auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				bool hasVerScrollbar = (style & WS_VSCROLL) == WS_VSCROLL;
				if (hasVerScrollbar)
				{
					lpRect->right -= ::GetSystemMetrics(SM_CXVSCROLL);
				}

				bool hasHorScrollbar = (style & WS_HSCROLL) == WS_HSCROLL;
				if (hasHorScrollbar)
				{
					lpRect->bottom -= ::GetSystemMetrics(SM_CXHSCROLL);
				}

				return 0;
			}
			break;

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
			}
			break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassCustomBorderForListBoxAndEditControls(HWND hwnd)
	{
		SetWindowSubclass(hwnd, CustomBorderSubclass, g_customBorderSubclassID, 0);
	}

	constexpr UINT_PTR g_comboBoxSubclassID = 42;

	LRESULT CALLBACK ComboBoxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto hwndEdit = reinterpret_cast<HWND>(dwRefData);

		switch (uMsg)
		{
			case WM_PAINT:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				RECT rc = {};
				::GetClientRect(hWnd, &rc);

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);
				
				::SelectObject(hdc, reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0)));
				::SetBkColor(hdc, NppDarkMode::getBackgroundColor());

				auto holdBrush = ::SelectObject(hdc, NppDarkMode::getDarkerBackgroundBrush());

				auto dpiManager = NppParameters::getInstance()._dpiManager;

				RECT rcArrow = {
				rc.right - dpiManager.scaleX(17), rc.top + 1,
				rc.right - 1, rc.bottom - 1
				};

				bool hasFocus = false;

				// CBS_DROPDOWN text is handled by parent by WM_CTLCOLOREDIT
				auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
				if ((style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
				{
					hasFocus = ::GetFocus() == hWnd;

					RECT rcTextBg = rc;
					rcTextBg.left += 1;
					rcTextBg.top += 1;
					rcTextBg.right = rcArrow.left - 1;
					rcTextBg.bottom -= 1;
					::FillRect(hdc, &rcTextBg, NppDarkMode::getBackgroundBrush()); // erase background on item change

					auto index = static_cast<int>(::SendMessage(hWnd, CB_GETCURSEL, 0, 0));
					if (index != CB_ERR)
					{
						::SetTextColor(hdc, NppDarkMode::getTextColor());
						::SetBkColor(hdc, NppDarkMode::getBackgroundColor());
						auto bufferLen = static_cast<size_t>(::SendMessage(hWnd, CB_GETLBTEXTLEN, index, 0));
						TCHAR* buffer = new TCHAR[(bufferLen + 1)];
						::SendMessage(hWnd, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));

						RECT rcText = rc;
						rcText.left += 4;
						rcText.right = rcArrow.left - 5;

						::DrawText(hdc, buffer, -1, &rcText, DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
						delete[]buffer;
					}
				}
				else if ((style & CBS_DROPDOWN) == CBS_DROPDOWN && hwndEdit != NULL)
				{
					hasFocus = ::GetFocus() == hwndEdit;
				}

				POINT ptCursor = {};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				bool isHot = ::PtInRect(&rc, ptCursor);

				bool isWindowEnabled = ::IsWindowEnabled(hWnd) == TRUE;

				auto colorEnabledText = isHot ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor();
				::SetTextColor(hdc, isWindowEnabled ? colorEnabledText : NppDarkMode::getDisabledTextColor());
				::SetBkColor(hdc, isHot ? NppDarkMode::getHotBackgroundColor() : NppDarkMode::getBackgroundColor());
				::ExtTextOut(hdc,
					rcArrow.left + (rcArrow.right - rcArrow.left) / 2 - dpiManager.scaleX(4),
					rcArrow.top + 3,
					ETO_OPAQUE | ETO_CLIPPED,
					&rcArrow, L"˅",
					1,
					nullptr);
				::SetBkColor(hdc, NppDarkMode::getBackgroundColor());

				auto hEnabledPen = (isHot || hasFocus) ? NppDarkMode::getHotEdgePen() : NppDarkMode::getEdgePen();
				auto hSelectedPen = isWindowEnabled ? hEnabledPen : NppDarkMode::getDisabledEdgePen();
				auto holdPen = static_cast<HPEN>(::SelectObject(hdc, hSelectedPen));

				POINT edge[] = {
					{rcArrow.left - 1, rcArrow.top},
					{rcArrow.left - 1, rcArrow.bottom}
				};
				::Polyline(hdc, edge, _countof(edge));

				int roundCornerValue = NppDarkMode::isWindows11() ? dpiManager.scaleX(4) : 0;
				NppDarkMode::paintRoundFrameRect(hdc, rc, hSelectedPen, roundCornerValue, roundCornerValue);

				::SelectObject(hdc, holdPen);
				::SelectObject(hdc, holdBrush);

				::EndPaint(hWnd, &ps);
				return 0;
			}

			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, ComboBoxSubclass, uIdSubclass);
				break;
			}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassComboBoxControl(HWND hwnd)
	{
		DWORD_PTR hwndEditData = 0;
		auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		if ((style & CBS_DROPDOWN) == CBS_DROPDOWN)
		{
			POINT pt = { 5, 5 };
			hwndEditData = reinterpret_cast<DWORD_PTR>(::ChildWindowFromPoint(hwnd, pt));
		}
		SetWindowSubclass(hwnd, ComboBoxSubclass, g_comboBoxSubclassID, hwndEditData);
	}

	constexpr UINT_PTR g_listViewSubclassID = 42;

	LRESULT CALLBACK ListViewSubclass(
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

	void subclassListViewControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, ListViewSubclass, g_listViewSubclassID, 0);
	}

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass, bool theme)
	{
		struct Params
		{
			const wchar_t* themeClassName = nullptr;
			bool subclass = false;
			bool theme = false;
		};

		Params p{
			g_isAtLeastWindows10 && NppDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnableThemeDialogTexture(hwndParent, theme && !NppDarkMode::isEnabled() ? ETDT_ENABLETAB : ETDT_DISABLE);

		EnumChildWindows(hwndParent, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
			auto& p = *reinterpret_cast<Params*>(lParam);
			constexpr size_t classNameLen = 16;
			TCHAR className[classNameLen]{};
			GetClassName(hwnd, className, classNameLen);

			if (wcscmp(className, WC_COMBOBOX) == 0)
			{
				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);

				if ((style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || (style & CBS_DROPDOWN) == CBS_DROPDOWN)
				{
					COMBOBOXINFO cbi = {};
					cbi.cbSize = sizeof(COMBOBOXINFO);
					BOOL result = GetComboBoxInfo(hwnd, &cbi);
					if (result == TRUE)
					{
						if (p.theme && cbi.hwndList)
						{
							//dark scrollbar for listbox of combobox
							SetWindowTheme(cbi.hwndList, p.themeClassName, nullptr);
						}
					}

					NppDarkMode::subclassComboBoxControl(hwnd);
				}
				return TRUE;
			}

			if (wcscmp(className, WC_LISTBOX) == 0)
			{
				if (p.theme)
				{
					//dark scrollbar for listbox
					SetWindowTheme(hwnd, p.themeClassName, nullptr);
				}

				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
				bool isComboBox = (style & LBS_COMBOBOX) == LBS_COMBOBOX;
				auto exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
				bool hasClientEdge = (exStyle & WS_EX_CLIENTEDGE) == WS_EX_CLIENTEDGE;

				if (p.subclass && !isComboBox && hasClientEdge)
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

				return TRUE;
			}

			if (wcscmp(className, WC_EDIT) == 0)
			{
				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
				bool hasScrollBar = ((style & WS_HSCROLL) == WS_HSCROLL) || ((style & WS_VSCROLL) == WS_VSCROLL);
				if (p.theme && hasScrollBar)
				{
					//dark scrollbar for edit control
					SetWindowTheme(hwnd, p.themeClassName, nullptr);
				}

				auto exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
				bool hasClientEdge = (exStyle & WS_EX_CLIENTEDGE) == WS_EX_CLIENTEDGE;
				if (p.subclass && hasClientEdge)
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

				return TRUE;
			}

			if (wcscmp(className, WC_BUTTON) == 0)
			{
				auto nButtonStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE);
				switch (nButtonStyle & BS_TYPEMASK)
				{
					// Plugin might use BS_3STATE and BS_AUTO3STATE button style
					case BS_CHECKBOX:
					case BS_AUTOCHECKBOX:
					case BS_3STATE:
					case BS_AUTO3STATE:
					case BS_RADIOBUTTON:
					case BS_AUTORADIOBUTTON:
					{
						if ((nButtonStyle & BS_PUSHLIKE) == BS_PUSHLIKE)
						{
							if (p.theme)
							{
								SetWindowTheme(hwnd, p.themeClassName, nullptr);
							}
							break;
						}
						if (p.subclass)
						{
							NppDarkMode::subclassButtonControl(hwnd);
						}
						break;
					}
					case BS_GROUPBOX:
					{
						if (p.subclass)
						{
							NppDarkMode::subclassGroupboxControl(hwnd);
						}
						break;
					}
					case BS_DEFPUSHBUTTON:
					case BS_PUSHBUTTON:
					{
						if (p.theme)
						{
							SetWindowTheme(hwnd, p.themeClassName, nullptr);
						}
						break;
					}
				}
				return TRUE;
			}

			if (wcscmp(className, TOOLBARCLASSNAME) == 0)
			{
				NppDarkMode::setDarkLineAbovePanelToolbar(hwnd);
				NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::toolbar);

				return TRUE;
			}

			if (wcscmp(className, WC_LISTVIEW) == 0)
			{
				NppDarkMode::setDarkListView(hwnd);
				NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::listview);

				ListView_SetTextColor(hwnd, NppParameters::getInstance().getCurrentDefaultFgColor());
				ListView_SetTextBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());
				ListView_SetBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());

				if (p.subclass)
				{
					auto exStyle = ListView_GetExtendedListViewStyle(hwnd);
					ListView_SetExtendedListViewStyle(hwnd, exStyle | LVS_EX_DOUBLEBUFFER);
					NppDarkMode::subclassListViewControl(hwnd);
				}

				return TRUE;
			}

			if (wcscmp(className, WC_TREEVIEW) == 0)
			{
				TreeView_SetTextColor(hwnd, NppParameters::getInstance().getCurrentDefaultFgColor());
				TreeView_SetBkColor(hwnd, NppParameters::getInstance().getCurrentDefaultBgColor());

				NppDarkMode::calculateTreeViewStyle();
				NppDarkMode::setTreeViewStyle(hwnd);

				NppDarkMode::setDarkTooltips(hwnd, NppDarkMode::ToolTipsType::treeview);

				return TRUE;
			}

			// Plugin might use rich edit control version 2.0 and later
			if (wcscmp(className, L"RichEdit20W") == 0 || wcscmp(className, L"RICHEDIT50W") == 0)
			{
				if (p.theme)
				{
					//dark scrollbar for rich edit control
					SetWindowTheme(hwnd, p.themeClassName, nullptr);
				}

				return TRUE;
			}

			return TRUE;
		}, reinterpret_cast<LPARAM>(&p));
	}

	void autoThemeChildControls(HWND hwndParent)
	{
		autoSubclassAndThemeChildControls(hwndParent, false, g_isAtLeastWindows10);
	}

	LRESULT darkToolBarNotifyCustomDraw(LPARAM lParam)
	{
		auto nmtbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
		static int roundCornerValue = 0;

		switch (nmtbcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				if (NppDarkMode::isEnabled())
				{
					auto dpiManager = NppParameters::getInstance()._dpiManager;
					roundCornerValue = NppDarkMode::isWindows11() ? dpiManager.scaleX(5) : 0;

					::FillRect(nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, NppDarkMode::getDarkerBackgroundBrush());
					return CDRF_NOTIFYITEMDRAW;
				}
				return CDRF_DODEFAULT;
			}

			case CDDS_ITEMPREPAINT:
			{
				nmtbcd->hbrLines = NppDarkMode::getEdgeBrush();
				nmtbcd->clrText = NppDarkMode::getTextColor();
				nmtbcd->clrTextHighlight = NppDarkMode::getTextColor();
				nmtbcd->clrBtnFace = NppDarkMode::getBackgroundColor();
				nmtbcd->clrBtnHighlight = NppDarkMode::getSofterBackgroundColor();
				nmtbcd->clrHighlightHotTrack = NppDarkMode::getHotBackgroundColor();
				nmtbcd->nStringBkMode = TRANSPARENT;
				nmtbcd->nHLStringBkMode = TRANSPARENT;

				if ((nmtbcd->nmcd.uItemState & CDIS_CHECKED) == CDIS_CHECKED)
				{
					auto holdBrush = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getSofterBackgroundBrush());
					auto holdPen = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getEdgePen());
					::RoundRect(nmtbcd->nmcd.hdc, nmtbcd->nmcd.rc.left, nmtbcd->nmcd.rc.top, nmtbcd->nmcd.rc.right, nmtbcd->nmcd.rc.bottom, roundCornerValue, roundCornerValue);
					::SelectObject(nmtbcd->nmcd.hdc, holdBrush);
					::SelectObject(nmtbcd->nmcd.hdc, holdPen);

					nmtbcd->nmcd.uItemState &= ~CDIS_CHECKED;
				}

				return TBCDRF_HILITEHOTTRACK | TBCDRF_USECDCOLORS | CDRF_NOTIFYPOSTPAINT;
			}

			case CDDS_ITEMPOSTPAINT:
			{
				bool isDropDown = false;

				auto exStyle = ::SendMessage(nmtbcd->nmcd.hdr.hwndFrom, TB_GETEXTENDEDSTYLE, 0, 0);
				if ((exStyle & TBSTYLE_EX_DRAWDDARROWS) == TBSTYLE_EX_DRAWDDARROWS)
				{
					TBBUTTONINFO tbButtonInfo{};
					tbButtonInfo.cbSize = sizeof(TBBUTTONINFO);
					tbButtonInfo.dwMask = TBIF_STYLE;
					::SendMessage(nmtbcd->nmcd.hdr.hwndFrom, TB_GETBUTTONINFO, nmtbcd->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&tbButtonInfo));

					isDropDown = (tbButtonInfo.fsStyle & BTNS_DROPDOWN) == BTNS_DROPDOWN;
				}

				if ( !isDropDown && (nmtbcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
				{
					NppDarkMode::paintRoundFrameRect(nmtbcd->nmcd.hdc, nmtbcd->nmcd.rc, NppDarkMode::getHotEdgePen(), roundCornerValue, roundCornerValue);
				}

				return CDRF_DODEFAULT;
			}

			default:
				return CDRF_DODEFAULT;
		}
	}

	LRESULT darkListViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool isPlugin)
	{
		auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

		switch (lplvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				return CDRF_NOTIFYITEMDRAW;
			}

			case CDDS_ITEMPREPAINT:
			{
				auto isSelected = ListView_GetItemState(lplvcd->nmcd.hdr.hwndFrom, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;

				if (NppDarkMode::isEnabled())
				{
					if (isSelected)
					{
						lplvcd->clrText = NppDarkMode::getTextColor();
						lplvcd->clrTextBk = NppDarkMode::getSofterBackgroundColor();

						::FillRect(lplvcd->nmcd.hdc, &lplvcd->nmcd.rc, NppDarkMode::getSofterBackgroundBrush());
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

				LRESULT lr = CDRF_DODEFAULT;

				if (isPlugin)
				{
					lr = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				return lr | CDRF_NEWFONT;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT darkTreeViewNotifyCustomDraw(LPARAM lParam)
	{
		auto lptvcd = reinterpret_cast<LPNMTVCUSTOMDRAW>(lParam);

		switch (lptvcd->nmcd.dwDrawStage)
		{
			case CDDS_PREPAINT:
			{
				if (NppDarkMode::isEnabled())
				{
					return CDRF_NOTIFYITEMDRAW;
				}
				return CDRF_DODEFAULT;
			}

			case CDDS_ITEMPREPAINT:
			{
				if ((lptvcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
				{
					lptvcd->clrText = NppDarkMode::getTextColor();
					lptvcd->clrTextBk = NppDarkMode::getSofterBackgroundColor();
					::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, NppDarkMode::getSofterBackgroundBrush());

					return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
				}

				if ((lptvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
				{
					lptvcd->clrText = NppDarkMode::getTextColor();
					lptvcd->clrTextBk = NppDarkMode::getHotBackgroundColor();

					auto notifyResult =  CDRF_DODEFAULT;
					if (g_isAtLeastWindows10 || g_treeViewStyle == TreeViewStyle::light)
					{
						::FillRect(lptvcd->nmcd.hdc, &lptvcd->nmcd.rc, NppDarkMode::getHotBackgroundBrush());
						notifyResult = CDRF_NOTIFYPOSTPAINT;
					}

					return CDRF_NEWFONT | notifyResult;
				}

				return CDRF_DODEFAULT;
			}

			case CDDS_ITEMPOSTPAINT:
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

				return CDRF_DODEFAULT;
				
			}

			default:
				return CDRF_DODEFAULT;
		}
	}

	constexpr UINT_PTR g_pluginDockWindowSubclassID = 42;

	LRESULT CALLBACK PluginDockWindowSubclass(
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
					RECT rect = {};
					GetClientRect(hWnd, &rect);
					::FillRect(reinterpret_cast<HDC>(wParam), &rect, NppDarkMode::getDarkerBackgroundBrush());
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
				return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
			}

			case WM_CTLCOLORLISTBOX:
			{
				return NppDarkMode::onCtlColorListbox(wParam, lParam);
			}

			case WM_CTLCOLORDLG:
			{
				return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
			}

			case WM_CTLCOLORSTATIC:
			{
				if (NppDarkMode::isEnabled())
				{
					constexpr size_t classNameLen = 16;
					TCHAR className[classNameLen]{};
					auto hwndEdit = reinterpret_cast<HWND>(lParam);
					GetClassName(hwndEdit, className, classNameLen);
					if (wcscmp(className, WC_EDIT) == 0)
					{
						return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
					}
					return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
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
				auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);

				constexpr size_t classNameLen = 16;
				TCHAR className[classNameLen]{};
				GetClassName(nmhdr->hwndFrom, className, classNameLen);

				switch (nmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						if (wcscmp(className, TOOLBARCLASSNAME) == 0)
						{
							return NppDarkMode::darkToolBarNotifyCustomDraw(lParam);
						}
								
						if (wcscmp(className, WC_LISTVIEW) == 0)
						{
							return NppDarkMode::darkListViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, true);
						}
								
						if (wcscmp(className, WC_TREEVIEW) == 0)
						{
							return NppDarkMode::darkTreeViewNotifyCustomDraw(lParam);
						}
					}
					break;
				}
				break;
			}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassAndThemePluginDockWindow(HWND hwnd)
	{
		SetWindowSubclass(hwnd, PluginDockWindowSubclass, g_pluginDockWindowSubclassID, 0);
		NppDarkMode::autoSubclassAndThemeChildControls(hwnd, true, g_isAtLeastWindows10);
	}

	constexpr UINT_PTR g_windowNotifySubclassID = 42;

	LRESULT CALLBACK WindowNotifySubclass(
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
				::RemoveWindowSubclass(hWnd, PluginDockWindowSubclass, uIdSubclass);
				break;
			}

			case WM_NOTIFY:
			{
				auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);

				constexpr size_t classNameLen = 16;
				TCHAR className[classNameLen]{};
				GetClassName(nmhdr->hwndFrom, className, classNameLen);

				switch (nmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						if (wcscmp(className, TOOLBARCLASSNAME) == 0)
						{
							return NppDarkMode::darkToolBarNotifyCustomDraw(lParam);
						}
								
						if (wcscmp(className, WC_LISTVIEW) == 0)
						{
							return NppDarkMode::darkListViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam, false);
						}
								
						if (wcscmp(className, WC_TREEVIEW) == 0)
						{
							return NppDarkMode::darkTreeViewNotifyCustomDraw(lParam);
						}
					}
					break;
				}
				break;
			}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void autoSubclassAndThemeWindowNotify(HWND hwnd)
	{
		SetWindowSubclass(hwnd, WindowNotifySubclass, g_windowNotifySubclassID, 0);
	}

	constexpr UINT_PTR g_tabUpDownSubclassID = 42;

	LRESULT CALLBACK TabUpDownSubclass(
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

				bool hasTheme = pButtonData->ensureTheme(hWnd);

				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);

				PAINTSTRUCT ps{};
				auto hdc = ::BeginPaint(hWnd, &ps);

				::FillRect(hdc, &rcClient, NppDarkMode::getDarkerBackgroundBrush());

				auto dpiManager = NppParameters::getInstance()._dpiManager;

				RECT rcArrowLeft = {
					rcClient.left, rcClient.top,
					rcClient.right - ((rcClient.right - rcClient.left) / 2) , rcClient.bottom
				};

				RECT rcArrowRight = {
					rcArrowLeft.right, rcClient.top,
					rcClient.right, rcClient.bottom
				};

				POINT ptCursor = {};
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWnd, &ptCursor);

				bool isHotLeft = ::PtInRect(&rcArrowLeft, ptCursor);
				bool isHotRight = ::PtInRect(&rcArrowRight, ptCursor);

				::SetBkMode(hdc, TRANSPARENT);

				if (hasTheme)
				{
					::DrawThemeBackground(pButtonData->hTheme, hdc, BP_PUSHBUTTON, isHotLeft ? PBS_HOT : PBS_NORMAL, &rcArrowLeft, nullptr);
					::DrawThemeBackground(pButtonData->hTheme, hdc, BP_PUSHBUTTON, isHotRight ? PBS_HOT : PBS_NORMAL, &rcArrowRight, nullptr);
				}
				else
				{
					::FillRect(hdc, &rcArrowLeft, isHotLeft ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getBackgroundBrush());
					::FillRect(hdc, &rcArrowRight, isHotRight ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getBackgroundBrush());
				}

				LOGFONT lf = {};
				auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
				::GetObject(font, sizeof(lf), &lf);
				lf.lfHeight = (dpiManager.scaleY(16) - 5) * -1;
				auto holdFont = static_cast<HFONT>(::SelectObject(hdc, CreateFontIndirect(&lf)));

				auto mPosX = ((rcArrowLeft.right - rcArrowLeft.left - dpiManager.scaleX(7) + 1) / 2);
				auto mPosY = ((rcArrowLeft.bottom - rcArrowLeft.top + lf.lfHeight - dpiManager.scaleY(1) - 3) / 2);

				::SetTextColor(hdc, isHotLeft ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
				::ExtTextOut(hdc,
					rcArrowLeft.left + mPosX,
					rcArrowLeft.top + mPosY,
					ETO_CLIPPED,
					&rcArrowLeft, L"<",
					1,
					nullptr);

				::SetTextColor(hdc, isHotRight ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
				::ExtTextOut(hdc,
					rcArrowRight.left + mPosX - dpiManager.scaleX(2) + 3,
					rcArrowRight.top + mPosY,
					ETO_CLIPPED,
					&rcArrowRight, L">",
					1,
					nullptr);

				if (!hasTheme)
				{
					NppDarkMode::paintRoundFrameRect(hdc, rcArrowLeft, NppDarkMode::getEdgePen());
					NppDarkMode::paintRoundFrameRect(hdc, rcArrowRight, NppDarkMode::getEdgePen());
				}

				::SelectObject(hdc, holdFont);
				::EndPaint(hWnd, &ps);
				return FALSE;
			}

			case WM_THEMECHANGED:
			{
				pButtonData->closeTheme();
				break;
			}

			case WM_NCDESTROY:
			{
				::RemoveWindowSubclass(hWnd, TabUpDownSubclass, uIdSubclass);
				delete pButtonData;
				break;
			}

			case WM_ERASEBKGND:
			{
				if (NppDarkMode::isEnabled())
				{
					RECT rcClient{};
					::GetClientRect(hWnd, &rcClient);
					::FillRect(reinterpret_cast<HDC>(wParam), &rcClient, NppDarkMode::getDarkerBackgroundBrush());
					return TRUE;
				}
				break;
			}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	bool subclassTabUpDownControl(HWND hwnd)
	{
		constexpr size_t classNameLen = 16;
		TCHAR className[classNameLen]{};
		GetClassName(hwnd, className, classNameLen);
		if (wcscmp(className, UPDOWN_CLASS) == 0)
		{
			auto pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
			SetWindowSubclass(hwnd, TabUpDownSubclass, g_tabUpDownSubclassID, pButtonData);
			NppDarkMode::setDarkExplorerTheme(hwnd);
			return true;
		}

		return false;
	}

	void setDarkTitleBar(HWND hwnd)
	{
		NppDarkMode::allowDarkModeForWindow(hwnd, NppDarkMode::isEnabled());
		NppDarkMode::setTitleBarThemeColor(hwnd);
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
			default:
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
			scheme.clrBtnHighlight = NppDarkMode::getDarkerBackgroundColor();
			scheme.clrBtnShadow = NppDarkMode::getDarkerBackgroundColor();
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

		if (g_treeViewBg != bgColor || g_lighnessTreeView == 50.0)
		{
			g_lighnessTreeView = calculatePerceivedLighness(bgColor);
			g_treeViewBg = bgColor;
		}

		if (g_lighnessTreeView < (50.0 - g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::dark;
		}
		else if (g_lighnessTreeView > (50.0 + g_middleGrayRange))
		{
			g_treeViewStyle = TreeViewStyle::light;
		}
		else
		{
			g_treeViewStyle = TreeViewStyle::classic;
		}
	}

	void setTreeViewStyle(HWND hwnd)
	{
		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool hasHotStyle = (style & TVS_TRACKSELECT) == TVS_TRACKSELECT;
		bool change = false;
		switch (g_treeViewStyle)
		{
			case TreeViewStyle::light:
			{
				if (!hasHotStyle)
				{
					style |= TVS_TRACKSELECT;
					change = true;
				}
				SetWindowTheme(hwnd, L"Explorer", nullptr);
				break;
			}
			case TreeViewStyle::dark:
			{
				if (!hasHotStyle)
				{
					style |= TVS_TRACKSELECT;
					change = true;
				}
				SetWindowTheme(hwnd, g_isAtLeastWindows10 ? L"DarkMode_Explorer" : nullptr, nullptr);
				break;
			}
			default:
			{
				if (hasHotStyle)
				{
					style &= ~TVS_TRACKSELECT;
					change = true;
				}
				SetWindowTheme(hwnd, nullptr, nullptr);
				break;
			}
		}

		if (change)
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
		}
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

	BOOL CALLBACK enumAutocompleteProc(HWND hwnd, LPARAM /*lParam*/)
	{
		constexpr size_t classNameLen = 16;
		TCHAR className[classNameLen]{};
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
		::EnumThreadWindows(::GetCurrentThreadId(), (WNDENUMPROC)enumAutocompleteProc, 0);
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

	LRESULT onCtlColorSofter(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getSofterBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getSofterBackgroundBrush());
	}

	LRESULT onCtlColorDarker(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getDarkerBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getDarkerBackgroundBrush());
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
	
	LRESULT onCtlColorDarkerBGStaticText(HDC hdc, bool isTextEnabled)
	{
		if (!NppDarkMode::isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
			return FALSE;
		}

		::SetTextColor(hdc, isTextEnabled ? NppDarkMode::getTextColor() : NppDarkMode::getDisabledTextColor());
		::SetBkColor(hdc, NppDarkMode::getDarkerBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getDarkerBackgroundBrush());
	}

	INT_PTR onCtlColorListbox(WPARAM wParam, LPARAM lParam)
	{
		auto hdc = reinterpret_cast<HDC>(wParam);
		auto hwnd = reinterpret_cast<HWND>(lParam);

		auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		bool isComboBox = (style & LBS_COMBOBOX) == LBS_COMBOBOX;
		if (!isComboBox && ::IsWindowEnabled(hwnd))
		{
			return static_cast<INT_PTR>(NppDarkMode::onCtlColorSofter(hdc));
		}
		return static_cast<INT_PTR>(NppDarkMode::onCtlColor(hdc));
	}
}
