// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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


#include "fileBrowser.h"
#include "resource.h"
#include "tinyxml.h"
#include "FileDialog.h"
#include "localization.h"
#include "Parameters.h"
#include "RunDlg.h"
#include "ReadDirectoryChanges.h"
#include "menuCmdID.h"
#include "Parameters.h"

#define CX_BITMAP         16
#define CY_BITMAP         16

#define INDEX_OPEN_ROOT      0
#define INDEX_CLOSE_ROOT     1
#define INDEX_OPEN_NODE	     2
#define INDEX_CLOSE_NODE     3
#define INDEX_LEAF           4


#define GET_X_LPARAM(lp) static_cast<short>(LOWORD(lp))
#define GET_Y_LPARAM(lp) static_cast<short>(HIWORD(lp))

#define FB_ADDFILE (WM_USER + 1024)
#define FB_RMFILE  (WM_USER + 1025)
#define FB_RNFILE  (WM_USER + 1026)
#define FB_CMD_AIMFILE 1
#define FB_CMD_FOLDALL 2
#define FB_CMD_EXPANDALL 3


FileBrowser::~FileBrowser()
{
	for (const auto folder : _folderUpdaters)
	{
		folder->stopWatcher();
		delete folder;
	}

	for (const auto cd : sortingDataArray)
	{
		delete cd;
	}
}

vector<generic_string> split(const generic_string & string2split, TCHAR sep)
{
	vector<generic_string> splitedStrings;
	size_t len = string2split.length();
	size_t beginPos = 0;
	for (size_t i = 0; i < len + 1; ++i)
	{
		if (string2split[i] == sep || string2split[i] == '\0')
		{
			splitedStrings.push_back(string2split.substr(beginPos, i - beginPos));
			beginPos = i + 1;
		}
	}
	return splitedStrings;
};

bool isRelatedRootFolder(const generic_string & relatedRoot, const generic_string & subFolder)
{
	if (relatedRoot.empty())
		return false;

	if (subFolder.empty())
		return false;

	size_t pos = subFolder.find(relatedRoot);
	if (pos != 0) // pos == 0 is the necessary condition, but not enough
		return false;

	vector<generic_string> relatedRootArray = split(relatedRoot, '\\');
	vector<generic_string> subFolderArray = split(subFolder, '\\');

	size_t index2Compare = relatedRootArray.size() - 1;

	return relatedRootArray[index2Compare] == subFolderArray[index2Compare];
}

