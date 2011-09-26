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

#define CX_BITMAP         16
#define CY_BITMAP         16

#define INDEX_CLEAN_ROOT     0
#define INDEX_DIRTY_ROOT     1
#define INDEX_PROJECT        2
#define INDEX_OPEN_NODE	     3
#define INDEX_CLOSED_NODE    4
#define INDEX_LEAF           5
#define INDEX_LEAF_INVALID   6

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

BOOL CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			ProjectPanel::initMenus();

			// Create toolbar menu
			int style = WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT;
			_hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
								   0,0,0,0,_hSelf,(HMENU)0, _hInst, NULL);
			TBBUTTON tbButtons[2];

			static TCHAR *projectMenuStr = TEXT("WorkSpace");
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

			setImageList(IDI_PROJECT_WORKSPACE, IDI_PROJECT_WORKSPACEDIRTY, IDI_PROJECT_PROJECT, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE, IDI_PROJECT_FILEINVALID);
			_treeView.display();
			if (!openWorkSpace(_workSpaceFilePath.c_str()))
				newWorkSpace();
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
			destroyMenus();
			::DestroyWindow(_hToolbarMenu);
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void ProjectPanel::checkIfNeedSave(const TCHAR *title)
{
	if (_isDirty)
	{
		display();
		int res = ::MessageBox(_hSelf, TEXT("The work space was modified. Do you want to save the it?"), title, MB_YESNO | MB_ICONQUESTION);
		if (res == IDYES)
		{
			if (!saveWorkSpace())
				::MessageBox(_hSelf, TEXT("Your work space was not saved."), title, MB_OK | MB_ICONERROR);
		}
		//else if (res == IDNO)
			// Don't save so do nothing here
	}
}

void ProjectPanel::initMenus()
{
	_hWorkSpaceMenu = ::CreatePopupMenu();
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWPROJECT, TEXT("Add New Project"));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWWS, TEXT("New WorkSpace"));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_OPENWS, TEXT("Open WorkSpace"));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RELOADWS, TEXT("Reload WorkSpace"));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEWS, TEXT("Save"));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEASWS, TEXT("Save As..."));
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEACOPYASWS, TEXT("Save a Copy As..."));

	_hProjectMenu = ::CreatePopupMenu();
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("Add Folder"));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, TEXT("Remove"));

	_hFolderMenu = ::CreatePopupMenu();
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("Add Folder"));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, TEXT("Remove"));

	_hFileMenu = ::CreatePopupMenu();
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFILE, TEXT("Remove"));
}


