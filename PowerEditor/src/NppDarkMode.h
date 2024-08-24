// This file is part of Notepad++ project
// Copyright (c) 2021 adzm / Adam D. Walling

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

#pragma once

#include <string>
#include <windows.h>


namespace NppDarkMode
{
	struct Colors
	{
		COLORREF background = 0;
		COLORREF softerBackground = 0;
		COLORREF hotBackground = 0;
		COLORREF pureBackground = 0;
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
		COLORREF hotEdge = 0;
		COLORREF disabledEdge = 0;
	};

	struct Options
	{
		bool enable = false;
		bool enableMenubar = false;
		bool enablePlugin = false;
	};

	struct NppDarkModeParams
	{
		const wchar_t* _themeClassName = nullptr;
		bool _subclass = false;
		bool _theme = false;
	};

	enum class ToolTipsType
	{
		tooltip,
		toolbar,
		listview,
		treeview,
		tabbar
	};

	enum ColorTone {
		blackTone  = 0,
		redTone    = 1,
		greenTone  = 2,
		blueTone   = 3,
		purpleTone = 4,
		cyanTone   = 5,
		oliveTone  = 6,
		customizedTone = 32
	};

	enum class TreeViewStyle
	{
		classic = 0,
		light = 1,
		dark = 2
	};

	struct AdvOptDefaults
	{
		std::wstring _xmlFileName;
		int _toolBarIconSet = -1;
		int _tabIconSet = -1;
		bool _tabUseTheme = false;
	};

	struct AdvancedOptions
	{
		bool _enableWindowsMode = false;

		NppDarkMode::AdvOptDefaults _darkDefaults{ L"DarkModeDefault.xml", 0, 2, false };
		NppDarkMode::AdvOptDefaults _lightDefaults{ L"", 4, 0, true };
	};

	constexpr UINT WM_SETBUTTONIDEALSIZE = (WM_USER + 4200);

	void initDarkMode();				// pulls options from NppParameters
	void refreshDarkMode(HWND hwnd, bool forceRefresh = false);	// attempts to apply new options from NppParameters, sends NPPM_INTERNAL_REFRESHDARKMODE to hwnd's top level parent

	void initAdvancedOptions();

	bool isEnabled();
	bool isDarkMenuEnabled();
	bool isEnabledForPlugins();
	bool isExperimentalActive();
	bool isExperimentalSupported();

	bool isWindowsModeEnabled();
	void setWindowsMode(bool enable);
	std::wstring getThemeName();
	void setThemeName(const std::wstring& newThemeName);
	int getToolBarIconSet(bool useDark);
	void setToolBarIconSet(int state2Set, bool useDark);
	int getTabIconSet(bool useDark);
	void setTabIconSet(bool useAltIcons, bool useDark);
	bool useTabTheme();
	void setAdvancedOptions();

	bool isWindows10();
	bool isWindows11();
	DWORD getWindowsBuildNumber();

	COLORREF invertLightness(COLORREF c);
	double calculatePerceivedLightness(COLORREF c);

	void setDarkTone(ColorTone colorToneChoice);

	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDarkerBackgroundColor();
	COLORREF getErrorBackgroundColor();

	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getLinkTextColor();

	COLORREF getEdgeColor();
	COLORREF getHotEdgeColor();
	COLORREF getDisabledEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getDarkerBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();

	HBRUSH getEdgeBrush();
	HBRUSH getHotEdgeBrush();
	HBRUSH getDisabledEdgeBrush();

	HPEN getDarkerTextPen();
	HPEN getEdgePen();
	HPEN getHotEdgePen();
	HPEN getDisabledEdgePen();

	void setBackgroundColor(COLORREF c);
	void setSofterBackgroundColor(COLORREF c);
	void setHotBackgroundColor(COLORREF c);
	void setDarkerBackgroundColor(COLORREF c);
	void setErrorBackgroundColor(COLORREF c);
	void setTextColor(COLORREF c);
	void setDarkerTextColor(COLORREF c);
	void setDisabledTextColor(COLORREF c);
	void setLinkTextColor(COLORREF c);
	void setEdgeColor(COLORREF c);
	void setHotEdgeColor(COLORREF c);
	void setDisabledEdgeColor(COLORREF c);

	Colors getDarkModeDefaultColors(ColorTone colorTone = ColorTone::blackTone);
	void changeCustomTheme(const Colors& colors);

	// handle events
	void handleSettingChange(HWND hwnd, LPARAM lParam, bool isFromBtn = false);
	bool isDarkModeReg();

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);
	void drawUAHMenuNCBottomLine(HWND hWnd);

	// from DarkMode.h
	void initExperimentalDarkMode();
	void setDarkMode(bool useDark, bool fixDarkScrollbar);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd);

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);

	inline void paintRoundFrameRect(HDC hdc, const RECT rect, const HPEN hpen, int width = 0, int height = 0);

	void subclassButtonControl(HWND hwnd);
	void subclassGroupboxControl(HWND hwnd);
	void subclassTabControl(HWND hwnd);
	void subclassComboBoxControl(HWND hwnd);

	bool subclassTabUpDownControl(HWND hwnd);

	void subclassAndThemeButton(HWND hwnd, NppDarkModeParams p);
	void subclassAndThemeComboBox(HWND hwnd, NppDarkModeParams p);
	void subclassAndThemeListBoxOrEditControl(HWND hwnd, NppDarkModeParams p, bool isListBox);
	void subclassAndThemeListView(HWND hwnd, NppDarkModeParams p);
	void themeTreeView(HWND hwnd, NppDarkModeParams p);
	void themeToolbar(HWND hwnd, NppDarkModeParams p);
	void themeRichEdit(HWND hwnd, NppDarkModeParams p);

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass = true, bool theme = true);
	void autoThemeChildControls(HWND hwndParent);

	void autoSubclassAndThemePluginDockWindow(HWND hwnd);
	ULONG autoSubclassAndThemePlugin(HWND hwnd, ULONG dmFlags);
	void autoSubclassAndThemeWindowNotify(HWND hwnd);

	void setDarkTitleBar(HWND hwnd);
	void setDarkExplorerTheme(HWND hwnd);
	void setDarkScrollBar(HWND hwnd);
	void setDarkTooltips(HWND hwnd, ToolTipsType type);
	void setDarkLineAbovePanelToolbar(HWND hwnd);
	void setDarkListView(HWND hwnd);

	void disableVisualStyle(HWND hwnd, bool doDisable);
	void calculateTreeViewStyle();
	void updateTreeViewStylePrev();
	TreeViewStyle getTreeViewStyle();
	void setTreeViewStyle(HWND hWnd, bool force = false);
	bool isThemeDark();
	void setBorder(HWND hwnd, bool border = true);

	void setDarkAutoCompletion();

	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorSofter(HDC hdc);
	LRESULT onCtlColorDarker(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
	LRESULT onCtlColorDarkerBGStaticText(HDC hdc, bool isTextEnabled);
	INT_PTR onCtlColorListbox(WPARAM wParam, LPARAM lParam);
}
