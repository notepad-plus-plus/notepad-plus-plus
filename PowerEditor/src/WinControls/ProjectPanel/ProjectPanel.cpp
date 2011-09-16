/*
this file is part of notepad++
Copyright (C)2011 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "precompiledHeaders.h"
#include "ProjectPanel.h"
#include "resource.h"
#include "tinyxml.h"
#include "FileDialog.h"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

BOOL CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
          // Create toolbar menu
          int style = WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT;
        _hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
                               0,0,0,0,_hSelf,(HMENU)0, _hInst, NULL);
        TBBUTTON tbButtons[2];

        static TCHAR *projectMenuStr = TEXT("Project");
        tbButtons[0].idCommand = IDB_PROJECT_BTN;
        tbButtons[0].iBitmap = I_IMAGENONE;
        tbButtons[0].fsState = TBSTATE_ENABLED;
        tbButtons[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        tbButtons[0].iString = (INT_PTR)projectMenuStr;

        static TCHAR *editMenuStr = TEXT("Edit");
        tbButtons[1].idCommand = IDB_EDIT_BTN;
        tbButtons[1].iBitmap = I_IMAGENONE;
        tbButtons[1].fsState = TBSTATE_ENABLED;
        tbButtons[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        tbButtons[1].iString = (INT_PTR)editMenuStr;

        SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
        SendMessage(_hToolbarMenu, TB_ADDBUTTONS,       (WPARAM)sizeof(tbButtons) / sizeof(TBBUTTON),       (LPARAM)&tbButtons);
        SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0); 
        ShowWindow(_hToolbarMenu, SW_SHOW);

			_treeView.init(_hInst, _hSelf, ID_PROJECTTREEVIEW);

			_treeView.initImageList(IDI_PROJECT_ROOT, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE, IDI_PROJECT_FILEINVALID);
			_treeView.display();
			openProject(TEXT("D:\\source\\notepad++\\trunk\\PowerEditor\\src\\WinControls\\ProjectPanel\\demo.xml"));
            return TRUE;
        }

		case WM_NOTIFY:
		{
			notified((LPNMHDR)lParam);
		}
		return TRUE;

        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            RECT toolbarMenuRect;
            ::GetClientRect(_hToolbarMenu, &toolbarMenuRect);

            ::MoveWindow(_hToolbarMenu, 0, 0, width, toolbarMenuRect.bottom, TRUE);

			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
        ::MoveWindow(hwnd, 0, toolbarMenuRect.bottom + 2, width, height - toolbarMenuRect.bottom - 2, TRUE);
            break;
        }

        case WM_CONTEXTMENU:
			showContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

		case WM_COMMAND:
		{
			popupMenuCmd(LOWORD(wParam));
			break;
		}

		case WM_DESTROY:
        {
			_treeView.destroy();
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void ProjectPanel::init(HINSTANCE hInst, HWND hPere)
{
	DockingDlgInterface::init(hInst, hPere);

	_hProjectMenu = ::CreatePopupMenu();
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWPROJ, TEXT("New Project"));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_OPENPROJ, TEXT("Open Project File"));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEPROJ, TEXT("Save"));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEASPROJ, TEXT("Save As..."));

	_hRootMenu = ::CreatePopupMenu();
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("Add Folder"));
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, TEXT("Remove"));

	_hFolderMenu = ::CreatePopupMenu();
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("Add Folder"));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, TEXT("Remove"));

	_hFileMenu = ::CreatePopupMenu();
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFILE, TEXT("Remove"));
}

bool ProjectPanel::openProject(TCHAR *projectFileName)
{
	TiXmlDocument *pXmlDocProject = new TiXmlDocument(projectFileName);
	bool loadOkay = pXmlDocProject->LoadFile();
	if (!loadOkay)
		return false;

	TiXmlNode *root = pXmlDocProject->FirstChild(TEXT("NotepadPlus"));
	if (!root) 
		return false;

	root = root->FirstChild(TEXT("Project"));
	if (!root) 
		return false;

	HTREEITEM rootItem = _treeView.addItem((root->ToElement())->Attribute(TEXT("name")), TVI_ROOT, INDEX_PROJECT_ROOT);
    buildTreeFrom(root, rootItem);
	delete pXmlDocProject;
	
	return loadOkay;
}

bool ProjectPanel::buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem)
{
	for (TiXmlNode *childNode = projectRoot->FirstChildElement();
		childNode ;
		childNode = childNode->NextSibling())
	{
		const TCHAR *v = childNode->Value();
		if (lstrcmp(TEXT("Folder"), v) == 0)
		{
			//::MessageBox(NULL, (childNode->ToElement())->Attribute(TEXT("name")), TEXT("Folder"), MB_OK);
			HTREEITEM addedItem = _treeView.addItem((childNode->ToElement())->Attribute(TEXT("name")), hParentItem, INDEX_CLOSED_NODE);
			if (!childNode->NoChildren())
			{
				bool isOK = buildTreeFrom(childNode, addedItem);
				if (!isOK)
					return false;
			}
		}
		else if (lstrcmp(TEXT("File"), v) == 0)
		{
			const TCHAR *strValue = (childNode->ToElement())->Attribute(TEXT("name"));
			TCHAR *strValueLabel = ::PathFindFileName(strValue);
      int iImage = ::PathFileExists(strValue)?INDEX_LEAF:INDEX_LEAF_INVALID;
      _treeView.addItem(strValueLabel, hParentItem, iImage, strValue);
		}
	}
	return true;
}

void ProjectPanel::notified(LPNMHDR notification)
{
	if((notification->hwndFrom == _treeView.getHSelf()))
	{
		TCHAR textBuffer[MAX_PATH];
		TVITEM tvItem;
		tvItem.mask = TVIF_TEXT | TVIF_PARAM;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;

		switch (notification->code)
		{
			case NM_DBLCLK:
			{
				tvItem.hItem = _treeView.getSelection();
				::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);
				generic_string *fn = (generic_string *)tvItem.lParam;
				if (fn)
				{
					tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					if (::PathFileExists(fn->c_str()))
					{
						::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)(fn->c_str()));
						tvItem.iImage = INDEX_LEAF;
						tvItem.iSelectedImage = INDEX_LEAF;
					}
					else
					{
						tvItem.iImage = INDEX_LEAF_INVALID;
						tvItem.iSelectedImage = INDEX_LEAF_INVALID;
					}
					TreeView_SetItem(_treeView.getHSelf(), &tvItem);
				}
			}
			break;
	
			case TVN_ENDLABELEDIT:
			{
				LPNMTVDISPINFO tvnotif = (LPNMTVDISPINFO)notification;
				if (!tvnotif->item.pszText)
					return;

				// Processing for only File case
				if (tvnotif->item.lParam) 
				{
					// Get the old label
					tvItem.hItem = _treeView.getSelection();
					::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);
					size_t len = lstrlen(tvItem.pszText);

					// Find the position of old label in File path
					generic_string *filePath = (generic_string *)tvnotif->item.lParam;
					size_t found = filePath->rfind(tvItem.pszText);

					// If found the old label, replace it with the modified one
					if (found != generic_string::npos)
						filePath->replace(found, len, tvnotif->item.pszText);

					// Check the validity of modified file path
					tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					if (::PathFileExists(filePath->c_str()))
					{
						tvItem.iImage = INDEX_LEAF;
						tvItem.iSelectedImage = INDEX_LEAF;
					}
					else
					{
						tvItem.iImage = INDEX_LEAF_INVALID;
						tvItem.iSelectedImage = INDEX_LEAF_INVALID;
					}
					TreeView_SetItem(_treeView.getHSelf(), &tvItem);
				}

				// For File, Folder and Project
				::SendMessage(_treeView.getHSelf(), TVM_SETITEM, 0,(LPARAM)(&(tvnotif->item)));
			}
			break;

			case TVN_GETINFOTIP:
			{
				LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)notification;
				generic_string *str = (generic_string *)lpGetInfoTip->lParam;
				if (!str)
					return;
				lpGetInfoTip->pszText = (LPTSTR)str->c_str();
				lpGetInfoTip->cchTextMax = str->size();
			}
			break;

			case TVN_ITEMEXPANDED:
			{
				LPNMTREEVIEW nmtv = (LPNMTREEVIEW)notification;
				tvItem.hItem = nmtv->itemNew.hItem;
				tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);
				if (tvItem.iImage != INDEX_PROJECT_ROOT)
				{
					if (nmtv->action == TVE_COLLAPSE)
					{
						tvItem.iImage = INDEX_CLOSED_NODE;
						tvItem.iSelectedImage = INDEX_CLOSED_NODE;
						TreeView_SetItem(_treeView.getHSelf(), &tvItem);
					}
					else if (nmtv->action == TVE_EXPAND)
					{
						tvItem.iImage = INDEX_OPEN_NODE;
						tvItem.iSelectedImage = INDEX_OPEN_NODE;
						TreeView_SetItem(_treeView.getHSelf(), &tvItem);
					}
				}
			}
			break;
		}
	}
}

NodeType ProjectPanel::getNodeType(HTREEITEM hItem)
{
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);

	
	// Root
	if (tvItem.iImage == INDEX_PROJECT_ROOT)
	{
		return nodeType_root;
	}
	// Folder
	else if (tvItem.lParam == NULL)
	{
		return nodeType_node;
	}
	// File
	else
	{
		return nodeType_leaf;
	}
}

void ProjectPanel::showContextMenu(int x, int y)
{
	TVHITTESTINFO tvHitInfo;
	HTREEITEM hTreeItem;

	// Detect if the given position is on the element TVITEM
	tvHitInfo.pt.x = x;
	tvHitInfo.pt.y = y;
	tvHitInfo.flags = 0;
	ScreenToClient(_treeView.getHSelf(), &(tvHitInfo.pt));
	hTreeItem = TreeView_HitTest(_treeView.getHSelf(), &tvHitInfo);

	if(tvHitInfo.hItem != NULL)
	{
		// Make item selected
		TreeView_SelectItem(_treeView.getHSelf(), tvHitInfo.hItem);

		// get clicked item type
		NodeType nodeType = getNodeType(tvHitInfo.hItem);
		HMENU hMenu = NULL;
		if (nodeType == nodeType_root)
			hMenu = _hRootMenu;
		else if (nodeType == nodeType_node)
			hMenu = _hFolderMenu;
		else //nodeType_leaf
			hMenu = _hFileMenu;
		TrackPopupMenu(hMenu, TPM_LEFTALIGN, x, y, 0, _hSelf, NULL);
	}
}

POINT ProjectPanel::getMenuDisplyPoint(int iButton)
{
	POINT p;
	RECT btnRect;
	SendMessage(_hToolbarMenu, TB_GETITEMRECT, iButton, (LPARAM)&btnRect);

	p.x = btnRect.left;
	p.y = btnRect.top + btnRect.bottom;
	ClientToScreen(_hToolbarMenu, &p);
	return p;
}

void ProjectPanel::popupMenuCmd(int cmdID)
{
	// get selected item handle
	HTREEITEM hTreeItem = _treeView.getSelection();
	if (!hTreeItem)
		return;

	switch (cmdID)
	{
		case IDB_PROJECT_BTN:
		{
		  POINT p = getMenuDisplyPoint(0);
		  TrackPopupMenu(_hProjectMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		case IDB_EDIT_BTN:
		{
			POINT p = getMenuDisplyPoint(0);
			HMENU hMenu = NULL;
			NodeType nodeType = getNodeType(hTreeItem);
			if (nodeType == nodeType_root)
				hMenu = _hRootMenu;
			else if (nodeType == nodeType_node)
				hMenu = _hFolderMenu;
			else //nodeType_leaf
				hMenu = _hFileMenu;
		  TrackPopupMenu(hMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		case IDM_PROJECT_RENAME :
			TreeView_EditLabel(_treeView.getHSelf(), hTreeItem);
			break;
		
		case IDM_PROJECT_NEWFOLDER :
		{
			HTREEITEM addedItem = _treeView.addItem(TEXT("Folder Name"), hTreeItem, INDEX_CLOSED_NODE);
			TreeView_Expand(_treeView.getHSelf(), hTreeItem, TVE_EXPAND);
			TreeView_EditLabel(_treeView.getHSelf(), addedItem);
			_treeView.expandItemGUI(hTreeItem);
		}
		break;
		
		case IDM_PROJECT_ADDFILES :
		{
			addFiles(hTreeItem);
			_treeView.expandItemGUI(hTreeItem);
		}
		break;
		case IDM_PROJECT_DELETEFOLDER :
		{
			HTREEITEM parent = TreeView_GetParent(_treeView.getHSelf(), hTreeItem);

			if (_treeView.getChildFrom(hTreeItem) != NULL)
			{
				TCHAR str2display[MAX_PATH] = TEXT("All the sub-items will be removed.\rAre you sure to remove this folder from the project?");
				if (::MessageBox(_hSelf, str2display, TEXT("Remove folder from projet"), MB_YESNO) == IDYES)
				{
					_treeView.removeItem(hTreeItem);
				}
			}
			else
			{
				_treeView.removeItem(hTreeItem);
			}
			_treeView.collapsItemGUI(parent);
		}
		break;

		case IDM_PROJECT_DELETEFILE :
		{
			HTREEITEM parent = TreeView_GetParent(_treeView.getHSelf(), hTreeItem);

			TCHAR str2display[MAX_PATH] = TEXT("Are you sure to remove this file from the project?");
			if (::MessageBox(_hSelf, str2display, TEXT("Remove file from projet"), MB_YESNO) == IDYES)
			{
				_treeView.removeItem(hTreeItem);
				_treeView.collapsItemGUI(parent);
			}
		}
		break;
	}
}

void ProjectPanel::addFiles(HTREEITEM hTreeItem)
{
	FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);

	if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
	{
		size_t sz = pfns->size();
		for (size_t i = 0 ; i < sz ; i++)
		{
			TCHAR *strValueLabel = ::PathFindFileName(pfns->at(i).c_str());
			_treeView.addItem(strValueLabel, hTreeItem, INDEX_LEAF, pfns->at(i).c_str());
		}
		TreeView_Expand(_treeView.getHSelf(), hTreeItem, TVE_EXPAND);
	}
}