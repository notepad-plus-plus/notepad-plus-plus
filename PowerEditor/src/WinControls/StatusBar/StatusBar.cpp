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
#include <windows.h>
#include <commctrl.h>
#include "StatusBar.h"
#include <algorithm>
#include <cassert>
#include "Parameters.h"
#include "NppDarkMode.h"
#include <Uxtheme.h>
#include <Vssym32.h>

//#define IDC_STATUSBAR 789


enum
{
	defaultPartWidth = 5,
};


StatusBar::~StatusBar()
{
	delete[] _lpParts;
}


void StatusBar::init(HINSTANCE, HWND)
{
	assert(false and "should never be called");
}


struct StatusBarSubclassInfo
{
	HTHEME hTheme = nullptr;

	~StatusBarSubclassInfo()
	{
		closeTheme();
	}

	bool ensureTheme(HWND hwnd)
	{
		if (!hTheme)
		{
			hTheme = OpenThemeData(hwnd, L"Status");
		}
		return hTheme != nullptr;
	}

	void closeTheme()
	{
		if (hTheme)
		{
			CloseThemeData(hTheme);
			hTheme = nullptr;
		}
	}
};


constexpr UINT_PTR g_statusBarSubclassID = 42;


LRESULT CALLBACK StatusBarSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	StatusBarSubclassInfo* pStatusBarInfo = reinterpret_cast<StatusBarSubclassInfo*>(dwRefData);

	switch (uMsg)
	{
		case WM_ERASEBKGND:
		{
			if (!NppDarkMode::isEnabled())
			{
				return DefSubclassProc(hWnd, uMsg, wParam, lParam);
			}

			RECT rc;
			GetClientRect(hWnd, &rc);
			FillRect((HDC)wParam, &rc, NppDarkMode::getBackgroundBrush());
			return TRUE;
		}

		case WM_PAINT:
		{
			if (!NppDarkMode::isEnabled())
			{
				return DefSubclassProc(hWnd, uMsg, wParam, lParam);
			}

			struct {
				int horizontal;
				int vertical;
				int between;
			} borders = {};

			SendMessage(hWnd, SB_GETBORDERS, 0, (LPARAM)&borders);

			DWORD style = GetWindowLong(hWnd, GWL_STYLE);
			bool isSizeGrip = style & SBARS_SIZEGRIP;

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			HFONT holdFont = (HFONT)::SelectObject(hdc, NppParameters::getInstance().getDefaultUIFont());

			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			FillRect(hdc, &ps.rcPaint, NppDarkMode::getBackgroundBrush());

			int nParts = static_cast<int>(SendMessage(hWnd, SB_GETPARTS, 0, 0));
			std::wstring str;
			for (int i = 0; i < nParts; ++i)
			{
				RECT rcPart = {};
				SendMessage(hWnd, SB_GETRECT, i, (LPARAM)&rcPart);
				RECT rcIntersect = {};
				if (!IntersectRect(&rcIntersect, &rcPart, &ps.rcPaint))
				{
					continue;
				}

				if (nParts > 2) //to not apply on status bar in find dialog
				{
					POINT edges[] = {
						{rcPart.right - 2, rcPart.top + 1},
						{rcPart.right - 2, rcPart.bottom - 3}
					};
					Polyline(hdc, edges, _countof(edges));
				}

				RECT rcDivider = { rcPart.right - borders.vertical, rcPart.top, rcPart.right, rcPart.bottom };

				DWORD cchText = 0;
				cchText = LOWORD(SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0));
				str.resize(cchText + 1); // technically the std::wstring might not have an internal null character at the end of the buffer, so add one
				LRESULT lr = SendMessage(hWnd, SB_GETTEXT, i, (LPARAM)&str[0]);
				str.resize(cchText); // remove the extra NULL character
				bool ownerDraw = false;
				if (cchText == 0 && (lr & ~(SBT_NOBORDERS | SBT_POPOUT | SBT_RTLREADING)) != 0)
				{
					// this is a pointer to the text
					ownerDraw = true;
				}
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, NppDarkMode::getTextColor());

				rcPart.left += borders.between;
				rcPart.right -= borders.vertical;

				if (ownerDraw)
				{
					UINT id = GetDlgCtrlID(hWnd);
					DRAWITEMSTRUCT dis = {
						0
						, 0
						, static_cast<UINT>(i)
						, ODA_DRAWENTIRE
						, id
						, hWnd
						, hdc
						, rcPart
						, static_cast<ULONG_PTR>(lr)
					};

					SendMessage(GetParent(hWnd), WM_DRAWITEM, id, (LPARAM)&dis);
				}
				else
				{
					DrawText(hdc, str.data(), static_cast<int>(str.size()), &rcPart, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
				}

				if (!isSizeGrip && i < (nParts - 1))
				{
					FillRect(hdc, &rcDivider, NppDarkMode::getSofterBackgroundBrush());
				}
			}

			if (isSizeGrip)
			{
				pStatusBarInfo->ensureTheme(hWnd);
				SIZE gripSize = {};
				GetThemePartSize(pStatusBarInfo->hTheme, hdc, SP_GRIPPER, 0, &rcClient, TS_DRAW, &gripSize);
				RECT rc = rcClient;
				rc.left = rc.right - gripSize.cx;
				rc.top = rc.bottom - gripSize.cy;
				DrawThemeBackground(pStatusBarInfo->hTheme, hdc, SP_GRIPPER, 0, &rc, nullptr);
			}

			::SelectObject(hdc, holdFont);
			::SelectObject(hdc, holdPen);

			EndPaint(hWnd, &ps);
			return FALSE;
		}

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, StatusBarSubclass, g_statusBarSubclassID);
			break;

		case WM_THEMECHANGED:
			pStatusBarInfo->closeTheme();
			break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


