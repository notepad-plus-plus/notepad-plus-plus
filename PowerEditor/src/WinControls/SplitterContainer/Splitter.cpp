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
#include "Splitter.h"
#include "Parameters.h"
#include "NppDarkMode.h"

bool Splitter::_isHorizontalRegistered = false;
bool Splitter::_isVerticalRegistered = false;
bool Splitter::_isHorizontalFixedRegistered = false;
bool Splitter::_isVerticalFixedRegistered = false;


#define SPLITTER_SIZE 8

void Splitter::init( HINSTANCE hInst, HWND hPere, int splitterSize, double iSplitRatio, DWORD dwFlags)
{
	if (hPere == NULL)
		throw std::runtime_error("Splitter::init : Parameter hPere is null");

	if (iSplitRatio < 0)
		throw std::runtime_error("Splitter::init : Parameter iSplitRatio shoulds be 0 < ratio < 100");

	Window::init(hInst, hPere);
	_splitterSize = splitterSize;

	WNDCLASSEX wcex;
	DWORD dwExStyle = 0L;
	DWORD dwStyle   = WS_CHILD | WS_VISIBLE;


	_hParent = hPere;
	_dwFlags = dwFlags;

	if (_dwFlags & SV_FIXED)
	{
		//Fixed spliter
		_isFixed = true;
	}
	else
	{
		if (iSplitRatio >= 100)
		{
			//cant be 100 % or more
			throw std::runtime_error("Splitter::init : Parameter iSplitRatio shoulds be 0 < ratio < 100");
		}
	}

	_splitPercent = iSplitRatio;

	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)staticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= _hInst;
	wcex.hIcon			= NULL;

	::GetClientRect(_hParent, &_rect);

	if (_dwFlags & SV_HORIZONTAL) //Horizontal spliter
	{
		_rect.top  = (LONG)((_rect.bottom * _splitPercent)/100) - _splitterSize / 2;
		// y axis determined by the split% of the parent windows height

		_rect.left = 0;
		// x axis is always 0

		_rect.bottom = _splitterSize;
		// the height of the spliter

		// the width of the splitter remains the same as the width of the parent window.
	}
	else //Vertical spliter
	{
		// y axis is 0 always

		_rect.left = (LONG)((_rect.right * _splitPercent)/100) - _splitterSize / 2;
		// x axis determined by split% of the parent windows width.

		_rect.right = _splitterSize;
		// width of the spliter.

		//height of the spliter remains the same as the height of the parent window
	}

	if (!_isFixed)
	{
		if ((_dwFlags & SV_ENABLERDBLCLK) || (_dwFlags & SV_ENABLELDBLCLK))
		{
			wcex.style = wcex.style | CS_DBLCLKS;
			// enable mouse double click messages.
		}
	}

	if (_isFixed)
	{
		wcex.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
		// if fixed spliter then choose default cursor type.
		if (_dwFlags & SV_HORIZONTAL)
			wcex.lpszClassName	= L"fxdnsspliter";
		else
			wcex.lpszClassName	= L"fxdwespliter";
	}
	else
	{
		if (_dwFlags & SV_HORIZONTAL)
		{
			//double sided arrow pointing north-south as cursor
			wcex.hCursor		= ::LoadCursor(NULL,IDC_SIZENS);
			wcex.lpszClassName	= L"nsspliter";
		}
		else
		{
			// double sided arrow pointing east-west as cursor
			wcex.hCursor		= ::LoadCursor(NULL,IDC_SIZEWE);
			wcex.lpszClassName	= L"wespliter";
		}
	}

	wcex.hbrBackground	= (HBRUSH)(COLOR_3DFACE+1);
	wcex.lpszMenuName	= NULL;
	wcex.hIconSm		= NULL;

	if ((_dwFlags & SV_HORIZONTAL)&&(!_isHorizontalRegistered))
	{
		RegisterClassEx(&wcex);
		_isHorizontalRegistered = true;
	}
	else if (isVertical()&&(!_isVerticalRegistered))
	{
		RegisterClassEx(&wcex);
		_isVerticalRegistered = true;
	}
	else if ((_dwFlags & SV_HORIZONTAL)&&(!_isHorizontalFixedRegistered))
	{
		RegisterClassEx(&wcex);
		_isHorizontalFixedRegistered = true;
	}
	else if (isVertical()&&(!_isVerticalFixedRegistered))
	{
		RegisterClassEx(&wcex);
		_isVerticalFixedRegistered = true;
	}

	_hSelf = CreateWindowEx(dwExStyle, wcex.lpszClassName,
		L"",
		dwStyle,
		_rect.left, _rect.top, _rect.right, _rect.bottom,
		_hParent, NULL, _hInst, this);

	if (!_hSelf)
		throw std::runtime_error("Splitter::init : CreateWindowEx() function return null");

	RECT rc;
	getClientRect(rc);
	//::GetClientRect(_hParent,&rc);

	_clickZone2TL.left   = rc.left;
	_clickZone2TL.top    = rc.top;

	int clickZoneWidth   = getClickZone(WH::width);
	int clickZoneHeight  = getClickZone(WH::height);
	_clickZone2TL.right  = clickZoneWidth;
	_clickZone2TL.bottom = clickZoneHeight;

	_clickZone2BR.left   = rc.right - clickZoneWidth;
	_clickZone2BR.top    = rc.bottom - clickZoneHeight;
	_clickZone2BR.right  = clickZoneWidth;
	_clickZone2BR.bottom = clickZoneHeight;

	display();
	::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
}


