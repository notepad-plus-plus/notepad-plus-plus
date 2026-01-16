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


#include "ProjectPanel.h"

#include <windows.h>

#include <shlwapi.h>
#include <windowsx.h>

#include <wchar.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "Common.h"
#include "CustomFileDialog.h"
#include "DockingDlgInterface.h"
#include "Notepad_plus_msgs.h"
#include "NppDarkMode.h"
#include "NppXml.h"
#include "Parameters.h"
#include "ProjectPanel_rc.h"
#include "StaticDialog.h"
#include "dockingResource.h"
#include "localization.h"
#include "menuCmdID.h"
#include "resource.h"

enum TvIndex : int
{
	INDEX_CLEAN_ROOT,
	INDEX_DIRTY_ROOT,
	INDEX_PROJECT,
	INDEX_OPEN_NODE,
	INDEX_CLOSED_NODE,
	INDEX_LEAF,
	INDEX_LEAF_INVALID,
};

using namespace std;

intptr_t CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			ProjectPanel::initMenus();

			// Create toolbar menu
			int style = WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_AUTOSIZE | TBSTYLE_FLAT | TBSTYLE_LIST;
			_hToolbarMenu = CreateWindowEx(0,TOOLBARCLASSNAME,NULL, style,
								   0,0,0,0,_hSelf, nullptr, _hInst, nullptr);

			TBBUTTON tbButtons[2]{};

			NppParameters& nppParam = NppParameters::getInstance();
			NativeLangSpeaker *pNativeSpeaker = nppParam.getNativeLangSpeaker();
			wstring workspace_entry = pNativeSpeaker->getProjectPanelLangMenuStr("Entries", 0, PM_WORKSPACEMENUENTRY);
			wstring edit_entry = pNativeSpeaker->getProjectPanelLangMenuStr("Entries", 1, PM_EDITMENUENTRY);

			tbButtons[0].idCommand = IDB_PROJECT_BTN;
			tbButtons[0].iBitmap = I_IMAGENONE;
			tbButtons[0].fsState = TBSTATE_ENABLED;
			tbButtons[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[0].iString = reinterpret_cast<intptr_t>(workspace_entry.c_str());

			tbButtons[1].idCommand = IDB_EDIT_BTN;
			tbButtons[1].iBitmap = I_IMAGENONE;
			tbButtons[1].fsState = TBSTATE_ENABLED;
			tbButtons[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
			tbButtons[1].iString = reinterpret_cast<intptr_t>(edit_entry.c_str());

			SendMessage(_hToolbarMenu, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			SendMessage(_hToolbarMenu, TB_ADDBUTTONS, sizeof(tbButtons) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(&tbButtons));
			SendMessage(_hToolbarMenu, TB_AUTOSIZE, 0, 0); 
			ShowWindow(_hToolbarMenu, SW_SHOW);

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_PROJECT_WORKSPACE, IDI_PROJECT_WORKSPACEDIRTY, IDI_PROJECT_PROJECT, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE, IDI_PROJECT_FILEINVALID }
				, { IDI_PROJECT_WORKSPACE_DM, IDI_PROJECT_WORKSPACEDIRTY_DM, IDI_PROJECT_PROJECT_DM, IDI_PROJECT_FOLDEROPEN_DM, IDI_PROJECT_FOLDERCLOSE_DM, IDI_PROJECT_FILE_DM, IDI_PROJECT_FILEINVALID_DM }
				, { IDI_PROJECT_WORKSPACE2, IDI_PROJECT_WORKSPACEDIRTY2, IDI_PROJECT_PROJECT2, IDI_PROJECT_FOLDEROPEN2, IDI_PROJECT_FOLDERCLOSE2, IDI_PROJECT_FILE2, IDI_PROJECT_FILEINVALID2 }
			);

			_treeView.init(_hInst, _hSelf, ID_PROJECTTREEVIEW);
			_treeView.setImageList(imgIds);

			_treeView.addCanNotDropInList(INDEX_LEAF);
			_treeView.addCanNotDropInList(INDEX_LEAF_INVALID);

			_treeView.addCanNotDragOutList(INDEX_CLEAN_ROOT);
			_treeView.addCanNotDragOutList(INDEX_DIRTY_ROOT);
			_treeView.addCanNotDragOutList(INDEX_PROJECT);

			_treeView.display();
			if (!openWorkSpace(_workSpaceFilePath.c_str(), true))
				newWorkSpace();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			if (static_cast<BOOL>(lParam) != TRUE)
			{
				NppDarkMode::autoThemeChildControls(_hSelf);
			}
			else
			{
				NppDarkMode::setTreeViewStyle(_treeView.getHSelf());
			}

			std::vector<int> imgIds = _treeView.getImageIds(
				{ IDI_PROJECT_WORKSPACE, IDI_PROJECT_WORKSPACEDIRTY, IDI_PROJECT_PROJECT, IDI_PROJECT_FOLDEROPEN, IDI_PROJECT_FOLDERCLOSE, IDI_PROJECT_FILE, IDI_PROJECT_FILEINVALID }
				, { IDI_PROJECT_WORKSPACE_DM, IDI_PROJECT_WORKSPACEDIRTY_DM, IDI_PROJECT_PROJECT_DM, IDI_PROJECT_FOLDEROPEN_DM, IDI_PROJECT_FOLDERCLOSE_DM, IDI_PROJECT_FILE_DM, IDI_PROJECT_FILEINVALID_DM }
				, { IDI_PROJECT_WORKSPACE2, IDI_PROJECT_WORKSPACEDIRTY2, IDI_PROJECT_PROJECT2, IDI_PROJECT_FOLDEROPEN2, IDI_PROJECT_FOLDERCLOSE2, IDI_PROJECT_FILE2, IDI_PROJECT_FILEINVALID2 }
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
					setWorkSpaceDirty(true);
			break;

		case WM_NOTIFY:
		{
			notified(reinterpret_cast<LPNMHDR>(lParam));
		}
		return TRUE;

		case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			RECT toolbarMenuRect{};
			::GetClientRect(_hToolbarMenu, &toolbarMenuRect);

			::MoveWindow(_hToolbarMenu, 0, 0, width, toolbarMenuRect.bottom, TRUE);

			HWND hwnd = _treeView.getHSelf();
			if (hwnd)
				::MoveWindow(hwnd, 0, toolbarMenuRect.bottom + 2, width, height - toolbarMenuRect.bottom - 2, TRUE);
			break;
		}

		case WM_CONTEXTMENU:
			if (!_treeView.isDragging())
			{
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);

				// If the context menu is generated from the keyboard, then we
				// should display it at the location of the current selection
				if (xPos == -1 && yPos == -1)
				{
					HTREEITEM selectedItem = _treeView.getSelection();

					if (selectedItem)
					{
						RECT selectedItemRect{};
						if (TreeView_GetItemRect(_treeView.getHSelf(), selectedItem, &selectedItemRect, TRUE))
						{
							showContextMenuFromMenuKey(selectedItem, (selectedItemRect.left + selectedItemRect.right) / 2, (selectedItemRect.top + selectedItemRect.bottom) / 2);
						}
					}
				}
				else
				{
					showContextMenu(xPos, yPos);
				}
			}
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

