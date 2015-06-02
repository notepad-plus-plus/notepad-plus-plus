// this file is part of docking functionality for Notepad++
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



#include "DockingSplitter.h"
#include "Notepad_plus_msgs.h"
#include "Parameters.h"

BOOL DockingSplitter::_isVertReg = FALSE;
BOOL DockingSplitter::_isHoriReg = FALSE;

static HWND		hWndMouse		= NULL;
static HHOOK	hookMouse		= NULL;

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif

static LRESULT CALLBACK hookProcMouse(UINT nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode >= 0)
    {
		switch (wParam)
		{
			case WM_MOUSEMOVE:
			case WM_NCMOUSEMOVE:
				::PostMessage(hWndMouse, wParam, 0, 0);
				break;
			case WM_LBUTTONUP:
			case WM_NCLBUTTONUP:
				::PostMessage(hWndMouse, wParam, 0, 0);
				return TRUE;
			default: 
				break;
		}
	}

	return ::CallNextHookEx(hookMouse, nCode, wParam, lParam);
}

void DockingSplitter::init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags) 
{
	Window::init(hInst, hWnd);
	_hMessage = hMessage;
	_flags = flags;

	WNDCLASS wc;

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
			pDockingSplitter = (DockingSplitter *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			pDockingSplitter->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pDockingSplitter);
			return TRUE;

		default :
			pDockingSplitter = (DockingSplitter *)::GetWindowLongPtr(hwnd, GWL_USERDATA);
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
			hWndMouse = hwnd;
			hookMouse = ::SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)hookProcMouse, _hInst, 0);
			if (!hookMouse)
			{
				DWORD dwError = ::GetLastError();
				TCHAR  str[128];
				::wsprintf(str, TEXT("GetLastError() returned %lu"), dwError);
				::MessageBox(NULL, str, TEXT("SetWindowsHookEx(MOUSE) failed on runProc"), MB_OK | MB_ICONERROR);
			}
			else
			{
				::SetCapture(_hSelf);
				::GetCursorPos(&_ptOldPos);
				_isLeftButtonDown = TRUE;
			}

			break;
		}
		case WM_LBUTTONUP:
		case WM_NCLBUTTONUP:
		{
			/* end hooking */
			if (hookMouse)
			{
				::UnhookWindowsHookEx(hookMouse);
				::SetCapture(NULL);
				hookMouse = NULL;
			}
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
					::SendMessage(_hMessage, DMM_MOVE_SPLITTER, (WPARAM)_ptOldPos.y - pt.y, (LPARAM)_hSelf);
				}
				else if (_ptOldPos.x != pt.x)
				{
					::SendMessage(_hMessage, DMM_MOVE_SPLITTER, (WPARAM)_ptOldPos.x - pt.x, (LPARAM)_hSelf);
				}
				_ptOldPos = pt;
			}
			break;
		}
		default :
			break;
	}
	return ::DefWindowProc(hwnd, message, wParam, lParam);
}




