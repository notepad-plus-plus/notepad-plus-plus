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


#include "functionListPanel.h"
#include "ScintillaEditView.h"
#include "localization.h"

using namespace std;

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
				++cntOpen;
			}
			else // if it's not open symbol, then it must be the close one
			{
				--cntOpen;
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

void FunctionListPanel::addInStateArray(TreeStateNode tree2Update, const TCHAR *searchText, bool isSorted)
{
	bool found = false;
	for (size_t i = 0, len = _treeParams.size(); i < len; ++i)
	{
		if (_treeParams[i]._treeState._extraData == tree2Update._extraData)
		{
			_treeParams[i]._searchParameters._text2Find = searchText;
			_treeParams[i]._searchParameters._doSort = isSorted;
			_treeParams[i]._treeState = tree2Update;
			found = true;
		}
	}
	if (!found)
	{
		TreeParams params;
		params._treeState = tree2Update;
		params._searchParameters._text2Find = searchText;
		params._searchParameters._doSort = isSorted;
		_treeParams.push_back(params);
	}
}

TreeParams* FunctionListPanel::getFromStateArray(generic_string fullFilePath)
{
	for (size_t i = 0, len = _treeParams.size(); i < len; ++i)
	{
		if (_treeParams[i]._treeState._extraData == fullFilePath)
			return &_treeParams[i];
	}
	return NULL;
}

void FunctionListPanel::sortOrUnsort()
{
	bool doSort = shouldSort();
	if (doSort)
		_pTreeView->sort(_pTreeView->getRoot());
	else
	{
		TCHAR text2search[MAX_PATH] ;
		::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, (LPARAM)text2search);

		if (text2search[0] == '\0') // main view
		{
			reload();
		}
		else // aux view
		{
			reload();

			if (_treeView.getRoot() == NULL)
				return;

			_treeViewSearchResult.removeAllItems();
			const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();
			_treeViewSearchResult.addItem(fn, NULL, INDEX_ROOT, TEXT("-1"));
			_treeView.searchLeafAndBuildTree(_treeViewSearchResult, text2search, INDEX_LEAF);
			_treeViewSearchResult.display(true);
			_treeViewSearchResult.expand(_treeViewSearchResult.getRoot());
			_treeView.display(false);
			_pTreeView = &_treeViewSearchResult;
		}
	}
}

void FunctionListPanel::reload()
{
	// clean up
	TreeStateNode currentTree;
	bool isOK = _treeView.retrieveFoldingStateTo(currentTree, _treeView.getRoot());
	if (isOK)
	{
		TCHAR text2Search[MAX_PATH];
		::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, (LPARAM)text2Search);
		bool isSorted =  shouldSort();
		addInStateArray(currentTree, text2Search, isSorted);
	}
	removeAllEntries();
	::SendMessage(_hSearchEdit, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	setSort(false);

	vector<foundInfo> fi;

	const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();
	LangType langID = ((*_ppEditView)->getCurrentBuffer())->getLangType();
	const TCHAR *udln = NULL;
	if (langID == L_USER)
	{
		udln = ((*_ppEditView)->getCurrentBuffer())->getUserDefineLangName();
	}

	TCHAR *ext = ::PathFindExtension(fn);

	if (_funcParserMgr.parse(fi, AssociationInfo(-1, langID, ext, udln)))
	{
		_treeView.addItem(fn, NULL, INDEX_ROOT, TEXT("-1"));
	}

	for (size_t i = 0, len = fi.size(); i < len; ++i)
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
		TreeParams *previousParams = getFromStateArray(fullFilePath);
		if (!previousParams)
		{
			::SendMessage(_hSearchEdit, WM_SETTEXT, 0, (LPARAM)TEXT(""));
			setSort(false);
			_treeView.expand(root);
		}
		else
		{
			::SendMessage(_hSearchEdit, WM_SETTEXT, 0, (LPARAM)(previousParams->_searchParameters)._text2Find.c_str());

			_treeView.restoreFoldingStateFrom(previousParams->_treeState, root);

			bool isSort = (previousParams->_searchParameters)._doSort;
			setSort(isSort);
			if (isSort)
				_pTreeView->sort(_pTreeView->getRoot());
		}
	}

	// invalidate the editor rect
	::InvalidateRect(_hSearchEdit, NULL, TRUE);
}


