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

#define INDEX_PROJECT_ROOT   0
#define INDEX_OPEN_NODE	     1
#define INDEX_CLOSED_NODE    2
#define INDEX_LEAF           3
#define INDEX_LEAF_INVALID   4

#include "window.h"

class TreeView : public Window
{
public:
	TreeView() : Window() {};

	virtual ~TreeView() {};
	virtual void init(HINSTANCE hInst, HWND parent, int treeViewID);
	virtual void destroy();
	HTREEITEM addItem(const TCHAR *itemName, HTREEITEM hParentItem, int iImage, const TCHAR *filePath = NULL);
	void removeItem(HTREEITEM hTreeItem);
	void cleanSubEntries(HTREEITEM hTreeItem);
	HTREEITEM getChildFrom(HTREEITEM hTreeItem) {
		return TreeView_GetChild(_hSelf, hTreeItem);
	};
	HTREEITEM getNextSibling(HTREEITEM hTreeItem){
		return TreeView_GetNextSibling(_hSelf ,hTreeItem);
	};
	HTREEITEM getSelection() {
		return TreeView_GetSelection(_hSelf);
	};
	void expandItemGUI(HTREEITEM hTreeItem);
	void collapsItemGUI(HTREEITEM hTreeItem);
	BOOL initImageList(int project_root_id, int open_node_id, int closed_node_id, int leaf_id, int ivalid_leaf_id);

protected:
	HIMAGELIST _hImaLst;
	WNDPROC _defaultProc;
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((TreeView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};


#endif // TREE_VIEW_H