bool ProjectPanel::checkIfNeedSave()
{
	if (_isDirty)
	{
		const wchar_t * title = _workSpaceFilePath.length() > 0 ? PathFindFileName (_workSpaceFilePath.c_str()) : _panelTitle.c_str();
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		int res = pNativeSpeaker->messageBox("ProjectPanelChanged",
			_hSelf,
			L"The workspace was modified. Do you want to save it?",
			L"$STR_REPLACE$",
			MB_YESNOCANCEL | MB_ICONQUESTION,
			0,
			title);

		if (res == IDYES)
		{
			if (!saveWorkSpace())
				return false;
		}
		else if (res == IDNO)
		{
			// Don't save so do nothing here
		}
		else
		{
			// Cancelled
			return false;
		}
	}
	return true;
}

void ProjectPanel::initMenus()
{
	_hWorkSpaceMenu = ::CreatePopupMenu();
	
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	wstring new_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_NEWWS, PM_NEWWORKSPACE);
	wstring open_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_OPENWS, PM_OPENWORKSPACE);
	wstring reload_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_RELOADWS, PM_RELOADWORKSPACE);
	wstring save_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_SAVEWS, PM_SAVEWORKSPACE);
	wstring saveas_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_SAVEASWS, PM_SAVEASWORKSPACE);
	wstring saveacopyas_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_SAVEACOPYASWS, PM_SAVEACOPYASWORKSPACE);
	wstring newproject_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_NEWPROJECT, PM_NEWPROJECTWORKSPACE);
	wstring findinprojects_workspace = pNativeSpeaker->getProjectPanelLangMenuStr("WorkspaceMenu", IDM_PROJECT_FINDINPROJECTSWS, PM_FINDINFILESWORKSPACE);

	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWWS, new_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_OPENWS, open_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RELOADWS, reload_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEWS, save_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEASWS, saveas_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_SAVEACOPYASWS, saveacopyas_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWPROJECT, newproject_workspace.c_str());
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hWorkSpaceMenu, 0, MF_BYCOMMAND, IDM_PROJECT_FINDINPROJECTSWS, findinprojects_workspace.c_str());

	wstring edit_moveup = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_MOVEUP, PM_MOVEUPENTRY);
	wstring edit_movedown = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_MOVEDOWN, PM_MOVEDOWNENTRY);
	wstring edit_rename = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_RENAME, PM_EDITRENAME);
	wstring edit_addfolder = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_NEWFOLDER, PM_EDITNEWFOLDER);
	wstring edit_addfiles = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_ADDFILES, PM_EDITADDFILES);
	wstring edit_addfilesRecursive = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_ADDFILESRECUSIVELY, PM_EDITADDFILESRECUSIVELY);
	wstring edit_remove = pNativeSpeaker->getProjectPanelLangMenuStr("ProjectMenu", IDM_PROJECT_DELETEFOLDER, PM_EDITREMOVE);

	_hProjectMenu = ::CreatePopupMenu();
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEUP, edit_moveup.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEDOWN, edit_movedown.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, edit_rename.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, edit_addfolder.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, edit_addfiles.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILESRECUSIVELY, edit_addfilesRecursive.c_str());
	::InsertMenu(_hProjectMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, edit_remove.c_str());

	edit_moveup = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_MOVEUP, PM_MOVEUPENTRY);
	edit_movedown = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_MOVEDOWN, PM_MOVEDOWNENTRY);
	edit_rename = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_RENAME, PM_EDITRENAME);
	edit_addfolder = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_NEWFOLDER, PM_EDITNEWFOLDER);
	edit_addfiles = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_ADDFILES, PM_EDITADDFILES);
	edit_addfilesRecursive = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_ADDFILESRECUSIVELY, PM_EDITADDFILESRECUSIVELY);
	edit_remove = pNativeSpeaker->getProjectPanelLangMenuStr("FolderMenu", IDM_PROJECT_DELETEFOLDER, PM_EDITREMOVE);

	_hFolderMenu = ::CreatePopupMenu();
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEUP,        edit_moveup.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEDOWN,      edit_movedown.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME,        edit_rename.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER,     edit_addfolder.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES,      edit_addfiles.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILESRECUSIVELY, edit_addfilesRecursive.c_str());
	::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER,  edit_remove.c_str());

	edit_moveup = pNativeSpeaker->getProjectPanelLangMenuStr("FileMenu", IDM_PROJECT_MOVEUP, PM_MOVEUPENTRY);
	edit_movedown = pNativeSpeaker->getProjectPanelLangMenuStr("FileMenu", IDM_PROJECT_MOVEDOWN, PM_MOVEDOWNENTRY);
	edit_rename = pNativeSpeaker->getProjectPanelLangMenuStr("FileMenu", IDM_PROJECT_RENAME, PM_EDITRENAME);
	edit_remove = pNativeSpeaker->getProjectPanelLangMenuStr("FileMenu", IDM_PROJECT_DELETEFILE, PM_EDITREMOVE);
	wstring edit_modifyfile = pNativeSpeaker->getProjectPanelLangMenuStr("FileMenu", IDM_PROJECT_MODIFYFILEPATH, PM_EDITMODIFYFILE);

	_hFileMenu = ::CreatePopupMenu();
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEUP, edit_moveup.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MOVEDOWN, edit_movedown.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, static_cast<UINT>(-1), 0);
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, edit_rename.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFILE, edit_remove.c_str());
	::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_MODIFYFILEPATH, edit_modifyfile.c_str());
}

