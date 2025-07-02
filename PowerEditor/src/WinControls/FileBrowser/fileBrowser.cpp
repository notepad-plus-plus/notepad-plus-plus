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


#include "fileBrowser.h"
#include "resource.h"
#include "tinyxml.h"
#include "localization.h"
#include "Parameters.h"
#include "RunDlg.h"
#include "ReadDirectoryChanges.h"
#include "menuCmdID.h"

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

	for (auto hImgList : _iconListVector)
	{
		if (hImgList != nullptr)
		{
			::ImageList_Destroy(hImgList);
		}
	}
	_iconListVector.clear();
}

vector<wstring> split(const wstring & string2split, wchar_t sep)
{
	vector<wstring> splitedStrings;
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
}

bool isRelatedRootFolder(const wstring & relatedRoot, const wstring & subFolder)
{
	if (relatedRoot.empty())
		return false;

	if (subFolder.empty())
		return false;

	size_t pos = subFolder.find(relatedRoot);
	if (pos != 0) // pos == 0 is the necessary condition, but not enough
		return false;

	vector<wstring> relatedRootArray = split(relatedRoot, '\\');
	vector<wstring> subFolderArray = split(subFolder, '\\');

	size_t index2Compare = relatedRootArray.size() - 1;

	return relatedRootArray[index2Compare] == subFolderArray[index2Compare];
}

