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



#include "TreeView.h"
#include "Parameters.h"

using namespace std;

constexpr int g_treeviewItemPadding = 1;
constexpr int g_treeviewIcoSize = 16;

void TreeView::init(HINSTANCE hInst, HWND parent, int treeViewID)
{
	Window::init(hInst, parent);
	_hSelf = ::GetDlgItem(parent, treeViewID);

	const auto treeViewStyles = WS_HSCROLL | WS_TABSTOP | TVS_LINESATROOT\
						| TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS\
						| TVS_EDITLABELS | TVS_INFOTIP;

	_hSelf = CreateWindowEx(0,
							WC_TREEVIEW,
							L"Tree View",
							WS_CHILD | WS_BORDER | treeViewStyles,
							0,
							0,
							0,
							0,
							_hParent,
							nullptr,
							_hInst,
							nullptr);

	NppDarkMode::setTreeViewStyle(_hSelf, true);

	const int itemHeight = DPIManagerV2::scale(g_treeviewIcoSize + g_treeviewItemPadding * 2, _hParent);
	TreeView_SetItemHeight(_hSelf, itemHeight);

	constexpr UINT_PTR idSubclassTreeview = 1;
	::SetWindowSubclass(_hSelf, staticProc, idSubclassTreeview, reinterpret_cast<DWORD_PTR>(this));
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
	if (Message == TVM_EXPAND)
	{
		if (wParam & (TVE_COLLAPSE | TVE_EXPAND))
		{
			// If TVIS_EXPANDEDONCE flag is set, TVM_EXPAND messages do not generate
			// TVN_ITEMEXPANDING or TVN_ITEMEXPANDED notifications.
			// To reset the flag, you must send a TVM_EXPAND message
			// with the TVE_COLLAPSE and TVE_COLLAPSERESET flags set.
			// That in turn removes child items which is unwanted.
			// Below is a workaround for that.
			TVITEM tvItem{};
			tvItem.hItem = reinterpret_cast<HTREEITEM>(lParam);
			tvItem.mask = TVIF_STATE | TVIF_HANDLE | TVIF_PARAM;
			tvItem.stateMask = TVIS_EXPANDEDONCE;
			TreeView_GetItem(_hSelf, &tvItem);
			// Check if a flag is set.
			if (tvItem.state & TVIS_EXPANDEDONCE)
			{
				// If the flag is set, then manually notify parent that an item is collapsed/expanded
				// so that it can change icon, etc.
				NMTREEVIEW nmtv{};
				nmtv.hdr.code = TVN_ITEMEXPANDED;
				nmtv.hdr.hwndFrom = _hSelf;
				nmtv.hdr.idFrom = 0;
				nmtv.action = (wParam & TVE_COLLAPSE) ? TVE_COLLAPSE : TVE_EXPAND;
				nmtv.itemNew.hItem = tvItem.hItem;
				::SendMessage(_hParent, WM_NOTIFY, nmtv.hdr.idFrom, reinterpret_cast<LPARAM>(&nmtv));
			}
		}
	}
	return ::DefSubclassProc(hwnd, Message, wParam, lParam);
}

LRESULT TreeView::staticProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (Message)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hwnd, staticProc, uIdSubclass);
			break;
		}

		default:
			break;
	}
	return reinterpret_cast<TreeView*>(dwRefData)->runProc(hwnd, Message, wParam, lParam);
}

void TreeView::makeLabelEditable(bool toBeEnabled)
{
	DWORD dwNewStyle = (DWORD)GetWindowLongPtr(_hSelf, GWL_STYLE);
	if (toBeEnabled)
		dwNewStyle |= TVS_EDITLABELS;
	else
		dwNewStyle &= ~TVS_EDITLABELS;
	::SetWindowLongPtr(_hSelf, GWL_STYLE, dwNewStyle);
}