BOOL ProjectPanel::setImageList(int root_clean_id, int root_dirty_id, int project_id, int open_node_id, int closed_node_id, int leaf_id, int ivalid_leaf_id) 
{
	int i;
	HBITMAP hbmp;

	const int nbBitmaps = 7;

	// Creation of image list
	if ((_hImaLst = ImageList_Create(CX_BITMAP, CY_BITMAP, ILC_COLOR32 | ILC_MASK, nbBitmaps, 0)) == NULL) 
		return FALSE;

	// Add the bmp in the list
	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_clean_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_dirty_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(project_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(open_node_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(closed_node_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(leaf_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(ivalid_leaf_id));
	if(hbmp == NULL)
		return FALSE;
	i =ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	if (ImageList_GetImageCount(_hImaLst) < nbBitmaps)
		return FALSE;

	// Set image list to the tree view
  TreeView_SetImageList(_treeView.getHSelf(), _hImaLst, TVSIL_NORMAL);

	return TRUE;
}


void ProjectPanel::destroyMenus() 
{
	::DestroyMenu(_hWorkSpaceMenu);
	::DestroyMenu(_hProjectMenu);
	::DestroyMenu(_hFolderMenu);
	::DestroyMenu(_hFileMenu);
}

bool ProjectPanel::openWorkSpace(const TCHAR *projectFileName)
{
	TiXmlDocument *pXmlDocProject = new TiXmlDocument(projectFileName);
	bool loadOkay = pXmlDocProject->LoadFile();
	if (!loadOkay)
		return false;

	TiXmlNode *root = pXmlDocProject->FirstChild(TEXT("NotepadPlus"));
	if (!root) 
		return false;


	TiXmlNode *childNode = root->FirstChildElement(TEXT("Project"));
	if (!childNode) 
		return false;

	if (!::PathFileExists(projectFileName))
		return false;

	_workSpaceFilePath = projectFileName;

	HTREEITEM rootItem = _treeView.addItem(TEXT("Work Space"), TVI_ROOT, INDEX_CLEAN_ROOT);

	for ( ; childNode ; childNode = childNode->NextSibling(TEXT("Project")))
	{
		HTREEITEM projectItem = _treeView.addItem((childNode->ToElement())->Attribute(TEXT("name")), rootItem, INDEX_PROJECT);
		buildTreeFrom(childNode, projectItem);
	}
	setWorkSpaceDirty(false);
  _treeView.expand(rootItem);

	delete pXmlDocProject;
	return loadOkay;
}

void ProjectPanel::newWorkSpace()
{
  _treeView.addItem(TEXT("Work Space"), TVI_ROOT, INDEX_CLEAN_ROOT);
  setWorkSpaceDirty(false);
  _workSpaceFilePath = TEXT("");
}

bool ProjectPanel::saveWorkSpace()
{
	if (_workSpaceFilePath == TEXT(""))
	{
		return saveWorkSpaceAs(false);
	}
	else
	{
		writeWorkSpace();
		setWorkSpaceDirty(false);
		_isDirty = false;
		return true;
	} 
}

bool ProjectPanel::writeWorkSpace(TCHAR *projectFileName)
{
    //write <NotepadPlus>: use the default file name if new file name is not given
	const TCHAR * fn2write = projectFileName?projectFileName:_workSpaceFilePath.c_str();
	TiXmlDocument projDoc(fn2write);
    TiXmlNode *root = projDoc.InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));

	TCHAR textBuffer[MAX_PATH];
	TVITEM tvItem;
    tvItem.mask = TVIF_TEXT;
    tvItem.pszText = textBuffer;
    tvItem.cchTextMax = MAX_PATH;

    //for each project, write <Project>
    HTREEITEM tvRoot = _treeView.getRoot();
    if (!tvRoot)
      return false;

    for (HTREEITEM tvProj = _treeView.getChildFrom(tvRoot);
        tvProj != NULL;
        tvProj = _treeView.getNextSibling(tvProj))
    {        
        tvItem.hItem = tvProj;
        SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);
        //printStr(tvItem.pszText);

		TiXmlNode *projRoot = root->InsertEndChild(TiXmlElement(TEXT("Project")));
		projRoot->ToElement()->SetAttribute(TEXT("name"), tvItem.pszText);
		buildProjectXml(projRoot, tvProj, fn2write);
    }
    projDoc.SaveFile();
	return true;
}

void ProjectPanel::buildProjectXml(TiXmlNode *node, HTREEITEM hItem, const TCHAR* fn2write)
{
	TCHAR textBuffer[MAX_PATH];
	TVITEM tvItem;
	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;

    for (HTREEITEM hItemNode = _treeView.getChildFrom(hItem);
		hItemNode != NULL;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);
		if (tvItem.lParam != NULL)
		{
			generic_string *fn = (generic_string *)tvItem.lParam;
			generic_string newFn = getRelativePath(*fn, fn2write);
			TiXmlNode *fileLeaf = node->InsertEndChild(TiXmlElement(TEXT("File")));
			fileLeaf->ToElement()->SetAttribute(TEXT("name"), newFn.c_str());
		}
		else
		{
			TiXmlNode *folderNode = node->InsertEndChild(TiXmlElement(TEXT("Folder")));
			folderNode->ToElement()->SetAttribute(TEXT("name"), tvItem.pszText);
			buildProjectXml(folderNode, hItemNode, fn2write);
		}
	}
}

generic_string ProjectPanel::getRelativePath(const generic_string & filePath, const TCHAR *workSpaceFileName)
{
	TCHAR wsfn[MAX_PATH];
	lstrcpy(wsfn, workSpaceFileName);
	::PathRemoveFileSpec(wsfn);

	size_t pos_found = filePath.find(wsfn);
	if (pos_found == generic_string::npos)
		return filePath;
	const TCHAR *relativeFile = filePath.c_str() + lstrlen(wsfn) + 1;
	
	//printStr(relativeFile);
	return relativeFile;
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
      generic_string fullPath = getAbsoluteFilePath(strValue);
			TCHAR *strValueLabel = ::PathFindFileName(strValue);
			int iImage = ::PathFileExists(fullPath.c_str())?INDEX_LEAF:INDEX_LEAF_INVALID;
			_treeView.addItem(strValueLabel, hParentItem, iImage, fullPath.c_str());
		}
	}
	return true;
}