intptr_t CALLBACK FileBrowser::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			constexpr DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_CUSTOMERASE;
			_hToolbarMenu = CreateWindowEx(WS_EX_LAYOUTRTL, TOOLBARCLASSNAME, NULL, style, 0, 0, 0, 0, _hSelf, nullptr, _hInst, NULL);

			const DWORD tbExStyle = static_cast<DWORD>(::SendMessage(_hToolbarMenu, TB_GETEXTENDEDSTYLE, 0, 0));
			::SendMessage(_hToolbarMenu, TB_SETEXTENDEDSTYLE, 0, tbExStyle | TBSTYLE_EX_DOUBLEBUFFER);

			setDpi();
			const int iconSizeDyn = _dpiManager.scale(16);
			constexpr int nbIcons = 3;
			int iconIDs[nbIcons] = { IDI_FB_SELECTCURRENTFILE, IDI_FB_FOLDALL, IDI_FB_EXPANDALL};
			int iconDarkModeIDs[nbIcons] = { IDI_FB_SELECTCURRENTFILE_DM, IDI_FB_FOLDALL_DM, IDI_FB_EXPANDALL_DM};

			// Create an image lists for the toolbar icons
			HIMAGELIST hImageList = ImageList_Create(iconSizeDyn, iconSizeDyn, ILC_COLOR32 | ILC_MASK, nbIcons, 0);
			HIMAGELIST hImageListDm = ImageList_Create(iconSizeDyn, iconSizeDyn, ILC_COLOR32 | ILC_MASK, nbIcons, 0);
			_iconListVector.push_back(hImageList);
			_iconListVector.push_back(hImageListDm);

			for (size_t i = 0; i < nbIcons; ++i)
			{
				int icoID = iconIDs[i];
				HICON hIcon = nullptr;
				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSizeDyn, iconSizeDyn, &hIcon, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
				ImageList_AddIcon(_iconListVector.at(0), hIcon);
				::DestroyIcon(hIcon);
				hIcon = nullptr;

				icoID = iconDarkModeIDs[i];
				DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(icoID), iconSizeDyn, iconSizeDyn, &hIcon, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
				ImageList_AddIcon(_iconListVector.at(1), hIcon);
				::DestroyIcon(hIcon); // Clean up the loaded icon
			}

			// Attach the image list to the toolbar
			::SendMessage(_hToolbarMenu, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(_iconListVector.at(NppDarkMode::isEnabled() ? 1 : 0)));

			TBBUTTON tbButtons[nbIcons]{};

			tbButtons[0].idCommand = FB_CMD_AIMFILE;
			tbButtons[0].iBitmap = 0;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_BUTTON;
			tbButtons[0].iString = 0;

			tbButtons[1].idCommand = FB_CMD_FOLDALL;
			tbButtons[1].iBitmap = 1;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_BUTTON;
			tbButtons[1].iString = 0;

			tbButtons[2].idCommand = FB_CMD_EXPANDALL;
			tbButtons[2].iBitmap = 2;
			tbButtons[2].fsState = TBSTATE_ENABLED;
			tbButtons[2].fsStyle = BTNS_BUTTON;
			tbButtons[2].iString = 0;

			::SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			::SendMessage(_hToolbarMenu, TB_SETBUTTONSIZE, 0, MAKELONG(iconSizeDyn, iconSizeDyn));
			::SendMessage(_hToolbarMenu, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(&tbButtons));
			::SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0);
			
			ShowWindow(_hToolbarMenu, SW_SHOW);

			reloadLang();

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_FB_ROOTOPEN, IDI_FB_ROOTCLOSE, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE }
				, { IDI_FB_ROOTOPEN_DM, IDI_FB_ROOTCLOSE_DM, IDI_PROJECT_FOLDEROPEN_DM, IDI_PROJECT_FOLDERCLOSE_DM, IDI_PROJECT_FILE_DM }
				, { IDI_FB_ROOTOPEN2, IDI_FB_ROOTCLOSE2, IDI_PROJECT_FOLDEROPEN2, IDI_PROJECT_FOLDERCLOSE2, IDI_PROJECT_FILE2 }
			);

			_treeView.init(_hInst, _hSelf, ID_FILEBROWSERTREEVIEW);
			_treeView.setImageList(imgIds);

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

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			if (static_cast<BOOL>(lParam) != TRUE)
			{
				NppDarkMode::autoThemeChildControls(_hSelf);
				::SendMessage(_hToolbarMenu, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(_iconListVector.at(NppDarkMode::isEnabled() ? 1 : 0)));
			}
			else
			{
				NppDarkMode::setTreeViewStyle(_treeView.getHSelf());
			}

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_FB_ROOTOPEN, IDI_FB_ROOTCLOSE, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE }
				, { IDI_FB_ROOTOPEN_DM, IDI_FB_ROOTCLOSE_DM, IDI_PROJECT_FOLDEROPEN_DM, IDI_PROJECT_FOLDERCLOSE_DM, IDI_PROJECT_FILE_DM }
				, { IDI_FB_ROOTOPEN2, IDI_FB_ROOTCLOSE2, IDI_PROJECT_FOLDEROPEN2, IDI_PROJECT_FOLDERCLOSE2, IDI_PROJECT_FILE2 }
			);

			_treeView.setImageList(imgIds);

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
			int extraValue = _dpiManager.scale(4);

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

			std::vector<FilesToChange> groupedFiles = getFilesFromParam(lParam);

			for (auto & group : groupedFiles) 
			{
				addToTree(group, nullptr);
			}

			break;
		}

		case FB_RMFILE:
		{

			std::vector<FilesToChange> groupedFiles = getFilesFromParam(lParam);

			for (auto & group : groupedFiles) 
			{
				deleteFromTree(group);
			}

			break;
		}

		case FB_RNFILE:
		{
			const std::vector<wstring> file2Change = *(std::vector<wstring> *)lParam;
			wstring separator = L"\\\\";

			size_t sepPos = file2Change[0].find(separator);
			if (sepPos == wstring::npos)
				return false;

			wstring pathSuffix = file2Change[0].substr(sepPos + separator.length(), file2Change[0].length() - 1);

			// remove prefix of file/folder in changeInfo, splite the remained path
			vector<wstring> linarPathArray = split(pathSuffix, '\\');

			wstring rootPath = file2Change[0].substr(0, sepPos);

			size_t sepPos2 = file2Change[1].find(separator);
			if (sepPos2 == wstring::npos)
				return false;

			wstring pathSuffix2 = file2Change[1].substr(sepPos2 + separator.length(), file2Change[1].length() - 1);
			vector<wstring> linarPathArray2 = split(pathSuffix2, '\\');

			bool isRenamed = renameInTree(rootPath, nullptr, linarPathArray, linarPathArray2[linarPathArray2.size() - 1]);
			if (!isRenamed)
			{
				//MessageBox(NULL, file2Change[0].c_str(), L"file/folder is not removed", MB_OK);
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

	wstring addRoot = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_ADDROOT, FB_ADDROOT);
	wstring removeAllRoot = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_REMOVEALLROOTS, FB_REMOVEALLROOTS);
	wstring removeRootFolder = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_REMOVEROOTFOLDER, FB_REMOVEROOTFOLDER);
	wstring copyPath = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_COPYPATH, FB_COPYPATH);
	wstring copyFileName = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_COPYFILENAME, FB_COPYFILENAME);
	wstring findInFile = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_FINDINFILES, FB_FINDINFILES);
	wstring explorerHere = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_EXPLORERHERE, FB_EXPLORERHERE);
	wstring cmdHere = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_CMDHERE, FB_CMDHERE);
	wstring openInNpp = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_OPENINNPP, FB_OPENINNPP);
	wstring shellExecute = pNativeSpeaker->getDlgLangMenuStr(FOLDERASWORKSPACE_NODE, nullptr, IDM_FILEBROWSER_SHELLEXECUTE, FB_SHELLEXECUTE);

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