void FunctionListPanel::init(HINSTANCE hInst, HWND hPere, ScintillaEditView **ppEditView)
{
	DockingDlgInterface::init(hInst, hPere);
	_ppEditView = ppEditView;
	bool isOK = false;
	bool doLocalConf = (NppParameters::getInstance())->isLocal();

	if (!doLocalConf)
	{
		generic_string funcListXmlPath = (NppParameters::getInstance())->getUserPath();
		PathAppend(funcListXmlPath, TEXT("functionList.xml"));

		if (!PathFileExists(funcListXmlPath.c_str()))
		{
			generic_string funcListDefaultXmlPath = (NppParameters::getInstance())->getNppPath();
			PathAppend(funcListDefaultXmlPath, TEXT("functionList.xml"));
			if (PathFileExists(funcListDefaultXmlPath.c_str()))
			{
				::CopyFile(funcListDefaultXmlPath.c_str(), funcListXmlPath.c_str(), TRUE);
				isOK = _funcParserMgr.init(funcListXmlPath, ppEditView);
			}
		}
		else
		{
			isOK = _funcParserMgr.init(funcListXmlPath, ppEditView);
		}
	}
	else
	{
		generic_string funcListDefaultXmlPath = (NppParameters::getInstance())->getNppPath();
		PathAppend(funcListDefaultXmlPath, TEXT("functionList.xml"));
		if (PathFileExists(funcListDefaultXmlPath.c_str()))
		{
			isOK = _funcParserMgr.init(funcListDefaultXmlPath, ppEditView);
		}
	}

	//return isOK;
}

bool FunctionListPanel::openSelection(const TreeView & treeView)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	tvItem.hItem = treeView.getSelection();
	::SendMessage(treeView.getHSelf(), TVM_GETITEM, 0,(LPARAM)&tvItem);

	if (tvItem.iImage == INDEX_ROOT || tvItem.iImage == INDEX_NODE)
	{
		return false;
	}

	generic_string *posStr = (generic_string *)tvItem.lParam;
	if (!posStr)
		return false;

	int pos = generic_atoi(posStr->c_str());
	if (pos == -1)
		return false;

	int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos);
	(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
	(*_ppEditView)->scrollPosToCenter(pos);

	return true;
}

void FunctionListPanel::notified(LPNMHDR notification)
{
	if (notification->code == TTN_GETDISPINFO)
	{
		LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)notification;
		lpttt->hinst = NULL;

		if (notification->idFrom == IDC_SORTBUTTON_FUNCLIST)
		{
			lstrcpy(lpttt->szText, _sortTipStr.c_str());
		}
		else if (notification->idFrom == IDC_RELOADBUTTON_FUNCLIST)
		{
			lstrcpy(lpttt->szText, _reloadTipStr.c_str());
		}
	}
	else if (notification->hwndFrom == _treeView.getHSelf() || notification->hwndFrom == this->_treeViewSearchResult.getHSelf())
	{
		const TreeView & treeView = notification->hwndFrom == _treeView.getHSelf()?_treeView:_treeViewSearchResult;
		switch (notification->code)
		{
			case NM_DBLCLK:
			{
				openSelection(treeView);
			}
			break;

			case TVN_KEYDOWN:
			{
				LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)notification;

				if (ptvkd->wVKey == VK_RETURN)
				{
					if (!openSelection(treeView))
					{
						HTREEITEM hItem = treeView.getSelection();
						treeView.toggleExpandCollapse(hItem);
					}
				}
			}
			break;
		}
	}
	else if (notification->code == DMN_SWITCHIN)
	{
		reload();
	}
	else if (notification->code == DMN_CLOSE)
	{
		::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FUNC_LIST, 0);
	}
}

BOOL FunctionListPanel::setTreeViewImageList(int root_id, int node_id, int leaf_id)
{
	HBITMAP hbmp;
	COLORREF maskColour = RGB(192, 192, 192);
	const int nbBitmaps = 3;

	// Creation of image list
	if ((_hTreeViewImaLst = ImageList_Create(CX_BITMAP, CY_BITMAP, ILC_COLOR32 | ILC_MASK, nbBitmaps, 0)) == NULL)
		return FALSE;

	// Add the bmp in the list
	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hTreeViewImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(node_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hTreeViewImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(leaf_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hTreeViewImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	if (ImageList_GetImageCount(_hTreeViewImaLst) < nbBitmaps)
		return FALSE;

	// Set image list to the tree view
	TreeView_SetImageList(_treeView.getHSelf(), _hTreeViewImaLst, TVSIL_NORMAL);
	TreeView_SetImageList(_treeViewSearchResult.getHSelf(), _hTreeViewImaLst, TVSIL_NORMAL);

	return TRUE;
}

void FunctionListPanel::searchFuncAndSwitchView()
{
	TCHAR text2search[MAX_PATH] ;
	::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, (LPARAM)text2search);
	bool doSort = shouldSort();

	if (text2search[0] == '\0')
	{
		_treeViewSearchResult.display(false);
		_treeView.display(true);
		_pTreeView = &_treeView;
	}
	else
	{
		if (_treeView.getRoot() == NULL)
			return;

		_treeViewSearchResult.removeAllItems();
		const TCHAR *fn = ((*_ppEditView)->getCurrentBuffer())->getFileName();
		_treeViewSearchResult.addItem(fn, NULL, INDEX_ROOT, TEXT("-1"));
		_treeView.searchLeafAndBuildTree(_treeViewSearchResult, text2search, INDEX_LEAF);
		_treeViewSearchResult.display(true);
		_treeViewSearchResult.expand(_treeViewSearchResult.getRoot());
		_treeView.display(false);
		_pTreeView = &_treeViewSearchResult;

		// invalidate the editor rect
		::InvalidateRect(_hSearchEdit, NULL, TRUE);
	}

	if (doSort)
		_pTreeView->sort(_pTreeView->getRoot());
}

static WNDPROC oldFunclstToolbarProc = NULL;
static LRESULT CALLBACK funclstToolbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
    {
		case WM_CTLCOLOREDIT :
		{
			return ::SendMessage(::GetParent(hwnd), WM_CTLCOLOREDIT, wParam, lParam);
		}
	}
	return oldFunclstToolbarProc(hwnd, message, wParam, lParam);
}