void Splitter::destroy()
{
	::DestroyWindow(_hSelf);
}


int Splitter::getClickZone(WH which)
{
	// determinated by (_dwFlags & SV_VERTICAL) && _splitterSize
	if (_splitterSize <= 8)
	{
		return isVertical()
			? (which == WH::width ? _splitterSize  : HIEGHT_MINIMAL)
			: (which == WH::width ? HIEGHT_MINIMAL : _splitterSize);
	}
	else // (_spiltterSize > 8)
	{
		return isVertical()
			? ((which == WH::width) ? 8  : 15)
			: ((which == WH::width) ? 15 : 8);
	}
}


LRESULT CALLBACK Splitter::staticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_NCCREATE:
		{
			Splitter * pSplitter = static_cast<Splitter *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
			pSplitter->_hSelf = hWnd;
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pSplitter));
			return TRUE;
		}
		default:
		{
			Splitter * pSplitter = (Splitter *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (!pSplitter)
				return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

			return pSplitter->spliterWndProc(uMsg, wParam, lParam);
		}
	}
}


LRESULT CALLBACK Splitter::spliterWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_LBUTTONDOWN:
		{
			POINT p;
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);

			if ((isInLeftTopZone(p))&&(wParam == MK_LBUTTON))
			{
				gotoTopLeft();
				return TRUE;
			}

			if ((isInRightBottomZone(p))&&(wParam == MK_LBUTTON))
			{
				gotoRightBottom();
				return TRUE;
			}

			if (!_isFixed)
			{
				::SetCapture(_hSelf);
				_isDraged = true;
				_isLeftButtonDown = true;
			}

			return 0;
		}

		case WM_RBUTTONDOWN:
		{
			::SendMessage(_hParent, WM_DOPOPUPMENU, wParam, lParam);
			return TRUE;
		}

		case WM_MOUSEMOVE:
		{
			POINT p;
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);

			if (isInLeftTopZone(p) || isInRightBottomZone(p))
			{
				//::SetCursor(::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_UP_ARROW)));
				::SetCursor(::LoadCursor(NULL, IDC_HAND));
				return TRUE;
			}

			if ((!_isFixed) && (wParam == MK_LBUTTON) && _isLeftButtonDown)
			{
				POINT pt; RECT rt;
				::GetClientRect(_hParent, &rt);

				::GetCursorPos(&pt);
				::ScreenToClient(_hParent, &pt);

				if (_dwFlags & SV_HORIZONTAL)
				{
					if (pt.y <= 1)
					{
						_rect.top = 1;
						_splitPercent = 1;
					}
					else
					{
						if (pt.y <= (rt.bottom - 5))
						{
							_rect.top = pt.y;
							_splitPercent = ((pt.y * 100 / (double)rt.bottom*100) / 100);
						}
						else
						{
							_rect.top = rt.bottom - 5;
							_splitPercent = 99;
						}
					}
				}
				else
				{
					if (pt.x <= 1)
					{
						_rect.left = 1;
						_splitPercent = 1;
					}
					else
					{
						if (pt.x <= (rt.right - 5))
						{
							_rect.left = pt.x;
							_splitPercent = ((pt.x*100 / (double)rt.right*100) / 100);
						}
						else
						{
							_rect.left = rt.right - 5;
							_splitPercent = 99;
						}
					}
				}

				::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
				::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, FALSE);
				redraw();
			}
			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			RECT r;
			::GetClientRect(_hParent, &r);

			if (_dwFlags & SV_HORIZONTAL)
			{
				_rect.top = (r.bottom - _splitterSize) / 2;
			}
			else
			{
				_rect.left = (r.right - _splitterSize) / 2;
			}

			_splitPercent = 50;

			::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
			::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, FALSE);
			redraw();

			return 0;
		}

		case WM_LBUTTONUP:
		{
			if (!_isFixed && _isLeftButtonDown)
			{
				ReleaseCapture();
				_isLeftButtonDown = false;
			}
			return 0;
		}

		case WM_CAPTURECHANGED:
		{
			if (_isDraged)
			{
				::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
				::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, TRUE);
				_isDraged = false;
			}
			return 0;
		}

		case WM_ERASEBKGND:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;
			}

			RECT rc{};
			getClientRect(rc);
			::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDlgBackgroundBrush());
			return 1;
		}

		case WM_PAINT:
		{
			drawSplitter();
			return 0;
		}

		case WM_CLOSE:
		{
			destroy();
			return 0;
		}
	}

	return ::DefWindowProc(_hSelf, uMsg, wParam, lParam);
}


