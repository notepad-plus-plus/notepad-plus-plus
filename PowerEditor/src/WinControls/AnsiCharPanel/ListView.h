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


#pragma once

#include "Window.h"
#include "Common.h"

#include <commctrl.h>

struct columnInfo {
	size_t _width;
	generic_string _label;

	columnInfo(const generic_string & label, size_t width) : _width(width), _label(label) {};
};

class ListView : public Window
{
public:
	ListView() = default;
	virtual ~ListView() = default;

	enum SortDirection {
		sortEncrease = 0,
		sortDecrease = 1
	};
	// addColumn() should be called before init()
	void addColumn(const columnInfo & column2Add) {
		_columnInfos.push_back(column2Add);
	};

	void setColumnText(size_t i, generic_string txt2Set) {
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = const_cast<TCHAR *>(txt2Set.c_str());
		ListView_SetColumn(_hSelf, i, &lvColumn);
	}

	// setStyleOption() should be called before init()
	void setStyleOption(int32_t extraStyle) {
		_extraStyle = extraStyle;
	};

	size_t findAlphabeticalOrderPos(const generic_string& string2search, SortDirection sortDir);

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

	bool removeFromIndex(size_t i)	{
		if (i >= nbItem())
			return false;

		return (ListView_DeleteItem(_hSelf, i) == TRUE);
	}

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