INT_PTR CALLBACK FileBrowser::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			NppParameters& nppParam = NppParameters::getInstance();
			int style = WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | BTNS_AUTOSIZE | BTNS_SEP | TBSTYLE_TOOLTIPS;
			_hToolbarMenu = CreateWindowEx(WS_EX_LAYOUTRTL, TOOLBARCLASSNAME, NULL, style, 0, 0, 0, 0, _hSelf, nullptr, _hInst, NULL);
			TBBUTTON tbButtons[3];
			// Add the bmap image into toolbar's imagelist
			TBADDBITMAP addbmp = { _hInst, 0 };
			addbmp.nID = IDI_FB_SELECTCURRENTFILE;
			::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));
			addbmp.nID = IDI_FB_FOLDALL;
			::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));
			addbmp.nID = IDI_FB_EXPANDALL;
			::SendMessage(_hToolbarMenu, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));
			tbButtons[0].idCommand = FB_CMD_AIMFILE;
			tbButtons[0].iBitmap = 0;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[0].iString = reinterpret_cast<INT_PTR>(TEXT(""));
			tbButtons[1].idCommand = FB_CMD_FOLDALL;
			tbButtons[1].iBitmap = 1;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[1].iString = reinterpret_cast<INT_PTR>(TEXT(""));
			tbButtons[2].idCommand = FB_CMD_EXPANDALL;
			tbButtons[2].iBitmap = 2;
			tbButtons[2].fsState = TBSTATE_ENABLED;
			tbButtons[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[2].iString = reinterpret_cast<INT_PTR>(TEXT(""));

			// tips text for toolbar buttons
			NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
			_expandAllFolders = pNativeSpeaker->getAttrNameStr(_expandAllFolders.c_str(), FOLDERASWORKSPACE_NODE, "ExpandAllFoldersTip");
			_collapseAllFolders = pNativeSpeaker->getAttrNameStr(_collapseAllFolders.c_str(), FOLDERASWORKSPACE_NODE, "CollapseAllFoldersTip");
			_locateCurrentFile = pNativeSpeaker->getAttrNameStr(_locateCurrentFile.c_str(), FOLDERASWORKSPACE_NODE, "LocateCurrentFileTip");

			::SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			::SendMessage(_hToolbarMenu, TB_SETBUTTONSIZE, 0, MAKELONG(nppParam._dpiManager.scaleX(20), nppParam._dpiManager.scaleY(20)));
			::SendMessage(_hToolbarMenu, TB_SETPADDING, 0, MAKELONG(20, 0));
			::SendMessage(_hToolbarMenu, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(&tbButtons));
			::SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0);
			ShowWindow(_hToolbarMenu, SW_SHOW);

			FileBrowser::initPopupMenus();

			_treeView.init(_hInst, _hSelf, ID_FILEBROWSERTREEVIEW);
			setImageList(IDI_FB_ROOTOPEN, IDI_FB_ROOTCLOSE, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE);

			_treeView.addCanNotDropInList(INDEX_OPEN_ROOT);
			_treeView.addCanNotDropInList(INDEX_CLOSE_ROOT);
			_treeView.addCanNotDropInList(INDEX_OPEN_NODE);
			_treeView.addCanNotDropInList(INDEX_CLOSE_NODE);
			_treeView.addCanNotDropInList(INDEX_LEAF);

			_treeView.addCanNotDragOutList(INDEX_OPEN_ROOT);
			_treeView.addCanNotDragOutList(INDEX_CLOSE_ROOT);
			_treeView.addCanNotDragOutList(INDEX_OPEN_NODE);
			_treeView.addCanNotDragOutList(INDEX_CLOSE_NODE);
			_treeView.addCanNotDragOutList(INDEX_LEAF);

			_treeView.makeLabelEditable(false);
			_treeView.display();

            return TRUE;
        }

		case WM_MOUSEMOVE:
			if (_treeView.isDragging())
				_treeView.dragItem(_hSelf, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONUP:
			if (_treeView.isDragging())
				if (_treeView.dropItem())
				{
				
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
			int extraValue = NppParameters::getInstance()._dpiManager.scaleX(4);

			RECT toolbarMenuRect;
			::GetClientRect(_hToolbarMenu, &toolbarMenuRect);

			::MoveWindow(_hToolbarMenu, 0, 0, width, toolbarMenuRect.bottom, TRUE);

			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, toolbarMenuRect.bottom + extraValue, width, height - toolbarMenuRect.bottom - extraValue, TRUE);
            break;
        }

        case WM_CONTEXTMENU:
			if (!_treeView.isDragging())
				showContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case FB_CMD_AIMFILE:
				{
					selectCurrentEditingFile();
					break;
				}

				case FB_CMD_FOLDALL:
				{
					_treeView.foldAll();
					break;
				}

				case FB_CMD_EXPANDALL:
				{
					_treeView.expandAll();
					break;
				}

				default:
					popupMenuCmd(LOWORD(wParam));
			}
			break;
		}

		case WM_DESTROY:
        {
			::DestroyWindow(_hToolbarMenu);
			_treeView.destroy();
			destroyMenus();
            break;
        }

		case FB_ADDFILE:
		{
			const std::vector<generic_string> file2Change = *(std::vector<generic_string> *)lParam;
			generic_string separator = TEXT("\\\\");

			size_t sepPos = file2Change[0].find(separator);
			if (sepPos == generic_string::npos)
				return false;

			generic_string pathSuffix = file2Change[0].substr(sepPos + separator.length(), file2Change[0].length() - 1);

			// remove prefix of file/folder in changeInfo, splite the remained path
			vector<generic_string> linarPathArray = split(pathSuffix, '\\');

			generic_string rootPath = file2Change[0].substr(0, sepPos);
			generic_string path = rootPath;

			generic_string addedFilePath = file2Change[0].substr(0, sepPos + 1);
			addedFilePath += pathSuffix;
			bool isAdded = addInTree(rootPath, addedFilePath, nullptr, linarPathArray);
			if (!isAdded)
			{
				//MessageBox(NULL, addedFilePath.c_str(), TEXT("file/folder is not added"), MB_OK);
			}

			break;
		}

		case FB_RMFILE:
		{
			const std::vector<generic_string> file2Change = *(std::vector<generic_string> *)lParam;
			generic_string separator = TEXT("\\\\");

			size_t sepPos = file2Change[0].find(separator);
			if (sepPos == generic_string::npos)
				return false;

			generic_string pathSuffix = file2Change[0].substr(sepPos + separator.length(), file2Change[0].length() - 1);

			// remove prefix of file/folder in changeInfo, splite the remained path
			vector<generic_string> linarPathArray = split(pathSuffix, '\\');

			generic_string rootPath = file2Change[0].substr(0, sepPos);
			// search recursively and modify the tree structure

			bool isRemoved = deleteFromTree(rootPath, nullptr, linarPathArray);
			if (!isRemoved)
			{
				//MessageBox(NULL, file2Change[0].c_str(), TEXT("file/folder is not removed"), MB_OK);
			}
			break;
		}

		case FB_RNFILE:
		{
			const std::vector<generic_string> file2Change = *(std::vector<generic_string> *)lParam;
			generic_string separator = TEXT("\\\\");

			size_t sepPos = file2Change[0].find(separator);
			if (sepPos == generic_string::npos)
				return false;

			generic_string pathSuffix = file2Change[0].substr(sepPos + separator.length(), file2Change[0].length() - 1);

			// remove prefix of file/folder in changeInfo, splite the remained path
			vector<generic_string> linarPathArray = split(pathSuffix, '\\');

			generic_string rootPath = file2Change[0].substr(0, sepPos);

			size_t sepPos2 = file2Change[1].find(separator);
			if (sepPos2 == generic_string::npos)
				return false;

			generic_string pathSuffix2 = file2Change[1].substr(sepPos2 + separator.length(), file2Change[1].length() - 1);
			vector<generic_string> linarPathArray2 = split(pathSuffix2, '\\');

			bool isRenamed = renameInTree(rootPath, nullptr, linarPathArray, linarPathArray2[linarPathArray2.size() - 1]);
			if (!isRenamed)
			{
				//MessageBox(NULL, file2Change[0].c_str(), TEXT("file/folder is not removed"), MB_OK);
			}
			break;
		}

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void FileBrowser::initPopupMenus()
{
	NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();

	generic_string addRoot = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_ADDROOT, FB_ADDROOT);
	generic_string removeAllRoot = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_REMOVEALLROOTS, FB_REMOVEALLROOTS);
	generic_string removeRootFolder = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_REMOVEROOTFOLDER, FB_REMOVEROOTFOLDER);
	generic_string copyPath = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_COPYPATH, FB_COPYPATH);
	generic_string copyFileName = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_COPYFILENAME, FB_COPYFILENAME);
	generic_string findInFile = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_FINDINFILES, FB_FINDINFILES);
	generic_string explorerHere = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_EXPLORERHERE, FB_EXPLORERHERE);
	generic_string cmdHere = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_CMDHERE, FB_CMDHERE);
	generic_string openInNpp = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_OPENINNPP, FB_OPENINNPP);
	generic_string shellExecute = pNativeSpeaker->getFileBrowserLangMenuStr(IDM_FILEBROWSER_SHELLEXECUTE, FB_SHELLEXECUTE);

	_hGlobalMenu = ::CreatePopupMenu();
	::InsertMenu(_hGlobalMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_ADDROOT, addRoot.c_str());
	::InsertMenu(_hGlobalMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_REMOVEALLROOTS, removeAllRoot.c_str());

	_hRootMenu = ::CreatePopupMenu();
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_REMOVEROOTFOLDER, removeRootFolder.c_str());
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_COPYPATH, copyPath.c_str());
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_FINDINFILES, findInFile.c_str());
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_EXPLORERHERE, explorerHere.c_str());
	::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_CMDHERE, cmdHere.c_str());

	_hFolderMenu = ::CreatePopupMenu();
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_COPYPATH, copyPath.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_FINDINFILES, findInFile.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_EXPLORERHERE, explorerHere.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_CMDHERE, cmdHere.c_str());
	
	_hFileMenu = ::CreatePopupMenu();
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_OPENINNPP, openInNpp.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_COPYPATH, copyPath.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_COPYFILENAME, copyFileName.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_SHELLEXECUTE, shellExecute.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_EXPLORERHERE, explorerHere.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_FILEBROWSER_CMDHERE, cmdHere.c_str());
}

