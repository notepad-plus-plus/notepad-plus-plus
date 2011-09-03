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

BOOL CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			_treeView.init(_hInst, _hSelf, ID_PROJECTTREEVIEW);

			_treeView.initImageList(IDR_WRAP, IDR_ZOOMIN, IDR_ZOOMOUT, IDR_FIND);
			_treeView.display();
			openProject(TEXT("C:\\sources\\Notepad++\\trunk\\PowerEditor\\src\\WinControls\\ProjectPanel\\demo.xml"));
            return TRUE;
        }

		case WM_NOTIFY:
		{
			notified((LPNMTREEVIEW)lParam);
		}
		return TRUE;

        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, 0, width, height, TRUE);
            break;
        }
        
		case WM_DESTROY:
        {
			//_fileListView.destroy();
            break;
        }

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
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
			_treeView.addItem((childNode->ToElement())->Attribute(TEXT("name")), hParentItem, INDEX_LEAF);
			//::MessageBox(NULL, (childNode->ToElement())->Attribute(TEXT("name")), TEXT("File"), MB_OK);
		}
	}
	return true;
}

void ProjectPanel::notified(LPNMTREEVIEW notification)
{
	TVITEM tv_item;
	TCHAR text_buffer[MAX_PATH];

	if((notification->hdr).hwndFrom == _treeView.getHSelf())
	{
		if((notification->hdr).code == TVN_SELCHANGED)
		{
			tv_item.hItem = notification->itemNew.hItem;
			tv_item.mask = TVIF_TEXT | TVIF_PARAM;
			tv_item.pszText = text_buffer;
			tv_item.cchTextMax = MAX_PATH;
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tv_item);

			/* 
			if(tv_item.lParam==DOMAIN_LEVEL)
			ShowDomainContent(tv_item.pszText);
			else if(tv_item.lParam==SERVER_LEVEL)
			ShowServerContent(tv_item.pszText);
			*/
		}
	}
}