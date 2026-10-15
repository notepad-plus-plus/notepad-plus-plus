// This file is part of Notepad++ project
// Copyright (c) 2026 LucaSorvillo and Notepad++ team

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

#include <windows.h>

class TabBar;

namespace VerticalTabs
{
	// "List mode": the tab bar is vertical and "Horizontal titles" is checked (Preferences > Tab Bar).
	// The document tab bars are then horizontal multi-line tab controls, one tab per row, in a column on the left.
	bool isListMode();

	// Called at the end of TabBarPlus::init(): watches the tab control (window subclass)
	void attach(HWND hTabBar);

	// Called at the start of TabBar::reSizeTo(): in list mode lays the column out, adjusts rc2Adjust for the
	// editor and returns true; otherwise restores the original styles if needed and returns false.
	bool reSizeTo(TabBar& tabBar, RECT& rc2Adjust);
}
