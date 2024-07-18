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


#pragma once

#include "DockingDlgInterface.h"
#include "TreeView.h"
#include "ProjectPanel_rc.h"

#define PM_PROJECTPANELTITLE       L"Project Panel"
#define PM_WORKSPACEROOTNAME       L"Workspace"
#define PM_NEWFOLDERNAME           L"Folder Name"
#define PM_NEWPROJECTNAME          L"Project Name"

#define PM_NEWWORKSPACE            L"New Workspace"
#define PM_OPENWORKSPACE           L"Open Workspace"
#define PM_RELOADWORKSPACE         L"Reload Workspace"
#define PM_SAVEWORKSPACE           L"Save"
#define PM_SAVEASWORKSPACE         L"Save As..."
#define PM_SAVEACOPYASWORKSPACE    L"Save a Copy As..."
#define PM_NEWPROJECTWORKSPACE     L"Add New Project"
#define PM_FINDINFILESWORKSPACE    L"Find in Projects..."

#define PM_EDITRENAME              L"Rename"
#define PM_EDITNEWFOLDER           L"Add Folder"
#define PM_EDITADDFILES            L"Add Files..."
#define PM_EDITADDFILESRECUSIVELY  L"Add Files from Directory..."
#define PM_EDITREMOVE              L"Remove\tDEL"
#define PM_EDITMODIFYFILE          L"Modify File Path"

#define PM_WORKSPACEMENUENTRY      L"Workspace"
#define PM_EDITMENUENTRY           L"Edit"

#define PM_MOVEUPENTRY             L"Move Up\tCtrl+Up"
#define PM_MOVEDOWNENTRY           L"Move Down\tCtrl+Down"

enum NodeType {
	nodeType_root = 0, nodeType_project = 1, nodeType_folder = 2, nodeType_file = 3
};

class TiXmlNode;
class CustomFileDialog;

class ProjectPanel : public DockingDlgInterface {
public:
	ProjectPanel(): DockingDlgInterface(IDD_PROJECTPANEL) {};
	~ProjectPanel();

	void init(HINSTANCE hInst, HWND hPere, int panelID) {
		DockingDlgInterface::init(hInst, hPere);
		_panelID = panelID;
	}

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};

	void setPanelTitle(std::wstring title) {
		_panelTitle = title;
	};
	const wchar_t * getPanelTitle() const {
		return _panelTitle.c_str();
	};

	void newWorkSpace();
	bool saveWorkspaceRequest();
	bool openWorkSpace(const wchar_t *projectFileName, bool force = false);
	bool saveWorkSpace();
	bool saveWorkSpaceAs(bool saveCopyAs);
	void setWorkSpaceFilePath(const wchar_t *projectFileName){
		_workSpaceFilePath = projectFileName;
	};
	const wchar_t * getWorkSpaceFilePath() const {
		return _workSpaceFilePath.c_str();
	};
	bool isDirty() const {
		return _isDirty;
	};
	bool checkIfNeedSave();

	void setBackgroundColor(COLORREF bgColour) override {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
	};
	void setForegroundColor(COLORREF fgColour) override {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
	};
	bool enumWorkSpaceFiles(HTREEITEM tvFrom, const std::vector<std::wstring> & patterns, std::vector<std::wstring> & fileNames);

protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;
	HWND _hToolbarMenu = nullptr;
	HMENU _hWorkSpaceMenu = nullptr;
	HMENU _hProjectMenu = nullptr;
	HMENU _hFolderMenu = nullptr;
	HMENU _hFileMenu = nullptr;
	std::wstring _panelTitle;
	std::wstring _workSpaceFilePath;
	std::wstring _selDirOfFilesFromDirDlg;
	bool _isDirty = false;
	int _panelID = 0;

	void initMenus();
	void destroyMenus();
	void addFiles(HTREEITEM hTreeItem);
	void addFilesFromDirectory(HTREEITEM hTreeItem);
	void recursiveAddFilesFrom(const wchar_t *folderPath, HTREEITEM hTreeItem);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const wchar_t *folderName);

	bool writeWorkSpace(const wchar_t *projectFileName = NULL, bool doUpdateGUI = true);
	std::wstring getRelativePath(const std::wstring & fn, const wchar_t *workSpaceFileName);
	void buildProjectXml(TiXmlNode *root, HTREEITEM hItem, const wchar_t* fn2write);
	NodeType getNodeType(HTREEITEM hItem);
	void setWorkSpaceDirty(bool isDirty);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplayPoint(int iButton);
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void showContextMenuFromMenuKey(HTREEITEM selectedItem, int x, int y);
	HMENU getMenuHandler(HTREEITEM selectedItem);
	std::wstring getAbsoluteFilePath(const wchar_t * relativePath);
	void openSelectFile();
	void setFileExtFilter(CustomFileDialog & fDlg);
	std::vector<std::wstring*> fullPathStrs;
};

class FileRelocalizerDlg : public StaticDialog
{
public :
	FileRelocalizerDlg() = default;

	int doDialog(const wchar_t *fn, bool isRTL = false);

	void destroy() override {};

	std::wstring getFullFilePath() {
		return _fullFilePath;
	};

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	std::wstring _fullFilePath;

};
