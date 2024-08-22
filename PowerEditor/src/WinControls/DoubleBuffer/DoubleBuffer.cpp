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

#include "DoubleBuffer.h"
#include <commctrl.h>
#include <cassert>
#include <memory>

void DoubleBuffer::subclass(HWND hWnd)
{
	std::unique_ptr<DoubleBuffer> self(new DoubleBuffer);
	if (::SetWindowSubclass(hWnd, s_subclassWndProc, 0, reinterpret_cast<DWORD_PTR>(self.get())))
	{
		self.release();
	}
}

DoubleBuffer::~DoubleBuffer()
{
	if (_hBufferBitmap)
	{
		::SelectObject(_hMemoryDC, _hDefaultBitmap);
		::DeleteObject(_hBufferBitmap);
	}

	if (_hMemoryDC)
	{
		::DeleteDC(_hMemoryDC);
	}
}

void DoubleBuffer::prepareBuffer(HDC hOriginalDC, const RECT& clientRc)
{
	if (!_hMemoryDC)
	{
		_hMemoryDC = ::CreateCompatibleDC(hOriginalDC);
	}

	if (!_hBufferBitmap || _size.cx != clientRc.right || _size.cy != clientRc.bottom)
	{
		_size = { clientRc.right, clientRc.bottom };

		if (_hBufferBitmap)
		{
			::SelectObject(_hMemoryDC, _hDefaultBitmap);
			::DeleteObject(_hBufferBitmap);
		}

		_hBufferBitmap = ::CreateCompatibleBitmap(hOriginalDC, _size.cx, _size.cy);
		if (_hBufferBitmap)
		{
			_hDefaultBitmap = static_cast<HBITMAP>(::SelectObject(_hMemoryDC, _hBufferBitmap));
		}
	}
}

LRESULT CALLBACK DoubleBuffer::s_subclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
	return reinterpret_cast<DoubleBuffer*>(dwRefData)->subclassWndProc(hWnd, uMsg, wParam, lParam);
}

LRESULT DoubleBuffer::subclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC hPaintDC = ::BeginPaint(hWnd, &ps);
			if (!hPaintDC)
			{
				// Something is wrong, no point trying to do anything further
				return 0;
			}

			if (ps.rcPaint.right <= ps.rcPaint.left || ps.rcPaint.bottom <= ps.rcPaint.top)
			{
				// There is nothing to actually paint
				::EndPaint(hWnd, &ps);
				return 0;
			}

			RECT clientRc{};
			::GetClientRect(hWnd, &clientRc);
			prepareBuffer(hPaintDC, clientRc);

			// Save DC state and set an appropriate clipping rectangle every time. There are two reasons:
			// - Set the clipping at all, to speed up drawing operations the same as a real WM_PAINT would do
			// - Reset the clipping region, as some controls leave something set on the DC (e.g. the Tab control)
			int savedState = ::SaveDC(_hMemoryDC);
			assert(ps.rcPaint.left  <  _size.cx && ps.rcPaint.top    <  _size.cy);
			assert(ps.rcPaint.right <= _size.cx && ps.rcPaint.bottom <= _size.cy);
			::IntersectClipRect(_hMemoryDC, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

			// If not further overriden, DefWindowProc() will send WM_ERASEBKGND and WM_PRINTCLIENT messages
			// to the window. At least the latter must be supported by the target for all this to work.
			// The background must be erased always, the contents of the memory DC cannot rely on ps.fErase.
			::DefSubclassProc(hWnd, WM_PRINT, reinterpret_cast<WPARAM>(_hMemoryDC), PRF_ERASEBKGND | PRF_CLIENT);

			::RestoreDC(_hMemoryDC, savedState);
			::BitBlt(
				hPaintDC, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
				_hMemoryDC, ps.rcPaint.left, ps.rcPaint.top,
				SRCCOPY);
			::EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_ERASEBKGND:
		{
			if (reinterpret_cast<HDC>(wParam) == _hMemoryDC)
			{
				// The target DC is our back buffer, so drawing anything there is safe.
				// Proceed to the original window procedure and let it do whatever it wants.
				break;
			}
			else
			{
				// This is BeginPaint() calling from WM_PAINT above, or some ad-hoc WM_ERASEBKGND
				// triggered by other operations. Erasing background directly in the window DC passed
				// in wParam now would be done unbuffered and thus prone to causing visual glitches.
				// Do nothing and return FALSE, which will only set fErase in PAINTSTRUCT for later.
				return FALSE;
			}
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, s_subclassWndProc, 0);
			delete this;
			break;
		}
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