void ProjectPanel::destroyMenus() const 
{
	::DestroyMenu(_hWorkSpaceMenu);
	::DestroyMenu(_hProjectMenu);
	::DestroyMenu(_hFolderMenu);
	::DestroyMenu(_hFileMenu);
}

bool ProjectPanel::openWorkSpace(const wchar_t* projectFileName, bool force)
{
	if (!projectFileName)
		return false;

	if ((!force) && (_workSpaceFilePath.length() > 0))
	{ // Return if it is better to keep the current workspace tree
		wstring newWorkspace = projectFileName;
		if (newWorkspace == _workSpaceFilePath)
			return true;
		if (!saveWorkspaceRequest())
			return true;
	}

	if (!doesFileExist(projectFileName))
		return false;

	NppXml::NewDocument xmlDocProject{};
	const bool loadOkay = NppXml::loadFile(&xmlDocProject, projectFileName);
	if (!loadOkay)
		return false;

	NppXml::Element root = NppXml::firstChildElement(&xmlDocProject, "NotepadPlus");
	if (!root)
		return false;

	NppXml::Element childNode = NppXml::firstChildElement(root, "Project");
	if (!childNode)
		return false;

	_treeView.removeAllItems();
	_workSpaceFilePath = projectFileName;

	wchar_t * fileName = PathFindFileName(projectFileName);
	HTREEITEM rootItem = _treeView.addItem(fileName, TVI_ROOT, INDEX_CLEAN_ROOT);

	for (;
		childNode;
		childNode = NppXml::nextSiblingElement(childNode, "Project"))
	{
		const char* itemName = NppXml::attribute(childNode, "name");
		HTREEITEM projectItem = _treeView.addItem(string2wstring(itemName).c_str(), rootItem, INDEX_PROJECT);
		buildTreeFrom(childNode, projectItem);
	}
	setWorkSpaceDirty(false);
	_treeView.expand(rootItem);

	return true;
}

void ProjectPanel::newWorkSpace()
{
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
	wstring workspace = pNativeSpeaker->getAttrNameStr(PM_WORKSPACEROOTNAME, "ProjectManager", "WorkspaceRootName");
	_treeView.addItem(workspace.c_str(), TVI_ROOT, INDEX_CLEAN_ROOT);
	setWorkSpaceDirty(false);
	_workSpaceFilePath = L"";
}

bool ProjectPanel::saveWorkSpace()
{
	if (_workSpaceFilePath == L"")
	{
		return saveWorkSpaceAs(false);
	}
	else
	{
		if (!writeWorkSpace())
			return false;

		setWorkSpaceDirty(false);
		return true;
	} 
}

