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

// "Smooth scrolling (by pixels)" of the two editor views (Preferences > Editing 1)
namespace SmoothScrolling
{
	// Vertical mouse wheel on an editor, called by ScintillaEditView::ScintillaProc after the original code has
	// chosen the view to scroll and handled the right button and Shift. Returns true when the message has been
	// handled (option on, one of the two editor views); Ctrl + wheel (zoom) is left to Scintilla.
	bool onMouseWheel(HWND hwnd, WPARAM wParam);

	// Horizontal wheel (WM_MOUSEHWHEEL) or Shift + wheel on an editor, called by ScintillaEditView::ScintillaProc
	// once the original code has chosen the view to scroll. Returns true when handled.
	bool onHorizontalWheel(HWND hwnd, UINT message, WPARAM wParam);

	// Option changed, or a document activated (the editors exist): with the option on, the two editor views get
	// our top border and scrollbar; off, everything goes back to the original windows
	void apply(HWND mainEditor, HWND subEditor);
}