bool FileBrowser::selectItemFromPath(const wstring& itemPath) const
{
	if (itemPath.empty())
		return false;

	size_t itemPathLen = itemPath.size();

	for (const auto f : _folderUpdaters)
	{
		if (isRelatedRootFolder(f->_rootFolder._rootPath, itemPath))
		{
			wstring rootPath = f->_rootFolder._rootPath;
			size_t rootPathLen = rootPath.size();
			if (rootPathLen > itemPathLen) // It should never happen
				return false;

			vector<wstring> linarPathArray;
			if (rootPathLen == itemPathLen)
			{
				// Do nothing and use empty linarPathArray
			}
			else
			{
				wstring pathSuffix = itemPath.substr(rootPathLen + 1, itemPathLen - rootPathLen);
				linarPathArray = split(pathSuffix, '\\');
			}
			HTREEITEM foundItem = findInTree(rootPath, nullptr, linarPathArray);

			if (foundItem)
			{
				_treeView.selectItem(foundItem);
				_treeView.grabFocus();
				return true;
			}
		}
	}
	return false;
}

bool FileBrowser::selectCurrentEditingFile() const
{
	wchar_t currentDocPath[MAX_PATH] = { '\0' };
	::SendMessage(_hParent, NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(currentDocPath));
	wstring currentDocPathStr = currentDocPath;

	return selectItemFromPath(currentDocPathStr);
}

void FileBrowser::destroyMenus() 
{
	if (::IsMenu(_hGlobalMenu))
		::DestroyMenu(_hGlobalMenu);

	if (::IsMenu(_hRootMenu))
		::DestroyMenu(_hRootMenu);

	if (::IsMenu(_hFolderMenu))
		::DestroyMenu(_hFolderMenu);

	if (::IsMenu(_hFileMenu))
		::DestroyMenu(_hFileMenu);
}

wstring FileBrowser::getNodePath(HTREEITEM node) const
{
	if (!node) return L"";

	vector<wstring> fullPathArray;
	wstring fullPath;

	// go up until to root, then get the full path
	HTREEITEM parent = node;
	for (; parent != nullptr;)
	{
		wstring folderName = _treeView.getItemDisplayName(parent);
	
		HTREEITEM temp = _treeView.getParent(parent);
		if (temp == nullptr)
		{
			const SortingData4lParam* customData = reinterpret_cast<SortingData4lParam*>(_treeView.getItemParam(parent));
			folderName = customData->_rootPath;
		}
		parent = temp;
		fullPathArray.push_back(folderName);
	}


	for (int i = int(fullPathArray.size()) - 1; i >= 0; --i)
	{
		fullPath += fullPathArray[i];
		if (i != 0)
			fullPath += L"\\";
	}

	return fullPath;
}

wstring FileBrowser::getNodeName(HTREEITEM node) const
{
	return node ? _treeView.getItemDisplayName(node) : L"";
}

