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

#pragma once


#include "DockingFeature/Window.h"


class ColorCombo : public Window
{
public :
	ColorCombo() : Window(), _comboBoxInfo { 0 }, _color(0) {};

	~ColorCombo ()
	{
		destroy();
	};

	virtual void init(HINSTANCE hInst, HWND hParent, HWND hCombo);
	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
	};

	void onSelect()
	{
		drawColor();
	};

	void setColor(COLORREF color)
	{
		_color = color;
		::RedrawWindow(_comboBoxInfo.hwndCombo, &_comboBoxInfo.rcItem, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	};

	COLORREF getColor()
	{
		return _color;
	};

private:
	COMBOBOXINFO	_comboBoxInfo;
	WNDPROC			_hDefaultComboProc;
	COLORREF		_color;

	void drawColor();
	void pickColor();

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK wndDefaultProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return ((ColorCombo*)(::GetWindowLongPtrW(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam);
	};
};
