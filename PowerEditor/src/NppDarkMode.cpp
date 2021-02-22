#include "nppDarkMode.h"

#include "DarkMode/DarkMode.h"
#include "DarkMode/UAHMenuBar.h"

#include <Uxtheme.h>
#include <Vssym32.h>

#include "Parameters.h"
#include "resource.h"

#include <Shlwapi.h>

#pragma comment(lib, "uxtheme.lib")

namespace NppDarkMode
{
	static Options _options;			// actual runtime options

	const Options& configuredOptions()
	{
		return NppParameters::getInstance().getNppGUI()._darkmode;
	}

	void initDarkMode()
	{
		_options = configuredOptions();

		if (_options.enableExperimental)
		{
			initExperimentalDarkMode(_options.enableScrollbarHack);
		}
	}

	// attempts to apply new options from NppParameters, sends NPPM_INTERNAL_REFRESHDARKMODE to hwnd's top level parent
	void refreshDarkMode(HWND hwnd, bool forceRefresh)
	{
		bool supportedChanged = false;

		auto& config = configuredOptions();

		if (_options.enable != config.enable)
		{
			supportedChanged = true;
			_options.enable = config.enable;
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
		::SendMessage(hwndRoot, NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
	}

	bool isEnabled()
	{
		return _options.enable;
	}

	bool isDarkMenuEnabled()
	{
		return _options.enableMenubar;
	}

	bool isExperimentalEnabled()
	{
		return _options.enableExperimental;
	}

	bool isScrollbarHackEnabled()
	{
		return _options.enableScrollbarHack;
	}

	bool isExperimentalActive()
	{
		return g_darkModeEnabled;
	}

	bool isExperimentalSupported()
	{
		return g_darkModeSupported;
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

	COLORREF getBackgroundColor()
	{
		return RGB(0x20, 0x20, 0x20);
	}

	COLORREF getSofterBackgroundColor()
	{
		return RGB(0x2B, 0x2B, 0x2B);
	}

	COLORREF getHotBackgroundColor()
	{
		return RGB(0x4D, 0x4D, 0x4D);
	}

	COLORREF getPureBackgroundColor()
	{
		return RGB(0, 0, 0);
	}

	COLORREF getTextColor()
	{
		return RGB(0xE0, 0xE0, 0xE0);
	}

	COLORREF getDarkerTextColor()
	{
		return RGB(0xC0, 0xC0, 0xC0);
	}

	COLORREF getEdgeColor()
	{
		return RGB(0x80, 0x80, 0x80);
	}

	COLORREF getErrorBackgroundColor()
	{
		return RGB(0xB0, 0x00, 0x00);
	}

	HBRUSH getBackgroundBrush()
	{
		static HBRUSH g_hbrBackground = ::CreateSolidBrush(getBackgroundColor());
		return g_hbrBackground;
	}

	HBRUSH getSofterBackgroundBrush()
	{
		static HBRUSH g_hbrSofterBackground = ::CreateSolidBrush(getSofterBackgroundColor());
		return g_hbrSofterBackground;
	}

	HBRUSH getHotBackgroundBrush()
	{
		static HBRUSH g_hbrHotBackground = ::CreateSolidBrush(getHotBackgroundColor());
		return g_hbrHotBackground;
	}

	HBRUSH getPureBackgroundBrush()
	{
		static HBRUSH g_hbrPureBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		return g_hbrPureBackground;
	}

	HBRUSH getErrorBackgroundBrush()
	{
		static HBRUSH g_hbrErrorBackground = ::CreateSolidBrush(getErrorBackgroundColor());
		return g_hbrErrorBackground;
	}

	// handle events

	bool handleSettingChange(HWND hwnd, LPARAM lParam) // true if dark mode toggled
	{
		if (!isExperimentalEnabled())
		{
			return false;
		}

		bool toggled = false;
		if (IsColorSchemeChangeMessage(lParam))
		{
			bool darkModeWasEnabled = g_darkModeEnabled;
			g_darkModeEnabled = ShouldAppsUseDarkMode() && !IsHighContrast();

			NppDarkMode::refreshTitleBarThemeColor(hwnd);

			if (!!darkModeWasEnabled != !!g_darkModeEnabled)
			{
				toggled = true;
			}
		}

		return toggled;
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
				RECT rc = { 0 };

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

				FillRect(pUDM->hdc, &rc, NppDarkMode::getPureBackgroundBrush());

				*lr = 0;

				return true;
			}
			case WM_UAHDRAWMENUITEM:
			{
				UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

				// get the menu item string
				wchar_t menuString[256] = { 0 };
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
					FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, NppDarkMode::getPureBackgroundBrush());
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

	// from DarkMode.h

	void initExperimentalDarkMode(bool fixDarkScrollbar)
	{
		::InitDarkMode(fixDarkScrollbar);
	}

	void allowDarkModeForApp(bool allow)
	{
		::AllowDarkModeForApp(allow);
	}

	bool allowDarkModeForWindow(HWND hWnd, bool allow)
	{
		return ::AllowDarkModeForWindow(hWnd, allow);
	}

	void refreshTitleBarThemeColor(HWND hWnd)
	{
		::RefreshTitleBarThemeColor(hWnd);
	}

	void enableDarkScrollBarForWindowAndChildren(HWND hwnd)
	{
		::EnableDarkScrollBarForWindowAndChildren(hwnd);
	}

	struct ButtonData
	{
		HTHEME hTheme = nullptr;

		~ButtonData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, L"Button");
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

	void paintButton(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		RECT rcClient = { 0 };
		WCHAR szText[256] = { 0 };
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		bool isEnabled = IsWindowEnabled(hwnd);;
		DWORD uiState = static_cast<DWORD>(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0));
		DWORD nStyle = GetWindowLong(hwnd, GWL_STYLE);
		DWORD nButtonStyle = nStyle & 0xF;

		int iPartID = BP_CHECKBOX;
		if (nButtonStyle == BS_CHECKBOX || nButtonStyle == BS_AUTOCHECKBOX)
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

		if (!isEnabled)					iStateID = RBS_UNCHECKEDDISABLED;
		else if (nState & BST_PUSHED)	iStateID = RBS_UNCHECKEDPRESSED;
		else if (nState & BST_HOT)		iStateID = RBS_UNCHECKEDHOT;

		if (nState & BST_CHECKED)		iStateID += 4;

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf = { 0 };
		if (SUCCEEDED(GetThemeFont(buttonData.hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont) {
			hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		}

		SelectObject(hdc, hFont);

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
		GetThemePartSize(buttonData.hTheme, hdc, iPartID, iStateID, NULL, TS_DRAW, &szBox);

		RECT rcText = rcClient;
		GetThemeBackgroundContentRect(buttonData.hTheme, hdc, iPartID, iStateID, &rcClient, &rcText);

		RECT rcBackground = rcClient;
		if (dtFlags & DT_SINGLELINE)
		{
			rcBackground.top += (rcText.bottom - rcText.top - szBox.cy) / 2;
		}
		rcBackground.bottom = rcBackground.top + szBox.cy;
		rcBackground.right = rcBackground.left + szBox.cx;
		rcText.left = rcBackground.right + 3;

		DrawThemeParentBackground(hwnd, hdc, &rcClient);
		DrawThemeBackground(buttonData.hTheme, hdc, iPartID, iStateID, &rcBackground, nullptr);

		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
		dtto.crText = NppDarkMode::getTextColor();

		DrawThemeTextEx(buttonData.hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if ((nState & BST_FOCUS) && !(uiState & UISF_HIDEFOCUS))
		{
			RECT rcTextOut = rcText;
			dtto.dwFlags |= DTT_CALCRECT;
			DrawThemeTextEx(buttonData.hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags | DT_CALCRECT, &rcTextOut, &dtto);
			RECT rcFocus = rcTextOut;
			rcFocus.bottom++;
			rcFocus.left--;
			rcFocus.right++;
			DrawFocusRect(hdc, &rcFocus);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
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
					PAINTSTRUCT ps = { 0 };
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
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassButtonControl(HWND hwnd)
	{
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
		SetWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID, pButtonData);
	}

	constexpr UINT_PTR g_toolbarSubclassID = 42;

	LRESULT CALLBACK ToolbarSubclass(
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
				RemoveWindowSubclass(hWnd, ToolbarSubclass, g_toolbarSubclassID);
				break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassToolbarControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, ToolbarSubclass, g_toolbarSubclassID, 0);
	}
}