void FileBrowser::openSelectFile()
{
	// Get the selected item
	HTREEITEM selectedNode = _treeView.getSelection();
	if (!selectedNode) return;

	_selectedNodeFullPath = getNodePath(selectedNode);

	// test the path - if it's a file, open it, otherwise just fold or unfold it
	if (!doesFileExist(_selectedNodeFullPath.c_str()))
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
	else if (notification->hwndFrom == _treeView.getHSelf())
	{
		wchar_t textBuffer[MAX_PATH] = { '\0' };
		TVITEM tvItem{};
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
					wstring *filePath = &(customData->_rootPath);
					size_t found = filePath->rfind(tvItem.pszText);

					// If found the old label, replace it with the modified one
					if (found != wstring::npos)
						filePath->replace(found, len, tvnotif->item.pszText);

					// Check the validity of modified file path
					tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					if (doesPathExist(filePath->c_str()))
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
				static wstring tipStr;
				BrowserNodeType nType = getNodeType(lpGetInfoTip->hItem);
				if (nType == browserNodeType_root)
				{
					tipStr = *((wstring *)lpGetInfoTip->lParam);
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
	TVITEM tvItem{};
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	// File
	if (tvItem.iImage == INDEX_LEAF)
	{
		return browserNodeType_file;
	}
	// Root
	else if (tvItem.lParam && !reinterpret_cast<SortingData4lParam*>(tvItem.lParam)->_rootPath.empty())
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
	TVHITTESTINFO tvHitInfo{};

	// Detect if the given position is on the element TVITEM
	tvHitInfo.pt.x = x;
	tvHitInfo.pt.y = y;
	tvHitInfo.flags = 0;
	ScreenToClient(_treeView.getHSelf(), &(tvHitInfo.pt));
	TreeView_HitTest(_treeView.getHSelf(), &tvHitInfo);

	if (tvHitInfo.hItem == nullptr)
	{
		TrackPopupMenu(_hGlobalMenu, 
			NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
			x, y, 0, _hSelf, NULL);
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

		TrackPopupMenu(hMenu, 
			NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
			x, y, 0, _hSelf, NULL);
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

			const wstring* rootPath = (wstring *)_treeView.getItemParam(selectedNode);
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

			wstring path = getNodePath(selectedNode);
			if (doesPathExist(path.c_str()))
			{
				wchar_t cmdStr[1024] = {};
				if (getNodeType(selectedNode) == browserNodeType_file)
					wsprintf(cmdStr, L"explorer /select,\"%s\"", path.c_str());
				else
					wsprintf(cmdStr, L"explorer \"%s\"", path.c_str());
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

			wstring path = getNodePath(selectedNode);
			if (doesPathExist(path.c_str()))
			{
				Command cmd(NppParameters::getInstance().getNppGUI()._commandLineInterpreter.c_str());
				cmd.run(nullptr, path.c_str());
			}
		}
		break;

		case IDM_FILEBROWSER_COPYPATH:
		{
			if (!selectedNode) return;
			wstring path = getNodePath(selectedNode);
			str2Clipboard(path, _hParent);
		}
		break;

		case IDM_FILEBROWSER_COPYFILENAME:
		{
			if (!selectedNode) return;
			wstring fileName = getNodeName(selectedNode);
			str2Clipboard(fileName, _hParent);
		}
		break;

		case IDM_FILEBROWSER_FINDINFILES:
		{
			if (!selectedNode) return;
			wstring path = getNodePath(selectedNode);
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
			wstring openWorkspaceStr = pNativeSpeaker->getAttrNameStr(L"Select a folder to add in Folder as Workspace panel", FOLDERASWORKSPACE_NODE, "SelectFolderFromBrowserString");
			wstring folderPath = folderBrowser(_hParent, openWorkspaceStr);
			if (!folderPath.empty())
			{
				addRootFolder(folderPath);
			}
		}
		break;

		case IDM_FILEBROWSER_SHELLEXECUTE:
		{
			if (!selectedNode) return;
			wstring path = getNodePath(selectedNode);

			if (doesPathExist(path.c_str()))
				::ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		break;
	}
}



void FileBrowser::getDirectoryStructure(const wchar_t *dir, const std::vector<wstring> & patterns, FolderInfo & directoryStructure, bool isRecursive, bool isInHiddenDir)
{
	if (directoryStructure._parent == nullptr) // Root!
		directoryStructure.setRootPath(dir);

	wstring dirFilter(dir);
	if (dirFilter[dirFilter.length() - 1] != '\\')
		dirFilter += L"\\";
	dirFilter += L"*.*";
	WIN32_FIND_DATA foundData;

	HANDLE hFindFile = ::FindFirstFile(dirFilter.c_str(), &foundData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
				{
					// do nothing
				}
				else if (isRecursive)
				{
					if ((wcscmp(foundData.cFileName, L".") != 0) && 
						(wcscmp(foundData.cFileName, L"..") != 0))
					{
						wstring pathDir(dir);
						if (pathDir[pathDir.length() - 1] != '\\')
							pathDir += L"\\";
						pathDir += foundData.cFileName;
						pathDir += L"\\";

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
		} while (::FindNextFile(hFindFile, &foundData));
		::FindClose(hFindFile);
	}
}

void FileBrowser::addRootFolder(wstring rootFolderPath)
{
	if (!doesDirectoryExist(rootFolderPath.c_str()))
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
				wstring rootPath = f->_rootFolder._rootPath;
				wstring pathSuffix = rootFolderPath.substr(rootPath.size() + 1, rootFolderPath.size() - rootPath.size());
				vector<wstring> linarPathArray = split(pathSuffix, '\\');
				
				HTREEITEM foundItem = findInTree(rootPath, nullptr, linarPathArray);
				if (foundItem)
					_treeView.selectItem(foundItem);
				return;
			}
			
			if (isRelatedRootFolder(rootFolderPath, f->_rootFolder._rootPath))
			{
				NppParameters::getInstance().getNativeLangSpeaker()->messageBox("FolderAsWorspaceSubfolderExists",
					_hParent,
					L"A sub-folder of the folder you want to add exists.\rPlease remove its root from the panel before you add folder \"$STR_REPLACE$\".",
					L"Folder as Workspace adding folder problem",
					MB_OK,
					0, // not used
					rootFolderPath.c_str());
				return;
			}
		}
	}

	std::vector<wstring> patterns2Match;
	patterns2Match.push_back(L"*.*");

	wchar_t *label = ::PathFindFileName(rootFolderPath.c_str());
	wchar_t rootLabel[MAX_PATH] = {'\0'};
	wcscpy_s(rootLabel, label);
	size_t len = lstrlen(rootLabel);
	if (rootLabel[len - 1] == '\\')
		rootLabel[len - 1] = '\0';

	FolderInfo directoryStructure(rootLabel, nullptr);
	getDirectoryStructure(rootFolderPath.c_str(), patterns2Match, directoryStructure, true, false);
	HTREEITEM hRootItem = createFolderItemsFromDirStruct(nullptr, directoryStructure);
	_treeView.customSorting(hRootItem, categorySortFunc, 0, true); // needed here for possible *nix like storages (Samba, WebDAV, WSL, ...)
	_treeView.expand(hRootItem);
	_folderUpdaters.push_back(new FolderUpdater(directoryStructure, this));
	_folderUpdaters[_folderUpdaters.size() - 1]->startWatcher();
}

HTREEITEM FileBrowser::createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure)
{
	HTREEITEM hFolderItem = nullptr;
	if (directoryStructure._parent == nullptr && hParentItem == nullptr)
	{
		wchar_t rootPath[MAX_PATH] = { '\0' };
		wcscpy_s(rootPath, directoryStructure._rootPath.c_str());
		size_t len = lstrlen(rootPath);
		if (rootPath[len - 1] == '\\')
			rootPath[len - 1] = '\0';

		SortingData4lParam* customData = new SortingData4lParam(rootPath, L"", true);
		sortingDataArray.push_back(customData);

		hFolderItem = _treeView.addItem(directoryStructure._name.c_str(), TVI_ROOT, INDEX_CLOSE_ROOT, reinterpret_cast<LPARAM>(customData));
	}
	else
	{
		SortingData4lParam* customData = new SortingData4lParam(L"", directoryStructure._name, true);
		sortingDataArray.push_back(customData);

		hFolderItem = _treeView.addItem(directoryStructure._name.c_str(), hParentItem, INDEX_CLOSE_NODE, reinterpret_cast<LPARAM>(customData));
	}

	for (const auto& folder : directoryStructure._subFolders)
	{
		createFolderItemsFromDirStruct(hFolderItem, folder);
	}

	for (const auto& file : directoryStructure._files)
	{
		SortingData4lParam* customData = new SortingData4lParam(L"", file._name, false);
		sortingDataArray.push_back(customData);

		_treeView.addItem(file._name.c_str(), hFolderItem, INDEX_LEAF, reinterpret_cast<LPARAM>(customData));
	}

	_treeView.fold(hParentItem);

	return hFolderItem;
}

