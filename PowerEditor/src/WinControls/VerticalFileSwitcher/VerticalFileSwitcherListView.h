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

#include <windows.h>

#include <commctrl.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Buffer.h"
#include "NppDarkMode.h"
#include "TaskListDlg.h"
#include "Window.h"

#define FS_ROOTNODE					"DocList"
#define FS_CLMNNAME					"ColumnName"
#define FS_CLMNEXT					"ColumnExt"
#define FS_CLMNPATH					"ColumnPath"
#define FS_LVGROUPS					"ListGroups"


class VerticalFileSwitcherListView : public Window
{
public:
	VerticalFileSwitcherListView() = default;
	~VerticalFileSwitcherListView() override = default;

	virtual void init(HINSTANCE hInst, HWND hParent, UINT_PTR listViewID, HIMAGELIST hImaLst);
	void destroy() override;
	void initList();
	BufferID getBufferInfoFromIndex(int index, int & view) const;
	int newItem(BufferID bufferID, int iView);
	int closeItem(BufferID bufferID, int iView);
	void activateItem(BufferID bufferID, int iView);
	void setItemIconStatus(BufferID bufferID);
	std::wstring getFullFilePath(size_t i) const;
	void setItemColor(BufferID bufferID);
	
	void insertColumn(const wchar_t *name, int width, int index);
	void resizeColumns(int totalWidth);
	void deleteColumn(size_t i) {
		ListView_DeleteColumn(_hSelf, i);
	}
	int nbSelectedFiles() const {
		return static_cast<int>(::SendMessage(_hSelf, LVM_GETSELECTEDCOUNT, 0, 0));
	}

	std::vector<BufferViewInfo> getSelectedFiles(bool reverse = false) const;
	void reload();
	void redrawItems();
	void ensureVisibleCurrentItem() const {
		ListView_EnsureVisible(_hSelf, _currentIndex, false);
	}

	void setBackgroundColor(COLORREF bgColour) {
		ListView_SetBkColor(_hSelf, bgColour);
		ListView_SetTextBkColor(_hSelf, bgColour);
		redraw(true);
	}

	void setForegroundColor(COLORREF fgColour) {
		ListView_SetTextColor(_hSelf, fgColour);
		redraw(true);
	}

	void reSizeToWH(int width, int height);

	void setDarkTheme() {
		NppDarkMode::setDarkListView(_hSelf);
		NppDarkMode::setDarkTooltips(_hSelf, NppDarkMode::ToolTipsType::listview);
	}

protected:
	HIMAGELIST _hImaLst = nullptr;

	int _currentIndex = 0;

	using Window::init;
	using Window::reSizeToWH;

	int find(BufferID bufferID, int iView) const;
	int add(BufferID bufferID, int iView);
	void remove(int index, bool removeFromListview = true);
	void removeAll();
	void selectCurrentItem() const {
		ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
};
