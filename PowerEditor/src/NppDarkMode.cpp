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
#include "ColourPicker.h"

#include <Shlwapi.h>

#ifdef __GNUC__
#include <cmath>
#define WINAPI_LAMBDA WINAPI
#else
#define WINAPI_LAMBDA
#endif

#pragma comment(lib, "uxtheme.lib")

#define BKLUMINANCE_BRIGHTER 140
#define BKLUMINANCE_SOFTER 80
#define TEXTLUMINANCE_MAX 240
#define EDGELUMINANCE_BRIGHTER 220
#define EDGELUMINANCE_DARKER 60

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

namespace NppDarkMode
{
	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH softerBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH pureBackground = nullptr;
		HBRUSH errorBackground = nullptr;
		HBRUSH hardlightBackground = nullptr;
		HBRUSH softlightBackground = nullptr;
		HBRUSH textColorBrush = nullptr;
		HBRUSH darkerTextColorBrush = nullptr;
		HBRUSH edgeBrush = nullptr;
		HBRUSH hotEdgeBrush = nullptr;
		HBRUSH disabledEdgeBrush = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, softerBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, pureBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))
			, hardlightBackground(::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_BRIGHTER)))
			, softlightBackground(::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_SOFTER)))
			, textColorBrush(::CreateSolidBrush(colors.text))
			, darkerTextColorBrush(::CreateSolidBrush(colors.darkerText))
			, edgeBrush(::CreateSolidBrush(colors.edge))
			, hotEdgeBrush(::CreateSolidBrush(colors.hotEdge))
			, disabledEdgeBrush(::CreateSolidBrush(lightColor(colors.edge, EDGELUMINANCE_DARKER)))
		{}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(softerBackground);	softerBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(pureBackground);		pureBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;
			::DeleteObject(hardlightBackground);	hardlightBackground = nullptr;
			::DeleteObject(softlightBackground);	softlightBackground = nullptr;
			::DeleteObject(textColorBrush);			textColorBrush = nullptr;
			::DeleteObject(darkerTextColorBrush);	darkerTextColorBrush = nullptr;
			::DeleteObject(edgeBrush);				edgeBrush = nullptr;
			::DeleteObject(hotEdgeBrush);			hotEdgeBrush = nullptr;
			::DeleteObject(disabledEdgeBrush);		disabledEdgeBrush = nullptr;

		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(softerBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(pureBackground);
			::DeleteObject(errorBackground);
			::DeleteObject(hardlightBackground);
			::DeleteObject(softlightBackground);
			::DeleteObject(textColorBrush);
			::DeleteObject(darkerTextColorBrush);
			::DeleteObject(edgeBrush);
			::DeleteObject(hotEdgeBrush);
			::DeleteObject(disabledEdgeBrush);

			background = ::CreateSolidBrush(colors.background);
			softerBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			pureBackground = ::CreateSolidBrush(colors.pureBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);
			hardlightBackground = ::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_BRIGHTER));
			softlightBackground = ::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_SOFTER));
			textColorBrush = ::CreateSolidBrush(colors.text);
			darkerTextColorBrush = ::CreateSolidBrush(colors.darkerText);
			edgeBrush = ::CreateSolidBrush(colors.edge);
			hotEdgeBrush = ::CreateSolidBrush(colors.hotEdge);
			disabledEdgeBrush = ::CreateSolidBrush(lightColor(colors.edge, EDGELUMINANCE_DARKER));
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
			, disabledEdgePen(::CreatePen(PS_SOLID, 1, lightColor(colors.edge, EDGELUMINANCE_DARKER)))
		{}

		~Pens()
		{
			::DeleteObject(darkerTextPen);	    darkerTextPen = nullptr;
			::DeleteObject(edgePen);		    edgePen = nullptr;
			::DeleteObject(hotEdgePen);	        hotEdgePen = nullptr;
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
			disabledEdgePen = ::CreatePen(PS_SOLID, 1, lightColor(colors.edge, EDGELUMINANCE_DARKER));
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
		HEXRGB(0x9B9B9B)	// hotEdgeColor
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
		HEXRGB(0xBBABAB)	// hotEdgeColor
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
		HEXRGB(0xABBBAB)	// hotEdgeColor
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
		HEXRGB(0xABABCB)	// hotEdgeColor
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
		HEXRGB(0xBBABCB)	// hotEdgeColor
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
		HEXRGB(0xBBBBCB)	// hotEdgeColor
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
		HEXRGB(0xBBBBAB)	// hotEdgeColor
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
		HEXRGB(0x9B9B9B)	// hotEdgeColor
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

		g_colorToneChoice = nppGui._darkmode._colorTone;
		tCustom.change(nppGui._darkmode._customColors);

		return opt;
	}

	void initDarkMode()
	{
		_options = configuredOptions();

		initExperimentalDarkMode();
		setDarkMode(_options.enable, true);
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

	COLORREF lightColor(COLORREF color, WORD luminance)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(color, &h, &l, &s);

		l = luminance;

		COLORREF newColor = ColorHLSToRGB(h, l, s);

		return newColor;
	}

	TreeViewStyle treeViewStyle = TreeViewStyle::classic;
	COLORREF treeViewBg = NppParameters::getInstance().getCurrentDefaultBgColor();
	double lighnessTreeView = 50.0;

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
	COLORREF getHotTextColor()            { return lightColor(getTheme()._colors.text, TEXTLUMINANCE_MAX); }
	COLORREF getDarkerTextColor()         { return getTheme()._colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme()._colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme()._colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme()._colors.edge; }
	COLORREF getHotEdgeColor()            { return getTheme()._colors.hotEdge; }

	HBRUSH getBackgroundBrush()          { return getTheme()._brushes.background; }
	HBRUSH getSofterBackgroundBrush()    { return getTheme()._brushes.softerBackground; }
	HBRUSH getHotBackgroundBrush()       { return getTheme()._brushes.hotBackground; }
	HBRUSH getDarkerBackgroundBrush()    { return getTheme()._brushes.pureBackground; }
	HBRUSH getErrorBackgroundBrush()     { return getTheme()._brushes.errorBackground; }
	HBRUSH getHardlightBackgroundBrush() { return getTheme()._brushes.hardlightBackground; }
	HBRUSH getSoftlightBackgroundBrush() { return getTheme()._brushes.softlightBackground; }
	HBRUSH getTextBrush()				 { return getTheme()._brushes.textColorBrush; }
	HBRUSH getDarkerTextBrush()			 { return getTheme()._brushes.darkerTextColorBrush; }
	HBRUSH getEdgeBrush()				 { return getTheme()._brushes.edgeBrush; }
	HBRUSH getHotEdgeBrush()			 { return getTheme()._brushes.hotEdgeBrush; }
	HBRUSH getDisabledEdgeBrush()		 { return getTheme()._brushes.disabledEdgeBrush; }

	HPEN getDarkerTextPen()               { return getTheme()._pens.darkerTextPen; }
	HPEN getEdgePen()                     { return getTheme()._pens.edgePen; }
	HPEN getHotEdgePen()				  { return getTheme()._pens.hotEdgePen; }
	HPEN getDisabledEdgePen()			  { return getTheme()._pens.disabledEdgePen; }

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

	enum class SpinButtonType { LeftRight, UpDown };
	enum class SpinArrowDirection { Left, Right, Up, Down };

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

	struct SpinData
	{
		bool isInitialized = false;
		HTHEME hTheme = nullptr;

		SpinButtonType type = SpinButtonType::LeftRight;

		int iStateID1 = 0;
		int iStateID2 = 0;
		bool isHotOne = false;
		bool isHotTwo = false;
		RECT rcClient = {}, rcArrowOne = {}, rcArrowTwo = {};
		~SpinData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, L"Spin");
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

		void updateSize(HWND hwnd)
		{
			GetClientRect(hwnd, &rcClient);
			if (rcClient.right - rcClient.left > rcClient.bottom - rcClient.top)
				type = SpinButtonType::LeftRight;
			else
				type = SpinButtonType::UpDown;

			if (type == SpinButtonType::LeftRight)
			{
				// Control has a small margin depending on orientation.
				rcClient.top += 1;
				rcClient.left += 1;
				rcClient.right -= 1;

				rcArrowOne = {
					rcClient.left, rcClient.top,
					rcClient.right - ((rcClient.right - rcClient.left) / 2) , rcClient.bottom
				};
				rcArrowTwo = {
					rcArrowOne.right, rcClient.top,
					rcClient.right, rcClient.bottom
				};
			}
			else
			{
				// Control has a small margin depending on orientation.
				InflateRect(&rcClient, -1, -1);

				rcArrowOne = {
					rcClient.left, rcClient.top,
					rcClient.right, rcClient.bottom - ((rcClient.bottom - rcClient.top) / 2)
				};
				rcArrowTwo = {
					rcClient.left, rcArrowOne.bottom,
					rcClient.right, rcClient.bottom
				};
			}
		}
	};

	// Draws a BS_PUSBUTON or DEF_PUSHBUTTON or Checkbox with BS_PUSHLIKE control background
	// nState is the same as static_cast<DWORD>(SendMessage(hwndButton, BM_GETSTATE, 0, 0));
	// nStyle is the same as GetWindowLongPtr(hwndButton, GWL_STYLE);
	void renderButtonBackground(HDC hdc, DWORD nState, LONG_PTR nStyle, const RECT& rcClient)
	{
		HBRUSH hBckBrush = ((nState & BST_HOT) != 0) ? NppDarkMode::getSoftlightBackgroundBrush() : NppDarkMode::getSofterBackgroundBrush();
		if ((nState & BST_PUSHED) != 0 || ((nState & BST_CHECKED) != 0))
			hBckBrush = NppDarkMode::getBackgroundBrush();

		HPEN hOldPen = nullptr;
		if ((nStyle & WS_DISABLED) != 0 || ((nState & BST_CHECKED) != 0))
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, NppDarkMode::getDisabledEdgePen()));
		else if ((nState & (BST_FOCUS) | (nState & BST_HOT)) || ((nStyle & BS_DEFPUSHBUTTON) && !(nStyle & BS_PUSHLIKE)))
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, NppDarkMode::getHotEdgePen()));
		else
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, NppDarkMode::getEdgePen()));

		FillRect(hdc, &rcClient, NppDarkMode::getBackgroundBrush());
		HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hBckBrush));
		if (isWindows11())
			RoundRect(hdc, rcClient.left, rcClient.top, rcClient.right,
				rcClient.bottom, NppParameters::getInstance()._dpiManager.scaleX(5), NppParameters::getInstance()._dpiManager.scaleY(5));
		else
			Rectangle(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

		SelectObject(hdc, hOldBrush);

		if (hOldPen)
			SelectObject(hdc, hOldPen);
	}

	void renderButtonImage(HWND hwndButton, HDC hdc, DWORD nState, LONG_PTR nStyle,
		RECT& rcClient, RECT& rcText, WCHAR* szText, DWORD& dtFlags)
	{
		ICONINFO ii;
		BITMAP bm;

		HICON hIcon = reinterpret_cast<HICON>(SendMessage(hwndButton, BM_GETIMAGE, IMAGE_ICON, 0));
		HBITMAP hBitmap = reinterpret_cast<HBITMAP>(hIcon); // BM_GETIMAGE returns the same handler for IMAGE_ICON and IMAGE_BITMAP.
		BOOL bIcon = GetIconInfo(hIcon, &ii);
		BOOL bBitmap = GetObject(hBitmap, sizeof(bm), &bm);

		// Calculate final rectangle based on current drawed text size + image size + padding
		const int padding = NppParameters::getInstance()._dpiManager.scaleX(8);
		RECT rcImageAndText = {};

		// Get text width
		DrawText(hdc, szText, static_cast<int>(std::wstring(szText).size()), &rcImageAndText, DT_CALCRECT);
		SIZE szImageText = { rcImageAndText.right, rcImageAndText.bottom };
		int imageCx = 0;

		// Center final text vertically;
		rcImageAndText.top = (rcClient.bottom - rcImageAndText.bottom) / 2;

		// Calculate if image is standalone or with text
		bool bStandalone = ((nStyle & BS_BITMAP) != 0) || ((nStyle & BS_ICON) != 0) || (szText[0] == '\0');

		if (bIcon)
		{
			POINT pxIcon = {};
			imageCx = ii.xHotspot * 2;
			// Centralize final text + image + padding together horizontally
			rcImageAndText.left = (rcClient.right - szImageText.cx - imageCx - padding) / 2;
			// Centralize image horizontally and vertically
			pxIcon.x = bStandalone ? (rcClient.right - imageCx) / 2 : rcImageAndText.left;
			pxIcon.y = (rcClient.bottom - (ii.yHotspot * 2)) / 2;
			if (nState & BST_PUSHED)
			{
				pxIcon.x += NppParameters::getInstance()._dpiManager.scaleX(1);
				pxIcon.y += NppParameters::getInstance()._dpiManager.scaleY(1);
			}
			DrawIconEx(hdc, pxIcon.x, pxIcon.y, hIcon, ii.xHotspot * 2, ii.yHotspot * 2, 0, NULL, DI_NORMAL);
		}

		if (bBitmap)
		{
			POINT pxBmp = {};
			imageCx = bm.bmWidth;
			// Centralize final text + image + padding together horizontally
			rcImageAndText.left = (rcClient.right - szImageText.cx - imageCx - padding) / 2;
			// Centralize image horizontally and vertically
			pxBmp.x = bStandalone ? (rcClient.right - imageCx) / 2 : rcImageAndText.left;
			pxBmp.y = (rcClient.bottom - bm.bmHeight) / 2;
			if (nState & BST_PUSHED)
			{
				pxBmp.x += NppParameters::getInstance()._dpiManager.scaleX(1);
				pxBmp.y += NppParameters::getInstance()._dpiManager.scaleY(1);
			}

			HDC memDC = CreateCompatibleDC(hdc);
			HBITMAP oldBmp = reinterpret_cast<HBITMAP>(SelectObject(memDC, hBitmap));
			if (bm.bmBitsPixel == 32)
			{
				BLENDFUNCTION bf1;
				bf1.BlendOp = AC_SRC_OVER;
				bf1.BlendFlags = 0;
				bf1.SourceConstantAlpha = 0xff;
				bf1.AlphaFormat = AC_SRC_ALPHA;
				GdiAlphaBlend(hdc, pxBmp.x, pxBmp.y, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, bf1);
			}
			else
				BitBlt(hdc, pxBmp.x, pxBmp.y, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);

			SelectObject(memDC, oldBmp);
			DeleteDC(memDC);
		}

		// Text position is now centered but added the image size + a space padding 
		// (remove horizontal alignment flags because we already calculated them).
		if (bIcon || bBitmap)
		{
			rcText.left = rcImageAndText.left + imageCx + padding;
			rcText.right = rcText.left + szImageText.cx;
			dtFlags &= ~(DT_CENTER | DT_RIGHT); // DT_LEFT = 0;
		}
	}

	void renderButton(HWND hwndButton, HDC hdc, HTHEME hTheme)
	{
		RECT rcClient = {};
		WCHAR szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwndButton, BM_GETSTATE, 0, 0));
		LONG_PTR nStyle = GetWindowLongPtr(hwndButton, GWL_STYLE);
		DWORD uiState = static_cast<DWORD>(SendMessage(hwndButton, WM_QUERYUISTATE, 0, 0));

		GetClientRect(hwndButton, &rcClient);
		GetWindowText(hwndButton, szText, _countof(szText));

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;

		renderButtonBackground(hdc, nState, nStyle, rcClient);

		// Prepare to draw button image
		RECT rcText = rcClient;
		InflateRect(&rcText, -3, -3);

		DWORD dtFlags = DT_LEFT; // DT_LEFT is 0
		dtFlags |= (nStyle & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
		dtFlags |= ((nStyle & BS_CENTER) == BS_CENTER) ? DT_CENTER : (nStyle & BS_RIGHT) ? DT_RIGHT : 0;
		dtFlags |= ((nStyle & BS_VCENTER) == BS_VCENTER) ? DT_VCENTER : (nStyle & BS_BOTTOM) ? DT_BOTTOM : 0;
		dtFlags |= (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		// Modifications to DrawThemeText
		dtFlags &= ~(DT_RIGHT);
		dtFlags |= DT_VCENTER | DT_CENTER;

		// Calculate actual text output rectangle and centralize with image
		hFont = reinterpret_cast<HFONT>(SendMessage(hwndButton, WM_GETFONT, 0, 0));
		hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

		// Draw button image
		renderButtonImage(hwndButton, hdc, nState, nStyle, rcClient, rcText, szText, dtFlags);

		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
		dtto.crText = NppDarkMode::getTextColor();

		if (nStyle & WS_DISABLED)
		{
			dtto.crText = NppDarkMode::getDisabledTextColor();
		}

		if (nState & BST_PUSHED)
		{
			rcText.left += NppParameters::getInstance()._dpiManager.scaleX(1);
			rcText.right += NppParameters::getInstance()._dpiManager.scaleX(1);
			rcText.top += NppParameters::getInstance()._dpiManager.scaleY(1);
			rcText.bottom += NppParameters::getInstance()._dpiManager.scaleY(1);
		}

		int iStateID = PBS_NORMAL;
		if (nStyle & WS_DISABLED)				iStateID = PBS_DISABLED;
		else if (nState & BST_PUSHED)			iStateID = PBS_PRESSED;
		else if (nState & BST_HOT)				iStateID = PBS_HOT;
		else if (nStyle & BS_DEFPUSHBUTTON)		iStateID = PBS_DEFAULTED;

		DrawThemeTextEx(hTheme, hdc, BP_PUSHBUTTON, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if ((nState & BST_FOCUS) && !(uiState & UISF_HIDEFOCUS))
		{
			rcClient.left += NppParameters::getInstance()._dpiManager.scaleX(2); rcClient.right -= NppParameters::getInstance()._dpiManager.scaleX(2);
			rcClient.top += NppParameters::getInstance()._dpiManager.scaleY(2); rcClient.bottom -= NppParameters::getInstance()._dpiManager.scaleY(2);
			DrawFocusRect(hdc, &rcClient);
		}

		SelectObject(hdc, hOldFont);
	}

	void renderCheckboxOrRadioButton(HWND hwnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
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
		LONG_PTR nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		DWORD nButtonStyle = nStyle & 0xF;

		int iPartID = 0;
		if (nButtonStyle == BS_PUSHBUTTON || nButtonStyle == BS_DEFPUSHBUTTON || (nStyle & BS_PUSHLIKE) > 0)
		{
			iPartID = BP_PUSHBUTTON;
		}
		else if (nButtonStyle == BS_RADIOBUTTON || nButtonStyle == BS_AUTORADIOBUTTON)
		{
			iPartID = BP_RADIOBUTTON;
		}
		else if (nButtonStyle == BS_CHECKBOX || nButtonStyle == BS_AUTOCHECKBOX)
		{
			iPartID = BP_CHECKBOX;
		}
		else
		{
			assert(false);
		}

		// states of BP_CHECKBOX, BP_RADIOBUTTON and BP_PUSHBUTTON are the same
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
				if (iPartID == BP_PUSHBUTTON)
					renderButton(hwnd, hdcFrom, buttonData.hTheme);
				else
					renderCheckboxOrRadioButton(hwnd, hdcFrom, buttonData.hTheme, iPartID, buttonData.iStateID);
			}
			if (hdcTo)
			{
				if (iPartID == BP_PUSHBUTTON)
					renderButton(hwnd, hdcTo, buttonData.hTheme);
				else
					renderCheckboxOrRadioButton(hwnd, hdcTo, buttonData.hTheme, iPartID, iStateID);
			}

			buttonData.iStateID = iStateID;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			if (iPartID == BP_PUSHBUTTON)
				renderButton(hwnd, hdc, buttonData.hTheme);
			else
				renderCheckboxOrRadioButton(hwnd, hdc, buttonData.hTheme, iPartID, iStateID);

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
		int iPartID = BP_GROUPBOX;
		int iStateID = GBS_NORMAL;

		if (nStyle & WS_DISABLED)
		{
			iStateID = GBS_DISABLED;
		}

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
		DrawThemeBackground(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, nullptr);

		SelectClipRgn(hdc, nullptr);

		if (szText[0])
		{
			rcText.right -= 2;
			rcText.left += 2;

			DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
			dtto.crText = NppDarkMode::getTextColor();

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

			RECT rcArrow = {
			rc.right - NppParameters::getInstance()._dpiManager.scaleX(17), rc.top + 1,
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

			::SetTextColor(hdc, isHot ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());
			::SetBkColor(hdc, isHot ? NppDarkMode::getHotBackgroundColor() : NppDarkMode::getBackgroundColor());
			::ExtTextOut(hdc,
				rcArrow.left + (rcArrow.right - rcArrow.left) / 2 - NppParameters::getInstance()._dpiManager.scaleX(4),
				rcArrow.top + 3,
				ETO_OPAQUE | ETO_CLIPPED,
				&rcArrow, L"˅",
				1,
				nullptr);
			::SetBkColor(hdc, NppDarkMode::getBackgroundColor());

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, isHot || hasFocus ? NppDarkMode::getHotEdgePen() : NppDarkMode::getEdgePen()));
			POINT edge[] = {
				{rcArrow.left - 1, rcArrow.top},
				{rcArrow.left - 1, rcArrow.bottom}
			};
			::Polyline(hdc, edge, _countof(edge));

			::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
			::RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 0, 0);

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
		DWORD_PTR hwndEditData = NULL;
		auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		if ((style & CBS_DROPDOWN) == CBS_DROPDOWN)
		{
			POINT pt = { 5, 5 };
			hwndEditData = reinterpret_cast<DWORD_PTR>(::ChildWindowFromPoint(hwnd, pt));
		}
		SetWindowSubclass(hwnd, ComboBoxSubclass, g_comboBoxSubclassID, hwndEditData);
	}

	constexpr UINT_PTR g_spinControlSubclassID = 42;

	void renderSpinButtonArrow(HDC hdc, const RECT& rcArrow, SpinArrowDirection direction, int iStateID)
	{
		struct FPOINT {
			float x = 0.0f;
			float y = 0.0f;
		};

		struct FSIZE {
			float cx = 0.0f;
			float cy = 0.0f;
		};

		FPOINT ptTriangleF[3] = {};
		switch (direction)
		{
		case SpinArrowDirection::Left:
			ptTriangleF[0] = { 1.0f, 0.0f };
			ptTriangleF[1] = { 0.0f, 0.5f };
			ptTriangleF[2] = { 1.0f, 1.0f };
			break;
		case SpinArrowDirection::Right:
			ptTriangleF[0] = { 0.0f, 0.0f };
			ptTriangleF[1] = { 1.0f, 0.5f };
			ptTriangleF[2] = { 0.0f, 1.0f };
			break;
		case SpinArrowDirection::Up:
			ptTriangleF[0] = { 0.0f, 1.0f };
			ptTriangleF[1] = { 0.5f, 0.0f };
			ptTriangleF[2] = { 1.0f, 1.0f };
			break;
		case SpinArrowDirection::Down:
			ptTriangleF[0] = { 0.0f, 0.0f };
			ptTriangleF[1] = { 0.5f, 1.0f };
			ptTriangleF[2] = { 1.0f, 0.0f };
			break;
		}

		FSIZE szArrowRc = { static_cast<float>(rcArrow.right - rcArrow.left), static_cast<float>(rcArrow.bottom - rcArrow.top) };
		// Size on triangle actually begins at 0, so a 2x4 is actually a 3x5 triangle 
		// (aliased triangles on screen are more harmonic having odd pixel sizes). 
		// The remainder there is for scaling.
		FSIZE szTriangleRect;
		if ((direction == SpinArrowDirection::Left) || (direction == SpinArrowDirection::Right))
			szTriangleRect = { static_cast<float>(NppParameters::getInstance()._dpiManager.scaleX(2)),
				static_cast<float>(NppParameters::getInstance()._dpiManager.scaleY(4) + (NppParameters::getInstance()._dpiManager.scaleY(4) % 2)) };
		else
			szTriangleRect = { static_cast<float>(NppParameters::getInstance()._dpiManager.scaleX(4) + (NppParameters::getInstance()._dpiManager.scaleX(4) % 2)),
				static_cast<float>(NppParameters::getInstance()._dpiManager.scaleY(2)) };
		POINT ptTriangleFinal[3];
		for (int i = 0; i < 3; i++)
		{
			ptTriangleF[i].x = ptTriangleF[i].x * szTriangleRect.cx; // Scale triangle
			ptTriangleF[i].y = ptTriangleF[i].y * szTriangleRect.cy;

			// Centralize
			ptTriangleF[i].x += static_cast<float>(rcArrow.left) + (szArrowRc.cx - szTriangleRect.cx - 1) / 2.0f;
			ptTriangleF[i].y += static_cast<float>(rcArrow.top) + (szArrowRc.cy - szTriangleRect.cy - 1) / 2.0f;

			// Convert to int point
			ptTriangleFinal[i].x = static_cast<int>(ptTriangleF[i].x);
			ptTriangleFinal[i].y = static_cast<int>(ptTriangleF[i].y);
		}

		HPEN trianglePen = (iStateID == UPS_HOT || iStateID == UPS_PRESSED) ?
			getHotEdgePen() : (iStateID == UPS_DISABLED) ? getDisabledEdgePen() : getEdgePen();
		HBRUSH hTriangleBrush = (iStateID == UPS_HOT || iStateID == UPS_PRESSED) ?
			getHotEdgeBrush() : (iStateID == UPS_DISABLED) ? getDisabledEdgeBrush() : getEdgeBrush();

		HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, trianglePen));
		HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hTriangleBrush));

		Polygon(hdc, &ptTriangleFinal[0], 3);
		SelectObject(hdc, hOldPen);
		SelectObject(hdc, hOldBrush);
	}

	void renderSpinButtonArrowWin10(HWND hWnd, HDC hdc, const RECT& rcArrow, SpinArrowDirection direction, int iStateID)
	{
		LOGFONT lf = {};
		auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
		::GetObject(font, sizeof(lf), &lf);
		lf.lfHeight = (NppParameters::getInstance()._dpiManager.scaleY(16) - 5) * -1;
		HFONT arrowFont = CreateFontIndirect(&lf);
		auto holdFont = static_cast<HFONT>(::SelectObject(hdc, arrowFont));

		auto mPosX = ((rcArrow.right - rcArrow.left - NppParameters::getInstance()._dpiManager.scaleX(7) + 1) / 2);
		auto mPosY = ((rcArrow.bottom - rcArrow.top + lf.lfHeight - NppParameters::getInstance()._dpiManager.scaleY(1) - 3) / 2);

		std::wstring sArrow;
		switch (direction)
		{
		case SpinArrowDirection::Left:
			sArrow = L"<";
			break;
		case SpinArrowDirection::Right:
			sArrow = L">";
			break;
		case SpinArrowDirection::Up:
			sArrow = L"˄";
			break;
		case SpinArrowDirection::Down:
			sArrow = L"˅";
			break;
		}

		COLORREF arrowColor = (iStateID == UPS_HOT || iStateID == UPS_PRESSED) ?
			getHotTextColor() : (iStateID == UPS_DISABLED) ? getDarkerTextColor() : getTextColor();

		::SetBkMode(hdc, TRANSPARENT);
		::SetTextColor(hdc, arrowColor);
		::ExtTextOut(hdc, rcArrow.left + mPosX, rcArrow.top + mPosY, ETO_CLIPPED, &rcArrow, sArrow.c_str(), 1, nullptr);
		SelectObject(hdc, holdFont);
		DeleteObject(arrowFont);
	}

	void renderSpinButtonBackground(HDC hdc, int iStateID, const RECT& rcClient)
	{
		HBRUSH hBckBrush = (iStateID == UPS_HOT || iStateID == UPS_PRESSED) ? getSofterBackgroundBrush() : getDarkerBackgroundBrush();

		HPEN hOldPen = nullptr;
		if (iStateID == UPS_DISABLED)
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, getDisabledEdgePen()));
		else if (iStateID == UPS_PRESSED)
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, getHotEdgePen()));
		else
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, getEdgePen()));

		HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hBckBrush));
		Rectangle(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
		SelectObject(hdc, hOldBrush);

		if (hOldPen)
			SelectObject(hdc, hOldPen);
	}

	void renderSpinButton(HWND hWnd, const SpinData& pSpinData, HDC hdc, int iStateID1, int iStateID2)
	{
		renderSpinButtonBackground(hdc, iStateID1, pSpinData.rcArrowOne);
		renderSpinButtonBackground(hdc, iStateID2, pSpinData.rcArrowTwo);

		RECT rcArrowOne = pSpinData.rcArrowOne;
		RECT rcArrowTwo = pSpinData.rcArrowTwo;
		InflateRect(&rcArrowOne, -1, -1);
		InflateRect(&rcArrowTwo, -1, -1);

		if (isWindows11())
		{
			renderSpinButtonArrow(hdc, rcArrowOne,
				pSpinData.type == SpinButtonType::LeftRight ? SpinArrowDirection::Left : SpinArrowDirection::Up, iStateID1);
			renderSpinButtonArrow(hdc, rcArrowTwo,
				pSpinData.type == SpinButtonType::LeftRight ? SpinArrowDirection::Right : SpinArrowDirection::Down, iStateID2);
		}
		else
		{
			renderSpinButtonArrowWin10(hWnd, hdc, rcArrowOne, 
				pSpinData.type == SpinButtonType::LeftRight ? SpinArrowDirection::Left : SpinArrowDirection::Up, iStateID1);
			renderSpinButtonArrowWin10(hWnd, hdc, rcArrowTwo,
				pSpinData.type == SpinButtonType::LeftRight ? SpinArrowDirection::Right : SpinArrowDirection::Down, iStateID2);
		}
	}

	void paintSpinButtons(HWND hWnd, HDC hdc, SpinData& pSpinData)
	{
		if (BufferedPaintRenderAnimation(hWnd, hdc))
		{
			return;
		}

		RECT rcBackground;
		::GetClientRect(hWnd, &rcBackground);
		FillRect(hdc, &rcBackground, getBackgroundBrush());

		bool bPressed = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
		LONG_PTR nStyle = GetWindowLongPtr(hWnd, GWL_STYLE); // To check enabled / disabled

		int iStateID1 = (bPressed && pSpinData.isHotOne) ? UPS_PRESSED : (pSpinData.isHotOne ? UPS_HOT : UPS_NORMAL);
		int iStateID2 = (bPressed && pSpinData.isHotTwo) ? UPS_PRESSED : (pSpinData.isHotTwo ? UPS_HOT : UPS_NORMAL);
		if (nStyle & WS_DISABLED)
		{
			iStateID1 = UPS_DISABLED;
			iStateID2 = UPS_DISABLED;
		}

		BP_ANIMATIONPARAMS animParams = { sizeof(animParams) };
		animParams.style = BPAS_LINEAR;
		if (iStateID1 != pSpinData.iStateID1)
			GetThemeTransitionDuration(pSpinData.hTheme, SPNP_UP, pSpinData.iStateID1, iStateID1, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		if (iStateID2 != pSpinData.iStateID2)
			GetThemeTransitionDuration(pSpinData.hTheme, SPNP_UP, pSpinData.iStateID2, iStateID2, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);

		HDC hdcFrom = nullptr;
		HDC hdcTo = nullptr;
		HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(hWnd, hdc, &pSpinData.rcClient, BPBF_COMPATIBLEBITMAP, nullptr, &animParams, &hdcFrom, &hdcTo);
		if (hbpAnimation)
		{
			if (hdcFrom)
			{
				renderSpinButton(hWnd, pSpinData, hdcFrom, pSpinData.iStateID1, pSpinData.iStateID2);
			}
			if (hdcTo)
			{
				renderSpinButton(hWnd, pSpinData, hdcTo, iStateID1, iStateID2);
			}

			pSpinData.iStateID1 = iStateID1;
			pSpinData.iStateID2 = iStateID2;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			renderSpinButton(hWnd, pSpinData, hdc, iStateID1, iStateID2);
			pSpinData.iStateID1 = iStateID1;
			pSpinData.iStateID2 = iStateID2;
		}
	}

	LRESULT CALLBACK SpinControlSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pSpinData = reinterpret_cast<SpinData*>(dwRefData);
		if (!pSpinData->isInitialized)
		{
			pSpinData->updateSize(hWnd);
		}

		switch (uMsg)
		{
		case WM_PRINTCLIENT:
		case WM_PAINT:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			if (!pSpinData->ensureTheme(hWnd))
			{
				break;
			}

			PAINTSTRUCT ps{};
			auto hdc = ::BeginPaint(hWnd, &ps);
			paintSpinButtons(hWnd, hdc, *pSpinData);
			::EndPaint(hWnd, &ps);

			return TRUE;
		}
		case WM_NCHITTEST:
		{
			POINT MousePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			::ScreenToClient(hWnd, &MousePos);

			bool isHotOne = ::PtInRect(&pSpinData->rcArrowOne, MousePos);
			bool isHotTwo = ::PtInRect(&pSpinData->rcArrowTwo, MousePos);

			if (pSpinData->isHotOne != isHotOne || pSpinData->isHotTwo != isHotTwo)
			{
				pSpinData->isHotOne = isHotOne;
				pSpinData->isHotTwo = isHotTwo;
				InvalidateRect(hWnd, NULL, true);
			}

			break;
		}
		case WM_MOUSELEAVE:
		{
			pSpinData->isHotOne = false;
			pSpinData->isHotTwo = false;
			InvalidateRect(hWnd, NULL, true);
		}

		case WM_THEMECHANGED:
		{
			pSpinData->closeTheme();
			break;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, SpinControlSubclass, uIdSubclass);
			delete pSpinData;
			break;
		}

		case WM_SIZE:
		{
			pSpinData->updateSize(hWnd);
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

	void subclassSpinControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, SpinControlSubclass, g_spinControlSubclassID, reinterpret_cast<DWORD_PTR>(new SpinData()));
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

					SetTextColor(hdc, (bHot || isSelectedTab) ? NppDarkMode::getTextColor() : NppDarkMode::getDarkerTextColor());

					::InflateRect(&rcItem, -1, -1);
					rcItem.right += 1;

					// for consistency getBackgroundBrush() 
					// would be better, than getSofterBackgroundBrush(),
					// however default getBackgroundBrush() has same color
					// as getDarkerBackgroundBrush()
					::FillRect(hdc, &rcItem, isSelectedTab ? NppDarkMode::getDarkerBackgroundBrush() : bHot ? NppDarkMode::getHotBackgroundBrush() : NppDarkMode::getSofterBackgroundBrush());

					SetBkMode(hdc, TRANSPARENT);

					TCHAR label[MAX_PATH];
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
			case WM_CREATE:  // Spin control inside tab
			{
				subclassSpinControl(reinterpret_cast<HWND>(lParam));

				break;
			}
			}
			break;
		}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassTabControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, TabSubclass, g_tabSubclassID, 0);
	}

	LRESULT CALLBACK TabOwnerDrawnSubclass(
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
		case WM_NCDESTROY:
		{
			RemoveWindowSubclass(hWnd, TabSubclass, g_tabSubclassID);
			break;
		}
		case WM_PARENTNOTIFY:
		{
			switch (LOWORD(wParam))
			{
			case WM_CREATE:  // Spin control inside tab
			{
				subclassSpinControl(reinterpret_cast<HWND>(lParam));
				break;
			}
			}
			break;
		}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassTabOwnerDrawnControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, TabOwnerDrawnSubclass, g_tabSubclassID, 0);
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
			NppDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnableThemeDialogTexture(hwndParent, theme && !NppDarkMode::isEnabled() ? ETDT_ENABLETAB : ETDT_DISABLE);

		EnumChildWindows(hwndParent, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
			auto& p = *reinterpret_cast<Params*>(lParam);
			const size_t classNameLen = 16;
			TCHAR className[classNameLen] = { '\0' };
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

				return TRUE;
			}

			if (wcscmp(className, WC_BUTTON) == 0)
			{
				auto nButtonStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE) & 0xF;
				auto nColorPickerClass = GetWindowLongPtr(hwnd, GWLP_USERDATA);
				switch (nButtonStyle)
				{
				case BS_PUSHBUTTON:
				case BS_DEFPUSHBUTTON:
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					// Patch for Notepad++ ColorPicker class
					if (dynamic_cast<ColourPicker*>(reinterpret_cast<Window*>(nColorPickerClass)))
						break;

					if (p.theme && (nButtonStyle == BS_PUSHBUTTON || nButtonStyle == BS_DEFPUSHBUTTON))
						SetWindowTheme(hwnd, p.themeClassName, nullptr);

					if (p.subclass && NppDarkMode::isEnabled())
						NppDarkMode::subclassButtonControl(hwnd);
					if (p.subclass && !NppDarkMode::isEnabled())
						RemoveWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID);
					break;
				}
				case BS_GROUPBOX:
					if (p.subclass && NppDarkMode::isEnabled())
						NppDarkMode::subclassGroupboxControl(hwnd);
					if (p.subclass && !NppDarkMode::isEnabled())
						RemoveWindowSubclass(hwnd, GroupboxSubclass, g_groupboxSubclassID);
					break;
				}
				return TRUE;
			}

			if (wcscmp(className, UPDOWN_CLASS) == 0)
			{
				if (p.theme)
					NppDarkMode::setDarkExplorerTheme(hwnd);

				if (p.subclass && NppDarkMode::isEnabled())
					NppDarkMode::subclassSpinControl(hwnd);
				if (p.subclass && !NppDarkMode::isEnabled())
					RemoveWindowSubclass(hwnd, SpinControlSubclass, g_spinControlSubclassID);
			}

			return TRUE;
			}, reinterpret_cast<LPARAM>(&p));
	}

	void autoThemeChildControls(HWND hwndParent)
	{
		autoSubclassAndThemeChildControls(hwndParent, false, true);
	}

	void setDarkTitleBar(HWND hwnd)
	{
		NppDarkMode::allowDarkModeForWindow(hwnd, NppDarkMode::isEnabled());
		NppDarkMode::setTitleBarThemeColor(hwnd);
	}

	void setDarkExplorerTheme(HWND hwnd)
	{
		SetWindowTheme(hwnd, NppDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr, nullptr);
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
		COLORSCHEME scheme;
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
		bool useDark = NppDarkMode::isEnabled();

		HWND hHeader = ListView_GetHeader(hwnd);
		NppDarkMode::allowDarkModeForWindow(hHeader, useDark);
		SetWindowTheme(hHeader, useDark ? L"ItemsView" : nullptr, nullptr);

		NppDarkMode::allowDarkModeForWindow(hwnd, useDark);
		SetWindowTheme(hwnd, L"Explorer", nullptr);
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
	constexpr double middleGrayRange = 2.0;

	void calculateTreeViewStyle()
	{
		COLORREF bgColor = NppParameters::getInstance().getCurrentDefaultBgColor();

		if (treeViewBg != bgColor || lighnessTreeView == 50.0)
		{
			lighnessTreeView = calculatePerceivedLighness(bgColor);
			treeViewBg = bgColor;
		}

		if (lighnessTreeView < (50.0 - middleGrayRange))
		{
			treeViewStyle = TreeViewStyle::dark;
		}
		else if (lighnessTreeView > (50.0 + middleGrayRange))
		{
			treeViewStyle = TreeViewStyle::light;
		}
		else
		{
			treeViewStyle = TreeViewStyle::classic;
		}
	}

	void setTreeViewStyle(HWND hwnd)
	{
		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool hasHotStyle = (style & TVS_TRACKSELECT) == TVS_TRACKSELECT;
		bool change = false;
		switch (treeViewStyle)
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
				SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
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
}