bool TreeView::setItemParam(HTREEITEM Item2Set, LPARAM param)
{
	if (!Item2Set)
		return false;

	TVITEM tvItem{};
	tvItem.hItem = Item2Set;
	tvItem.mask = TVIF_PARAM;
	tvItem.lParam = param;
	
	SendMessage(_hSelf, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	return true;
}

LPARAM TreeView::getItemParam(HTREEITEM Item2Get) const
{
	if (!Item2Get)
		return false;
	//wchar_t textBuffer[MAX_PATH];
	TVITEM tvItem{};
	tvItem.hItem = Item2Get;
	tvItem.mask = TVIF_PARAM;
	//tvItem.pszText = textBuffer;
	tvItem.lParam = 0;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	return tvItem.lParam;
}

wstring TreeView::getItemDisplayName(HTREEITEM Item2Set) const
{
	if (!Item2Set)
		return L"";
	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.hItem = Item2Set;
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	return tvItem.pszText;
}

bool TreeView::renameItem(HTREEITEM Item2Set, const wchar_t *newName)
{
	if (!Item2Set || !newName)
		return false;

	TVITEM tvItem{};
	tvItem.hItem = Item2Set;
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = (LPWSTR)newName;
	tvItem.cchTextMax = MAX_PATH;
	SendMessage(_hSelf, TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	return true;
}

HTREEITEM TreeView::addItem(const wchar_t *itemName, HTREEITEM hParentItem, int iImage, LPARAM lParam)
{
	TVITEM tvi{};
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

	// Set the item label.
	tvi.pszText = (LPTSTR)itemName;
	tvi.cchTextMax = MAX_PATH;

	// Set icon
	tvi.iImage = iImage;//isNode?INDEX_CLOSED_NODE:INDEX_LEAF;
	tvi.iSelectedImage = iImage;//isNode?INDEX_OPEN_NODE:INDEX_LEAF;

	tvi.lParam = lParam;

	TVINSERTSTRUCT tvInsertStruct{};
	tvInsertStruct.item = tvi;
	tvInsertStruct.hInsertAfter = TVI_LAST;
	tvInsertStruct.hParent = hParentItem;

	return reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)));
}

void TreeView::removeItem(HTREEITEM hTreeItem)
{
	// Deallocate all the sub-entries recursively
	cleanSubEntries(hTreeItem);

	// Deallocate current entry
	TVITEM tvItem{};
	tvItem.hItem = hTreeItem;
	tvItem.mask = TVIF_PARAM;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

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
		wchar_t textBuffer[MAX_PATH]{};
		TVITEM tvItem{};
		tvItem.hItem = hItem;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		TVINSERTSTRUCT tvInsertStruct{};
		tvInsertStruct.item = tvItem;
		tvInsertStruct.hInsertAfter = TVI_LAST;
		tvInsertStruct.hParent = hParentItem;
		HTREEITEM hTreeParent = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)));
		dupTree(hItem, hTreeParent);
	}
}

HTREEITEM TreeView::searchSubItemByName(const wchar_t *itemName, HTREEITEM hParentItem)
{
	HTREEITEM hItem = nullptr;
	if (hParentItem != nullptr)
		hItem = getChildFrom(hParentItem);
	else
		hItem = getRoot();

	while (hItem != nullptr)
	{
		wchar_t textBuffer[MAX_PATH] = { '\0' };
		TVITEM tvItem{};
		tvItem.hItem = hItem;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.mask = TVIF_TEXT;
		SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		if (lstrcmp(itemName, tvItem.pszText) == 0)
		{
			return hItem;
		}

		hItem = getNextSibling(hItem);
	}
	return nullptr;
}

bool TreeView::setImageList(const std::vector<int>& imageIds, int imgSize)
{
	const int nbImage = static_cast<int>(imageIds.size());
	if (imgSize <= 0)
		imgSize = g_treeviewIcoSize;

	// Creation of image list
	int dpiImgSize = DPIManagerV2::scale(imgSize, _hParent);

	NppParameters& nppParam = NppParameters::getInstance();
	const bool useStdIcons = nppParam.getNppGUI()._tbIconInfo._tbIconSet == TB_STANDARD;

	if (_hImaLst != nullptr)
	{
		int prevImageSize = 0;
		::ImageList_GetIconSize(_hImaLst, &prevImageSize, nullptr);
		if ((prevImageSize != dpiImgSize) || (!(useStdIcons && _tvStyleType == NppDarkMode::TreeViewStyle::classic) && _tvStyleType != NppDarkMode::getTreeViewStyle()))
		{
			::ImageList_Destroy(_hImaLst);
			_hImaLst = nullptr;
		}
		else
			return false;
	}

	if (_hImaLst == nullptr)
	{
		if ((_hImaLst = ::ImageList_Create(dpiImgSize, dpiImgSize, ILC_COLOR32 | ILC_MASK, nbImage, 0)) == nullptr)
			return false;

		// Add the ico into the list
		for (const int& id : imageIds)
		{
			HICON hIcon = nullptr;
			DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(id), dpiImgSize, dpiImgSize, &hIcon, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
			if (hIcon == nullptr)
				return false;

			::ImageList_AddIcon(_hImaLst, hIcon);
			::DestroyIcon(hIcon);
		}
	}

	// Set image list to the tree view
	TreeView_SetImageList(_hSelf, _hImaLst, TVSIL_NORMAL);
	_tvStyleType = useStdIcons ? NppDarkMode::TreeViewStyle::classic : NppDarkMode::getTreeViewStyle();

	return true;
}