bool ProjectPanel::writeWorkSpace(const wchar_t* projectFileName, bool doUpdateGUI)
{
	//write <NotepadPlus>: use the default file name if new file name is not given
	const wchar_t* fn2write = projectFileName ? projectFileName : _workSpaceFilePath.c_str();
	NppXml::NewDocument projDoc{};
	NppXml::Node root = NppXml::createChildElement(projDoc, "NotepadPlus");

	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;

	//for each project, write <Project>
	HTREEITEM tvRoot = _treeView.getRoot();
	if (!tvRoot)
		return false;

	for (HTREEITEM tvProj = _treeView.getChildFrom(tvRoot);
		tvProj != nullptr;
		tvProj = _treeView.getNextSibling(tvProj))
	{
		tvItem.hItem = tvProj;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		//printStr(tvItem.pszText);

		NppXml::Element projRoot = NppXml::createChildElement(root, "Project");
		NppXml::setAttribute(projRoot, "name", wstring2string(tvItem.pszText).c_str());
		buildProjectXml(projRoot, tvProj, fn2write);
	}

	if (!NppXml::saveFileProject(&projDoc, fn2write))
	{
		const wchar_t* title = _workSpaceFilePath.length() > 0 ? PathFindFileName (_workSpaceFilePath.c_str()) : _panelTitle.c_str();
		NativeLangSpeaker *pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
		pNativeSpeaker->messageBox("ProjectPanelSaveError",
		_hSelf,
		L"An error occurred while writing your workspace file.\nYour workspace has not been saved.",
		L"$STR_REPLACE$",
		MB_OK | MB_ICONERROR,
		0,
		title);

		return false;
	}
	wchar_t * fileName = PathFindFileName(fn2write);
	if (doUpdateGUI)
	{
		_treeView.renameItem(tvRoot, fileName);
	}

	return true;
}

void ProjectPanel::buildProjectXml(NppXml::Node& root, HTREEITEM hItem, const wchar_t* fn2write)
{
	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;

	for (HTREEITEM hItemNode = _treeView.getChildFrom(hItem);
		hItemNode != nullptr;
		hItemNode = _treeView.getNextSibling(hItemNode))
	{
		tvItem.hItem = hItemNode;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		if (tvItem.lParam)
		{
			const auto* const fn = reinterpret_cast<std::wstring*>(tvItem.lParam);
			std::string newFn = wstring2string(getRelativePath(*fn, fn2write));
			NppXml::Element fileLeaf = NppXml::createChildElement(root, "File");
			NppXml::setAttribute(fileLeaf, "name", newFn.c_str());
		}
		else
		{
			NppXml::Element folderNode = NppXml::createChildElement(root, "Folder");
			NppXml::setAttribute(folderNode, "name", wstring2string(tvItem.pszText).c_str());
			buildProjectXml(folderNode, hItemNode, fn2write);
		}
	}
}

bool ProjectPanel::enumWorkSpaceFiles(HTREEITEM tvFrom, const std::vector<wstring> & patterns, std::vector<wstring> & fileNames)
{
	wchar_t textBuffer[MAX_PATH] = { '\0' };
	TVITEM tvItem{};
	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText = textBuffer;
	tvItem.cchTextMax = MAX_PATH;

	HTREEITEM tvRoot = tvFrom ? tvFrom : _treeView.getRoot();
	if (!tvRoot) return false;

	for (HTREEITEM tvProj = _treeView.getChildFrom(tvRoot);
		tvProj != nullptr;
		tvProj = _treeView.getNextSibling(tvProj))
	{
		tvItem.hItem = tvProj;
		SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		if (tvItem.lParam)
		{
			if (matchInList(tvItem.pszText, patterns))
			{
				auto* const fn = reinterpret_cast<std::wstring*>(tvItem.lParam);
				fileNames.push_back(*fn);
			}
		}
		else
		{
			if (!enumWorkSpaceFiles (tvProj, patterns, fileNames)) return false;
		}
	}
	return true;
}

wstring ProjectPanel::getRelativePath(const wstring& filePath, const wchar_t *workSpaceFileName)
{
	wchar_t wsfn[MAX_PATH] = { '\0' };
	wcscpy_s(wsfn, workSpaceFileName);
	::PathRemoveFileSpec(wsfn);

	size_t pos_found = filePath.find(wsfn);
	if (pos_found == wstring::npos)
		return filePath;
	const wchar_t *relativeFile = filePath.c_str() + lstrlen(wsfn);
	if (relativeFile[0] == '\\')
		++relativeFile;
	return relativeFile;
}

bool ProjectPanel::buildTreeFrom(const NppXml::Node& projectRoot, HTREEITEM hParentItem)
{
	for (NppXml::Node childNode = NppXml::firstChildElement(projectRoot);
		childNode;
		childNode = NppXml::nextSibling(childNode))
	{
		const char* nodeName = NppXml::name(childNode);
		if (std::strcmp(nodeName, "Folder") == 0)
		{
			const char* itemName = NppXml::attribute(childNode, "name");
			HTREEITEM addedItem = _treeView.addItem(string2wstring(itemName).c_str(), hParentItem, INDEX_CLOSED_NODE);
			if (NppXml::firstChild(childNode))
			{
				bool isOK = buildTreeFrom(childNode, addedItem);
				if (!isOK)
					return false;
			}
		}
		else if (std::strcmp(nodeName, "File") == 0)
		{
			const std::wstring strValue = string2wstring(NppXml::attribute(childNode, "name"));
			const std::wstring fullPath = getAbsoluteFilePath(strValue.c_str());
			const wchar_t* strValueLabel = ::PathFindFileNameW(strValue.c_str());
			int iImage = doesFileExist(fullPath.c_str()) ? INDEX_LEAF : INDEX_LEAF_INVALID;

			_fullPathStrs.push_back(std::make_unique<std::wstring>(fullPath));
			auto lParamFullPathStr = reinterpret_cast<LPARAM>(_fullPathStrs.back().get());

			_treeView.addItem(strValueLabel, hParentItem, iImage, lParamFullPathStr);
		}
	}
	return true;
}

