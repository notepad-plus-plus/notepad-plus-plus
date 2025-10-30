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

#include <stdexcept>
#include "ColourPicker.h"
#include "ColourPopup.h"
#include "NppDarkMode.h"

#include <commctrl.h>

void ColourPicker::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);

	_hSelf = ::CreateWindowEx(
		0,
		L"Button",
		L"F",
		WS_CHILD | WS_VISIBLE,
		0, 0, 25, 25,
		_hParent, nullptr, _hInst, nullptr);

	if (!_hSelf)
		throw std::runtime_error("ColourPicker::init : CreateWindowEx() function return null");

	static constexpr UINT_PTR idSubclassClrPicker = 123;
	::SetWindowSubclass(_hSelf, staticProc, idSubclassClrPicker, reinterpret_cast<DWORD_PTR>(this));
}


void ColourPicker::destroy()
{
	ColourPicker::destroyColorPopup();
	::DestroyWindow(_hSelf);
}

void ColourPicker::drawBackground(HDC hDC) const
{
	RECT rc;
	HBRUSH hbrush;

	if (!hDC)
		return;

	getClientRect(rc);
	hbrush = ::CreateSolidBrush(_currentColour);
	HGDIOBJ oldObj = ::SelectObject(hDC, hbrush);
	HPEN holdPen = nullptr;
	if (NppDarkMode::isEnabled())
	{
		holdPen = static_cast<HPEN>(::SelectObject(hDC, NppDarkMode::getEdgePen()));
	}
	::Rectangle(hDC, 0, 0, rc.right, rc.bottom);
	if (NppDarkMode::isEnabled() && holdPen)
	{
		::SelectObject(hDC, holdPen);
	}
	::SelectObject(hDC, oldObj);
	//FillRect(hDC, &rc, hbrush);
	::DeleteObject(hbrush);
}

void ColourPicker::drawForeground(HDC hDC) const
{
	RECT rc;

	if (!hDC || _isEnabled)
		return;

	int oldMode = ::SetBkMode(hDC, TRANSPARENT);
	getClientRect(rc);
	COLORREF strikeOut = RGB(0,0,0);
	if ((((_currentColour      ) & 0xFF) +
		 ((_currentColour >>  8) & 0xFF) +
		 ((_currentColour >> 16) & 0xFF)) < 200)	//check if the color is too dark, if so, use white strikeout
		strikeOut = RGB(0xFF,0xFF,0xFF);

	HBRUSH hbrush = ::CreateHatchBrush(HS_FDIAGONAL, strikeOut);
	HGDIOBJ oldObj = ::SelectObject(hDC, hbrush);
	::Rectangle(hDC, 0, 0, rc.right, rc.bottom);
	::SelectObject(hDC, oldObj);
	//FillRect(hDC, &rc, hbrush);
	::DeleteObject(hbrush);
	::SetBkMode(hDC, oldMode);
}

void ColourPicker::destroyColorPopup()
{
	if (_pColourPopup != nullptr)
	{
		_pColourPopup->destroy();
		delete _pColourPopup;
		_pColourPopup = nullptr;
	}
}

LRESULT CALLBACK ColourPicker::staticProc(
	HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto* cpData = reinterpret_cast<ColourPicker*>(dwRefData);

	switch (Message)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hwnd, staticProc, uIdSubclass);
			break;
		}

		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			RECT rc{};
			cpData->Window::getClientRect(rc);
			::InflateRect(&rc, -2, -2);
			POINT p{ rc.left, rc.top + rc.bottom };
			::ClientToScreen(hwnd, &p);

			if (!cpData->_pColourPopup)
			{
				cpData->_pColourPopup = new ColourPopup(cpData->_currentColour);
				cpData->_pColourPopup->init(cpData->_hInst, hwnd);
				cpData->_pColourPopup->doDialog(p);
			}
			else
			{
				cpData->_pColourPopup->setColour(cpData->_currentColour);
				cpData->_pColourPopup->doDialog(p);
				cpData->_pColourPopup->display(true);
			}
			return 0;
		}

		case WM_RBUTTONDOWN:
		{
			if (cpData->_disableRightClick)
				break;

			cpData->_isEnabled = !cpData->_isEnabled;
			::SendMessage(cpData->_hParent, WM_COMMAND, MAKEWPARAM(0, CPN_COLOURPICKED), reinterpret_cast<LPARAM>(hwnd));
			cpData->redraw();
			return 0;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			if (cpData->_pColourPopup)
			{
				::SendMessage(cpData->_pColourPopup->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			}
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			auto* hdc = reinterpret_cast<HDC>(wParam);
			cpData->drawBackground(hdc);
			return TRUE;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC hdc = ::BeginPaint(hwnd, &ps);
			cpData->drawForeground(hdc);
			::EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_PICKUP_COLOR:
		{
			if (const auto clr = static_cast<COLORREF>(wParam); cpData->_currentColour != clr)
			{
				cpData->_currentColour = clr;
				::SendMessage(cpData->_hParent, WM_COMMAND, MAKEWPARAM(0, CPN_COLOURPICKED), reinterpret_cast<LPARAM>(hwnd));
				cpData->redraw();
			}
			return TRUE;
		}

		case WM_ENABLE:
		{
			if (static_cast<BOOL>(wParam) == FALSE)
			{
				cpData->_currentColour = NppDarkMode::isEnabled() ? NppDarkMode::getDlgBackgroundColor() : ::GetSysColor(COLOR_3DFACE);
				cpData->redraw();
			}
			return 0;
		}

		case WM_PICKUP_CANCEL:
		{
			cpData->_pColourPopup->display(false);
			return TRUE;
		}

		default:
			break;
	}
	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
}