void StatusBar::init(HINSTANCE hInst, HWND hPere, int nbParts)
{
	Window::init(hInst, hPere);
	InitCommonControls();

	// _hSelf = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS, NULL, _hParent, IDC_STATUSBAR);
	_hSelf = ::CreateWindowEx(
		0,
		STATUSCLASSNAME,
		TEXT(""),
		WS_CHILD | SBARS_SIZEGRIP ,
		0, 0, 0, 0,
		_hParent, nullptr, _hInst, 0);

	if (!_hSelf)
		throw std::runtime_error("StatusBar::init : CreateWindowEx() function return null");

	StatusBarSubclassInfo* pStatusBarInfo = new StatusBarSubclassInfo();
	_pStatusBarInfo = pStatusBarInfo;

	SetWindowSubclass(_hSelf, StatusBarSubclass, g_statusBarSubclassID, reinterpret_cast<DWORD_PTR>(pStatusBarInfo));

	_partWidthArray.clear();
	if (nbParts > 0)
		_partWidthArray.resize(nbParts, defaultPartWidth);

	// Allocate an array for holding the right edge coordinates.
	if (_partWidthArray.size())
		_lpParts = new int[_partWidthArray.size()];

	RECT rc;
	::GetClientRect(_hParent, &rc);
	adjustParts(rc.right);
}


bool StatusBar::setPartWidth(int whichPart, int width)
{
	if ((size_t) whichPart < _partWidthArray.size())
	{
		_partWidthArray[whichPart] = width;
		return true;
	}
	assert(false and "invalid status bar index");
	return false;
}


void StatusBar::destroy()
{
	::DestroyWindow(_hSelf);
	delete _pStatusBarInfo;
}


void StatusBar::reSizeTo(const RECT& rc)
{
	::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
	adjustParts(rc.right);
	redraw();
}


int StatusBar::getHeight() const
{
	return (FALSE != ::IsWindowVisible(_hSelf)) ? Window::getHeight() : 0;
}


void StatusBar::adjustParts(int clientWidth)
{
	// Calculate the right edge coordinate for each part, and
	// copy the coordinates to the array.
	int nWidth = std::max<int>(clientWidth - 20, 0);

	for (int i = static_cast<int>(_partWidthArray.size()) - 1; i >= 0; i--)
	{
		_lpParts[i] = nWidth;
		nWidth -= _partWidthArray[i];
	}

	// Tell the status bar to create the window parts.
	::SendMessage(_hSelf, SB_SETPARTS, _partWidthArray.size(), reinterpret_cast<LPARAM>(_lpParts));
}


bool StatusBar::setText(const TCHAR* str, int whichPart)
{
	if ((size_t) whichPart < _partWidthArray.size())
	{
		if (str != nullptr)
			_lastSetText = str;
		else
			_lastSetText.clear();

		return (TRUE == ::SendMessage(_hSelf, SB_SETTEXT, whichPart, reinterpret_cast<LPARAM>(_lastSetText.c_str())));
	}
	assert(false and "invalid status bar index");
	return false;
}


bool StatusBar::setOwnerDrawText(const TCHAR* str)
{
	if (str != nullptr)
		_lastSetText = str;
	else
		_lastSetText.clear();

	return (::SendMessage(_hSelf, SB_SETTEXT, SBT_OWNERDRAW, reinterpret_cast<LPARAM>(_lastSetText.c_str())) == TRUE);
}
