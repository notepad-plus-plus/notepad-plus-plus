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

#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include "window.h"

struct Heading {
    TCHAR _name[64]; 
    int _level;
	Heading(TCHAR *name, int level): _level(level){
		lstrcpy(_name, name);
	};
};

class TreeView : public Window
{
public:
	TreeView() : Window() {};

	virtual ~TreeView() {};
	virtual void init(HINSTANCE hInst, HWND parent, int treeViewID);
	virtual void destroy();
	HTREEITEM addItem(const TCHAR *itemName, int nLevel);
	HTREEITEM addItem(const TCHAR *itemName, HTREEITEM hParentItem);
	BOOL initTreeViewItems(std::vector<Heading> & headings, int idOpen, int idClosed, int idDocument);

protected:
	HIMAGELIST _hImaLst;
	WNDPROC _defaultProc;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TreeView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
	BOOL initImageList(int open_node_id, int closed_node_id, int leaf_id);

};


#endif // TREE_VIEW_H
