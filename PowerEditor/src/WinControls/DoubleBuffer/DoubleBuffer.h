// This file is part of Notepad++ project
// Copyright (C) 2024 Jiri Hruska <jirka@fud.cz>

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

/// <summary>
/// Subclasses a window to use double buffering for paint operations
/// </summary>
/// <remarks>
/// Unfortunately, many controls - even those provided by Windows as part of
/// Common Controls, like Status bar or Tab bar - suffer from flickering and
/// other paint artifacts during frequent updates. This is mainly caused by
/// erasing background first in WM_ERASEBKGND and only then painting the
/// actual window content over it in WM_PAINT.
///
/// This class solves the problem by subclassing any window and transparently
/// allowing it to paint to a double/back buffer, thus avoiding the blinking.
///
/// The original window (or whatever parent window procedure) must support the
/// WM_PRINTCLIENT message. Most standard controls do, otherwise it is simple
/// to add - reuse WM_PAINT code, just get HDC from wParam instead of calling
/// BeginPaint() and EndPaint() later. If optimizing with ps.rcPaint, rely on
/// the clipping rectangle and functions like PtVisible() or RectVisible().
/// </remarks>
class DoubleBuffer final
{
public:
	static void subclass(HWND hWnd);
	~DoubleBuffer();

private:
	DoubleBuffer() = default;
	DoubleBuffer(const DoubleBuffer&) = delete;
	DoubleBuffer& operator=(const DoubleBuffer&) = delete;

	void prepareBuffer(HDC hOriginalDC, const RECT& clientRc);
	static LRESULT CALLBACK s_subclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData);
	LRESULT subclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	SIZE _size{};
	HDC _hMemoryDC = nullptr;
	HBITMAP _hDefaultBitmap = nullptr;
	HBITMAP _hBufferBitmap = nullptr;
};
