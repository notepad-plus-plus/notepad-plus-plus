#pragma once

#include <Windows.h>

constexpr COLORREF HEXRGB(DWORD rrggbb) {
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return
		((rrggbb & 0xFF0000) >> 16) |
		((rrggbb & 0x00FF00) ) |
		((rrggbb & 0x0000FF) << 16);
}

namespace NppDarkMode
{
	struct Options
	{
		bool enable = false;
		bool enableExperimental = false;
		bool enableMenubar = false;
		bool enableScrollbarHack = false;
	};
	
	enum class ToolTipsType
	{
		tooltip,
		toolbar,
		listview,
		treeview
	};

	void initDarkMode();				// pulls options from NppParameters
	void refreshDarkMode(HWND hwnd, bool forceRefresh = false);	// attempts to apply new options from NppParameters, sends NPPM_INTERNAL_REFRESHDARKMODE to hwnd's top level parent

	bool isEnabled();
	bool isDarkMenuEnabled();
	bool isExperimentalEnabled();
	bool isScrollbarHackEnabled();

	COLORREF invertLightness(COLORREF c);
	COLORREF invertLightnessSofter(COLORREF c);

	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDarkerBackgroundColor();
	COLORREF getErrorBackgroundColor();

	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getDarkerBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();

	// handle events
	void handleSettingChange(HWND hwnd, LPARAM lParam);

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);
	void drawUAHMenuNCBottomLine(HWND hWnd);

	// from DarkMode.h
	void initExperimentalDarkMode(bool fixDarkScrollbar, bool dark);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd, bool dark);

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);

	void subclassButtonControl(HWND hwnd);
	void subclassGroupboxControl(HWND hwnd);
	void subclassToolbarControl(HWND hwnd);
	void subclassTabControl(HWND hwnd);
	void disableVisualStyle(HWND hwnd);

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass = true, bool theme = true);
	void autoThemeChildControls(HWND hwndParent);

	void setDarkTitleBar(HWND hwnd);
	void setDarkTooltips(HWND hwnd, ToolTipsType type);
	void setDarkLineAbovePanelToolbar(HWND hwnd);
}
