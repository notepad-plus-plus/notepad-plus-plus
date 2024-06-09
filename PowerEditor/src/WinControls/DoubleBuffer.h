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
#include <cassert>

class DoubleBuffer final
{
private:
	SIZE _size{};
	HDC _hMemoryDC = nullptr;
	HBITMAP _hDefaultBitmap = nullptr;
	HBITMAP _hBufferBitmap = nullptr;

public:
	DoubleBuffer() {}
	DoubleBuffer(const DoubleBuffer&) = delete;
	DoubleBuffer& operator=(const DoubleBuffer&) = delete;

	~DoubleBuffer()
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

	HDC beginPaint(HWND hWnd, PAINTSTRUCT* ps)
	{
		if (!::BeginPaint(hWnd, ps))
		{
			return nullptr;
		}

		if (!_hMemoryDC)
		{
			_hMemoryDC = ::CreateCompatibleDC(ps->hdc);
		}

		RECT clientRc{};
		::GetClientRect(hWnd, &clientRc);
		if (clientRc.right != _size.cx || clientRc.bottom != _size.cy || !_hBufferBitmap)
		{
			_size = { clientRc.right, clientRc.bottom };

			if (_hBufferBitmap)
			{
				::SelectObject(_hMemoryDC, _hDefaultBitmap);
				::DeleteObject(_hBufferBitmap);
			}

			_hBufferBitmap = ::CreateCompatibleBitmap(ps->hdc, _size.cx, _size.cy);
			_hDefaultBitmap = static_cast<HBITMAP>(::SelectObject(_hMemoryDC, _hBufferBitmap));
		}

		assert(ps->rcPaint.left  <  _size.cx && ps->rcPaint.top    <  _size.cy);
		assert(ps->rcPaint.right <= _size.cx && ps->rcPaint.bottom <= _size.cy);

		return _hMemoryDC;
	}

	void endPaint(HWND hWnd, PAINTSTRUCT* ps)
	{
		::BitBlt(
			ps->hdc, ps->rcPaint.left, ps->rcPaint.top, ps->rcPaint.right - ps->rcPaint.left, ps->rcPaint.bottom - ps->rcPaint.top,
			_hMemoryDC, ps->rcPaint.left, ps->rcPaint.top,
			SRCCOPY);

		::EndPaint(hWnd, ps);
	}

	int getWidth() const
	{
		return _size.cx;
	}

	int getHeight() const
	{
		return _size.cy;
	}
};
