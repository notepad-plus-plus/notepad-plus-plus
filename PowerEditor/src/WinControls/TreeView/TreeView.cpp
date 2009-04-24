//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#include "TreeView.h"

HTREEITEM TreeView::insertTo(HTREEITEM parent, TCHAR *itemStr, int imgIndex)
{
	TV_INSERTSTRUCT tvinsert;
	tvinsert.hParent=parent;
	tvinsert.hInsertAfter=parent?TVI_LAST:TVI_ROOT;
	tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvinsert.item.pszText=itemStr;
	tvinsert.item.iImage=imgIndex;
	tvinsert.item.iSelectedImage=0;
	return (HTREEITEM)::SendMessage(_hSelf, TVM_INSERTITEM, 0, (LPARAM)&tvinsert);
}

void TreeView::init(HINSTANCE hInst, HWND pere)
{
	Window::init(hInst, pere);
	InitCommonControls(); 

	HTREEITEM Parent;           // Tree item handle
	HTREEITEM Before;           // .......
	HTREEITEM Root; 
	
    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    
    _hSelf = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Tree View"),
                            WS_VISIBLE | WS_CHILD | WS_BORDER | 
							TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS , 
                            0,  0,  0, 0,
                            _hParent, 
                            NULL, 
                            _hInst, 
                            NULL); 
	
	if (!_hSelf)
		throw int(56);

	Parent = insertTo(NULL, TEXT("MAOCS30 Command"), 0);
	Root=Parent;
	Before=Parent;

	Parent = insertTo(Parent, TEXT("Native command"), 0);
	insertTo(Parent, TEXT("Power On"), 0);
	insertTo(Parent, TEXT("Power off"), 0);
	insertTo(Parent, TEXT("Entrant"), 0);
	insertTo(Parent, TEXT("Sortant"), 0);
	Parent = insertTo(Before, TEXT("Macro"), 0);
	insertTo(Parent, TEXT("ChangeCode"), 0);
	insertTo(Parent, TEXT("CipherData"), 0);
	
	insertTo(NULL, TEXT("Bla bla bla bla..."), 0);
	//display();
}