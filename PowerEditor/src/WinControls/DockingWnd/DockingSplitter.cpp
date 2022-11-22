// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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


#include <stdexcept>
#include "DockingSplitter.h"
#include "Notepad_plus_msgs.h"
#include "Parameters.h"
#include "NppDarkMode.h"

BOOL DockingSplitter::_isVertReg = FALSE;
BOOL DockingSplitter::_isHoriReg = FALSE;

void DockingSplitter::init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags)
{
	Window::init(hInst, hWnd);
	_hMessage = hMessage;
	_flags = flags;

	WNDCLASS wc;
	DWORD hwndExStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	_isRTL = hwndExStyle & WS_EX_LAYOUTRTL;

	if (flags & DMS_HORIZONTAL)
	{
		//double sided arrow pointing north-south as cursor
		wc.hCursor			= ::LoadCursor(NULL,IDC_SIZENS);
		wc.lpszClassName	= TEXT("nsdockspliter");
	}
	else
	{
		// double sided arrow pointing east-west as cursor
		wc.hCursor			= ::LoadCursor(NULL,IDC_SIZEWE);
		wc.lpszClassName	= TEXT("wedockspliter");
	}

	if (((_isHoriReg == FALSE) && (flags & DMS_HORIZONTAL)) ||
		((_isVertReg == FALSE) && (flags & DMS_VERTICAL)))
	{
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = staticWinProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInst;
		wc.hIcon = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName = NULL;

		if (!::RegisterClass(&wc))
		{
			throw std::runtime_error("DockingSplitter::init : RegisterClass() function failed");
		}
		else if (flags & DMS_HORIZONTAL)
		{
			_isHoriReg	= TRUE;
		}
		else
		{
			_isVertReg	= TRUE;
		}
	}

	/* create splitter windows and initialize it */
	_hSelf = ::CreateWindowEx( 0, wc.lpszClassName, TEXT(""), WS_CHILD | WS_VISIBLE,
								CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								_hParent, NULL, _hInst, (LPVOID)this);

	if (!_hSelf)
	{
		throw std::runtime_error("DockingSplitter::init : CreateWindowEx() function return null");
	}
}



LRESULT CALLBACK DockingSplitter::staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DockingSplitter *pDockingSplitter = NULL;
	switch (message)
	{
		case WM_NCCREATE :
			pDockingSplitter = reinterpret_cast<DockingSplitter *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
			pDockingSplitter->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDockingSplitter));
			return TRUE;

		default :
			pDockingSplitter = reinterpret_cast<DockingSplitter *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!pDockingSplitter)
				return ::DefWindowProc(hwnd, message, wParam, lParam);
			return pDockingSplitter->runProc(hwnd, message, wParam, lParam);
	}
}


LRESULT DockingSplitter::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			::SetCapture(_hSelf);
			::GetCursorPos(&_ptOldPos);
			_isLeftButtonDown = TRUE;
			break;
		}
		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
		{
			::ReleaseCapture();
			_isLeftButtonDown = FALSE;
			break;
		}
		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		{
			if (_isLeftButtonDown == TRUE)
			{
				POINT	pt;

				::GetCursorPos(&pt);

				if ((_flags & DMS_HORIZONTAL) && (_ptOldPos.y != pt.y))
				{
					::SendMessage(_hMessage, DMM_MOVE_SPLITTER, _ptOldPos.y - pt.y, reinterpret_cast<LPARAM>(_hSelf));
				}
				else if (_ptOldPos.x != pt.x)
				{
					::SendMessage(_hMessage, DMM_MOVE_SPLITTER, _isRTL ? pt.x - _ptOldPos.x : _ptOldPos.x - pt.x, reinterpret_cast<LPARAM>(_hSelf));
				}
				_ptOldPos = pt;
			}
			break;
		}
		case WM_ERASEBKGND:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			RECT rc = {};
			getClientRect(rc);
			::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getBackgroundBrush());
			return TRUE;
		}
		default :
			break;
	}
	return ::DefWindowProc(hwnd, message, wParam, lParam);
}




