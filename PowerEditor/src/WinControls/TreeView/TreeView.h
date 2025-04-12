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
#include "Window.h"
#include "Common.h"
#include "NppDarkMode.h"

struct TreeStateNode {
	std::wstring _label;
	std::wstring _extraData;
	bool _isExpanded = false;
	bool _isSelected = false;
	std::vector<TreeStateNode> _children;
};


class TreeView : public Window {
public:
	TreeView() = default;
	virtual ~TreeView() = default;

	virtual void init(HINSTANCE hInst, HWND parent, int treeViewID);
	virtual void destroy();
	HTREEITEM addItem(const wchar_t *itemName, HTREEITEM hParentItem, int iImage, LPARAM lParam = 0);
	bool setItemParam(HTREEITEM Item2Set, LPARAM param);
	LPARAM getItemParam(HTREEITEM Item2Get) const;
	std::wstring getItemDisplayName(HTREEITEM Item2Set) const;
	HTREEITEM searchSubItemByName(const wchar_t *itemName, HTREEITEM hParentItem);
	void removeItem(HTREEITEM hTreeItem);
	void removeAllItems();
	bool renameItem(HTREEITEM Item2Set, const wchar_t *newName);
	void makeLabelEditable(bool toBeEnabled);

	HTREEITEM getChildFrom(HTREEITEM hTreeItem) const {
		return TreeView_GetChild(_hSelf, hTreeItem);
	};
	HTREEITEM getSelection() const {
		return TreeView_GetSelection(_hSelf);
	};
	bool selectItem(HTREEITEM hTreeItem2Select) const {
		return TreeView_SelectItem(_hSelf, hTreeItem2Select) == TRUE;
	};
	HTREEITEM getRoot() const {
		return TreeView_GetRoot(_hSelf);
	};
	HTREEITEM getParent(HTREEITEM hItem) const {
		return TreeView_GetParent(_hSelf, hItem);
	};
	HTREEITEM getNextSibling(HTREEITEM hItem) const {
		return TreeView_GetNextSibling(_hSelf, hItem);
	};
	HTREEITEM getPrevSibling(HTREEITEM hItem) const {
		return TreeView_GetPrevSibling(_hSelf, hItem);
	};
	
	void expand(HTREEITEM hItem) const {
		TreeView_Expand(_hSelf, hItem, TVE_EXPAND);
	};

	void fold(HTREEITEM hItem) const {
		TreeView_Expand(_hSelf, hItem, TVE_COLLAPSE);
	};

	void foldExpandRecursively(HTREEITEM hItem, bool isFold) const;
	void foldExpandAll(bool isFold) const;
	
	void foldAll() const {
		foldExpandAll(true);
	};

	void expandAll() const {
		foldExpandAll(false);
	};

	void toggleExpandCollapse(HTREEITEM hItem) const {
		TreeView_Expand(_hSelf, hItem, TVE_TOGGLE);
	};
	void setItemImage(HTREEITEM hTreeItem, int iImage, int iSelectedImage);

	// Drag and Drop operations
	void beginDrag(NMTREEVIEW* tv);
	void dragItem(HWND parentHandle, int x, int y);
	bool isDragging() const {
		return _isItemDragged;
	};
	bool dropItem();
	void addCanNotDropInList(int val2set) {
		_canNotDropInList.push_back(val2set);
	};

	void addCanNotDragOutList(int val2set) {
		_canNotDragOutList.push_back(val2set);
	};

	bool moveDown(HTREEITEM itemToMove);
	bool moveUp(HTREEITEM itemToMove);
	bool swapTreeViewItem(HTREEITEM itemGoDown, HTREEITEM itemGoUp);
	bool restoreFoldingStateFrom(const TreeStateNode & treeState2Compare, HTREEITEM treeviewNode);
	bool retrieveFoldingStateTo(TreeStateNode & treeState2Construct, HTREEITEM treeviewNode);
	bool searchLeafAndBuildTree(const TreeView & tree2Build, const std::wstring & text2Search, int index2Search);
	void sort(HTREEITEM hTreeItem, bool isRecusive);
	void customSorting(HTREEITEM hTreeItem, PFNTVCOMPARE sortingCallbackFunc, LPARAM lParam, bool isRecursive);
	bool setImageList(const std::vector<int>& imageIds, int imgSize = 0);
	std::vector<int> getImageIds(std::vector<int> stdIds, std::vector<int>darkIds, std::vector<int> lightIds);

protected:
	HIMAGELIST _hImaLst = nullptr;
	NppDarkMode::TreeViewStyle _tvStyleType = NppDarkMode::TreeViewStyle::classic;

	static LRESULT CALLBACK staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	void cleanSubEntries(HTREEITEM hTreeItem);
	void dupTree(HTREEITEM hTree2Dup, HTREEITEM hParentItem);
	bool searchLeafRecusivelyAndBuildTree(HTREEITEM tree2Build, const std::wstring & text2Search, int index2Search, HTREEITEM tree2Search);

	// Drag and Drop operations
	HTREEITEM _draggedItem = nullptr;
	HIMAGELIST _draggedImageList = nullptr;
	bool _isItemDragged = false;
	std::vector<int> _canNotDragOutList;
	std::vector<int> _canNotDropInList;
	bool canBeDropped(HTREEITEM draggedItem, HTREEITEM targetItem);
	void moveTreeViewItem(HTREEITEM draggedItem, HTREEITEM targetItem);
	bool isParent(HTREEITEM targetItem, HTREEITEM draggedItem);
	bool isDescendant(HTREEITEM targetItem, HTREEITEM draggedItem);
	bool canDragOut(HTREEITEM targetItem);
	bool canDropIn(HTREEITEM targetItem);
};
