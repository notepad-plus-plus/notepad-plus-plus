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


#ifndef VERTICALFILESWITCHERLISTVIEW_H
#define VERTICALFILESWITCHERLISTVIEW_H

#include "window.h"
#include "TaskListDlg.h"

#define SORT_DIRECTION_UP     0
#define SORT_DIRECTION_DOWN   1

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
	generic_string getFullFilePath(size_t i) const;
	
	void insertColumn(TCHAR *name, int width, int index);


protected:
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