bool FileBrowser::selectItemFromPath(const generic_string& itemPath) const
{
	if (itemPath.empty())
		return false;

	size_t itemPathLen = itemPath.size();

	for (const auto f : _folderUpdaters)
	{
		if (isRelatedRootFolder(f->_rootFolder._rootPath, itemPath))
		{
			generic_string rootPath = f->_rootFolder._rootPath;
			size_t rootPathLen = rootPath.size();
			if (rootPathLen > itemPathLen) // It should never happen
				return false;

			vector<generic_string> linarPathArray;
			if (rootPathLen == itemPathLen)
			{
				// Do nothing and use empty linarPathArray
			}
			else
			{
				generic_string pathSuffix = itemPath.substr(rootPathLen + 1, itemPathLen - rootPathLen);
				linarPathArray = split(pathSuffix, '\\');
			}
			HTREEITEM foundItem = findInTree(rootPath, nullptr, linarPathArray);

			if (foundItem)
			{
				_treeView.selectItem(foundItem);
				_treeView.getFocus();
				return true;
			}
		}
	}
	return false;
}

bool FileBrowser::selectCurrentEditingFile() const
{
	TCHAR currentDocPath[MAX_PATH] = { '0' };
	::SendMessage(_hParent, NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(currentDocPath));
	generic_string currentDocPathStr = currentDocPath;

	return selectItemFromPath(currentDocPathStr);
}

