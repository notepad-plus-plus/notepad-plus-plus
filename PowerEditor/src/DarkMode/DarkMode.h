#pragma once

extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;


bool ShouldAppsUseDarkMode();
bool AllowDarkModeForWindow(HWND hWnd, bool allow);
bool IsHighContrast();
void RefreshTitleBarThemeColor(HWND hWnd);
void SetTitleBarThemeColor(HWND hWnd, BOOL dark);
bool IsColorSchemeChangeMessage(LPARAM lParam);
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void AllowDarkModeForApp(bool allow);
void EnableDarkScrollBarForWindowAndChildren(HWND hwnd);
void InitDarkMode();
void SetDarkMode(bool useDarkMode, bool fixDarkScrollbar);
bool IsWindows10();
bool IsWindows11();
DWORD GetWindowsBuildNumber();
