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


#include "StatusBar.h"

#include <windows.h>

#include <uxtheme.h>
#include <vsstyle.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "DoubleBuffer/DoubleBuffer.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "Window.h"
#include "dpiManagerV2.h"

#define IDC_STATUSBAR 789

static constexpr int defaultPartWidth = 5;

StatusBar::~StatusBar()
{
	closeTheme();
	destroyFont();
}

LRESULT CALLBACK StatusBar::StatusBarSubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto pStatusBar = reinterpret_cast<StatusBar*>(dwRefData);

	switch (uMsg)
	{
		case WM_ERASEBKGND:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;  // Let the control paint background the default way
			}

			RECT rc{};
			::GetClientRect(hWnd, &rc);
			::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getBackgroundBrush());
			return TRUE;
		}

		case WM_PAINT:
		case WM_PRINTCLIENT:
		{
			if (!NppDarkMode::isEnabled())
			{
				break;  // Let the control paint itself the default way
			}

			const bool isPaint = uMsg == WM_PAINT;

			PAINTSTRUCT ps{};
			HDC hdc = isPaint ? ::BeginPaint(hWnd, &ps) : reinterpret_cast<HDC>(wParam);

			struct {
				int horizontal = 0;
				int vertical = 0;
				int between = 0;
			} borders{};

			::SendMessage(hWnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(&borders));

			const auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			bool isSizeGrip = style & SBARS_SIZEGRIP;

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, NppDarkMode::getEdgePen()));

			auto holdFont = static_cast<HFONT>(::SelectObject(hdc, pStatusBar->_hFont));

			int nParts = static_cast<int>(SendMessage(hWnd, SB_GETPARTS, 0, 0));
			std::wstring str;
			for (int i = 0; i < nParts; ++i)
			{
				RECT rcPart{};
				::SendMessage(hWnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rcPart));
				if (!::RectVisible(hdc, &rcPart))
				{
					continue;
				}

				if (!pStatusBar->_partWidthArray.empty()) // to not apply on status bar in find dialog
				{
					POINT edges[] = {
						{rcPart.right - 2, rcPart.top + 1},
						{rcPart.right - 2, rcPart.bottom - 3}
					};
					Polyline(hdc, edges, _countof(edges));
				}

				RECT rcDivider = { rcPart.right - borders.vertical, rcPart.top, rcPart.right, rcPart.bottom };

				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, NppDarkMode::getTextColor());

				rcPart.left += borders.between;
				rcPart.right -= borders.vertical;

				const auto retVal = ::SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0);
				const WORD cchText = LOWORD(retVal);
				str.resize(size_t{ cchText } + 1); // technically the std::wstring might not have an internal null character at the end of the buffer, so add one
				const LRESULT lr = ::SendMessage(hWnd, SB_GETTEXT, i, reinterpret_cast<LPARAM>(str.data()));
				str.resize(cchText); // remove the extra NULL character

				if (const bool ownerDraw = (cchText == 0 && (HIWORD(retVal) & SBT_OWNERDRAW) != 0);
					ownerDraw)
				{
					const UINT id = ::GetDlgCtrlID(hWnd);
					DRAWITEMSTRUCT dis{
						.CtlType = 0,
						.CtlID = id,
						.itemID = static_cast<UINT>(i),
						.itemAction = 0,
						.itemState = 0,
						.hwndItem = hWnd,
						.hDC = hdc,
						.rcItem = rcPart,
						.itemData = static_cast<ULONG_PTR>(lr)
					};

					::SendMessage(::GetParent(hWnd), WM_DRAWITEM, id, reinterpret_cast<LPARAM>(&dis));
				}
				else
				{
					DrawText(hdc, str.c_str(), static_cast<int>(str.size()), &rcPart, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
				}

				if (!isSizeGrip && i < (nParts - 1))
				{
					FillRect(hdc, &rcDivider, NppDarkMode::getCtrlBackgroundBrush());
				}
			}

			if (isSizeGrip && pStatusBar->ensureTheme())
			{
				SIZE gripSize{};
				RECT rc{};
				::GetClientRect(hWnd, &rc);
				::GetThemePartSize(pStatusBar->_hTheme, hdc, SP_GRIPPER, 0, &rc, TS_DRAW, &gripSize);
				rc.left = rc.right - gripSize.cx;
				rc.top = rc.bottom - gripSize.cy;
				::DrawThemeBackground(pStatusBar->_hTheme, hdc, SP_GRIPPER, 0, &rc, nullptr);
			}

			::SelectObject(hdc, holdFont);
			::SelectObject(hdc, holdPen);

			if (isPaint)
			{
				::EndPaint(hWnd, &ps);
			}
			return 0;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, StatusBarSubclass, uIdSubclass);
			break;
		}

		case WM_THEMECHANGED:
		{
			pStatusBar->closeTheme();
			break;
		}

		case WM_DPICHANGED:
		{
			const UINT dpi = LOWORD(wParam);

			pStatusBar->closeTheme();
			pStatusBar->resetFont(dpi);

			if (pStatusBar->_partWidthArray.empty())
			{
				pStatusBar->setFontAndHeight();
			}

			break;
		}

		case WM_DPICHANGED_AFTERPARENT:
		{
			::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);
			break;
		}
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


