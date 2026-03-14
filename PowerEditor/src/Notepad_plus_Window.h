     1|// This file is part of npminmin project
     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|#pragma once
    17|#include "Notepad_plus.h"
    18|
    19|constexpr int splitterSize = 8;
    20|
    21|
    22|class Notepad_plus_Window : public Window
    23|{
    24|public:
    25|	void init(HINSTANCE, HWND, const wchar_t *cmdLine, CmdLineParams *cmdLineParams);
    26|
    27|	bool isDlgsMsg(MSG *msg) const;
    28|
    29|	HACCEL getAccTable() const {
    30|		return _notepad_plus_plus_core.getAccTable();
    31|	}
    32|
    33|	bool emergency(const std::wstring& emergencySavedDir) {
    34|		return _notepad_plus_plus_core.emergency(emergencySavedDir);
    35|	}
    36|
    37|	bool isPrelaunch() const {
    38|		return _isPrelaunch;
    39|	}
    40|
    41|	void setIsPrelaunch(bool val) {
    42|		_isPrelaunch = val;
    43|	}
    44|
    45|	std::wstring getPluginListVerStr() const {
    46|		return _notepad_plus_plus_core.getPluginListVerStr();
    47|	}
    48|
    49|	void destroy() override {
    50|		if (_hIconAbsent)
    51|			::DestroyIcon(_hIconAbsent);
    52|		::DestroyWindow(_hSelf);
    53|	}
    54|
    55|	static const wchar_t * getClassName() {
    56|		return _className;
    57|	}
    58|
    59|	HICON getAbsentIcoHandle() {
    60|		return _hIconAbsent;
    61|	}
    62|
    63|	static HWND gNppHWND;	//static handle to npminmin window, NULL if non-existent
    64|
    65|	void setStartupBgColor(COLORREF BgColor);
    66|
    67|	static void loadTrayIcon(HINSTANCE hinst, HICON* icon) {
    68|		DPIManagerV2::loadIcon(hinst, MAKEINTRESOURCE(IDI_M30ICON), ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), icon);
    69|	}
    70|
    71|private:
    72|	Notepad_plus _notepad_plus_plus_core;
    73|	static LRESULT CALLBACK Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    74|	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    75|
    76|	static constexpr wchar_t _className[32] = L"npminmin";
    77|	bool _isPrelaunch = false;
    78|	bool _disablePluginsManager = false;
    79|
    80|	QuoteParams _quoteParams; // keep the availability of quote parameters for thread using
    81|	std::wstring _userQuote; // keep the availability of this string for thread using
    82|
    83|	HICON _hIconAbsent = nullptr;
    84|};
    85|