std::vector<int> TreeView::getImageIds(std::vector<int> stdIds, std::vector<int> darkIds, std::vector<int> lightIds)
{
	NppParameters& nppParam = NppParameters::getInstance();
	const bool useStdIcons = nppParam.getNppGUI()._tbIconInfo._tbIconSet == TB_STANDARD;
	if (useStdIcons)
	{
		return stdIds;
	}

	switch (NppDarkMode::getTreeViewStyle())
	{
		case NppDarkMode::TreeViewStyle::light:
		{
			return lightIds;
		}

		case NppDarkMode::TreeViewStyle::dark:
		{
			return darkIds;
		}

		case NppDarkMode::TreeViewStyle::classic:
		{
			return stdIds;
		}
	}
	return stdIds;
}

void TreeView::cleanSubEntries(HTREEITEM hTreeItem)
{
	for (HTREEITEM hItem = getChildFrom(hTreeItem); hItem != NULL; hItem = getNextSibling(hItem))
	{
		TVITEM tvItem{};
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_PARAM;
		SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		cleanSubEntries(hItem);
	}
}

void TreeView::foldExpandRecursively(HTREEITEM hParentItem, bool isFold) const
{
	if (!hParentItem)
		return;

	HTREEITEM hItem = getChildFrom(hParentItem);

	for (; hItem != NULL; hItem = getNextSibling(hItem))
	{
		foldExpandRecursively(hItem, isFold);
		if (isFold)
		{
			fold(hItem);
		}
		else
		{
			expand(hItem);
		}
	}
}

void TreeView::foldExpandAll(bool isFold) const
{
	for (HTREEITEM tvProj = getRoot();
		tvProj != NULL;
		tvProj = getNextSibling(tvProj))
	{
		foldExpandRecursively(tvProj, isFold);
		if (isFold)
		{
			fold(tvProj);
		}
		else
		{
			expand(tvProj);
		}
	}
}

void TreeView::setItemImage(HTREEITEM hTreeItem, int iImage, int iSelectedImage)
{
	TVITEM tvItem{};
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
	_draggedImageList = reinterpret_cast<HIMAGELIST>(::SendMessage(_hSelf, TVM_CREATEDRAGIMAGE, 0, reinterpret_cast<LPARAM>(_draggedItem)));

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
	POINT point{};
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
	TVHITTESTINFO hitTestInfo{};
	hitTestInfo.pt.x = point.x;
	hitTestInfo.pt.y = point.y;
	HTREEITEM targetItem = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTestInfo)));
	if (targetItem)
	{
		::SendMessage(_hSelf, TVM_SELECTITEM, TVGN_DROPHILITE, reinterpret_cast<LPARAM>(targetItem));
	}

	// show the dragged image
	::ImageList_DragShowNolock(true);
}

bool TreeView::dropItem()
{
	bool isFilesMoved = false;
	// get the target item
	HTREEITEM targetItem = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_GETNEXTITEM, TVGN_DROPHILITE, 0));

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

	SendMessage(_hSelf, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(targetItem));
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
	wchar_t textBuffer[MAX_PATH]{};
	TVITEM tvDraggingItem{};
	tvDraggingItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvDraggingItem.pszText = textBuffer;
	tvDraggingItem.cchTextMax = MAX_PATH;
	tvDraggingItem.hItem = draggedItem;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvDraggingItem));

	TVINSERTSTRUCT tvInsertStruct{};
	tvInsertStruct.item = tvDraggingItem;
	tvInsertStruct.hInsertAfter = (HTREEITEM)TVI_LAST;
	tvInsertStruct.hParent = targetItem;

	HTREEITEM hTreeParent = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct)));
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
	wchar_t textBufferUp[MAX_PATH]{};
	wchar_t textBufferDown[MAX_PATH]{};
	TVITEM tvUpItem{};
	TVITEM tvDownItem{};
	tvUpItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvDownItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvUpItem.pszText = textBufferUp;
	tvDownItem.pszText = textBufferDown;
	tvUpItem.cchTextMax = MAX_PATH;
	tvDownItem.cchTextMax = MAX_PATH;
	tvUpItem.hItem = itemGoUp;
	tvDownItem.hItem = itemGoDown;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvUpItem));
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvDownItem));

	// add 2 new items
	TVINSERTSTRUCT tvInsertUp{};
	tvInsertUp.item = tvUpItem;
	tvInsertUp.hInsertAfter = itemTop;
	tvInsertUp.hParent = parentGoUp;
	HTREEITEM hTreeParent1stInserted = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertUp)));
	dupTree(itemGoUp, hTreeParent1stInserted);

	TVINSERTSTRUCT tvInsertDown{};
	tvInsertDown.item = tvDownItem;
	tvInsertDown.hInsertAfter = hTreeParent1stInserted;
	tvInsertDown.hParent = parentGoDown;
	HTREEITEM hTreeParent2ndInserted = reinterpret_cast<HTREEITEM>(::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertDown)));
	dupTree(itemGoDown, hTreeParent2ndInserted);

	// remove 2 old items
	removeItem(itemGoUp);
	removeItem(itemGoDown);

	// Restore the selection if needed
	switch (itemSelected)
	{
		case 1:
			selectItem(hTreeParent2ndInserted);
			break;
		case 2:
			selectItem(hTreeParent1stInserted);
			break;
		default:
			break;
	}
	return true;
}


