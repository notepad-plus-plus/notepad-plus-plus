// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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
#include <iostream>
#include <stdexcept>
#include <windows.h>
#include "SplitterContainer.h"
#include "Parameters.h"
#include "localization.h"
#include <cassert>



bool SplitterContainer::_isRegistered = false;



void SplitterContainer::create(Window *pWin0, Window *pWin1, int splitterSize, SplitterMode mode, int ratio, bool isVertical)
{
	//Window::init(hInst, parent);
	_pWin0 = pWin0;
	_pWin1 = pWin1;
	_splitterSize = splitterSize;
	_splitterMode = mode;
	_ratio = ratio;
	_dwSplitterStyle |= isVertical?SV_VERTICAL:SV_HORIZONTAL;

	if (_splitterMode != SplitterMode::DYNAMIC)
	{
		_dwSplitterStyle |= SV_FIXED;
		_dwSplitterStyle &= ~SV_RESIZEWTHPERCNT;
	}
	if (!_isRegistered)
	{
		WNDCLASS splitterContainerClass;

		splitterContainerClass.style = CS_DBLCLKS;
		splitterContainerClass.lpfnWndProc = staticWinProc;
		splitterContainerClass.cbClsExtra = 0;
		splitterContainerClass.cbWndExtra = 0;
		splitterContainerClass.hInstance = _hInst;
		splitterContainerClass.hIcon = NULL;
		splitterContainerClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);

		// hbrBackground must be NULL,
		// otherwise this window will hide some parts of 2 windows
		splitterContainerClass.hbrBackground = NULL;
		splitterContainerClass.lpszMenuName = NULL;
		splitterContainerClass.lpszClassName = SPC_CLASS_NAME;

		if (!::RegisterClass(&splitterContainerClass))
			throw std::runtime_error(" SplitterContainer::create : RegisterClass() function failed");

		_isRegistered = true;
	}

	_hSelf = ::CreateWindowEx(
		0, SPC_CLASS_NAME, TEXT("a koi sert?"),
		WS_CHILD | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		_hParent, NULL, _hInst, this);

	if (!_hSelf)
		throw std::runtime_error(" SplitterContainer::create : CreateWindowEx() function return null");
}


void SplitterContainer::destroy()
{
	if (_hPopupMenu)
		::DestroyMenu(_hPopupMenu);

	_splitter.destroy();
	::DestroyWindow(_hSelf);
}


void SplitterContainer::reSizeTo(RECT & rc)
{
	_x = rc.left;
	_y = rc.top;
	::MoveWindow(_hSelf, _x, _y, rc.right, rc.bottom, FALSE);
	_splitter.resizeSpliter();
}


void SplitterContainer::display(bool toShow) const
{
	Window::display(toShow);

	assert(_pWin0 != nullptr);
	assert(_pWin1 != nullptr);
	_pWin0->display(toShow);
	_pWin1->display(toShow);

	_splitter.display(toShow);
}


void SplitterContainer::redraw(bool forceUpdate) const
{
	assert(_pWin0 != nullptr);
	assert(_pWin1 != nullptr);
	_pWin0->redraw(forceUpdate);
	_pWin1->redraw(forceUpdate);
}


void SplitterContainer::rotateTo(DIRECTION direction)
{
	bool doSwitchWindow = false;
	if (_dwSplitterStyle & SV_VERTICAL)
	{
		_dwSplitterStyle ^= SV_VERTICAL;
		_dwSplitterStyle |= SV_HORIZONTAL;
		doSwitchWindow = (direction == DIRECTION::LEFT);
	}
	else
	{
		_dwSplitterStyle ^= SV_HORIZONTAL;
		_dwSplitterStyle |= SV_VERTICAL;
		doSwitchWindow = (direction == DIRECTION::RIGHT);
	}
	if (doSwitchWindow)
	{
		Window *tmp = _pWin0;
		_pWin0 = _pWin1;
		_pWin1 = tmp;
	}
	_splitter.rotate();

}

LRESULT CALLBACK SplitterContainer::staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SplitterContainer *pSplitterContainer = NULL;
	switch (message)
	{
		case WM_NCCREATE:
		{
			pSplitterContainer = reinterpret_cast<SplitterContainer *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
			pSplitterContainer->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pSplitterContainer));
			return TRUE;
		}

		default:
		{
			pSplitterContainer = (SplitterContainer *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (!pSplitterContainer)
				return ::DefWindowProc(hwnd, message, wParam, lParam);
			return pSplitterContainer->runProc(message, wParam, lParam);
		}
	}
}


