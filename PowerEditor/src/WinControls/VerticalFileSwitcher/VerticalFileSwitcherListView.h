//this file is part of notepad++
//Copyright (C)2011 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef VERTICALFILESWITCHERLISTVIEW_H
#define VERTICALFILESWITCHERLISTVIEW_H

#include "window.h"
#include "TaskListDlg.h"

class VerticalFileSwitcherListView : public Window
{
public:
	VerticalFileSwitcherListView() : Window() {};

	virtual ~VerticalFileSwitcherListView() {};
	virtual void init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst);
	virtual void destroy();
	void initList();
	int getBufferInfoFromIndex(int index, int & view) const;
	void setBgColour(int i) {
		ListView_SetItemState(_hSelf, i, LVIS_SELECTED|LVIS_FOCUSED, 0xFF);
	}
	int newItem(int bufferID, int iView);
	int closeItem(int bufferID, int iView);
	void activateItem(int bufferID, int iView);
	void setItemIconStatus(int bufferID);
	generic_string getFullFilePath(size_t i) const {
		if (i < 0 || i > _taskListInfo._tlfsLst.size())
			return TEXT("");
		return _taskListInfo._tlfsLst[i]._fn;
	};

protected:
	TaskListInfo _taskListInfo;
	HIMAGELIST _hImaLst;
	WNDPROC _defaultProc;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((VerticalFileSwitcherListView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};

	int find(int bufferID, int iView) const;
	int add(int bufferID, int iView);
	void remove(int index);
};


#endif // VERTICALFILESWITCHERLISTVIEW_H
