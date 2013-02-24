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


#include "precompiledHeaders.h"
#include "functionListPanel.h"
#include "ScintillaEditView.h"

#define CX_BITMAP         16
#define CY_BITMAP         16

#define INDEX_ROOT        0
#define INDEX_NODE        1
#define INDEX_LEAF        2

void FunctionListPanel::addEntry(const TCHAR *nodeName, const TCHAR *displayText, size_t pos)
{
	HTREEITEM itemParent = NULL;
	TCHAR posStr[32];
	generic_itoa(pos, posStr, 10);
	HTREEITEM root = _treeView.getRoot();

	if (nodeName != NULL && *nodeName != '\0')
	{
		itemParent = _treeView.searchSubItemByName(nodeName, root);
		if (!itemParent)
		{
			itemParent = _treeView.addItem(nodeName, root, INDEX_NODE, TEXT("-1"));
		}
	}
	else
		itemParent = root;

	_treeView.addItem(displayText, itemParent, INDEX_LEAF, posStr);
}

void FunctionListPanel::removeAllEntries()
{
	_treeView.removeAllItems();
}

// bodyOpenSybe mbol & bodyCloseSymbol should be RE
size_t FunctionListPanel::getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol)
{
	size_t cntOpen = 1;

	int docLen = (*_ppEditView)->getCurrentDocLen();

	if (begin >= (size_t)docLen)
		return docLen;

	generic_string exprToSearch = TEXT("(");
	exprToSearch += bodyOpenSymbol;
	exprToSearch += TEXT("|");
	exprToSearch += bodyCloseSymbol;
	exprToSearch += TEXT(")");


	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), begin, docLen);
	int targetEnd = 0;

	do
	{
		if (targetStart != -1 && targetStart != -2) // found open or close symbol
		{
			targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));

			// Now we determinate the symbol (open or close)
			int tmpStart = (*_ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
			if (tmpStart != -1 && tmpStart != -2) // open symbol found 
			{
				cntOpen++;
			}
			else // if it's not open symbol, then it must be the close one
			{
				cntOpen--;
			}
		}
		else // nothing found
		{
			cntOpen = 0; // get me out of here
			targetEnd = begin;
		}

		targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), targetEnd, docLen);

	} while (cntOpen);

	return targetEnd;
}

generic_string FunctionListPanel::parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos)
{
	if (begin >= end)
	{
		foundPos = -1;
		return TEXT("");
	}

	if (!dataToSearch.size())
		return TEXT("");

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const TCHAR *regExpr2search = dataToSearch[0].c_str();
	int targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart == -1 || targetStart == -2)
	{
		foundPos = -1;
		return TEXT("");
	}
	int targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos);
	}
	else // only one processed element, so we conclude the result
	{
		TCHAR foundStr[1024];

		(*_ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		foundPos = targetStart;
		return foundStr;
	}
}

void FunctionListPanel::addInTreeStateArray(TreeStateNode tree2Update)
{
	bool found = false;
	for (size_t i = 0; i < _treeStates.size(); i++)
	{
		if (_treeStates[i]._extraData == tree2Update._extraData)
		{
			_treeStates[i] = tree2Update;
			found = true;
		}
	}
	if (!found)
		_treeStates.push_back(tree2Update);

}

TreeStateNode* FunctionListPanel::getFromTreeStateArray(generic_string fullFilePath)
{
	for (size_t i = 0; i < _treeStates.size(); i++)
	{
		if (_treeStates[i]._extraData == fullFilePath)
			return &_treeStates[i];
	}
	return NULL;
}