BOOL FileBrowser::setImageList(int root_clean_id, int root_dirty_id, int open_node_id, int closed_node_id, int leaf_id) 
{
	HBITMAP hbmp;
	COLORREF maskColour = RGB(192, 192, 192);
	const int nbBitmaps = 5;

	// Creation of image list
	if ((_hImaLst = ImageList_Create(CX_BITMAP, CY_BITMAP, ILC_COLOR32 | ILC_MASK, nbBitmaps, 0)) == NULL) 
		return FALSE;

	// Add the bmp in the list
	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_clean_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(root_dirty_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(open_node_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(closed_node_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	hbmp = LoadBitmap(_hInst, MAKEINTRESOURCE(leaf_id));
	if (hbmp == NULL)
		return FALSE;
	ImageList_AddMasked(_hImaLst, hbmp, maskColour);
	DeleteObject(hbmp);

	if (ImageList_GetImageCount(_hImaLst) < nbBitmaps)
		return FALSE;

	// Set image list to the tree view
	TreeView_SetImageList(_treeView.getHSelf(), _hImaLst, TVSIL_NORMAL);

	return TRUE;
}


void FileBrowser::destroyMenus() 
{
	::DestroyMenu(_hGlobalMenu);
	::DestroyMenu(_hRootMenu);
	::DestroyMenu(_hFolderMenu);
	::DestroyMenu(_hFileMenu);
}

generic_string FileBrowser::getNodePath(HTREEITEM node) const
{
	if (!node) return TEXT("");

	vector<generic_string> fullPathArray;
	generic_string fullPath;

	// go up until to root, then get the full path
	HTREEITEM parent = node;
	for (; parent != nullptr;)
	{
		generic_string folderName = _treeView.getItemDisplayName(parent);
	
		HTREEITEM temp = _treeView.getParent(parent);
		if (temp == nullptr)
		{
			SortingData4lParam* customData = reinterpret_cast<SortingData4lParam*>(_treeView.getItemParam(parent));
			folderName = customData->_rootPath;
		}
		parent = temp;
		fullPathArray.push_back(folderName);
	}


	for (int i = int(fullPathArray.size()) - 1; i >= 0; --i)
	{
		fullPath += fullPathArray[i];
		if (i != 0)
			fullPath += TEXT("\\");
	}

	return fullPath;
}

generic_string FileBrowser::getNodeName(HTREEITEM node) const
{
	return node ? _treeView.getItemDisplayName(node) : TEXT("");
}

void FileBrowser::openSelectFile()
{
	// Get the selected item
	HTREEITEM selectedNode = _treeView.getSelection();
	if (!selectedNode) return;

	_selectedNodeFullPath = getNodePath(selectedNode);

	// test the path - if it's a file, open it, otherwise just fold or unfold it
	if (!::PathFileExists(_selectedNodeFullPath.c_str()))
		return;
	if (::PathIsDirectory(_selectedNodeFullPath.c_str()))
		return;

	::PostMessage(_hParent, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(_selectedNodeFullPath.c_str()));
}


void FileBrowser::notified(LPNMHDR notification)
{			
	if (notification->code == DMN_CLOSE)
	{
		::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_FILEBROWSER, 0);
	}
	else if (notification->code == TTN_GETDISPINFO)
	{
		LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)notification;
		lpttt->hinst = NULL;

		if (notification->idFrom == FB_CMD_AIMFILE)
		{
			wcscpy_s(lpttt->szText, _locateCurrentFile.c_str());
		}
		else if (notification->idFrom == FB_CMD_FOLDALL)
		{
			wcscpy_s(lpttt->szText, _collapseAllFolders.c_str());
		}
		else if (notification->idFrom == FB_CMD_EXPANDALL)
		{
			wcscpy_s(lpttt->szText, _expandAllFolders.c_str());
		}
	}
	else if ((notification->hwndFrom == _treeView.getHSelf()))
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
				openSelectFile();
			}
			break;

			case TVN_ENDLABELEDIT:
			{
				LPNMTVDISPINFO tvnotif = (LPNMTVDISPINFO)notification;
				if (!tvnotif->item.pszText)
					return;
				if (getNodeType(tvnotif->item.hItem) == browserNodeType_root)
					return;

				// Processing for only File case
				if (tvnotif->item.lParam) 
				{
					// Get the old label
					tvItem.hItem = _treeView.getSelection();
					::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
					size_t len = lstrlen(tvItem.pszText);

					// Find the position of old label in File path
					SortingData4lParam* customData = reinterpret_cast<SortingData4lParam*>(tvnotif->item.lParam);
					generic_string *filePath = &(customData->_rootPath);
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
						//TODO: remove it
					}
					TreeView_SetItem(_treeView.getHSelf(), &tvItem);
				}

				// For File, Folder and Project
				::SendMessage(_treeView.getHSelf(), TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&(tvnotif->item)));
			}
			break;

			case TVN_GETINFOTIP:
			{
				LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)notification;
				static generic_string tipStr;
				BrowserNodeType nType = getNodeType(lpGetInfoTip->hItem);
				if (nType == browserNodeType_root)
				{
					tipStr = *((generic_string *)lpGetInfoTip->lParam);
				}
				else if (nType == browserNodeType_file)
				{
					tipStr = getNodePath(lpGetInfoTip->hItem);
				}
				else
					return;
				lpGetInfoTip->pszText = (LPTSTR)tipStr.c_str();
				lpGetInfoTip->cchTextMax = static_cast<int>(tipStr.size());
			}
			break;

			case NM_RETURN:
				SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, 1);
			break;

			case TVN_KEYDOWN:
			{
				LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)notification;
				
				if (ptvkd->wVKey == VK_RETURN)
				{
					HTREEITEM hItem = _treeView.getSelection();
					BrowserNodeType nType = getNodeType(hItem);
					if (nType == browserNodeType_file)
						openSelectFile();
					else
						_treeView.toggleExpandCollapse(hItem);
				}
				else if (ptvkd->wVKey == VK_DELETE)
				{
					HTREEITEM hItem = _treeView.getSelection();
					BrowserNodeType nType = getNodeType(hItem);
					if (nType == browserNodeType_root)
						popupMenuCmd(IDM_FILEBROWSER_REMOVEROOTFOLDER);
				}
				/*
				else if (ptvkd->wVKey == VK_UP)
				{
					if (0x80 & GetKeyState(VK_CONTROL))
					{
						popupMenuCmd(IDM_FILEBROWSER_MOVEUP);
					}
				}
				else if (ptvkd->wVKey == VK_DOWN)
				{
					if (0x80 & GetKeyState(VK_CONTROL))
					{
						popupMenuCmd(IDM_FILEBROWSER_MOVEDOWN);
					}
				}
				else if (ptvkd->wVKey == VK_F2)
					popupMenuCmd(IDM_FILEBROWSER_RENAME);
				*/
			}
			break;

			case TVN_ITEMEXPANDED:
			{
				LPNMTREEVIEW nmtv = (LPNMTREEVIEW)notification;
				tvItem.hItem = nmtv->itemNew.hItem;
				tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

				if (getNodeType(nmtv->itemNew.hItem) == browserNodeType_folder)
				{
					if (nmtv->action == TVE_COLLAPSE)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_CLOSE_NODE, INDEX_CLOSE_NODE);
					}
					else if (nmtv->action == TVE_EXPAND)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
					}
				}
				else if (getNodeType(nmtv->itemNew.hItem) == browserNodeType_root)
				{
					if (nmtv->action == TVE_COLLAPSE)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_CLOSE_ROOT, INDEX_CLOSE_ROOT);
					}
					else if (nmtv->action == TVE_EXPAND)
					{
						_treeView.setItemImage(nmtv->itemNew.hItem, INDEX_OPEN_ROOT, INDEX_OPEN_ROOT);
					}
				}
			}
			break;

			case TVN_BEGINDRAG:
			{
				_treeView.beginDrag((LPNMTREEVIEW)notification);
				
			}
			break;
		}
	}
}

BrowserNodeType FileBrowser::getNodeType(HTREEITEM hItem)
{
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	// File
	if (tvItem.iImage == INDEX_LEAF)
	{
		return browserNodeType_file;
	}
	// Root
	else if (tvItem.lParam != NULL && !reinterpret_cast<SortingData4lParam*>(tvItem.lParam)->_rootPath.empty())
	{
		return browserNodeType_root;
	}
	// Folder
	else
	{
		return browserNodeType_folder;
	}
}

void FileBrowser::showContextMenu(int x, int y)
{
	TVHITTESTINFO tvHitInfo;
	HTREEITEM hTreeItem;

	// Detect if the given position is on the element TVITEM
	tvHitInfo.pt.x = x;
	tvHitInfo.pt.y = y;
	tvHitInfo.flags = 0;
	ScreenToClient(_treeView.getHSelf(), &(tvHitInfo.pt));
	hTreeItem = TreeView_HitTest(_treeView.getHSelf(), &tvHitInfo);

	if (tvHitInfo.hItem == nullptr)
	{
		TrackPopupMenu(_hGlobalMenu, TPM_LEFTALIGN, x, y, 0, _hSelf, NULL);
	}
	else
	{
		// Make item selected
		_treeView.selectItem(tvHitInfo.hItem);

		// get clicked item type
		BrowserNodeType nodeType = getNodeType(tvHitInfo.hItem);
		HMENU hMenu = NULL;
		if (nodeType == browserNodeType_root)
			hMenu = _hRootMenu;
		else if (nodeType == browserNodeType_folder)
			hMenu = _hFolderMenu;
		else //nodeType_file
			hMenu = _hFileMenu;

		TrackPopupMenu(hMenu, TPM_LEFTALIGN, x, y, 0, _hSelf, NULL);
	}
}

