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

#include "precompiledHeaders.h"
#include "TreeView.h"

#define CY_ITEMHEIGHT     18

void TreeView::init(HINSTANCE hInst, HWND parent, int treeViewID)
{
	Window::init(hInst, parent);
	_hSelf = ::GetDlgItem(parent, treeViewID);

	_hSelf = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Tree View"),
                            TVS_HASBUTTONS | WS_CHILD | WS_BORDER | WS_HSCROLL |
							TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS |  TVS_INFOTIP | WS_TABSTOP, 
                            0,  0,  0, 0,
                            _hParent, 
                            NULL, 
                            _hInst, 
                            (LPVOID)0);

	TreeView_SetItemHeight(_hSelf, CY_ITEMHEIGHT);

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)this);
	_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, (LONG_PTR)staticProc));
}


void TreeView::destroy()
{
	HTREEITEM root = TreeView_GetRoot(_hSelf);
	cleanSubEntries(root);
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
} 

LRESULT TreeView::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{/*
	switch(Message)
	{
		case WM_MOUSEMOVE:
			//::MessageBoxA(NULL, "WM_MOUSEMOVE", "", MB_OK);
			break;
		case WM_LBUTTONUP:
			//::MessageBoxA(NULL, "WM_LBUTTONUP", "", MB_OK);
			//SendMessage to parent
			break;
		case WM_KEYDOWN:
			if (wParam == VK_F2)
				::MessageBoxA(NULL, "VK_F2", "", MB_OK);
			break;
		default:
			return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
	}
	*/
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}

HTREEITEM TreeView::addItem(const TCHAR *itemName, HTREEITEM hParentItem, int iImage, const TCHAR *filePath)
{
	TVITEM tvi;
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 

	// Set the item label.
	tvi.pszText = (LPTSTR)itemName; 
	tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]); 

	// Set icon
	tvi.iImage = iImage;//isNode?INDEX_CLOSED_NODE:INDEX_LEAF; 
	tvi.iSelectedImage = iImage;//isNode?INDEX_OPEN_NODE:INDEX_LEAF; 

	// Save the full path of file in the item's application-defined data area. 
	tvi.lParam = (filePath == NULL?0:(LPARAM)(new generic_string(filePath)));

	TVINSERTSTRUCT tvInsertStruct;
	tvInsertStruct.item = tvi; 
	tvInsertStruct.hInsertAfter = (HTREEITEM)TVI_LAST;
	tvInsertStruct.hParent = hParentItem;

	return (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertStruct);
}

void TreeView::removeItem(HTREEITEM hTreeItem)
{
	// Deallocate all the sub-entries recursively
	cleanSubEntries(hTreeItem);

	// Deallocate current entry
	TVITEM tvItem;
	tvItem.hItem = hTreeItem;
	tvItem.mask = TVIF_PARAM;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
	if (tvItem.lParam)
		delete (generic_string *)(tvItem.lParam);

	// Remove the node
	TreeView_DeleteItem(_hSelf, hTreeItem);
}

void TreeView::removeAllItems()
{
	for (HTREEITEM tvProj = getRoot();
        tvProj != NULL;
        tvProj = getNextSibling(tvProj))
	{
		cleanSubEntries(tvProj);
	}
	TreeView_DeleteAllItems(_hSelf);
}

void TreeView::cleanSubEntries(HTREEITEM hTreeItem)
{
	for (HTREEITEM hItem = getChildFrom(hTreeItem); hItem != NULL; hItem = getNextSibling(hItem))
	{
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_PARAM;
		SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
		if (tvItem.lParam)
		{
			delete (generic_string *)(tvItem.lParam);
		}
		cleanSubEntries(hItem);
	}
}

void TreeView::setItemImage(HTREEITEM hTreeItem, int iImage, int iSelectedImage)
	{
		TVITEM tvItem;
		tvItem.hItem = hTreeItem;
		tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvItem.iImage = iImage;
		tvItem.iSelectedImage = iSelectedImage;
		TreeView_SetItem(_hSelf, &tvItem);
}