HTREEITEM FileBrowser::getRootFromFullPath(const wstring & rootPath) const
{
	HTREEITEM node = nullptr;
	for (HTREEITEM hItemNode = _treeView.getRoot();
		hItemNode != nullptr && node == nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		TVITEM tvItem{};
		tvItem.mask = TVIF_PARAM;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		if (tvItem.lParam != 0 && rootPath == reinterpret_cast<SortingData4lParam *>(tvItem.lParam)->_rootPath)
			node = hItemNode;
	}
	return node;
}

HTREEITEM FileBrowser::findChildNodeFromName(HTREEITEM parent, const wstring& label) const
{
	for (HTREEITEM hItemNode = _treeView.getChildFrom(parent);
		hItemNode != NULL;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		wchar_t textBuffer[MAX_PATH] = { '\0' };
		TVITEM tvItem{};
		tvItem.mask = TVIF_TEXT;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		if (label == tvItem.pszText)
		{
			return hItemNode;
		}
	}
	return nullptr;
}

vector<wstring> FileBrowser::getRoots() const
{
	vector<wstring> roots;

	for (HTREEITEM hItemNode = _treeView.getRoot();
		hItemNode != nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		TVITEM tvItem{};
		tvItem.mask = TVIF_PARAM;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		roots.push_back(reinterpret_cast<SortingData4lParam*>(tvItem.lParam)->_rootPath);
	}
	return roots;
}

