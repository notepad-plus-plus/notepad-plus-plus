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

// Custom colours of the scrollbars of the two editors (Preferences > Editing 2 > Editor Scrollbars > Custom Colors),
// set in Style Configurator > Global Styles: "Editor scrollbar" and "Editor scrollbar hover".
// The scrollbars keep their native look and behaviour: only their colours change.
namespace EditorScrollbars
{
	// At startup, before the editor windows are created: with the option on, the editor scrollbars are
	// coloured from their first paint
	void init();

	// The option, a colour or the theme has changed:
	// - option on: the editor scrollbars get the colours (the hooks are installed the first time);
	// - option off: the editor scrollbars get their original theme back (the hooks stay, inactive).
	void apply(HWND mainEditor, HWND subEditor);

	// Called when the dark mode is refreshed, before the windows are themed again: switching the dark mode on
	// runs FixDarkScrollBar() (DarkMode.cpp) again, which replaces the OpenNcThemeData hook of comctl32
	void onDarkModeChanging();
}
