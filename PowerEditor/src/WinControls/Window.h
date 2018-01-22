// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#pragma once
#include <windows.h>

class Window
{
public:
	//! \name Constructors & Destructor
	//@{
	Window() = default;
	Window(const Window&) = delete;
	virtual ~Window() = default;
	//@}


	virtual void init(HINSTANCE hInst, HWND parent)
	{
		_hInst = hInst;
		_hParent = parent;
	}

	virtual void destroy() = 0;

	virtual void display(bool toShow = true) const
	{
		::ShowWindow(_hSelf, toShow ? SW_SHOW : SW_HIDE);
	}


	virtual void reSizeTo(RECT & rc) // should NEVER be const !!!
	{
		::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
		redraw();
	}


	virtual void reSizeToWH(RECT& rc) // should NEVER be const !!!
	{
		::MoveWindow(_hSelf, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		redraw();
	}


	virtual void redraw(bool forceUpdate = false) const
	{
		::InvalidateRect(_hSelf, nullptr, TRUE);
		if (forceUpdate)
			::UpdateWindow(_hSelf);
	}


    virtual void getClientRect(RECT & rc) const
	{
		::GetClientRect(_hSelf, &rc);
	}

	virtual void getWindowRect(RECT & rc) const
	{
		::GetWindowRect(_hSelf, &rc);
	}

	virtual int getWidth() const
	{
		RECT rc;
		::GetClientRect(_hSelf, &rc);
		return (rc.right - rc.left);
	}

	virtual int getHeight() const
	{
		RECT rc;
		::GetClientRect(_hSelf, &rc);
		if (::IsWindowVisible(_hSelf) == TRUE)
			return (rc.bottom - rc.top);
		return 0;
	}

	virtual bool isVisible() const
	{
    	return (::IsWindowVisible(_hSelf)?true:false);
	}

	HWND getHSelf() const
	{
		return _hSelf;
	}

	HWND getHParent() const {
		return _hParent;
	}

	void getFocus() const {
		::SetFocus(_hSelf);
	}

    HINSTANCE getHinst() const
	{
		//assert(_hInst != 0);
		return _hInst;
	}


	Window& operator = (const Window&) = delete;


protected:
	HINSTANCE _hInst = NULL;
	HWND _hParent = NULL;
	HWND _hSelf = NULL;
};