wstring FileBrowser::getSelectedItemPath() const
{
	wstring itemPath;
	HTREEITEM hItemNode = _treeView.getSelection();
	if (hItemNode)
	{
		itemPath = getNodePath(hItemNode);
	}
	return itemPath;
}

std::vector<FileBrowser::FilesToChange> FileBrowser::getFilesFromParam(LPARAM lParam) const
{
	const std::vector<wstring> filesToChange = *(std::vector<wstring>*)lParam;
	const wstring separator = L"\\\\";
	const size_t separatorLength = separator.length();

	std::vector<FilesToChange> groupedFiles;
	for (size_t i = 0; i < filesToChange.size(); i++)
	{
		const size_t sepPos = filesToChange[i].find(separator);
		if (sepPos == wstring::npos)
			continue;

		const wstring pathSuffix = filesToChange[i].substr(sepPos + separatorLength, filesToChange[i].length() - 1);

		// remove prefix of file/folder in changeInfo, split the remained path
		vector<wstring> linarPathArray = split(pathSuffix, '\\');

		const wstring lastElement = linarPathArray.back();
		linarPathArray.pop_back();

		const wstring rootPath = filesToChange[i].substr(0, sepPos);

		const wstring addedFilePath = filesToChange[i].substr(0, sepPos + 1) + pathSuffix;

		wstring commonPath = rootPath;

		for (const auto & element : linarPathArray)
		{
			commonPath.append(L"\\");
			commonPath.append(element);
		}

		commonPath.append(L"\\");

		const auto it = std::find_if(groupedFiles.begin(), groupedFiles.end(), [&commonPath](const auto & group) { return group._commonPath == commonPath; });

		if (it == groupedFiles.end())
		{
			// Add a new file group
			FilesToChange group;
			group._commonPath = commonPath;
			group._rootPath = rootPath;
			group._linarWithoutLastPathElement = linarPathArray;
			group._files.push_back(lastElement);
			groupedFiles.push_back(group);
		}
		else
		{
			// Add to an existing file group
			it->_files.push_back(lastElement);
		}
	}

	return groupedFiles;
}

bool FileBrowser::addToTree(FilesToChange & group, HTREEITEM node)
{
	if (node == nullptr) // it's a root. Search the right root with rootPath
	{
		// Search
		if ((node = getRootFromFullPath(group._rootPath)) == nullptr)
			return false;
	}

	if (group._linarWithoutLastPathElement.size() == 0)
	{
		// Items to add should exist on the disk
		group._files.erase(std::remove_if(group._files.begin(), group._files.end(), 
			[&group](const auto & file)
			{
				return !doesPathExist((group._commonPath + file).c_str());
			}),
			group._files.end());

		if (group._files.empty()) 
		{
			return false;
		}

		// Search: if not found, add
		removeNamesAlreadyInNode(node, group._files);
		if (group._files.empty()) 
		{
			return false;
		}

		// Not found, good - Action
		for (auto & file : group._files) {
			if (doesDirectoryExist((group._commonPath + file).c_str()))
			{
				SortingData4lParam* customData = new SortingData4lParam(L"", file, true);
				sortingDataArray.push_back(customData);

				_treeView.addItem(file.c_str(), node, INDEX_CLOSE_NODE, reinterpret_cast<LPARAM>(customData));
			}
			else
			{
				SortingData4lParam* customData = new SortingData4lParam(L"", file, false);
				sortingDataArray.push_back(customData);

				_treeView.addItem(file.c_str(), node, INDEX_LEAF, reinterpret_cast<LPARAM>(customData));
			}
		}
		_treeView.customSorting(node, categorySortFunc, 0, false);
		return true;
	}
	else
	{
		for (HTREEITEM hItemNode = _treeView.getChildFrom(node);
			hItemNode != NULL;
			hItemNode = _treeView.getNextSibling(hItemNode))
		{
			wchar_t textBuffer[MAX_PATH] = { '\0' };
			TVITEM tvItem{};
			tvItem.mask = TVIF_TEXT;
			tvItem.pszText = textBuffer;
			tvItem.cchTextMax = MAX_PATH;
			tvItem.hItem = hItemNode;
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

			if (group._linarWithoutLastPathElement[0] == tvItem.pszText)
			{
				// search recursively the node for an action
				group._linarWithoutLastPathElement.erase(group._linarWithoutLastPathElement.begin());
				return addToTree(group, hItemNode);
			}
		}
		return false;
	}

}