void FileBrowser::popupMenuCmd(int cmdID)
{
	// get selected item handle
	HTREEITEM selectedNode = _treeView.getSelection();

	switch (cmdID)
	{
		//
		// FileBrowser menu commands
		//
		case IDM_FILEBROWSER_REMOVEROOTFOLDER:
		{
			if (!selectedNode) return;

			generic_string *rootPath = (generic_string *)_treeView.getItemParam(selectedNode);
			if (_treeView.getParent(selectedNode) != nullptr || rootPath == nullptr)
				return;

			size_t nbFolderUpdaters = _folderUpdaters.size();
			for (size_t i = 0; i < nbFolderUpdaters; ++i)
			{
				if (_folderUpdaters[i]->_rootFolder._rootPath == *rootPath)
				{
					_folderUpdaters[i]->stopWatcher();
					_folderUpdaters.erase(_folderUpdaters.begin() + i);
					_treeView.removeItem(selectedNode);
					break;
				}
			}
		}
		break;
		
		case IDM_FILEBROWSER_EXPLORERHERE:
		{
			if (!selectedNode) return;

			generic_string path = getNodePath(selectedNode);
			if (::PathFileExists(path.c_str()))
			{
				TCHAR cmdStr[1024] = {};
				if (getNodeType(selectedNode) == browserNodeType_file)
					wsprintf(cmdStr, TEXT("explorer /select,\"%s\""), path.c_str());
				else
					wsprintf(cmdStr, TEXT("explorer \"%s\""), path.c_str());
				Command cmd(cmdStr);
				cmd.run(nullptr);
			}
		}
		break;

		case IDM_FILEBROWSER_CMDHERE:
		{
			if (!selectedNode) return;

			if (getNodeType(selectedNode) == browserNodeType_file)
				selectedNode = _treeView.getParent(selectedNode);

			generic_string path = getNodePath(selectedNode);
			if (::PathFileExists(path.c_str()))
			{
				Command cmd(NppParameters::getInstance().getNppGUI()._commandLineInterpreter.c_str());
				cmd.run(nullptr, path.c_str());
			}
		}
		break;

		case IDM_FILEBROWSER_COPYPATH:
		{
			if (!selectedNode) return;
			generic_string path = getNodePath(selectedNode);
			str2Clipboard(path, _hParent);
		}
		break;

		case IDM_FILEBROWSER_COPYFILENAME:
		{
			if (!selectedNode) return;
			generic_string fileName = getNodeName(selectedNode);
			str2Clipboard(fileName, _hParent);
		}
		break;

		case IDM_FILEBROWSER_FINDINFILES:
		{
			if (!selectedNode) return;
			generic_string path = getNodePath(selectedNode);
			::SendMessage(_hParent, NPPM_LAUNCHFINDINFILESDLG, reinterpret_cast<WPARAM>(path.c_str()), 0);
		}
		break;

		case IDM_FILEBROWSER_OPENINNPP:
		{
			openSelectFile();
		}
		break;

		case IDM_FILEBROWSER_REMOVEALLROOTS:
		{
			for (int i = static_cast<int>(_folderUpdaters.size()) - 1; i >= 0; --i)
			{
				_folderUpdaters[i]->stopWatcher();

				HTREEITEM root =  getRootFromFullPath(_folderUpdaters[i]->_rootFolder._rootPath);
				if (root)
					_treeView.removeItem(root);

				_folderUpdaters.erase(_folderUpdaters.begin() + i);
			}
		}
		break;

		case IDM_FILEBROWSER_ADDROOT:
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			generic_string openWorkspaceStr = pNativeSpeaker->getAttrNameStr(TEXT("Select a folder to add in Folder as Workspace panel"), FOLDERASWORKSPACE_NODE, "SelectFolderFromBrowserString");
			generic_string folderPath = folderBrowser(_hParent, openWorkspaceStr.c_str());
			if (!folderPath.empty())
			{
				addRootFolder(folderPath);
			}
		}
		break;

		case IDM_FILEBROWSER_SHELLEXECUTE:
		{
			if (!selectedNode) return;
			generic_string path = getNodePath(selectedNode);

			if (::PathFileExists(path.c_str()))
				::ShellExecute(NULL, TEXT("open"), path.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		break;
	}
}



void FileBrowser::getDirectoryStructure(const TCHAR *dir, const std::vector<generic_string> & patterns, FolderInfo & directoryStructure, bool isRecursive, bool isInHiddenDir)
{
	if (directoryStructure._parent == nullptr) // Root!
		directoryStructure.setRootPath(dir);

	generic_string dirFilter(dir);
	if (dirFilter[dirFilter.length() - 1] != '\\')
		dirFilter += TEXT("\\");
	dirFilter += TEXT("*.*");
	WIN32_FIND_DATA foundData;

	HANDLE hFile = ::FindFirstFile(dirFilter.c_str(), &foundData);

	if (hFile != INVALID_HANDLE_VALUE)
	{

		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// do nothing
			}
			else if (isRecursive)
			{
				if ((OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT(".")) != 0) && (OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT("..")) != 0))
				{
					generic_string pathDir(dir);
					if (pathDir[pathDir.length() - 1] != '\\')
						pathDir += TEXT("\\");
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");

					FolderInfo subDirectoryStructure(foundData.cFileName, &directoryStructure);
					getDirectoryStructure(pathDir.c_str(), patterns, subDirectoryStructure, isRecursive, isInHiddenDir);
					directoryStructure.addSubFolder(subDirectoryStructure);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				directoryStructure.addFile(foundData.cFileName);
			}
		}
	}

	while (::FindNextFile(hFile, &foundData))
	{
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// do nothing
			}
			else if (isRecursive)
			{
				if ((OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT(".")) != 0) && (OrdinalIgnoreCaseCompareStrings(foundData.cFileName, TEXT("..")) != 0))
				{
					generic_string pathDir(dir);
					if (pathDir[pathDir.length() - 1] != '\\')
						pathDir += TEXT("\\");
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");

					FolderInfo subDirectoryStructure(foundData.cFileName, &directoryStructure);
					getDirectoryStructure(pathDir.c_str(), patterns, subDirectoryStructure, isRecursive, isInHiddenDir);
					directoryStructure.addSubFolder(subDirectoryStructure);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				directoryStructure.addFile(foundData.cFileName);
			}
		}
	}
	::FindClose(hFile);
}

