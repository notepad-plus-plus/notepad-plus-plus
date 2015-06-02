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



#include "TreeView.h"

#define CY_ITEMHEIGHT     18

void TreeView::init(HINSTANCE hInst, HWND parent, int treeViewID)
{
	Window::init(hInst, parent);
	_hSelf = ::GetDlgItem(parent, treeViewID);

	_hSelf = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Tree View"),
                            WS_CHILD | WS_BORDER | WS_HSCROLL | WS_TABSTOP | TVS_LINESATROOT | TVS_HASLINES |
							TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS | TVS_INFOTIP, 
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
{
	return ::CallWindowProc(_defaultProc, hwnd, Message, wParam, lParam);
}


bool TreeView::setItemParam(HTREEITEM Item2Set, const TCHAR *paramStr)
{
	if (!Item2Set)
		return false;

	TVITEM tvItem;
	tvItem.hItem = Item2Set;
	tvItem.mask = TVIF_PARAM;

	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
	
	if (!tvItem.lParam) 
		tvItem.lParam = (LPARAM)(new generic_string(paramStr));
	else
	{
		*((generic_string *)tvItem.lParam) = paramStr;
	}
	SendMessage(_hSelf, TVM_SETITEM, 0,(LPARAM)&tvItem);
	return true;
}

HTREEITEM TreeView::addItem(const TCHAR *itemName, HTREEITEM hParentItem, int iImage, const TCHAR *filePath)
{
	TVITEM tvi;
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 

	// Set the item label.
	tvi.pszText = (LPTSTR)itemName; 
	tvi.cchTextMax = MAX_PATH; 

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


void TreeView::dupTree(HTREEITEM hTree2Dup, HTREEITEM hParentItem)
{
	for (HTREEITEM hItem = getChildFrom(hTree2Dup); hItem != NULL; hItem = getNextSibling(hItem))
	{
		TCHAR textBuffer[MAX_PATH];
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
		if (tvItem.lParam)
		{
			tvItem.lParam = (LPARAM)(new generic_string(*((generic_string *)(tvItem.lParam))));
		}

		TVINSERTSTRUCT tvInsertStruct;
		tvInsertStruct.item = tvItem;
		tvInsertStruct.hInsertAfter = (HTREEITEM)TVI_LAST;
		tvInsertStruct.hParent = hParentItem;
		HTREEITEM hTreeParent = (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertStruct);
		dupTree(hItem, hTreeParent);
	}
}

HTREEITEM TreeView::searchSubItemByName(const TCHAR *itemName, HTREEITEM hParentItem)
{
	HTREEITEM hItem = NULL;
	if (hParentItem != NULL)
		hItem = getChildFrom(hParentItem);
	else
		hItem = getRoot();
	
	for ( ; hItem != NULL; hItem = getNextSibling(hItem))
	{
		TCHAR textBuffer[MAX_PATH];
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.mask = TVIF_TEXT;
		SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
		
		if (lstrcmp(itemName, tvItem.pszText) == 0)
		{
			return hItem;
		}
	}
	return NULL;
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

// pass LPARAM of WM_NOTIFY here after casted to NMTREEVIEW*
void TreeView::beginDrag(NMTREEVIEW* tv)
{
	if (!canDragOut(tv->itemNew.hItem))
		return;

    // create dragging image for you using TVM_CREATEDRAGIMAGE
    // You have to delete it after drop operation, so remember it.
    _draggedItem = tv->itemNew.hItem;
    _draggedImageList = (HIMAGELIST)::SendMessage(_hSelf, TVM_CREATEDRAGIMAGE, (WPARAM)0, (LPARAM)_draggedItem);

    // start dragging operation
    // PARAMS: HIMAGELIST, imageIndex, xHotspot, yHotspot
    ::ImageList_BeginDrag(_draggedImageList, 0, 0, 0);
    ::ImageList_DragEnter(_hSelf, tv->ptDrag.x, tv->ptDrag.y);

    // redirect mouse input to the parent window
    ::SetCapture(::GetParent(_hSelf));
    ::ShowCursor(false);          // hide the cursor

    _isItemDragged = true;
}

void TreeView::dragItem(HWND parentHandle, int x, int y)
{
    // convert the dialog coords to control coords
    POINT point;
    point.x = (SHORT)x;
    point.y = (SHORT)y;
    ::ClientToScreen(parentHandle, &point);
    ::ScreenToClient(_hSelf, &point);

    // drag the item to the current the cursor position
    ::ImageList_DragMove(point.x, point.y);

    // hide the dragged image, so the background can be refreshed
    ::ImageList_DragShowNolock(false);

    // find out if the pointer is on an item
    // If so, highlight the item as a drop target.
    TVHITTESTINFO hitTestInfo;
    hitTestInfo.pt.x = point.x;
    hitTestInfo.pt.y = point.y;
    HTREEITEM targetItem = (HTREEITEM)::SendMessage(_hSelf, TVM_HITTEST, (WPARAM)0, (LPARAM)&hitTestInfo);
    if(targetItem)
    {
		::SendMessage(_hSelf, TVM_SELECTITEM, (WPARAM)(TVGN_DROPHILITE), (LPARAM)targetItem);
    }

    // show the dragged image
    ::ImageList_DragShowNolock(true);
}

bool TreeView::dropItem()
{
	bool isFilesMoved = false;
    // get the target item
    HTREEITEM targetItem = (HTREEITEM)::SendMessage(_hSelf, TVM_GETNEXTITEM, (WPARAM)TVGN_DROPHILITE, (LPARAM)0);

    // make a copy of the dragged item and insert the clone under
    // the target item, then, delete the original dragged item
    // Note that the dragged item may have children. In this case,
    // you have to move (copy and delete) for every child items, too.
	if (canBeDropped(_draggedItem, targetItem))
	{
		moveTreeViewItem(_draggedItem, targetItem);
		isFilesMoved = true;
	}
    // finish drag-and-drop operation
    ::ImageList_EndDrag();
    ::ImageList_Destroy(_draggedImageList);
    ::ReleaseCapture();
    ::ShowCursor(true);
	    
	SendMessage(_hSelf,TVM_SELECTITEM,TVGN_CARET,(LPARAM)targetItem);
    SendMessage(_hSelf,TVM_SELECTITEM,TVGN_DROPHILITE,0);

    // clear global variables
    _draggedItem = 0;
    _draggedImageList = 0;
    _isItemDragged = false;
	return isFilesMoved;
}

bool TreeView::canBeDropped(HTREEITEM draggedItem, HTREEITEM targetItem)
{
	if (targetItem == NULL)
		return false;
	if (draggedItem == targetItem)
		return false;
	if (targetItem == TreeView_GetRoot(_hSelf))
		return false;
	if (isDescendant(targetItem, draggedItem))
		return false;
	if (isParent(targetItem, draggedItem))
		return false;
	// candragItem, canBeDropInItems
	if (!canDropIn(targetItem))
		return false;

	return true;
}

bool TreeView::isDescendant(HTREEITEM targetItem, HTREEITEM draggedItem)
{
	if (targetItem == NULL)
		return false;

	if (TreeView_GetRoot(_hSelf) == targetItem)
		return false;

	HTREEITEM parent = getParent(targetItem);
	if (parent == draggedItem)
		return true;
	
	return isDescendant(parent, draggedItem);
}

bool TreeView::isParent(HTREEITEM targetItem, HTREEITEM draggedItem)
{
	HTREEITEM parent = getParent(draggedItem);
	if (parent == targetItem)
		return true;
	return false;
}

void TreeView::moveTreeViewItem(HTREEITEM draggedItem, HTREEITEM targetItem)
{
	TCHAR textBuffer[MAX_PATH];
	TVITEM tvDraggingItem;
	tvDraggingItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvDraggingItem.pszText = textBuffer;
	tvDraggingItem.cchTextMax = MAX_PATH;
	tvDraggingItem.hItem = draggedItem;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvDraggingItem);

	if (tvDraggingItem.lParam)
		tvDraggingItem.lParam = (LPARAM)(new generic_string(*((generic_string *)(tvDraggingItem.lParam))));

    TVINSERTSTRUCT tvInsertStruct;
	tvInsertStruct.item = tvDraggingItem;
	tvInsertStruct.hInsertAfter = (HTREEITEM)TVI_LAST;
	tvInsertStruct.hParent = targetItem;

	HTREEITEM hTreeParent = (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertStruct);
	dupTree(draggedItem, hTreeParent);
	removeItem(draggedItem);
}

bool TreeView::moveDown(HTREEITEM itemToMove)
{
	HTREEITEM hItemToUp = getNextSibling(itemToMove);
	if (!hItemToUp)
		return false;
	return swapTreeViewItem(itemToMove, hItemToUp);
}

bool TreeView::moveUp(HTREEITEM itemToMove)
{
	HTREEITEM hItemToDown = getPrevSibling(itemToMove);
	if (!hItemToDown)
		return false;
	return swapTreeViewItem(hItemToDown, itemToMove);
}

bool TreeView::swapTreeViewItem(HTREEITEM itemGoDown, HTREEITEM itemGoUp)
{
	HTREEITEM selectedItem = getSelection();
	int itemSelected = selectedItem == itemGoDown?1:(selectedItem == itemGoUp?2:0);

	// get previous and next for both items with () function
	HTREEITEM itemTop = getPrevSibling(itemGoDown);
	itemTop = itemTop?itemTop:(HTREEITEM)TVI_FIRST;
	HTREEITEM parentGoDown = getParent(itemGoDown);
	HTREEITEM parentGoUp = getParent(itemGoUp);

	if (parentGoUp != parentGoDown)
		return false;

	// get both item infos
	TCHAR textBufferUp[MAX_PATH];
	TCHAR textBufferDown[MAX_PATH];
	TVITEM tvUpItem;
	TVITEM tvDownItem;
	tvUpItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvDownItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvUpItem.pszText = textBufferUp;
	tvDownItem.pszText = textBufferDown;
	tvUpItem.cchTextMax = MAX_PATH;
	tvDownItem.cchTextMax = MAX_PATH;
	tvUpItem.hItem = itemGoUp;
	tvDownItem.hItem = itemGoDown;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvUpItem);
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvDownItem);

	// make copy recursively for both items

	if (tvUpItem.lParam)
		tvUpItem.lParam = (LPARAM)(new generic_string(*((generic_string *)(tvUpItem.lParam))));
	if (tvDownItem.lParam)
		tvDownItem.lParam = (LPARAM)(new generic_string(*((generic_string *)(tvDownItem.lParam))));

	// add 2 new items
    TVINSERTSTRUCT tvInsertUp;
	tvInsertUp.item = tvUpItem;
	tvInsertUp.hInsertAfter = itemTop;
	tvInsertUp.hParent = parentGoUp;
	HTREEITEM hTreeParent1stInserted = (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertUp);
	dupTree(itemGoUp, hTreeParent1stInserted);

	TVINSERTSTRUCT tvInsertDown;
	tvInsertDown.item = tvDownItem;
	tvInsertDown.hInsertAfter = hTreeParent1stInserted;
	tvInsertDown.hParent = parentGoDown;
	HTREEITEM hTreeParent2ndInserted = (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertDown);
	dupTree(itemGoDown, hTreeParent2ndInserted);

	// remove 2 old items
	removeItem(itemGoUp);
	removeItem(itemGoDown);

	// Restore the selection if needed
	if (itemSelected != 0)
	{
		if (itemSelected == 1)
		{
			selectItem(hTreeParent2ndInserted);
		}
		else if (itemSelected == 2)
		{
			selectItem(hTreeParent1stInserted);
		}
	}
	return true;
}


