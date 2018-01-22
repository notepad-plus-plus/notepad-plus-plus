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

#include "Window.h"
#include "Common.h"

#include <Commctrl.h>

struct columnInfo {
	size_t _width;
	generic_string _label;

	columnInfo(const generic_string & label, size_t width) : _width(width), _label(label) {};
};

class ListView : public Window
{
public:
	ListView() : Window() {};
	virtual ~ListView() {};

	// addColumn() should be called before init()
	void addColumn(const columnInfo & column2Add) {
		_columnInfos.push_back(column2Add);
	};

	// setStyleOption() should be called before init()
	void setStyleOption(int32_t extraStyle) {
		_extraStyle = extraStyle;
	};

	void addLine(const std::vector<generic_string> & values2Add, LPARAM lParam = 0, int pos2insert = -1);
	
	size_t nbItem() const {
		return ListView_GetItemCount(_hSelf);
	};

	long getSelectedIndex() const {
		return ListView_GetSelectionMark(_hSelf);
	};

	void setSelection(int itemIndex) const {
		ListView_SetItemState(_hSelf, itemIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(_hSelf, itemIndex, false);
		ListView_SetSelectionMark(_hSelf, itemIndex);
	};

	LPARAM getLParamFromIndex(int itemIndex) const;

	std::vector<size_t> getCheckedIndexes() const;

	virtual void init(HINSTANCE hInst, HWND hwnd);
	virtual void destroy();


protected:
	WNDPROC _defaultProc = nullptr;
	int32_t _extraStyle = 0;
	std::vector<columnInfo> _columnInfos;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((ListView *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};

