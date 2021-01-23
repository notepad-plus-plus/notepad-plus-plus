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

//#define IDC_STATUSBAR 789



enum
{
	defaultPartWidth = 5,
};




StatusBar::~StatusBar()
{
	delete[] _lpParts;
}


void StatusBar::init(HINSTANCE /*hInst*/, HWND /*hPere*/)
{
	assert(false and "should never be called");
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