generic_string ProjectPanel::getAbsoluteFilePath(const TCHAR * relativePath)
{
  if (!::PathIsRelative(relativePath))
    return relativePath;

  TCHAR absolutePath[MAX_PATH];
  lstrcpy(absolutePath, _workSpaceFilePath.c_str());
  ::PathRemoveFileSpec(absolutePath);
  ::PathAppend(absolutePath, relativePath);
  return absolutePath;
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
				if (getNodeType(tvnotif->item.hItem) == nodeType_root)
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
				setWorkSpaceDirty(true);
			}
			break;

			case TVN_GETINFOTIP:
			{
				LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)notification;
				generic_string *str = NULL ;

				if (_treeView.getRoot() == lpGetInfoTip->hItem)
				{
					str = &_workSpaceFilePath;
				}
				else
				{
					str = (generic_string *)lpGetInfoTip->lParam;
					if (!str)
						return;
				}
				lpGetInfoTip->pszText = (LPTSTR)str->c_str();
				lpGetInfoTip->cchTextMax = str->size();
			}
			break;

			case TVN_ITEMEXPANDED:
			{
				LPNMTREEVIEW nmtv = (LPNMTREEVIEW)notification;
				tvItem.hItem = nmtv->itemNew.hItem;
				tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

				if (getNodeType(nmtv->itemNew.hItem) == nodeType_folder)
				{
					if (nmtv->action == TVE_COLLAPSE)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_CLOSED_NODE, INDEX_CLOSED_NODE);
					}
					else if (nmtv->action == TVE_EXPAND)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
					}
				}
			}
			break;
		}
	}
}

void ProjectPanel::setWorkSpaceDirty(bool isDirty)
{
  _isDirty = isDirty;
  int iImg = _isDirty?INDEX_DIRTY_ROOT:INDEX_CLEAN_ROOT;
  _treeView.setItemImage(_treeView.getRoot(), iImg, iImg);
}