bool FileBrowser::deleteFromTree(FilesToChange & group)
{
	std::vector<HTREEITEM> foundItems = findInTree(group, nullptr);

	if (foundItems.empty() == true)
	{
		return false;
	}

	for (auto & item : foundItems)
	{
		_treeView.removeItem(item);
	}

	return true;
}

HTREEITEM FileBrowser::findInTree(const wstring& rootPath, HTREEITEM node, std::vector<wstring> linarPathArray) const
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
			wchar_t textBuffer[MAX_PATH] = { '\0' };
			TVITEM tvItem{};
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

std::vector<HTREEITEM> FileBrowser::findInTree(FilesToChange & group, HTREEITEM node) const
{
	if (node == nullptr) // it's a root. Search the right root with rootPath
	{
		// Search
		if ((node = getRootFromFullPath(group._rootPath)) == nullptr)
		{
			return {};
		}
	}

	if (group._linarWithoutLastPathElement.empty())
	{
		// Search
		return findChildNodesFromNames(node, group._files);
	}
	else
	{
		for (HTREEITEM hItemNode = _treeView.getChildFrom(node);
			hItemNode != NULL;
			hItemNode = _treeView.getNextSibling(hItemNode))
		{
			wchar_t textBuffer[MAX_PATH] = {'\0'};
			TVITEM tvItem{};
			tvItem.mask = TVIF_TEXT;
			tvItem.pszText = textBuffer;
			tvItem.cchTextMax = MAX_PATH;
			tvItem.hItem = hItemNode;
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

			if (group._linarWithoutLastPathElement[0] == tvItem.pszText)
			{
				// search recursively the node for an action
				group._linarWithoutLastPathElement.erase(group._linarWithoutLastPathElement.begin());
				return findInTree(group, hItemNode);
			}
		}
		return {};
	}
}

std::vector<HTREEITEM> FileBrowser::findChildNodesFromNames(HTREEITEM parent, std::vector<wstring> & labels) const
{
	std::vector<HTREEITEM> itemNodes;

	for (HTREEITEM hItemNode = _treeView.getChildFrom(parent);
		hItemNode != NULL && !labels.empty();
		hItemNode = _treeView.getNextSibling(hItemNode)
		)
	{
		wchar_t textBuffer[MAX_PATH]{};
		TVITEM tvItem{};
		tvItem.mask = TVIF_TEXT;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		auto it = std::find(labels.begin(), labels.end(), tvItem.pszText);
		if (it != labels.end())
		{
			labels.erase(it); // remove, as it was already found
			itemNodes.push_back(hItemNode);
		}
	}
	return itemNodes;
}

void FileBrowser::removeNamesAlreadyInNode(HTREEITEM parent, std::vector<wstring> & labels) const
{
	// We have to search for the labels in the child nodes of parent, and remove the ones that already exist
	for (HTREEITEM hItemNode = _treeView.getChildFrom(parent);
		hItemNode != NULL && !labels.empty();
		hItemNode = _treeView.getNextSibling(hItemNode)
		)
	{
		wchar_t textBuffer[MAX_PATH]{};
		TVITEM tvItem{};
		tvItem.mask = TVIF_TEXT;
		tvItem.pszText = textBuffer;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

		auto it = std::find(labels.begin(), labels.end(), tvItem.pszText);
		if (it != labels.end())
		{
			labels.erase(it);
		}
	}
}