LRESULT SplitterContainer::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			_splitter.init(_hInst, _hSelf, _splitterSize, _ratio, _dwSplitterStyle);
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case ROTATION_LEFT:
				{
					rotateTo(DIRECTION::LEFT);
					break;
				}
				case ROTATION_RIGHT:
				{
					rotateTo(DIRECTION::RIGHT);
					break;
				}
			}
			return TRUE;
		}

		case WM_RESIZE_CONTAINER:
		{
			RECT rc0, rc1;
			getClientRect(rc0);

			rc1.top = rc0.top += _y;
			rc1.bottom = rc0.bottom;
			rc1.left = rc0.left += _x;
			rc1.right = rc0.right;

			if (_dwSplitterStyle & SV_VERTICAL)
			{
				if (wParam != 0)
				{
					rc0.right = int(wParam);

					rc1.left = int(wParam) + _x + _splitter.getPhisicalSize();
					rc1.right = rc1.right - rc1.left + _x;
				}
			}
			else //SV_HORIZONTAL
			{
				if (lParam != 0)
				{
					rc0.bottom = int(lParam);

					rc1.top   = int(lParam) + _y + _splitter.getPhisicalSize();
					rc1.bottom = rc1.bottom - rc1.top + _y;
				}
			}
			_pWin0->reSizeTo(rc0);
			_pWin1->reSizeTo(rc1);

			::InvalidateRect(_splitter.getHSelf(), NULL, TRUE);
			return TRUE;
		}

		case WM_DOPOPUPMENU:
		{
			if ((_splitterMode != SplitterMode::LEFT_FIX) && (_splitterMode != SplitterMode::RIGHT_FIX) )
			{
				POINT p;
				::GetCursorPos(&p);

				if (!_hPopupMenu)
				{
					_hPopupMenu = ::CreatePopupMenu();

					NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
					const generic_string textLeft =
						nativeLangSpeaker->getLocalizedStrFromID("splitter-rotate-left", TEXT("Rotate to left"));
					const generic_string textRight =
						nativeLangSpeaker->getLocalizedStrFromID("splitter-rotate-right", TEXT("Rotate to right"));

					::InsertMenu(_hPopupMenu, 1, MF_BYPOSITION, ROTATION_LEFT, textLeft.c_str());
					::InsertMenu(_hPopupMenu, 0, MF_BYPOSITION, ROTATION_RIGHT, textRight.c_str());
				}

				::TrackPopupMenu(_hPopupMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
			}
			return TRUE;
		}

		case WM_GETSPLITTER_X:
		{
			switch (_splitterMode)
			{
				case SplitterMode::LEFT_FIX:
				{
					return MAKELONG(_pWin0->getWidth(), static_cast<std::uint8_t>(SplitterMode::LEFT_FIX));
				}

				case SplitterMode::RIGHT_FIX:
				{
					int x = getWidth()-_pWin1->getWidth();
					if (x < 0)
						x = 0;
					return MAKELONG(x, static_cast<std::uint8_t>(SplitterMode::RIGHT_FIX));
				}
				default:
					break;
			}
			return MAKELONG(0, static_cast<std::uint8_t>(SplitterMode::DYNAMIC));
		}

		case WM_GETSPLITTER_Y:
		{
			switch (_splitterMode)
			{
				case SplitterMode::LEFT_FIX:
				{
					return MAKELONG(_pWin0->getHeight(), static_cast<std::uint8_t>(SplitterMode::LEFT_FIX));
				}
				case SplitterMode::RIGHT_FIX:
				{
					int y = getHeight()-_pWin1->getHeight();
					if (y < 0)
						y = 0;
					return MAKELONG(y, static_cast<std::uint8_t>(SplitterMode::RIGHT_FIX));
				}
				default:
					break;
			}
			return MAKELONG(0, static_cast<std::uint8_t>(SplitterMode::DYNAMIC));
		}

		case WM_LBUTTONDBLCLK:
		{
			POINT pt;
			::GetCursorPos(&pt);
			::ScreenToClient(_splitter.getHSelf(), &pt);

			HWND parent = ::GetParent(getHSelf());

			Window* targetWindow = (isVertical())
				? (pt.x < 0 ? _pWin0 : _pWin1)
				: (pt.y < 0 ? _pWin0 : _pWin1);

			::SendMessage(parent, NPPM_INTERNAL_SWITCHVIEWFROMHWND, 0, reinterpret_cast<LPARAM>(targetWindow->getHSelf()));
			::SendMessage(parent, WM_COMMAND, IDM_FILE_NEW, 0);
			return TRUE;
		}

		default:
			return ::DefWindowProc(_hSelf, message, wParam, lParam);
	}
}