NodeType ProjectPanel::getNodeType(HTREEITEM hItem)
{
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);

	// Root
	if (tvItem.iImage == INDEX_CLEAN_ROOT || tvItem.iImage == INDEX_DIRTY_ROOT)
	{
		return nodeType_root;
	}
	// Project
	else if (tvItem.iImage == INDEX_PROJECT)
	{
		return nodeType_project;
	}
	// Folder
	else if (tvItem.lParam == NULL)
	{
		return nodeType_folder;
	}
	// File
	else
	{
		return nodeType_file;
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
			hMenu = _hWorkSpaceMenu;
		else if (nodeType == nodeType_project)
			hMenu = _hProjectMenu;
		else if (nodeType == nodeType_folder)
			hMenu = _hFolderMenu;
		else //nodeType_file
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
		//
		// Toolbar menu buttons
		//
		case IDB_PROJECT_BTN:
		{
		  POINT p = getMenuDisplyPoint(0);
		  TrackPopupMenu(_hWorkSpaceMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		case IDB_EDIT_BTN:
		{
			POINT p = getMenuDisplyPoint(1);
			HMENU hMenu = NULL;
			NodeType nodeType = getNodeType(hTreeItem);
			if (nodeType == nodeType_project)
				hMenu = _hProjectMenu;
			else if (nodeType == nodeType_folder)
				hMenu = _hFolderMenu;
			else if (nodeType == nodeType_file)
				hMenu = _hFileMenu;
			if (hMenu)
				TrackPopupMenu(hMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		//
		// Toolbar menu commands
		//
		case IDM_PROJECT_NEWPROJECT :
		{
			HTREEITEM root = _treeView.getRoot();
			HTREEITEM addedItem = _treeView.addItem(TEXT("Project Name"),  root, INDEX_PROJECT);
			setWorkSpaceDirty(true);
			_treeView.expand(hTreeItem);
			TreeView_EditLabel(_treeView.getHSelf(), addedItem);
		}
		break;

		case IDM_PROJECT_NEWWS :
		{
			if (_isDirty)
			{
				int res = ::MessageBox(_hSelf, TEXT("The current work space was modified. Do you want to save the current project?"), TEXT(""), MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
				if (res == IDYES)
				{
					if (!saveWorkSpace())
						return;
				}
				else if (res == IDNO)
				{
					// Don't save so do nothing here
				}
				else if (res == IDCANCEL) 
				{
					// User cancels action "New WorkSpace" so we interrupt here
					return;
				}
			}
			_treeView.removeAllItems();
			newWorkSpace();
		}
		break;

		case IDM_PROJECT_RENAME :
			TreeView_EditLabel(_treeView.getHSelf(), hTreeItem);
		break;
		case IDM_PROJECT_NEWFOLDER :
		{
			HTREEITEM addedItem = _treeView.addItem(TEXT("Folder Name"), hTreeItem, INDEX_CLOSED_NODE);
			setWorkSpaceDirty(true);
			TreeView_Expand(_treeView.getHSelf(), hTreeItem, TVE_EXPAND);
			TreeView_EditLabel(_treeView.getHSelf(), addedItem);
			if (getNodeType(hTreeItem) == nodeType_folder)
				_treeView.setItemImage(hTreeItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
		}
		break;
		
		case IDM_PROJECT_ADDFILES :
		{
			addFiles(hTreeItem);
			if (getNodeType(hTreeItem) == nodeType_folder)
				_treeView.setItemImage(hTreeItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
		}
		break;

		case IDM_PROJECT_OPENWS:
		{
			if (_isDirty)
			{
				int res = ::MessageBox(_hSelf, TEXT("The current work space was modified. Do you want to save the current project?"), TEXT(""), MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
				if (res == IDYES)
				{
					if (!saveWorkSpace())
						return;
				}
				else if (res == IDNO)
				{
					// Don't save so do nothing here
				}
				else if (res == IDCANCEL) 
				{
					// User cancels action "New WorkSpace" so we interrupt here
					return;
				}
			}

			FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
			fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
			if (TCHAR *fn = fDlg.doOpenSingleFileDlg())
			{
				_treeView.removeAllItems();
				openWorkSpace(fn);
				_workSpaceFilePath = fn;
			}
		}
		break;

		case IDM_PROJECT_RELOADWS:
		{
			if (_isDirty)
			{
				int res = ::MessageBox(_hSelf, TEXT("The current work space was modified. Reload will discard all modification.\rDo you want to continue?"), TEXT(""), MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
				if (res == IDYES)
				{
					// Do nothing
				}
				else if (res == IDNO)
				{
					return;
				}
			}

			if (::PathFileExists(_workSpaceFilePath.c_str()))
			{
				_treeView.removeAllItems();
				openWorkSpace(_workSpaceFilePath.c_str());
			}
			else
			{
				::MessageBox(_hSelf, TEXT("Can not find file to reload"), TEXT(""), MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
			}
		}
		break;

		case IDM_PROJECT_SAVEWS:
			saveWorkSpace();
		break;

		case IDM_PROJECT_SAVEACOPYASWS:
		case IDM_PROJECT_SAVEASWS:
		{
			saveWorkSpaceAs(cmdID == IDM_PROJECT_SAVEACOPYASWS);
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
					setWorkSpaceDirty(true);
				}
			}
			else
			{
				_treeView.removeItem(hTreeItem);
				setWorkSpaceDirty(true);
			}
			if (getNodeType(parent) == nodeType_folder)
				_treeView.setItemImage(parent, INDEX_CLOSED_NODE, INDEX_CLOSED_NODE);
		}
		break;

		case IDM_PROJECT_DELETEFILE :
		{
			HTREEITEM parent = TreeView_GetParent(_treeView.getHSelf(), hTreeItem);

			TCHAR str2display[MAX_PATH] = TEXT("Are you sure to remove this file from the project?");
			if (::MessageBox(_hSelf, str2display, TEXT("Remove file from projet"), MB_YESNO) == IDYES)
			{
				_treeView.removeItem(hTreeItem);
				setWorkSpaceDirty(true);
				if (getNodeType(parent) == nodeType_folder)
					_treeView.setItemImage(parent, INDEX_CLOSED_NODE, INDEX_CLOSED_NODE);
			}
		}
		break;
	}
}

bool ProjectPanel::saveWorkSpaceAs(bool saveCopyAs)
{
	FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);

	if (TCHAR *fn = fDlg.doSaveDlg())
	{
		writeWorkSpace(fn);
		if (!saveCopyAs)
		{
			_workSpaceFilePath = fn;
			setWorkSpaceDirty(false);
		}
		return true;
	}
	return false;
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
    _treeView.expand(hTreeItem);
		setWorkSpaceDirty(true);
	}
}