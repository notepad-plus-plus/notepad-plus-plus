#pragma once

#include <Windows.h>

namespace NppDarkMode
{
	bool isEnabled();

	COLORREF invertLightness(COLORREF c);
	COLORREF invertLightnessSofter(COLORREF c);

	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();

	// handle events
	bool handleSettingChange(HWND hwnd, LPARAM lParam); // true if dark mode toggled

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);

	// from DarkMode.h
	void initDarkMode();
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void refreshTitleBarThemeColor(HWND hWnd);

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);
}