void FunctionListPanel::reload()
{
	// clean up
	TreeStateNode currentTree;
	bool isOK = _treeView.retrieveFoldingStateTo(currentTree, _treeView.getRoot());
	if (isOK)
		addInTreeStateArray(currentTree);
	removeAllEntries();

	vector<foundInfo> fi;
	
	const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();
	
	TCHAR *ext = ::PathFindExtension(fn);
	if (_funcParserMgr.parse(fi, ext))
		_treeView.addItem(fn, NULL, INDEX_ROOT, TEXT("-1"));

	for (size_t i = 0; i < fi.size(); i++)
	{
		// no 2 level
		bool b = false;
		if (b)
		{
			generic_string entryName = TEXT("");
			if (fi[i]._pos2 != -1)
			{
				entryName = fi[i]._data2;
				entryName += TEXT("=>");
			}
			entryName += fi[i]._data;
			addEntry(NULL, entryName.c_str(), fi[i]._pos);
		}
		else
		{
			addEntry(fi[i]._data2.c_str(), fi[i]._data.c_str(), fi[i]._pos);
		}
	}
	HTREEITEM root = _treeView.getRoot();
	const TCHAR *fullFilePath = ((*_ppEditView)->getCurrentBuffer())->getFullPathName();
	if (root)
	{
		_treeView.setItemParam(root, fullFilePath);
		TreeStateNode *previousTree = getFromTreeStateArray(fullFilePath);
		if (!previousTree)
		{
			_treeView.expand(root);
		}
		else
		{
			_treeView.restoreFoldingStateFrom(*previousTree, root);
		}
	}
}
void FunctionListPanel::init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView)
{
		DockingDlgInterface::init(hInst, hPere);
		_ppEditView = ppEditView;
		generic_string funcListXmlPath = (NppParameters::getInstance())->getUserPath();
		PathAppend(funcListXmlPath, TEXT("functionList.xml"));
		_funcParserMgr.init(funcListXmlPath, ppEditView);
}

void FunctionListPanel::notified(LPNMHDR notification)
{
	if((notification->hwndFrom == _treeView.getHSelf()))
	{
		switch (notification->code)
		{
			case NM_DBLCLK:
			{
				TVITEM tvItem;
				tvItem.mask = TVIF_PARAM;
				tvItem.hItem = _treeView.getSelection();
				::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);

				//NodeType nType = getNodeType(tvItem.hItem);
				generic_string *posStr = (generic_string *)tvItem.lParam;
				if (posStr)
				{
					int pos = generic_atoi(posStr->c_str());
					if (pos != -1)
					{
						int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos);
						(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
						(*_ppEditView)->execute(SCI_GOTOPOS, pos);
					}
				}
			}
			break;
		}
	}
				
}


BOOL FunctionListPanel::setImageList(int root_id, int node_id, int leaf_id)
{
	HBITMAP hbmp;

	const int nbBitmaps = 3;

	// Creation of image list
	if ((_hImaLst = ImageList_Create(CX_BITMAP, CY_BITMAP, ILC_COLOR32 | ILC_MASK, nbBitmaps, 0)) == NULL) 
		return FALSE;

	// Add the bmp in the list
	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(node_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(leaf_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_Add(_hImaLst, hbmp, (HBITMAP)NULL);
	DeleteObject(hbmp);

	if (ImageList_GetImageCount(_hImaLst) < nbBitmaps)
		return FALSE;

	// Set image list to the tree view
	TreeView_SetImageList(_treeView.getHSelf(), _hImaLst, TVSIL_NORMAL);

	return TRUE;
}

BOOL CALLBACK FunctionListPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			_treeView.init(_hInst, _hSelf, IDC_LIST_FUNCLIST);
			setImageList(IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF);
			_treeView.display();
            return TRUE;
        }
		
		case WM_DESTROY:
			break;

		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
            {
                case IDC_LIST_FUNCLIST:
				{
					if (HIWORD(wParam) == LBN_DBLCLK)
					{
						int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCURSEL, 0, 0);
						if (i != LB_ERR)
						{
							int pos = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETITEMDATA, i, (LPARAM)0);
							//printInt(pos);
							int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, pos);
						}
					}
					return TRUE;
				}
			}
		}
		break;
		
		case WM_NOTIFY:
		{
			notified((LPNMHDR)lParam);
		}
		return TRUE;

        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			//::MoveWindow(::GetDlgItem(_hSelf, IDC_LIST_FUNCLIST), 0, 0, width, height, TRUE);
			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, 0, width, height, TRUE);
            break;
        }
/*
		case WM_VKEYTOITEM:
		{
			if (LOWORD(wParam) == VK_RETURN)
			{
				int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_GETCURSEL, 0, 0);
				printInt(i);
				return TRUE;
			}//return TRUE;
			break;
		}
*/

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