void FileBrowser::addRootFolder(generic_string rootFolderPath)
{
	if (!::PathFileExists(rootFolderPath.c_str()))
		return;

	if (!::PathIsDirectory(rootFolderPath.c_str()))
		return;

	// make sure there's no '\' at the end
	if (rootFolderPath[rootFolderPath.length() - 1] == '\\')
	{
		rootFolderPath = rootFolderPath.substr(0, rootFolderPath.length() - 1);
	}

	for (const auto f : _folderUpdaters)
	{
		if (f->_rootFolder._rootPath == rootFolderPath)
			return;
		else
		{
			if (isRelatedRootFolder(f->_rootFolder._rootPath, rootFolderPath))
			{
				//do nothing, go down to select the dir
				generic_string rootPath = f->_rootFolder._rootPath;
				generic_string pathSuffix = rootFolderPath.substr(rootPath.size() + 1, rootFolderPath.size() - rootPath.size());
				vector<generic_string> linarPathArray = split(pathSuffix, '\\');
				
				HTREEITEM foundItem = findInTree(rootPath, nullptr, linarPathArray);
				if (foundItem)
					_treeView.selectItem(foundItem);
				return;
			}
			
			if (isRelatedRootFolder(rootFolderPath, f->_rootFolder._rootPath))
			{
				NppParameters::getInstance().getNativeLangSpeaker()->messageBox("FolderAsWorspaceSubfolderExists",
					_hParent,
					TEXT("A sub-folder of the folder you want to add exists.\rPlease remove its root from the panel before you add folder \"$STR_REPLACE$\"."),
					TEXT("Folder as Workspace adding folder problem"),
					MB_OK,
					0, // not used
					rootFolderPath.c_str());
				return;
			}
		}
	}

	std::vector<generic_string> patterns2Match;
 	patterns2Match.push_back(TEXT("*.*"));

	TCHAR *label = ::PathFindFileName(rootFolderPath.c_str());
	TCHAR rootLabel[MAX_PATH];
	wcscpy_s(rootLabel, label);
	size_t len = lstrlen(rootLabel);
	if (rootLabel[len - 1] == '\\')
		rootLabel[len - 1] = '\0';

	FolderInfo directoryStructure(rootLabel, nullptr);
	getDirectoryStructure(rootFolderPath.c_str(), patterns2Match, directoryStructure, true, false);
	HTREEITEM hRootItem = createFolderItemsFromDirStruct(nullptr, directoryStructure);
	_treeView.expand(hRootItem);
	_folderUpdaters.push_back(new FolderUpdater(directoryStructure, this));
	_folderUpdaters[_folderUpdaters.size() - 1]->startWatcher();
}

HTREEITEM FileBrowser::createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure)
{
	HTREEITEM hFolderItem = nullptr;
	if (directoryStructure._parent == nullptr && hParentItem == nullptr)
	{
		TCHAR rootPath[MAX_PATH];
		wcscpy_s(rootPath, directoryStructure._rootPath.c_str());
		size_t len = lstrlen(rootPath);
		if (rootPath[len - 1] == '\\')
			rootPath[len - 1] = '\0';

		SortingData4lParam* customData = new SortingData4lParam(rootPath, TEXT(""), true);
		sortingDataArray.push_back(customData);

		hFolderItem = _treeView.addItem(directoryStructure._name.c_str(), TVI_ROOT, INDEX_CLOSE_ROOT, reinterpret_cast<LPARAM>(customData));
	}
	else
	{
		SortingData4lParam* customData = new SortingData4lParam(TEXT(""), directoryStructure._name, true);
		sortingDataArray.push_back(customData);

		hFolderItem = _treeView.addItem(directoryStructure._name.c_str(), hParentItem, INDEX_CLOSE_NODE, reinterpret_cast<LPARAM>(customData));
	}

	for (const auto& folder : directoryStructure._subFolders)
	{
		createFolderItemsFromDirStruct(hFolderItem, folder);
	}

	for (const auto& file : directoryStructure._files)
	{
		SortingData4lParam* customData = new SortingData4lParam(TEXT(""), file._name, false);
		sortingDataArray.push_back(customData);

		_treeView.addItem(file._name.c_str(), hFolderItem, INDEX_LEAF, reinterpret_cast<LPARAM>(customData));
	}

	_treeView.fold(hParentItem);

	return hFolderItem;
}

HTREEITEM FileBrowser::getRootFromFullPath(const generic_string & rootPath) const
{
	HTREEITEM node = nullptr;
	for (HTREEITEM hItemNode = _treeView.getRoot();
		hItemNode != nullptr && node == nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		TVITEM tvItem;
		tvItem.mask = TVIF_PARAM;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		if (tvItem.lParam != 0 && rootPath == reinterpret_cast<SortingData4lParam *>(tvItem.lParam)->_rootPath)
			node = hItemNode;
	}
	return node;
}

HTREEITEM FileBrowser::findChildNodeFromName(HTREEITEM parent, const generic_string& label) const
{
	HTREEITEM childNodeFound = nullptr;

	for (HTREEITEM hItemNode = _treeView.getChildFrom(parent);
		hItemNode != NULL && childNodeFound == nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		TCHAR textBuffer[MAX_PATH];
		TVITEM tvItem;
		tvItem.mask = TVIF_TEXT;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		if (label == tvItem.pszText)
		{
			childNodeFound = hItemNode;
		}
	}
	return childNodeFound;
}

vector<generic_string> FileBrowser::getRoots() const
{
	vector<generic_string> roots;

	for (HTREEITEM hItemNode = _treeView.getRoot();
		hItemNode != nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		TVITEM tvItem;
		tvItem.mask = TVIF_PARAM;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		roots.push_back(reinterpret_cast<SortingData4lParam*>(tvItem.lParam)->_rootPath);
	}
	return roots;
}

generic_string FileBrowser::getSelectedItemPath() const
{
	generic_string itemPath;
	HTREEITEM hItemNode = _treeView.getSelection();
	if (hItemNode)
	{
		itemPath = getNodePath(hItemNode);
	}
	return itemPath;
}