void Splitter::resizeSpliter(RECT *pRect)
{
	RECT rect{};

	if (pRect)
		rect = *pRect;
	else
		::GetClientRect(_hParent,&rect);

	if (_dwFlags & SV_HORIZONTAL)
	{
		// for a Horizontal spliter the width remains the same
		// as the width of the parent window, so get the new width of the parent.
		_rect.right = rect.right;

		//if resizeing should be done proportionately.
		if (_dwFlags & SV_RESIZEWTHPERCNT)
			_rect.top  = (LONG)((rect.bottom * _splitPercent)/100) - _splitterSize / 2;
		else // soit la fenetre en haut soit la fenetre en bas qui est fixee
			_rect.top = getSplitterFixPosY();
	}
	else
	{
		// for a (default) Vertical spliter the height remains the same
		// as the height of the parent window, so get the new height of the parent.
		_rect.bottom = rect.bottom;

		//if resizeing should be done proportionately.
		if (_dwFlags & SV_RESIZEWTHPERCNT)
		{
			_rect.left = (LONG)((rect.right * _splitPercent)/100) - _splitterSize / 2;
		}
		else // soit la fenetre gauche soit la fenetre droit qui est fixee
			_rect.left = getSplitterFixPosX();

	}
	::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, TRUE);
	::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);

	RECT rc;
	getClientRect(rc);
	_clickZone2BR.right = getClickZone(WH::width);
	_clickZone2BR.bottom = getClickZone(WH::height);
	_clickZone2BR.left = rc.right - _clickZone2BR.right;
	_clickZone2BR.top = rc.bottom - _clickZone2BR.bottom;


	//force the window to repaint, to make sure the splitter is visible
	// in the new position.
	redraw();
}


void Splitter::gotoTopLeft()
{
	if ((_dwFlags & SV_ENABLELDBLCLK) && (!_isFixed) && (_splitPercent > 1))
	{
		if (_dwFlags & SV_HORIZONTAL)
			_rect.top  = 1;
		else
			_rect.left = 1;

		_splitPercent = 1;

		::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
		::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, TRUE);
		redraw();
	}
}