wstring ProjectPanel::getAbsoluteFilePath(const wchar_t * relativePath)
{
	if (!::PathIsRelative(relativePath))
		return relativePath;

	wchar_t absolutePath[MAX_PATH] = { '\0' };
	wcscpy_s(absolutePath, _workSpaceFilePath.c_str());
	::PathRemoveFileSpec(absolutePath);
	::PathAppend(absolutePath, relativePath);
	return absolutePath;
}

void ProjectPanel::openSelectFile()
{
	TVITEM tvItem{};
	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = _treeView.getSelection();
	::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

	NodeType nType = getNodeType(tvItem.hItem);
	const auto* const fn = reinterpret_cast<std::wstring*>(tvItem.lParam);
	if (nType == nodeType_file && fn)
	{
		tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		if (doesFileExist(fn->c_str()))
		{
			::PostMessage(_hParent, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(fn->c_str()));
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


void ProjectPanel::notified(LPNMHDR notification)
{
	if (notification->code == DMN_CLOSE)
	{
		::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_PROJECT_PANEL_1 + _panelID, 0);
		SetWindowLongPtr (getHSelf(), DWLP_MSGRESULT, _isClosed ? 0 : 1);
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

			case NM_RETURN:
				SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, 1);
			break;
	
			case TVN_ENDLABELEDIT:
			{
				auto* tvnotif = reinterpret_cast<LPNMTVDISPINFOW>(notification);
				if (!tvnotif->item.pszText)
					return;
				if (getNodeType(tvnotif->item.hItem) == nodeType_root)
					return;

				// Processing for only File case
				if (tvnotif->item.lParam)
				{
					// Get the old label
					tvItem.hItem = _treeView.getSelection();
					::SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
					size_t len = std::wcslen(tvItem.pszText);

					// Find the position of old label in File path
					auto* const filePath = reinterpret_cast<std::wstring*>(tvnotif->item.lParam);
					size_t found = filePath->rfind(tvItem.pszText);

					// If found the old label, replace it with the modified one
					if (found != wstring::npos)
						filePath->replace(found, len, tvnotif->item.pszText);

					// Check the validity of modified file path
					tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					if (doesFileExist(filePath->c_str()))
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
				::SendMessage(_treeView.getHSelf(), TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&(tvnotif->item)));
				setWorkSpaceDirty(true);
			}
			break;

			case TVN_GETINFOTIP:
			{
				auto* lpGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIPW>(notification);
				std::wstring* str = nullptr;

				if (_treeView.getRoot() == lpGetInfoTip->hItem)
				{
					str = &_workSpaceFilePath;
				}
				else
				{
					str = reinterpret_cast<std::wstring*>(lpGetInfoTip->lParam);
					if (!str)
						return;
				}
				lpGetInfoTip->pszText = str->data();
				lpGetInfoTip->cchTextMax = static_cast<int>(str->size());
			}
			break;

			case TVN_KEYDOWN:
			{
				auto* ptvkd = reinterpret_cast<LPNMTVKEYDOWN>(notification);
				
				if (ptvkd->wVKey == VK_DELETE)
				{
					HTREEITEM hItem = _treeView.getSelection();
					NodeType nType = getNodeType(hItem);
					if (nType == nodeType_project || nType == nodeType_folder)
						popupMenuCmd(IDM_PROJECT_DELETEFOLDER);
					else if (nType == nodeType_file)
						popupMenuCmd(IDM_PROJECT_DELETEFILE);
				}
				else if (ptvkd->wVKey == VK_RETURN)
				{
					HTREEITEM hItem = _treeView.getSelection();
					NodeType nType = getNodeType(hItem);
					if (nType == nodeType_file)
						openSelectFile();
					else
						_treeView.toggleExpandCollapse(hItem);
				}
				else if (ptvkd->wVKey == VK_UP)
				{
					if (0x80 & GetKeyState(VK_CONTROL))
					{
						popupMenuCmd(IDM_PROJECT_MOVEUP);
					}
				}
				else if (ptvkd->wVKey == VK_DOWN)
				{
					if (0x80 & GetKeyState(VK_CONTROL))
					{
						popupMenuCmd(IDM_PROJECT_MOVEDOWN);
					}
				}
				else if (ptvkd->wVKey == VK_F2)
					popupMenuCmd(IDM_PROJECT_RENAME);
				
			}
			break;

			case TVN_ITEMEXPANDED:
			{
				auto* nmtv = reinterpret_cast<LPNMTREEVIEWW>(notification);
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

			case TVN_BEGINDRAG:
			{
				_treeView.beginDrag(reinterpret_cast<LPNMTREEVIEWW>(notification));
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
	TVITEM tvItem{};
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_IMAGE | TVIF_PARAM;
	SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));

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
	else if (!tvItem.lParam)
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
	TVHITTESTINFO tvHitInfo{};

	// Detect if the given position is on the element TVITEM
	tvHitInfo.pt.x = x;
	tvHitInfo.pt.y = y;
	tvHitInfo.flags = 0;
	ScreenToClient(_treeView.getHSelf(), &(tvHitInfo.pt));
	TreeView_HitTest(_treeView.getHSelf(), &tvHitInfo);

	if (tvHitInfo.hItem != NULL)
	{
		// Make item selected
		_treeView.selectItem(tvHitInfo.hItem);
		HMENU hMenu = getMenuHandler(tvHitInfo.hItem);
		TrackPopupMenu(hMenu,
			NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
			x, y, 0, _hSelf, NULL);
	}
}

void ProjectPanel::showContextMenuFromMenuKey(HTREEITEM selectedItem, int x, int y)
{
	POINT p{};
	p.x = x;
	p.y = y;

	ClientToScreen(_treeView.getHSelf(), &p);

	if (selectedItem != NULL)
	{
		HMENU hMenu = getMenuHandler(selectedItem);
		TrackPopupMenu(hMenu,
			NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
			x, y, 0, _hSelf, NULL);
	}
}

HMENU ProjectPanel::getMenuHandler(HTREEITEM selectedItem)
{
	// get clicked item type
	NodeType nodeType = getNodeType(selectedItem);
	HMENU hMenu = NULL;

	if (nodeType == nodeType_root)
		hMenu = _hWorkSpaceMenu;
	else if (nodeType == nodeType_project)
		hMenu = _hProjectMenu;
	else if (nodeType == nodeType_folder)
		hMenu = _hFolderMenu;
	else //nodeType_file
		hMenu = _hFileMenu;

	return hMenu;
}

POINT ProjectPanel::getMenuDisplayPoint(int iButton) const
{
	POINT p{};
	RECT btnRect{};
	SendMessage(_hToolbarMenu, TB_GETITEMRECT, iButton, reinterpret_cast<LPARAM>(&btnRect));

	p.x = btnRect.left;
	p.y = btnRect.top + btnRect.bottom;
	ClientToScreen(_hToolbarMenu, &p);
	return p;
}

HTREEITEM ProjectPanel::addFolder(HTREEITEM hTreeItem, const wchar_t *folderName)
{
	HTREEITEM addedItem = _treeView.addItem(folderName, hTreeItem, INDEX_CLOSED_NODE);
	
	TreeView_Expand(_treeView.getHSelf(), hTreeItem, TVE_EXPAND);
	TreeView_EditLabel(_treeView.getHSelf(), addedItem);
	if (getNodeType(hTreeItem) == nodeType_folder)
		_treeView.setItemImage(hTreeItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);

	return addedItem;
}

bool ProjectPanel::saveWorkspaceRequest()
{ // returns true for continue and false for break
	if (_isDirty)
	{
		NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		int res = pNativeSpeaker->messageBox("ProjectPanelOpenDoSaveDirtyWsOrNot",
					_hSelf,
					L"The current workspace was modified. Do you want to save the current project?",
					L"Open Workspace",
					MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
				
		if (res == IDYES)
		{
			if (!saveWorkSpace())
				return false;
		}
		else if (res == IDNO)
		{
			// Don't save so do nothing here
		}
		else if (res == IDCANCEL) 
		{
			// User cancels action "New Workspace" so we interrupt here
			return false;
		}
	}
	return true;
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
		  POINT p = getMenuDisplayPoint(0);
		  TrackPopupMenu(_hWorkSpaceMenu,
			  NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
			  p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		case IDB_EDIT_BTN:
		{
			POINT p = getMenuDisplayPoint(1);
			HMENU hMenu = NULL;
			NodeType nodeType = getNodeType(hTreeItem);
			if (nodeType == nodeType_project)
				hMenu = _hProjectMenu;
			else if (nodeType == nodeType_folder)
				hMenu = _hFolderMenu;
			else if (nodeType == nodeType_file)
				hMenu = _hFileMenu;
			if (hMenu)
				TrackPopupMenu(hMenu,
					NppParameters::getInstance().getNativeLangSpeaker()->isRTL() ? TPM_RIGHTALIGN | TPM_LAYOUTRTL : TPM_LEFTALIGN,
					p.x, p.y, 0, _hSelf, NULL);
		}
		break;

		//
		// Toolbar menu commands
		//
		case IDM_PROJECT_NEWPROJECT :
		{
			HTREEITEM root = _treeView.getRoot();

			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			wstring newProjectLabel = pNativeSpeaker->getAttrNameStr(PM_NEWPROJECTNAME, "ProjectManager", "NewProjectName");
			HTREEITEM addedItem = _treeView.addItem(newProjectLabel.c_str(),  root, INDEX_PROJECT);
			setWorkSpaceDirty(true);
			_treeView.expand(hTreeItem);
			TreeView_EditLabel(_treeView.getHSelf(), addedItem);
		}
		break;

		case IDM_PROJECT_NEWWS :
		{
			if (_isDirty)
			{
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				int res = pNativeSpeaker->messageBox("ProjectPanelNewDoSaveDirtyWsOrNot",
					_hSelf,
					L"The current workspace was modified. Do you want to save the current project?",
					L"New Workspace",
					MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
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
					// User cancels action "New Workspace" so we interrupt here
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
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			wstring newFolderLabel = pNativeSpeaker->getAttrNameStr(PM_NEWFOLDERNAME, "ProjectManager", "NewFolderName");
			addFolder(hTreeItem, newFolderLabel.c_str());
			setWorkSpaceDirty(true);
		}
		break;

		case IDM_PROJECT_MOVEDOWN :
		{
			if (_treeView.moveDown(hTreeItem))
				setWorkSpaceDirty(true);
		}
		break;

		case IDM_PROJECT_MOVEUP :
		{
			if (_treeView.moveUp(hTreeItem))
				setWorkSpaceDirty(true);
		}
		break;

		case IDM_PROJECT_ADDFILES :
		{
			addFiles(hTreeItem);
			if (getNodeType(hTreeItem) == nodeType_folder)
				_treeView.setItemImage(hTreeItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
		}
		break;

		case IDM_PROJECT_ADDFILESRECUSIVELY :
		{
			addFilesFromDirectory(hTreeItem);
			if (getNodeType(hTreeItem) == nodeType_folder)
				_treeView.setItemImage(hTreeItem, INDEX_OPEN_NODE, INDEX_OPEN_NODE);
		}
		break;

		case IDM_PROJECT_OPENWS:
		{
			if (!saveWorkspaceRequest())
				break;

			CustomFileDialog fDlg(_hSelf);
			setFileExtFilter(fDlg);
			const wstring fn = fDlg.doOpenSingleFileDlg();
			if (!fn.empty())
			{
				if (!openWorkSpace(fn.c_str(), true))
				{
					NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
					pNativeSpeaker->messageBox("ProjectPanelOpenFailed",
						_hSelf,
						L"The workspace could not be opened.\rIt seems the file to open is not a valid project file.",
						L"Open Workspace",
						MB_OK);
					return;
				}
			}
		}
		break;

		case IDM_PROJECT_RELOADWS:
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			if (_isDirty)
			{
				int res = pNativeSpeaker->messageBox("ProjectPanelReloadDirty",
					_hSelf,
					L"The current workspace was modified. Reloading will discard all modifications.\rDo you want to continue?",
					L"Reload Workspace",
					MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);

				if (res == IDNO)
				{
					return;
				}
			}

			if (doesFileExist(_workSpaceFilePath.c_str()))
			{
				openWorkSpace(_workSpaceFilePath.c_str(), true);
			}
			else
			{
				pNativeSpeaker->messageBox("ProjectPanelReloadError",
					_hSelf,
					L"Cannot find the file to reload.",
					L"Reload Workspace",
					MB_OK);
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

		case IDM_PROJECT_FINDINPROJECTSWS:
		{
			::SendMessage(_hParent, NPPM_INTERNAL_FINDINPROJECTS, static_cast<WPARAM>(1) << _panelID, 0);
		}
		break;

		case IDM_PROJECT_DELETEFOLDER :
		{
			HTREEITEM parent = _treeView.getParent(hTreeItem);

			if (_treeView.getChildFrom(hTreeItem) != nullptr)
			{
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				int res = pNativeSpeaker->messageBox("ProjectPanelRemoveFolderFromProject",
					_hSelf,
					L"All the sub-items will be removed.\rAre you sure you want to remove this folder from the project?",
					L"Remove folder from project",
					MB_YESNO);
				if (res == IDYES)
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
			HTREEITEM parent = _treeView.getParent(hTreeItem);
			
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			int res = pNativeSpeaker->messageBox("ProjectPanelRemoveFileFromProject",
				_hSelf,
				L"Are you sure you want to remove this file from the project?",
				L"Remove file from project",
				MB_YESNO);
			if (res == IDYES)
			{
				_treeView.removeItem(hTreeItem);
				setWorkSpaceDirty(true);
				if (getNodeType(parent) == nodeType_folder)
					_treeView.setItemImage(parent, INDEX_CLOSED_NODE, INDEX_CLOSED_NODE);
			}
		}
		break;

		case IDM_PROJECT_MODIFYFILEPATH :
		{
			FileRelocalizerDlg fileRelocalizerDlg;
			fileRelocalizerDlg.init(_hInst, _hParent);

			wchar_t textBuffer[MAX_PATH] = { '\0' };
			TVITEM tvItem{};
			tvItem.hItem = hTreeItem;
			tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvItem.pszText = textBuffer;
			tvItem.cchTextMax = MAX_PATH;
			
			SendMessage(_treeView.getHSelf(), TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
			if (!tvItem.lParam)
				return;
			auto* const fn = reinterpret_cast<std::wstring*>(tvItem.lParam);

			if (fileRelocalizerDlg.doDialog(fn->c_str()) == 0)
			{
				std::wstring newValue = fileRelocalizerDlg.getFullFilePath();
				if (*fn == newValue)
					return;

				*fn = newValue;
				wchar_t *strValueLabel = ::PathFindFileName(fn->c_str());
				wcscpy_s(textBuffer, strValueLabel);
				int iImage = doesFileExist(fn->c_str()) ? INDEX_LEAF : INDEX_LEAF_INVALID;
				tvItem.iImage = tvItem.iSelectedImage = iImage;
				SendMessage(_treeView.getHSelf(), TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
				setWorkSpaceDirty(true);
			}
		}
		break;
	}
}

bool ProjectPanel::saveWorkSpaceAs(bool saveCopyAs)
{
	CustomFileDialog fDlg(_hSelf);
	setFileExtFilter(fDlg);
	fDlg.setExtIndex(0);		// 0 index for "custom extension" type if any else for "All types *.*"
	fDlg.setFolder(getWorkSpaceFilePath());

	const wstring fn = fDlg.doSaveDlg();
	if (fn.empty())
		return false;

	if (!writeWorkSpace(fn.c_str(), !saveCopyAs))
		return false;

	if (!saveCopyAs)
	{
		_workSpaceFilePath = fn;
		setWorkSpaceDirty(false);
	}
	return true;
}

void ProjectPanel::setFileExtFilter(CustomFileDialog & fDlg)
{
	const wchar_t *ext = NppParameters::getInstance().getNppGUI()._definedWorkspaceExt.c_str();
	if (*ext != '\0')
	{
		wstring workspaceExt = L"";

		if (*ext != '.')
			workspaceExt += L".";
		workspaceExt += ext;
		fDlg.setExtFilter(L"Workspace file", workspaceExt.c_str());
		fDlg.setDefExt(ext);
	}
	fDlg.setExtFilter(L"All types", L".*");
}

void ProjectPanel::addFiles(HTREEITEM hTreeItem)
{
	CustomFileDialog fDlg(_hSelf);
	fDlg.setExtFilter(L"All types", L".*");

	const auto& fns = fDlg.doOpenMultiFilesDlg();
	if (!fns.empty())
	{
		size_t sz = fns.size();
		for (size_t i = 0 ; i < sz ; ++i)
		{
			wchar_t *strValueLabel = ::PathFindFileName(fns.at(i).c_str());

			_fullPathStrs.push_back(std::make_unique<std::wstring>(fns.at(i)));
			auto lParamPathFileStr = reinterpret_cast<LPARAM>(_fullPathStrs.back().get());

			_treeView.addItem(strValueLabel, hTreeItem, INDEX_LEAF, lParamPathFileStr);
		}
		_treeView.expand(hTreeItem);
		setWorkSpaceDirty(true);
	}
}

void ProjectPanel::recursiveAddFilesFrom(const wchar_t *folderPath, HTREEITEM hTreeItem)
{
	wstring dirFilter(folderPath);
	if (folderPath[lstrlen(folderPath)-1] != '\\')
		dirFilter += L"\\";

	dirFilter += L"*.*";
	WIN32_FIND_DATA foundData;
	std::vector<wstring> files;

	HANDLE hFile = ::FindFirstFile(dirFilter.c_str(), &foundData);
	
	do {
		if (hFile == INVALID_HANDLE_VALUE)
			break;

		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) // Hidden directories are ignored
			{
				// do nothing
			}
			else // Always recursive
			{
				if ((wcscmp(foundData.cFileName, L".") != 0) && (wcscmp(foundData.cFileName, L"..") != 0))
				{
					wstring pathDir(folderPath);
					if (folderPath[lstrlen(folderPath)-1] != '\\')
						pathDir += L"\\";
					pathDir += foundData.cFileName;
					pathDir += L"\\";
					HTREEITEM addedItem = addFolder(hTreeItem, foundData.cFileName);
					recursiveAddFilesFrom(pathDir.c_str(), addedItem);
				}
			}
		}
		else
		{
			files.push_back(foundData.cFileName);
		}
	} while (::FindNextFile(hFile, &foundData));
	
	for (size_t i = 0, len = files.size() ; i < len ; ++i)
	{
		wstring pathFile(folderPath);
		if (folderPath[lstrlen(folderPath)-1] != '\\')
			pathFile += L"\\";
		pathFile += files[i];

		_fullPathStrs.push_back(std::make_unique<std::wstring>(pathFile));
		auto lParamPathFileStr = reinterpret_cast<LPARAM>(_fullPathStrs.back().get());
		_treeView.addItem(files[i].c_str(), hTreeItem, INDEX_LEAF, lParamPathFileStr);
	}

	::FindClose(hFile);
}

void ProjectPanel::addFilesFromDirectory(HTREEITEM hTreeItem)
{
	if (_selDirOfFilesFromDirDlg == L"" && _workSpaceFilePath != L"")
	{
		wchar_t dir[MAX_PATH] = { '\0' };
		wcscpy_s(dir, _workSpaceFilePath.c_str());
		::PathRemoveFileSpec(dir);
		_selDirOfFilesFromDirDlg = dir;
	}
	wstring dirPath;
	if (_selDirOfFilesFromDirDlg != L"")
		dirPath = getFolderName(_hSelf, _selDirOfFilesFromDirDlg.c_str());
	else
		dirPath = getFolderName(_hSelf);

	if (dirPath != L"")
	{
		recursiveAddFilesFrom(dirPath.c_str(), hTreeItem);
		_treeView.expand(hTreeItem);
		setWorkSpaceDirty(true);
		_selDirOfFilesFromDirDlg = dirPath;
	}
}

intptr_t CALLBACK FileRelocalizerDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			::SetDlgItemText(_hSelf, IDC_EDIT_FILEFULLPATHNAME, _fullFilePath.c_str());
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			goToCenter(SWP_SHOWWINDOW | SWP_NOSIZE);

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
				{
					wchar_t textBuf[MAX_PATH] = { '\0' };
					::GetDlgItemText(_hSelf, IDC_EDIT_FILEFULLPATHNAME, textBuf, MAX_PATH);
					_fullFilePath = textBuf;
					::EndDialog(_hSelf, 0);
				}
				return TRUE;

				case IDCANCEL :
					::EndDialog(_hSelf, -1);
				return TRUE;

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
	return FALSE;
}

int FileRelocalizerDlg::doDialog(const wchar_t *fn, bool isRTL)
{
	_fullFilePath = fn;

	return static_cast<int>(StaticDialog::myCreateDialogBoxIndirectParam(IDD_FILERELOCALIZER_DIALOG, isRTL));
}