bool FileBrowser::addInTree(const generic_string& rootPath, const generic_string& addItemFullPath, HTREEITEM node, vector<generic_string> linarPathArray)
{
	if (node == nullptr) // it's a root. Search the right root with rootPath
	{
		// Search
		if ((node = getRootFromFullPath(rootPath)) == nullptr)
			return false;
	}

	if (linarPathArray.size() == 1)
	{
		// Of course item to add should be exist on the disk
		if (!::PathFileExists(addItemFullPath.c_str()))
			return false;

		// Search : if no found, add
		HTREEITEM childNodeFound = findChildNodeFromName(node, linarPathArray[0]);
		if (childNodeFound != nullptr)
			return false;

		// No found, good - Action
		if (::PathIsDirectory(addItemFullPath.c_str()))
		{
			SortingData4lParam* customData = new SortingData4lParam(TEXT(""), linarPathArray[0], true);
			sortingDataArray.push_back(customData);

			_treeView.addItem(linarPathArray[0].c_str(), node, INDEX_CLOSE_NODE, reinterpret_cast<LPARAM>(customData));
		}
		else
		{
			SortingData4lParam* customData = new SortingData4lParam(TEXT(""), linarPathArray[0], false);
			sortingDataArray.push_back(customData);

			_treeView.addItem(linarPathArray[0].c_str(), node, INDEX_LEAF, reinterpret_cast<LPARAM>(customData));
		}

		_treeView.customSorting(node, categorySortFunc, 0);
		return true;
	}
	else
	{
		for (HTREEITEM hItemNode = _treeView.getChildFrom(node);
			hItemNode != NULL ;
			hItemNode = _treeView.getNextSibling(hItemNode))
		{
			TCHAR textBuffer[MAX_PATH];
			TVITEM tvItem;
			tvItem.mask = TVIF_TEXT;
			tvItem.pszText = textBuffer;
			tvItem.cchTextMax = MAX_PATH;
			tvItem.hItem = hItemNode;
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

			if (linarPathArray[0] == tvItem.pszText)
			{
				// search recursively the node for an action
				linarPathArray.erase(linarPathArray.begin());
				return addInTree(rootPath, addItemFullPath, hItemNode, linarPathArray);
			}
		}
		return false;
	}
}

HTREEITEM FileBrowser::findInTree(const generic_string& rootPath, HTREEITEM node, std::vector<generic_string> linarPathArray) const
{
	if (node == nullptr) // it's a root. Search the right root with rootPath
	{
		// Search
		if ((node = getRootFromFullPath(rootPath)) == nullptr)
			return nullptr;
	}

	if (linarPathArray.empty()) // nothing to search, return node
	{
		return node;
	}
	else if (linarPathArray.size() == 1)
	{
		// Search
		return findChildNodeFromName(node, linarPathArray[0]);
	}
	else
	{
		for (HTREEITEM hItemNode = _treeView.getChildFrom(node);
			hItemNode != NULL;
			hItemNode = _treeView.getNextSibling(hItemNode))
		{
			TCHAR textBuffer[MAX_PATH];
			TVITEM tvItem;
			tvItem.mask = TVIF_TEXT;
			tvItem.pszText = textBuffer;
			tvItem.cchTextMax = MAX_PATH;
			tvItem.hItem = hItemNode;
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

			if (linarPathArray[0] == tvItem.pszText)
			{
				// search recursively the node for an action
				linarPathArray.erase(linarPathArray.begin());
				return findInTree(rootPath, hItemNode, linarPathArray);
			}
		}
		return nullptr;
	}
}

bool FileBrowser::deleteFromTree(const generic_string& rootPath, HTREEITEM node, const std::vector<generic_string>& linarPathArray)
{
	HTREEITEM foundItem = findInTree(rootPath, node, linarPathArray);
	if (foundItem == nullptr)
			return false;

	// found it, delete it
	_treeView.removeItem(foundItem);
	return true;
}

bool FileBrowser::renameInTree(const generic_string& rootPath, HTREEITEM node, const std::vector<generic_string>& linarPathArrayFrom, const generic_string & renameTo)
{
	HTREEITEM foundItem = findInTree(rootPath, node, linarPathArrayFrom);
	if (foundItem == nullptr)
			return false;

	// found it, rename it
	_treeView.renameItem(foundItem, renameTo.c_str());
	SortingData4lParam* compareData = reinterpret_cast<SortingData4lParam*>(_treeView.getItemParam(foundItem));
	compareData->_label = renameTo;
	_treeView.customSorting(_treeView.getParent(foundItem), categorySortFunc, 0);

	return true;
}

int CALLBACK FileBrowser::categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	SortingData4lParam* item1 = reinterpret_cast<SortingData4lParam*>(lParam1);
	SortingData4lParam* item2 = reinterpret_cast<SortingData4lParam*>(lParam2);

	if (!item1 || !item2)
		return 0;

	if (item1->_isFolder && !item2->_isFolder)
		return -1;
	else if (!item1->_isFolder && item2->_isFolder)
		return 1;
	else
		return lstrcmpi(item1->_label.c_str(), item2->_label.c_str());
}

bool FolderInfo::addToStructure(generic_string & fullpath, std::vector<generic_string> linarPathArray)
{
	if (linarPathArray.size() == 1) // could be file or folder
	{
		fullpath += TEXT("\\");
		fullpath += linarPathArray[0];
		if (PathIsDirectory(fullpath.c_str()))
		{
			// search in folders, if found - no good
			for (const auto& folder : _subFolders)
			{
				if (linarPathArray[0] == folder.getName())
					return false; // Maybe already added?
			}
			_subFolders.push_back(FolderInfo(linarPathArray[0], this));
			return true;
		}
		else
		{
			// search in files, if found - no good
			for (const auto& file : _files)
			{
				if (linarPathArray[0] == file.getName())
					return false; // Maybe already added?
			}
			_files.push_back(FileInfo(linarPathArray[0], this));
			return true;
		}	
	}
	else // folder
	{
		for (auto& folder : _subFolders)
		{
			if (folder.getName() == linarPathArray[0])
			{
				fullpath += TEXT("\\");
				fullpath += linarPathArray[0];
				linarPathArray.erase(linarPathArray.begin());
				return folder.addToStructure(fullpath, linarPathArray);
			}
		}
		return false;
	}
}