void Splitter::gotoRightBottom()
{
	if ((_dwFlags & SV_ENABLERDBLCLK) && (!_isFixed) && (_splitPercent < 99))
	{
		RECT rt;
		GetClientRect(_hParent,&rt);

		if (_dwFlags & SV_HORIZONTAL)
			_rect.top   = rt.bottom - _splitterSize;
		else
			_rect.left   = rt.right - _splitterSize;

		_splitPercent = 99;

		::SendMessage(_hParent, WM_RESIZE_CONTAINER, _rect.left, _rect.top);
		::MoveWindow(_hSelf, _rect.left, _rect.top, _rect.right, _rect.bottom, TRUE);
		redraw();
	}
}


void Splitter::drawSplitter()
{
	PAINTSTRUCT ps;
	RECT rc, rcToDraw1, rcToDraw2, TLrc, BRrc;

	HDC hdc = ::BeginPaint(_hSelf, &ps);
	getClientRect(rc);

	bool isDarkMode = NppDarkMode::isEnabled();

	HBRUSH hBrush = nullptr;
	HBRUSH hBrushTop = nullptr;

	HPEN holdPen = nullptr;
	if (isDarkMode)
	{
		hBrush = NppDarkMode::getBackgroundBrush();
		hBrushTop = NppDarkMode::getCtrlBackgroundBrush();

		holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getDarkerTextPen()));
		::FillRect(hdc, &rc, NppDarkMode::getDlgBackgroundBrush());
	}
	else
	{
		hBrush = ::CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
		hBrushTop = ::GetSysColorBrush(COLOR_3DSHADOW);
	}

	DPIManager& dpiMgr = NppParameters::getInstance()._dpiManager;

	if ((_splitterSize >= dpiMgr.scaleX(4)) && (_dwFlags & SV_RESIZEWTHPERCNT))
	{
		adjustZoneToDraw(TLrc, ZONE_TYPE::topLeft);
		adjustZoneToDraw(BRrc, ZONE_TYPE::bottomRight);
		paintArrow(hdc, TLrc, isVertical() ? Arrow::left : Arrow::up);
	}

	if (isVertical())
	{
		rcToDraw2.top    = (_dwFlags & SV_RESIZEWTHPERCNT) ? _clickZone2TL.bottom : 0;
		rcToDraw2.bottom = rcToDraw2.top + dpiMgr.scaleX(2);

		rcToDraw1.top    = rcToDraw2.top + dpiMgr.scaleX(1);
		rcToDraw1.bottom = rcToDraw1.top + dpiMgr.scaleX(2);
	}
	else
	{
		rcToDraw2.top    = dpiMgr.scaleX(1);
		rcToDraw2.bottom = dpiMgr.scaleX(3);

		rcToDraw1.top    = dpiMgr.scaleX(2);
		rcToDraw1.bottom = dpiMgr.scaleX(4);
	}

	int bottom = 0;
	if (_dwFlags & SV_RESIZEWTHPERCNT)
		bottom = (isVertical() ? rc.bottom - _clickZone2BR.bottom : rc.bottom);
	else
		bottom = rc.bottom;

	while (rcToDraw1.bottom <= bottom)
	{
		if (isVertical())
		{
			rcToDraw2.left  = dpiMgr.scaleX(1);
			rcToDraw2.right = dpiMgr.scaleX(3);

			rcToDraw1.left  = dpiMgr.scaleX(2);
			rcToDraw1.right = dpiMgr.scaleX(4);
		}
		else
		{
			rcToDraw2.left = _clickZone2TL.right;
			rcToDraw2.right = rcToDraw2.left + dpiMgr.scaleX(2);

			rcToDraw1.left = rcToDraw2.left;
			rcToDraw1.right = rcToDraw1.left + dpiMgr.scaleX(2);
		}

		int n = dpiMgr.scaleX(4);
		while (rcToDraw1.right <= (isVertical() ? rc.right : rc.right - _clickZone2BR.right))
		{
			::FillRect(hdc, &rcToDraw1, hBrush);
			::FillRect(hdc, &rcToDraw2, hBrushTop);

			rcToDraw2.left  += n;
			rcToDraw2.right += n;
			rcToDraw1.left  += n;
			rcToDraw1.right += n;
		}

		rcToDraw2.top    += n;
		rcToDraw2.bottom += n;
		rcToDraw1.top    += n;
		rcToDraw1.bottom += n;
	}

	if ((_splitterSize >= dpiMgr.scaleX(4)) && (_dwFlags & SV_RESIZEWTHPERCNT))
		paintArrow(hdc, BRrc, isVertical() ? Arrow::right : Arrow::down);

	if (isDarkMode)
	{
		::SelectObject(hdc, holdPen);
	}
	else
	{
		::DeleteObject(hBrush);
	}

	::EndPaint(_hSelf, &ps);
}