bool TreeView::canDropIn(HTREEITEM targetItem)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_IMAGE;
	tvItem.hItem = targetItem;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);

	for (size_t i = 0, len = _canNotDropInList.size(); i < len; ++i)
	{
		if (tvItem.iImage == _canNotDropInList[i])
			return false;
	}
	return true;
}


bool TreeView::canDragOut(HTREEITEM targetItem)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_IMAGE;
	tvItem.hItem = targetItem;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);

	for (size_t i = 0, len = _canNotDragOutList.size(); i < len; ++i)
	{
		if (tvItem.iImage == _canNotDragOutList[i])
			return false;
	}
	return true;
}



bool TreeView::searchLeafAndBuildTree(TreeView & tree2Build, const generic_string & text2Search, int index2Search)
{
	//tree2Build.removeAllItems();
	//HTREEITEM root = getRoot();

	return searchLeafRecusivelyAndBuildTree(tree2Build.getRoot(), text2Search, index2Search, getRoot());
}

bool TreeView::searchLeafRecusivelyAndBuildTree(HTREEITEM tree2Build, const generic_string & text2Search, int index2Search, HTREEITEM tree2Search)
{
	if (!tree2Search)
		return false;

	TCHAR textBuffer[MAX_PATH];
	TVITEM tvItem;
	tvItem.hItem = tree2Search;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
	
	if (tvItem.iImage == index2Search)
	{
		generic_string itemNameUpperCase = stringToUpper(tvItem.pszText);
		generic_string text2SearchUpperCase = stringToUpper(text2Search);

		size_t res = itemNameUpperCase.find(text2SearchUpperCase);
		if (res != generic_string::npos)
		{
			if (tvItem.lParam)
			{
				tvItem.lParam = (LPARAM)(new generic_string(*((generic_string *)(tvItem.lParam))));
			}
			TVINSERTSTRUCT tvInsertStruct;
			tvInsertStruct.item = tvItem;
			tvInsertStruct.hInsertAfter = (HTREEITEM)TVI_LAST;
			tvInsertStruct.hParent = tree2Build;
			::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvInsertStruct);
		}
	}

	size_t i = 0;
	bool isOk = true;
	for (HTREEITEM hItem = getChildFrom(tree2Search); hItem != NULL; hItem = getNextSibling(hItem))
	{
		isOk = searchLeafRecusivelyAndBuildTree(tree2Build, text2Search, index2Search, hItem);
		if (!isOk)
			break;
		++i;
	}
	return isOk;
}