bool FolderInfo::removeFromStructure(std::vector<generic_string> linarPathArray)
{
	if (linarPathArray.size() == 1) // could be file or folder
	{
		for (size_t i = 0; i < _files.size(); ++i)
		{
			if (_files[i].getName() == linarPathArray[0])
			{
				// remove this file
				_files.erase(_files.begin() + i);
				return true;
			}
		}

		for (size_t i = 0; i < _subFolders.size(); ++i)
		{
			if (_subFolders[i].getName() == linarPathArray[0])
			{
				// remove this folder
				_subFolders.erase(_subFolders.begin() + i);
				return true;
			}
		}
	}
	else // folder
	{
		for (size_t i = 0; i < _subFolders.size(); ++i)
		{
			if (_subFolders[i].getName() == linarPathArray[0])
			{
				linarPathArray.erase(linarPathArray.begin());
				return _subFolders[i].removeFromStructure(linarPathArray);
			}
		}
	}
	return false;
}

bool FolderInfo::renameInStructure(std::vector<generic_string> linarPathArrayFrom, std::vector<generic_string> linarPathArrayTo)
{
	if (linarPathArrayFrom.size() == 1) // could be file or folder
	{
		for (auto& file : _files)
		{
			if (file.getName() == linarPathArrayFrom[0])
			{
				// rename this file
				file.setName(linarPathArrayTo[0]);
				return true;
			}
		}

		for (auto& folder : _subFolders)
		{
			if (folder.getName() == linarPathArrayFrom[0])
			{
				// rename this folder
				folder.setName(linarPathArrayTo[0]);
				return true;
			}
		}
		return false;
	}
	else // folder
	{
		for (auto& folder : _subFolders)
		{
			if (folder.getName() == linarPathArrayFrom[0])
			{
				linarPathArrayFrom.erase(linarPathArrayFrom.begin());
				linarPathArrayTo.erase(linarPathArrayTo.begin());
				return folder.renameInStructure(linarPathArrayFrom, linarPathArrayTo);
			}
		}
		return false;
	}
}

void FolderUpdater::startWatcher()
{
	// no thread yet, create a event with non-signaled, to block all threads
	_EventHandle = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	_watchThreadHandle = ::CreateThread(NULL, 0, watching, this, 0, NULL);
}

void FolderUpdater::stopWatcher()
{
	::SetEvent(_EventHandle);
	::CloseHandle(_watchThreadHandle);
	::CloseHandle(_EventHandle);
}

LPCWSTR explainAction(DWORD dwAction)
{
	switch (dwAction)
	{
	case FILE_ACTION_ADDED:
		return L"Added";
	case FILE_ACTION_REMOVED:
		return L"Deleted";
	case FILE_ACTION_MODIFIED:
		return L"Modified";
	case FILE_ACTION_RENAMED_OLD_NAME:
		return L"Renamed From";
	case FILE_ACTION_RENAMED_NEW_NAME:
		return L"Renamed ";
	default:
		return L"BAD DATA";
	}
};


DWORD WINAPI FolderUpdater::watching(void *params)
{
	FolderUpdater *thisFolderUpdater = (FolderUpdater *)params;

	generic_string dir2Watch = (thisFolderUpdater->_rootFolder)._rootPath;
	if (dir2Watch[dir2Watch.length() - 1] != '\\')
		dir2Watch += TEXT("\\"); // CReadDirectoryChanges will add another '\' so we will get "\\" as a separator (of monitored root) in the notification

	const DWORD dwNotificationFlags = FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME;

	// Create the monitor and add directory to watch.
	CReadDirectoryChanges changes;
	changes.AddDirectory(dir2Watch.c_str(), true, dwNotificationFlags);

	HANDLE changeHandles[] = { thisFolderUpdater->_EventHandle, changes.GetWaitHandle() };

	bool toBeContinued = true;

	while (toBeContinued)
	{
		DWORD waitStatus = ::WaitForMultipleObjects(_countof(changeHandles), changeHandles, FALSE, INFINITE);
		switch (waitStatus)
		{
			case WAIT_OBJECT_0 + 0:
			// Mutex was signaled. User removes this folder or file browser is closed
				toBeContinued = false;
				break;

			case WAIT_OBJECT_0 + 1:
			// We've received a notification in the queue.
			{
				DWORD dwAction;
				generic_string wstrFilename;
				// Process all available changes, ignore User actions
				while (changes.Pop(dwAction, wstrFilename))
				{
					static generic_string oldName;
					static std::vector<generic_string> file2Change;
					file2Change.clear();

					switch (dwAction)
					{
						case FILE_ACTION_ADDED:
							file2Change.push_back(wstrFilename);
							::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_ADDFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&file2Change));
							oldName = TEXT("");
							break;

						case FILE_ACTION_REMOVED:
							file2Change.push_back(wstrFilename);
							::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_RMFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&file2Change));
							oldName = TEXT("");
							break;

						case FILE_ACTION_MODIFIED:
							oldName = TEXT("");
							break;

						case FILE_ACTION_RENAMED_OLD_NAME:
							oldName = wstrFilename;
							break;

						case FILE_ACTION_RENAMED_NEW_NAME:
							if (!oldName.empty())
							{
								file2Change.push_back(oldName);
								file2Change.push_back(wstrFilename);
								//thisFolderUpdater->updateTree(dwAction, file2Change);
								::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_RNFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&file2Change));
							}
							oldName = TEXT("");
							break;

						default:
							oldName = TEXT("");
							break;
					}
				}
			}
			break;

			case WAIT_IO_COMPLETION:
				// Nothing to do.
				break;
		}
	}

	// Just for sample purposes. The destructor will
	// call Terminate() automatically.
	changes.Terminate();
	//printStr(L"Quit watching thread");
	return EXIT_SUCCESS;
}