void Splitter::rotate()
{
	if (!_isFixed)
	{
		destroy();

		if (_dwFlags & SV_HORIZONTAL)
		{
			_dwFlags ^= SV_HORIZONTAL;
			_dwFlags |= SV_VERTICAL;
		}
		else //SV_VERTICAL
		{
			_dwFlags ^= SV_VERTICAL;
			_dwFlags |= SV_HORIZONTAL;
		}

		init(_hInst, _hParent, _splitterSize, _splitPercent, _dwFlags);
	}
}


void Splitter::paintArrow(HDC hdc, const RECT &rect, Arrow arrowDir)
{
	RECT rc;
	rc.left = rect.left;
	rc.top = rect.top;
	rc.right = rect.right;
	rc.bottom = rect.bottom;

	switch (arrowDir)
	{
		case Arrow::left:
		{
			int x = rc.right;
			int y = rc.top;

			for (; (x > rc.left) && (y != rc.bottom) ; --x)
			{
				::MoveToEx(hdc, x, y++, NULL);
				::LineTo(hdc, x, rc.bottom--);
			}
			break;
		}

		case Arrow::right:
		{
			int x = rc.left;
			int y = rc.top;

			for (; (x < rc.right) && (y != rc.bottom) ; ++x)
			{
				::MoveToEx(hdc, x, y++, NULL);
				::LineTo(hdc, x, rc.bottom--);
			}
			break;
		}

		case Arrow::up:
		{
			int x = rc.left;
			int y = rc.bottom;

			for (; (y > rc.top) && (x != rc.right) ; --y)
			{
				::MoveToEx(hdc, x++, y, NULL);
				::LineTo(hdc, rc.right--, y);
			}
			break;
		}

		case Arrow::down:
		{
			int x = rc.left;
			int y = rc.top;

			for (; (y < rc.bottom) && (x != rc.right) ; ++y)
			{
				::MoveToEx(hdc, x++, y, NULL);
				::LineTo(hdc, rc.right--, y);
			}
			break;
		}
	}
}


void Splitter::adjustZoneToDraw(RECT& rc2def, ZONE_TYPE whichZone)
{
	if (_splitterSize < 4)
		return;

	int x0, y0, x1, y1, w, h;

	if (/*(4 <= _splitterSize) && */(_splitterSize <= 8))
	{
		w = (isVertical() ? 4 : 7);
		h = (isVertical() ? 7 : 4);
	}
	else // (_spiltterSize > 8)
	{
		w = (isVertical() ? 6  : 11);
		h = (isVertical() ? 11 : 6);
	}

	if (isVertical())
	{
		// w=4 h=7
		if (whichZone == ZONE_TYPE::topLeft)
		{
			x0 = 0;
			y0 = (_clickZone2TL.bottom - h) / 2;
		}
		else // whichZone == BOTTOM_RIGHT
		{
			x0 = _clickZone2BR.left + _clickZone2BR.right - w;
			y0 = (_clickZone2BR.bottom - h) / 2 + _clickZone2BR.top;
		}

		x1 = x0 + w;
		y1 = y0 + h;
	}
	else // Horizontal
	{
		//w=7 h=4
		if (whichZone == ZONE_TYPE::topLeft)
		{
			x0 = (_clickZone2TL.right - w) / 2;
			y0 = 0;
		}
		else // whichZone == BOTTOM_RIGHT
		{
			x0 = ((_clickZone2BR.right - w) / 2) + _clickZone2BR.left;
			y0 = _clickZone2BR.top + _clickZone2BR.bottom - h;
		}

		x1 = x0 + w;
		y1 = y0 + h;
	}

	rc2def.left = x0;
	rc2def.top = y0;
	rc2def.right = x1;
	rc2def.bottom = y1;
}
