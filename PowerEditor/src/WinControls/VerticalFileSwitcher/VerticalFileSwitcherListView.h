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
#include "TaskListDlg.h"

class Buffer;
typedef Buffer * BufferID;	//each buffer has unique ID by which it can be retrieved

#define SORT_DIRECTION_NONE     -1
#define SORT_DIRECTION_UP     0
#define SORT_DIRECTION_DOWN   1

#define FS_ROOTNODE					"DocList"
#define FS_CLMNNAME					"ColumnName"
#define FS_CLMNEXT					"ColumnExt"
#define FS_CLMNPATH					"ColumnPath"

struct SwitcherFileInfo {
	BufferID _bufID = 0;
	int _iView = 0;

	SwitcherFileInfo() = delete;
	SwitcherFileInfo(BufferID buf, int view) : _bufID(buf), _iView(view){};
};

class VerticalFileSwitcherListView : public Window
{
public:
	VerticalFileSwitcherListView() = default;
	virtual ~VerticalFileSwitcherListView() = default;

	virtual void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst);
	virtual void destroy();
	void initList();
	BufferID getBufferInfoFromIndex(int index, int & view) const;
	void setBgColour(int i) {
		ListView_SetItemState(_hSelf, i, LVIS_SELECTED|LVIS_FOCUSED, 0xFF);
	}
	int newItem(BufferID bufferID, int iView);
	int closeItem(BufferID bufferID, int iView);
	void activateItem(BufferID bufferID, int iView);
	void setItemIconStatus(BufferID bufferID);
	generic_string getFullFilePath(size_t i) const;
	
	void insertColumn(const TCHAR *name, int width, int index);
	void resizeColumns(int totalWidth);
	void deleteColumn(size_t i) {
		ListView_DeleteColumn(_hSelf, i);
	};
	int nbSelectedFiles() const {
		return static_cast<int32_t>(SendMessage(_hSelf, LVM_GETSELECTEDCOUNT, 0, 0));
	};

	std::vector<SwitcherFileInfo> getSelectedFiles(bool reverse = false) const;
	void reload();
	void ensureVisibleCurrentItem() const {
		ListView_EnsureVisible(_hSelf, _currentIndex, false);
	};

	void setBackgroundColor(COLORREF bgColour) {
		ListView_SetBkColor(_hSelf, bgColour);
		ListView_SetTextBkColor(_hSelf, bgColour);
		redraw(true);
    };

	void setForegroundColor(COLORREF fgColour) {
		ListView_SetTextColor(_hSelf, fgColour);
		redraw(true);
    };

protected:
	HIMAGELIST _hImaLst = nullptr;
	WNDPROC _defaultProc = nullptr;

	int _currentIndex = 0;

	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((VerticalFileSwitcherListView *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};

	int find(BufferID bufferID, int iView) const;
	int add(BufferID bufferID, int iView);
	void remove(int index);
	void removeAll();
	void selectCurrentItem() const {
		ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	};
};
