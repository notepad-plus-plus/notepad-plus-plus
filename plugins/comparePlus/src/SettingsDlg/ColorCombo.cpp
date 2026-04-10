/*
 * This file is part of ComparePlus Plugin for Notepad++
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma comment (lib, "comdlg32")


#include <windows.h>
#include <commdlg.h>

#include "ColorCombo.h"
#include "UserSettings.h"


void ColorCombo::init(HINSTANCE hInst, HWND hParent, HWND hCombo)
{
	Window::init(hInst, hParent);

	// Subclass combo to get edit messages
	_comboBoxInfo.cbSize = sizeof(_comboBoxInfo);

	::SendMessageW(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&_comboBoxInfo);
	::SetWindowLongPtrW(_comboBoxInfo.hwndCombo, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	_hDefaultComboProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(_comboBoxInfo.hwndCombo, GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(wndDefaultProc)));
}


LRESULT ColorCombo::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_KEYDOWN:
			if (wParam != VK_SPACE && wParam != VK_DOWN && wParam != VK_RIGHT)
				return FALSE;
		// Intentional fall-through
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			pickColor();
		return TRUE;

		case WM_PAINT:
		{
			LRESULT lpRet = ::CallWindowProcW(_hDefaultComboProc, hwnd, Message, wParam, lParam);
			drawColor();

			return lpRet;
		}
	}

	return ::CallWindowProcW(_hDefaultComboProc, hwnd, Message, wParam, lParam);
}


void ColorCombo::drawColor()
{
	HDC		hDc		= ::GetDC(_comboBoxInfo.hwndCombo);
	HBRUSH	hBrush	= ::CreateSolidBrush(_color);

	::FillRect(hDc, &_comboBoxInfo.rcItem, hBrush);

	if (_comboBoxInfo.hwndCombo == ::GetFocus())
	{
		RECT rc	= _comboBoxInfo.rcItem;
		::InflateRect(&rc, -1, -1);
		::DrawFocusRect(hDc, &rc);
	}

	::DeleteObject(hBrush);
	::ReleaseDC(_comboBoxInfo.hwndCombo, hDc);
}


void ColorCombo::pickColor()
{
	static COLORREF custColors[16] = {
		DEFAULT_ADDED_COLOR,		DEFAULT_REMOVED_COLOR,		DEFAULT_MOVED_COLOR,
		DEFAULT_CHANGED_COLOR,		DEFAULT_PART_COLOR,			DEFAULT_MOVED_PART_COLOR,
		RGB(0x00, 0xBB, 0x00),		RGB(0xAA, 0x00, 0x00),
		DEFAULT_ADDED_COLOR_DARK,	DEFAULT_REMOVED_COLOR_DARK,	DEFAULT_MOVED_COLOR_DARK,
		DEFAULT_CHANGED_COLOR_DARK,	DEFAULT_PART_COLOR_DARK,	DEFAULT_MOVED_PART_COLOR_DARK,
		RGB(0x00, 0xBB, 0x00),		RGB(0xAA, 0x00, 0x00)
	};

	CHOOSECOLORW cc { 0 };
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = _hParent;

	cc.lpCustColors = (LPDWORD)custColors;
	cc.rgbResult = _color;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (::ChooseColorW(&cc) == TRUE)
		setColor(cc.rgbResult);
}