bool TreeView::retrieveFoldingStateTo(TreeStateNode & treeState2Construct, HTREEITEM treeviewNode)
{
	if (!treeviewNode)
		return false;

	TCHAR textBuffer[MAX_PATH];
	TVITEM tvItem;
	tvItem.hItem = treeviewNode;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
	
	treeState2Construct._label = textBuffer;
	treeState2Construct._isExpanded = (tvItem.state & TVIS_EXPANDED) != 0;
	treeState2Construct._isSelected = (tvItem.state & TVIS_SELECTED) != 0;

	if (tvItem.lParam)
	{
		treeState2Construct._extraData = *((generic_string *)tvItem.lParam);
	}

	int i = 0;
	for (HTREEITEM hItem = getChildFrom(treeviewNode); hItem != NULL; hItem = getNextSibling(hItem))
	{
		treeState2Construct._children.push_back(TreeStateNode());
		retrieveFoldingStateTo(treeState2Construct._children.at(i), hItem);
		++i;
	}
	return true;
}

bool TreeView::restoreFoldingStateFrom(const TreeStateNode & treeState2Compare, HTREEITEM treeviewNode)
{
	if (!treeviewNode)
		return false;

	TCHAR textBuffer[MAX_PATH];
	TVITEM tvItem;
	tvItem.hItem = treeviewNode;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	SendMessage(_hSelf, TVM_GETITEM, 0,(LPARAM)&tvItem);
	
	if (treeState2Compare._label != textBuffer)
		return false;

	if (tvItem.lParam)
	{
		if (treeState2Compare._extraData != *((generic_string *)tvItem.lParam))
			return false;
	}

	if (treeState2Compare._isExpanded) //= (tvItem.state & TVIS_EXPANDED) != 0;
		expand(treeviewNode);
	else
		fold(treeviewNode);

	if (treeState2Compare._isSelected) //= (tvItem.state & TVIS_SELECTED) != 0;
		selectItem(treeviewNode);

	size_t i = 0;
	bool isOk = true;
	for (HTREEITEM hItem = getChildFrom(treeviewNode); hItem != NULL; hItem = getNextSibling(hItem))
	{
		if (i >= treeState2Compare._children.size())
			return false;
		isOk = restoreFoldingStateFrom(treeState2Compare._children.at(i), hItem);
		if (!isOk)
			break;
		++i;
	}
	return isOk;
}

void TreeView::sort(HTREEITEM hTreeItem)
{
	::SendMessage(_hSelf, TVM_SORTCHILDREN, TRUE, (LPARAM)hTreeItem);

	for (HTREEITEM hItem = getChildFrom(hTreeItem); hItem != NULL; hItem = getNextSibling(hItem))
		sort(hItem);
}