bool FileBrowser::renameInTree(const wstring& rootPath, HTREEITEM node, const std::vector<wstring>& linarPathArrayFrom, const wstring & renameTo)
{
	HTREEITEM foundItem = findInTree(rootPath, node, linarPathArrayFrom);
	if (foundItem == nullptr)
			return false;

	// found it, rename it
	_treeView.renameItem(foundItem, renameTo.c_str());
	SortingData4lParam* compareData = reinterpret_cast<SortingData4lParam*>(_treeView.getItemParam(foundItem));
	compareData->_label = renameTo;
	_treeView.customSorting(_treeView.getParent(foundItem), categorySortFunc, 0, false);

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

bool FolderInfo::addToStructure(wstring & fullpath, std::vector<wstring> linarPathArray)
{
	if (linarPathArray.size() == 1) // could be file or folder
	{
		fullpath += L"\\";
		fullpath += linarPathArray[0];
		if (doesDirectoryExist(fullpath.c_str()))
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
				fullpath += L"\\";
				fullpath += linarPathArray[0];
				linarPathArray.erase(linarPathArray.begin());
				return folder.addToStructure(fullpath, linarPathArray);
			}
		}
		return false;
	}
}

bool FolderInfo::removeFromStructure(std::vector<wstring> linarPathArray)
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

bool FolderInfo::renameInStructure(std::vector<wstring> linarPathArrayFrom, std::vector<wstring> linarPathArrayTo)
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
}


DWORD WINAPI FolderUpdater::watching(void *params)
{
	FolderUpdater *thisFolderUpdater = (FolderUpdater *)params;

	wstring dir2Watch = (thisFolderUpdater->_rootFolder)._rootPath;
	if (dir2Watch[dir2Watch.length() - 1] != '\\')
		dir2Watch += L"\\"; // CReadDirectoryChanges will add another '\' so we will get "\\" as a separator (of monitored root) in the notification

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
				static const unsigned int MAX_BATCH_SIZE = 100;

				DWORD dwPreviousAction = 0;
				DWORD dwAction;
				wstring wstrFilename;

				std::vector<wstring> filesToChange;
				// Process all available changes, ignore User actions
				while (changes.Pop(dwAction, wstrFilename))
				{

					// FILE_ACTION_ADDED and FILE_ACTION_REMOVED are done in batches
					if (dwAction != FILE_ACTION_ADDED && dwAction != FILE_ACTION_REMOVED)
					{
						processChange(dwAction, { wstrFilename }, thisFolderUpdater);
					}
					else 
					{
						// first iteration
						if (dwPreviousAction == 0)
						{
							dwPreviousAction = dwAction;
						}

						if (dwPreviousAction == dwAction)
						{
							filesToChange.push_back(wstrFilename);

							if (filesToChange.size() > MAX_BATCH_SIZE) // Process some so the editor doesn't block for too long
							{
								processChange(dwAction, filesToChange, thisFolderUpdater);
								filesToChange.clear();
							}
						}
						else
						{
							// Different action. Process the previous batch and start saving a new one
							processChange(dwPreviousAction, filesToChange, thisFolderUpdater);
							filesToChange.clear();

							dwPreviousAction = dwAction;
							filesToChange.push_back(wstrFilename);
						}
					}
				}

				// process the last changes
				if (dwAction == FILE_ACTION_ADDED || dwAction == FILE_ACTION_REMOVED)
				{
					processChange(dwAction, filesToChange, thisFolderUpdater);
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
	return ERROR_SUCCESS;
}

void FolderUpdater::processChange(DWORD dwAction, std::vector<wstring> filesToChange, FolderUpdater* thisFolderUpdater)
{
	static wstring oldName;

	switch (dwAction)
	{
	case FILE_ACTION_ADDED:

		::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_ADDFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&filesToChange));
		oldName = L"";
		break;

	case FILE_ACTION_REMOVED:
		
		::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_RMFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&filesToChange));
		oldName = L"";
		break;

	case FILE_ACTION_MODIFIED:
		oldName = L"";
		break;

	case FILE_ACTION_RENAMED_OLD_NAME:
		oldName = filesToChange.back();
		break;

	case FILE_ACTION_RENAMED_NEW_NAME:
		if (!oldName.empty())
		{
			std::vector<wstring> fileRename;
			fileRename.push_back(oldName);
			fileRename.push_back(filesToChange.back());
			//thisFolderUpdater->updateTree(dwAction, fileRename);
			::SendMessage((thisFolderUpdater->_pFileBrowser)->getHSelf(), FB_RNFILE, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&fileRename));
		}
		oldName = L"";
		break;

	default:
		oldName = L"";
		break;
	}
}