void StatusBar::init(HINSTANCE hInst, HWND hPere, int nbParts)
{
	Window::init(hInst, hPere);
	InitCommonControls();

	// _hSelf = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS, NULL, _hParent, IDC_STATUSBAR);
	_hSelf = ::CreateWindowExW(
		0,
		STATUSCLASSNAME,
		L"",
		WS_CHILD | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		_hParent, reinterpret_cast<HMENU>(IDC_STATUSBAR), _hInst, 0);

	if (!_hSelf)
		throw std::runtime_error("StatusBar::init : CreateWindowEx() function return null");

	LOGFONT lf{};
	if (nbParts == 0)
	{
		lf = DPIManagerV2::getDefaultGUIFontForDpi(_hParent, NppParameters::getInstance().getDlgFontSize(), DPIManagerV2::FontType::status);
	}
	else
	{
		lf = DPIManagerV2::getDefaultGUIFontForDpi(_hParent, DPIManagerV2::FontType::status);
	}
	_hFont = ::CreateFontIndirectW(&lf);

	::SetWindowSubclass(_hSelf, StatusBarSubclass, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

	DoubleBuffer::subclass(_hSelf);

	_partWidthArray.clear();
	if (nbParts > 0)
	{
		_partWidthArray.resize(nbParts, defaultPartWidth);
	}
	else
	{
		setFontAndHeight();
	}

	// Allocate an array for holding the right edge coordinates.
	if (!_partWidthArray.empty())
		_parts.resize(_partWidthArray.size());

	RECT rc{};
	::GetClientRect(_hParent, &rc);
	adjustParts(rc.right);
}


bool StatusBar::setPartWidth(int whichPart, int width)
{
	if (static_cast<size_t>(whichPart) < _partWidthArray.size())
	{
		_partWidthArray[whichPart] = width;
		return true;
	}
	assert(false && "invalid status bar index");
	return false;
}


void StatusBar::destroy()
{
	::DestroyWindow(_hSelf);
}

int StatusBar::getHeight() const
{
	if (_partWidthArray.empty()) // It is Find dialog status bar
	{
		RECT rc{};
		::GetWindowRect(_hSelf, &rc);
		return (rc.bottom - rc.top);
	}
	return Window::getHeight();
}

void StatusBar::adjustParts(int clientWidth)
{
	// Calculate the right edge coordinate for each part, and
	// copy the coordinates to the array.
	int nWidth = std::max<int>(clientWidth - 20, 0);

	for (int i = static_cast<int>(_partWidthArray.size()) - 1; i >= 0; --i)
	{
		_parts[i] = nWidth;
		nWidth -= _partWidthArray[i];
	}

	// Tell the status bar to create the window parts.
	::SendMessage(_hSelf, SB_SETPARTS, _parts.size(), reinterpret_cast<LPARAM>(_parts.data()));
}


bool StatusBar::setText(const wchar_t* str, int whichPart)
{
	if (static_cast<size_t>(whichPart) < _partWidthArray.size())
	{
		if (str != nullptr)
			_lastSetText = str;
		else
			_lastSetText.clear();

		return (TRUE == ::SendMessage(_hSelf, SB_SETTEXT, whichPart, reinterpret_cast<LPARAM>(_lastSetText.c_str())));
	}
	assert(false && "invalid status bar index");
	return false;
}


bool StatusBar::setOwnerDrawText(const wchar_t* str)
{
	if (str != nullptr)
		_lastSetText = str;
	else
		_lastSetText.clear();

	return (::SendMessage(_hSelf, SB_SETTEXT, SBT_OWNERDRAW, reinterpret_cast<LPARAM>(_lastSetText.c_str())) == TRUE);
}

bool StatusBar::ensureTheme() noexcept
{
	if (_hTheme == nullptr)
	{
		_hTheme = ::OpenThemeData(_hSelf, VSCLASS_STATUS);
	}
	return _hTheme != nullptr;
}

void StatusBar::closeTheme() noexcept
{
	if (_hTheme != nullptr)
	{
		::CloseThemeData(_hTheme);
		_hTheme = nullptr;
	}
}

void StatusBar::resetFont(UINT dpi) noexcept
{
	destroyFont();

	LOGFONT lf{};
	if (_partWidthArray.empty())
	{
		lf = DPIManagerV2::getDefaultGUIFontForDpi(dpi, DPIManagerV2::FontType::status);
		lf.lfHeight = DPIManagerV2::scaleFont(NppParameters::getInstance().getDlgFontSize(), dpi);
	}
	else
	{
		lf = DPIManagerV2::getDefaultGUIFontForDpi(dpi, DPIManagerV2::FontType::status);
	}
	_hFont = ::CreateFontIndirectW(&lf);
}

void StatusBar::destroyFont() noexcept
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}
}

void StatusBar::setFontAndHeight() noexcept
{
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), MAKELPARAM(TRUE, 0));

	if (_partWidthArray.empty()) // it is owner draw status bar for Find dialog
	{
		const int height = DPIManagerV2::getFontAdjustedHeight(_hSelf, _hFont);
		::SendMessage(_hSelf, SB_SETMINHEIGHT, static_cast<WPARAM>(height), 0);
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	}
}