bool FunctionListPanel::shouldSort()
{
	TBBUTTONINFO tbbuttonInfo;
	tbbuttonInfo.cbSize = sizeof(TBBUTTONINFO);
	tbbuttonInfo.dwMask = TBIF_STATE;

	::SendMessage(_hToolbarMenu, TB_GETBUTTONINFO, IDC_SORTBUTTON_FUNCLIST, (LPARAM)&tbbuttonInfo);

	return (tbbuttonInfo.fsState & TBSTATE_CHECKED) != 0;
}

void FunctionListPanel::setSort(bool isEnabled)
{
	TBBUTTONINFO tbbuttonInfo;
	tbbuttonInfo.cbSize = sizeof(TBBUTTONINFO);
	tbbuttonInfo.dwMask = TBIF_STATE;
	tbbuttonInfo.fsState = isEnabled ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
	::SendMessage(_hToolbarMenu, TB_SETBUTTONINFO, IDC_SORTBUTTON_FUNCLIST, (LPARAM)&tbbuttonInfo);
}

INT_PTR CALLBACK FunctionListPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		// Make edit field red if not found
		case WM_CTLCOLOREDIT :
		{
			// if the text not found modify the background color of the editor
			static HBRUSH hBrushBackground = CreateSolidBrush(BCKGRD_COLOR);
			TCHAR text2search[MAX_PATH] ;
			::SendMessage(_hSearchEdit, WM_GETTEXT, MAX_PATH, (LPARAM)text2search);
			if (text2search[0] == '\0')
			{
				return FALSE; // no text, use the default color
			}

			HTREEITEM searchViewRoot = _treeViewSearchResult.getRoot();
			if (searchViewRoot)
			{
				if (_treeViewSearchResult.getChildFrom(searchViewRoot))
					return FALSE; // children on root found, use the default color
			}
			else
				return FALSE; // no root (no parser), use the default color
			// text not found
			SetTextColor((HDC)wParam, TXT_COLOR);
			SetBkColor((HDC)wParam, BCKGRD_COLOR);
			return (LRESULT)hBrushBackground;
		}

        case WM_INITDIALOG :
        {
			int editWidth = NppParameters::getInstance()->_dpiManager.scaleX(100);
			int editWidthSep = NppParameters::getInstance()->_dpiManager.scaleX(105); //editWidth + 5
			int editHeight = NppParameters::getInstance()->_dpiManager.scaleY(20);
			int iconSizeX = NppParameters::getInstance()->_dpiManager.scaleX(16);
			int iconSizeY = NppParameters::getInstance()->_dpiManager.scaleY(16);
			// Create toolbar menu
			//int style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS |TBSTYLE_FLAT | CCS_TOP | BTNS_AUTOSIZE | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER;

			// CCS_ADJUSTABLE -- we have no customization, thus it caused double-clicking the toolbar to open and close instantly the dialog to select icons.
			// TBSTYLE_LIST -- "TBSTYLE_FLAT repaints its portion of the parent window after activating the tool bar (CHILDACTIVATE message), TBSTYLE_LIST does _NOT_ do this (and is reasonably annoying...)." (comment at https://msdn.microsoft.com/en-us/library/windows/desktop/bb760439.aspx)
			// TBSTYLE_WRAPABLE -- requires refreshing toolbar somewhere else, apparently.

			int style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | BTNS_AUTOSIZE | TBSTYLE_TOOLTIPS;
			_hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
								   0,0,0,0,_hSelf,(HMENU)0, _hInst, NULL);

			//::GetWindowLongPtr(_hToolbarMenu, GWLP_WNDPROC);
			oldFunclstToolbarProc = (WNDPROC)::SetWindowLongPtr(_hToolbarMenu, GWLP_WNDPROC, (LONG_PTR)funclstToolbarProc);
			TBBUTTON tbButtons[3];

			// Add the bmap image into toolbar's imagelist
			TBADDBITMAP addbmp = {_hInst, 0};
			addbmp.nID = IDI_FUNCLIST_SORTBUTTON;
			::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, (LPARAM)&addbmp);
			addbmp.nID = IDI_FUNCLIST_RELOADBUTTON;
			::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, (LPARAM)&addbmp);

			// Place holder of search text field
			tbButtons[0].idCommand = 0;
			tbButtons[0].iBitmap = editWidthSep;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_SEP; //This is just a separator (blank space)
			tbButtons[0].iString = 0;

			tbButtons[1].idCommand = IDC_SORTBUTTON_FUNCLIST;
			tbButtons[1].iBitmap = 0;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_CHECK | BTNS_AUTOSIZE;
			tbButtons[1].iString = (INT_PTR)TEXT("");

			tbButtons[2].idCommand = IDC_RELOADBUTTON_FUNCLIST;
			tbButtons[2].iBitmap = 1;
			tbButtons[2].fsState = TBSTATE_ENABLED;
			tbButtons[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[2].iString = (INT_PTR)TEXT("");

			::SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
			::SendMessage(_hToolbarMenu, TB_ADDBUTTONS, (WPARAM)sizeof(tbButtons) / sizeof(TBBUTTON), (LPARAM)&tbButtons);
			::SendMessage(_hToolbarMenu, TB_SETBUTTONSIZE, 0, MAKELPARAM(iconSizeX, iconSizeY)); //TB_SETBUTTONSIZE should be called after adding buttons.
			::SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0);

			ShowWindow(_hToolbarMenu, SW_SHOW);

			// tips text for toolbar buttons
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance())->getNativeLangSpeaker();
			_sortTipStr = pNativeSpeaker->getAttrNameStr(_sortTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_SORTLOCALNODENAME);
			_reloadTipStr = pNativeSpeaker->getAttrNameStr(_reloadTipStr.c_str(), FL_FUCTIONLISTROOTNODE, FL_RELOADLOCALNODENAME);

			_hSearchEdit = CreateWindowEx(0, L"Edit", NULL,
                                   WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOVSCROLL,
                                   2, 2, editWidth, editHeight,
                                   _hToolbarMenu, (HMENU) IDC_SEARCHFIELD_FUNCLIST, _hInst, 0 );

			HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
			if (hf)
				::SendMessage(_hSearchEdit, WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));

			_treeViewSearchResult.init(_hInst, _hSelf, IDC_LIST_FUNCLIST_AUX);
			_treeView.init(_hInst, _hSelf, IDC_LIST_FUNCLIST);
			setTreeViewImageList(IDI_FUNCLIST_ROOT, IDI_FUNCLIST_NODE, IDI_FUNCLIST_LEAF);

			_treeView.display();
            return TRUE;
        }

		case WM_DESTROY:
			_treeView.destroy();
			_treeViewSearchResult.destroy();
			::DestroyWindow(_hToolbarMenu);
			break;

		case WM_COMMAND :
		{
			if (HIWORD(wParam) == EN_CHANGE)
			{
				switch (LOWORD(wParam))
				{
					case  IDC_SEARCHFIELD_FUNCLIST:
					{
						searchFuncAndSwitchView();
						return TRUE;
					}
				}
			}

			switch (LOWORD(wParam))
            {
				case IDC_SORTBUTTON_FUNCLIST:
				{
					sortOrUnsort();
				}
				return TRUE;

				case IDC_RELOADBUTTON_FUNCLIST:
				{
					reload();
				}
				return TRUE;
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
			int extraValue = NppParameters::getInstance()->_dpiManager.scaleX(4);

			RECT toolbarMenuRect;
			::GetClientRect(_hToolbarMenu, &toolbarMenuRect);

			::MoveWindow(_hToolbarMenu, 0, 0, width, toolbarMenuRect.bottom, TRUE);

			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, toolbarMenuRect.bottom + extraValue, width, height - toolbarMenuRect.bottom - extraValue, TRUE);

			HWND hwnd_aux = _treeViewSearchResult.getHSelf();
			if (hwnd_aux)
				::MoveWindow(hwnd_aux, 0, toolbarMenuRect.bottom + extraValue, width, height - toolbarMenuRect.bottom - extraValue, TRUE);

			break;
		}

		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