bool TreeView::canDropIn(HTREEITEM targetItem)
{
	TVITEM tvItem{};
	tvItem.mask = TVIF_IMAGE;
	tvItem.hItem = targetItem;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	for (size_t i = 0, len = _canNotDropInList.size(); i < len; ++i)
	{
		if (tvItem.iImage == _canNotDropInList[i])
			return false;
	}
	return true;
}


bool TreeView::canDragOut(HTREEITEM targetItem)
{
	TVITEM tvItem{};
	tvItem.mask = TVIF_IMAGE;
	tvItem.hItem = targetItem;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	for (size_t i = 0, len = _canNotDragOutList.size(); i < len; ++i)
	{
		if (tvItem.iImage == _canNotDragOutList[i])
			return false;
	}
	return true;
}



bool TreeView::searchLeafAndBuildTree(const TreeView & tree2Build, const wstring & text2Search, int index2Search)
{
	//tree2Build.removeAllItems();
	//HTREEITEM root = getRoot();

	return searchLeafRecusivelyAndBuildTree(tree2Build.getRoot(), text2Search, index2Search, getRoot());
}

bool TreeView::searchLeafRecusivelyAndBuildTree(HTREEITEM tree2Build, const wstring & text2Search, int index2Search, HTREEITEM tree2Search)
{
	if (!tree2Search)
		return false;

	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.hItem = tree2Search;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	if (tvItem.iImage == index2Search)
	{
		wstring itemNameUpperCase = stringToUpper(tvItem.pszText);
		wstring text2SearchUpperCase = stringToUpper(text2Search);

		size_t res = itemNameUpperCase.find(text2SearchUpperCase);
		if (res != wstring::npos)
		{
			TVINSERTSTRUCT tvInsertStruct{};
			tvInsertStruct.item = tvItem;
			tvInsertStruct.hInsertAfter = TVI_LAST;
			tvInsertStruct.hParent = tree2Build;
			::SendMessage(_hSelf, TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvInsertStruct));
		}
	}

	bool isOk = true;
	for (HTREEITEM hItem = getChildFrom(tree2Search); hItem != NULL; hItem = getNextSibling(hItem))
	{
		isOk = searchLeafRecusivelyAndBuildTree(tree2Build, text2Search, index2Search, hItem);
		if (!isOk)
			break;
	}
	return isOk;
}


bool TreeView::retrieveFoldingStateTo(TreeStateNode & treeState2Construct, HTREEITEM treeviewNode)
{
	if (!treeviewNode)
		return false;

	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.hItem = treeviewNode;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	treeState2Construct._label = textBuffer;
	treeState2Construct._isExpanded = (tvItem.state & TVIS_EXPANDED) != 0;
	treeState2Construct._isSelected = (tvItem.state & TVIS_SELECTED) != 0;

	if (tvItem.lParam)
	{
		treeState2Construct._extraData = *((wstring *)tvItem.lParam);
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

void TreeView::sort(HTREEITEM hTreeItem, bool isRecusive)
{
	::SendMessage(_hSelf, TVM_SORTCHILDREN, TRUE, reinterpret_cast<LPARAM>(hTreeItem));
	if (!isRecusive)
		return;

	for (HTREEITEM hItem = getChildFrom(hTreeItem); hItem != NULL; hItem = getNextSibling(hItem))
		sort(hItem, isRecusive);
}


void TreeView::customSorting(HTREEITEM hTreeItem, PFNTVCOMPARE sortingCallbackFunc, LPARAM lParam, bool isRecusive)
{
	TVSORTCB treeViewSortCB{};
	treeViewSortCB.hParent = hTreeItem;
	treeViewSortCB.lpfnCompare = sortingCallbackFunc;
	treeViewSortCB.lParam = lParam;

	::SendMessage(_hSelf, TVM_SORTCHILDRENCB, 0, reinterpret_cast<LPARAM>(&treeViewSortCB));
	if (!isRecusive)
		return;

	for (HTREEITEM hItem = getChildFrom(hTreeItem); hItem != NULL; hItem = getNextSibling(hItem))
		customSorting(hItem, sortingCallbackFunc, lParam, isRecusive);
}
